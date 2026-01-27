<template>
  <div class="content">
    <div class="header">
      <h2 class="title">🏠 智能家居 App</h2>
      <div class="status-bar">
        <span class="badge" :class="{ online: isConnected }">
          {{ isConnected ? '☁️ 已连接' : '❌ 未连接' }}
        </span>
        <button class="refresh-btn" @click="manualQuery" :disabled="!isConnected">
          🔄 刷新
        </button>
      </div>
    </div>

    <div class="card env-card">
      <div class="card-title">🌡️ 环境监测</div>
      <div class="env-grid">
        <div class="env-item">
          <text class="env-val temp">{{ env.temp }}</text>
          <text class="env-unit">℃</text>
          <text class="env-label">室内温度</text>
        </div>
        <div class="env-item">
          <text class="env-val hum">{{ env.hum }}</text>
          <text class="env-unit">%</text>
          <text class="env-label">空气湿度</text>
        </div>
      </div>
      <div class="update-time">更新时间: {{ lastUpdate }}</div>
    </div>

    <div class="card light-card">
      <div class="card-header">
        <text class="card-title">💡 灯光控制</text>
        <text class="state-text" :class="{on: light.isOn}">{{ light.isOn ? '已开启' : '已关闭' }}</text>
      </div>
      
      <div class="btn-group">
        <button class="btn btn-yellow" @click="sendCmd('ON')">全开</button>
        <button class="btn btn-gray" @click="sendCmd('OFF')">全关</button>
      </div>

      <div class="slider-area">
        <text>亮度: {{ light.val }}%</text>
        <slider 
          :value="light.val" 
          @change="onSliderChange" 
          min="0" max="100" 
          active-color="#ffc107" 
          block-size="24"
        />
      </div>
    </div>

    <div class="card door-card" :class="{ alert: door.isAlert }">
      <div class="card-header">
        <text class="card-title">🔒 门禁系统</text>
        <text class="state-text" :class="{open: door.isOpen}">{{ door.isOpen ? '🔓 已开启' : '🔒 已关闭' }}</text>
      </div>

      <div class="btn-group">
        <button class="btn btn-green" @click="sendDoorCmd('door open')">远程开门</button>
        <button class="btn btn-red" @click="sendDoorCmd('door close')">远程关门</button>
      </div>
      
      <div class="msg-box" v-if="door.lastMsg">
        📢 最新消息: {{ door.lastMsg }}
      </div>
    </div>

    <div class="log-box">
      <view v-for="(log, index) in logs" :key="index" class="log-item">{{ log }}</view>
    </div>
  </div>
</template>

<script>
import Paho from '@/common/mqtt.js'

export default {
  data() {
    return {
      client: null,
      isConnected: false,
      lastUpdate: '--:--',
      
      // 状态数据
      env: { temp: '--', hum: '--' },
      light: { isOn: false, val: 0 },
      door: { isOpen: false, lastMsg: '', isAlert: false },
      
      logs: [],
      
      // MQTT 配置 (与 Web 端完全一致)
      config: {
        host: 'broker.emqx.io',
        port: 8084,
        path: '/mqtt',
        
        // 核心修改 1: 门禁订阅改为通配符 '+'，同时监听报警和确认信号
        subDoor: 'China/Beijing/huayuan/302/door/+', 
        subHome: 'China/Beijing/huayuan/302/home/status',
        
        pubCmd: 'China/Beijing/huayuan/302/command',
        pubDoor: 'China/Beijing/huayuan/302/door/status'
      }
    }
  },
  onLoad() {
    setTimeout(() => {
      this.connectMQTT();
    }, 500);
  },
  onUnload() {
    if (this.client && this.isConnected) this.client.disconnect();
  },
  methods: {
    // --- 连接逻辑 ---
    connectMQTT() {
      this.addLog('正在连接服务器...');
      let clientId = 'AppV3-' + Math.random().toString(16).substr(2, 8);
      
      try {
        this.client = new Paho.MQTT.Client(this.config.host, this.config.port, this.config.path, clientId);
      } catch (e) {
        this.addLog('初始化失败: ' + e.message);
        return;
      }

      this.client.onConnectionLost = (res) => {
        this.isConnected = false;
        this.addLog('连接断开:' + res.errorMessage);
        // 断线重连
        setTimeout(() => this.connectMQTT(), 5000);
      };

      this.client.onMessageArrived = (message) => {
        this.handleMessage(message.destinationName, message.payloadString);
      };

      this.client.connect({
        useSSL: true, 
        cleanSession: true,
        keepAliveInterval: 60,
        onSuccess: () => {
          this.isConnected = true;
          this.addLog('✅ 连接成功!');
          // 订阅
          this.client.subscribe(this.config.subHome);
          this.client.subscribe(this.config.subDoor);
          
          // 核心修改 2: 连接成功后只查一次，不再自动轮询
          this.manualQuery();
        },
        onFailure: (e) => {
          this.isConnected = false;
          this.addLog('❌ 连接失败: ' + e.errorMessage);
        }
      });
    },

    // --- 消息处理 (核心逻辑) ---
    handleMessage(topic, msg) {
      // 1. 处理环境状态 (匹配 home/status)
      // 使用 includes 而不是 ===，增加容错性
      if (topic.includes('home/status')) {
        this.lastUpdate = new Date().toLocaleTimeString();
        if (msg.includes('温度')) this.env.temp = this.extractNum(msg);
        if (msg.includes('湿度')) this.env.hum = this.extractNum(msg);
        if (msg.includes('灯光')) this.light.isOn = msg.includes('开启');
        if (msg.includes('亮度')) {
           let val = parseInt(this.extractNum(msg));
           if (!isNaN(val)) this.light.val = val;
        }
        if (msg.includes('大门')) this.door.isOpen = msg.includes('开启');
      }

      // 2. 处理门禁消息 (匹配 door 下的任何消息)
      // 核心修改 3: 使用 includes('door') 来同时捕获 status 和 command
      if (topic.includes('door')) {
        this.door.lastMsg = msg + ' (' + new Date().toLocaleTimeString() + ')';
        
        // 报警处理
        if (msg.includes('客人') || msg.includes('错误')) {
          this.triggerAlert(); 
          uni.vibrateLong(); // 手机震动
        }
        
        // 核心修改 4: 监听开门/关门确认信号 (解决按钮失灵问题)
        if (msg.includes('door open')) {
          this.door.isOpen = true;
          this.addLog('收到确认: 门已开');
        }
        if (msg.includes('door close')) {
          this.door.isOpen = false;
          this.addLog('收到确认: 门已关');
        }
      }
    },

    // --- 控制指令 ---
    sendCmd(cmd) {
      this.publish(this.config.pubCmd, cmd);
    },

    sendDoorCmd(cmd) {
      this.publish(this.config.pubDoor, cmd);
    },

    onSliderChange(e) {
      this.light.val = e.detail.value;
      this.sendCmd('light:' + this.light.val);
    },

    manualQuery() {
      this.sendCmd('get_status');
      uni.showToast({ title: '已发送查询', icon: 'none' });
    },
    
    // --- 辅助函数 ---
    publish(topic, payload) {
      if (this.client && this.isConnected) {
        let message = new Paho.MQTT.Message(payload);
        message.destinationName = topic;
        this.client.send(message);
        this.addLog('发送: ' + payload);
      } else {
        uni.showToast({ title: '未连接', icon: 'none' });
      }
    },

    extractNum(str) {
      let match = str.match(/-?\d+(\.\d+)?/);
      return match ? match[0] : '--';
    },

    triggerAlert() {
      this.door.isAlert = true;
      setTimeout(() => { this.door.isAlert = false }, 3000);
    },

    addLog(txt) {
      let time = new Date().toLocaleTimeString();
      this.logs.unshift(`[${time}] ${txt}`);
      if(this.logs.length > 20) this.logs.pop();
    }
  }
}
</script>

