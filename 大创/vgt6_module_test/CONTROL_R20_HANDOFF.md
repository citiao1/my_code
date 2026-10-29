# R20：轮速高频日志、距离限速、新车头

2026-10-21（Asia/Shanghai）。用户要求三项程序修改，未授权本轮实车运动或PID搜索。

## 行为

1. 按更新后的PID skill，角环HLOG仍50Hz、16列，微秒间隔19900..25000us；修正网页旧的20000us下界，与新skill一致。轮速新增独立WLOG，直接在10ms速度控制更新后记录，包含停机基线，不用TEL/ATEL评分。
2. 以TOF为车头：距离≥100mm不额外限速；50<d<100mm时，前进上限=`当前轮速上限*(d-50)/50`；≤50mm禁止前进。倒退、横移和原地转向保留。每个控制更新重算，不必等待新遥控命令。
3. 用户明确选择测距无效、断线、超过350ms时保持原请求速度并提示异常；没有采用故障即停车策略。距离恢复后按新读数重新限制。阈值是触发目标限速/停止前进的距离，尚未测量实际刹停距离。
4. 车体前进/左移轴旋转180°；逆时针yaw正号不变，IMU相对航向无需再加180°。电机/编码器A/B/C/D物理编号及极性不变；新轮位C=右前、D=左前、A=左后、B=右后。MOVE/DRV/HEADMOVE/TRACKMOVE、手柄统一经过速度层变换。网页里程计和ground_speed的TEL前进量同步反转。
5. 原始SPD/单轮JOG保留物理极性，但不能绕过距离保护；单轮限速按整组请求缩放，不会为补偿前进量而启动其他三个轮。车体移动只裁剪前进分量，保留横移与转向。
6. PID/YAWPID/HEADPID协议写入要求速度环关闭、角环停止、无JOG且无正在记录的日志；ACK后可读回。现有默认HEADPID6/1.25、YAWPID0.1815/1.8062保留，未进行整定。

## WLOG 与调参

- 与HLOG共用一个union缓冲，容量1024，100Hz约10.24s；建议单次≤9s。先停车、停止记录，再CLEAR显式切换格式；CLEAR替换上一份RAM数据。不能同时记录两种格式。
- `WLOG,CLEAR/START/STOP/STATUS/READ,offset,count`；READ≤8，停止运动和记录后下载。`WLOGINFO`返回增量PID形式、10ms周期、滤波0.7/0.3、PWM60%、160RPM及380CPR元数据。
- DATA共30列：`WLOG,DATA,sequence,tick_ms,tick_us,enabled,armed,angle_mode`，随后A/B/C/D各四列`target_rpm,filtered_rpm_x10,raw_rpm_x10,pwm_mpercent`，最后`saturated_mask,tof_limited,battery_mv,tof_mm,tof_valid,encoder_ready`。PWM字段记录实际舍入后的PWM指令×1000；饱和位指PID内部限幅。
- 记录在主循环，ISR不格式化、不发送日志。开始日志不ARM。TOF实时流在任一日志记录及下载期间暂停；板载测距与保护仍运行。
- 新`wheel_tuner.py`默认只读；精确匹配R20 VER/CAPS/WLOGINFO。单轮PI只搜索Kp/Ki_step，Kd_step必须为0；Ki是每更新系数，PID忽略dt，不直接复制连续Ki。
- 实验路径为WSPD/WKEEP，300ms看门狗，过期清目标不自动恢复；单轮原始反馈绝对值>300RPM中止。参数写入逐次≤20%、ACK/读回，锁定其他轮和角环参数。
- 采用现有BO/DE与重复/独立留出campaign；当前工作区先前更新的共享搜索/评分依赖也经过复核，相关源文件随本版备份，旧运行数据不加入提交。每轮原始wire/记录/重试/命令线/增益保存；失败立即STOP，只在初值已独立验证且当前参数、健康仍可核对时恢复，否则记录未恢复。
- 下载要求连续序号、count=sequence、容量1024、无溢出、精确END及下载前后STATUS一致；最多3次有限重读，可去重相同重传，冲突/缺样/乱序拒绝。ms/us相位<1000us，间隔9900..15000us；不插值补样。
- 评分进一步要求实际MCU参考边沿/阶段时长、单轮隔离、健康电池/编码器/模式、无TOF限速、一秒静止基线、至少10个10–90%独立瞬态样本、最终一秒RMS/漂移、≤25%饱和和campaign≤30%超调。快到无法解析的响应仍拒绝调参。
- 示例只读：`mecanum-motor-console/bridge/.venv/Scripts/python.exe pid-tuning-adapter/wheel_tuner.py --wheel A --output pid-tuning-adapter/runs/NEW-WHEEL-SESSION`。实车需另有完整运动计划授权才加`--live`。`replay_wlog.py`离线复核原始JSON，无连接/写参。

