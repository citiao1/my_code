const test=require('node:test');
const assert=require('node:assert/strict');
const Serial=require('../serial-transport.js');
const motionFault=require('./fixtures/r25-motion-fault.json');
const tick=()=>new Promise(setImmediate);

function fixture(options={}) {
  let controller;
  const sent=[];
  const state={reply:true};
  const frames=[];
  const port={
    getInfo:()=>({usbVendorId:0x1a86,usbProductId:0x7523}),
    readable:new ReadableStream({start(c){controller=c;}}),
    writable:new WritableStream({write(data){
      const command=new TextDecoder().decode(data).trim();sent.push(command);
      state.onWrite?.(command);
      if(state.reply)queueMicrotask(()=>feed((state.replies?.[command]||Serial.expected(command))+'\r\n'));
    }}),
    async open(config){this.config=config;},
    async close(){assert.equal(this.readable.locked,false);assert.equal(this.writable.locked,false);this.closed=true;},
  };
  function feed(text){controller.enqueue(new TextEncoder().encode(text));}
  const transport=new Serial({onLine:line=>frames.push(line),...options});
  return {transport,port,sent,state,frames,feed};
}

test('direct startup verifies the MCU and closing confirms STOP/DISARM without ARM',async()=>{
  const f=fixture();await f.transport.open(f.port);
  assert.deepEqual(f.sent,['STOP','DISARM','TEL,0','TOF,0','PING']);
  assert.equal(f.transport.verified,true);
  assert.equal(f.port.config.baudRate,115200);assert.equal(f.port.config.flowControl,'none');
  await f.transport.close();
  assert.deepEqual(f.sent.slice(-2),['STOP','DISARM']);assert.equal(f.port.closed,true);
  assert.equal(f.transport.isOpen(),false);
});

test('STOP preempts unanswered MOVE and rejects older queued targets without replay',async()=>{
  const f=fixture();await f.transport.open(f.port);f.sent.length=0;f.state.reply=false;
  const move=f.transport.command('MOVE,30,0,0');const rejected=assert.rejects(move,/cancelled/);
  await tick();
  const queued=f.transport.command('MOVE,40,0,0');const rejectedQueued=assert.rejects(queued,/cancelled/);
  f.state.reply=true;
  const stop=f.transport.command('STOP');const duplicate=f.transport.command('STOP');
  assert.equal(stop,duplicate);assert.equal(await stop,'ACK,STOP');
  await rejected;await rejectedQueued;
  assert.deepEqual(f.sent,['MOVE,30,0,0','STOP']);
  await f.transport.close(false);
});

test('stale queued motion is rejected before writing',async()=>{
  let now=0;const f=fixture({clock:()=>now});await f.transport.open(f.port);f.sent.length=0;f.state.reply=false;
  const ping=f.transport.command('PING');await tick();
  const move=f.transport.command('MOVE,30,0,0');const rejected=assert.rejects(move,/过期/);
  now=251;f.feed('PONG\r\n');await ping;await rejected;
  assert.deepEqual(f.sent,['PING']);await f.transport.close(false);
});

test('only the latest unsent motion target survives',async()=>{
  const f=fixture();await f.transport.open(f.port);f.sent.length=0;f.state.reply=false;
  const ping=f.transport.command('PING');await tick();
  const first=f.transport.command('MOVE,10,0,0');const cancelled=assert.rejects(first,/cancelled/);
  const last=f.transport.command('MOVE,20,0,0');
  f.state.reply=true;f.feed('PONG\r\n');await ping;await last;await cancelled;
  assert.deepEqual(f.sent,['PING','MOVE,20,0,0']);await f.transport.close(false);
});

