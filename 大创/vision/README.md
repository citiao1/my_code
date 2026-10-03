# 香橙派视觉识别第一阶段

这部分代码运行在香橙派 Zero 3 上，从本机 MediaMTX 的 RTSP 视频流取帧。
电脑只通过 SSH 输入命令、查看日志和取回输出图片。
当前阶段只做视频读取、人脸检测、人脸位置跟踪、中心偏差计算、画框、
保存图片和 JSONL 日志。
程序不会连接 STM32，也不会发送 `STOP`、`DRV` 或其他底盘命令。

## 目录

```text
vision/
  camera.py          RTSP/摄像头取流和断线重连
  camera_preview.py  最小预览程序
  capture_frames.py  按间隔采集训练/调试图片
  detector.py        检测器接口和颜色区域示例
  face_detector.py   YuNet 人脸检测
  face_tracker.py    目标选择和中心偏差计算
  run_face_tracking.py 人脸跟踪主循环
  run_detection.py   主循环、标注、保存和日志
  requirements.txt   Python 依赖
  requirements-orangepi.txt  香橙派 ARM64 依赖
  models/            ONNX 模型文件
  outputs/           运行时输出，不提交模型和大图片
```

## 香橙派安装

先把本目录同步到香橙派已有的项目目录，例如从 Windows PowerShell 执行：

```powershell
scp -r vision orangepi@192.168.1.215:~/mecanum-motor-console/
```

然后通过 SSH 登录香橙派：

```powershell
ssh orangepi@192.168.1.215
```

在香橙派上安装系统级 OpenCV，避免在 ARM 板上编译大型 Python wheel：

```bash
sudo apt update
sudo apt install -y python3-opencv python3-numpy python3-venv
python3 -m venv --system-site-packages ~/.venvs/orange-pi-vision
```

检查摄像头服务：

```bash
sudo systemctl is-active mecanum-console.service
ss -lntp | grep -E '8088|8554|8766|8889'
```

## 在香橙派运行

香橙派本机的 RTSP 地址是：

```text
rtsp://127.0.0.1:8554/camera
```

无显示器环境下先保存一张图片：

```bash
cd ~/mecanum-motor-console
~/.venvs/orange-pi-vision/bin/python vision/camera_preview.py --headless
```

运行识别框架：

```bash
cd ~/mecanum-motor-console
~/.venvs/orange-pi-vision/bin/python vision/run_detection.py \
  --source rtsp://127.0.0.1:8554/camera \
  --detector color \
  --headless
```

SSH 中观察状态：

```bash
tail -f vision/outputs/detections.jsonl
ls -lh vision/outputs
```

按 `Ctrl+C` 停止程序。识别结果图片和日志都会保存在香橙派的
`vision/outputs` 目录中。

## 人脸跟踪与中心偏差

下载 YuNet 模型到香橙派：

```bash
cd ~/mecanum-motor-console
mkdir -p vision/models
wget -O vision/models/face_detection_yunet_2023mar.onnx \
  https://github.com/opencv/opencv_zoo/raw/main/models/face_detection_yunet/face_detection_yunet_2023mar.onnx
```

启动人脸跟踪：

```bash
cd ~/mecanum-motor-console
~/.venvs/orange-pi-vision/bin/python vision/run_face_tracking.py \
  --source rtsp://127.0.0.1:8554/camera \
  --inference-width 320 \
  --detect-every 2 \
  --monitor-port 8090 \
  --headless
```

电脑浏览器打开实时监控页面：

```text
http://192.168.1.215:8090/
```

页面通过“单请求只取最新帧”的方式显示实时画面，不使用会积压旧帧的长连接
MJPEG 播放。采集线程只保留最新帧，并启用 FFmpeg 低延迟参数，避免处理积压旧帧。
画面会显示屏幕中心十字、
人脸框、人脸中心点，以及人脸中心到屏幕中心的连线。页面下方同步显示
当前 FPS、`dx` 和 `dy`。

接口：

- `/frame.jpg`：当前最新标注帧，页面会自动请求下一帧。
- `/stream.mjpg`：兼容旧客户端的连续标注视频流。
- `/api/status`：最新人脸状态和中心偏差 JSON。
- `/healthz`：进程健康检查。

输出文件：

- `vision/outputs/face_tracking.jsonl`：人脸框、置信度、脸中心、屏幕中心、`dx/dy`。
- `vision/outputs/face_annotated_*.jpg`：中心十字、人脸框和偏差标注。

`dx > 0` 表示人脸在画面中心右侧，`dy > 0` 表示人脸在画面中心下方。
这两个值是图像像素偏差，不是实际厘米距离。

## 舵机 PWM 单通道测试

舵机测试程序只使用 Linux PWM sysfs，不连接 STM32，也不发送底盘命令。
默认测试 `PWM3`，对应交接文档中的顶部调试排针 `TX/PH0`。程序启动先输出
1500 us 中位脉冲，只允许 1000--2000 us，退出时回到中位并停止 PWM。

