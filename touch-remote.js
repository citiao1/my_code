const TARGET_STORAGE_KEY = 'c30d-remote-target';
const GITHUB_PAGES_HOST = 'citiao1.github.io';

function normalizeRemoteTarget(value) {
  if (!value) return '';
  try {
    const target = new URL(value);
    if (!['http:', 'https:'].includes(target.protocol)) return '';
    if (window.location.protocol === 'https:' && target.protocol !== 'https:') return '';
    return target.origin;
  } catch (_) {
    return '';
  }
}

function loadRemoteTarget() {
  const queryTarget = normalizeRemoteTarget(new URLSearchParams(window.location.search).get('target'));
  if (queryTarget) {
    try { window.localStorage.setItem(TARGET_STORAGE_KEY, queryTarget); } catch (_) {}
    return queryTarget;
  }
  try { return normalizeRemoteTarget(window.localStorage.getItem(TARGET_STORAGE_KEY)); } catch (_) { return ''; }
}

function buildRemoteEndpoints() {
  const savedTarget = loadRemoteTarget();
  if (savedTarget) {
    const target = new URL(savedTarget);
    return {
      ready: true,
      bridge: `${target.protocol === 'https:' ? 'wss:' : 'ws:'}//${target.hostname}:8443`,
      camera: `${target.protocol}//${target.hostname}:10000/camera`,
    };
  }

  if (window.location.hostname === GITHUB_PAGES_HOST) {
    return { ready: false, bridge: '', camera: '' };
  }

  const host = window.location.hostname || '127.0.0.1';
  if (window.location.protocol === 'https:') {
    return {
      ready: true,
      bridge: `wss://${window.location.hostname}:8443`,
      camera: `https://${window.location.hostname}:10000/camera`,
    };
  }

  return {
    ready: true,
    bridge: `ws://${host}:8766`,
    camera: `http://${host}:8889/camera`,
  };
}

const REMOTE_ENDPOINTS = buildRemoteEndpoints();
const BRIDGE_URL = REMOTE_ENDPOINTS.bridge;
const CAMERA_URL = REMOTE_ENDPOINTS.camera +
  '?controls=false&muted=true&autoplay=true&playsInline=true';
const HOLD_REFRESH_MS = 100;
const TELEMETRY_STALE_MS = 900;
const JOYSTICK_DEAD_ZONE = 0.10;

const ui = {
  connectionBadge: document.querySelector('#connectionBadge'),
  connectButton: document.querySelector('#connectButton'),
  cameraFrame: document.querySelector('#cameraFrame'),
  cameraPlaceholder: document.querySelector('#cameraPlaceholder'),
  cameraState: document.querySelector('#cameraState'),
  cameraReloadButton: document.querySelector('#cameraReloadButton'),
  speedSlider: document.querySelector('#speedSlider'),
  speedValue: document.querySelector('#speedValue'),
  translationJoystick: document.querySelector('#translationJoystick'),
  translationJoystickValue: document.querySelector('#translationJoystickValue'),
  rotationJoystick: document.querySelector('#rotationJoystick'),
  rotationJoystickValue: document.querySelector('#rotationJoystickValue'),
  stopButton: document.querySelector('#stopButton'),
  eventText: document.querySelector('#eventText'),
};

let bridgeSocket;
let hardwareConnected = false;
let receiveBuffer = '';
let writeChain = Promise.resolve();
let connecting = false;
let manualDisconnect = false;
let reconnectTimer;
let cameraRetryTimer;
let joystickTimer;
let joystickCommandActive = false;
let joystickLastSendMs = 0;

const firmware = {
  enabled: false,
  speedPidEnabled: false,
  gyroReady: false,
  yawEnabled: false,
  receivedAt: 0,
};

const joystickState = {
  translation: createJoystickState(ui.translationJoystick),
  rotation: createJoystickState(ui.rotationJoystick),
};

function createJoystickState(element) {
  return {
    element,
    knob: element.querySelector('.joystick-knob'),
    pointerId: undefined,
    x: 0,
    y: 0,
  };
}

