#include "speed_control.h"

#include "encoder.h"
#include "pid.h"
#define SPEED_CONTROL_DEFAULT_KP         0.8f
#define SPEED_CONTROL_DEFAULT_KI         0.1f
#define SPEED_CONTROL_DEFAULT_KD         0.0f
#define SPEED_CONTROL_DEFAULT_OUTPUT     60U
#define SPEED_CONTROL_DEFAULT_INTEGRAL   30U

static const float encoder_counts_per_rev[MOTOR_COUNT] =
{
  1560.3f, 1557.4f, 1562.6f, 1557.6f
};

static PidController wheel_pid[MOTOR_COUNT];
static SpeedControlConfig control_config;
static SpeedControlSnapshot control_snapshot;
static float filtered_rpm[MOTOR_COUNT];
static uint8_t control_active;

static int16_t SpeedControl_Round(float value)
{
  if (value >= 0.0f)
  {
    return (int16_t)(value + 0.5f);
  }
  return (int16_t)(value - 0.5f);
}

void SpeedControl_Init(void)
{
  uint8_t index;

  control_config.kp = SPEED_CONTROL_DEFAULT_KP;
  control_config.ki = SPEED_CONTROL_DEFAULT_KI;
  control_config.kd = SPEED_CONTROL_DEFAULT_KD;
  control_config.output_limit = SPEED_CONTROL_DEFAULT_OUTPUT;
  control_config.integral_limit = SPEED_CONTROL_DEFAULT_INTEGRAL;
  control_config.enabled = 1U;
  control_active = 0U;
  for (index = 0U; index < MOTOR_COUNT; ++index)
  {
    Pid_Init(&wheel_pid[index], control_config.kp, control_config.ki, control_config.kd,
             (float)control_config.output_limit, (float)control_config.integral_limit);
    control_snapshot.rpm[index] = 0;
    control_snapshot.target_rpm[index] = 0;
    filtered_rpm[index] = 0.0f;
  }
}

void SpeedControl_Process(uint32_t elapsed_ms)
{
  uint8_t index;
  int32_t delta[MOTOR_COUNT];
  float raw_rpm;
  float output;

  if (elapsed_ms == 0U)
  {
    return;
  }
  Encoder_Sample(delta);
  for (index = 0U; index < MOTOR_COUNT; ++index)
  {
    raw_rpm = ((float)delta[index] * 60000.0f) /
              (encoder_counts_per_rev[index] * (float)elapsed_ms);
    filtered_rpm[index] = filtered_rpm[index] * 0.7f + raw_rpm * 0.3f;
    control_snapshot.rpm[index] = SpeedControl_Round(filtered_rpm[index]);
  }

  if ((control_config.enabled == 0U) || (control_active == 0U))
  {
    return;
  }

  for (index = 0U; index < MOTOR_COUNT; ++index)
  {
    if (control_snapshot.target_rpm[index] == 0)
    {
      Pid_Reset(&wheel_pid[index]);
      Motor_Set((MotorId)index, 0);
      continue;
    }

    output = Pid_Update(&wheel_pid[index], (float)control_snapshot.target_rpm[index],
                        filtered_rpm[index], (float)elapsed_ms / 1000.0f);
    Motor_Set((MotorId)index, SpeedControl_Round(output));
  }
}

void SpeedControl_SetTargets(int16_t a, int16_t b, int16_t c, int16_t d)
{
  control_snapshot.target_rpm[MOTOR_A] = a;
  control_snapshot.target_rpm[MOTOR_B] = b;
  control_snapshot.target_rpm[MOTOR_C] = c;
  control_snapshot.target_rpm[MOTOR_D] = d;
  control_active = 1U;
}

void SpeedControl_Stop(void)
{
  uint8_t index;

  control_active = 0U;
  for (index = 0U; index < MOTOR_COUNT; ++index)
  {
    control_snapshot.target_rpm[index] = 0;
    Pid_Reset(&wheel_pid[index]);
  }
  Motor_StopAll();
}

void SpeedControl_Reset(void)
{
  uint8_t index;

  for (index = 0U; index < MOTOR_COUNT; ++index)
  {
    Pid_Reset(&wheel_pid[index]);
  }
}

void SpeedControl_SetEnabled(uint8_t enabled)
{
  control_config.enabled = (enabled != 0U) ? 1U : 0U;
  if (control_config.enabled == 0U)
  {
    SpeedControl_Stop();
  }
}

uint8_t SpeedControl_IsEnabled(void)
{
  return control_config.enabled;
}

SpeedControlConfig SpeedControl_GetConfig(void)
{
  return control_config;
}

SpeedControlSnapshot SpeedControl_GetSnapshot(void)
{
  return control_snapshot;
}
