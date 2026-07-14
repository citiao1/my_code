#include "vehicle_control.h"

#include "vehicle_battery.h"
#include "vehicle_config.h"
#include "vehicle_imu.h"
#include "vehicle_internal.h"
#include "vehicle_line.h"
#include "vehicle_motor.h"

#include <math.h>

static uint32_t last_battery_ms;

float Vehicle_WrapAngle(float angle)
{
  while (angle > 180.0f) angle -= 360.0f;
  while (angle < -180.0f) angle += 360.0f;
  return angle;
}

void VehicleControl_InitDefaults(void)
{
  pid_left.kp = pid_right.kp = DEFAULT_PID_KP;
  pid_left.ki = pid_right.ki = DEFAULT_PID_KI;
  pid_left.kd = pid_right.kd = DEFAULT_PID_KD;
  state.max_speed = MOTOR_SPEED_LIMIT_MPS;
  pid_yaw.kp = DEFAULT_YAW_KP;
  pid_yaw.ki = DEFAULT_YAW_KI;
  pid_yaw.kd = DEFAULT_YAW_KD;
  pid_yaw.kff = DEFAULT_YAW_KFF;
  pid_heading.kp = DEFAULT_HEADING_KP;
  pid_heading.kd = DEFAULT_HEADING_KD;
  pid_heading.kff = DEFAULT_HEADING_KFF;
  state.max_yaw_rate = DEFAULT_MAX_YAW_RATE_DPS;
  state.max_heading_yaw_rate = DEFAULT_HEADING_MAX_RATE_DPS;
  state.yaw_control_enabled = 0U;
  state.heading_control_enabled = 0U;
}

void VehicleControl_EnableDefaultLoops(void)
{
  state.yaw_control_enabled = state.mpu_ok ? 1U : 0U;
  state.heading_control_enabled = state.mpu_ok ? 1U : 0U;
  state.target_heading = state.yaw;
  state.heading_reference = state.yaw;
  state.heading_reference_rate = 0.0f;
  state.heading_error = 0.0f;
  state.heading_output = 0.0f;
  state.heading_hold_active = 0U;
}