function setEvent(message) {
  ui.eventText.textContent = message;
}

function setBadge(state, text) {
  ui.connectionBadge.className = `badge ${state}`;
  ui.connectionBadge.replaceChildren();
  const dot = document.createElement('span');
  dot.className = 'dot';
  ui.connectionBadge.append(dot, document.createTextNode(text));
}

function websocketOpen() {
  return bridgeSocket?.readyState === WebSocket.OPEN;
}

function connected() {
  return websocketOpen() && hardwareConnected;
}

function telemetryFresh() {
  return firmware.receivedAt > 0 && Date.now() - firmware.receivedAt <= TELEMETRY_STALE_MS;
}

function translationReady() {
  return connected() && telemetryFresh() && firmware.enabled && firmware.speedPidEnabled;
}

function rotationReady() {
  return translationReady() && firmware.gyroReady && firmware.yawEnabled;
}

function updateConnectionUi() {
  ui.connectButton.textContent = websocketOpen() ? '断开' : (connecting ? '连接中' : '连接');
  ui.connectButton.disabled = connecting;
  ui.stopButton.disabled = !connected();

  if (connected()) setBadge('online', '控制在线');
  else if (websocketOpen()) setBadge('waiting', '等待 STM32');
  else setBadge('offline', '未连接');

  ui.translationJoystick.setAttribute('aria-disabled', String(!translationReady()));
  ui.rotationJoystick.setAttribute('aria-disabled', String(!rotationReady()));
}

function setCameraState(message, online = false) {
  ui.cameraState.textContent = message;
  ui.cameraState.className = online ? 'connected' : '';
}

function startCamera() {
  clearTimeout(cameraRetryTimer);
  ui.cameraPlaceholder.hidden = false;
  if (!REMOTE_ENDPOINTS.ready) {
    ui.cameraPlaceholder.textContent = '请先配置香橙派 HTTPS 地址';
    setCameraState('等待配置');
    return;
  }
  setCameraState('连接中');
  ui.cameraFrame.src = '';
  window.setTimeout(() => { ui.cameraFrame.src = CAMERA_URL; }, 30);
}

async function writeCommand(command) {
  if (!connected()) return false;
  writeChain = writeChain.catch(() => {}).then(() => {
    if (!websocketOpen()) throw new Error('控制链路已断开');
    bridgeSocket.send(`${command}\n`);
  });
  try {
    await writeChain;
    return true;
  } catch (_) {
    setEvent('命令发送失败');
    return false;
  }
}

async function initializeFirmwareLink() {
  await writeCommand('STOP');
  await writeCommand(`SPEED,${ui.speedSlider.value}`);
  await writeCommand('STATUS');
}

function handleBridgeStatus(fields) {
  const wasConnected = hardwareConnected;
  hardwareConnected = fields[1] === '1';
  setEvent(fields.slice(2).join(',') || (hardwareConnected ? 'STM32 已连接' : '等待 STM32'));
  updateConnectionUi();

  if (!wasConnected && hardwareConnected) void initializeFirmwareLink();
  if (!hardwareConnected) stopJoystickDrive(false);
}

function handleTelemetry(fields) {
  if (fields.length < 27) return;
  const enabled = Number(fields[3]);
  const speedPidEnabled = Number(fields[26]);
  if (![enabled, speedPidEnabled].every(Number.isFinite)) return;

  firmware.enabled = enabled === 1;
  firmware.speedPidEnabled = speedPidEnabled === 1;
  firmware.gyroReady = fields.length >= 39 && Number(fields[28]) === 1;
  firmware.yawEnabled = fields.length >= 39 && Number(fields[38]) === 1;
  firmware.receivedAt = Date.now();
  updateConnectionUi();
}

