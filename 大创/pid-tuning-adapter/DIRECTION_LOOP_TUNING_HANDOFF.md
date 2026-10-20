# VGT6 方向环整定交接文档

更新日期：2026-10-19

## 1. 当前硬件和固件状态

- 固件：`VER,VGT6_BM50_ATTITUDE_R10`
- 能力：`CAPS,ATTITUDE_R10,20,10,200,60,60,80,SPEED_TUNABLE,HLOG`
- 方向环控制周期：20 ms
- 角速度环控制周期：20 ms
- 方向环输出限幅：`+/-80 deg/s`
- 角速度环输出限幅：`+/-60 RPM`
- 轮速上限：`160 RPM`
- 当前桥接：`BRIDGE,BM50_ACK_R3`
- 当前测试方式：整车四轮运动，禁止单独电机运动
- 当前车体状态：测试结束后已发送 `STOP`、`TEL,0`

最后一次状态复核：

```text
HEADPID,6.0000,1.2500,80,20,D_MEAS
YAWPID,0.1500,2.5000,60,80,20
SPEEDLIMIT,160
STACK,4096,1992,2104,1
LINK,...,0,0,0,0
```

## 2. 当前已验证参数

### 轮速环

| 电机 | Kp | Ki | Kd |
|---|---:|---:|---:|
| A | 1.2 | 0.0991 | 0 |
| B | 1.2 | 0.0998 | 0 |
| C | 1.2 | 0.0998 | 0 |
| D | 1.2 | 0.1008 | 0 |

### 角速度环

```text
YAWPID = 0.1500 / 2.5000
```

已测试 10、20、30 deg/s；40 deg/s 出现持续饱和。当前参数保持不变。

### 方向环

```text
HEADPID = 6.0000 / 1.2500
```

方向环使用外环 PD，D 项为测量角速度形式 `D_MEAS`。该参数经过
`±5°/10 RPM`、`±10°/20 RPM`、`±5°/30 RPM` 三档整车前进-返回验证，
均未出现方向输出饱和。

## 3. 已完成的方向测试

方向测试的正确轨迹是连续运动中的航向切换，不是原地转向：

```text
直走
-> +A 度斜向直走
-> 直走
-> -A 度斜向直走
-> 直走
```

每次前进测试完成后：

```text
HEADMOVE,基准航向,0,0
等待 1.5 秒
按反向 RPM 整车返回
```

中间航向切换保持前进速度，不发送零速转向命令。

### 有效结果

| 角度 | 速度 | 结果 | 饱和比例 | 超调 |
|---:|---:|---|---:|---:|
| 15 deg | 10 RPM | 完成往返 | 0% | 0 |
| 15 deg | 20 RPM | 完成往返 | 0% | 0 |
| 15 deg | 30 RPM | 完成往返 | 0% | 0 |
| 30 deg | 10 RPM | 完成往返 | 9.2% | 0 |
| 30 deg | 20 RPM | 完成往返 | 16.1% | 0 |
| 30 deg | 30 RPM | 完成往返 | 23.9% | 0 |

### 无效或拒绝结果

- 之前的“到终点后立即反向”测试结果全部作废。
- 之前的原地转向分段测试结果全部作废。
- `45 deg / 10 RPM`：持续饱和，不能用于 PID 整定。
- `45 deg / 20 RPM`：持续饱和，不能用于 PID 整定。
- `45 deg / 30 RPM`：持续饱和，不能用于 PID 整定。

45 deg 测试失败不是通信故障。失败原因为方向环输出达到固定的 `80 deg/s` 限幅。不要根据这些数据直接增大或减小 `HEADPID`。

### 本轮方向环辨识和候选验证

基线参数为 `HEADPID=5.0000/1.2500`，先采集三组 HLOG，再离线搜索候选。
`6.0000/1.5000` 在 `±10°/20 RPM` 有改善，但在 `±5°/30 RPM` 变差，
因此回退。最终保留 `6.0000/1.2500`，验证结果如下：