static void ApplySpeedControl(uint32_t now)
{
  float base_speed;
  float correction;
  float left_target;
  float right_target;
  float peak;
  uint8_t line_mode_active = VehicleLine_IsControlActive();
  uint8_t heading_mode_active = state.heading_control_enabled &&
                                state.yaw_control_enabled &&
                                state.mpu_ok &&
                                state.heading_hold_active &&
                                state.steering == 0;

  if (!VehicleMotor_IsEnabled() || (!state.link_active && !line_mode_active) ||
      (state.throttle == 0 && state.steering == 0 && !heading_mode_active))
  {
    state.target_left = 0.0f;
    state.target_right = 0.0f;
    state.error_left = 0.0f;
    state.error_right = 0.0f;
    state.target_yaw_rate = 0.0f;
    VehicleMotor_Stop();
    return;
  }

  if (VehicleLine_IsRunning())
  {
    state.target_yaw_rate = VehicleLine_GetTargetYawRate();
    state.heading_reference = state.yaw;
    state.heading_reference_rate = 0.0f;
    state.heading_error = 0.0f;
    state.heading_output = 0.0f;
    state.heading_hold_active = 0U;
  }
  else if (state.heading_control_enabled && state.yaw_control_enabled && state.mpu_ok &&
      state.steering == 0 && (state.throttle != 0 || state.heading_hold_active))
  {
    if (!state.heading_hold_active)
    {
      state.target_heading = state.yaw;
      state.heading_reference = state.yaw;
      state.heading_reference_rate = 0.0f;
      state.heading_error = 0.0f;
      state.heading_output = 0.0f;
      state.heading_hold_active = 1U;
      last_heading_ms = now;
    }
    if ((now - last_heading_ms) >= HEADING_PERIOD_MS)
    {
      float dt = (float)(now - last_heading_ms) / 1000.0f;
      float output_limit = fminf(state.max_heading_yaw_rate, state.max_yaw_rate);
      float reference_error;
      float reference_step;
      float tracking_error;
      float derivative_error;
      last_heading_ms = now;

      if (dt <= 0.0f || dt > 0.2f) dt = (float)HEADING_PERIOD_MS / 1000.0f;
      state.heading_error = Vehicle_WrapAngle(state.target_heading - state.yaw);
      reference_error = Vehicle_WrapAngle(state.target_heading - state.heading_reference);
      reference_step = output_limit * dt;
      if (fabsf(reference_error) <= reference_step)
      {
        state.heading_reference_rate = reference_error / dt;
        state.heading_reference = state.target_heading;
      }
      else
      {
        state.heading_reference_rate = reference_error > 0.0f ? output_limit : -output_limit;
        state.heading_reference = Vehicle_WrapAngle(state.heading_reference + state.heading_reference_rate * dt);
      }

      tracking_error = Vehicle_WrapAngle(state.heading_reference - state.yaw);
      derivative_error = state.heading_reference_rate - state.yaw_rate;
      state.heading_output = pid_heading.kff * state.heading_reference_rate +
                             pid_heading.kp * tracking_error +
                             pid_heading.kd * derivative_error;
      if (state.heading_output > output_limit) state.heading_output = output_limit;
      if (state.heading_output < -output_limit) state.heading_output = -output_limit;
      if (fabsf(Vehicle_WrapAngle(state.target_heading - state.heading_reference)) < 0.01f &&
          fabsf(state.heading_error) > HEADING_CORRECTION_DEADBAND_DEG &&
          fabsf(state.yaw_rate) < HEADING_CORRECTION_RATE_GATE_DPS &&
          fabsf(state.heading_output) < HEADING_MIN_CORRECTION_DPS)
      {
        float minimum_correction = fminf(HEADING_MIN_CORRECTION_DPS, output_limit);
        state.heading_output = state.heading_error > 0.0f ? minimum_correction : -minimum_correction;
      }
    }
    state.target_yaw_rate = state.heading_output;
  }
  else
  {
    state.target_yaw_rate = (float)state.steering * state.max_yaw_rate / 100.0f;
    state.heading_reference = state.yaw;
    state.heading_reference_rate = 0.0f;
    state.heading_error = 0.0f;
    state.heading_output = 0.0f;
    if (state.steering != 0 || state.throttle == 0) state.heading_hold_active = 0U;
  }

  base_speed = (float)state.throttle * state.max_speed / 100.0f;
  if (state.yaw_control_enabled && state.mpu_ok)
  {
    float error = state.target_yaw_rate - state.yaw_rate;
    float candidate_integral = pid_yaw.integral + error * 0.01f;
    float derivative = (error - pid_yaw.previous_error) / 0.01f;

    if (candidate_integral > YAW_INTEGRAL_LIMIT) candidate_integral = YAW_INTEGRAL_LIMIT;
    if (candidate_integral < -YAW_INTEGRAL_LIMIT) candidate_integral = -YAW_INTEGRAL_LIMIT;
    state.yaw_feedforward = pid_yaw.kff * state.target_yaw_rate;
    correction = state.yaw_feedforward + pid_yaw.kp * error +
                 pid_yaw.ki * candidate_integral + pid_yaw.kd * derivative;
    if (correction > state.max_speed) correction = state.max_speed;
    if (correction < -state.max_speed) correction = -state.max_speed;
    if (fabsf(correction) < state.max_speed || correction * error < 0.0f)
    {
      pid_yaw.integral = candidate_integral;
    }
    pid_yaw.previous_error = error;
    state.yaw_error = error;
    state.yaw_correction = correction;
  }
  else
  {
    correction = (float)state.steering * state.max_speed / 100.0f;
    state.yaw_feedforward = 0.0f;
    pid_yaw.integral = 0.0f;
    pid_yaw.previous_error = 0.0f;
    state.yaw_error = state.target_yaw_rate - state.yaw_rate;
    state.yaw_correction = correction;
  }

  left_target = base_speed - correction;
  right_target = base_speed + correction;
  peak = fmaxf(fabsf(left_target), fabsf(right_target));
  if (peak > state.max_speed)
  {
    left_target *= state.max_speed / peak;
    right_target *= state.max_speed / peak;
  }

  state.target_left = left_target;
  state.target_right = right_target;
  VehicleMotor_SetLeftPwm(VehicleMotor_SpeedPidStep(&pid_left, state.target_left,
                                                    state.speed_left, &state.error_left));
  VehicleMotor_SetRightPwm(VehicleMotor_SpeedPidStep(&pid_right, state.target_right,
                                                     state.speed_right, &state.error_right));
}