## 构建与测试

STM32F407VGT6、Cube FW_F4 V1.28.3、DFP3.1.1、ARMCC5.06u5 build528；沿用原.ioc，无引脚/外设变化，不需要重新生成。原Keil工程新增front_guard.c。

最终Keil构建0错误0警告：Code59878、RO2606、RW216、ZI103816，处于128KiB SRAM范围内。HEX SHA256：`3D5216665F11867CAF516AFE2CDFAD03A720E5AEF3220DDF92B62924D6E5E9A4`。

- 固件协议2353项断言通过，涵盖距离阈值/失效策略、动态限速、新车体轴/yaw、WLOG混录隔离/环形溢出/30列/下载保护、WSPD看门狗、停车写参；陀螺仪测试通过。停机采样只观察反馈、不覆写JOG拥有的PWM，新增实际电机输出stub验证JOG保持/限速/超时停止。
- 网页73项、桥接20项通过；新轮速解析/下载/评分/模拟campaign测试通过，新增R20角环只读兼容测试。最终数量以本次命令日志为准。
- Python完整适配器最终59项通过（42.22秒），含停机采集顺序、R20角环只读兼容、轮速BO/独立留出/失败不盲写的模拟测试；这些是离线/内存桥接证据。
- PS2原有松杆模型`reverse_arc<0.5°`仍失败，沿用R19已知0.500896°边界，不改阈值、不擅改用户PID。仅将纯前进的物理轮目标预期改为-140RPM以匹配新车头；未宣称全套PS2回归通过。
- 不以离线仿真证明实际方向、距离精度、刹停距离或PID性能。单轮悬空调参和底盘落地验收是后续不同验证层。

## 板上验证状态

首段停机WLOG真实75条出现9047..10952us（另有10003us）交替，未通过9900..15000us门槛，作为失败诊断保留，不用于评分。原因是HAL毫秒门控叠加周期内传感器处理时间，实际控制调用可能早约1ms。随后App改为微秒门控，从上一控制块完成后至少等待10000us；保留每条实际采集时间和实际elapsed_ms，不插值、不伪造时间戳、不放宽校验下界。

最终源码备份 `1bb7f460` 后烧录（前置源码/修正备份 `86bce50e`、`12257d17`）。Keil全镜像 `Verify OK.Application running`；独立读回设备0x101f6413、Flash1024KiB、向量表及0x080059e4处R20标记一致；复位后RUNNING，最终HEX哈希见上。

修正后的实板停机采集75条，全部通过完整下载和ms/us校验，实际间隔 **10146..12016us**，标称100Hz但以实际时间戳为准。全程DISARM、mode=0、speed=0、四轮目标/PWM均为0，日志不包含可评分的速度阶跃，不作增益结论。下载后STACK4096/2544/1552、保护正常；UART各错误0，LINK溢出/TX丢弃/串口错误0。最终YAWPID0.1815/1.8062、HEADPID6/1.25、四轮默认增益读回不变。

`validation/r20-stopped-wlog.json`保留所有原始wire、命令、重试、记录和时序范围；CSV同名。首轮失败保留在`validation/r20-first-stopped-cadence-rejected.json`。`validation/r20-wheel-readonly-preflight.json`是实际新入口只读预检，motion_started=false、未ARM/写参。

本次TOF连通、I2C错误0，但采集时未获得有效目标读数（状态4）；按用户选择保持请求速度并提示保护暂不可用，未把无效读数当成距离0。测距本身此前R19已观察有效目标，当前R20不声称实际50mm刹停已验收。

烧录前实际VER=R19，DISARM应答成功；ANGLESTATUS mode=0、speed=0，手柄锁定且摇杆居中。源码完成本地Git备份后，按项目既有授权进行非破坏烧录及独立读回；本节后续追加结果。

上位机 `http://127.0.0.1:8088/`，脚本v32、chassis-test v3、wheel-log v1；新面板可停机采集/停止/下载轮速CSV。连接时不ARM，不自动开日志。

已在真实网页点击“下载轮速CSV”，完整校验75条并完成CSV下载，界面显示75/1024、溢出0及新轮位映射；已恢复实时距离开关。截图：`C:/Users/28097/.codex/visualizations/2026/10/20/01a159dd-7ace-7e23-9b1f-3af439e2da66/r20-wheel-log-front.png`。浏览器/桥接保留运行；未点击ARM或任何运动按钮。
