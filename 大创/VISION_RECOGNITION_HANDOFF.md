# 香橙派视觉识别项目交接文档

更新时间：2026-09-28

本文面向一台完全没有配置过环境的新电脑。目标是：新电脑通过 Tailscale 连接已经配置好的香橙派 Zero 3，读取 USB 摄像头视频，先在电脑上完成视觉识别，再决定是否把识别结果发送给底盘。

本文不要求重新烧录香橙派系统，也不要求第一阶段修改 STM32 固件。

## 1. 先看结论

系统结构：

~~~text
Logitech C270 USB 摄像头
        |
Orange Pi Zero 3
  FFmpeg + MediaMTX
        |                         \
        | RTSP: 8554/camera        \ HTTPS WebRTC: /camera/
        v                           v
新电脑上的视觉程序         手机/浏览器实时预览
        |
        | 后续再通过 WebSocket 发送控制命令
        v
Orange Pi WebSocket bridge
        |
        | /dev/ttyS5, 115200 8N1
        v
STM32F407 USART3 (PC10/PC11)
~~~

推荐开发顺序：

1. 新电脑能通过 Tailscale SSH 登录香橙派。
2. 新电脑能用 RTSP 读取摄像头画面。
3. 在新电脑上完成 OpenCV 取帧、保存图片、画框和识别算法。
4. 识别稳定后，再设计识别结果到运动控制的接口。
5. 最后才允许视觉程序发送底盘命令。

视觉程序一开始只读视频，不发送 DRV、W、A 等运动命令。实车测试必须先保持 STOP，并把轮子悬空。

## 2. 已完成的香橙派状态

香橙派型号：Orange Pi Zero 3，64 位 Linux。

| 功能 | 地址或参数 |
| --- | --- |
| 控制网页 | http://127.0.0.1:8088 |
| WebSocket 控制桥 | ws://127.0.0.1:8766 |
| MediaMTX RTSP | rtsp://127.0.0.1:8554/camera |
| MediaMTX WebRTC 页面 | http://127.0.0.1:8889/camera/ |
| 摄像头设备 | 通常为 /dev/video1 |
| 摄像头采集 | 320x240 @ 15 FPS |
| 编码 | H.264，约 400 kbit/s |
| 串口 | /dev/ttyS5，115200 8N1 |
| 开机启动 | mecanum-console.service |

摄像头是 Logitech C270。设备编号可能因 USB 插拔顺序改变，不要在新电脑上假设设备一定是 /dev/video1；设备编号只在香橙派上使用。

香橙派已经安装：

- mecanum-motor-console：网页、WebSocket 桥和启动脚本。
- MediaMTX：RTSP/WebRTC 转发。
- FFmpeg：从 V4L2 摄像头编码为 H.264。
- Tailscale：远程访问和 HTTPS。
- Python 虚拟环境：用于串口桥和网页服务。

## 3. 新电脑需要安装的环境

以下步骤按 Windows 编写。新电脑不需要安装 STM32、Keil 或香橙派交叉编译环境，除非后续明确要改固件。

### 3.1 必装软件

安装官方版本：

1. Tailscale Windows 客户端。
2. Git for Windows。
3. Python 3.11 或更新的 3.x 版本，安装时勾选 Add Python to PATH。
4. VS Code，推荐但不是强制。
5. FFmpeg，只有需要使用 ffplay 或 ffprobe 检查 RTSP 时才必须安装。

PowerShell 检查：

~~~powershell
tailscale version
git --version
py --version
~~~

如果 py 不存在，后续命令中的 py 改成 python。

### 3.2 登录 Tailscale

新电脑必须登录与香橙派相同的 Tailscale 账号或同一个 tailnet：

~~~powershell
tailscale status
tailscale ping orangepizero3
~~~

看到香橙派为 active 或 ping 成功，才继续。

香橙派最后一次记录的 Tailscale 信息：

~~~text
主机名：orangepizero3
最后已知 IPv4：100.109.90.22
MagicDNS：orangepizero3.tail6cea4e.ts.net
~~~