static void SquareSetHeadingTarget(float target, uint32_t now)
{
  state.target_heading = Vehicle_WrapAngle(target);
  state.heading_reference = state.yaw;
  state.heading_reference_rate = 0.0f;
  state.heading_error = Vehicle_WrapAngle(state.target_heading - state.yaw);
  state.heading_output = 0.0f;
  state.heading_hold_active = 1U;
  last_heading_ms = now;
}

static void SquareBeginDrive(uint32_t now)
{
  square_test.start_left = state.encoder_left;
  square_test.start_right = state.encoder_right;
  square_test.progress_m = 0.0f;
  square_test.phase = SQUARE_PHASE_DRIVE;
  square_test.phase_start_ms = now;
  square_test.settle_start_ms = 0U;
  state.steering = 0;
  SquareSetHeadingTarget(square_test.baseline_heading +
                         (float)square_test.direction * 90.0f * (float)square_test.leg, now);
}

static void SquareBeginTurn(uint32_t now)
{
  state.throttle = 0;
  state.steering = 0;
  square_test.phase = SQUARE_PHASE_TURN;
  square_test.phase_start_ms = now;
  square_test.settle_start_ms = 0U;
  SquareSetHeadingTarget(square_test.baseline_heading +
                         (float)square_test.direction * 90.0f * (float)(square_test.leg + 1U), now);
}

void VehicleControl_CancelSquare(SquarePhase phase)
{
  square_test.active = 0U;
  square_test.phase = phase;
  state.throttle = 0;
  state.steering = 0;
  VehicleMotor_Stop();
}

uint8_t VehicleControl_StartSquare(uint8_t throttle, int8_t direction, uint32_t now)
{
  if (!VehicleMotor_IsEnabled() || !state.mpu_ok || direction == 0) return 0U;

  VehicleMotor_Stop();
  square_test.active = 1U;
  square_test.direction = direction > 0 ? 1 : -1;
  square_test.max_throttle = throttle;
  square_test.leg = 0U;
  square_test.baseline_heading = state.yaw;
  state.yaw_control_enabled = 1U;
  state.heading_control_enabled = 1U;
  last_command_ms = now;
  SquareBeginDrive(now);
  return 1U;
}

