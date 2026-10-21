#include "speed_control.h"

#include <math.h>
#include <string.h>

#include "chassis_test.h"
#include "encoder.h"
#include "mecanum.h"
#include "motor.h"
#include "pid.h"
#include "front_guard.h"
#include "telemetry.h"
#include "timebase.h"
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
static uint8_t drive_mode;
static int16_t requested_forward, requested_left, requested_rotation;
static int32_t requested_wheel[4];
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
    speed_state.output_mpercent[index] = 0;
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
  drive_mode = 0U;
  memset(requested_wheel, 0, sizeof(requested_wheel));
}

void SpeedControl_Stop(void)
{
  uint32_t index;

  speed_state.enabled = 0U;
  speed_state.saturated_mask = speed_state.tof_limited = 0U;
  memset(requested_wheel, 0, sizeof(requested_wheel));
  requested_forward = requested_left = requested_rotation = 0;
  drive_mode = 0U;
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
  if (drive_mode) memcpy(requested_wheel, speed_state.target_rpm, sizeof(requested_wheel));
  drive_mode = 0U;
  requested_wheel[(uint32_t)channel] = SpeedControl_ClampTarget(rpm);
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
  requested_forward = forward_rpm;
  requested_left = left_rpm;
  requested_rotation = rotate_rpm;
  drive_mode = 1U;
  /* Rotate the body frame by 180 degrees. Hardware channel and encoder
   * polarities stay physical; positive yaw remains CCW. */
  Mecanum_CalculateTargets((int16_t)-forward_rpm, (int16_t)-left_rpm, rotate_rpm,
                           speed_state.target_limit_rpm, &targets);
  speed_state.target_rpm[0] = targets.motor1_right_front;
  speed_state.target_rpm[1] = targets.motor2_left_front;
  speed_state.target_rpm[2] = targets.motor3_left_rear;
  speed_state.target_rpm[3] = targets.motor4_right_rear;
  last_drive_tick = HAL_GetTick();
  drive_watchdog_enabled = 1U;
  return 1U;
}

uint8_t SpeedControl_KeepAlive(void)
{
  if (!speed_state.enabled || drive_mode) return 0U;
  last_drive_tick = HAL_GetTick();
  drive_watchdog_enabled = 1U;
  return 1U;
}

static void apply_front_guard(void)
{
  uint16_t limit = FrontGuard_ForwardLimit(speed_state.target_limit_rpm);
  uint32_t i;
  speed_state.tof_limited = 0U;
  if (drive_mode)
  {
    MecanumWheelTargets targets;
    int16_t forward = requested_forward;
    if (forward > (int16_t)limit)
    {
      forward = (int16_t)limit;
      speed_state.tof_limited = 1U;
    }
    Mecanum_CalculateTargets((int16_t)-forward, (int16_t)-requested_left,
        requested_rotation, speed_state.target_limit_rpm, &targets);
    speed_state.target_rpm[0] = targets.motor1_right_front;
    speed_state.target_rpm[1] = targets.motor2_left_front;
    speed_state.target_rpm[2] = targets.motor3_left_rear;
    speed_state.target_rpm[3] = targets.motor4_right_rear;
    return;
  }
  /* Raw single-wheel tests keep channel isolation. Scale their entire
   * requested vector rather than starting the other three wheels. */
  {
    int32_t forward_x4 = -(requested_wheel[0] + requested_wheel[1] +
                            requested_wheel[2] + requested_wheel[3]);
    speed_state.tof_limited = forward_x4 > (int32_t)limit * 4;
    for (i = 0U; i < 4U; ++i)
      speed_state.target_rpm[i] = speed_state.tof_limited ?
          requested_wheel[i] * (int32_t)limit * 4 / forward_x4 :
          SpeedControl_ClampTarget(requested_wheel[i]);
  }
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
    requested_wheel[index] = SpeedControl_ClampTarget(requested_wheel[index]);
  }
  return 1U;
}

void SpeedControl_Process(uint32_t elapsed_ms)
{
  EncoderSnapshot encoder;
  ChassisTestSnapshot chassis;
  uint32_t now;
  uint32_t index;
  uint32_t tick_ms, tick_us;

  if (elapsed_ms < SPEED_CONTROL_PERIOD_MS)
  {
    return;
  }
  chassis = ChassisTest_GetSnapshot();
  if (speed_state.enabled && chassis.armed == 0U)
  {
    SpeedControl_Stop();
  }
  now = HAL_GetTick();
  if ((drive_watchdog_enabled != 0U) &&
      ((now - last_drive_tick) > SPEED_CONTROL_WATCHDOG_MS))
  {
    for (index = 0U; index < SPEED_CONTROL_CHANNEL_COUNT; ++index)
    {
      speed_state.target_rpm[index] = 0;
      requested_wheel[index] = 0;
    }
  }
  if (drive_watchdog_enabled && now - last_drive_tick > SPEED_CONTROL_WATCHDOG_MS)
    requested_forward = requested_left = requested_rotation = 0;
  if (speed_state.enabled) apply_front_guard();
  Timebase_Capture(&tick_ms, &tick_us);
  speed_state.saturated_mask = 0U;
  encoder = Encoder_GetSnapshot();
  /* A watchdog-tracked single-wheel trial must stop on gross feedback fault.
   * This is an abort guard, not a new tunable operating ceiling. */
  if (speed_state.enabled && drive_watchdog_enabled && !drive_mode)
  {
    for (index = 0U; index < 4U; ++index)
      if (encoder.rpm_x10[index] > 3000 || encoder.rpm_x10[index] < -3000)
      {
        ChassisTest_Stop();
        break;
      }
  }
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
    speed_state.output_mpercent[index] = 0;
    if (!speed_state.enabled)
    {
      /* Disabled-loop logging observes feedback only. A timed raw JOG may
       * own the motor outputs; do not overwrite it from the logger path. */
      Pid_Reset(&speed_pid[index]);
      speed_state.output_percent[index] = 0;
      continue;
    }
    if (fabsf(target) < SPEED_CONTROL_STOP_EPSILON)
    {
      Pid_Reset(&speed_pid[index]);
      speed_state.output_percent[index] = 0;
      Motor_Stop((MotorChannel)index);
      continue;
    }
    output = Pid_Update(&speed_pid[index], target, actual,
                        (float)elapsed_ms / 1000.0f);
    if (fabsf(output) >= SPEED_CONTROL_OUTPUT_LIMIT - 0.001f)
      speed_state.saturated_mask |= (uint8_t)(1U << index);
    speed_state.output_mpercent[index] = (int32_t)lroundf(output * 1000.0f);
    direction = (output >= 0.0f) ? 1 : -1;
    duty = (uint8_t)(fabsf(output) + 0.5f);
    if (duty == 0U)
    {
      Motor_Stop((MotorChannel)index);
    }
    else if (Motor_Run((MotorChannel)index, direction, duty) == 0U)
    {
      speed_state.output_percent[index] = 0;
      speed_state.output_mpercent[index] = 0;
      continue;
    }
    speed_state.output_percent[index] =
        (output >= 0.0f) ? (int32_t)(output + 0.5f) :
                           (int32_t)(output - 0.5f);
    speed_state.output_mpercent[index] = speed_state.output_percent[index] * 1000;
  }
  Telemetry_Wlog_Record(&speed_state, &encoder, tick_ms, tick_us);
}

SpeedControlSnapshot SpeedControl_GetSnapshot(void)
{
  SpeedControlSnapshot snapshot = speed_state;
  ChassisTestSnapshot chassis = ChassisTest_GetSnapshot();

  snapshot.armed = chassis.armed;
  return snapshot;
}
