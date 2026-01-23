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
// 引入 mqtt 库
import mqtt from 'mqtt'

export default {
  data() {
    return {
      client: null,
      isConnected: false,
      statusText: '正在连接...',
      brightness: 0,
      logs: [],
      // --- 配置你的 MQTT 信息 ---
      // 注意：App/H5 必须用 wxs (加密) 或 ws (非加密) 协议
      // EMQX 的 WebSocket 端口通常是 8083
      url: 'ws://broker.emqx.io:8083/mqtt',
      topicPub: 'China/Beijing/huayuan/302/command',
      topicSub: 'China/Beijing/huayuan/302/status'
    }
  },
  onLoad() {
    this.connectMQTT()
  },
  methods: {
    // 1. 连接 MQTT
    connectMQTT() {
      this.addLog('开始连接服务器...')
      
      // 创建客户端实例
      this.client = mqtt.connect(this.url, {
        clientId: 'App-' + Math.random().toString(16).substr(2, 8),
        clean: true,
        connectTimeout: 4000,
        reconnectPeriod: 1000,
      })

      // 连接成功回调
      this.client.on('connect', () => {
        this.isConnected = true
        this.statusText = '在线'
        this.addLog('✅ 连接成功!')
        
        // 订阅状态
        this.client.subscribe(this.topicSub, (err) => {
          if (!err) this.addLog('已订阅状态频道')
        })
      })

      // 连接断开回调
      this.client.on('close', () => {
        this.isConnected = false
        this.statusText = '离线'
      })

      // 收到消息回调
      this.client.on('message', (topic, message) => {
        const msg = message.toString()
        this.addLog('收到: ' + msg)
        
        // 解析亮度 (如果设备发回 "亮度:50%")
        if (msg.includes('亮度:')) {
           // 简单的正则提取数字
           let num = msg.replace(/[^0-9]/g, '');
           if(num) this.brightness = parseInt(num);
        }
      })
    },

    // 2. 发送指令
    sendCmd(cmd) {
      if (this.client && this.isConnected) {
        this.client.publish(this.topicPub, cmd)
        this.addLog('发送: ' + cmd)
      } else {
        uni.showToast({ title: '未连接服务器', icon: 'none' })
      }
    },

    // 3. 滑块拖动结束
    onSliderChange(e) {
      this.brightness = e.detail.value
      // 发送 L:50 格式
      this.sendCmd('L:' + this.brightness)
    },

    // 日志辅助
    addLog(txt) {
      let time = new Date().toLocaleTimeString();
      this.logs.unshift(`[${time}] ${txt}`) // 新消息在最上面
      if(this.logs.length > 20) this.logs.pop() // 只保留20条
    }
  }
}
</script>

<style>
/* 简单的 CSS 美化 */
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