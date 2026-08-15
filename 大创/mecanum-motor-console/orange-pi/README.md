# Orange Pi Zero 3 部署说明

当前封装会同时启动纯遥控网页、串口桥接和 WebRTC 图传：

- 控制网页：端口 `8088`
- 串口桥接：`/dev/ttyS5`，`115200 8N1`
- WebSocket：端口 `8766`
- MediaMTX WebRTC：端口 `8889`，ICE UDP 端口 `8189`
- FFmpeg 摄像头采集：默认 `/dev/video1`，`320x240@15 FPS`

## 手动启动

```bash
cd ~/mecanum-motor-console
bash orange-pi/start.sh
```

按 `Ctrl+C` 会停止本次启动的所有模块。

## 安装开机自启

先按 `Ctrl+C` 停止手动运行的 `start.sh`，再执行：

```bash
cd ~/mecanum-motor-console
sudo bash orange-pi/install-service.sh
```

脚本会立即启动服务，并设置香橙派以后每次开机自动启动。常用管理命令：

```bash
sudo systemctl status mecanum-console.service
journalctl -u mecanum-console.service -f
sudo systemctl restart mecanum-console.service
sudo systemctl stop mecanum-console.service
```

查看日志时按 `Ctrl+C` 只会退出日志查看，不会停止后台服务。

## 访问地址

### 安装为手机桌面应用

控制台服务正常运行后执行一次：

```bash
cd ~/mecanum-motor-console
sudo bash orange-pi/setup-pwa.sh
```

脚本会配置开机后仍然有效的 Tailscale Serve HTTPS 代理，并打印：

- `Orange Pi HTTPS page`：直接访问香橙派的加密地址
- `GitHub remote page`：用于首次打开并安装手机桌面应用的完整地址

HTTPS 代理使用同一域名的三个路径：网页 `/`、控制 WebSocket `/ws`、MediaMTX 图传 `/camera/`。图传路径保留结尾斜杠，以便同时转发 MediaMTX 的 WHEP 子请求。

手机保持 Tailscale 在线，打开 `GitHub remote page`，再从浏览器菜单选择“安装应用”或“添加到主屏幕”。页面会保存目标香橙派地址，从桌面图标再次打开时不需要重新配置。

查看当前代理状态：

```bash
tailscale serve status
```

### HTTP 直接访问

桌面和手机使用同一个 Tailscale 地址：

```text
http://100.109.90.22:8088/
```

局域网地址：

```text
http://192.168.62.34:8088/
```

页面只显示图传、速度、双摇杆、STOP 和连接状态。服务运行后不需要保持 SSH 窗口打开。不要把 `8088`、`8766`、`8889` 或 `8189` 直接映射到公网；当前控制台没有登录认证，应继续通过 Tailscale 使用。
