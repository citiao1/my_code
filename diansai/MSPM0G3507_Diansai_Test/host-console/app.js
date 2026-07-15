const BAUD_RATE = 9600;
const BRIDGE_URL = 'ws://127.0.0.1:8766';

const ids = [
  'connectionMode', 'connectionBadge', 'connectionText', 'connectButton',
  'motorBadge', 'stopButton', 'speedSlider', 'speedValue', 'driveCommand',
  'leftMotor', 'rightMotor', 'leftMotorValue', 'rightMotorValue', 'motorCommand',
  'runMotorsButton', 'stopMotorsButton', 'zeroEncoderButton', 'zeroImuButton',
  'grayWhiteButton', 'grayBlackButton', 'packetTime', 'yawValue', 'pitchValue',
  'rollValue', 'leftSpeed', 'rightSpeed', 'leftEncoder', 'rightEncoder',
  'leftPwm', 'rightPwm', 'linkValue', 'wheeltecValue', 'imuState', 'imuPins',
  'grayGrid', 'grayCalibrationState', 'grayCalibration', 'accelRaw', 'gyroRaw',
  'speedChart', 'browserStatus', 'eventLog'
];
const ui = Object.fromEntries(ids.map(id => [id, document.getElementById(id)]));

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
const history = { left: [], right: [] };

const grayChannels = Array.from({ length: 8 }, (_, index) => {
  const element = document.createElement('div');
  element.className = 'gray-channel';
  element.innerHTML = `<span>G${index}</span><div class="gray-meter"><i></i></div><strong>0</strong>`;
  ui.grayGrid.appendChild(element);
  return { value: element.querySelector('strong'), fill: element.querySelector('i') };
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
    return;
  }

  const values = fields.slice(1).map(Number);
  if (fields[0] === 'TEL' && values.length >= 23 && values.every(Number.isFinite)) {
    const [time, enabled, link, yaw10, leftMm, rightMm, , , pwmLeft, pwmRight] = values;
    ui.packetTime.textContent = `${time} ms`;
    ui.yawValue.textContent = `${(yaw10 / 10).toFixed(1)}°`;
    ui.leftSpeed.textContent = `${leftMm} mm/s`;
    ui.rightSpeed.textContent = `${rightMm} mm/s`;
    ui.leftPwm.textContent = `${Math.round(pwmLeft / 168)}%`;
    ui.rightPwm.textContent = `${Math.round(pwmRight / 168)}%`;
    ui.linkValue.textContent = link ? '控制中' : '待机';
    ui.imuState.textContent = values[15] ? '0x6B 正常' : '未检测到';
    ui.imuState.style.color = values[15] ? '#147d58' : '#bd2f2f';
    ui.motorBadge.className = `badge ${enabled ? 'enabled' : 'neutral'}`;
    ui.motorBadge.innerHTML = `<span class="dot"></span>${enabled ? '电机运行中' : '电机已停止'}`;
    pushHistory(leftMm, rightMm);
    return;
  }

  if (fields[0] === 'STA' && values.length >= 30 && values.every(Number.isFinite)) {
    ui.pitchValue.textContent = `${(values[1] / 10).toFixed(1)}°`;
    ui.rollValue.textContent = `${(values[2] / 10).toFixed(1)}°`;
    ui.leftEncoder.textContent = values[3].toLocaleString();
    ui.rightEncoder.textContent = values[4].toLocaleString();
    values.slice(22, 30).forEach(updateGray);
    const white = Boolean(values[32]);
    const black = Boolean(values[33]);
    ui.grayCalibrationState.textContent = white && black ? '标定完成' : (white ? '已采白底' : '未标定');
    ui.grayCalibrationState.classList.toggle('complete', white && black);
    return;
  }

  if (fields[0] === 'DBG' && values.length >= 7 && values.every(Number.isFinite)) {
    ui.accelRaw.textContent = `${values[1]} / ${values[2]} / ${values[3]}`;
    ui.gyroRaw.textContent = `${values[4]} / ${values[5]} / ${values[6]}`;
    if (values.length >= 9) ui.imuPins.textContent = `MOSI PA${values[7]} · MISO PA${values[8]}`;
    return;
  }

  if (fields[0] === 'CAL' && values.length >= 16 && values.every(Number.isFinite)) {
    ui.grayCalibration.textContent = `白底 ${values.slice(0, 8).join('/')} · 黑线 ${values.slice(8, 16).join('/')}`;
    return;
  }

  if (fields[0] === 'BOOT' || fields[0] === 'ACK' || fields[0] === 'ERR') {
    setLog(line);
  }
}

