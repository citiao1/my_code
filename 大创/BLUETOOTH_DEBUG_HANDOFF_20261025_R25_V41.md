# 蓝牙／串口断联交接：R25 固件 + v41 上位机

更新时间：2026-10-25，北京时间。给下一窗口优先阅读。

**后续更新：当前已为R26/v42、COM12，网页已确认重新连接。**
本文件正文是R25/v41历史快照；最新情况先读 `R26_LINK_REPAIR_20261025.md` 和
`R26_COM12_RECONNECT_RECORD.md`。一次完全无回传在整车／主控同时断电后恢复，
尚不能归因于独立蓝牙模块复位。

## 1. 接手后的第一件事

**先确认用户实际加载了 v41，再收集一次新的故障日志。**

代码和静态HTTP资源已更新，但本窗口最后看到的浏览器页面仍是v40。
随后浏览器自动化连接中断，没有确认替用户完成刷新。
已让用户停车后Ctrl+F5，确认按钮显示“导出链路诊断 v41”，**尚未收到完成确认**。
新导出的JSON应有 `clientVersion: "safe-link-v41"`。
不要把v40历史事件或旧文件反复当作v41的新故障。

核心未完成事项：**运动时的底层字节损坏／确认丢失仍未定位，不能宣布断联已解决。**
v41修复的是网页把短时故障放大为关端口，以及停车状态验证和重连释放竞态。

## 2. 用户的目标与最新说明

- 解决蓝牙串口越用越延迟、运动后断联、按住S后退中断等问题。
- 默认完全使用Web Serial直连；PID参数和曲线直接读串口，不依赖Python/WebSocket桥接。
- 四轮速度目标／实际曲线；可临时绕过角速度环、方向环，专注轮速。
- 暂停占内存最多的高频日志，排除其影响；每次异常能保留原因和前后证据。
- 最新复现：“动一下就断了”。用户说明是**下地测试、驱动集成在电机上**，
  并认为不是电压问题，强调最开始蓝牙很流畅。不要未经测量就反复断言供电问题。
- 用户说两端蓝牙都升级过固件。具体固件二进制／编译版本未独立核实。

此前用户询问过FreeRTOS和多ToF共用I2C；目前仍是裸机工程，**没有实际移植FreeRTOS**。
不要把换RTOS当作本次损坏数据的既定解法。

## 3. 仓库、版本与授权边界

- 工作目录：`D:/my_code/my_code/大创`
- Git根目录：`D:/my_code/my_code`
- 工程：`D:/my_code/my_code/大创/vgt6_module_test`
- 上位机：`D:/my_code/my_code/大创/mecanum-motor-console`
- `.codegraph/`存在；用户要求**理解／定位代码先用CodeGraph**，没有覆盖的具体区域再读源码。
- 根仓库有其他项目、PID调参、IDE等未提交改动；**不能git reset、clean或add全部文件**。

版本提交：

| 提交 | 内容 |
|---|---|
| `e71be4f9` | R24，115200串口版基础备份 |
| `6315ce85` | R25初始功能：四轮曲线、速度专用模式、关闭日志、故障历史 |
| `537ae9ae` | R25修正保活与遥测静默窗口的周期碰撞 |
| `7a38d1f9` | R25烧录工具和停车验证脚本 |
| `6abaf0ce` | R25烧录／停车验证结果、v40补充 |
| **`f9234e0c`** | **最新代码修复：v41，DKEEP/STOP安全停车验证、现场证据** |

本轮结束检查时，App/Bsp及本轮上位机修复已提交。上位机目录还有先前遗留改动：
`bridge/motor_vofa_bridge.py`、`bridge/test_motor_vofa_bridge.py`已修改；
`bridge/diagnose_serial.py`未跟踪。不要覆盖／误当成下一窗口刚产生的修改。

若涉及固件，先读已安装技能：
`C:/Users/28097/.codex/skills/nuedc-stm32-mspm0-skill/SKILL.md`。
其中记录了本工程“完成修改、验证构建、单独Git备份、检测匹配目标后自动烧录”的既有偏好，
但不授权ARM、运动实验、PID试跑、关闭保护、解锁或大擦除。以用户当前指令和现场为准。
**本窗口没有自动ARM或驱动电机；不要把用户自行测试当作后续自动运动的新授权。**

## 4. 硬件与构建配置

