import test from 'node:test';
import assert from 'node:assert/strict';
import {request,decode,checksum,ServoClient,motionValues,FIRMWARE,PROFILES} from './protocol.mjs';
const reply=(id,mode='PAUSE')=>{const s=`${id},OK,${FIRMWARE},${mode},2,3,4`;return `!${s}*${checksum(s)}\n`;};
test('matches reference profiles and encodes signed angles/ranges',()=>{
  assert.deepEqual(PROFILES.type1,{service:0xffe0,write:0xffe1,notify:0xffe1});
  assert.equal(PROFILES.type2.write,0xffe2);
  assert.match(request(12,'M',-300,500),/^@12,M,-300,500\*[0-9A-F]{2}\n$/);
  assert.throws(()=>request(12,'M',0,500));assert.throws(()=>request(12,'C',2,500));
  assert.deepEqual(motionValues('30','45',-1),[-300,512]);
  assert.throws(()=>motionValues('NaN','45',1));assert.throws(()=>motionValues('721','45',1));
});
test('strict firmware/status/checksum parsing',()=>{
  assert.equal(decode(reply(1).trim()).commands,3);
  assert.throws(()=>decode(reply(1).replace('PAUSE','RUN')));
  const old='1,OK,SERVO AUTO R5,PAUSE,0,0,0';assert.throws(()=>decode(`!${old}*${checksum(old)}`));
});
test('fragmented notifications respect DataView byte offsets',async()=>{
  const sent=[];const client=new ServoClient(async bytes=>sent.push(new TextDecoder().decode(bytes)));
  const pending=client.command('V');await Promise.resolve();
  const line=new TextEncoder().encode(reply(client.id));
  const framed=new Uint8Array(line.length+8);framed.set(line,4);
  client.feed(new DataView(framed.buffer,4,5));client.feed(new DataView(framed.buffer,9,line.length-5));
  assert.equal((await pending).version,FIRMWARE);assert.equal(sent.length,1);client.close();
});
test('lost acknowledgement never resends relative motion',async()=>{
  let sends=0;const client=new ServoClient(async()=>sends++,{timeout:25});
  await assert.rejects(client.command('M',300,500),/未自动重发/);assert.equal(sends,1);client.close();
});
test('STOP cancels pending requests; stale replies cannot update state',async()=>{
  const states=[];const client=new ServoClient(async()=>{}, {onStatus:s=>states.push(s)});
  const movement=client.command('M',300,500);const movementRejected=assert.rejects(movement,/停止命令取消/);const old=client.id;
  const stopping=client.command('S');client.feed(reply(old,'MOVE'));assert.equal(states.length,0);
  client.feed(reply(client.id));await stopping;await movementRejected;assert.equal(states.length,1);client.close();
});
test('disconnect rejects pending request and releases its timeout',async()=>{
  const client=new ServoClient(async()=>{});const p=client.command('V');client.close();await assert.rejects(p,/断开/);assert.equal(client.busy,false);
});

test('heartbeats keep status current without evicting motion logs',async()=>{
  const logs=[];const client=new ServoClient(async()=>{}, {onLog:line=>logs.push(line)});
  const p=client.command('K');client.feed(reply(client.id));await p;
  assert.deepEqual(logs,[]);
  const q=client.command('C',-1,700);client.feed(reply(client.id,'RUN'));await q;
  assert.match(logs[0],/连续旋转 -1,700/);client.close();
});
