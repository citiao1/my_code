# RM_Fu 与 RM_Li 项目代码分析报告

生成日期：2026-07-01  
分析对象：`RM_Fu/`、`RM_Li/RM/`  
用途：为后续基于两个 RoboMaster 控制工程开发新项目提供代码理解、复用建议和对接参考。

## 1. 总览结论

这两个项目都是基于 STM32F407 的 RoboMaster 机器人控制工程，核心任务是接收 DR16 遥控器输入、解析 IMU/CAN 电机反馈，并控制底盘、云台和射击机构。它们使用的底层技术栈基本一致：STM32 HAL、CMSIS、CAN、电机反馈解析、UART DMA + IDLE 中断、PID 控制、麦克纳姆轮运动学。

两者最大的区别在架构层：

| 项目 | 当前定位 | 调度方式 | 完整度 | 适合作为 |
| --- | --- | --- | --- | --- |
| `RM_Fu` | 分层较完整的整车控制框架 | FreeRTOS 多任务 | 较完整，包含底盘、云台、射击、视觉、IMU、编码器、统一电机抽象 | 新项目主框架参考 |
| `RM_Li/RM` | 更直接、更轻量的裸机式控制工程 | TIM2 定时中断 | 模块已写，但当前只启用模式解析和射击控制；底盘/云台调用被注释 | 算法与简化实现参考 |

如果后续要做新项目，建议以 `RM_Fu` 的分层结构和统一电机接口为主，吸收 `RM_Li` 中较简单直观的底盘坐标耦合、PID 单文件实现和快速调试方式。

## 2. 共同硬件与软件基础

### 2.1 硬件平台

两个项目都面向 STM32F407 系列控制板。典型外设如下：

- CAN：连接 DJI 电机，接收电机编码器、速度、电流、温度反馈，并发送电流控制量。
- USART3 + DMA：接收 DR16 遥控器 SBUS 数据。
- USART1 + DMA：接收 AHRS/IMU 姿态数据。
- TIM：`RM_Li` 使用 TIM2 做周期控制调度；`RM_Fu` 使用 FreeRTOS。
- IWDG：看门狗，用于防止程序卡死。
- USART6：`RM_Fu` 用作视觉模块通信。

### 2.2 共通控制方法

两个项目都采用这些核心方法：

- 遥控器解析：按 SBUS 位域解析 `ch0~ch3`、`s1/s2`、鼠标、键盘。
- 运动学：麦克纳姆轮逆运动学，把 `vx`、`vy`、`wz/vz` 转成四个轮子的目标转速。
- 闭环控制：用 PID 将目标速度/角度转成电机电流输出。
- CAN 通信：8 字节标准帧，每个电机输出占 2 字节，高字节在前。
- 姿态处理：IMU yaw 角通过跨 180 度检测累计成多圈角度。
- 坐标耦合：底盘速度可根据云台/底盘相对偏角做旋转变换，使“车的前进方向”与云台朝向关联。

## 3. RM_Fu 项目详解

### 3.1 项目作用

`RM_Fu` 是一个 RoboMaster 整车控制工程。它不只是单独控制底盘，而是把底盘、云台、射击、视觉通信、遥控器、IMU、编码器、电机抽象、PID 和 CAN 封装到一个分层系统里。

主工程路径：

- `RM_Fu/Core/`：STM32CubeMX 生成的入口、外设初始化、中断、FreeRTOS 配置。
- `RM_Fu/application/`：整车业务逻辑。
- `RM_Fu/BSP/`：板级驱动和设备协议解析。
- `RM_Fu/module/`：可复用控制模块，如 PID、电机抽象。
- `RM_Fu/Chassis_Control_Simple/`：从主工程提取的底盘精简版。
- `RM_Fu/Drivers/`、`RM_Fu/Middlewares/`：STM32 HAL、CMSIS、FreeRTOS 等依赖。

### 3.2 分层结构

`RM_Fu` 的核心结构是：

