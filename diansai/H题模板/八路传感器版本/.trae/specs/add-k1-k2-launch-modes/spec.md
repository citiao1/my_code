# K1/K2 双模式发车与里程减速/加速 Spec

## Why

当前系统仅 K1 按键负责"发车/停止"切换，发车速度恒为 `__chassis_vx`（默认 120 mm/s），且无里程反馈、无速度渐变机制。需要新增 K2 按键作为第二种发车模式，并引入"按里程减速 / 渐进加速"与"里程数 OLED 显示"能力，以支撑不同赛道场景（K1=快发+到点减速至低速，会触发脱线停车；K2=慢发+渐进回速，不触发脱线停车）。

## What Changes

- **新增 K2 发车模式**：K2 短按从"停止"状态发车，发车速度 = `__chassis_vx - 100`，随后每 20ms `chassis_vx` 递增 +10，直至达到 `__chassis_vx`（原始目标值）后保持。
- **修改 K1 发车模式**：K1 发车速度由原 `__chassis_vx` 改为 `__chassis_vx + 200`；发车后用编码器积分计算行驶里程，当里程 ≥ 减速阈值（暂定 3000mm / 3m）时，每 20ms `chassis_vx` 递减 -10，直至减到 `__chassis_vx - 100` 后保持。K1 发车仍保留原有的 IMU 陀螺仪校准（`MID_IMU_CalibrateGyro`）。
- **新增里程计算（编码器脉冲差分）**：在 mid_encoder 模块扩展，利用 `MID_Encoder_GetTotal` 的累计脉冲差值，按 `MID_ENCODER_COUNTS_PER_METER`（3726 脉冲/米）换算为行驶里程（mm）。左右轮取平均。精度远高于加速度计积分。
- **新增紧急停车模式开关**：K1 模式下保留现有 8 路脱线紧急停车逻辑（`MID_Line_CheckEmergency` 生效）；K2 模式下跳过紧急停车检测（脱线不停车）。
- **新增 OLED 里程显示**：K1 发车模式下在 OLED 显示累计里程（mm），与现有运行时间一同显示。
- **新增发车模式状态查询 API**：`MID_Key_GetLaunchMode()` 返回当前模式（IDLE / K1 / K2），供调度器与显示层判断逻辑分支。
- **可调参数宏定义**：减速阈值、±步进、速度偏移均以宏定义集中在 mid_key.h，便于后续调整（用户已声明"暂定，后续还要改"）。

## Impact

- 受影响 specs：无既有 spec 文档（项目首次引入 spec 流程）。
- 受影响代码：
  - `LQ_MSPM0GX_LIB_V2.0.0\Code\Middle\mid_key.c` / `mid_key.h` — K1/K2 发车状态机、模式查询、参数宏
  - `LQ_MSPM0GX_LIB_V2.0.0\Code\Middle\mid_encoder.c` / `mid_encoder.h` — 新增里程归零与里程查询 API（基于 GetTotal 脉冲差分）
  - `LQ_MSPM0GX_LIB_V2.0.0\Code\App\app_scheduler.c` — 20ms 周期速度步进 + K2 模式跳过紧急停车（里程无需 5ms 积分，按需读取）
  - `LQ_MSPM0GX_LIB_V2.0.0\Code\Middle\mid_line.c` / `mid_line.h` — 新增紧急停车使能开关（或由上层跳过调用）
  - `LQ_MSPM0GX_LIB_V2.0.0\User\main.c` — OLED 新增里程显示
  - mid_imu 模块**不修改**（K1 发车仍复用现有 `MID_IMU_CalibrateGyro`）

## 关键技术说明

1. **编码器里程精度高**：基于 `MID_Encoder_GetTotal` 的脉冲差分换算，无积分漂移问题，`COUNTS_PER_METER=3726` 已实测定标。里程精度取决于编码器机械安装，软件层面无累积误差。
2. **K2 模式不触发紧急停车 = 脱线不保护**：K2 模式下 8 路全脱线也不会停车，车辆可能冲出赛道。这是用户明确要求的语义，非 bug。

## ADDED Requirements

### Requirement: K2 慢发渐进加速模式

系统 SHALL 提供一个由 K2 按键触发的发车模式，从停止状态发车时设置 `g_param.chassis_vx = __chassis_vx - 100`，随后每 20ms 将 `g_param.chassis_vx` 递增 +10，直到达到 `__chassis_vx` 后停止递增并保持。

