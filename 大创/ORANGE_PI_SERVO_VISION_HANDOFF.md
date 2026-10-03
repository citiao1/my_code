# 香橙派舵机与视觉协同项目交接文档

更新时间：2026-10-03

本文用于交给下一个 AI，继续完成 Orange Pi Zero 3 上的“摄像头人脸识别 + 舵机跟踪”程序。

## 1. 当前目标

项目最终目标：

```text
Logitech C270 摄像头
        |
Orange Pi Zero 3
  人脸识别与中心偏差计算
        |
        +-- PWM3/PWM4 -> 舵机云台
        |
        +-- 浏览器实时监控页面
```

当前阶段只控制香橙派上的舵机，不连接 STM32，也不发送底盘运动命令。

## 2. 香橙派与网络

| 项目 | 当前值 |
| --- | --- |
| 开发板 | Orange Pi Zero 3 v1.2 |
| SSH 用户 | `orangepi` |
| 局域网 IP | `192.168.1.215` |
| SSH 命令 | `ssh orangepi@192.168.1.215` |
| 摄像头 | Logitech C270 |
| 摄像头设备 | 通常为 `/dev/video1` |
| 摄像头采集 | `640x480 @ 30 FPS` |
| Tailscale | 当前不需要，使用局域网 |

电脑只负责 SSH 输入命令、查看日志和浏览器监控；视觉和舵机程序必须运行在香橙派上。

## 3. 舵机接线与 PWM 对应关系

用户已经完成舵机接线。当前使用的是开发板顶部的 **3 针调试串口排针**，不是 26 针扩展排针里的物理 `P28/P30`。

准确对应关系：

| 图上标识 | 芯片引脚 | PWM 通道 |
| --- | --- | --- |
| `TX` | `PH0` | `PWM3` |
| `RX` | `PH1` | `PWM4` |
| `GND` | GND | 地线 |

因此当前推荐：

```text
水平舵机信号线 -> TX / PH0 / PWM3
垂直舵机信号线 -> RX / PH1 / PWM4
舵机 GND       -> Orange Pi GND
舵机电源       -> 独立 5V 电源
独立电源 GND   -> Orange Pi GND 共地
```

如果当前只有一个舵机，优先接 `PWM3`，用于水平左右转动。

注意：

- `P28/P30` 不是这块 26 针排针上的物理位置，不能按 `P28`、`P30` 找插针。
- 舵机红线不要接 Orange Pi 的 3.3V。
- 舵机电源建议使用独立稳定的 5V 电源。
- Orange Pi、舵机电源、舵机信号必须共地。
- `PWM3/PWM4` 与顶部调试串口 `UART0` 复用。启用 PWM3/PWM4 后，顶部 TX/RX 不能继续作为调试串口使用，但 SSH 和网络不会受影响。
- 26 针排针上的 `PH2`、`PH3` 是另一组 PWM 复用脚，分别对应 PWM2/PWM1；当前接线没有使用它们。

## 4. 启用 PWM 的系统配置

Orange Pi 官方系统默认关闭 PWM。需要在香橙派上执行：

```bash
sudo orangepi-config
```

菜单中选择：

```text
System
  -> Hardware
     -> 开启 ph-pwm34
     -> 关闭/不要勾选 uart0
```

保存后重启：

```bash
sudo reboot
```

重启后检查 PWM 节点：

```bash
ls -l /sys/class/pwm/
```

下一步 AI 必须先确认实际系统里的 PWM 节点编号，不要只根据 `pwm3`、`pwm4` 的名字猜测物理引脚：

```bash
find /sys/class/pwm -maxdepth 2 -type f -print
cat /sys/kernel/debug/pinctrl/*/pinmux-pins | grep -E 'PH0|PH1|pwm'
```

## 5. 当前视觉程序

项目目录：

```text
~/mecanum-motor-console/
```

主要文件：

```text
vision/camera.py
    RTSP 取流、断线重连、只保留最新帧，避免旧帧堆积。

vision/face_detector.py
    OpenCV YuNet ONNX 人脸检测器。

vision/face_tracker.py
    多人脸目标选择、IoU 跟踪、中心偏差计算和指数平滑。

vision/run_face_tracking.py
    人脸跟踪主循环、画框、画中心连线、保存日志和更新监控页面。

vision/monitor_server.py
    实时监控 HTTP 服务。

vision/models/face_detection_yunet_2023mar.onnx
    当前使用的 YuNet 模型。
```

