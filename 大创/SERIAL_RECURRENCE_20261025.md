# 2026-10-25 再次断开：损坏回传、STOP未确认与串口占用

## 本次日志的结论

原件：用户桌面`bm50-link-1792935768557.json`，已原样归档到`vgt6_module_test/validation/`。
SHA256：`02392d2c33c0788bd7cf0f24fa378a3a447bbef7e459d449ad9f5633e564abc7`。

- 确实运行Web Serial直连、115200、客户端v38，KEEP例外计数0；不是兼容桥接记录。
- 连接握手及初次UART/LINK/STACK回读正常，MCU UART错误0，栈保护正常。
- 主机16107.1ms发送`MOVE,30,-30,0`后，16133.6/16133.7ms收到了9个非ASCII字节。
  原始两块是`fe`及`7ffcfe82ecfd33f02c30e1`。无法把它们修复或认作有效ACK。
- 随后通信曾恢复，17340.3ms发出的STOP在17359.7ms收到完整`ACK,STOP`。
- 18147.9ms用户松开S，网页发送STOP，write_done已记录；18169.9ms只收到裸行`STOP\n`。
  它既不是`ACK,STOP`，也不能证明STOP命令已被MCU执行。
- 从18351.4ms起继续收到完整ATEL，mode=0、速度环enabled=0；TOF也持续回传。
  21156.2ms网页仍因旧STOP无ACK关闭会话，距最近接收只有117.4ms，等待约3008.3ms。
- 本段有效TOF距离约3.3～3.5m，未观察到100mm近障碍条件，不能将这次故障归因于该阈值。

裸行STOP可能是确认报文缺前缀，也可能来自异常回显；现有记录无法区分。
MCU UART错误计数0不代表整条发送、无线、USB与浏览器接收链路没有损坏。
最初坏字节来自哪个具体环节，仍需同步信号或更深入的模块证据。

## “直连打不开”是另一个问题

现场CH340枚举为**COM11**，不是历史COM9；DAP探针仍为COM8。
初查8766/1347由本项目兼容桥接占用，桥接独占串口。
在兼容网页里断开WebSocket或改下拉框，不会退出后台Python进程及释放COM。
实际网页错误是`SerialPort.open: Failed to open serial port`。

首次经现有桥接只读查询，PING/VER/UART/LINK/LINKQ/ANGLESTATUS全部成功，约16～31ms：
R24，115200，UART错误0，角环mode=0、速度环关闭。
后续两次尝试通过该桥接确认STOP/DISARM时，分别遇到WebSocket握手超时与连接被拒绝，
没有获得确认，不计作成功停车操作。随后兼容桥接已不再监听，才继续验证直连。

原版直连启动器在Windows PowerShell下约7.4秒完成并打开Edge，
`LQ208 direct Web Serial console - no serial bridge`只是说明文字，不是报错。
桥接退出后，真实Edge直接打开COM11成功，并完成STOP/DISARM和身份/参数回读。
没有重启MCU、蓝牙模块或改波特率来完成这一恢复。

## v39主机端修正

固件仍为R24，未改源码或烧录，未执行ARM、非零运动或PID写入。

1. **单次STOP确认缺失的有条件安全恢复。** 只有原写入完成、已验证会话、
   近期仍收到数据且有近期停止状态时，才清空旧队列、撤销运动所有权并锁定新请求。
   使用`DISARM → ANGLESTATUS → PING`验证，必须有完整DISARM ACK、合法停车ATEL及PONG。
   不伪造ACK,STOP；原STOP请求仍报告未确认。成功只恢复通信，保持DISARM，不恢复旧运动。
2. **失败仍关闭。** 无停止状态、无回传、DISARM无ACK、非法或运动中的状态回读，均不能恢复。
   原有KEEP单次例外、连续失败边界和车端保护保留。
3. **重置链接时清除旧输入。** 同时清除方向键、目标向量和运动状态，避免恢复后残留目标被重发。
4. **占用提示更具体。** 页面明确提示退出兼容BAT及其他串口页面后再直连。
5. **启动进度。** 控制台输出四步进度，一次枚举监听端口以减少Windows管理接口等待；
   继续只关闭准确识别为本项目的桥接，不关闭无关程序。HTTP服务仍只提供静态文件。

网页缓存版本：`touch-remote.js?v=39`，`serial-transport.js?v=3`。

## 验证结果与边界

- 网页101/101项测试通过，包括丢STOP ACK后的安全握手、禁止旧/新运动、
  非法/运动状态、DISARM未确认、TOF/裸STOP不能单独作为停车依据。
- 直连启动器五种模拟归属/复用场景通过；兼容启动器既有归属测试通过。
  测试边界经过mock，不会实际终止系统进程。
- v39实际Edge连接成功，读回R24、四轮PID、UART、LINK、STACK、PS2及停车状态，8766/1347无监听。
- 用户随后提供刚导出的`bm50-link-1792936972901.json`：
  `clientVersion=safe-link-v39`，`verified=true`，保留约194.17秒；
  118条命令/118条接收行，98次PING匹配确认，中位21.85ms、最大36.4ms；
  badBytes=0、timeouts=0、stopRecoveries=0。只有STOP/DISARM和只读查询/空闲PING，没有ARM或运动。
  因此证明的是当前停机直连和空闲应答可用，**没有实机复现损坏回传并验收自动恢复**，
  也不能证明行驶中的断联已解决。
- 浏览器界面自动化在处理导出保存状态时因无法可靠确认当前URL而停止；
  随后使用用户提供的实际文件完成核验，没有用界面操作失败冒充验收成功。

当前网页保留直连供用户继续测试，不启动兼容桥接；勿同时开启两种方式占用串口。

## 证据

`vgt6_module_test/validation/`：

- `bm50-link-1792935768557.json`：原始故障。
- `serial-recurrence-analysis-20261025.json`：坏字节、STOP及超时时间计算。
- `serial-recurrence-live-20261025.json`：经原有桥接的只读回读。
- `serial-direct-startup-repro-20261025.txt`：原启动器实测输出。
- `serial-recurrence-web-tests-20261025.txt`：101项网页回归。
- `bm50-link-1792936972901.json`：用户提供的v39真实导出。
- `serial-direct-v39-verification-20261025.json`：该导出的统计与命令范围核验。