```text
Core/Src/main.c
  -> HAL/CubeMX 外设初始化
  -> ControlInit()
  -> UART DMA 接收启动
  -> MX_FREERTOS_Init()
  -> osKernelStart()

Core/Src/freertos.c
  -> ControlTask()
  -> ChassisTask()
  -> WatchingTask()

application/contor
  -> 整车控制命令生成

application/chassic
application/gimbal
application/shoot
application/watching
  -> 执行子系统控制

module/motor
module/algorithm/pid
  -> 电机抽象、PID、多环控制、CAN 批量发送

BSP/DR16
BSP/IMU
BSP/can
BSP/ecoder
  -> 遥控器、IMU、CAN、编码器底层适配
```

这个结构比 `RM_Li` 更适合后续扩展，因为应用层不直接拼 CAN 帧，而是通过 `MotorInit()`、`MotorSetRef()`、`MotorControl()` 统一控制电机。

### 3.3 启动流程

入口在 `RM_Fu/Core/Src/main.c`。

启动顺序：

1. `HAL_Init()` 初始化 HAL、SysTick、NVIC。
2. `SystemClock_Config()` 配置系统时钟。
3. 初始化 GPIO、DMA、USART3、IWDG、CAN1、USART1、USART6、CAN2。
4. 获取 DMA 接收缓冲区：
   - `GetRxAHRBuff()`：IMU 接收缓冲。
   - `GiveSbusBuff()`：遥控器 SBUS 接收缓冲。
   - `GetRCData()`：遥控器解析结果。
   - `GetWatchingRevBufff()`：视觉接收缓冲。
5. 调用 `ControlInit()` 初始化底盘、云台、射击、视觉模块。
6. 对 USART6、USART1、USART3 启动 `HAL_UARTEx_ReceiveToIdle_DMA()`。
7. 调用 `MX_FREERTOS_Init()` 创建任务。
8. `osKernelStart()` 启动调度器。

### 3.4 FreeRTOS 任务

定义在 `RM_Fu/Core/Src/freertos.c`。

| 任务 | 周期 | 作用 |
| --- | --- | --- |
| `StartDefaultTask` | 5 ms | 空任务，预留 |
| `ControlTask` | 5 ms | 如果 `Rc_Start == 1`，调用 `RoboCmdTask()` 解析遥控器/键鼠命令 |
| `ChassisTask` | 5 ms | 顺序调用 `GimbalTask()`、`Chassistask()`、`ShootTask()` |
| `WatchingTask` | 100 ms | 调用 `WatchingTra()` 向视觉模块发送姿态数据 |

注意：`GimbalTask()`、`Chassistask()`、`ShootTask()` 内部当前都会调用 `MotorControl()`。因此虽然注释里说统一在底盘任务末尾处理电机输出，但实际代码是每个子模块都可能触发一次电机批量控制和 CAN 发送。后续重构时可以考虑让三个模块只设置参考值，最后统一调用一次 `MotorControl()`，降低 CAN 重复发送和控制周期不一致风险。

### 3.5 中断与数据接收

`HAL_UART_IDLE_IRQHandler()` 位于 `RM_Fu/Core/Src/main.c`，由 `stm32f4xx_it.c` 的 USART1/3/6 中断调用。

处理逻辑：

- USART3：接收遥控器 SBUS。
  - 置位 `Rc_Start = 1`。
  - 喂狗 `HAL_IWDG_Refresh()`。
  - 重置 DMA。
  - 调用 `Dbus_to_rc(subsbuff)` 解析遥控器数据。
- USART1：接收 IMU/AHRS。
  - 重置 DMA。
  - 调用 `AHRSPackHandle(imubuff)` 解析姿态。
  - 重新启动 DMA。
- USART6：接收视觉模块数据。
  - 重置 DMA。
  - 调用 `WatchingRec(watchrevbuff)` 解析视觉目标。
  - 重新启动 DMA。

CAN 接收由 `BSP/can/bsp_can.c` 封装为注册和回调分发机制。电机模块注册 CAN 实例后，收到对应 `rx_id` 的报文时会调用 `DecodeMotor()` 更新电机反馈。

### 3.6 遥控器模块

路径：`RM_Fu/BSP/DR16.c`、`RM_Fu/BSP/DR16.h`

主要作用：

