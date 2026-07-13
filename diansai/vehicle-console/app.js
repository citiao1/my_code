const BAUD_RATE = 9600;
const LEFT_COUNTS_PER_METER = 7514;
const RIGHT_COUNTS_PER_METER = 7263;

const ui = {
  connectButton: document.querySelector('#connectButton'),
  connectionMode: document.querySelector('#connectionMode'),
  connectionBadge: document.querySelector('#connectionBadge'),
  connectionText: document.querySelector('#connectionText'),
  enableBadge: document.querySelector('#enableBadge'),
  speedSlider: document.querySelector('#speedSlider'),
  speedValue: document.querySelector('#speedValue'),
  stopButton: document.querySelector('#stopButton'),
  zeroButton: document.querySelector('#zeroButton'),
  throttleValue: document.querySelector('#throttleValue'),
  steeringValue: document.querySelector('#steeringValue'),
  packetTime: document.querySelector('#packetTime'),
  yawValue: document.querySelector('#yawValue'),
  pitchValue: document.querySelector('#pitchValue'),
  rollValue: document.querySelector('#rollValue'),
  leftSpeed: document.querySelector('#leftSpeed'),
  rightSpeed: document.querySelector('#rightSpeed'),
  leftTarget: document.querySelector('#leftTarget'),
  rightTarget: document.querySelector('#rightTarget'),
  leftError: document.querySelector('#leftError'),
  rightError: document.querySelector('#rightError'),
  leftEncoder: document.querySelector('#leftEncoder'),
  rightEncoder: document.querySelector('#rightEncoder'),
  distanceValue: document.querySelector('#distanceValue'),
  linkValue: document.querySelector('#linkValue'),
  leftPwm: document.querySelector('#leftPwm'),
  rightPwm: document.querySelector('#rightPwm'),
  browserStatus: document.querySelector('#browserStatus'),
  eventLog: document.querySelector('#eventLog'),
  chart: document.querySelector('#speedChart'),
  pidLeftKp: document.querySelector('#pidLeftKp'),
  pidLeftKi: document.querySelector('#pidLeftKi'),
  pidLeftKd: document.querySelector('#pidLeftKd'),
  pidRightKp: document.querySelector('#pidRightKp'),
  pidRightKi: document.querySelector('#pidRightKi'),
  pidRightKd: document.querySelector('#pidRightKd'),
  maxSpeed: document.querySelector('#maxSpeed'),
  applyPidButton: document.querySelector('#applyPidButton'),
  pidCurrent: document.querySelector('#pidCurrent'),
  yawEnabled: document.querySelector('#yawEnabled'),
  yawKp: document.querySelector('#yawKp'),
  yawKi: document.querySelector('#yawKi'),
  yawKd: document.querySelector('#yawKd'),
  yawKff: document.querySelector('#yawKff'),
  kffTargetRate: document.querySelector('#kffTargetRate'),
  kffCorrection: document.querySelector('#kffCorrection'),
  kffFactor: document.querySelector('#kffFactor'),
  kffResult: document.querySelector('#kffResult'),
  captureKffButton: document.querySelector('#captureKffButton'),
  calculateKffButton: document.querySelector('#calculateKffButton'),
  headingEnabled: document.querySelector('#headingEnabled'),
  headingKp: document.querySelector('#headingKp'),
  headingKd: document.querySelector('#headingKd'),
  headingKff: document.querySelector('#headingKff'),
  headingMaxRate: document.querySelector('#headingMaxRate'),
  headingTarget: document.querySelector('#headingTarget'),
  captureHeadingButton: document.querySelector('#captureHeadingButton'),
  sendHeadingTargetButton: document.querySelector('#sendHeadingTargetButton'),
  applyHeadingButton: document.querySelector('#applyHeadingButton'),
  headingTestThrottle: document.querySelector('#headingTestThrottle'),
  headingTestAngle: document.querySelector('#headingTestAngle'),
  headingTestDuration: document.querySelector('#headingTestDuration'),
  headingTestStatus: document.querySelector('#headingTestStatus'),
  startHeadingTestButton: document.querySelector('#startHeadingTestButton'),
  startQuarterTurnLeftButton: document.querySelector('#startQuarterTurnLeftButton'),
  startQuarterTurnRightButton: document.querySelector('#startQuarterTurnRightButton'),
  stopHeadingTestButton: document.querySelector('#stopHeadingTestButton'),
  squareTestThrottle: document.querySelector('#squareTestThrottle'),
  squareTestStatus: document.querySelector('#squareTestStatus'),
  startSquareLeftButton: document.querySelector('#startSquareLeftButton'),
  startSquareRightButton: document.querySelector('#startSquareRightButton'),
  maxYawRate: document.querySelector('#maxYawRate'),
  applyYawButton: document.querySelector('#applyYawButton'),
  targetYawRate: document.querySelector('#targetYawRate'),
  yawRate: document.querySelector('#yawRate'),
  yawRateError: document.querySelector('#yawRateError'),
  yawCorrection: document.querySelector('#yawCorrection'),
  yawFeedforward: document.querySelector('#yawFeedforward'),
  targetHeading: document.querySelector('#targetHeading'),
  headingError: document.querySelector('#headingError'),
  headingOutput: document.querySelector('#headingOutput'),
  headingLoopState: document.querySelector('#headingLoopState'),
  yawLoopState: document.querySelector('#yawLoopState'),
  mpuState: document.querySelector('#mpuState'),
  batteryVoltage: document.querySelector('#batteryVoltage'),
  batteryRaw: document.querySelector('#batteryRaw'),
};

