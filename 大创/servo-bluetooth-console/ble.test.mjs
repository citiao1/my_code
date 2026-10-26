import test from 'node:test';
import assert from 'node:assert/strict';
import {BleSession} from './ble.mjs';
import {checksum,FIRMWARE} from './protocol.mjs';

function fixture({wrongVersion=false,withoutOnly=false,alreadyPermitted=false}={}){
  let input='',connected=false,holdHeartbeat=false,mode='PAUSE';
  const sent=[],requests=[],writeModes=[];
  const notify=new EventTarget();notify.properties={write:!withoutOnly,writeWithoutResponse:true};
  notify.startNotifications=async()=>notify;
  function respond(line){
    const [id,op]=line.slice(1,line.indexOf('*')).split(',');
    if(op==='C')mode='RUN';else if(op==='M')mode='MOVE';else if(op==='A')mode='AUTO';else if(op==='S')mode='PAUSE';
    const body=`${id},OK,${wrongVersion?'OLD':FIRMWARE},${mode},0,1,0`;
    const bytes=new TextEncoder().encode(`!${body}*${checksum(body)}\n`);
    const wrapped=new Uint8Array(bytes.length+4);wrapped.set(bytes,2);
    notify.value=new DataView(wrapped.buffer,2,bytes.length);notify.dispatchEvent(new Event('characteristicvaluechanged'));
  }
  async function write(bytes){
    assert.ok(bytes.length<=20);input+=new TextDecoder().decode(bytes);
    let at;while((at=input.indexOf('\n'))>=0){const line=input.slice(0,at);input=input.slice(at+1);sent.push(line);if(line.includes(',K*')&&holdHeartbeat)requests.push(line);else queueMicrotask(()=>respond(line));}
  }
  notify.writeValueWithoutResponse=async bytes=>{writeModes.push('without');await write(bytes);};
  if(!withoutOnly)notify.writeValueWithResponse=async bytes=>{writeModes.push('with');await write(bytes);};
  const device=new EventTarget();device.name='WHEELTEC-IOS';device.id='fixture-servo';
  device.gatt={get connected(){return connected;},async connect(){connected=true;return{getPrimaryService:async uuid=>{assert.equal(uuid,0xffe0);return{getCharacteristic:async()=>notify};}};},disconnect(){connected=false;}};
  const bluetooth={requestDevice:async options=>{assert.equal(alreadyPermitted,false);assert.equal(options.filters[0].services[0],0xffe0);return device;}};
  if(alreadyPermitted)bluetooth.getDevices=async()=>[device];
  Object.defineProperty(globalThis,'navigator',{configurable:true,value:{bluetooth}});
  return{sent,device,writeModes,set hold(value){holdHeartbeat=value;},flush(){for(const line of requests.splice(0))respond(line);}};
}

test('BLE profile handshake/version, chunked command, feedback and stopped disconnect',async()=>{
  const fake=fixture();const states=[];const session=new BleSession({onStatus:s=>states.push(s)});
  try{
    assert.equal(await session.connect('type1'),'WHEELTEC-IOS');assert.equal(session.connected,true);
    await session.command('M',-7200,1500);assert.equal(states.at(-1).mode,'MOVE');
    await session.disconnect();assert.equal(session.connected,false);assert.ok(fake.sent.at(-1).includes(',S*'));
  }finally{session.release(false);}
});

test('acknowledged BLE writes are preferred; repeated wheel starts preserve RUN across heartbeats',async()=>{
  const fake=fixture();const logs=[];const session=new BleSession({onLog:line=>logs.push(line)});
  try{
    await session.connect('type1');
    for(const direction of [1,1,-1,-1]){
      await session.command('C',direction,341);await session.heartbeat();assert.equal(session.mode,'RUN');
      await session.command('S');assert.equal(session.mode,'PAUSE');
    }
    assert.ok(fake.writeModes.length>0 && fake.writeModes.every(mode=>mode==='with'));
    assert.equal(fake.sent.filter(line=>line.includes(',C,')).length,4);
    assert.ok(!logs.some(line=>line==='发送 K'));
  }finally{session.release(false);}
});

test('modules that only support unacknowledged writes still connect',async()=>{
  const fake=fixture({withoutOnly:true});const session=new BleSession();
  try{await session.connect('type1');assert.ok(fake.writeModes.every(mode=>mode==='without'));}
  finally{session.release(false);}
});

test('a single previously authorized WHEELTEC can reconnect after a page upgrade',async()=>{
  fixture({alreadyPermitted:true});const session=new BleSession();
  try{await session.connect('type1');assert.equal(session.connected,true);}
  finally{session.release(false);}
});
test('wrong firmware never opens movement controls and closes GATT',async()=>{
  const fake=fixture({wrongVersion:true});const session=new BleSession();
  await assert.rejects(session.connect('type1'),/版本/);assert.equal(session.connected,false);assert.equal(fake.device.gatt.connected,false);
});
test('manual motion waits for in-flight heartbeat rather than being swallowed',async()=>{
  const fake=fixture();const session=new BleSession();
  try{
    await session.connect('type1');fake.hold=true;
    const heartbeat=session.heartbeat();await new Promise(resolve=>setImmediate(resolve));
    const motion=session.command('M',300,500);fake.flush();await heartbeat;await motion;
    assert.equal(fake.sent.filter(line=>line.includes(',M,')).length,1);
  }finally{session.release(false);}
});
test('STOP aborts heartbeat without unintentionally disconnecting',async()=>{
  const fake=fixture();const session=new BleSession();
  try{
    await session.connect('type1');fake.hold=true;
    const heartbeat=session.heartbeat();await new Promise(resolve=>setImmediate(resolve));
    await session.command('S');await heartbeat;assert.equal(session.connected,true);fake.flush();
  }finally{session.release(false);}
});
