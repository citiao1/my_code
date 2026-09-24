#include "app.h"

#include "battery.h"
#include "display.h"
#include "ps2_control.h"
#include "vehicle.h"

void App_Init(void)
{
  uint8_t vehicle_ready = Vehicle_Init();
  (void)Battery_Init();

  Ps2Control_Init();
  Display_Init();
  if (vehicle_ready == 0U)
  {
    Vehicle_Stop();
  }
}

void App_Process(void)
{
  Ps2Control_Process();
  Vehicle_Process();
  Battery_Process();
  Display_Process();
}