let port;
let reader;
let writer;
let bleDevice;
let bleNotifyCharacteristic;
let bleWriteCharacteristic;
let bridgeSocket;
let connectedTransport;
let readBuffer = '';
let writeChain = Promise.resolve();
let commandTimer;
let closing = false;
const activeDirections = new Set();
const history = { left: [], right: [], targetLeft: [], targetRight: [] };
let latestYawSample = { target: 0, correction: 0 };
let latestYawAngle = 0;
let latestYawRate = 0;
let latestHeadingError = 0;
let latestHeadingState = { motorEnabled: 0, yawEnabled: 0, mpuOk: 0, headingEnabled: 0, headingActive: 0 };
let latestSquareState = { active: 0, phase: 0, leg: 0, direction: 0, progressMm: 0, remainingMm: 0 };
let headingTestActive = false;
let headingTestToken = 0;
let headingTestCommandTimer;

function setLog(message) {
  ui.eventLog.textContent = `${new Date().toLocaleTimeString()}  ${message}`;
}

function setConnected(connected) {
  ui.connectionBadge.className = `badge ${connected ? 'connected' : 'disconnected'}`;
  const modeName = connectedTransport === 'bridge' ? '桥接' : (connectedTransport === 'ble' ? 'BLE' : '串口');
  ui.connectionText.textContent = connected ? `${modeName}已连接` : '未连接';
  const connectLabels = { bridge: '连接桥接', ble: '连接蓝牙', serial: '连接串口' };
  ui.connectButton.textContent = connected ? '断开连接' : connectLabels[ui.connectionMode.value];
  ui.connectionMode.disabled = connected;
}

async function sendLine(line) {
  if (!writer && !bleWriteCharacteristic && bridgeSocket?.readyState !== WebSocket.OPEN) return;
  const payload = new TextEncoder().encode(`${line}\n`);
  writeChain = writeChain.then(async () => {
    if (writer) return writer.write(payload);
    if (bridgeSocket?.readyState === WebSocket.OPEN) return bridgeSocket.send(`${line}\n`);
    if (bleWriteCharacteristic?.properties.writeWithoutResponse) {
      return bleWriteCharacteristic.writeValueWithoutResponse(payload);
    }
    return bleWriteCharacteristic?.writeValue(payload);
  }).catch((error) => {
    setLog(`发送失败: ${error.message}`);
  });
  return writeChain;
}

async function connectSerial() {
  if (!('serial' in navigator)) {
    setLog('当前浏览器不支持 Web Serial');
    return;
  }
  try {
    port = await navigator.serial.requestPort();
    await port.open({ baudRate: BAUD_RATE, bufferSize: 255 });
    writer = port.writable.getWriter();
    connectedTransport = 'serial';
    closing = false;
    setConnected(true);
    setLog(`已连接 ${BAUD_RATE} baud`);
    await sendLine('STOP');
    readLoop();
  } catch (error) {
    setLog(`连接失败: ${error.message}`);
    await disconnect(false);
  }
}

async function connectBle() {
  if (!('bluetooth' in navigator)) {
    setLog('当前浏览器不支持 Web Bluetooth');
    return;
  }
  try {
    bleDevice = await navigator.bluetooth.requestDevice({ filters: [{ services: [0xffe0] }] });
    bleDevice.addEventListener('gattserverdisconnected', handleBleDisconnected);
    const server = await bleDevice.gatt.connect();
    const service = await server.getPrimaryService(0xffe0);
    bleNotifyCharacteristic = await service.getCharacteristic(0xffe1);
    await bleNotifyCharacteristic.startNotifications();
    bleNotifyCharacteristic.addEventListener('characteristicvaluechanged', handleBleNotification);

    if (bleNotifyCharacteristic.properties.write || bleNotifyCharacteristic.properties.writeWithoutResponse) {
      bleWriteCharacteristic = bleNotifyCharacteristic;
    } else {
      bleWriteCharacteristic = await service.getCharacteristic(0xffe2);
    }

    connectedTransport = 'ble';
    closing = false;
    setConnected(true);
    setLog(`已连接 ${bleDevice.name || 'BLE device'} · FFE0`);
    await sendLine('STOP');
  } catch (error) {
    setLog(`蓝牙连接失败: ${error.message}`);
    await disconnect(false);
  }
}

async function connectBridge() {
  try {
    bridgeSocket = new WebSocket('ws://127.0.0.1:8766');
    await new Promise((resolve, reject) => {
      const timeout = setTimeout(() => reject(new Error('bridge connection timeout')), 3000);
      bridgeSocket.addEventListener('open', () => { clearTimeout(timeout); resolve(); }, { once: true });
      bridgeSocket.addEventListener('error', () => { clearTimeout(timeout); reject(new Error('bridge is not running')); }, { once: true });
    });
    bridgeSocket.addEventListener('message', (event) => consumeIncomingText(String(event.data)));
    bridgeSocket.addEventListener('close', handleBridgeClosed);
    connectedTransport = 'bridge';
    closing = false;
    setConnected(true);
    setLog('已连接本地 BLE 桥接 · ws://127.0.0.1:8766');
    await sendLine('STOP');
  } catch (error) {
    setLog(`桥接连接失败: ${error.message}`);
    try { bridgeSocket?.close(); } catch (_) {}
    bridgeSocket = undefined;
    connectedTransport = undefined;
    setConnected(false);
  }
}

function handleBridgeClosed() {
  if (!closing) disconnect(false);
}

function connect() {
  if (ui.connectionMode.value === 'bridge') return connectBridge();
  return ui.connectionMode.value === 'ble' ? connectBle() : connectSerial();
}

function consumeIncomingText(text) {
  readBuffer += text;
  const lines = readBuffer.split(/\r?\n/);
  readBuffer = lines.pop() ?? '';
  lines.forEach(parseLine);
}

