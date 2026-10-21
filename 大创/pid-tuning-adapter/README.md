# BM50 PID tuning adapter

当前角环入口使用“合格 HLOG → 重复测量 → 有界搜索 → 独立留出验证 → 恢复并读回”。
默认 BO，可用 DE 或原启发式作同流程对照。只读预检是默认行为；实车运行需显式
`--live` 和覆盖全部实验计划的硬件授权。本次升级只做离线验证，没有运行实车。

## 当前能力

| 环路 | 形式 / 周期 | 评分数据与输出 | 自动写回 |
|---|---|---|---|
| 角速度 | parallel PI / 20 ms | mode=1 HLOG，deg/s → rotation RPM，±60 | 验证通过后保留 RAM 参数 |
| 航向 | measurement-PD / 20 ms | mode=2 HLOG，deg → deg/s，±80 | 内环已验证且锁定时可用 |
| 轮速 | incremental / 10 ms | R20 WLOG，RPM → PWM%，±60 | 单轮Kp/Ki_step，Kd固定0，独立验证后保留RAM参数 |

`loop_profiles.py` 定义单位、参数和周期；`attitude_tuner.FIRMWARE_CAPS` 是精确固件
白名单，目前含 R17/R18/R19/R20。运行时读取 VER/CAPS、控制器形式、限幅、周期、
健康及当前增益；源码或历史刷写记录不能代替板上身份。

R17 有 mode 标签；R18/R19/R20 还必须有微秒时间戳。HLOG 样本间隔分别要求
20..25 ms / 19.9..25 ms，校验独立 uint32 计数器回绕和 ms/us 相位。
TEL/ATEL 只用于安全监控，绝不作为角环增益评分数据。桥接是唯一串口/BLE 所有者，
本工具只经 `ws://127.0.0.1:8766` 通信，不直接占用 COM 口。

## 运行

R20轮速用独立入口（默认只读，不会ARM或写参）：

```powershell
.\mecanum-motor-console\bridge\.venv\Scripts\python.exe pid-tuning-adapter/wheel_tuner.py --wheel A --target 40 --validation-targets -40 20 --rounds 5 --repeats 2 --output pid-tuning-adapter/runs/NEW-WHEEL-SESSION
```

只有完整计划明确授权后才加 `--live`。单轮实验要求可靠悬空、四轮自由、急停可用。
轮速 Ki 是每次更新的离散系数，不能直接复制连续 PI 的 Ki。默认基线2s、阶跃3s，
总记录≤9s；100Hz×1024条约10.24s，WLOG与HLOG共用RAM，CLEAR会替换旧数据。
`WSPD,A,RPM`与`WKEEP`使用300ms看门狗；写参需停车并停止记录，保持其他轮与角环增益。
`wheel_log.py`校验完整批次、uint32微秒/毫秒相位、9900..15000us间隔、模式/电池/
编码器/通道隔离/TOF限速、完整阶段与基线/最终窗口、10个瞬态独立样本，失败不写参。
单轮悬空调参不代替底盘落地验收；本次没有运行实车搜索。

WLOG成功或失败的原始wire、批次重试、增益与命令时间线保存在新会话目录。
离线复查：`python pid-tuning-adapter/replay_wlog.py <round-001-raw.json>`，不连接硬件。
R20新车头为TOF端，车体前/左轴反转180°，A/B/C/D物理通道和逆时针转向不变。
距离100→50mm线性限制前进，≤50mm禁止前进，倒退/横移可用；无效/过期测距按用户选择
保持原请求速度并提示异常。任何实际触发限速的轮次不可评分。详见
[`R20交接`](../vgt6_module_test/CONTROL_R20_HANDOFF.md)。

推荐项目已有 `.venv`（系统 Python 可能没有 websockets）：

```powershell
.\pid-tuning-adapter\.venv\Scripts\python.exe pid-tuning-adapter/attitude_tuner.py --loop angular_velocity --experiment spin --target 15 --rounds 5 --search bo --repeats 2 --seed 0 --validation-targets -10 10 --output pid-tuning-adapter/runs/NEW-SESSION
```

该命令只读预检并打印完整计划，不 ARM。已明确授权运动、场地、急停、全部目标、
实验次数和 RAM 写参范围后，才加 `--live`。只有完整计划已在会话中确认时才使用
`--confirm-hardware`。角环需要车体实际旋转，悬空转轮不能验证 yaw。
始终用新输出目录；已有 session/raw 数据会被拒绝覆盖。

新环境可 `py -3 -m venv pid-tuning-adapter/.venv`，再用该解释器安装
`-r pid-tuning-adapter/requirements.txt`。搜索算法本身只用标准库，无 scipy/sklearn 依赖。

## 搜索与接受规则

- `--search bo|de|heuristic`：BO 为固定 RBF GP + expected improvement；DE 为顺序
  rand/1/bin；heuristic 是原有有限规则。它们都使用相同重复测量和验证流程。
- `--rounds 1..20`：训练参数组预算，包含初始参数；`--repeats 2..10` 默认 2。
  单次记录仍不超过 18 s，但整个会话包含多次停止、下载和验证。
- 默认全局边界是初始增益的 60–140%，限制在 0..100；可预先指定
  `--kp-bounds MIN MAX --second-bounds MIN MAX`。第二维分别是 Ki / Kd。
  每次建议和实际写入都受当前值 ±20% 约束，零系数保持零。
- `--validation-targets` 至少两个不同于训练的阶跃，训练与验证合起来覆盖正反向。
  不指定时预检会显示自动生成的反向/不同幅值目标。它们固定在 ARM 之前。
