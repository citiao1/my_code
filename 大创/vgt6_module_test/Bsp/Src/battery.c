#include "battery.h"

#include <string.h>

#include "stm32f4xx_hal.h"

extern ADC_HandleTypeDef hadc1;

#define BATTERY_SAMPLE_PERIOD_MS 100U
#define BATTERY_ADC_FULL_SCALE   4095U
#define BATTERY_VDDA_MV          3300U
#define BATTERY_DIVIDER_TOP_KOHM 100U
#define BATTERY_DIVIDER_BOT_KOHM 22U

static BatterySnapshot battery_snapshot;
static uint32_t battery_tick;
static uint32_t filtered_mv;

void Battery_Init(void)
{
  memset(&battery_snapshot, 0, sizeof(battery_snapshot));
  filtered_mv = 0U;
  battery_tick = HAL_GetTick() - BATTERY_SAMPLE_PERIOD_MS;
  Battery_Process();
}

void Battery_Process(void)
{
  uint32_t now;
  uint32_t raw;
  uint32_t voltage_mv;

  now = HAL_GetTick();
  if ((now - battery_tick) < BATTERY_SAMPLE_PERIOD_MS)
  {
    return;
  }
  battery_tick = now;

  if ((HAL_ADC_Start(&hadc1) != HAL_OK) ||
      (HAL_ADC_PollForConversion(&hadc1, 5U) != HAL_OK))
  {
    ++battery_snapshot.read_failures;
    (void)HAL_ADC_Stop(&hadc1);
    return;
  }

  raw = HAL_ADC_GetValue(&hadc1);
  (void)HAL_ADC_Stop(&hadc1);
  voltage_mv = (raw * BATTERY_VDDA_MV *
                (BATTERY_DIVIDER_TOP_KOHM + BATTERY_DIVIDER_BOT_KOHM)) /
               (BATTERY_ADC_FULL_SCALE * BATTERY_DIVIDER_BOT_KOHM);
  if (battery_snapshot.ready == 0U)
  {
    filtered_mv = voltage_mv;
  }
  else
  {
    filtered_mv = ((filtered_mv * 7U) + voltage_mv) / 8U;
  }
  battery_snapshot.raw_adc = (uint16_t)raw;
  battery_snapshot.voltage_mv = (uint16_t)filtered_mv;
  battery_snapshot.ready = 1U;
}

BatterySnapshot Battery_GetSnapshot(void)
{
  return battery_snapshot;
}