当前虚拟环境：

```text
~/.venvs/orange-pi-vision/
```

当前视觉程序仍然是只读模式：

```text
不会连接 STM32
不会打开 /dev/ttyS5
不会发送 DRV、STOP 或其他底盘命令
```

## 6. 摄像头与监控地址

香橙派本机 RTSP：

```text
rtsp://127.0.0.1:8554/camera
```

局域网电脑可访问的 RTSP：

```text
rtsp://192.168.1.215:8554/camera
```

局域网 WebRTC 预览：

```text
http://192.168.1.215:8889/camera/
```

人脸识别监控页面：

```text
http://192.168.1.215:8090/
```

当前人脸跟踪启动命令：

```bash
cd ~/mecanum-motor-console
VISION_RTSP_TRANSPORT=udp nohup ~/.venvs/orange-pi-vision/bin/python \
  vision/run_face_tracking.py \
  --source rtsp://127.0.0.1:8554/camera \
  --model vision/models/face_detection_yunet_2023mar.onnx \
  --inference-width 320 \
  --detect-every 2 \
  --monitor-host 0.0.0.0 \
  --monitor-port 8090 \
  --headless \
  --status-interval 5 \
  > vision/outputs/face_tracking.log 2>&1 &
```

启动前先检查旧进程，避免重复运行：

```bash
pgrep -af run_face_tracking.py
```

日志：

```bash
tail -f ~/mecanum-motor-console/vision/outputs/face_tracking.log
```

## 7. 视觉输出定义

监控页面画面包含：

- 屏幕中心十字。
- 人脸矩形框。
- 人脸中心点。
- 从屏幕中心到人脸中心的连线。
- FPS。
- `dx` 和 `dy`。

接口：

```text
/api/status
/frame.jpg
/stream.mjpg
/healthz
```

核心输出文件：

```text
vision/outputs/face_tracking.jsonl
vision/outputs/face_latest.json
vision/outputs/face_latest.jpg
vision/outputs/face_tracking.log
```

偏差定义：

```text
dx = 人脸中心 x - 画面中心 x
dy = 人脸中心 y - 画面中心 y
```

符号约定：

```text
dx > 0：人脸在画面中心右侧
dx < 0：人脸在画面中心左侧
dy > 0：人脸在画面中心下方
dy < 0：人脸在画面中心上方
```

`offset_px` 是图像像素偏差，不是厘米距离。`offset_norm` 是归一化偏差，范围通常接近 `-1.0` 到 `1.0`。

## 8. 下一步舵机程序设计建议

不要把舵机控制直接写成阻塞式代码塞进人脸识别循环。建议拆成以下模块：

```text
vision/servo_pwm.py
    负责 PWM 初始化、占空比写入、停止和释放。

vision/servo_tracker.py
    负责把 dx/dy 转换为目标舵机角度或脉宽。

vision/run_servo_face_tracking.py
    组合摄像头、人脸识别、监控和舵机控制。
```

推荐控制链路：

```text
face offset
    -> deadband
    -> 比例控制或限速控制
    -> 角度/脉宽限幅
    -> PWM 输出
```

初始安全参数建议：

| 参数 | 初始值 |
| --- | --- |
| PWM 频率 | 50 Hz |
| 周期 | 20,000,000 ns |
| 中位脉宽 | 1,500,000 ns |
| 初始安全范围 | 1,000,000 到 2,000,000 ns |
| 控制更新周期 | 20 到 50 ms |
| 中心死区 | 约 10 到 20 像素 |
| 最大单次脉宽变化 | 需要限速，避免抖动 |

不要一开始使用舵机的极限脉宽。先用 1.0 ms 到 2.0 ms 测试，确认机械结构和方向后再扩大范围。

两个舵机的推荐逻辑：

```text
水平舵机 PWM3:
    根据 dx 控制左右转动。

垂直舵机 PWM4:
    根据 dy 控制上下转动。
```

方向必须做成配置项，因为云台安装方向可能相反：

