const BAUD_RATE = 9600;
const BRIDGE_URL = 'ws://127.0.0.1:8766';
const MAX_DRIVE_SPEED_MM_S = 600;
const CONTROL_INTERVAL_MS = 200;

const ids = [
  'connectionMode', 'connectionBadge', 'connectionText', 'connectButton',
  'motorBadge', 'stopButton', 'speedSlider', 'speedValue', 'driveCommand',
  'pidLeftKp', 'pidLeftKi', 'pidLeftKd', 'pidRightKp', 'pidRightKi', 'pidRightKd',
  'pidCurrent', 'yawEnabled', 'yawMaxRate', 'yawKp', 'yawKi', 'yawKd', 'yawKff',
  'yawCurrent', 'applyYawButton',
  'headingEnabled', 'headingMaxRate', 'headingKp', 'headingKd', 'headingKff',
  'headingCurrent', 'applyHeadingButton',
  'headingTestAngle', 'headingTestRepeats', 'headingTestTolerance',
  'headingTestRateTolerance', 'headingTestSettleMs', 'headingTestTimeoutMs',
  'startHeadingTestButton', 'stopHeadingTestButton', 'headingTestState',
  'headingTestResult', 'startSquareTestButton', 'stopSquareTestButton', 'squareTestState',
  'lineKp', 'lineKi', 'lineKd', 'lineDiff', 'lineSpeed', 'lineCurrent',
  'applyLineButton', 'startLineButton', 'stopLineButton',
  'leftMotor', 'rightMotor', 'leftMotorValue', 'rightMotorValue', 'motorCommand',
  'runMotorsButton', 'stopMotorsButton', 'zeroEncoderButton', 'zeroImuButton',
  'deadzoneTarget', 'deadzoneVoltage', 'deadzonePwm', 'deadzonePwmValue',
  'deadzoneDecrease', 'deadzoneIncrease', 'deadzoneRunButton', 'deadzoneStopButton',
  'deadzoneCommand', 'deadzoneSpeed', 'deadzoneEncoderDelta', 'recordStartButton',
  'recordRunButton', 'clearDeadzoneButton', 'leftForwardStart', 'leftForwardRun',
  'leftReverseStart', 'leftReverseRun', 'rightForwardStart', 'rightForwardRun',
  'rightReverseStart', 'rightReverseRun',
  'grayWhiteButton', 'grayBlackButton', 'packetTime', 'yawValue', 'pitchValue',
  'rollValue', 'leftSpeed', 'rightSpeed', 'leftTarget', 'rightTarget',
  'yawTarget', 'yawRate', 'yawError', 'yawFeedforward', 'yawPidCorrection',
  'yawCorrection', 'yawLoopState',
  'targetHeading', 'headingReference', 'headingActual', 'headingError',
  'headingOutput', 'headingLoopState',
  'lineState', 'lineRawError', 'lineFilteredError', 'linePidOutput',
  'lineTargetYaw', 'lineActualYaw', 'lineCorrection', 'lineNormalizedSum',
  'leftError', 'rightError', 'leftFeedforward', 'rightFeedforward',
  'leftPidCorrection', 'rightPidCorrection', 'leftEncoder', 'rightEncoder',
  'leftPwm', 'rightPwm', 'linkValue', 'wheeltecValue', 'batteryVoltage',
  'trackModeState', 'localRunState', 'dipSwitchState', 'imuState', 'imuPins',
  'grayGrid', 'grayCalibrationState', 'grayCalibration', 'grayNormalization',
  'accelRaw', 'gyroRaw',
  'speedChart', 'headingChart', 'browserStatus', 'eventLog'
];
const ui = Object.fromEntries(ids.map(id => [id, document.getElementById(id)]));

const DEADZONE_STORAGE_KEY = 'mspm0g3507-motor-deadzone-v1';
const deadzoneTargets = {
  leftForward: { label: '左电机正转', motor: 'left', sign: 1 },
  leftReverse: { label: '左电机反转', motor: 'left', sign: -1 },
  rightForward: { label: '右电机正转', motor: 'right', sign: 1 },
  rightReverse: { label: '右电机反转', motor: 'right', sign: -1 },
};

let socket;
let port;
let reader;
let writer;
let connectedTransport;
let closing = false;
let receiveBuffer = '';
let writeChain = Promise.resolve();
let heartbeatTimer;
let controlTimer;
let controlRunning = false;
let controlGeneration = 0;
let activeDriveButton;
let keyboardControlActive = false;
let deadzoneOutputActive = false;
let deadzoneBaseline;
let deadzoneResults = loadDeadzoneResults();
let yawFormDirty = false;
let pendingYawParameters;
let headingFormDirty = false;
let pendingHeadingParameters;
let headingTestTimer;
let squareControlActive = false;
let lineFormDirty = false;
let pendingLineParameters;
let lineControlActive = false;
let latestLinePid = [200000, 0, 350000];
let latestLineDiff = 650;
let grayWhiteValid = false;
let grayBlackValid = false;
let grayNormalizationValid = false;
let trackColorMode = 0;
let localRunActive = false;
let latestGrayBackground = Array(8).fill(0);
let latestGrayLine = Array(8).fill(0);
const pressedDriveKeys = new Set();
const history = { left: [], right: [], targetLeft: [], targetRight: [] };
const headingHistory = { actual: [], target: [] };
const latestVehicle = {
  leftSpeed: 0,
  rightSpeed: 0,
  leftTarget: 0,
  rightTarget: 0,
  leftEncoder: 0,
  rightEncoder: 0,
  yawDeg: 0,
  yawRate: 0,
};
const latestHeading = {
  target: 0,
  reference: 0,
  referenceRate: 0,
  actual: 0,
  error: 0,
  output: 0,
  enabled: false,
  active: false,
  receivedAt: 0,
};
const headingTest = {
  active: false,
  targets: [],
  index: 0,
  stepStartedAt: 0,
  stableSince: 0,
  stepStartHeading: 0,
  maxOvershoot: 0,
  results: [],
  config: undefined,
};
const latestGrayRaw = Array(8).fill(0);

const grayChannels = Array.from({ length: 8 }, (_, index) => {
  const element = document.createElement('div');
  element.className = 'gray-channel';
  element.innerHTML = `<span>G${index}</span><div class="gray-meter"><i></i></div>`
    + '<div class="gray-values"><strong>R 0</strong><em>N --</em></div>';
  ui.grayGrid.appendChild(element);
  return {
    rawValue: element.querySelector('strong'),
    normalizedValue: element.querySelector('em'),
    fill: element.querySelector('i'),
  };
});

function setLog(message) {
  ui.eventLog.textContent = `${new Date().toLocaleTimeString()}  ${message}`;
}

function transportReady() {
  return Boolean(writer || socket?.readyState === WebSocket.OPEN);
}

function setConnected(connected, label = '') {
  ui.connectionBadge.className = `badge ${connected ? 'connected' : 'disconnected'}`;
  ui.connectionText.textContent = connected ? label : '未连接';
  ui.connectButton.textContent = connected
    ? '断开连接'
    : (ui.connectionMode.value === 'bridge' ? '连接桥接' : '连接串口');
  ui.connectionMode.disabled = connected;
  document.querySelectorAll('button').forEach(button => {
    if (button === ui.connectButton) return;
    button.disabled = !connected;
  });
  ui.applyHeadingButton.disabled = true;
  renderHeadingTest();
  renderSquareTest();
}