IP 地址可能变化。实际使用时优先从 tailscale status 或 tailscale ip -4 获取当前地址，不要永久写死 100.109.90.22。

## 4. 获取项目代码

推荐把项目放在没有中文和空格的路径，例如 D:\robot\my_code：

~~~powershell
New-Item -ItemType Directory -Force D:\robot | Out-Null
Set-Location D:\robot
git clone https://github.com/citiao1/my_code.git
Set-Location D:\robot\my_code
~~~

主要文件：

~~~text
mecanum-motor-console\       当前遥控网页、串口桥和摄像头服务说明
STM32_ORANGE_PI_UART_HANDOFF.md  STM32 与香橙派串口协议
ORANGE_PI_HOST_HANDOFF.md     完整的底盘上位机协议与安全规则
VISION_RECOGNITION_HANDOFF.md 本文
~~~

mecanum-motor-console\bridge\.venv 是旧电脑留下的 Windows 虚拟环境，不要复制或依赖它。每台新电脑都要自己创建虚拟环境。

## 5. 建立视觉 Python 环境

在项目根目录执行：

~~~powershell
Set-Location D:\robot\my_code
py -3 -m venv .venv-vision
.\.venv-vision\Scripts\Activate.ps1
python -m pip install --upgrade pip
python -m pip install numpy opencv-python matplotlib
~~~

如果 PowerShell 禁止激活脚本，只对当前用户执行一次：

~~~powershell
Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
~~~

然后重新激活：

~~~powershell
Set-Location D:\robot\my_code
.\.venv-vision\Scripts\Activate.ps1
~~~

第一阶段只做视频读取和传统 OpenCV 处理。确认视频读取无误后，如果项目决定采用 YOLO，再安装：

~~~powershell
python -m pip install ultralytics
~~~

如果需要训练模型，还要根据电脑是否有 NVIDIA 显卡单独选择 PyTorch 版本，不要复制另一台电脑的 torch 或虚拟环境目录。

## 6. 先验证香橙派

### 6.1 SSH 登录

PowerShell 中执行：

~~~powershell
ssh orangepi@100.109.90.22
~~~

如果已经配置 SSH 别名，也可以：

~~~powershell
ssh orangepi
~~~

登录香橙派后检查：

~~~bash
tailscale ip -4
sudo systemctl status mecanum-console.service --no-pager
tailscale serve status
~~~

如果服务状态是 active (running)，不要再手工执行 bash orange-pi/start.sh，否则可能启动两套服务并争用摄像头或串口。

常用维护命令：

~~~bash
sudo systemctl restart mecanum-console.service
sudo systemctl stop mecanum-console.service
sudo journalctl -u mecanum-console.service -n 100 --no-pager
sudo journalctl -u mecanum-console.service -f
~~~

按 Ctrl+C 只会退出日志查看，不会停止 systemd 服务。

### 6.2 浏览器验证 WebRTC 图传

在新电脑浏览器打开：

~~~text
https://orangepizero3.tail6cea4e.ts.net/camera/
~~~

也可以打开控制页面：

~~~text
https://orangepizero3.tail6cea4e.ts.net/
~~~

控制页面能打开、图传能显示，只能证明 WebRTC 页面正常，不代表 Python/OpenCV 已经能读取视频。视觉程序建议使用下一节的 RTSP。

## 7. 视觉程序的视频输入

### 7.1 推荐使用 RTSP

视觉程序在新电脑上使用：

~~~text
rtsp://100.109.90.22:8554/camera
~~~

把 100.109.90.22 替换为当前 tailscale status 显示的香橙派 IP。RTSP 通过 Tailscale 在 tailnet 内传输，不需要把 8554 映射到公网。

如果主机名可以解析，也可以使用：

~~~text
rtsp://orangepizero3:8554/camera
~~~

### 7.2 用 FFmpeg 检查 RTSP

~~~powershell
ffprobe -rtsp_transport tcp rtsp://100.109.90.22:8554/camera
~~~