test('fragmented logs reach the page in DATA/END/result order and require exact END offset',async()=>{
  const order=[];const f=fixture({onLine:line=>order.push(line)});await f.transport.open(f.port);
  order.length=0;f.state.reply=false;
  const result=f.transport.command('WLOG,READ,8,2').then(reply=>{order.push('RESULT');return reply;});
  await tick();f.feed('WLOG,DA');f.feed('TA,8,raw\r\nWLOG,END,0,2,10\r\n');await tick();
  assert.equal(f.transport.pending.expected,'WLOG,END,8');
  f.feed('WLOG,DATA,9,raw\r\nWLOG,END,8,2,10\r\n');
  assert.equal(await result,'WLOG,END,8,2,10');
  assert.deepEqual(order,['WLOG,DATA,8,raw','WLOG,END,0,2,10','WLOG,DATA,9,raw','WLOG,END,8,2,10','RESULT']);
  await f.transport.close(false);
});

test('bad bytes and malformed ASCII prefixes cannot acknowledge KEEP; raw evidence survives',async()=>{
  const warnings=[];const f=fixture({onWarning:m=>warnings.push(m)});await f.transport.open(f.port);f.state.reply=false;
  const keep=f.transport.command('KEEP');await tick();
  f.feed('KQQATEL,100,1,1\r\n');f.transport.consume(Uint8Array.from([65,67,75,0xff,10]));
  assert.ok(f.transport.pending);f.feed('ACK,KEEP\r\n');assert.equal(await keep,'ACK,KEEP');
  assert.equal(f.transport.stats.badBytes,1);assert.equal(warnings.length,2);
  assert.ok(f.transport.diagnostic().trace.some(x=>x.event==='rx'&&x.detail.includes('ff')));
  await f.transport.close(false);
});

test('remote silence fails closed, cancels queued motion and never retries',async()=>{
  let offline=0;const f=fixture({onDisconnect:()=>offline++});await f.transport.open(f.port);f.sent.length=0;f.state.reply=false;
  const ping=f.transport.command('PING',20);const rejected=assert.rejects(ping,/应答超时/);
  await tick();const move=f.transport.command('MOVE,30,0,0');const cancelled=assert.rejects(move,/应答超时/);
  await rejected;await cancelled;await f.transport.close(false);
  assert.deepEqual(f.sent,['PING']);assert.equal(offline,1);assert.equal(f.transport.verified,false);
  assert.equal(f.transport.stats.timeouts,1);assert.equal(f.port.closed,true);
});

test('one missing KEEP ACK releases the queue before the MCU watchdog when active telemetry continues',async()=>{
  const f=fixture();await f.transport.open(f.port);f.sent.length=0;f.state.reply=false;
  const keep=f.transport.command('KEEP',40);
  const missed=assert.rejects(keep,error=>error.heartbeatAckMissing===true);
  await tick();f.feed('ATEL,100,1,1,0,0,0,0,0,0,0,0,1,12000\r\n');
  await missed;
  assert.equal(f.transport.isOpen(),true);assert.equal(f.transport.verified,true);
  f.state.reply=true;assert.equal(await f.transport.command('KEEP'),'ACK,KEEP');
  assert.deepEqual(f.sent,['KEEP','KEEP']);
  assert.equal(f.transport.stats.keepAckMisses,1);
  assert.equal(f.transport.diagnostic().heartbeatAckMisses,0);
  await f.transport.close(false);
});

test('two consecutive missing KEEP ACKs fail closed even with active telemetry',async()=>{
  const f=fixture();await f.transport.open(f.port);f.sent.length=0;f.state.reply=false;
  const first=f.transport.command('KEEP',40);
  const soft=assert.rejects(first,error=>error.heartbeatAckMissing===true);
  await tick();f.feed('ATEL,100,1,1,0,0,0,0,0,0,0,0,1,12000\r\n');await soft;
  const second=f.transport.command('KEEP',40);
  const hard=assert.rejects(second,error=>!error.heartbeatAckMissing);
  await tick();f.feed('ATEL,120,1,1,0,0,0,0,0,0,0,0,1,12000\r\n');await hard;
  await f.transport.close(false);assert.equal(f.port.closed,true);
  assert.deepEqual(f.sent,['KEEP','KEEP']);
});