async function sendLine(line) {
  if (!transportReady()) return;
  const payload = `${line}\n`;
  writeChain = writeChain.then(async () => {
    if (writer) return writer.write(new TextEncoder().encode(payload));
    if (socket?.readyState === WebSocket.OPEN) socket.send(payload);
  }).catch(error => setLog(`发送失败: ${error.message}`));
  return writeChain;
}

function consumeText(text) {
  receiveBuffer += text;
  const lines = receiveBuffer.replace(/\r/g, '').split('\n');
  receiveBuffer = lines.pop() ?? '';
  lines.filter(Boolean).forEach(parseLine);
}

function parseLine(line) {
  const fields = line.trim().split(',');
  if (fields[0] === 'STATUS') {
    const online = Number(fields[1]) === 1;
    ui.wheeltecValue.textContent = online ? 'WHEELTEC-IOS 已连接' : '等待 WHEELTEC-IOS';
    ui.wheeltecValue.style.color = online ? '#147d58' : '#b46a11';
    setLog(fields.slice(2).join(',') || (online ? '蓝牙已连接' : '蓝牙未连接'));
    if (!online && headingTest.active) finishHeadingTest('蓝牙链路断开', false);
    if (!online && squareControlActive) stopControl(false);
    return;
  }

  const values = fields.slice(1).map(Number);
  if (fields[0] === 'SPD' && values.length >= 20 && values.every(Number.isFinite)) {
    const [time, active, targetLeft, leftMm, errorLeft, feedforwardLeft10,
      pidLeft10, pwmLeft, targetRight, rightMm, errorRight, feedforwardRight10,
      pidRight10, pwmRight, kpLeft, kiLeft, kdLeft, kpRight, kiRight, kdRight] = values;
    latestVehicle.leftSpeed = leftMm;
    latestVehicle.rightSpeed = rightMm;
    latestVehicle.leftTarget = targetLeft;
    latestVehicle.rightTarget = targetRight;
    ui.packetTime.textContent = `${time} ms`;
    ui.leftSpeed.textContent = `${leftMm} mm/s`;
    ui.rightSpeed.textContent = `${rightMm} mm/s`;
    ui.leftTarget.textContent = `${targetLeft} mm/s`;
    ui.rightTarget.textContent = `${targetRight} mm/s`;
    ui.leftError.textContent = `${errorLeft} mm/s`;
    ui.rightError.textContent = `${errorRight} mm/s`;
    ui.leftFeedforward.textContent = `${(feedforwardLeft10 / 10).toFixed(1)}%`;
    ui.rightFeedforward.textContent = `${(feedforwardRight10 / 10).toFixed(1)}%`;
    ui.leftPidCorrection.textContent = `${(pidLeft10 / 10).toFixed(1)}%`;
    ui.rightPidCorrection.textContent = `${(pidRight10 / 10).toFixed(1)}%`;
    ui.leftPwm.textContent = `${pwmLeft}%`;
    ui.rightPwm.textContent = `${pwmRight}%`;
    ui.motorBadge.className = `badge ${active ? 'enabled' : 'neutral'}`;
    ui.motorBadge.innerHTML = `<span class="dot"></span>${active ? '速度闭环运行中' : '电机已停止'}`;
    updatePidReadback([kpLeft, kiLeft, kdLeft], [kpRight, kiRight, kdRight]);
    if (values.length >= 32) {
      const [targetYaw10, actualYaw10, yawError10, yawFeedforwardMm,
        yawPidMm, yawCorrectionMm, yawEnabled, maxYawRate,
        yawKp, yawKi, yawKd, yawKff] = values.slice(20, 32);
      updateYawTelemetry(targetYaw10, actualYaw10, yawError10,
        yawFeedforwardMm, yawPidMm, yawCorrectionMm, yawEnabled);
      updateYawReadback(yawEnabled, maxYawRate, [yawKp, yawKi, yawKd, yawKff]);
      latestVehicle.yawRate = actualYaw10 / 10;
    }
    if (values.length >= 44) {
      const [targetHeading10, referenceHeading10, referenceRate10,
        actualHeading10, headingError10, headingOutput10,
        headingEnabled, headingActive, headingMaxRate,
        headingKp, headingKd, headingKff] = values.slice(32, 44);
      updateHeadingTelemetry(targetHeading10, referenceHeading10,
        referenceRate10, actualHeading10, headingError10, headingOutput10,
        headingEnabled, headingActive);
      updateHeadingReadback(headingEnabled, headingMaxRate,
        [headingKp, headingKd, headingKff]);
    }
    pushHistory(leftMm, rightMm, targetLeft, targetRight);
    updateDeadzoneLive();
    return;
  }

  if (fields[0] === 'TEL' && values.length >= 23 && values.every(Number.isFinite)) {
    const [time, enabled, link, yaw10, leftMm, rightMm,
      targetLeft, targetRight, pwmLeft, pwmRight] = values;
    latestVehicle.leftSpeed = leftMm;
    latestVehicle.rightSpeed = rightMm;
    latestVehicle.leftTarget = targetLeft;
    latestVehicle.rightTarget = targetRight;
    latestVehicle.yawDeg = yaw10 / 10;
    latestVehicle.yawRate = values[11] / 10;
    ui.packetTime.textContent = `${time} ms`;
    ui.yawValue.textContent = `${(yaw10 / 10).toFixed(1)}°`;
    ui.leftSpeed.textContent = `${leftMm} mm/s`;
    ui.rightSpeed.textContent = `${rightMm} mm/s`;
    ui.leftTarget.textContent = `${targetLeft} mm/s`;
    ui.rightTarget.textContent = `${targetRight} mm/s`;
    ui.leftError.textContent = `${targetLeft - leftMm} mm/s`;
    ui.rightError.textContent = `${targetRight - rightMm} mm/s`;
    ui.leftPwm.textContent = `${Math.round(pwmLeft / 168)}%`;
    ui.rightPwm.textContent = `${Math.round(pwmRight / 168)}%`;
    updateYawTelemetry(values[10], values[11], values[12], values[16],
      values[13] - values[16], values[13], values[14]);
    updateHeadingTelemetry(values[17], undefined, undefined, yaw10,
      values[18], values[19], values[20], values[21]);
    ui.linkValue.textContent = link ? '控制中' : '待机';
    ui.imuState.textContent = values[15] ? '0x6B 正常' : '未检测到';
    ui.imuState.style.color = values[15] ? '#147d58' : '#bd2f2f';
    ui.motorBadge.className = `badge ${enabled ? 'enabled' : 'neutral'}`;
    ui.motorBadge.innerHTML = `<span class="dot"></span>${enabled ? '电机运行中' : '电机已停止'}`;
    if (values[22] > 0) ui.batteryVoltage.textContent = `${(values[22] / 1000).toFixed(2)} V`;
    if (!enabled) {
      ui.leftFeedforward.textContent = '0.0%';
      ui.rightFeedforward.textContent = '0.0%';
      ui.leftPidCorrection.textContent = '0.0%';
      ui.rightPidCorrection.textContent = '0.0%';
    }
    pushHistory(leftMm, rightMm, targetLeft, targetRight);
    updateDeadzoneLive();
    return;
  }

  if (fields[0] === 'STA' && values.length >= 30 && values.every(Number.isFinite)) {
    latestVehicle.leftEncoder = values[3];
    latestVehicle.rightEncoder = values[4];
    ui.pitchValue.textContent = `${(values[1] / 10).toFixed(1)}°`;
    ui.rollValue.textContent = `${(values[2] / 10).toFixed(1)}°`;
    ui.leftEncoder.textContent = values[3].toLocaleString();
    ui.rightEncoder.textContent = values[4].toLocaleString();
    updatePidReadback(values.slice(5, 8), values.slice(8, 11));
    if (values.length >= 50) {
      updateYawReadback(values[44], values[45], values.slice(46, 50));
    }
    if (values.length >= 54) {
      updateLineReadback(values.slice(50, 53), values[53]);
    }
    if (values.length >= 59) {
      updateHeadingReadback(values[54], values[55], values.slice(56, 59));
    }
    values.slice(22, 30).forEach(updateGray);
    grayWhiteValid = Boolean(values[32]);
    grayBlackValid = Boolean(values[33]);
    renderGrayCalibrationState();
    updateDeadzoneLive();
    return;
  }

  if (fields[0] === 'SQR' && values.length >= 7 && values.every(Number.isFinite)) {
    const [, active, phase, leg, progressMm, remainingMm, targetHeading10] = values;
    const phaseNames = ['待机', '直行', '左转', '完成', '异常停止'];
    if (active) {
      ui.squareTestState.textContent = `${phaseNames[phase] ?? '未知'} ${leg}/4 · `
        + `${progressMm} mm · 余 ${remainingMm} mm · ${Number(targetHeading10 / 10).toFixed(1)}°`;
    } else if (phase === 3 || phase === 4) {
      if (squareControlActive) stopControl(false);
      ui.squareTestState.textContent = phaseNames[phase];
    } else {
      ui.squareTestState.textContent = '待机';
    }
    renderSquareTest();
    return;
  }

  if (fields[0] === 'DBG' && values.length >= 7 && values.every(Number.isFinite)) {
    ui.accelRaw.textContent = `${values[1]} / ${values[2]} / ${values[3]}`;
    ui.gyroRaw.textContent = `${values[4]} / ${values[5]} / ${values[6]}`;
    if (values.length >= 9) ui.imuPins.textContent = `MOSI PA${values[7]} · MISO PA${values[8]}`;
    return;
  }

  if (fields[0] === 'CAL' && values.length >= 16 && values.every(Number.isFinite)) {
    latestGrayBackground = values.slice(0, 8);
    latestGrayLine = values.slice(8, 16);
    if (values.length >= 17) trackColorMode = values[16];
    renderTrackMode();
    renderGrayReferences();
    return;
  }

  if (fields[0] === 'NRM' && values.length >= 10 && values.every(Number.isFinite)) {
    grayNormalizationValid = Boolean(values[1]);
    const normalized = values.slice(2, 10);
    if (values.length >= 11) trackColorMode = values[10];
    normalized.forEach(updateGrayNormalized);
    ui.grayNormalization.textContent = `归一化 ${normalized.join('/')}`;
    renderGrayCalibrationState();
    renderTrackMode();
    return;
  }

  if (fields[0] === 'MOD' && values.length >= 8 && values.every(Number.isFinite)) {
    const [time, mode, switch1Down, switch2Down, localRun,
      backgroundValid, lineValid, batteryMv] = values;
    trackColorMode = mode;
    localRunActive = Boolean(localRun);
    if (localRunActive && (controlRunning || headingTest.active)) stopControl(false);
    grayWhiteValid = Boolean(backgroundValid);
    grayBlackValid = Boolean(lineValid);
    ui.packetTime.textContent = `${time} ms`;
    ui.dipSwitchState.textContent = `SW1 ${switch1Down ? '下' : '上'} · SW2 ${switch2Down ? '下' : '上'}`;
    ui.localRunState.textContent = localRunActive ? '运行中' : '待机';
    ui.localRunState.style.color = localRunActive ? '#147d58' : '';
    if (batteryMv > 0) ui.batteryVoltage.textContent = `${(batteryMv / 1000).toFixed(2)} V`;
    renderTrackMode();
    renderGrayReferences();
    renderGrayCalibrationState();
    return;
  }

  if (fields[0] === 'LIN' && values.length >= 16 && values.every(Number.isFinite)) {
    const [time, active, normalized, visible, lost, rawError10,
      filteredError10, pidOutput, targetYaw10, actualYaw10, correctionMm,
      targetLeft, targetRight, baseSpeed, diffMilli, normalizedSum] = values;
    const lineMode = values.length >= 17 ? values[16] : (lost ? 4 : (visible ? 1 : 2));
    const recoveryMs = values.length >= 18 ? values[17] : 0;
    const activeCount = values.length >= 19 ? values[18] : 0;
    lineControlActive = Boolean(active);
    ui.packetTime.textContent = `${time} ms`;
    const modeLabels = ['等待目标线', '正常跟踪', '短缝保持', '盲转找线', '确认丢线'];
    ui.lineState.textContent = `${modeLabels[lineMode] ?? '未知状态'}`
      + ((lineMode === 2 || lineMode === 3) ? ` · ${recoveryMs} ms` : '');
    ui.lineState.style.color = lineMode === 4 ? '#bd2f2f'
      : (lineMode === 1 ? '#147d58' : '#b46a11');
    ui.lineRawError.textContent = `${(rawError10 / 10).toFixed(1)}%`;
    ui.lineFilteredError.textContent = `${(filteredError10 / 10).toFixed(1)}%`;
    ui.linePidOutput.textContent = Math.round(pidOutput).toLocaleString();
    ui.lineTargetYaw.textContent = `${(targetYaw10 / 10).toFixed(1)}°/s`;
    ui.lineActualYaw.textContent = `${(actualYaw10 / 10).toFixed(1)}°/s`;
    ui.lineCorrection.textContent = `${correctionMm} mm/s · 上限 ${formatMilli(diffMilli)}`;
    ui.lineNormalizedSum.textContent = `${normalizedSum} · ${activeCount} 路目标`
      + (normalized ? '' : ' · 未归一化');
    latestVehicle.leftTarget = targetLeft;
    latestVehicle.rightTarget = targetRight;
    ui.leftTarget.textContent = `${targetLeft} mm/s`;
    ui.rightTarget.textContent = `${targetRight} mm/s`;
    ui.yawTarget.textContent = `${(targetYaw10 / 10).toFixed(1)}°/s`;
    ui.yawRate.textContent = `${(actualYaw10 / 10).toFixed(1)}°/s`;
    ui.driveCommand.textContent = `LINE,1,${baseSpeed}`;
    renderLineCurrent();
    return;
  }

  if (fields[0] === 'BOOT' || fields[0] === 'ACK' || fields[0] === 'ERR') {
    if (fields[0] === 'ACK' && fields[1] === 'HEADCFG') {
      const headingValues = fields.slice(2).map(Number);
      if (headingValues.length >= 5 && headingValues.every(Number.isFinite)) {
        updateHeadingReadback(headingValues[0], headingValues[4],
          headingValues.slice(1, 4));
      }
    }
    if (fields[0] === 'ERR' && fields[1]?.startsWith('LINE_')) {
      stopControl(false);
    }
    if (fields[0] === 'ERR' && fields[1]?.startsWith('HEAD')) {
      finishHeadingTest(line, true);
    }
    setLog(line);
  }
}

