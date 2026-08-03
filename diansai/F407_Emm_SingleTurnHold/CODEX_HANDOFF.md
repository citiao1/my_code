> 注意：本文描述的是旧版单电机绝对 30 度逻辑，现已停用。当前双电机同步回零、PE0 按键每次顺时针相对转 30 度的说明，请以 `MULTI_MOTOR_GUIDE.md` 为准。

# F407 Emm 单圈绝对定位与 30 度保持：交接文档（旧版）

## 1. 目的与边界

本工程让 STM32F407 通过 USART2 控制一台 Emm 闭环步进电机。它不使用限位开关或霍尔传感器，而是依赖电机内部保存的**单圈零点**：上电后先回到该零点，再以零点为参考走到固定目标角度，之后持续检测并在被外力扰动后回到同一个绝对目标。

它仅适用于机构的全部运动都在一圈之内的场景。不要将“单圈就近回零”直接用于需要跨多圈累计位置的机构。

当前配置：地址 `1`，一圈 `3200` 脉冲，目标 `30.0` 度，正常运行不重新保存零点。

## 2. 接线与运行前提

| F407 端 | 电机端 | 作用 |
| --- | --- | --- |
| PD5 / USART2_TX | 电机 TTL_RX | F407 向电机发送命令 |
| PD6 / USART2_RX | 电机 TTL_TX | F407 接收电机回复 |
| GND | GND | 必须共地 |

电机电源独立供电。连接 PD6 前确认电机 TTL_TX 是 3.3 V 电平；不应将 5 V TTL 输出直接接到 F407 引脚。

## 3. 源码分工

| 文件 | 负责内容 | 修改建议 |
| --- | --- | --- |
| `Inc/stepper_hold.h` | 所有可调参数、状态和故障枚举、对外状态结构 | 修改目标、速度、超时、容差时优先改这里 |
| `Src/stepper_hold.c` | 单圈回零、走目标、位置轮询、保持和故障状态机 | 需要改变控制策略时改这里 |
| `Src/main.c` | 外设初始化、启动 DMA 接收、循环调用状态机 | 保持主循环非阻塞 |
| `Src/stm32f4xx_it.c` | USART2 IDLE 中断，将 DMA 接收数据封为一帧 | 改变帧接收机制时改这里 |
| `Inc/usart.h` | `rxCmd`、`rxFrame`、`rxCount`、`rxFrameFlag` 的外部声明 | 不要绕过帧缓冲直接解析 DMA 缓冲 |
| `Src/usart.c` | USART2 和 DMA 关联配置 | 改串口、波特率或引脚时核对这里和 `.ioc` |
| `Src/dma.c` | DMA 时钟与中断优先级 | 改 DMA 通道时同步核对中断文件 |
| `Src/Emm_V5.c` / `Inc/Emm_V5.h` | 厂商串口协议命令封装 | 一般不改；扩展其他电机功能时调用其已有 API |

## 4. 主流程

`main()` 完成 GPIO、DMA、USART2 初始化后：

1. 清 USART2 空闲标志，打开 `UART_IT_IDLE` 空闲中断。
2. 调用 `HAL_UART_Receive_DMA(&huart2, rxCmd, CMD_LEN)`，让 DMA 持续接收串口字节。
3. 调用 `StepperHold_Init(HAL_GetTick())` 初始化状态机。
4. 在无限循环中反复调用 `StepperHold_Update(HAL_GetTick())`。

状态机不能加入 `HAL_Delay()` 等阻塞等待。它依靠 `HAL_GetTick()` 比较时间，让主循环持续有机会处理新收到的位置帧。

## 5. 状态机总览

```text
BOOT_DELAY
  -> CALIBRATING_ORIGIN
  -> HOMING
  -> MOVING_TO_TARGET
  -> HOLDING
       | 位置连续偏离
       v
   MOVING_TO_TARGET

任一阶段出现配置错误、通信超时、动作超时或重试耗尽
  -> FAULT（立即发送停止命令）
```

### `STEPPER_HOLD_BOOT_DELAY`

上电后等待 `STEPPER_HOLD_POWERUP_DELAY_MS`，当前为 500 ms。等待完成后执行 `Emm_V5_En_Control(..., true, false)` 使能电机，并进入下一状态。

### `STEPPER_HOLD_CALIBRATING_ORIGIN`

先等待 `STEPPER_HOLD_COMMAND_GAP_MS`，当前为 100 ms，再按编译开关选择动作：

- `STEPPER_HOLD_CALIBRATE_ORIGIN_ON_BOOT == 1`：调用 `Emm_V5_Origin_Set_O(addr, true)`，将**当前机械位置**永久保存为单圈零点。随后进入 `HOMING`，并在其中隔 100 ms 再发回零命令。
- `STEPPER_HOLD_CALIBRATE_ORIGIN_ON_BOOT == 0`：直接调用 `Emm_V5_Origin_Trigger_Return(addr, 0, false)`，以单圈就近模式回到之前保存的零点，然后进入 `HOMING`。

