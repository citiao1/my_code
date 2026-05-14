<template>
  <div class="content">
    <div class="header">
      <div class="title-box">
        <h2 class="title">智 慧 家</h2>
        <text class="subtitle">Smart Home Control</text>
      </div>
      <div class="status-bar">
        <div class="status-dot" :class="connectionStateClass"></div>
        <text class="status-text">{{ connectionStatusText }}</text>
        <div class="refresh-icon" @click="manualQuery" :class="{ spinning: !isConnected }">🔄</div>
      </div>
    </div>

    <div class="container">
      
      <div class="col-1">
        <div class="card glass-card">
          <div class="card-header"><text class="card-title">📊 环境状态</text></div>
          <div class="dashboard-grid">
            <div class="stat-item">
              <div class="mini-icon temp">🌡️</div>
              <div class="mini-val">{{ env.temp }}<text class="unit">℃</text></div>
            </div>
            <div class="stat-item">
              <div class="mini-icon hum">💧</div>
              <div class="mini-val">{{ env.hum }}<text class="unit">%</text></div>
            </div>
            <div class="stat-item" :class="{ 'on': light.isOn }">
              <div class="mini-icon light">💡</div>
              <div class="mini-val">{{ light.isOn ? 'ON' : 'OFF' }}</div>
            </div>
            <div class="stat-item" :class="{ 'open': door.isOpen }">
              <div class="mini-icon door">🔒</div>
              <div class="mini-val" style="font-size: 16px;">{{ door.isOpen ? '开启' : '关闭' }}</div>
            </div>
          </div>
        </div>

        <div class="card glass-card">
          <div class="card-header"><text class="card-title">🎛️ 快速控制</text></div>
          <div class="control-row">
            <button class="btn btn-yellow" @click="sendCmd('ON')">💡 开灯</button>
            <button class="btn btn-gray" @click="sendCmd('OFF')">🌑 关灯</button>
          </div>
          <div class="slider-box">
            <text class="label">亮度 {{ light.val }}%</text>
            <slider :value="light.val" @change="onSliderChange" min="0" max="100" active-color="#ffd700" block-size="20"/>
          </div>
          <div class="control-row" style="margin-top: 15px;">
            <button class="btn btn-green" @click="sendDoorCmd('door open')">🔓 开门</button>
            <button class="btn btn-red" @click="sendDoorCmd('door close')">🔒 关门</button>
          </div>
        </div>

        <div class="card glass-card">
          <div class="card-header"><text class="card-title">📝 系统日志</text></div>
          <div class="log-box">
            <div v-for="(log, i) in logs" :key="i" class="log-line">
              <text class="log-time">[{{ log.time }}]</text> {{ log.msg }}
            </div>
          </div>
        </div>
      </div>

      <div class="col-2">
        <div class="card glass-card main-cam-card">
          <div class="card-header">
            <text class="card-title">📸 本地门禁 (AI)</text>
            <div class="tag" :class="{ active: isCameraOn }">{{ isCameraOn ? '运行中' : '已停止' }}</div>
          </div>
          <div class="video-box" id="local-video-container">
            <div class="overlay" v-if="!isCameraOn">
              <text class="icon">📷</text>
              <text>点击下方启动摄像头</text>
            </div>
            <div class="overlay loading" v-if="isModelLoading && isCameraOn">
              <text>⌛ AI 模型加载中...</text>
            </div>
            </div>
          
          <div class="cam-controls">
            <button class="main-btn" @click="toggleCamera" :class="{ active: isCameraOn }">
              {{ isCameraOn ? '关闭摄像头' : '① 启动识别系统' }}
            </button>
            <div v-if="isCameraOn && !isModelLoading" class="sub-btns fade-in">
              <button class="sub-btn" @click="triggerRegister">② 录入人脸</button>
              <button class="sub-btn action" :class="{ stop: isRecognizing }" @click="triggerRecognize">
                {{ isRecognizing ? '⏹ 停止识别' : '③ 开始刷脸开门' }}
              </button>
            </div>
          </div>
          <div class="tips" v-if="faceStatus">{{ faceStatus }}</div>
        </div>
      </div>

      <div class="col-3">
        <div class="card glass-card">
          <div class="card-header">
            <text class="card-title">🎥 远程监控 (ESP32)</text>
          </div>
          
          <div class="ip-input-box">
            <input class="ip-input" type="text" v-model="camIP" placeholder="输入 ESP32 IP" />
          </div>

          <div class="video-box remote-box">
            <div class="overlay" v-if="!showRemote">
              <text>点击连接查看画面</text>
            </div>
            <image v-if="showRemote" :src="streamUrl" class="remote-img" mode="aspectFit"></image>
          </div>

          <button class="btn" :class="showRemote ? 'btn-red' : 'btn-blue'" style="width:100%; margin-top:10px;" @click="toggleRemote">
            {{ showRemote ? '断开连接' : '连接监控' }}
          </button>

          <div class="divider"></div>
          <div class="card-header" style="margin-bottom: 5px;"><text class="card-title">🕹️ 云台控制</text></div>
          
          <div id="joystick-zone" class="joystick-area">
            <text class="joystick-tip">拖动控制方向</text>
          </div>
          
          <button class="btn btn-gray" style="margin-top: 10px;" @click="sendCmd('Cam_center')">归位 (Reset)</button>
        </div>
      </div>

    </div>
    
    <div :prop="commandData" :change:prop="ai.receiveCommand" style="display:none;"></div>
  </div>
