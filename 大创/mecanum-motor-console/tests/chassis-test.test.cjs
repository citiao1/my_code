const test = require('node:test');
const assert = require('node:assert/strict');
const k = require('../chassis-test.js');

function fixture(options = {}) {
  let now = 10000, tick = 1000, yaw = options.yaw ?? 170, rate = 0;
  let heading = yaw, forward = 0, left = 0, mode = 0, enabled = 1;
  let fieldForward = 0, fieldLeft = 0;
  let cancelled = false, frozenTick = null;
  let counts = [0, 0, 0, 0];
  const commands = [], rows = [];
  function advance(ms) {
    const dt = ms / 1000;
    now += ms; tick = (tick + ms) >>> 0;
    rate = mode ? Math.max(-70, Math.min(70, k.wrap(heading - yaw) * 6)) : 0;
    const radians = yaw * Math.PI / 180;
    forward = fieldForward * Math.cos(radians) + fieldLeft * Math.sin(radians);
    left = -fieldForward * Math.sin(radians) + fieldLeft * Math.cos(radians);
    // Observable wheel-derived lateral bias, not unobservable ground slip.
    if (Math.hypot(forward, left) > 1) left += options.lateralBias ?? 0;
    const rotation = rate * 0.4;
    const f = options.frontReversed ? -forward : forward;
    const l = options.frontReversed ? -left : left;
    const wheels = [f + l + rotation, f - l - rotation,
      f + l - rotation, f - l + rotation];
    if (!options.blocked) counts = counts.map((v, i) => v + wheels[i] * 380 * dt / 60);
    yaw = k.wrap(yaw + rate * dt);
  }
  const port = {
    now: () => now,
    sleep: async (ms) => advance(ms),
    check: () => { if (cancelled) throw new Error('cancelled'); },
    record: (row) => {
      rows.push(row);
      if (options.cancelAfter && rows.length >= options.cancelAfter) cancelled = true;
    },
    progress: () => {},
    send: async (command) => {
      commands.push(command);
      advance(40);
      if (options.failCommand === command) throw new Error('BLE disconnected');
      const p = command.split(',');
      if (p[0] === 'TRACKMOVE') {
        heading = Number(p[1]); fieldForward = Number(p[2]); fieldLeft = Number(p[3]); mode = 2;
      }
      if (command === 'STOP') { fieldForward = fieldLeft = forward = left = 0; mode = enabled = 0; }
      if (command === 'PS2STATUS') return 'PS2,1,0,0x73,0x0000,128,128,128,128,30,0,18';
      if (command === 'ENC') {
        if (options.freeze && rows.length > 3) frozenTick ??= tick;
        return ['ENC', frozenTick ?? tick, 1, 380, ...counts.map(Math.round),
          0, 0, 0, 0, ...[forward + left, forward - left, forward + left, forward - left]
            .map((v) => options.noisyRpm ? 158 : Math.round(v * 10))].join(',');
      }
      if (command === 'ANGLESTATUS') return ['ATEL', frozenTick ?? tick, mode, 1,
        0, Math.round(rate * 1000), 0, Math.round(heading * 1000),
        Math.round(yaw * 1000), 0, 0, 0, enabled, 12000].join(',');
      return 'ACK,' + p[0];
    },
  };
  return { port, commands, rows };
}
const config = { kind: 'translation', rpm: 30, distance: 0.3, angle: 30,
  headingOffset: 0, confirmed: true };

test('R20 reversed front uses physical encoders for translation, strafe and wrap', async () => {
  assert.equal(k.bodyDelta([-380,-380,-380,-380],380,true).forward,Math.PI*.127);
  assert.equal(k.bodyDelta([-380,380,-380,380],380,true).left,Math.PI*.127);
  for(const angle of [0,45,90,180,-90]) {
    const f=fixture({frontReversed:true,yaw:175});
    const result=await k.run(f.port,{...config,angle,frontReversed:true});
    assert.ok(result.residualM<.05,`${angle}: ${result.residualM}`);
  }
});

