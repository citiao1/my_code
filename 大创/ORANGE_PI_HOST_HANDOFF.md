# C30D 麦轮底盘与香橙派上位机交接说明

更新时间：2026-08-10

本文面向香橙派上位机开发者，描述当前 `mecanum_motor_test` 固件、网页遥控器和本地桥接程序已经实现的通信接口与遥控逻辑。协议以当前源码为准，不包含尚未实现的功能。

## 1. 先看结论

- 主控：STM32F407VET6，CubeMX + Keil MDK-ARM V5，裸机主循环。
- 香橙派串口：USART3，`115200 8N1`，无硬件流控，PC10=TX、PC11=RX。
- 蓝牙串口：USART2，`9600 8N1`，无硬件流控，PD5=TX、PD6=RX。
- 两路串口共用同一套命令解析；ACK、ERR、TEL 会同时广播到 USART2 和 USART3。
- 香橙派链路：STM32 USART3 -> 香橙派 `/dev/ttyS5`；蓝牙链路：STM32 USART2 -> FFE0 蓝牙串口模块 -> BLE Central。
- BLE 服务：FFE0；通知：FFE1；写入优先 FFE1，不可写时回退 FFE2。
- 应用协议：可打印 ASCII 文本，一行一条命令，以 `\n` 结束；收到的 `\r` 会忽略。
- 遥控核心命令：`DRV,forward_rpm,left_rpm,yaw_dps`。
- 上位机应以约 100 ms 周期刷新运动命令。固件连续 300 ms 没收到运动刷新会自动停车。
- 遥测以 `TEL,...\r\n` 输出，周期 200 ms，即 5 Hz。
- BLE 同一时间只能被一个 Central 占用。香橙派连接 BLE 后，手机不能同时直连 BT05。
- 当前 VOFA TCP 端口只输出波形，不接收控制命令。

## 2. 当前文件位置

| 内容 | 路径 |
| --- | --- |
| CubeMX 配置源 | `mecanum_motor_test/mecanum_motor_test.ioc` |
| Keil 工程 | `mecanum_motor_test/MDK-ARM/mecanum_motor_test.uvprojx` |
| 现有 HEX | `mecanum_motor_test/MDK-ARM/mecanum_motor_test/mecanum_motor_test.hex` |
| 命令解析 | `mecanum_motor_test/App/Src/command.c` |
| 车辆状态机与通信看门狗 | `mecanum_motor_test/App/Src/vehicle.c` |
| 麦轮解算 | `mecanum_motor_test/Algorithm/Src/mecanum.c` |
| 遥测组帧 | `mecanum_motor_test/App/Src/telemetry.c` |
| UART DMA 收发 | `mecanum_motor_test/Bsp/Src/serial_dma.c` |
| 网页遥控逻辑 | `mecanum-motor-console/app.js` |
| 可复用 Python 桥接 | `mecanum-motor-console/bridge/motor_vofa_bridge.py` |
| 桥接测试 | `mecanum-motor-console/bridge/test_motor_vofa_bridge.py` |
| VOFA 通道说明 | `mecanum-motor-console/VOFA使用说明.md` |

当前手机页面：<https://citiao1.github.io/my_code/?mobile=1>

## 3. 推荐的香橙派架构

香橙派应成为唯一的硬件连接拥有者，再把状态分发给本机 UI、局域网网页或其他进程：

```text
STM32 USART3 (PC10/PC11)
    |
Orange Pi /dev/ttyS5
    |
Orange Pi hardware transport
    |
line buffer + protocol parser + latest vehicle state
    |                         |
control UI / WebSocket       optional VOFA TCP output
```

推荐直接复用 `motor_vofa_bridge.py` 的 BLE/串口连接、断线重扫、16 字节分片和行缓冲逻辑。若上位机需要自己的界面，保留硬件传输层和协议解析层，把网页/VOFA部分替换掉即可。

不要让两个进程同时打开同一个串口，也不要让香橙派和手机同时连接同一个 BLE 模块。需要手机遥控香橙派时，应让手机通过局域网连接香橙派，上位机再独占 BLE。

## 4. 物理与传输接口

### 4.1 UART