function updateGray(value, index) {
  const raw = Math.max(0, Math.min(4095, value));
  latestGrayRaw[index] = raw;
  grayChannels[index].rawValue.textContent = `R ${Math.round(raw)}`;
  if (!grayNormalizationValid) {
    grayChannels[index].fill.style.height = `${raw * 100 / 4095}%`;
  }
}

function updateGrayNormalized(value, index) {
  const normalized = Math.max(0, Math.min(1000, value));
  grayChannels[index].normalizedValue.textContent = grayNormalizationValid
    ? `N ${Math.round(normalized)}` : 'N --';
  if (grayNormalizationValid) {
    grayChannels[index].fill.style.height = `${normalized / 10}%`;
  }
}

function renderGrayCalibrationState() {
  let label = '未标定';
  let stateClass = '';
  if (grayNormalizationValid) {
    label = '归一化有效';
    stateClass = 'complete';
  } else if (grayWhiteValid && grayBlackValid) {
    label = '归一化无效';
    stateClass = 'invalid';
  } else if (grayWhiteValid) {
    label = trackColorMode === 0 ? '已采白底' : '已采蓝底';
  } else if (grayBlackValid) {
    label = trackColorMode === 0 ? '已采黑线' : '已采白线';
  }
  ui.grayCalibrationState.textContent = label;
  ui.grayCalibrationState.className = stateClass;
  if (!grayNormalizationValid) {
    grayChannels.forEach((channel, index) => {
      channel.fill.style.height = `${latestGrayRaw[index] * 100 / 4095}%`;
    });
  }
}

