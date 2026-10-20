#ifndef HOST_PS2_HAL_H
#define HOST_PS2_HAL_H
#include "../stm32f4xx_hal.h"
typedef enum { GPIO_PIN_RESET = 0, GPIO_PIN_SET } GPIO_PinState;
typedef struct { int unused; } GPIO_TypeDef;
typedef struct { uint32_t Pin, Mode, Pull, Speed; } GPIO_InitTypeDef;
typedef struct { volatile uint32_t CTRL, CYCCNT; } HostDwt;
typedef struct { uint32_t DEMCR; } HostCoreDebug;
extern HostDwt host_dwt;
extern HostCoreDebug host_debug;
extern GPIO_TypeDef host_gpio;
extern uint32_t SystemCoreClock;
#define DWT (&host_dwt)
#define CoreDebug (&host_debug)
#define DWT_CTRL_CYCCNTENA_Msk 1U
#define CoreDebug_DEMCR_TRCENA_Msk 1U
void Host_AdvanceCycles(void);
#define __NOP() Host_AdvanceCycles()
#define GPIOE (&host_gpio)
#define GPIO_MODE_OUTPUT_PP 1U
#define GPIO_MODE_INPUT 0U
#define GPIO_PULLUP 1U
#define GPIO_SPEED_FREQ_VERY_HIGH 3U
void HAL_GPIO_Init(GPIO_TypeDef *, GPIO_InitTypeDef *);
void HAL_GPIO_WritePin(GPIO_TypeDef *, uint16_t, GPIO_PinState);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *, uint16_t);
#endif
