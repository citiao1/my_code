<template>
	<div class="content">
		<div class="card">
			<div class="title">电脑剪贴板同步助手</div>
			
			<div class="input-group">
				<text>电脑 IP 地址：</text>
				<input class="input" v-model="ip" type="text" placeholder="例如 192.168.1.5" />
			</div>
			
			<div class="status-box">
				<text>状态：{{ statusText }}</text>
			</div>

			<button class="btn" :class="{active: isRunning}" @click="toggleSync">
				{{ isRunning ? '正在运行中 (点击停止)' : '开始自动同步' }}
			</button>
			
			<scroll-view scroll-y="true" class="log-box">
				<div v-for="(log, index) in logs" :key="index" class="log-item">
					{{ log }}
				</div>
			</scroll-view>
		</div>
	</div>
</template>

<script>
	export default {
		data() {
			return {
				ip: '192.168.1.5', // 默认IP，你可以改成你常用的
				port: '5000',
				isRunning: false,
				timer: null,
				lastHash: 'init',
				statusText: '等待开始',
				logs: []
			}
		},
		onLoad() {
			// 读取上次存的IP，不用每次都输
			const savedIp = uni.getStorageSync('pc_ip');
			if(savedIp) this.ip = savedIp;
		},
		methods: {
			addLog(msg) {
				const time = new Date().toTimeString().split(' ')[0];
				this.logs.unshift(`[${time}] ${msg}`);
				if(this.logs.length > 50) this.logs.pop();
			},
			toggleSync() {
				if (this.isRunning) {
					this.stopSync();
				} else {
					this.startSync();
				}
			},
			startSync() {
				if (!this.ip) return uni.showToast({title: '请输入IP', icon:'none'});
				
				// 保存IP方便下次用
				uni.setStorageSync('pc_ip', this.ip);
				
				this.isRunning = true;
				this.statusText = "正在监听电脑...";
				this.addLog("服务已启动");
				
				// 开启轮询，每1.5秒检查一次
				this.timer = setInterval(() => {
					this.checkUpdate();
				}, 1500);
			},
			stopSync() {
				clearInterval(this.timer);
				this.isRunning = false;
				this.statusText = "已停止";
				this.addLog("服务已停止");
			},
			checkUpdate() {
				uni.request({
					url: `http://${this.ip}:${this.port}/check`,
					method: 'GET',
					success: (res) => {
						if (res.statusCode == 200 && res.data.hash) {
							// 发现新哈希且不等于旧哈希
							if (res.data.hash !== 'init' && res.data.hash !== this.lastHash) {
								this.addLog("发现新图片，准备下载...");
								this.downloadImage(res.data.hash);
							}
						}
					},
					fail: () => {
						this.statusText = "连接电脑失败，请检查IP";
					}
				});
			},
			downloadImage(newHash) {
				const url = `http://${this.ip}:${this.port}/download`;
				
				// 1. 下载文件到临时目录
				uni.downloadFile({
					url: url,
					success: (res) => {
						if (res.statusCode === 200) {
							// 2. 保存到系统相册
							uni.saveImageToPhotosAlbum({
								filePath: res.tempFilePath,
								success: () => {
									this.lastHash = newHash; // 更新哈希，防止重复下载
									this.addLog("成功保存到相册！");
									uni.showToast({title: '图片已同步', icon: 'success'});
								},
								fail: (err) => {
									this.addLog("保存相册失败: " + JSON.stringify(err));
									// 如果失败，通常是权限问题，引导去设置
									uni.showModal({
										content: '保存失败，请检查是否授予相册权限',
										showCancel: false
									});
								}
							});
						}
					}
				});
			}
		}
	}
</script>

<style>
	page { background-color: #f5f5f5; }
	.content { padding: 30px 20px; display: flex; justify-content: center; }
	.card { width: 100%; background: #fff; border-radius: 15px; padding: 20px; box-shadow: 0 4px 10px rgba(0,0,0,0.05); }
	.title { font-size: 20px; font-weight: bold; text-align: center; margin-bottom: 30px; color: #333; }
	.input-group { margin-bottom: 20px; }
	.input { border: 1px solid #ddd; height: 45px; border-radius: 8px; padding: 0 10px; margin-top: 5px; font-size: 16px; }
	.btn { background-color: #007aff; color: #fff; border-radius: 8px; margin-bottom: 20px; }
	.btn.active { background-color: #dd524d; } /* 运行时变成红色 */
	.status-box { text-align: center; margin-bottom: 15px; color: #666; font-size: 14px; }
	.log-box { height: 200px; background: #f9f9f9; border-radius: 8px; padding: 10px; border: 1px solid #eee; }
	.log-item { font-size: 12px; color: #888; margin-bottom: 5px; }
</style>