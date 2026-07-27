const APP_VERSION = "2026-07-27 22:15";
const UART_SERVICE_UUID = "0000ffe0-0000-1000-8000-00805f9b34fb";
const UART_NOTIFY_UUID = "0000ffe1-0000-1000-8000-00805f9b34fb";
const UART_WRITE_UUID = "0000ffe2-0000-1000-8000-00805f9b34fb";

const stateNames = {
  0: "上电等待",
  1: "使能电机2",
  2: "电机1回零",
  3: "电机2回零",
  4: "等待回零",
  5: "保持",
  6: "装载电机2",
  7: "同步启动",
  8: "运动中",
  9: "回正电机2",
  10: "回正同步",
  11: "回正中",
  12: "设置电机2",
  13: "设置同步",
};

const elements = {
  connectButton: document.querySelector("#connectButton"),
  connectionStatus: document.querySelector("#connectionStatus"),
  deviceLabel: document.querySelector("#deviceLabel"),
  actual1: document.querySelector("#actual1"),
  actual2: document.querySelector("#actual2"),
  target1: document.querySelector("#target1"),
  target2: document.querySelector("#target2"),
  angle1: document.querySelector("#angle1"),
  angle2: document.querySelector("#angle2"),
  sendButton: document.querySelector("#sendButton"),
  plusButton: document.querySelector("#plusButton"),
  zeroButton: document.querySelector("#zeroButton"),
  controllerState: document.querySelector("#controllerState"),
  bluetoothDebug: document.querySelector("#bluetoothDebug"),
  commandState: document.querySelector("#commandState"),
  clearLogButton: document.querySelector("#clearLogButton"),
  communicationLog: document.querySelector("#communicationLog"),
};

let bluetoothDevice = null;
let writeCharacteristic = null;
let notifyCharacteristic = null;
let writeMode = "";
let receiveBuffer = "";
const textEncoder = new TextEncoder();
const textDecoder = new TextDecoder("ascii");

function appendLog(message) {
  const timestamp = new Date().toLocaleTimeString("zh-CN", { hour12: false });
  elements.communicationLog.textContent += `[${timestamp}] ${message}\n`;
  const lines = elements.communicationLog.textContent.split("\n");
  if (lines.length > 250) {
    elements.communicationLog.textContent = lines.slice(-200).join("\n");
  }
  elements.communicationLog.scrollTop = elements.communicationLog.scrollHeight;
}

function setConnected(connected) {
  elements.connectionStatus.textContent = connected ? "已连接" : "未连接";
  elements.connectionStatus.className = connected
    ? "status status-online"
    : "status status-offline";
  elements.connectButton.textContent = connected ? "断开设备" : "连接设备";
  elements.connectButton.disabled = false;
  elements.sendButton.disabled = !connected;
  elements.plusButton.disabled = !connected;
  elements.zeroButton.disabled = !connected;
  if (!connected) {
    elements.commandState.textContent = "等待连接";
  }
}

async function connectBluetooth() {
  if (!navigator.bluetooth) {
    throw new Error("当前浏览器不支持 Web Bluetooth，请使用最新版 Edge 或 Chrome");
  }

  elements.connectButton.disabled = true;
  elements.connectionStatus.textContent = "正在连接";
  elements.connectionStatus.className = "status status-working";
  appendLog("打开附近蓝牙设备列表");

  bluetoothDevice = await navigator.bluetooth.requestDevice({
    acceptAllDevices: true,
    optionalServices: [UART_SERVICE_UUID],
  });
  bluetoothDevice.addEventListener("gattserverdisconnected", handleDisconnected);

  const server = await bluetoothDevice.gatt.connect();
  let service;
  try {
    service = await server.getPrimaryService(UART_SERVICE_UUID);
  } catch (error) {
    throw new Error("所选设备没有 FFE0 蓝牙串口服务");
  }
  notifyCharacteristic = await service.getCharacteristic(UART_NOTIFY_UUID);
  await notifyCharacteristic.startNotifications();
  notifyCharacteristic.addEventListener("characteristicvaluechanged", handleNotification);

  // 与 diansai_test 的 vehicle-console 相同：FFE1 可写时优先使用，否则使用 FFE2。
  if (notifyCharacteristic.properties.writeWithoutResponse ||
      notifyCharacteristic.properties.write) {
    writeCharacteristic = notifyCharacteristic;
  } else {
    writeCharacteristic = await service.getCharacteristic(UART_WRITE_UUID);
  }
  writeMode = writeCharacteristic.properties.write &&
    typeof writeCharacteristic.writeValueWithResponse === "function"
    ? "writeWithResponse"
    : "writeWithoutResponse";

  elements.deviceLabel.textContent = bluetoothDevice.name || "未命名蓝牙设备";
  setConnected(true);
  const writeUuid = writeCharacteristic.uuid.includes("ffe1") ? "FFE1" : "FFE2";
  appendLog(
    `连接成功  ${writeUuid} ${writeMode} ` +
    `[write=${writeCharacteristic.properties.write}, ` +
    `wwr=${writeCharacteristic.properties.writeWithoutResponse}] / FFE1 通知`
  );
}

function disconnectBluetooth() {
  if (bluetoothDevice?.gatt?.connected) {
    bluetoothDevice.gatt.disconnect();
  } else {
    handleDisconnected();
  }
}

