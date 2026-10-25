const TARGET_STORAGE_KEY = 'bm50-console-target';
const GITHUB_PAGES_HOST = 'citiao1.github.io';
const REQUIRED_FIRMWARE = 'VGT6_BM50_ATTITUDE_R17';
const SUPPORTED_FIRMWARE = [REQUIRED_FIRMWARE, 'VGT6_BM50_ATTITUDE_R18', 'VGT6_BM50_ATTITUDE_R19', 'VGT6_BM50_ATTITUDE_R20', 'VGT6_BM50_ATTITUDE_R21', 'VGT6_BM50_ATTITUDE_R22', 'VGT6_BM50_ATTITUDE_R23', 'VGT6_BM50_ATTITUDE_R24', 'VGT6_BM50_ATTITUDE_R25', 'VGT6_BM50_ATTITUDE_R26'];
const MAX_TARGET_RPM = 160;
const HLOG_REQUEST_TIMEOUT_MS = 25000; // Bridge allows 20 s for a stopped RAM batch.

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
  transport: document.querySelector('#transportSelect'),
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
  hlogStatus: document.querySelector('#hlogStatus'),
  hlogClear: document.querySelector('#hlogClearButton'),
  hlogStart: document.querySelector('#hlogStartButton'),
  hlogStop: document.querySelector('#hlogStopButton'),
  hlogRead: document.querySelector('#hlogReadButton'),
  squareTest: document.querySelector('#squareTestButton'),
  squareStatus: document.querySelector('#squareTestStatus'),
  translateTest: document.querySelector('#translateTestButton'),
  turnTest: document.querySelector('#quarterTurnButton'),
  testDownload: document.querySelector('#chassisCsvButton'),
  testStatus: document.querySelector('#chassisTestStatus'),
  testStop: document.querySelector('#chassisStopButton'),
  ps2Status: document.querySelector('#ps2Status'),
  ps2Query: document.querySelector('#ps2QueryButton'),
  ps2Raw: document.querySelector('#ps2RawStatus'),
  tofDistance: document.querySelector('#tofDistance'),
  tofState: document.querySelector('#tofState'),
  tofRealtime: document.querySelector('#tofRealtime'),
};
const requestedTransport=new URLSearchParams(window.location.search).get('transport');
if(ui.transport && ['serial','bridge'].includes(requestedTransport))ui.transport.value=requestedTransport;

let socket;
let serialTransport;
let lastSerialDiagnostic;
let stopRequest;
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
let ownsSpeedMotion=false;
let speedOnlyMode=false;
let speedModeSwitching=false;
let speedLeaseSentAt=0;
let keepPending = false;
let chassisTestActive = false;
let chassisTestRows = [];
let chassisTestSummary = '';
let motionEpoch = 0;
const chartSamples = [];
const pending = new Map();
const logLines = [];
const uiTrace=[];
let uiTraceDropped=0;
const firmware = {
  speedEnabled: false, telemetryEnabled: false, targetLimit: MAX_TARGET_RPM,
  receivedAt: 0, expectedAt: 0, timeoutReported: false,
  hlogSupported: false,
  hlogMicros: false,
  tofSupported: false, tofReceivedAt: 0, tofStreaming: false, tofMaxAge: 350,
  guardStopMm: 50, guardSlowMm: 100, guardSlowRpm: MAX_TARGET_RPM,
  hlog: { active: false, count: 0, capacity: 0, overflow: 0, sequence: 0, lastBatch: 0 },
};
const hlogRows = [];
const wheelLogFrames = [];
const linkJournal=new Bm50LinkJournal(localStorage);
let lastHistoryRender=0;
function incident(event,detail,evidence) {
  const item=linkJournal.event(event,detail,evidence);
  if(!item || (item.repeats>1 && performance.now()-lastHistoryRender<250))return;
  renderLinkHistory();
}
function renderLinkHistory() {
  lastHistoryRender=performance.now();
  const history=linkJournal.snapshot();
  const rows=history.incidents.slice().reverse().map(item=>
    `${new Date(item.startedAt).toLocaleTimeString()} · ${item.mode} · ${item.events.map(x=>x.reason).join(' → ')}${item.repeats>1?`（同类损坏重复${item.repeats}次）`:''}\n`+
    item.events.map(x=>typeof x.detail==='string'?x.detail:JSON.stringify(x.detail)).join(' | '));
  document.querySelector('#linkHistory').textContent=rows.join('\n\n')||'尚无故障记录';
  document.querySelector('#linkHistoryStatus').textContent=`保留 ${history.incidents.length} 条事件；最近3条附原始数据。${history.storageError}`;
}

function setEvent(text) { ui.event.textContent = text; }
function recordUi(event, detail) {
  if(uiTrace.length>=512){uiTrace.shift();uiTraceDropped++;}
  uiTrace.push({hostMs:performance.now(),event,detail});
}
function linkDiagnostic() {
  const current=ui.transport?.value==='bridge'?{mode:'bridge',commands:logLines.slice()}:
    serialTransport?.diagnostic()||lastSerialDiagnostic||{mode:'direct_web_serial'};
  return {...current,
    clientVersion:'safe-link-v42',incidentHistory:linkJournal.snapshot(),uiTrace:uiTrace.slice(),uiTraceDropped,
    inputState:{keys:[...driveKeys],vector:{...driveVector},ownsAngleMotion,ownsSpeedMotion,speedOnlyMode,driveActive,keepPending}};
}
function logCommand(text) {
  logLines.push(text);
  if (logLines.length > 12) logLines.shift();
  ui.log.textContent = logLines.join('\n');
}
function isOpen() { return serialTransport?.isOpen() || socket?.readyState === WebSocket.OPEN; }
function connected() { return isOpen() && hardwareConnected && bridgeReady; }
function setBadge(state, text) {
  ui.badge.className = `badge ${state}`;
  ui.badge.replaceChildren();
  const dot = document.createElement('span');
  dot.className = 'dot';
  ui.badge.append(dot, document.createTextNode(text));
}