test('signed RF/LF/LR/RR inverse cancels pure rotation, not strafe', () => {
  const scale = Math.PI * 0.127;
  assert.deepEqual(k.bodyDelta([380, -380, -380, 380], 380), { forward: 0, left: 0 });
  assert.equal(k.bodyDelta([380, -380, 380, -380], 380).left, scale);
  assert.equal(k.bodyDelta([380, 380, 380, 380], 380).forward, scale);
  assert.deepEqual(k.bodyVector(90, 30), {forward: 0, left: 30});
  const v = k.bodyVector(17, 60);
  assert.ok(v.forward > v.left && v.left > 0);
});

test('feedback parsers reject incomplete, NaN, disabled and invalid CPR', () => {
  assert.throws(() => k.attitude('ATEL,1'), /格式/);
  assert.throws(() => k.attitude('ATEL,1,2,0,0,0,0,0,0,0,0,0,1,12000'), /IMU/);
  assert.throws(() => k.encoders('ENC,1,1,NaN,0,0,0,0,0,0,0,0,0,0,0,0'), /格式/);
  assert.throws(() => k.encoders('ENC,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0'), /CPR/);
});

for (const angle of [0, 17, 30, 45, 90, -90, 135, -150, 180]) {
  test(`measured ${angle} degree translation goes out and back holding yaw`, async () => {
    const f = fixture();
    const result = await k.run(f.port, {...config, angle});
    assert.ok(result.samples > 10);
    assert.ok(result.residualM < 0.045, `residual ${result.residualM}`);
    const outward = f.rows.filter((r) => r.phase === '去程');
    const projection = outward.map((r) => r.x_m * Math.cos(angle * Math.PI / 180) + r.y_m * Math.sin(angle * Math.PI / 180));
    assert.ok(Math.max(...projection) > 0.28);
    assert.equal(f.commands.filter((c) => c === 'STOP').length, 1);
    assert.equal(f.commands.at(-1), 'STOP');
    assert.ok(!f.commands.some((c) => c === 'ARM' || c.startsWith('PID,')));
    assert.ok(!f.commands.includes('ATEL,1'));
  });
}

test('travel direction and independently offset chassis yaw are not confused', async () => {
  const f = fixture({yaw: 175});
  const result = await k.run(f.port, {...config, angle: 30, headingOffset: 25});
  const moves = f.rows.filter((r) => r.phase === '去程');
  assert.ok(moves.some((r) => r.forward_rpm > r.left_rpm && r.left_rpm > 0));
  // Targets are now in the initial test frame, not an assumed body heading.
  assert.ok(result.residualM < 0.05);
});

test('square has four absolute 90 degree turns with no inter-leg STOP', async () => {
  const f = fixture();
  const result = await k.run(f.port, {...config, kind: 'square'});
  for (const target of [-100, -10, 80, 170])
    assert.ok(f.commands.includes(`TRACKMOVE,${target.toFixed(2)},0.00,0.00`));
  assert.equal(f.commands.filter((c) => c === 'STOP').length, 1);
  assert.ok(result.residualM < 0.09, `residual ${result.residualM}`);
});

test('quarter-turn returns to initial heading across +/-180', async () => {
  const f = fixture();
  await k.run(f.port, {...config, kind: 'turn'});
  assert.ok(f.commands.includes('TRACKMOVE,-100.00,0.00,0.00'));
  assert.ok(f.commands.includes('TRACKMOVE,170.00,0.00,0.00'));
  assert.ok(f.rows.slice(-2).every((r) => Math.abs(r.error) <= 3 && Math.abs(r.rate) <= 4));
});

test('blocked motion times out with explicit STOP, never timed-distance success', async () => {
  const f = fixture({blocked: true});
  await assert.rejects(k.run(f.port, config), /无有效行程/);
  assert.equal(f.commands.at(-1), 'STOP');
});

test('repeated feedback cannot accumulate settle time', async () => {
  const f = fixture({freeze: true});
  await assert.rejects(k.run(f.port, config), /反馈重复/);
  assert.equal(f.commands.at(-1), 'STOP');
});