- 保存 SBUS 原始接收缓冲 `sbus_buff[256]`。
- 解析 DR16 数据到 `RC_Ctl_t RC_CtrlData[2]`。
- 当前帧和上一帧都保留，用于键盘按键计数和组合键判断。

核心解析内容：

- `rc.ch0~ch3`：四个摇杆通道，中心值约 1024，范围约 364 到 1684。
- `rc.s1/s2`：两组三档拨杆。
- `mouse.x/y/z`、`press_l/press_r`：鼠标输入。
- `key`：键盘按键位图，额外拆分了普通、Shift 组合、Ctrl 组合状态。

这个模块比 `RM_Li` 的 DR16 解析更完整，尤其适合后续键鼠控制。

### 3.7 整车控制模块

路径：`RM_Fu/application/contor/contor.c`

`ControlInit()` 做四件事：

1. 获取遥控器数据指针。
2. 获取底盘、云台、射击、视觉命令结构指针。
3. 初始化底盘 `ChassisInit()`。
4. 初始化云台 `GimbalInit()`、射击 `ShootInit()`、视觉 `WatchingInit()`。

`RoboCmdTask()` 是控制命令分发入口：

- R 键短按切换控制模式：
  - `RCCONTROLMODE = 0`：遥控器模式。
  - `MOUSECONTROLMODE = 1`：键鼠模式。
- 遥控器模式调用 `RcControlSet()`。
- 键鼠模式调用 `MouseControlSet()`。
- B 键触发 `NVIC_SystemReset()`。

遥控器模式：

- `s2` 下：底盘小陀螺。
- `s2` 中：底盘云台分离。
- `s2` 上：底盘跟随云台。
- `ch3` 映射 `vx`，`ch2` 映射 `vy`。
- `ch0` 累加 yaw 目标角，`ch1` 转 pitch 改变量。
- `s1` 控制射击模式。

键鼠模式：

- WASD 通过渐增/渐减方式平滑生成 `vx/vy`。
- 鼠标左键射击，右键反转疏弹。
- C 键短按切换小陀螺。
- 鼠标 x/y 控制云台 yaw/pitch。

### 3.8 底盘模块

路径：`RM_Fu/application/chassic/chassic.c`

作用：控制四个 M3508 底盘电机，实现麦克纳姆轮底盘运动。

关键结构：

- `Chassic_Ctrl_Cmd`
  - `vx`：前后速度。
  - `vy`：左右速度。
  - `wz`：旋转角速度。
  - `offset_angle`：云台相对底盘偏角。
  - `Chassis_Mode`：底盘模式。

底盘模式：

- `CHASSIS_ZERO_FORCE`：零力/停止输出。
- `CHASSIS_NO_FOLLOW`：底盘不跟随云台。
- `CHASSIS_FOLLOW_GIMBLE_YAW`：底盘跟随云台 yaw。
- `CHASSIS_ROTATE`：小陀螺旋转。

实现方法：

1. `ChassisInit()` 注册四个 M3508 电机，配置速度环 PID 和电流环 PID，并初始化编码器。
2. `CalcOffsetAngle()` 用编码器角度计算云台和底盘之间的偏差角。
3. `Chassistask()` 根据底盘模式计算 `wz`。
4. 通过 `cos/sin(offset_angle)` 把云台坐标系速度转换到底盘坐标系。
5. `OMNICALCULATE()` 用麦克纳姆轮公式计算四个轮子目标转速。
6. `ChassisOutput()` 调用 `MotorSetRef()` 写入目标。
7. `MotorControl()` 统一执行 PID 和 CAN 下发。

麦轮逆运动学公式形态：

```c
motor_speedset[0] = k * (-vx + vy + wz); // 右前
motor_speedset[1] = k * ( vx + vy + wz); // 左前
motor_speedset[2] = k * ( vx - vy + wz); // 左后
motor_speedset[3] = k * (-vx - vy + wz); // 右后
```

其中 `k = -(30/pi) * (1/radius)`，用于把角速度换成 RPM 量级。

### 3.9 云台模块

路径：`RM_Fu/application/gimbal/gimbal.c`

作用：控制 GM6020 云台 yaw 和 pitch 两个轴。

实现方法：