function updateUi() {
  ui.connect.textContent = connecting ? '验证中' : (isOpen() ? '断开' : '连接');
  ui.connect.disabled = connecting;
  if (ui.transport) ui.transport.disabled = connecting || isOpen();
  /*
   * Commands are serialized by runAction(), but controls stay visually
   * stable while a request is in flight.  Disabling the whole panel here
   * made every button flash because the stylesheet lowers disabled opacity.
   */
  const disabled = !connected() || !firmwareCompatible || !stackHealthy;
  [ui.arm, ui.speed, ui.limitButton, ui.telemetry]
    .forEach((element) => { element.disabled = disabled; });
  document.querySelector('#speedOnlyButton').disabled=disabled || busy || speedModeSwitching || !firmware.directSpeedKeep;
  document.querySelector('#speedOnlyButton').textContent=speedOnlyMode?'退出速度环专用模式（先停车）':'速度环专用模式（暂时停用外环）';
  document.querySelector('#speedOnlyStatus').textContent=speedOnlyMode?'仅速度环：W/S/A/D 平移；Q/E 已禁用。先手动 ARM、启用速度环，再按方向键。':'普通模式：方向键使用角速度环；速度专用模式需要 R25 固件。';
  ui.query.disabled = !connected();
  ui.ps2Query.disabled = !connected();
  ui.tofRealtime.disabled = !connected() || !firmware.tofSupported;
  for (const action of ['Clear', 'Start', 'Stop', 'Read'])
    document.querySelector(`#wlog${action}Button`).disabled = disabled || !firmware.wlogSupported;
  ui.stop.disabled = !connected();
  ui.testStop.disabled = !connected();
  document.querySelectorAll('.angle-command,.drive-command').forEach((button) => { button.disabled = disabled; });
  for(const id of ['#rateStartButton','#headStartButton'])
    document.querySelector(id).disabled=disabled||speedOnlyMode||speedModeSwitching;
  document.querySelectorAll('.drive-command').forEach(button=>{
    button.disabled=disabled||speedModeSwitching||(speedOnlyMode&&Number(button.dataset.rotate)!==0);
  });
  document.querySelectorAll('.pid-send').forEach((button) => { button.disabled = disabled; });
  document.querySelector('#speedPidReadButton').disabled = disabled;
  document.querySelector('#speedPidWriteAllButton').disabled = disabled;
  [ui.hlogClear, ui.hlogStart, ui.hlogRead].forEach((element) => {
    element.disabled = disabled || !firmware.hlogSupported;
  });
  ui.hlogStop.disabled = disabled || !firmware.hlogSupported;
  document.querySelector('#padStop').disabled = !connected();
  [ui.squareTest, ui.translateTest, ui.turnTest].forEach((button) => {
    button.disabled = disabled || chassisTestActive || speedOnlyMode || speedModeSwitching;
  });
  ui.testDownload.disabled = chassisTestActive || !chassisTestRows.length;
  ui.speed.textContent = firmware.speedEnabled ? '关闭速度环' : '启用速度环';
  if (connected()) setBadge('online', '通信在线');
  else if (isOpen()) setBadge('waiting', '等待设备');
  else setBadge('offline', '未连接');
}

function resetLinkState() {
  const interruptedTest = chassisTestActive;
  chassisTestActive = false;
  motionEpoch += 1;
  document.querySelector('#testSafety').checked = false;
  ownsAngleMotion = ownsSpeedMotion = false;
  speedOnlyMode=false;firmware.directSpeedKeep=false;
  driveKeys.clear();driveVector={forward:0,left:0,rotate:0};driveVectorSerial++;
  driveActive=false;driveMovePending=false;
  chartSamples.length = 0;
  operationSerial += 1;
  busy = false;
  receiveBuffer = '';
  firmware.speedEnabled = false;
  firmware.hlogSupported = false;
  firmware.hlogMicros = false;
  firmware.wlogSupported = firmware.frontReversed = firmware.tofGuard = false;
  wheelLogFrames.length = 0;
  document.querySelector('#wlogStatus').textContent = '未查询';
  document.querySelector('#frontGuardState').textContent = '保护状态未知';
  document.querySelector('#frontFrameState').textContent = '车头方向等待固件确认';
  firmware.tofSupported = firmware.tofStreaming = false;
  firmware.tofReceivedAt = 0;
  ui.tofRealtime.checked = false;
  ui.tofDistance.textContent = '— mm';
  ui.tofState.textContent = '连接已断开';
  firmware.hlog = { active: false, count: 0, capacity: 0, overflow: 0, sequence: 0, lastBatch: 0 };
  hlogRows.length = 0;
  firmwareCompatible = false;
  stackHealthy = false;
  setTelemetry(false);
  firmware.receivedAt = 0;
  ui.version.textContent = '未查询';
  ui.link.textContent = '未查询';
  ui.uart.textContent = '未查询';
  ui.stack.textContent = '未查询';
  ui.ps2Status.textContent = '未查询';
  ui.ps2Raw.textContent = '等待原始帧';
  ui.squareStatus.textContent = interruptedTest ? '连接中断，测试已取消；不会自动恢复' : '未运行';
  if (interruptedTest) ui.testStatus.textContent = ui.squareStatus.textContent;
  ui.status.textContent = '等待状态查询';
  ui.hlogStatus.textContent = '未查询';
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
  if(command!=='STOP')return writeCommandOnce(command);
  if(stopRequest)return stopRequest;
  stopRequest=writeCommandOnce(command);
  stopRequest.finally(()=>{stopRequest=undefined;}).catch(()=>{});
  return stopRequest;
}

