const state = {
  token: null,
  step: 10,
  activeKeys: new Set(),
  repeatTimer: null,
  sessionReady: false,
  autoEnabled: false,
  resumeAutoOnReconnect: false,
  reconnectTimer: null,
  reconnecting: false,
  heartbeatInFlight: false,
  statusInFlight: false,
};

const $ = (selector) => document.querySelector(selector);

function setConnection(mode, text) {
  const element = $("#connection");
  element.classList.remove("online", "offline");
  if (mode) element.classList.add(mode);
  $("#connection-text").textContent = text;
}

function setMessage(text) {
  $("#message").textContent = text;
}

async function request(path, options = {}, timeoutMs = 2500) {
  const headers = { "Content-Type": "application/json", ...(options.headers || {}) };
  if (state.token) headers["X-Servo-Session"] = state.token;
  const controller = new AbortController();
  const timeout = window.setTimeout(() => controller.abort(), timeoutMs);
  try {
    const response = await fetch(path, {
      cache: "no-store",
      ...options,
      headers,
      signal: controller.signal,
    });
    const payload = await response.json().catch(() => ({}));
    if (!response.ok) throw new Error(payload.error || `HTTP ${response.status}`);
    return payload;
  } catch (error) {
    if (error.name === "AbortError") throw new Error("请求超时");
    throw error;
  } finally {
    window.clearTimeout(timeout);
  }
}

async function startSession(allowResume = true) {
  if (state.reconnecting) return;
  state.reconnecting = true;
  if (state.reconnectTimer) {
    window.clearTimeout(state.reconnectTimer);
    state.reconnectTimer = null;
  }
  const shouldResume = allowResume && state.resumeAutoOnReconnect;
  state.sessionReady = false;
  state.token = null;
  const payload = await request("/api/session", { method: "POST", body: "{}" });
  state.token = payload.token;
  state.sessionReady = true;
  setConnection("online", "已连接");
  setMessage("控制就绪");
  render(payload.status);
  if (shouldResume) {
    await request("/api/tracking", {
      method: "POST",
      body: JSON.stringify({ enabled: true }),
    });
    state.autoEnabled = true;
    setMessage("已重连，人脸锁定已恢复");
  }
  state.resumeAutoOnReconnect = false;
  state.reconnecting = false;
}

function markDisconnected(message) {
  if (state.sessionReady) state.resumeAutoOnReconnect = state.autoEnabled;
  state.sessionReady = false;
  state.token = null;
  setConnection("offline", "连接中断");
  setMessage(message);
  scheduleReconnect();
}

function scheduleReconnect() {
  if (state.reconnectTimer || state.reconnecting) return;
  state.reconnectTimer = window.setTimeout(async () => {
    state.reconnectTimer = null;
    try {
      await startSession(true);
    } catch (error) {
      state.reconnecting = false;
      setConnection("offline", "重连中");
      setMessage(error.message);
      scheduleReconnect();
    }
  }, 1000);
}

async function heartbeat() {
  if (!state.sessionReady || state.heartbeatInFlight) return;
  state.heartbeatInFlight = true;
  try {
    await request("/api/heartbeat", { method: "POST", body: "{}" });
    setConnection("online", "已连接");
  } catch (error) {
    markDisconnected(error.message);
  } finally {
    state.heartbeatInFlight = false;
  }
}

