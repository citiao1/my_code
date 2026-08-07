const BAUD_RATE = 9600;
const BRIDGE_URL = 'ws://127.0.0.1:8766';
const BLE_SERVICE_UUID = '0000ffe0-0000-1000-8000-00805f9b34fb';
const BLE_NOTIFY_UUID = '0000ffe1-0000-1000-8000-00805f9b34fb';
const BLE_FALLBACK_WRITE_UUID = '0000ffe2-0000-1000-8000-00805f9b34fb';
const MOTORS = ['A', 'B', 'C', 'D'];
const HOLD_REFRESH_MS = 100;
const TELEMETRY_STALE_MS = 900;
const JOYSTICK_DEAD_ZONE = 0.10;
const MOBILE_DIRECT = new URLSearchParams(window.location.search).get('mobile') === '1' ||
  window.matchMedia('(display-mode: standalone)').matches;

const ui = {
  connectionMode: document.querySelector('#connectionMode'),
  connectButton: document.querySelector('#connectButton'),
  connectionBadge: document.querySelector('#connectionBadge'),
  modeValue: document.querySelector('#modeValue'),
  enableValue: document.querySelector('#enableValue'),
  batteryValue: document.querySelector('#batteryValue'),
  gyroValue: document.querySelector('#gyroValue'),
  timeValue: document.querySelector('#timeValue'),
  telemetryValue: document.querySelector('#telemetryValue'),
  speedSlider: document.querySelector('#speedSlider'),
  speedValue: document.querySelector('#speedValue'),
  webPath: document.querySelector('#webPath'),
  stopButton: document.querySelector('#stopButton'),
  zeroButton: document.querySelector('#zeroButton'),
  yawPidEnabled: document.querySelector('#yawPidEnabled'),
  yawPidState: document.querySelector('#yawPidState'),
  yawTargetValue: document.querySelector('#yawTargetValue'),
  yawRateValue: document.querySelector('#yawRateValue'),
  yawOutputValue: document.querySelector('#yawOutputValue'),
  yawAngleValue: document.querySelector('#yawAngleValue'),
  resetYawPidButton: document.querySelector('#resetYawPidButton'),
  calibrateGyroButton: document.querySelector('#calibrateGyroButton'),
  zeroYawButton: document.querySelector('#zeroYawButton'),
  headingEnabled: document.querySelector('#headingEnabled'),
  headingState: document.querySelector('#headingState'),
  headingTargetValue: document.querySelector('#headingTargetValue'),
  headingFeedbackValue: document.querySelector('#headingFeedbackValue'),
  headingErrorValue: document.querySelector('#headingErrorValue'),
  headingOutputValue: document.querySelector('#headingOutputValue'),
  captureHeadingButton: document.querySelector('#captureHeadingButton'),
  translationJoystick: document.querySelector('#translationJoystick'),
  translationJoystickValue: document.querySelector('#translationJoystickValue'),
  rotationJoystick: document.querySelector('#rotationJoystick'),
  rotationJoystickValue: document.querySelector('#rotationJoystickValue'),
  autoTestMode: document.querySelector('#autoTestMode'),
  autoTestValue: document.querySelector('#autoTestValue'),
  autoTestValueLabel: document.querySelector('#autoTestValueLabel'),
  autoTestDuration: document.querySelector('#autoTestDuration'),
  autoTestDurationLabel: document.querySelector('#autoTestDurationLabel'),
  autoTestCycles: document.querySelector('#autoTestCycles'),
  autoTestCyclesLabel: document.querySelector('#autoTestCyclesLabel'),
  autoTestTurnAngle: document.querySelector('#autoTestTurnAngle'),
  autoTestTurnTimeout: document.querySelector('#autoTestTurnTimeout'),
  autoTestTurnDirection: document.querySelector('#autoTestTurnDirection'),
  autoTestMotionFields: [...document.querySelectorAll('[data-motion-test-field]')],
  autoTestRouteFields: [...document.querySelectorAll('[data-route-test-field]')],
  startAutoTestButton: document.querySelector('#startAutoTestButton'),
  stopAutoTestButton: document.querySelector('#stopAutoTestButton'),
  autoTestState: document.querySelector('#autoTestState'),
  terminal: document.querySelector('#terminal'),
  clearLogButton: document.querySelector('#clearLogButton'),
  commandForm: document.querySelector('#commandForm'),
  commandInput: document.querySelector('#commandInput'),
  eventText: document.querySelector('#eventText'),
  mobileEventText: document.querySelector('#mobileEventText'),
  driveButtons: [...document.querySelectorAll('[data-motion]')],
  jogButtons: [...document.querySelectorAll('[data-jog]')],
  wheelCards: Object.fromEntries([...document.querySelectorAll('[data-wheel]')]
    .map((element) => [element.dataset.wheel, element])),
  encoder: Object.fromEntries([...document.querySelectorAll('[data-encoder]')]
    .map((element) => [element.dataset.encoder, element])),
  pwm: Object.fromEntries([...document.querySelectorAll('[data-pwm]')]
    .map((element) => [element.dataset.pwm, element])),
  rpm: Object.fromEntries([...document.querySelectorAll('[data-rpm]')]
    .map((element) => [element.dataset.rpm, element])),
  target: Object.fromEntries([...document.querySelectorAll('[data-target]')]
    .map((element) => [element.dataset.target, element])),
};