function handleBleNotification(event) {
  const bytes = new Uint8Array(event.target.value.buffer);
  consumeIncomingText(new TextDecoder().decode(bytes));
}

function handleBleDisconnected() {
  if (!closing) disconnect(false);
}

async function disconnect(sendStop = true) {
  if ((!port && !bleDevice && !bridgeSocket) || closing) return;
  closing = true;
  stopHeadingAutoTest(false, false);
  clearInterval(commandTimer);
  activeDirections.clear();
  updateDirectionButtons();

  try {
    if (sendStop && writer) await sendLine('STOP');
    if (reader) await reader.cancel();
  } catch (_) {}

  try { reader?.releaseLock(); } catch (_) {}
  try { writer?.releaseLock(); } catch (_) {}
  reader = undefined;
  writer = undefined;
  try { await port.close(); } catch (_) {}
  port = undefined;
  if (bleNotifyCharacteristic) {
    try { bleNotifyCharacteristic.removeEventListener('characteristicvaluechanged', handleBleNotification); } catch (_) {}
  }
  if (bleDevice) {
    try { bleDevice.removeEventListener('gattserverdisconnected', handleBleDisconnected); } catch (_) {}
    try { bleDevice.gatt?.disconnect(); } catch (_) {}
  }
  bleNotifyCharacteristic = undefined;
  bleWriteCharacteristic = undefined;
  bleDevice = undefined;
  if (bridgeSocket) {
    try { bridgeSocket.removeEventListener('close', handleBridgeClosed); } catch (_) {}
    try { bridgeSocket.close(); } catch (_) {}
  }
  bridgeSocket = undefined;
  connectedTransport = undefined;
  closing = false;
  setConnected(false);
  setLog('连接已断开');
}

async function readLoop() {
  const decoder = new TextDecoder();
  reader = port.readable.getReader();
  try {
    while (!closing) {
      const { value, done } = await reader.read();
      if (done) break;
      consumeIncomingText(decoder.decode(value, { stream: true }));
    }
  } catch (error) {
    if (!closing) setLog(`接收中断: ${error.message}`);
  } finally {
    if (!closing) await disconnect(false);
  }
}

