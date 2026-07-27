# 双电机同步回零与按键相对转 30 度

## 程序结构

当前版本删除了原来的 `stepper_hold`、`stepper_display` 和
`stepper_uart_rx` 三层封装，主要逻辑只有以下几处：

- `Src/main.c`：上电回零、PE0 按键、两台电机同步运动、位置帧解析和 OLED 内容。
- `Src/usart.c`：USART2 初始化、固定 8 字节 DMA 缓冲区、`HAL_UART_RxCpltCallback()` 和 `HAL_UART_ErrorCallback()`。
- `Src/oled.c`、`Src/font.c`：完整采用 `stm32_hal_ssd1306` 的显存、字体和绘图逻辑，只把最底层 I2C 发送改成适合当前接线的软件 SPI。
- `Src/Emm_V5.c`：厂商电机协议，不改变其命令接口。

主循环不会因为位置超时进入永久故障状态。位置接收异常只增加 OLED 上的
`ERR`，随后继续查询；按键控制不会因此被锁死。

## 实际动作

1. 上电等待 500 ms，依次使能地址 `1`、`2`。
2. 依次向地址 `1`、`2` 发送单圈就近回零命令。
3. 等待 3 秒后开始检测 PE0 用户按键。
4. 每次稳定按下 PE0，分别给地址 `1`、`2` 装载 267 脉冲命令。
5. 两条命令都使用 `raF=0`，即相对上一次输入目标继续顺时针增加 30 度。
6. 最后向广播地址 `0` 发送同步启动命令。

所以连续按键的目标是 30、60、90 度，而不是重复回到绝对 30 度。

## OLED 接线与显示

| OLED 信号 | F407 引脚 |
| --- | --- |
| DC | PD11 |
| RST | PD12 |
| DATA | PD13 |
| CLK | PD14 |
| VCC | 3.3 V |
| GND | GND |

屏幕内容：

- `M1 ANG`、`M2 ANG`：两台电机的实时位置。
- `KEY`：检测到的按键次数。
- `CMD`：已经下发的双电机同步运动次数。正常情况下应与 `KEY` 同步增加。
- `RX`：当前是否正在等待位置回复。
- `V1`、`V2`：是否至少收到过对应电机的有效位置帧。
- `ERR`：DMA错误、位置查询超时或帧格式错误的累计次数。

## 串口接收

当前 HAL 是 `STM32Cube FW_F4 V1.24.2`，没有
`HAL_UARTEx_ReceiveToIdle_DMA()`。电机的 `S_CPOS` 回复固定为 8 字节，
所以每次查询前直接调用：

```c
HAL_UART_Receive_DMA(&huart2, uart2_position_frame, 8);
```

收满 8 字节后，HAL 自动调用 `HAL_UART_RxCpltCallback()`。因此不需要
手写 IDLE 中断，也不需要为了这个功能重新生成 CubeMX 工程。

## 接线

| F407 | 电机总线 |
| --- | --- |
| PD5 / USART2_TX | 两台电机 TTL_RX |
| PD6 / USART2_RX | 两台电机 TTL_TX |
| GND | 两台电机 GND |
| PE0 | 板载用户按键 |

电机单独供电并与开发板共地。连接 PD6 前确认电机 TTL_TX 为 3.3 V。
两台电机必须分别设置地址 `1` 和 `2`，并关闭主动周期上报，避免两个地址
同时向同一根 RX 线发送数据。

## 零点校准

正常运行时，`Src/main.c` 顶部的：

```c
#define CALIBRATE_MOTOR_1_ON_BOOT  0
#define CALIBRATE_MOTOR_2_ON_BOOT  0
```

必须保持为 `0`。需要重新保存某台电机零点时，把机构手动放到真实零点，
只将对应宏临时改为 `1`，编译烧录并上电一次；随后立即恢复为 `0`，重新
编译烧录正常固件。不要长期保持为 `1`。