| 参数 | 当前值 |
| --- | --- |
| 外设 | USART3 |
| 引脚 | PC10 TX、PC11 RX |
| 波特率 | 115200 |
| 数据位 | 8 |
| 停止位 | 1 |
| 校验 | None |
| 流控 | None |
| RX | Receive-to-IDLE DMA，DMA1 Stream1 |
| TX | DMA 队列，DMA1 Stream3 |

直接用香橙派 GPIO UART 或 USB-UART 前，必须根据原理图确认电平并共地；不要把 5 V 串口电平直接接到 STM32 或香橙派 GPIO。蓝牙模块继续使用 USART2 的 PD5/PD6，不要将 USART3 或其他 TX 并联到同一信号线上。

串口接收规则：

- 一行最长 63 个可打印 ASCII 字符，不包含行尾。
- `\n` 触发命令解析，`\r` 被忽略，因此发送 `\n` 或 `\r\n` 都可以。
- 非打印字符会被丢弃。
- 超长行返回 `ERR,LINE_TOO_LONG`。
- 命令进入解析器后会去除首尾空白并转为大写。
- 协议没有 CRC、序号和请求 ID；`TEL`、`ACK`、`ERR` 可能交错到达。

### 4.2 BLE

| 项目 | UUID |
| --- | --- |
| UART Service | `0000ffe0-0000-1000-8000-00805f9b34fb` |
| Notify | `0000ffe1-0000-1000-8000-00805f9b34fb` |
| 首选 Write | FFE1 本身可写时使用 FFE1 |
| 备用 Write | `0000ffe2-0000-1000-8000-00805f9b34fb` |

连接顺序：

1. 扫描广告中包含 FFE0 Service 的设备。
2. 连接 GATT。
3. 先订阅 FFE1 notification。
4. 如果 FFE1 有 `write` 或 `write-without-response`，直接用 FFE1；否则查找 FFE2。
5. 建立行缓冲，通知数据可能拆行或粘包，必须累计到 `\n` 再解析。

该模块在实际测试中写入 17 字节可能断链，因此当前桥接和网页都使用：

```text
每个 BLE 写包最多 16 字节
分片之间等待 20 ms
完整命令最后追加 \n
```

香橙派实现必须保留这个限制，不要依赖系统报告的 20 字节 ATT payload。

## 5. 最小可用命令集

香橙派第一版只需要实现下面五类命令：

| 命令 | 作用 | 典型响应 |
| --- | --- | --- |
| `STOP` | 立即停止并清空各控制环 | `ACK,STOP` |
| `SPEED,n` | 设置速度档位，范围 5-120 | `ACK,SPEED,n` |
| `DRV,f,l,y` | 同时设置前后、左右、角速度 | `ACK,DRV,f,l,y` |
| `STATUS` | 立即请求一帧遥测 | 一帧 `TEL,...` |
| `GYROCAL` | 停车并重新初始化/校准陀螺仪 | `ACK,GYROCAL` 或 `ERR,MPU6050_OFFLINE` |

建议连接成功后依次发送：

```text
STOP
SPEED,30
STATUS
```

不要在重连后恢复断线前的非零 `DRV`。必须从零状态重新等待用户输入。

## 6. `DRV` 遥控协议

格式：

```text
DRV,forward_rpm,left_rpm,yaw_dps
```

语义：

| 分量 | 正值 | 负值 | 单位 |
| --- | --- | --- | --- |
| `forward_rpm` | 前进 | 后退 | 等效轮速 RPM |
| `left_rpm` | 左移 | 右移 | 等效轮速 RPM |
| `yaw_dps` | 左旋 | 右旋 | °/s |

三个分量的绝对值都不能大于当前 `SPEED`。例如 `SPEED,30` 后：

```text
DRV,30,0,0       前进
DRV,0,30,0       左移
DRV,0,0,30       以 30 °/s 左旋
DRV,21,21,-15    右前方向平移，同时右旋
DRV,0,0,0        等价于停车
```

组合运动时，麦轮解算会先相加三个分量；如果任一轮超过 `SPEED`，四轮按同一比例缩放，保持运动方向与旋转比例。

车轮编号和安装位置：

```text
A = 右后 RR
B = 左后 LR
C = 右前 RF
D = 左前 LF
```

当前 X 型麦轮解算：