let transport;
let bridgeSocket;
let bridgeHardwareConnected = false;
let serialPort;
let serialReader;
let serialWriter;
let bleDevice;
let bleNotify;
let bleWrite;
let closing = false;
let receiveBuffer = '';
let writeChain = Promise.resolve();
let activeHold;
let holdTimer;
let keyboardDriveTimer;
let joystickTimer;
let joystickCommandActive = false;
let joystickLogged = false;
let joystickLastSendMs = 0;
let autoTestToken = 0;
let autoTestRunning = false;
let autoTestFailureReason = '';
const keyboardDriveKeys = new Set();
// This module disconnects on a single BLE write longer than 16 bytes.
const BLE_WRITE_CHUNK_SIZE = 16;
const BLE_WRITE_CHUNK_DELAY_MS = 20;
const telemetry = {
  time: 0,
  mode: 'STOP',
  enabled: 0,
  batteryMv: 0,
  encoder: { A: 0, B: 0, C: 0, D: 0 },
  pwm: { A: 0, B: 0, C: 0, D: 0 },
  rpm: { A: 0, B: 0, C: 0, D: 0 },
  target: { A: 0, B: 0, C: 0, D: 0 },
  pid: { enabled: 1 },
  gyro: { connected: 0, ready: 0, calibrating: 0, rawRate: 0, rate: 0, yaw: 0 },
  yaw: { target: 0, output: 0, kp: 0.5, ki: 0.1, kff: 0, enabled: 1 },
  heading: { target: 0, feedback: 0, error: 0, output: 0, kp: 5, kd: 1.25,
    maxRate: 80, enabled: 1, holding: 0 },
  receivedAt: 0,
};
const joystickState = {
  translation: {
    element: ui.translationJoystick,
    knob: ui.translationJoystick.querySelector('.joystick-knob'),
    pointerId: undefined,
    x: 0,
    y: 0,
  },
  rotation: {
    element: ui.rotationJoystick,
    knob: ui.rotationJoystick.querySelector('.joystick-knob'),
    pointerId: undefined,
    x: 0,
    y: 0,
  },
};

if (MOBILE_DIRECT) {
  document.documentElement.classList.add('mobile-direct-root');
  document.body.classList.add('mobile-direct');
  [...ui.connectionMode.options].forEach((option) => {
    if (option.value !== 'ble') option.remove();
  });
  ui.connectionMode.value = 'ble';
  ui.webPath.textContent = '手机直连 BLE FFE0 / FFE1';
}

function nowTime() {
  return new Date().toLocaleTimeString('zh-CN', { hour12: false });
}

function setEvent(message) {
  ui.eventText.textContent = `${nowTime()}  ${message}`;
  if (ui.mobileEventText) ui.mobileEventText.textContent = message;
}

function appendLog(direction, message, kind = '') {
  const row = document.createElement('div');
  row.className = `log-line ${direction.toLowerCase()} ${kind}`.trim();
  row.innerHTML = '<span class="time"></span><span class="direction"></span><span class="message"></span>';
  row.querySelector('.time').textContent = nowTime();
  row.querySelector('.direction').textContent = direction;
  row.querySelector('.message').textContent = message;
  ui.terminal.append(row);
  while (ui.terminal.childElementCount > 250) ui.terminal.firstElementChild.remove();
  ui.terminal.scrollTop = ui.terminal.scrollHeight;
}

function setBadge(state, text) {
  ui.connectionBadge.className = `badge ${state}`;
  ui.connectionBadge.innerHTML = '<span class="dot"></span>';
  ui.connectionBadge.append(document.createTextNode(text));
}

function isTransportOpen() {
  if (transport === 'bridge') return bridgeSocket?.readyState === WebSocket.OPEN;
  if (transport === 'serial') return Boolean(serialPort && serialWriter);
  if (transport === 'ble') return Boolean(bleDevice?.gatt?.connected && bleWrite);
  return false;
}

function isConnected() {
  if (transport === 'bridge') return isTransportOpen() && bridgeHardwareConnected;
  return isTransportOpen();
}

function telemetryFresh() {
  return telemetry.receivedAt > 0 && (Date.now() - telemetry.receivedAt) <= TELEMETRY_STALE_MS;
}

function motionReady(command) {
  const baseReady = isConnected() && telemetryFresh() && telemetry.enabled === 1;
  if (!baseReady) return false;
  if (MOTORS.includes(command[0]) && command.length === 2) return true;
  return telemetry.pid.enabled === 1;
}

function updateConnectionUi() {
  const open = isTransportOpen();
  ui.connectionMode.disabled = open;
  ui.connectButton.textContent = open ? '断开' : '连接';

  if (isConnected()) setBadge('online', '硬件已连接');
  else if (open) setBadge('waiting', '等待硬件');
  else setBadge('offline', '未连接');

  updateControlAvailability();
}

function updateControlAvailability() {
  const hardwareReady = isConnected() && telemetryFresh() && telemetry.enabled === 1;
  const pidReady = hardwareReady && telemetry.pid.enabled === 1;

  ui.driveButtons.forEach((button) => {
    const needsGyro = ['Q', 'E'].includes(button.dataset.motion);
    button.disabled = !pidReady || (needsGyro && (!telemetry.gyro.ready || !telemetry.yaw.enabled));
  });
  ui.jogButtons.forEach((button) => { button.disabled = !hardwareReady; });
  ui.stopButton.disabled = !isConnected();
  ui.zeroButton.disabled = !isConnected();
  [ui.yawPidEnabled, ui.resetYawPidButton, ui.calibrateGyroButton,
    ui.zeroYawButton, ui.headingEnabled, ui.captureHeadingButton,
    ui.startAutoTestButton].forEach((control) => {
    control.disabled = !isConnected();
  });
  ui.startAutoTestButton.disabled = !isConnected() || autoTestRunning;
  ui.stopAutoTestButton.disabled = !autoTestRunning;
  ui.translationJoystick.setAttribute('aria-disabled', String(!pidReady || autoTestRunning));
  const rotationReady = pidReady && telemetry.gyro.ready && telemetry.yaw.enabled;
  ui.rotationJoystick.setAttribute('aria-disabled', String(!rotationReady || autoTestRunning));
}

async function writeTransport(payload) {
  if (transport === 'bridge') {
    bridgeSocket.send(payload);
    return;
  }
  if (transport === 'serial') {
    await serialWriter.write(new TextEncoder().encode(payload));
    return;
  }
  if (transport === 'ble') {
    const data = new TextEncoder().encode(payload);
    for (let offset = 0; offset < data.length; offset += BLE_WRITE_CHUNK_SIZE) {
      const chunk = data.slice(offset, offset + BLE_WRITE_CHUNK_SIZE);
      if (bleWrite.properties.writeWithoutResponse) await bleWrite.writeValueWithoutResponse(chunk);
      else await bleWrite.writeValue(chunk);
      if (offset + BLE_WRITE_CHUNK_SIZE < data.length) {
        await new Promise((resolve) => setTimeout(resolve, BLE_WRITE_CHUNK_DELAY_MS));
      }
    }
  }
}