function handleDisconnected() {
  writeCharacteristic = null;
  notifyCharacteristic = null;
  writeMode = "";
  receiveBuffer = "";
  setConnected(false);
  appendLog("蓝牙连接已断开");
}

function handleNotification(event) {
  receiveBuffer += textDecoder.decode(event.target.value, { stream: true });
  let lineEnd = receiveBuffer.indexOf("\n");
  while (lineEnd >= 0) {
    const line = receiveBuffer.slice(0, lineEnd).replace(/\r$/, "");
    receiveBuffer = receiveBuffer.slice(lineEnd + 1);
    processLine(line);
    lineEnd = receiveBuffer.indexOf("\n");
  }
}

function processLine(line) {
  const parts = line.split(",");
  if (parts.length >= 6 && parts[0] === "STAT") {
    const values = parts.slice(1).map(Number);
    if (values.slice(0, 5).every(Number.isFinite)) {
      elements.actual1.textContent = `${(values[0] / 10).toFixed(1)}°`;
      elements.actual2.textContent = `${(values[1] / 10).toFixed(1)}°`;
      elements.target1.textContent = `${(values[2] / 10).toFixed(1)}°`;
      elements.target2.textContent = `${(values[3] / 10).toFixed(1)}°`;
      const state = values[4];
      elements.controllerState.textContent = `${String(state).padStart(2, "0")}  ${stateNames[state] || "未知"}`;
      if (values.length >= 10 && values.slice(5, 10).every(Number.isFinite)) {
        const last = values[6].toString(16).toUpperCase().padStart(2, "0");
        const rxState = values[8].toString(16).toUpperCase().padStart(2, "0");
        const statusRegister = values[9].toString(16).toUpperCase().padStart(4, "0");
        elements.bluetoothDebug.textContent =
          `BT ${values[5]}　LAST ${last}　ERR ${values[7]}　RX ${rxState}　SR ${statusRegister}`;
      }
      return;
    }
  }

  appendLog(`接收  ${line}`);
  if (line.startsWith("OK,SET,")) {
    elements.commandState.textContent = "单片机已接收命令";
  } else if (line.startsWith("ERR,")) {
    elements.commandState.textContent = "命令被拒绝";
  }
}

function readAngles() {
  const angle1 = Number(elements.angle1.value);
  const angle2 = Number(elements.angle2.value);
  if (!Number.isFinite(angle1) || !Number.isFinite(angle2)) {
    throw new Error("请输入有效角度");
  }
  if (angle1 < -360 || angle1 > 360 || angle2 < -360 || angle2 > 360) {
    throw new Error("角度范围必须在 -360.0° 到 360.0° 之间");
  }
  return [angle1, angle2];
}

async function sendTargets() {
  const [angle1, angle2] = readAngles();
  const command = `SET,${Math.round(angle1 * 10)},${Math.round(angle2 * 10)}\r\n`;
  await sendCommand(command);
}

async function sendCommand(command) {
  if (!writeCharacteristic) {
    throw new Error("请先连接蓝牙设备");
  }

  elements.commandState.textContent = "正在发送";
  appendLog(`发送  ${command.trim()}`);
  const data = textEncoder.encode(command);
  if (writeMode === "writeWithResponse") {
    await writeCharacteristic.writeValueWithResponse(data);
  } else if (typeof writeCharacteristic.writeValueWithoutResponse === "function") {
    await writeCharacteristic.writeValueWithoutResponse(data);
  } else {
    await writeCharacteristic.writeValue(data);
  }
  elements.commandState.textContent = "BLE 已写入，等待单片机应答";
  appendLog(`BLE 已写入(${writeMode})  ${command.trim()}`);
}

async function runAction(action) {
  try {
    await action();
  } catch (error) {
    elements.connectButton.disabled = false;
    elements.connectionStatus.textContent = bluetoothDevice?.gatt?.connected ? "已连接" : "连接失败";
    elements.connectionStatus.className = bluetoothDevice?.gatt?.connected
      ? "status status-online"
      : "status status-offline";
    elements.commandState.textContent = "操作失败";
    appendLog(`错误  ${error.message}`);
  }
}

elements.connectButton.addEventListener("click", () => {
  if (bluetoothDevice?.gatt?.connected) {
    disconnectBluetooth();
  } else {
    runAction(connectBluetooth);
  }
});

elements.sendButton.addEventListener("click", () => runAction(sendTargets));

elements.plusButton.addEventListener("click", () => {
  runAction(async () => {
    const [angle1, angle2] = readAngles();
    if (angle1 + 30 > 360 || angle2 + 30 > 360) {
      throw new Error("目标加 30° 后超过 360.0°");
    }
    elements.angle1.value = (angle1 + 30).toFixed(1);
    elements.angle2.value = (angle2 + 30).toFixed(1);
    await sendTargets();
  });
});

elements.zeroButton.addEventListener("click", () => {
  elements.angle1.value = "0.0";
  elements.angle2.value = "0.0";
  runAction(sendTargets);
});

elements.clearLogButton.addEventListener("click", () => {
  elements.communicationLog.textContent = "";
});

setConnected(false);
appendLog(`网页控制台已启动  ${APP_VERSION}`);
