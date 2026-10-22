# R21：TOF 刷新延迟修正、单轮限制来源核查

2026-10-22，用户要求核查单轮限制来源、缩短近距离识别延迟；车已上电。沿用本项目验证、Git 备份、自动烧录授权，全程不 ARM、不运行电机、不调 PID。

## 单轮限制的实际位置

下位机 `SpeedControl_SetTarget()` 仅更新指定物理通道，不会清零其他通道；四条 `WSPD,A/B/C/D,rpm` 可同时保持四轮目标，`SpeedControl_Process()` 每次更新四个独立 PI。300 ms WKEEP 看门狗对整组目标生效。固件没有只准一轮非零的检查，R21 没有更改此行为。

限制来自 PID skill 的 `references/vgt6-wheel.md`、`wheel_tuner.py --wheel` 和 `wheel_log.py` 的 `single-wheel isolation violated` 评分门槛。本次按用户“如果是下位机规定才改”的条件保留单轮自动整定流程。新增真实固件主机测试证明四轮 WSPD 同时产生 PWM、超时同时停止。此测试使用电机 stub，不是实车运动证据。更改为四轮同时自动整定需要另行定义实验和多轮质量校验，不能删除隔离检查后沿用原评分结论。

## TOF 修改

- 原测量预算 100 ms、间隔 120 ms、ready 轮询 10 ms；原距离流 500 ms。
- 按 ST ULD `VL53L1X_SetTimingBudgetInMs(33)` 的长距离模式配置：0x005E=0x0060、0x0061=0x006E，保留原长距离模式和 1..4000 mm 有效范围检查；预算 33 ms、间隔 40 ms、轮询 2 ms。间隔仍用校准振荡器低 10 bit ×40×1.075。未更改 PB8/PB9、I2C1 400 kHz 或未接线的 XS。
- `Tof_Process()` 移到手柄、点动保护、速度环之前，消费完成的测距后再处理电机保护；全部传输仍非阻塞，不在 ISR 格式化、延时或控制电机。
- 120 ms 未更新即标为过期，ready 等待超时从 600 ms 缩至 200 ms，断线仍每秒重试。过期/无效时维持原请求速度并报警，保留用户选择的故障策略。
- TOF 实时流单独使用时 100 ms；与 TEL/ATEL 同开时 TOF 200 ms、TEL/ATEL 500 ms，9600 baud 下即使 55 B TOF 与 255 B 状态帧也低于 800 B/s，保留 ACK 余量。日志记录和连续下载期间仍暂停 TOF 串流，板载测距和保护持续运行。
- 原 8 字段 TOF 报文保持。增加只读 `TOFINFO`：`TOFINFO,LONG,33,40,2,120,samples,tick_ms,age_ms`，用于通过样本计数检查真实测量频率，不把显示频率当成传感器频率。
- CAPS 增加 TOF_FAST，固件/显示/烧录核对标记为 R21。网页脚本 v33，按能力使用 120 ms 新鲜度门槛；兼容旧固件。角环和轮速适配器同步 R21 精确身份，控制方程、增益、HLOG/WLOG 格式和采样门槛不变。
- 100→50 mm 只限前进、≤50 mm 停止前进，倒退/横移/转向仍可用；没有做实车刹停距离验收。

ST 配置来源：[STMicroelectronics stm32duino VL53L1X driver](https://github.com/stm32duino/VL53L1X/blob/main/src/vl53l1x_class.cpp)，SetTimingBudgetInMs / SetInterMeasurementInMs。

## 构建与离线验证

沿用 STM32F407VGT6 / Cube FW_F4 V1.28.3 / DFP3.1.1 / ARMCC5.06u5 build528 / 原 Keil 工程，无原生硬件配置变化，无需 CubeMX 重生成。

Keil 0 错误 0 警告；Code60030、RO2618、RW216、ZI103816。
HEX SHA256：`422B07D54AA86CD3D28A282467748301BB0E5C20E96F0E923F215E067666A70A`。

固件协议 2366 项断言通过，包括四轮并行目标及统一看门狗、10/5 Hz TOF 与共享 UART 周期、TOFINFO、日志期间暂停。ToF 状态机/保护联合模拟通过：40 个不同采样相位的 50 mm 阶跃均 ≤46 ms 可见，另覆盖 75 mm 限速、状态无效、超量程、过期、断线重连和 tick 回绕。网页 74/74；桥接测试及 R21 角环只读兼容测试通过。适配器原完整 59 项通过，新增 R21 用例另行通过。

本轮未改 PS2 控制模型、未重跑其已知失败的 reverse_arc<0.5° 合成扫频，不宣称该历史问题已修复。模拟 46 ms 上界不能当成实车响应或刹停时间。

## 板上验证

烧录前 R20 已 DISARM，角环 mode=0、speed=0、四轮目标/输出零、PS2 锁定居中；TOF 有效。开机后已有 UART FE=1、LINK uart_errors=1，保留作为前置计数，不能称原计数为零。检测仅一枚匹配 PUSO DAPLink 1234:5678 / puso_daplink_lab。

完成源代码备份后再烧录，烧录及实际频率结果在完成后追加。验证原始 wire/命令见 `validation/r21-before-distance.json`、`validation/r21-after-distance.json`。这些采集仅观察测距，不用于 PID 评分。
