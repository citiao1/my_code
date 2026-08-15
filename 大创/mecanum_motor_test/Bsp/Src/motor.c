#include "motor.h"

#include "main.h"
#include "tim.h"

typedef struct
{
  TIM_HandleTypeDef *input1_timer;
  uint32_t input1_channel;
  TIM_HandleTypeDef *input2_timer;
  uint32_t input2_channel;
  int8_t drive_sign;
} MotorChannel;

static const MotorChannel motor_channels[MOTOR_COUNT] =
{
  {&htim10, TIM_CHANNEL_1, &htim11, TIM_CHANNEL_1, -1},
  {&htim9,  TIM_CHANNEL_1, &htim9,  TIM_CHANNEL_2, -1},
  {&htim1,  TIM_CHANNEL_1, &htim1,  TIM_CHANNEL_2,  1},
  {&htim1,  TIM_CHANNEL_3, &htim1,  TIM_CHANNEL_4,  1}
};

static int16_t motor_output[MOTOR_COUNT];

static int16_t Motor_Limit(int16_t value)
{
  if (value > 100)
  {
    return 100;
  }
  if (value < -100)
  {
    return -100;
  }
  return value;
}

static void Motor_SetCompare(const MotorChannel *channel, uint32_t input1, uint32_t input2)
{
  __HAL_TIM_SET_COMPARE(channel->input1_timer, channel->input1_channel, input1);
  __HAL_TIM_SET_COMPARE(channel->input2_timer, channel->input2_channel, input2);
}

uint8_t Motor_Init(void)
{
  HAL_StatusTypeDef status = HAL_OK;

  status |= HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  status |= HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
  status |= HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
  status |= HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
  status |= HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_1);
  status |= HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_2);
  status |= HAL_TIM_PWM_Start(&htim10, TIM_CHANNEL_1);
  status |= HAL_TIM_PWM_Start(&htim11, TIM_CHANNEL_1);

  Motor_StopAll();
  return (status == HAL_OK) ? 1U : 0U;
}

uint8_t Motor_IsEnabled(void)
{
  return (HAL_GPIO_ReadPin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin) == GPIO_PIN_SET) ? 1U : 0U;
}

void Motor_Set(MotorId id, int16_t percent)
{
  const MotorChannel *channel;
  uint32_t period;
  uint32_t drive;
  int16_t hardware_output;

  if (id >= MOTOR_COUNT)
  {
    return;
  }

  percent = Motor_Limit(percent);
  motor_output[id] = percent;
  channel = &motor_channels[id];

  if (percent == 0)
  {
    Motor_SetCompare(channel, 0U, 0U);
    return;
  }

  hardware_output = percent * channel->drive_sign;
  period = __HAL_TIM_GET_AUTORELOAD(channel->input1_timer) + 1U;
  drive = (period * (uint32_t)((hardware_output > 0) ? hardware_output : -hardware_output)) / 100U;

  if (hardware_output > 0)
  {
    Motor_SetCompare(channel, period - drive, period);
    return;
  }

  Motor_SetCompare(channel, period, period - drive);
}

void Motor_SetAll(int16_t a, int16_t b, int16_t c, int16_t d)
{
  Motor_Set(MOTOR_A, a);
  Motor_Set(MOTOR_B, b);
  Motor_Set(MOTOR_C, c);
  Motor_Set(MOTOR_D, d);
}

void Motor_StopAll(void)
{
  Motor_SetAll(0, 0, 0, 0);
}

int16_t Motor_GetOutput(MotorId id)
{
  if (id >= MOTOR_COUNT)
  {
    return 0;
  }
  return motor_output[id];
}

void Motor_GetAll(int16_t output[MOTOR_COUNT])
{
  uint8_t index;

  for (index = 0U; index < MOTOR_COUNT; ++index)
  {
    output[index] = motor_output[index];
  }
}
