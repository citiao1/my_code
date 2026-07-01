# RM_Fu 项目架构文档

## 项目概述
这是一个基于STM32F407的RoboMaster机器人控制系统，采用FreeRTOS实时操作系统，实现了底盘、云台、射击和视觉识别等模块的协同控制。

## 整体架构

### 分层架构设计

```
┌─────────────────────────────────────────┐
│      Application Layer (应用层)          │
│  contor, chassic, gimbal, shoot, watching│
├─────────────────────────────────────────┤
│      Module Layer (模块层)               │
│  motor, pid, remote, C_board            │
├─────────────────────────────────────────┤
│      BSP Layer (板级支持包)               │
│  DR16, IMU, ecoder, can                 │
├─────────────────────────────────────────┤
│      HAL Layer (硬件抽象层)              │
│  STM32 HAL库 (CAN, UART, GPIO, DMA等)    │
└─────────────────────────────────────────┘
```

## 详细架构分析

### 1. 硬件层 (HAL Layer)

#### 外设配置
- **CAN总线**: CAN1和CAN2用于电机通信
- **UART串口**:
  - USART1: 接收IMU数据（AHRS姿态传感器）
  - USART3: 接收遥控器数据（DR16/SBUS协议，256字节DMA）
  - USART6: 接收视觉数据（Watching模块，128字节DMA）
- **DMA**: 用于UART数据接收，提高效率
- **IWDG**: 看门狗定时器，防止系统死锁

#### 时钟配置
- 系统时钟: 168MHz (HSE 8MHz, PLL倍频)
- APB1: 42MHz (HCLK/4)
- APB2: 84MHz (HCLK/2)

### 2. BSP层 (Board Support Package)

#### 2.1 DR16 遥控器驱动 (`BSP/DR16`)
- **功能**: 解析SBUS协议，获取遥控器数据
- **数据结构**: `RC_Ctl_t`
  - 遥控器摇杆: ch0-ch3 (右水平、右竖直、左水平、左竖直)
  - 开关: s1, s2 (左侧开关、右侧开关)
  - 鼠标数据: x, y, z, press_l, press_r
  - 键盘数据: 16个按键状态 (w,s,a,d,shift,ctrl,q,e,r,f,g,z,x,c,v,b)
- **通信方式**: USART3 + DMA，IDLE中断触发数据处理
- **关键函数**: `Dbus_to_rc()` - 将SBUS数据转换为控制结构

#### 2.2 IMU 姿态传感器 (`BSP/IMU`)
- **功能**: 接收并解析AHRS数据包
- **数据包格式**: 
  - 起始字节: 0xFC
  - 数据包类型: 0x41 (AHRS)
  - 数据长度: 48字节 (Roll, Pitch, Yaw角度和角速度，四元数等)
  - 结束字节: 0xFD
- **通信方式**: USART1 + DMA，IDLE中断触发
- **数据结构**: `AHRS_FEED`
  - 角度: Roll, Pitch, Heading
  - 角速度: RollSpeed, PitchSpeed, HeadingSpeed
  - 四元数: Q1-Q4
  - 累计角度: YawTotalDegree, PitchDegree
- **关键函数**: `AHRSPackHandle()` - 解析AHRS数据包

#### 2.3 编码器驱动 (`BSP/ecoder`)
- **功能**: 通过CAN总线读取电机编码器数据
- **数据结构**: `EcoderInstance`
  - 单圈编码值、角度
  - 累计角度、圈数
  - CAN实例指针
- **转换系数**: `ENCODE_2_DEGREE = 0.010986328125` (编码值转角度)

#### 2.4 CAN总线驱动 (`BSP/can/bsp_can`)
- **功能**: CAN通信的底层封装
- **数据结构**: `CANInstance`
  - 发送/接收ID配置
  - 发送/接收缓冲区
  - 回调函数指针
- **关键函数**: 
  - `CANRegister()` - 注册CAN设备实例
  - `CANTransmit()` - 发送CAN数据

