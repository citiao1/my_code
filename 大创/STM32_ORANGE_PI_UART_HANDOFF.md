# STM32 与香橙派串口通信交接

更新时间：2026-08-10

本文只描述香橙派 Zero 3 遥控上位机与 STM32F407 底盘之间的串口接口。协议依据当前 `mecanum_motor_test` 源码整理，目标是让 STM32 开发可以直接开始联调。

## 1. 通信链路

```text
手机或电脑浏览器
    |  HTTP + WebSocket + WebRTC（Tailscale）
香橙派 Zero 3
    |  /dev/ttyS5，3.3 V TTL UART
STM32F407 USART3（PC10/PC11）
    |  命令解析、300 ms 看门狗、底盘控制
四轮电机
```

香橙派是唯一的香橙派串口拥有者。浏览器不直接打开 STM32 串口，而是通过香橙派上的 `motor_vofa_bridge.py` 发送命令。STM32 仍保留原蓝牙 USART2，蓝牙和香橙派串口均可发送命令；固件会把 ACK、ERR 和 TEL 同时广播到两路接口。

## 2. 接线

| 香橙派 | STM32F407 | 说明 |
| --- | --- | --- |
| UART5 TX | USART3 RX，PC11 | TX 接对方 RX |
| UART5 RX | USART3 TX，PC10 | RX 接对方 TX |
| GND | GND | 必须共地 |

注意：

- 两边都是 `3.3 V TTL` 串口，不接 RS-232 电平。
- 不要把 5 V 接到任何 TX/RX 信号脚。
- 串口只需要 TX、RX、GND，不要用香橙派给 STM32 主板供电。
- 蓝牙模块继续使用 USART2：PD5=TX、PD6=RX、9600 8N1。香橙派只接 USART3 的 PC10/PC11，不要把两路 TX 并联。

## 3. 必须先统一波特率

当前两个工程的默认值不一致：

| 位置 | 当前配置 |
| --- | --- |
| 香橙派 `orange-pi/start.sh` | `/dev/ttyS5`，`115200 8N1` |
| STM32 `mecanum_motor_test.ioc` 和 `Core/Src/usart.c` | USART3，`115200 8N1` |

当前代码已在 STM32CubeMX 中新增 USART3（PC10/PC11）并配置为 `115200`，重新生成了 Keil 工程；原 USART2 蓝牙链路保持 `9600`。不要只手改生成的 `usart.c` 而不改 `.ioc`。

如果香橙派实际服务仍需临时使用其他波特率，可在香橙派执行：

```bash
sudo systemctl edit mecanum-console.service
```

填入：

```ini
[Service]
Environment=SERIAL_BAUD=9600
```

然后执行：

```bash
sudo systemctl daemon-reload
sudo systemctl restart mecanum-console.service
```

两边均为：8 数据位、1 停止位、无校验、无流控。

## 4. 数据帧格式

协议是逐行 ASCII 文本：

```text
命令正文 + \n
```

示例字节：

```text
44 52 56 2C 33 30 2C 30 2C 30 0A
 D  R  V  ,  3  0  ,  0  ,  0 LF
```

STM32 接收规则：

- `\n` 表示一条命令结束；发送 `\r\n` 也可以，因为 `\r` 会被忽略。
- 只接受 `0x20` 到 `0x7E` 的可打印 ASCII 字符。
- 一条命令最多 63 个字符，不包含换行符。
- 命令会去掉首尾空白并转成大写。
- 超长命令返回 `ERR,LINE_TOO_LONG`。
- 当前协议没有 CRC、序号或请求 ID；响应与周期遥测可能交错出现，必须逐行解析。

## 5. 遥控需要实现的命令

### 5.1 立即停车

```text
STOP\n
```

响应：

```text
ACK,STOP\r\n
```

`STOP` 会把四轮目标清零，并复位速度、角速度和航向控制状态。

### 5.2 设置速度上限

```text
SPEED,n\n
```

`n` 的范围是 5 到 120。它同时作为平移 RPM 上限和旋转 °/s 上限。

```text
SPEED,30
ACK,SPEED,30
```

越界时：

```text
ERR,SPEED_RANGE,5,120
```

### 5.3 三自由度遥控

```text
DRV,forward_rpm,left_rpm,yaw_dps\n
```

| 参数 | 正值 | 负值 | 单位 |
| --- | --- | --- | --- |
| `forward_rpm` | 前进 | 后退 | RPM |
| `left_rpm` | 左移 | 右移 | RPM |
| `yaw_dps` | 左旋 | 右旋 | °/s |

示例：

```text
DRV,30,0,0       前进
DRV,0,30,0       左移
DRV,0,0,-20      右旋
DRV,20,15,-10    前进、左移并右旋
DRV,0,0,0        等价于停车
```

每个分量的绝对值都不能超过当前 `SPEED`。成功响应：

```text
ACK,DRV,20,15,-10
```

常见错误：

```text
ERR,DRV_FORMAT,FORWARD_RPM,LEFT_RPM,YAW_DPS
ERR,DRV_RANGE,30
ERR,DRV_REJECTED
```