async function sendCommand(command, log = true) {
  const normalized = command.trim().toUpperCase();
  if (!normalized || !isConnected()) return false;
  if (log) appendLog('TX', normalized);

  writeChain = writeChain.catch(() => {}).then(() => writeTransport(`${normalized}\n`));
  try {
    await writeChain;
    return true;
  } catch (error) {
    appendLog('ERR', error.message, 'error');
    setEvent('发送失败');
    return false;
  }
}

function consumeText(text) {
  receiveBuffer += text;
  const lines = receiveBuffer.replaceAll('\r', '').split('\n');
  receiveBuffer = lines.pop();
  lines.map((line) => line.trim()).filter(Boolean).forEach(handleLine);
}

function handleBridgeStatus(fields) {
  const wasConnected = bridgeHardwareConnected;
  bridgeHardwareConnected = fields[1] === '1';
  updateConnectionUi();
  setEvent(fields.slice(2).join(',') || (bridgeHardwareConnected ? '硬件已连接' : '硬件已断开'));

  if (!wasConnected && bridgeHardwareConnected) initializeFirmwareLink();
  if (!bridgeHardwareConnected) {
    stopHold(false);
    stopKeyboardDrive(false);
    stopJoystickDrive(false);
    stopAutoTest(false);
  }
}

function handleTelemetry(fields) {
  if (fields.length < 27) return false;
  const values = fields.slice(1).map((value, index) => (index === 1 ? value : Number(value)));
  if ([values[0], values[2], ...values.slice(3)].some((value) => Number.isNaN(value))) return false;

  telemetry.time = values[0];
  telemetry.mode = values[1];
  telemetry.enabled = values[2];
  MOTORS.forEach((motor, index) => {
    telemetry.encoder[motor] = values[3 + index];
    telemetry.pwm[motor] = values[7 + index];
    telemetry.rpm[motor] = values[11 + index];
    telemetry.target[motor] = values[15 + index];
  });
  telemetry.batteryMv = values[19];
  telemetry.pid.enabled = values[25];
  if (values.length >= 38) {
    telemetry.gyro.connected = values[26];
    telemetry.gyro.ready = values[27];
    telemetry.gyro.calibrating = values[28];
    telemetry.gyro.rawRate = values[29] / 1000;
    telemetry.gyro.rate = values[30] / 1000;
    telemetry.gyro.yaw = values[31] / 1000;
    telemetry.yaw.target = values[32] / 1000;
    telemetry.yaw.output = values[33] / 1000;
    telemetry.yaw.kp = values[34] / 1000;
    telemetry.yaw.ki = values[35] / 1000;
    telemetry.yaw.kff = values[36] / 1000;
    telemetry.yaw.enabled = values[37];
  }
  if (values.length >= 47) {
    telemetry.heading.target = values[38] / 1000;
    telemetry.heading.feedback = values[39] / 1000;
    telemetry.heading.error = values[40] / 1000;
    telemetry.heading.output = values[41] / 1000;
    telemetry.heading.kp = values[42] / 1000;
    telemetry.heading.kd = values[43] / 1000;
    telemetry.heading.maxRate = values[44] / 1000;
    telemetry.heading.enabled = values[45];
    telemetry.heading.holding = values[46];
  }
  telemetry.receivedAt = Date.now();
  renderTelemetry();
  return true;
}

function handleLine(line) {
  const fields = line.split(',');
  if (fields[0] === 'STATUS') {
    handleBridgeStatus(fields);
    appendLog('RX', line);
    return;
  }
  if (fields[0] === 'TEL' && handleTelemetry(fields)) return;

  appendLog('RX', line, fields[0] === 'ERR' ? 'error' : '');
  if (fields[0] === 'ACK' && fields[1] === 'SPEED') {
    ui.speedSlider.value = fields[2];
    ui.speedValue.value = fields[2];
  }
  if (fields[0] === 'ACK' && fields[1] === 'YAWPID') {
    const gains = fields.slice(2, 5).map(Number);
    if (gains.length === 3 && gains.every(Number.isFinite)) {
      telemetry.yaw.kp = gains[0] / 1000;
      telemetry.yaw.ki = gains[1] / 1000;
      telemetry.yaw.kff = gains[2] / 1000;
      setEvent('角速度环状态已更新');
    }
  }
  if (fields[0] === 'ACK' && fields[1] === 'HEADPID') {
    const config = fields.slice(2, 6).map(Number);
    if (config.length === 4 && config.every(Number.isFinite)) {
      telemetry.heading.kp = config[0] / 1000;
      telemetry.heading.kd = config[1] / 1000;
      telemetry.heading.maxRate = config[2];
      telemetry.heading.enabled = config[3];
      setEvent('航向角外环状态已更新');
    }
  }
}

function renderTelemetry() {
  const hasTelemetry = telemetry.receivedAt > 0;

  ui.modeValue.textContent = telemetry.mode;
  ui.enableValue.textContent = hasTelemetry ? (telemetry.enabled ? 'ON' : 'OFF') : '--';
  ui.batteryValue.textContent = hasTelemetry && telemetry.batteryMv > 0
    ? `${(telemetry.batteryMv / 1000).toFixed(2)} V` : '--';
  ui.gyroValue.textContent = !hasTelemetry || !telemetry.gyro.connected ? '--'
    : (telemetry.gyro.ready ? 'READY' : (telemetry.gyro.calibrating ? 'CAL' : 'FAULT'));
  ui.timeValue.textContent = hasTelemetry ? `${(telemetry.time / 1000).toFixed(1)} s` : '--';
  ui.telemetryValue.textContent = hasTelemetry ? '实时' : '等待数据';

  MOTORS.forEach((motor) => {
    ui.encoder[motor].textContent = telemetry.encoder[motor].toLocaleString('zh-CN');
    ui.pwm[motor].textContent = telemetry.pwm[motor];
    ui.rpm[motor].textContent = telemetry.rpm[motor];
    ui.target[motor].textContent = telemetry.target[motor];
    ui.wheelCards[motor].classList.toggle('active', telemetry.target[motor] !== 0);
  });
  ui.yawTargetValue.textContent = `${telemetry.yaw.target.toFixed(2)} °/s`;
  ui.yawRateValue.textContent = `${telemetry.gyro.rate.toFixed(2)} °/s`;
  ui.yawOutputValue.textContent = `${telemetry.yaw.output.toFixed(2)} RPM`;
  ui.yawAngleValue.textContent = `${telemetry.gyro.yaw.toFixed(2)}°`;
  ui.yawPidEnabled.checked = telemetry.yaw.enabled === 1;
  ui.yawPidState.textContent = telemetry.yaw.enabled ? 'ON' : 'OFF';
  ui.headingTargetValue.textContent = `${telemetry.heading.target.toFixed(2)}°`;
  ui.headingFeedbackValue.textContent = `${telemetry.heading.feedback.toFixed(2)}°`;
  ui.headingErrorValue.textContent = `${telemetry.heading.error.toFixed(2)}°`;
  ui.headingOutputValue.textContent = `${telemetry.heading.output.toFixed(2)} °/s`;
  ui.headingEnabled.checked = telemetry.heading.enabled === 1;
  ui.headingState.textContent = telemetry.heading.enabled
    ? (telemetry.heading.holding ? 'LOCK' : 'TRACK') : 'OFF';
  updateControlAvailability();
}

