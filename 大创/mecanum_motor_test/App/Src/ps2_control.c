#include "ps2_control.h"

#include "ps2.h"
#include "stm32f4xx_hal.h"
#include "vehicle.h"

#define PS2_CONTROL_PERIOD_MS       20U
#define PS2_RECONFIGURE_PERIOD_MS 1000U
#define PS2_AXIS_CENTER            128
#define PS2_AXIS_DEADZONE           12

static uint32_t control_tick;
static uint32_t reconfigure_tick;
static uint16_t previous_buttons;
static uint8_t connected;
static uint8_t armed;

static int16_t Ps2Control_GetAxis(uint8_t raw, uint16_t limit)
{
  int16_t value = (int16_t)PS2_AXIS_CENTER - (int16_t)raw;
  int16_t magnitude;
  int16_t range = PS2_AXIS_CENTER - 1 - PS2_AXIS_DEADZONE;
  int32_t scaled;

  if ((value >= -PS2_AXIS_DEADZONE) && (value <= PS2_AXIS_DEADZONE))
  {
    return 0;
  }

  if (value > 0)
  {
    magnitude = value - PS2_AXIS_DEADZONE;
    scaled = ((int32_t)magnitude * limit) / range;
    return (int16_t)((scaled > limit) ? limit : scaled);
  }

  magnitude = (int16_t)(-value - PS2_AXIS_DEADZONE);
  scaled = ((int32_t)magnitude * limit) / range;
  return (int16_t)-((scaled > limit) ? limit : scaled);
}

static uint8_t Ps2Control_SticksAreNeutral(const Ps2State *state)
{
  return (Ps2Control_GetAxis(state->left_x, 100U) == 0) &&
         (Ps2Control_GetAxis(state->left_y, 100U) == 0) &&
         (Ps2Control_GetAxis(state->right_x, 100U) == 0);
}

void Ps2Control_Init(void)
{
  connected = 0U;
  armed = 0U;
  previous_buttons = 0U;
  Ps2_Init();
  control_tick = HAL_GetTick() - PS2_CONTROL_PERIOD_MS;
  reconfigure_tick = HAL_GetTick();
}

void Ps2Control_Process(void)
{
  Ps2State state;
  uint16_t pressed;
  uint16_t speed;
  int16_t forward_rpm;
  int16_t left_rpm;
  int16_t yaw_rate_dps;
  uint32_t now = HAL_GetTick();

  if ((now - control_tick) < PS2_CONTROL_PERIOD_MS)
  {
    return;
  }
  control_tick = now;

  if ((Ps2_Poll(&state) == 0U) || (state.analog_mode == 0U))
  {
    connected = 0U;
    armed = 0U;
    previous_buttons = 0U;
    Vehicle_Stop();

    if ((now - reconfigure_tick) >= PS2_RECONFIGURE_PERIOD_MS)
    {
      Ps2_ConfigureAnalog();
      reconfigure_tick = HAL_GetTick();
    }
    return;
  }

  connected = 1U;
  pressed = state.buttons & (uint16_t)(~previous_buttons);
  previous_buttons = state.buttons;

  if ((pressed & PS2_BUTTON_SELECT) != 0U)
  {
    armed = 0U;
    Vehicle_Stop();
    return;
  }

  if (((pressed & PS2_BUTTON_START) != 0U) &&
      (Ps2Control_SticksAreNeutral(&state) != 0U))
  {
    armed = 1U;
  }

  if (armed == 0U)
  {
    Vehicle_Stop();
    return;
  }

  speed = Vehicle_GetSpeed();
  forward_rpm = Ps2Control_GetAxis(state.left_y, speed);
  left_rpm = Ps2Control_GetAxis(state.left_x, speed);
  yaw_rate_dps = Ps2Control_GetAxis(state.right_x, speed);

  if (Vehicle_Drive(forward_rpm, left_rpm, yaw_rate_dps) == 0U)
  {
    Vehicle_Stop();
  }
}

uint8_t Ps2Control_IsConnected(void)
{
  return connected;
}

uint8_t Ps2Control_IsArmed(void)
{
  return armed;
}

char Ps2Control_GetStatusChar(void)
{
  if (connected == 0U)
  {
    return '-';
  }
  return (armed != 0U) ? 'A' : 'S';
}
