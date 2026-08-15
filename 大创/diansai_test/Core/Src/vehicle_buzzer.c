#include "vehicle_buzzer.h"

#include "main.h"

static uint32_t buzzer_deadline_ms;
static uint8_t buzzer_pulse_active;

void VehicleBuzzer_Init(void)
{
  buzzer_pulse_active = 0U;
  VehicleBuzzer_Set(0U);
}

void VehicleBuzzer_Set(uint8_t enabled)
{
  HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin,
                    enabled ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void VehicleBuzzer_Beep(uint32_t duration_ms)
{
  if (duration_ms == 0U)
  {
    buzzer_pulse_active = 0U;
    VehicleBuzzer_Set(0U);
    return;
  }
  buzzer_deadline_ms = HAL_GetTick() + duration_ms;
  buzzer_pulse_active = 1U;
  VehicleBuzzer_Set(1U);
}

void VehicleBuzzer_Update(uint32_t now)
{
  if (buzzer_pulse_active && (int32_t)(now - buzzer_deadline_ms) >= 0)
  {
    buzzer_pulse_active = 0U;
    VehicleBuzzer_Set(0U);
  }
}
