#include "vehicle.h"

#include "encoder.h"
#include "gyro.h"
#include "heading_control.h"
#include "mecanum.h"
#include "speed_control.h"
#include "stm32f4xx_hal.h"
#include "yaw_rate_control.h"

#define VEHICLE_DEFAULT_SPEED       30U
#define VEHICLE_MIN_SPEED            5U
#define VEHICLE_MAX_SPEED          120U
#define VEHICLE_JOG_PWM             15
#define VEHICLE_COMMAND_TIMEOUT_MS  300U
#define VEHICLE_CONTROL_PERIOD_MS    20U

static VehicleMode vehicle_mode;
static uint16_t vehicle_speed;
static uint8_t vehicle_ready;
static uint32_t command_tick;
static uint32_t control_tick;
static int16_t command_forward_rpm;
static int16_t command_left_rpm;
static float command_yaw_rate_dps;
static char wheel_mode_text[3] = "--";

static int16_t Vehicle_Round(float value)
{
  return (value >= 0.0f) ? (int16_t)(value + 0.5f) : (int16_t)(value - 0.5f);
}

static int16_t Vehicle_Clamp(int16_t value, int16_t limit)
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

static uint8_t Vehicle_CanMove(void)
{
  return (vehicle_ready != 0U) && (Motor_IsEnabled() != 0U) &&
         (SpeedControl_IsEnabled() != 0U);
}

static uint8_t Vehicle_IsStationary(void)
{
  uint8_t index;
  SpeedControlSnapshot speed;

  if (vehicle_mode != VEHICLE_STOP)
  {
    return 0U;
  }

  speed = SpeedControl_GetSnapshot();
  for (index = 0U; index < MOTOR_COUNT; ++index)
  {
    if ((speed.rpm[index] > 2) || (speed.rpm[index] < -2))
    {
      return 0U;
    }
  }
  return 1U;
}

static void Vehicle_SetCommandTargets(VehicleMode mode)
{
  int16_t speed = (int16_t)vehicle_speed;

  command_forward_rpm = 0;
  command_left_rpm = 0;
  command_yaw_rate_dps = 0.0f;

  switch (mode)
  {
    case VEHICLE_FORWARD:
      command_forward_rpm = speed;
      break;

    case VEHICLE_BACKWARD:
      command_forward_rpm = -speed;
      break;

    case VEHICLE_LEFT:
      command_left_rpm = speed;
      break;

    case VEHICLE_RIGHT:
      command_left_rpm = -speed;
      break;

    case VEHICLE_ROTATE_LEFT:
      command_yaw_rate_dps = (float)speed;
      break;

    case VEHICLE_ROTATE_RIGHT:
      command_yaw_rate_dps = -(float)speed;
      break;

    default:
      break;
  }
}

static void Vehicle_ControlStep(uint32_t elapsed_ms)
{
  float dt_seconds = (float)elapsed_ms / 1000.0f;
  float target_yaw_rate = command_yaw_rate_dps;
  int16_t rotate_rpm = 0;
  uint8_t heading_ready;
  uint8_t yaw_ready;
  MecanumWheelTargets targets;
  GyroSnapshot gyro;

  Gyro_Process(dt_seconds, Vehicle_IsStationary());
  gyro = Gyro_GetSnapshot();

  if ((vehicle_mode == VEHICLE_STOP) || (vehicle_mode == VEHICLE_WHEEL_TEST))
  {
    HeadingControl_Reset();
    YawRateControl_Reset();
    SpeedControl_Process(elapsed_ms);
    return;
  }

  yaw_ready = (gyro.ready != 0U) &&
              (YawRateControl_GetSnapshot().enabled != 0U);
  heading_ready = yaw_ready && HeadingControl_GetSnapshot().enabled;
  if ((command_yaw_rate_dps != 0.0f) && (yaw_ready == 0U))
  {
    Vehicle_Stop();
    SpeedControl_Process(elapsed_ms);
    return;
  }

  if (yaw_ready != 0U)
  {
    if (command_yaw_rate_dps != 0.0f)
    {
      HeadingControl_Track(gyro.yaw_deg);
    }
    else if (heading_ready != 0U)
    {
      target_yaw_rate = HeadingControl_Update(
        gyro.yaw_deg, gyro.rate_dps, (float)vehicle_speed);
    }
    else
    {
      HeadingControl_Reset();
    }
    rotate_rpm = Vehicle_Round(YawRateControl_Update(
      target_yaw_rate, gyro.rate_dps, dt_seconds, (float)vehicle_speed));
  }
  else
  {
    HeadingControl_Reset();
    YawRateControl_Reset();
  }

  Mecanum_CalculateTargets(command_forward_rpm, command_left_rpm, rotate_rpm,
                           vehicle_speed, &targets);
  SpeedControl_SetTargets(targets.right_rear, targets.left_rear,
                          targets.right_front, targets.left_front);
  SpeedControl_Process(elapsed_ms);
}