- STM32F407VGTx，168MHz，原生CubeMX `.ioc` → Keil `.uvprojx`。
- Keil：`D:/Keil5/UV4/UV4.exe`，ARMCC5.06 update5 build528。
- 工程：`vgt6_module_test/MDK-ARM/vgt6_module_test.uvprojx`，target=`vgt6_module_test`。
- USART2 PA2/PA3，115200、8N1、无流控，PCLK1=42MHz，BRR=364。
- RX DMA1 Stream5、TX Stream6；RX DMA64B、环形区256B、命令行64B；TX队列16×256B。
- 蓝牙为LQ208BLE V2。最近PC端为**COM11，VID1A86/PID7523**，端口号以后须重新枚举。
  老文档COM9不要照抄；COM4/5等旧蓝牙虚拟端口不是当前设备。
- PUSO CMSIS-DAP：UID=`puso_daplink_lab`，VID1234/PID5678，虚拟串口COM8。
- pyOCD环境：`mecanum-motor-console/bridge/.venv/Scripts/python.exe`，已装pyOCD0.45.1。
- DFP：`D:/Keil5/ARM/PACK/Keil/STM32F4xx_DFP/3.1.1`。

默认 `启动上位机.bat` → `bridge/start-serial-console.ps1`，静态HTTP8088 + Edge，
页面地址 `http://127.0.0.1:8088/?transport=serial`。它不运行串口桥接。
兼容BAT才使用Python桥接，8766/WebSocket及1347/VOFA。两种方式不能同时占用COM。
终端显示“no serial bridge”是启动说明，不是错误。

## 5. 当前下位机 R25：已烧录，不是待烧录

HEX：`vgt6_module_test/MDK-ARM/vgt6_module_test/vgt6_module_test.hex`

SHA256：`02ABF3E6466D8ED1551B65E7F0A37A0CB9D13F02F4012D0715766514CB68D7D2`

Keil构建0错误0警告；原生Erase/Programming/Verify OK/Application running。
独立读回ID=`0x101f6413`、Flash1024KiB、向量表和R25标记`0x08005934`一致，复位后RUNNING。
串口回读 `VER,VGT6_BM50_ATTITUDE_R25`，CAPS含 `DKEEP,LOG_DISABLED`。
本次v41只改上位机，**不需要为了使用v41再刷一次固件**。

### 已完成的功能

- 四轮A/B/C/D的目标／实际RPM曲线，单轮或四轮同屏。
- 速度专用模式先STOP、查询停车，再由用户手动ARM、启用速度环。
  W/S/A/D使用DRV绕过角速度／方向控制；Q/E和外环启动、自动底盘测试禁用。
  退出也停车，不恢复旧动作。A..D仍是物理电机通道，不要为了正负号随意改极性。
- 当前车头为ToF端：右前C、左前D、左后A、右后B。
- `App/Inc/app_config.h` 中 `APP_ENABLE_TUNING_LOG=0`：HLOG/WLOG共享88KiB缓冲及采集下载代码
  被编译移除；实时低频曲线和PID读写保留。设1重编译可恢复日志，但本轮不要无故改回。
- 静态RW+ZI从104072B降到13464B（13.15KiB）。ROM61360B。
  栈4096B，静态已知深度1928B；最新现场水位已用2576B、剩余1520B，保护正常。
- PID、控制周期、TOF阈值没有因本轮排障改变。

### 保活、遥测与保护

- 速度控制10ms，角环20ms。普通MOVE使用角速度控制，KEEP约400ms，角环看门狗1500ms。
- 裸DRV有300ms看门狗；网页DKEEP约120ms（40ms定时检查），ACK等待200ms。
  DKEEP不能恢复已过期的旧目标，不能在PS2／角环占用时生效。
- 网页松键、失焦会请求STOP；切后台还可能影响浏览器定时器。应保持前台测试，
  用`uiTrace`区分这些正常保护触发和真正的通信故障。
- TEL单独名义3.3Hz，与TOF同开约2Hz；保活窗口下实测约2.8Hz／2Hz。
  **不是100Hz速度环原始采样，不适合据此声称精确评价PID瞬态。**
- 保留80ms命令优先静默窗口。R25先检查TX是否可发，状态成功发出才更新时间戳，
  避免120ms保活与300ms状态周期锁相导致曲线一直空白；不补发旧样本。
- TOF：DL1B/VL53L1X，I2C1 PB8/9，400kHz，33ms预算、40ms测量间隔，非阻塞IRQ状态机。
- 前进保护：200→100mm线性减速，上限40RPM，≤100mm停止正向前进；倒退／横移可用。
  现有测距失效策略未改；不要为试连通性关闭保护或拉长看门狗。

## 6. 已抓到的故障及证据

优先读：`R25_MOTION_DISCONNECT_V41_20261025.md`，含逐次时间和直接原因表。