function writeCommandOnce(command) {
  if (!connected()) return Promise.reject(new Error('设备未连接'));
  const diagnostic = ['VER', 'UART', 'LINK', 'STACK', 'LINKQ', 'SPEEDSTATUS', 'SPEEDLIMIT', 'PIDGET', 'PING', 'STOP', 'CAPS', 'YAWGET', 'HEADGET', 'ANGLESTATUS', 'PS2STATUS', 'PS2RAW', 'TOF', 'TOF,0', 'TOF,1'].includes(command) ||
    command.startsWith('HLOG,') || command.startsWith('WLOG,') || command === 'WLOGINFO';
  if (!diagnostic && (!firmwareCompatible || !stackHealthy)) {
    return Promise.reject(new Error('固件版本或栈保护检查未通过；请查询状态'));
  }
  if((speedOnlyMode||speedModeSwitching) && /^(MOVE|YAW|HEAD|HEADMOVE|TRACKMOVE),/.test(command))
    return Promise.reject(new Error('速度环专用模式已停用角速度和方向控制'));
  const id = ++requestSerial;
  logCommand(`发送 > ${command}`);
  if (serialTransport?.isOpen()) return serialTransport.command(command);
  return new Promise((resolve, reject) => {
    const timer = setTimeout(() => {
      pending.delete(id);
      incident('bridge_timeout',{command},linkDiagnosticEvidence());
      reject(new Error(`车端应答超时：${command}；请导出链路诊断，检查回传方向`));
    }, /^(HLOG|WLOG),READ,/.test(command) ? HLOG_REQUEST_TIMEOUT_MS : 5000);
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
  const view=document.querySelector('#chartLoop').value;
  if(view.startsWith('wheel')) {
    const targets=fields.slice(4,8).map(Number),actuals=fields.slice(12,16).map(x=>Number(x)/10);
    const wheel=view==='wheel_all'?-1:view==='wheel'?0:Number(view.slice(5))-1;
    if(wheel===-1)plotSample(Number(fields[1]),targets,actuals);
    else if(wheel>=0&&wheel<4)plotSample(Number(fields[1]),targets[wheel],actuals[wheel]);
  }
  if(ownsSpeedMotion && !firmware.speedEnabled){ownsSpeedMotion=false;stopMotion('speed_disabled');setEvent('车端速度环已关闭，旧按键已取消');}
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
  if (fields[2] === '0' || fields[3] !== '1' || fields[12] !== '1') {
    if(ownsAngleMotion) {
      const evidence={line:fields.join(','),keys:[...driveKeys],keepPending};
      recordUi('motion_stopped',evidence);incident('motion_stopped',evidence,linkDiagnosticEvidence());
      setEvent('车端已停止，已停止保活；如需继续请松开方向键后重新按下。请导出链路诊断检查停车原因');
    }
    ownsAngleMotion = false;
  }
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
  const values=points.flatMap(x=>[x.actual,x.target].flat());
  let lo=Math.min(...values),hi=Math.max(...values);
  const margin = Math.max(1,(hi-lo)*0.15); lo-=margin; hi+=margin;
  const t0=points[0].tick, span=Math.max(1000,points.at(-1).tick-t0);
  ctx.font='12px Consolas'; ctx.lineWidth=1;
  for (let i=0;i<=4;i++) {
    const y=20+(h-50)*i/4;
    ctx.strokeStyle='#d7ddda'; ctx.beginPath(); ctx.moveTo(48,y); ctx.lineTo(w-16,y); ctx.stroke();
    ctx.fillStyle='#68736f'; ctx.fillText((hi-(hi-lo)*i/4).toFixed(1),4,y+4);
  }
  const all=Array.isArray(points[0].target),colors=['#147d58','#167a94','#b36312','#933ca3'];
  for(let wheel=0;wheel<(all?4:1);wheel++) {
    for(const key of ['target','actual']) {
      ctx.strokeStyle=all?colors[wheel]:key==='target'?'#147d58':'#167a94';
      ctx.lineWidth=key==='target'?1:2;ctx.setLineDash?.(key==='target'?[6,4]:[]);ctx.beginPath();
      points.forEach((p,i)=>{
        const value=all?p[key][wheel]:p[key];
        const x=48+(w-64)*(p.tick-t0)/span,y=20+(h-50)*(hi-value)/(hi-lo);
        if(i && p.tick-points[i-1].tick<2000)ctx.lineTo(x,y);else ctx.moveTo(x,y);
      });ctx.stroke();
    }
  }
  ctx.setLineDash?.([]);
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
    if(wasConnected && !hardwareConnected)
      incident('bridge_status_lost',{message:fields.slice(2).join(',')},linkDiagnosticEvidence());
    if (!hardwareConnected) resetLinkState();
    setEvent(bridgeReady ? (fields.slice(2).join(',') || '设备状态更新') :
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
        await send('PS2STATUS');
        await send('YAWGET');
        await send('HEADGET');
        await send('ANGLESTATUS');
        if(firmware.hlogSupported) await send('HLOG,STATUS');
        if (firmware.wlogSupported) await send('WLOG,STATUS');
        if (firmware.tofSupported) await send('TOF');
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
    firmwareCompatible = SUPPORTED_FIRMWARE.includes(version);
    logCommand(`收到 < ${line}`);
    setEvent(firmwareCompatible ? `固件：${version}` : `固件不兼容：请烧录 ${REQUIRED_FIRMWARE}`);
    updateUi();
    return;
  }
  if (fields[0] === 'CAPS') {
    firmware.directSpeedKeep=fields.includes('DKEEP');
    if(fields.includes('LOG_DISABLED'))document.querySelector('#wlogStatus').textContent='固件已编译关闭高频日志；实时曲线可用';
    firmware.hlogSupported = fields.includes('HLOG') && fields.includes('HLOG_MODE');
    firmware.hlogMicros = fields.includes('HLOG_US');
    firmware.tofSupported = fields.includes('TOF_DL1B');
    firmware.tofMaxAge = fields.includes('TOF_FAST') ? 120 : 350;
    ui.tofRealtime.title = fields.includes('TOF_FAST') ?
      '传感器约 25 Hz；显示 10 Hz，与周期遥测同时开启时显示 5 Hz' : '开启实时距离';
    firmware.wlogSupported = fields.includes('WLOG_US');
    firmware.frontReversed = fields.includes('FRONT_REVERSED');
    firmware.tofGuard = fields.includes('TOF_GUARD');
    const newGuard = fields.includes('TOF_GUARD_200_100_40');
    firmware.guardStopMm = newGuard ? 100 : 50;
    firmware.guardSlowMm = newGuard ? 200 : 100;
    firmware.guardSlowRpm = newGuard ? 40 : MAX_TARGET_RPM;
    document.querySelector('#frontFrameState').textContent = firmware.frontReversed ?
      'TOF 端为车头 · 新轮位：右前 C / 左前 D / 左后 A / 右后 B · A/B/C/D 仍为物理通道' : '旧版车头方向';
    document.querySelector('#frontGuardState').textContent = firmware.tofGuard ?
      (newGuard ? '前进保护：≤200 mm 上限40 RPM，向100 mm线性减速至停车；倒退/横移可用' :
      '前进保护：100→50 mm 限速，≤50 mm 停止前进；倒退/横移可用') : '当前固件未启用距离保护';
    if (!firmware.tofSupported) {
      ui.tofState.textContent = '当前固件不支持 TOF';
      ui.tofDistance.textContent = '— mm';
      ui.tofRealtime.checked = firmware.tofStreaming = false;
    }
    logCommand(`收到 < ${line}`);
    if (!firmware.hlogSupported) ui.hlogStatus.textContent = fields.includes('LOG_DISABLED')?'固件已编译关闭高频日志，释放88 KiB RAM':'固件不支持 HLOG';
    updateUi();
    return;
  }
  if (fields[0] === 'TOF') {
    const values = fields.slice(1).map(Number);
    if (fields.length !== 8 || !fields.slice(1).every(x => /^-?\d+$/.test(x)) ||
        !values.every(Number.isSafeInteger) ||
        ![0, 1].includes(values[1]) || ![0, 1].includes(values[2]) ||
        values.some((x, i) => i !== 3 && (x < 0 || x > 0xffffffff)) ||
        values[4] > 255 ||
        (values[2] === 1 && (values[1] !== 1 || values[3] < 1 || values[3] > 4000 || values[4] !== 9 || values[5] > firmware.tofMaxAge)) ||
        (values[2] === 0 && values[3] !== -1)) {
      ui.tofDistance.textContent = '— mm';
      ui.tofState.textContent = '距离报文无效';
      return;
    }
    const [, present, valid, mm, status, age, errors] = values;
    const zone=!valid||!present?'invalid':mm<=firmware.guardStopMm?'blocked':mm<=firmware.guardSlowMm?'slow':'clear';
    if(zone!==firmware.guardZone) {
      firmware.guardZone=zone;
      recordUi('tof_zone',{tick:values[0],zone,mm,age,errors});
    }
    firmware.tofReceivedAt = Date.now();
    ui.tofDistance.textContent = valid ? `${mm} mm` : '— mm';
    ui.tofState.textContent = !present ? '传感器未连接 / 初始化失败' : valid ?
      `${(mm / 10).toFixed(1)} cm · 更新于 ${age} ms 前` :
      age > firmware.tofMaxAge ? '等待新测量 / 数据已过期' : `测量无效 · 状态 ${status}`;
    if (errors) ui.tofState.textContent += ` · 通信/超时 ${errors}`;
    if (firmware.tofGuard) document.querySelector('#frontGuardState').textContent = !valid || !present ?
      '测距异常：按设置保持原速度，距离保护暂不可用' : mm <= firmware.guardStopMm ?
      `距离≤${firmware.guardStopMm} mm：禁止前进，允许倒退/横移` :
      (firmware.guardStopMm === 100 ? mm <= firmware.guardSlowMm : mm < firmware.guardSlowMm) ?
      `前进上限约 ${Math.min(firmware.targetLimit, Math.floor(firmware.guardSlowRpm * (mm - firmware.guardStopMm) /
      (firmware.guardSlowMm - firmware.guardStopMm)))} RPM（${firmware.guardSlowMm}→${firmware.guardStopMm} mm 限速）` :
      `距离${firmware.guardStopMm === 100 ? '>' : '≥'}${firmware.guardSlowMm} mm：不额外限制前进`;
    return;
  }
  if (fields[0] === 'WLOG') {
    if (fields[1] === 'DATA') wheelLogFrames.push(line);
    if (fields[1] === 'STATUS' && fields.length === 7)
      document.querySelector('#wlogStatus').textContent = `${fields[2] === '1' ? '记录中' : '已停止'} · ${fields[3]}/${fields[4]} 条 · 溢出 ${fields[5]}`;
    return;
  }
  if (fields[0] === 'PS2' && fields.length === 12) {
    const values = fields.slice(1).map(Number);
    if (values.every(Number.isFinite)) {
      const connectedState = values[0] === 1;
      const enabledState = values[1] === 1;
      ui.ps2Status.textContent =
        `${connectedState ? '已连接' : '未连接'} · ` +
        `${enabledState ? '已解锁' : '锁定'} · ${values[8]} RPM · 模式 ${fields[3]} · ` +
        `左杆 ${values[4]}/${values[5]} · 右杆 ${values[6]}/${values[7]} · 按键 ${fields[4]}`;
      logCommand(`收到 < ${line}`);
    } else {
      setEvent(`PS2 状态格式错误：${line}`);
    }
    updateUi();
    return;
  }
  if (fields[0] === 'PS2RAW' && fields.length === 12) {
    const bytes = fields.slice(1, 10);
    if (!bytes.every(x => /^[0-9A-F]{2}$/i.test(x))) return;
    let hint = fields[10] === '1' ? '模拟帧有效' : '等待有效模拟帧';
    if (bytes.every(x => x === 'FF')) hint = 'DAT 全高：检查供电、DAT 接线、片选和手柄配对';
    else if (bytes.every(x => x === '00')) hint = 'DAT 全低：检查接线或短路';
    else if (bytes[1] === '41') hint = '数字模式，固件正在自动切换模拟模式';
    ui.ps2Raw.textContent = `${bytes.join(' ')} · ${hint} · 配置次数 ${fields[11]}`;
    logCommand(`收到 < ${line}`);
    return;
  }
  if (fields[0] === 'HLOG' && fields[1] === 'STATUS' && fields.length === 7) {
    const values = fields.slice(2).map(Number);
    if (values.every(Number.isFinite) && values[0] >= 0 && values[0] <= 1 &&
        values[1] >= 0 && values[2] >= 1 && values[3] >= 0 && values[4] >= 0) {
      firmware.hlog.active = values[0] === 1;
      firmware.hlog.count = values[1];
      firmware.hlog.capacity = values[2];
      firmware.hlog.overflow = values[3];
      firmware.hlog.sequence = values[4];
      ui.hlogStatus.textContent =
        `${firmware.hlog.active ? '记录中' : '已停止'} · ${values[1]}/${values[2]} 条 · 溢出 ${values[3]}`;
      logCommand(`收到 < ${line}`);
    } else {
      setEvent(`HLOG 状态格式错误：${line}`);
    }
    updateUi();
    return;
  }
  if (fields[0] === 'HLOG' && fields[1] === 'DATA' && [14, 15, 16].includes(fields.length)) {
    const values = fields.slice(2).map(Number);
    if (fields.slice(2).every(x => /^-?\d+$/.test(x)) && values.every(Number.isSafeInteger) &&
        values[0] >= 0 && values[1] >= 0 && values[1] <= 0xffffffff &&
        [0, 1].includes(values[9]) && (fields.length === 14 || [1, 2].includes(values[12])) &&
        (fields.length !== 16 || (values[13] >= 0 && values[13] <= 0xffffffff)))
      hlogRows.push(fields.length === 14 ? [...values, 0] : values);
    else setEvent(`HLOG 数据格式错误：${line}`);
    return;
  }
  if (fields[0] === 'HLOG' && fields[1] === 'END' && fields.length === 5) {
    const values = fields.slice(2).map(Number);
    if (values.every(Number.isFinite)) {
      firmware.hlog.lastBatch = values[1];
      firmware.hlog.count = values[2];
      logCommand(`收到 < ${line}`);
    } else setEvent(`HLOG 结束格式错误：${line}`);
    return;
  }
  if (fields[0] === 'HLOG' && ['CLEAR', 'START', 'STOP'].includes(fields[1])) {
    logCommand(`收到 < ${line}`);
    if (fields[1] === 'CLEAR' && fields[2] === 'OK') hlogRows.length = 0;
    if (fields[1] === 'START') firmware.hlog.active = true;
    if (fields[1] === 'STOP') firmware.hlog.active = false;
    setEvent(line);
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
      firmware.targetLimit = limit;
      ui.limit.value = limit;
      logCommand(`收到 < ${line}`);
    } else {
      setEvent(`SPEEDLIMIT 格式错误：${line}`);
    }
    return;
  }
  if (fields[0] === 'PID') {
    if (!applySpeedPidReadback(line)) setEvent(`PIDGET 格式错误：${line}`);
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
    // ACK,SPEED,LIMIT is not an enable/disable response.  SPEED,0 stops
    // the loop but deliberately preserves the selected telemetry stream.
    if (fields[2] === '0' || fields[2] === '1') {
      firmware.speedEnabled = fields[2] === '1';
      if (fields[2] === '1') setTelemetry(true);
    }
    logCommand(`收到 < ${line}`);
    setEvent(line);
    updateUi();
    return;
  }
  if (fields[0] === 'ACK' && fields[1] === 'STOP') {
    ownsAngleMotion = ownsSpeedMotion = false;
    firmware.speedEnabled = false;
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
      ownsAngleMotion = ownsSpeedMotion = false;
      firmware.speedEnabled = false;
      updateUi();
    }
    logCommand(`收到 < ${line}`);
    setEvent(line);
  }
}

