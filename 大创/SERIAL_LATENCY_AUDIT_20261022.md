# 串口卡顿排查及蓝牙固件升级前后对比

> 后续用户行驶时仍复现断联。以下停机改善数据不证明断联已解决。
> 已抓到回传方向失效，并实现网页串口直连；最新证据见 `SERIAL_DISCONNECT_INVESTIGATION_20261022.md`。

日期：2026-10-22，Asia/Shanghai。交接入口：`BLUETOOTH_DEBUG_HANDOFF_20261022.md`。

## 实测结论

**用户升级蓝牙模块固件后，当前链路的应答延迟显著改善。**
升级前后使用 COM9 / CH340、115200 / 8N1，无流控，直接用 pyserial 收发，
不经过网页、WebSocket、VOFA 或桥接进程。MCU 应答仍为 `VGT6_BM50_ATTITUDE_R24`。
没有修改 MCU 固件、波特率、控制周期或 PID 参数。

同一诊断流程，每组 24 次 PING，每次收到匹配 PONG 后等待 120ms：

| 显示回传负载 | 升级前中位 / P95 / 最大值 | 升级后中位 / P95 / 最大值 |
|---|---|---|
| 全关闭 | 172 / 329 / 344 ms | 16 / 47 / 47 ms |
| 仅 TOF | 219 / 485 / 562 ms | 31 / 63 / 78 ms |
| 仅 TEL | 406.5 / 859 / 1078 ms | 16 / 47 / 47 ms |
| TEL＋TOF | 633 / 2047 / 3515 ms | 16 / 47 / 110 ms |

升级后四组 96 次 PING 全部应答，无命令超时或清理失败。
旧链路的 3515ms 应答超过桥接普通命令的 3 秒超时，存在触发心跳失效、
关闭串口并重连的条件；这不等同于 USB 消失或无线模块实际断开。

升级前还做过交替复测：both/off/both/off，各 16 次，中位数分别为
352/250/336/234.5ms。它支持“显示回传放大延迟”，也显示无线环境存在波动。
升级前两种上位机入口的无回传测量中，串口直连中位数 242ms，桥接 172ms；
没有证据证明正常桥接本身是主要延迟来源。

## 升级后持续与实际桥接验证

- 直接串口，TEL＋TOF 同开，连续 **117.25 秒**。每次 PING 应答后等待 500ms，
  每 20 次额外查询一次 STOP。220/220 PING、11/11 周期 STOP 应答成功。
  PING 中位 31ms / P95 62ms / 最大 141ms；周期 STOP 中位 32ms / 最大 141ms。
  期间收到 209 条 TEL、112 条 TOF，说明显示流确实持续回传。
- 实际 `motor_vofa_bridge.py` 进程＋WebSocket，TEL＋TOF 同开，查询阶段 22.078 秒。
  40/40 PING 应答，中位 31ms / P95 93ms / 最大 110ms；
  8 次周期 STOP 加连接/退出的 2 次 STOP 全部应答，中位 31ms / 最大 47ms。
  测试服务使用临时端口 18766/11347，结束后已退出。
- 两种路径都没有记录远端不可用、UART 错误、RX 溢出或 TX 丢帧。
  LINKQ 离散抽样的待发帧/字节为 0。最后收到 STOP、DISARM、TEL,0、TOF,0 的匹配 ACK，
  TEL 的解锁/速度使能/四轮目标/PWM 为 0，ATEL mode=0。

这些是停机、当前距离及无线环境的测量，没有测试行驶电磁干扰、遮挡、长距离、
长时间掉电重连或模块供电纹波。TOF 此时持续 valid=0/status=4，固件将相同无效读数
压缩为约 1Hz；**不能将本次结果称为有效 TOF 5/10Hz 满负载验收**。
快速 PING 还会触发固件 80ms 遥测退让，因此增加 500ms 查询间隔的持续测试。
主机 monotonic 时钟在本次环境有约 15.6ms 量化，16/31/47ms 是测得的离散值，
不是微秒级单程测量。升级包含重启，其他环境变量未完全锁定；未通过 AT 或厂家
工具读取模块固件版本，升级版本以用户提供的信息为准。

## 项目数据路径与本轮软件修改

电脑端的“串口”仍接 LQ208 无线透传模块，**不是有线 USB 直连 MCU**。

```text
网页/VOFA → 本地桥接 → COM9 → LQ208 无线 → USART2 → STM32F407
PID 串口入口 → COM9 → LQ208 无线 → USART2 → STM32F407
```

1. 网页广播原先处于 MCU 应答的串行发布路径中，慢网页会增加等待。
   现在每个真实网页连接使用独立、有界的 FIFO，保留原始 DATA、END、JSON result 顺序；
   发送超时或队列溢出关闭该连接。VOFA drain 改为并行处理，避免逐个累加等待。
2. 串口桥接连接验证增加 TEL,0/TOF,0，防止旧会话留下显示流。
   收到近期匹配应答时，空闲监测不再额外发送重复 PING。
3. `attitude_tuner.py` 和 `wheel_tuner.py` 增加 `--serial-port`。
   不启动 HTTP、WebSocket 或 VOFA 服务；内部复用同一命令确认、STOP 优先、
   过期目标拒绝和 drive lease 实现，避免另写一套运动安全逻辑。
   串口被其他进程占用时失败，不结束别人的进程。不自动重连或重放运动请求。