```text
A(RR) = forward - left + rotate
B(LR) = forward + left - rotate
C(RF) = forward + left + rotate
D(LF) = forward - left - rotate
```

## 7. 当前双摇杆逻辑

网页的速度滑块同时限定平移最大 RPM 和旋转最大 °/s，默认 30，范围 5-120。

### 7.1 左摇杆

- 上推：`forward > 0`
- 下推：`forward < 0`
- 左推：`left > 0`
- 右推：`left < 0`
- 支持任意二维方向，不是只有八方向。

### 7.2 右摇杆

- 只读取横轴。
- 左推：`yaw > 0`，左旋。
- 右推：`yaw < 0`，右旋。
- 左右摇杆可以同时操作，最终合并为同一条 `DRV`。

### 7.3 数值换算

两个摇杆都有 10% 中心死区，死区外重新线性映射到 0-100%。当前网页等价逻辑：

```text
forward = round(-left_stick_y * speed)
left    = round(-left_stick_x * speed)
yaw     = round(-right_stick_x * speed)
```

指针移动时最短约 45 ms 可更新一次；按住后每 100 ms 强制刷新一次命令。

### 7.4 松手与停车

- 松开一个摇杆但另一个还按着：立即发送剩余摇杆对应的 `DRV`。
- 两个摇杆都回中或全部松开：发送 `STOP`。
- 页面失焦、切到后台、BLE/桥接断开、点击 STOP：发送或执行停车清理。
- 固件仍有 300 ms 看门狗，所以即使上位机进程崩溃，运动也会超时停止。

香橙派版本必须保留这些行为，尤其是“窗口关闭/页面后台/手柄断开时立即 STOP”。

## 8. 航向与角速度控制行为

当前控制链：

```text
航向位置式 PD
    -> 目标角速度
角速度位置式 PI
    -> rotate_rpm
麦轮解算
    -> 四轮目标 RPM
四个增量式速度 PI
    -> 四路 PWM
```

固定参数：

| 控制环 | 参数 |
| --- | --- |
| 四轮速度环 | Kp=0.8，Ki=0.1，Kd=0，PWM 限幅 60% |
| 角速度环 | Kp=0.15，Ki=2.5，无前馈 |
| 航向环 | Kp=5.0，Kd=1.25，最大修正 80 °/s |

遥控时的实际逻辑：

- `yaw != 0`：右摇杆直接给角速度目标；航向环只跟踪当前角度，不保持旧航向。
- `yaw == 0` 且仍在平移：航向环捕获并保持当前车头角度，抑制平移过程中的意外偏航。
- 左摇杆角度只是平移方向，不会成为车头目标航向。
- 两个摇杆都归零后执行 STOP，不在静止状态持续顶住外力保持航向。
- 主动旋转结束、右摇杆回中且左摇杆仍有平移量时，会保持回中瞬间的新航向。

非零 `yaw` 必须满足 `gyro_ready=1` 且 `yaw_enabled=1`，否则固件返回 `ERR,DRV_REJECTED`。

## 9. 完整命令表

| 命令 | 说明 |
| --- | --- |
| `STOP` / `X` | 停车，返回 `ACK,STOP` |
| `W` / `S` | 按当前 SPEED 前进/后退，需要约 100 ms 刷新 |
| `A` / `D` | 按当前 SPEED 左移/右移，需要刷新 |
| `Q` / `E` | 按当前 SPEED 左旋/右旋，需要陀螺仪和角速度环 |
| `DRV,f,l,y` | 三自由度组合遥控，推荐使用 |
| `SPEED,n` | 速度档位 5-120，默认 30 |
| `PIDON,0/1` | 关闭/打开四轮速度环；关闭时会先停车 |
| `PIDRESET` | 清空四轮速度环状态 |
| `YAWON,0/1` | 关闭/打开角速度环；关闭时会先停车 |
| `YAWRESET` | 清空角速度环积分和状态 |
| `HEADON,0/1` | 关闭/打开航向环 |
| `HEADRESET` | 清空航向环状态 |
| `HEADSTEP,deg` | 在 `DRV,...,0` 模式中相对改变目标航向，范围 -170 到 +170，不能为 0 |
| `HEADHOLD` | 航向清零后保持 0°；需要重复刷新，STOP 退出 |
| `YAWZERO` | 当前积分航向清零，并复位航向环 |
| `GYROCAL` | 停车并启动 MPU6050 重新校准 |
| `ZERO` | 四路编码器累计值清零，并立即输出一帧 TEL |
| `A+`/`A-` ... `D+`/`D-` | 单轮 15% 开环点动，仅用于测试 |
| `STATUS` | 立即输出一帧 TEL |
| `PING` | 返回 `PONG` |
| `HELP` / `?` | 返回简略命令列表 |

