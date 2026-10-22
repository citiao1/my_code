# R24：LQ208BLE V2.0 USB串口链路

2026-10-22。用户已配对新模块，指定115200，授权自主完成连接和烧录。未授权本轮ARM、非零运动或PID写参。

## 实物与配置

本地《LQ208蓝牙模块使用手册_V2.0_261015》：固定115200/8N1、无硬件流控，模块间UART透传，无需AT配置；USB与排针两端配对已由用户完成。本轮未更新模块自身固件、未执行文档中的解除保护/清空Flash步骤。

电脑实测USB-SERIAL CH340 COM9 / VID1A86 PID7523；PUSO DAPLink COM8 / VID1234 PID5678仅作SWD，不作通信口。COM9是当前枚举值，不应永久硬编码。原WHEELTEC BLE FFE0/16字节20ms pacing只保留在显式`--transport ble`路径。

USART2 PA2/PA3，APB1=42MHz，实际HAL初始化115200的BRR=364，实板回读确认；42MHz/364约115384.62bps（+0.160%），而临时手工分频365约115068.49bps（-0.114%）。8位、无校验、1停止位、无流控；DMA1 Stream5/6 Channel4和GPIO/控制周期保持原样。

原板R23仍9600；新模块已能透传乱码。先用只读SWD确认chassis armed=0/active=0、speed enabled=0、四轮target=0、TIM8四CCR=0，再仅临时写USART2 BRR=365，未改Flash/电机/PID。COM9立即收到STOP、DISARM、VER及停车状态。临时UART诊断仍显示软件初始化9600、BRR365是预期；正式R24实测显示115200/364。此前错配期间4次NE错误为前置诊断，烧录后另取新基线。

## 源码改动

`.ioc` USART2改115200，用本机CubeMX6.17.0在D:/Temp/r24-cubemx独立生成完整MDK工程，核对其USART2初始化，再把唯一波特率差异合入现有Core/Src/main.c。保留本项目App/Bsp和Keil分组；当前未参与编译的usart.c也同步，防止将来恢复分文件时用回9600。生成过程存在既有DMA参数导入和RCC派生缓存校验警告，最终生成OK；未因此改时钟/DMA。原始生成日志保留。

VER/OLED/校验标记R24；CAPS在R23基础上增加SERIAL_115200。PID默认值/运行时PID与PIDGET/YAWGET/HEADGET接口、100mm禁止前进、200mm限40RPM、异常距离仅提示、倒退/横移、车头定义和日志格式均保持。

桥接/启动脚本默认serial115200，批处理直接启动、不再要求BLE名字；唯一CH340/CH910可自动选口，也可显式-Port COM9。串口不走BLE分包；按可读字节读取，空闲最长20ms等待，避免旧read(256)/100ms定时积攒应答；检查完整write长度。打开COM后必须取得STOP/DISARM/PING应答才显示在线，空闲定期PING验证远端，丢应答撤销在线并重连，重连后保持DISARM。所有命令继续ACK串行及STOP优先。保留另一窗口已做的有序RX发布队列并回归；不会将END越过此前DATA发布。

网页JS v36兼容R24，使用中性通信状态文字，保留显式BLE兼容。桥接端口WS8766/TCP1347、网页8088不变，PID工具仍走WS无需直接抢占COM。

## PID skill与适配器

按skill-creator做窄范围更新：pid-tuning/SKILL.md与两个BM50 profile加入R24和LQ208 serial115200说明；实机VER/CAPS/UART仍须查询，不能用打开COM代替远端响应。角环FIRMWARE_CAPS和轮速FIRMWARE_PROFILES增加精确R24，保持微秒/前向翻转语义。换链路不授权继续旧实验，不改评分门槛/100Hz与50Hz采样要求、不改增益。本轮仅停机通信/日志验证。

另一窗口已有wheel_tuner/tuning_campaign大幅未提交改动，未覆盖或纳入本次固件备份。wheel_log既有解析错误细分为type/count两种的6行改动随R24白名单保留，不改变接受规则。当前工作区全适配器套件71项中6项失败（4errors/2failures），集中于未改的地面试验实现与旧模拟测试不一致：ARM完整ACK、先正向holdout、路程裕量和下载重读行为；不能声称全套通过。R24专用角环兼容6/6、轮速R21–R24身份相关3/3通过。未用失败试验调参或放宽校验。

## 构建与验证

Keil原工程、STM32F407VGT6/LQFP100、FW_F4 V1.28.3、DFP3.1.1、ARMCC5.06u5 build528。0错误0警告；Code60442/RO2662/RW236/ZI103836。

