#include "display.h"

#include <stdio.h>

#include "battery.h"
#include "gyro.h"
#include "motor.h"
#include "oled.h"
#include "speed_control.h"
#include "stm32f4xx_hal.h"
#include "vehicle.h"

#define DISPLAY_PERIOD_MS 250U

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
  SpeedControlSnapshot speed;
  GyroSnapshot gyro;
  char gyro_status;

  if ((now - display_tick) < DISPLAY_PERIOD_MS)
  {
    return;
  }
  display_tick = now;
  speed = SpeedControl_GetSnapshot();
  gyro = Gyro_GetSnapshot();
  gyro_status = gyro.ready ? 'R' : (gyro.calibrating ? 'C' : 'E');

  OLED_NewFrame();
  (void)snprintf(line, sizeof(line), "%-4s SET:%3u RPM", Vehicle_GetModeText(), Vehicle_GetSpeed());
  Display_PrintLine(0U, line);
  (void)snprintf(line, sizeof(line), "A:%4d B:%4d", speed.rpm[MOTOR_A], speed.rpm[MOTOR_B]);
  Display_PrintLine(16U, line);
  (void)snprintf(line, sizeof(line), "C:%4d D:%4d", speed.rpm[MOTOR_C], speed.rpm[MOTOR_D]);
  Display_PrintLine(32U, line);
  (void)snprintf(line, sizeof(line), "BAT:%2u.%02uV EN:%u G:%c",
                 Battery_GetMillivolts() / 1000U,
                 (Battery_GetMillivolts() % 1000U) / 10U,
                 Motor_IsEnabled(), gyro_status);
  Display_PrintLine(48U, line);
  OLED_ShowFrame();
}