参数已经写死，以下命令不会修改参数：

```text
PID,...      -> ERR,PID_FIXED,KP0.8,KI0.1,KD0.0
PIDLIM,...   -> ERR,PID_FIXED,KP0.8,KI0.1,KD0.0
YAWPID,...   -> ERR,YAWPID_FIXED,KP0.15,KI2.5,KFF0.0
HEADPID,...  -> ERR,HEADPID_FIXED,KP5.0,KD1.25,MAX80
```

## 10. 响应类型

固件输出是多种行的混合流：

| 前缀 | 含义 |
| --- | --- |
| `BOOT,...` | 上电标识，例如 `BOOT,MECANUM_F407` |
| `MAP,...` | 轮子映射，例如 `MAP,A=RR,B=LR,C=RF,D=LF` |
| `ACK,...` | 命令已接受 |
| `ERR,...` | 命令格式、范围、状态或硬件错误 |
| `TEL,...` | 周期遥测或 STATUS 请求结果 |
| `PONG` | PING 响应 |
| `CMD,...` | HELP 响应 |

关键错误：

| 错误 | 常见原因 |
| --- | --- |
| `ERR,DRV_RANGE,n` | DRV 任一分量超过当前 SPEED |
| `ERR,DRV_REJECTED` | 电机/速度环未就绪，或非零 yaw 时陀螺仪/角速度环未就绪 |
| `ERR,MOVE_REJECTED` | 单方向运动被当前硬件或控制环状态拒绝 |
| `ERR,MPU6050_OFFLINE` | GYROCAL 时 MPU6050 初始化失败 |
| `ERR,HEADSTEP_REJECTED` | 不在 DRV 模式、当前 yaw 非零，或相关控制环未就绪 |
| `ERR,HEADHOLD_REJECTED` | 电机、陀螺仪、角速度环或航向环未就绪 |
| `ERR,MOTOR_DISABLED` | 单轮点动时电机使能未打开 |

不要把“写入 GATT 成功”当成“命令执行成功”。上位机应继续监听对应 ACK/ERR；当前协议没有请求 ID，建议串行发送低频管理命令。

## 11. TEL 遥测格式

完整格式共 47 个值，加上开头的 `TEL` 共 48 个 CSV 字段：

```text
TEL,
time_ms,mode,enable,
encA,encB,encC,encD,
pwmA,pwmB,pwmC,pwmD,
rpmA,rpmB,rpmC,rpmD,
targetA,targetB,targetC,targetD,
battery_mV,
speed_kp_x1000,speed_ki_x1000,speed_kd_x1000,
speed_output_limit,speed_integral_limit,speed_pid_enabled,
gyro_connected,gyro_ready,gyro_calibrating,
gyro_raw_mdps,gyro_filtered_mdps,yaw_mdeg,
yaw_target_mdps,yaw_output_mrpm,
yaw_kp_x1000,yaw_ki_x1000,yaw_kff_x1000,yaw_enabled,
heading_target_mdeg,heading_feedback_mdeg,heading_error_mdeg,
heading_output_mdps,heading_kp_x1000,heading_kd_x1000,
heading_max_rate_mdps,heading_enabled,heading_holding
```

按 `TEL` 后第一个值为索引 0：

