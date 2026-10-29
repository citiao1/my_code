# R18：角环 HLOG 微秒时间戳

2026-10-20（Asia/Shanghai）。延续 R17 交接；本轮只修日志时间与下载链路，不开展实车 PID 试验或写 PID 参数。

## 原始问题与留档

板上独立查询确认 R17、角环 mode=0、速度环关闭，HLOG 停止、count=sequence=646、overflow=0。
原有两个诊断文件分别只有250/448条。停车后补下载已保存完整646条连续序号：
`../pid-tuning-adapter/runs/rate-retune-r17-20261020-a8/r18-before-flash-diagnostic.json`。
645个时间差为19ms×160、20ms×324、21ms×161。没有评分、出参或写参。
该文件的旧 `validation_error` 保留首次下载失败原因，`download_complete=true` 表示后续补读已补齐；不代表时序合格。

R17 在控制计算结束时读取 HAL 毫秒 tick；计算/调度耗时与1ms量化会进入日志差分。
这不能证明所有19ms都只是量化误差。旧数据没有微秒信息，不能补算、取整或按序号合成时间。

## 固件与协议

- 版本 `VER,VGT6_BM50_ATTITUDE_R18`，CAPS 为R18并追加 `HLOG_US`。
- `Bsp/Inc/timebase.h` 在极短临界区读取现有1kHz SysTick的HAL tick、LOAD、VAL及挂起位，恢复原PRIMASK；不清COUNTFLAG、不改外设配置。
- 在实际角环更新开始、取得反馈快照前同时捕获毫秒和微秒时间，再与此次更新结果存入RAM。
- 原15字段保留，末尾追加uint32 `tick_us`，单位1us，约71.6分钟独立回绕：

```text
HLOG,DATA,seq,tick_ms,heading_target_mdeg,heading_actual_mdeg,heading_error_mdeg,
rate_target_mdps,rate_actual_mdps,heading_output_mdps,rotation_output_mrpm,
saturated,forward_rpm,left_rpm,mode,tick_us
```

- 记录从44增至48字节，1024条容量不变，RAM增加4096字节；控制周期、增益、限幅、符号、UART和低频ATEL不变。
- 能证明更细的实际时序，不能自动消除真实采样抖动，也不能提高独立样本数量。

## 电脑端与质量门槛

网页脚本缓存v29，兼容R17/R18；解析、CSV导出及停车下载保留微秒列。
适配器严格匹配相应VER/CAPS；R18每条必须有tick_us，禁止混合分辨率，检查两个tick相位一致。
使用uint32差分并按真实微秒构建评分时间轴，要求20000..25000us；R17仍要求20..25ms。
19.999ms与25.001ms均拒绝；缺号、冲突、覆盖、模式、基线/终段与至少10个瞬态样本等门槛不变。

已审查并整合交接时已有的按序号补读改动。仅停车后的幂等HLOG READ允许最多3次补读，保存失败原因；
命令超时、畸形/过期END可有界重试，总量改变或同号内容冲突立即失败。
桥接按READ的offset匹配END，迟到的其他批次END不能应答当前请求；原始HLOG仍直接转发、不混入VOFA。
适配器保存实际R17/R18版本、时间单位和补读记录。PID skill及其vgt6参考已同步，门槛未放宽。

## 离线验证与镜像

- 固件2305项协议断言、IMU和PS2/角环集成回归通过；新增真实timebase头文件的相位、挂起IRQ、PRIMASK恢复及两种回绕测试。
- 适配器31项Python通过，含R18微秒评分/CSV/原始版本留档、R18只读预检、补读超时和缺失微秒拒绝。
- 网页/运动学67项Node通过；桥接17项Python通过，含延迟END偏移匹配及R18数据原样转发。
- PID skill quick_validate通过；本地skill安装位于仓库外，代码commit不包含该安装目录。
- Keil ARMCC5.06 update5 build528，STM32F407VGTx/DFP3.1.1，0 errors / 0 warnings。
- Code=51090、RO-data=2270、RW-data=168、ZI-data=62416。
- HEX：`MDK-ARM/vgt6_module_test/vgt6_module_test.hex`
- SHA256：`9598821F1AED07FC8ADCB8185B77B27B340C8EE7D9B98CF7837AF85655A0143B`。

R17上一固件备份为b19318a6。本轮提交仅涉及HLOG、相关适配/测试/文档及经审查的现有Keil调试配置；
不提交个人uvguix、host EXE、C51修改或整个运行目录。原有.ioc及编译配置继续沿用。
探针检测为单个PUSO CMSIS-DAP1234:5678/COM8，当前uvoptx的CMSIS_AGDI和STM32F4xx_1024.FLM与目标相符。

## 烧录与后续

按本项目既有授权，先commit，再确认停车/解锁关闭并进行标准Keil烧录及读回；完成结果见下节。
发现连接、供电或校验异常应停止，不尝试擦除/解锁/修改保护。烧录结果将追加于此。

后续调参必须重新确认板上VER/CAPS为R18且HLOG_US存在，再使用新的获授权实验采集微秒日志。
本轮未ARM、未运动、未写PID；离线测试不证明实车时序已满足20–25ms，也不证明PID性能。

## 烧录及启动验证结果（已完成）

固件/适配备份提交 `b814730e`。先确认角环停止、速度环失能，再收到 `ACK,DISARM` 后烧录。
Keil本次报告 `Erase Done.Programming Done.Verify OK.Application running`；
独立critical-only读回确认device=0x101f6413、Flash1024KiB、向量表和R18标记0x08005670匹配。
普通复位后再次读回通过，CPU为RUNNING；未使用整片擦除、解锁或保护位修改。

蓝牙启动查询独立确认：

```text
VER,VGT6_BM50_ATTITUDE_R18
CAPS,ATTITUDE_R18,20,10,160,60,60,80,SPEED_TUNABLE,HLOG,PS2,TRACKMOVE,HLOG_MODE,HLOG_US
STACK,4096,1992,2104,1
UART,9600,42000000,4375,0x00,0,0,0,0,0
HLOG,STATUS,0,0,1024,0,0
YAWPID,0.1500,2.5000,60,80,20
HEADPID,6.0000,1.2500,80,20,D_MEAS
SPEEDLIMIT,160
ACK,DISARM
```

ANGLESTATUS mode=0/speed=0、IMU ready=1，电压11.875V。烧录后的空HLOG符合RAM日志重启清空行为；
旧646条已提前留档。未启动角环采样，尚无R18运动日志用于判定真实微秒间隔。
镜像SHA256仍为上述9598821F...55A0143B。

桥接已用项目`.venv`重启并连接同一WHEELTEC-IOS（2A:A2:19:08:08:84），新END偏移匹配已生效。
重连后曾出现一次UART噪声错误（0x04/NE=1），只读整定预检如实拒绝；停车下执行一次普通NRST复位后，
R18只读预检通过，随后三次查询均UART各计数0、LINK各错误计数0、mode=0/speed=0，最终DISARM确认。
这只是当前查询结果，不证明无线链路从此无噪声；后续实车试验仍需逐轮健康检查。
记录见`../pid-tuning-adapter/runs/r18-readonly-preflight-20261020/preflight-after-reset.json`。
网页需Ctrl+F5加载v29。没有进行R18运动实验或实际PID调参。
