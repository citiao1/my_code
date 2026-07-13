#include "vehicle_gray.h"

#include "adc.h"
#include "main.h"

static void SelectChannel(uint8_t channel)
{
  HAL_GPIO_WritePin(GRAY_SEL_A_GPIO_Port, GRAY_SEL_A_Pin,
                    (channel & 0x01U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GRAY_SEL_B_GPIO_Port, GRAY_SEL_B_Pin,
                    (channel & 0x02U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GRAY_SEL_C_GPIO_Port, GRAY_SEL_C_Pin,
                    (channel & 0x04U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void VehicleGray_Init(void)
{
  SelectChannel(0U);
}

uint16_t VehicleGray_ReadChannel(uint8_t channel)
{
  volatile uint32_t settle;
  uint16_t value = 0U;

  if (channel >= GRAY_SENSOR_CHANNELS) return 0U;
  SelectChannel(channel);
  for (settle = 0U; settle < 64U; ++settle) __NOP();

  if (HAL_ADC_Start(&hadc2) != HAL_OK) return 0U;
  if (HAL_ADC_PollForConversion(&hadc2, 2U) == HAL_OK)
  {
    value = (uint16_t)HAL_ADC_GetValue(&hadc2);
  }
  HAL_ADC_Stop(&hadc2);
  return value;
}

void VehicleGray_ReadAll(uint16_t values[GRAY_SENSOR_CHANNELS])
{
  uint8_t channel;
  if (values == 0) return;
  for (channel = 0U; channel < GRAY_SENSOR_CHANNELS; ++channel)
  {
    values[channel] = VehicleGray_ReadChannel(channel);
  }
}
