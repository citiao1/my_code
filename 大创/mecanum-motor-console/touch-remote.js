const TARGET_STORAGE_KEY = 'bm50-console-target';
const GITHUB_PAGES_HOST = 'citiao1.github.io';
const REQUIRED_FIRMWARE = 'VGT6_BM50_ATTITUDE_R7';
const MAX_TARGET_RPM = 200;

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

function buildBridgeUrl() {
  const queryTarget = normalizeRemoteTarget(
    new URLSearchParams(window.location.search).get('target')
  );
  const localHost = ['127.0.0.1', 'localhost', ''].includes(window.location.hostname);
  const savedTarget = queryTarget || (!localHost &&
    normalizeRemoteTarget(localStorage.getItem(TARGET_STORAGE_KEY)));
  if (queryTarget) localStorage.setItem(TARGET_STORAGE_KEY, queryTarget);
  if (savedTarget) {
    const target = new URL(savedTarget);
    return `${target.protocol === 'https:' ? 'wss:' : 'ws:'}//${target.host}/ws`;
  }
  if (window.location.hostname === GITHUB_PAGES_HOST) return '';
  return `ws://${window.location.hostname || '127.0.0.1'}:8766`;
}

const BRIDGE_URL = buildBridgeUrl();
const ui = {
  badge: document.querySelector('#connectionBadge'),
  connect: document.querySelector('#connectButton'),
  arm: document.querySelector('#armButton'),
  speed: document.querySelector('#speedEnableButton'),
  stop: document.querySelector('#stopButton'),
  limit: document.querySelector('#speedLimitInput'),
  limitButton: document.querySelector('#speedLimitButton'),
  telemetry: document.querySelector('#telemetrySelect'),
  status: document.querySelector('#speedStatus'),
  event: document.querySelector('#eventText'),
  version: document.querySelector('#firmwareVersion'),
  link: document.querySelector('#linkStatus'),
  uart: document.querySelector('#uartStatus'),
  stack: document.querySelector('#stackStatus'),
  log: document.querySelector('#commandLog'),
  query: document.querySelector('#queryButton'),
};

let socket;
let hardwareConnected = false;
let bridgeReady = false;
let firmwareCompatible = false;
let stackHealthy = false;
let receiveBuffer = '';
let connecting = false;
let busy = false;
let operationSerial = 0;
let requestSerial = 0;
let ownsAngleMotion = false;
let keepPending = false;
const chartSamples = [];
const pending = new Map();
const logLines = [];
const firmware = {
  speedEnabled: false, telemetryEnabled: false,
  receivedAt: 0, expectedAt: 0, timeoutReported: false,
};

function setEvent(text) { ui.event.textContent = text; }
function logCommand(text) {
  logLines.push(text);
  if (logLines.length > 12) logLines.shift();
  ui.log.textContent = logLines.join('\n');
}
function isOpen() { return socket?.readyState === WebSocket.OPEN; }
function connected() { return isOpen() && hardwareConnected && bridgeReady; }
function setBadge(state, text) {
  ui.badge.className = `badge ${state}`;
  ui.badge.replaceChildren();
  const dot = document.createElement('span');
  dot.className = 'dot';
  ui.badge.append(dot, document.createTextNode(text));
}

function updateUi() {
  ui.connect.textContent = isOpen() ? '断开' : (connecting ? '连接中' : '连接');
  ui.connect.disabled = connecting;
  const disabled = !connected() || busy || !firmwareCompatible || !stackHealthy;
  [ui.arm, ui.speed, ui.limitButton, ui.telemetry]
    .forEach((element) => { element.disabled = disabled; });
  ui.query.disabled = !connected() || busy;
  ui.stop.disabled = !connected();
  document.querySelectorAll('.pid-send').forEach((button) => { button.disabled = disabled; });
  document.querySelectorAll('.angle-command,.drive-command').forEach((button) => { button.disabled = disabled; });
  document.querySelector('#padStop').disabled = !connected();
  ui.speed.textContent = firmware.speedEnabled ? '关闭速度环' : '启用速度环';
  if (connected()) setBadge('online', '蓝牙在线');
  else if (isOpen()) setBadge('waiting', '等待蓝牙');
  else setBadge('offline', '未连接');
}