uint8_t Vehicle_Init(void)
{
  uint8_t motor_ready = Motor_Init();
  uint8_t encoder_ready = Encoder_Init();

  vehicle_speed = VEHICLE_DEFAULT_SPEED;
  vehicle_mode = VEHICLE_STOP;
  vehicle_ready = motor_ready && encoder_ready;
  SpeedControl_Init();
  YawRateControl_Init();
  HeadingControl_Init();
  (void)Gyro_Init();
  command_tick = HAL_GetTick();
  control_tick = command_tick;
  Vehicle_SetCommandTargets(VEHICLE_STOP);
  SpeedControl_Stop();
  return vehicle_ready;
}

void Vehicle_Process(void)
{
  uint32_t now = HAL_GetTick();
  uint32_t elapsed_ms;

  if ((vehicle_mode != VEHICLE_STOP) && (Motor_IsEnabled() == 0U))
  {
    Vehicle_Stop();
    return;
  }

  if ((vehicle_mode != VEHICLE_STOP) &&
      ((now - command_tick) > VEHICLE_COMMAND_TIMEOUT_MS))
  {
    Vehicle_Stop();
  }

  elapsed_ms = now - control_tick;
  if (elapsed_ms < VEHICLE_CONTROL_PERIOD_MS)
  {
    return;
  }
  control_tick = now;
  Vehicle_ControlStep(elapsed_ms);
}

uint8_t Vehicle_Move(VehicleMode mode)
{
  if ((mode < VEHICLE_FORWARD) || (mode > VEHICLE_ROTATE_RIGHT))
  {
    return 0U;
  }

  if (Vehicle_CanMove() == 0U)
  {
    Vehicle_Stop();
    return 0U;
  }
  if (((mode == VEHICLE_ROTATE_LEFT) || (mode == VEHICLE_ROTATE_RIGHT)) &&
      ((Gyro_GetSnapshot().ready == 0U) ||
       (YawRateControl_GetSnapshot().enabled == 0U)))
  {
    return 0U;
  }

  if (vehicle_mode != mode)
  {
    SpeedControl_Reset();
    YawRateControl_Reset();
    HeadingControl_Reset();
  }
  vehicle_mode = mode;
  command_tick = HAL_GetTick();
  Vehicle_SetCommandTargets(mode);
  return 1U;
}

uint8_t Vehicle_Drive(int16_t forward_rpm, int16_t left_rpm, int16_t yaw_rate_dps)
{
  int16_t limit = (int16_t)vehicle_speed;

  if ((forward_rpm > limit) || (forward_rpm < -limit) ||
      (left_rpm > limit) || (left_rpm < -limit) ||
      (yaw_rate_dps > limit) || (yaw_rate_dps < -limit))
  {
    return 0U;
  }
  if ((forward_rpm == 0) && (left_rpm == 0) && (yaw_rate_dps == 0))
  {
    Vehicle_Stop();
    return 1U;
  }
  if (Vehicle_CanMove() == 0U)
  {
    Vehicle_Stop();
    return 0U;
  }
  if ((yaw_rate_dps != 0) &&
      ((Gyro_GetSnapshot().ready == 0U) ||
       (YawRateControl_GetSnapshot().enabled == 0U)))
  {
    return 0U;
  }

  if (vehicle_mode != VEHICLE_DRIVE)
  {
    SpeedControl_Reset();
    YawRateControl_Reset();
    HeadingControl_Reset();
  }
  command_forward_rpm = forward_rpm;
  command_left_rpm = left_rpm;
  command_yaw_rate_dps = (float)yaw_rate_dps;
  vehicle_mode = VEHICLE_DRIVE;
  command_tick = HAL_GetTick();
  return 1U;
}

