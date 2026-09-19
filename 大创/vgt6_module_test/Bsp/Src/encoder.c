#include "encoder.h"

#include <string.h>

#include "main.h"
#include "stm32f4xx_hal.h"

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim4;

static EncoderSnapshot encoder_snapshot;
static uint32_t previous_tim2;
static uint16_t previous_tim1;
static uint16_t previous_tim4;
static volatile int32_t encoder3_isr_count;
static int32_t previous_encoder3;
static volatile uint8_t encoder3_previous_state;

static const int8_t quadrature_table[16] =
{
   0, -1,  1,  0,
   1,  0,  0, -1,
  -1,  0,  0,  1,
   0,  1, -1,  0
};

static uint8_t Encoder3_ReadState(void)
{
  uint8_t state = 0U;

  if (HAL_GPIO_ReadPin(ENCODER3_A_GPIO_Port, ENCODER3_A_Pin) != GPIO_PIN_RESET)
  {
    state |= 2U;
  }
  if (HAL_GPIO_ReadPin(ENCODER3_B_GPIO_Port, ENCODER3_B_Pin) != GPIO_PIN_RESET)
  {
    state |= 1U;
  }
  return state;
}

uint8_t Encoder_Init(void)
{
  memset(&encoder_snapshot, 0, sizeof(encoder_snapshot));
  encoder_snapshot.counts_per_revolution = 380U;
  encoder3_isr_count = 0;
  encoder3_previous_state = Encoder3_ReadState();

  if ((HAL_TIM_Encoder_Start(&htim1, TIM_CHANNEL_ALL) != HAL_OK) ||
      (HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL) != HAL_OK) ||
      (HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL) != HAL_OK))
  {
    return 0U;
  }

  __HAL_TIM_SET_COUNTER(&htim1, 0U);
  __HAL_TIM_SET_COUNTER(&htim2, 0U);
  __HAL_TIM_SET_COUNTER(&htim4, 0U);
  previous_tim1 = 0U;
  previous_tim2 = 0U;
  previous_tim4 = 0U;
  previous_encoder3 = 0;
  encoder_snapshot.ready = 1U;
  return 1U;
}

void Encoder_Process(uint32_t elapsed_ms)
{
  uint16_t current_tim1;
  uint32_t current_tim2;
  uint16_t current_tim4;
  int32_t current_encoder3;
  uint32_t index;

  if ((encoder_snapshot.ready == 0U) || (elapsed_ms == 0U))
  {
    return;
  }

  current_tim1 = (uint16_t)__HAL_TIM_GET_COUNTER(&htim1);
  current_tim2 = __HAL_TIM_GET_COUNTER(&htim2);
  current_tim4 = (uint16_t)__HAL_TIM_GET_COUNTER(&htim4);
  __disable_irq();
  current_encoder3 = encoder3_isr_count;
  __enable_irq();

  encoder_snapshot.delta[0] = (int32_t)(int16_t)(current_tim1 - previous_tim1);
  encoder_snapshot.delta[1] = (int32_t)(current_tim2 - previous_tim2);
  encoder_snapshot.delta[2] = current_encoder3 - previous_encoder3;
  encoder_snapshot.delta[3] = (int32_t)(int16_t)(current_tim4 - previous_tim4);
  previous_tim1 = current_tim1;
  previous_tim2 = current_tim2;
  previous_encoder3 = current_encoder3;
  previous_tim4 = current_tim4;

  for (index = 0U; index < ENCODER_CHANNEL_COUNT; ++index)
  {
    encoder_snapshot.total[index] += encoder_snapshot.delta[index];
    encoder_snapshot.rpm_x10[index] =
        (int32_t)(((int64_t)encoder_snapshot.delta[index] * 600000LL) /
                  ((int64_t)encoder_snapshot.counts_per_revolution *
                   (int64_t)elapsed_ms));
  }
}

void Encoder_Zero(void)
{
  uint32_t index;

  for (index = 0U; index < ENCODER_CHANNEL_COUNT; ++index)
  {
    encoder_snapshot.total[index] = 0;
    encoder_snapshot.delta[index] = 0;
    encoder_snapshot.rpm_x10[index] = 0;
  }
}

uint8_t Encoder_SetCountsPerRevolution(uint16_t counts_per_revolution)
{
  if ((counts_per_revolution < 4U) || (counts_per_revolution > 20000U))
  {
    return 0U;
  }
  encoder_snapshot.counts_per_revolution = counts_per_revolution;
  return 1U;
}

EncoderSnapshot Encoder_GetSnapshot(void)
{
  return encoder_snapshot;
}

void Encoder_ExtiCallback(uint16_t gpio_pin)
{
  uint8_t current_state;
  uint8_t transition;

  if ((gpio_pin != ENCODER3_A_Pin) && (gpio_pin != ENCODER3_B_Pin))
  {
    return;
  }

  current_state = Encoder3_ReadState();
  transition = (uint8_t)((encoder3_previous_state << 2U) | current_state);
  encoder3_isr_count += quadrature_table[transition & 0x0FU];
  encoder3_previous_state = current_state;
}

void HAL_GPIO_EXTI_Callback(uint16_t gpio_pin)
{
  Encoder_ExtiCallback(gpio_pin);
}