function resetLinkState() {
  ownsAngleMotion = false;
  chartSamples.length = 0;
  operationSerial += 1;
  busy = false;
  receiveBuffer = '';
  firmware.speedEnabled = false;
  firmwareCompatible = false;
  stackHealthy = false;
  setTelemetry(false);
  firmware.receivedAt = 0;
  ui.version.textContent = '未查询';
  ui.link.textContent = '未查询';
  ui.uart.textContent = '未查询';
  ui.stack.textContent = '未查询';
  ui.status.textContent = '等待状态查询';
  for (const request of pending.values()) {
    clearTimeout(request.timer);
    request.reject(new Error('连接已断开'));
  }
  pending.clear();
}

function setTelemetry(enabled) {
  firmware.telemetryEnabled = enabled;
  firmware.expectedAt = enabled ? Date.now() : 0;
  firmware.timeoutReported = false;
  ui.telemetry.value = enabled ? '1' : '0';
}

function writeCommand(command) {
  if (!connected()) return Promise.reject(new Error('蓝牙未连接'));
  const diagnostic = ['VER', 'UART', 'LINK', 'STACK', 'SPEEDSTATUS', 'SPEEDLIMIT', 'PIDGET', 'PING', 'STOP', 'CAPS', 'YAWGET', 'HEADGET', 'ANGLESTATUS'].includes(command);
  if (!diagnostic && (!firmwareCompatible || !stackHealthy)) {
    return Promise.reject(new Error('固件版本或栈保护检查未通过；请查询状态'));
  }
  const id = ++requestSerial;
  logCommand(`发送 > ${command}`);
  return new Promise((resolve, reject) => {
    const timer = setTimeout(() => {
      pending.delete(id);
      reject(new Error(`桥接应答超时：${command}；请重启桥接程序`));
    }, 5000);
    pending.set(id, { resolve, reject, timer });
    socket.send(JSON.stringify({ type: 'command', id, command }));
  });
}

function handleResult(result) {
  const request = pending.get(result.id);
  if (!request) return;
  pending.delete(result.id);
  clearTimeout(request.timer);
  if (result.ok) request.resolve(result.reply);
  else request.reject(new Error(result.error || '命令执行失败'));
}

async function runAction(action, urgent = false) {
  if (!connected() || (busy && !urgent)) return;
  const epoch = ++operationSerial;
  busy = true;
  updateUi();
  const send = async (command) => {
    if (epoch !== operationSerial) throw new Error('操作已取消');
    return writeCommand(command);
  };
  try {
    await action(send);
  } catch (error) {
    if (epoch === operationSerial) {
      logCommand(`错误 ! ${error.message}`);
      setEvent(error.message);
    }
  } finally {
    if (epoch === operationSerial) {
      busy = false;
      updateUi();
    }
  }
}

function handleTelemetry(fields) {
  if (fields.length !== 27 || fields.slice(1).some((field) => field === '' || !Number.isFinite(Number(field)))) {
    setEvent(`TEL 格式错误：收到 ${fields.length} 字段，预期 27`);
    return;
  }
  firmware.speedEnabled = Number(fields[26]) === 1;
  firmware.receivedAt = Date.now();
  if (firmware.timeoutReported) setEvent('TEL 遥测已恢复');
  firmware.timeoutReported = false;
  ui.status.textContent =
    `A ${(Number(fields[12] || 0) / 10).toFixed(1)}/${fields[4] || 0}  ` +
    `B ${(Number(fields[13] || 0) / 10).toFixed(1)}/${fields[5] || 0}  ` +
    `C ${(Number(fields[14] || 0) / 10).toFixed(1)}/${fields[6] || 0}  ` +
    `D ${(Number(fields[15] || 0) / 10).toFixed(1)}/${fields[7] || 0} RPM`;
  document.querySelector('#batteryActual').textContent = `${(Number(fields[20])/1000).toFixed(2)} V`;
  document.querySelector('#rateActual').textContent = `${(Number(fields[22])/1000).toFixed(2)} °/s`;
  document.querySelector('#yawActual').textContent = `${(Number(fields[25])/1000).toFixed(2)} °`;
  if (document.querySelector('#chartLoop').value === 'wheel')
    plotSample(Number(fields[1]), Number(fields[4]), Number(fields[12])/10);
  updateUi();
}