function hlogCsv(rows) {
  const header = [
    'sequence', 'tick', 'target_heading_mdeg', 'actual_heading_mdeg',
    'heading_error_mdeg', 'target_rate_mdps', 'actual_rate_mdps',
    'heading_output_mdps', 'rate_output_mrpm', 'saturated',
    'forward_rpm', 'left_rpm', 'mode',
  ];
  if (rows.some(row => row.length === 14)) header.push('tick_us');
  const escape = (value) => {
    const text = String(value);
    return /[,"\r\n]/.test(text) ? `"${text.replaceAll('"', '""')}"` : text;
  };
  return [header, ...rows].map((row) => row.map(escape).join(',')).join('\r\n') + '\r\n';
}

async function readStoppedHlog(send) {
  const angle = (await send('ANGLESTATUS')).split(',');
  if (angle.length !== 14 || angle[0] !== 'ATEL' || angle[2] !== '0' || angle[12] !== '0')
    throw new Error('请先停止运动，确认角环停止且速度环关闭后再读取 HLOG');
  const status = (await send('HLOG,STATUS')).split(',');
  const numbers = status.slice(2).map(Number);
  if (status.length !== 7 || status[0] !== 'HLOG' || status[1] !== 'STATUS' ||
      !status.slice(2).every(x => /^\d+$/.test(x)) || !numbers.every(Number.isSafeInteger))
    throw new Error('HLOG 状态格式错误');
  const [active, total, capacity, overflow, sequence] = numbers;
  if (active !== 0) throw new Error('请先停止记录再下载 HLOG');
  if (!total || total > capacity || capacity > 1024 || overflow !== 0 || sequence !== total)
    throw new Error('日志为空、溢出或已覆盖，不能作为整定数据；请重新采集');
  hlogRows.length = 0;
  for (let offset = 0; offset < total; offset += 8) {
    const count = Math.min(8, total - offset);
    const reply = await send(`HLOG,READ,${offset},${count}`);
    if (reply !== `HLOG,END,${offset},${count},${total}`)
      throw new Error(`HLOG 分批校验失败：${reply}`);
    if (hlogRows.length !== offset + count)
      throw new Error(`HLOG 读取不完整：${hlogRows.length}/${offset + count}`);
    ui.hlogStatus.textContent = `下载中 · ${hlogRows.length}/${total} 条`;
  }
  if (hlogRows.some((row, index) => row[0] !== index || ![1, 2].includes(row[12])))
    throw new Error('日志序号或控制模式不完整，不能作为整定数据');
  const micros = hlogRows[0].length === 14;
  if ((firmware.hlogMicros && !micros) || hlogRows.some(row =>
      (row.length === 14) !== micros || (micros && ((row[13] - row[1] * 1000) >>> 0) >= 1000)))
    throw new Error('日志时间戳分辨率混用、缺少微秒字段或时间戳不一致');
  for (let index = 1; index < hlogRows.length; index += 1) {
    const column = micros ? 13 : 1, scale = micros ? 1000 : 1;
    const dt = (hlogRows[index][column] - hlogRows[index - 1][column]) >>> 0;
    if (dt < (micros ? 19900 : 20) || dt > 25 * scale)
      throw new Error('日志时间不连续或采样不足，保留诊断数据但不能作为完整整定日志');
  }
  return hlogRows;
}

function downloadHlogCsv(rows) {
  const csv = hlogCsv(rows);
  const blob = new Blob([`\uFEFF${csv}`], { type: 'text/csv;charset=utf-8' });
  const url = URL.createObjectURL(blob);
  const anchor = document.createElement('a');
  anchor.href = url;
  anchor.download = `attitude-hlog-${new Date().toISOString().replaceAll(':', '-')}.csv`;
  anchor.click();
  setTimeout(() => URL.revokeObjectURL(url), 0);
  return csv;
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
    setEvent('请使用启动脚本打开本地通信上位机');
    return;
  }
  linkJournal.start('bridge');
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
  candidate.addEventListener('close', (event) => {
    if (socket !== candidate) return;
    incident('bridge_closed',{code:event.code,reason:event.reason},linkDiagnosticEvidence());
    hardwareConnected = false;
    bridgeReady = false;
    resetLinkState();
    socket = undefined;
    setEvent(`本地 WebSocket 已断开，代码 ${event.code}：${event.reason || '无关闭说明'}；测试不会自动恢复`);
    updateUi();
  });
  candidate.addEventListener('error', () => {incident('bridge_error',{},linkDiagnosticEvidence());setEvent('无法连接本地通信桥接');});
  try {
    await new Promise((resolve, reject) => {
      const timeout = setTimeout(() => reject(new Error('连接超时')), 5000);
      candidate.addEventListener('open', () => { clearTimeout(timeout); resolve(); }, { once: true });
      candidate.addEventListener('error', () => { clearTimeout(timeout); reject(new Error('桥接连接失败')); }, { once: true });
    });
    setEvent('桥接已连接，正在连接设备');
  } catch (error) {
    candidate.close();
    socket = undefined;
    setEvent(error.message);
  } finally {
    connecting = false;
    updateUi();
  }
}

