#include "vehicle_display.h"

#include "gpio.h"
#include "vehicle_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void OLEDWriteByte(uint8_t value, GPIO_PinState data_mode)
{
  uint8_t bit;
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, data_mode);
  for (bit = 0; bit < 8U; ++bit)
  {
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, (value & 0x80U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
    value <<= 1;
  }
}

static void OLEDCommand(uint8_t value)
{
  OLEDWriteByte(value, GPIO_PIN_RESET);
}

static void OLEDSetPage(uint8_t page)
{
  OLEDCommand((uint8_t)(0xB0U + page));
  OLEDCommand(0x00);
  OLEDCommand(0x10);
}

static const uint8_t glyphs[][5] = {
  {0x00,0x00,0x00,0x00,0x00}, {0x08,0x08,0x3E,0x08,0x08},
  {0x08,0x08,0x08,0x08,0x08}, {0x00,0x60,0x60,0x00,0x00},
  {0x00,0x36,0x36,0x00,0x00},
  {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
  {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
  {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
  {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
  {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E},
  {0x7E,0x11,0x11,0x11,0x7E}, {0x7F,0x49,0x49,0x49,0x36},
  {0x3E,0x41,0x41,0x41,0x22}, {0x7F,0x41,0x41,0x22,0x1C},
  {0x7F,0x49,0x49,0x49,0x41}, {0x7F,0x09,0x09,0x09,0x01},
  {0x3E,0x41,0x49,0x49,0x7A}, {0x7F,0x08,0x08,0x08,0x7F},
  {0x00,0x41,0x7F,0x41,0x00}, {0x20,0x40,0x41,0x3F,0x01},
  {0x7F,0x08,0x14,0x22,0x41}, {0x7F,0x40,0x40,0x40,0x40},
  {0x7F,0x02,0x0C,0x02,0x7F}, {0x7F,0x04,0x08,0x10,0x7F},
  {0x3E,0x41,0x41,0x41,0x3E}, {0x7F,0x09,0x09,0x09,0x06},
  {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46},
  {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7F,0x01,0x01},
  {0x3F,0x40,0x40,0x40,0x3F}, {0x1F,0x20,0x40,0x20,0x1F},
  {0x3F,0x40,0x38,0x40,0x3F}, {0x63,0x14,0x08,0x14,0x63},
  {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43}
};

static const char glyph_chars[] = " +-.:0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

void VehicleDisplay_WriteLine(uint8_t page, const char *text)
{
  uint8_t column = 0;
  OLEDSetPage(page);
  while (*text != '\0' && column <= 122U)
  {
    const char *match = strchr(glyph_chars, *text);
    uint8_t index = match ? (uint8_t)(match - glyph_chars) : 0U;
    uint8_t glyph_column;
    for (glyph_column = 0; glyph_column < 5U; ++glyph_column)
    {
      OLEDWriteByte(glyphs[index][glyph_column], GPIO_PIN_SET);
    }
    OLEDWriteByte(0x00, GPIO_PIN_SET);
    column = (uint8_t)(column + 6U);
    ++text;
  }
  while (column++ < 128U) OLEDWriteByte(0x00, GPIO_PIN_SET);
}

void VehicleDisplay_Init(void)
{
  static const uint8_t commands[] = {
    0xAE,0xD5,0x50,0xA8,0x3F,0xD3,0x00,0x40,0x8D,0x14,0x20,0x02,
    0xA1,0xC8,0xDA,0x12,0x81,0xEF,0xD9,0xF1,0xDB,0x30,0xA4,0xA6,0xAF
  };
  uint8_t index;
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);
  HAL_Delay(20);
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
  HAL_Delay(20);
  for (index = 0; index < sizeof(commands); ++index) OLEDCommand(commands[index]);
  for (index = 0; index < 8U; ++index) VehicleDisplay_WriteLine(index, "");
}

void VehicleDisplay_Update(void)
{
  char line[24];
  int yaw10 = (int)(state.yaw * 10.0f);
  int pitch10 = (int)(state.pitch * 10.0f);
  int roll10 = (int)(state.roll * 10.0f);

  if (line_follow.phase != LINE_PHASE_IDLE)
  {
    if (line_follow.phase == LINE_PHASE_WAIT)
    {
      uint32_t elapsed = HAL_GetTick() - line_follow.phase_start_ms;
      uint32_t remaining = elapsed < LINE_START_DELAY_MS ? LINE_START_DELAY_MS - elapsed : 0U;
      snprintf(line, sizeof(line), "LINE WAIT %03luMS", (unsigned long)remaining);
    }
    else if (line_follow.phase == LINE_PHASE_RUN)
    {
      snprintf(line, sizeof(line), "LINE RUN %03u %s", (unsigned int)line_follow.speed_percent,
               line_follow.line_visible ? "TRACK" : "SEARCH");
    }
    else if (line_follow.phase == LINE_PHASE_CORNER_ADVANCE)
    {
      snprintf(line, sizeof(line), "LINE CORNER FORWARD");
    }
    else if (line_follow.phase == LINE_PHASE_CORNER_TURN)
    {
      snprintf(line, sizeof(line), "LINE TURN LEFT 90");
    }
    else if (line_follow.phase == LINE_PHASE_LOST)
    {
      snprintf(line, sizeof(line), "LINE LOST PRESS KEY");
    }
    else
    {
      snprintf(line, sizeof(line), "LINE ERROR CHECK HW");
    }
    VehicleDisplay_WriteLine(0, line);
    snprintf(line, sizeof(line), "G0 %03u %03u %03u %03u", (unsigned int)(line_follow.gray[0] >> 4),
             (unsigned int)(line_follow.gray[1] >> 4), (unsigned int)(line_follow.gray[2] >> 4),
             (unsigned int)(line_follow.gray[3] >> 4));
    VehicleDisplay_WriteLine(1, line);
    snprintf(line, sizeof(line), "G4 %03u %03u %03u %03u", (unsigned int)(line_follow.gray[4] >> 4),
             (unsigned int)(line_follow.gray[5] >> 4), (unsigned int)(line_follow.gray[6] >> 4),
             (unsigned int)(line_follow.gray[7] >> 4));
    VehicleDisplay_WriteLine(2, line);
    snprintf(line, sizeof(line), "ERR %+04d YR %+04d S%03u", (int)line_follow.error,
             (int)state.target_yaw_rate, (unsigned int)line_follow.speed_percent);
    VehicleDisplay_WriteLine(3, line);
    snprintf(line, sizeof(line), "VL %+04d VR %+04d", (int)(state.speed_left * 1000.0f),
             (int)(state.speed_right * 1000.0f));
    VehicleDisplay_WriteLine(4, line);
    snprintf(line, sizeof(line), "PWM %+05d %+05d", state.pwm_left, state.pwm_right);
    VehicleDisplay_WriteLine(5, line);
    snprintf(line, sizeof(line), "E %+06ld %+06ld", (long)state.encoder_left, (long)state.encoder_right);
    VehicleDisplay_WriteLine(6, line);
    snprintf(line, sizeof(line), "BAT %u.%02uV MPU %u", (unsigned int)state.battery_voltage,
             (unsigned int)(state.battery_voltage * 100.0f) % 100U, state.mpu_ok);
    VehicleDisplay_WriteLine(7, line);
    return;
  }

  snprintf(line, sizeof(line), "YAW %+04d.%d", yaw10 / 10, abs(yaw10 % 10));
  VehicleDisplay_WriteLine(0, line);
  snprintf(line, sizeof(line), "PIT %+03d.%d ROL %+03d.%d", pitch10 / 10, abs(pitch10 % 10),
           roll10 / 10, abs(roll10 % 10));
  VehicleDisplay_WriteLine(1, line);
  snprintf(line, sizeof(line), "TL %+04d TR %+04d", (int)(state.target_left * 1000.0f),
           (int)(state.target_right * 1000.0f));
  VehicleDisplay_WriteLine(2, line);
  snprintf(line, sizeof(line), "VL %+04d VR %+04d", (int)(state.speed_left * 1000.0f),
           (int)(state.speed_right * 1000.0f));
  VehicleDisplay_WriteLine(3, line);
  snprintf(line, sizeof(line), "ER %+04d %+04d", (int)(state.error_left * 1000.0f),
           (int)(state.error_right * 1000.0f));
  VehicleDisplay_WriteLine(4, line);
  snprintf(line, sizeof(line), "EA %+07ld", (long)state.encoder_left);
  VehicleDisplay_WriteLine(5, line);
  snprintf(line, sizeof(line), "EB %+07ld", (long)state.encoder_right);
  VehicleDisplay_WriteLine(6, line);
  snprintf(line, sizeof(line), "BAT %u.%02uV MPU %u", (unsigned int)state.battery_voltage,
           (unsigned int)(state.battery_voltage * 100.0f) % 100U, state.mpu_ok);
  VehicleDisplay_WriteLine(7, line);
}