function parseLine(line) {
  const fields = line.trim().split(',');
  const values = fields.slice(1).map(Number);
  if (values.some(Number.isNaN)) return;

  if (fields[0] === 'STA') {
    if (values.length < 12) return;
    const [time, pitch10, roll10, encL, encR, batteryRaw,
      kpL, kiL, kdL, kpR, kiR, kdR,
      squareActive = 0, squarePhase = 0, squareLeg = 0, squareDirection = 0,
      squareProgressMm = 0, squareRemainingMm = 0] = values;
    ui.packetTime.textContent = `${time} ms`;
    ui.pitchValue.textContent = `${(pitch10 / 10).toFixed(1)}°`;
    ui.rollValue.textContent = `${(roll10 / 10).toFixed(1)}°`;
    ui.leftEncoder.textContent = encL.toLocaleString();
    ui.rightEncoder.textContent = encR.toLocaleString();
    const distanceMm = 0.5 * (encL / LEFT_COUNTS_PER_METER + encR / RIGHT_COUNTS_PER_METER) * 1000;
    ui.distanceValue.textContent = `${Math.round(distanceMm)} mm`;
    ui.batteryRaw.textContent = batteryRaw.toLocaleString();
    ui.pidCurrent.textContent = `L ${kpL}/${kiL}/${kdL} · R ${kpR}/${kiR}/${kdR}`;
    latestSquareState = {
      active: squareActive,
      phase: squarePhase,
      leg: squareLeg,
      direction: squareDirection,
      progressMm: squareProgressMm,
      remainingMm: squareRemainingMm,
    };
    if (squareActive) {
      const directionName = squareDirection > 0 ? '左转' : '右转';
      ui.squareTestStatus.textContent = squarePhase === 1
        ? `${directionName} · 第${squareLeg}/4边 · ${squareProgressMm}mm`
        : `${directionName} · 第${squareLeg}/4次转向`;
    } else if (squarePhase === 3) {
      ui.squareTestStatus.textContent = '完成';
    } else if (squarePhase === 4) {
      ui.squareTestStatus.textContent = '异常停止';
    }
    return;
  }

  if (fields[0] !== 'TEL') return;

  let data;
  if (values.length >= 36) {
    const [time, enabled, link, yaw10, pitch10, roll10, leftMm, rightMm,
      targetLeftMm, targetRightMm, errorLeftMm, errorRightMm,
      encL, encR, pwmL, pwmR, kpL, kiL, kdL, kpR, kiR, kdR,
      targetYaw10, yawRate10, yawError10, yawCorrectionMm,
      yawEnabled, batteryMv, batteryRaw, mpuOk, yawFeedforwardMm,
      targetHeading10, headingError10, headingOutput10, headingEnabled, headingActive] = values;
    data = { time, enabled, link, yaw10, leftMm, rightMm, targetLeftMm, targetRightMm,
      errorLeftMm, errorRightMm, pwmL, pwmR, targetYaw10, yawRate10, yawError10,
      yawCorrectionMm, yawEnabled, batteryMv, mpuOk, yawFeedforwardMm,
      targetHeading10, headingError10, headingOutput10, headingEnabled, headingActive };
    ui.pitchValue.textContent = `${(pitch10 / 10).toFixed(1)}°`;
    ui.rollValue.textContent = `${(roll10 / 10).toFixed(1)}°`;
    ui.leftEncoder.textContent = encL.toLocaleString();
    ui.rightEncoder.textContent = encR.toLocaleString();
    const distanceMm = 0.5 * (encL / LEFT_COUNTS_PER_METER + encR / RIGHT_COUNTS_PER_METER) * 1000;
    ui.distanceValue.textContent = `${Math.round(distanceMm)} mm`;
    ui.batteryRaw.textContent = batteryRaw.toLocaleString();
    ui.pidCurrent.textContent = `L ${kpL}/${kiL}/${kdL} · R ${kpR}/${kiR}/${kdR}`;
  } else if (values.length >= 23) {
    const [time, enabled, link, yaw10, leftMm, rightMm, targetLeftMm, targetRightMm,
      pwmL, pwmR, targetYaw10, yawRate10, yawError10, yawCorrectionMm,
      yawEnabled, mpuOk, yawFeedforwardMm, targetHeading10, headingError10,
      headingOutput10, headingEnabled, headingActive, batteryMv] = values;
    data = { time, enabled, link, yaw10, leftMm, rightMm, targetLeftMm, targetRightMm,
      errorLeftMm: targetLeftMm - leftMm, errorRightMm: targetRightMm - rightMm,
      pwmL, pwmR, targetYaw10, yawRate10, yawError10, yawCorrectionMm,
      yawEnabled, batteryMv, mpuOk, yawFeedforwardMm,
      targetHeading10, headingError10, headingOutput10, headingEnabled, headingActive };
  } else {
    return;
  }

  const { time, enabled, link, yaw10, leftMm, rightMm, targetLeftMm, targetRightMm,
    errorLeftMm, errorRightMm, pwmL, pwmR, targetYaw10, yawRate10, yawError10,
    yawCorrectionMm, yawEnabled, batteryMv, mpuOk, yawFeedforwardMm,
    targetHeading10, headingError10, headingOutput10, headingEnabled, headingActive } = data;
  ui.packetTime.textContent = `${time} ms`;
  ui.yawValue.textContent = `${(yaw10 / 10).toFixed(1)}°`;
  latestYawAngle = yaw10 / 10;
  ui.leftSpeed.textContent = `${(leftMm / 1000).toFixed(3)} m/s`;
  ui.rightSpeed.textContent = `${(rightMm / 1000).toFixed(3)} m/s`;
  ui.leftTarget.textContent = `${(targetLeftMm / 1000).toFixed(3)} m/s`;
  ui.rightTarget.textContent = `${(targetRightMm / 1000).toFixed(3)} m/s`;
  ui.leftError.textContent = `${(errorLeftMm / 1000).toFixed(3)} m/s`;
  ui.rightError.textContent = `${(errorRightMm / 1000).toFixed(3)} m/s`;
  ui.linkValue.textContent = link ? '控制中' : '待机';
  ui.leftPwm.textContent = `${pwmL} (${Math.round(Math.abs(pwmL) / 16799 * 100)}%)`;
  ui.rightPwm.textContent = `${pwmR} (${Math.round(Math.abs(pwmR) / 16799 * 100)}%)`;
  ui.targetYawRate.textContent = `${(targetYaw10 / 10).toFixed(1)} °/s`;
  ui.yawRate.textContent = `${(yawRate10 / 10).toFixed(1)} °/s`;
  latestYawRate = yawRate10 / 10;
  ui.yawRateError.textContent = `${(yawError10 / 10).toFixed(1)} °/s`;
  ui.yawCorrection.textContent = `${(yawCorrectionMm / 1000).toFixed(3)} m/s`;
  latestYawSample = { target: targetYaw10 / 10, correction: yawCorrectionMm / 1000 };
  ui.yawFeedforward.textContent = `${((yawFeedforwardMm ?? 0) / 1000).toFixed(3)} m/s`;
  ui.targetHeading.textContent = `${((targetHeading10 ?? 0) / 10).toFixed(1)} °`;
  ui.headingError.textContent = `${((headingError10 ?? 0) / 10).toFixed(1)} °`;
  latestHeadingError = (headingError10 ?? 0) / 10;
  ui.headingOutput.textContent = `${((headingOutput10 ?? 0) / 10).toFixed(1)} °/s`;
  ui.headingLoopState.textContent = headingEnabled
    ? (headingActive ? '保持中' : '等待直行') : '关闭';
  latestHeadingState = { motorEnabled: enabled, yawEnabled, mpuOk, headingEnabled, headingActive };
  ui.yawLoopState.textContent = yawEnabled ? '已启用' : '关闭';
  ui.mpuState.textContent = mpuOk === undefined ? '旧固件' : (mpuOk ? '正常' : '故障/重试中');
  ui.batteryVoltage.textContent = `${(batteryMv / 1000).toFixed(2)} V`;

  ui.enableBadge.className = `badge ${enabled ? 'enabled' : 'disabled'}`;
  ui.enableBadge.innerHTML = `<span class="dot"></span>${enabled ? '使能开启' : '使能关闭'}`;
  pushHistory(leftMm / 1000, rightMm / 1000, targetLeftMm / 1000, targetRightMm / 1000);
}

function applyPidParameters() {
  const left = [ui.pidLeftKp, ui.pidLeftKi, ui.pidLeftKd].map((input) => Math.round(Number(input.value)));
  const right = [ui.pidRightKp, ui.pidRightKi, ui.pidRightKd].map((input) => Math.round(Number(input.value)));
  const maxSpeed = Math.round(Number(ui.maxSpeed.value));
  const validPid = ([kp, ki, kd]) => Number.isFinite(kp) && kp >= 0 && kp <= 50000
    && Number.isFinite(ki) && ki >= 0 && ki <= 50000
    && Number.isFinite(kd) && kd >= 0 && kd <= 5000;
  const valid = validPid(left) && validPid(right)
    && Number.isFinite(maxSpeed) && maxSpeed >= 50 && maxSpeed <= 1500;
  if (!valid) {
    setLog('PID 参数超出允许范围');
    return;
  }
  sendLine(`PIDL,${left.join(',')}`);
  sendLine(`PIDR,${right.join(',')}`);
  sendLine(`MAX,${maxSpeed}`);
  setLog(`已发送左 PID ${left.join('/')} · 右 PID ${right.join('/')} · ${maxSpeed} mm/s`);
}