function handleAttitude(fields) {
  if (fields.length !== 14 || fields.slice(1).some(x => x === '' || !Number.isFinite(Number(x)))) {
    setEvent('ATEL 格式错误');
    return;
  }
  firmware.receivedAt = Date.now();
  firmware.timeoutReported = false;
  firmware.speedEnabled = fields[12] === '1';
  document.querySelector('#rateActual').textContent = `${(Number(fields[5])/1000).toFixed(2)} °/s`;
  document.querySelector('#yawActual').textContent = `${(Number(fields[8])/1000).toFixed(2)} °`;
  document.querySelector('#batteryActual').textContent = `${(Number(fields[13])/1000).toFixed(2)} V`;
  document.querySelector('#angleMode').textContent =
    ['停止','角速度 PI','方向 PD'][Number(fields[2])] || '未知';
  if (fields[2] === '0' || fields[3] !== '1') ownsAngleMotion = false;
  const view = document.querySelector('#chartLoop').value;
  if (view === 'rate') plotSample(Number(fields[1]), Number(fields[4])/1000, Number(fields[5])/1000);
  if (view === 'heading') plotSample(Number(fields[1]), Number(fields[7])/1000, Number(fields[8])/1000);
  if (fields[11] === '1') setEvent('角环输出限幅中；当前响应不适合继续提高增益');
  updateUi();
}

function plotSample(tick, target, actual) {
  chartSamples.push({tick,target,actual});
  if (chartSamples.length > 200) chartSamples.shift();
  drawChart();
}
function drawChart() {
  const canvas = document.querySelector('#responseChart');
  const ctx = canvas.getContext('2d');
  const w = canvas.width, h = canvas.height;
  ctx.clearRect(0,0,w,h);
  if (!chartSamples.length) return;
  const points = chartSamples.map(x => ({...x}));
  if (document.querySelector('#chartLoop').value === 'heading') {
    const wrap = x => ((x+180)%360+360)%360-180;
    for (let i=1;i<points.length;i++) {
      points[i].actual = points[i-1].actual + wrap(points[i].actual-points[i-1].actual);
      points[i].target = points[i].actual + wrap(points[i].target-points[i].actual);
    }
  }
  let lo = Math.min(...points.flatMap(x => [x.actual,x.target]));
  let hi = Math.max(...points.flatMap(x => [x.actual,x.target]));
  const margin = Math.max(1,(hi-lo)*0.15); lo-=margin; hi+=margin;
  const t0=points[0].tick, span=Math.max(1000,points.at(-1).tick-t0);
  ctx.font='12px Consolas'; ctx.lineWidth=1;
  for (let i=0;i<=4;i++) {
    const y=20+(h-50)*i/4;
    ctx.strokeStyle='#d7ddda'; ctx.beginPath(); ctx.moveTo(48,y); ctx.lineTo(w-16,y); ctx.stroke();
    ctx.fillStyle='#68736f'; ctx.fillText((hi-(hi-lo)*i/4).toFixed(1),4,y+4);
  }
  for (const [key,color] of [['target','#147d58'],['actual','#167a94']]) {
    ctx.strokeStyle=color; ctx.lineWidth=2; ctx.beginPath();
    points.forEach((p,i) => {
      const x=48+(w-64)*(p.tick-t0)/span;
      const y=20+(h-50)*(hi-p[key])/(hi-lo);
      if (i) ctx.lineTo(x,y); else ctx.moveTo(x,y);
    }); ctx.stroke();
  }
  ctx.fillStyle='#68736f'; ctx.fillText(`${((points.at(-1).tick-t0)/1000).toFixed(1)} s`,w-70,h-8);
}