test('KEEP uses a 600ms deadline and stopped telemetry never permits missing-ACK grace',async()=>{
  const f=fixture();await f.transport.open(f.port);f.state.reply=false;
  const keep=f.transport.command('KEEP');const stopped=assert.rejects(keep,/STOP/);
  assert.equal(f.transport.pending.entry.timeout,600);
  await tick();
  // Explicit STOP cancels this wait; a fresh session below tests stopped state.
  f.state.reply=true;await f.transport.command('STOP');await stopped;await f.transport.close(false);
  const g=fixture();await g.transport.open(g.port);g.state.reply=false;
  const next=g.transport.command('KEEP',40);const failed=assert.rejects(next,error=>!error.heartbeatAckMissing);
  await tick();g.feed('ATEL,100,0,1,0,0,0,0,0,0,0,0,0,12000\r\n');await failed;
  await g.transport.close(false);assert.equal(g.port.closed,true);
});

test('a dropped KEEP request allows the next 400ms heartbeat slot before a 1500ms MCU watchdog',async t=>{
  t.mock.timers.enable({apis:['setTimeout','setInterval']});
  let now=0,lastMcuRefresh=-400;
  const f=fixture({clock:()=>now});await f.transport.open(f.port);f.sent.length=0;f.state.reply=false;
  const first=f.transport.command('KEEP');const soft=assert.rejects(first,e=>e.heartbeatAckMissing);
  await tick(); // Model loss of the first request: MCU last refreshed at -400ms.
  now=500;t.mock.timers.tick(500);
  f.feed('ATEL,500,1,1,0,0,0,0,0,0,0,0,1,12000\r\n');await tick();
  now=600;t.mock.timers.tick(100);await soft;
  now=800;t.mock.timers.tick(200);assert.ok(now-lastMcuRefresh<1500);
  f.state.reply=true;f.state.onWrite=command=>{if(command==='KEEP')lastMcuRefresh=now;};
  await f.transport.command('KEEP');assert.equal(lastMcuRefresh,800);
  assert.deepEqual(f.sent,['KEEP','KEEP']);await f.transport.close(false);
});

test('a stalled write cannot use heartbeat grace just because telemetry is arriving',async()=>{
  const f=fixture();await f.transport.open(f.port);f.state.reply=false;
  await f.transport.writer.abort();f.transport.writer.releaseLock();
  let finish;
  f.port.writable=new WritableStream({write:()=>new Promise(resolve=>{finish=resolve;})});
  f.transport.writer=f.port.writable.getWriter();
  const keep=f.transport.command('KEEP',40);const failed=assert.rejects(keep,e=>!e.heartbeatAckMissing);
  await tick();f.feed('ATEL,100,1,1,0,0,0,0,0,0,0,0,1,12000\r\n');await failed;
  finish();await f.transport.close(false);assert.equal(f.port.closed,true);
});

test('opening an occupied port does not retry or claim online',async()=>{
  const f=fixture();let attempts=0;f.port.open=async()=>{attempts++;throw new Error('port busy');};
  await assert.rejects(f.transport.open(f.port),/port busy/);
  assert.equal(attempts,1);assert.equal(f.transport.verified,false);assert.deepEqual(f.sent,[]);
});

