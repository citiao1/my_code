#ifndef DRV8701_H
#define DRV8701_H

#include <stdbool.h>
#include <stdint.h>

#include "stm32f1xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRV8701_COMMAND_MAX 1000

typedef struct
{
    TIM_HandleTypeDef *pwm_timer;
    uint32_t pwm_channel;

    GPIO_TypeDef *phase_port;
    GPIO_TypeDef *sleep_port;
    GPIO_TypeDef *fault_port;

    uint16_t phase_pin;
    uint16_t sleep_pin;
    uint16_t fault_pin;

    bool reverse_phase;
    bool awake;
} DRV8701_HandleTypeDef;

HAL_StatusTypeDef DRV8701_Init(DRV8701_HandleTypeDef *driver);
HAL_StatusTypeDef DRV8701_Wake(DRV8701_HandleTypeDef *driver);
HAL_StatusTypeDef DRV8701_SetOutput(DRV8701_HandleTypeDef *driver,
                                    int16_t command);
void DRV8701_Brake(DRV8701_HandleTypeDef *driver);
void DRV8701_Coast(DRV8701_HandleTypeDef *driver);
bool DRV8701_IsFaultActive(const DRV8701_HandleTypeDef *driver);

#ifdef __cplusplus
}
#endif

#endif