`ERR,DRV_REJECTED` 表示电机使能、速度环或旋转所需的陀螺仪尚未就绪。

### 5.4 请求状态

```text
STATUS\n
```

STM32 不返回 `ACK,STATUS`，而是立即返回一行 `TEL,...`。

## 6. 双摇杆换算

速度滑块值记作 `speed`，范围 5 到 120。摇杆归一化范围为 -1.0 到 +1.0，并使用 10% 中心死区。

```text
forward = round(-left_stick_y  * speed)
left    = round(-left_stick_x  * speed)
yaw     = round(-right_stick_x * speed)
```

- 左摇杆上推：前进。
- 左摇杆左推：左移。
- 右摇杆左推：左旋。
- 两个手指可以同时控制两个摇杆，最终合并为一条 `DRV`。
- 摇杆按住期间约每 100 ms 重发一次最新 `DRV`。
- 任一摇杆松开后立即发送剩余分量；两个摇杆全部松开后立即发送 `STOP`。

## 7. 看门狗与安全约束

STM32 的运动命令超时为 300 ms。只要车辆不在 STOP 状态，超过 300 ms 没有收到新的运动命令就自动停车。

上位机必须遵守：

1. 建立或重建连接后先发送 `STOP`，不得恢复断线前的非零 `DRV`。
2. 运动时每 100 ms 刷新 `DRV`，给 300 ms 看门狗留出余量。
3. 页面失焦、切入后台、网络断开、串口断开或最后一个网页退出时立即发送 `STOP`。
4. 遥测超过 900 ms 未更新时停止遥控并发送 `STOP`。
5. 非零 `yaw` 只有在陀螺仪 ready 且角速度环 enabled 时发送。
6. STM32 固件看门狗必须保留，它是最终安全层，不能只依赖网页。

## 8. STM32 输出格式

输出同样是 ASCII 行，以 `\r\n` 结束：

| 前缀 | 含义 |
| --- | --- |
| `ACK,...` | 命令已接受 |
| `ERR,...` | 格式、范围、状态或硬件错误 |
| `TEL,...` | 周期状态，当前周期 200 ms |
| `PONG` | `PING` 的响应 |

遥控页面不显示详细遥测，但后台会使用以下字段判断能否发车：

| 完整 CSV 字段下标 | 字段 | 用途 |
| --- | --- | --- |
| 0 | `TEL` | 帧类型 |
| 3 | `enable` | 电机硬件使能是否有效 |
| 26 | `speed_pid_enabled` | 四轮速度环是否启用 |
| 28 | `gyro_ready` | 是否允许旋转 |
| 38 | `yaw_enabled` | 角速度环是否启用 |

当前 `TEL` 完整格式由 `mecanum_motor_test/App/Src/telemetry.c` 生成。接收端应按逗号切分，验证数值后整帧更新；不要使用固定字节位置。

## 9. 上电联调顺序

1. 只连接 GND、TX、RX，确认两边均为 3.3 V TTL。
2. 香橙派 USART3 使用 115200 8N1；蓝牙 USART2 使用 9600 8N1，分别确认对应链路两端参数一致。
3. STM32 上电后保持车轮悬空，香橙派启动服务。
4. 发送 `PING`，应收到 `PONG`。
5. 发送 `STOP`，应收到 `ACK,STOP`。
6. 发送 `SPEED,10`，应收到 `ACK,SPEED,10`。
7. 发送 `STATUS`，应收到 `TEL,...`。
8. 车轮继续悬空，以 100 ms 周期发送 `DRV,5,0,0`，检查四轮方向。
9. 停止刷新，确认 300 ms 左右自动停车。
10. 最后再落地进行低速测试。

## 10. 对应源码

| 功能 | 文件 |
| --- | --- |
| CubeMX 双串口和 DMA 配置 | `mecanum_motor_test/mecanum_motor_test.ioc` |
| USART2/USART3 生成代码 | `mecanum_motor_test/Core/Src/usart.c` |
| DMA 接收、行缓冲、TX 队列 | `mecanum_motor_test/Bsp/Src/serial_dma.c` |
| 命令解析和 ACK/ERR | `mecanum_motor_test/App/Src/command.c` |
| 300 ms 看门狗和 DRV 执行 | `mecanum_motor_test/App/Src/vehicle.c` |
| TEL 组帧 | `mecanum_motor_test/App/Src/telemetry.c` |
| 香橙派串口/WebSocket 桥接 | `mecanum-motor-console/bridge/motor_vofa_bridge.py` |
| 桌面与手机遥控页面 | `mecanum-motor-console/index.html`、`touch-remote.js`、`touch-remote.css` |

## 11. 当前验证边界

- 香橙派 `/dev/ttyS5`、网页桥接和 USB-TTL 收发已经测试有反应。
- WebRTC 图传已经建立并能显示。
- 本文协议与 STM32 当前源码一致。
- 香橙派 GPIO UART 与 STM32 USART3 的整车闭环通信、轮序和方向仍需要按第 9 节进行实物验证；蓝牙 USART2 需要单独验证。
