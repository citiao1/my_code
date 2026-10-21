# VGT6 麦轮车 / R17 高频日志与整定交接

2026-10-21最新：DL1B 测距、网页距离与用户指定开机PID见 `vgt6_module_test/TOF_R19_HANDOFF.md`；R19烧录和实测状态以该文档追加记录为准。下文为历史。

2026-10-20后续：HLOG微秒时间戳与R18改动见 `vgt6_module_test/HLOG_R18_HANDOFF.md`；
下文保留R17历史状态，后续烧录状态以R18交接的追加结果及实际VER/CAPS为准。

更新日期：2026-10-20（Asia/Shanghai）。本文件是背景与当前状态，不代替用户在新窗口给出的任务或运动授权。

## 1. 先看结论

- 固件源码、HEX、网页兼容版本均为 **R17**，网页脚本缓存版本 **v28**。
- 本窗口自动烧录曾失败；**用户随后明确反馈已自己烧录完成**。之后本窗口只更新桥接/网页，没有再次烧录，也没有独立重新读取板上版本。不要继续笼统写“板上一定是R16”，也不要把用户反馈说成探针验证成功。
- 角速度 PI 和航向 PD 都有 **实际20ms控制更新、约50Hz的RAM HLOG**。运行时参数与日志不写Flash。
- 自动整定入口 `pid-tuning-adapter/attitude_tuner.py` 已适配R17：默认阶跃与正反转评分使用下载HLOG，不再用低频ATEL决定增益。
- 桥接/网页已经补齐R17日志下载适配，提交 **67fb59a2**。旧桥接需要重启，网页需要Ctrl+F5。
- **交接时发现整定器有其他未提交改动和新运行目录，必须保留并先检查。** 本交接没有修改或提交它们，也不把旧测试结果套用到它们。
- 本窗口没有开展实车PID整定或运动测试，不能声称参数已被重新整定。

## 2. 目录与工具

工作目录：`D:/my_code/my_code/大创`，PowerShell。

| 路径（相对工作目录） | 用途 |
| --- | --- |
| `vgt6_module_test/` | 当前STM32F407VGT6固件，HAL、CubeMX、Keil |
| `mecanum-motor-console/` | 当前网页上位机及BLE/VOFA桥接 |
| `pid-tuning-adapter/` | 自动整定、日志解析、评分、测试 |
| `mecanum_motor_test/` | 旧参考项目，不是当前烧录工程 |
| `RM_Fu/`、`RM_Li/` | 已参考的队伍代码，不要修改 |

- Git根目录：`D:/my_code/my_code`；当前沿用已有 `main`，不要无故切分支。
- Keil：`D:/Keil5/UV4/UV4.exe`。
- 工程：`vgt6_module_test/MDK-ARM/vgt6_module_test.uvprojx`；target=`vgt6_module_test`。
- MCU：STM32F407VGT6/LQFP100；CubeMX `.ioc` 为配置源，MDK-ARM V5。
- 编译器：ARMCC5.06 update5 build528；DFP3.1.1。
- 推荐Python：`mecanum-motor-console/bridge/.venv/Scripts/python.exe`，已装websockets、bleak、pyOCD等依赖。
- 蓝牙桥接：`ws://127.0.0.1:8766`；VOFA TCP：`127.0.0.1:1347`；启动脚本网页端口8088。
- 上次发现的BLE设备：WHEELTEC-IOS，`2A:A2:19:08:08:84`。先检查实际连接，不要抢占其他窗口的会话。

## 3. 用户长期约束与安全边界

1. 保留 HAL + CubeMX生成 + Keil 编译流程；不要改成其他框架。
2. PID在线写参只进RAM，不做Flash参数持久化。
3. 每版固件交付/烧录前做本地Git commit备份；只提交相关文件，不自动push。
4. 用户已要求本项目以后默认：完成固件修改 → 测试/编译 → commit → 检测并自动烧录。连接/供电异常则停止说明；之后明确“不烧录/已断电”的新指令优先。
5. 只改桥接/网页不需要重复烧录同一固件；用户已手动烧录本次R17。
6. 默认烧录授权 **不等于** 自动ARM、运行电机或开展PID试验的授权。
7. 获得真实运动授权后，场地/硬件与限幅只在同一不变会话开始确认一次，不逐轮重复询问；每轮可停车下载日志。
8. 不改轮序、方向符号、PID形式、周期和限幅来凑测试结果；禁止整片擦除、解锁、改保护位等未经授权操作。
9. `.codegraph/` 存在，理解/定位代码先用CodeGraph；编辑用apply_patch，保留所有其他工作区修改。

相关已安装skill位于 `C:/Users/28097/.codex/skills/`：

