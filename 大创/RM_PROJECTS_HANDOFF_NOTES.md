# RM 项目后续对接速查笔记

这份笔记给后续继续开发时快速恢复上下文用。详细分析见 `RM_PROJECTS_ANALYSIS_REPORT.md`。

## 当前判断

- `RM_Fu`：主参考项目，完整度更高，采用 FreeRTOS + 分层架构。
- `RM_Li/RM`：简化/实验项目，业务集中在 `userfile/`，当前 TIM2 中只启用了模式解析和射击控制。
- 新项目建议从 `RM_Fu/Chassis_Control_Simple` 验证底盘，再迁移到 `RM_Fu` 主框架。

## RM_Fu 关键入口

- 启动入口：`RM_Fu/Core/Src/main.c`
- 任务入口：`RM_Fu/Core/Src/freertos.c`
- 总控入口：`RM_Fu/application/contor/contor.c`
- 底盘：`RM_Fu/application/chassic/`
- 云台：`RM_Fu/application/gimbal/`
- 射击：`RM_Fu/application/shoot/`
- 视觉：`RM_Fu/application/watching/`
- 遥控器：`RM_Fu/BSP/DR16.*`
- IMU：`RM_Fu/BSP/IMU.*`
- CAN：`RM_Fu/BSP/can/`
- 电机抽象：`RM_Fu/module/motor/`
- PID：`RM_Fu/module/algorithm/`

## RM_Li 关键入口

- 启动入口：`RM_Li/RM/Core/Src/main.c`
- 定时器：`RM_Li/RM/Core/Src/tim.c`
- 业务代码：`RM_Li/RM/userfile/`
- 总控：`RM_Li/RM/userfile/conter.c`
- CAN：`RM_Li/RM/userfile/mycan.c`
- 底盘：`RM_Li/RM/userfile/chassic.c`
- 云台：`RM_Li/RM/userfile/gimbal.c`
- 射击：`RM_Li/RM/userfile/shoot.c`

## 最重要的注意点

1. `RM_Li/RM/Core/Src/main.c` 的 TIM2 回调里 `ChassicControl()` 和 `GimbalControl()` 当前被注释，只有 `GlobalModeSelect()` 和 `ShootControl()` 在跑。
2. `RM_Fu` 的 `GimbalTask()`、`Chassistask()`、`ShootTask()` 当前都会调用 `MotorControl()`，后续可考虑每周期统一调用一次。
3. `RM_Fu/application/watching.c` 的 `color` 指针需要补初始化，否则视觉模块可能有空指针风险。
4. 两个项目 PID 的 `dt` 都是固定值，换控制周期前要改。
5. 迁移到实车前必须确认电机 ID、正反转、反馈方向、CAN 分组和机械零位。

## 推荐复用路径

1. 先用 `RM_Fu/Chassis_Control_Simple` 跑通底盘四电机。
2. 复用 `RM_Fu/module/motor` 管理所有电机。
3. 复用 `RM_Fu/BSP/DR16` 做遥控器和键鼠输入。
4. 加入 `RM_Fu/BSP/IMU` 和云台 yaw 同步。
5. 接入 `RM_Fu/application/gimbal`、`shoot`。
6. 最后处理 `watching` 视觉协议。

## 一句话方案

以 `RM_Fu` 为新项目骨架，以 `RM_Li` 为控制算法参考；先稳定底盘闭环，再接云台和射击，视觉最后接入。
