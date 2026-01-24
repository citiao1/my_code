<template>
  <div class="content">
    <div class="header">
      <h2 class="title">🏠 智能家居控制</h2>
      <p class="status">状态: <span :class="{ online: isConnected }">{{ statusText }}</span></p>
    </div>

    <div class="btn-group">
      <button class="btn on-btn" @click="sendCmd('ON')">开灯</button>
      <button class="btn off-btn" @click="sendCmd('OFF')">关灯</button>
    </div>

    <div class="slider-area">
      <p>当前亮度: {{ brightness }}%</p>
      <slider :value="brightness" @change="onSliderChange" min="0" max="100" show-value block-size="20"/>
    </div>

    <div class="log-box">
      <p v-for="(log, index) in logs" :key="index">{{ log }}</p>
    </div>
  </div>
</template>

<script>
// 1. 引入刚才创建的兼容库文件
// (Paho 库会自动把全局变量挂载到 window 对象上，所以不需要 import x from ...)
// 加一个 "Paho from"，意思是从那个文件里把 Paho 拿出来
import Paho from '@/common/mqtt.js'

export default {
  data() {
    return {
      client: null,
      isConnected: false,
      statusText: '等待连接...',
      brightness: 0,
      logs: [],
      // 配置信息
      host: 'broker.emqx.io',
      port: 8084,
      topicPub: 'China/Beijing/huayuan/302/command',
      topicSub: 'China/Beijing/huayuan/302/status'
    }
  },
  onLoad() {
    // 延迟一点点执行，确保库加载完毕
    setTimeout(() => {
      this.connectMQTT()
    }, 500)
  },
  methods: {
    // --- 连接 MQTT (Paho 写法) ---
    connectMQTT() {
      this.addLog('正在初始化 Paho 客户端...')
      
      // 生成随机ID
      let clientId = 'App-' + Math.random().toString(16).substr(2, 8);
      
      // 创建客户端实例 (注意：Paho 是全局对象)
      // 参数：主机地址, 端口, 路径(默认/mqtt), 客户端ID
      try {
        this.client = new Paho.MQTT.Client(this.host, this.port, "/mqtt", clientId);
      } catch (e) {
        this.addLog('初始化失败: ' + e.message);
        return;
      }

      // 设置回调
      this.client.onConnectionLost = (responseObject) => {
        this.isConnected = false;
        this.statusText = '离线';
        if (responseObject.errorCode !== 0) {
          this.addLog('连接断开:' + responseObject.errorMessage);
        }
      };

      this.client.onMessageArrived = (message) => {
        const msg = message.payloadString;
        this.addLog('收到: ' + msg);
        
        // 解析亮度 (如果设备发回 "亮度:50%")
        if (msg.includes('亮度:')) {
           let num = msg.replace(/[^0-9]/g, '');
           if(num) this.brightness = parseInt(num);
        }
      };

      // 开始连接
      this.addLog('开始连接服务器...');
      this.client.connect({
        useSSL: true, // EMQX 8083 是非加密 ws
        cleanSession: true,
        keepAliveInterval: 60,
        onSuccess: () => {
          this.isConnected = true;
          this.statusText = '在线';
          this.addLog('✅ 连接成功!');
          // 订阅
          this.client.subscribe(this.topicSub);
        },
        onFailure: (e) => {
          this.isConnected = false;
          this.statusText = '连接失败';
          this.addLog('❌ 连接失败: ' + e.errorMessage);
        }
      });
    },

    // --- 发送指令 ---
    sendCmd(cmd) {
      if (this.client && this.isConnected) {
        let message = new Paho.MQTT.Message(cmd);
        message.destinationName = this.topicPub;
        this.client.send(message);
        this.addLog('发送: ' + cmd);
      } else {
        uni.showToast({ title: '未连接', icon: 'none' });
      }
    },

    // 滑块变动
    onSliderChange(e) {
      this.brightness = e.detail.value;
      this.sendCmd('L:' + this.brightness);
    },

    // 日志辅助
    addLog(txt) {
      let time = new Date().toLocaleTimeString();
      this.logs.unshift(`[${time}] ${txt}`);
    }
  }
}
</script>

<style>
/* 样式保持不变 */
.content { padding: 30px; }
.header { text-align: center; margin-bottom: 30px; }
.title { font-size: 24px; font-weight: bold; }
.status { color: #999; margin-top: 10px; }
.status span.online { color: #4cd964; font-weight: bold; }

.btn-group { display: flex; justify-content: space-around; margin-bottom: 40px; }
.btn { width: 120px; height: 120px; border-radius: 60px; border: none; font-size: 20px; color: white; display: flex; align-items: center; justify-content: center; box-shadow: 0 4px 10px rgba(0,0,0,0.2); }
.on-btn { background: #007aff; }
.off-btn { background: #ff3b30; }
.on-btn:active { background: #0056b3; }
.off-btn:active { background: #ce2c24; }

.slider-area { margin: 20px 0; padding: 20px; background: #f8f8f8; border-radius: 10px; }
.log-box { background: #333; color: #0f0; padding: 10px; height: 150px; overflow-y: scroll; font-size: 12px; border-radius: 8px; margin-top: 20px; }
</style>