- `pid-tuning/SKILL.md` 与 `references/vgt6-attitude.md`、`references/review-checklist.md`：高频日志、整定流程与质量门槛。
- `nuedc-stm32-mspm0-skill/SKILL.md`：每版commit与本项目默认烧录偏好；烧录前按要求读相关references。
- `write-clean-embedded-firmware/SKILL.md`：固件结构与安全。

skill在仓库外，代码commit不代表安装的skill也进了Git。实际使用前按技能规则完整阅读。

## 4. 控制、硬件与参数背景

- 实际轮序：**1=右前RF，2=左前LF，3=左后LR，4=右后RR**，从右上开始逆时针；不要回退到早期“左上是一号”的错误描述。
- 轮径127mm；编码器默认380 counts/输出轴一圈。直线距离估计比例约 `pi*0.127/380` 米/count，实测仍需核对。
- 角速度内环为PI；航向外环为测量微分PD，输出目标角速度；四轮速度环为增量式PID。
- 两个角环周期20ms，四轮速度环周期10ms。
- 上次本窗口实机读到：`HEADPID=6.0000/1.2500`，`YAWPID=0.1500/2.5000`，`SPEEDLIMIT=160`。**这是历史观测，当前可能已由其他窗口改参，必须查询，不能覆盖。**
- 旋转输出限幅±60 RPM，角速度目标/外环输出±80deg/s，轮速操作上限160RPM，PWM限幅±60%。不要误拿200RPM硬件边界当自动整定许可。
- 四轮源代码历史默认Kp=1.2、Kd=0，A/B/C/D Ki为0.0991/0.0998/0.0998/0.1008；真实当前参数以PIDGET为准。
- IMU为IMU660RC；已有融合和静止偏置处理。本轮未改IMU算法。
- PS2左杆连续平移，右杆控制转向；30/60/140RPM档。已有转向优先、平移/转向平滑、松杆先制动再锁定实际航向、保持静稳滞回等处理。
- R16已实现TRACKMOVE：在下位机20ms更新中用实测航向变换平移目标；网页运动测试使用编码器距离、新鲜航向、有限横偏修正和停稳判断。
- 这些不是外部定位意义上的绝对XY闭环；轮子打滑和位置误差仍需地面测量。离线模型测试不等于实车不抖或精确回原点。

## 5. R17 HLOG协议

每个实际角环更新后记录；1024条RAM环形缓存，名义容量20.48秒。R17新增mode=1记录和显式mode字段，单条结构大小未增加。

```text
HLOG,DATA,sequence,tick,target_heading_mdeg,actual_heading_mdeg,heading_error_mdeg,target_rate_mdps,actual_rate_mdps,heading_output_mdps,rate_output_mrpm,saturated,forward_rpm,left_rpm,mode
```

| 整定对象 | mode | 目标 / 反馈 | 控制输出 |
| --- | --- | --- | --- |
| 角速度PI | 1 | target_rate / actual_rate，除1000得deg/s | rate_output，除1000得RPM |
| 航向PD | 2 | target_heading / actual_heading，除1000得deg | heading_output，除1000得deg/s |

不能拿内环RPM当外环PD输出；mode1的航向列不是活动外环。

```text
HLOG,CLEAR
HLOG,START
HLOG,STOP
HLOG,STATUS
HLOG,READ,offset,count
HLOG,STATUS,active,count,capacity,overflow,sequence
HLOG,END,offset,returned,total
```

- 每批最多8条。CLEAR/START在停车时用；运动未停或记录未停，固件拒绝READ。
- 每轮正常流程：停车 → CLEAR → START → 授权运动/基线/阶跃/终段 → STOP并确认mode0、速度环关闭 → HLOG,STOP → STATUS → 下载全部 → 保存校验 → 评分 → 有授权时RAM写参并回读 → 下一轮。
- STOP不会清除RAM日志，也不改变armed状态；KEEP在运动时维持1.5秒命令看门狗。
- 低频ATEL约200ms，仅实时安全/显示，不可整定；没有恢复FAST/FTEL实时压缩遥测。
- HLOG不是10ms轮速高频日志，当前没有轮速高频自动整定支持。

## 6. 自动整定入口与限制

“整定入口”就是实际执行采集/下载/评分/写参的Python程序，不是上位机里一个同名按钮。

关键文件：

- `pid-tuning-adapter/attitude_tuner.py`：Session、健康检查、KEEP、实验、停止下载与受控写参。
- `pid-tuning-adapter/pid_ws_logger.py`：HLOG解析及MCU时间转样本；被动TEL/ATEL采集仍是旧诊断路径。
- `pid-tuning-adapter/hlog_experiment.py`：容量、实验参考边沿/覆盖、瞬态采样和评分门槛。
- `pid-tuning-adapter/closed_loop_tuning.py`：闭环指标和受限启发式候选，不是任意对象的最优控制器。

