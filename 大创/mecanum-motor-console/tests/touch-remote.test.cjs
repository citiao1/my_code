const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

function consoleFixture() {
  class Element {
    constructor(value = '') { this.value = value; this.listeners = {}; this.textContent = ''; }
    addEventListener(name, callback) { this.listeners[name] = callback; }
    replaceChildren() {}
    append() {}
    getContext() { return {clearRect(){},beginPath(){},moveTo(){},lineTo(){},stroke(){},fillText(){}}; }
  }
  const elements = new Map();
  const rows = ['A', 'B', 'C', 'D'].map((motor) => {
    const inputs = {
      '.target-input': new Element('30'), '.kp-input': new Element('1.2'),
      '.ki-input': new Element('0.2'), '.kd-input': new Element('0'),
      '.pid-send': new Element(),
    };
    return { dataset: { motor }, querySelector: (selector) => inputs[selector] };
  });
  const selectors = (selector) => {
    if (selector === '.pid-row[data-motor]') return rows;
    if (selector === '.pid-send') return rows.map((row) => row.querySelector('.pid-send'));
    if (selector === '.target-input') return rows.map((row) => row.querySelector('.target-input'));
    return [];
  };
  const intervals = [];
  const sent = [];
  let now = 10000;
  let respond = true;
  let fail = '';
  let context;
  class WebSocket {
    static OPEN = 1;
    constructor() { this.readyState = WebSocket.OPEN; }
    addEventListener() {}
    close() { this.readyState = 3; }
    send(message) {
      const request = JSON.parse(message);
      sent.push(request.command);
      if (!respond) return;
      queueMicrotask(() => {
        const command = request.command;
        const replies = {
          VER: 'VER,VGT6_BM50_ATTITUDE_R7', LINK: 'LINK,42,3,0,0,0,0',
          UART: 'UART,9600,42000000,4375,0x00,0,0,0,0,0',
          STACK: 'STACK,4096,1024,3072,1',
          SPEEDLIMIT: 'SPEEDLIMIT,200',
          SPEEDSTATUS: 'TEL,100,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12000,0,0,1,0,0,0',
          PIDGET: 'PID,1.2,0.2,0,1.2,0.2,0,1.2,0.2,0,1.2,0.2,0',
          ARM: 'ACK,ARM,WHEELS_OFF_GROUND', STOP: 'ACK,STOP',
          CAPS: 'CAPS,ATTITUDE_R7,20,10,200,60,60,80,SPEED_FIXED',
          YAWGET: 'YAWPID,0.1500,2.5000,60,80,20',
          HEADGET: 'HEADPID,5.0000,1.2500,80,20,D_MEAS',
          ANGLESTATUS: 'ATEL,100,0,1,0,0,0,0,0,0,0,0,0,12000',
        };
        const reply = command === fail ? 'ERR,SPEED,ARM_FIRST' :
          replies[command] || `ACK,${command.startsWith('SPD,') ? 'SPD' : command.startsWith('PID,') ? 'PID' : command.startsWith('YAW,') ? 'YAW' : command}`;
        context.testLine = reply;
        vm.runInContext('handleLine(testLine)', context);
        context.testResult = {
          type: 'result', id: request.id, ok: command !== fail,
          reply, error: command === fail ? reply : undefined,
        };
        vm.runInContext('handleResult(testResult)', context);
      });
    }
  }
  context = vm.createContext({
    console, URL, URLSearchParams, WebSocket,
    Date: { now: () => now },
    setTimeout: () => ({}), clearTimeout: () => {},
    setInterval: (callback) => intervals.push(callback),
    localStorage: { getItem: () => '', setItem: () => {} },
    window: { location: { protocol: 'http:', hostname: '127.0.0.1', search: '' }, addEventListener() {} },
    document: {
      activeElement: null,
      querySelector: (selector) => {
        if (!elements.has(selector)) elements.set(selector, new Element(selector === '#speedLimitInput' ? '200' : selector === '#chartLoop' ? 'rate' : '0'));
        return elements.get(selector);
      },
      querySelectorAll: selectors,
      createElement: () => new Element(), createTextNode: (text) => text,
    },
  });
  const source = fs.readFileSync(path.join(__dirname, '..', 'touch-remote.js'), 'utf8');
  vm.runInContext(source, context);
  vm.runInContext('hardwareConnected = true; bridgeReady = true; firmwareCompatible = true; stackHealthy = true; connecting = false; updateUi()', context);
  return {
    context, sent, rows, elements,
    eval: (source) => vm.runInContext(source, context),
    line: (line) => { context.testLine = line; vm.runInContext('handleLine(testLine)', context); },
    advance: (ms) => { now += ms; intervals.forEach((callback) => callback()); },
    fail: (command) => { fail = command; },
    noReply: () => { respond = false; },
    flush: async () => { for (let i = 0; i < 12; i++) await new Promise(setImmediate); },
  };
}