#### Scenario: K2 从停止状态发车
- **WHEN** 系统处于停止状态（`s_running == 0`）且 K2 被短按
- **THEN** 执行 IMU 陀螺仪校准（复用现有 `MID_IMU_CalibrateGyro`），重置紧急停车标志与窗口，设置 `g_param.chassis_vx = __chassis_vx - 100`，调用 `MID_Chassis_Start()`，进入 K2 模式，蜂鸣器 1 响提示"发车"

#### Scenario: K2 模式渐进加速
- **WHEN** 系统处于 K2 模式且 `g_param.chassis_vx < __chassis_vx`
- **AND** 距上次加速步进已 ≥ 20ms
- **THEN** `g_param.chassis_vx += 10`（不超过 `__chassis_vx`）

#### Scenario: K2 模式加速完成
- **WHEN** `g_param.chassis_vx` 已达到 `__chassis_vx`
- **THEN** 不再递增，保持 `__chassis_vx` 运行

#### Scenario: K2 运行中按 K2 停止
- **WHEN** 系统处于 K2 模式运行中且 K2 被短按
- **THEN** 调用 `MID_Chassis_Stop()`，置 `s_launch_mode = IDLE`，蜂鸣器 2 响提示"停止"

### Requirement: K1 快发按里程减速模式

系统 SHALL 在 K1 发车时设置 `g_param.chassis_vx = __chassis_vx + 200`，并在行驶里程达到减速阈值（默认 3000mm）后，每 20ms 将 `g_param.chassis_vx` 递减 -10，直到达到 `__chassis_vx - 100` 后保持。

#### Scenario: K1 从停止状态发车
- **WHEN** 系统处于停止状态且 K1 被短按
- **THEN** 执行 IMU 陀螺仪校准，重置紧急停车标志与窗口，**重置里程积分归零**，设置 `g_param.chassis_vx = __chassis_vx + 200`，调用 `MID_Chassis_Start()`，进入 K1 模式，蜂鸣器 1 响

#### Scenario: K1 模式里程未达阈值
- **WHEN** K1 模式运行中且累计里程 < 减速阈值
- **THEN** `g_param.chassis_vx` 保持 `__chassis_vx + 200` 不变

#### Scenario: K1 模式达到阈值开始减速
- **WHEN** K1 模式运行中且累计里程 ≥ 减速阈值
- **AND** 距上次减速步进已 ≥ 20ms
- **AND** `g_param.chassis_vx > __chassis_vx - 100`
- **THEN** `g_param.chassis_vx -= 10`（不低于 `__chassis_vx - 100`）

#### Scenario: K1 模式减速完成
- **WHEN** `g_param.chassis_vx` 已达到 `__chassis_vx - 100`
- **THEN** 不再递减，保持 `__chassis_vx - 100` 运行

### Requirement: 紧急停车模式开关

系统 SHALL 在 K1 模式下启用 8 路脱线紧急停车检测（现有逻辑），在 K2 模式下禁用紧急停车检测（脱线不停车）。

#### Scenario: K1 模式触发紧急停车
- **WHEN** K1 模式运行中且 `MID_Line_CheckEmergency` 检测到 8 路中 ≥ N 路为 L 持续满窗口
- **THEN** 调用 `MID_Chassis_Stop()`，置 `g_line_emergency_stopped = 1`

#### Scenario: K2 模式不触发紧急停车
- **WHEN** K2 模式运行中且 8 路全部脱线
- **THEN** 不调用 `MID_Chassis_Stop()`，`g_line_emergency_stopped` 保持 0，车辆继续按当前速度运行

### Requirement: 编码器里程计算

系统 SHALL 在 mid_encoder 模块提供基于编码器脉冲差分的里程计算能力：记录 K1 发车时刻的左右轮累计脉冲基准值，运行中按 `((total_l - start_l) + (total_r - start_r)) / 2 / COUNTS_PER_METER * 1000` 实时计算累计里程（mm），供 K1 模式减速判断与 OLED 显示使用。

#### Scenario: 里程查询正常
- **WHEN** K1 模式运行中调用 `MID_Encoder_GetOdomMm()`
- **THEN** 返回从 K1 发车时刻起的累计行驶里程（mm），左右轮取平均