test('lost STOP ACK with stopped telemetry revalidates DISARM without closing or replaying motion',async()=>{
  let recovering=0,recovered=0,offline=0;
  const f=fixture({onRecovering:()=>recovering++,onRecovered:()=>recovered++,onDisconnect:()=>offline++});
  await f.transport.open(f.port);f.sent.length=0;f.state.reply=false;
  const stop=f.transport.command('STOP',40);const missing=assert.rejects(stop,e=>e.stopAckMissing===true);
  await tick();
  const move=f.transport.command('MOVE,30,0,0');const cancelled=assert.rejects(move,/cancelled/);
  f.feed('STOP\nATEL,100,0,1,0,0,0,0,0,0,0,0,0,12000\r\n');
  f.state.replies={ANGLESTATUS:'ATEL,140,0,1,0,0,0,0,0,0,0,0,0,12000'};f.state.reply=true;
  await missing;await f.transport.recoveryTask;await cancelled;
  assert.deepEqual(f.sent,['STOP','DISARM','ANGLESTATUS','PING']);
  assert.equal(recovering,1);assert.equal(recovered,1);assert.equal(offline,0);
  assert.equal(f.transport.verified,true);assert.equal(f.port.closed,undefined);
  assert.equal(f.transport.stats.stopRecoveries,1);
  await f.transport.close(false);
});

test('STOP recovery rejects new motion and fails closed on an active state readback',async()=>{
  let recovered=0,blocked;
  const f=fixture({onRecovering:()=>{blocked=assert.rejects(f.transport.command('MOVE,30,0,0'),/DISARM/);},onRecovered:()=>recovered++});
  await f.transport.open(f.port);f.sent.length=0;f.state.reply=false;
  const stop=f.transport.command('STOP',40);const missing=assert.rejects(stop,e=>e.stopAckMissing===true);
  await tick();f.feed('ATEL,100,0,1,0,0,0,0,0,0,0,0,0,12000\r\n');
  f.state.replies={ANGLESTATUS:'ATEL,140,1,1,0,0,0,0,0,0,0,0,1,12000'};f.state.reply=true;
  await missing;await f.transport.recoveryTask;await blocked;await f.transport.close(false);
  assert.deepEqual(f.sent,['STOP','DISARM','ANGLESTATUS']);
  assert.equal(recovered,0);assert.equal(f.transport.verified,false);assert.equal(f.port.closed,true);
});

test('bare STOP or ongoing TOF alone cannot authorize STOP recovery',async()=>{
  const f=fixture();await f.transport.open(f.port);f.sent.length=0;f.state.reply=false;
  // Attempts to stop are safe; TOF/bare STOP cannot prove that they succeeded.
  f.transport.motionReport=null;
  const stop=f.transport.command('STOP',40);const failed=assert.rejects(stop,e=>e.stopAckMissing);
  await tick();f.feed('STOP\nTOF,100,1,1,3300,9,5,0\r\n');await failed;
  await f.transport.recoveryTask;await f.transport.close(false);
  assert.deepEqual(f.sent,['STOP','DISARM','DISARM']);assert.equal(f.transport.stats.stopRecoveries,0);assert.equal(f.port.closed,true);
});

test('missing DISARM ACK during STOP recovery retries parking once then closes without motion replay',async()=>{
  const f=fixture();await f.transport.open(f.port);f.sent.length=0;f.state.reply=false;
  const stop=f.transport.command('STOP',40);const missing=assert.rejects(stop,e=>e.stopAckMissing===true);
  await tick();f.feed('ATEL,100,0,1,0,0,0,0,0,0,0,0,0,12000\r\n');
  await missing;await f.transport.recoveryTask;await f.transport.close(false);
  assert.deepEqual(f.sent,['STOP','DISARM','DISARM']);assert.equal(f.transport.stats.stopRecoveries,0);assert.equal(f.port.closed,true);
});

test('write failure takes the connection offline rather than advancing queued commands',async()=>{
  const f=fixture();await f.transport.open(f.port);f.state.reply=false;
  await f.transport.writer.abort();f.sent.length=0;
  await assert.rejects(f.transport.command('PING'));
  await f.transport.close(false);assert.ok(f.transport.fault);assert.equal(f.port.closed,true);
});