test('writing speed limit does not clear speed enable or TEL state', () => {
  const f = consoleFixture();
  f.line('ACK,SPEED,1');
  f.line('ACK,SPEED,LIMIT');
  assert.equal(f.eval('firmware.speedEnabled'), true);
  assert.equal(f.eval('firmware.telemetryEnabled'), true);
});

test('STOP and TEL,0 do not report a false TEL timeout', () => {
  const f = consoleFixture();
  f.line('ACK,SPEED,1');
  f.line('ACK,STOP');
  f.advance(5000);
  assert.equal(f.elements.get('#eventText').textContent, 'ACK,STOP');
  assert.equal(f.eval('firmware.telemetryEnabled'), false);
  f.line('ACK,TEL,1');
  f.line('ACK,TEL,0');
  f.advance(5000);
  assert.equal(f.elements.get('#eventText').textContent, 'ACK,TEL,0');
});

test('enabled TEL with no first packet really times out', () => {
  const f = consoleFixture();
  f.line('ACK,TEL,1');
  f.advance(3100);
  assert.match(f.elements.get('#eventText').textContent, /TEL 超时/);
  f.line('TEL,100,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12000,0,0,1,0,0,0');
  assert.equal(f.elements.get('#eventText').textContent, 'TEL 遥测已恢复');
});

test('a one-shot packet does not enable a telemetry watchdog', () => {
  const f = consoleFixture();
  f.line('TEL,100,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12000,0,0,1,0,0,0');
  f.advance(5000);
  assert.doesNotMatch(f.elements.get('#eventText').textContent, /TEL 超时/);
});

test('speed enable failure never sends four target commands', async () => {
  const f = consoleFixture();
  f.fail('SPEED,1');
  f.elements.get('#speedEnableButton').listeners.click();
  await f.flush();
  assert.deepEqual(f.sent, ['SPEED,1', 'STOP']);
  assert.match(f.elements.get('#eventText').textContent, /ARM_FIRST/);
});

test('STOP then start restores targets without Bluetooth reconnect', async () => {
  const f = consoleFixture();
  f.line('ACK,SPEED,1');
  f.elements.get('#stopButton').listeners.click();
  await f.flush();
  f.elements.get('#speedEnableButton').listeners.click();
  await f.flush();
  assert.deepEqual(f.sent, ['STOP', 'SPEED,1', 'SPD,A,30', 'SPD,B,30', 'SPD,C,30', 'SPD,D,30']);
  assert.equal(f.eval('firmware.speedEnabled'), true);
});

test('connection queries are sequential and never turn on streaming', async () => {
  const f = consoleFixture();
  f.line('STATUS,0,disconnected');
  f.line('STATUS,1,BLE connected');
  await f.flush();
  assert.deepEqual(f.sent, ['VER', 'UART', 'LINK', 'STACK', 'SPEEDLIMIT', 'SPEEDSTATUS', 'PIDGET', 'CAPS', 'YAWGET', 'HEADGET', 'ANGLESTATUS', 'STACK']);
  assert.equal(f.eval('firmware.telemetryEnabled'), false);
});

test('a WebSocket send is not considered a successful MCU execution', async () => {
  const f = consoleFixture();
  f.noReply();
  let resolved = false;
  f.eval("writeCommand('ARM')").then(() => { resolved = true; });
  await f.flush();
  assert.equal(resolved, false);
  assert.equal(f.eval('pending.size'), 1);
});

test('an old bridge is detected without sending JSON to the MCU', async () => {
  const f = consoleFixture();
  f.eval('bridgeReady = false; hardwareConnected = false');
  f.line('STATUS,1,BLE connected');
  await f.flush();
  assert.deepEqual(f.sent, []);
  assert.equal(f.elements.get('#armButton').disabled, true);
  assert.match(f.elements.get('#eventText').textContent, /旧桥接程序/);
});

test('R3 bridge handshake matches this page', () => {
  const f = consoleFixture();
  f.line('BRIDGE,BM50_ACK_R3');
  assert.equal(f.eval('bridgeReady'), true);
});

test('R7 firmware passes the page compatibility check', () => {
  const f = consoleFixture();
  f.line('VER,VGT6_BM50_ATTITUDE_R7');
  assert.equal(f.eval('firmwareCompatible'), true);
  assert.match(f.elements.get('#eventText').textContent, /ATTITUDE_R7/);
});

test('UART and stack diagnostics display the MCU measurements', () => {
  const f = consoleFixture();
  f.line('UART,9600,42000000,4375,0x0C,0,0,1,1,0');
  f.line('STACK,4096,1400,2696,1');
  f.line('RXHEX,33504545442C');
  assert.match(f.elements.get('#uartStatus').textContent, /FE 1 \/ ORE 1/);
  assert.match(f.elements.get('#stackStatus').textContent, /已用水位 1400 B/);
  assert.match(f.elements.get('#commandLog').textContent, /RXHEX,33504545442C/);
  assert.equal(f.eval('stackHealthy'), true);
});