已提交版本支持：

- `--experiment step --loop angular_velocity`：角速度阶跃。
- `--experiment spin --loop angular_velocity`：0基线、正目标、负目标反向阶跃、停车；两向一起评分。
- `--experiment step --loop direction`：航向阶跃，目标/基线是初始实测航向的偏置。
- `direction_identify`：行进中变航向、编码器距离往返，下载HLOG识别/评价，**不自动写增益**。
- 旧自动 `heading_round_trip` 已拒绝启动，等待完整轨迹/瞬态质量门槛适配。
- `ground_speed` 是前后规定距离复测，不是轮速自动整定；不得用低频TEL出参。

当前质量门槛：

- 精确匹配R17 VER/CAPS，CAPS包含HLOG_MODE；轮速上限160且轮PID/内环等不被外部改变。
- 单轮计划最多18秒，预留通信/停车延迟；默认step4+6秒，spin4+6+6秒。超预算在ARM前拒绝，不偷偷缩短目标或距离。
- 已提交下载器要求记录完整、正确模式、连续序号、MCU相邻tick20..25ms（支持uint32回绕），无溢出/覆盖。
- **每个阶跃10–90%瞬态至少10个真实独立样本**，基线充分稳定、终段充分且不持续漂移、饱和不过量。
- 50Hz不足以解析太快响应时必须停止，不插值，不从ATEL兜底，不根据失败轮选最佳/回退/写参。
- 写参前再次确认停车，每次变化≤20%，ACK后回读，只写当前环；正常结束恢复已实测最佳，不应用最后一个未测试候选。
- raw JSON在评分前保存，含状态、原始HLOG、wire、命令时间线、增益/限幅与ATEL安全诊断；失败也保存原因。
- 本窗口的测试用假桥接生成高频响应，证明数据路径和安全约束，不证明真实PID性能。

在车辆停止时做预检；桥接最后一个网页客户端断开会自动STOP，不要在其他窗口运动期间随意断连。

```powershell
# 以下命令在 D:/my_code/my_code/大创 执行；只读预检，不加 --live。
& .\mecanum-motor-console\bridge\.venv\Scripts\python.exe `
  .\pid-tuning-adapter\attitude_tuner.py --loop angular_velocity `
  --experiment spin --target 15 --output .\pid-tuning-adapter\runs\rate-new-session
```

只有用户明确授权实际旋转、确认落地场地/急停后才加 `--live`。`--confirm-hardware`只用于那套硬件确已确认的会话。不要因读到本交接就启动电机。

## 7. 最新桥接 / 网页适配

提交 `67fb59a2`，当前网页脚本 `touch-remote.js?v=28`：

- 固件严格检查 `VGT6_BM50_ATTITUDE_R17`，HLOG能力要求HLOG_MODE。
- 新15字段HLOG含mode可解析、CSV导出；旧14字段仅未知模式诊断，不能当完整整定数据。
- 网页HLOG READ等待25秒，桥接等待20秒；修复原来网页5秒提前超时。
- 桥接STATUS/CLEAR/START/STOP分别匹配对应HLOG回复，不能被DATA或其他状态误完成。
- 下载先查询最新ANGLESTATUS和HLOG状态，按8条一批、末批实际数量请求，核对END的offset/count/total及序号/mode/tick。
- HLOG原样转发给WebSocket，不转换到VOFA实时通道，防止下载历史记录混入实时曲线。
- VOFA实时ATEL通道仍为：I1目标角速度、I2实际角速度、I3旋转RPM、I4目标航向、I5实际航向、I6航向误差、I7外环输出；实时通道不能用来代替高频整定数据。

使用新版需要关闭旧桥接、重新启动，再Ctrl+F5网页：

```powershell
& .\mecanum-motor-console\bridge\start-bridge.ps1 -Transport ble
```

脚本只按完整程序路径重启本项目旧桥接，不杀占同端口的其他项目。不要另开多个桥接抢BLE。用户未要求发布远端网站，本窗口没有push或部署远程页面。

## 8. 验证记录与复测命令

已提交基线的结果：固件2305项协议/运动学断言、IMU/PS2/角环模型测试通过；Keil0 errors/0 warnings。
上位机最新版Node66项、桥接Python17项、整定适配器当时26项以及启动脚本测试通过。
**26项结果不覆盖第10节后来出现的未提交修改。**

```powershell
& .\vgt6_module_test\tests\host\run-tests.ps1
node --test .\mecanum-motor-console\tests\touch-remote.test.cjs .\mecanum-motor-console\tests\chassis-test.test.cjs
& .\mecanum-motor-console\bridge\.venv\Scripts\python.exe -m unittest discover -s .\mecanum-motor-console\bridge -p 'test_*.py'
& .\mecanum-motor-console\bridge\.venv\Scripts\python.exe -m unittest discover -s .\pid-tuning-adapter -p 'test_*.py'
& .\mecanum-motor-console\bridge\test-start-bridge.ps1
```