### 用户提交的两次起步故障

原文件：`C:/Users/28097/Desktop/bm50-link-1792940005702.json`

已归档到 `vgt6_module_test/validation/bm50-link-1792940005702.json`。
SHA256=`01785C215A3F52CFD5DB97B48A798064CF71D0C3F81D09D63003F50D37D2DCE4`。

1. DRV正常ACK、前两次DKEEP正常ACK；接着出现**60个非法字节**和裸`DKEEP`（缺ACK前缀）。
   v40约200ms后主动关端口。
2. 再次连接后DRV、前两次DKEEP正常；第三次等待期间没有新字节，v40约200ms后关端口。
   关闭时最后接收距当时306ms，是本次DKEEP发送前收到的。

两次均没有USB拔出事件。R25日志缓冲已关闭仍复现，不能继续将它视为必要根因。
“无USB事件”不证明无线链路没有停顿；“网页主动关闭”也不等于底层丢字节已解释。

### 页面后续历史

本窗口通过导出按钮得到 `C:/Users/28097/Desktop/bm50-link-1792941040465.json`，
已归档到validation同名文件。包含28条事件：7次超时后关端口（3 DKEEP、4 STOP），
另1次旧端口未释放导致`InvalidStateError: port is already open`。
其中SPEEDLIMIT正常应答被v40误标的事件不能计作真实损坏。

最后一次STOP案例：STOP未见ACK，但之后TEL持续报告速度关闭、目标0。
网页还拿旧ACK,MOVE记录的mode1判断恢复资格。轮速显示选择TEL后不再持续回ATEL，
所以旧版本缺少“新ATEL停车证明”，关掉了仍能收数据的端口。
**TEL27字段没有角环mode，不能为了恢复而伪造mode0。**

### 现场只读检查与停车串口复测

- `validation/r25-motion-fault-swd-20261025.json`：只读SWD，无halt/reset/write。
  核心RUNNING、tick递增、CFSR/HFSR=0；目标/PWM0，UART计数无错误，TX槽内是完整ASCII。
  多地址读数不是原子快照，不能据同一行中不同时间的队列值误判竞态。
- `validation/r25-motion-fault-serial-20261025.json`：打开COM后先被动监听2秒即收6条正常TEL；
  STOP/DISARM/PING等16～47ms应答。没有对MCU或模块断电重启。
  但打开PC串口可能重初始化PC侧状态，不能据此精确断言是哪一端恢复。
- MCU端UART错误0，只能说明其接收／DMA计数，没有证明“整个链路零丢包”。
- 早期10月22日另有“PC端拔插不恢复、仅车端模块重启才恢复”的证据。
  这与本次重新打开COM就可收数据的表现不同，不要预设所有故障只有一个根因。

## 7. 最新上位机 v41 的修复逻辑

代码：`serial-transport.js`、`touch-remote.js`、`link-journal.js`。
HTML资源版本：transport v5、journal v2、touch-remote v41。

1. DKEEP未确认仍立即取消运动；不延长300ms车端看门狗，不自动重发DRV或DKEEP。
2. 已验证会话且写入完成时，DKEEP／STOP缺ACK保留端口进入安全验证，不要求旧ATEL先证明停车。
3. DISARM首次等1秒；未确认只允许再发一次DISARM、等3秒。
   再用ANGLESTATUS读取新mode0/速度关闭，并要求PONG。全部成功才恢复在线；失败关闭。
4. 取消旧按键、目标和队列；不自动ARM、启用速度环或恢复运动。
5. 新增`rxDuringWait`、预期应答、最近ACK间隔、独立TEL轮速快照、车端ERR原因。
   旧提示“最近500ms收过字节=仍有回传”已改，SPEEDLIMIT正常帧不再误报。
6. 重连先等旧端口关闭（最长5秒），禁止重复点击竞态。
7. DRV不再附带重复TEL,1。用户启用速度环后明确关闭周期遥测，可保持关闭进行低负载对照。
   普通MOVE会开启角环遥测，因此对照应使用**速度环专用模式**。

故障历史使用localStorage `bm50-link-incidents-v1`，最多40条，最近3条附原始证据。
健康RX不持久写盘；同秒同类损坏合并计数，防止故障记录自身写盘过密。
刷新保留历史，**不要为排障先清空浏览器存储**。

## 8. 验证完成到哪一层

- 最新 `node --test mecanum-motor-console/tests/*.test.cjs`：**122/122通过**。
  结果在 `vgt6_module_test/validation/v41-web-tests.txt`。