#### Scenario: K1 发车时里程归零
- **WHEN** K1 发车触发
- **THEN** 调用 `MID_Encoder_OdomReset()` 记录当前 `MID_Encoder_GetTotal(L/R)` 为基准，后续里程从 0 开始累计

#### Scenario: 停止状态里程
- **WHEN** 系统处于 IDLE 状态调用 `MID_Encoder_GetOdomMm()`
- **THEN** 返回 0（或上次停止时的里程，仅供显示，不参与控制）

### Requirement: OLED 里程显示

系统 SHALL 在 K1 模式运行时于 OLED 显示累计里程（mm），与运行时间一同呈现；非 K1 模式下不显示里程（保留原有时间显示）。

#### Scenario: K1 模式显示里程
- **WHEN** 系统处于 K1 模式运行中
- **THEN** OLED 刷新时显示"DIST:"标签 + 里程数值（mm）+ 运行时间

#### Scenario: 非 K1 模式
- **WHEN** 系统处于 K2 模式或停止状态
- **THEN** OLED 仅显示运行时间（保留现有行为）

### Requirement: 发车模式状态查询 API

系统 SHALL 提供 `MID_Key_GetLaunchMode()` 返回当前发车模式枚举（IDLE / K1 / K2），供调度器判断是否跳过紧急停车、显示层判断是否显示里程。

#### Scenario: 查询当前模式
- **WHEN** 上层调用 `MID_Key_GetLaunchMode()`
- **THEN** 返回 `MID_MODE_IDLE`（停止）、`MID_MODE_K1`（K1 模式运行）、`MID_MODE_K2`（K2 模式运行）之一

## MODIFIED Requirements

### Requirement: K1 按键发车逻辑

现有 K1 发车逻辑：发车速度 = `__chassis_vx`，无里程、无减速。
修改为：发车速度 = `__chassis_vx + 200`，重置里程归零，进入 K1 模式，里程达阈值后按 20ms 周期递减 -10 至 `__chassis_vx - 100`。

### Requirement: K2 按键功能

现有 K2 按键在 `MID_Key_Scan` 中被 `(void)LQ_Key_Scan(KEY2);` 忽略，无任何功能。
修改为：K2 短按从停止状态触发 K2 慢发渐进加速模式。

### Requirement: MID_Key_Scan 按键分发

现有 `MID_Key_Scan` 仅处理 K1 toggle。修改为：K1 短按在停止态触发 K1 发车、在 K1 运行态触发停止（保留原 toggle）；K2 短按在停止态触发 K2 发车、在 K2 运行态触发停止（toggle）。

### Requirement: app_scheduler 控制帧

现有 `s_scheduler_step`（5ms）执行：8路采集→位姿环→紧急停车检测→偏航环→SFLP yaw→速度环。
修改为：新增 20ms 节拍（4 帧一次）调用速度步进 `MID_Chassis_RampStep()`；紧急停车检测前判断模式，K2 模式跳过。里程无需在 5ms 周期积分，由 `MID_Encoder_GetOdomMm()` 按需读取（基于脉冲差分，无积分累加）。

## REMOVED Requirements

无移除。K1 原有"停止"语义（运行中按 K1 停止）保留不变。

## 待用户审核确认的假设

以下点未单独提问，按最合理方式假设，请在审核 spec 时确认：

1. **跨模式按键行为**：K1 运行中按 K2、或 K2 运行中按 K1 的行为未明确。假设：**跨模式按键忽略**（K1 运行中按 K2 无效，K2 运行中按 K1 无效），仅同模式按键 toggle 停止。若希望"运行中按任意键均停止"，请说明。
2. **"显示1在OLED上"**：理解为"显示里程数在OLED上"（"1"为笔误）。
3. **减速阈值/步进/偏移参数**：均以宏定义集中，用户声明"暂定后续还要改"，宏定义便于调整。
4. **K2 模式里程**：K2 模式不显示里程、不重置里程（里程仅供 K1 减速用）。
5. **VOFA 修改 `g_param.chassis_vx` 的交互**：运行中若 VOFA 下发 `vx=...`，会直接覆盖当前 `chassis_vx`，可能与渐进加减速冲突。假设保持现有 VOFA 行为（接受覆盖，但下次 20ms 步进会再次覆盖）。请在审核时确认是否需要在运行中屏蔽 VOFA 的 vx 修改。