| 索引 | 字段 | 单位/说明 |
| --- | --- | --- |
| 0 | `time_ms` | STM32 `HAL_GetTick()`，ms，32 位回绕 |
| 1 | `mode` | 字符串：STOP/W/S/A/D/Q/E/DRV/HOLD/A+ 等 |
| 2 | `enable` | `Motor_IsEnabled()`，即当前电机使能输入状态 |
| 3-6 | `encA..encD` | 四路累计编码器计数，int32 |
| 7-10 | `pwmA..pwmD` | 四路电机输出百分比，范围约 -100..100 |
| 11-14 | `rpmA..rpmD` | 四路滤波后实测 RPM |
| 15-18 | `targetA..targetD` | 四路目标 RPM |
| 19 | `battery_mV` | 电池电压，mV |
| 20-22 | 速度环 Kp/Ki/Kd | 参数乘 1000 |
| 23 | `speed_output_limit` | 当前为 60，表示 PWM 百分比限幅 |
| 24 | `speed_integral_limit` | 保留字段；当前增量式实现没有实际使用它 |
| 25 | `speed_pid_enabled` | 0/1 |
| 26-28 | `gyro_connected/ready/calibrating` | MPU6050 状态，0/1 |
| 29 | `gyro_raw_mdps` | 零偏修正后、低通前角速度乘 1000 |
| 30 | `gyro_filtered_mdps` | 低通后角速度乘 1000 |
| 31 | `yaw_mdeg` | 积分航向角乘 1000，范围约 -180000..180000 |
| 32 | `yaw_target_mdps` | 角速度目标乘 1000 |
| 33 | `yaw_output_mrpm` | 角速度 PI 输出的旋转轮速分量乘 1000 |
| 34-36 | 角速度 Kp/Ki/Kff | 参数乘 1000 |
| 37 | `yaw_enabled` | 角速度环 0/1 |
| 38-40 | 航向 target/feedback/error | 角度乘 1000 |
| 41 | `heading_output_mdps` | 航向环输出的目标角速度乘 1000 |
| 42-43 | 航向 Kp/Kd | 参数乘 1000 |
| 44 | `heading_max_rate_mdps` | 航向修正限幅乘 1000 |
| 45-46 | `heading_enabled/holding` | 0/1 |

解析注意事项：

- `mode` 是字符串，其余当前字段都是十进制整数。
- 不要用固定字节偏移解析，按逗号分割并检查字段数量。
- 为兼容未来扩展，建议接受“至少 48 个字段”，解析已知前缀并忽略尾部新增字段。
- 收到字段不足或数字转换失败的 TEL 时，丢弃整帧，不要局部更新状态。
- 遥测超过 900 ms 没更新时，当前网页会禁用运动控制；香橙派建议采用相同或更严格策略。

## 12. 香橙派连接与状态机建议

```text
DISCONNECTED
  -> scan/open transport
CONNECTING
  -> BLE subscribe FFE1 or open serial
SYNCING
  -> STOP
  -> SPEED,<saved value>
  -> STATUS
WAIT_TELEMETRY
  -> require fresh TEL
READY_TRANSLATION
  -> enable=1 and speed_pid_enabled=1
READY_FULL
  -> additionally gyro_ready=1 and yaw_enabled=1
DRIVING
  -> send DRV every 100 ms
  -> any input loss/background/network error => STOP
  -> no fresh TEL within timeout => STOP and leave DRIVING
```

陀螺仪处理：

- 上电要预热 25 个样本，再在静止状态收集 200 个样本；20 ms 周期下最少约 4.5 s。
- `gyro_connected=1, gyro_ready=0, gyro_calibrating=1`：保持车辆静止并等待，不要反复发送 GYROCAL，否则会不断重启校准。
- `gyro_connected=0`，或长时间 `ready=0` 且 `calibrating=0`：发送一次 `GYROCAL`，然后等待状态变化。
- 连续 5 次 I2C 读取失败后，当前固件会把 MPU6050 标为离线，运行路径不会自动重连；`GYROCAL` 会尝试重新初始化。

## 13. 复用现有 Python 桥接

依赖文件：`mecanum-motor-console/bridge/requirements.txt`。

香橙派 BLE 示例：

```bash
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
.venv/bin/python motor_vofa_bridge.py \
  --transport ble \
  --device BT05 \
  --ws-host 127.0.0.1 \
  --tcp-host 127.0.0.1
```

香橙派 USB 串口示例：

```bash
.venv/bin/python motor_vofa_bridge.py \
  --transport serial \
  --port /dev/ttyUSB0 \
  --baud 9600
```

默认端口：

| 服务 | 默认地址 |
| --- | --- |
| 网页 WebSocket | `ws://127.0.0.1:8766` |
| VOFA FireWater TCP | `127.0.0.1:1347` |
| 本地网页启动器 | `http://127.0.0.1:8088` |