- yaw 电机：
  - 电机类型：GM6020。
  - 外层：角度环。
  - 反馈来源：IMU 的 `YawTotalDegree`。
  - 内层：速度环。
  - 带前馈补偿 `fw_value`。
- pitch 电机：
  - 电机类型：GM6020。
  - 角度反馈主要来自电机编码器。
  - 通过 `AngleContol()` 积分 pitch 输入，并限制在安全范围。

特殊机制：

- 上电初期通过 `yawcotor` 标志把 yaw 目标同步到 IMU 当前累计角，避免启动瞬间乱转。
- 上电 1 秒内降低 yaw 角度环 `MaxOut`，降低初始抖动。
- `GimbalYawForward()` 和 `MouseGimbalYawForward()` 根据当前速度、小陀螺状态计算前馈，用于补偿惯性和减小抖动。

### 3.10 射击模块

路径：`RM_Fu/application/shoot/shoot.c`

作用：控制拨弹盘和两个摩擦轮。

电机配置：

- 拨弹盘：M2006，CAN ID 7。
- 摩擦轮 1：M3508，CAN ID 5。
- 摩擦轮 2：M3508，CAN ID 6，设置反向。

射击模式：

- `MOVENONE`：全部停止。
- `MOVEHEAD`：只启动摩擦轮。
- `MOVEALL`：摩擦轮和拨弹盘全部启动。
- `MOVEMOUSE`：键鼠模式下摩擦轮恒速，拨盘逐渐升速。
- `MOVERESERVE`：反转疏弹。

`ShootReserve()` 根据拨弹盘 PID 输出阈值触发短时间反转，目的是缓解卡弹。

### 3.11 视觉通信模块

路径：`RM_Fu/application/watching/watching.c`

作用：与视觉识别模块通信。

协议：

- MCU 发送到视觉：16 字节。
  - `0xFF | color | pitch(float) | yaw(float) | reserved | 0xFE`
- 视觉发送到 MCU：16 字节。
  - `0xFF | fire_cmd | yaw_ref(float) | pitch_ref(float) | distance(float) | 0xFE`

主要函数：

- `WatchingInit()`：绑定云台电机和 IMU 数据，初始化帧头帧尾。
- `WatchingRec()`：在 USART6 IDLE 中断里解析视觉返回数据。
- `WatchingTra()`：在 FreeRTOS 视觉任务中周期发送 yaw/pitch 姿态。

注意点：

- `color` 是一个 `uint8_t *`，但当前代码里没有看到明确赋值，`WatchingInit()` 中 `watching_transprot.color = *color;` 有空指针风险。后续使用视觉模块前需要补齐颜色变量来源。
- 视觉接收判断使用 `wrevbuff[0] == 0xFF && wrevbuff[15] == 0xFE`，但 `WatchingInit()` 设置发送帧尾为 `w_trabuff[12] = 0xFE`，发送帧尾位置和注释的 16 字节协议不一致，建议复核协议。

### 3.12 电机抽象模块

路径：`RM_Fu/module/motor/motor.c`、`motor.h`

这是 `RM_Fu` 最值得复用的模块。

它把电机抽象为 `MOTORInstance`：

- `measure`：电机反馈，包含编码器、单圈角度、累计角度、速度、电流、温度。
- `motor_settings`：闭环类型、反馈来源、正反转、前馈设置。
- `motor_contorller`：角度、速度、电流三个 PID。
- `motor_can_instance`：CAN 注册实例。
- `senter_group/message_num`：CAN 分组发送位置。

支持电机：

- `GM6020`
- `M3508`
- `M2006`

支持闭环：

- `CURRENT_LOOP`
- `SPEED_LOOP`
- `ANGLE_LOOP`
- `SPEED_AND_CURRENT_LOOP`
- `ANGLE_AND_SPEED_LOOP`
- `ALL_THREE_LOOP`

控制流程：

1. `MotorInit()` 分配并初始化电机实例。
2. 初始化角度/速度/电流 PID。
3. 根据电机类型和 ID 设置 CAN 发送组。
4. 注册 CAN 接收回调 `DecodeMotor()`。
5. 外部模块通过 `MotorSetRef()` 设置目标。
6. `MotorControl()` 遍历所有电机，按闭环配置依次执行角度环、速度环、电流环。
7. 把最终输出打包到对应 CAN 发送组。
8. 对 `0x1FF`、`0x200`、`0x2FF` 等组进行批量发送。