test('link failure stops the experiment and never resumes targets', async () => {
  const f = fixture({failCommand: 'ENC'});
  await assert.rejects(k.run(f.port, config), /BLE disconnected/);
  assert.equal(f.commands.at(-1), 'STOP');
  assert.equal(f.commands.filter((c) => c.startsWith('TRACKMOVE')).length, 1);
});

test('an invalidated test cannot send late targets or STOP a newer session', async () => {
  const f = fixture({cancelAfter: 5});
  await assert.rejects(k.run(f.port, config), /cancelled/);
  assert.ok(!f.commands.includes('STOP')); // User cancellation owns STOP.
  assert.equal(f.rows.length, 5);
});

test('confirmation and range guards run before any hardware command', async () => {
  const f = fixture();
  await assert.rejects(k.run(f.port, {...config, confirmed: false}), /确认/);
  await assert.rejects(k.run(f.port, {...config, rpm: 100}), /范围/);
  assert.equal(f.commands.length, 0);
});

test('a missing final STOP acknowledgement is not reported as success', async () => {
  const f = fixture({failCommand: 'STOP'});
  await assert.rejects(k.run(f.port, {...config, kind: 'turn'}), /STOP 未确认/);
});

test('odometry handles signed encoder wrap and uint32 tick wrap', () => {
  const previous = { e: { tick: 0xfffffff0, cpr: 380,
    counts: [2147483640,2147483640,2147483640,2147483640] },
    a: { tick: 0xfffffff8, yaw: 179 } };
  const next = { e: { tick: 184, cpr: 380,
    counts: [-2147483646,-2147483646,-2147483646,-2147483646] },
    a: { tick: 192, yaw: -179 } };
  const pos = k.integrate({x:0,y:0}, previous, next, 179);
  assert.ok(pos.x > 0.010 && pos.x < 0.011);
  assert.ok(pos.y > 0 && pos.y < 0.001);
});

test('wheel jumps or a CPR change abort instead of fabricating distance', () => {
  const previous = { e: {tick:1000,cpr:380,counts:[0,0,0,0]}, a:{tick:1000,yaw:0} };
  const next = { e: {tick:1200,cpr:380,counts:[100000,0,0,0]}, a:{tick:1200,yaw:0} };
  assert.throws(() => k.integrate({x:0,y:0}, previous, next, 0), /编码器突跳/);
  next.e.cpr = 500;
  assert.throws(() => k.integrate({x:0,y:0}, previous, next, 0), /CPR 被修改/);
});

test('path correction is opposite cross error, deadbanded and bounded', () => {
  assert.equal(k.pathVector(0, 30, 0.2, 0.004).left, 0);
  assert.ok(k.pathVector(0, 30, 0.2, 0.02).left < 0);
  assert.ok(k.pathVector(0, 30, 0.2, -0.02).left > 0);
  const v = k.pathVector(0, 60, 0.001, 1);
  assert.equal(v.forward, 8); // No sub-RPM creep or reverse endpoint hunting.
  assert.ok(Math.abs(v.left) <= v.forward * 0.4);
  const rotated = k.pathVector(90, 30, 0.2, 0.02);
  assert.ok(rotated.forward > 0 && rotated.left > 0);
});

test('stillness uses accumulated encoder travel, not quantized 10 ms RPM', async () => {
  const f = fixture({noisyRpm: true});
  const result = await k.run(f.port, {...config, kind: 'turn'});
  assert.ok(result.samples > 10);
  assert.equal(f.commands.at(-1), 'STOP');
});

test('observable lateral bias is corrected and return aims at the original line', async () => {
  const f = fixture({yaw: 0, lateralBias: 1});
  const result = await k.run(f.port, {...config, angle: 0});
  const outbound = f.rows.filter((r) => r.phase === '去程');
  assert.ok(outbound.some((r) => r.left_rpm < -0.1));
  assert.ok(Math.abs(result.y) < 0.025, `cross residual ${result.y}`);
  assert.ok(result.residualM < 0.04);
  // Once the final zero target is sent there is no new nonzero creep command.
  const finalZero = f.commands.findLastIndex((c) => c.startsWith('TRACKMOVE'));
  assert.match(f.commands[finalZero], /,0\.00,0\.00$/);
  assert.equal(f.commands.at(-1), 'STOP');
});