function applyYawParameters() {
  const gains = [ui.yawKp, ui.yawKi, ui.yawKd, ui.yawKff].map((input) => Number(input.value));
  const maxYawRate = Math.round(Number(ui.maxYawRate.value));
  const valid = gains.every((gain) => Number.isFinite(gain) && gain >= 0 && gain <= 0.1)
    && Number.isFinite(maxYawRate) && maxYawRate >= 10 && maxYawRate <= 360;
  if (!valid) {
    setLog('角速度环参数超出允许范围');
    return;
  }
  const scaled = gains.map((gain) => Math.round(gain * 1000000));
  sendLine(`YAWPID,${scaled.join(',')}`);
  sendLine(`YAWRATE,${maxYawRate}`);
  sendLine(`YAW,${ui.yawEnabled.checked ? 1 : 0}`);
  setLog(`角速度 PID ${gains.join('/')} · 最大 ${maxYawRate} °/s · ${ui.yawEnabled.checked ? '启用' : '关闭'}`);
}

function calculateKff() {
  const target = Math.abs(Number(ui.kffTargetRate.value));
  const correction = Math.abs(Number(ui.kffCorrection.value));
  const factor = Number(ui.kffFactor.value);
  if (!Number.isFinite(target) || target < 1 || !Number.isFinite(correction)
      || correction <= 0 || !Number.isFinite(factor) || factor < 1 || factor > 100) {
    setLog('Kff计算数据无效');
    return;
  }

  const kff = correction / target * factor / 100;
  if (!Number.isFinite(kff) || kff <= 0 || kff > 0.1) {
    setLog('Kff计算结果超出允许范围');
    return;
  }

  const formatted = kff.toFixed(6);
  ui.yawKff.value = formatted;
  ui.kffResult.textContent = `Kff ${formatted}`;
  setLog(`Kff ${correction.toFixed(3)} / ${target.toFixed(1)} × ${factor.toFixed(0)}% = ${formatted}`);
}

function captureCurrentKffSample() {
  if (Math.abs(latestYawSample.target) < 1 || Math.abs(latestYawSample.correction) <= 0) {
    setLog('当前没有可用的稳定转弯数据');
    return;
  }
  ui.kffTargetRate.value = latestYawSample.target.toFixed(1);
  ui.kffCorrection.value = latestYawSample.correction.toFixed(3);
  calculateKff();
}

function applyHeadingParameters() {
  const kp = Number(ui.headingKp.value);
  const kd = Number(ui.headingKd.value);
  const kff = Number(ui.headingKff.value);
  const maxRate = Math.round(Number(ui.headingMaxRate.value));
  const valid = Number.isFinite(kp) && kp >= 0 && kp <= 20
    && Number.isFinite(kd) && kd >= 0 && kd <= 10
    && Number.isFinite(kff) && kff >= 0 && kff <= 2
    && Number.isFinite(maxRate) && maxRate >= 5 && maxRate <= 360;
  if (!valid) {
    setLog('方向环参数超出允许范围');
    return;
  }

  sendLine(`HEADPID,${Math.round(kp * 1000)},${Math.round(kd * 1000)},${Math.round(kff * 1000)},${maxRate}`);
  if (ui.headingEnabled.checked) sendLine('YAW,1');
  sendLine(`HEAD,${ui.headingEnabled.checked ? 1 : 0}`);
  setLog(`方向环 PD+FF ${kp}/${kd}/${kff} · 最大 ${maxRate} °/s · ${ui.headingEnabled.checked ? '启用' : '关闭'}`);
}

function captureCurrentHeading() {
  ui.headingTarget.value = latestYawAngle.toFixed(1);
  setLog(`已采集当前航向 ${latestYawAngle.toFixed(1)}°`);
}

function sendHeadingTarget() {
  const target = Number(ui.headingTarget.value);
  if (!Number.isFinite(target) || target < -180 || target > 180) {
    setLog('测试目标航向超出允许范围');
    return;
  }
  sendLine(`HEADSET,${Math.round(target * 10)}`);
  setLog(`已发送测试目标航向 ${target.toFixed(1)}°`);
}

function wrapHeading(angle) {
  let wrapped = angle;
  while (wrapped > 180) wrapped -= 360;
  while (wrapped < -180) wrapped += 360;
  return wrapped;
}