function handleLine(line) {
  const fields = line.split(',');
  if (fields[0] === 'BRIDGE') {
    bridgeReady = fields[1] === 'BM50_ACK_R3';
    if (!bridgeReady) setEvent('桥接版本不匹配，请关闭启动窗口后重新运行启动脚本');
    updateUi();
    return;
  }
  if (fields[0] === 'STATUS') {
    const wasConnected = hardwareConnected;
    hardwareConnected = fields[1] === '1';
    if (!hardwareConnected) resetLinkState();
    setEvent(bridgeReady ? (fields.slice(2).join(',') || '蓝牙状态更新') :
      '旧桥接程序仍在运行，请关闭启动窗口，再重新运行“启动上位机.bat”');
    updateUi();
    if (hardwareConnected && !wasConnected && bridgeReady) {
      // Query only: connecting must not start motors or enable a stream.
      void runAction(async (send) => {
        await send('VER');
        if (!firmwareCompatible) throw new Error(`需要烧录 ${REQUIRED_FIRMWARE}；目前固件不兼容`);
        await send('UART');
        await send('LINK');
        await send('STACK');
        if (!stackHealthy) throw new Error('栈保护异常，已禁用电机控制；请重新启动下位机');
        await send('SPEEDLIMIT');
        await send('SPEEDSTATUS');
        await send('PIDGET');
        await send('CAPS');
        await send('YAWGET');
        await send('HEADGET');
        await send('ANGLESTATUS');
        await send('STACK');
        if (!stackHealthy) throw new Error('查询后栈保护异常，已禁用电机控制');
        setEvent('下位机应答正常；遥测由开关或速度环启动控制');
      });
    }
    return;
  }
  if (fields[0] === 'VER') {
    const version = fields.slice(1).join(',') || '未知固件';
    ui.version.textContent = version;
    firmwareCompatible = version === REQUIRED_FIRMWARE;
    logCommand(`收到 < ${line}`);
    setEvent(firmwareCompatible ? `固件：${version}` : `固件不兼容：请烧录 ${REQUIRED_FIRMWARE}`);
    updateUi();
    return;
  }
  if (fields[0] === 'UART' && fields.length === 10) {
    ui.uart.textContent =
      `${fields[1]} baud；PCLK ${fields[2]} Hz；BRR ${fields[3]}；最后错误 ${fields[4]}；` +
      `PE ${fields[5]} / NE ${fields[6]} / FE ${fields[7]} / ORE ${fields[8]} / DMA ${fields[9]}`;
    logCommand(`收到 < ${line}`);
    return;
  }
  if (fields[0] === 'STACK' && fields.length === 5) {
    const values = fields.slice(1).map(Number);
    stackHealthy = fields.slice(1).every((field) => field !== '') &&
      values.every(Number.isFinite) && values[0] === 4096 &&
      values[1] >= 0 && values[2] >= 32 && values[1] + values[2] === values[0] && values[3] === 1;
    ui.stack.textContent =
      `分配 ${fields[1]} B；已用水位 ${fields[2]} B；剩余 ${fields[3]} B；保护 ${stackHealthy ? '正常' : '异常'}`;
    logCommand(`收到 < ${line}`);
    if (!stackHealthy) setEvent('栈保护异常，已禁用电机控制；请重新启动下位机');
    updateUi();
    return;
  }
  if (fields[0] === 'RXHEX') {
    logCommand(`收到 < ${line}`);
    return;
  }
  if (fields[0] === 'LINK' && fields.length === 7) {
    setTelemetry(fields[6] === '1');
    ui.link.textContent =
      `RX ${fields[1]} 字节 / ${fields[2]} 条；溢出 ${fields[3]}；TX 丢弃 ${fields[4]}；串口错误 ${fields[5]}`;
    logCommand(`收到 < ${line}`);
    return;
  }
  if (fields[0] === 'SPEEDLIMIT' && fields.length === 2) {
    const limit = Number(fields[1]);
    if (Number.isInteger(limit) && limit >= 1 && limit <= MAX_TARGET_RPM) {
      ui.limit.value = limit;
      logCommand(`收到 < ${line}`);
    } else {
      setEvent(`SPEEDLIMIT 格式错误：${line}`);
    }
    return;
  }
  if (fields[0] === 'PID' && fields.length === 13) {
    document.querySelectorAll('.pid-row[data-motor]').forEach((row, index) => {
      ['.kp-input', '.ki-input', '.kd-input'].forEach((selector, offset) => {
        const input = row.querySelector(selector);
        if (document.activeElement !== input) input.value = fields[1 + index * 3 + offset];
      });
    });
    return;
  }
  if ((fields[0] === 'YAWPID' || fields[0] === 'HEADPID') && fields.length === 6) {
    const rate = fields[0] === 'YAWPID';
    const ids = rate ? ['#rateKp','#rateKi'] : ['#headKp','#headKd'];
    ids.forEach((id,i) => {
      const input = document.querySelector(id);
      if (document.activeElement !== input) input.value=fields[i+1];
    });
    document.querySelector(rate ? '#rateReadback' : '#headingReadback').textContent =
      `已回读 ${fields[1]} / ${fields[2]}`;
    logCommand(`收到 < ${line}`);
    return;
  }
  if (fields[0] === 'ATEL') { handleAttitude(fields); return; }
  if (fields[0] === 'ACK' && ['YAW','HEAD','MOVE','ATEL'].includes(fields[1])) {
    setTelemetry(fields[1] !== 'ATEL' || fields[2] === '1');
    updateUi();
    return;
  }
  if (fields[0] === 'TEL') { handleTelemetry(fields); return; }
  if (fields[0] === 'ACK' && fields[1] === 'TEL') {
    setTelemetry(fields[2] === '1');
    logCommand(`收到 < ${line}`);
    setEvent(line);
    updateUi();
    return;
  }
  if (fields[0] === 'ACK' && fields[1] === 'SPEED') {
    // ACK,SPEED,LIMIT is not an enable/disable response.
    if (fields[2] === '0' || fields[2] === '1') {
      firmware.speedEnabled = fields[2] === '1';
      setTelemetry(firmware.speedEnabled);
    }
    logCommand(`收到 < ${line}`);
    setEvent(line);
    updateUi();
    return;
  }
  if (fields[0] === 'ACK' && fields[1] === 'STOP') {
    ownsAngleMotion = false;
    firmware.speedEnabled = false;
    setTelemetry(false);
    logCommand(`收到 < ${line}`);
    setEvent(line);
    updateUi();
    return;
  }
  if (fields[0] === 'ACK' || fields[0] === 'ERR') {
    if (fields[0] === 'ERR' && fields[1] === 'STACK_GUARD') {
      stackHealthy = false;
      firmware.speedEnabled = false;
      setTelemetry(false);
      updateUi();
    }
    if (fields[0] === 'ACK' && ['ARM', 'DISARM'].includes(fields[1])) {
      firmware.speedEnabled = false;
      updateUi();
    }
    logCommand(`收到 < ${line}`);
    setEvent(line);
  }
}