- 含真实60坏字节／裸DKEEP回放、迟到ACK、静默、仅TOF、旧角环状态、
  DISARM丢失、未停车、写阻塞、关闭／重连竞态、遥测关闭保留等测试。
- HTTP实际验证已提供v41和新transport资源。
- **没有确认用户页面已刷新，没有v41下用户运动复现结果。**
- R25默认关闭日志协议测试2670项、启用日志配置2744项通过。
  Gyro、Timebase、ToF和I2C测试通过。
- 完整MCU主机测试脚本仍有已有PS2仿真失败：
  `test_release_backtrack_sweep`中`reverse_arc < 0.5f`。
  换回Git R24速度源码也在同处失败。不能报告完整主机测试全部通过，
  也不要为消除这个断言擅改PID。
- R25停车实机12秒测试：101次停止态DKEEP按预期拒绝，最大78ms；
  TEL单独6秒17帧，TEL+TOF6秒11帧；错误字节0。**停车正常不等于运动正常。**

## 9. 下一窗口建议步骤

1. 确认v41实际启用。保留原文件，收集**新的**v41导出，检查保护恢复是否成功而非关端口。
2. 用户手动测试时，可在速度专用模式、已启用速度环之后关闭周期遥测做对照。
   此时曲线不回传，但DKEEP仍在；其余速度、方向、位置和设备条件保持一致。
   不要同时换波特率、PID、固件、保护阈值等多个变量。
3. 区分：收到坏字节、仅ACK丢失、完全无RX、明确ERR拒绝、真实USB设备事件、
   DISARM证明失败、打开旧端口未释放。不要把“软件关端口”当作硬件根因。
4. 若关闭回传仍损坏，需要实际有线串口／模块与线路对照，或在模块UART边界捕获波形，
   才能把MCU发送、车端模块接收、无线透传、USB输出分开。此处尚无实测结论。
5. 如需固件更改，先按原生工程构建、备份，再检测并确认停车／DISARM，遵循技能烧录校验。
   不要用放宽看门狗、把裸DKEEP当ACK、自动重放运动来掩盖故障。

本地厂家手册：
`8、LQ208BLE蓝牙模块_V2.0配套资料/1-LQ208BLE蓝牙模块使用教程/LQ208蓝牙模块使用手册_V2.0_261015.pdf`。
第1页固定115200/8N1、无硬件流控；第7页提示双向负载、环境和上层完整性处理。
不要尝试未支持的AT／角色／流控配置；文档的烧录操作不构成新的用户授权。

## 10. 直接可用的排障入口

以下命令以大创目录为工作目录。先检查端口占用，别抢占用户正在使用的串口。

```powershell
node --test mecanum-motor-console/tests/*.test.cjs

# 只读SWD，专用于当前R25镜像／map，绝不能套用到新版本地址
& mecanum-motor-console/bridge/.venv/Scripts/python.exe -u `
  vgt6_module_test/tools/capture-r25-link-state.py --output <新的JSON路径>

# 先被动监听，再STOP/DISARM和查询；不ARM、不运动
& mecanum-motor-console/bridge/.venv/Scripts/python.exe -u `
  mecanum-motor-console/bridge/capture_fault_serial.py --port COM11 --output <新的JSON路径>
```

这些脚本拒绝覆盖已有输出。只读SWD脚本按map定位RAM并先核对R25镜像关键地址；
更换镜像后必须重新适配。pyOCD附加使用attach、100kHz、deferred_transfers=False、
limit_packets=True、auto_unlock=False、resume_on_disconnect=False，不halt/reset。

烧录／读回工具：`vgt6_module_test/tools/flash-dap.ps1`、`verify-dap.py`。
当前日志名和标记检查仍是R25，新固件版本需要同步更新，不能拿旧日志宣称成功。

## 11. 其他参考文件

- `R25_MOTION_DISCONNECT_V41_20261025.md`：本轮详细证据及逐次故障表。
- `R25_SPEED_ONLY_AND_LINK_DIAGNOSTICS_20261025.md`：R25功能、构建、烧录与内存数据。
- `BLUETOOTH_DEBUG_HANDOFF_20261022.md`：更早背景，版本和COM号已旧。
- `SERIAL_RECURRENCE_20261025.md`：v39之前的STOP缺确认处理。
- `FREERTOS_FEASIBILITY_20261025.md`：仅评估，未移植，RAM数据需以当前R25为准。

下一窗口可直接使用：

> 请阅读本交接文件，先确认实际页面已加载v41，再继续排查运动时损坏字节和丢确认。
> 保留现有证据、PID和保护参数；区分已修复的软件主动关口与尚未定位的底层链路问题。
