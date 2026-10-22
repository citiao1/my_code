# 蓝牙遥控延迟与断联排查交接

更新时间：2026-10-22，Asia/Shanghai。工作区：`D:/my_code/my_code/大创`。

本文是下一窗口的入口，汇总 R23 旧模块诊断、R24 新模块迁移和启动脚本修复。旧文档保留历史状态；当前已烧录版本以本文件、R24 验证记录和下一窗口实际回读为准。

## 1. 当前结论与下一步目标

用户最初反馈：蓝牙遥控延迟、卡顿、间歇断联，怀疑新增 TOF 的频率影响。之后换成已配对的 **LQ208BLE V2.0 模块对**，电脑端直接枚举 USB 串口，固定 **115200 / 8N1 / 无硬件流控**。

现在下位机、桥接、网页和 PID 适配器均已迁移到 R24，新模块双向通信和正式烧录验证成功。已经修复若干确定存在的软件问题，**但蓝牙延迟/断联的完整根因尚未查明，不能宣称已彻底解决**。

新链路仍有约 100–300 ms 的常见命令往返等待，持续测试曾出现 781 ms 的 PING 长尾；短时间没有复现物理断联。下一窗口应拆分延迟来源并复现问题，而不是只提高 UART 波特率、降低 TOF 频率或直接修改 PID。

本次授权与实际工作覆盖通信修改、编译、备份、烧录及停机验证；没有执行 ARM、非零运动或 PID 写参。历史其他窗口的地面调参许可，不能直接当作下一窗口的当前运动许可。

## 2. 当前硬件、连接与启动方式

```text
网页 http://127.0.0.1:8088/
  ↕ WebSocket 127.0.0.1:8766
motor_vofa_bridge.py（一个进程独占串口）
  ↕ COM9，115200 / 8N1，无流控
电脑 USB 端 LQ208 模块 ⇄ 无线透传 ⇄ 车板排针端 LQ208 模块
  ↕ USART2：PA2 TX / PA3 RX，交叉接线、共地
STM32F407VGT6
```

| 项目 | 已确认内容 |
| --- | --- |
| 新通信串口 | COM9，USB-SERIAL CH340，VID `1A86` / PID `7523` |
| 烧录探针 | PUSO DAPLink，UID `puso_daplink_lab`，VID `1234` / PID `5678`，COM8 |
| 注意 | COM8 是探针串口，不能当成新模块通信口；COM9 是当时枚举值，重插后要重新发现 |
| MCU / 工程 | STM32F407VGT6 / LQFP100，`vgt6_module_test` |
| UART | USART2，PCLK1=42 MHz，软件初始化 115200，实板 BRR=364 |
| UART 分频 | 现有 HAL 的 BRR=364 对应约 115384.62 bps，偏差 +0.160%；不能仅因理想四舍五入为365就判错 |
| DMA | USART2 RX：DMA1 Stream5；TX：Stream6；均 Channel4；未随迁移改变 |
| 其他服务 | VOFA TCP `127.0.0.1:1347`；网页 HTTP `127.0.0.1:8088` |
| 网页 | `mecanum-motor-console/index.html`，touch-remote JS v36 |

资料在 `D:/my_code/my_code/大创/8、LQ208BLE蓝牙模块_V2.0配套资料/`，主要依据 `1-LQ208BLE蓝牙模块使用教程/LQ208蓝牙模块使用手册_V2.0_261015.pdf`。手册说明固定115200、无需AT配置；资料中的模块升级、解除保护、清空Flash流程不属于本次任务，未执行。

常规启动：双击 `D:/my_code/my_code/大创/mecanum-motor-console/启动上位机.bat`。默认 serial/115200，自动选择唯一 CH340/CH910。显式指定端口：

```powershell
Set-Location 'D:/my_code/my_code/大创'
& './mecanum-motor-console/bridge/start-bridge.ps1' -Transport serial -Port COM9 -Baud 115200
```

旧 `--transport ble` 路径仍保留，但它针对原 WHEELTEC FFE0 模块；本次新模块使用 USB 串口，不走 Bleak 扫描、16字节分包和20ms分包等待。PID工具继续接 WS8766，不直接占COM9。不要同时打开串口助手、第二份桥接或另一个直接占口的工具。

### 编写本文时的运行态快照

21:34–21:36 左右只读检查：COM8、COM9仍存在，HTTP8088由PID47980监听。桥接监听进程在采样期间从35864变为6480；最后观察到6480同时监听8766/1347，命令行为本项目 `motor_vofa_bridge.py --transport serial --baud 115200`，父进程45716。**本文编写期间没有主动重启桥接；该进程变化本身不能证明物理无线断联。**

