#include "battery.h"

#include "adc.h"

#define BATTERY_SAMPLE_PERIOD_MS  100U
#define BATTERY_ADC_FULL_SCALE   4095UL
#define BATTERY_REFERENCE_MV     3300UL
#define BATTERY_DIVIDER_RATIO      11UL

static uint32_t battery_tick;
static uint16_t battery_raw;
static uint16_t battery_mv;
static uint8_t battery_ready;

static uint16_t Battery_ConvertMillivolts(uint16_t raw)
{
  uint32_t millivolts = (uint32_t)raw * BATTERY_REFERENCE_MV * BATTERY_DIVIDER_RATIO;
  return (uint16_t)((millivolts + BATTERY_ADC_FULL_SCALE / 2UL) / BATTERY_ADC_FULL_SCALE);
}

uint8_t Battery_Init(void)
{
  battery_tick = HAL_GetTick() - BATTERY_SAMPLE_PERIOD_MS;
  battery_raw = 0U;
  battery_mv = 0U;
  battery_ready = 0U;
  Battery_Process();
  return battery_ready;
}

void Battery_Process(void)
{
  uint32_t now = HAL_GetTick();
  uint16_t sample_mv;

  if ((now - battery_tick) < BATTERY_SAMPLE_PERIOD_MS)
  {
    return;
  }
  battery_tick = now;

  if (HAL_ADC_Start(&hadc1) != HAL_OK)
  {
    battery_ready = 0U;
    return;
  }
  if (HAL_ADC_PollForConversion(&hadc1, 1U) != HAL_OK)
  {
    (void)HAL_ADC_Stop(&hadc1);
    battery_ready = 0U;
    return;
  }

  battery_raw = (uint16_t)HAL_ADC_GetValue(&hadc1);
  (void)HAL_ADC_Stop(&hadc1);
  sample_mv = Battery_ConvertMillivolts(battery_raw);
  battery_mv = (battery_ready == 0U) ? sample_mv :
               (uint16_t)(((uint32_t)battery_mv * 7UL + sample_mv) / 8UL);
  battery_ready = 1U;
}

uint8_t Battery_IsReady(void)
{
  return battery_ready;
}

uint16_t Battery_GetMillivolts(void)
{
  return battery_mv;
}

uint16_t Battery_GetRaw(void)
{
  return battery_raw;
}
