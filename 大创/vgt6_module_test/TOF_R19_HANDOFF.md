# R19：DL1B 测距与实时距离显示

2026-10-21，STM32F407VGT6；本文件记录实现和实测，不能代替运动授权。

## 接线和功能

- 用户确认模块已接好：PB8=SCL，PB9=SDA；XS 未接 PC5。代码不配置或驱动 PC5。模块供电按用户资料接 5V、共地；I2C 为 3.3V 逻辑，XS 依靠模块自身上拉释放。
- I2C1 400kHz，7位地址 0x29，16位寄存器地址。DL1B/VL53L1X ID 要求 0xEACC。
- 主循环状态机和 HAL I2C 中断传输；不在控制中断中等待测距。长距离模式，100ms 测量预算、120ms 周期；350ms 无新有效结果即作废，掉线每秒重试。
- 仅显示距离，不接入避障、停车或速度控制。有效范围 1..4000mm，原始状态9有效；无效、过期、断线均清除旧距离。
- 独立 TOF 流默认关闭，可在网页“实时距离”开启；2Hz 上报，HLOG 录制和分批下载期间暂停，保留 TEL/ATEL/HLOG 原格式和 HLOG 微秒时间戳。

协议：`TOF` 查询；`TOF,1` 开流、`TOF,0` 关流，回执 `ACK,TOF,1/0`。
数据为 `TOF,tick_ms,connected,valid,distance_mm或-1,raw_status,age_ms,errors`，共8列。
网页本机超过1600ms未收到TOF也会清除读数。

## PID 默认值

按用户要求，开机 HEADPID=6.0000/1.2500、YAWPID=0.1815/1.8062。
它们仅用于初始化，现有 HEADPID/YAWPID 命令和网页仍可修改、读回 RAM 参数；STOP 后保留，重启恢复这组默认值。不新增 Flash 参数持久化，也不在控制计算中重复覆盖参数。

## 原生工程与来源

- `.ioc` 添加 I2C1/PB8/PB9、Fast400kHz、事件/错误中断优先级5；修正本机数据库合法 MCU 别名，实际芯片仍为 STM32F407VGT6。
- CubeMX 隔离生成目录 `D:/Temp/codex_vgt6_tof_r19_20261021/vgt6_module_test`。第二轮成功生成，无 I2C 未完成配置警告；日志 `D:/Temp/codex_vgt6_tof_r19_20261021/cubemx-stdout-2.log`。数据库另有 TIM/null/IP 及更新器通用告警，未作为编译成功证据。
- 仅移入生成的 I2C 初始化/MSP/IRQ、驱动及工程条目，保留原工程其他配置。不要用隔离工程覆盖完整主工程。
- STM32F4xx DFP3.1.1，ARMCC5.06 update5 build528，Keil `D:/Keil5/UV4/UV4.exe`，target=`vgt6_module_test`。
- ST 官方 [VL53L1X 驱动](https://github.com/stm32duino/VL53L1X/blob/main/src/vl53l1x_class.cpp) 和 [UM2510](https://www.st.com/resource/en/user_manual/um2510-a-guide-to-using-the-vl53l1x-ultra-lite-driver-stmicroelectronics.pdf) 用于寄存器/时序。默认配置数组来源及 BSD3 许可证保存在 `App/Inc/vl53l1x_defaults.h`、`ThirdParty/VL53L1X/LICENSE.md`。未复制用户资料中的 GPL 例程代码。

## 验证和已知限制

- Keil 构建0错误0警告，Code57430、RO2374、RW204、ZI62556。
- HEX：`MDK-ARM/vgt6_module_test/vgt6_module_test.hex`；SHA256：`E5ED905D37076797B522BC1A87A93090ADA73442CA4C94E2E5C1C00A07F5D615`。
- 固件协议2327项断言通过，含TOF及默认PID/运行时改写后STOP保持；陀螺仪测试通过。
- 新增传感器模拟测试覆盖初始化、校准、字节序、无效/越界/过期、掉线重试、tick回绕和无数据超时；总线模拟测试覆盖TX缓冲区所有权、句柄过滤、BUSY、超时恢复，均通过。
- 网页 Node69/69、桥接18/18、R19只读兼容测试1项通过。
- 原 `test_ps2.c` 的0.5度反向轨迹断言未通过。临时诊断副本显示108组模型中4组为0.500896度（要求<0.5）；未更改原阈值或用户指定PID，不能声称全套PS2回归通过，也不能据模型认定实车性能。未进行实车运动测试。
- 整定器正被另一窗口并行修改：全套37项当时1项历史长度断言失败；本次只修改R19版本/CAPS与微秒要求的两处兼容逻辑、增加独立兼容测试。其他搜索器改动和数据目录保留，不纳入本次提交。

## 本次烧录记录

用户最新指令“车已上电，可以烧录”覆盖此前暂不烧录要求。
烧录前 BLE 读回 VER=R18；ANGLESTATUS mode=0、speed=0；PS2摇杆居中，DISARM 返回 ACK,DISARM，随后再次确认 mode=0、speed=0。
识别到单个 PUSO CMSIS-DAP（1234:5678，puso_daplink_lab，COM8）。
先做相关源码本地 Git 备份，再运行 `tools/flash-dap.ps1`，要求 Keil 全镜像 Verify OK、独立设备/向量/版本标记读回和复位运行验证。
当前状态：待执行烧录；后续追加实际结果，不能将构建通过等同于板上测距成功。

## 启动上位机

网页 `http://127.0.0.1:8088/`，WebSocket `ws://127.0.0.1:8766`；桥接使用 `mecanum-motor-console/bridge/.venv/Scripts/python.exe -u motor_vofa_bridge.py --transport ble`。
刷新网页获取 JS v30 / CSS v21，连接后只开启“实时距离”即可，不需要 ARM。
