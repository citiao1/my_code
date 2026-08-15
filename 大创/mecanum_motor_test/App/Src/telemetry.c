#include "telemetry.h"

#include <stdio.h>

#include "battery.h"
#include "encoder.h"
#include "gyro.h"
#include "heading_control.h"
#include "motor.h"
#include "serial_dma.h"
#include "speed_control.h"
#include "stm32f4xx_hal.h"
#include "vehicle.h"
#include "yaw_rate_control.h"

#define TELEMETRY_PERIOD_MS 200U

static uint32_t telemetry_tick;

static int32_t Telemetry_GainToMilli(float gain)
{
  float scaled = gain * 1000.0f;

  return (int32_t)(scaled >= 0.0f ? scaled + 0.5f : scaled - 0.5f);
}

void Telemetry_Init(void)
{
  telemetry_tick = HAL_GetTick();
}

void Telemetry_Process(void)
{
  uint32_t now = HAL_GetTick();

  if ((now - telemetry_tick) < TELEMETRY_PERIOD_MS)
  {
    return;
  }

  telemetry_tick = now;
  Telemetry_SendNow();
}

void Telemetry_SendNow(void)
{
  char message[512];
  int32_t encoder[MOTOR_COUNT];
  int16_t output[MOTOR_COUNT];
  SpeedControlSnapshot speed = SpeedControl_GetSnapshot();
  SpeedControlConfig config = SpeedControl_GetConfig();
  GyroSnapshot gyro = Gyro_GetSnapshot();
  YawRateControlSnapshot yaw = YawRateControl_GetSnapshot();
  HeadingControlSnapshot heading = HeadingControl_GetSnapshot();

  Encoder_GetAll(encoder);
  Motor_GetAll(output);
  (void)snprintf(message, sizeof(message),
                 "TEL,%lu,%s,%u,%ld,%ld,%ld,%ld,%d,%d,%d,%d,"
                 "%d,%d,%d,%d,%d,%d,%d,%d,%u,%ld,%ld,%ld,%u,%u,%u,"
                 "%u,%u,%u,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%u,"
                 "%ld,%ld,%ld,%ld,%ld,%ld,%ld,%u,%u\r\n",
                 (unsigned long)HAL_GetTick(), Vehicle_GetModeText(), Motor_IsEnabled(),
                 (long)encoder[MOTOR_A], (long)encoder[MOTOR_B],
                 (long)encoder[MOTOR_C], (long)encoder[MOTOR_D],
                 output[MOTOR_A], output[MOTOR_B], output[MOTOR_C], output[MOTOR_D],
                 speed.rpm[MOTOR_A], speed.rpm[MOTOR_B],
                 speed.rpm[MOTOR_C], speed.rpm[MOTOR_D],
                 speed.target_rpm[MOTOR_A], speed.target_rpm[MOTOR_B],
                 speed.target_rpm[MOTOR_C], speed.target_rpm[MOTOR_D],
                 Battery_GetMillivolts(),
                 (long)Telemetry_GainToMilli(config.kp),
                 (long)Telemetry_GainToMilli(config.ki),
                 (long)Telemetry_GainToMilli(config.kd),
                 config.output_limit, config.integral_limit, config.enabled,
                 gyro.connected, gyro.ready, gyro.calibrating,
                 (long)Telemetry_GainToMilli(gyro.raw_rate_dps),
                 (long)Telemetry_GainToMilli(gyro.rate_dps),
                 (long)Telemetry_GainToMilli(gyro.yaw_deg),
                 (long)Telemetry_GainToMilli(yaw.target_dps),
                 (long)Telemetry_GainToMilli(yaw.output_rpm),
                 (long)Telemetry_GainToMilli(yaw.kp),
                 (long)Telemetry_GainToMilli(yaw.ki),
                 (long)Telemetry_GainToMilli(yaw.kff), yaw.enabled,
                 (long)Telemetry_GainToMilli(heading.target_deg),
                 (long)Telemetry_GainToMilli(heading.feedback_deg),
                 (long)Telemetry_GainToMilli(heading.error_deg),
                 (long)Telemetry_GainToMilli(heading.output_dps),
                 (long)Telemetry_GainToMilli(heading.kp),
                 (long)Telemetry_GainToMilli(heading.kd),
                 (long)Telemetry_GainToMilli(heading.max_rate_dps),
                 heading.enabled, heading.holding);
  SerialDma_Write(message);
}
