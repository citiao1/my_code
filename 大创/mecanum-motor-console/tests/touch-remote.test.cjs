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
      '.pid-state': new Element('—'),
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
  const windowListeners = {};
  const sent = [];
  const timeouts = [];
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
          VER: 'VER,VGT6_BM50_ATTITUDE_R17', LINK: 'LINK,42,3,0,0,0,0',
          UART: 'UART,9600,42000000,4375,0x00,0,0,0,0,0',
          STACK: 'STACK,4096,1024,3072,1',
          SPEEDLIMIT: 'SPEEDLIMIT,160',
          SPEEDSTATUS: 'TEL,100,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12000,0,0,1,0,0,0',
          PIDGET: 'PID,1.2,0.2,0,1.2,0.2,0,1.2,0.2,0,1.2,0.2,0',
          ARM: 'ACK,ARM,WHEELS_OFF_GROUND', STOP: 'ACK,STOP',
          CAPS: 'CAPS,ATTITUDE_R17,20,10,160,60,60,80,SPEED_TUNABLE,HLOG,PS2,TRACKMOVE,HLOG_MODE',
          PS2STATUS: 'PS2,1,0,0x73,0x0000,128,128,128,128,30,0,10',
          PS2RAW: 'PS2RAW,FF,73,5A,FF,FF,7F,80,7F,80,1,1',
          YAWGET: 'YAWPID,0.1500,2.5000,60,80,20',
          HEADGET: 'HEADPID,6.0000,1.2500,80,20,D_MEAS',
          ANGLESTATUS: 'ATEL,100,0,1,0,0,0,0,0,0,0,0,0,12000',
          'HLOG,STATUS': 'HLOG,STATUS,0,0,1024,0,0',
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
    setTimeout: (_callback, delay) => { timeouts.push(delay); return {}; }, clearTimeout: () => {},
    setInterval: (callback) => intervals.push(callback),
    localStorage: { getItem: () => '', setItem: () => {} },
    window: {
      location: { protocol: 'http:', hostname: '127.0.0.1', search: '' },
      addEventListener: (name, callback) => { windowListeners[name] = callback; },
    },
    document: {
      activeElement: null,
      querySelector: (selector) => {
        if (!elements.has(selector)) {
          const value = selector === '#speedLimitInput' ? '200' :
            selector === '#driveSpeed' ? '30' :
            selector === '#driveTurnRate' ? '20' :
            selector === '#chartLoop' ? 'rate' : '0';
          elements.set(selector, new Element(value));
        }
        return elements.get(selector);
      },
      querySelectorAll: selectors,
      createElement: () => new Element(), createTextNode: (text) => text,
    },
  });
  const source = fs.readFileSync(path.join(__dirname, '..', 'touch-remote.js'), 'utf8');
  vm.runInContext(fs.readFileSync(path.join(__dirname, '..', 'chassis-test.js'), 'utf8'), context);
  vm.runInContext(source, context);
  vm.runInContext('hardwareConnected = true; bridgeReady = true; firmwareCompatible = true; stackHealthy = true; connecting = false; updateUi()', context);
  return {
    context, sent, rows, elements, timeouts,
    eval: (source) => vm.runInContext(source, context),
    line: (line) => { context.testLine = line; vm.runInContext('handleLine(testLine)', context); },
    advance: (ms) => { now += ms; intervals.forEach((callback) => callback()); },
    fail: (command) => { fail = command; },
    noReply: () => { respond = false; },
    key: (name, event) => windowListeners[name]?.(event),
    flush: async () => { for (let i = 0; i < 12; i++) await new Promise(setImmediate); },
  };
}

test('HLOG batch download waits longer than the bridge 20 second allowance', () => {
  const f = consoleFixture();
  f.noReply();
  f.eval("socket = new WebSocket(); void writeCommand('HLOG,READ,0,8')");
  assert.equal(f.timeouts.at(-1), 25000);
  f.eval("void writeCommand('VER')");
  assert.equal(f.timeouts.at(-1), 5000);
});

