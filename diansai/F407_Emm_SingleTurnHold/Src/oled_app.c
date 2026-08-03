#include "oled_app.h"
#include "bluetooth_app.h"
#include "oled.h"
#include "key_app.h"
#include "motor_app.h"
#include <stdio.h>

/* ARMCC5 不合并这些可写字符串，可避免 L6480W，同时仍直接使用 sprintf。 */
static char fmt_motor_1[] = "M1:%c%03lu.%lu V:%u";
static char fmt_motor_2[] = "M2:%c%03lu.%lu V:%u";
static char fmt_count[] = "KEY:%03lu CMD:%03lu";
static char fmt_target[] = "T1:%c%03lu.%lu T2:%c%03lu.%lu";
static char fmt_state[] = "ST:%02u PEND:%02u";
static char fmt_error[] = "RXE:%03lu COR:%03lu";
static char fmt_bt[] = "BT:%03lu LAST:%02X";
static char line_address[] = "M1 ADDR:1 M2:2";

void oled_pro(void)
{
  static uint32_t oled_time = 0UL;
  char str[22];
  int32_t angle_10;
  int32_t angle_10_2;
  uint32_t angle_value;
  uint32_t angle_value_2;

  if ((uint32_t)(HAL_GetTick() - oled_time) < 200UL)
  {
    return;
  }
  oled_time = HAL_GetTick();

  OLED_NewFrame();

  angle_10 = (int32_t)(motor_angle[0] * 10.0f +
                       ((motor_angle[0] < 0.0f) ? -0.5f : 0.5f));
  angle_value = (angle_10 < 0) ? (uint32_t)(-angle_10) : (uint32_t)angle_10;
  sprintf(str, fmt_motor_1, (angle_10 < 0) ? '-' : '+',
          (unsigned long)((angle_value / 10UL) % 1000UL),
          (unsigned long)(angle_value % 10UL), motor_position_valid[0]);
  OLED_PrintASCIIString(0U, 0U, str, &afont8x6, OLED_COLOR_NORMAL);

  angle_10 = (int32_t)(motor_angle[1] * 10.0f +
                       ((motor_angle[1] < 0.0f) ? -0.5f : 0.5f));
  angle_value = (angle_10 < 0) ? (uint32_t)(-angle_10) : (uint32_t)angle_10;
  sprintf(str, fmt_motor_2, (angle_10 < 0) ? '-' : '+',
          (unsigned long)((angle_value / 10UL) % 1000UL),
          (unsigned long)(angle_value % 10UL), motor_position_valid[1]);
  OLED_PrintASCIIString(0U, 8U, str, &afont8x6, OLED_COLOR_NORMAL);

  sprintf(str, fmt_count, (unsigned long)(key_count % 1000UL),
          (unsigned long)(motor_cmd_count % 1000UL));
  OLED_PrintASCIIString(0U, 16U, str, &afont8x6, OLED_COLOR_NORMAL);

  angle_10 = (int32_t)(motor_target_angle[0] * 10.0f +
                       ((motor_target_angle[0] < 0.0f) ? -0.5f : 0.5f));
  angle_value = (angle_10 < 0) ? (uint32_t)(-angle_10) : (uint32_t)angle_10;
  angle_10_2 = (int32_t)(motor_target_angle[1] * 10.0f +
                         ((motor_target_angle[1] < 0.0f) ? -0.5f : 0.5f));
  angle_value_2 = (angle_10_2 < 0) ?
                  (uint32_t)(-angle_10_2) : (uint32_t)angle_10_2;
  sprintf(str, fmt_target, (angle_10 < 0) ? '-' : '+',
          (unsigned long)((angle_value / 10UL) % 1000UL),
          (unsigned long)(angle_value % 10UL),
          (angle_10_2 < 0) ? '-' : '+',
          (unsigned long)((angle_value_2 / 10UL) % 1000UL),
          (unsigned long)(angle_value_2 % 10UL));
  OLED_PrintASCIIString(0U, 24U, str, &afont8x6, OLED_COLOR_NORMAL);

  sprintf(str, fmt_state, motor_state, motor_pending_steps);
  OLED_PrintASCIIString(0U, 32U, str, &afont8x6, OLED_COLOR_NORMAL);

  sprintf(str, fmt_error,
          (unsigned long)(motor_error_count % 1000UL),
          (unsigned long)(motor_correction_count % 1000UL));
  OLED_PrintASCIIString(0U, 40U, str, &afont8x6, OLED_COLOR_NORMAL);

  OLED_PrintASCIIString(0U, 48U, line_address, &afont8x6,
                        OLED_COLOR_NORMAL);
  sprintf(str, fmt_bt, (unsigned long)(bluetooth_rx_count % 1000UL),
          (unsigned int)bluetooth_last_byte);
  OLED_PrintASCIIString(0U, 56U, str, &afont8x6, OLED_COLOR_NORMAL);
  OLED_ShowFrame();
}
