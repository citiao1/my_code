#include "yaw_rate_control.h"

#define YAW_RATE_FIXED_KP                0.15f
#define YAW_RATE_FIXED_KI                2.5f
#define YAW_RATE_DEFAULT_INTEGRAL_LIMIT 100.0f

static YawRateControlSnapshot yaw_state;
static float yaw_integral;

static float YawRateControl_Clamp(float value, float limit)
{
  if (value > limit)
  {
    return limit;
  }
  if (value < -limit)
  {
    return -limit;
  }
  return value;
}

void YawRateControl_Init(void)
{
  yaw_state.kp = YAW_RATE_FIXED_KP;
  yaw_state.ki = YAW_RATE_FIXED_KI;
  yaw_state.kff = 0.0f;
  yaw_state.integral_limit = YAW_RATE_DEFAULT_INTEGRAL_LIMIT;
  yaw_state.enabled = 1U;
  YawRateControl_Reset();
}

void YawRateControl_Reset(void)
{
  yaw_integral = 0.0f;
  yaw_state.target_dps = 0.0f;
  yaw_state.feedback_dps = 0.0f;
  yaw_state.error_dps = 0.0f;
  yaw_state.output_rpm = 0.0f;
}

float YawRateControl_Update(float target_dps, float feedback_dps,
                            float dt_seconds, float output_limit_rpm)
{
  float candidate_integral;
  float unlimited_output;

  yaw_state.target_dps = target_dps;
  yaw_state.feedback_dps = feedback_dps;
  yaw_state.error_dps = target_dps - feedback_dps;

  if ((yaw_state.enabled == 0U) || (dt_seconds <= 0.0f) ||
      (output_limit_rpm <= 0.0f))
  {
    yaw_integral = 0.0f;
    yaw_state.output_rpm = 0.0f;
    return 0.0f;
  }

  candidate_integral = YawRateControl_Clamp(
    yaw_integral + yaw_state.error_dps * dt_seconds,
    yaw_state.integral_limit);
  unlimited_output = yaw_state.kp * yaw_state.error_dps +
                     yaw_state.ki * candidate_integral;
  yaw_state.output_rpm = YawRateControl_Clamp(unlimited_output, output_limit_rpm);

  if (((unlimited_output <= output_limit_rpm) &&
       (unlimited_output >= -output_limit_rpm)) ||
      (yaw_state.output_rpm * yaw_state.error_dps < 0.0f))
  {
    yaw_integral = candidate_integral;
  }
  return yaw_state.output_rpm;
}

void YawRateControl_SetEnabled(uint8_t enabled)
{
  yaw_state.enabled = (enabled != 0U) ? 1U : 0U;
  YawRateControl_Reset();
}

YawRateControlSnapshot YawRateControl_GetSnapshot(void)
{
  return yaw_state;
}
