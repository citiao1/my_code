/* Measurement-driven tests. RF/LF/LR/RR; x forward, y left, yaw CCW.
 * Encoder odometry cannot detect wheel slip or certify real ground position. */
(function (root) {
  'use strict';
  const DIAMETER_M = 0.127;
  const RAD = Math.PI / 180;
  const wrap = (angle) => ((angle + 180) % 360 + 360) % 360 - 180;
  const tickDelta = (next, previous) => (next - previous) >>> 0;
  const countDelta = (next, previous) => (next - previous) | 0;
  const clamp = (value, limit) => Math.max(-limit, Math.min(limit, value));
  const METRES_PER_RPM_SECOND = Math.PI * DIAMETER_M / 60;

  // Slow supervisory cross-track P, NOT a Bluetooth-rate wheel/heading loop.
  // Firmware transforms this fixed-frame vector using measured yaw at 20 ms.
  function pathVector(direction, rpm, remaining, cross) {
    const along = Math.min(rpm, Math.max(8,
      Math.max(0, remaining - 0.005) / (0.65 * METRES_PER_RPM_SECOND)));
    const error = Math.sign(cross) * Math.max(0, Math.abs(cross) - 0.005);
    const lateral = clamp(-0.6 * error / METRES_PER_RPM_SECOND,
      Math.min(8, rpm * 0.2, along * 0.4));
    const radians = direction * RAD;
    return { forward: along * Math.cos(radians) - lateral * Math.sin(radians),
      left: along * Math.sin(radians) + lateral * Math.cos(radians) };
  }

  function fields(reply, name, length) {
    const parts = String(reply).trim().split(',');
    if (parts[0] !== name || parts.length !== length ||
        parts.slice(1).some((x) => x.trim() === '' || !Number.isFinite(Number(x))))
      throw new Error(`${name} 数据格式错误`);
    return parts.slice(1).map(Number);
  }
  function attitude(reply) {
    const p = fields(reply, 'ATEL', 14);
    if (!Number.isInteger(p[0]) || p[0] < 0 || p[0] > 0xffffffff ||
        ![0, 1, 2].includes(p[1]) || ![0, 1].includes(p[11]) || Math.abs(p[7]) > 180000)
      throw new Error('ATEL 状态或航向范围无效');
    if (p[2] !== 1) throw new Error('IMU 未就绪，测试停止');
    return { tick: p[0], mode: p[1], rate: p[4] / 1000, yaw: p[7] / 1000,
      target: p[6] / 1000, output: p[5] / 1000, saturated: p[10], enabled: p[11] };
  }
  function encoders(reply) {
    const p = fields(reply, 'ENC', 16);
    if (p[1] !== 1 || !Number.isInteger(p[2]) || p[2] < 4 || p[2] > 20000 ||
        !Number.isInteger(p[0]) || p[0] < 0 || p[0] > 0xffffffff ||
        p.slice(3).some((v) => !Number.isInteger(v) || v < -2147483648 || v > 2147483647))
      throw new Error('编码器未就绪或 CPR 无效');
    return { tick: p[0], cpr: p[2], counts: p.slice(3, 7), rpm: p.slice(11, 15).map((v) => v / 10) };
  }
  function bodyVector(angle, rpm) {
    return { forward: Math.round(rpm * Math.cos(angle * RAD)),
      left: Math.round(rpm * Math.sin(angle * RAD)) };
  }
  function bodyDelta(wheels, cpr) {
    const [rf, lf, lr, rr] = wheels;
    const scale = Math.PI * DIAMETER_M / (4 * cpr);
    return { forward: (rf + lf + lr + rr) * scale,
      left: (rf - lf + lr - rr) * scale };
  }
  function integrate(position, previous, next, yaw0) {
    if (next.e.cpr !== previous.e.cpr) throw new Error('测试中编码器 CPR 被修改');
    const elapsed = tickDelta(next.e.tick, previous.e.tick);
    if (!elapsed || elapsed > 1200 || !tickDelta(next.a.tick, previous.a.tick) ||
        tickDelta(next.a.tick, previous.a.tick) > 1200)
      throw new Error('反馈重复或超过 1.2 秒未更新，测试停止');
    if (Math.abs(wrap(next.a.yaw - previous.a.yaw)) > 90)
      throw new Error('航向突跳，测试停止');
    const deltas = next.e.counts.map((v, i) => countDelta(v, previous.e.counts[i]));
    if (deltas.some((v) => Math.abs(v) > next.e.cpr * elapsed / 60000 * 220 + 8))
      throw new Error('编码器突跳或轮速异常，测试停止');
    const body = bodyDelta(deltas, next.e.cpr);
    const yaw = (previous.a.yaw + wrap(next.a.yaw - previous.a.yaw) / 2 - yaw0) * RAD;
    return { x: position.x + body.forward * Math.cos(yaw) - body.left * Math.sin(yaw),
      y: position.y + body.forward * Math.sin(yaw) + body.left * Math.cos(yaw) };
  }

  async function run(port, config) {
    const { kind, rpm, distance = 0.3, angle = 0, headingOffset = 0, confirmed } = config;
    if (!confirmed) throw new Error('请先确认测试区域、下地状态和急停可用');
    if (!['square', 'translation', 'turn'].includes(kind) ||
        ![rpm, distance, angle, headingOffset].every(Number.isFinite) ||
        rpm < 10 || rpm > 60 || distance < 0.1 || distance > 1 ||
        Math.abs(angle) > 180 || Math.abs(headingOffset) > 45)
      throw new Error('测试参数超出范围');
    const start = port.now();
    let previous, yaw0, position = { x: 0, y: 0 }, currentHeading;
    let samples = 0, maxError = 0, sumError2 = 0, saturated = 0;
    let movingSamples = 0, movingMaxError = 0, movingError2 = 0;
    let geometry = null, maxCross = 0;
    let lastDrive = '', phase = '准备';
    let failure;
    let command = { heading: 0, forward: 0, left: 0 };
    const check = () => {
      port.check();
      if (port.now() - start > 180000) throw new Error('测试超过 180 秒，已停止');
    };
    const send = async (text) => {
      check();
      const reply = await port.send(text);
      check();
      return reply;
    };
    const sample = async () => {
      await send('KEEP');
      const e = encoders(await send('ENC'));
      const a = attitude(await send('ANGLESTATUS'));
      if (a.mode !== 2 || a.enabled !== 1)
        throw new Error('方向环或速度环已停止，不自动重启');
      const next = { a, e };
      if (previous) position = integrate(position, previous, next, yaw0);
      previous = next;
      let along = 0, cross = 0;
      if (geometry) {
        const { origin, lineOrigin, radians } = geometry;
        along = (position.x - origin.x) * Math.cos(radians) +
          (position.y - origin.y) * Math.sin(radians);
        cross = -(position.x - lineOrigin.x) * Math.sin(radians) +
          (position.y - lineOrigin.y) * Math.cos(radians);
        maxCross = Math.max(maxCross, Math.abs(cross));
      }
      const error = wrap(command.heading - a.yaw);
      samples += 1; maxError = Math.max(maxError, Math.abs(error));
      sumError2 += error * error; saturated += a.saturated ? 1 : 0;
      if (command.forward || command.left) {
        movingSamples += 1; movingMaxError = Math.max(movingMaxError, Math.abs(error));
        movingError2 += error * error;
      }
      port.record({ phase, host_ms: port.now() - start, tick: a.tick, heading: command.heading,
        yaw: a.yaw, error, rate: a.rate, rotation_rpm: a.output, saturated: a.saturated,
        forward_rpm: command.forward, left_rpm: command.left,
        translation_frame: 'INITIAL', along_m: along, cross_m: cross,
        x_m: position.x, y_m: position.y,
        rf: e.counts[0], lf: e.counts[1], lr: e.counts[2], rr: e.counts[3] });
      return next;
    };
    const drive = async (heading, forward = 0, left = 0) => {
      command = { heading: wrap(heading), forward, left };
      // Convert the test's initial frame to the IMU fixed frame only once.
      // TRACKMOVE then uses ACTUAL yaw continuously, not the desired heading.
      const yaw = yaw0 * RAD;
      const fieldForward = forward * Math.cos(yaw) - left * Math.sin(yaw);
      const fieldLeft = forward * Math.sin(yaw) + left * Math.cos(yaw);
      const text = `TRACKMOVE,${command.heading.toFixed(2)},${fieldForward.toFixed(2)},${fieldLeft.toFixed(2)}`;
      if (text === lastDrive) return;
      await send(text);
      lastDrive = text;
    };
    const pause = async () => { await port.sleep(120); check(); };
    const settle = async (heading, label) => {
      phase = label;
      geometry = null;
      await drive(heading);
      const deadline = port.now() + 15000;
      let stableSince = null;
      let wheelSample = null, wheelTime = 0, wheelTravel = [0, 0, 0, 0];
      let wheelsStopped = false;
      while (port.now() < deadline) {
        const { a, e } = await sample();
        if (wheelSample) {
          wheelTime += tickDelta(e.tick, wheelSample.tick);
          wheelTravel = wheelTravel.map((v, i) => v + Math.abs(countDelta(e.counts[i], wheelSample.counts[i])));
        }
        wheelSample = e;
        const error = wrap(heading - a.yaw);
        port.progress(`${label} · 误差 ${error.toFixed(1)}° · ${a.rate.toFixed(1)}°/s`);
        // At 380 CPR a single 10 ms count looks like 15.8 RPM. Judge stillness
        // over a count window, not that quantized instantaneous speed sample.
        if (wheelTime >= 600) {
          wheelsStopped = wheelTravel.every((counts) =>
            counts * 60000 / (e.cpr * wheelTime) <= 6);
          wheelTime = 0; wheelTravel.fill(0);
        }
        const stable = Math.abs(error) <= 3 && Math.abs(a.rate) <= 4 &&
          wheelsStopped;
        if (!stable) stableSince = null;
        else if (stableSince === null) stableSince = a.tick;
        else if (tickDelta(a.tick, stableSince) >= 600) return;
        await pause();
      }
      throw new Error(`${label} 15 秒未收敛；请检查角环响应/限幅`);
    };
    const travel = async (direction, heading, length, label, lineOrigin = position) => {
      phase = label;
      const origin = { ...position };
      const radians = direction * RAD;
      geometry = { origin, lineOrigin, radians };
      const deadline = port.now() + Math.max(10000,
        4000 * length / (rpm * Math.PI * DIAMETER_M / 60) + 3000);
      let best = 0, lastProgress = port.now();
      let reached = false;
      while (port.now() < deadline) {
        const dx = position.x - origin.x, dy = position.y - origin.y;
        const along = dx * Math.cos(radians) + dy * Math.sin(radians);
        const cross = -(position.x - lineOrigin.x) * Math.sin(radians) +
          (position.y - lineOrigin.y) * Math.cos(radians);
        if (Math.abs(cross) > Math.max(0.08, length * 0.35) || along < -0.04)
          throw new Error('横向误差过大或行进方向相反，测试停止');
        if (along >= length - 0.005) { reached = true; break; }
        if (along > best + 0.005) { best = along; lastProgress = port.now(); }
        if (port.now() - lastProgress > 2500) throw new Error('2.5 秒无有效行程，检查编码器/电机');
        // Non-integrating path correction; never reverse along the path to hunt
        // the last few millimetres. Terminal arrival is latched for this leg.
        const v = pathVector(direction, rpm, length - along, cross);
        await drive(heading, v.forward, v.left);
        const data = await sample();
        if (Math.abs(wrap(heading - data.a.yaw)) > 35)
          throw new Error('移动中航向误差超过 35°，测试停止');
        port.progress(`${label} · ${(along * 100).toFixed(1)}/${(length * 100).toFixed(0)} cm · 横偏 ${(cross * 100).toFixed(1)} cm`);
        await pause();
      }
      if (!reached) throw new Error(`${label} 行程超时，测试停止`);
      await settle(heading, `${label}减速保持`);
    };

    try {
      const ps2 = String(await send('PS2STATUS')).split(',');
      if (ps2.length !== 12 || !['0', '1'].includes(ps2[2]) || ps2[2] !== '0')
        throw new Error('请先锁定 PS2 手柄，避免两个操控来源冲突');
      await send('ATEL,0');
      const initial = attitude(await send('ANGLESTATUS'));
      if (initial.enabled !== 1) throw new Error('请先 ARM 并启用速度环；测试不自动重启');
      yaw0 = initial.yaw;
      currentHeading = yaw0;
      await settle(yaw0, '准备');
      // Discard setup displacement, but retain unique feedback tick history.
      position = { x: 0, y: 0 };
      if (kind === 'turn') {
        await settle(wrap(yaw0 + 90), '左转 90°');
        await settle(yaw0, '右转返回起始航向');
      } else if (kind === 'square') {
        for (let leg = 0; leg < 4; leg += 1) {
          await travel(leg * 90, currentHeading, 0.3, `第 ${leg + 1}/4 边`);
          currentHeading = wrap(yaw0 + (leg + 1) * 90);
          await settle(currentHeading, `第 ${leg + 1}/4 次转向`);
        }
      } else {
        currentHeading = wrap(yaw0 + headingOffset);
        await settle(currentHeading, '设定车头航向');
        await travel(angle, currentHeading, distance, '去程');
        // Return the actual measured along-track distance including coast.
        const projected = position.x * Math.cos(angle * RAD) + position.y * Math.sin(angle * RAD);
        await travel(wrap(angle + 180), currentHeading, Math.max(0.05, projected), '回程', {x: 0, y: 0});
      }
      return { samples, maxHeadingError: maxError,
        rmsHeadingError: Math.sqrt(sumError2 / samples), saturationFraction: saturated / samples,
        movingMaxError, movingRmsError: movingSamples ? Math.sqrt(movingError2 / movingSamples) : 0,
        maxCrossM: maxCross,
        x: position.x, y: position.y, residualM: Math.hypot(position.x, position.y) };
    } catch (error) {
      failure = error;
      throw error;
    } finally {
      // Cancellation invalidates this port. An old test must never STOP a new
      // operation after reconnect. User STOP/disconnect owns cancellation.
      try { await send('STOP'); }
      catch (error) {
        if (!failure) {
          port.check();
          throw new Error(`STOP 未确认，依赖固件看门狗停车：${error.message}`);
        }
      }
    }
  }
  const api = { wrap, attitude, encoders, bodyVector, bodyDelta, integrate, pathVector, run };
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
  else root.ChassisTest = api;
})(typeof globalThis !== 'undefined' ? globalThis : this);