</template>

<script>
import Paho from '@/common/mqtt.js'

export default {
  data() {
    return {
      client: null,
      isConnected: false,
      isReconnecting: false,
      config: {
        host: 'broker.emqx.io', port: 8083, path: '/mqtt',
        subHome: 'China/Beijing/huayuan/302/home/status',
        subDoor: 'China/Beijing/huayuan/302/door/+',
        pubCmd: 'China/Beijing/huayuan/302/command',
        pubDoor: 'China/Beijing/huayuan/302/door/status'
      },
      env: { temp: '--', hum: '--' },
      light: { isOn: false, val: 0 },
      door: { isOpen: false, lastMsg: '' },
      logs: [],
      
      // 本地 AI 相关
      isCameraOn: false,
      isModelLoading: false,
      isRecognizing: false,
      faceStatus: '',
      
      // 远程 ESP32 相关
      camIP: '192.168.1.16', // 默认内置 IP
      showRemote: false,
      
      commandData: { type: 'init', timestamp: 0 }
    }
  },
  computed: {
    connectionStateClass() {
      if (this.isConnected) return 'online';
      if (this.isReconnecting) return 'reconnecting';
      return '';
    },
    connectionStatusText() {
      if (this.isConnected) return '在线';
      if (this.isReconnecting) return '重连中';
      return '离线';
    },
    streamUrl() {
      return this.camIP ? `http://${this.camIP}:81/stream` : '';
    }
  },
  onLoad() { setTimeout(() => this.connectMQTT(), 1000); },
  methods: {
    // --- 远程监控逻辑 ---
    toggleRemote() {
      if(!this.camIP) return uni.showToast({ title: '请输入IP', icon: 'none' });
      this.showRemote = !this.showRemote;
    },
    // 由 renderjs 调用，发送摇杆指令
    sendJoystickCmd(cmd) {
      this.sendCmd(cmd);
    },

    // --- 本地 AI 逻辑 ---
    toggleCamera() { this.commandData = { type: this.isCameraOn ? 'stopCam' : 'startCam', timestamp: Date.now() }; },
    triggerRegister() { this.commandData = { type: 'register', timestamp: Date.now() }; },
    triggerRecognize() { this.commandData = { type: this.isRecognizing ? 'stopRec' : 'startRec', timestamp: Date.now() }; },
    
    // 接收 renderjs 的状态更新
    onAiStatus(e) {
      if(e.camera !== undefined) this.isCameraOn = e.camera;
      if(e.loading !== undefined) this.isModelLoading = e.loading;
      if(e.recognizing !== undefined) this.isRecognizing = e.recognizing;
      if(e.msg) this.faceStatus = e.msg;
    },
    onFaceMatch() {
      uni.showToast({ title: '欢迎回家！', icon: 'success' });
      this.sendDoorCmd('door open');
      this.commandData = { type: 'stopRec', timestamp: Date.now() };
    },

    // --- MQTT 核心 ---
    connectMQTT() {
      if(this.isConnected) return;
      this.isReconnecting = true;
      let clientId = 'App_' + Math.random().toString(16).substr(2, 8);
      try { this.client = new Paho.MQTT.Client(this.config.host, this.config.port, this.config.path, clientId); } catch (e) { return; }

      this.client.onConnectionLost = (res) => { 
        this.isConnected = false; this.isReconnecting = true;
        setTimeout(() => this.connectMQTT(), 5000); 
      };
      this.client.onMessageArrived = (msg) => this.handleMessage(msg.destinationName, msg.payloadString);

      this.client.connect({
        useSSL: false, cleanSession: true, keepAliveInterval: 60,
        onSuccess: () => {
          this.isConnected = true; this.isReconnecting = false;
          this.client.subscribe(this.config.subHome);
          this.client.subscribe(this.config.subDoor);
          this.manualQuery();
        },
        onFailure: () => { this.isConnected = false; setTimeout(() => this.connectMQTT(), 3000); }
      });
    },
    handleMessage(topic, msg) {
      if (topic.includes('home/status')) {
        if (msg.includes('温度')) this.env.temp = this.extractNum(msg);
        if (msg.includes('湿度')) this.env.hum = this.extractNum(msg);
        if (msg.includes('灯光')) this.light.isOn = msg.includes('开启');
        if (msg.includes('亮度')) this.light.val = parseInt(this.extractNum(msg)) || 0;
        if (msg.includes('大门')) this.door.isOpen = msg.includes('开启');
      }
      if (topic.includes('door')) {
        this.door.lastMsg = msg;
        if (msg.includes('door open')) this.door.isOpen = true;
        if (msg.includes('door close')) this.door.isOpen = false;
        // 日志记录
        this.logs.unshift({ time: new Date().toLocaleTimeString(), msg: msg });
        if(this.logs.length > 20) this.logs.pop();
      }
    },
    sendCmd(cmd) { if(this.client && this.isConnected) { let m = new Paho.MQTT.Message(cmd); m.destinationName = this.config.pubCmd; this.client.send(m); } },
    sendDoorCmd(cmd) { if(this.client && this.isConnected) { let m = new Paho.MQTT.Message(cmd); m.destinationName = this.config.pubDoor; this.client.send(m); } },
    onSliderChange(e) { this.light.val = e.detail.value; this.sendCmd('light:'+this.light.val); },
    manualQuery() { this.sendCmd('get_status'); },
    extractNum(s) { return (s.match(/-?\d+(\.\d+)?/)||['--'])[0]; }
  }
}
</script>