4. 诊断脚本 `mecanum-motor-console/bridge/diagnose_serial.py` 保留原始收包块、
   命令时间线、匹配应答和错误；支持负载顺序、查询间隔和周期 STOP。
   不 ARM、不运动、不写 PID，不 CLEAR/START RAM 日志。

普通网页当前仍使用桥接。PID 可以独占串口，不再需要独立桥接进程；
不能同时让 PID、串口助手和桥接打开 COM9。详细用法见 `pid-tuning-adapter/README.md`。

```powershell
# 均为停机预检，不加 --live；每次使用新的输出目录。
.\pid-tuning-adapter\.venv\Scripts\python.exe pid-tuning-adapter/attitude_tuner.py --loop angular_velocity --serial-port COM9 --output pid-tuning-adapter/runs/NEW-SERIAL-ANGLE
.\pid-tuning-adapter\.venv\Scripts\python.exe pid-tuning-adapter/wheel_tuner.py --wheel A --serial-port COM9 --output pid-tuning-adapter/runs/NEW-SERIAL-WHEEL
```

两个串口入口已经各自通过实板停机预检。PID 评分继续使用 MCU RAM 中的
HLOG 50Hz / WLOG 100Hz，停车后下载并检查序号、END、微秒时间轴与完整性；
不能把低频 TEL、主机数据到达间隔或蓝牙传输速率当作 PID 采样周期。

## 其他高频事项的审查

| 项目 | 源码事实及判断 |
|---|---|
| USART2 | RX/TX 使用 DMA；串口路径不走旧 BLE 的 16 字节分包与 20ms 等待。115200 的 UART 容量不是本次测得低吞吐的合理解释，不能由此推算无线实际容量。 |
| 遥测 | TEL 默认约 3.33Hz；ATEL 5Hz；与 TOF 同开时状态流约 2Hz。有效 TOF 单开10Hz、同开5Hz；相同无效状态约1Hz。命令后80ms退让，已有 TX 帧时跳过过时显示。 |
| 高速 PID 日志 | 50/100Hz 在 RAM 记录，不是连续以该频率输出串口；不应因蓝牙问题降低控制或日志采样率。批量下载仍需严格校验，不能丢坏行后评分。 |
| KEEP / drive lease | 网页运动期间 KEEP 约400ms；轮速 lease 每100ms刷新 DRV，以满足300ms板载看门狗。它会产生 ACK 流量，但不能不加验证地降频或延长看门狗。此次未运动，未做实车刷新验收。 |
| TOF 本地读取 | 约25Hz，非阻塞总线状态机；本地频率和显示回传频率是两件事。未调整本地保护策略。 |
| PS2 / OLED / IMU | PS2每20ms轮询，存在约毫秒级软件时序；OLED每250ms整帧刷新；IMU常规SPI调用有10ms超时，重连初始化包含延时。这些会影响主循环最坏时延，尚无证据解释此次数秒长尾。 |
| 队列及故障恢复 | 固件 RX ring=256字节、TX可用15帧，每帧小于256字节；主机应避免洪泛查询。TX DMA硬故障后软件 tx_busy 的恢复路径值得故障注入复核；本次DMA/UART错误均为零，不能把它列为已发生根因。 |
| 断联分层 | COM消失、远端无应答、桥接退出、WS关闭、网页状态过期、无线物理断开需分别判断。本轮未复现物理断联，也没有证明所有场景永不掉线。 |

## 验证与证据

本轮桥接回归32/32、网页77/77、新增串口适配5/5通过，R19–R24角环身份兼容6/6通过。
完整 PID 套件76项仍有4 errors＋2 failures，与交接列出的6项相同，涉及原有轮速试验
ACK、holdout顺序、距离裕量和日志重读夹具；没有据此宣称 live 调参全套通过。
其他窗口的 `wheel_tuner.py`、README、tuning_campaign、Keil/C51改动均保留。

证据位于 `vgt6_module_test/validation/`：

- `serial-audit-raw-20261022.json`：升级前四负载原始数据。
- `serial-audit-summary-corrected-20261022.json`：原首测偶数样本的中位数按 statistics.median 重算；原始文件不改写。
- `serial-audit-interleaved-20261022.json`：升级前交替复测。
- `serial-audit-endpoints-20261022.json`：升级前直连与桥接两路径。
- `serial-audit-after-ble-upgrade-20261022.json`：升级后同流程四负载。
- `serial-audit-after-ble-upgrade-soak-20261022.json`：升级后117秒持续测试。
- `serial-audit-after-ble-upgrade-ws-20261022.json`：升级后实际桥接路径。
- `serial-audit-after-ble-upgrade-ws-runtime-20261022.txt`：对应桥接进程原始日志。
- `serial-audit-angle-preflight-20261022.txt`、`serial-audit-wheel-preflight-20261022.txt`：直连预检。
- `serial-audit-pid-tests-20261022.txt`、`serial-audit-web-tests-20261022.txt`：回归明细。

MCU HEX SHA256 保持 `4D92496210545D3ECFC03F97004E4546A5E01070CAD36705CEB587B67B6D84A9`。
本轮软件修改无需重烧 MCU。没有执行 ARM、非零运动、PID 写参或模块升级操作。