### 3. 模块层 (Module Layer)

#### 3.1 PID控制器 (`module/algorithm/pid`)
- **功能**: 实现PID控制算法
- **PID改进算法**:
  - 积分限幅 (`PID_Integral_limit`)
  - 梯形积分 (`PID_Trapezoid_Intergral`)
  - 变速积分 (`PID_ChangingIntegrationRate`)
  - 微分滤波 (`PID_Derivative_DerivativeFilter`)
- **数据结构**: `PIDInstance`
  - 参数: Kp, Ki, Kd, KF (前馈)
  - 限幅: MaxOut, IntegralLimit
  - 死区: DeadLimit
  - 输出滤波: Output_LPF_RC
- **关键函数**: `PIDCalculate()` - PID计算

#### 3.2 电机控制模块 (`module/motor/motor`)
- **功能**: 统一的电机控制接口，支持多种电机类型
- **支持的电机类型**:
  - `GM6020`: 云台电机
  - `M3508`: 底盘/拨弹盘电机
  - `M2006`: 摩擦轮电机
- **闭环控制**:
  - 电流环 (`CURRENT_LOOP`)
  - 速度环 (`SPEED_LOOP`)
  - 角度环 (`ANGLE_LOOP`)
  - 支持多环嵌套控制
- **前馈控制**:
  - 电流前馈 (`CURRENT_FEEDFORWARD`)
  - 速度前馈 (`SPEED_FEEDFORWARD`)
  - 角度前馈 (`ANGLE_FEEDFORWARD`)
- **反馈来源**:
  - 电机自身反馈 (`MOTOR_FEED`)
  - 外部反馈 (`OTHER_FEED`) - 如IMU、编码器
- **电机分组发送**:
  - 组0: 0x1FF (M3508 ID 1-4 或 GM6020 ID 1-4)
  - 组1: 0x200 (M3508 ID 5-8)
  - 组2: 0x2FF (GM6020 ID 5-8)
- **关键函数**:
  - `MotorInit()` - 电机初始化
  - `MotorSetRef()` - 设置目标值
  - `MotorOuterLoop()` - 外层控制环
  - `MotorControl()` - 电机控制主函数（批量处理所有电机）

#### 3.3 遥控器模块 (`module/remote`)
- **状态**: 空实现，功能集成在DR16驱动中

#### 3.4 双板通信模块 (`module/C_board`)
- **状态**: 结构体已定义，但功能未实现

### 4. 应用层 (Application Layer)

#### 4.1 控制模块 (`application/contor`)
- **功能**: 整车控制逻辑，整合所有子模块
- **控制模式**:
  - `RCCONTROLMODE` (0): 遥控器控制模式
  - `MOUSECONTROLMODE` (1): 键鼠控制模式
- **关键功能**:
  - `ControlInit()`: 初始化所有模块
  - `RoboCmdTask()`: 主控制任务，根据模式调用相应控制函数
  - `RcControlSet()`: 遥控器控制逻辑
  - `MouseControlSet()`: 键鼠控制逻辑
- **小陀螺模式**: 通过C键切换，实现底盘旋转
- **模式切换**: 通过R键在遥控器/键鼠模式间切换

#### 4.2 底盘模块 (`application/chassic`)
- **功能**: 底盘运动控制
- **底盘模式** (`Chassis_Mode_e`):
  - `CHASSIS_ZERO_FORCE`: 零力模式
  - `CHASSIS_NO_FOLLOW`: 底盘不跟随云台
  - `CHASSIS_FOLLOW_GIMBLE_YAW`: 底盘跟随云台Yaw轴
  - `CHASSIS_ROTATE`: 小陀螺模式（旋转）
- **运动学参数**:
  - 底盘长度: 0.37m
  - 底盘宽度: 0.37m
  - 轮子半径: 0.150m
  - 偏移角度: 84.0度