test('R17 stopped download checks fresh state, exact batches, modes and MCU ticks', async () => {
  for (const fault of [null, 'active', 'moving', 'overflow', 'end', 'duplicate', 'tick', 'mode']) {
    const f = consoleFixture();
    const sent = [];
    f.context.downloadSend = async (command) => {
      sent.push(command);
      if (command === 'ANGLESTATUS')
        return `ATEL,100,${fault === 'moving' ? 1 : 0},1,0,0,0,0,0,0,0,0,0,12000`;
      if (command === 'HLOG,STATUS')
        return `HLOG,STATUS,${fault === 'active' ? 1 : 0},10,1024,${fault === 'overflow' ? 1 : 0},10`;
      const [, , offsetText, countText] = command.split(',');
      const offset = Number(offsetText), count = Number(countText);
      for (let index = offset; index < offset + count; index += 1) {
        const seq = fault === 'duplicate' && index === 1 ? 0 : index;
        const tick = (0xfffffff0 + 20 * (fault === 'tick' && index === 1 ? 0 : index)) >>> 0;
        const mode = fault === 'mode' && index === 1 ? 3 : 1;
        f.line(`HLOG,DATA,${seq},${tick},0,0,0,15000,12000,0,5000,0,0,0,${mode}`);
      }
      return `HLOG,END,${fault === 'end' ? offset + 1 : offset},${count},10`;
    };
    const action = f.eval('readStoppedHlog(downloadSend)');
    if (fault) {
      await assert.rejects(action, undefined, fault);
    } else {
      const rows = await action;
      assert.equal(rows.length, 10);
      assert.deepEqual(sent, ['ANGLESTATUS', 'HLOG,STATUS', 'HLOG,READ,0,8', 'HLOG,READ,8,2']);
      assert.match(f.eval('hlogCsv(hlogRows)'), /mode/);
    }
    assert.ok(sent.every(command => !/^(ARM|MOVE|HEAD,|YAW,|PID,)/.test(command)));
  }
});

test('PS2 diagnostic query reads status and raw bytes without enabling motors', async () => {
  const f = consoleFixture();
  f.elements.get('#ps2QueryButton').listeners.click();
  await f.flush();
  assert.deepEqual(f.sent, ['PS2STATUS', 'PS2RAW', 'ANGLESTATUS']);
  assert.match(f.elements.get('#ps2Status').textContent, /已连接/);
  assert.match(f.elements.get('#ps2RawStatus').textContent, /FF 73 5A/);
  assert.match(f.elements.get('#ps2RawStatus').textContent, /模拟帧有效/);
});

test('STOP cancellation of an old KEEP cannot stop a new motion', async () => {
  const f = consoleFixture();
  f.noReply();
  f.eval('ownsAngleMotion = true');
  f.advance(400);
  const keepId = f.eval('requestSerial');
  assert.deepEqual(f.sent, ['KEEP']);
  f.eval('stopMotion(); ownsAngleMotion = true');
  f.eval(`handleResult({id:${keepId},ok:false,error:'cancelled by STOP/DISARM'})`);
  await f.flush();
  assert.deepEqual(f.sent, ['KEEP', 'STOP']);
  assert.equal(f.eval('ownsAngleMotion'), true);
});

test('automatic tests own KEEP; global heartbeat and drive keys cannot interfere', async () => {
  const f = consoleFixture();
  f.eval('chassisTestActive = true; ownsAngleMotion = true');
  f.advance(400);
  f.key('keydown', {key:'w', preventDefault(){}});
  f.eval('sendDriveVector(1, 0, 0)');
  await f.flush();
  assert.deepEqual(f.sent, []);
});

test('disconnect cancels a test epoch instead of resuming or retaining busy state', () => {
  const f = consoleFixture();
  f.eval("chassisTestActive = true; busy = true; document.querySelector('#testSafety').checked = true");
  const epoch = f.eval('operationSerial');
  f.line('STATUS,0,BLE peripheral disconnected');
  assert.equal(f.eval('chassisTestActive'), false);
  assert.equal(f.eval('busy'), false);
  assert.ok(f.eval('operationSerial') > epoch);
  assert.equal(f.eval("document.querySelector('#testSafety').checked"), false);
  assert.match(f.elements.get('#chassisTestStatus').textContent, /不会自动恢复/);
  assert.match(f.elements.get('#eventText').textContent, /BLE peripheral disconnected/);
});

test('test safety confirmation is required before even a query is sent', async () => {
  const f = consoleFixture();
  f.eval('firmware.speedEnabled = true');
  await f.eval("runChassisTest('translation')");
  assert.deepEqual(f.sent, []);
  assert.match(f.elements.get('#eventText').textContent, /确认/);
});

