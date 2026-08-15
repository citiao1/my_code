#include "heading_control.h"

#define HEADING_DEFAULT_KP                 5.0f
#define HEADING_DEFAULT_KD                 1.25f
#define HEADING_DEFAULT_MAX_RATE_DPS      80.0f
#define HEADING_ERROR_DEADBAND_DEG         0.5f
#define HEADING_MIN_CORRECTION_DPS         8.0f
#define HEADING_CORRECTION_RATE_GATE_DPS   5.0f

static HeadingControlSnapshot heading_state;

static float HeadingControl_Abs(float value)
{
  return (value >= 0.0f) ? value : -value;
}

static float HeadingControl_Clamp(float value, float limit)
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

static float HeadingControl_Wrap(float angle)
{
  while (angle > 180.0f)
  {
    angle -= 360.0f;
  }
  while (angle < -180.0f)
  {
    angle += 360.0f;
  }
  return angle;
}

void HeadingControl_Init(void)
{
  heading_state.kp = HEADING_DEFAULT_KP;
  heading_state.kd = HEADING_DEFAULT_KD;
  heading_state.max_rate_dps = HEADING_DEFAULT_MAX_RATE_DPS;
  heading_state.enabled = 1U;
  HeadingControl_Reset();
}

void HeadingControl_Reset(void)
{
  heading_state.target_deg = 0.0f;
  heading_state.feedback_deg = 0.0f;
  heading_state.error_deg = 0.0f;
  heading_state.output_dps = 0.0f;
  heading_state.holding = 0U;
}

void HeadingControl_Track(float current_heading_deg)
{
  heading_state.target_deg = current_heading_deg;
  heading_state.feedback_deg = current_heading_deg;
  heading_state.error_deg = 0.0f;
  heading_state.output_dps = 0.0f;
  heading_state.holding = 0U;
}

uint8_t HeadingControl_StepTarget(float current_heading_deg, float delta_deg)
{
  if ((heading_state.enabled == 0U) ||
      (delta_deg < -170.0f) || (delta_deg > 170.0f) ||
      (delta_deg == 0.0f))
  {
    return 0U;
  }

  heading_state.target_deg = HeadingControl_Wrap(current_heading_deg + delta_deg);
  heading_state.feedback_deg = current_heading_deg;
  heading_state.error_deg = HeadingControl_Wrap(
    heading_state.target_deg - current_heading_deg);
  heading_state.output_dps = 0.0f;
  heading_state.holding = 1U;
  return 1U;
}

uint8_t HeadingControl_HoldTarget(float current_heading_deg, float target_heading_deg)
{
  if (heading_state.enabled == 0U)
  {
    return 0U;
  }

  heading_state.target_deg = HeadingControl_Wrap(target_heading_deg);
  heading_state.feedback_deg = current_heading_deg;
  heading_state.error_deg = HeadingControl_Wrap(
    heading_state.target_deg - current_heading_deg);
  heading_state.output_dps = 0.0f;
  heading_state.holding = 1U;
  return 1U;
}

float HeadingControl_Update(float current_heading_deg, float current_rate_dps,
                            float output_limit_dps)
{
  float limit;
  float output;

  heading_state.feedback_deg = current_heading_deg;
  if ((heading_state.enabled == 0U) || (output_limit_dps <= 0.0f))
  {
    HeadingControl_Reset();
    return 0.0f;
  }

  if (heading_state.holding == 0U)
  {
    heading_state.target_deg = current_heading_deg;
    heading_state.error_deg = 0.0f;
    heading_state.output_dps = 0.0f;
    heading_state.holding = 1U;
    return 0.0f;
  }

  limit = output_limit_dps;
  if (limit > heading_state.max_rate_dps)
  {
    limit = heading_state.max_rate_dps;
  }

  heading_state.error_deg = HeadingControl_Wrap(
    heading_state.target_deg - current_heading_deg);
  output = heading_state.kp * heading_state.error_deg -
           heading_state.kd * current_rate_dps;
  output = HeadingControl_Clamp(output, limit);

  if ((HeadingControl_Abs(heading_state.error_deg) <= HEADING_ERROR_DEADBAND_DEG) &&
      (HeadingControl_Abs(current_rate_dps) <= HEADING_CORRECTION_RATE_GATE_DPS))
  {
    output = 0.0f;
  }
  else if ((HeadingControl_Abs(heading_state.error_deg) > HEADING_ERROR_DEADBAND_DEG) &&
           (HeadingControl_Abs(current_rate_dps) < HEADING_CORRECTION_RATE_GATE_DPS) &&
           (HeadingControl_Abs(output) < HEADING_MIN_CORRECTION_DPS))
  {
    float minimum = (limit < HEADING_MIN_CORRECTION_DPS) ?
                    limit : HEADING_MIN_CORRECTION_DPS;
    output = (heading_state.error_deg > 0.0f) ? minimum : -minimum;
  }

  heading_state.output_dps = output;
  return output;
}

void HeadingControl_SetEnabled(uint8_t enabled)
{
  heading_state.enabled = (enabled != 0U) ? 1U : 0U;
  HeadingControl_Reset();
}

HeadingControlSnapshot HeadingControl_GetSnapshot(void)
{
  return heading_state;
}