Python venv 启动器及其运行时子进程可能同时出现在进程列表中，不等于有两个桥接；以端口监听者、父子关系及命令行确认。PID不是长期配置，下一窗口必须重新枚举。本次仅整理文档，没有重新向板上查询版本、停车状态或电量；以下实板状态均注明为先前验证记录。

## 3. 版本、烧录验证与备份

当前已验证烧录的镜像为 **R24**：

```text
VER,VGT6_BM50_ATTITUDE_R24
CAPS,ATTITUDE_R24,20,10,160,60,60,80,SPEED_TUNABLE,HLOG,PS2,TRACKMOVE,HLOG_MODE,HLOG_US,TOF_DL1B,WLOG_US,TOF_GUARD,FRONT_REVERSED,TOF_FAST,TOF_GUARD_200_100_40,UART_QOS,SERIAL_115200
UART,115200,42000000,364,0x00,0,0,0,0,0
```

HEX：`D:/my_code/my_code/大创/vgt6_module_test/MDK-ARM/vgt6_module_test/vgt6_module_test.hex`

SHA256（编写本文时再次核对文件一致）：

```text
4D92496210545D3ECFC03F97004E4546A5E01070CAD36705CEB587B67B6D84A9
```

Keil在2026-10-22 20:00:01报告 Erase Done / Programming Done / Verify OK / Application running；随后独立读回设备 `0x101f6413`、1024KiB、向量表及 R24 标记 `0x08005ab8` 一致，复位后再次读回通过、RUNNING。只做正常应用区烧录，没有整片清空、解保护或修改option bytes。

构建为原Keil工程、ARMCC5.06u5 build528、STM32Cube FW_F4 V1.28.3、DFP3.1.1，0错误0警告；Code60442 / RO2662 / RW236 / ZI103836。`.ioc`已改115200，并在独立临时目录经CubeMX6.17.0完整生成核对。现有工程实际编译的是 **Core/Src/main.c 内的静态USART2初始化**；单改保留但未编译的usart.c不够。当前两处均已同步。

