# R17：恢复原遥测，补齐角速度环 RAM 日志

2026-10-20。先撤回未完成的 FAST/FTEL 压缩实时遥测，六个相关文件曾与 R16
基线提交 `41ee7d9e` 比对一致；随后仅按用户补充增加角速度日志及 mode 字段。
没有保留 FAST 命令或采样钩子，不改 UART、PID、控制周期、电机符号、运动学或
操控行为。最初R17提交未连接实车、烧录或启动电机；后续状态见末节。

## 日志

HLOG 在每次实际 20 ms 角环更新后记录 mode=1（角速度 PI）和 mode=2（航向级联）。
mode 使用原结构体对齐空位，单条 RAM 记录大小不变，容量仍为 1024 条，约20.48秒。
这不是提高 ATEL 发送频率；运行时不写 Flash。

```text
HLOG,DATA,seq,tick,heading_target_mdeg,heading_actual_mdeg,heading_error_mdeg,
rate_target_mdps,rate_actual_mdps,heading_output_mdps,rotation_output_mrpm,
saturated,forward_rpm,left_rpm,mode
```

角速度整定使用 rate_target/rate_actual/rotation_output（deg/s、deg/s、RPM）。
航向外环使用 heading_target/heading_actual/heading_output（deg、deg、deg/s），
不能用内环 RPM 代替外环输出。mode=1 的航向列不是活动的外环。

每轮：停止状态下 HLOG,CLEAR → HLOG,START → 获授权后执行基线/阶跃/稳定段 →
STOP并确认停车 → HLOG,STOP → HLOG,STATUS → 最多8条一批下载 → 验证保存 →
分析 → 有授权时写参数并回读 → 下一轮。没有额外的每轮硬件确认要求。
读取时角环仍在运动或记录未停止，固件会拒绝；这些日志命令本身不解锁、不启动电机。

网页支持两种角环并在 CSV 追加 mode。旧14字段记录只作未知模式展示，不自动当作
角速度数据。空日志、溢出、覆盖、缺号、模式不明不能作为完整整定数据。
pid_ws_logger.hlog_samples 按模式映射高频数据并验证序号、有限值、MCU tick顺序、
回绕及20..60 ms间隔。Session.read_hlog 在读取前检查状态、读取后验证记录。
direction_identify 现在实际用下载 HLOG 算指标，不再只导出它却用 ATEL 评分。

边界：旧默认step/spin仍有低频评分路径，适配器还检查旧R10版本/能力；本轮没有
宣称已经完成整个R17自动整定器。PID skill 明确禁止这些旧路径自动出参/写回，
适配高频采集和评分前只能诊断。轮速10 ms RAM日志不在本轮范围，不能用角环日志
冒充轮速日志。1024条覆盖后也不能把丢失起始瞬态的剩余尾段当成完整试验。

## Skill 与备份

`C:/Users/28097/.codex/skills/pid-tuning/` 已规定：整定决策只能使用校验完整的
板载高频日志；不能用 TEL/ATEL、截图或插值伪造高频数据；不合格日志不出参、
不写回。真实采样率必须足以解析瞬态，不是任意 PID 都默认50 Hz足够。

`C:/Users/28097/.codex/skills/nuedc-stm32-mspm0-skill/` 已规定每版交付/烧录代码
必须有本地Git commit，包含版本、验证状态与镜像哈希；先备份上一版再开发新版。
只提交相关目录，不自动push、不重置历史、不改Git身份。已有同版提交则核验复用。
本地skill安装文件位于仓库外，本次代码commit不代表这些安装文件也已纳入Git。

## 离线验证

- 2305项固件协议/运动学断言通过，含mode=1的目标/反馈/输出、活动日志拒读、
  停止后读取、容量和覆盖标志；IMU、PS2及角环集成回归通过。
- 网页64项Node、适配器23项Python测试通过；两个skill通过quick_validate。
- Keil ARMCC5.06 update5 build528：0 errors / 0 warnings；
  Code=51010、RO-data=2262、RW-data=168、ZI-data=58320。
- HEX：`MDK-ARM/vgt6_module_test/vgt6_module_test.hex`；SHA256：
  `121485C1C256E5EB60641ED4BE2607EE237B095462B1B8762556675CAB08C60B`。
- 未烧录、未做实车采集或PID整定；主机和假设模型测试不是实测性能。

## 后续：R17 高频日志自动整定入口适配

attitude_tuner.py 现在严格要求R17版本和完整CAPS（包括HLOG_MODE），支持
角速度PI/航向PD阶跃以及角速度正负转向测试，评分只使用下载HLOG，不用ATEL。
原先默认step/spin低频路径已从run移除；旧自动heading_round_trip拒绝启动，
direction_identify保留为高频日志的运动采集/识别，不写参数。轮速高频整定仍未实现。

每轮STOP后确认mode=0且速度环失能，再停日志、下载；先保存原始JSON，再验证
评分，最后仅在停车时写当前环RAM参数并回读。一次确认/ARM，后续自动循环。
每次写参仍限20%，不碰其他环，不写Flash参数。失败轮保存错误、原始记录/报文、
HLOG状态、控制命令时间线、低频安全诊断；不从失败轮出参或回退决策。

记录计划最多18秒（1024条20.48秒预留延迟）：默认step4+6秒、spin4+6+6秒。
读回要求20..25ms间隔、连续序号、模式匹配；每个阶跃至少10个独立10–90%瞬态
样本、充分基线和终段。50Hz解析不了过快瞬态就停止，不插值、不退回ATEL。

用户已授权本项目以后“完成→验证编译→commit→检测并烧录”；STM32 skill已保存
该项目专用偏好。无电/连接异常停止；不等同于授权自动开电机或实车整定。
本轮固件源码/HEX保持已备份的R17，不为Python适配器改动重复升固件版本。
最新烧录结果将在完成校验后追加。

适配器26项Python测试通过，覆盖真实下载批次解复用、错误ATEL不影响HLOG评分、
正负转向、只写当前环、停车写参/回读/恢复已测最佳、坏日志禁止写参及只读不运动。
固件2305项断言与PS2/IMU集成回归、网页64项测试均通过；两个skill校验通过。
Keil重新编译0 errors/0 warnings，HEX哈希仍为上述121485C1...B08C60B。