function render(data) {
  if (!data || !data.axes) return;
  for (const axisName of ["pan", "tilt"]) {
    const axis = data.axes[axisName];
    if (!axis) continue;
    const angle = Number(axis.angle_deg || 0);
    const pulse = Number(axis.pulse_us || 1500);
    $(`#${axisName}-angle`).textContent = `${angle.toFixed(1)}°`;
    $(`#${axisName}-pulse`).textContent = `${pulse} us`;
    const slider = $(`#${axisName}-slider`);
    slider.min = String(axis.min_us);
    slider.max = String(axis.max_us);
    slider.value = String(pulse);
    $(`#${axisName}-marker`).style.left = `${((pulse - axis.min_us) / (axis.max_us - axis.min_us)) * 100}%`;
    const status = $(`#${axisName}-state`);
    status.textContent = axis.enabled ? "输出中" : "已停止";
    status.classList.toggle("active", Boolean(axis.enabled));
  }
  $("#pwm-status").textContent = data.pwm_running ? "输出中" : "已停止";
  $("#frequency-status").textContent = `${data.period_hz} Hz`;
  $("#last-command").textContent = data.last_command || "-";
  const vision = data.vision;
  if (vision) {
    const statusText = {
      starting: "启动中",
      face: "检测到人脸",
      no_face: "未检测到",
      error: "视觉错误",
    }[vision.status] || vision.status || "未知";
    $("#vision-status").textContent = statusText;
    $("#vision-fps").textContent = Number(vision.fps || 0).toFixed(1);
    $("#vision-horizontal").textContent = vision.offset_px ? vision.offset_px[0] : "-";
    $("#vision-vertical").textContent = vision.offset_px ? vision.offset_px[1] : "-";
    $("#vision-confidence").textContent = vision.confidence
      ? Number(vision.confidence).toFixed(2)
      : "-";
    renderTrackingConfig(vision.tracking_config);
    state.autoEnabled = Boolean(vision.auto_enabled);
    const autoButton = $("#auto-button");
    autoButton.classList.toggle("active", state.autoEnabled);
    autoButton.innerHTML = state.autoEnabled
      ? '<span aria-hidden="true">●</span> 跟踪中'
      : '<span aria-hidden="true">◎</span> 自动跟踪';
  }
}

async function refreshStatus() {
  if (state.statusInFlight) return;
  state.statusInFlight = true;
  try {
    const data = await request("/api/status");
    render(data);
    if (data.status === "waiting" && state.sessionReady) {
      markDisconnected("会话已断开");
    }
  } catch (error) {
    if (state.sessionReady) markDisconnected(error.message);
  } finally {
    state.statusInFlight = false;
  }
}

async function sendCommand(axis, deltaUs) {
  if (!state.sessionReady) return;
  try {
    await request("/api/command", {
      method: "POST",
      body: JSON.stringify({ axis, delta_us: deltaUs }),
    });
    setMessage(`${axis === "pan" ? "水平" : "垂直"} ${deltaUs > 0 ? "+" : ""}${deltaUs} us`);
  } catch (error) {
    setMessage(error.message);
  }
}

async function setPulse(axis, pulseUs) {
  if (!state.sessionReady) return;
  try {
    await request("/api/command", {
      method: "POST",
      body: JSON.stringify({ axis, pulse_us: Number(pulseUs) }),
    });
  } catch (error) {
    setMessage(error.message);
  }
}

async function center() {
  if (!state.sessionReady) return;
  try {
    await request("/api/center", { method: "POST", body: "{}" });
    setMessage("两个舵机已回中");
  } catch (error) {
    setMessage(error.message);
  }
}

async function stop() {
  if (!state.sessionReady) return;
  try {
    await request("/api/stop", { method: "POST", body: "{}" });
    setMessage("PWM 已停止");
  } catch (error) {
    setMessage(error.message);
  }
}

async function toggleAuto() {
  if (!state.sessionReady) return;
  const enabled = !state.autoEnabled;
  try {
    await request("/api/tracking", {
      method: "POST",
      body: JSON.stringify({ enabled }),
    });
    state.autoEnabled = enabled;
    setMessage(enabled ? "人脸锁定已开启" : "人脸锁定已关闭");
  } catch (error) {
    setMessage(error.message);
  }
}

const tuningDefaults = {
  control_mode: "pid",
  deadband_px: 5,
  max_step_us: 50,
  return_step_us: 5,
  kp: 500,
  ki: 0,
  kd: 200,
  integral_limit: 0.5,
  derivative_alpha: 0.35,
  smooth_alpha: 0.35,
  detect_every: 1,
  max_fps: 30,
};

const tuningFields = {
  control_mode: "#tuning-control-mode",
  deadband_px: "#tuning-deadband",
  max_step_us: "#tuning-max-step",
  return_step_us: "#tuning-return-step",
  kp: "#tuning-kp",
  ki: "#tuning-ki",
  kd: "#tuning-kd",
  integral_limit: "#tuning-integral-limit",
  derivative_alpha: "#tuning-derivative-alpha",
  smooth_alpha: "#tuning-smooth-alpha",
  detect_every: "#tuning-detect-every",
  max_fps: "#tuning-max-fps",
};

function renderTrackingConfig(config) {
  if (!config) return;
  for (const [key, selector] of Object.entries(tuningFields)) {
    const input = $(selector);
    if (document.activeElement !== input && config[key] !== undefined) {
      input.value = String(config[key]);
    }
  }
}