| HEADPID | 角度 | 速度 | 结果 | 饱和 | 评分/稳态误差 |
|---|---:|---:|---|---:|---:|
| 5.0000/1.2500 基线 | ±5° | 10 RPM | 基线 | 0% | 2.374 / 1.02° |
| 5.0000/1.2500 基线 | ±5° | 30 RPM | 基线 | 0% | 2.362 / 0.86° |
| 5.0000/1.2500 基线 | ±10° | 20 RPM | 基线 | 0% | 6.505 / 2.80° |
| 6.0000/1.2500 验证 | ±5° | 10 RPM | 通过 | 0% | HLOG 平均绝对误差 1.217° |
| 6.0000/1.2500 验证 | ±5° | 30 RPM | 通过 | 0% | 2.328 / 0.78° |
| 6.0000/1.2500 验证 | ±10° | 20 RPM | 通过 | 0% | 4.489 / 1.99° |

## 4. 当前代码修改

### `attitude_tuner.py`

当前方向轨迹实现位于：

```text
Session.collect_heading_round_trip()
```

当前行为：

- 前进航向序列：`[0, +A, 0, -A, 0]`
- 前进时使用固定正 RPM
- 通过编码器测量每个小段的距离
- 末端停车并等待 `1.5 s`
- 反向使用相反 RPM，并按相反段序列返回
- 最终停车后额外采集 `1.0 s`
- 方向轨迹遥测帧允许年龄：`1.25 s`
- 评分器允许的换向间隔：`HEADING_REVERSAL_PAUSE_S + 1.0`

当前常量：

```python
HEADING_FINAL_SETTLE_S = 1.0
HEADING_REVERSAL_PAUSE_S = 1.5
HEADING_TELEMETRY_MAX_AGE_S = 1.25
```

### 输出目录

方向连续运动扫描结果：

```text
D:/my_code/my_code/大创/pid-tuning-adapter/runs/direction-continuous-scan-20261019/
```

重测结果：

```text
D:/my_code/my_code/大创/pid-tuning-adapter/runs/direction-continuous-a30-v10-20261019-retry/
```

## 5. 当前方法的主要限制

固件方向环虽然每 20 ms 运行一次，但 `ATEL` 遥测约每 200 ms 输出一帧。当前上位机只能看到低频采样：

```text
ATEL,tick,mode,imu_ready,target_rate,actual_rate,
output_rpm,target_heading,actual_heading,
heading_error,output_dps,saturated,speed_enabled,battery
```

这足以做低频轨迹验证，但不适合精确辨识方向环 D 项、延迟和快速瞬态。当前评分结果只能作为运行稳定性指标，不能单独作为最优 `HEADPID` 的证明。

## 6. 下一阶段推荐方案

### 阶段 A：增加 MCU 本地日志

建议修改：

- `vgt6_module_test/App/Src/telemetry.c`
- `vgt6_module_test/App/Inc/telemetry.h`
- `vgt6_module_test/App/Src/command.c`
- 必要时修改 `attitude_control.c/.h`

在方向环 20 ms 控制周期内写入 MCU RAM 环形缓存，至少记录：

```text
tick
target_heading
actual_heading
heading_error
target_yaw_rate
actual_yaw_rate
heading_output
rate_output
saturated
forward_command
left_command
```

建议增加命令：

```text
HLOG,CLEAR
HLOG,START
HLOG,STOP
HLOG,STATUS
HLOG,READ,<offset>,<count>
```

运动过程中只写 RAM，停车后再通过串口分批导出，避免 9600 baud 遥测阻塞控制。

### 阶段 B：桥接和上位机支持 HLOG

建议修改：

- `mecanum-motor-console/bridge/motor_vofa_bridge.py`
- `pid-tuning-adapter/attitude_tuner.py`

要求：

- 支持 HLOG 数据流转发
- 支持批量读取
- 保存原始 CSV
- 保存测试参数、固件版本、PID、速度和角度
- 读取期间保持电机停止

### 阶段 C：增加方向环辨识实验

增加一个独立实验模式，例如：

```text
--experiment direction_identify
```

建议测试矩阵：

```text
角度：5、10、15 deg
速度：10、20、30 RPM
轨迹：0 -> +A -> 0 -> -A -> 0
```

每组至少重复 3 次，分别保存正向和反向数据。45 deg 只做验证，不参与整定。

### 阶段 D：离线模型拟合

保持当前角速度环不变，把已经闭合的角速度环视为方向环对象：

