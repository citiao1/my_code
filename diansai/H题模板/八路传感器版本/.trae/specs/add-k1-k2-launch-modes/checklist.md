# Checklist

## K2 慢发渐进加速模式
- [x] K2 短按从停止状态发车，发车速度 = `__chassis_vx - 100`
- [x] K2 发车执行 IMU 陀螺仪校准、重置紧急停车标志与窗口
- [x] K2 模式运行中每 20ms `chassis_vx` 递增 +10
- [x] K2 模式加速至 `__chassis_vx` 后停止递增并保持
- [x] K2 模式不显示里程、不重置里程
- [x] K2 运行中按 K2 停止（toggle 语义）
- [x] K2 发车蜂鸣器 1 响

## K1 快发按里程减速模式
- [x] K1 发车速度 = `__chassis_vx + 200`
- [x] K1 发车保留原有 IMU 陀螺仪校准（`MID_IMU_CalibrateGyro`）
- [x] K1 发车时里程归零（`MID_Encoder_OdomReset`）
- [x] K1 模式里程 < 减速阈值时 `chassis_vx` 保持 `__chassis_vx + 200`
- [x] K1 模式里程 ≥ 减速阈值（默认 3000mm）时每 20ms 递减 -10
- [x] K1 减速至 `__chassis_vx - 100` 后停止递减并保持
- [x] K1 运行中按 K1 停止（保留原 toggle 语义）

## 跨模式按键行为
- [x] K1 运行中按 K2 忽略（不切换、不停止）
- [x] K2 运行中按 K1 忽略（不切换、不停止）
- [x] 仅同模式按键 toggle 停止

## 紧急停车模式开关
- [x] K1 模式下 `MID_Line_CheckEmergency` 生效（脱线触发停车）
- [x] K2 模式下 `MID_Line_CheckEmergency` 被跳过（脱线不停车）
- [x] K2 模式下 `g_line_emergency_stopped` 保持 0
- [x] 停止状态恢复紧急停车使能（默认启用，兼容现有行为）

## 编码器里程计算
- [x] `MID_Encoder_OdomReset()` 记录当前左右轮累计脉冲为基准
- [x] `MID_Encoder_GetOdomMm()` 返回左右轮脉冲差平均换算的 mm 里程
- [x] 换算系数使用 `MID_ENCODER_COUNTS_PER_METER`（3726 脉冲/米）
- [x] K1 发车时调用 `MID_Encoder_OdomReset()` 里程归零
- [x] 里程无积分漂移（基于脉冲差分）

## OLED 里程显示
- [x] K1 模式运行时 OLED 显示 "DIST:" + 里程(mm)
- [x] K1 模式同时保留运行时间显示
- [x] 非 K1 模式（K2/IDLE）仅显示运行时间

## 发车模式状态查询 API
- [x] `MID_Key_GetLaunchMode()` 返回 IDLE / K1 / K2 枚举
- [x] `MID_Key_IsRunning()` 在 K1/K2 模式均返回 1（兼容现有调用）

## 可调参数宏定义
- [x] 减速阈值、±步进、速度偏移、步进周期均以宏定义集中在 mid_key.h
- [x] 宏定义有注释说明默认值与可调性

## 代码质量与编译
- [x] Keil 编译 0 错误 0 警告
- [x] GB2312 编码文件保持 GB2312（mid_key.c/h, mid_encoder.c/h, mid_chassis.c/h, mid_line.c/h, app_scheduler.c, main.c）
- [x] 新增函数与重要变量均有详细注释
- [x] 未引入未使用的 include / 变量
- [x] 未擅自优化/重构与本次修改无关的代码
- [x] 未删除项目原有死代码
- [x] 遵循项目现有代码风格
- [x] mid_imu 模块未被修改（K1 复用现有 CalibrateGyro）

## 交互假设确认（用户审核 spec 时确认）
- [x] 跨模式按键忽略（K1运行中按K2无效，K2运行中按K1无效）
- [x] "显示1在OLED上" = 显示里程数
- [x] K2 模式不显示里程
- [x] VOFA 运行中修改 vx 的处理策略
