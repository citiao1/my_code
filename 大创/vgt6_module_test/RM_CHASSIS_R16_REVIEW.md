# R16：战队英雄底盘对照与平移/停车改进

2026-10-20。开发阶段只修改源码、离线回归和 Keil 编译；随后用户授权烧录，
R16 已通过无线 DAP 写入、全镜像独立读回及蓝牙版本确认，未运行电机。
固件、网页及当前板端版本均为 `VGT6_BM50_ATTITUDE_R16`。执行记录见末节。

## 实际读过的开源代码

- 浙大国际校区 Meta 战队 [Meta-Embedded](https://github.com/Meta-Team/Meta-Embedded)：
  仓库包含英雄等兵种。
  [mecanum_chassis_scheduler.cpp](https://github.com/Meta-Team/Meta-Embedded/blob/master/dev/scheduler/mecanum_chassis_scheduler.cpp)
  的 `CHASSIS_REF_MODE` 用线性运动学生成四轮目标，轮距/轴距及轮周长换算物理单位；
  `GIMBAL_REF_MODE` 按实测云台相对角变换平移速度。
  [chassis_logic.cpp](https://github.com/Meta-Team/Meta-Embedded/blob/master/dev/logic/chassis_logic.cpp)
  跟随模式用自动回正 PID 生成旋转目标。该文件存在未启用的减抖注释，不当作现成功能。
- 沈阳理工大学 Ambition 战队
  [RM2021_Ambition_hero](https://github.com/Lonelysky1999/RM2021_Ambition_hero)，
  战队作者在 [RM 官方社区开源帖](https://bbs.robomaster.com/article/8667?source=5) 说明项目来源。
  [Gimbal 侧 chassis_task.c](https://github.com/Lonelysky1999/RM2021_Ambition_hero/blob/master/Gimbal/TASK/chassis_task/chassis_task.c)
  处理输入死区、一阶平移滤波、停止时清零滤波尾巴、航向跟随和实测相对角坐标变换；
  [Chassis 侧 chassis_task.c](https://github.com/Lonelysky1999/RM2021_Ambition_hero/blob/master/Chassis/TASK/chassis_task/chassis_task.c)
  用线性分解、四轮峰值同比缩放和各电机速度 PID 输出电流。

结论：可借鉴的是“输入整形 → 坐标变换 → 运动学 → 轮速闭环”的分层，以及独立的
转向反馈；上述遥控底盘路径并不能证明有真实地面 X/Y 位置闭环。麦轮线性矩阵本身
不是简陋错误，也不需要为了任意角平移再叠一层同反馈的车体速度 PID。
两队电机/CAN 电流接口与本车 BM50 PWM 不同，不照抄增益、轴距、减速比或轮序。
参考控制思路后独立实现，未复制其源码或把其未启用/占位逻辑当作成熟算法移植。

## 本轮代码改变

### 下位机 TRACKMOVE：20 ms 实际航向变换

新增 `TRACKMOVE,<heading_deg>,<field_x_rpm>,<field_y_rpm>`，对应
`AttitudeControl_SetTrackedDrive`。heading 是绝对 IMU yaw，平移向量是同一 IMU 固定
坐标系中的等效 RPM，不是 m/s；向量模长限制 100 RPM。仅用于有人监督的运动测试。

每 20 ms 以实测 yaw 执行：

```text
body_forward = field_x*cos(yaw) + field_y*sin(yaw)
body_left    = -field_x*sin(yaw) + field_y*cos(yaw)
```

旧测试按“目标航向”只变换一次，实际航向偏离时平移方向会一起偏。新路径在 MCU
本地持续变换，蓝牙不承担快速轮速/航向控制。世界向量更新采用 100 RPM/s 的二维
幅值斜率限制，保持方向；明确零目标立即取消平移尾巴，STOP 仍直接制动。

固定航向仍用已整定 HEAD PD → YAW PI（6/1.25、0.15/2.5），不重新捕获目标掩盖误差。
D 测量增加 40 ms 低通；误差 ≤0.35°、原始及滤波角速度 ≤1.5°/s 连续 120 ms 后
进入静稳区，清除角速度积分并将旋转轮速置零。误差 >0.8°或原始角速度 >3°/s 时
退出静稳区并恢复纠偏。同目标重复命令/KEEP 不会重置静稳状态。

旋转优先分配四轮总限幅余量，平移不足时等比缩放，方向不变。160 RPM 运行上限、
200 RPM 硬上限、60% PWM、60 RPM 旋转上限、80°/s 目标上限均不增加。
原始 HEAD/HEADMOVE/YAW/MOVE 调参公式保持不变，PS2 保留 R15 松杆刹停状态机。
TRACKMOVE 不自动开启遥测，不接受 PS2 控制权冲突，也不在速度环失能后自动恢复。
失联 1.5 s、IMU 异常、调度超时、STOP 等仍进入原安全停车路径。

### 上位机：慢速横偏纠正与终点退出

90°、方形及任意角测试统一使用 TRACKMOVE，仍独占串行 KEEP/ENC/ANGLESTATUS。
不自动 ARM、写 PID、变更限幅或写 Flash；启动前仍需人工确认区域/下地/急停。

- 测试起始 yaw 定义测试坐标系；用四轮增量和 IMU 中点角做里程计。
- 横偏不再仅用于报警：加 0.6 s^-1 的非积分 P 修正，5 mm 死区，修正速度不超过
  min(8 RPM, 设定速度的20%, 当前纵向速度的40%)。它是蓝牙反馈频率下的低带宽
  路径修正，不是 MCU 高频 X/Y 位置环，也无法修正编码器不可观测的打滑。
- 减速按剩余距离/0.65 s 生成目标，最低纵向目标 8 RPM，避免极小轮速爬行；
  余量 ≤5 mm 后锁存本段到达、发送零平移，不为毫米级误差反向追点。
  上位机采样和实际制动会带来终点误差，不能声称 5 mm 实车精度。
- 回程纵向距离包含去程停稳后的实际估计投影，横偏纠正指向原始路径线，
  不把去程漂移后的平行线当作返回路径。
- 停稳使用 ≥600 ms 累计绝对计数变化的平均轮速 ≤6 RPM，加航向误差 ≤3°、
  角速度 ≤4°/s，稳定判据持续 ≥600 ms。替换 10 ms 瞬时轮速 ≤8 RPM 判据：
  380 CPR 时一个 10 ms 脉冲就代表约 15.8 RPM，旧判据易反复失效。
- 完成或异常发 STOP，STOP 未确认不显示成功。没有增加重连后自动恢复运动。
- CSV 新增 translation_frame=INITIAL、along_m、cross_m；forward_rpm/left_rpm
  是测试起始坐标系目标，不再是原来按假定车头变换的车体量；AttitudeControlSnapshot 中的平移量
  仍为 MCU 实际输出的车体量。摘要显示最大横偏与终点偏移，均标明编码器估计。

## 验证与边界

通过真实固件模块的主机协议测试、角度融合测试、PS2/速度/角环集成测试；覆盖
TRACKMOVE 范围/NaN/完整帧/控制权、实际 yaw 变换、持续噪声零输出、静稳退出、
零目标取消尾巴、失联停车与不自动重启。18 组假设转向模型的双向固定角目标
收敛与末段零输出回归通过，另保留 108 组手柄松杆假设模型回归。
Node 测试覆盖任意方向往返、偏置车头、横偏修正符号/限幅、量化轮速停稳、
STOP/KEEP 竞态、断链、冻结反馈及取消所有权；Python 桥接与启动脚本测试通过。
这些模型不是本车数据，没有据此重新整定 PID 或宣称实车抖动已消除。
本轮最终计数：2293 项固件协议/运动学断言、63 项 Node 测试、16 项 Python 桥接测试
通过，另有陀螺仪、PS2 集成、启动脚本回归通过。

Keil ARMCC 5.06 update5 build528 编译链接：0 errors / 0 warnings。
Code=50938、RO-data=2250、RW-data=168、ZI-data=58320。
HEX：`MDK-ARM/vgt6_module_test/vgt6_module_test.hex`。开发验证完成后已按用户授权烧录。

下次在确认区域/急停可用并明确授权烧录后，先 30 RPM / 30 cm 测 0°、90°、17°，
再测小车头偏置、90°转向和方形；对比 ground 标线与 CSV，特别记录回程减速、
减速保持以及最终 STOP 之后是否仍有抖动。若 STOP 后仍剧烈抖动，应转查驱动制动、
供电、接线、结构，而不是继续增加航向环增益。尚需实车记录判断剩余低速量化、
静摩擦、滚子/安装误差、方向环带宽和电机电气制动各自的贡献。

## R16 实际烧录记录

2026-10-20，用户明确授权后通过 PUSO DAPLink（1234:5678，puso_daplink_lab）
使用现有 Keil STM32F407VGTx 工程烧录。烧录前 DISARM 得到 ACK；没有启动电机。
Keil 预编译 0 errors / 0 warnings；烧录日志显示 Erase Done / Programming Done /
Verify OK / Application running，日志为 `MDK-ARM/flash-r16.log`。

独立 verify-dap.py 全镜像 53356 字节逐字读回一致，向量表与 R16 标记一致，
标记地址 0x080055f8；设备 ID 0x101f6413、Flash 1024 KiB，CPU 状态 RUNNING。
读回记录为 `MDK-ARM/readback-r16.stdout.log`，stderr 无异常。
HEX SHA256：`6339DF0B98380801B1798A1B806EBF469F3A14C98A70D0A8567B8845533B2885`。
蓝牙回读 VER=VGT6_BM50_ATTITUDE_R16，CAPS 包含 TRACKMOVE，
STACK=4096/1496/2600/healthy，PS2 已连接但未解锁，IMU 就绪，角环及速度环停止。
没有修改保护/选项字节，没有写入运行时参数到 Flash，也没有做地面运动验收。