async function initializeFirmwareLink() {
  await sendCommand(`SPEED,${ui.speedSlider.value}`);
  await sendCommand('STATUS', false);
}

async function connectBridge() {
  transport = 'bridge';
  bridgeHardwareConnected = false;
  bridgeSocket = new WebSocket(BRIDGE_URL);
  bridgeSocket.addEventListener('message', (event) => consumeText(String(event.data)));
  bridgeSocket.addEventListener('close', () => {
    bridgeHardwareConnected = false;
    if (transport === 'bridge') transport = undefined;
    stopHold(false);
    stopKeyboardDrive(false);
    stopJoystickDrive(false);
    stopAutoTest(false);
    updateConnectionUi();
    setEvent('桥接已断开');
  });

  await new Promise((resolve, reject) => {
    const timeout = setTimeout(() => reject(new Error('桥接连接超时')), 4000);
    bridgeSocket.addEventListener('open', () => {
      clearTimeout(timeout);
      resolve();
    }, { once: true });
    bridgeSocket.addEventListener('error', () => {
      clearTimeout(timeout);
      reject(new Error('无法连接本地桥接程序'));
    }, { once: true });
  });
  setEvent('网页已连接桥接程序');
}

async function readSerialLoop() {
  const decoder = new TextDecoder();
  try {
    while (!closing && serialPort?.readable) {
      serialReader = serialPort.readable.getReader();
      try {
        while (!closing) {
          const { value, done } = await serialReader.read();
          if (done) break;
          if (value) consumeText(decoder.decode(value, { stream: true }));
        }
      } finally {
        serialReader.releaseLock();
        serialReader = undefined;
      }
    }
  } catch (error) {
    if (!closing) appendLog('ERR', error.message, 'error');
  }
}

async function connectSerial() {
  if (!('serial' in navigator)) throw new Error('当前浏览器不支持 Web Serial');
  closing = false;
  serialPort = await navigator.serial.requestPort();
  await serialPort.open({ baudRate: BAUD_RATE });
  serialWriter = serialPort.writable.getWriter();
  transport = 'serial';
  void readSerialLoop();
  setEvent('串口已连接');
  await initializeFirmwareLink();
}

function onBleValue(event) {
  const value = event.target.value;
  const bytes = new Uint8Array(value.buffer, value.byteOffset, value.byteLength);
  consumeText(new TextDecoder().decode(bytes));
}

async function connectBle() {
  if (!window.isSecureContext) throw new Error('手机直连 BLE 需要 HTTPS 页面');
  if (!navigator.bluetooth) throw new Error('当前浏览器不支持 Web Bluetooth');
  bleDevice = await navigator.bluetooth.requestDevice({
    filters: [{ services: [BLE_SERVICE_UUID] }],
    optionalServices: [BLE_SERVICE_UUID],
  });
  const server = await bleDevice.gatt.connect();
  const service = await server.getPrimaryService(BLE_SERVICE_UUID);
  bleNotify = await service.getCharacteristic(BLE_NOTIFY_UUID);
  await bleNotify.startNotifications();
  bleNotify.addEventListener('characteristicvaluechanged', onBleValue);
  if (bleNotify.properties.write || bleNotify.properties.writeWithoutResponse) {
    bleWrite = bleNotify;
  } else {
    bleWrite = await service.getCharacteristic(BLE_FALLBACK_WRITE_UUID);
  }

  bleDevice.addEventListener('gattserverdisconnected', () => {
    stopHold(false);
    stopKeyboardDrive(false);
    stopJoystickDrive(false);
    stopAutoTest(false);
    transport = undefined;
    bleWrite = undefined;
    updateConnectionUi();
    setEvent('BLE 已断开');
  });
  transport = 'ble';
  setEvent('BLE 已连接');
  await initializeFirmwareLink();
}

async function disconnect() {
  const shouldStop = isConnected();
  stopHold(false);
  stopKeyboardDrive(false);
  stopJoystickDrive(false);
  stopAutoTest(false);
  if (shouldStop) await sendCommand('STOP');
  closing = true;
  const current = transport;

  if (current === 'bridge') bridgeSocket?.close();
  if (current === 'serial') {
    try { await serialReader?.cancel(); } catch (_) {}
    try { serialWriter?.releaseLock(); } catch (_) {}
    serialWriter = undefined;
    try { await serialPort?.close(); } catch (_) {}
    serialPort = undefined;
  }
  if (current === 'ble') bleDevice?.gatt?.disconnect();

  transport = undefined;
  bridgeHardwareConnected = false;
  bleWrite = undefined;
  updateConnectionUi();
  setEvent('连接已关闭');
}