能够看到 H.264 视频流信息，说明网络和 MediaMTX 正常。

也可以直接预览：

~~~powershell
ffplay -rtsp_transport tcp -fflags nobuffer -flags low_delay rtsp://100.109.90.22:8554/camera
~~~

关闭 ffplay 按 q。

### 7.3 第一个 OpenCV 取帧测试

在项目根目录创建 vision\camera_preview.py：

~~~python
import os
import cv2

url = os.environ.get(
    "CAMERA_URL",
    "rtsp://100.109.90.22:8554/camera",
)

cap = cv2.VideoCapture(url, cv2.CAP_FFMPEG)
if not cap.isOpened():
    raise RuntimeError(f"无法打开摄像头流: {url}")

while True:
    ok, frame = cap.read()
    if not ok:
        print("读取视频帧失败")
        break

    cv2.imshow("Orange Pi camera", frame)
    if cv2.waitKey(1) & 0xFF == ord("q"):
        break

cap.release()
cv2.destroyAllWindows()
~~~

如果路径中没有 vision 目录，先创建：

~~~powershell
New-Item -ItemType Directory -Force vision | Out-Null
~~~

运行：

~~~powershell
Set-Location D:\robot\my_code
.\.venv-vision\Scripts\Activate.ps1
python vision\camera_preview.py
~~~

如果 OpenCV 打开 RTSP 失败，先用 ffplay 验证；ffplay 能播而 OpenCV 不能播时，再尝试去掉 cv2.CAP_FFMPEG 或安装包含 FFmpeg 的 OpenCV wheel。不要先改香橙派配置。

## 8. 视觉识别开发建议

第一阶段建议只完成：

1. 连续读取 RTSP 帧。
2. 每隔固定帧保存一张原图和一张标注图。
3. 在画面上显示 FPS、时间戳和识别结果。
4. 对识别结果进行日志记录。
5. 摄像头断开时能重连，不让程序死循环占满 CPU。

建议目录：

~~~text
vision/
  camera_preview.py       最小取帧测试
  capture_frames.py       保存原始图片
  detector.py             目标检测接口
  run_detection.py        主循环与画面显示
  requirements.txt        视觉环境依赖
  models/                 模型文件
  outputs/                识别结果和调试图片
~~~

视觉程序要把采集、识别、显示、控制输出分开。识别失败或网络中断时，默认结果应是无目标，不能默认让车运动。

### 8.1 不要把 WebRTC 页面当作 OpenCV 输入

https://.../camera/ 是给浏览器播放的 WebRTC 页面。OpenCV 通常不能直接把这个 HTML 页面当成视频源。

因此：

- 浏览器预览使用 HTTPS /camera/。
- Python/OpenCV 识别使用 RTSP rtsp://...:8554/camera。
- 不要让 OpenCV 去读取 GitHub Pages 页面或 iframe 地址。

### 8.2 识别程序暂时不要控制底盘

视觉识别初期只输出日志或窗口标注。需要控制时，必须通过香橙派 WebSocket：

~~~text
wss://orangepizero3.tail6cea4e.ts.net/ws
~~~

命令仍是逐行 ASCII 文本，例如：

~~~text
STOP
SPEED,10
DRV,0,0,0
~~~

底盘通信的完整协议见 STM32_ORANGE_PI_UART_HANDOFF.md 和 ORANGE_PI_HOST_HANDOFF.md。

视觉程序不要直接连接 STM32，也不要在新电脑上试图打开香橙派的 /dev/ttyS5。香橙派是串口的唯一拥有者。

## 9. 安全和实车测试规则

- 新电脑和香橙派必须登录同一 Tailscale tailnet。
- 不要把 RTSP 8554、WebSocket 8766、网页 8088 或 WebRTC 8889/8189 直接暴露到公网。
- 不要把控制 WebSocket 写进公开 GitHub Pages 代码中的固定密码；当前协议没有鉴权。
- 视觉识别程序启动时默认只读视频，不自动发 DRV。
- 新增控制逻辑时，连接成功先发送 STOP，并等待新遥测。
- 运动命令必须按约 100 ms 刷新；停止刷新后 STM32 会在约 300 ms 内自动停车。
- 页面关闭、网络断开、视频断开、识别程序异常退出时，必须发送 STOP，并保留 STM32 自带看门狗。
- 第一次联调轮子悬空，先低速 SPEED,5，确认轮序和方向后再落地。