test('diagnostic observer failures cannot break startup or command processing',async()=>{
  const f=fixture();f.transport.onEvent=()=>{throw new Error('storage failed');};
  await f.transport.open(f.port);assert.equal(f.transport.verified,true);
  assert.equal(await f.transport.command('PING'),'PONG');await f.transport.close();
});

test('native USB removal records the observed cause before fault and unregisters on close',async()=>{
  const f=fixture(),events=[];let disconnect,removed=false;
  f.port.addEventListener=(name,fn)=>{assert.equal(name,'disconnect');disconnect=fn;};
  f.port.removeEventListener=(name,fn)=>{removed=fn===disconnect;};
  f.transport.onEvent=(event,detail)=>events.push({event,detail});
  await f.transport.open(f.port);disconnect();await f.transport.close(false);
  assert.deepEqual(events.slice(-2).map(x=>x.event),['native_disconnect','fault']);
  assert.match(events.at(-1).detail.message,/移除/);assert.equal(removed,true);
});

test('actual 60-byte corruption and bare DKEEP trigger verified parking without port closure or motion replay',async()=>{
  let recovered=0,offline=0,reason;
  const f=fixture({onRecovered:()=>recovered++,onDisconnect:()=>offline++,onRecovering:trigger=>{reason=trigger;}});
  await f.transport.open(f.port);f.sent.length=0;
  f.state.replies={ANGLESTATUS:'ATEL,100,0,1,0,0,0,0,0,0,0,0,0,12000'};
  f.state.onWrite=command=>{f.state.reply=command!=='DKEEP';if(command==='DISARM')f.feed('ACK,DKEEP\r\n');};
  for(const hex of motionFault.corruptChunks)f.transport.consume(Uint8Array.from(Buffer.from(hex,'hex')));
  const keep=f.transport.command('DKEEP',40);const missing=assert.rejects(keep,e=>e.directHeartbeatFailed);
  await tick();for(const hex of motionFault.residualChunks)f.transport.consume(Uint8Array.from(Buffer.from(hex,'hex')));
  const stale=f.transport.command('DRV,30,0,0');const cancelled=assert.rejects(stale,/cancelled/);
  await missing;await f.transport.recoveryTask;await cancelled;
  assert.equal(f.transport.stats.badBytes,60);assert.equal(reason,'DKEEP');
  assert.equal(recovered,1);assert.equal(offline,0);assert.equal(f.transport.isOpen(),true);
  assert.equal(f.port.closed,undefined);assert.equal(f.transport.verified,true);
  assert.deepEqual(f.sent,['DKEEP','DISARM','ANGLESTATUS','PING']);
  await f.transport.close(false);
});

test('DKEEP silence records no RX during wait and rejects new motion until fresh stop proof arrives',async()=>{
  const f=fixture();await f.transport.open(f.port);f.sent.length=0;f.state.reply=false;
  const keep=f.transport.command('DKEEP',40),missing=assert.rejects(keep,e=>e.directHeartbeatFailed);
  await missing;await tick();assert.equal(f.transport.recovering,true);
  await assert.rejects(f.transport.command('DRV,30,0,0'),/验证/);
  const timeout=f.transport.trace.find(x=>x.event==='command_timeout');assert.equal(timeout.detail.rxDuringWait,false);
  f.state.reply=true;f.state.replies={ANGLESTATUS:'ATEL,100,0,1,0,0,0,0,0,0,0,0,0,12000'};
  f.feed('ACK,DISARM\r\n');await f.transport.recoveryTask;
  assert.deepEqual(f.sent,['DKEEP','DISARM','ANGLESTATUS','PING']);assert.equal(f.transport.verified,true);
  await f.transport.close(false);
});