async function connectSerial() {
  if (connecting || isOpen()) return;
  if (typeof navigator === 'undefined' || !navigator.serial) {
    setEvent('请使用电脑 Chrome / Edge 打开本地页面，点击连接后选择 LQ208 的 CH340 串口');
    return;
  }
  const previousTransport=serialTransport;
  linkJournal.start('direct_web_serial');
  connecting=true; updateUi();
  let candidate;
  try {
    // Windows may still be closing the old SerialPort after a fault. Wait for
    // release before asking the same authorized object to open again.
    if(previousTransport) {
      let closeTimer;
      try {
        await Promise.race([previousTransport.close(false),new Promise((_,reject)=>{
          closeTimer=setTimeout(()=>reject(Object.assign(new Error('旧串口5秒内未能释放；请先关闭其他串口工具，必要时拔插电脑端模块'),{phase:'previous_close'})),5000);
        })]);
      } finally {clearTimeout(closeTimer);}
      lastSerialDiagnostic=previousTransport.diagnostic();
      if(serialTransport===previousTransport)serialTransport=undefined;
    }
    // Reuse only a uniquely authorized, attached CH340/CH910 port after a click.
    const authorized=await navigator.serial.getPorts?.()||[];
    const available=authorized.filter(port=>port.getInfo().usbVendorId===0x1a86 && port.connected!==false);
    const port=available.length===1 ? available[0] :
      await navigator.serial.requestPort({filters:[{usbVendorId:0x1a86}]});
    candidate=new Bm50SerialTransport({
      onEvent:(event,detail)=>incident(event,detail,linkDiagnosticEvidence(candidate)),
      onLine:line=>{if(serialTransport===candidate)handleLine(line);},
      onWarning:message=>{logCommand(message);setEvent(message);},
      onRecovering:trigger=>{
        if(serialTransport!==candidate)return;
        recordUi('safe_recovery_start',{trigger});
        hardwareConnected=false;bridgeReady=false;resetLinkState();connecting=true;updateUi();
        setEvent(`${trigger==='DKEEP'?'速度环保活未确认':'STOP 确认丢失'}，正在确认DISARM和停车状态；旧运动已取消`);
      },
      onRecovered:()=>{
        if(serialTransport!==candidate)return;
        recordUi('safe_recovery_verified',{});connecting=false;bridgeReady=true;
        handleLine('STATUS,1,停车和DISARM已重新验证；不会恢复旧运动，请重新操作');
      },
      onDisconnect:error=>{
        if(serialTransport!==candidate)return;
        lastSerialDiagnostic=candidate.diagnostic();
        hardwareConnected=false;bridgeReady=false;resetLinkState();connecting=false;updateUi();
        setEvent(`串口直连中断：${error.message}；不会自动恢复运动，请导出链路诊断`);
      },
    });
    serialTransport=candidate;
    setEvent('串口已选择，正在确认 STOP / DISARM / PING');
    await candidate.open(port);
    bridgeReady=true;
    handleLine('STATUS,1,串口直连 115200 / 8N1，车端应答已验证；未运行桥接');
  } catch(error) {
    if(!candidate)incident('open_failed',{name:error.name,message:error.message,phase:error.phase},linkDiagnosticEvidence());
    if(candidate) {lastSerialDiagnostic=candidate.diagnostic();await candidate.close(false);}
    hardwareConnected=false;bridgeReady=false;resetLinkState();
    setEvent(`串口直连失败：${error.message}；请退出“启动桥接兼容模式.bat”的窗口及其他串口页面，再用“启动上位机.bat”打开直连`);
  } finally {connecting=false;updateUi();}
}

async function disconnectBridge() {
  incident('manual_disconnect',{},linkDiagnosticEvidence());
  if(serialTransport?.isOpen()) {
    connecting=true;updateUi();
    await serialTransport.close();lastSerialDiagnostic=serialTransport.diagnostic();
  }
  // Closing the last web client makes the bridge send STOP immediately.
  socket?.close();
  socket = undefined;
  hardwareConnected = false;
  bridgeReady = false;
  resetLinkState();
  connecting=false;
  updateUi();
}

