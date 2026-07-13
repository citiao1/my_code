# 车辆控制工程结构

车辆应用代码位于 `Core/Src` 和 `Core/Inc`。入口文件只负责初始化和周期调度，具体功能按模块拆分。

| 模块 | 主要职责 |
|---|---|
| `vehicle_app.c` | 系统初始化、10ms控制调度、遥测/OLED周期调度、HAL UART回调转发 |
| `vehicle_config.h` | 控制周期、PID默认值、编码器标定、限幅和超时常量 |
| `vehicle_types.h` | 车辆状态、PID状态、正方形测试状态结构体 |
| `vehicle_motor.c` | PWM方向映射、电机停车、编码器采样、左右轮速度PID |
| `vehicle_imu.c` | MPU6050初始化、零偏校准、角速度滤波、姿态和航向积分 |
| `vehicle_control.c` | 轮速/角速度/方向三级控制、方向前馈、四次90度和1米正方形测试 |
| `vehicle_comm.c` | 蓝牙串口接收、控制命令解析、TEL/STA遥测和UART中断缓冲 |
| `vehicle_battery.c` | PB0/ADC1电池电压采样和滤波 |
| `vehicle_display.c` | OLED底层时序、字库和数据显示 |
| `vehicle_gray.c` | 8路灰度传感器译码选择与ADC2采样API |

## 默认控制参数

MPU6050初始化成功后，角速度环和方向环默认开启。初始化失败时两个闭环保持关闭，停车状态下重试成功后自动开启。

```text
左右轮速度环: Kp=4000, Ki=800, Kd=0
角速度环:     Kp=0.001, Ki=0.002, Kd=0, Kff=0.001205
方向环:       Kp=4.0, Kd=0.3, Kff=1.0, 最大输出=80deg/s
```

## 8路灰度传感器接口

使用主板H1预留IO接口，模拟输出和三位译码地址均为3.3V电平。

| 信号 | MCU引脚 | H1针脚 | 作用 |
|---|---|---:|---|
| 灰度模拟输出 | PC0 / ADC2_IN10 | 6 | 读取当前选中的灰度通道 |
| 译码A | PC1 | 8 | 通道地址bit0 |
| 译码B | PC2 | 10 | 通道地址bit1 |
| 译码C | PC3 | 12 | 通道地址bit2 |
| 3.3V | - | 1 | 传感器逻辑电源 |
| GND | - | 5、18或26 | 共地 |

`VehicleGray_ReadChannel(0..7)`读取单通道，`VehicleGray_ReadAll(values)`依次读取8个通道。当前控制环尚未自动使用灰度值，安装传感器并确认黑白电平范围后，再在 `vehicle_control.c` 中加入循迹偏差计算和方向环输入切换。

## CubeMX配置

- `PB0`: `ADC1_IN8`，电池采样。
- `PC0`: `ADC2_IN10`，灰度模拟量。
- `PC1/PC2/PC3`: 推挽输出，默认低电平。
- 重新生成CubeMX代码前保留用户代码，并确认 `vehicle_*.c` 仍在Keil工程分组中。