| Git提交 | 用途 |
| --- | --- |
| `c8405951`、`a2ebb66f` | R23控制优先、松键STOP和无效TOF帧压缩 |
| `e71be4f9` | R24固件、串口桥接、网页、精确身份白名单及构建前备份 |
| `c07bfd8c` | STOP取消串口心跳不误判掉线；主机测试夹具按实板BRR364修正，固件镜像不变 |
| `030941b8` | R24烧录、链路、停机日志与实测证据归档 |
| `0a7d4404` | 启动脚本 `/`、`\` 路径识别修复及验收 |

此前R23自动追加烧录曾失败、用户后来自行烧录；不要沿用旧失败日志判断当前R24未烧录，也不要拿首版R23的成功验证冒充最终R23镜像验证。当前R24有自己独立的完整验证链。

## 4. 已落实的通信修复

### R23继承到R24的修复

- 网页松键立即发送STOP，不再等正在进行的MOVE应答；MOVE失败补STOP。晚到MOVE ACK不能重新开启KEEP或恢复运动所有权。
- 桥接过期运动目标超过250ms拒绝执行，连续运动指令保留最新排队目标；STOP/DISARM取消旧请求，电机命令超时不自动重发。
- 慢网页发送与TCP drain有上界，慢观察者不能无限阻塞应答；原始状态先于匹配的JSON结果发布。
- MCU可选周期遥测让位于控制应答：收到完整命令后80ms内退让，TX已有帧时跳过过时显示，不积压实时画面。ACK、显式查询和日志下载仍走可靠FIFO。
- 有效TOF显示保留单流10Hz、与TEL同开5Hz；相同无效状态改1Hz保活，状态变化及时上报。本地测距持续约25Hz。
- `LINKQ`提供待发帧/字节、主动跳过显示帧数量和命令年龄。主动跳过显示帧不等于UART错误或RAM日志丢样。

### R24新串口路径

- 固件USART2、桥接、启动脚本统一115200/8N1；USB serial不使用旧BLE分包等待。
- 按串口现有可读字节读取，空闲等待20ms，替代旧 `read(256)` / 100ms 聚包等待；写入检查完整字节数。
- 不能以“COM口打开”当成远端在线：启动须完成STOP、DISARM、PING应答；空闲心跳检查远端。重连保持未解锁，不恢复旧运动。
- STOP/DISARM主动取消正在等待的心跳属于正常优先处理，不能因此误报无线掉线。`c07bfd8c`修复了这一分支。
- 保留有序RX发布队列，避免多个异步发布任务把END发到此前DATA前面；原始日志数据必须完整、有序。

### 用户刚遇到的启动报错已修复

报错：`Port 8766 is used by another program (PID 50616)`。50616实际就是本项目的桥接。其命令行使用 `D:/.../motor_vofa_bridge.py`，启动脚本只按 `D:\...\motor_vofa_bridge.py` 匹配，误判归属。

现在匹配前统一路径分隔符，仍保留完整路径边界和Python进程检查，不按端口号无条件杀进程。已通过7种归属/语法用例，包括正斜杠、混合斜杠、其他程序和相似后缀路径。实际执行Windows PowerShell启动脚本，成功识别并重启原进程；新进程实测PING、R24、UART115200零错误、DISARM与停止状态成功。无需为此重烧固件。

另外曾在迅速重启桥接时遇到WinError10048（旧8766套接字未完全释放）；待释放后正常启动。这是本地进程/套接字重启问题，应与物理无线断联分开记录，不要用关闭系统安全选项或任意端口复用掩盖。

## 5. 实板测量：事实与限制

以下均为电机未解锁的通信诊断。RTT是主机发送命令到收到匹配结果的往返时间，包含桥接、无线、MCU及回传，**没有完成各层时延拆分**。不同日期/模块/环境的试验不构成严格受控A/B。

| 链路及阶段 | 次数 | 中位数 | P95 | 最大值 |
| --- | ---: | ---: | ---: | ---: |
| R22旧模块，无串流 | 18 PING | 117ms | 172ms | 203ms |
| R22旧模块，TOF+TEL | 18 PING | 140.5ms | 266ms | 313ms |
| R23最终版旧模块，两分钟 | 286 PING | 109ms | 234ms | 328ms |
| R23最终版旧模块，两分钟 | 28 STOP | 94ms | 172ms | 172ms |
| R24新模块，无串流 | 18 PING | 156.5ms | 203ms | 219ms |
| R24新模块，仅TOF | 18 PING | 203ms | 375ms | 453ms |
| R24新模块，TOF+TEL | 18 PING | 219ms | 359ms | 359ms |
| R24新模块，两分钟 | 236 PING | 187ms | 344ms | 781ms |
| R24新模块，两分钟 | 23 STOP | 157ms | 281ms | 343ms |
| R24最终桥接短测 | 20 PING | 140ms | 未统计 | 297ms |
| R24最终桥接短测 | 20 STOP | 141ms | 未统计 | 375ms |

R24两分钟120.25秒中所有236 PING、23 STOP应答；未观察断联，UART/LINK错误计数不增长、TX队列抽样最大0帧、TOF样本计数约24.6733Hz。网页曾连入初始化，包含观察者流量。该两分钟数据来自心跳取消修复前的串口桥接；修复后的桥接另有20 STOP+20 PING短测，无离线事件。

重要限制：队列查询为离散抽样，不能排除两次查询间的短暂积压；TOF当时经常无效状态4，未覆盖有效距离持续高频上报时的全部负载；没有进行遥控行驶、长距离/遮挡、长时间掉线重连、供电纹波或逻辑分析仪同步验收。

### 日志完整性

- 旧WHEELTEC/R23曾在8条一批WLOG下载时出现截断、缺前缀和畸形行，严格校验拒绝；该数据没有评分或提参。
- 旧链路临时逐条读取60条停机记录成功，间隔10146..12050us。只说明当时逐条读取可完整取回，不证明根因或运动响应。
- 新LQ208/R24完整下载213条停机WLOG：8条一批、零重试、零缺号，字段、END、最终STATUS、微秒相位通过；间隔10146..12051us，下载约15.078秒。四轮目标/PWM、enabled/armed/angle_mode均0。这是传输和时间轴证据，不能用于PID阶跃评分。
- 同一诊断随后尝试HLOG，在角环mode0时得到0样本，组合脚本的非空断言失败。原失败报告保留；单独派生的WLOG报告重新严格验证213条通过。没有为获得HLOG而解锁或转车，本轮运动HLOG尚未实测。

## 6. 关键源码、资料和证据位置

以下路径均相对于 `D:/my_code/my_code/大创/`：

| 路径 | 查看重点 |
| --- | --- |
| `mecanum-motor-console/bridge/motor_vofa_bridge.py` | `execute_command`、`send_hardware`、`serial_loop`、`monitor_serial`、`consume_bytes`、`publish_queued_lines`、广播、drive_lease及锁竞争 |
| `mecanum-motor-console/bridge/start-bridge.ps1` | 端口归属识别、安全重启、serial115200默认值 |
| `mecanum-motor-console/bridge/test-start-bridge.ps1` | 真实启动片段的模拟归属回归，不会终止真实进程 |
| `mecanum-motor-console/touch-remote.js` | 松键STOP、MOVE epoch、KEEP、连接/查询与状态更新 |
| `mecanum-motor-console/tests/` | 网页与日志回归 |
| `vgt6_module_test/Bsp/Src/serial_dma.c` | TX FIFO、DMA所有权、命令接收/错误恢复、遥测退让 |
| `vgt6_module_test/App/Src/telemetry.c` | 遥测速率、无效TOF压缩、日志下载 |
| `vgt6_module_test/App/Src/app.c` | 控制周期和TOF/串口调度位置 |
| `vgt6_module_test/Core/Src/main.c`、`vgt6_module_test/vgt6_module_test.ioc` | 实际USART2初始化及配置来源 |
| `vgt6_module_test/tools/flash-dap.ps1`、`verify-dap.py` | 正常烧录与设备/向量/标记/运行态验证 |
| `vgt6_module_test/LINK_R23_HANDOFF.md`、`LINK_R24_HANDOFF.md` | 详细开发历史、验证边界 |
| `vgt6_module_test/validation/r24-build-flash.json` | 镜像哈希、实际Keil/独立读回日志、PID skill文件哈希 |
| `vgt6_module_test/validation/r24-boot.json` | 正式R24初次协议与参数回读 |
| `vgt6_module_test/validation/r24-latency.json` | 新模块三种负载的命令时间线与原始线数据 |
| `vgt6_module_test/validation/r24-soak-initial-bridge.json` | 两分钟持续收发与健康计数 |
| `vgt6_module_test/validation/r24-final-stop-check.json` | 心跳取消修复后的短时STOP/PING复核 |
| `vgt6_module_test/validation/r24-stopped-logs.json` | 原始组合诊断，含HLOG非空断言失败 |
| `vgt6_module_test/validation/r24-stationary-wlog.json` | 重新验证通过的213条停机WLOG |
| `vgt6_module_test/validation/r24-startup-path-fix.json` | 启动脚本实测回读 |
| `vgt6_module_test/validation/r24-test-results.json` | 测试输出，包括完整PID套件失败详情 |
| `vgt6_module_test/validation/r24-console.png` | R24/115200网页回读截图，历史截图不代表当前在线 |

`D:/Temp/r24-*` 还保留诊断脚本、生成日志及运行日志；这些是临时文件，关键原始数据已归档到上述validation，优先读取归档。不要假定临时脚本是正式接口或直接运行含采集/CLEAR的脚本覆盖现存日志。

## 7. 下一窗口建议的排查顺序

1. **先确认现场与身份。** 重新枚举COM、8766/1347监听者和进程命令行。已有正确桥接时复用WS，不另开串口。查询VER/CAPS/UART/LINK/LINKQ/STACK/ANGLESTATUS/PS2STATUS/SPEEDSTATUS，确认版本、停车与错误基线；不要因文档或打开COM就认定实机状态。
2. **先复查现有实现，再采新数据。** 本仓库有`.codegraph/`，理解/定位代码先用CodeGraph；若它未覆盖PowerShell等文件，再针对性读取。重点检查串口read/write线程、锁等待、有序RX队列、慢观察者、周期心跳和drive_lease/KEEP之间的竞争。1Hz诊断PING不是电机看门狗续命。
3. **做停机对照。** 固定同一链路和环境，分别关闭TEL/TOF、只开TOF、两者同开；控制网页/VOFA/调参客户端数量。记录每条命令发送/返回、timeout、STATUS事件、UART/LINK增量和LINKQ。测量脚本本身不能自动重发运动命令；启动验证完成后再发普通查询。
4. **拆分RTT。** 目前尚无统一的主机入队/写完成/首字节/整行/WS结果/页面更新时间测点。若加埋点，应使用单调时钟，分别归因；主机和MCU时钟未同步时不能直接相减当单程时延。逻辑分析仪或合适的有线对照可区分UART、模块缓存和无线等待，但需先确认现有硬件条件，不盲改连线。
5. **检查传输负载与丢包。** 用停机RAM日志、序号和严格END检查批量完整性，记录请求大小、字节数与下载耗时。保留畸形行和失败原报文；不能插值、拼补、丢坏行后评分。与有效TOF持续串流的负载对照尚未完成。
6. **若复现断联，分清层次。** USB COM消失、COM仍开但远端无应答、桥接进程退出、WS关闭、网页旧缓存、无线模块未连接是不同事件。同步记录模块供电/灯态、USB驱动和进程日志；不要只靠“网页离线”判定蓝牙物理断联。
7. **有证据后做最小修改。** 先跑对应回归、记录新固件/桥接版本与哈希；若需要改下位机，再遵守原生.ioc配置、Git备份、停车、探针识别与非破坏烧录验证。单修启动脚本无需重刷板子。

只读发现入口：

```powershell
Set-Location 'D:/my_code/my_code/大创'
Get-NetTCPConnection -LocalPort 8766,1347,8088 -State Listen -ErrorAction SilentlyContinue |
  Select-Object LocalPort,OwningProcess