uint8_t Vehicle_StepHeading(int16_t delta_deg)
{
  GyroSnapshot gyro = Gyro_GetSnapshot();

  if ((vehicle_mode != VEHICLE_DRIVE) || (command_yaw_rate_dps != 0.0f))
  {
    return 0U;
  }
  if ((gyro.ready == 0U) ||
      (YawRateControl_GetSnapshot().enabled == 0U) ||
      (HeadingControl_GetSnapshot().enabled == 0U))
  {
    return 0U;
  }
  if (HeadingControl_StepTarget(gyro.yaw_deg, (float)delta_deg) == 0U)
  {
    return 0U;
  }

  command_tick = HAL_GetTick();
  return 1U;
}

uint8_t Vehicle_HoldHeading(void)
{
  GyroSnapshot gyro = Gyro_GetSnapshot();

  if ((Vehicle_CanMove() == 0U) || (gyro.ready == 0U) ||
      (YawRateControl_GetSnapshot().enabled == 0U) ||
      (HeadingControl_GetSnapshot().enabled == 0U))
  {
    return 0U;
  }

  if (vehicle_mode != VEHICLE_HEADING_HOLD)
  {
    SpeedControl_Reset();
    YawRateControl_Reset();
    Gyro_ZeroYaw();
    Vehicle_SetCommandTargets(VEHICLE_HEADING_HOLD);
    if (HeadingControl_HoldTarget(0.0f, 0.0f) == 0U)
    {
      Vehicle_Stop();
      return 0U;
    }
    vehicle_mode = VEHICLE_HEADING_HOLD;
  }

  command_tick = HAL_GetTick();
  return 1U;
}

uint8_t Vehicle_Jog(MotorId id, int8_t direction)
{
  if ((id >= MOTOR_COUNT) || ((direction != 1) && (direction != -1)))
  {
    return 0U;
  }

  if ((vehicle_ready == 0U) || (Motor_IsEnabled() == 0U))
  {
    Vehicle_Stop();
    return 0U;
  }

  SpeedControl_Stop();
  YawRateControl_Reset();
  HeadingControl_Reset();
  Vehicle_SetCommandTargets(VEHICLE_WHEEL_TEST);
  Motor_Set(id, VEHICLE_JOG_PWM * direction);
  vehicle_mode = VEHICLE_WHEEL_TEST;
  wheel_mode_text[0] = (char)('A' + (uint8_t)id);
  wheel_mode_text[1] = (direction > 0) ? '+' : '-';
  wheel_mode_text[2] = '\0';
  command_tick = HAL_GetTick();
  return 1U;
}

void Vehicle_Stop(void)
{
  Vehicle_SetCommandTargets(VEHICLE_STOP);
  YawRateControl_Reset();
  HeadingControl_Reset();
  SpeedControl_Stop();
  vehicle_mode = VEHICLE_STOP;
}

uint8_t Vehicle_SetSpeed(uint16_t rpm)
{
  if ((rpm < VEHICLE_MIN_SPEED) || (rpm > VEHICLE_MAX_SPEED))
  {
    return 0U;
  }

  vehicle_speed = rpm;
  if ((vehicle_mode >= VEHICLE_FORWARD) &&
      (vehicle_mode <= VEHICLE_ROTATE_RIGHT))
  {
    Vehicle_SetCommandTargets(vehicle_mode);
  }
  else if (vehicle_mode == VEHICLE_DRIVE)
  {
    int16_t limit = (int16_t)vehicle_speed;
    command_forward_rpm = Vehicle_Clamp(command_forward_rpm, limit);
    command_left_rpm = Vehicle_Clamp(command_left_rpm, limit);
    command_yaw_rate_dps = (float)Vehicle_Clamp(
      Vehicle_Round(command_yaw_rate_dps), limit);
  }
  return 1U;
}

uint16_t Vehicle_GetSpeed(void)
{
  return vehicle_speed;
}

uint8_t Vehicle_IsReady(void)
{
  return vehicle_ready;
}

const char *Vehicle_GetModeText(void)
{
  static const char *mode_text[] = {
    "STOP", "W", "S", "A", "D", "Q", "E", "DRV", "HOLD", "JOG"
  };

  if (vehicle_mode == VEHICLE_WHEEL_TEST)
  {
    return wheel_mode_text;
  }
  return mode_text[vehicle_mode];
}