`#if` 是预处理条件编译：`0` 分支根本不会被编译进固件，不是程序运行时才判断。正常运行必须为 `0`，否则每次上电都会把当前角度覆盖为新零点。

### `STEPPER_HOLD_HOMING`

该状态的职责是确认回零完成，而不是持续发回零指令。

1. 若从进入该状态起超过 `STEPPER_HOLD_HOME_TIMEOUT_MS`（当前 15 s），进入 `STEPPER_HOLD_FAULT_HOME_TIMEOUT`。
2. 每 `STEPPER_HOLD_POSITION_POLL_MS`（当前 200 ms）请求一次实时位置 `S_CPOS`。
3. 连续 `STEPPER_HOLD_SETTLE_SAMPLES`（当前 2）次收到的位置均在零点容差内，调用 `StartTargetMove()`。
4. 若已经发出查询但 `STEPPER_HOLD_COMMS_TIMEOUT_MS`（当前 600 ms）内没有收到有效回复，进入通信故障。

当前代码以 `AbsoluteFloat(actual_deg) <= tolerance` 判断零点。若电机将接近零点的同一物理位置表示为 `359.9` 度，这个判断会失败。更稳妥的改法是使用 `AbsoluteFloat(WrapError(actual_deg, 0.0f))`，或读取并解析厂商回零状态 `S_OFLAG` 后再确认完成。

### `STEPPER_HOLD_MOVING_TO_TARGET`

该状态已经发送了绝对位置命令，随后只确认是否到位。

1. 若运动超过 `STEPPER_HOLD_MOVE_TIMEOUT_MS`（当前 15 s），在重试次数小于 `STEPPER_HOLD_MAX_RECOVERY_RETRIES`（当前 2）时重新调用 `StartTargetMove()`；耗尽后进入 `STEPPER_HOLD_FAULT_MOVE_TIMEOUT`。
2. 周期查询实际位置，并用 `WrapError(actual_deg, target_deg)` 得到 `-180` 到 `+180` 度的最短角度误差。
3. 连续两次 `abs(error_deg) <= 1.0` 度，进入 `HOLDING`。

### `STEPPER_HOLD_HOLDING`

保持状态仍每 200 ms 查询位置。连续 `STEPPER_HOLD_ERROR_SAMPLES`（当前 3）次发现 `abs(error_deg) > tolerance` 时，若仍有重试次数，调用 `StartTargetMove()`；否则进入 `STEPPER_HOLD_FAULT_POSITION_UNRECOVERABLE`。

`StartTargetMove()` 使用 `EMM_ABSOLUTE_POSITION == 1`。因此外力把电机从 30 度推到 20 度时，重新下达的是“去零点参考下的 30 度”，不是“从 20 度再转 30 度”。正确回零和电机内部位置坐标有效是此行为的前提。

### `STEPPER_HOLD_FAULT`

`EnterFault()` 首次进入时调用 `Emm_V5_Stop_Now()`，记录故障码，并将状态切换为 `FAULT`。之后 `StepperHold_Update()` 立即返回，不再发送新的运动或查询命令。恢复策略目前是复位或重新初始化状态机；若设计运行时恢复，需要单独增加明确的故障清除接口。

## 6. 位置数据如何更新

### 6.1 请求与接收

`RequestPosition(now_ms)` 仅在没有未完成请求、且距离上次轮询达到 200 ms 时发送：

```c
Emm_V5_Read_Sys_Params(STEPPER_HOLD_MOTOR_ADDRESS, S_CPOS);
controller.awaiting_position = true;
```

它不会等待电机回复。回复字节由 DMA 写入 `rxCmd`。USART2 检测到 IDLE 后，中断服务程序：停止本轮 DMA、计算已接收字节数、把 `rxCmd` 复制到 `rxFrame`、置 `rxFrameFlag = true`，然后立即重新启动 DMA 接收。

### 6.2 解析与赋值

每次 `StepperHold_Update()` 的第一步都是 `ConsumePositionResponse()`。该函数短暂关中断，将 `rxFrame` 复制到局部 `frame[8]`，再校验：

- 长度必须为 8；
- 地址必须等于电机地址；
- 命令必须为 `0x36`（`S_CPOS` 回复）；
- 帧尾必须为 `0x6B`。

位置字段为大端 32 位无符号数：

```c
position_raw = frame[3] << 24 | frame[4] << 16 |
               frame[5] << 8  | frame[6];
actual_deg = position_raw * 360.0f / 65536.0f;
if (frame[2] != 0U) actual_deg = -actual_deg;
```

随后更新 `error_deg`、标记 `position_valid`，并清除 `awaiting_position`。因此状态机在本轮判断中使用的是**上一次位置请求的回复**，但该数值描述的是电机回复时的实际位置，而不是发请求瞬间的位置。

## 7. 关键辅助函数