<style>
/* 样式与之前保持一致，无需改动 */
.content { padding: 20px; background-color: #f4f6f9; min-height: 100vh; }
.header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px; }
.title { font-size: 20px; font-weight: bold; color: #333; }
.status-bar { display: flex; align-items: center; gap: 10px; }
.badge { font-size: 12px; padding: 4px 8px; background: #eee; border-radius: 10px; color: #666; }
.badge.online { background: #d4edda; color: #155724; }
.refresh-btn { font-size: 12px; padding: 5px 10px; background: #007bff; color: white; border-radius: 20px; border:none; }
.card { background: white; border-radius: 15px; padding: 20px; margin-bottom: 20px; box-shadow: 0 2px 10px rgba(0,0,0,0.03); }
.card-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px; border-bottom: 1px solid #f0f0f0; padding-bottom: 10px; }
.card-title { font-size: 16px; font-weight: bold; color: #333; }
.state-text { font-size: 14px; color: #999; }
.state-text.on { color: #ffc107; font-weight: bold; }
.state-text.open { color: #dc3545; font-weight: bold; }
.env-grid { display: flex; justify-content: space-around; text-align: center; margin: 15px 0; }
.env-val { font-size: 28px; font-weight: bold; }
.env-val.temp { color: #e67e22; }
.env-val.hum { color: #3498db; }
.env-unit { font-size: 12px; color: #999; margin-left: 2px; }
.env-label { font-size: 12px; color: #666; display: block; margin-top: 5px; }
.update-time { text-align: center; font-size: 10px; color: #ccc; margin-top: 10px; }
.btn-group { display: flex; gap: 15px; margin-bottom: 15px; }
.btn { flex: 1; height: 44px; line-height: 44px; text-align: center; border-radius: 8px; font-size: 16px; border: none; color: white; }
.btn-yellow { background: linear-gradient(135deg, #f1c40f, #f39c12); color: #333; }
.btn-gray { background: #bdc3c7; }
.btn-green { background: linear-gradient(135deg, #2ecc71, #27ae60); }
.btn-red { background: linear-gradient(135deg, #e74c3c, #c0392b); }
.slider-area { background: #f8f9fa; padding: 15px; border-radius: 10px; }
.door-card.alert { animation: flash 1s infinite; border: 2px solid red; }
@keyframes flash { 0% { opacity: 1; } 50% { opacity: 0.8; background-color: #fff0f0; } 100% { opacity: 1; } }
.msg-box { background: #fff3cd; color: #856404; padding: 10px; font-size: 12px; border-radius: 6px; text-align: center; }
.log-box { background: #2c3e50; color: #7bed9f; padding: 10px; height: 120px; border-radius: 8px; font-size: 10px; overflow-y: scroll; }
.log-item { margin-bottom: 4px; border-bottom: 1px dashed #444; }
</style>