function renderTrackMode() {
  const blueWhite = trackColorMode === 1;
  ui.trackModeState.textContent = blueWhite ? '蓝底白线' : '白底黑线';
  ui.grayWhiteButton.textContent = blueWhite ? '采集蓝底' : '采集白底';
  ui.grayBlackButton.textContent = blueWhite ? '采集白线' : '采集黑线';
}

function renderGrayReferences() {
  const backgroundName = trackColorMode === 1 ? '蓝底' : '白底';
  const lineName = trackColorMode === 1 ? '白线' : '黑线';
  ui.grayCalibration.textContent = `${backgroundName} ${latestGrayBackground.join('/')}`
    + ` · ${lineName} ${latestGrayLine.join('/')}`;
}

async function connectBridge() {
  socket = new WebSocket(BRIDGE_URL);
  await new Promise((resolve, reject) => {
    const timeout = setTimeout(() => reject(new Error('桥接未运行')), 3000);
    socket.addEventListener('open', () => { clearTimeout(timeout); resolve(); }, { once: true });
    socket.addEventListener('error', () => { clearTimeout(timeout); reject(new Error('无法连接本地桥接')); }, { once: true });
  });
  socket.addEventListener('message', event => consumeText(String(event.data)));
  socket.addEventListener('close', () => { if (!closing) disconnect(false); });
  connectedTransport = 'bridge';
  setConnected(true, '本地桥接已连接');
}

async function readSerialLoop() {
  const decoder = new TextDecoder();
  reader = port.readable.getReader();
  try {
    while (!closing) {
      const { value, done } = await reader.read();
      if (done) break;
      consumeText(decoder.decode(value, { stream: true }));
    }
  } catch (error) {
    if (!closing) setLog(`串口接收中断: ${error.message}`);
  } finally {
    if (!closing) disconnect(false);
  }
}

async function connectSerial() {
  if (!navigator.serial) throw new Error('浏览器不支持 Web Serial');
  port = await navigator.serial.requestPort();
  await port.open({ baudRate: BAUD_RATE, bufferSize: 1024 });
  writer = port.writable.getWriter();
  connectedTransport = 'serial';
  setConnected(true, `串口 ${BAUD_RATE}`);
  readSerialLoop();
}

async function connect() {
  try {
    closing = false;
    if (ui.connectionMode.value === 'bridge') await connectBridge();
    else await connectSerial();
    setLog('上位机连接成功');
    await sendLine('STOP');
    await sendLine('GRAYCAL');
    await sendLine('HEADCFG');
    heartbeatTimer = setInterval(() => sendLine('PING'), 1000);
  } catch (error) {
    setLog(`连接失败: ${error.message}`);
    await disconnect(false);
  }
}

async function disconnect(sendStop = true) {
  if (closing) return;
  closing = true;
  stopControl(false);
  clearInterval(heartbeatTimer);
  try { if (sendStop) await sendLine('STOP'); } catch (_) {}
  try { await reader?.cancel(); } catch (_) {}
  try { reader?.releaseLock(); } catch (_) {}
  try { writer?.releaseLock(); } catch (_) {}
  reader = undefined;
  writer = undefined;
  try { await port?.close(); } catch (_) {}
  port = undefined;
  try { socket?.close(); } catch (_) {}
  socket = undefined;
  connectedTransport = undefined;
  ui.wheeltecValue.textContent = '桥接未连接';
  ui.wheeltecValue.style.color = '';
  setConnected(false);
  closing = false;
  setLog('连接已断开');
}

async function controlLoop(generation, commandFactory) {
  if (!controlRunning || generation !== controlGeneration || !transportReady()) return;
  const command = commandFactory();
  ui.driveCommand.textContent = command;
  await sendLine(command);
  if (controlRunning && generation === controlGeneration && transportReady()) {
    controlTimer = setTimeout(() => controlLoop(generation, commandFactory), CONTROL_INTERVAL_MS);
  }
}

function startControl(commandFactory, button) {
  if (!transportReady()) return;
  stopControl(false);
  controlRunning = true;
  activeDriveButton = button;
  activeDriveButton?.classList.add('active');
  const generation = controlGeneration;
  controlLoop(generation, commandFactory);
}

function stopControl(sendStop = true) {
  if (headingTest.active) finishHeadingTest('已停止', false);
  controlGeneration++;
  controlRunning = false;
  clearTimeout(controlTimer);
  activeDriveButton?.classList.remove('active');
  activeDriveButton = undefined;
  keyboardControlActive = false;
  pressedDriveKeys.clear();
  updateKeyboardButtons();
  deadzoneOutputActive = false;
  lineControlActive = false;
  squareControlActive = false;
  renderLineCurrent();
  renderSquareTest();
  ui.driveCommand.textContent = 'STOP';
  if (sendStop && transportReady()) sendLine('STOP');
}

function driveCommandFactory(direction) {
  return () => {
    const speed = Number(ui.speedSlider.value);
    if (direction === 'forward') return `DRV,${speed},0`;
    if (direction === 'back') return `DRV,${-speed},0`;
    if (direction === 'left') return `DRV,0,${-speed}`;
    return `DRV,0,${speed}`;
  };
}

function keyboardDriveCommand() {
  const speed = Number(ui.speedSlider.value);
  const throttle = ((pressedDriveKeys.has('w') ? 1 : 0) - (pressedDriveKeys.has('s') ? 1 : 0)) * speed;
  const steering = ((pressedDriveKeys.has('d') ? 1 : 0) - (pressedDriveKeys.has('a') ? 1 : 0)) * speed;
  return `DRV,${throttle},${steering}`;
}

function updateKeyboardButtons() {
  document.querySelectorAll('.drive-button[data-key]').forEach(button => {
    button.classList.toggle('active', keyboardControlActive && pressedDriveKeys.has(button.dataset.key));
  });
}

function startKeyboardControl(key) {
  if (!transportReady()) return;
  if (!keyboardControlActive) {
    stopControl(false);
    pressedDriveKeys.add(key);
    keyboardControlActive = true;
    controlRunning = true;
    const generation = controlGeneration;
    controlLoop(generation, keyboardDriveCommand);
  } else {
    pressedDriveKeys.add(key);
    const command = keyboardDriveCommand();
    ui.driveCommand.textContent = command;
    sendLine(command);
  }
  updateKeyboardButtons();
}

function releaseKeyboardControl(key) {
  if (!pressedDriveKeys.delete(key)) return;
  if (pressedDriveKeys.size === 0) {
    stopControl();
    return;
  }
  const command = keyboardDriveCommand();
  ui.driveCommand.textContent = command;
  sendLine(command);
  updateKeyboardButtons();
}