- **控制输入**: `Chassic_Ctrl_Cmd`
  - vx, vy: X/Y方向速度
  - wz: 旋转角速度
  - Chassis_Mode: 底盘模式
- **关键函数**: `Chassistask()` - 底盘控制任务

#### 4.3 云台模块 (`application/gimbal`)
- **功能**: 云台姿态控制
- **控制输入**: `Gimbal_Ctrl_Cmd`
  - cmd_mode: 控制模式（遥控器/键鼠）
  - rotatemode: 旋转模式
  - watchingcmd: 视觉控制命令
  - yaw_total_degree: Yaw轴累计角度
  - pitch_total_degree: Pitch轴累计角度
  - pitch_change_degree: Pitch轴变化量
- **电机**: 
  - Yaw轴电机: `GetYawMotor()`
  - Pitch轴电机: `GetPitchMotor()`
- **关键函数**: `GimbalTask()` - 云台控制任务

#### 4.4 射击模块 (`application/shoot`)
- **功能**: 射击控制（摩擦轮、拨弹盘）
- **射击模式** (`SHOOT_MODE_e`):
  - `MOVENONE` (2): 不动作
  - `MOVEHEAD` (3): 仅拨弹盘
  - `MOVEALL` (1): 摩擦轮+拨弹盘
  - `MOVEMOUSE` (4): 鼠标控制射击
  - `MOVERESERVE` (5): 反向
- **关键函数**: `ShootTask()` - 射击控制任务

#### 4.5 视觉模块 (`application/watching`)
- **功能**: 与视觉识别模块通信，实现自动瞄准
- **发送数据** (`WatchingTransprot_t`):
  - color: 颜色信息
  - pitch_measure: Pitch轴测量值
  - yaw_measure: Yaw轴测量值
- **接收数据** (`WatchingRecive_t`):
  - fire_cmd: 开火命令
  - yaw_ref: Yaw轴目标角度
  - pitch_ref: Pitch轴目标角度
  - distance_measure: 距离测量值
- **通信方式**: USART6 + DMA，IDLE中断触发
- **关键函数**:
  - `WatchingRec()`: 接收视觉数据
  - `WatchingTra()`: 发送视觉数据

## 5. 任务调度 (FreeRTOS)

### 5.1 任务列表

| 任务名称 | 优先级 | 堆栈大小 | 周期 | 功能 |
|---------|--------|----------|------|------|
| defaultTask | Normal | 128 | 5ms | 默认任务（空闲） |
| ControlTask | Idle | 128 | 5ms | 控制任务（解析遥控器命令） |
| ChassisTask | Idle | 128 | 5ms | 底盘任务（云台+底盘+射击） |
| WatchingTask | High | 128 | 100ms | 视觉任务（视觉通信） |

### 5.2 任务执行流程

```
系统启动
  ↓
main() 初始化外设
  ↓
ControlInit() 初始化所有模块
  ↓
启动FreeRTOS调度器
  ↓
┌─────────────────────────────────────┐
│  ControlTask (5ms)                  │
│  - 解析遥控器/键鼠命令               │
│  - 模式切换判断                      │
│  - 生成控制命令                      │
└─────────────────────────────────────┘
           ↓
┌─────────────────────────────────────┐
│  ChassisTask (5ms)                   │
│  - GimbalTask() 云台控制             │
│  - Chassistask() 底盘控制            │
│  - ShootTask() 射击控制              │
│  - MotorControl() 电机控制           │
└─────────────────────────────────────┘
           ↓
┌─────────────────────────────────────┐
│  WatchingTask (100ms)                │
│  - WatchingTra() 发送视觉数据        │
└─────────────────────────────────────┘
```

### 5.3 中断处理

#### UART IDLE中断
- **触发时机**: UART接收完成（DMA传输完成）
- **处理函数**: `HAL_UART_IDLE_IRQHandler()`
- **处理内容**:
  - USART3: 解析SBUS数据 → `Dbus_to_rc()`
  - USART1: 解析AHRS数据 → `AHRSPackHandle()`
  - USART6: 解析视觉数据 → `WatchingRec()`
  - 重新启动DMA接收

