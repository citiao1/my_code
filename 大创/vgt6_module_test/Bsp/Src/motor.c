#include "motor.h"

#include "main.h"
#include "stm32f4xx_hal.h"

extern TIM_HandleTypeDef htim8;

typedef struct
{
  GPIO_TypeDef *dir_port;
  uint16_t dir_pin;
  GPIO_TypeDef *brake_port;
  uint16_t brake_pin;
  uint32_t timer_channel;
} MotorHardware;

static const MotorHardware motor_hardware[MOTOR_CHANNEL_COUNT] =
{
  {MOTOR1_DIR_GPIO_Port, MOTOR1_DIR_Pin,
   MOTOR1_BRAKE_GPIO_Port, MOTOR1_BRAKE_Pin, TIM_CHANNEL_1},
  {MOTOR2_DIR_GPIO_Port, MOTOR2_DIR_Pin,
   MOTOR2_BRAKE_GPIO_Port, MOTOR2_BRAKE_Pin, TIM_CHANNEL_2},
  {MOTOR3_DIR_GPIO_Port, MOTOR3_DIR_Pin,
   MOTOR3_BRAKE_GPIO_Port, MOTOR3_BRAKE_Pin, TIM_CHANNEL_3},
  {MOTOR4_DIR_GPIO_Port, MOTOR4_DIR_Pin,
   MOTOR4_BRAKE_GPIO_Port, MOTOR4_BRAKE_Pin, TIM_CHANNEL_4}
};

static uint8_t motor_ready;

static uint8_t Motor_IsValid(MotorChannel channel)
{
  return ((uint32_t)channel < (uint32_t)MOTOR_CHANNEL_COUNT) ? 1U : 0U;
}

static void Motor_SetCompare(MotorChannel channel, uint32_t compare)
{
  __HAL_TIM_SET_COMPARE(&htim8, motor_hardware[channel].timer_channel,
                        compare);
}

uint8_t Motor_Init(void)
{
  uint32_t index;

  motor_ready = 0U;
  for (index = 0U; index < (uint32_t)MOTOR_CHANNEL_COUNT; ++index)
  {
    HAL_GPIO_WritePin(motor_hardware[index].brake_port,
                      motor_hardware[index].brake_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(motor_hardware[index].dir_port,
                      motor_hardware[index].dir_pin, GPIO_PIN_RESET);
    Motor_SetCompare((MotorChannel)index, 0U);
  }

  if ((HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1) != HAL_OK) ||
      (HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_2) != HAL_OK) ||
      (HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_3) != HAL_OK) ||
      (HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_4) != HAL_OK))
  {
    Motor_StopAll();
    return 0U;
  }

  Motor_StopAll();
  motor_ready = 1U;
  return 1U;
}

void Motor_Stop(MotorChannel channel)
{
  if (Motor_IsValid(channel) == 0U)
  {
    return;
  }
  /* BM50 uses low-level PWM duty as the run command. In PWM2 mode CCR=0
   * leaves the pin high (stop), then BTAK low applies the emergency brake. */
  Motor_SetCompare(channel, 0U);
  HAL_GPIO_WritePin(motor_hardware[channel].brake_port,
                    motor_hardware[channel].brake_pin, GPIO_PIN_RESET);
}

void Motor_StopAll(void)
{
  uint32_t index;

  for (index = 0U; index < (uint32_t)MOTOR_CHANNEL_COUNT; ++index)
  {
    Motor_Stop((MotorChannel)index);
  }
}

uint8_t Motor_Run(MotorChannel channel, int8_t direction,
                  uint8_t duty_percent)
{
  uint32_t period_ticks;
  uint32_t compare;

  if ((motor_ready == 0U) || (Motor_IsValid(channel) == 0U) ||
      (duty_percent == 0U) || (duty_percent > 100U))
  {
    return 0U;
  }

  period_ticks = __HAL_TIM_GET_AUTORELOAD(&htim8) + 1U;
  compare = (period_ticks * (uint32_t)duty_percent) / 100U;
  HAL_GPIO_WritePin(motor_hardware[channel].dir_port,
                    motor_hardware[channel].dir_pin,
                    (direction >= 0) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(motor_hardware[channel].brake_port,
                    motor_hardware[channel].brake_pin, GPIO_PIN_SET);
  Motor_SetCompare(channel, compare);
  return 1U;
}

uint8_t Motor_Recover(MotorChannel channel)
{
  if ((motor_ready == 0U) || (Motor_IsValid(channel) == 0U))
  {
    return 0U;
  }

  Motor_SetCompare(channel, 0U);
  HAL_GPIO_WritePin(motor_hardware[channel].brake_port,
                    motor_hardware[channel].brake_pin, GPIO_PIN_SET);
  HAL_Delay(10U);
  HAL_GPIO_WritePin(motor_hardware[channel].brake_port,
                    motor_hardware[channel].brake_pin, GPIO_PIN_RESET);
  HAL_Delay(10U);
  HAL_GPIO_WritePin(motor_hardware[channel].brake_port,
                    motor_hardware[channel].brake_pin, GPIO_PIN_SET);
  HAL_Delay(10U);
  Motor_Stop(channel);
  return 1U;
}