async function toggleConnection() {
  if (isTransportOpen()) {
    await disconnect();
    return;
  }

  ui.connectButton.disabled = true;
  try {
    if (ui.connectionMode.value === 'bridge') await connectBridge();
    else if (ui.connectionMode.value === 'serial') await connectSerial();
    else await connectBle();
  } catch (error) {
    appendLog('ERR', error.message, 'error');
    setEvent(error.message);
    transport = undefined;
    bridgeHardwareConnected = false;
  } finally {
    ui.connectButton.disabled = false;
    updateConnectionUi();
  }
}

function startHold(command, element) {
  if (!motionReady(command)) {
    if (!telemetry.pid.enabled && command.length === 1) setEvent('请先启用 PID');
    else setEvent('电机使能未打开或遥测已超时');
    return;
  }
  if (activeHold?.command === command) return;

  stopHold();
  stopKeyboardDrive();
  stopJoystickDrive();
  activeHold = { command, element };
  element?.classList.add('active');
  void sendCommand(command);
  holdTimer = setInterval(() => { void sendCommand(command, false); }, HOLD_REFRESH_MS);
}

function stopHold(sendStop = true) {
  clearInterval(holdTimer);
  holdTimer = undefined;
  activeHold?.element?.classList.remove('active');
  const wasActive = Boolean(activeHold);
  activeHold = undefined;
  if (sendStop && isConnected() && wasActive) void sendCommand('STOP');
}

function keyboardDriveCommand() {
  const speed = Number(ui.speedSlider.value);
  const forward = (keyboardDriveKeys.has('W') ? speed : 0) -
                  (keyboardDriveKeys.has('S') ? speed : 0);
  const left = (keyboardDriveKeys.has('A') ? speed : 0) -
               (keyboardDriveKeys.has('D') ? speed : 0);
  const yaw = (keyboardDriveKeys.has('Q') ? speed : 0) -
              (keyboardDriveKeys.has('E') ? speed : 0);
  return { forward, left, yaw };
}

function renderKeyboardDriveKeys() {
  ui.driveButtons.forEach((button) => {
    button.classList.toggle('active', keyboardDriveKeys.has(button.dataset.motion));
  });
}

function sendKeyboardDrive(log = true) {
  const command = keyboardDriveCommand();
  const readinessCommand = command.yaw === 0 ? 'W' : 'Q';

  if (!motionReady(readinessCommand)) {
    setEvent(command.yaw === 0 ? '电机使能未打开或遥测已超时' : '陀螺仪或角速度环未就绪');
    stopKeyboardDrive();
    return;
  }
  void sendCommand(`DRV,${command.forward},${command.left},${command.yaw}`, log);
}

function startKeyboardDrive(key) {
  if (keyboardDriveKeys.has(key)) return;

  stopHold();
  stopJoystickDrive();
  keyboardDriveKeys.add(key);
  renderKeyboardDriveKeys();
  sendKeyboardDrive();
  if ((keyboardDriveKeys.size > 0) && !keyboardDriveTimer) {
    keyboardDriveTimer = setInterval(() => sendKeyboardDrive(false), HOLD_REFRESH_MS);
  }
}

function releaseKeyboardDrive(key) {
  if (!keyboardDriveKeys.delete(key)) return;

  if (keyboardDriveKeys.size === 0) {
    stopKeyboardDrive(false);
    if (isConnected()) void sendCommand('STOP');
    return;
  }
  renderKeyboardDriveKeys();
  sendKeyboardDrive();
}

function stopKeyboardDrive(sendStop = true) {
  clearInterval(keyboardDriveTimer);
  keyboardDriveTimer = undefined;
  const wasActive = keyboardDriveKeys.size > 0;
  keyboardDriveKeys.clear();
  renderKeyboardDriveKeys();
  if (sendStop && isConnected() && wasActive) void sendCommand('STOP');
}

function applyAxisDeadZone(value) {
  const magnitude = Math.abs(value);
  if (magnitude <= JOYSTICK_DEAD_ZONE) return 0;
  return Math.sign(value) * (magnitude - JOYSTICK_DEAD_ZONE) / (1 - JOYSTICK_DEAD_ZONE);
}

function translationJoystickVector() {
  const { x, y } = joystickState.translation;
  const magnitude = Math.hypot(x, y);
  if (magnitude <= JOYSTICK_DEAD_ZONE) return { x: 0, y: 0 };

  const scaledMagnitude = Math.min(1,
    (magnitude - JOYSTICK_DEAD_ZONE) / (1 - JOYSTICK_DEAD_ZONE));
  return {
    x: x * scaledMagnitude / magnitude,
    y: y * scaledMagnitude / magnitude,
  };
}

function joystickDriveCommand() {
  const speed = Number(ui.speedSlider.value);
  const translation = translationJoystickVector();
  const rotation = applyAxisDeadZone(joystickState.rotation.x);

  return {
    forward: Math.round(-translation.y * speed),
    left: Math.round(-translation.x * speed),
    yaw: Math.round(-rotation * speed),
  };
}

function joystickTravel(control) {
  const radius = Math.min(control.element.clientWidth, control.element.clientHeight) / 2;
  return Math.max(1, radius - control.knob.offsetWidth / 2 - 7);
}

function renderJoystick(control) {
  const travel = joystickTravel(control);
  control.knob.style.transform =
    `translate3d(${control.x * travel}px, ${control.y * travel}px, 0) translate(-50%, -50%)`;
}

function renderJoystickValues() {
  const command = joystickDriveCommand();
  ui.translationJoystickValue.textContent =
    `前后 ${command.forward} · 左右 ${command.left}`;
  ui.rotationJoystickValue.textContent = `角速度 ${command.yaw}°/s`;
}

function updateJoystickFromPointer(control, event) {
  const rect = control.element.getBoundingClientRect();
  const travel = joystickTravel(control);
  let x = (event.clientX - rect.left - rect.width / 2) / travel;
  let y = (event.clientY - rect.top - rect.height / 2) / travel;

  if (control === joystickState.rotation) {
    x = Math.max(-1, Math.min(1, x));
    y = 0;
  } else {
    const magnitude = Math.hypot(x, y);
    if (magnitude > 1) {
      x /= magnitude;
      y /= magnitude;
    }
  }

  control.x = x;
  control.y = y;
  renderJoystick(control);
  renderJoystickValues();
}