function consumeText(text) {
  receiveBuffer += text;
  const lines = receiveBuffer.replaceAll('\r', '').split('\n');
  receiveBuffer = lines.pop();
  lines.map((line) => line.trim()).filter(Boolean).forEach(handleLine);
}

async function connectBridge() {
  if (connecting || isOpen()) return;
  if (!BRIDGE_URL) {
    setEvent('请使用启动脚本打开纯蓝牙上位机');
    return;
  }
  connecting = true;
  updateUi();
  const candidate = new WebSocket(BRIDGE_URL);
  socket = candidate;
  bridgeReady = false;
  candidate.addEventListener('message', (event) => {
    if (socket !== candidate) return;
    const text = String(event.data);
    if (text.startsWith('{')) {
      try { handleResult(JSON.parse(text)); } catch (_) { setEvent('桥接返回格式错误'); }
    } else consumeText(text);
  });
  candidate.addEventListener('close', () => {
    if (socket !== candidate) return;
    hardwareConnected = false;
    bridgeReady = false;
    resetLinkState();
    socket = undefined;
    setEvent('蓝牙桥接已断开');
    updateUi();
  });
  candidate.addEventListener('error', () => setEvent('无法连接本地蓝牙桥接'));
  try {
    await new Promise((resolve, reject) => {
      const timeout = setTimeout(() => reject(new Error('连接超时')), 5000);
      candidate.addEventListener('open', () => { clearTimeout(timeout); resolve(); }, { once: true });
      candidate.addEventListener('error', () => { clearTimeout(timeout); reject(new Error('桥接连接失败')); }, { once: true });
    });
    setEvent('桥接已连接，正在扫描蓝牙');
  } catch (error) {
    candidate.close();
    socket = undefined;
    setEvent(error.message);
  } finally {
    connecting = false;
    updateUi();
  }
}

