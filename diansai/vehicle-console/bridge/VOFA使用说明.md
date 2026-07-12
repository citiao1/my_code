# BLE 桥接与 VOFA+ 使用说明

## 启动

在 `D:\my_code\my_code\diansai` 打开 PowerShell，运行：

```powershell
powershell -ExecutionPolicy Bypass -File .\vehicle-console\bridge\start-bridge.ps1
```

第一次运行会创建隔离的 Python 环境并安装 `bleak` 与 `websockets`，需要等待几十秒。之后会自动：

- 扫描并连接带有 FFE0 服务的 BLE 串口模块。
- 在 `127.0.0.1:1347` 启动 VOFA+ TCP 数据端口。
- 在 `127.0.0.1:8766` 启动网页控制 WebSocket。
- 在 `http://127.0.0.1:8765` 启动网页上位机。

如果扫描到多个 FFE0 设备，输入设备前面的数字并回车。运行期间不要再用网页的“BLE 蓝牙”模式或 `serial.keysking.com` 连接同一个模块。

## 网页控制

打开 `http://127.0.0.1:8765`，连接方式选择“本地桥接”，点击“连接桥接”。

## VOFA+ 配置

1. 打开 VOFA+ 并新建工程。
2. 数据引擎选择 `TCPClient`。
3. 远端地址填写 `127.0.0.1`。
4. 远端端口填写 `1347`。
5. 协议引擎选择 `FireWater`。
6. 点击连接。

FireWater 的格式是一个可选前缀加一组纯数字：

```text
vehicle:ch0,ch1,ch2,...,ch28\n
```

冒号后不能再写 `speedL:` 之类的字段名，否则只会成功解析第一个通道。桥接程序当前输出 29 个通道，固定对应关系如下：

```text
ch0  time_ms       ch1  enable       ch2  link
ch3  yaw_deg       ch4  pitch_deg    ch5  roll_deg
ch6  speed_left    ch7  speed_right
ch8  target_left   ch9  target_right
ch10 error_left    ch11 error_right
ch12 encoder_left  ch13 encoder_right
ch14 pwm_left      ch15 pwm_right
ch16 left_kp       ch17 left_ki      ch18 left_kd
ch19 right_kp      ch20 right_ki     ch21 right_kd
ch22 target_yaw_rate_deg_s
ch23 yaw_rate_deg_s
ch24 yaw_rate_error_deg_s
ch25 yaw_correction_m_s
ch26 yaw_loop_enabled
ch27 battery_voltage_v
ch28 battery_adc_raw
```

速度、目标和误差的单位都是 `m/s`。在波形控件中添加 `ch6`、`ch7`、`ch8`、`ch9`，即可同时观察左右轮实际速度与目标速度。

不要把 `ch0` 加入速度波形。它是从上电开始累计的毫秒计时值，数值会越来越大，只用于检查数据包时间。

当前霍尔编码器实测标定值：左轮 `1558.3 count/rev、7514 count/m`，右轮 `1557.4 count/rev、7263 count/m`。物理左轮对应 TIM3，物理右轮对应 TIM2，固件已经交换为正确的左右显示。

左右速度环采用独立增量式 PID：`deltaPWM = Kp*(e-e1) + Ki*e + Kd*(e-2e1+e2)`，再通过 `PWM += deltaPWM` 累加输出。停车、断链或修改参数时会清空累计输出。

## 速度环初次调参

1. 先把车架空，确保两个轮子离地。
2. 固件和网页默认使用最高轮速 `200 mm/s`、左右轮 `Kp=4000, Ki=800, Kd=0`；连接后先点击一次“应用参数”。
3. 短按前进，确认 `ch8/ch9` 和 `ch6/ch7` 的正负号一致。若目标为正而实际速度为负，立即急停，不要继续加参数。
4. 逐步增加 Kp，直到响应足够快但不持续振荡。
5. 再从较小的 Ki 开始增加，用于消除稳态误差。速度环通常先保持 `Kd=0`。

## 角速度环初次调试

1. 角速度环默认关闭。先保持关闭，按住左转，确认 `ch22` 与 `ch23` 同号；右转时两者也应同号。
2. 若正负号正确，设置 `Kp=0.001, Ki=0, Kd=0`、最大角速度 `120 deg/s`，勾选启用并应用。
3. 波形同时观察 `ch22`、`ch23`、`ch24`、`ch25`。先增加 Kp，角速度能跟随但不持续振荡后，再考虑少量 Ki。
4. 若实际角速度与目标反号或启用后车辆突然满差速，立即急停并关闭角速度环。
5. `ch29` 为 MPU6050 状态：`1` 正常，`0` 表示初始化失败或连续读取失败，固件会在停车时自动重试。

## 电池 ADC

电池采样使用 `PB0 / ADC1_IN8`。原理图分压为 VIN 经 `100k` 到 PB0、PB0 经 `10k` 到地，固件按 `11:1` 换算。`ch27` 是估算电池电压，`ch28` 是 12 位 ADC 原始值。首次使用需用万用表对比校准，不启用自动欠压停车。

## 常见问题

- `no FFE0 device found`：确认蓝牙模块已供电，并关闭其他正在连接该模块的网页或应用。
- 网页显示桥接未运行：先保持桥接 PowerShell 窗口开启，再刷新网页。
- VOFA+ 连接不上：确认数据引擎是 TCPClient，不是 SerialPort，并检查端口是否为 1347。
- 关闭桥接：在桥接 PowerShell 窗口按 `Ctrl+C`，固件会在 400ms 内因命令超时停车。
