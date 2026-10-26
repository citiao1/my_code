# SERVO BLE R7 — 2026-10-26

R6 故障复现：网页连续启动得到 RUN 应答，错误数保持 0，用户确认舵机没有转动。仅检查 0x42 移动标志不足以证明电机模式正在转动。重复 K 是正常心跳，原日志会将操作记录挤出。

R7 改为参考 Waveshare 官方 ST3215.zip / SCServo / SMS_STS.cpp 的 WriteSpe，单独写加速度和 0x2E 速度寄存器，不再发送包含目标位置的整块 Goal 包。连续启动流程为关闭扭矩、确认模式 1、设置速度单位及零速停止、清零速度、使能扭矩、写目标速度、读回扭矩及目标速度。所有相位/模式设置保持 EEPROM 锁定；不更改 ID、波特率、校准或保护设置。

连续运行每 100 ms 读取 0x3A 实际速度及状态，1.5 秒持续无有效方向速度则停止并进入 FAULT。恢复后停留 PAUSE，需新命令。3 秒远程心跳租期保持不变。其他模式保留原有单次相对命令及无自动重试规则。

验证：三个 GCC C99 主机测试套件通过（-Wall -Wextra -Werror）；包括同速度重复启停、正反切换、扭矩使能清掉速度的模拟、ACK 成功但速度未写入的拒绝、零转速故障、故障恢复不自启、按键、租期、校验与去重。Node 11 项测试通过，包括心跳不刷屏且状态继续更新。

原生 Keil ARMCC V5.06u5 build 528：0 errors，0 warnings。Code 20962，RO 1938，RW 108，ZI 12956。HEX SHA256：`865f5c6a14dcf02117a3494b27c93009eb2db538f40a9ac5659fdd3bc5e5ce09`。

烧录和实物重复启停结果待本版本源代码备份完成后记录。编译/模拟测试不等于实物转动验证。

参考：[官方内存表](https://docs.waveshare.com/Memory_Map_Explanation/ST_Servo_Memory_Map_Explanation)，[官方代码包](https://www.waveshare.net/w/upload/4/47/ST3215.zip)。