test('one missing DISARM ACK retries only DISARM; no ARM, DRV or DKEEP is automatically retried',async()=>{
  const f=fixture();await f.transport.open(f.port);f.sent.length=0;let attempts=0;
  f.state.replies={ANGLESTATUS:'ATEL,100,0,1,0,0,0,0,0,0,0,0,0,12000'};
  f.state.onWrite=command=>{f.state.reply=command==='DKEEP'?false:command==='DISARM'?++attempts>1:true;};
  await assert.rejects(f.transport.command('DKEEP',40),e=>e.directHeartbeatFailed);
  await f.transport.recoveryTask;
  assert.equal(f.transport.verified,true);assert.deepEqual(f.sent,['DKEEP','DISARM','DISARM','ANGLESTATUS','PING']);
  assert.ok(f.transport.trace.some(x=>x.event==='safe_stop_retry'));await f.transport.close(false);
});

test('DKEEP recovery fails closed if DISARM is unconfirmed twice or stopped state is false',async()=>{
  for(const mode of ['silence','active']) {
    const f=fixture();await f.transport.open(f.port);f.sent.length=0;
    f.state.replies={ANGLESTATUS:'ATEL,100,1,1,0,0,0,0,0,0,0,0,1,12000'};
    f.state.onWrite=command=>{f.state.reply=mode==='active'&&command!=='DKEEP';};
    await assert.rejects(f.transport.command('DKEEP',40),e=>e.directHeartbeatFailed);
    await f.transport.recoveryTask;await f.transport.close(false);
    assert.equal(f.transport.verified,false);assert.ok(f.transport.fault);assert.equal(f.port.closed,true);
    assert.deepEqual(f.sent,mode==='silence'?['DKEEP','DISARM','DISARM']:['DKEEP','DISARM','ANGLESTATUS']);
  }
});

test('an incomplete DKEEP write cannot enter soft recovery; valid SPEEDLIMIT is not a corruption event',async()=>{
  const f=fixture(),events=[];await f.transport.open(f.port);f.sent.length=0;
  f.transport.onEvent=e=>events.push(e);f.feed('SPEEDLIMIT,160\r\n');await tick();
  assert.ok(!events.includes('rx_unknown_line'));
  await f.transport.writer.abort();f.transport.writer.releaseLock();let finish;
  f.port.writable=new WritableStream({write:()=>new Promise(resolve=>{finish=resolve;})});
  f.transport.writer=f.port.writable.getWriter();
  await assert.rejects(f.transport.command('DKEEP',40),e=>!e.directHeartbeatFailed);
  assert.ok(f.transport.fault);assert.ok(!events.includes('safe_recovery_start'));
  finish();await f.transport.close(false);
});

test('TEL snapshot records current raw speed state separately from the last angular/ARM report',async()=>{
  const f=fixture();await f.transport.open(f.port);
  f.feed('TEL,100,0,1,30,30,30,30,1,2,3,4,110,220,330,440,30,30,30,30,12000,0,0,1,0,0,1\r\n');
  await tick();assert.equal(f.transport.speedReport.enabled,true);
  assert.deepEqual(f.transport.speedReport.targets,[30,30,30,30]);
  assert.deepEqual(f.transport.speedReport.actualX10,[110,220,330,440]);
  await f.transport.close(false);
});

test('lost STOP ACK with stale active MOVE state can recover only after fresh DISARM/ATEL/PING proof',async()=>{
  const f=fixture();await f.transport.open(f.port);f.sent.length=0;
  f.transport.motionReport={receivedAt:0,mode:1,enabled:true,source:'ACK,MOVE'};
  f.state.replies={ANGLESTATUS:'ATEL,500,0,1,0,0,0,0,0,0,0,0,0,12000'};
  f.state.onWrite=command=>{f.state.reply=command!=='STOP';};
  await assert.rejects(f.transport.command('STOP',40),e=>e.stopAckMissing);
  await f.transport.recoveryTask;
  assert.equal(f.transport.verified,true);assert.equal(f.transport.motionReport.mode,0);
  assert.deepEqual(f.sent,['STOP','DISARM','ANGLESTATUS','PING']);assert.equal(f.port.closed,undefined);
  await f.transport.close(false);
});
