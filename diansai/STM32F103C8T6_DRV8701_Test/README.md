# STM32F103C8T6 + DRV8701E 测试程序

本例适用于 72 MHz 的 STM32F103C8T6 和采用 PH/EN 控制模式的
DRV8701E。PWM 频率为 20 kHz，上电后持续输出固定的正转 20% 占空比。
需要改变测试占空比时，修改 `Src/main.c` 顶部的
`TEST_MOTOR_COMMAND`：`200` 表示正转 20%，`-200` 表示反转 20%，允许范围
为 -1000 到 +1000。

## 直接使用 Keil 编译和烧录

用 Keil 5 打开：

`MDK-ARM/DRV8701_F103C8.uvprojx`

工程已经配置为 STM32F103C8、ARM Compiler 6、64 KB Flash、20 KB RAM，
并启用了 HEX 输出。默认下载器为 ST-Link，接口为 SWD。

| ST-Link | STM32F103C8T6 |
| --- | --- |
| SWDIO | PA13 / SWDIO |
| SWCLK | PA14 / SWCLK |
| GND | GND |
| 3.3V / VTref | 3.3V |
| NRST（建议连接） | NRST |

烧录步骤：

1. 把开发板 `BOOT0` 接 GND，连接 ST-Link 和开发板。
2. 在 Keil 中按 `F7` 编译。
3. 按 `F8` 下载，或者点击工具栏的 `LOAD` 按钮。
4. 下载完成后复位，上电即持续输出固定 20% PWM。

已验证生成的固件位于：

`MDK-ARM/Objects/DRV8701_F103C8.hex`

如果使用 DAPLink/CMSIS-DAP，在 `Options for Target -> Debug` 中选择
`CMSIS-DAP Debugger`，在 `Utilities` 中勾选 `Use Debug Driver`；SWDIO、
SWCLK、GND、VTref 和 NRST 接法不变。

## STM32 与成品驱动模块接线

| STM32F103C8T6 | DRV8701E 模块功能端 | 作用 |
| --- | --- | --- |
| PA6 / TIM3_CH1 | EN/PWM 或 PWM | 20 kHz 调速信号 |
| PB0 | PH 或 DIR | 方向信号 |
| PB1 | nSLEEP 或 SLEEP（若模块有引出） | 高电平唤醒，低电平高阻 |
| PB10 | nFAULT 或 FAULT（若模块有引出） | 故障输入，低电平有效 |
| PA0 / ADC1_IN0 | SO、CS 或 ISEN（可选） | 经电压限制后的模拟电流采样 |
| GND | GND | 逻辑地和功率地共地 |
| 电机电源正极 | VM、VIN 或 POWER+ | 电机供电 |
| 电机电源负极 | GND 或 POWER- | 功率地 |
| 直流有刷电机两根线 | OUT1/OUT2 或 M+/M- | 电机输出 |

最少必须连接 `PWM`、`DIR` 和 `GND` 三根控制线。简化模块如果已经在板上
处理了 nSLEEP 和 nFAULT，就不需要再从 STM32 接这两根线。驱动代码也允许
把相应的 `sleep_port` 或 `fault_port` 设置为 `NULL`。

如果模块没有引出 nSLEEP，`DRV8701_Coast()` 只能把 PWM 降为 0，此时
DRV8701E 实际处于低侧制动状态，不能由 STM32 主动切成真正的高阻滑行。

注意：有些模块把唤醒端也标成 `EN`，另一些模块把 PWM 输入标成 `EN`。
不能只凭 `EN` 三个字母接线，必须看它在模块原理图中接到 DRV8701E 的
14 脚 EN，还是 13 脚 nSLEEP。当前程序的 PA6 必须接 14 脚 EN/PWM。

## 上电注意事项

- PB10/nFAULT 是开漏输出，必须上拉。程序启用了 STM32 内部上拉，实际使用
  建议再用 10 kohm 电阻上拉到 3.3 V。
- 如果模块有逻辑 `VCC`，应按该模块原理图要求供电。只有模块明确支持
  3.3 V 时才能接 STM32 的 3.3 V。裸 DRV8701 芯片没有 VCC 输入脚。
- STM32、驱动模块和电机电源必须共地。
- 裸芯片 SO 输出可能高于 STM32 ADC 允许的 3.3 V。只有确认模块已经限幅，
  或自行增加分压/钳位后，才能连接 PA0。
- 不能用蓝板的 3.3 V 或 5 V 引脚给电机供电。
- 第一次测试应把车轮架空，并给电机电源设置较小的限流值。
- DRV8701 芯片的 VM 工作范围是 5.9-45 V，但成品模块还受 MOSFET、电容和
  PCB 限制，实际输入电压必须服从模块自己的额定值。

## 裸 DRV8701E 芯片接线

DRV8701 只是栅极驱动器，不是完整的电机功率模块。裸芯片还需要 4 个外置
N 沟道 MOSFET、电流采样电阻、充电泵电容、VM 去耦、VREF/IDRIVE 配置和
完整的功率回路。电机不能直接接 GHx、GLx 或 SHx。

| DRV8701E 引脚 | 应连接到 |
| --- | --- |
| PH（15 脚） | STM32 PB0 |
| EN（14 脚） | STM32 PA6 / TIM3_CH1 |
| nSLEEP（13 脚） | STM32 PB1 |
| nFAULT（9 脚） | STM32 PB10，并用 10 kohm 上拉到 3.3 V |
| SO（11 脚） | 先分压/钳位到 0-3.3 V，再可选接 STM32 PA0 ADC 输入 |
| VM（1 脚） | 电机电源正极，就近接 0.1 uF 和至少 10 uF 去耦电容 |
| GND（5、16 脚和散热焊盘） | 公共地 |
| DVDD（8 脚） | 芯片内部 3.3 V 稳压输出，仅接 1 uF 到地，不能接外部 3.3 V |
| AVDD（7 脚） | 芯片内部 4.8 V 稳压输出，仅接 1 uF 到地，不能外部供电 |
| VCP（2 脚） | 经 1 uF 电容接 VM |
| CPH/CPL（3/4 脚） | 两脚之间接 0.1 uF X7R 电容 |
| IDRIVE（12 脚） | 按 MOSFET 栅极电荷选择配置电阻 |
| VREF（6 脚） | 电流调节参考电压，不能悬空 |
| GH1/GH2 | 两个高侧 N-MOS 栅极 |
| GL1/GL2 | 两个低侧 N-MOS 栅极 |
| SH1/SH2 | 两个半桥中点，也是电机两端 |
| SP/SN | 采样电阻两端 |

## 放入 STM32CubeMX 工程

1. 选择 `STM32F103C8Tx`，把系统时钟配置为 72 MHz。
2. PA6 选择 `TIM3_CH1 -> PWM Generation CH1`，PWM 配成 20 kHz。
3. PB0、PB1 配成推挽输出，PB10 配成上拉输入。
4. 把 `Inc/drv8701.h` 和 `Src/drv8701.c` 加入工程。
5. 可以直接使用本目录的 `Src/main.c`，也可以只把驱动调用复制到 CubeMX
   生成文件的 USER CODE 区域。

`DRV8701_SetOutput(&motor, value)` 的输入范围是 -1000 到 +1000，正负号控制
方向，绝对值控制占空比。`DRV8701_Brake()` 令 EN 为低电平，是低侧制动；
`DRV8701_Coast()` 令 nSLEEP 为低电平，是 H 桥高阻滑行。

本程序只适用于 DRV8701E（PH/EN）。DRV8701P 使用 IN1/IN2 真值表控制，
输出函数不同，不能直接使用本程序。
