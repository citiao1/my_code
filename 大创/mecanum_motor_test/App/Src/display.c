#include "display.h"
#include "app.h"
#include "oled.h"
#include "servo_test.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>

#define DISPLAY_PERIOD_MS 100U
static uint32_t display_tick;

static void Display_PrintLine(uint8_t y, char *text)
{
  OLED_PrintASCIIString(0U, y, text, &afont12x6, OLED_COLOR_NORMAL);
}

void Display_Init(void)
{
  OLED_Init();
  display_tick = HAL_GetTick() - DISPLAY_PERIOD_MS;
  Display_Process();
}

void Display_Process(void)
{
  char line[24];
  uint32_t now = HAL_GetTick();
  ServoTestSnapshot servo;
  if ((now - display_tick) < DISPLAY_PERIOD_MS) return;
  display_tick = now;
  servo = ServoTest_GetSnapshot();
  OLED_NewFrame();
  Display_PrintLine(0U, APP_FIRMWARE_VERSION);
  (void)snprintf(line, sizeof(line), "KEY:%u PRESS:%05lu",
                 (unsigned)servo.key_down, (unsigned long)(servo.presses % 100000U));
  Display_PrintLine(16U, line);
  (void)snprintf(line, sizeof(line), "S:%c CMD:%c OK:%04lu", servo.status,
                 servo.last_command, (unsigned long)(servo.commands % 10000U));
  Display_PrintLine(32U, line);
  (void)snprintf(line, sizeof(line), "UP:%06lu ERR:%03lu",
                 (unsigned long)((now / 1000U) % 1000000U),
                 (unsigned long)(servo.errors % 1000U));
  Display_PrintLine(48U, line);
  OLED_ShowFrame();
}