function sendJoystickDrive(log = false, force = false) {
  const command = joystickDriveCommand();
  const moving = command.forward !== 0 || command.left !== 0 || command.yaw !== 0;
  const now = performance.now();

  if (!moving) {
    if (joystickCommandActive && isConnected()) void sendCommand('STOP', log);
    joystickCommandActive = false;
    return;
  }
  if (!force && (now - joystickLastSendMs) < 45) return;
  if (!motionReady(command.yaw === 0 ? 'W' : 'Q') ||
      (command.yaw !== 0 && (!telemetry.gyro.ready || !telemetry.yaw.enabled))) {
    setEvent(command.yaw === 0 ? '电机使能未打开或遥测已超时' : '陀螺仪或角速度环未就绪');
    stopJoystickDrive();
    return;
  }

  joystickLastSendMs = now;
  joystickCommandActive = true;
  void sendCommand(`DRV,${command.forward},${command.left},${command.yaw}`, log);
}

function startJoystick(control, event) {
  if (control.element.getAttribute('aria-disabled') === 'true') {
    setEvent('硬件、遥测或控制环未就绪');
    return;
  }
  if (control.pointerId !== undefined) return;

  event.preventDefault();
  stopHold(false);
  stopKeyboardDrive(false);
  control.pointerId = event.pointerId;
  control.element.setPointerCapture?.(event.pointerId);
  control.element.classList.add('active');
  updateJoystickFromPointer(control, event);
  if (!joystickTimer) {
    joystickLogged = false;
    joystickTimer = setInterval(() => sendJoystickDrive(false, true), HOLD_REFRESH_MS);
  }
  sendJoystickDrive(!joystickLogged, true);
  joystickLogged = joystickCommandActive;
}

function moveJoystick(control, event) {
  if (control.pointerId !== event.pointerId) return;
  event.preventDefault();
  updateJoystickFromPointer(control, event);
  sendJoystickDrive(!joystickLogged);
  joystickLogged = joystickLogged || joystickCommandActive;
}

function releaseJoystick(control, event) {
  if (control.pointerId !== event.pointerId) return;

  event.preventDefault();
  control.pointerId = undefined;
  control.x = 0;
  control.y = 0;
  control.element.classList.remove('active');
  renderJoystick(control);
  renderJoystickValues();

  const anotherPointerActive = Object.values(joystickState)
    .some((item) => item.pointerId !== undefined);
  if (anotherPointerActive) sendJoystickDrive(false, true);
  else stopJoystickDrive();
}

function stopJoystickDrive(sendStop = true) {
  clearInterval(joystickTimer);
  joystickTimer = undefined;
  const wasActive = joystickCommandActive || Object.values(joystickState)
    .some((control) => control.pointerId !== undefined);

  Object.values(joystickState).forEach((control) => {
    control.pointerId = undefined;
    control.x = 0;
    control.y = 0;
    control.element.classList.remove('active');
    renderJoystick(control);
  });
  joystickCommandActive = false;
  joystickLogged = false;
  renderJoystickValues();
  if (sendStop && wasActive && isConnected()) void sendCommand('STOP');
}

function bindJoystick(control) {
  control.element.addEventListener('pointerdown', (event) => startJoystick(control, event));
  control.element.addEventListener('pointermove', (event) => moveJoystick(control, event));
  ['pointerup', 'pointercancel', 'lostpointercapture'].forEach((name) => {
    control.element.addEventListener(name, (event) => releaseJoystick(control, event));
  });
}

function waitMs(durationMs) {
  return new Promise((resolve) => setTimeout(resolve, durationMs));
}

function failAutoTest(message) {
  autoTestFailureReason = message;
  setEvent(message);
  appendLog('TEST', message, 'error');
  return false;
}

async function holdAutoTestCommand(command, durationMs, token) {
  const endTime = Date.now() + durationMs;
  let log = true;

  while ((token === autoTestToken) && (Date.now() < endTime)) {
    if (!await sendCommand(command, log)) return false;
    log = false;
    await waitMs(HOLD_REFRESH_MS);
  }
  return token === autoTestToken;
}

async function waitHeadingStep(config, turnIndex, token) {
  const deltaDeg = config.turnDirection * config.turnAngle;
  const endTime = Date.now() + config.turnTimeoutMs;
  let lastTelemetryAt = telemetry.receivedAt;
  let stepObserved = false;
  let settledFrames = 0;

  appendLog('TEST', `航向阶跃 ${turnIndex}: ${deltaDeg > 0 ? '+' : ''}${deltaDeg}°`);
  if (!await sendCommand(`HEADSTEP,${deltaDeg}`)) return false;

  while ((token === autoTestToken) && (Date.now() < endTime)) {
    if (!telemetryFresh() || !telemetry.gyro.ready || !telemetry.heading.enabled) {
      return failAutoTest('航向阶跃中遥测、陀螺仪或航向环失效');
    }
    if (!await sendCommand(`DRV,${config.value},0,0`, false)) return false;

    if (telemetry.receivedAt !== lastTelemetryAt) {
      lastTelemetryAt = telemetry.receivedAt;
      if (Math.abs(telemetry.heading.error) >= Math.max(3, config.turnAngle * 0.25)) {
        stepObserved = true;
      }
      const settled = stepObserved && Math.abs(telemetry.heading.error) <= 2 &&
        Math.abs(telemetry.gyro.rate) <= 3;
      settledFrames = settled ? settledFrames + 1 : 0;
    }

    ui.autoTestState.textContent = `TURN ${turnIndex} ERR ${telemetry.heading.error.toFixed(1)}°`;
    if (settledFrames >= 3) {
      appendLog('TEST', `航向已收敛: error=${telemetry.heading.error.toFixed(2)}°, rate=${telemetry.gyro.rate.toFixed(2)}°/s`);
      return true;
    }
    await waitMs(HOLD_REFRESH_MS);
  }

  if (token === autoTestToken) return failAutoTest('航向阶跃未在限定时间内收敛');
  return false;
}

