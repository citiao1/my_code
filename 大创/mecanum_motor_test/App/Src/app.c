#include "app.h"

#include "display.h"
#include "servo_test.h"

void App_Init(void)
{
  ServoTest_Init();
  Display_Init();
}

void App_Process(void)
{
  ServoTest_Process();
  Display_Process();
}
