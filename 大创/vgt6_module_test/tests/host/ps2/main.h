#ifndef HOST_PS2_MAIN_H
#define HOST_PS2_MAIN_H
#include "stm32f4xx_hal.h"
#define PS2_CLK_Pin (1U << 8)
#define PS2_SEL_Pin (1U << 10)
#define PS2_CMD_Pin (1U << 12)
#define PS2_DAT_Pin (1U << 15)
#define PS2_CLK_GPIO_Port GPIOE
#define PS2_SEL_GPIO_Port GPIOE
#define PS2_CMD_GPIO_Port GPIOE
#define PS2_DAT_GPIO_Port GPIOE
#endif