static void SquareTestUpdate(uint32_t now)
{
  if (!square_test.active) return;
  if (!state.link_active || !VehicleMotor_IsEnabled() || !state.mpu_ok ||
      !state.yaw_control_enabled || !state.heading_control_enabled)
  {
    VehicleControl_CancelSquare(SQUARE_PHASE_ERROR);
    return;
  }

  if (square_test.phase == SQUARE_PHASE_DRIVE)
  {
    float left_distance = (float)(state.encoder_left - square_test.start_left) / LEFT_COUNTS_PER_METER;
    float right_distance = (float)(state.encoder_right - square_test.start_right) / RIGHT_COUNTS_PER_METER;
    float remaining;
    float desired_speed;
    float max_drive_speed = state.max_speed * (float)square_test.max_throttle / 100.0f;
    float min_drive_speed = fminf(SQUARE_MIN_DRIVE_SPEED_MPS, max_drive_speed);
    float linear_speed = 0.5f * (state.speed_left + state.speed_right);
    int throttle_command;

    square_test.progress_m = 0.5f * (left_distance + right_distance);
    remaining = SQUARE_SIDE_DISTANCE_M - square_test.progress_m;
    desired_speed = SQUARE_POSITION_KP * remaining;
    if (desired_speed > max_drive_speed) desired_speed = max_drive_speed;
    if (desired_speed < -max_drive_speed) desired_speed = -max_drive_speed;
    if (fabsf(remaining) <= SQUARE_DISTANCE_TOLERANCE_M)
    {
      desired_speed = 0.0f;
    }
    else if (fabsf(desired_speed) < min_drive_speed)
    {
      desired_speed = remaining > 0.0f ? min_drive_speed : -min_drive_speed;
    }

    throttle_command = (int)(desired_speed * 100.0f / state.max_speed +
                             (desired_speed >= 0.0f ? 0.5f : -0.5f));
    if (throttle_command > (int)square_test.max_throttle) throttle_command = square_test.max_throttle;
    if (throttle_command < -(int)square_test.max_throttle) throttle_command = -(int)square_test.max_throttle;
    state.throttle = (int8_t)throttle_command;
    state.steering = 0;

    if (fabsf(remaining) <= SQUARE_DISTANCE_TOLERANCE_M &&
        fabsf(linear_speed) <= SQUARE_DRIVE_SPEED_TOLERANCE_MPS &&
        fabsf(Vehicle_WrapAngle(state.target_heading - state.yaw)) <= SQUARE_HEADING_TOLERANCE_DEG &&
        fabsf(state.yaw_rate) <= SQUARE_YAW_RATE_TOLERANCE_DPS)
    {
      if (square_test.settle_start_ms == 0U) square_test.settle_start_ms = now;
      if ((now - square_test.settle_start_ms) >= SQUARE_SETTLE_MS) SquareBeginTurn(now);
    }
    else
    {
      square_test.settle_start_ms = 0U;
    }

    if ((now - square_test.phase_start_ms) > SQUARE_DRIVE_TIMEOUT_MS)
    {
      VehicleControl_CancelSquare(SQUARE_PHASE_ERROR);
    }
  }
  else if (square_test.phase == SQUARE_PHASE_TURN)
  {
    state.throttle = 0;
    state.steering = 0;
    if (fabsf(Vehicle_WrapAngle(state.target_heading - state.yaw)) <= SQUARE_HEADING_TOLERANCE_DEG &&
        fabsf(state.yaw_rate) <= SQUARE_YAW_RATE_TOLERANCE_DPS)
    {
      if (square_test.settle_start_ms == 0U) square_test.settle_start_ms = now;
      if ((now - square_test.settle_start_ms) >= SQUARE_SETTLE_MS)
      {
        ++square_test.leg;
        if (square_test.leg >= 4U) VehicleControl_CancelSquare(SQUARE_PHASE_COMPLETE);
        else SquareBeginDrive(now);
      }
    }
    else
    {
      square_test.settle_start_ms = 0U;
    }

    if (square_test.active && (now - square_test.phase_start_ms) > SQUARE_TURN_TIMEOUT_MS)
    {
      VehicleControl_CancelSquare(SQUARE_PHASE_ERROR);
    }
  }
}

void VehicleControl_Update(uint32_t now)
{
  VehicleMotor_UpdateFeedback();
  state.link_active = (last_command_ms != 0U && (now - last_command_ms) <= COMMAND_TIMEOUT_MS);
  VehicleImu_Update(now);
  VehicleLine_Update(now);
  if (!state.link_active && !VehicleLine_IsControlActive())
  {
    state.throttle = 0;
    state.steering = 0;
  }
  SquareTestUpdate(now);
  ApplySpeedControl(now);
  if ((now - last_battery_ms) >= BATTERY_PERIOD_MS)
  {
    last_battery_ms = now;
    VehicleBattery_Update();
  }
}
