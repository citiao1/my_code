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

    <div class="card glass-card">
      <div class="card-header">
        <text class="card-title">📷 智能门禁</text>
        <div class="door-status-tag" :class="{ open: door.isOpen }">
          {{ door.isOpen ? '🔓 已开启' : '🔒 已锁' }}
        </div>
      </div>
      
      <div id="video-container" class="video-box">
        <div class="overlay" v-if="!isCameraOn">
          <text class="icon">📸</text>
          <text>点击启动摄像头</text>
        </div>
        <div class="overlay loading" v-if="isModelLoading && isCameraOn">
          <text>⌛ AI 模型加载中...</text>
        </div>
      </div>

      <div class="cam-controls">
        <button class="main-btn" @click="toggleCamera" :class="{ active: isCameraOn }">
          {{ isCameraOn ? '关闭摄像头' : '启动识别系统' }}
        </button>
        
        <div v-if="isCameraOn && !isModelLoading" class="sub-btns fade-in">
          <button class="sub-btn" @click="triggerRegister">录入人脸</button>
          <button class="sub-btn action" :class="{ stop: isRecognizing }" @click="triggerRecognize">
            {{ isRecognizing ? '停止识别' : '开始开门' }}
          </button>
        </div>
      </div>
      <div class="tips" v-if="faceStatus">{{ faceStatus }}</div>
    </div>

    <div class="grid-row">
      <div class="mini-card glass-card">
        <div class="mini-icon temp">🌡️</div>
        <div class="mini-info">
          <text class="mini-val">{{ env.temp }}</text>
          <text class="mini-unit">℃ 温度</text>
        </div>
      </div>
      <div class="mini-card glass-card">
        <div class="mini-icon hum">💧</div>
        <div class="mini-info">
          <text class="mini-val">{{ env.hum }}</text>
          <text class="mini-unit">% 湿度</text>
        </div>
      </div>
    </div>

    <div class="card glass-card light-card" :class="{ 'light-on': light.isOn }">
      <div class="card-header">
        <div class="header-left">
          <text class="card-title">💡 智能灯光</text>
          <text class="light-status-text">{{ light.isOn ? '已开启' : '已关闭' }}</text>
        </div>
        <div class="light-bulb" :class="{ on: light.isOn }" :style="{ opacity: light.isOn ? (light.val/100 + 0.3) : 0.3 }">
          💡
        </div>
      </div>

      <div class="light-controls">
        <div class="switch-group">
          <div class="switch-btn" :class="{ active: light.isOn }" @click="sendCmd('ON')">ON</div>
          <div class="switch-btn" :class="{ active: !light.isOn }" @click="sendCmd('OFF')">OFF</div>
        </div>
        
        <div class="slider-container">
          <text class="slider-label">亮度 {{ light.val }}%</text>
          <slider 
            :value="light.val" 
            @change="onSliderChange" 
            min="0" max="100" 
            active-color="#ffd700" 
            backgroundColor="rgba(255,255,255,0.2)"
            block-size="20" 
            block-color="#fff"
          />
        </div>
      </div>
    </div>

    <div class="card glass-card door-control" :class="{ alert: door.isAlert }">
      <div class="card-header">
        <text class="card-title">🔒 远程控制</text>
      </div>
      <div class="door-btns">
        <button class="door-btn open" @click="sendDoorCmd('door open')">
          <text>🔓</text> 开门
        </button>
        <button class="door-btn close" @click="sendDoorCmd('door close')">
          <text>🔒</text> 关门
        </button>
      </div>
      <div class="msg-box" v-if="door.lastMsg">🔔 {{ door.lastMsg }}</div>
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
        host: 'broker.emqx.io', 
        port: 8083, 
        path: '/mqtt',
        subHome: 'China/Beijing/huayuan/302/home/status',
        subDoor: 'China/Beijing/huayuan/302/door/+',
        pubCmd: 'China/Beijing/huayuan/302/command',
        pubDoor: 'China/Beijing/huayuan/302/door/status'
      },
      env: { temp: '--', hum: '--' },
      light: { isOn: false, val: 0 },
      door: { isOpen: false, lastMsg: '', isAlert: false },
      
      isCameraOn: false,
      isModelLoading: false,
      isRecognizing: false,
      faceStatus: '',
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
    }
  },
  onLoad() { setTimeout(() => this.connectMQTT(), 1500); },
  onUnload() {
    if (this.client && this.isConnected) { try { this.client.disconnect(); } catch(e){} }
    this.commandData = { type: 'stopCam', timestamp: Date.now() };
  },
  methods: {
    toggleCamera() { this.commandData = { type: this.isCameraOn ? 'stopCam' : 'startCam', timestamp: Date.now() }; },
    triggerRegister() { this.commandData = { type: 'register', timestamp: Date.now() }; },
    triggerRecognize() { this.commandData = { type: this.isRecognizing ? 'stopRec' : 'startRec', timestamp: Date.now() }; },
    
    onAiStatus(e) {
      if(e.camera !== undefined) this.isCameraOn = e.camera;
      if(e.loading !== undefined) this.isModelLoading = e.loading;
      if(e.recognizing !== undefined) this.isRecognizing = e.recognizing;
      if(e.msg) this.faceStatus = e.msg;
    },
    
    onFaceMatch(e) {
      uni.showToast({ title: '欢迎回家！', icon: 'success' });
      this.sendDoorCmd('door open');
      this.commandData = { type: 'stopRec', timestamp: Date.now() };
    },

    // --- 调试版连接代码 ---
    connectMQTT() {
      if(this.isConnected) return;
      this.isReconnecting = true;
      
      let clientId = 'AppAI_' + Math.random().toString(16).substr(2, 8);
      try {
        this.client = new Paho.MQTT.Client(this.config.host, this.config.port, this.config.path, clientId);
      } catch (e) {
        return;
      }

      this.client.onConnectionLost = (res) => { 
        this.isConnected = false; 
        this.isReconnecting = true;
        console.log('连接断开:', res.errorMessage);
        // 5秒后自动重连
        setTimeout(() => this.connectMQTT(), 5000); 
      };
      
      this.client.onMessageArrived = (msg) => this.handleMessage(msg.destinationName, msg.payloadString);

      this.client.connect({
        useSSL: false, 
        cleanSession: true,
        keepAliveInterval: 60,
        onSuccess: () => {
          console.log('✅ MQTT 连接成功!');
          // 🟢 核心修改：删除了 uni.showToast，这样重连时就不会弹窗了
          this.isConnected = true;
          this.isReconnecting = false;
          
          this.client.subscribe(this.config.subHome);
          this.client.subscribe(this.config.subDoor);
          
          this.manualQuery();
        },
        onFailure: (e) => {
          this.isConnected = false;
          setTimeout(() => { if(!this.isConnected) this.connectMQTT() }, 3000);
        }
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
        if (msg.includes('客人') || msg.includes('错误')) {
           this.door.isAlert = true; 
           setTimeout(()=>this.door.isAlert=false, 3000);
           uni.vibrateLong();
        }
      }
    },

    // --- 修复发送指令的 Bug ---
    sendCmd(cmd) { 
        if(this.client && this.isConnected) {
            try {
                // 1. 创建消息对象
                let message = new Paho.MQTT.Message(cmd);
                // 2. 设置目标主题
                message.destinationName = this.config.pubCmd;
                // 3. 发送对象 (不能写成一行，否则返回的是字符串)
                this.client.send(message);
                console.log('发送成功:', cmd);
            } catch (e) { console.error('发送失败:', e); }
        }
    },

    sendDoorCmd(cmd) { 
        if(this.client && this.isConnected) {
            try {
                let message = new Paho.MQTT.Message(cmd);
                message.destinationName = this.config.pubDoor;
                this.client.send(message);
                console.log('门禁指令发送:', cmd);
            } catch (e) { console.error('发送失败:', e); }
        }
    },
    onSliderChange(e) { this.light.val = e.detail.value; this.sendCmd('light:'+this.light.val); },
    manualQuery() { this.sendCmd('get_status'); },
    extractNum(s) { return (s.match(/-?\d+(\.\d+)?/)||['--'])[0]; }
  }
}
</script>

