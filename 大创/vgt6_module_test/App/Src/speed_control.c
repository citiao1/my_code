#include "speed_control.h"

#include <math.h>
#include <string.h>

#include "chassis_test.h"
#include "encoder.h"
#include "mecanum.h"
#include "motor.h"
#include "pid.h"
#include "stm32f4xx_hal.h"

#define SPEED_CONTROL_PERIOD_MS       10U
#define SPEED_CONTROL_DEFAULT_LIMIT   160U
#define SPEED_CONTROL_OUTPUT_LIMIT    60.0f
#define SPEED_CONTROL_WATCHDOG_MS     300U
#define SPEED_CONTROL_STOP_EPSILON    0.05f

static SpeedControlSnapshot speed_state;
static PidController speed_pid[SPEED_CONTROL_CHANNEL_COUNT];
static float filtered_rpm[SPEED_CONTROL_CHANNEL_COUNT];
static uint32_t last_drive_tick;
static uint8_t drive_watchdog_enabled;
static const float default_ki[4] = {0.0991f, 0.0998f, 0.0998f, 0.1008f};

static uint8_t SpeedControl_IsValid(MotorChannel channel)
{
  return ((uint32_t)channel < SPEED_CONTROL_CHANNEL_COUNT) ? 1U : 0U;
}

static int32_t SpeedControl_ClampTarget(int32_t rpm)
{
  int32_t limit = (int32_t)speed_state.target_limit_rpm;

  if (rpm > limit)
  {
    return limit;
  }
  if (rpm < -limit)
  {
    return -limit;
  }
  return rpm;
}

static void SpeedControl_ResetControllers(void)
{
  uint32_t index;

  for (index = 0U; index < SPEED_CONTROL_CHANNEL_COUNT; ++index)
  {
    Pid_Reset(&speed_pid[index]);
    speed_state.output_percent[index] = 0;
  }
}

void SpeedControl_Init(void)
{
  uint32_t index;

  memset(&speed_state, 0, sizeof(speed_state));
  speed_state.target_limit_rpm = SPEED_CONTROL_DEFAULT_LIMIT;
  for (index = 0U; index < SPEED_CONTROL_CHANNEL_COUNT; ++index)
  {
    speed_state.kp[index] = SPEED_CONTROL_DEFAULT_KP;
    speed_state.ki[index] = default_ki[index];
    speed_state.kd[index] = SPEED_CONTROL_DEFAULT_KD;
    Pid_Init(&speed_pid[index], speed_state.kp[index],
             speed_state.ki[index], speed_state.kd[index],
             SPEED_CONTROL_OUTPUT_LIMIT, 0.0f);
    filtered_rpm[index] = 0.0f;
  }
  last_drive_tick = HAL_GetTick();
  drive_watchdog_enabled = 0U;
}

void SpeedControl_Stop(void)
{
  uint32_t index;

  speed_state.enabled = 0U;
  drive_watchdog_enabled = 0U;
  for (index = 0U; index < SPEED_CONTROL_CHANNEL_COUNT; ++index)
  {
    speed_state.target_rpm[index] = 0;
  }
  SpeedControl_ResetControllers();
  memset(filtered_rpm, 0, sizeof(filtered_rpm));
  Motor_StopAll();
}

uint8_t SpeedControl_Enable(uint8_t enable)
{
  ChassisTestSnapshot chassis;

  if (enable == 0U)
  {
    SpeedControl_Stop();
    return 1U;
  }
  chassis = ChassisTest_GetSnapshot();
  if ((chassis.armed == 0U) || (chassis.motor_ready == 0U))
  {
    return 0U;
  }
  Motor_StopAll();
  speed_state.enabled = 1U;
  speed_state.armed = 1U;
  last_drive_tick = HAL_GetTick();
  drive_watchdog_enabled = 0U;
  SpeedControl_ResetControllers();
  memset(filtered_rpm, 0, sizeof(filtered_rpm));
  return 1U;
}

uint8_t SpeedControl_SetTarget(MotorChannel channel, int32_t rpm)
{
  if (SpeedControl_IsValid(channel) == 0U)
  {
    return 0U;
  }
  speed_state.target_rpm[(uint32_t)channel] = SpeedControl_ClampTarget(rpm);
  last_drive_tick = HAL_GetTick();
  drive_watchdog_enabled = 0U;
  return 1U;
}

uint8_t SpeedControl_SetDrive(int16_t forward_rpm,
                              int16_t left_rpm,
                              int16_t rotate_rpm)
{
  MecanumWheelTargets targets;

  if (speed_state.enabled == 0U)
  {
    return 0U;
  }
  Mecanum_CalculateTargets(forward_rpm, left_rpm, rotate_rpm,
                           speed_state.target_limit_rpm, &targets);
  speed_state.target_rpm[0] = targets.motor1_right_front;
  speed_state.target_rpm[1] = targets.motor2_left_front;
  speed_state.target_rpm[2] = targets.motor3_left_rear;
  speed_state.target_rpm[3] = targets.motor4_right_rear;
  last_drive_tick = HAL_GetTick();
  drive_watchdog_enabled = 1U;
  return 1U;
}