桥接程序行为：

- WebSocket 收到的每一行会转发到 STM32。
- STM32 的原始行会转发给所有 WebSocket 客户端。
- 桥接状态使用 `STATUS,0/1,detail`，这是桥接到网页的本地消息，不是 STM32 TEL。
- 最后一个 WebSocket 客户端离开时，桥接发送 STOP。
- BLE 写入失败会触发断开、清理 GATT 对象、重新扫描并连接。
- VOFA TCP 当前只发送 FireWater 数字流；TCP 客户端发来的字节会被读取后丢弃。

当前 `app.js` 会按页面的 `location.hostname` 生成桥接地址，例如从 `http://192.168.1.20:8088` 打开页面时连接 `ws://192.168.1.20:8766`。因此局域网部署时应由香橙派提供网页，并把桥接的 WebSocket 监听地址设为 `0.0.0.0`。该接口没有认证和加密，只能用于可信局域网，不能直接暴露到公网；若网页通过 HTTPS 提供，还需要为桥接配置对应的 WSS 反向代理。

现有桥接使用 Python `bleak`。代码逻辑可运行于 Linux/BlueZ，但目前没有在目标香橙派镜像上完成权限、D-Bus 和蓝牙适配器实测，这部分需要目标机验收。

## 14. VOFA 映射

当前桥接把新 TEL 转成 23 路 FireWater 数值：

```text
I0     time_ms
I1-I4  rpm A-D
I5-I8  target RPM A-D
I9-I12 PWM A-D
I13    battery_mV
I14    gyro filtered °/s
I15    yaw target °/s
I16    yaw PI output RPM
I17    integrated yaw °
I18    gyro raw corrected °/s
I19    heading target °
I20    heading feedback °
I21    heading output °/s
I22    heading error °
```

## 15. 已知限制与待处理事项

1. MPU6050 连续 5 次读取失败后不会自动恢复，需要 `GYROCAL` 或重启；香橙派应实现状态识别和单次恢复请求。
2. 手机精简 UI 隐藏了手动陀螺仪校准按钮，因此不要照搬 UI 可见控件作为完整能力列表。
3. 当前网页连接后只发送 SPEED 和 STATUS，不自动发送 GYROCAL。
4. BLE 模块只允许一个 Central；手机直连与香橙派 BLE 连接不能并存。
5. 管理命令没有请求 ID，连续快速发送多个查询时只能按行顺序和前缀判断响应。
6. UART TX 队列深度为 8，队列满时写入会失败但大部分上层调用没有重试；不要高频发送管理命令。
7. 速度环是离散增量式 PI，当前实现忽略传入的 `dt_seconds` 和 `integral_limit`；不要在上位机文档中把它描述成时间归一化位置式 PID。
8. `enable` 来自电机使能输入，不是上位机软件开关；上位机无法通过现有命令拉高物理使能。
9. 目前的左摇杆是车体坐标系平移，不是世界坐标系/场地坐标系控制。
10. 当前协议没有鉴权、加密、CRC 或重放保护，只适合本机或可信局域网。

## 16. 上位机验收清单

- 能连接 BLE FFE0 或 `/dev/ttyUSB*`，并在断电重上电后自动恢复。
- 能正确处理 BLE 拆包、粘包、空行和 `\r\n`。
- 能解析 48 字段新 TEL，并容忍尾部新增字段。
- 连接成功先 STOP，绝不恢复上次非零速度。
- 运动时约每 100 ms 发送一次最新 DRV。
- 松开摇杆、手柄断开、UI 失焦、进程退出前、网络断开时发送 STOP。
- 遥测超过设定时间未更新时停止发送运动命令并进入故障状态。
- 陀螺仪 CAL/READY/OFFLINE 三种状态显示明确。
- 非零 yaw 只有在 `gyro_ready=1` 且 `yaw_enabled=1` 时允许。
- 双摇杆同时输入时，平移与旋转三个分量都正确。
- 实车验证前先悬空检查 A/B/C/D 轮序、方向和编码器正负号。
- 分别记录“协议单元测试通过”“香橙派传输测试通过”“实车闭环测试通过”，不要混为一个结论。