这个设计的好处是业务模块不需要知道 CAN 帧细节，只需要配置电机类型、ID、PID 和目标值。

### 3.13 PID 模块

路径：`RM_Fu/module/algorithm/pid.c`、`pid.h`

核心函数：

- `PID_Init()`：初始化 PID 参数。
- `PIDCalculate()`：根据测量值和参考值计算输出。

支持改进：

- `PID_Integral_limit`：积分限幅。
- `PID_Trapezoid_Intergral`：梯形积分。
- `PID_ChangingIntegrationRate`：变速积分。
- `PID_Derivative_DerivativeFilter`：微分低通滤波。
- 输出低通滤波和输出限幅。

注意点：

- `dt` 被写死为 `3`，不是从真实周期计算。如果控制周期变化，PID 参数会漂移。
- `abs()` 用于浮点误差判断，在 C 中 `abs` 是整型函数，建议改为 `fabsf()`。

### 3.14 `Chassis_Control_Simple` 精简工程

`RM_Fu/Chassis_Control_Simple/` 是从 `RM_Fu` 主工程剥离出来的底盘控制精简版，只保留：

- DR16 遥控器。
- 四个 M3508 底盘电机。
- 麦克纳姆轮运动学。
- PID。
- CAN。
- FreeRTOS 基本任务。

它移除了：

- 云台。
- 射击。
- 视觉。
- IMU。
- 编码器反馈。
- 键鼠复杂控制。

这个子工程适合后续先做底盘验证，尤其适合新项目早期硬件联调。

## 4. RM_Li 项目详解

### 4.1 项目作用

`RM_Li/RM` 也是一个 STM32F407 RoboMaster 控制工程，功能模块覆盖底盘、云台、射击、遥控器、IMU、CAN 和 PID，但架构更简单，所有业务代码集中在 `RM_Li/RM/userfile/`。

目录结构：

```text
RM_Li/RM/
  Core/
    Src/main.c
    Src/tim.c
    Src/stm32f4xx_it.c
  userfile/
    DR16.c/h
    IMU.c/h
    mycan.c/h
    pid.c/h
    conter.c/h
    chassic.c/h
    gimbal.c/h
    shoot.c/h
```

### 4.2 当前运行状态

入口在 `RM_Li/RM/Core/Src/main.c`。

启动后做这些事：

1. 初始化 GPIO、DMA、CAN1、USART3、TIM2、IWDG、USART1。
2. 获取遥控器、IMU、电机数据缓冲区。
3. 调用 `MycanInit()` 启动 CAN。
4. 调用 `GlobalInit()` 把控制层指针连接到各模块命令结构。
5. 调用 `PidAllInit(6, PidData)` 初始化一组测试 PID。
6. 启动 TIM2 中断。
7. 启动 USART1、USART3 的 DMA + IDLE 接收。
8. 主循环为空。

关键注意：当前 `HAL_TIM_PeriodElapsedCallback()` 中，实际启用的是：

```c
GlobalModeSelect();
// ChassicControl();
// GimbalControl();
ShootControl();
```

也就是说，`RM_Li` 当前编译运行时只周期执行模式解析和射击控制，底盘控制、云台控制已经写好但被注释掉。后续如果要拿它跑整车，需要恢复 `ChassicControl()` 和 `GimbalControl()` 的周期调用，并确认 CAN 输出不会冲突。

### 4.3 调度方式

`RM_Li` 没有启用 FreeRTOS。它使用 TIM2 定时中断作为控制周期。

TIM2 配置在 `RM_Li/RM/Core/Src/tim.c`：

- Prescaler = 839
- Period = 999
- TIM2 IRQ 优先级 0

按照 STM32F407 常见 84 MHz APB1 定时器时钟估算，周期约为 10 ms 左右，具体要结合实际 TIM 时钟倍频确认。

周期控制入口：

- `HAL_TIM_PeriodElapsedCallback()`
  - `GlobalModeSelect()`
  - `ShootControl()`
  - 底盘和云台控制当前注释。