ui.connect.addEventListener('click', () => {
  if(isOpen())return void disconnectBridge();
  return ui.transport?.value==='bridge' ? void connectBridge() : void connectSerial();
});
document.querySelector('#linkLogButton').addEventListener('click',()=>{
  const diagnostic=linkDiagnostic();
  const blob=new Blob([JSON.stringify(diagnostic,null,2)],{type:'application/json'});
  const url=URL.createObjectURL(blob),anchor=document.createElement('a');
  anchor.href=url;anchor.download=`bm50-link-${Date.now()}.json`;anchor.click();
  setTimeout(()=>URL.revokeObjectURL(url),0);
});
ui.ps2Query.addEventListener('click', () => runAction(async send => {
  await send('PS2STATUS');
  await send('PS2RAW');
  await send('ANGLESTATUS');
}));
ui.arm.addEventListener('click', () => { ownsAngleMotion=false; void runAction((send) => send('ARM')); });
function stopMotion(reason='user_stop') {
  recordUi('stop_requested',{reason:typeof reason==='string'?reason:'user_stop',keys:[...driveKeys],vector:{...driveVector}});
  motionEpoch += 1;
  if (chassisTestActive) {
    chassisTestActive = false;
    operationSerial += 1;
    ui.squareStatus.textContent = '已停止';
    ui.testStatus.textContent = '已停止；可下载已采集数据';
  }
  driveVector = { forward: 0, left: 0, rotate: 0 };
  driveVectorSerial += 1;
  driveKeys.clear();
  driveActive = false;
  driveMovePending = false;
  ownsAngleMotion=ownsSpeedMotion=false;
  void runAction((send) => send('STOP'), true);
}
ui.stop.addEventListener('click', stopMotion);
ui.testStop.addEventListener('click', stopMotion);
document.querySelector('#padStop').addEventListener('click', stopMotion);

function waitMilliseconds(milliseconds) {
  return new Promise((resolve) => setTimeout(resolve, milliseconds));
}

function runChassisTest(kind) {
  return runAction(async (send) => {
    if(speedOnlyMode||speedModeSwitching)throw new Error('请先退出速度环专用模式');
    if (chassisTestActive) return;
    if (!firmware.speedEnabled) throw new Error('请先 ARM 并启用速度环');
    if (driveActive || driveFlushRunning || driveKeys.size)
      throw new Error('请先松开方向键，等待手动运动命令结束，再启用速度环开始测试');
    if (!document.querySelector('#testSafety').checked)
      throw new Error('请先确认测试区域、下地状态和急停可用');
    const config = { kind, confirmed: true, frontReversed: firmware.frontReversed,
      rpm: numeric('#testRpm', 60, true),
      distance: numeric('#translateDistance', 100, true) / 100,
      angle: numeric('#translateAngle', 180),
      headingOffset: numeric('#translateHeading', 45) };
    const epoch = operationSerial;
    chassisTestActive = true;
    motionEpoch += 1;
    ownsAngleMotion = false; // The test owns serialized KEEP + feedback.
    driveActive = false;
    driveVector = { forward: 0, left: 0, rotate: 0 };
    driveKeys.clear();
    chassisTestRows = [];
    chassisTestSummary = '';
    updateUi();
    try {
      const result = await ChassisTest.run({
        send, now: () => Date.now(), sleep: waitMilliseconds,
        check: () => {
          if (!chassisTestActive || epoch !== operationSerial || !connected())
            throw new Error('测试已取消或连接中断');
        },
        progress: (text) => {
          ui.testStatus.textContent = text;
          if (kind === 'square') ui.squareStatus.textContent = text;
        },
        record: (row) => chassisTestRows.push({ test: kind, ...row }),
      }, config);
      const maxError = kind === 'turn' ? result.maxHeadingError : result.movingMaxError;
      const rmsError = kind === 'turn' ? result.rmsHeadingError : result.movingRmsError;
      chassisTestSummary = `流程完成（非实地验收）· ${kind === 'turn' ? '转向' : '移动中'}航向最大/RMS ${maxError.toFixed(1)}/${rmsError.toFixed(1)}° · 估计最大横偏 ${(result.maxCrossM * 100).toFixed(1)} cm · 估计终点偏移 ${(result.residualM * 100).toFixed(1)} cm`;
      ui.testStatus.textContent = chassisTestSummary;
      if (kind === 'square') ui.squareStatus.textContent = chassisTestSummary;
      setEvent(chassisTestSummary);
    } catch (error) {
      if (epoch === operationSerial) {
        ui.testStatus.textContent = `测试中止：${error.message}`;
        if (kind === 'square') ui.squareStatus.textContent = ui.testStatus.textContent;
      }
      throw error;
    } finally {
      if (epoch === operationSerial) {
        ownsAngleMotion = false;
        chassisTestActive = false;
        updateUi();
      }
    }
  });
}
ui.squareTest.addEventListener('click', () => runChassisTest('square'));
ui.translateTest.addEventListener('click', () => runChassisTest('translation'));
ui.turnTest.addEventListener('click', () => runChassisTest('turn'));
ui.testDownload.addEventListener('click', () => {
  if (!chassisTestRows.length || chassisTestActive) return;
  const keys = Object.keys(chassisTestRows[0]);
  const rows = chassisTestRows.map((row) => keys.map((key) => row[key]));
  const csv = [keys, ...rows].map((row) => row.join(',')).join('\r\n') + '\r\n';
  const url = URL.createObjectURL(new Blob(['\uFEFF' + csv], {type:'text/csv;charset=utf-8'}));
  const anchor = document.createElement('a');
  anchor.href = url; anchor.download = 'chassis-test.csv'; anchor.click();
  setTimeout(() => URL.revokeObjectURL(url), 0);
});

