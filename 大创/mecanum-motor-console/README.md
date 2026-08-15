# C30D 遥控上位机

这是香橙派 Zero 3 上运行的麦轮底盘遥控页面。桌面和手机共用同一个响应式界面，只保留：

- WebRTC 实时图传
- 左侧平移摇杆
- 右侧旋转摇杆
- 速度滑块
- STOP 按钮
- 控制链路状态

页面不显示轮速、编码器、PID、串口日志或自动测试数据。STM32 遥测仍在后台用于判断电机、速度环和陀螺仪是否就绪，并在遥测超时时停车。

## Orange Pi 启动

```bash
cd ~/mecanum-motor-console
bash orange-pi/start.sh
```

已安装 systemd 服务时不需要手动启动。检查状态：

```bash
sudo systemctl status mecanum-console.service
```

## 访问

### 手机桌面应用（GitHub Pages + Tailscale HTTPS）

香橙派首次执行一次：

```bash
cd ~/mecanum-motor-console
sudo bash orange-pi/setup-pwa.sh
```

脚本会打印一个包含 `?target=https://...ts.net` 的 `GitHub remote page`。手机保持 Tailscale 在线，打开这个完整地址一次，页面会记住香橙派地址。随后从浏览器菜单选择“安装应用”或“添加到主屏幕”；以后从桌面图标打开不需要再次输入地址。

旧的 `https://citiao1.github.io/my_code/?mobile=1` 是蓝牙调试版入口，应先删除旧桌面快捷方式。

### 直接访问香橙派

Tailscale：

```text
http://100.109.90.22:8088/
```

当前局域网：

```text
http://192.168.62.34:8088/
```

通过 HTTP 直接打开香橙派时，网页会自动连接当前主机的 WebSocket `8766` 端口和 WebRTC `8889` 端口。通过 HTTPS 或 GitHub Pages 打开时，页面改用 Tailscale Serve 提供的同域 `/ws` 和 `/camera/` 加密路径；图传路径保留结尾斜杠，以转发 MediaMTX 的全部 WHEP 子请求。

## 遥控映射

速度范围为 5 到 120。左摇杆控制前后和左右平移，右摇杆横轴控制旋转：

```text
forward = round(-left_stick_y  * speed)
left    = round(-left_stick_x  * speed)
yaw     = round(-right_stick_x * speed)
```

两个摇杆支持手机双指同时操作。按住期间每 100 ms 发送一次 `DRV,forward,left,yaw`，全部松开、页面后台、连接断开或遥测超时会发送 `STOP`。

## 默认服务

| 模块 | 默认值 |
| --- | --- |
| 控制网页 | `0.0.0.0:8088` |
| WebSocket 桥接 | `0.0.0.0:8766` |
| 香橙派串口 | `/dev/ttyS5` |
| 香橙派串口波特率 | `115200 8N1` |
| MediaMTX WebRTC | HTTP `8889`、ICE UDP `8189` |
| 摄像头 | `/dev/video1`，`320x240@15 FPS` |

香橙派现在连接 STM32 USART3：PC10=TX、PC11=RX，`115200 8N1`。STM32 USART2 的 PD5/PD6 仍保留给蓝牙，参数为 `9600 8N1`；两路接口可同时使用，固件会广播响应和遥测到两路。

完整接线、命令、响应、遥测字段和联调步骤见仓库根目录的 `STM32_ORANGE_PI_UART_HANDOFF.md`。

## 安全

- 当前网页没有账号、鉴权或加密，不要把端口直接映射到公网。
- 继续通过 Tailscale 访问。
- STM32 必须保留 300 ms 运动命令看门狗。
- 实车首次联调前先把四个轮子悬空。