## 10. 断电重启后的操作

香橙派重新上电后等待 1–2 分钟，然后在新电脑检查：

~~~powershell
tailscale ping orangepizero3
ssh orangepi
~~~

登录后：

~~~bash
sudo systemctl is-active mecanum-console.service
tailscale serve status
~~~

服务正常时，新电脑不需要在 SSH 窗口中运行任何长期命令，直接启动视觉程序即可。

如果服务未运行：

~~~bash
sudo systemctl restart mecanum-console.service
sudo journalctl -u mecanum-console.service -n 100 --no-pager
~~~

## 11. 故障排查

### Tailscale ping 失败

两台电脑上都执行：

~~~powershell
tailscale status
tailscale netcheck
~~~

确认两台设备使用同一账号、Tailscale 状态为在线。先解决 Tailscale，不要先改 RTSP 或 OpenCV。

### 网页能打开，RTSP 不能打开

检查：

1. 香橙派服务是否为 active (running)。
2. 新电脑是否使用当前 Tailscale IP。
3. ffprobe -rtsp_transport tcp 是否指定正确的 /camera 路径。
4. 香橙派是否仍识别到 USB 摄像头。

香橙派上执行：

~~~bash
lsusb
ls -l /dev/video*
sudo journalctl -u mecanum-console.service -n 100 --no-pager
~~~

### RTSP 有画面但很卡

当前配置是低分辨率低码率的初版，优先保持识别稳定。不要一开始提高分辨率；先确认算法处理速度，再逐步尝试：

- 降低识别频率而不是丢弃所有帧。
- 只处理最新帧，避免处理队列堆积造成延迟越来越高。
- 将模型推理放到独立线程或进程。
- 记录采集 FPS、推理耗时和端到端延迟。

### OpenCV 窗口打不开

这通常是新电脑的图形环境或远程桌面问题，不一定是视频流问题。先用 ffplay 验证视频，再把 OpenCV 程序改为保存 JPEG 或只打印帧尺寸：

~~~python
print(frame.shape)
cv2.imwrite("outputs/test.jpg", frame)
~~~

## 12. 完成标准

- [ ] Tailscale 已登录同一 tailnet。
- [ ] tailscale ping orangepizero3 成功。
- [ ] SSH 能登录香橙派。
- [ ] 香橙派 mecanum-console.service 为 active。
- [ ] 浏览器能打开 HTTPS 图传。
- [ ] ffprobe 能识别 RTSP H.264 流。
- [ ] OpenCV 能连续读取并显示或保存视频帧。
- [ ] 视觉程序能在摄像头断开后提示错误并退出或重连。
- [ ] 没有明确授权前，视觉程序不会发送运动命令。
- [ ] 实车测试前已阅读 STM32_ORANGE_PI_UART_HANDOFF.md 的看门狗和接线规则。

## 13. 相关文件

| 文件 | 用途 |
| --- | --- |
| mecanum-motor-console/README.md | 遥控网页和服务总览 |
| mecanum-motor-console/orange-pi/README.md | 香橙派安装、开机自启和访问方式 |
| mecanum-motor-console/orange-pi/start.sh | 网页、串口桥、MediaMTX、FFmpeg 启动脚本 |
| mecanum-motor-console/bridge/motor_vofa_bridge.py | 串口/WebSocket 桥接实现 |
| STM32_ORANGE_PI_UART_HANDOFF.md | STM32 串口协议、命令和安全规则 |
| ORANGE_PI_HOST_HANDOFF.md | 完整底盘上位机协议和遥控逻辑 |
| VISION_RECOGNITION_HANDOFF.md | 本文，新电脑视觉环境交接 |

