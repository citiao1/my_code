#include "app.h"

#include "display.h"
#include "servo_test.h"
#include "servo_link.h"

void App_Init(void)
{
  ServoTest_Init();
  ServoLink_Init();
  Display_Init();
}

void App_Process(void)
{
  ServoLink_Process();
  ServoTest_Process();
  Display_Process();
}