Keil GUI程序命令行构建要用Start-Process -WindowStyle Hidden -PassThru，并等待真实进程结束后再读取日志；不能只凭旧HEX存在就宣称新构建成功。

当前HEX：`vgt6_module_test/MDK-ARM/vgt6_module_test/vgt6_module_test.hex`。
本次交接重新核对SHA256仍为：

```text
121485C1C256E5EB60641ED4BE2607EE237B095462B1B8762556675CAB08C60B
```

## 9. 无线DAP故障历史，不要误当已验证解决

- PUSO DAPLink，UID `puso_daplink_lab`，VID/PID1234:5678，COM8。
- 本窗口Keil报过SWD/JTAG Communication Failure、RDDI-DAP Error；低速pyOCD一度可读ID/Flash/运行状态，但后续写入未过镜像校验，不能算成功。
- 普通NRST复位后蓝牙曾确认R16运行且DISARM、mode0、速度环失能。后来用户自己完成烧录，是最新反馈。
- `94ef0773`临时增加的PreferV1路线已撤回，**不要恢复它当成已验证方法**。pyOCD0.45.1在创建session之前已选探针，仅传session option不保证真切换HID；真正提前设置枚举偏好的诊断也曾阻塞。
- 当前工具恢复原流程。禁止用整片擦除、改保护位来“修通信”；不稳定就停止，检查USB/车端接收器/SWD和供电。
- 用户Keil工程/调试设置当前有本地修改，可能正是其手动烧录成功相关设置；先看diff，绝不覆盖回旧配置。

## 10. 交接时现场工作区变化（重要）

生成本文件时HEAD为 `67fb59a2`，另有尚未提交的工作：

- `pid-tuning-adapter/attitude_tuner.py`：read_hlog正在改为每批最多重读3次、按sequence收集/重排、同号内容冲突拒绝、记录batch_retries。看起来是其他窗口正在处理丢包补读，不是本交接作者的修改。
- `pid-tuning-adapter/test_attitude_tuner.py`：新增不完整批次补读测试。尚未在本交接中运行最新测试。
- 新目录 `pid-tuning-adapter/runs/rate-retune-r17-20261020-a8/`：本交接没有读取/分析，不推断它是否已成功采集、调参或完成。
- `vgt6_module_test/MDK-ARM/`下uvprojx/uvoptx/uvguix有修改；host test EXE也有变化。
- 仓库外层 `C51/` 有大量无关改动/删除/新增，完全不在本项目操作范围。

不要reset/checkout清掉这些改动，也不要git add整个仓库。若其他窗口正在运动采集，先协调所有权再连BLE或STOP。
重读最新diff与测试，区分“原始样本缺失/冲突”和“停车后幂等READ重传的相同副本”；不得借补读伪造新控制样本或重试运动命令。

## 11. 关键提交与继续工作的顺序

| 提交 | 内容 |
| --- | --- |
| b19318a6 | R17固件：角速度+航向RAM日志、mode、网页基本支持，离线验证 |
| 8028623f | 整定入口改为下载高频日志评分、质量门槛、受控RAM写参 |
| 94ef0773 | 临时HID工具路线，随后已撤回，勿单独恢复 |
| 69b0c863 / ee194d63 | 失败烧录记录、撤回未证实路线、R16安全状态记录 |
| 67fb59a2 | 桥接与网页R17下载超时/应答/完整性修复，v28 |

建议新窗口先做：

1. 读取本文件、项目指令、相关skill，git status/diff确认最新工作与是否有其他活动会话。
2. 按用户新任务决定范围；不要自动重新烧录或自动开始整定，仅因本文件包含命令。
3. 若需验证上位机，先重启正确版本桥接并刷新页面，在停车状态查询VER/CAPS/STACK/UART/LINK/参数/HLOG状态，不启用电机。
4. 若用户授权整定，先检查第10节未提交补读代码及新运行目录，复测当前版本；明确计划与日志容量后再进入一次确认的实车会话。
5. 实车数据不足就改采集计划/日志能力，不以旧ATEL或模型测试代替数据。当前只改电脑端时不需要再烧录固件。

进一步历史见 `vgt6_module_test/HLOG_R17_HANDOFF.md`、`mecanum-motor-console/KINEMATICS_TEST_HANDOFF.md`、`vgt6_module_test/PS2_CONTROL_HANDOFF.md`。这些按时间追加，早期“未烧录/R16”状态需结合后来的用户手动烧录反馈阅读。