#### CAN中断
- **触发时机**: CAN接收完成
- **处理内容**: 解析电机反馈数据，更新电机状态

## 6. 数据流

### 6.1 控制数据流

```
遥控器/键鼠输入
    ↓
DR16解析 (SBUS)
    ↓
ControlTask (RoboCmdTask)
    ↓
生成控制命令:
  - Chassic_Ctrl_Cmd
  - Gimbal_Ctrl_Cmd
  - Shoot_Ctrl_cmd
    ↓
ChassisTask
    ↓
各模块控制任务
    ↓
MotorControl
    ↓
CAN发送到电机
```

### 6.2 反馈数据流

```
电机反馈 (CAN)
    ↓
motor_measure_s (角度、速度、电流)
    ↓
PID控制器
    ↓
电机控制输出
```

```
IMU数据 (USART1)
    ↓
AHRS_FEED (姿态角度)
    ↓
云台/底盘控制
```

```
视觉数据 (USART6)
    ↓
WatchingRecive_t (目标角度)
    ↓
云台控制
```

## 7. 关键设计特点

### 7.1 模块化设计
- 清晰的层次结构：BSP → Module → Application
- 每个模块职责单一，接口清晰
- 便于维护和扩展

### 7.2 实时性保障
- FreeRTOS多任务调度
- DMA传输减少CPU占用
- 中断驱动数据处理
- 看门狗防止死锁

### 7.3 灵活的控制架构
- 支持遥控器和键鼠两种控制模式
- 多种底盘运动模式
- 可配置的电机控制参数
- 多环PID控制支持

### 7.4 通信协议
- **SBUS**: 遥控器通信（256字节）
- **CAN**: 电机通信（标准帧，8字节数据）
- **AHRS**: IMU数据包（自定义协议）
- **视觉协议**: 自定义16字节协议

## 8. 文件结构总结

```
RM_Fu/
├── application/          # 应用层
│   ├── chassic/         # 底盘控制
│   ├── contor/          # 整车控制
│   ├── gimbal/          # 云台控制
│   ├── shoot/           # 射击控制
│   └── watching/        # 视觉通信
├── BSP/                 # 板级支持包
│   ├── can/             # CAN驱动
│   ├── DR16.*           # 遥控器驱动
│   ├── IMU.*            # IMU驱动
│   └── ecoder/          # 编码器驱动
├── module/              # 模块层
│   ├── algorithm/       # PID算法
│   ├── motor/           # 电机控制
│   ├── remote/          # 遥控器（预留）
│   └── C_board/         # 双板通信（预留）
├── Core/                # STM32核心代码
│   ├── Src/             # 源文件
│   │   ├── main.c      # 主函数
│   │   ├── freertos.c  # RTOS任务
│   │   └── ...
│   └── Inc/             # 头文件
└── Drivers/             # STM32 HAL库
```

## 9. 配置参数

### 9.1 底盘参数
- 长度: 0.37m
- 宽度: 0.37m
- 轮半径: 0.150m
- 偏移角度: 84.0度

### 9.2 通信参数
- CAN发送ID: 0x1FF, 0x200, 0x2FF
- CAN接收ID: 0x201-0x208 (M3508/M2006), 0x205-0x208 (GM6020)
- UART波特率: 根据实际配置

### 9.3 控制周期
- 控制任务: 5ms
- 视觉任务: 100ms
- 电机控制: 在ChassisTask中调用（5ms）

## 10. 扩展建议

1. **完善双板通信**: 实现`C_board`模块，支持多主控协同
2. **优化视觉通信**: 增加数据校验和错误处理
3. **增加日志系统**: 便于调试和故障排查
4. **参数配置化**: 将PID参数等配置外置，便于调参
5. **安全保护**: 增加限位保护、急停等功能
6. **状态机设计**: 为各模块设计状态机，提高系统稳定性