- 默认 `--max-overshoot 0.30`、`--min-improvement 0.01`。这些参数必须预先声明。
  搜索器不能改目标、限幅或时长；参数域有界不等于已证明闭环安全。

先重复测量初始参数的训练和留出试验，再只用训练分数搜索。每个未被接受的训练
候选会先恢复训练 incumbent，再提出下一组局部候选；incumbent 在最终验证前仍是
暂定值。最终只让一个训练 finalist 接受独立留出验证，不把 holdout 反馈给搜索器。

每阶跃固定目标函数（越低越好）：

`J = normalized_IAE + 2*overshoot + steady_error_RMS + 0.5*normalized_settling + 0.1*saturation_fraction`

IAE 和稳态 RMS 按阶跃幅值归一化；整定时间按记录时长归一化，未整定取 1。
spin 取两阶跃分数平均，同时保留最差超调/饱和指标。
训练聚合 `mean(J) + 0.5*sample_stdev(J)`；噪声筛选要求均值改善超过
`max(min_improvement, 1.96*sqrt(var_mean_new+var_mean_old))`。这只是工程筛选，
两次重复不能当成严格统计置信保证。

接受还要求 holdout 总体改善、最差分数不劣化，每个验证工况的均值/最差分数及
超调、稳态 RMS、饱和不超过 incumbent+0.01，整定不超过 +0.1 s。
无可靠改善或留出未通过，恢复实测验证过的初始参数。

## 日志、硬约束与恢复

每次记录 STOP/确认 -> CLEAR/START -> 固定基线/阶跃/稳态 -> STOP/确认 ->
HLOG,STOP/STATUS -> 完整下载 -> 保存 raw -> 评分 -> 健康/增益复核。
状态要求 active=0、overflow=0、count=sequence>0、容量 1024，批次最多 8 条，
序号连续、时间戳/模式正确。读取失败可有界重试，完全相同的重传不算新样本。

评分还检查参考阶段、时长覆盖、无意外平移、反馈方向、饱和 <=25%、一秒稳定基线、
每阶跃至少十个独立 10–90% 瞬态样本，以及末段覆盖、RMS 和漂移。
失败不会插值补齐或进入排名；超调等性能硬约束失败也立即中止。

每次写参均停车并验证当前参数，单步 <=20%，ACK 后读回。正常结束逐步恢复并
精确读回留出验证通过的最好值。异常先 STOP；只有通信/健康/当前参数和锁定内环
仍可核实时才回到已验证初值。失联或外部改参时不盲写，报告恢复未完成。
所有写参仅在 RAM；本工具不写 Flash。

`session.json` 保存计划/profile、固件/CAPS、单位、目标函数、每次重复的指标、
训练汇总、初始/候选 holdout、原始日志路径、写入/读回、失败和最终选择依据。
`round-NN-raw.json` 在评分前保存 status/records/wire/命令时间线/安全遥测，失败也保留；
成功记录另有普通 CSV 和 HLOG CSV。NN 是物理实验序号，不是候选参数组序号。

## 离线比较与历史回放

```powershell
.\pid-tuning-adapter\.venv\Scripts\python.exe pid-tuning-adapter/benchmark_search.py --seeds 3 --rounds 10 --output pid-tuning-adapter/runs/NEW-OFFLINE-COMPARISON
.\pid-tuning-adapter\.venv\Scripts\python.exe pid-tuning-adapter/replay_hlog.py --input pid-tuning-adapter/runs/EXISTING-RUN --output pid-tuning-adapter/runs/NEW-REPLAY.json
```

benchmark 使用真实 HLOG 评分器、简化合成 PI/PD 植物、相同种子/预算、无噪声与有噪声
场景，以及不同幅值/方向/负载的留出验证。输出收敛曲线、试验数、失败数、越界数和
最终保留分数；始终标注 `simulation_only`。它不是 BM50 辨识模型或实机性能证明。

回放仅校验和评分历史 raw JSON，不连接桥接、不提参、不跨会话排名。缺少原始阶跃
phases 的轨迹日志不能强行套用阶跃评分器。日志质量通过也不等于满足新的 30% 超调门槛。

本次结果见 [离线对照](runs/offline-search-comparison-20261021-final/comparison.md) 和
[历史回放](runs/offline-search-comparison-20261021-final/historical-replay.json)。

## 其他入口与明确缺口

- `pid_tuning.py` / `tune_pid.py`：通用稳定自调节 SISO 的开环 FOPDT 候选。
  需要足够采样、执行器阶跃/保持段和已整定响应；闭环参考阶跃不满足这个模型假设。
  parallel/incremental/position-discrete 必须按真实控制周期和公式换算。
- `pid_ws_logger.py`：被动 TEL/ATEL 诊断，不能作为 BM50 自动增益数据源。
- `direction_identify`：HLOG 轨迹采集和模型诊断，无参数写回；自动
  `heading_round_trip` 调参仍禁用，不能用旧例子绕过缺失的轨迹质量门。
- `ground_speed`：按 ENC 距离对当前轮速参数做低频观察，不调轮速 PID；轮径 127 mm，
  每输出转 380 counts。它不能代替 10 ms 轮速高频记录。
- R20已实现WLOG与单轮评分/搜索入口；底盘联合验证仍需另行实测。旧 live_tune_session / R6 低频
  整定流程不能重新启用为自动写参路径；外部 GUI 也需协议和 PID 形式适配。

## 检查

```powershell
.\pid-tuning-adapter\.venv\Scripts\python.exe -m unittest discover -s pid-tuning-adapter -p "test_*.py"
```

测试使用内存 bridge 和合成记录，覆盖数据门、搜索边界/重现性、重复/留出选择、
停车写入/读回、回退，以及故障时停止和恢复未完成。它们不建立实车性能结论。
