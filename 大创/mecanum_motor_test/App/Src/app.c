#include "app.h"

#include "battery.h"
#include "display.h"
#include "ps2_control.h"
#include "vehicle.h"
#include "servo_test.h"

void App_Init(void)
{
  uint8_t vehicle_ready = Vehicle_Init();
  (void)Battery_Init();

  Ps2Control_Init();
  ServoTest_Init();
  Display_Init();
  if (vehicle_ready == 0U)
  {
    Vehicle_Stop();
  }
}

void App_Process(void)
{
  ServoTest_Process();
  Ps2Control_Process();
  Vehicle_Process();
  Battery_Process();
  Display_Process();
}