HEX SHA256：`4D92496210545D3ECFC03F97004E4546A5E01070CAD36705CEB587B67B6D84A9`。

固件协议2383断言通过；网页77/77；桥接30/30（含新串口启动验证、无远端不报在线、完整串口write与短write拒绝）；PS启动语法及进程归属测试通过。未声称历史PS2模型问题已修复。

先做本地源码备份，再正常非破坏烧录、验证和板上协议复测；完成结果另行追加。全程未ARM/运动/PID写参。


## 烧录和实板验收结果

源码e71be4f9备份后正常烧录。Keil在20:00:01报告Erase Done/Programming Done/Verify OK/Application running；独立读回设备0x101f6413、1024KiB、向量表和R24标记0x08005ab8一致，复位后再次读回通过并确认RUNNING。未mass erase/解保护/改option bytes。镜像仍为上面的4D924962…7B6D84A9。校验器采用安装版pyOCD commander同款Session/SharedDebugProbeProxy构造，禁用auto_unlock和resume_on_disconnect，实际读回通过。

COM9重新启动桥接后实际VER=R24/CAPS含SERIAL_115200；UART=115200,42000000,364，所有错误为0；PS2锁定、角环/速度环停止、四轮目标/PWM0。PIDGET/YAWGET/HEADGET与烧录前一致，没有写PID。BRR364为本项目HAL宏实际结果；最初主机测试夹具误按四舍五入写365，已按实板和HAL公式改364并重新通过2383断言，不影响已烧录的镜像。

停机对照54次查询全部成功：无串流中位156.5ms/P95203ms/最大219ms；TOF单流203/375/453ms；TOF+TEL219/359/359ms。这轮串口波特率升级不等于无线RTT必然下降，不能宣称已消除延迟。

WLOG停机213条、8条一批、零重试完整下载，序号/字段/END/状态与微秒相位通过，间隔10146..12051us，下载约15.078s。四轮目标/PWM/enable/arm均0；仅为传输/时间轴验证，无PID评分。原组合诊断随后尝试HLOG时，在角环mode0下正确得到0样本，因其非空断言失败；完整原报告保留，单独派生的WLOG报告再次严格校验213条通过。没有为了获得HLOG样本而ARM/转动，因此本轮未验证运动HLOG，不把空HLOG冒充通过。

首版串口桥接120.25秒、236次PING（中位187ms/P95344ms/最大781ms）、23次STOP（中位157ms/P95281ms/最大343ms）均应答；未观察断联，UART/LINK错误不增长，队列观察最大0帧，TOF计数24.6733Hz。测试中网页连入初始化，包含观察者流量。随后仅桥接补充STOP/DISARM取消心跳时不误判掉线的竞态处理（离线30项已过），固件镜像不变；最终桥接另做短时STOP/查询复核。串口首次启动验证尚未完成时，过早发出的DISARM曾被启动STOP取消，该失败未作为成功数据，等待验证在线后查询全部通过。

PID skill已按skill-creator更新，quick_validate通过（使用-X utf8）；文件路径/哈希在validation/r24-build-flash.json。全适配器71项的6个既有不一致仍保留，未宣称全套通过。下次开展live前应先维护该窗口的地面测试与模拟夹具一致性；本轮不恢复任何旧运动计划。


最终桥接c07bfd8c实板交替20次STOP+20次PING全部应答，无离线事件，UART错误仍为0。STOP中位141ms/最大375ms、PING中位140ms/最大297ms。上位机已重新连接并加载JS v36。一次桥接重启因旧8766套接字尚未完全释放而启动失败（10048），待端口释放后重启成功；该软件进程重启不计作无线断联，也未修改系统端口复用/安全选项。

交付保持COM9/115200串口桥接和8088网页服务运行；电机DISARM、角环/速度环停止、PS2锁定、周期TEL/TOF关闭。未做地面运动或PID写参。本轮验证只覆盖当前近距离环境；新模块仍有约0.1–0.3s常见往返等待及更长尾部，不宣称遥控延迟完全解决。TOF仍常见状态4无效，按原设置提示而不改保护策略。最终测试/原始线数据在validation/r24-*，源备份e71be4f9/c07bfd8c。


启动脚本补修：原50616确为本项目桥接，因进程命令行绝对路径使用正斜杠、归属校验仅匹配反斜杠而被误拒绝。现对两端路径分隔符统一后再做原有完整路径/边界/解释器检查；仍拒绝其他程序和相似后缀路径。7种归属用例与语法检查通过。实际执行Windows PowerShell启动脚本，成功识别并重启50616；新桥接56112监听8766/1347，COM9实测PING/R24/UART115200、零错误、DISARM和停止状态通过。固件镜像不变，无重烧录。