function disconnectBridge() {
  // Closing the last web client makes the bridge send STOP immediately.
  socket?.close();
  socket = undefined;
  hardwareConnected = false;
  bridgeReady = false;
  resetLinkState();
  updateUi();
}

ui.connect.addEventListener('click', () => (isOpen() ? disconnectBridge() : void connectBridge()));
ui.arm.addEventListener('click', () => { ownsAngleMotion=false; void runAction((send) => send('ARM')); });
function stopMotion() {
  ownsAngleMotion=false;
  void runAction((send) => send('STOP'), true);
}
ui.stop.addEventListener('click', stopMotion);
document.querySelector('#padStop').addEventListener('click', stopMotion);
ui.limitButton.addEventListener('click', () => {
  const limit = Math.round(Math.max(1, Math.min(MAX_TARGET_RPM, Number(ui.limit.value) || MAX_TARGET_RPM)));
  ui.limit.value = limit;
  void runAction((send) => send(`SPDLIMIT,${limit}`));
});
ui.telemetry.addEventListener('change', () => void runAction((send) => send(`TEL,${ui.telemetry.value}`)));
ui.query.addEventListener('click', () => void runAction(async (send) => {
  await send('VER');
  if (!firmwareCompatible) throw new Error(`需要烧录 ${REQUIRED_FIRMWARE}`);
  await send('UART');
  await send('LINK');
  await send('STACK');
  if (!stackHealthy) throw new Error('栈保护异常，已禁用电机控制');
  await send('SPEEDLIMIT');
  await send('SPEEDSTATUS');
  await send('PIDGET');
  await send('YAWGET');
  await send('HEADGET');
  await send('ANGLESTATUS');
  await send('STACK');
  if (!stackHealthy) throw new Error('查询后栈保护异常，已禁用电机控制');
}));
document.querySelectorAll('.pid-row[data-motor]').forEach((row) => {
  row.querySelector('.pid-send').addEventListener('click', () => {
    const motor = row.dataset.motor;
    const value = (selector) => Number(row.querySelector(selector).value);
    const target = value('.target-input');
    if (!Number.isInteger(target) ||
        Math.abs(target) > Math.min(MAX_TARGET_RPM, Number(ui.limit.value))) {
      setEvent('目标必须为上限范围内的整数 RPM，最高 200 RPM；四轮速度 PID 已固定');
      return;
    }
    void runAction(async (send) => {
      await send(`SPD,${motor},${target}`);
    });
  });
});
async function restoreConfiguredTargets(send) {
  for (const row of document.querySelectorAll('.pid-row[data-motor]')) {
    const motor = row.dataset.motor;
    const target = Number(row.querySelector('.target-input').value) || 0;
    await send(`SPD,${motor},${target}`);
  }
}
ui.speed.addEventListener('click', () => {
  ownsAngleMotion=false;
  void runAction(async (send) => {
    if (firmware.speedEnabled) return send('SPEED,0');
    const targets = [...document.querySelectorAll('.target-input')].map((input) => Number(input.value));
    if (targets.some((target) => !Number.isInteger(target) ||
        Math.abs(target) > Math.min(MAX_TARGET_RPM, Number(ui.limit.value)))) {
      throw new Error('目标必须为上限范围内的整数 RPM，最高 200 RPM');
    }
    let attempted = false;
    try {
      attempted = true;
      await send('SPEED,1');
      await restoreConfiguredTargets(send);
    } catch (error) {
      // A lost enable ACK or partial target write must not leave wheels running.
      if (attempted) {
        try { await send('STOP'); } catch (_) {}
      }
      throw error;
    }
  });
});
function numeric(id, limit, positive=false) {
  const value=Number(document.querySelector(id).value);
  if (!Number.isFinite(value) || Math.abs(value)>limit || (positive && value<0))
    throw new Error(`输入超出范围：${id}`);
  return value;
}
function angleAction(action) {
  void runAction(async send => {
    try { await action(send); }
    catch (error) {
      ownsAngleMotion=false;
      try { await send('STOP'); } catch (_) {}
      throw error;
    }
  });
}
document.querySelector('#ratePidButton').addEventListener('click', () => angleAction(async send => {
  const p=numeric('#rateKp',100,true), i=numeric('#rateKi',100,true);
  await send(`YAWPID,${p.toFixed(4)},${i.toFixed(4)}`);
  const expected=[p.toFixed(4),i.toFixed(4)];
  const read=(await send('YAWGET')).split(',');
  if (expected.some((x,i) => x!==read[i+1])) throw new Error('PI 参数回读不一致');
}));
document.querySelector('#headPidButton').addEventListener('click', () => angleAction(async send => {
  const p=numeric('#headKp',100,true), d=numeric('#headKd',100,true);
  await send(`HEADPID,${p.toFixed(4)},${d.toFixed(4)}`);
  const expected=[p.toFixed(4),d.toFixed(4)];
  const read=(await send('HEADGET')).split(',');
  if (expected.some((x,i) => x!==read[i+1])) throw new Error('PD 参数回读不一致');
}));
document.querySelector('#rateStartButton').addEventListener('click', () => angleAction(async send => {
  await send(`YAW,${numeric('#rateTarget',80)}`);
  ownsAngleMotion=true;
}));
document.querySelector('#headStartButton').addEventListener('click', () => angleAction(async send => {
  await send(`HEAD,${numeric('#headTarget',180)}`);
  ownsAngleMotion=true;
}));
for (const [id,command] of [['#yawZeroButton','ZERO'],['#calButton','CAL']])
  document.querySelector(id).addEventListener('click', () => {
    ownsAngleMotion=false; angleAction(send => send(command));
  });