在香橙派上先确认 PWM 复用已经开启。当前镜像需要在
`orangepi-config -> System -> Hardware` 中开启 `ph-pwm34`，并关闭
`uart0`，保存后重启。检查到 `pwmchip0` 后再运行：

```bash
cd ~/mecanum-motor-console
sudo ~/.venvs/orange-pi-vision/bin/python vision/run_servo_test.py \
  --channel 3 \
  --pulse-us 1500
```

确认舵机悬空、独立 5 V 供电且共地后，再做小范围方向测试：

```bash
sudo ~/.venvs/orange-pi-vision/bin/python vision/run_servo_test.py \
  --channel 3 \
  --sequence 1400,1500,1600,1500 \
  --hold-seconds 2
```

垂直舵机接通后使用 `--channel 4`。不要把 `P28/P30` 当作物理排针号，
也不要把舵机电源接到 Orange Pi 3.3 V。

## 双舵机浏览器上位机

舵机控制后端运行在香橙派，Windows 电脑只需用浏览器访问页面。后端独占
PWM3/PWM4，启动时两个舵机回到 1500 us；浏览器断开超过约 2 秒后，两个
舵机回中并停止 PWM。上位机按图片中的标称关系使用 `500--2500 us`：
`500 us=-90°`、`1500 us=0°`、`2500 us=+90°`，总行程为 180°。

`500--2500 us` 是舵机商品描述的标称范围，不代表每个舵机和机械结构都能
安全到达极限。第一次调试仍建议从 `1000--2000 us` 开始，逐步扩大范围；
底层单舵机测试程序默认也仍限制在 `1000--2000 us`。

手动启动：

```bash
cd ~/mecanum-motor-console
sudo ~/.venvs/orange-pi-vision/bin/python vision/servo_server.py \
  --host 0.0.0.0 --port 8091
```

Windows 浏览器打开：

```text
http://192.168.1.215:8091/
```

页面现在还显示摄像头标注画面、FPS、检测状态、`dx/dy` 和置信度。
点击“自动跟踪”后，水平/垂直偏差会经过 15 像素死区、平滑和单次限速，
再分别更新 PWM3/PWM4。默认方向为 `PAN_INVERT=false`、
`TILT_INVERT=true`；云台安装方向相反时，应在配置中调整方向。
丢脸后先保持约 1.5 秒，再以较慢速度回中。

闭环误差采用摄像头视角坐标：水平向左为正，垂直向上为正。

页面支持方向键、按钮、脉宽滑块、回中、停止和自动跟踪。安装开机服务：

```bash
sudo cp vision/orangepi-servo.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now orangepi-servo.service
sudo systemctl status orangepi-servo.service
```

## 可选：设置开机自启

仓库提供了 `orangepi-vision.service`，确认手动运行正常后再安装：

```bash
sudo cp vision/orangepi-vision.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now orangepi-vision.service
sudo journalctl -u orangepi-vision.service -f
```

## Windows 开发环境

在仓库根目录执行：

```powershell
py -3 -m venv .venv-vision
.\.venv-vision\Scripts\Activate.ps1
python -m pip install -r vision\requirements.txt
```

如果 PowerShell 禁止激活脚本：

```powershell
Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
```

## 第一步：只验证视频（Windows）

```powershell
$env:CAMERA_URL = "rtsp://192.168.1.215:8554/camera"
python vision\camera_preview.py
```

窗口中按 `q` 退出。没有图形桌面时保存一张图片：

```powershell
python vision\camera_preview.py --headless --output vision\outputs\preview.jpg
```

## 第二步：采集样本

```powershell
python vision\capture_frames.py --count 50 --interval 0.5
```

无窗口运行：

```powershell
python vision\capture_frames.py --headless --count 50
```

## 第三步：运行识别框架

默认使用空检测器，只验证采集、FPS、保存和日志：

```powershell
python vision\run_detection.py
```

运行颜色区域示例检测器，默认检测 HSV 中的橙红色区域：

```powershell
python vision\run_detection.py --detector color
```

无窗口运行：

```powershell
python vision\run_detection.py --detector color --headless
```

输出位于 `vision\outputs`：

- `raw_*.jpg`：原始帧。
- `annotated_*.jpg`：画框、中心点和 FPS 后的帧。
- `detections.jsonl`：每帧一条识别记录。

## 后续接入 YOLO 的位置

不要修改 `camera.py` 和 `run_detection.py` 的采集/输出边界，只在
`detector.py` 中新增一个实现 `detect(frame)` 的检测器，再在
`create_detector()` 注册。模型推理失败时必须返回空列表或明确报错，不能
把失败当成运动指令。

任何底盘控制逻辑都应当放在单独的输出模块，并且默认关闭。需要联调时，
还要遵守 `VISION_RECOGNITION_HANDOFF.md` 中关于 WebSocket、STOP、刷新周期
和轮子悬空的规则。