```text
方向环输出角速度目标
-> 实际角速度
-> 实际航向
```

拟合带延迟的离散模型，估计：

- 方向响应延迟
- 上升时间
- 阻尼
- 正负方向不对称
- 不同前进速度下的模型变化

不要把参考航向直接当成执行器输入。辨识输入应使用方向环输出的目标角速度，输出使用实际航向和实际角速度。

### 阶段 E：候选 PID 搜索和验证

建议修改：

- `pid-tuning-adapter/closed_loop_tuning.py`
- 新增方向环模型/搜索模块
- 新增对应单元测试

搜索约束：

- 只搜索 `HEADPID(Kp,Kd)`
- 保持 `Ki=0`
- 每次实际写入变化不超过当前参数的 20%
- 不使用持续饱和数据整定
- 优先最小化最差速度下的表现，而不是只优化某一个速度

目标函数包含：

```text
航向 IAE
超调
稳态误差
响应延迟
输出饱和比例
正负方向不对称
```

候选参数必须经过未参与拟合的数据验证后才能写入。

## 7. 安全约束

- 不测试单独电机。
- 每次前进必须完成反向返回。
- 不使用绝对 `HEADMOVE,0` 作为当前航向参考。
- 方向测试只使用当前航向加相对偏移。
- 角度优先限制在 `5/10/15 deg`。
- 发现饱和、遥测过期、链路计数异常、IMU 异常、编码器异常时立即停止。
- 所有 PID 写入必须 `ACK -> GET 读回 -> 实车验证`。
- 未验证的候选参数不能作为最终参数。
- 任何异常退出都必须执行：

```text
STOP
SPEED,0
TEL,0
```

## 8. 复现命令

当前连续运动方向验证示例：

```powershell
& "D:\my_code\my_code\大创\pid-tuning-adapter\.venv\Scripts\python.exe" `
  "D:\my_code\my_code\大创\pid-tuning-adapter\attitude_tuner.py" `
  --loop direction `
  --experiment heading_round_trip `
  --target 15 `
  --distance-m 0.5 `
  --forward-rpm 20 `
  --seconds 30 `
  --rounds 1 `
  --live `
  --confirm-hardware `
  --output "D:\my_code\my_code\大创\pid-tuning-adapter\runs\direction-continuous-a15-v20"
```

`--rounds 1` 只做验证，不写入新 PID。方向环整定阶段必须先完成 HLOG 和离线辨识，再启用受保护的候选参数写回流程。

## 9. 验证状态

已通过：

- `attitude_tuner.py` Python 编译检查
- `pid-tuning-adapter` 单元测试 20 项通过
- 上位机 Node 测试 29 项通过
- 15/30 deg 连续前进-反向整车测试
- 速度、角速度、方向状态复核

## 10. 可参考的开源方法

- WPILib SysId：机器人端激励和记录，上位机建模和验证。
- ArduPilot AutoTune：小幅度、双向、重复激励，检测饱和/不稳定后回退。
- ROS `control_toolbox`：PID 限幅、抗饱和和滤波实现参考。

本项目不直接复制这些项目的参数或控制器，而是借鉴其“可重复激励、记录、建模、验证、回退”的流程。

## 11. 本次适配状态

已按上述方案增加 RAM-only HLOG。固件不写 Flash，`HLOG,READ` 每次最多
返回 8 条记录，读取必须在停止运动后进行；桥接对 HLOG READ 使用独立长
超时，上位机接收队列满时丢弃旧遥测帧而不丢 HLOG 帧；能力行现在追加 `HLOG`：

```text
CAPS,ATTITUDE_R10,20,10,200,60,60,80,SPEED_TUNABLE,HLOG
```

`attitude_tuner.py --experiment direction_identify` 会执行连续前进、航向
切换、反向返回，停车后分批读取 HLOG 并保存 `round-XX-hlog.csv`。新增的
`direction_model.py` 仅使用 HLOG 中的目标角速度和实际角速度拟合延迟一阶
模型，并对当前 `HEADPID(Kp,Kd)` 附近做不超过 20% 的候选搜索；候选不会自动
写回，必须通过独立验证轨迹后再人工授权写入。本轮实际写回并验证的最终
方向参数为 `6.0000/1.2500`。