& './mecanum-motor-console/bridge/.venv/Scripts/python.exe' -c 'from serial.tools.list_ports import comports; print([(p.device,p.description,p.hwid) for p in comports()])'
```

针对启动/桥接/网页的离线回归（不接电机、不启动live调参）：

```powershell
& './mecanum-motor-console/bridge/test-start-bridge.ps1'
& './mecanum-motor-console/bridge/.venv/Scripts/python.exe' -m unittest discover -s './mecanum-motor-console/bridge' -p 'test_motor_vofa_bridge.py'
# 用本机已有Node运行 mecanum-motor-console/tests 下的 *.test.cjs。
```

## 8. PID、TOF、权限与工作区保护

- PID skill已更新：`C:/Users/28097/.codex/skills/pid-tuning/SKILL.md`、`references/vgt6-attitude.md`、`references/vgt6-wheel.md`。R24精确VER/CAPS白名单已加入角环和轮速适配器；本轮没有改PID、评分门槛、50Hz HLOG或100Hz WLOG语义。
- 先前实板回读：HEADPID=`6.0000/1.2500`，YAWPID=`0.1815/1.8062`；轮速A=`1.2/0.0991/0`、B/C=`1.2/0.0998/0`、D=`1.2/0.1008/0`。这些是历史回读和默认值，不代表下一窗口当前RAM参数；必须重新查询，不能盲写恢复。
- TOF端已定义为车头，新轮位C右前/D左前/A左后/B右后，物理通道与电机/编码器极性未改。
- 距离≤100mm禁止朝TOF前进；100..200mm前进上限由0线性增至40RPM；>200mm不额外限速。倒退/横移保留。用户明确选择测距无效/超时只提示、保持请求速度；不能借蓝牙排查悄悄改变该策略。状态4问题独立存在，尚未完成近距离停车实测。
- 当前相关离线结果：固件协议2383断言、网页77/77、桥接30/30、启动归属7用例通过；角环兼容6/6、轮速身份3/3通过。**完整PID适配器套件71项中6项未通过（4errors/2failures）**，涉及完整ARM ACK、正向holdout顺序、路程裕量和日志重读的实现/旧夹具不一致。与另一窗口的未提交地面试验修改相关，需要单独复核，不能宣称全套通过或归因于新模块。
- `pid-tuning-adapter/wheel_tuner.py`、`tuning_campaign.py`、README，以及个人Keil IDE状态、C51项目和大量试验目录仍有其他窗口修改。先看git status/diff，不reset、不清理历史试验、不整仓add、不覆盖或盲目恢复。相关文件是否又变化以实时工作区为准。
- 当前请求是排查蓝牙；本交接不授权ARM、非零运动、live调参、修改增益、恢复中断实验、整片擦除、解保护或重刷模块自身固件。

## 9. 可复制给下一窗口的任务说明

> 请先阅读 `D:/my_code/my_code/大创/BLUETOOTH_DEBUG_HANDOFF_20261022.md`，继续检查蓝牙遥控延迟、卡顿和断联。现在使用已配对的LQ208BLE V2.0模块对，电脑USB串口上次为COM9，固定115200/8N1，下位机已验证烧录R24。先重新确认现场版本、进程、端口和停车状态，复用现有桥接；基于原始证据拆分网页、桥接、UART与无线时延，不要把波特率提高等同于问题解决。先做停机诊断，本次不要自动解锁、运动或改PID；保留其他窗口修改和全部失败数据。
