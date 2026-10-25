const test=require('node:test');
const assert=require('node:assert/strict');
const Journal=require('../link-journal.js');
function fixture(){let time=1000;const data=new Map();const storage={getItem:k=>data.get(k),setItem:(k,v)=>data.set(k,v)};
return {storage,data,now:()=>time,advance:n=>time+=n};}
test('two failed sessions survive reconnect and reload with distinct causes and evidence',()=>{
 const f=fixture(),j=new Journal(f.storage,f.now);
 j.start('direct_web_serial');j.event('command_timeout',{command:'KEEP',writeDone:true,rxAgeMs:900},{trace:[{event:'rx',detail:'00ff'}]});
 j.event('fault',{message:'KEEP timeout'});f.advance(2000);
 j.start('direct_web_serial');j.event('native_disconnect',{usbVendorId:6790});j.event('fault',{message:'removed'});
 const reload=new Journal(f.storage,f.now).snapshot();
 assert.equal(reload.incidents.length,2);assert.equal(reload.incidents[0].events[0].detail.command,'KEEP');
 assert.equal(reload.incidents[0].evidence.trace[0].detail,'00ff');assert.equal(reload.incidents[1].events[0].event,'native_disconnect');
});
test('safe STOP revalidation has an outcome separate from a port disconnect',()=>{
 const f=fixture(),j=new Journal(f.storage,f.now);j.start('direct');
 for(const e of ['command_timeout','stop_ack_missing','safe_recovery_start','safe_recovery_verified'])j.event(e,{});
 assert.equal(j.incidents.length,1);assert.equal(j.incidents[0].outcome,'revalidated');
 f.advance(100);j.event('command_timeout',{command:'PING'});j.event('fault',{});
 assert.equal(j.incidents.length,2);assert.equal(j.incidents[1].outcome,'disconnected');
});
test('healthy traffic never writes storage; retention and raw evidence are bounded',()=>{
 const f=fixture(),j=new Journal(f.storage,f.now);j.start('direct');
 for(let i=0;i<1000;i++)j.event('rx',{data:i});assert.equal(f.data.size,0);
 for(let i=0;i<80;i++){j.start('direct');j.event('fault',{message:'x'},{trace:Array.from({length:600},()=>({detail:'a'.repeat(700)}))});}
 assert.equal(j.incidents.length,40);
 assert.ok(j.incidents.filter(x=>x.evidence).length<=3);
 assert.ok([...f.data.values()][0].length<=220000);
});
test('storage quota failure leaves incident export available',()=>{
 const j=new Journal({getItem:()=>'{bad',setItem:()=>{throw new Error('quota');}});
 j.start('direct');j.event('open_failed',{name:'NetworkError'});
 assert.equal(j.snapshot().incidents.length,1);assert.match(j.snapshot().storageError,/未能保存/);
});