let drivePressSerial=0;
document.querySelectorAll('.drive-command').forEach(button => {
  button.addEventListener('pointerdown', event => {
    if (button.disabled) return;
    const press=++drivePressSerial;
    button.setPointerCapture(event.pointerId);
    angleAction(async send => {
      const speed=Math.round(numeric('#driveSpeed',100,true));
      await send(`MOVE,${speed*Number(button.dataset.forward)},${speed*Number(button.dataset.left)},0`);
      if (press!==drivePressSerial) return send('STOP');
      ownsAngleMotion=true;
    });
  });
  const release = () => { drivePressSerial++; stopMotion(); };
  button.addEventListener('pointerup',release);
  button.addEventListener('pointercancel',release);
  button.addEventListener('lostpointercapture', () => { if (ownsAngleMotion) release(); });
});
document.querySelector('#chartLoop').addEventListener('change', () => {
  chartSamples.length=0; drawChart();
});
setInterval(async () => {
  if (!ownsAngleMotion || !connected() || keepPending) return;
  keepPending=true;
  try { await writeCommand('KEEP'); }
  catch (error) { ownsAngleMotion=false; stopMotion(); setEvent(error.message); }
  finally { keepPending=false; }
},400);
window.addEventListener('pagehide', () => { socket?.close(); });
setInterval(() => {
  const reference = Math.max(firmware.receivedAt, firmware.expectedAt);
  if (connected() && firmware.telemetryEnabled && reference &&
      Date.now() - reference > 3000 && !firmware.timeoutReported) {
    firmware.timeoutReported = true;
    setEvent('TEL 超时：已开启遥测但 3 秒没有完整数据；点击“查询状态”检查 UART、LINK 和栈空间');
    updateUi();
  }
}, 300);
updateUi();
void connectBridge();
