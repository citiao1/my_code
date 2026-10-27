# 四轮地面速度环整定交接

更新日期：2026-10-22（Asia/Shanghai）

## 当前结论

- 本轮 R22 四轮地面速度环整定尚未开始采样，更没有合格数据或参数更新。
- 最新预检目录：`runs/R22-ground-speed-A-direction-diagnostic-20261022-06/`。
- 最新记录只发出一条只读 `ANGLESTATUS`，MCU 回复超时；`motion_started=false`，未发出 `ARM`、`DRV` 或 PID 写入。
- 在本次失败轮次中，速度环、角速度环、航向环参数均未改。由于最新预检未走到 `PIDGET`，恢复上位机后应重新读回当前参数，不能仅凭历史值假定板上状态。
- 用户报告上位机启动被 `127.0.0.1:8766` 的 PID 46684 占用。已核实该 PID 是本任务启动的桥接进程并将其结束；结束后端口无监听进程。下一次操作应使用用户上位机启动的桥接，不要另开一个桥接实例。

## 用户已授权的测试计划

- 全程四轮着地、正常承载；路线已清空，带车头 TOF。不得架空或单独测试电机。
- 四轮 A/B/C/D 每次同步运行、同步记录；优化某一通道增益时，其余三轮仍同时参与测量。
- 先做轮速环。用户已授权将训练目标调整为 `30 RPM`，留出目标为 `-30 RPM` 与 `+25 RPM`；每项重复 2 次，最多 5 组候选参数。每个地面测试约 1 m 后按编码器反向返回。
- 速度环变化后再测角速度环；如果速度环增益没有变化，则跳过角速度环，继续优化航向环。
- PWM 上限 60%，单次参数变化不超过 20%，仅 RAM 写入，不写 Flash。
- 任何运动都必须走现有桥接的安全控制路径和四轮同步 `DRV`，全程监督日志、编码器、TOF、超时看门狗及回程；出现质量或通信异常立即停止该轮，不将失败数据纳入评分。

## 固件与采集边界

- 最新成功读到的固件身份为 `VER,VGT6_BM50_ATTITUDE_R22`。
- 对应 `CAPS` 包含 `SPEED_TUNABLE,WLOG_US,TOF_GUARD,FRONT_REVERSED,TOF_FAST,TOF_GUARD_200_100_40`；`WLOGINFO` 为 10 ms、1024 条、incremental、`KI_STEP/KD_STEP`、PWM 60%、160 RPM、380 CPR。
- 使用 `pid-tuning-adapter/wheel_tuner.py` 与 MCU 本地 WLOG。只有完整下载、序号/时基/标志/模式有效、无 TOF 干预且 A-D 各自均通过评分的同步数据才可用于调参。
- 控制更新周期为 10 ms；不能用低频 TEL/ATEL、UI 曲线或主机到达时间代替 WLOG。
- `FRONT_REVERSED` 会影响命令符号与物理前进方向的对应关系。每轮从很短的安全预检开始，并通过编码器位移确认方向；方向不符立即 STOP，不能为了赶计划继续加时或提速。

## TEL 与 ANGLESTATUS 诊断

- 固件 `TEL,0` 关闭周期 TEL 流；`ANGLESTATUS` 走独立的 `Telemetry_SendAttitude()`，直接发送 `ATEL`，不依赖周期 TEL 开关。
- 历史 `diagnostic-20261022-04` 记录中，`TEL,0` 获得 ACK 后，`ANGLESTATUS` 仍成功返回 `ATEL`。
- 最新 `diagnostic-20261022-06/session.json` 中，`ANGLESTATUS` 是唯一命令且为第一条命令，之前没有发送 `TEL,0`。因此最新超时不能由该轮关闭 TEL 引起。
- 为让用户重新启动上位机，已停止本任务的桥接进程；这会停止本任务的遥测转发。当前应由用户上位机重新启动桥接，并独占 8766 端口。不要尝试从另一客户端连接 BLE，也不要在桥接未恢复时写参数或运动。

## 最新失败记录

文件：`runs/R22-ground-speed-A-direction-diagnostic-20261022-06/session.json`

```text
status=failed
motion_started=false
commands=[ANGLESTATUS -> MCU reply timeout]
PID writes=0
```

较早诊断中的 UART framing / LINK 错误计数不为零，且有命令超时、坏帧或丢帧。那些轮次没有有效 WLOG，不得评分。保持原始目录，勿覆盖或拼接修复成训练样本。

## 代码改动与测试

- `wheel_tuner.py`：零目标基线只发送一次 `DRV,0,0,0`，避免重复零命令；WLOG 下载期间暂停周期 TEL，完成后在 `finally` 恢复 `TEL,1`。
- `test_wheel_log.py`：覆盖相应命令次数/遥测恢复行为。
- 上一轮验证记录：Python 编译通过，`test_wheel_log` 14 项通过，整套 adapter tests 67 项通过。当前工作树有大量用户既有修改，续接时不要重置或清理。

## 续接步骤

1. 等用户上位机重新启动后，确认 `8766` 由该上位机桥接监听，并收到 `BRIDGE,BM50_ACK_R3`；不要自行另启桥接。
2. 仅做只读预检：`ANGLESTATUS`、`PS2STATUS`、`VER/CAPS/WLOGINFO`、UART/LINK/STACK、速度上限、角环参数、`PIDGET`、WLOG/HLOG 状态。PS2 必须锁定，角度/速度模式停止，固件身份匹配，错误计数稳定且为零，日志不活动，所有限值和当前增益可读。
3. 任意 MCU 超时、错误计数非零/增长、身份不匹配、日志状态异常或安全模式不符：停止活动，不 ARM、不 DRV、不改参数；保留 raw 命令时间线。不要用重复 reset 或快速重试掩盖问题。
4. 预检全部通过后，依照已授权的四轮同步地面方案进行；每一轮先停止并复核状态，再做 1 m 往返。前进方向必须按编码器实测确认，回程以编码器回到起点为准。WLOG 下载期间暂停 TEL 的代码路径不得绕开 `finally` 恢复。
5. 只在所有轮次数据质量合格且 A-D 分别通过后比较候选。变化前 STOP，按每步不超过 20% 写 RAM、ACK 后 `PIDGET` 精确读回；独立留出验证通过才保留新值，否则仅在链路健康且当前值可读时恢复已验证参数。
6. 速度环阶段真实完成后，再按用户条件决定角速度环或航向环；不能把本交接中的预检失败描述成任何环已经测完。

## 关联路径

- 最新预检：`runs/R22-ground-speed-A-direction-diagnostic-20261022-06/session.json`
- R22 轮速 profile：`wheel_log.py`
- 地面同步采集与受限写回入口：`wheel_tuner.py`
- PID skill：`C:/Users/28097/.codex/skills/pid-tuning/SKILL.md`