<script module="ai" lang="renderjs">
export default {
  data() { 
    return { 
      videoEl: null, stream: null, faceMatcher: null, loopTimer: null, myDescriptor: null, isLoaded: false,
      joystickManager: null, joystickTimer: null
    } 
  },
  mounted() {
    // 页面加载后尝试初始化摇杆 (需延迟等待DOM)
    setTimeout(() => {
      this.loadNippleJS();
    }, 1000);
  },
  methods: {
    receiveCommand(n) {
      if (!n || !n.type) return;
      switch(n.type) {
        case 'startCam': this.startSequence(); break;
        case 'stopCam': this.stopCamera(); break;
        case 'register': this.registerFace(); break;
        case 'startRec': this.startRecognize(); break;
        case 'stopRec': this.stopRecognize(); break;
      }
    },
    
    // --- 摇杆逻辑 (Nipple.js) ---
    loadNippleJS() {
      if (window.nipplejs) { this.initJoystick(); return; }
      const script = document.createElement('script');
      script.src = 'https://cdnjs.cloudflare.com/ajax/libs/nipplejs/0.10.1/nipplejs.min.js';
      script.onload = () => this.initJoystick();
      document.head.appendChild(script);
    },
    initJoystick() {
      const zone = document.getElementById('joystick-zone');
      if(!zone || !window.nipplejs) return;
      
      this.joystickManager = window.nipplejs.create({
        zone: zone, mode: 'static', position: { left: '50%', top: '50%' }, color: '#007bff', size: 100
      });

      let currentCmd = '';
      
      this.joystickManager.on('move', (evt, data) => {
        if (data.direction) {
          const dir = data.direction.angle;
          let newCmd = '';
          if (dir === 'up') newCmd = 'CamY_up';
          if (dir === 'down') newCmd = 'CamY_down';
          if (dir === 'left') newCmd = 'CamX_down';
          if (dir === 'right') newCmd = 'CamX_up';

          if (newCmd && newCmd !== currentCmd) {
            if (this.joystickTimer) { clearInterval(this.joystickTimer); this.joystickTimer = null; }
            currentCmd = newCmd;
            // 发送指令给逻辑层
            this.$ownerInstance.callMethod('sendJoystickCmd', newCmd);
            // 开启连续发送定时器
            this.joystickTimer = setInterval(() => {
              this.$ownerInstance.callMethod('sendJoystickCmd', newCmd);
            }, 150); 
          }
        }
      });

      this.joystickManager.on('end', () => {
        if (this.joystickTimer) { clearInterval(this.joystickTimer); this.joystickTimer = null; }
        currentCmd = '';
      });
    },

    // --- AI 逻辑 (Face-api.js) ---
    async startSequence() {
      if (this.isLoaded) { this.startCamera(); return; }
      this.updateOwner({ loading: true, msg: '正在加载 AI...' });
      if (!window.faceapi) await this.loadFaceScript();
      await this.initAI();
      this.startCamera();
    },
    loadFaceScript() {
      return new Promise((resolve, reject) => {
        const script = document.createElement('script');
        script.src = 'https://cdn.jsdelivr.net/npm/face-api.js@0.22.2/dist/face-api.min.js';
        script.onload = resolve;
        script.onerror = () => { this.updateOwner({ loading: false, msg: '脚本加载失败' }); reject(); };
        document.head.appendChild(script);
      });
    },
    async initAI() {
      const faceapi = window.faceapi; 
      try {
        const modelUrl = 'https://cdn.jsdelivr.net/gh/justadudewhohacks/face-api.js/weights';
        await Promise.all([
          faceapi.nets.ssdMobilenetv1.loadFromUri(modelUrl),
          faceapi.nets.faceLandmark68Net.loadFromUri(modelUrl),
          faceapi.nets.faceRecognitionNet.loadFromUri(modelUrl)
        ]);
        this.isLoaded = true;
        this.updateOwner({ loading: false, msg: '✅ AI 就绪' });
      } catch (e) { this.updateOwner({ loading: false, msg: '模型失败' }); }
    },
    async startCamera() {
      const container = document.getElementById('local-video-container');
      if (!this.videoEl) {
        this.videoEl = document.createElement('video');
        this.videoEl.style.cssText = 'width:100%;height:100%;object-fit:cover;transform:scaleX(-1);border-radius:12px;';
        this.videoEl.autoplay = true; this.videoEl.muted = true; this.videoEl.setAttribute('playsinline', 'true');
        container.appendChild(this.videoEl);
      }
      try {
        this.stream = await navigator.mediaDevices.getUserMedia({ video: { facingMode: 'user' } });
        this.videoEl.srcObject = this.stream;
        this.updateOwner({ camera: true, msg: '📸 运行中' });
      } catch (e) { this.updateOwner({ msg: '无摄像头权限' }); }
    },
    stopCamera() {
      if (this.stream) { this.stream.getTracks().forEach(t => t.stop()); this.videoEl.srcObject = null; }
      this.updateOwner({ camera: false, recognizing: false, msg: '' }); this.stopRecognize();
    },
    async registerFace() {
      if (!this.videoEl || this.videoEl.paused) return;
      const faceapi = window.faceapi;
      const detection = await faceapi.detectSingleFace(this.videoEl).withFaceLandmarks().withFaceDescriptor();
      if (detection) {
        this.myDescriptor = detection.descriptor;
        this.faceMatcher = new faceapi.FaceMatcher(this.myDescriptor, 0.6);
        this.updateOwner({ msg: '✅ 已录入' });
      } else { this.updateOwner({ msg: '⚠️ 未检测到人脸' }); }
    },
    startRecognize() {
      if (!this.myDescriptor) return alert('请先录入!');
      this.updateOwner({ recognizing: true, msg: '👀 识别中...' });
      const faceapi = window.faceapi;
      if (this.loopTimer) clearInterval(this.loopTimer);
      this.loopTimer = setInterval(async () => {
        if (!this.videoEl || this.videoEl.paused) return;
        const detection = await faceapi.detectSingleFace(this.videoEl).withFaceLandmarks().withFaceDescriptor();
        if (detection) {
          const match = this.faceMatcher.findBestMatch(detection.descriptor);
          if (match.label !== 'unknown' && match.distance < 0.5) {
            if (this.$ownerInstance) this.$ownerInstance.callMethod('onFaceMatch');
            this.updateOwner({ msg: '🔓 通过' });
          }
        }
      }, 800);
    },
    stopRecognize() { if (this.loopTimer) clearInterval(this.loopTimer); this.updateOwner({ recognizing: false }); },
    updateOwner(statusObj) { if (this.$ownerInstance) this.$ownerInstance.callMethod('onAiStatus', statusObj); }
  }
}
</script>

