const test = require('node:test');
const assert = require('node:assert/strict');
const w = require('../wheel-log.js');
function line(i, us = 1000000 + i*10000) {
  return ['WLOG','DATA',i,Math.floor(us/1000),us>>>0,1,1,0,
    40,300,316,15000,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12000,500,1,1].join(',');
}
test('wheel wire schema and exact ms/us cadence reject bad flags, ranges and sampling', () => {
  assert.equal(line(0).split(',').length,30);
  const rows=[w.row(line(0)),w.row(line(1))];
  assert.equal(w.validate(rows).length,2);
  assert.match(w.csv(rows),/A_target_rpm,A_filtered_rpm_x10,A_raw_rpm_x10,A_pwm_mpercent/);
  for (const change of [{i:24,v:16},{i:25,v:2},{i:11,v:60001},{i:27,v:-1}]) {
    const parts=line(0).split(',');parts[change.i]=change.v;
    assert.throws(()=>w.row(parts.join(',')));
  }
  assert.throws(()=>w.row(line(0).replace(',40,',',NaN,')));
  assert.throws(()=>w.validate([rows[0],w.row(line(1,1009899))]));
  assert.throws(()=>w.validate([rows[0],w.row(line(1,1015001))]));
  const wrap=[w.row(line(0,4294967200)),w.row(line(1,4294977200))];
  assert.equal(w.validate(wrap).length,2);
});
test('wheel download validates stopped status, exact END, retries identical fragments, and rejects conflict', async () => {
  for (const fault of [null,'retry','conflict','end','overflow','active','gap','changed','moving']) {
    const frames=[];let attempts=0,statusReads=0;
    const send=async cmd=> {
      if(cmd==='ANGLESTATUS') return `ATEL,1,${fault==='moving'?1:0},1,0,0,0,0,0,0,0,0,0,12000`;
      if(cmd==='PS2STATUS') return 'PS2,1,0,0x73,0x0000,128,128,128,128,30,0,1';
      if(cmd==='WLOG,STATUS') { statusReads++;return `WLOG,STATUS,${fault==='active'?1:0},2,1024,${fault==='overflow'?1:0},${fault==='changed'&&statusReads>1?3:2}`; }
      attempts++;
      frames.push(fault==='conflict'&&attempts>1?line(0).replace(',40,',',41,'):line(0));
      if(!['gap','conflict'].includes(fault) && (fault!=='retry'||attempts>1)) frames.push(line(1));
      return fault==='end'?'WLOG,END,0,1,2':'WLOG,END,0,2,2';
    };
    if(fault&&fault!=='retry') await assert.rejects(w.read(send,frames),undefined,fault);
    else { assert.equal((await w.read(send,frames)).length,2);assert.equal(attempts,fault==='retry'?2:1); }
  }
});