ui.limitButton.addEventListener('click', () => {
  const limit = Math.round(Math.max(1, Math.min(MAX_TARGET_RPM, Number(ui.limit.value) || MAX_TARGET_RPM)));
  ui.limit.value = limit;
  void runAction((send) => send(`SPDLIMIT,${limit}`));
});
ui.telemetry.addEventListener('change', () => void runAction((send) => send(`TEL,${ui.telemetry.value}`)));
ui.tofRealtime.addEventListener('change', () => {
  const enable = ui.tofRealtime.checked;
  void runAction(async send => {
    try {
      if (!firmware.tofSupported) throw new Error('当前固件不支持 TOF');
      await send(`TOF,${enable ? 1 : 0}`);
      firmware.tofStreaming = enable;
      ui.tofRealtime.checked = enable;
      if (enable) await send('TOF');
    } catch (error) {
      ui.tofRealtime.checked = firmware.tofStreaming;
      throw error;
    }
  });
});
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
  await send('CAPS');
  await send('PS2STATUS');
  await send('YAWGET');
  await send('HEADGET');
  await send('ANGLESTATUS');
  if(firmware.hlogSupported) await send('HLOG,STATUS');
  if (firmware.wlogSupported) await send('WLOG,STATUS');
  if (firmware.tofSupported) await send('TOF');
  await send('STACK');
  if (!stackHealthy) throw new Error('查询后栈保护异常，已禁用电机控制');
}));
const speedPidRows = [...document.querySelectorAll('.pid-row[data-motor]')];
const speedPidReadback = document.querySelector('#speedPidReadback');
function speedPidValues(row) {
  const values = ['.kp-input', '.ki-input', '.kd-input']
    .map((selector) => {
      const value = Number(row.querySelector(selector).value);
      if (!Number.isFinite(value) || value < 0 || value > 100)
        throw new Error(`输入超出范围：${row.dataset.motor} 轮 ${selector}`);
      return value;
    });
  return values;
}
function applySpeedPidReadback(reply) {
  const fields = Array.isArray(reply) ? reply : String(reply).split(',');
  if (fields.length !== 13 || fields[0] !== 'PID') return false;
  const values = fields.slice(1).map(Number);
  if (values.some((value) => !Number.isFinite(value) || value < 0 || value > 100)) return false;
  speedPidRows.forEach((row, index) => {
    const rowValues = values.slice(index * 3, index * 3 + 3);
    ['.kp-input', '.ki-input', '.kd-input'].forEach((selector, valueIndex) => {
      const input = row.querySelector(selector);
      if (document.activeElement !== input) input.value = rowValues[valueIndex].toFixed(4);
    });
    row.querySelector('.pid-state').textContent = '已回读';
  });
  speedPidReadback.textContent = '已回读四轮参数';
  logCommand(`收到 < ${fields.join(',')}`);
  return true;
}
async function writeSpeedPidRow(row, send) {
  const values = speedPidValues(row);
  const encoded = values.map((value) => value.toFixed(4));
  await send(`PID,${row.dataset.motor},${encoded.join(',')}`);
  const reply = await send('PIDGET');
  if (!applySpeedPidReadback(reply)) throw new Error('速度环 PID 回读格式错误');
  row.querySelector('.pid-state').textContent = '已写入并校验';
}
document.querySelector('#speedPidReadButton').addEventListener('click', () => runAction(async (send) => {
  const reply = await send('PIDGET');
  if (!applySpeedPidReadback(reply)) throw new Error('速度环 PID 回读格式错误');
}));
document.querySelector('#speedPidWriteAllButton').addEventListener('click', () => runAction(async (send) => {
  for (const row of speedPidRows) await writeSpeedPidRow(row, send);
  setEvent('四轮速度环 PID 已写入并回读校验');
}));
speedPidRows.forEach((row) => {
  row.querySelector('.pid-send').addEventListener('click', () => runAction(async (send) => {
    await writeSpeedPidRow(row, send);
    setEvent(`${row.dataset.motor} 轮速度环 PID 已写入并回读校验`);
  }));
});
ui.speed.addEventListener('click', () => {
  ownsAngleMotion=ownsSpeedMotion=false;
  void runAction(async (send) => {
    if (firmware.speedEnabled) return send('SPEED,0');
    let attempted = false;
    try {
      attempted = true;
      await send('SPEED,1');
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
  if(speedOnlyMode||speedModeSwitching)throw new Error('速度环专用模式已停用外环');
  await send(`YAW,${numeric('#rateTarget',80)}`);
  ownsAngleMotion=true;
}));
document.querySelector('#headStartButton').addEventListener('click', () => angleAction(async send => {
  if(speedOnlyMode||speedModeSwitching)throw new Error('速度环专用模式已停用外环');
  await send(`HEAD,${numeric('#headTarget',180)}`);
  ownsAngleMotion=true;
}));
ui.hlogClear.addEventListener('click', () => runAction(async (send) => {
  if (firmware.hlog.active || firmware.speedEnabled || document.querySelector('#angleMode').textContent !== '停止')
    throw new Error('请先停止运动再清空 HLOG');
  await send('HLOG,CLEAR');
  if(firmware.hlogSupported) await send('HLOG,STATUS');
}));
ui.hlogStart.addEventListener('click', () => runAction(async (send) => {
  if (firmware.hlog.active) throw new Error('HLOG 已在记录');
  if (firmware.speedEnabled || document.querySelector('#angleMode').textContent !== '停止')
    throw new Error('请先停止运动再开始 HLOG');
  hlogRows.length = 0;
  await send('HLOG,START');
  if(firmware.hlogSupported) await send('HLOG,STATUS');
}));
ui.hlogStop.addEventListener('click', () => runAction(async (send) => {
  await send('HLOG,STOP');
  if(firmware.hlogSupported) await send('HLOG,STATUS');
}));
ui.hlogRead.addEventListener('click', () => runAction(async (send) => {
  const rows = await readStoppedHlog(send);
  downloadHlogCsv(rows);
  setEvent(`已下载 ${hlogRows.length} 条角环日志（mode=1 角速度，2 航向）`);
}));
for (const action of ['Clear', 'Start', 'Stop', 'Read']) {
  document.querySelector(`#wlog${action}Button`).addEventListener('click', () => runAction(async send => {
    if (action === 'Read') {
      try {
        const rows = await WheelLog.read(send, wheelLogFrames, (n, total) => {
          document.querySelector('#wlogStatus').textContent = `下载中 ${n}/${total}`;
        });
        const blob = new Blob(['\uFEFF'+WheelLog.csv(rows)], {type:'text/csv;charset=utf-8'});
        const url = URL.createObjectURL(blob), anchor = document.createElement('a');
        anchor.href = url; anchor.download = `wheel-wlog-${Date.now()}.csv`; anchor.click();
        setTimeout(() => URL.revokeObjectURL(url), 0);
        setEvent(`已校验下载 ${rows.length} 条四轮100 Hz日志；评分还需基线/瞬态/稳态校验`);
      } catch (error) {
        setEvent(`${error.message}；原始WLOG留在本次页面会话，不能用于写参`);
        throw error;
      }
      return;
    }
    if (action !== 'Stop') {
      const status = (await send('ANGLESTATUS')).split(',');
      const ps2 = (await send('PS2STATUS')).split(',');
      if (status.length !== 14 || status[2] !== '0' || status[12] !== '0' || ps2.length !== 12 || ps2[2] !== '0')
        throw new Error('先停车并锁定手柄，再切换或开始轮速日志');
    }
    await send(`WLOG,${action.toUpperCase()}`);
    await send('WLOG,STATUS');
  }));
}
for (const [id,command] of [['#yawZeroButton','ZERO'],['#calButton','CAL']])
  document.querySelector(id).addEventListener('click', () => {
    ownsAngleMotion=false; angleAction(send => send(command));
  });
let drivePressSerial=0;
const driveKeys = new Set();
let driveVector = { forward: 0, left: 0, rotate: 0 };
let driveVectorSerial = 0;
let driveFlushRunning = false;
let driveActive = false;
let driveMovePending = false;

function linkDiagnosticEvidence(transport=serialTransport) {
  return {...(transport?.diagnostic()||{mode:'bridge',commands:logLines.slice()}),uiTrace:uiTrace.slice(-32),
    inputState:{keys:[...driveKeys],vector:{...driveVector},ownsAngleMotion,ownsSpeedMotion,speedOnlyMode}};
}
document.querySelector('#speedOnlyButton').addEventListener('click',()=>{
  if(!connected()||busy||speedModeSwitching)return;
  if(!firmware.directSpeedKeep){setEvent('速度环专用模式需要 R25 DKEEP 能力');return;}
  const next=!speedOnlyMode;
  motionEpoch++;driveKeys.clear();driveVector={forward:0,left:0,rotate:0};driveVectorSerial++;
  driveActive=driveMovePending=ownsAngleMotion=ownsSpeedMotion=false;
  speedModeSwitching=true;
  void runAction(async send=>{
    try {
      await send('STOP');
      const status=(await send('ANGLESTATUS')).split(',');
      if(status.length!==14||status[2]!=='0'||status[12]!=='0')throw new Error('未确认停车，拒绝切换');
      await send(next?'TEL,1':'ATEL,1');
      speedOnlyMode=next;
      document.querySelector('#chartLoop').value=next?'wheel_all':'rate';
      chartSamples.length=0;drawChart();
      setEvent(next?'已停用角速度和方向环。先手动 ARM、启用速度环；按住方向键观察四轮曲线。':'已退出速度环专用模式；保持停车');
    } finally {speedModeSwitching=false;updateUi();}
  },true);
});
renderLinkHistory();

function sendDriveVector(forward, left, rotate) {
  if (chassisTestActive || speedModeSwitching) return;
  if(speedOnlyMode && rotate){setEvent('速度环专用模式仅支持平移，Q/E 旋转已禁用');return;}
  driveVector = { forward, left, rotate };
  driveVectorSerial += 1;
  if (connected()) void flushDriveVector();
}

async function flushDriveVector() {
  if (driveFlushRunning || !connected()) return;
  driveFlushRunning = true;
  let attemptedSerial = driveVectorSerial;
  let epoch = motionEpoch;
  try {
    while (connected()) {
      const serial = driveVectorSerial;
      attemptedSerial = serial;
      epoch = motionEpoch;
      const vector = { ...driveVector };
      if (vector.forward === 0 && vector.left === 0 && vector.rotate === 0) {
        if (driveActive) {
          await writeCommand('STOP');
          driveActive = false;
          ownsAngleMotion = false;
        }
        break;
      }
      const speed = Math.round(numeric('#driveSpeed', 100, true));
      const turnRate = Math.round(numeric('#driveTurnRate', 80, true));
      driveMovePending = true;
      await writeCommand(
        `${speedOnlyMode?'DRV':'MOVE'},${speed * vector.forward},${speed * vector.left},${speedOnlyMode?0:turnRate * vector.rotate}`,
      );
      driveMovePending = false;
      if (epoch !== motionEpoch) return; // A late MOVE reply cannot undo release/STOP.
      driveActive = true;
      ownsSpeedMotion=speedOnlyMode;ownsAngleMotion=!speedOnlyMode;speedLeaseSentAt=performance.now();
      // DRV preserves the selected stream. SPEED,1 already enables TEL;
      // respect an explicit TEL,0 for low-load comparisons and avoid an extra
      // radio exchange on every raw-speed target change.
      if(!speedOnlyMode && document.querySelector('#chartLoop').value.startsWith('wheel'))await writeCommand('TEL,1');
      if (serial === driveVectorSerial) break;
    }
  } catch (error) {
    if (epoch !== motionEpoch) return; // STOP already cancelled this request.
    const mayBeMoving = driveActive || driveMovePending;
    driveActive = false;
    ownsAngleMotion = false;
    if (mayBeMoving) stopMotion('move_error');
    setEvent(error.message);
  } finally {
    driveMovePending = false;
    driveFlushRunning = false;
    if (connected() && attemptedSerial !== driveVectorSerial &&
        (driveVector.forward || driveVector.left || driveVector.rotate)) void flushDriveVector();
  }
}

function releaseDrive() {
  drivePressSerial++;
  driveKeys.clear();
  if (driveActive || driveMovePending) {
    stopMotion(); // Urgent STOP must not wait behind an unacknowledged MOVE.
    return;
  }
  sendDriveVector(0, 0, 0);
}

function sendDriveKeys() {
  const forward = (driveKeys.has('w') ? 1 : 0) +
                  (driveKeys.has('s') ? -1 : 0);
  /* A is vehicle-left and D is vehicle-right in the shared chassis frame. */
  const left = (driveKeys.has('a') ? 1 : 0) +
               (driveKeys.has('d') ? -1 : 0);
  const rotate = (driveKeys.has('q') ? 1 : 0) +
                 (driveKeys.has('e') ? -1 : 0);
  if (forward === 0 && left === 0 && rotate === 0) {
    releaseDrive();
    return;
  }
  sendDriveVector(forward, left, rotate);
}

document.querySelectorAll('.drive-command').forEach(button => {
  button.addEventListener('pointerdown', event => {
    if (button.disabled) return;
    button.setPointerCapture(event.pointerId);
    sendDriveVector(Number(button.dataset.forward), Number(button.dataset.left),
                    Number(button.dataset.rotate));
  });
  const release = () => releaseDrive();
  button.addEventListener('pointerup',release);
  button.addEventListener('pointercancel',release);
  button.addEventListener('lostpointercapture', () => { if (ownsAngleMotion||ownsSpeedMotion) release(); });
});
window.addEventListener('keydown', event => {
  if (event.ctrlKey || event.altKey || event.metaKey) return;
  const target = event.target;
  if (target && ['INPUT', 'SELECT', 'TEXTAREA', 'BUTTON'].includes(target.tagName)) return;
  const key = event.key.toLowerCase();
  if (chassisTestActive) return;
  if (!['w', 'a', 's', 'd', 'q', 'e'].includes(key)) return;
  event.preventDefault();
  if(event.repeat)return; // A held key must not restart after STOP/fault clears input.
  if (!driveKeys.has(key)) {
    driveKeys.add(key);
    recordUi('keydown',{key,keys:[...driveKeys]});
    sendDriveKeys();
  }
});
window.addEventListener('keyup', event => {
  const key = event.key.toLowerCase();
  if (!['w', 'a', 's', 'd', 'q', 'e'].includes(key)) return;
  event.preventDefault();
  driveKeys.delete(key);
  recordUi('keyup',{key,keys:[...driveKeys]});
  sendDriveKeys();
});
window.addEventListener('blur', () => {recordUi('window_blur',{});releaseDrive();});
document.querySelector('#chartLoop').addEventListener('change', () => {
  chartSamples.length=0; drawChart();
  void runAction(send=>send(document.querySelector('#chartLoop').value.startsWith('wheel')?'TEL,1':'ATEL,1'));
});
setInterval(async () => {
  if (chassisTestActive || !ownsAngleMotion || !connected() || keepPending) return;
  const epoch = motionEpoch;
  keepPending=true;
  try { await writeCommand('KEEP'); }
  catch (error) {
    if (epoch !== motionEpoch || chassisTestActive || !ownsAngleMotion) return;
    if(error.heartbeatAckMissing && connected()) {
      recordUi('keep_ack_missing',{message:error.message,keys:[...driveKeys]});
      setEvent(error.message);
      return;
    }
    ownsAngleMotion=false;stopMotion('keep_error');setEvent(`KEEP 故障停车：${error.message}`);
  }
  finally { keepPending=false; }
},400);
setInterval(async()=>{
  if(!ownsSpeedMotion||!connected()||keepPending||performance.now()-speedLeaseSentAt<120)return;
  const epoch=motionEpoch;keepPending=true;speedLeaseSentAt=performance.now();
  try{await writeCommand('DKEEP');}
  catch(error){if(epoch===motionEpoch){ownsSpeedMotion=false;stopMotion('direct_speed_keep_error');setEvent(`速度环保活失败，已停车：${error.message}`);}}
  finally{keepPending=false;}
},40);
window.addEventListener('pagehide', () => {incident('pagehide',{},linkDiagnosticEvidence());socket?.close();void serialTransport?.close();});
setInterval(() => {
  if (firmware.tofReceivedAt && Date.now() - firmware.tofReceivedAt > 1600) {
    ui.tofDistance.textContent = '— mm';
    ui.tofState.textContent = firmware.tofStreaming ? '未收到新距离（日志采集/下载期间暂停）' : '读数已过期，开启实时距离以更新';
    if (firmware.tofGuard) document.querySelector('#frontGuardState').textContent = '距离显示已过期；板载保护独立运行，测距异常时按设置保持速度';
  }
  const reference = Math.max(firmware.receivedAt, firmware.expectedAt);
  if (connected() && firmware.telemetryEnabled && reference &&
      Date.now() - reference > 3000 && !firmware.timeoutReported) {
    firmware.timeoutReported = true;
    setEvent('TEL 超时：已开启遥测但 3 秒没有完整数据；点击“查询状态”检查 UART、LINK 和栈空间');
    updateUi();
  }
}, 300);
updateUi();
if(ui.transport?.value!=='serial')void connectBridge();