function delay(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

function setHeadingTestRunning(running, status) {
  headingTestActive = running;
  ui.startHeadingTestButton.disabled = running;
  ui.startQuarterTurnLeftButton.disabled = running;
  ui.startQuarterTurnRightButton.disabled = running;
  ui.startSquareLeftButton.disabled = running;
  ui.startSquareRightButton.disabled = running;
  ui.stopHeadingTestButton.disabled = !running;
  ui.headingTestStatus.textContent = status;
}

function stopHeadingAutoTest(sendStop = true, writeLog = true) {
  const wasActive = headingTestActive;
  headingTestToken += 1;
  clearInterval(headingTestCommandTimer);
  headingTestCommandTimer = undefined;
  setHeadingTestRunning(false, '待机');
  if (wasActive) ui.squareTestStatus.textContent = '待机';
  if (sendStop && connectedTransport) sendLine('STOP');
  if (writeLog && wasActive) setLog('方向环自动测试已停止');
}

async function waitHeadingTestStage(durationMs, token) {
  const end = performance.now() + durationMs;
  while (performance.now() < end) {
    await delay(100);
    if (token !== headingTestToken || !headingTestActive) return false;
    if (!connectedTransport || !latestHeadingState.mpuOk || !latestHeadingState.yawEnabled
        || !latestHeadingState.headingEnabled || !latestHeadingState.headingActive) {
      stopHeadingAutoTest(true, false);
      setLog('自动测试中止：闭环或MPU状态异常');
      return false;
    }
  }
  return true;
}

async function startHeadingAutoTest() {
  if (!connectedTransport || headingTestActive) {
    setLog(headingTestActive ? '方向环自动测试正在运行' : '请先连接车辆');
    return;
  }

  const throttle = Math.round(Number(ui.headingTestThrottle.value));
  const angle = Number(ui.headingTestAngle.value);
  const durationMs = Number(ui.headingTestDuration.value) * 1000;
  const valid = Number.isFinite(throttle) && throttle >= 5 && throttle <= 50
    && Number.isFinite(angle) && angle >= 1 && angle <= 45
    && Number.isFinite(durationMs) && durationMs >= 1000 && durationMs <= 10000;
  if (!valid) {
    setLog('自动测试参数超出允许范围');
    return;
  }

  activeDirections.clear();
  clearInterval(commandTimer);
  updateDirectionButtons();
  ui.yawEnabled.checked = true;
  ui.headingEnabled.checked = true;
  applyYawParameters();
  applyHeadingParameters();

  const token = ++headingTestToken;
  setHeadingTestRunning(true, '准备');
  const driveCommand = `DRV,${throttle},0`;
  sendLine(driveCommand);
  headingTestCommandTimer = setInterval(() => sendLine(driveCommand), 100);
  ui.throttleValue.textContent = throttle;
  ui.steeringValue.textContent = '0';

  await delay(1200);
  if (token !== headingTestToken || !headingTestActive) return;
  if (!latestHeadingState.motorEnabled || !latestHeadingState.mpuOk
      || !latestHeadingState.yawEnabled || !latestHeadingState.headingEnabled
      || !latestHeadingState.headingActive) {
    stopHeadingAutoTest(true, false);
    setLog('自动测试无法开始：检查电机使能、MPU和闭环状态');
    return;
  }

  const baseline = latestYawAngle;
  const stages = [
    { label: `+${angle.toFixed(1)}°`, target: wrapHeading(baseline + angle) },
    { label: '回基准', target: baseline },
    { label: `-${angle.toFixed(1)}°`, target: wrapHeading(baseline - angle) },
    { label: '回基准', target: baseline },
  ];

  for (const stage of stages) {
    if (token !== headingTestToken || !headingTestActive) return;
    ui.headingTestStatus.textContent = stage.label;
    ui.headingTarget.value = stage.target.toFixed(1);
    sendLine(`HEADSET,${Math.round(stage.target * 10)}`);
    if (!await waitHeadingTestStage(durationMs, token)) return;
  }

  clearInterval(headingTestCommandTimer);
  headingTestCommandTimer = undefined;
  sendLine('STOP');
  ui.throttleValue.textContent = '0';
  ui.steeringValue.textContent = '0';
  setHeadingTestRunning(false, '完成');
  setLog('方向环自动测试完成');
}

async function waitQuarterTurnSettled(target, token) {
  const startedAt = performance.now();
  const deadline = performance.now() + 7000;
  let stableSince;

  while (performance.now() < deadline) {
    await delay(100);
    if (token !== headingTestToken || !headingTestActive) return false;
    if (!connectedTransport || !latestHeadingState.motorEnabled || !latestHeadingState.mpuOk
        || !latestHeadingState.yawEnabled || !latestHeadingState.headingEnabled) {
      stopHeadingAutoTest(true, false);
      setLog('四次90°测试中止：闭环、使能或MPU状态异常');
      return false;
    }
    if (!latestHeadingState.headingActive) {
      if (performance.now() - startedAt < 800) continue;
      stopHeadingAutoTest(true, false);
      setLog('四次90°测试中止：原地航向控制未激活');
      return false;
    }

    const angleError = Math.abs(wrapHeading(target - latestYawAngle));
    const settled = angleError <= 1.0 && Math.abs(latestYawRate) <= 4.0
      && Math.abs(latestHeadingError) <= 1.0;
    if (settled) {
      stableSince ??= performance.now();
      if (performance.now() - stableSince >= 400) return true;
    } else {
      stableSince = undefined;
    }
  }

  stopHeadingAutoTest(true, false);
  setLog('四次90°测试中止：单次转向7秒内未稳定');
  return false;
}

async function startQuarterTurnTest(direction) {
  if (!connectedTransport || headingTestActive) {
    setLog(headingTestActive ? '方向环自动测试正在运行' : '请先连接车辆');
    return;
  }

  activeDirections.clear();
  clearInterval(commandTimer);
  updateDirectionButtons();
  ui.yawEnabled.checked = true;
  ui.headingEnabled.checked = true;
  applyYawParameters();
  applyHeadingParameters();

  const token = ++headingTestToken;
  const directionName = direction > 0 ? '左转' : '右转';
  const holdCommand = 'DRV,0,0';
  setHeadingTestRunning(true, `${directionName}准备`);
  sendLine(holdCommand);
  headingTestCommandTimer = setInterval(() => sendLine(holdCommand), 100);
  ui.throttleValue.textContent = '0';
  ui.steeringValue.textContent = '0';

  await delay(700);
  if (token !== headingTestToken || !headingTestActive) return;
  if (!latestHeadingState.motorEnabled || !latestHeadingState.mpuOk
      || !latestHeadingState.yawEnabled || !latestHeadingState.headingEnabled) {
    stopHeadingAutoTest(true, false);
    setLog('四次90°测试无法开始：检查电机使能、MPU和闭环状态');
    return;
  }

  const baseline = latestYawAngle;
  for (let index = 1; index <= 4; index += 1) {
    if (token !== headingTestToken || !headingTestActive) return;
    const target = wrapHeading(baseline + direction * 90 * index);
    ui.headingTestStatus.textContent = `${directionName} ${index}/4 · ${target.toFixed(1)}°`;
    ui.headingTarget.value = target.toFixed(1);
    sendLine(`HEADSET,${Math.round(target * 10)}`);
    if (!await waitQuarterTurnSettled(target, token)) return;
  }

  const finalError = Math.abs(wrapHeading(baseline - latestYawAngle));
  clearInterval(headingTestCommandTimer);
  headingTestCommandTimer = undefined;
  sendLine('STOP');
  setHeadingTestRunning(false, `完成 · 误差 ${finalError.toFixed(1)}°`);
  setLog(`${directionName}四次90°测试完成 · 回到起点误差 ${finalError.toFixed(1)}°`);
}

async function startSquareTest(direction) {
  if (!connectedTransport || headingTestActive) {
    setLog(headingTestActive ? '自动测试正在运行' : '请先连接车辆');
    return;
  }

  const throttle = Math.round(Number(ui.squareTestThrottle.value));
  if (!Number.isFinite(throttle) || throttle < 10 || throttle > 60) {
    setLog('正方形测试油门应在10%到60%之间');
    return;
  }

  activeDirections.clear();
  clearInterval(commandTimer);
  updateDirectionButtons();
  ui.yawEnabled.checked = true;
  ui.headingEnabled.checked = true;
  applyPidParameters();
  applyYawParameters();
  applyHeadingParameters();

  const token = ++headingTestToken;
  const directionName = direction > 0 ? '左转' : '右转';
  setHeadingTestRunning(true, '正方形测试');
  ui.squareTestStatus.textContent = `${directionName} · 准备`;
  ui.throttleValue.textContent = '自动';
  ui.steeringValue.textContent = '自动';

  await delay(600);
  if (token !== headingTestToken || !headingTestActive) return;
  sendLine(`SQUARE,${throttle},${direction}`);
  headingTestCommandTimer = setInterval(() => sendLine('PING'), 100);

  const startDeadline = performance.now() + 2000;
  while (performance.now() < startDeadline && !latestSquareState.active) {
    await delay(100);
    if (token !== headingTestToken || !headingTestActive) return;
  }
  if (!latestSquareState.active) {
    stopHeadingAutoTest(true, false);
    setLog('正方形测试无法启动：检查电机使能、MPU和新固件');
    return;
  }

  const deadline = performance.now() + 180000;
  while (performance.now() < deadline) {
    await delay(200);
    if (token !== headingTestToken || !headingTestActive) return;
    if (!connectedTransport || !latestHeadingState.motorEnabled || !latestHeadingState.mpuOk) {
      stopHeadingAutoTest(true, false);
      setLog('正方形测试中止：连接、使能或MPU状态异常');
      return;
    }
    if (!latestSquareState.active && latestSquareState.phase === 3) {
      clearInterval(headingTestCommandTimer);
      headingTestCommandTimer = undefined;
      sendLine('STOP');
      ui.throttleValue.textContent = '0';
      ui.steeringValue.textContent = '0';
      setHeadingTestRunning(false, '完成');
      ui.squareTestStatus.textContent = '完成 · 4m / 360°';
      setLog(`${directionName}1米正方形测试完成`);
      return;
    }
    if (!latestSquareState.active && latestSquareState.phase === 4) {
      stopHeadingAutoTest(true, false);
      setLog('正方形测试异常停止：单边或转向超时');
      return;
    }
  }

  stopHeadingAutoTest(true, false);
  setLog('正方形测试超过180秒，已自动停止');
}

function currentCommand() {
  const speed = Number(ui.speedSlider.value);
  const throttle = (activeDirections.has('forward') ? speed : 0) - (activeDirections.has('back') ? speed : 0);
  const steering = (activeDirections.has('left') ? speed : 0) - (activeDirections.has('right') ? speed : 0);
  return { throttle, steering };
}

function transmitCommand() {
  const { throttle, steering } = currentCommand();
  ui.throttleValue.textContent = throttle;
  ui.steeringValue.textContent = steering;
  if (throttle || steering) sendLine(`DRV,${throttle},${steering}`);
}

function beginDirection(direction) {
  if (headingTestActive) stopHeadingAutoTest(true, false);
  activeDirections.add(direction);
  updateDirectionButtons();
  transmitCommand();
  clearInterval(commandTimer);
  commandTimer = setInterval(transmitCommand, 100);
}

function endDirection(direction) {
  activeDirections.delete(direction);
  updateDirectionButtons();
  if (activeDirections.size === 0) {
    clearInterval(commandTimer);
    sendLine('STOP');
    ui.throttleValue.textContent = '0';
    ui.steeringValue.textContent = '0';
  } else {
    transmitCommand();
  }
}

function emergencyStop() {
  stopHeadingAutoTest(false, false);
  activeDirections.clear();
  clearInterval(commandTimer);
  updateDirectionButtons();
  ui.throttleValue.textContent = '0';
  ui.steeringValue.textContent = '0';
  sendLine('STOP');
  setLog('已发送急停');
}

function updateDirectionButtons() {
  document.querySelectorAll('[data-direction]').forEach((button) => {
    button.classList.toggle('active', activeDirections.has(button.dataset.direction));
  });
}

document.querySelectorAll('[data-direction]').forEach((button) => {
  const direction = button.dataset.direction;
  button.addEventListener('pointerdown', (event) => {
    event.preventDefault();
    button.setPointerCapture(event.pointerId);
    beginDirection(direction);
  });
  button.addEventListener('pointerup', () => endDirection(direction));
  button.addEventListener('pointercancel', () => endDirection(direction));
});

const keyDirections = {
  ArrowUp: 'forward', w: 'forward', W: 'forward',
  ArrowDown: 'back', s: 'back', S: 'back',
  ArrowLeft: 'left', a: 'left', A: 'left',
  ArrowRight: 'right', d: 'right', D: 'right',
};

window.addEventListener('keydown', (event) => {
  const direction = keyDirections[event.key];
  if (!direction || event.repeat) return;
  event.preventDefault();
  beginDirection(direction);
});

window.addEventListener('keyup', (event) => {
  const direction = keyDirections[event.key];
  if (!direction) return;
  event.preventDefault();
  endDirection(direction);
});

window.addEventListener('blur', emergencyStop);
ui.stopButton.addEventListener('click', emergencyStop);
ui.zeroButton.addEventListener('click', () => sendLine('ZERO'));
ui.applyPidButton.addEventListener('click', applyPidParameters);
ui.applyYawButton.addEventListener('click', applyYawParameters);
ui.captureKffButton.addEventListener('click', captureCurrentKffSample);
ui.calculateKffButton.addEventListener('click', calculateKff);
ui.applyHeadingButton.addEventListener('click', applyHeadingParameters);
ui.captureHeadingButton.addEventListener('click', captureCurrentHeading);
ui.sendHeadingTargetButton.addEventListener('click', sendHeadingTarget);
ui.startHeadingTestButton.addEventListener('click', startHeadingAutoTest);
ui.startQuarterTurnLeftButton.addEventListener('click', () => startQuarterTurnTest(1));
ui.startQuarterTurnRightButton.addEventListener('click', () => startQuarterTurnTest(-1));
ui.startSquareLeftButton.addEventListener('click', () => startSquareTest(1));
ui.startSquareRightButton.addEventListener('click', () => startSquareTest(-1));
ui.stopHeadingTestButton.addEventListener('click', () => stopHeadingAutoTest(true));
ui.speedSlider.addEventListener('input', () => {
  ui.speedValue.textContent = `${ui.speedSlider.value}%`;
  if (activeDirections.size) transmitCommand();
});
ui.connectButton.addEventListener('click', () => connectedTransport ? disconnect() : connect());
ui.connectionMode.addEventListener('change', () => {
  const labels = { bridge: '连接桥接', ble: '连接蓝牙', serial: '连接串口' };
  const statuses = { bridge: 'Local Bridge · 8766', ble: 'Web Bluetooth · FFE0', serial: `Web Serial · ${BAUD_RATE} baud` };
  ui.connectButton.textContent = labels[ui.connectionMode.value];
  ui.browserStatus.textContent = statuses[ui.connectionMode.value];
});

function pushHistory(left, right, targetLeft, targetRight) {
  history.left.push(left);
  history.right.push(right);
  history.targetLeft.push(targetLeft);
  history.targetRight.push(targetRight);
  Object.values(history).forEach((series) => {
    if (series.length > 120) series.shift();
  });
  drawChart();
}

function drawChart() {
  const canvas = ui.chart;
  const rect = canvas.getBoundingClientRect();
  const dpr = window.devicePixelRatio || 1;
  const width = Math.max(1, Math.round(rect.width * dpr));
  const height = Math.max(1, Math.round(rect.height * dpr));
  if (canvas.width !== width || canvas.height !== height) {
    canvas.width = width;
    canvas.height = height;
  }
  const ctx = canvas.getContext('2d');
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  const w = rect.width;
  const h = rect.height;
  ctx.clearRect(0, 0, w, h);

  ctx.strokeStyle = '#e2e7e4';
  ctx.lineWidth = 1;
  for (let i = 1; i < 4; i++) {
    const y = h * i / 4;
    ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke();
  }
  ctx.strokeStyle = '#aeb8b3';
  ctx.beginPath(); ctx.moveTo(0, h / 2); ctx.lineTo(w, h / 2); ctx.stroke();

  const peak = Math.max(0.25, ...Object.values(history).flatMap((series) => series.map(Math.abs)));
  const plot = (data, color) => {
    if (data.length < 2) return;
    ctx.strokeStyle = color;
    ctx.lineWidth = 2;
    ctx.beginPath();
    data.forEach((value, index) => {
      const x = index * w / 119;
      const y = h / 2 - value / peak * (h * 0.42);
      if (index === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
    });
    ctx.stroke();
  };
  plot(history.left, '#147d58');
  plot(history.right, '#167a94');
  plot(history.targetLeft, '#b46a11');
  plot(history.targetRight, '#bd2f2f');
}

{
  if (!('bluetooth' in navigator)) {
    ui.connectionMode.querySelector('option[value="ble"]').disabled = true;
  }
  if (!('serial' in navigator)) ui.connectionMode.querySelector('option[value="serial"]').disabled = true;
  ui.browserStatus.textContent = 'Local Bridge · 8766';
  navigator.serial?.addEventListener('disconnect', (event) => {
    if (event.target === port) disconnect(false);
  });
}

new ResizeObserver(drawChart).observe(ui.chart);
setConnected(false);
drawChart();