test('new buttons pass separate translation and yaw inputs to the measured runner', async () => {
  const f = consoleFixture();
  f.eval("firmware.speedEnabled = true; document.querySelector('#testSafety').checked = true;");
  for (const [id, value] of [['#testRpm','30'], ['#translateDistance','30'],
      ['#translateAngle','17'], ['#translateHeading','10']])
    f.eval(`document.querySelector('${id}').value = '${value}'`);
  f.eval(`ChassisTest.run = async (port, config) => {
    globalThis.capturedTest = config;
    port.record({tick:1});
    return {movingMaxError:1,movingRmsError:0.5,maxCrossM:0.005,residualM:0.01};
  }`);
  await f.elements.get('#translateTestButton').listeners.click();
  assert.equal(f.eval('capturedTest.angle'), 17);
  assert.equal(f.eval('capturedTest.headingOffset'), 10);
  assert.equal(f.eval('capturedTest.distance'), 0.3);
  assert.equal(f.eval('chassisTestActive'), false);
  assert.equal(f.elements.get('#chassisCsvButton').disabled, false);
});

test('a queued manual-drive write cannot be taken over by an automatic test', async () => {
  const f = consoleFixture();
  f.eval("firmware.speedEnabled = true; driveFlushRunning = true; document.querySelector('#testSafety').checked = true");
  await f.eval("runChassisTest('square')");
  assert.deepEqual(f.sent, []);
  assert.match(f.elements.get('#eventText').textContent, /手动运动命令结束/);
});

test('PS2 raw bytes distinguish idle lines and digital mode', () => {
  const f = consoleFixture();
  f.line('PS2RAW,FF,FF,FF,FF,FF,FF,FF,FF,FF,0,7');
  assert.match(f.elements.get('#ps2RawStatus').textContent, /DAT 全高/);
  f.line('PS2RAW,00,00,00,00,00,00,00,00,00,0,8');
  assert.match(f.elements.get('#ps2RawStatus').textContent, /DAT 全低/);
  f.line('PS2RAW,FF,41,5A,FF,FF,00,00,00,00,0,9');
  assert.match(f.elements.get('#ps2RawStatus').textContent, /数字模式/);
  const previous = f.elements.get('#ps2RawStatus').textContent;
  f.line('PS2RAW,ZZ,41,5A,FF,FF,00,00,00,00,0,9');
  assert.equal(f.elements.get('#ps2RawStatus').textContent, previous);
});

test('writing speed limit does not clear speed enable or TEL state', () => {
  const f = consoleFixture();
  f.line('ACK,SPEED,1');
  f.line('ACK,SPEED,LIMIT');
  assert.equal(f.eval('firmware.speedEnabled'), true);
  assert.equal(f.eval('firmware.telemetryEnabled'), true);
});

