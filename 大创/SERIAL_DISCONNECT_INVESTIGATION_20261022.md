# 回传方向停止与网页串口直连

日期：2026-10-22，Asia/Shanghai。本记录更新此前的停机延迟报告。

新增：用户随后提供两份直连导出，两次均在约3.1秒接收空窗后恢复回传，随后因旧STOP缺ACK被网页主动关闭。
这与下文“必须重启车端模块”的持续零接收现场不同，详见[SERIAL_CAPTURE_ANALYSIS_20261022.md](SERIAL_CAPTURE_ANALYSIS_20261022.md)。
本次原始导出已归档，证实真实浏览器下载功能可用；下文下载未完成描述保留为此前测试的历史记录。

## 结论及证据边界

用户升级两端蓝牙固件后，仍报告“刚连接延迟小，随后变慢、速度环关闭，最后无法连接”。
两端指示灯均常绿。**低延迟短测通过，不能证明断联已经解决。**

这次保留了实际故障，定位到车端→电脑的回传方向。MCU、接收路径及 TX 调度没有表现为卡死。
仅重启车端蓝牙模块后恢复，MCU 未重启。这强烈指向车端模块或其 UART/供电接口，
尚不能仅凭这些记录区分模块内部固件故障、模块 RX 引脚/连接电气问题或供电干扰。
没有厂家内部固件状态或逻辑分析仪/示波器证据，不把“模块固件死锁”写成确定根因。

## 故障现场的逐层检查

1. 未发现8766/1347桥接监听或桥接进程；COM9仍枚举为CH340。
   直接用pyserial发送STOP/DISARM/PING，电脑没有收到任何字节，排除“必须经Python桥接才复现”。
2. PUSO探针以100kHz、attach模式读取运行中的MCU，没有 halt/reset/烧录/寄存器写入。
   MCU `RUNNING`，uwTick和控制循环计时持续增加，CFSR/HFSR均0。
   USART2 BRR=364，RX DMA开启、NDTR=64，TX队列可清空，busy=0、ready=1。
3. 同时发送命令和读取MCU统计：

| 命令 | 主机写入 | MCU RX字节前→后 | MCU接收行前→后 | 主机接收 |
|---|---:|---:|---:|---:|
| STOP | 6 | 3769→3775 | 574→575 | 0字节 |
| DISARM | 8 | 3775→3783 | 575→576 | 0字节 |
| PING | 6 | 3783→3789 | 576→577 | 0字节 |
| UART | 6 | 3789→3795 | 577→578 | 0字节 |

4. 关闭显示流后再发PING：RX3815→3821，行581→582；TX head/tail13→14且相等，busy=0。
   最近TX槽13和12中都读到`PONG\r\n`，槽11是`ACK,TOF,0\r\n`，电脑依然零接收。
   这些槽是最近发送内容的保留副本，不是尚未发送的积压。
   UART错误、RX溢出、TX丢弃在该故障阶段均为0。
5. 用户只拔插电脑端USB模块：STOP仍无应答，未恢复。
6. 用户只重启车端蓝牙模块：回传恢复，8/8 PING成功，中位31ms、最大63ms。
   恢复后的TEL uptime约1,481,854ms，比此前1,187,164ms更大，RX累计计数也继续增加，
   证明不是MCU重启清状态带来的恢复。

大块DAP读取曾出现PUSO的`IndexError`传输错误，失败报告原样保留；随后改用逐字节读取
获得了TX内容。没有把失败的大块读取当作成功证据，也没有因此重启或重刷MCU。

模块断电复电区间，MCU UART帧错误计数由0变为7（last_error=0x04）。
恢复后的持续测试及真实网页复查均保持7，不继续增加。它不是此前故障现场的初始错误基线，
不能凭这个数反推此前断联由7次UART错误引起，也没有清零计数掩盖它。

## 为什么速度环会关闭

截图先出现畸形`KQQATEL,...`，KEEP没有匹配的ACK，随后出现STOP与反复验证失败。

- 网页拥有运动时每400ms请求KEEP；KEEP失败会执行STOP。
- `ChassisTest_Stop()`明确调用`AttitudeControl_Stop()`、`SpeedControl_Stop()`、`Motor_StopAll()`。
  STOP关闭速度环是既有安全语义，不是随机改参。
- MCU角环也会在KEEP超过1500ms未刷新、IMU不健康或控制节拍异常时停车。
- 当时没有同步的MCU停机原因记录，不能确定这次最终由网页STOP还是MCU看门狗先触发。
  不为掩盖症状而删除STOP或延长看门狗。

