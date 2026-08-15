#include "vehicle_motor.h"

#include "main.h"
#include "tim.h"
#include "vehicle_config.h"
#include "vehicle_internal.h"

#include <math.h>
#include <stdlib.h>

static int16_t ReadEncoderDelta(TIM_HandleTypeDef *htim)
{
  int16_t value = (int16_t)__HAL_TIM_GET_COUNTER(htim);
  __HAL_TIM_SET_COUNTER(htim, 0);
  return value;
}

void VehicleMotor_Init(void)
{
  HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim10, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim11, TIM_CHANNEL_1);
  HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
  HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
  VehicleMotor_Stop();
}

uint8_t VehicleMotor_IsEnabled(void)
{
  return HAL_GPIO_ReadPin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin) == GPIO_PIN_SET;
}

void VehicleMotor_Stop(void)
{
  __HAL_TIM_SET_COMPARE(&htim10, TIM_CHANNEL_1, 0);
  __HAL_TIM_SET_COMPARE(&htim11, TIM_CHANNEL_1, 0);
  __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, 0);
  __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_2, 0);
  state.pwm_left = 0;
  state.pwm_right = 0;
  pid_left.previous_error = 0.0f;
  pid_left.previous_previous_error = 0.0f;
  pid_left.output = 0.0f;
  pid_right.previous_error = 0.0f;
  pid_right.previous_previous_error = 0.0f;
  pid_right.output = 0.0f;
  pid_yaw.integral = 0.0f;
  pid_yaw.previous_error = 0.0f;
  state.yaw_error = 0.0f;
  state.yaw_correction = 0.0f;
  state.yaw_feedforward = 0.0f;
  state.heading_error = 0.0f;
  state.heading_output = 0.0f;
  state.heading_reference_rate = 0.0f;
  state.heading_hold_active = 0U;
}

void VehicleMotor_SetLeftPwm(int16_t physical_pwm)
{
  int16_t hardware_pwm = (int16_t)-physical_pwm;
  uint16_t magnitude = (uint16_t)abs(hardware_pwm);
  state.pwm_left = physical_pwm;

  if (hardware_pwm == 0)
  {
    __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_2, 0);
  }
  else if (hardware_pwm > 0)
  {
    __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, PWM_PERIOD - magnitude);
    __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_2, PWM_PERIOD);
  }
  else
  {
    __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, PWM_PERIOD);
    __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_2, PWM_PERIOD - magnitude);
  }
}

void VehicleMotor_SetRightPwm(int16_t physical_pwm)
{
  int16_t hardware_pwm = (int16_t)-physical_pwm;
  uint16_t magnitude = (uint16_t)abs(hardware_pwm);
  state.pwm_right = physical_pwm;

  if (hardware_pwm == 0)
  {
    __HAL_TIM_SET_COMPARE(&htim10, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim11, TIM_CHANNEL_1, 0);
  }
  else if (hardware_pwm > 0)
  {
    __HAL_TIM_SET_COMPARE(&htim10, TIM_CHANNEL_1, PWM_PERIOD - magnitude);
    __HAL_TIM_SET_COMPARE(&htim11, TIM_CHANNEL_1, PWM_PERIOD);
  }
  else
  {
    __HAL_TIM_SET_COMPARE(&htim10, TIM_CHANNEL_1, PWM_PERIOD);
    __HAL_TIM_SET_COMPARE(&htim11, TIM_CHANNEL_1, PWM_PERIOD - magnitude);
  }
}

int16_t VehicleMotor_SpeedPidStep(SpeedPidState *pid, float target, float measured, float *error_out)
{
  float error;
  float delta;

  if (fabsf(target) < 0.001f)
  {
    pid->previous_error = 0.0f;
    pid->previous_previous_error = 0.0f;
    pid->output = 0.0f;
    *error_out = 0.0f;
    return 0;
  }

  error = target - measured;
  delta = pid->kp * (error - pid->previous_error)
          + pid->ki * error
          + pid->kd * (error - 2.0f * pid->previous_error + pid->previous_previous_error);
  pid->output += delta;
  if (pid->output > (float)PWM_MAX) pid->output = (float)PWM_MAX;
  if (pid->output < -(float)PWM_MAX) pid->output = -(float)PWM_MAX;

  pid->previous_previous_error = pid->previous_error;
  pid->previous_error = error;
  *error_out = error;
  return (int16_t)pid->output;
}

void VehicleMotor_UpdateFeedback(void)
{
  int16_t delta_left = ReadEncoderDelta(&htim3);
  int16_t delta_right = (int16_t)-ReadEncoderDelta(&htim2);
  float raw_left = (float)delta_left * 100.0f / LEFT_COUNTS_PER_METER;
  float raw_right = (float)delta_right * 100.0f / RIGHT_COUNTS_PER_METER;

  state.encoder_left += delta_left;
  state.encoder_right += delta_right;
  state.speed_left = 0.65f * state.speed_left + 0.35f * raw_left;
  state.speed_right = 0.65f * state.speed_right + 0.35f * raw_right;
}