function handleLine(line) {
  const fields = line.split(',');
  if (fields[0] === 'STATUS') {
    handleBridgeStatus(fields);
    return;
  }
  if (fields[0] === 'TEL') {
    handleTelemetry(fields);
    return;
  }
  if (fields[0] === 'ACK' && fields[1] === 'SPEED' && fields[2]) {
    ui.speedSlider.value = fields[2];
    ui.speedValue.value = fields[2];
    renderJoystickValues();
    return;
  }
  if (fields[0] === 'ERR') setEvent(line);
}

function consumeText(text) {
  receiveBuffer += text;
  const lines = receiveBuffer.replaceAll('\r', '').split('\n');
  receiveBuffer = lines.pop();
  lines.map((line) => line.trim()).filter(Boolean).forEach(handleLine);
}

function scheduleReconnect() {
  clearTimeout(reconnectTimer);
  if (manualDisconnect || document.hidden) return;
  reconnectTimer = setTimeout(() => { void connectBridge(); }, 2000);
}

async function connectBridge() {
  if (connecting || websocketOpen()) return;
  if (!REMOTE_ENDPOINTS.ready) {
    setEvent('请使用香橙派脚本输出的 GitHub 链接打开');
    updateConnectionUi();
    return;
  }
  clearTimeout(reconnectTimer);
  connecting = true;
  updateConnectionUi();

  try {
    const socket = new WebSocket(BRIDGE_URL);
    bridgeSocket = socket;
    socket.addEventListener('message', (event) => consumeText(String(event.data)));
    socket.addEventListener('close', () => {
      if (bridgeSocket !== socket) return;
      hardwareConnected = false;
      bridgeSocket = undefined;
      firmware.receivedAt = 0;
      stopJoystickDrive(false);
      setEvent('控制链路已断开');
      updateConnectionUi();
      scheduleReconnect();
    });

    await new Promise((resolve, reject) => {
      const timeout = setTimeout(() => reject(new Error('连接超时')), 4000);
      socket.addEventListener('open', () => {
        clearTimeout(timeout);
        resolve();
      }, { once: true });
      socket.addEventListener('error', () => {
        clearTimeout(timeout);
        reject(new Error('无法连接香橙派控制服务'));
      }, { once: true });
    });
    setEvent('已连接香橙派，等待 STM32');
  } catch (error) {
    bridgeSocket?.close();
    bridgeSocket = undefined;
    hardwareConnected = false;
    setEvent(error.message);
    scheduleReconnect();
  } finally {
    connecting = false;
    updateConnectionUi();
  }
}

async function disconnectBridge() {
  manualDisconnect = true;
  clearTimeout(reconnectTimer);
  stopJoystickDrive();
  if (connected()) await writeCommand('STOP');
  bridgeSocket?.close();
  bridgeSocket = undefined;
  hardwareConnected = false;
  firmware.receivedAt = 0;
  setEvent('连接已关闭');
  updateConnectionUi();
}

function applyAxisDeadZone(value) {
  const magnitude = Math.abs(value);
  if (magnitude <= JOYSTICK_DEAD_ZONE) return 0;
  return Math.sign(value) * (magnitude - JOYSTICK_DEAD_ZONE) / (1 - JOYSTICK_DEAD_ZONE);
}

function translationVector() {
  const { x, y } = joystickState.translation;
  const magnitude = Math.hypot(x, y);
  if (magnitude <= JOYSTICK_DEAD_ZONE) return { x: 0, y: 0 };
  const scaled = Math.min(1, (magnitude - JOYSTICK_DEAD_ZONE) / (1 - JOYSTICK_DEAD_ZONE));
  return { x: x * scaled / magnitude, y: y * scaled / magnitude };
}

