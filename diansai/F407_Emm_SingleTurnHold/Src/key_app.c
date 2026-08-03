#include "key_app.h"
#include "gpio.h"
#include "motor_app.h"

uint32_t key_count = 0UL;

void key_pro(void)
{
  static uint32_t key_time = 0UL;

  if ((uint32_t)(HAL_GetTick() - key_time) < 10UL)
  {
    return;
  }
  key_time = HAL_GetTick();

  if (key_scanf() == 1U)
  {
    ++key_count;
    motor_add_30();
  }
}