### 4.4 遥控器模块

路径：`RM_Li/RM/userfile/DR16.c`、`DR16.h`

实现内容：

- `sbus_rx_buffer[256]` 保存 DMA 原始数据。
- `GiveBuffer()` 返回 DMA 缓冲指针。
- `RemoteDataProcess()` 解析 SBUS。
- `GetRCData()` 返回全局遥控器数据。

解析字段：

- `rc.ch0~ch3`
- `rc.s1/s2`
- `mouse.x/y/z`
- `mouse.press_l/press_r`
- `key.v`

相比 `RM_Fu`，`RM_Li` 的遥控器解析更简单，没有当前/上一帧，也没有组合键、按键计数等高级封装。

### 4.5 总控模块

路径：`RM_Li/RM/userfile/conter.c`

`GlobalInit()`：

- 获取遥控器数据指针。
- 获取底盘、云台、射击命令结构指针。

`GlobalModeSelect()`：

- 根据 `rc.s2` 设置底盘模式：
  - `s2 == 2`：小陀螺。
  - `s2 == 3`：云台底盘分离。
  - `s2 == 1`：云台底盘跟随。
- 把 `ch3` 映射为底盘 `vx`。
- 把 `ch2` 映射为底盘 `vy`。
- 把 `ch0` 累加到云台 yaw 目标。
- 把 `ch1` 映射为 pitch 改变量。
- 把 `s1` 直接作为射击模式。

这个模块相当于 `RM_Fu/application/contor` 的简化版。

### 4.6 底盘模块

路径：`RM_Li/RM/userfile/chassic.c`

作用：控制四个底盘电机，使用麦克纳姆轮运动学。

实现流程：

1. `GetChassic()` 返回底盘命令结构。
2. `ChassicControl()` 获取电机反馈 `GetMotorData()`。
3. 根据 `Mode` 设置 `vz`：
   - 不跟随：`vz = 0`
   - 跟随云台：当前也为 `vz = 0`
   - 小陀螺：`vz = 24`
4. `CaculateOstAngle()` 用电机数据中第 5 个电机的单圈角度减去机械偏置 `ANGLEOFFSET`，得到偏角。
5. 根据偏角旋转 `vx/vy`。
6. `WheelSpdSet()` 解算四轮目标速度。
7. `WheelPidCltr()` 计算四个速度 PID。
8. `CanMotorTransmit(0x200, ...)` 向底盘电机组发送控制量。

注意：

- `ChassicControl()` 当前没有在 TIM2 周期中启用。
- 底盘跟随云台模式里 `vz` 仍然为 0，并没有像 `RM_Fu` 那样根据偏角闭环调整底盘旋转。
- 底盘 PID 在 `WheelPidCltr()` 内部首次调用时初始化，属于懒初始化。

### 4.7 云台模块

路径：`RM_Li/RM/userfile/gimbal.c`

作用：控制 yaw 和 pitch 两个云台电机。

实现方法：

- yaw：
  - 外环角度 PID 以 `Ahrsfeed->YawTotalDegree` 为反馈。
  - 内环速度 PID 以 `Gimbal_motordata[7].speed` 为反馈。
  - 使用 `GimbalForwardFeedback()` 做简单速度前馈。
- pitch：
  - `PitchAngleCtl()` 根据遥控器输入累计 pitch 角度目标。
  - 外环角度 PID 以 `Gimbal_motordata[8].total_angle` 为反馈。
  - 内环速度 PID 以 `Gimbal_motordata[8].speed` 为反馈。

特殊机制：

- `YawEnableFlag` 由 IMU 解析模块在首次获得 yaw 后置位。
- `GimbalControl()` 检测 `YawEnableFlag == 1` 时，把 yaw 目标同步到当前 IMU yaw，避免上电乱转。

注意：

- `GimbalControl()` 当前没有在 TIM2 周期中启用。
- CAN 发送 ID 为 `0x2ff`，输出 yaw 和 pitch 两个 GM6020。

### 4.8 射击模块

路径：`RM_Li/RM/userfile/shoot.c`

作用：控制拨弹盘和两个摩擦轮。