MOVE会自动启用ATEL显示流，单开约5Hz；此前升级对照主要测TEL，不能替代此流量场景。
车端模块恢复后追加ATEL停机测试：117.469秒、490条ATEL、220/220 PING、11/11周期STOP。
PING中位31ms/P95 63ms/最大141ms；周期STOP最大110ms，未再次出现故障。
这个测试没有电机开关噪声和机械运动，不能据此排除行驶供电/地线/无线环境因素。

## 已完成的默认连接方式修改

双击 `mecanum-motor-console/启动上位机.bat` 现在调用 `bridge/start-serial-console.ps1`：

- 只提供localhost:8088静态网页，打开Edge/Chrome。
- 仅在识别为本项目准确路径时关闭原桥接；不终止无关程序。
- 网页默认“串口直连”，Web Serial直接读写CH340，115200/8N1、无流控。
- 不运行串口桥接，不经过8766 WebSocket或1347 VOFA；HTTP服务不参与串口收发。
- 首次由用户选择串口；后续只复用唯一的已授权、可用CH340/CH910；多个候选仍要求选择。
- 打开COM不能直接在线，须得到STOP、DISARM、关闭显示流及PING的匹配应答。
- 所有命令有序确认；STOP抢占旧等待、取消旧排队运动；未发的运动只保留最新目标，
  排队超过250ms拒绝；超时不重发。重复未完成STOP合并，避免按键事件引起STOP洪泛。
- 原始HLOG/WLOG DATA先于END和命令完成交给页面；坏字节不拼接修复成有效应答。
- 空闲PING最多约1Hz，有近期匹配应答则跳过；超时显示离线并关闭会话，不自动恢复运动。
- “导出链路诊断”提供最多2048条有界原始收发记录，旧记录覆盖数单独记录。
  这是通信诊断，不能替代PID采样日志。

需要旧模式时明确运行`启动桥接兼容模式.bat`，网页选择“桥接（兼容）”。
PID脚本继续支持`--serial-port COM9`独占直连；使用前先断开浏览器，不能双重占口。

浏览器API约束依据：[Chrome Web Serial文档](https://developer.chrome.com/docs/capabilities/serial)。

## 验证状态

- Node网页/日志/直连传输共88/88通过，包括真实Streams接口的模拟端口、状态握手、
  STOP抢占和合并、过期目标拒绝、日志顺序、坏字节、无应答与写入失败。
- 原桥接启动归属测试通过；新PowerShell脚本语法通过并实际启动成功。
- 用户在真实Edge选择COM9后，页面显示“串口直连 / 通信在线”，回读R24、四轮PID、
  角环PID、STACK、PS2和停车状态。查询复查FE保持7；8766/1347无监听，8088仅静态HTTP。
- 浏览器自动化在尝试等待下载事件时中断；未获得下载文件，不把“导出诊断”记为实机下载验收成功。
  真实连接已在此之前通过页面状态和截图确认。
- 最后的已观察状态为四轮目标/PWM=0、角环mode=0、速度环关闭，没有执行ARM、非零运动、
  PID写入、MCU复位或烧录。浏览器页面保留给用户，避免无意义地重启恢复后的链路。

## 原始证据入口

均位于`vgt6_module_test/validation/`：

- `serial-disconnect-scene-20261022.json`：最初无桥接零接收。
- `serial-disconnect-dap-scene-20261022.json`：运行态与UART/DMA/错误统计。
- `serial-disconnect-dap-uart-correlation-20261022.json`：逐命令RX计数增量。
- `serial-disconnect-tx-evidence-20261022.json`：包含大块DAP读取失败。
- `serial-disconnect-tx-evidence-word-20261022.json`：逐字节读出PONG及已清空的TX状态。
- `serial-disconnect-after-pc-replug-20261022.json`：只拔插电脑端仍失败。
- `serial-disconnect-after-car-module-restart-20261022.json`：只重启车端模块恢复。
- `serial-disconnect-recovered-atel-soak-20261022.json`：ATEL持续测试与错误基线。
- `serial-disconnect-web-tests-20261022.txt`：88项回归。

下一步针对再次发生的故障，应先导出链路诊断并记录灯态，不先重启MCU；
重点测车端模块供电/共地和模块RX处的UART信号，并将本次单向失效及单独重启恢复
记录交给模块供应方核对固件。没有示波器/逻辑分析仪同步记录前，不盲改波特率、PID或保护阈值。