test('STOP preserves TEL selection and TEL,0 disables its watchdog', () => {
  const f = consoleFixture();
  f.line('ACK,SPEED,1');
  f.line('ACK,STOP');
  assert.equal(f.eval('firmware.telemetryEnabled'), true);
  f.line('ACK,TEL,0');
  f.advance(5000);
  assert.doesNotMatch(f.elements.get('#eventText').textContent, /TEL 超时/);
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

test('STOP then start restores speed control without Bluetooth reconnect', async () => {
  const f = consoleFixture();
  f.line('ACK,SPEED,1');
  f.elements.get('#stopButton').listeners.click();
  await f.flush();
  f.elements.get('#speedEnableButton').listeners.click();
  await f.flush();
  assert.deepEqual(f.sent, ['STOP', 'SPEED,1']);
  assert.equal(f.eval('firmware.speedEnabled'), true);
});

test('connection queries are sequential and never turn on streaming', async () => {
  const f = consoleFixture();
  f.line('STATUS,0,disconnected');
  f.line('STATUS,1,BLE connected');
  await f.flush();
  assert.deepEqual(f.sent, ['VER', 'UART', 'LINK', 'STACK', 'SPEEDLIMIT', 'SPEEDSTATUS', 'PIDGET', 'CAPS', 'PS2STATUS', 'YAWGET', 'HEADGET', 'ANGLESTATUS', 'HLOG,STATUS', 'STACK']);
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

test('R17 firmware passes the page compatibility check', () => {
  const f = consoleFixture();
  f.line('VER,VGT6_BM50_ATTITUDE_R17');
  assert.equal(f.eval('firmwareCompatible'), true);
  assert.match(f.elements.get('#eventText').textContent, /ATTITUDE_R17/);
});

test('PS2 status is shown without enabling motion', () => {
  const f = consoleFixture();
  f.line('PS2,1,0,0x73,0x0000,128,128,128,128,30,0,10');
  assert.match(f.elements.get('#ps2Status').textContent, /已连接/);
  assert.match(f.elements.get('#ps2Status').textContent, /锁定/);
  assert.match(f.elements.get('#ps2Status').textContent, /30 RPM/);
  assert.equal(f.eval('firmware.speedEnabled'), false);
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
  for (const version of ['VGT6_BM50_SPEED_LINE_R3', 'VGT6_BM50_ATTITUDE_R13', 'VGT6_BM50_ATTITUDE_R14', 'VGT6_BM50_ATTITUDE_R15', 'VGT6_BM50_ATTITUDE_R16']) {
    const f = consoleFixture();
    f.line(`VER,${version}`);
    assert.equal(f.elements.get('#armButton').disabled, true);
    assert.equal(f.elements.get('#queryButton').disabled, false);
    assert.equal(f.elements.get('#stopButton').disabled, false);
    await assert.rejects(f.eval("writeCommand('ARM')"), /检查未通过/);
    await f.eval("writeCommand('STOP')");
    assert.deepEqual(f.sent, ['STOP']);
  }
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
  assert.deepEqual(f.sent, ['VER', 'UART', 'LINK', 'STACK', 'SPEEDLIMIT', 'SPEEDSTATUS', 'PIDGET', 'CAPS', 'PS2STATUS', 'YAWGET', 'HEADGET', 'ANGLESTATUS', 'HLOG,STATUS', 'STACK']);
  assert.equal(f.eval('firmware.telemetryEnabled'), false);
});

test('speed limit writes are capped to the 160 RPM operating ceiling', async () => {
  const f = consoleFixture();
  f.elements.get('#speedLimitInput').value = '1000';
  f.elements.get('#speedLimitButton').listeners.click();
  await f.flush();
  assert.deepEqual(f.sent, ['SPDLIMIT,160']);
  assert.equal(f.elements.get('#speedLimitInput').value, 160);
});

test('the page exposes the four-wheel tunable PID table', () => {
  const html = fs.readFileSync(path.join(__dirname, '..', 'index.html'), 'utf8');
  assert.match(html, /id="speedPidTable"/);
  assert.match(html, /data-motor="A"/);
  assert.match(html, /data-motor="D"/);
  assert.match(html, /id="speedPidWriteAllButton"/);
  assert.match(html, /id="speedLimitInput"[^>]*max="160" value="160"/);
});
test('the page exposes RAM-only HLOG controls and capability state', () => {
  const html = fs.readFileSync(path.join(__dirname, '..', 'index.html'), 'utf8');
  assert.match(html, /id="hlogClearButton"/);
  assert.match(html, /id="hlogStartButton"/);
  assert.match(html, /id="hlogStopButton"/);
  assert.match(html, /id="hlogReadButton"/);
  const f = consoleFixture();
  f.line('CAPS,ATTITUDE_R17,20,10,160,60,60,80,SPEED_TUNABLE,HLOG,PS2,TRACKMOVE,HLOG_MODE');
  f.line('HLOG,STATUS,0,12,1024,0,12');
  assert.equal(f.eval('firmware.hlogSupported'), true);
  assert.match(f.elements.get('#hlogStatus').textContent, /已停止 · 12\/1024 条/);
});
test('HLOG data is parsed and exported with a stable CSV schema', () => {
  const f = consoleFixture();
  f.line('HLOG,DATA,12,240,1000,1100,-100,500,450,20,10,0,30,-4');
  f.line('HLOG,END,0,1,1');
  const csv = f.eval('hlogCsv(hlogRows)');
  assert.match(csv, /^sequence,tick,target_heading_mdeg,actual_heading_mdeg/);
  assert.match(csv, /12,240,1000,1100,-100,500,450,20,10,0,30,-4/);
  assert.equal(f.eval('firmware.hlog.lastBatch'), 1);
});

test('R17 rate and heading logs export an explicit controller mode', () => {
  const f = consoleFixture();
  f.line('HLOG,DATA,0,120,0,0,0,15000,10000,0,1000,0,0,0,1');
  f.line('HLOG,DATA,1,140,15000,12000,3000,8000,7600,40000,12000,0,20,0,2');
  f.line('HLOG,DATA,2,160,0,0,0,0,0,0,0,0,0,0,3');
  const csv = f.eval('hlogCsv(hlogRows)');
  assert.match(csv, /forward_rpm,left_rpm,mode\r\n/);
  assert.match(csv, /0,120,0,0,0,15000,10000,0,1000,0,0,0,1/);
  assert.equal(f.eval('hlogRows.length'), 2);
});
test('PIDGET populates all four speed-loop rows', () => {
  const f = consoleFixture();
  f.line('PID,1.2000,0.0991,0,1.2000,0.0998,0,1.2000,0.0998,0,1.2000,0.1008,0');
  assert.equal(f.rows[0].querySelector('.kp-input').value, '1.2000');
  assert.equal(f.rows[1].querySelector('.ki-input').value, '0.0998');
  assert.equal(f.rows[3].querySelector('.ki-input').value, '0.1008');
});
test('writing one speed-loop PID sends PID then verifies with PIDGET', async () => {
  const f = consoleFixture();
  f.rows[0].querySelector('.kp-input').value = '1.3';
  f.rows[0].querySelector('.ki-input').value = '0.11';
  f.rows[0].querySelector('.kd-input').value = '0';
  f.rows[0].querySelector('.pid-send').listeners.click();
  await f.flush();
  assert.deepEqual(f.sent, ['PID,A,1.3000,0.1100,0.0000', 'PIDGET']);
  assert.equal(f.rows[0].querySelector('.pid-state').textContent, '已写入并校验');
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

test('WASD drives while focused outside form controls and stops on release', async () => {
  const f = consoleFixture();
  const event = (key) => ({
    key, repeat: false, ctrlKey: false, altKey: false, metaKey: false,
    target: { tagName: 'BODY' }, preventDefault() {},
  });
  f.key('keydown', event('w'));
  await f.flush();
  f.key('keyup', event('w'));
  await f.flush();
  assert.deepEqual(f.sent, ['MOVE,30,0,0', 'STOP']);
});

test('A/D use the corrected lateral sign', async () => {
  const f = consoleFixture();
  const event = (key) => ({
    key, repeat: false, ctrlKey: false, altKey: false, metaKey: false,
    target: { tagName: 'BODY' }, preventDefault() {},
  });
  f.key('keydown', event('a'));
  await f.flush();
  f.key('keyup', event('a'));
  await f.flush();
  f.key('keydown', event('d'));
  await f.flush();
  f.key('keyup', event('d'));
  await f.flush();
  assert.deepEqual(f.sent, ['MOVE,0,30,0', 'STOP', 'MOVE,0,-30,0', 'STOP']);
});

test('Q/E drive in-place left/right rotation and stop on release', async () => {
  const f = consoleFixture();
  const event = (key) => ({
    key, repeat: false, ctrlKey: false, altKey: false, metaKey: false,
    target: { tagName: 'BODY' }, preventDefault() {},
  });
  f.key('keydown', event('q'));
  await f.flush();
  f.key('keyup', event('q'));
  await f.flush();
  f.key('keydown', event('e'));
  await f.flush();
  f.key('keyup', event('e'));
  await f.flush();
  assert.deepEqual(f.sent, ['MOVE,0,0,20', 'STOP', 'MOVE,0,0,-20', 'STOP']);
});

test('translation and Q/E can be combined in one MOVE command', async () => {
  const f = consoleFixture();
  const event = (key) => ({
    key, repeat: false, ctrlKey: false, altKey: false, metaKey: false,
    target: { tagName: 'BODY' }, preventDefault() {},
  });
  f.key('keydown', event('w'));
  await f.flush();
  f.key('keydown', event('q'));
  await f.flush();
  assert.deepEqual(f.sent, ['MOVE,30,0,0', 'MOVE,30,0,20']);
  f.key('keyup', event('q'));
  f.key('keyup', event('w'));
  await f.flush();
  assert.deepEqual(f.sent, ['MOVE,30,0,0', 'MOVE,30,0,20', 'MOVE,30,0,0', 'STOP']);
});

test('rapid simultaneous key presses deliver the latest vector after the pending command', async () => {
  const f = consoleFixture();
  const event = (key) => ({
    key, repeat: false, ctrlKey: false, altKey: false, metaKey: false,
    target: { tagName: 'BODY' }, preventDefault() {},
  });
  f.key('keydown', event('w'));
  f.key('keydown', event('q'));
  await f.flush();
  assert.deepEqual(f.sent, ['MOVE,30,0,0', 'MOVE,30,0,20']);
  f.key('keyup', event('q'));
  f.key('keyup', event('w'));
  await f.flush();
  assert.deepEqual(f.sent, ['MOVE,30,0,0', 'MOVE,30,0,20',
    'MOVE,30,0,0', 'STOP']);
});

test('buttons stay visually enabled while a command is waiting', () => {
  const f = consoleFixture();
  f.noReply();
  f.elements.get('#speedLimitButton').listeners.click();
  assert.equal(f.elements.get('#speedLimitButton').disabled, false);
  assert.equal(f.elements.get('#queryButton').disabled, false);
});