uint8_t SpeedControl_SetPid(MotorChannel channel, float kp, float ki,
                            float kd)
{
  if ((SpeedControl_IsValid(channel) == 0U) ||
      (!isfinite(kp)) || (!isfinite(ki)) || (!isfinite(kd)) ||
      (kp < 0.0f) || (kp > SPEED_CONTROL_MAX_KP) ||
      (ki < 0.0f) || (ki > SPEED_CONTROL_MAX_KI) ||
      (kd < 0.0f) || (kd > SPEED_CONTROL_MAX_KD))
  {
    return 0U;
  }

  speed_state.kp[(uint32_t)channel] = kp;
  speed_state.ki[(uint32_t)channel] = ki;
  speed_state.kd[(uint32_t)channel] = kd;
  Pid_SetGains(&speed_pid[(uint32_t)channel], kp, ki, kd);
  Pid_Reset(&speed_pid[(uint32_t)channel]);
  speed_state.output_percent[(uint32_t)channel] = 0;
  return 1U;
}

uint8_t SpeedControl_SetTargetLimit(uint16_t limit_rpm)
{
  uint32_t index;

  if ((limit_rpm == 0U) || (limit_rpm > SPEED_CONTROL_MAX_TARGET_RPM))
  {
    return 0U;
  }
  speed_state.target_limit_rpm = limit_rpm;
  for (index = 0U; index < SPEED_CONTROL_CHANNEL_COUNT; ++index)
  {
    speed_state.target_rpm[index] =
        SpeedControl_ClampTarget(speed_state.target_rpm[index]);
  }
  return 1U;
}

void SpeedControl_Process(uint32_t elapsed_ms)
{
  EncoderSnapshot encoder;
  ChassisTestSnapshot chassis;
  uint32_t now;
  uint32_t index;

  if ((elapsed_ms < SPEED_CONTROL_PERIOD_MS) ||
      (speed_state.enabled == 0U))
  {
    return;
  }
  chassis = ChassisTest_GetSnapshot();
  if (chassis.armed == 0U)
  {
    SpeedControl_Stop();
    return;
  }
  now = HAL_GetTick();
  if ((drive_watchdog_enabled != 0U) &&
      ((now - last_drive_tick) > SPEED_CONTROL_WATCHDOG_MS))
  {
    for (index = 0U; index < SPEED_CONTROL_CHANNEL_COUNT; ++index)
    {
      speed_state.target_rpm[index] = 0;
    }
  }
  encoder = Encoder_GetSnapshot();
  for (index = 0U; index < SPEED_CONTROL_CHANNEL_COUNT; ++index)
  {
    float target = (float)speed_state.target_rpm[index];
    float raw_rpm = (float)encoder.rpm_x10[index] / 10.0f;
    float actual;
    float output;
    int8_t direction;
    uint8_t duty;

    /*
     * BM50's 380-count encoder produces a coarse 10 ms speed sample.
     * Match the reference speed loop's first-order low-pass filter before
     * calculating the inner-loop error.
     */
    filtered_rpm[index] = filtered_rpm[index] * 0.7f + raw_rpm * 0.3f;
    actual = filtered_rpm[index];
    speed_state.actual_rpm[index] =
        (int32_t)((actual >= 0.0f) ? (actual + 0.5f) : (actual - 0.5f));
    speed_state.actual_rpm_x10[index] =
        (int32_t)((actual >= 0.0f) ? (actual * 10.0f + 0.5f) :
                                      (actual * 10.0f - 0.5f));
    if (fabsf(target) < SPEED_CONTROL_STOP_EPSILON)
    {
      Pid_Reset(&speed_pid[index]);
      speed_state.output_percent[index] = 0;
      Motor_Stop((MotorChannel)index);
      continue;
    }
    output = Pid_Update(&speed_pid[index], target, actual,
                        (float)elapsed_ms / 1000.0f);
    direction = (output >= 0.0f) ? 1 : -1;
    duty = (uint8_t)(fabsf(output) + 0.5f);
    if (duty == 0U)
    {
      Motor_Stop((MotorChannel)index);
    }
    else if (Motor_Run((MotorChannel)index, direction, duty) == 0U)
    {
      speed_state.output_percent[index] = 0;
      continue;
    }
    speed_state.output_percent[index] =
        (output >= 0.0f) ? (int32_t)(output + 0.5f) :
                           (int32_t)(output - 0.5f);
  }
}

SpeedControlSnapshot SpeedControl_GetSnapshot(void)
{
  SpeedControlSnapshot snapshot = speed_state;
  ChassisTestSnapshot chassis = ChassisTest_GetSnapshot();

  snapshot.armed = chassis.armed;
  return snapshot;
}
