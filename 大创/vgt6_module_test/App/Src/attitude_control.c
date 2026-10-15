#include "attitude_control.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "chassis_test.h"
#include "gyro.h"
#include "speed_control.h"
#include "stm32f4xx_hal.h"

#define ATTITUDE_WATCHDOG_MS 1500U
static AttitudeControlSnapshot state;
static float integral_output;
static int16_t forward_rpm, left_rpm;
static uint32_t command_tick, accumulated_ms;
static float clamp(float x, float limit)
{
  return fmaxf(-limit, fminf(limit, x));
}
static float wrap(float x)
{
  return x - 360.0f * floorf((x + 180.0f) / 360.0f);
}
void AttitudeControl_Init(void)
{
  memset(&state, 0, sizeof(state));
  state.rate_kp = 0.15f;
  state.rate_ki = 2.5f;
  state.heading_kp = 5.0f;
  state.heading_kd = 1.25f;
  AttitudeControl_Stop();
}
void AttitudeControl_Stop(void)
{
  state.mode = 0U;
  state.saturated = 0U;
  state.target_dps = state.output_rpm = state.output_dps = 0.0f;
  integral_output = 0.0f;
  forward_rpm = left_rpm = 0;
  accumulated_ms = 0U;
}
static uint8_t ready(void)
{
  GyroSnapshot gyro = Gyro_GetSnapshot();
  return gyro.ready && !gyro.read_failures &&
         ChassisTest_GetSnapshot().armed;
}
static uint8_t start(uint8_t mode)
{
  if (!ready()) return 0U;
  if (state.mode != mode)
  {
    ChassisTest_Stop();
    if (!SpeedControl_Enable(1U)) return 0U;
    state.mode = mode;
  }
  command_tick = HAL_GetTick();
  return 1U;
}
uint8_t AttitudeControl_SetRate(float rate_dps, int16_t forward, int16_t left)
{
  if (!isfinite(rate_dps) || fabsf(rate_dps) > ATTITUDE_RATE_LIMIT_DPS ||
      abs((int)forward) + abs((int)left) > SPEED_CONTROL_MAX_TARGET_RPM)
    return 0U;
  if (!start(1U)) return 0U;
  state.target_dps = rate_dps;
  forward_rpm = forward;
  left_rpm = left;
  return 1U;
}
uint8_t AttitudeControl_SetHeading(float heading_deg)
{
  if (!isfinite(heading_deg) || fabsf(heading_deg) > 180.0f) return 0U;
  if (!start(2U)) return 0U;
  state.target_deg = wrap(heading_deg);
  forward_rpm = left_rpm = 0;
  return 1U;
}
uint8_t AttitudeControl_KeepAlive(void)
{
  if (!state.mode) return 0U;
  command_tick = HAL_GetTick();
  return 1U;
}
uint8_t AttitudeControl_SetRatePid(float kp, float ki)
{
  if (!isfinite(kp) || !isfinite(ki) || kp < 0 || kp > 100 ||
      ki < 0 || ki > 100) return 0U;
  state.rate_kp = kp;
  state.rate_ki = ki;
  /* Bumpless transfer at the current error; preserve the active experiment. */
  integral_output = clamp(state.output_rpm -
      kp * (state.target_dps - state.feedback_dps), ATTITUDE_OUTPUT_LIMIT_RPM);
  return 1U;
}
uint8_t AttitudeControl_SetHeadingPid(float kp, float kd)
{
  if (!isfinite(kp) || !isfinite(kd) || kp < 0 || kp > 100 ||
      kd < 0 || kd > 100) return 0U;
  state.heading_kp = kp;
  state.heading_kd = kd;
  return 1U;
}
void AttitudeControl_Process(uint32_t elapsed_ms)
{
  GyroSnapshot gyro;
  SpeedControlSnapshot speed;
  float error, candidate, raw, output, limit;
  if (!state.mode) return;
  if (!ready() || !SpeedControl_GetSnapshot().enabled ||
      HAL_GetTick() - command_tick > ATTITUDE_WATCHDOG_MS)
  {
    ChassisTest_Stop();
    return;
  }
  accumulated_ms += elapsed_ms;
  if (accumulated_ms < ATTITUDE_PERIOD_MS) return;
  if (accumulated_ms > 60U)
  {
    ChassisTest_Stop();
    return;
  }
  gyro = Gyro_GetSnapshot();
  speed = SpeedControl_GetSnapshot();
  state.feedback_deg = gyro.yaw_deg;
  state.feedback_dps = gyro.rate_dps;
  state.error_deg = wrap(state.target_deg - gyro.yaw_deg);
  state.saturated = 0U;
  if (state.mode == 2U)
  {
    raw = state.heading_kp * state.error_deg - state.heading_kd * gyro.rate_dps;
    state.output_dps = clamp(raw, ATTITUDE_RATE_LIMIT_DPS);
    state.saturated = fabsf(raw) > ATTITUDE_RATE_LIMIT_DPS;
    state.target_dps = state.output_dps;
  }
  /* The rotation output shares wheel RPM headroom with translation. */
  limit = fminf(ATTITUDE_OUTPUT_LIMIT_RPM,
      fmaxf(0, (float)speed.target_limit_rpm -
                    (float)(abs((int)forward_rpm) + abs((int)left_rpm))));
  error = state.target_dps - gyro.rate_dps;
  candidate = integral_output + state.rate_ki * error *
                                  (float)accumulated_ms / 1000.0f;
  raw = state.rate_kp * error + candidate;
  output = clamp(raw, limit);
  if ((raw == output) || (raw > output && error < 0) ||
      (raw < output && error > 0))
    integral_output = clamp(candidate, limit);
  raw = state.rate_kp * error + integral_output;
  state.output_rpm = clamp(raw, limit);
  state.saturated |= fabsf(raw) > limit;
  (void)SpeedControl_SetDrive(forward_rpm, left_rpm,
                              (int16_t)lroundf(state.output_rpm));
  accumulated_ms = 0U;
}
AttitudeControlSnapshot AttitudeControl_GetSnapshot(void) { return state; }