function isEditableControl(target) {
  return target instanceof Element && Boolean(target.closest('input, textarea, select, [contenteditable="true"]'));
}

function updateMotorInputs() {
  ui.leftMotorValue.textContent = `${ui.leftMotor.value}%`;
  ui.rightMotorValue.textContent = `${ui.rightMotor.value}%`;
  ui.motorCommand.textContent = `L ${ui.leftMotor.value}% · R ${ui.rightMotor.value}%`;
}

function updateSpeedTargetDisplay() {
  const percent = Number(ui.speedSlider.value);
  const speed = Math.round(percent * MAX_DRIVE_SPEED_MM_S / 100);
  ui.speedValue.textContent = `${percent}% · ${speed} mm/s`;
}

function updatePidReadback(left, right) {
  const normalizedLeft = left.map(value => Math.round(Number(value)));
  const normalizedRight = right.map(value => Math.round(Number(value)));
  const values = [...normalizedLeft, ...normalizedRight];
  if (!values.every(Number.isFinite)) return;
  ui.pidCurrent.textContent = `L ${normalizedLeft.join('/')} · R ${normalizedRight.join('/')} · 已锁定`;
  const inputs = [ui.pidLeftKp, ui.pidLeftKi, ui.pidLeftKd,
    ui.pidRightKp, ui.pidRightKi, ui.pidRightKd];
  inputs.forEach((input, index) => {
    input.value = values[index];
  });
}

function updateYawTelemetry(target10, actual10, error10, feedforwardMm,
  pidMm, correctionMm, enabled) {
  ui.yawTarget.textContent = `${(target10 / 10).toFixed(1)}°/s`;
  ui.yawRate.textContent = `${(actual10 / 10).toFixed(1)}°/s`;
  ui.yawError.textContent = `${(error10 / 10).toFixed(1)}°/s`;
  ui.yawFeedforward.textContent = `${Math.round(feedforwardMm)} mm/s`;
  ui.yawPidCorrection.textContent = `${Math.round(pidMm)} mm/s`;
  ui.yawCorrection.textContent = `${Math.round(correctionMm)} mm/s`;
  ui.yawLoopState.textContent = enabled ? '已启用' : '已关闭';
  ui.yawLoopState.style.color = enabled ? '#147d58' : '#b46a11';
}

function updateYawReadback(enabled, maxRate, pid) {
  const normalizedPid = pid.map(value => Math.round(Number(value)));
  const normalizedMaxRate = Math.round(Number(maxRate));
  const normalizedEnabled = Boolean(Number(enabled));
  if (![normalizedMaxRate, ...normalizedPid].every(Number.isFinite)) return;
  const matchesPending = pendingYawParameters
    && pendingYawParameters.enabled === normalizedEnabled
    && pendingYawParameters.maxRate === normalizedMaxRate
    && normalizedPid.every((value, index) => value === pendingYawParameters.pid[index]);
  if (matchesPending) {
    pendingYawParameters = undefined;
    yawFormDirty = false;
  }
  const state = pendingYawParameters ? ' · 等待确认' : (yawFormDirty ? ' · 待应用' : '');
  ui.yawCurrent.textContent = `${normalizedPid.join('/')} · ${normalizedMaxRate}°/s${state}`;
  if (pendingYawParameters || yawFormDirty) return;
  ui.yawEnabled.checked = normalizedEnabled;
  ui.yawMaxRate.value = normalizedMaxRate;
  [ui.yawKp, ui.yawKi, ui.yawKd, ui.yawKff].forEach((input, index) => {
    input.value = normalizedPid[index];
  });
}

function readYawInputs() {
  const pid = [ui.yawKp, ui.yawKi, ui.yawKd, ui.yawKff]
    .map(input => Math.round(Number(input.value)));
  const maxRate = Math.round(Number(ui.yawMaxRate.value));
  if (!pid.every(value => Number.isFinite(value) && value >= 0 && value <= 100000)
    || !Number.isFinite(maxRate) || maxRate < 10 || maxRate > 360) return undefined;
  return { enabled: ui.yawEnabled.checked, maxRate, pid };
}

async function applyYawParameters() {
  const parameters = readYawInputs();
  if (!parameters) {
    setLog('角速度参数超出允许范围');
    return;
  }
  pendingYawParameters = { ...parameters, pid: [...parameters.pid] };
  yawFormDirty = true;
  await sendLine(`YAWPID,${parameters.pid.join(',')}`);
  await sendLine(`YAWRATE,${parameters.maxRate}`);
  await sendLine(`YAW,${parameters.enabled ? 1 : 0}`);
  setLog(`已发送角速度 PID ${parameters.pid.join('/')} · ${parameters.maxRate}°/s`);
}

function wrapAngle(value) {
  let angle = Number(value);
  while (angle > 180) angle -= 360;
  while (angle < -180) angle += 360;
  return angle;
}

function formatHeadingMilli(value) {
  return (Number(value) / 1000).toFixed(3);
}

function updateHeadingTelemetry(target10, reference10, referenceRate10,
  actual10, error10, output10, enabled, active) {
  const target = Number(target10) / 10;
  const actual = Number(actual10) / 10;
  const error = Number(error10) / 10;
  const output = Number(output10) / 10;
  if (![target, actual, error, output].every(Number.isFinite)) return;

  latestHeading.target = target;
  latestHeading.actual = actual;
  latestHeading.error = error;
  latestHeading.output = output;
  latestHeading.enabled = Boolean(Number(enabled));
  latestHeading.active = Boolean(Number(active));
  latestHeading.receivedAt = performance.now();
  latestVehicle.yawDeg = actual;
  if (Number.isFinite(Number(reference10))) {
    latestHeading.reference = Number(reference10) / 10;
  }
  if (Number.isFinite(Number(referenceRate10))) {
    latestHeading.referenceRate = Number(referenceRate10) / 10;
  }

  ui.targetHeading.textContent = `${target.toFixed(1)}°`;
  ui.headingReference.textContent = `${latestHeading.reference.toFixed(1)}°`;
  ui.headingActual.textContent = `${actual.toFixed(1)}°`;
  ui.headingError.textContent = `${error.toFixed(1)}°`;
  ui.headingOutput.textContent = `${output.toFixed(1)}°/s`;
  ui.headingLoopState.textContent = latestHeading.active
    ? '保持中' : (latestHeading.enabled ? '已启用' : '已关闭');
  ui.headingLoopState.style.color = latestHeading.active ? '#147d58'
    : (latestHeading.enabled ? '#b46a11' : '#bd2f2f');
  pushHeadingHistory(actual, target);
}

function updateHeadingReadback(enabled, maxRate, pidMilli) {
  const normalizedPid = pidMilli.map(value => Math.round(Number(value)));
  const normalizedMaxRate = Math.round(Number(maxRate));
  const normalizedEnabled = Boolean(Number(enabled));
  if (![normalizedMaxRate, ...normalizedPid].every(Number.isFinite)) return;

  const matchesPending = pendingHeadingParameters
    && pendingHeadingParameters.enabled === normalizedEnabled
    && pendingHeadingParameters.maxRate === normalizedMaxRate
    && normalizedPid.every((value, index) => value === pendingHeadingParameters.pid[index]);
  if (matchesPending) {
    pendingHeadingParameters = undefined;
    headingFormDirty = false;
  }
  const state = pendingHeadingParameters ? ' · 等待确认'
    : (headingFormDirty ? ' · 待应用' : '');
  ui.headingCurrent.textContent = `${normalizedPid.map(formatHeadingMilli).join('/')}`
    + ` · ${normalizedMaxRate}°/s · 已锁定${state}`;
  if (pendingHeadingParameters || headingFormDirty) return;

  ui.headingEnabled.checked = normalizedEnabled;
  ui.headingMaxRate.value = normalizedMaxRate;
  [ui.headingKp, ui.headingKd, ui.headingKff].forEach((input, index) => {
    input.value = formatHeadingMilli(normalizedPid[index]);
  });
}