async function runRouteAutoTest(config, token) {
  if (!await sendCommand('HEADRESET')) return false;

  for (let leg = 0; leg < config.cycles && token === autoTestToken; ++leg) {
    ui.autoTestState.textContent = `STRAIGHT ${leg + 1}/${config.cycles}`;
    appendLog('TEST', `直行 ${leg + 1}/${config.cycles}`);
    if (!await holdAutoTestCommand(`DRV,${config.value},0,0`, config.durationMs, token)) {
      return false;
    }
    if (leg + 1 >= config.cycles) return true;

    if (!await waitHeadingStep(config, `${leg + 1}/${config.cycles - 1}`, token)) {
      return false;
    }
  }
  return token === autoTestToken;
}

async function runYawAutoTest(config, token) {
  for (let cycle = 0; cycle < config.cycles && token === autoTestToken; ++cycle) {
    ui.autoTestState.textContent = `YAW + ${cycle + 1}/${config.cycles}`;
    if (!await holdAutoTestCommand(`DRV,0,0,${config.value}`, config.durationMs, token)) {
      return false;
    }
    if (!await sendCommand('STOP')) return false;
    await waitMs(1000);

    ui.autoTestState.textContent = `YAW - ${cycle + 1}/${config.cycles}`;
    if (!await holdAutoTestCommand(`DRV,0,0,${-config.value}`, config.durationMs, token)) {
      return false;
    }
    if (!await sendCommand('STOP')) return false;
    if (cycle + 1 < config.cycles) await waitMs(1000);
  }
  return token === autoTestToken;
}

async function runDisturbanceAutoTest(config, token) {
  const endTime = Date.now() + config.durationMs;
  const startDeadline = Date.now() + 1000;
  let logCommand = true;
  let holdObserved = false;

  appendLog('TEST', '0°抗扰保持开始，请手动转动车体后松手');
  while ((token === autoTestToken) && (Date.now() < endTime)) {
    if (!telemetryFresh() || !telemetry.gyro.ready ||
        !telemetry.yaw.enabled || !telemetry.heading.enabled) {
      return failAutoTest('抗扰保持中遥测、陀螺仪或控制环失效');
    }
    if (!await sendCommand('HEADHOLD', logCommand)) return false;
    logCommand = false;
    holdObserved = holdObserved || telemetry.mode === 'HOLD';
    if (!holdObserved && Date.now() >= startDeadline) {
      return failAutoTest('固件未进入 0°抗扰保持模式');
    }
    ui.autoTestState.textContent = `HOLD 0° ERR ${telemetry.heading.error.toFixed(1)}°`;
    await waitMs(HOLD_REFRESH_MS);
  }

  if (token === autoTestToken) appendLog('TEST', '0°抗扰保持结束');
  return token === autoTestToken;
}

function readAutoTestConfig() {
  return {
    mode: ui.autoTestMode.value,
    value: Number(ui.autoTestValue.value),
    durationMs: Number(ui.autoTestDuration.value) * 1000,
    cycles: Number(ui.autoTestCycles.value),
    turnAngle: Number(ui.autoTestTurnAngle.value),
    turnTimeoutMs: Number(ui.autoTestTurnTimeout.value) * 1000,
    turnDirection: Number(ui.autoTestTurnDirection.value),
    speedLimit: Number(ui.speedSlider.value),
  };
}

function autoTestConfigIsValid(config) {
  if (!['route', 'yaw', 'disturbance'].includes(config.mode)) return false;
  const durationMaximum = config.mode === 'disturbance' ? 60000 : 15000;
  if (!Number.isFinite(config.durationMs) || config.durationMs < 500 ||
      config.durationMs > durationMaximum) {
    return false;
  }
  if (config.mode === 'disturbance') return true;

  const cycleMinimum = config.mode === 'route' ? 2 : 1;
  const cycleMaximum = config.mode === 'route' ? 8 : 10;

  if (!Number.isFinite(config.value) || config.value < 5 || config.value > config.speedLimit) {
    return false;
  }
  if (!Number.isInteger(config.cycles) || config.cycles < cycleMinimum ||
      config.cycles > cycleMaximum) {
    return false;
  }
  if (config.mode !== 'route') return true;
  return Number.isFinite(config.turnAngle) && config.turnAngle >= 10 && config.turnAngle <= 170 &&
         Number.isFinite(config.turnTimeoutMs) && config.turnTimeoutMs >= 2000 &&
         config.turnTimeoutMs <= 30000 && Math.abs(config.turnDirection) === 1;
}

function stopAutoTest(sendStop = true, message = '自动测试已停止') {
  const wasRunning = autoTestRunning;

  ++autoTestToken;
  autoTestRunning = false;
  ui.autoTestState.textContent = 'IDLE';
  updateControlAvailability();
  if (sendStop && wasRunning && isConnected()) void sendCommand('STOP');
  if (wasRunning) setEvent(message);
}

async function runAutoTest() {
  const config = readAutoTestConfig();

  if (!isConnected() || !telemetryFresh() || !telemetry.enabled || !telemetry.pid.enabled) {
    setEvent('硬件、遥测或速度环未就绪');
    return;
  }
  const needsHeading = config.mode === 'route' || config.mode === 'disturbance';
  if (!telemetry.gyro.ready || !telemetry.yaw.enabled ||
      (needsHeading && !telemetry.heading.enabled)) {
    setEvent(needsHeading ? '陀螺仪、角速度环或航向环未就绪' : '陀螺仪或角速度环未就绪');
    return;
  }
  if (!autoTestConfigIsValid(config)) {
    setEvent(`测试参数无效，速度不能超过当前档位 ${config.speedLimit}`);
    return;
  }

  stopHold(false);
  stopKeyboardDrive(false);
  stopJoystickDrive(false);
  stopAutoTest(false);
  autoTestRunning = true;
  autoTestFailureReason = '';
  const token = ++autoTestToken;
  let completed = false;
  let stopped = false;
  ui.autoTestState.textContent = 'RUN';
  updateControlAvailability();
  const testNames = {
    route: '航向折线路径',
    yaw: '角速度正反阶跃',
    disturbance: '0°抗扰保持',
  };
  appendLog('TEST', `自动测试开始: ${testNames[config.mode]}`);

  try {
    if (config.mode === 'route') completed = await runRouteAutoTest(config, token);
    else if (config.mode === 'yaw') completed = await runYawAutoTest(config, token);
    else completed = await runDisturbanceAutoTest(config, token);
  } catch (error) {
    autoTestFailureReason = `自动测试异常: ${error instanceof Error ? error.message : String(error)}`;
    appendLog('TEST', autoTestFailureReason, 'error');
  } finally {
    if (token !== autoTestToken) return;

    autoTestRunning = false;
    updateControlAvailability();
    stopped = await sendCommand('STOP');
    if (token !== autoTestToken) return;

    ui.autoTestState.textContent = completed && stopped ? 'DONE' : 'ERROR';
    setEvent(completed && stopped ? '自动测试完成，车辆已停车'
      : (autoTestFailureReason || '自动测试因发送失败而终止'));
    appendLog('TEST', completed && stopped ? '自动测试完成，车辆已停车'
      : (autoTestFailureReason || '自动测试已终止'), completed && stopped ? '' : 'error');
  }
}

