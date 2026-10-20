#include "ps2_control.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "chassis_test.h"
#include "attitude_control.h"
#include "ps2.h"
#include "speed_control.h"
#include "stm32f4xx_hal.h"

#define PS2_CONTROL_DEADZONE 18
#define PS2_CONTROL_TIMEOUT_MS 180U
#define PS2_CONTROL_HIGH_RPM 140U
#define PS2_TURN_ENTER_DEADZONE 24

static Ps2ControlSnapshot control_state;
static uint8_t last_start;
static uint8_t turn_active;

static int16_t Ps2_MapAxis(uint8_t raw)
{
  int32_t value = (int32_t)raw - 128;
  int32_t magnitude;

  if (abs((int)value) <= PS2_CONTROL_DEADZONE)
  {
    return 0;
  }
  magnitude = abs((int)value) - PS2_CONTROL_DEADZONE;
  magnitude = (magnitude * 100) / (127 - PS2_CONTROL_DEADZONE);
  if (magnitude > 100)
  {
    magnitude = 100;
  }
  return (int16_t)((value < 0) ? -magnitude : magnitude);
}

static float Ps2_MapTurnRate(uint8_t raw)
{
  int magnitude = abs((int)raw - 128);
  if (magnitude <= PS2_CONTROL_DEADZONE) turn_active = 0U;
  else if (magnitude >= PS2_TURN_ENTER_DEADZONE) turn_active = 1U;
  if (!turn_active) return 0.0f;
  /* Rotation authority is independent of the translation speed tier. */
  return -(float)Ps2_MapAxis(raw) * ATTITUDE_RATE_LIMIT_DPS / 100.0f;
}

static void Ps2_MapTranslation(const Ps2Snapshot *ps2,
                               int16_t *forward, int16_t *left)
{
  float x = 128.0f - (float)ps2->left_x;
  float y = 128.0f - (float)ps2->left_y;
  float radius = sqrtf(x * x + y * y);
  float magnitude, scale, wheel_rpm;

  *forward = *left = 0;
  if (radius <= PS2_CONTROL_DEADZONE) return;
  /* A radial deadzone preserves the X/Y ratio, including small cross-axis
   * inputs. No angular sectors or independent axis deadzones are applied. */
  magnitude = fminf(1.0f, (radius - PS2_CONTROL_DEADZONE) /
                            (127.0f - PS2_CONTROL_DEADZONE));
  scale = magnitude * (float)control_state.speed_rpm / radius;
  /* Bound translation's peak wheel RPM, preserving direction and leaving
   * at least 20 RPM at the default ceiling. The attitude allocator reduces
   * translation further when a turn needs more rotation authority. */
  wheel_rpm = (fabsf(x) + fabsf(y)) * scale;
  if (wheel_rpm > control_state.speed_rpm)
    scale *= (float)control_state.speed_rpm / wheel_rpm;
  *forward = (int16_t)(y * scale);
  *left = (int16_t)(x * scale);
}

static uint8_t Ps2_SelectedSpeed(const Ps2Snapshot *ps2)
{
  uint8_t l2 = (ps2->buttons & PS2_BUTTON_L2) != 0U;
  uint8_t r2 = (ps2->buttons & PS2_BUTTON_R2) != 0U;

  if (l2 && r2) return PS2_CONTROL_HIGH_RPM;
  if (l2 || r2) return 60U;
  return 30U;
}

static uint8_t Ps2_IsCentered(const Ps2Snapshot *ps2)
{
  int32_t x = (int32_t)ps2->left_x - 128;
  int32_t y = (int32_t)ps2->left_y - 128;
  return ((x * x + y * y) <=
          PS2_CONTROL_DEADZONE * PS2_CONTROL_DEADZONE &&
          abs((int)ps2->right_x - 128) <= PS2_CONTROL_DEADZONE) ? 1U : 0U;
}

void Ps2Control_Init(void)
{
  memset(&control_state, 0, sizeof(control_state));
  control_state.speed_rpm = 30U;
  last_start = 0U;
  turn_active = 0U;
}

void Ps2Control_Stop(void)
{
  ChassisTest_Stop();
  control_state.enabled = 0U;
  control_state.forward_rpm = 0;
  control_state.left_rpm = 0;
  control_state.rotate_rpm = 0;
  turn_active = 0U;
  /* Preserve START history across STOP: a held button cannot re-arm. */
}

uint8_t Ps2Control_IsActive(void)
{
  return control_state.enabled;
}

void Ps2Control_Process(void)
{
  Ps2Snapshot ps2 = Ps2_GetSnapshot();
  uint8_t start_pressed;
  uint8_t select_pressed;
  uint8_t start_edge;
  int16_t forward;
  int16_t left;
  int16_t rotate;
  float rate_dps;

  if ((ps2.connected == 0U) || (ps2.valid == 0U) ||
      ((HAL_GetTick() - ps2.last_tick) > PS2_CONTROL_TIMEOUT_MS))
  {
    last_start = 1U; /* Require a fresh release/press after reconnect. */
    if (control_state.enabled != 0U)
    {
      Ps2Control_Stop();
    }
    return;
  }

  start_pressed = (ps2.buttons & PS2_BUTTON_START) != 0U;
  select_pressed = (ps2.buttons & PS2_BUTTON_SELECT) != 0U;
  start_edge = start_pressed && !last_start;
  last_start = start_pressed;
  /* SELECT always wins, including when both buttons are held. */
  if (select_pressed != 0U)
  {
    if (control_state.enabled) Ps2Control_Stop();
    return;
  }
  if ((start_edge != 0U) &&
      (Ps2_IsCentered(&ps2) != 0U))
  {
    if (control_state.enabled == 0U)
    {
      ChassisTest_Arm();
      control_state.enabled = SpeedControl_Enable(1U);
    }
    else
    {
      Ps2Control_Stop();
      return;
    }
  }
  if (control_state.enabled == 0U)
  {
    return;
  }
  if (!SpeedControl_GetSnapshot().enabled)
  {
    Ps2Control_Stop();
    return;
  }

  control_state.speed_rpm = Ps2_SelectedSpeed(&ps2);
  Ps2_MapTranslation(&ps2, &forward, &left);
  rotate = (int16_t)(-Ps2_MapAxis(ps2.right_x) * control_state.speed_rpm / 100);
  rate_dps = Ps2_MapTurnRate(ps2.right_x);
  control_state.forward_rpm = forward;
  control_state.left_rpm = left;
  control_state.rotate_rpm = rotate;
  if (AttitudeControl_SetManualDrive(rate_dps, forward, left) == 0U)
  {
    Ps2Control_Stop();
  }
}

Ps2ControlSnapshot Ps2Control_GetSnapshot(void)
{
  return control_state;
}