function readHeadingInputs() {
  const gains = [ui.headingKp, ui.headingKd, ui.headingKff]
    .map(input => Number(input.value));
  const maxRate = Math.round(Number(ui.headingMaxRate.value));
  if (!Number.isFinite(gains[0]) || gains[0] < 0 || gains[0] > 20
    || !Number.isFinite(gains[1]) || gains[1] < 0 || gains[1] > 10
    || !Number.isFinite(gains[2]) || gains[2] < 0 || gains[2] > 2
    || !Number.isFinite(maxRate) || maxRate < 5 || maxRate > 360) return undefined;
  return {
    enabled: ui.headingEnabled.checked,
    maxRate,
    pid: gains.map(value => Math.round(value * 1000)),
  };
}

async function applyHeadingParameters() {
  if (controlRunning || headingTest.active) stopControl();
  await sendLine('HEAD,1');
  await sendLine('HEADCFG');
  setLog('遥控方向环已启用 · 参数锁定 4.000/0.300/1.000 · 80°/s');
  return true;
}

function readHeadingTestInputs() {
  const config = {
    angle: Number(ui.headingTestAngle.value),
    repeats: Math.round(Number(ui.headingTestRepeats.value)),
    tolerance: Number(ui.headingTestTolerance.value),
    rateTolerance: Number(ui.headingTestRateTolerance.value),
    settleMs: Math.round(Number(ui.headingTestSettleMs.value)),
    timeoutMs: Math.round(Number(ui.headingTestTimeoutMs.value)),
  };
  if (!Number.isFinite(config.angle) || config.angle < 5 || config.angle > 90
    || !Number.isFinite(config.repeats) || config.repeats < 1 || config.repeats > 5
    || !Number.isFinite(config.tolerance) || config.tolerance < 0.5 || config.tolerance > 10
    || !Number.isFinite(config.rateTolerance) || config.rateTolerance < 1 || config.rateTolerance > 30
    || !Number.isFinite(config.settleMs) || config.settleMs < 200 || config.settleMs > 3000
    || !Number.isFinite(config.timeoutMs) || config.timeoutMs < 2000 || config.timeoutMs > 20000) {
    return undefined;
  }
  return config;
}

function renderHeadingTest() {
  ui.startHeadingTestButton.disabled = !transportReady() || headingTest.active;
  ui.stopHeadingTestButton.disabled = !transportReady() || !headingTest.active;
}

function renderSquareTest() {
  ui.startSquareTestButton.disabled = !transportReady() || squareControlActive;
  ui.stopSquareTestButton.disabled = !transportReady() || !squareControlActive;
}

function startSquareTest() {
  if (!transportReady() || squareControlActive) return;
  let firstCommand = true;
  startControl(() => {
    if (firstCommand) {
      firstCommand = false;
      return 'SQUARE,1';
    }
    return 'SQUARE,2';
  }, ui.startSquareTestButton);
  squareControlActive = true;
  ui.squareTestState.textContent = '准备';
  renderSquareTest();
  setLog('正方形测试启动 · 1 m直行 + 左转90°，共4次');
}

function beginHeadingTestStep() {
  headingTest.stepStartedAt = performance.now();
  headingTest.stableSince = 0;
  headingTest.stepStartHeading = latestHeading.actual;
  headingTest.maxOvershoot = 0;
  ui.headingTestState.textContent =
    `步骤 ${headingTest.index + 1}/${headingTest.targets.length}`;
  runHeadingTestStep();
}

async function runHeadingTestStep() {
  if (!headingTest.active) return;
  if (!transportReady()) {
    finishHeadingTest('连接已断开', false);
    return;
  }

  const now = performance.now();
  const target = headingTest.targets[headingTest.index];
  const config = headingTest.config;
  if (now - latestHeading.receivedAt > 1800) {
    if (now - headingTest.stepStartedAt > config.timeoutMs) {
      finishHeadingTest('遥测超时', true);
      return;
    }
  } else {
    const direction = Math.sign(wrapAngle(target - headingTest.stepStartHeading)) || 1;
    const overshoot = direction * wrapAngle(latestHeading.actual - target);
    headingTest.maxOvershoot = Math.max(headingTest.maxOvershoot, overshoot, 0);
    if (Math.abs(latestHeading.error) <= config.tolerance
      && Math.abs(latestVehicle.yawRate) <= config.rateTolerance) {
      if (!headingTest.stableSince) headingTest.stableSince = now;
      if (now - headingTest.stableSince >= config.settleMs) {
        const settleMs = Math.round(now - headingTest.stepStartedAt);
        headingTest.results.push({ target, settleMs, overshoot: headingTest.maxOvershoot });
        ui.headingTestResult.textContent = `最近 ${settleMs} ms · 超调 ${headingTest.maxOvershoot.toFixed(1)}°`;
        headingTest.index++;
        if (headingTest.index >= headingTest.targets.length) {
          const worst = Math.max(0, ...headingTest.results.map(result => result.overshoot));
          finishHeadingTest(`完成 · 最大超调 ${worst.toFixed(1)}°`, true);
          return;
        }
        beginHeadingTestStep();
        return;
      }
    } else {
      headingTest.stableSince = 0;
    }
  }

  if (now - headingTest.stepStartedAt > config.timeoutMs) {
    finishHeadingTest(`步骤 ${headingTest.index + 1} 超时`, true);
    return;
  }
  await sendLine(`HEADSET,${Math.round(target * 10)}`);
  headingTestTimer = setTimeout(runHeadingTestStep, CONTROL_INTERVAL_MS);
}

async function startHeadingTest() {
  const config = readHeadingTestInputs();
  if (!transportReady() || !config) {
    setLog(config ? '方向测试尚未连接' : '方向测试参数超出允许范围');
    return;
  }
  if (!latestHeading.receivedAt || performance.now() - latestHeading.receivedAt > 2000) {
    setLog('没有新鲜航向遥测，不能开始测试');
    return;
  }

  stopControl();
  if (!await applyHeadingParameters()) {
    setLog('请先启用方向保持');
    return;
  }
  const base = latestHeading.actual;
  const targets = [];
  for (let repeat = 0; repeat < config.repeats; repeat++) {
    targets.push(wrapAngle(base + config.angle));
    targets.push(wrapAngle(base - config.angle));
  }
  targets.push(base);
  Object.values(headingHistory).forEach(series => { series.length = 0; });
  headingTest.active = true;
  headingTest.targets = targets;
  headingTest.index = 0;
  headingTest.results = [];
  headingTest.config = config;
  ui.headingTestResult.textContent = '--';
  renderHeadingTest();
  beginHeadingTestStep();
}

function finishHeadingTest(message, sendStop = true) {
  const wasActive = headingTest.active;
  clearTimeout(headingTestTimer);
  headingTest.active = false;
  headingTest.stableSince = 0;
  ui.headingTestState.textContent = message;
  renderHeadingTest();
  if (sendStop && wasActive && transportReady()) sendLine('STOP');
  if (wasActive) setLog(`方向自动测试: ${message}`);
}

function formatMilli(value) {
  return (Number(value) / 1000).toFixed(3);
}

