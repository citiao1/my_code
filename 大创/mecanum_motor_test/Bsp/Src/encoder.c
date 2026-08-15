#include "encoder.h"

#include "tim.h"

typedef struct
{
  TIM_HandleTypeDef *timer;
  int8_t sign;
  uint32_t last_raw;
  int32_t total;
} EncoderChannel;

static EncoderChannel encoder_channels[MOTOR_COUNT] =
{
  {&htim2, -1, 0U, 0},
  {&htim3,  1, 0U, 0},
  {&htim4, -1, 0U, 0},
  {&htim5,  1, 0U, 0}
};

static int32_t Encoder_GetDelta(MotorId id, uint32_t current)
{
  uint32_t previous = encoder_channels[id].last_raw;

  if ((id == MOTOR_B) || (id == MOTOR_C))
  {
    return (int32_t)(int16_t)((uint16_t)current - (uint16_t)previous);
  }

  return (int32_t)(current - previous);
}

uint8_t Encoder_Init(void)
{
  HAL_StatusTypeDef status = HAL_OK;

  status |= HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
  status |= HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
  status |= HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL);
  status |= HAL_TIM_Encoder_Start(&htim5, TIM_CHANNEL_ALL);

  Encoder_ResetAll();
  return (status == HAL_OK) ? 1U : 0U;
}

void Encoder_Sample(int32_t sample_delta[MOTOR_COUNT])
{
  uint8_t index;
  uint32_t current;
  int32_t delta;

  for (index = 0U; index < MOTOR_COUNT; ++index)
  {
    current = __HAL_TIM_GET_COUNTER(encoder_channels[index].timer);
    delta = Encoder_GetDelta((MotorId)index, current);
    encoder_channels[index].last_raw = current;
    encoder_channels[index].total += delta * encoder_channels[index].sign;
    sample_delta[index] = delta * encoder_channels[index].sign;
  }
}

void Encoder_ResetAll(void)
{
  uint8_t index;

  for (index = 0U; index < MOTOR_COUNT; ++index)
  {
    __HAL_TIM_SET_COUNTER(encoder_channels[index].timer, 0U);
    encoder_channels[index].last_raw = 0U;
    encoder_channels[index].total = 0;
  }
}

int32_t Encoder_Get(MotorId id)
{
  if (id >= MOTOR_COUNT)
  {
    return 0;
  }
  return encoder_channels[id].total;
}

void Encoder_GetAll(int32_t count[MOTOR_COUNT])
{
  uint8_t index;

  for (index = 0U; index < MOTOR_COUNT; ++index)
  {
    count[index] = encoder_channels[index].total;
  }
}
