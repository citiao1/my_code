#include "drv8701.h"

static uint32_t DRV8701_CommandToCompare(const DRV8701_HandleTypeDef *driver,
                                         int16_t command)
{
    uint32_t period = __HAL_TIM_GET_AUTORELOAD(driver->pwm_timer) + 1U;
    uint32_t magnitude;

    if (command < 0)
    {
        magnitude = (uint32_t)(-(int32_t)command);
    }
    else
    {
        magnitude = (uint32_t)command;
    }

    if (magnitude > DRV8701_COMMAND_MAX)
    {
        magnitude = DRV8701_COMMAND_MAX;
    }

    return (period * magnitude) / DRV8701_COMMAND_MAX;
}

bool DRV8701_IsFaultActive(const DRV8701_HandleTypeDef *driver)
{
    if ((driver == NULL) || (driver->fault_port == NULL))
    {
        return false;
    }

    /* nFAULT is an active-low, open-drain output. */
    return HAL_GPIO_ReadPin(driver->fault_port, driver->fault_pin) == GPIO_PIN_RESET;
}

HAL_StatusTypeDef DRV8701_Init(DRV8701_HandleTypeDef *driver)
{
    if ((driver == NULL) || (driver->pwm_timer == NULL) ||
        (driver->phase_port == NULL))
    {
        return HAL_ERROR;
    }

    driver->awake = false;
    __HAL_TIM_SET_COMPARE(driver->pwm_timer, driver->pwm_channel, 0U);
    HAL_GPIO_WritePin(driver->phase_port, driver->phase_pin, GPIO_PIN_RESET);
    if (driver->sleep_port != NULL)
    {
        HAL_GPIO_WritePin(driver->sleep_port, driver->sleep_pin, GPIO_PIN_RESET);
    }

    return HAL_TIM_PWM_Start(driver->pwm_timer, driver->pwm_channel);
}

HAL_StatusTypeDef DRV8701_Wake(DRV8701_HandleTypeDef *driver)
{
    if (driver == NULL)
    {
        return HAL_ERROR;
    }

    if (!driver->awake)
    {
        if (driver->sleep_port != NULL)
        {
            HAL_GPIO_WritePin(driver->sleep_port, driver->sleep_pin, GPIO_PIN_SET);
            HAL_Delay(2U); /* Datasheet wake-up time is 1 ms. */
        }
        driver->awake = true;
    }

    return DRV8701_IsFaultActive(driver) ? HAL_ERROR : HAL_OK;
}

HAL_StatusTypeDef DRV8701_SetOutput(DRV8701_HandleTypeDef *driver,
                                    int16_t command)
{
    GPIO_PinState phase;

    if ((driver == NULL) || (driver->pwm_timer == NULL) ||
        (driver->phase_port == NULL))
    {
        return HAL_ERROR;
    }

    if (DRV8701_Wake(driver) != HAL_OK)
    {
        __HAL_TIM_SET_COMPARE(driver->pwm_timer, driver->pwm_channel, 0U);
        return HAL_ERROR;
    }

    if (command > DRV8701_COMMAND_MAX)
    {
        command = DRV8701_COMMAND_MAX;
    }
    else if (command < -DRV8701_COMMAND_MAX)
    {
        command = -DRV8701_COMMAND_MAX;
    }

    phase = (command >= 0) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    if (driver->reverse_phase)
    {
        phase = (phase == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET;
    }

    HAL_GPIO_WritePin(driver->phase_port, driver->phase_pin, phase);
    __HAL_TIM_SET_COMPARE(driver->pwm_timer,
                          driver->pwm_channel,
                          DRV8701_CommandToCompare(driver, command));

    return HAL_OK;
}

void DRV8701_Brake(DRV8701_HandleTypeDef *driver)
{
    if ((driver == NULL) || (driver->pwm_timer == NULL))
    {
        return;
    }

    /* Always remove PWM first, including when nFAULT is active. */
    __HAL_TIM_SET_COMPARE(driver->pwm_timer, driver->pwm_channel, 0U);
    (void)DRV8701_Wake(driver);
}

void DRV8701_Coast(DRV8701_HandleTypeDef *driver)
{
    if ((driver == NULL) || (driver->pwm_timer == NULL))
    {
        return;
    }

    __HAL_TIM_SET_COMPARE(driver->pwm_timer, driver->pwm_channel, 0U);
    if (driver->sleep_port != NULL)
    {
        HAL_GPIO_WritePin(driver->sleep_port, driver->sleep_pin, GPIO_PIN_RESET);
    }
    /* Without an exposed nSLEEP pin, EN = 0 can only request braking. */
    driver->awake = false;
}