function renderLineCurrent() {
  const state = pendingLineParameters ? ' · 等待确认' : (lineFormDirty ? ' · 待应用' : '');
  const mode = lineControlActive ? '运行中' : '未运行';
  ui.lineCurrent.textContent = `${latestLinePid.map(formatMilli).join('/')} · 差速 ${formatMilli(latestLineDiff)} · ${mode}${state}`;
}

function updateLineReadback(pidMilli, diffMilli) {
  const normalizedPid = pidMilli.map(value => Math.round(Number(value)));
  const normalizedDiff = Math.round(Number(diffMilli));
  if (![...normalizedPid, normalizedDiff].every(Number.isFinite)) return;

  const matchesPending = pendingLineParameters
    && normalizedDiff === pendingLineParameters.diff
    && normalizedPid.every((value, index) => value === pendingLineParameters.pid[index]);
  if (matchesPending) {
    pendingLineParameters = undefined;
    lineFormDirty = false;
  }
  latestLinePid = normalizedPid;
  latestLineDiff = normalizedDiff;
  renderLineCurrent();
  if (pendingLineParameters || lineFormDirty) return;

  [ui.lineKp, ui.lineKi, ui.lineKd].forEach((input, index) => {
    input.value = formatMilli(normalizedPid[index]);
  });
  ui.lineDiff.value = formatMilli(normalizedDiff);
}

function readLineInputs() {
  const gains = [ui.lineKp, ui.lineKi, ui.lineKd].map(input => Number(input.value));
  const diff = Number(ui.lineDiff.value);
  if (!gains.every(value => Number.isFinite(value) && value >= 0 && value <= 1000)
    || !Number.isFinite(diff) || diff < 0 || diff > 1) return undefined;
  return {
    pid: gains.map(value => Math.round(value * 1000)),
    diff: Math.round(diff * 1000),
  };
}

async function applyLineParameters() {
  const parameters = readLineInputs();
  if (!parameters) {
    setLog('寻线参数超出允许范围');
    return;
  }
  if (controlRunning) stopControl();
  pendingLineParameters = { pid: [...parameters.pid], diff: parameters.diff };
  lineFormDirty = true;
  await sendLine(`LINEPID,${parameters.pid.join(',')}`);
  await sendLine(`LINEDIFF,${parameters.diff}`);
  setLog(`已发送寻线 PID ${parameters.pid.map(formatMilli).join('/')} · 差速 ${formatMilli(parameters.diff)}`);
}

function startLineControl() {
  const speed = Math.round(Number(ui.lineSpeed.value));
  if (!Number.isFinite(speed) || speed < 50 || speed > 300) {
    setLog('寻线基础速度必须在 50–300 mm/s');
    return;
  }
  startControl(() => `LINE,1,${speed}`, ui.startLineButton);
  lineControlActive = true;
  renderLineCurrent();
}

function loadDeadzoneResults() {
  try {
    const saved = JSON.parse(localStorage.getItem(DEADZONE_STORAGE_KEY));
    return saved && typeof saved === 'object' ? saved : {};
  } catch (_) {
    return {};
  }
}

function saveDeadzoneResults() {
  try {
    localStorage.setItem(DEADZONE_STORAGE_KEY, JSON.stringify(deadzoneResults));
  } catch (_) {}
}

function selectedDeadzoneTarget() {
  return deadzoneTargets[ui.deadzoneTarget.value];
}

function deadzoneCommandFactory() {
  return () => {
    const target = selectedDeadzoneTarget();
    const signedPwm = target.sign * Number(ui.deadzonePwm.value);
    const command = target.motor === 'left' ? `MOTOR,${signedPwm},0` : `MOTOR,0,${signedPwm}`;
    ui.deadzoneCommand.textContent = command;
    return command;
  };
}

function setDeadzonePwm(value) {
  ui.deadzonePwm.value = Math.max(0, Math.min(100, Math.round(Number(value) || 0)));
  ui.deadzonePwmValue.textContent = `${ui.deadzonePwm.value}%`;
  deadzoneCommandFactory()();
}

function updateDeadzoneLive() {
  const target = selectedDeadzoneTarget();
  const speed = target.motor === 'left' ? latestVehicle.leftSpeed : latestVehicle.rightSpeed;
  const encoder = target.motor === 'left' ? latestVehicle.leftEncoder : latestVehicle.rightEncoder;
  const baseline = deadzoneBaseline
    ? (target.motor === 'left' ? deadzoneBaseline.leftEncoder : deadzoneBaseline.rightEncoder)
    : encoder;
  ui.deadzoneSpeed.textContent = `${speed} mm/s`;
  ui.deadzoneEncoderDelta.textContent = (encoder - baseline).toLocaleString();
}

function startDeadzoneOutput() {
  if (!transportReady()) return;
  deadzoneBaseline = {
    leftEncoder: latestVehicle.leftEncoder,
    rightEncoder: latestVehicle.rightEncoder,
  };
  startControl(deadzoneCommandFactory(), ui.deadzoneRunButton);
  deadzoneOutputActive = true;
  updateDeadzoneLive();
}

function stopDeadzoneOutput() {
  stopControl();
  updateDeadzoneLive();
}

function formatDeadzoneResult(result) {
  if (!result || !Number.isFinite(result.pwm)) return '--';
  const voltage = Number.isFinite(result.voltage) ? ` · ${result.voltage.toFixed(2)}V` : '';
  return `${result.pwm}%${voltage}`;
}

function renderDeadzoneResults() {
  Object.keys(deadzoneTargets).forEach(key => {
    ui[`${key}Start`].textContent = formatDeadzoneResult(deadzoneResults[key]?.start);
    ui[`${key}Run`].textContent = formatDeadzoneResult(deadzoneResults[key]?.run);
  });
}

function recordDeadzoneResult(kind) {
  const key = ui.deadzoneTarget.value;
  const voltageValue = Number(ui.deadzoneVoltage.value);
  const result = {
    pwm: Number(ui.deadzonePwm.value),
    voltage: ui.deadzoneVoltage.value !== '' && Number.isFinite(voltageValue) ? voltageValue : null,
  };
  deadzoneResults[key] = { ...deadzoneResults[key], [kind]: result };
  saveDeadzoneResults();
  renderDeadzoneResults();
  const kindLabel = kind === 'start' ? '启动值' : '运行值';
  setLog(`${deadzoneTargets[key].label} ${kindLabel}: ${formatDeadzoneResult(result)}`);
}

function clearDeadzoneResults() {
  deadzoneResults = {};
  saveDeadzoneResults();
  renderDeadzoneResults();
  setLog('电机死区测量记录已清空');
}

function pushHistory(left, right, targetLeft, targetRight) {
  history.left.push(left);
  history.right.push(right);
  history.targetLeft.push(targetLeft);
  history.targetRight.push(targetRight);
  Object.values(history).forEach(series => {
    if (series.length > 120) series.shift();
  });
  drawChart();
}