test('old firmware disables motion but leaves STOP and query available', async () => {
  const f = consoleFixture();
  f.line('VER,VGT6_BM50_SPEED_LINE_R3');
  assert.equal(f.elements.get('#armButton').disabled, true);
  assert.equal(f.elements.get('#queryButton').disabled, false);
  assert.equal(f.elements.get('#stopButton').disabled, false);
  await assert.rejects(f.eval("writeCommand('ARM')"), /检查未通过/);
  await f.eval("writeCommand('STOP')");
  assert.deepEqual(f.sent, ['STOP']);
});

test('stack failure disables motion and a new valid query restores control', async () => {
  const f = consoleFixture();
  f.line('ERR,STACK_GUARD,MOTORS_DISABLED');
  assert.equal(f.elements.get('#armButton').disabled, true);
  await assert.rejects(f.eval("writeCommand('SPEED,1')"), /检查未通过/);
  f.line('STACK,4096,1200,2896,0');
  assert.equal(f.eval('stackHealthy'), false);
  f.line('STACK,4096,1200,2896,1');
  assert.equal(f.elements.get('#armButton').disabled, false);
});

test('state query refreshes UART and stack without enabling telemetry', async () => {
  const f = consoleFixture();
  f.elements.get('#queryButton').listeners.click();
  await f.flush();
  assert.deepEqual(f.sent, ['VER', 'UART', 'LINK', 'STACK', 'SPEEDLIMIT', 'SPEEDSTATUS', 'PIDGET', 'YAWGET', 'HEADGET', 'ANGLESTATUS', 'STACK']);
  assert.equal(f.eval('firmware.telemetryEnabled'), false);
});

test('writing a wheel target never sends a PID change', async () => {
  const f = consoleFixture();
  f.rows[0].querySelector('.kp-input').value = '0.8';
  f.rows[0].querySelector('.pid-send').listeners.click();
  await f.flush();
  assert.deepEqual(f.sent, ['SPD,A,30']);
});

test('speed limit writes are capped to the 200 RPM hardware ceiling', async () => {
  const f = consoleFixture();
  f.elements.get('#speedLimitInput').value = '1000';
  f.elements.get('#speedLimitButton').listeners.click();
  await f.flush();
  assert.deepEqual(f.sent, ['SPDLIMIT,200']);
  assert.equal(f.elements.get('#speedLimitInput').value, 200);
});

test('an oversized wheel target cannot start or write even with a modified limit input', async () => {
  const f = consoleFixture();
  f.elements.get('#speedLimitInput').value = '1000';
  f.rows[0].querySelector('.target-input').value = '201';
  f.rows[0].querySelector('.pid-send').listeners.click();
  await f.flush();
  f.elements.get('#speedEnableButton').listeners.click();
  await f.flush();
  assert.deepEqual(f.sent, []);
  assert.match(f.elements.get('#eventText').textContent, /最高 200 RPM/);
});

test('the page advertises fixed per-wheel gains and the correct default limit', () => {
  const html = fs.readFileSync(path.join(__dirname, '..', 'index.html'), 'utf8');
  assert.equal((html.match(/class="kp-input"[^>]*value="1.20" readonly/g) || []).length, 4);
  assert.equal((html.match(/class="ki-input"[^>]*value="0.0998" readonly/g) || []).length, 2);
  assert.match(html,/class="ki-input"[^>]*value="0.0991" readonly/);
  assert.match(html,/class="ki-input"[^>]*value="0.1008" readonly/);
  assert.match(html, /id="speedLimitInput"[^>]*max="200" value="200"/);
});
test('external angular telemetry cannot cause the page to send heartbeats', async () => {
  const f=consoleFixture();
  f.line('ATEL,1234,1,1,15000,14000,5000,0,10000,-10000,0,0,1,12300');
  f.advance(400);
  await f.flush();
  assert.deepEqual(f.sent,[]);
  assert.equal(f.elements.get('#rateActual').textContent,'14.00 °/s');
});
test('angle start sustains heartbeat and STOP permits another start', async () => {
  const f=consoleFixture();
  f.eval("document.querySelector('#rateTarget').value='15'");
  f.elements.get('#rateStartButton').listeners.click();
  await f.flush();
  f.advance(400); await f.flush();
  f.elements.get('#stopButton').listeners.click();
  await f.flush();
  f.elements.get('#rateStartButton').listeners.click();
  await f.flush();
  assert.deepEqual(f.sent,['YAW,15','KEEP','STOP','YAW,15']);
});