| 函数 | 做什么 | 注意点 |
| --- | --- | --- |
| `Elapsed()` | 用无符号减法比较经过的毫秒数 | 可正确跨越 `HAL_GetTick()` 溢出 |
| `AbsoluteFloat()` | 返回浮点绝对值 | 只用于容差判断 |
| `WrapError()` | 将角度误差规整到 `[-180, 180]` | 防止 `359` 度与 `0` 度被误判为相差 359 度 |
| `EnterState()` | 进入新状态并重置稳定/误差计数 | 同时清除 `awaiting_position` |
| `EnterFault()` | 停电机、记录故障、进入 `FAULT` | 只在首次进入时发送停止 |
| `TargetPulses()` | 将目标角度换算为目标脉冲数 | `30 * 3200 / 360 = 266.67`，四舍五入为 `267` |
| `StartTargetMove()` | 发送固定绝对目标位置命令并进入运动状态 | 不是相对移动命令 |
| `StepperHold_GetInfo()` | 返回只读运行信息指针 | 返回真实内部数据，不是快照 |

## 8. 参数修改指南

所有以下参数在 `Inc/stepper_hold.h`：

| 需求 | 修改项 | 修改原则 |
| --- | --- | --- |
| 改目标角度 | `STEPPER_HOLD_TARGET_DEG` | 必须满足 `0.0 <= target < 360.0` |
| 改一圈脉冲数 | `STEPPER_HOLD_PULSES_PER_REV` | 必须与电机实际细分/参数一致 |
| 改顺逆时针目标侧 | `STEPPER_HOLD_TARGET_DIRECTION` | 先低速实测，再固定方向定义 |
| 改速度 | `STEPPER_HOLD_SPEED_RPM` | 从低值开始，避免机构冲击 |
| 改加速度 | `STEPPER_HOLD_ACCELERATION` | 过高会造成振动、失步或超调 |
| 改到位容差 | `STEPPER_HOLD_POSITION_TOLERANCE` | 不应小于机构重复精度与编码器噪声 |
| 改轮询频率 | `STEPPER_HOLD_POSITION_POLL_MS` | 更快会增加串口占用；通信超时应相应留余量 |
| 改超时/重试 | `*_TIMEOUT_MS`、`*_RETRIES` | 基于实际最长动作时间和安全要求设置 |

不要只修改 `STEPPER_HOLD_TARGET_DEG` 后认为目标一定准确。还必须确认零点、每圈脉冲数、方向定义与机构传动比均正确。

## 9. 一次性标零与正常固件

保存零点需要两次编译/烧录：

1. 将机构手动放在真实机械零点。
2. 将 `STEPPER_HOLD_CALIBRATE_ORIGIN_ON_BOOT` 改为 `1`，编译并烧录；上电后程序把当前位置永久保存为电机单圈零点。
3. 将该宏立即改回 `0`，重新编译并烧录正常固件。
4. 断电后手动转动电机，再上电验证：应先回到保存零点，再走到目标角度。

绝不能长期把该宏保持为 `1`，否则每次上电都会覆写零点。

## 10. 安全修改顺序

1. 先确认改动是“配置变更”还是“状态机逻辑变更”。配置优先只改 `stepper_hold.h`。
2. 更改通信、帧格式或电机协议前，先对照当前 `Emm_V5.c` 和厂商协议，不能只凭猜测修改 `0x36`、`0x6B` 或字节顺序。
3. 每次修改后先在 Keil 完整编译，确认没有编译警告/错误，再生成并烧录 HEX。
4. 电机初次测试使用低速度、机构留有安全空间，并准备断电或急停。
5. 最少验证：标零、断电后手动转动、重上电回零、走目标、外力推偏后的回位、断开 RX 后的通信故障停止。

## 11. 当前验证状态与待改进项

- 当前源文件配置已为正常运行模式：标零开关 `0`、目标 `30.0f`、地址 `1`、一圈 `3200` 脉冲。
- 以前已完成主机侧语法检查；本交接文档生成时没有重新执行 Keil 编译、生成 HEX 或进行实机测试。因此不能声称当前版本已经通过硬件验证。
- `HOMING` 目前通过“位置接近零”推断回零完成；后续可靠性改进应优先评估读取厂商 `S_OFLAG` 的回零状态，而不是只看角度。
- UART 接收使用单帧缓冲和 IDLE 分帧；若将来增加异步电机上报、多电机或更高频数据，应改为可区分命令/响应且可排队的接收机制。

## 12. 给下一位 Codex 的最短定位路线

先读 `Inc/stepper_hold.h` 确认目标与安全参数；再读 `Src/stepper_hold.c` 的 `StepperHold_Update()` 跟状态迁移；需要解释角度值来源时，沿 `RequestPosition()` -> `USART2_IRQHandler()` -> `ConsumePositionResponse()` 跟踪；需要改协议命令时再读 `Src/Emm_V5.c`。