function updateGray(value, index) {
  const raw = Math.max(0, Math.min(4095, value));
  grayChannels[index].value.textContent = Math.round(raw);
  grayChannels[index].fill.style.height = `${raw * 100 / 4095}%`;
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
    controlTimer = setTimeout(() => controlLoop(generation, commandFactory), 100);
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
  controlGeneration++;
  controlRunning = false;
  clearTimeout(controlTimer);
  activeDriveButton?.classList.remove('active');
  activeDriveButton = undefined;
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

function updateMotorInputs() {
  ui.leftMotorValue.textContent = `${ui.leftMotor.value}%`;
  ui.rightMotorValue.textContent = `${ui.rightMotor.value}%`;
  ui.motorCommand.textContent = `L ${ui.leftMotor.value}% · R ${ui.rightMotor.value}%`;
}

function pushHistory(left, right) {
  history.left.push(left);
  history.right.push(right);
  if (history.left.length > 120) history.left.shift();
  if (history.right.length > 120) history.right.shift();
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
  const peak = Math.max(100, ...history.left.map(Math.abs), ...history.right.map(Math.abs));
  const plot = (series, color) => {
    if (series.length < 2) return;
    ctx.strokeStyle = color;
    ctx.lineWidth = 2;
    ctx.beginPath();
    series.forEach((value, index) => {
      const x = index * w / 119;
      const y = h / 2 - value / peak * h * .42;
      if (index === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
    });
    ctx.stroke();
  };
  plot(history.left, '#147d58');
  plot(history.right, '#167a94');
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

ui.connectButton.addEventListener('click', () => connectedTransport ? disconnect() : connect());
ui.connectionMode.addEventListener('change', () => {
  ui.connectButton.textContent = ui.connectionMode.value === 'bridge' ? '连接桥接' : '连接串口';
  ui.browserStatus.textContent = ui.connectionMode.value === 'bridge'
    ? `WHEELTEC bridge · ${BRIDGE_URL}`
    : `Web Serial · ${BAUD_RATE} baud`;
});
ui.stopButton.addEventListener('click', () => stopControl());
ui.stopMotorsButton.addEventListener('click', () => stopControl());
ui.speedSlider.addEventListener('input', () => { ui.speedValue.textContent = `${ui.speedSlider.value}%`; });
ui.leftMotor.addEventListener('input', updateMotorInputs);
ui.rightMotor.addEventListener('input', updateMotorInputs);
ui.runMotorsButton.addEventListener('click', () => startControl(
  () => `MOTOR,${Number(ui.leftMotor.value)},${Number(ui.rightMotor.value)}`
));
ui.zeroEncoderButton.addEventListener('click', () => sendLine('ENCZERO'));
ui.zeroImuButton.addEventListener('click', () => sendLine('IMUZERO'));
ui.grayWhiteButton.addEventListener('click', () => sendLine('GRAYWHITE'));
ui.grayBlackButton.addEventListener('click', () => sendLine('GRAYBLACK'));
window.addEventListener('blur', () => { if (controlRunning) stopControl(); });
document.addEventListener('visibilitychange', () => { if (document.hidden && controlRunning) stopControl(); });
navigator.serial?.addEventListener('disconnect', event => { if (event.target === port && !closing) disconnect(false); });
new ResizeObserver(drawChart).observe(ui.speedChart);

setConnected(false);
updateMotorInputs();
drawChart();
