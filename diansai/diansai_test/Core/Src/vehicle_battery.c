#include "vehicle_battery.h"

#include "adc.h"
#include "vehicle_config.h"
#include "vehicle_internal.h"

void VehicleBattery_Update(void)
{
  uint32_t sum = 0U;
  uint16_t sample;

  for (sample = 0U; sample < 16U; ++sample)
  {
    if (HAL_ADC_Start(&hadc1) != HAL_OK) return;
    if (HAL_ADC_PollForConversion(&hadc1, 2U) != HAL_OK)
    {
      HAL_ADC_Stop(&hadc1);
      return;
    }
    sum += HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
  }

  state.battery_raw = (uint16_t)(sum / 16U);
  {
    float voltage = (float)state.battery_raw * ADC_REFERENCE_VOLTAGE *
                    BATTERY_DIVIDER_RATIO / ADC_FULL_SCALE;
    state.battery_voltage = state.battery_voltage == 0.0f ? voltage :
                            0.85f * state.battery_voltage + 0.15f * voltage;
  }
}