<style>
/* --- 全局布局 --- */
.content {
  padding: 20px;
  background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
  min-height: 100vh;
  box-sizing: border-box;
}

/* 响应式网格布局 (模仿网页版) */
.container {
  display: flex;
  flex-direction: column; /* 手机默认单列 */
  gap: 20px;
}

/* 头部样式 */
.header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px; padding: 0 5px; }
.title-box { display: flex; flex-direction: column; }
.title { font-size: 24px; font-weight: 800; color: #2c3e50; margin-bottom: 2px; }
.subtitle { font-size: 12px; color: #7f8c8d; letter-spacing: 1px; }
.status-bar { display: flex; align-items: center; background: rgba(255,255,255,0.6); padding: 6px 12px; border-radius: 20px; backdrop-filter: blur(5px); }
.status-dot { width: 8px; height: 8px; border-radius: 50%; margin-right: 6px; background: #ccc; }
.status-dot.online { background: #2ecc71; box-shadow: 0 0 5px #2ecc71; }
.status-dot.reconnecting { background: #f1c40f; animation: pulse 1s infinite; }
.status-text { font-size: 12px; color: #555; font-weight: bold; margin-right: 8px; }
.refresh-icon.spinning { animation: spin 1s infinite linear; }

/* 通用卡片 */
.glass-card {
  background: rgba(255, 255, 255, 0.9);
  border-radius: 16px;
  padding: 20px;
  box-shadow: 0 8px 32px 0 rgba(31, 38, 135, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.5);
  margin-bottom: 0; /* 让 flex gap 处理间距 */
}
.card-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px; border-bottom: 2px solid #f0f0f0; padding-bottom: 10px; }
.card-title { font-size: 16px; font-weight: bold; color: #2c3e50; }

/* 第一列：状态网格 */
.dashboard-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 15px; }
.stat-item { background: #f8f9fa; padding: 12px; border-radius: 10px; text-align: center; border: 1px solid #eee; display: flex; flex-direction: column; align-items: center; justify-content: center; }
.mini-icon { font-size: 24px; margin-bottom: 5px; }
.mini-val { font-size: 18px; font-weight: bold; color: #333; }
.unit { font-size: 12px; color: #888; font-weight: normal; margin-left: 2px; }
.stat-item.on { background: #fff9db; border-color: #ffe066; }
.stat-item.open { background: #e3fafc; border-color: #99e9f2; }

/* 按钮通用 */
.btn { flex: 1; padding: 10px; border: none; border-radius: 8px; font-size: 14px; font-weight: bold; color: white; display: flex; align-items: center; justify-content: center; transition: 0.2s; }
.btn:active { transform: scale(0.98); }
.btn-yellow { background: #f39c12; color: white; }
.btn-gray { background: #95a5a6; color: white; }
.btn-green { background: #27ae60; color: white; }
.btn-red { background: #c0392b; color: white; }
.btn-blue { background: #007bff; color: white; }
.control-row { display: flex; gap: 10px; }

/* 摄像头卡片样式 */
.main-cam-card { border-top: 4px solid #28a745; }
.tag { font-size: 10px; padding: 2px 8px; border-radius: 10px; background: #eee; color: #999; }
.tag.active { background: #d4edda; color: #155724; }
.video-box { 
  width: 100%; height: 220px; background: #000; border-radius: 12px; 
  position: relative; overflow: hidden; display: flex; justify-content: center; align-items: center;
  margin-bottom: 15px;
}
.remote-img { width: 100%; height: 100%; }
.overlay { display: flex; flex-direction: column; align-items: center; color: rgba(255,255,255,0.7); gap: 10px; font-size: 14px; position: absolute; z-index: 10; }
.cam-controls { margin-top: 10px; }
.main-btn { background: #007bff; color: white; border-radius: 8px; height: 40px; font-size: 14px; font-weight: bold; width: 100%; display: flex; align-items: center; justify-content: center; }
.main-btn.active { background: #dc3545; }
.sub-btns { display: flex; gap: 10px; margin-top: 10px; }
.sub-btn { flex: 1; height: 35px; border-radius: 6px; background: #f1f2f6; color: #555; font-size: 12px; font-weight: bold; display: flex; align-items: center; justify-content: center; }
.sub-btn.action { background: #2ecc71; color: white; }
.sub-btn.action.stop { background: #dc3545; }

/* 摇杆区域 */
.joystick-area { width: 100%; height: 180px; background: #f8f9fa; border-radius: 12px; position: relative; border: 1px dashed #ddd; display: flex; justify-content: center; align-items: center; }
.joystick-tip { color: #ccc; font-size: 12px; pointer-events: none; }
.ip-input-box { margin-bottom: 10px; }
.ip-input { width: 100%; padding: 8px; background: #f1f1f1; border-radius: 6px; font-size: 12px; box-sizing: border-box; }
.divider { height: 1px; background: #eee; margin: 15px 0; }

/* 日志 */
.log-box { height: 120px; overflow-y: auto; background: #2c3e50; color: #7bed9f; padding: 10px; border-radius: 8px; font-family: monospace; font-size: 12px; }
.log-line { margin-bottom: 2px; }
.log-time { color: #aaa; margin-right: 5px; }

/* 动画 */
@keyframes spin { 100% { transform: rotate(360deg); } }
@keyframes pulse { 0% { opacity: 1; } 50% { opacity: 0.5; } 100% { opacity: 1; } }
@keyframes fadeIn { from { opacity: 0; transform: translateY(5px); } to { opacity: 1; transform: translateY(0); } }

/* 平板/桌面端大屏适配：三列布局 */
@media (min-width: 800px) {
  .container {
    flex-direction: row;
    align-items: flex-start;
  }
  .col-1 { width: 300px; flex-shrink: 0; }
  .col-2 { flex: 1; }
  .col-3 { width: 350px; flex-shrink: 0; }
}
</style>