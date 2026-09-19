#include "display.h"

#include <stdio.h>

#include "battery.h"
#include "chassis_test.h"
#include "encoder.h"
#include "font.h"
#include "gyro.h"
#include "oled.h"
#include "serial_dma.h"
#include "stm32f4xx_hal.h"

#define DISPLAY_PERIOD_MS 250U

static uint32_t display_tick;

static void Display_PrintLine(uint8_t y, char *text)
{
  OLED_PrintASCIIString(0U, y, text, &afont8x6, OLED_COLOR_NORMAL);
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
  uint32_t now;
  GyroSnapshot gyro;
  EncoderSnapshot encoder;
  BatterySnapshot battery;
  ChassisTestSnapshot driver;
  char state;

  now = HAL_GetTick();
  if ((now - display_tick) < DISPLAY_PERIOD_MS)
  {
    return;
  }
  display_tick = now;
  gyro = Gyro_GetSnapshot();
  encoder = Encoder_GetSnapshot();
  battery = Battery_GetSnapshot();
  driver = ChassisTest_GetSnapshot();
  state = (gyro.connected == 0U) ? 'E' :
          ((gyro.ready != 0U) ? 'R' : 'C');

  OLED_NewFrame();
  (void)snprintf(line, sizeof(line), "I:%c BLE:%c ARM:%u", state,
                 SerialDma_IsReady() ? '1' : '0', driver.armed);
  Display_PrintLine(0U, line);
  (void)snprintf(line, sizeof(line), "BAT:%2u.%02uV M:%c%c%u",
                 battery.voltage_mv / 1000U,
                 (battery.voltage_mv % 1000U) / 10U,
                 driver.active ? (char)('A' + (uint8_t)driver.channel) : '-',
                 driver.active ? ((driver.direction >= 0) ? '+' : '-') : '-',
                 driver.duty_percent);
  Display_PrintLine(8U, line);
  (void)snprintf(line, sizeof(line), "E1:%7ld d:%4ld",
                 (long)encoder.total[0], (long)encoder.delta[0]);
  Display_PrintLine(16U, line);
  (void)snprintf(line, sizeof(line), "E2:%7ld d:%4ld",
                 (long)encoder.total[1], (long)encoder.delta[1]);
  Display_PrintLine(24U, line);
  (void)snprintf(line, sizeof(line), "E3:%7ld d:%4ld",
                 (long)encoder.total[2], (long)encoder.delta[2]);
  Display_PrintLine(32U, line);
  (void)snprintf(line, sizeof(line), "E4:%7ld d:%4ld",
                 (long)encoder.total[3], (long)encoder.delta[3]);
  Display_PrintLine(40U, line);
  (void)snprintf(line, sizeof(line), "R:%4ld %4ld %4ld %4ld",
                 (long)(encoder.rpm_x10[0] / 10L),
                 (long)(encoder.rpm_x10[1] / 10L),
                 (long)(encoder.rpm_x10[2] / 10L),
                 (long)(encoder.rpm_x10[3] / 10L));
  Display_PrintLine(48U, line);
  (void)snprintf(line, sizeof(line), "Y:%6ld W:%02X F:%u",
                 (long)(gyro.yaw_deg * 10.0f), gyro.who_am_i,
                 gyro.read_failures);
  Display_PrintLine(56U, line);
  OLED_ShowFrame();
}