function joystickCommand() {
  const speed = Number(ui.speedSlider.value);
  const translation = translationVector();
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
  const command = joystickCommand();
  ui.translationJoystickValue.textContent = `前后 ${command.forward} · 左右 ${command.left}`;
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

function sendJoystickDrive(force = false) {
  const command = joystickCommand();
  const moving = command.forward !== 0 || command.left !== 0 || command.yaw !== 0;
  const now = performance.now();

  if (!moving) {
    if (joystickCommandActive && connected()) void writeCommand('STOP');
    joystickCommandActive = false;
    return;
  }
  if (!force && now - joystickLastSendMs < 45) return;
  if (!translationReady() || (command.yaw !== 0 && !rotationReady())) {
    setEvent(command.yaw === 0 ? 'STM32 未就绪或遥测超时' : '陀螺仪未就绪');
    stopJoystickDrive();
    return;
  }

  joystickLastSendMs = now;
  joystickCommandActive = true;
  void writeCommand(`DRV,${command.forward},${command.left},${command.yaw}`);
}

function startJoystick(control, event) {
  if (control.element.getAttribute('aria-disabled') === 'true' || control.pointerId !== undefined) {
    setEvent('控制链路尚未就绪');
    return;
  }
  event.preventDefault();
  control.pointerId = event.pointerId;
  control.element.setPointerCapture?.(event.pointerId);
  control.element.classList.add('active');
  updateJoystickFromPointer(control, event);
  if (!joystickTimer) joystickTimer = setInterval(() => sendJoystickDrive(true), HOLD_REFRESH_MS);
  sendJoystickDrive(true);
}

function moveJoystick(control, event) {
  if (control.pointerId !== event.pointerId) return;
  event.preventDefault();
  updateJoystickFromPointer(control, event);
  sendJoystickDrive();
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

  const anotherActive = Object.values(joystickState).some((item) => item.pointerId !== undefined);
  if (anotherActive) sendJoystickDrive(true);
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
  renderJoystickValues();
  if (sendStop && wasActive && connected()) void writeCommand('STOP');
}

function bindJoystick(control) {
  control.element.addEventListener('pointerdown', (event) => startJoystick(control, event));
  control.element.addEventListener('pointermove', (event) => moveJoystick(control, event));
  ['pointerup', 'pointercancel', 'lostpointercapture'].forEach((name) => {
    control.element.addEventListener(name, (event) => releaseJoystick(control, event));
  });
}

ui.connectButton.addEventListener('click', () => {
  if (websocketOpen()) void disconnectBridge();
  else {
    manualDisconnect = false;
    void connectBridge();
  }
});
ui.cameraReloadButton.addEventListener('click', startCamera);
ui.cameraFrame.addEventListener('load', () => {
  ui.cameraPlaceholder.hidden = true;
  setCameraState('图传窗口');
});
ui.speedSlider.addEventListener('input', () => {
  ui.speedValue.value = ui.speedSlider.value;
  renderJoystickValues();
});
ui.speedSlider.addEventListener('change', () => {
  void writeCommand(`SPEED,${ui.speedSlider.value}`);
  if (joystickCommandActive) sendJoystickDrive(true);
});
ui.stopButton.addEventListener('click', () => {
  stopJoystickDrive(false);
  void writeCommand('STOP');
  setEvent('已停车');
});
Object.values(joystickState).forEach(bindJoystick);

window.addEventListener('blur', () => stopJoystickDrive());
window.addEventListener('pagehide', () => stopJoystickDrive());
window.addEventListener('resize', () => Object.values(joystickState).forEach(renderJoystick));
document.addEventListener('visibilitychange', () => {
  if (document.hidden) {
    stopJoystickDrive();
    clearTimeout(cameraRetryTimer);
    setCameraState('后台暂停');
  } else {
    startCamera();
    if (!manualDisconnect && !websocketOpen()) void connectBridge();
  }
});

setInterval(() => {
  if (firmware.receivedAt > 0 && !telemetryFresh()) {
    firmware.receivedAt = 0;
    stopJoystickDrive();
    setEvent('STM32 遥测超时，已停车');
    updateConnectionUi();
  }
}, 250);

if ('serviceWorker' in navigator && window.isSecureContext) {
  navigator.serviceWorker.register('./service-worker.js').catch(() => {});
}

Object.values(joystickState).forEach(renderJoystick);
renderJoystickValues();
updateConnectionUi();
startCamera();
void connectBridge();