```text
PAN_INVERT = false
TILT_INVERT = true
```

人脸丢失时建议先保持最后位置一小段时间，然后缓慢回中或停止输出；不要因为单帧丢脸就突然把舵机打到极限。

## 9. 舵机控制安全要求

- 程序启动时先输出中位脉宽，不要直接输出上一轮残留角度。
- 程序退出、摄像头断开、监控程序异常时，停止 PWM 或回到中位。
- 先拆掉舵机负载或让云台悬空测试。
- 先只测试一个 PWM 通道和一个舵机。
- 用示波器或逻辑分析仪确认信号频率约为 50 Hz、脉宽在安全范围内。
- 舵机电源不足时会导致 Orange Pi 重启或摄像头断流，必须单独供电。
- 不要让舵机程序连接 STM32 或调用底盘 WebSocket。
- 后续如果要让底盘跟随人脸，必须另行设计控制协议，并保留 STOP、超时停车和断线保护。

## 10. 建议的验证顺序

### 第一步：确认接线

```text
舵机信号 -> TX/PWM3
舵机红线 -> 独立 5V
舵机黑/棕线 -> GND
电源 GND 与 Orange Pi GND 共地
```

### 第二步：确认 PWM 配置

```bash
sudo orangepi-config
```

开启 `ph-pwm34`，关闭 `uart0`，重启后确认 `/sys/class/pwm`。

### 第三步：只输出固定中位 PWM

先让一个舵机固定在约 1500 us，不接视觉控制，确认舵机不抖动、不发热、方向机构没有卡死。

### 第四步：人工改变目标脉宽

只允许在安全范围内小幅改变脉宽，例如 1400 us、1500 us、1600 us，确认舵机转动方向。

### 第五步：接入 `dx`

只先接水平舵机：

```text
dx 为正 -> 按配置决定向左或向右修正
dx 为负 -> 反向修正
```

### 第六步：接入 `dy`

确认水平控制稳定后，再把 `dy` 接到第二个舵机。

### 第七步：调整延迟和抖动

重点观察：

- 监控页面是否仍然低延迟。
- 舵机是否因检测噪声快速抖动。
- 人脸丢失后是否安全停留或回中。
- 摄像头断流时 PWM 是否进入安全状态。

## 11. 当前已知测试结果

- YuNet 人脸检测已完成。
- 人脸中心和画面中心连线已完成。
- `dx/dy` 已输出到监控页面、JSON 和日志。
- 采集线程只保留最新帧，已经处理过明显的旧帧堆积延迟问题。
- 本地测试曾通过：8 个测试通过，Python `compileall` 通过。
- 当前没有完成舵机 PWM 实际波形测试。
- 当前没有完成舵机和人脸偏差的闭环控制。

## 12. 不要误改的内容

- 不要重新接入 Tailscale；当前使用局域网 `192.168.1.215`。
- 不要把 Windows PowerShell 的 `$env:CAMERA_URL=...` 命令直接粘到香橙派 Bash。
- 不要把摄像头识别程序放回 Windows 上运行；目标是香橙派本机运行。
- 不要因为要控制舵机而修改 STM32 固件。
- 不要把 `P28/P30` 当成这块板的物理排针编号。
- 不要关闭 MediaMTX、FFmpeg 或现有实时监控页面，除非确认新的采集方案已经验证。

## 13. 相关文件

| 文件 | 用途 |
| --- | --- |
| `vision/README.md` | 当前视觉程序使用说明 |
| `vision/camera.py` | 最新帧摄像头采集 |
| `vision/face_detector.py` | YuNet 人脸检测 |
| `vision/face_tracker.py` | 人脸跟踪和偏差计算 |
| `vision/run_face_tracking.py` | 人脸跟踪主程序 |
| `vision/monitor_server.py` | 实时监控页面和状态接口 |
| `mecanum-motor-console/orange-pi/start.sh` | 摄像头、MediaMTX 和底盘桥启动脚本 |
| `VISION_RECOGNITION_HANDOFF.md` | 视觉项目早期交接说明，部分网络内容已过时 |
| `ORANGE_PI_HOST_HANDOFF.md` | STM32 底盘通信协议，只有后续明确接入底盘时才使用 |