<script module="ai" lang="renderjs">
export default {
  data() { return { videoEl: null, stream: null, faceMatcher: null, loopTimer: null, myDescriptor: null, isLoaded: false } },
  mounted() { }, beforeDestroy() { this.stopCamera(); },
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
    async startSequence() {
      if (this.isLoaded) { this.startCamera(); return; }
      this.updateOwner({ loading: true, msg: '正在加载 AI...' });
      if (!window.faceapi) await this.loadScript();
      await this.initAI();
      this.startCamera();
    },
    loadScript() {
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
      const container = document.getElementById('video-container');
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
/* 页面背景 - 现代灰蓝渐变 */
.content {
  padding: 20px;
  background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
  min-height: 100vh;
  box-sizing: border-box;
}

/* 顶部栏 */
.header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 25px;
  padding: 0 5px;
}
.title-box { display: flex; flex-direction: column; }
.title { font-size: 24px; font-weight: 800; color: #2c3e50; margin-bottom: 2px; }
.subtitle { font-size: 12px; color: #7f8c8d; letter-spacing: 1px; }

.status-bar {
  display: flex;
  align-items: center;
  background: rgba(255,255,255,0.6);
  padding: 6px 12px;
  border-radius: 20px;
  backdrop-filter: blur(5px);
}
.status-dot { width: 8px; height: 8px; border-radius: 50%; margin-right: 6px; background: #ccc; }
.status-dot.online { background: #2ecc71; box-shadow: 0 0 5px #2ecc71; }
.status-dot.reconnecting { background: #f1c40f; animation: pulse 1s infinite; }
.status-text { font-size: 12px; color: #555; font-weight: bold; margin-right: 8px; }
.refresh-icon { font-size: 14px; color: #555; transition: transform 0.5s; }
.refresh-icon.spinning { animation: spin 1s infinite linear; }

/* 玻璃拟态通用卡片 */
.glass-card {
  background: rgba(255, 255, 255, 0.85);
  border-radius: 16px;
  padding: 20px;
  margin-bottom: 20px;
  box-shadow: 0 8px 32px 0 rgba(31, 38, 135, 0.1);
  backdrop-filter: blur(8px);
  border: 1px solid rgba(255, 255, 255, 0.18);
  transition: transform 0.2s;
}
.glass-card:active { transform: scale(0.995); }
.card-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px; }
.card-title { font-size: 16px; font-weight: bold; color: #2c3e50; }

/* 摄像头卡片 */
.video-box { 
  width: 100%; height: 220px; background: #222; border-radius: 12px; 
  position: relative; overflow: hidden; display: flex; justify-content: center; align-items: center;
  box-shadow: inset 0 0 20px rgba(0,0,0,0.5);
}
.overlay { display: flex; flex-direction: column; align-items: center; color: rgba(255,255,255,0.7); gap: 10px; font-size: 14px; }
.icon { font-size: 30px; margin-bottom: 5px; }
.cam-controls { margin-top: 20px; }
.main-btn {
  background: linear-gradient(90deg, #4facfe 0%, #00f2fe 100%);
  color: white; border: none; border-radius: 12px; height: 44px; font-size: 16px; font-weight: bold;
  box-shadow: 0 4px 15px rgba(79, 172, 254, 0.3); transition: 0.3s;
}
.main-btn.active { background: #ff6b6b; box-shadow: 0 4px 15px rgba(255, 107, 107, 0.3); }
.sub-btns { display: flex; gap: 12px; margin-top: 15px; }
.sub-btn { flex: 1; height: 38px; border-radius: 10px; background: #f1f2f6; color: #555; font-size: 14px; font-weight: bold; border: none; }
.sub-btn.action { background: #2ecc71; color: white; }
.sub-btn.action.stop { background: #e74c3c; }
.fade-in { animation: fadeIn 0.5s ease; }

/* 环境小卡片 */
.grid-row { display: flex; gap: 15px; margin-bottom: 20px; }
.mini-card { flex: 1; display: flex; align-items: center; padding: 15px; margin-bottom: 0; }
.mini-icon { font-size: 24px; margin-right: 12px; width: 40px; height: 40px; border-radius: 50%; display: flex; justify-content: center; align-items: center; }
.mini-icon.temp { background: rgba(255, 159, 67, 0.1); color: #ff9f43; }
.mini-icon.hum { background: rgba(84, 160, 255, 0.1); color: #54a0ff; }
.mini-val { font-size: 22px; font-weight: 800; color: #333; }
.mini-unit { font-size: 12px; color: #999; margin-left: 5px; }

/* 灯光卡片优化 */
.light-card { position: relative; overflow: hidden; }
.light-card.light-on { background: rgba(255, 255, 255, 0.95); border: 2px solid #f1c40f; }
.light-bulb { font-size: 28px; filter: grayscale(100%); transition: all 0.5s; }
.light-bulb.on { filter: grayscale(0%); text-shadow: 0 0 20px #f1c40f; transform: scale(1.1); }
.light-status-text { font-size: 12px; color: #999; margin-left: 8px; }

.switch-group { display: flex; background: #f1f2f6; border-radius: 10px; padding: 4px; margin-bottom: 20px; }
.switch-btn { 
  flex: 1; text-align: center; padding: 10px 0; font-size: 14px; font-weight: bold; color: #7f8c8d; border-radius: 8px; transition: 0.3s;
}
.switch-btn.active { background: white; color: #333; box-shadow: 0 2px 8px rgba(0,0,0,0.1); }

.slider-container { background: linear-gradient(90deg, #333, #666); padding: 15px; border-radius: 12px; color: white; }
.slider-label { font-size: 12px; margin-bottom: 5px; display: block; opacity: 0.8; }

/* 门禁卡片 */
.door-status-tag { 
  font-size: 12px; padding: 4px 10px; border-radius: 20px; background: #ff6b6b; color: white; font-weight: bold; 
}
.door-status-tag.open { background: #2ecc71; }
.door-btns { display: flex; gap: 15px; }
.door-btn { 
  flex: 1; height: 50px; border-radius: 12px; border: none; font-size: 16px; font-weight: bold; color: white;
  display: flex; align-items: center; justify-content: center; gap: 8px;
}
.door-btn.open { background: linear-gradient(135deg, #2ecc71, #26de81); box-shadow: 0 4px 15px rgba(46, 204, 113, 0.3); }
.door-btn.close { background: linear-gradient(135deg, #ff6b6b, #ee5253); box-shadow: 0 4px 15px rgba(255, 107, 107, 0.3); }

/* 动画 */
@keyframes spin { 100% { transform: rotate(360deg); } }
@keyframes pulse { 0% { opacity: 1; } 50% { opacity: 0.5; } 100% { opacity: 1; } }
@keyframes fadeIn { from { opacity: 0; transform: translateY(10px); } to { opacity: 1; transform: translateY(0); } }

.msg-box { background: #fff3cd; color: #856404; padding: 10px; border-radius: 8px; margin-top: 15px; font-size: 13px; text-align: center; border: 1px solid #ffeeba; }
.door-control.alert { animation: pulse 0.5s infinite; border: 2px solid #ff4757; }
</style>