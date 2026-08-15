#include "pid.h"

static float Pid_Clamp(float value, float limit)
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

void Pid_Init(PidController *pid, float kp, float ki, float kd,
              float output_limit, float integral_limit)
{
  if (pid == 0)
  {
    return;
  }

  pid->kp = kp;
  pid->ki = ki;
  pid->kd = kd;
  pid->output_limit = output_limit;
  (void)integral_limit;
  Pid_Reset(pid);
}

void Pid_SetGains(PidController *pid, float kp, float ki, float kd)
{
  if (pid == 0)
  {
    return;
  }

  pid->kp = kp;
  pid->ki = ki;
  pid->kd = kd;
}

void Pid_SetLimits(PidController *pid, float output_limit, float integral_limit)
{
  if (pid == 0)
  {
    return;
  }

  pid->output_limit = output_limit;
  pid->output = Pid_Clamp(pid->output, output_limit);
  /* Keep the legacy argument for the existing command and telemetry format. */
  (void)integral_limit;
}

void Pid_Reset(PidController *pid)
{
  if (pid == 0)
  {
    return;
  }

  pid->output = 0.0f;
  pid->previous_error = 0.0f;
  pid->previous_previous_error = 0.0f;
}

float Pid_Update(PidController *pid, float target, float feedback, float dt_seconds)
{
  float error;
  float increment;

  if (pid == 0)
  {
    return 0.0f;
  }

  (void)dt_seconds;
  error = target - feedback;
  increment = pid->kp * (error - pid->previous_error) +
              pid->ki * error +
              pid->kd * (error - 2.0f * pid->previous_error +
                         pid->previous_previous_error);
  pid->output = Pid_Clamp(pid->output + increment, pid->output_limit);
  pid->previous_previous_error = pid->previous_error;
  pid->previous_error = error;
  return pid->output;
}