function readTrackingConfig() {
  return Object.fromEntries(
    Object.entries(tuningFields).map(([key, selector]) => [
      key,
      key === "control_mode" ? $(selector).value : Number($(selector).value),
    ]),
  );
}

async function applyTrackingConfig(config) {
  if (!state.sessionReady) return;
  try {
    const payload = await request("/api/tracking/config", {
      method: "POST",
      body: JSON.stringify(config),
    });
    renderTrackingConfig(payload.tracking_config);
    setMessage("跟踪参数已应用");
  } catch (error) {
    setMessage(error.message);
  }
}

$("#tuning-apply").addEventListener("click", () => {
  applyTrackingConfig(readTrackingConfig());
});

$("#tuning-reset").addEventListener("click", () => {
  renderTrackingConfig(tuningDefaults);
  applyTrackingConfig(tuningDefaults);
});

function startRepeating(axis, direction) {
  sendCommand(axis, direction * state.step);
  if (state.repeatTimer) clearInterval(state.repeatTimer);
  state.repeatTimer = setInterval(() => {
    sendCommand(axis, direction * state.step);
  }, 90);
}

function stopRepeating() {
  if (state.repeatTimer) {
    clearInterval(state.repeatTimer);
    state.repeatTimer = null;
  }
}

function bindHoldButton(button) {
  const axis = button.dataset.axis;
  const direction = Number(button.dataset.direction);
  button.addEventListener("pointerdown", (event) => {
    event.preventDefault();
    button.setPointerCapture(event.pointerId);
    startRepeating(axis, direction);
  });
  ["pointerup", "pointercancel", "pointerleave"].forEach((eventName) => {
    button.addEventListener(eventName, stopRepeating);
  });
}

function handleKeyChange() {
  if (state.activeKeys.has("ArrowLeft")) startRepeating("pan", -1);
  else if (state.activeKeys.has("ArrowRight")) startRepeating("pan", 1);
  else if (state.activeKeys.has("ArrowUp")) startRepeating("tilt", 1);
  else if (state.activeKeys.has("ArrowDown")) startRepeating("tilt", -1);
  else stopRepeating();
}

document.querySelectorAll(".nudge-button").forEach(bindHoldButton);

document.querySelectorAll("[data-step]").forEach((button) => {
  button.addEventListener("click", () => {
    state.step = Number(button.dataset.step);
    document.querySelectorAll("[data-step]").forEach((item) => {
      item.classList.toggle("selected", item === button);
    });
  });
});

document.querySelectorAll(".axis-center").forEach((button) => {
  button.addEventListener("click", async () => {
    const axis = button.dataset.axis;
    await setPulse(axis, 1500);
  });
});

document.querySelectorAll(".pulse-slider").forEach((slider) => {
  slider.addEventListener("input", () => {
    setPulse(slider.id.replace("-slider", ""), slider.value);
  });
});

$("#center-button").addEventListener("click", center);
$("#stop-button").addEventListener("click", stop);
$("#auto-button").addEventListener("click", toggleAuto);

window.addEventListener("keydown", (event) => {
  if (["ArrowLeft", "ArrowRight", "ArrowUp", "ArrowDown"].includes(event.key)) {
    event.preventDefault();
    state.activeKeys.add(event.key);
    handleKeyChange();
  } else if (event.key.toLowerCase() === "r") {
    event.preventDefault();
    center();
  } else if (event.code === "Space") {
    event.preventDefault();
    stop();
  }
});

window.addEventListener("keyup", (event) => {
  if (state.activeKeys.delete(event.key)) handleKeyChange();
});

window.addEventListener("blur", () => {
  state.activeKeys.clear();
  stopRepeating();
});

document.addEventListener("visibilitychange", () => {
  if (document.hidden) {
    state.activeKeys.clear();
    stopRepeating();
  }
});

startSession(false).catch((error) => {
  state.reconnecting = false;
  setConnection("offline", "连接失败");
  setMessage(error.message);
  scheduleReconnect();
});

setInterval(heartbeat, 700);
setInterval(refreshStatus, 500);

const visionFrame = $("#vision-frame");
const videoFrame = document.querySelector(".video-frame");
visionFrame.addEventListener("load", () => videoFrame.classList.add("has-frame"));
visionFrame.addEventListener("error", () => videoFrame.classList.remove("has-frame"));
setInterval(() => {
  visionFrame.src = `/vision/frame.jpg?t=${Date.now()}`;
}, 200);