当前 TIM2 周期中已经启用 `ShootControl()`。

模式：

- `MOVENONE = 2`：全部停止。
- `MOVEHEAD = 3`：摩擦轮转，拨弹盘不转。
- `MOVEALL = 1`：摩擦轮和拨弹盘都转。
- `MOVEMOUSE = 4`：键鼠相关，拨弹盘逐步升速，摩擦轮高速。
- `MOVERESERVE = 5`：反转。

目标速度常量：

- `BULLETSPEED = -15000`
- `FRICTIONLEFTSPEED = -5000`
- `FRICTIONRIGHTSPEED = 5000`

控制流程：

1. 读取电机反馈 `GetMotorData()`。
2. 根据 `shoot_mood` 设置三个目标速度。
3. 首次调用时初始化 3 个 PID。
4. 分别计算拨弹盘、左摩擦轮、右摩擦轮 PID。
5. 发送 `CanMotorTransmit(0x1ff, ...)`。

### 4.9 CAN 模块

路径：`RM_Li/RM/userfile/mycan.c`

作用：直接封装 CAN1 启动、全接收过滤器、电机反馈解析和电机输出。

主要函数：

- `MycanInit()`：启动 CAN1，配置过滤器，激活 FIFO0 接收中断。
- `MotorProcess()`：根据标准 ID 解析电机反馈。
- `EcodeProc()`：根据编码器单圈角计算累计角。
- `HAL_CAN_RxFifo0MsgPendingCallback()`：读取 CAN 报文并调用 `MotorProcess()`。
- `CanMotorTransmit()`：向指定标准 ID 发送 4 个 int16 控制值。

数据结构：

- `MotorData motor_data1[9]`
  - `0~3`：底盘 M3508。
  - `4~6`：射击相关电机。
  - `7~8`：云台 GM6020。

注意：

- ID 映射逻辑较硬编码，不如 `RM_Fu` 的注册式 CAN 抽象灵活。
- 发送函数每次直接调用 `HAL_CAN_AddTxMessage()`，没有邮箱等待超时处理。

### 4.10 IMU 模块

路径：`RM_Li/RM/userfile/IMU.c`

作用：解析 AHRS 数据包。

实现方法：

- `GiveAHRSBuffer()` 返回 DMA 接收缓冲。
- `AHRSPackHandle()` 从固定字节位置解析 RollSpeed、PitchSpeed、HeadingSpeed、Roll、Pitch、Heading、四元数、时间戳。
- `DataCharToFloat()` 手动把 4 字节转成 float。
- 根据 yaw 角跨越 `+-180` 度判断圈数，得到 `YawTotalDegree`。
- 首次解析后设置 `YawEnableFlag = 1`，用于云台上电同步 yaw。

注意：

- 手写 IEEE754 转换可用，但更稳妥的方式是使用 `memcpy` 到 `float`，避免别名和移位细节风险。
- `Timestamp` 在结构中是 `int64_t`，但赋值来自 float 转换结果，语义不完全一致。

### 4.11 PID 模块

路径：`RM_Li/RM/userfile/pid.c`

实现内容与 `RM_Fu` 类似，但更轻量：

- `PidInit()`
- `PidAllInit()`
- `PidReturn()`
- 积分限幅、变速积分、微分滤波、输出滤波、输出限幅。

注意：

- `dt` 被固定为 `3`。
- 浮点绝对值使用自写 `Abs()`，比 `abs()` 更适合 float。
- `f_Changing_Integration_Rate()` 中使用了 `pid->ITerm` 参与比例计算，建议后续复核算法是否应使用误差绝对值。

## 5. 两个项目的核心差异

### 5.1 架构差异

`RM_Fu` 是分层、注册式、任务式：

- 模块之间通过命令结构体和电机抽象解耦。
- 电机由 `MotorInit()` 注册，CAN 回调自动分发。
- FreeRTOS 让控制、执行、视觉通信分开。

`RM_Li` 是直接、集中、裸机式：

- 业务模块直接调用 `CanMotorTransmit()`。
- 电机数据存在固定数组 `motor_data1[9]`。
- TIM2 中断直接调业务函数。

### 5.2 电机控制差异

`RM_Fu`：

