# Tasks

- [x] Task 1: 在 mid_key.h 新增发车模式枚举、可调参数宏、模式查询 API 声明
  - [x] SubTask 1.1: 新增 `MID_LaunchMode_t` 枚举（MID_MODE_IDLE / MID_MODE_K1 / MID_MODE_K2）
  - [x] SubTask 1.2: 新增 `MID_Key_GetLaunchMode()` 函数声明
  - [x] SubTask 1.3: 新增可调参数宏（K1 发车偏移+200、K2 发车偏移-100、减速阈值 3000mm、步进 ±10、减速终点偏移-100、步进周期 20ms）
  - [x] 验证：mid_key.h 编译通过（无语法错误），不破坏现有 API

- [x] Task 2: 在 mid_key.c 实现 K1/K2 双模式发车状态机
  - [x] SubTask 2.1: 新增静态变量 `s_launch_mode`（MID_MODE_IDLE），`MID_Key_IsRunning` 改为返回 `s_launch_mode != IDLE`（兼容现有调用）
  - [x] SubTask 2.2: 修改 `s_handle_start`（K1 发车）：保留 `MID_IMU_CalibrateGyro`，设 `chassis_vx = __chassis_vx + 200`，置 K1 模式，调用 `MID_Encoder_OdomReset()` 里程归零
  - [x] SubTask 2.3: 新增 `s_handle_k2_start`（K2 发车）：校准陀螺仪、重置紧急停车窗口、设 `chassis_vx = __chassis_vx - 100`、置 K2 模式、`MID_Chassis_Start`、蜂鸣器 1 响
  - [x] SubTask 2.4: 修改 `s_handle_k1_press`：停止态→K1发车，K1运行态→停止（保留 toggle）
  - [x] SubTask 2.5: 新增 `s_handle_k2_press`：停止态→K2发车，K2运行态→停止（toggle）
  - [x] SubTask 2.6: 修改 `MID_Key_Scan`：K1 短按走 k1_press；K2 短按走 k2_press（去掉 `(void)LQ_Key_Scan(KEY2)`）。跨模式按键忽略（K1运行中按K2无效，K2运行中按K1无效）
  - [x] SubTask 2.7: 实现 `MID_Key_GetLaunchMode()` 返回 `s_launch_mode`
  - [x] SubTask 2.8: `s_handle_stop` 置 `s_launch_mode = IDLE`；K1发车置 `g_line_emergency_enable=1`，K2发车置 `g_line_emergency_enable=0`，停止恢复 `=1`
  - [x] 验证：K1/K2 发车与停止逻辑符合 spec 场景；`MID_Key_IsRunning` 在 K1/K2 均返回 1

- [x] Task 3: 在 mid_encoder.h / mid_encoder.c 新增编码器里程 API
  - [x] SubTask 3.1: mid_encoder.h 新增 `void MID_Encoder_OdomReset(void)` 与 `float MID_Encoder_GetOdomMm(void)` 声明
  - [x] SubTask 3.2: mid_encoder.c 新增静态变量 `s_odom_start_l`、`s_odom_start_r`（基准脉冲值）
  - [x] SubTask 3.3: `MID_Encoder_OdomReset` 记录当前 `MID_Encoder_GetTotal(L/R)` 为基准
  - [x] SubTask 3.4: `MID_Encoder_GetOdomMm` 返回 `((total_l - start_l) + (total_r - start_r)) / 2 / MID_ENCODER_COUNTS_PER_METER * 1000.0f`（mm）
  - [x] 验证：reset 后里程从 0 开始；左右轮取平均；单位为 mm

- [x] Task 4: 在 mid_chassis.c / mid_chassis.h 新增速度步进 API（供调度器 20ms 周期调用）
  - [x] SubTask 4.1: mid_chassis.h 新增 `void MID_Chassis_RampStep(void)` 声明（根据当前模式做 +10 或 -10）
  - [x] SubTask 4.2: mid_chassis.c 实现 `MID_Chassis_RampStep`：K1 模式且里程≥阈值且 `chassis_vx > __chassis_vx - 100` → -10；K2 模式且 `chassis_vx < __chassis_vx` → +10；IDLE 不动作
  - [x] SubTask 4.3: 步进用 clamp 防止越过上下限
  - [x] 验证：K1 减到 `__chassis_vx - 100` 停止；K2 加到 `__chassis_vx` 停止；IDLE 不动作

- [x] Task 5: 在 mid_line.c / mid_line.h 新增紧急停车使能开关
  - [x] SubTask 5.1: mid_line.h 新增 `extern uint8_t g_line_emergency_enable;` 声明
  - [x] SubTask 5.2: mid_line.c 定义 `g_line_emergency_enable = 1U`（默认启用，兼容现有行为）
  - [x] SubTask 5.3: `MID_Line_CheckEmergency` 入口判断 `if (!g_line_emergency_enable) return;`
  - [x] 验证：使能=1 时紧急停车逻辑与原有一致；使能=0 时完全跳过检测

- [x] Task 6: 修改 app_scheduler.c 集成速度步进与 K2 跳过紧急停车
  - [x] SubTask 6.1: 新增 20ms 节拍计数（4 帧一次），调用 `MID_Chassis_RampStep()`
  - [x] SubTask 6.2: 紧急停车检测 `MID_Line_CheckEmergency` 调用保留（由 `g_line_emergency_enable` 开关控制，K2 模式 mid_key 已置 0）
  - [x] 验证：20ms 速度步进正确；K2 模式不触发紧急停车；里程按需读取无需积分

- [x] Task 7: 修改 main.c 的 `s_screen_refresh` 增加 K1 模式里程显示
  - [x] SubTask 7.1: 包含 mid_key.h（已有）与 mid_encoder.h（已有）
  - [x] SubTask 7.2: K1 模式（`MID_Key_GetLaunchMode() == MID_MODE_K1`）时追加显示 "DIST:" + 里程(mm)
  - [x] SubTask 7.3: 非 K1 模式保留现有"TIME:"显示
  - [x] 验证：K1 模式 OLED 显示里程与时间；K2/IDLE 模式仅显示时间

- [x] Task 8: 编译验证与全面检查
  - [x] SubTask 8.1: 用 Keil 编译，确保 0 错误 0 警告
  - [x] SubTask 8.2: 检查所有新增/修改文件编码（GB2312 文件保持 GB2312，新文件 UTF-8）
  - [x] SubTask 8.3: 检查注释完整性与一致性（每个新函数、重要变量有注释）
  - [x] SubTask 8.4: 检查未引入未使用的 include / 变量
  - [x] 验证：build log 0 error 0 warning

# Task Dependencies

- Task 2 依赖 Task 1（需 mid_key.h 的枚举与宏）和 Task 3（需里程归零 API）、Task 5（需紧急停车开关）
- Task 3 独立（mid_encoder 扩展）
- Task 4 依赖 Task 1（需模式枚举）和 Task 3（需里程查询）
- Task 5 独立（mid_line 开关）
- Task 6 依赖 Task 4、Task 5（集成）
- Task 7 依赖 Task 1、Task 3（模式查询与里程查询）
- Task 8 依赖 Task 1~7 全部完成

# 可并行任务

- Task 1 + Task 3 + Task 5 可并行（不同文件，无依赖）
- Task 2 在 Task 1/3/5 完成后可与 Task 4 并行