function bindHoldButton(button, command) {
  button.addEventListener('pointerdown', (event) => {
    event.preventDefault();
    button.setPointerCapture?.(event.pointerId);
    startHold(command, button);
  });
  ['pointerup', 'pointercancel', 'lostpointercapture'].forEach((name) => {
    button.addEventListener(name, () => stopHold());
  });
}

function isTypingTarget(target) {
  return target instanceof HTMLInputElement || target instanceof HTMLSelectElement
    || target instanceof HTMLTextAreaElement || target.isContentEditable;
}

ui.connectButton.addEventListener('click', () => { void toggleConnection(); });
ui.connectionMode.addEventListener('change', () => {
  if (ui.connectionMode.value === 'bridge') ui.webPath.textContent = BRIDGE_URL;
  else if (ui.connectionMode.value === 'ble') ui.webPath.textContent = '浏览器直连 BLE FFE0 / FFE1';
  else ui.webPath.textContent = '浏览器直接占用串口';
});
ui.speedSlider.addEventListener('input', () => {
  ui.speedValue.value = ui.speedSlider.value;
  renderJoystickValues();
});
ui.speedSlider.addEventListener('change', () => { void sendCommand(`SPEED,${ui.speedSlider.value}`); });
ui.stopButton.addEventListener('click', () => {
  stopHold(false);
  stopKeyboardDrive(false);
  stopJoystickDrive(false);
  stopAutoTest(false);
  void sendCommand('STOP');
});
ui.zeroButton.addEventListener('click', () => { void sendCommand('ZERO'); });
ui.yawPidEnabled.addEventListener('change', () => {
  void sendCommand(`YAWON,${ui.yawPidEnabled.checked ? 1 : 0}`);
});
ui.resetYawPidButton.addEventListener('click', () => { void sendCommand('YAWRESET'); });
ui.calibrateGyroButton.addEventListener('click', () => { void sendCommand('GYROCAL'); });
ui.zeroYawButton.addEventListener('click', () => { void sendCommand('YAWZERO'); });
ui.headingEnabled.addEventListener('change', () => {
  void sendCommand(`HEADON,${ui.headingEnabled.checked ? 1 : 0}`);
});
ui.captureHeadingButton.addEventListener('click', () => { void sendCommand('HEADRESET'); });
function updateAutoTestModeUi() {
  const yawMode = ui.autoTestMode.value === 'yaw';
  const disturbanceMode = ui.autoTestMode.value === 'disturbance';
  ui.autoTestValueLabel.textContent = yawMode ? '角速度 (°/s)' : '直行速度 (RPM)';
  ui.autoTestDurationLabel.textContent = disturbanceMode ? '保持时间 (s)'
    : (yawMode ? '单段时间 (s)' : '每段直行时间 (s)');
  ui.autoTestDuration.max = disturbanceMode ? '60' : '15';
  ui.autoTestCyclesLabel.textContent = yawMode ? '正反循环' : '直线段数';
  ui.autoTestCycles.min = yawMode ? '1' : '2';
  ui.autoTestCycles.max = yawMode ? '10' : '8';
  ui.autoTestMotionFields.forEach((field) => { field.hidden = disturbanceMode; });
  ui.autoTestRouteFields.forEach((field) => { field.hidden = yawMode || disturbanceMode; });
}
ui.autoTestMode.addEventListener('change', updateAutoTestModeUi);
ui.startAutoTestButton.addEventListener('click', () => { void runAutoTest(); });
ui.stopAutoTestButton.addEventListener('click', () => { stopAutoTest(true); });

ui.driveButtons.forEach((button) => bindHoldButton(button, button.dataset.motion));
ui.jogButtons.forEach((button) => bindHoldButton(button, button.dataset.jog));
Object.values(joystickState).forEach(bindJoystick);
ui.clearLogButton.addEventListener('click', () => { ui.terminal.replaceChildren(); });
ui.commandForm.addEventListener('submit', (event) => {
  event.preventDefault();
  const command = ui.commandInput.value;
  ui.commandInput.value = '';
  void sendCommand(command);
});

document.addEventListener('keydown', (event) => {
  if (isTypingTarget(event.target)) return;
  const key = event.key.toUpperCase();
  if (!['W', 'A', 'S', 'D', 'Q', 'E'].includes(key)) return;
  event.preventDefault();
  startKeyboardDrive(key);
});
document.addEventListener('keyup', (event) => {
  releaseKeyboardDrive(event.key.toUpperCase());
});
window.addEventListener('blur', () => {
  stopHold();
  stopKeyboardDrive();
  stopJoystickDrive();
});
document.addEventListener('visibilitychange', () => {
  if (document.hidden) {
    stopHold();
    stopKeyboardDrive();
    stopJoystickDrive();
  }
});

setInterval(() => {
  if (telemetry.receivedAt > 0 && !telemetryFresh()) {
    ui.telemetryValue.textContent = '超时';
    updateControlAvailability();
  }
}, 250);

if ('serviceWorker' in navigator && window.isSecureContext) {
  navigator.serviceWorker.register('./service-worker.js').catch((error) => {
    appendLog('ERR', `离线缓存注册失败: ${error.message}`, 'error');
  });
}

renderTelemetry();
Object.values(joystickState).forEach(renderJoystick);
renderJoystickValues();
updateConnectionUi();
updateAutoTestModeUi();
appendLog('SYS', '控制台已启动');