- 每个电机有独立实例。
- 支持角度/速度/电流多环嵌套。
- 支持外部反馈源、前馈、正反转配置。
- CAN 输出分组批量发送。

`RM_Li`：

- 电机反馈用固定数组下标表示。
- 每个模块自己持有 PID 数组。
- 输出直接拼 CAN 帧。
- 简单易调，但扩展容易混乱。

### 5.3 调试和复用差异

`RM_Fu` 更适合长期项目，但学习成本更高。  
`RM_Li` 更适合看清楚某个控制算法怎么跑，但当前启用程度不完整。

## 6. 后续新项目建议

### 6.1 推荐主线

建议新项目采用：

- `RM_Fu` 的目录结构。
- `RM_Fu` 的 `module/motor` 电机抽象。
- `RM_Fu` 的 `module/algorithm/pid` 作为 PID 基础，但修复浮点绝对值和 `dt`。
- `RM_Fu` 的 `BSP/DR16` 作为遥控器输入。
- `RM_Fu/Chassis_Control_Simple` 作为底盘最小可跑版本。
- `RM_Li` 的 `chassic.c`、`gimbal.c` 作为算法对照和简化调试参考。

### 6.2 建议的开发顺序

1. 先跑通 CAN 电机反馈和单电机 PID。
2. 再跑底盘四轮速度环。
3. 加入 DR16 遥控器输入。
4. 加入底盘麦轮运动学。
5. 加入 IMU 和云台 yaw 同步。
6. 加入云台 pitch 限位。
7. 加入射击机构。
8. 最后加入视觉通信。

### 6.3 需要优先修复/确认的问题

1. `RM_Fu` 中多个子任务都调用 `MotorControl()`，建议统一成每个控制周期只调用一次。
2. `RM_Fu/application/watching.c` 中 `color` 指针未见初始化，存在空指针风险。
3. `RM_Fu` 视觉发送帧尾位置与注释协议不一致，建议复核 16 字节协议。
4. `RM_Fu` PID 使用 `abs()` 处理 float，建议改为 `fabsf()`。
5. 两个项目 PID 的 `dt` 都写死，建议用真实周期或统一常量。
6. `RM_Li` 当前 TIM2 周期没有启用底盘和云台控制。
7. `RM_Li` CAN ID 与电机数组下标硬编码，迁移前必须重新确认实车电机 ID。
8. 两个项目都需要补充遥控器失联保护、电机超温/堵转保护和急停逻辑。

## 7. 复用清单

优先复用：

- `RM_Fu/module/motor/`：统一电机抽象。
- `RM_Fu/module/algorithm/pid.*`：PID 框架。
- `RM_Fu/BSP/can/`：CAN 注册与回调分发。
- `RM_Fu/BSP/DR16.*`：完整遥控器/键鼠解析。
- `RM_Fu/application/chassic/`：底盘控制和云台坐标耦合。
- `RM_Fu/Chassis_Control_Simple/`：底盘最小版本。

谨慎复用：

- `RM_Fu/application/watching/`：需要先修复指针和协议位置。
- `RM_Fu/application/shoot/`：需要结合实际摩擦轮/拨弹盘方向重新标定。
- `RM_Li/userfile/mycan.*`：适合学习，不建议作为大型项目主 CAN 框架。

可作为算法参考：

- `RM_Li/userfile/chassic.c`：底盘偏角旋转和麦轮解算写法直观。
- `RM_Li/userfile/gimbal.c`：IMU yaw 外环 + 电机速度内环的简化实现。
- `RM_Li/userfile/pid.c`：轻量 PID 实现。

## 8. 总结

`RM_Fu` 是更成熟的整车控制框架，已经具备模块化、任务化和电机抽象能力；`RM_Li` 是更简洁的裸机控制样例，适合理解底层控制链路和快速实验。后续新项目应优先以 `RM_Fu` 为骨架，用 `Chassis_Control_Simple` 做最小可运行验证，再从 `RM_Li` 中吸收简单直接的控制思路。

一句话归纳：

> 用 `RM_Fu` 搭架子，用 `RM_Li` 查思路；先让底盘稳，再让云台准，最后把射击和视觉接进去。