function drawChart() {
  const canvas = ui.speedChart;
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
  const peak = Math.max(100, ...Object.values(history).flatMap(series => series.map(Math.abs)));
  const plot = (series, color, dashed = false) => {
    if (series.length < 2) return;
    ctx.strokeStyle = color;
    ctx.lineWidth = dashed ? 1.5 : 2;
    ctx.setLineDash(dashed ? [5, 4] : []);
    ctx.beginPath();
    series.forEach((value, index) => {
      const x = index * w / 119;
      const y = h / 2 - value / peak * h * .42;
      if (index === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
    });
    ctx.stroke();
  };
  plot(history.targetLeft, '#b46a11', true);
  plot(history.targetRight, '#bd2f2f', true);
  plot(history.left, '#147d58');
  plot(history.right, '#167a94');
  ctx.setLineDash([]);
}

function pushHeadingHistory(actual, target) {
  headingHistory.actual.push(actual);
  headingHistory.target.push(target);
  Object.values(headingHistory).forEach(series => {
    if (series.length > 120) series.shift();
  });
  drawHeadingChart();
}

function drawHeadingChart() {
  const canvas = ui.headingChart;
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
  const values = Object.values(headingHistory).flat();
  const minimum = values.length ? Math.min(...values) : -10;
  const maximum = values.length ? Math.max(...values) : 10;
  const center = (minimum + maximum) / 2;
  const span = Math.max(20, maximum - minimum + 10);
  const plot = (series, color, dashed = false) => {
    if (series.length < 2) return;
    ctx.strokeStyle = color;
    ctx.lineWidth = dashed ? 1.5 : 2;
    ctx.setLineDash(dashed ? [5, 4] : []);
    ctx.beginPath();
    series.forEach((value, index) => {
      const x = index * w / 119;
      const y = h / 2 - (value - center) / span * h * .84;
      if (index === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
    });
    ctx.stroke();
  };
  plot(headingHistory.target, '#b46a11', true);
  plot(headingHistory.actual, '#167a94');
  ctx.setLineDash([]);
}

document.querySelectorAll('.drive-button[data-direction]').forEach(button => {
  const start = event => {
    event.preventDefault();
    button.setPointerCapture?.(event.pointerId);
    startControl(driveCommandFactory(button.dataset.direction), button);
  };
  button.addEventListener('pointerdown', start);
  button.addEventListener('pointerup', () => stopControl());
  button.addEventListener('pointercancel', () => stopControl());
  button.addEventListener('lostpointercapture', () => { if (activeDriveButton === button) stopControl(); });
});

document.addEventListener('keydown', event => {
  const key = event.key.toLowerCase();
  if (!['w', 'a', 's', 'd'].includes(key) || event.ctrlKey || event.altKey || event.metaKey) return;
  if (isEditableControl(event.target)) return;
  event.preventDefault();
  if (!event.repeat) startKeyboardControl(key);
});

document.addEventListener('keyup', event => {
  const key = event.key.toLowerCase();
  if (!['w', 'a', 's', 'd'].includes(key) || !pressedDriveKeys.has(key)) return;
  event.preventDefault();
  releaseKeyboardControl(key);
});

ui.connectButton.addEventListener('click', () => connectedTransport ? disconnect() : connect());
ui.connectionMode.addEventListener('change', () => {
  ui.connectButton.textContent = ui.connectionMode.value === 'bridge' ? '连接桥接' : '连接串口';
  ui.browserStatus.textContent = ui.connectionMode.value === 'bridge'
    ? `WHEELTEC bridge · ${BRIDGE_URL}`
    : `Web Serial · ${BAUD_RATE} baud`;
});
ui.stopButton.addEventListener('click', () => stopControl());
ui.stopMotorsButton.addEventListener('click', () => stopControl());
ui.speedSlider.addEventListener('input', updateSpeedTargetDisplay);
ui.applyYawButton.addEventListener('click', applyYawParameters);
[ui.yawEnabled, ui.yawMaxRate, ui.yawKp, ui.yawKi, ui.yawKd, ui.yawKff].forEach(input => {
  input.addEventListener('input', () => {
    yawFormDirty = true;
    pendingYawParameters = undefined;
  });
  input.addEventListener('keydown', event => {
    if (event.key === 'Enter') applyYawParameters();
  });
});
ui.applyHeadingButton.addEventListener('click', applyHeadingParameters);
[ui.headingEnabled, ui.headingMaxRate, ui.headingKp, ui.headingKd, ui.headingKff]
  .forEach(input => {
    input.addEventListener('input', () => {
      headingFormDirty = true;
      pendingHeadingParameters = undefined;
    });
    input.addEventListener('keydown', event => {
      if (event.key === 'Enter') applyHeadingParameters();
    });
  });
ui.startHeadingTestButton.addEventListener('click', startHeadingTest);
ui.stopHeadingTestButton.addEventListener('click', () => stopControl());
ui.startSquareTestButton.addEventListener('click', startSquareTest);
ui.stopSquareTestButton.addEventListener('click', () => stopControl());
ui.applyLineButton.addEventListener('click', applyLineParameters);
[ui.lineKp, ui.lineKi, ui.lineKd, ui.lineDiff].forEach(input => {
  input.addEventListener('input', () => {
    lineFormDirty = true;
    pendingLineParameters = undefined;
  });
  input.addEventListener('keydown', event => {
    if (event.key === 'Enter') applyLineParameters();
  });
});
ui.startLineButton.addEventListener('click', startLineControl);
ui.stopLineButton.addEventListener('click', () => stopControl());
ui.lineSpeed.addEventListener('keydown', event => {
  if (event.key === 'Enter') startLineControl();
});
ui.leftMotor.addEventListener('input', updateMotorInputs);
ui.rightMotor.addEventListener('input', updateMotorInputs);
ui.runMotorsButton.addEventListener('click', () => startControl(
  () => `MOTOR,${Number(ui.leftMotor.value)},${Number(ui.rightMotor.value)}`
));
ui.deadzoneTarget.addEventListener('change', () => {
  if (deadzoneOutputActive) stopDeadzoneOutput();
  deadzoneBaseline = undefined;
  setDeadzonePwm(ui.deadzonePwm.value);
  updateDeadzoneLive();
});
ui.deadzonePwm.addEventListener('input', () => setDeadzonePwm(ui.deadzonePwm.value));
ui.deadzoneDecrease.addEventListener('click', () => setDeadzonePwm(Number(ui.deadzonePwm.value) - 1));
ui.deadzoneIncrease.addEventListener('click', () => setDeadzonePwm(Number(ui.deadzonePwm.value) + 1));
ui.deadzoneRunButton.addEventListener('click', startDeadzoneOutput);
ui.deadzoneStopButton.addEventListener('click', stopDeadzoneOutput);
ui.recordStartButton.addEventListener('click', () => recordDeadzoneResult('start'));
ui.recordRunButton.addEventListener('click', () => recordDeadzoneResult('run'));
ui.clearDeadzoneButton.addEventListener('click', clearDeadzoneResults);
ui.zeroEncoderButton.addEventListener('click', () => sendLine('ENCZERO'));
ui.zeroImuButton.addEventListener('click', () => sendLine('IMUZERO'));
ui.grayWhiteButton.addEventListener('click', () => sendLine('GRAYWHITE'));
ui.grayBlackButton.addEventListener('click', () => sendLine('GRAYBLACK'));
window.addEventListener('blur', () => {
  if (controlRunning || headingTest.active) stopControl();
});
document.addEventListener('visibilitychange', () => {
  if (document.hidden && (controlRunning || headingTest.active)) stopControl();
});
navigator.serial?.addEventListener('disconnect', event => { if (event.target === port && !closing) disconnect(false); });
new ResizeObserver(drawChart).observe(ui.speedChart);
new ResizeObserver(drawHeadingChart).observe(ui.headingChart);

setConnected(false);
updateSpeedTargetDisplay();
updatePidReadback([4000, 800, 0], [4000, 800, 0]);
updateHeadingReadback(1, 80, [4000, 300, 1000]);
updateLineReadback([200000, 0, 350000], 650);
renderTrackMode();
renderGrayReferences();
updateMotorInputs();
setDeadzonePwm(ui.deadzonePwm.value);
renderDeadzoneResults();
updateDeadzoneLive();
drawChart();
drawHeadingChart();
renderHeadingTest();
renderSquareTest();
