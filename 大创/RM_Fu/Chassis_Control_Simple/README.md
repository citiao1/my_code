# 底盘控制精简工程

## 项目简介

这是一个从RM_Fu工程中提取的精简版底盘控制工程，只保留**遥控器控制底盘四个电机**的核心功能。

## 功能特性

- ✅ 遥控器（DR16）SBUS协议解析
- ✅ 四个M3508底盘电机控制（速度环+电流环）
- ✅ 麦克纳姆轮运动学解算
- ✅ PID控制算法
- ✅ CAN总线通信
- ✅ FreeRTOS多任务调度

## 已移除功能

- ❌ 云台控制
- ❌ 射击控制
- ❌ 视觉通信
- ❌ IMU姿态解算
- ❌ 编码器反馈
- ❌ 键鼠控制模式
- ❌ 云台跟随模式

## 工程结构

```
Chassis_Control_Simple/
├── application/
│   ├── chassic/          # 底盘控制模块
│   │   ├── chassic.c
│   │   └── chassic.h
│   └── control/          # 控制模块（精简版）
│       ├── control.c
│       └── control.h
├── BSP/
│   ├── can/              # CAN总线驱动
│   │   ├── bsp_can.c
│   │   └── bsp_can.h
│   └── DR16/             # 遥控器驱动
│       ├── DR16.c
│       └── DR16.h
├── module/
│   ├── motor/            # 电机控制模块
│   │   ├── motor.c
│   │   └── motor.h
│   └── algorithm/        # PID算法模块
│       ├── pid.c
│       └── pid.h
└── Core/
    ├── Src/
    │   ├── main.c        # 主程序（精简版）
    │   └── freertos.c    # FreeRTOS任务（精简版）
    └── Inc/
        └── main.h
```

## 硬件要求

- **MCU**: STM32F407
- **遥控器**: DR16（SBUS协议）
- **电机**: 4个M3508底盘电机
- **通信**:
  - USART3: 遥控器通信（SBUS）
  - CAN1: 电机通信

## 软件架构

### 数据流

```
遥控器SBUS数据 
  → UART IDLE中断 
  → DR16解析 
  → ControlTask (解析命令)
  → ChassisTask (运动学解算)
  → MotorControl (PID计算)
  → CAN发送
  → 电机执行
```

### 任务调度

- **ControlTask** (优先级: Idle, 周期: 5ms)
  - 解析遥控器数据
  - 生成底盘控制命令

- **ChassisTask** (优先级: Idle, 周期: 5ms)
  - 执行底盘运动学解算
  - 设置电机参考值
  - 调用MotorControl()进行PID计算和CAN发送

- **defaultTask** (优先级: Normal, 周期: 5ms)
  - 预留任务（当前为空）

## 使用方法

### 1. 配置硬件

- 连接DR16遥控器到USART3
- 连接4个M3508电机到CAN1（ID: 1-4）
- 配置电机ID和正反转方向（在`chassic.c`的`ChassisInit()`中）

### 2. 编译工程

- 使用STM32CubeIDE或Keil MDK打开工程
- 配置编译选项
- 编译并下载到MCU

### 3. 控制说明

**遥控器摇杆**:
- `ch3` (左摇杆上下): X方向速度（前进/后退）
- `ch2` (左摇杆左右): Y方向速度（左移/右移）

**遥控器开关**:
- `s2` 上/中: 不跟随模式（角速度为0）
- `s2` 下: 旋转模式（小陀螺，固定角速度）

**键盘按键**:
- `B键`: 系统复位

## 参数配置

### 底盘参数（chassic.h）

```c
#define CHASSICLENGH        0.37f   // 底盘长度（米）
#define CHASSICWHEIGH       0.37f   // 底盘宽度（米）
#define radius              0.150f  // 麦克纳姆轮半径（米）
```

### PID参数（chassic.c）

**速度环PID**:
- Kp = 1.80
- Ki = 0.005
- Kd = 0.0
- MaxOut = 15000
- IntegralLimit = 3000

**电流环PID**:
- Kp = 1.00
- Ki = 0.00
- Kd = 0.01
- MaxOut = 5000

### 电机ID配置（chassic.c）

在`ChassisInit()`中配置：
```c
for(int i = 1; i <= 4; i++)
{
    ChassicMotorsConfig.can_init_config.tx_id = i;  // CAN ID: 1-4
    // 根据实际安装方向调整正反转
    ChassicMotorsConfig.contorller_setting_init_config.motor_reverse_flag = MOTOR_DIRECTION_RESERVE;
    motor[i - 1] = MotorInit(&ChassicMotorsConfig);
}
```

## 注意事项

1. **电机正反转**: 根据实际安装方向，在`ChassisInit()`中调整`motor_reverse_flag`
2. **PID参数**: 根据实际电机特性调整PID参数
3. **底盘尺寸**: 根据实际底盘尺寸调整`CHASSICLENGH`、`CHASSICWHEIGH`、`radius`
4. **CAN ID**: 确保电机CAN ID与代码中配置一致（1-4）
5. **精简版限制**: 不支持云台跟随模式，底盘角速度只能通过模式切换控制

## 扩展建议

如果需要添加更多功能，可以参考原RM_Fu工程：

1. **添加云台控制**: 参考`application/gimbal/`
2. **添加IMU反馈**: 参考`BSP/IMU.c`
3. **添加编码器反馈**: 参考`BSP/ecoder/`
4. **添加键鼠控制**: 参考`application/contor/contor.c`的`MouseControlSet()`

## 许可证

本项目基于原RM_Fu工程，遵循相同的许可证。

## 作者

精简自RM_Fu工程，保留核心底盘控制功能。

