#include "vehicle_app.h"

#include "adc.h"
#include "gpio.h"
#include "i2c.h"
#include "main.h"
#include "tim.h"
#include "usart.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PWM_PERIOD             16799U
#define PWM_MAX                15000U

#define CONTROL_PERIOD_MS      10U
#define OLED_PERIOD_MS         200U
#define TELEMETRY_PERIOD_MS    100U
#define BATTERY_PERIOD_MS      100U
#define COMMAND_TIMEOUT_MS     400U
#define DEFAULT_MAX_SPEED_MPS  0.20f
#define DEFAULT_PID_KP         4000.0f
#define DEFAULT_PID_KI         800.0f
#define DEFAULT_PID_KD         0.0f

#define DEFAULT_MAX_YAW_RATE_DPS 120.0f
#define DEFAULT_YAW_KP         0.0010f
#define DEFAULT_YAW_KI         0.0f
#define DEFAULT_YAW_KD         0.0f
#define YAW_INTEGRAL_LIMIT     300.0f
#define YAW_RATE_DEADBAND_DPS  0.50f
#define MPU_RETRY_PERIOD_MS    3000U
#define MPU_MAX_READ_FAILURES  10U

#define BATTERY_DIVIDER_RATIO  11.0f
#define ADC_REFERENCE_VOLTAGE  3.3f
#define ADC_FULL_SCALE         4095.0f

#define LEFT_COUNTS_PER_REV    1558.3f
#define RIGHT_COUNTS_PER_REV   1557.4f
#define LEFT_COUNTS_PER_METER  7514.0f
#define RIGHT_COUNTS_PER_METER 7263.0f

#define MPU_ADDRESS            (0x68U << 1)
#define MPU_REG_ACCEL_XOUT_H   0x3BU
#define MPU_REG_CONFIG         0x1AU
#define MPU_REG_GYRO_CONFIG    0x1BU
#define MPU_REG_ACCEL_CONFIG   0x1CU
#define MPU_REG_SMPLRT_DIV     0x19U
#define MPU_REG_PWR_MGMT_1     0x6BU
#define MPU_REG_WHO_AM_I       0x75U

#define UART_RX_BUFFER_SIZE    128U
#define UART_LINE_SIZE         64U

typedef struct
{
  float previous_error;
  float previous_previous_error;
  float output;
  float kp;
  float ki;
  float kd;
} SpeedPidState;

typedef struct
{
  float integral;
  float previous_error;
  float kp;
  float ki;
  float kd;
} YawPidState;

typedef struct
{
  float speed_left;
  float speed_right;
  float target_left;
  float target_right;
  float error_left;
  float error_right;
  float max_speed;
  float pitch;
  float roll;
  float yaw;
  float yaw_rate;
  float target_yaw_rate;
  float yaw_error;
  float yaw_correction;
  float max_yaw_rate;
  float battery_voltage;
  float gyro_bias_x;
  float gyro_bias_y;
  float gyro_bias_z;
  int32_t encoder_left;
  int32_t encoder_right;
  int16_t pwm_left;
  int16_t pwm_right;
  uint16_t battery_raw;
  int8_t throttle;
  int8_t steering;
  uint8_t mpu_ok;
  uint8_t link_active;
  uint8_t yaw_control_enabled;
} VehicleState;

static VehicleState state;
static SpeedPidState pid_left;
static SpeedPidState pid_right;
static YawPidState pid_yaw;

static uint8_t uart_rx_byte;
static volatile uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE];
static volatile uint16_t uart_rx_head;
static volatile uint16_t uart_rx_tail;
static char uart_line[UART_LINE_SIZE];
static uint8_t uart_line_length;

static char uart_tx_buffer[256];
static volatile uint8_t uart_tx_busy;

static uint32_t last_control_ms;
static uint32_t last_oled_ms;
static uint32_t last_telemetry_ms;
static uint32_t last_command_ms;
static uint32_t last_mpu_ms;
static uint32_t last_battery_ms;
static uint32_t last_mpu_retry_ms;
static uint8_t mpu_read_failures;

static int8_t ClampPercent(int value)
{
  if (value > 100) return 100;
  if (value < -100) return -100;
  return (int8_t)value;
}

static uint8_t MotorEnabled(void)
{
  return HAL_GPIO_ReadPin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin) == GPIO_PIN_SET;
}

static void MotorStop(void)
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
}

static void MotorSetLeftPwm(int16_t physical_pwm)
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

static void MotorSetRightPwm(int16_t physical_pwm)
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

static int16_t SpeedPidStep(SpeedPidState *pid, float target, float measured, float *error_out)
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

static void ApplySpeedControl(void)
{
  float base_speed;
  float correction;
  float left_target;
  float right_target;
  float peak;

  state.target_yaw_rate = (float)state.steering * state.max_yaw_rate / 100.0f;

  if (!MotorEnabled() || !state.link_active || (state.throttle == 0 && state.steering == 0))
  {
    state.target_left = 0.0f;
    state.target_right = 0.0f;
    state.error_left = 0.0f;
    state.error_right = 0.0f;
    state.target_yaw_rate = 0.0f;
    MotorStop();
    return;
  }

  base_speed = (float)state.throttle * state.max_speed / 100.0f;
  if (state.yaw_control_enabled && state.mpu_ok)
  {
    float error = state.target_yaw_rate - state.yaw_rate;
    float candidate_integral = pid_yaw.integral + error * 0.01f;
    float derivative = (error - pid_yaw.previous_error) / 0.01f;

    if (candidate_integral > YAW_INTEGRAL_LIMIT) candidate_integral = YAW_INTEGRAL_LIMIT;
    if (candidate_integral < -YAW_INTEGRAL_LIMIT) candidate_integral = -YAW_INTEGRAL_LIMIT;
    correction = pid_yaw.kp * error + pid_yaw.ki * candidate_integral + pid_yaw.kd * derivative;
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
  MotorSetLeftPwm(SpeedPidStep(&pid_left, state.target_left, state.speed_left, &state.error_left));
  MotorSetRightPwm(SpeedPidStep(&pid_right, state.target_right, state.speed_right, &state.error_right));
}

static void UpdateBattery(void)
{
  uint32_t sum = 0U;
  uint16_t sample;

  for (sample = 0U; sample < 16U; ++sample)
  {
    if (HAL_ADC_Start(&hadc1) != HAL_OK) return;
    if (HAL_ADC_PollForConversion(&hadc1, 2U) != HAL_OK)
    {
      HAL_ADC_Stop(&hadc1);
      return;
    }
    sum += HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
  }

  state.battery_raw = (uint16_t)(sum / 16U);
  {
    float voltage = (float)state.battery_raw * ADC_REFERENCE_VOLTAGE *
                    BATTERY_DIVIDER_RATIO / ADC_FULL_SCALE;
    state.battery_voltage = state.battery_voltage == 0.0f ? voltage :
                            0.85f * state.battery_voltage + 0.15f * voltage;
  }
}

static int16_t ReadEncoderDelta(TIM_HandleTypeDef *htim)
{
  int16_t value = (int16_t)__HAL_TIM_GET_COUNTER(htim);
  __HAL_TIM_SET_COUNTER(htim, 0);
  return value;
}

static uint8_t MPUWrite(uint8_t reg, uint8_t value)
{
  return HAL_I2C_Mem_Write(&hi2c2, MPU_ADDRESS, reg, I2C_MEMADD_SIZE_8BIT,
                           &value, 1, 100) == HAL_OK;
}

static uint8_t MPUReadRaw(int16_t *ax, int16_t *ay, int16_t *az,
                          int16_t *gx, int16_t *gy, int16_t *gz)
{
  uint8_t raw[14];
  if (HAL_I2C_Mem_Read(&hi2c2, MPU_ADDRESS, MPU_REG_ACCEL_XOUT_H,
                       I2C_MEMADD_SIZE_8BIT, raw, sizeof(raw), 100) != HAL_OK)
  {
    return 0;
  }

  *ax = (int16_t)((raw[0] << 8) | raw[1]);
  *ay = (int16_t)((raw[2] << 8) | raw[3]);
  *az = (int16_t)((raw[4] << 8) | raw[5]);
  *gx = (int16_t)((raw[8] << 8) | raw[9]);
  *gy = (int16_t)((raw[10] << 8) | raw[11]);
  *gz = (int16_t)((raw[12] << 8) | raw[13]);
  return 1;
}

static uint8_t MPUInit(void)
{
  uint8_t who = 0;
  int32_t sum_x = 0;
  int32_t sum_y = 0;
  int32_t sum_z = 0;
  int16_t ax, ay, az, gx, gy, gz;
  uint16_t sample;
  uint16_t valid_samples = 0U;

  if (HAL_I2C_IsDeviceReady(&hi2c2, MPU_ADDRESS, 3, 100) != HAL_OK) return 0;
  if (HAL_I2C_Mem_Read(&hi2c2, MPU_ADDRESS, MPU_REG_WHO_AM_I,
                       I2C_MEMADD_SIZE_8BIT, &who, 1, 100) != HAL_OK || who != 0x68U) return 0;
  if (!MPUWrite(MPU_REG_PWR_MGMT_1, 0x00)) return 0;
  HAL_Delay(50);
  MPUWrite(MPU_REG_SMPLRT_DIV, 9);
  MPUWrite(MPU_REG_CONFIG, 3);
  MPUWrite(MPU_REG_GYRO_CONFIG, 0);
  MPUWrite(MPU_REG_ACCEL_CONFIG, 0);

  for (sample = 0; sample < 500U; ++sample)
  {
    if (MPUReadRaw(&ax, &ay, &az, &gx, &gy, &gz))
    {
      sum_x += gx;
      sum_y += gy;
      sum_z += gz;
      ++valid_samples;
    }
    HAL_Delay(3);
  }
  if (valid_samples < 450U) return 0;
  state.gyro_bias_x = (float)sum_x / (float)valid_samples;
  state.gyro_bias_y = (float)sum_y / (float)valid_samples;
  state.gyro_bias_z = (float)sum_z / (float)valid_samples;
  return 1;
}

static uint8_t MPUInitWithRetry(uint8_t attempts)
{
  uint8_t attempt;
  for (attempt = 0U; attempt < attempts; ++attempt)
  {
    if (MPUInit())
    {
      mpu_read_failures = 0U;
      state.yaw_rate = 0.0f;
      last_mpu_ms = 0U;
      return 1U;
    }
    HAL_I2C_DeInit(&hi2c2);
    HAL_Delay(10U);
    MX_I2C2_Init();
    HAL_Delay(100U);
  }
  return 0U;
}

static void UpdateImu(uint32_t now)
{
  int16_t ax_raw, ay_raw, az_raw, gx_raw, gy_raw, gz_raw;
  float ax, ay, az, gx, gy, gz;
  float pitch_acc, roll_acc, dt;

  if (!state.mpu_ok) return;
  if (!MPUReadRaw(&ax_raw, &ay_raw, &az_raw, &gx_raw, &gy_raw, &gz_raw))
  {
    if (mpu_read_failures < 255U) ++mpu_read_failures;
    if (mpu_read_failures >= MPU_MAX_READ_FAILURES)
    {
      state.mpu_ok = 0U;
      state.yaw_control_enabled = 0U;
      MotorStop();
    }
    return;
  }
  mpu_read_failures = 0U;

  dt = (last_mpu_ms == 0U) ? 0.01f : (float)(now - last_mpu_ms) / 1000.0f;
  if (dt <= 0.0f || dt > 0.1f) dt = 0.01f;
  last_mpu_ms = now;

  ax = (float)ax_raw / 16384.0f;
  ay = (float)ay_raw / 16384.0f;
  az = (float)az_raw / 16384.0f;
  gx = ((float)gx_raw - state.gyro_bias_x) / 131.0f;
  gy = ((float)gy_raw - state.gyro_bias_y) / 131.0f;
  gz = ((float)gz_raw - state.gyro_bias_z) / 131.0f;

  if (state.throttle == 0 && state.steering == 0 &&
      fabsf(state.speed_left) < 0.01f && fabsf(state.speed_right) < 0.01f)
  {
    state.gyro_bias_z = 0.998f * state.gyro_bias_z + 0.002f * (float)gz_raw;
    gz = ((float)gz_raw - state.gyro_bias_z) / 131.0f;
  }
  if (fabsf(gz) < YAW_RATE_DEADBAND_DPS) gz = 0.0f;
  state.yaw_rate = 0.70f * state.yaw_rate + 0.30f * gz;

  roll_acc = atan2f(ay, az) * 57.2957795f;
  pitch_acc = atan2f(-ax, sqrtf(ay * ay + az * az)) * 57.2957795f;
  state.roll = 0.98f * (state.roll + gx * dt) + 0.02f * roll_acc;
  state.pitch = 0.98f * (state.pitch + gy * dt) + 0.02f * pitch_acc;
  state.yaw += state.yaw_rate * dt;
  if (state.yaw > 180.0f) state.yaw -= 360.0f;
  if (state.yaw < -180.0f) state.yaw += 360.0f;
}

static void ControlUpdate(uint32_t now)
{
  int16_t delta_left = ReadEncoderDelta(&htim3);
  int16_t delta_right = (int16_t)-ReadEncoderDelta(&htim2);
  float raw_left = (float)delta_left * 100.0f / LEFT_COUNTS_PER_METER;
  float raw_right = (float)delta_right * 100.0f / RIGHT_COUNTS_PER_METER;

  state.encoder_left += delta_left;
  state.encoder_right += delta_right;
  state.speed_left = 0.65f * state.speed_left + 0.35f * raw_left;
  state.speed_right = 0.65f * state.speed_right + 0.35f * raw_right;

  state.link_active = (last_command_ms != 0U && (now - last_command_ms) <= COMMAND_TIMEOUT_MS);
  if (!state.link_active)
  {
    state.throttle = 0;
    state.steering = 0;
  }
  UpdateImu(now);
  ApplySpeedControl();
  if ((now - last_battery_ms) >= BATTERY_PERIOD_MS)
  {
    last_battery_ms = now;
    UpdateBattery();
  }
}

static void UARTSendTelemetry(uint32_t now)
{
  int length;
  if (uart_tx_busy) return;

  length = snprintf(uart_tx_buffer, sizeof(uart_tx_buffer),
                    "TEL,%lu,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%ld,%ld,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%d,%u,%u\n",
                    (unsigned long)now, MotorEnabled(), state.link_active,
                    (int)(state.yaw * 10.0f), (int)(state.pitch * 10.0f),
                    (int)(state.roll * 10.0f), (int)(state.speed_left * 1000.0f),
                    (int)(state.speed_right * 1000.0f), (int)(state.target_left * 1000.0f),
                    (int)(state.target_right * 1000.0f), (int)(state.error_left * 1000.0f),
                    (int)(state.error_right * 1000.0f), (long)state.encoder_left,
                    (long)state.encoder_right, state.pwm_left, state.pwm_right,
                    (int)pid_left.kp, (int)pid_left.ki, (int)pid_left.kd,
                    (int)pid_right.kp, (int)pid_right.ki, (int)pid_right.kd,
                    (int)(state.target_yaw_rate * 10.0f), (int)(state.yaw_rate * 10.0f),
                    (int)(state.yaw_error * 10.0f), (int)(state.yaw_correction * 1000.0f),
                    state.yaw_control_enabled, (int)(state.battery_voltage * 1000.0f),
                    state.battery_raw, state.mpu_ok);
  if (length <= 0 || length >= (int)sizeof(uart_tx_buffer)) return;

  uart_tx_busy = 1U;
  if (HAL_UART_Transmit_IT(&huart2, (uint8_t *)uart_tx_buffer, (uint16_t)length) != HAL_OK)
  {
    uart_tx_busy = 0U;
  }
}

static void ProcessLine(char *line)
{
  int throttle;
  int steering;
  int kp;
  int ki;
  int kd;
  int max_speed_mm;
  int yaw_enable;
  int max_yaw_rate;
  int yaw_kp_micro;
  int yaw_ki_micro;
  int yaw_kd_micro;

  if (sscanf(line, "DRV,%d,%d", &throttle, &steering) == 2)
  {
    state.throttle = ClampPercent(throttle);
    state.steering = ClampPercent(steering);
    last_command_ms = HAL_GetTick();
  }
  else if (sscanf(line, "PIDL,%d,%d,%d", &kp, &ki, &kd) == 3)
  {
    if (kp >= 0 && kp <= 50000 && ki >= 0 && ki <= 50000 && kd >= 0 && kd <= 5000)
    {
      pid_left.kp = (float)kp;
      pid_left.ki = (float)ki;
      pid_left.kd = (float)kd;
      MotorStop();
    }
  }
  else if (sscanf(line, "PIDR,%d,%d,%d", &kp, &ki, &kd) == 3)
  {
    if (kp >= 0 && kp <= 50000 && ki >= 0 && ki <= 50000 && kd >= 0 && kd <= 5000)
    {
      pid_right.kp = (float)kp;
      pid_right.ki = (float)ki;
      pid_right.kd = (float)kd;
      MotorStop();
    }
  }
  else if (sscanf(line, "PID,%d,%d,%d", &kp, &ki, &kd) == 3)
  {
    if (kp >= 0 && kp <= 50000 && ki >= 0 && ki <= 50000 && kd >= 0 && kd <= 5000)
    {
      pid_left.kp = pid_right.kp = (float)kp;
      pid_left.ki = pid_right.ki = (float)ki;
      pid_left.kd = pid_right.kd = (float)kd;
      MotorStop();
    }
  }
  else if (sscanf(line, "MAX,%d", &max_speed_mm) == 1)
  {
    if (max_speed_mm >= 50 && max_speed_mm <= 1500) state.max_speed = (float)max_speed_mm / 1000.0f;
  }
  else if (sscanf(line, "YAW,%d", &yaw_enable) == 1)
  {
    state.yaw_control_enabled = (yaw_enable != 0 && state.mpu_ok) ? 1U : 0U;
    MotorStop();
  }
  else if (sscanf(line, "YAWRATE,%d", &max_yaw_rate) == 1)
  {
    if (max_yaw_rate >= 10 && max_yaw_rate <= 360) state.max_yaw_rate = (float)max_yaw_rate;
  }
  else if (sscanf(line, "YAWPID,%d,%d,%d", &yaw_kp_micro, &yaw_ki_micro, &yaw_kd_micro) == 3)
  {
    if (yaw_kp_micro >= 0 && yaw_kp_micro <= 100000 &&
        yaw_ki_micro >= 0 && yaw_ki_micro <= 100000 &&
        yaw_kd_micro >= 0 && yaw_kd_micro <= 100000)
    {
      pid_yaw.kp = (float)yaw_kp_micro / 1000000.0f;
      pid_yaw.ki = (float)yaw_ki_micro / 1000000.0f;
      pid_yaw.kd = (float)yaw_kd_micro / 1000000.0f;
      MotorStop();
    }
  }
  else if (strcmp(line, "STOP") == 0)
  {
    state.throttle = 0;
    state.steering = 0;
    last_command_ms = 0;
    state.link_active = 0;
    MotorStop();
  }
  else if (strcmp(line, "PING") == 0)
  {
    last_command_ms = HAL_GetTick();
  }
  else if (strcmp(line, "ZERO") == 0)
  {
    state.encoder_left = 0;
    state.encoder_right = 0;
    state.yaw = 0.0f;
  }
}

static void ProcessUart(void)
{
  while (uart_rx_tail != uart_rx_head)
  {
    char byte = (char)uart_rx_buffer[uart_rx_tail];
    uart_rx_tail = (uint16_t)((uart_rx_tail + 1U) % UART_RX_BUFFER_SIZE);

    if (byte == '\n' || byte == '\r')
    {
      if (uart_line_length > 0U)
      {
        uart_line[uart_line_length] = '\0';
        ProcessLine(uart_line);
        uart_line_length = 0U;
      }
    }
    else if (uart_line_length < UART_LINE_SIZE - 1U)
    {
      uart_line[uart_line_length++] = byte;
    }
    else
    {
      uart_line_length = 0U;
    }
  }
}

static void OLEDWriteByte(uint8_t value, GPIO_PinState data_mode)
{
  uint8_t bit;
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, data_mode);
  for (bit = 0; bit < 8U; ++bit)
  {
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, (value & 0x80U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
    value <<= 1;
  }
}

static void OLEDCommand(uint8_t value)
{
  OLEDWriteByte(value, GPIO_PIN_RESET);
}

static void OLEDSetPage(uint8_t page)
{
  OLEDCommand((uint8_t)(0xB0U + page));
  OLEDCommand(0x00);
  OLEDCommand(0x10);
}

static const uint8_t glyphs[][5] = {
  {0x00,0x00,0x00,0x00,0x00}, {0x08,0x08,0x3E,0x08,0x08},
  {0x08,0x08,0x08,0x08,0x08}, {0x00,0x60,0x60,0x00,0x00},
  {0x00,0x36,0x36,0x00,0x00},
  {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
  {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
  {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
  {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
  {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E},
  {0x7E,0x11,0x11,0x11,0x7E}, {0x7F,0x49,0x49,0x49,0x36},
  {0x3E,0x41,0x41,0x41,0x22}, {0x7F,0x41,0x41,0x22,0x1C},
  {0x7F,0x49,0x49,0x49,0x41}, {0x7F,0x09,0x09,0x09,0x01},
  {0x3E,0x41,0x49,0x49,0x7A}, {0x7F,0x08,0x08,0x08,0x7F},
  {0x00,0x41,0x7F,0x41,0x00}, {0x20,0x40,0x41,0x3F,0x01},
  {0x7F,0x08,0x14,0x22,0x41}, {0x7F,0x40,0x40,0x40,0x40},
  {0x7F,0x02,0x0C,0x02,0x7F}, {0x7F,0x04,0x08,0x10,0x7F},
  {0x3E,0x41,0x41,0x41,0x3E}, {0x7F,0x09,0x09,0x09,0x06},
  {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46},
  {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7F,0x01,0x01},
  {0x3F,0x40,0x40,0x40,0x3F}, {0x1F,0x20,0x40,0x20,0x1F},
  {0x3F,0x40,0x38,0x40,0x3F}, {0x63,0x14,0x08,0x14,0x63},
  {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43}
};

static const char glyph_chars[] = " +-.:0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

static void OLEDWriteLine(uint8_t page, const char *text)
{
  uint8_t column = 0;
  OLEDSetPage(page);
  while (*text != '\0' && column <= 122U)
  {
    const char *match = strchr(glyph_chars, *text);
    uint8_t index = match ? (uint8_t)(match - glyph_chars) : 0U;
    uint8_t glyph_column;
    for (glyph_column = 0; glyph_column < 5U; ++glyph_column)
    {
      OLEDWriteByte(glyphs[index][glyph_column], GPIO_PIN_SET);
    }
    OLEDWriteByte(0x00, GPIO_PIN_SET);
    column = (uint8_t)(column + 6U);
    ++text;
  }
  while (column++ < 128U) OLEDWriteByte(0x00, GPIO_PIN_SET);
}

static void OLEDInit(void)
{
  static const uint8_t commands[] = {
    0xAE,0xD5,0x50,0xA8,0x3F,0xD3,0x00,0x40,0x8D,0x14,0x20,0x02,
    0xA1,0xC0,0xDA,0x12,0x81,0xEF,0xD9,0xF1,0xDB,0x30,0xA4,0xA6,0xAF
  };
  uint8_t index;
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);
  HAL_Delay(20);
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
  HAL_Delay(20);
  for (index = 0; index < sizeof(commands); ++index) OLEDCommand(commands[index]);
  for (index = 0; index < 8U; ++index) OLEDWriteLine(index, "");
}

static void OLEDUpdate(void)
{
  char line[24];
  int yaw10 = (int)(state.yaw * 10.0f);
  int pitch10 = (int)(state.pitch * 10.0f);
  int roll10 = (int)(state.roll * 10.0f);
  long distance_mm = (long)(0.5f * ((float)state.encoder_left / LEFT_COUNTS_PER_METER +
                                    (float)state.encoder_right / RIGHT_COUNTS_PER_METER) * 1000.0f);

  snprintf(line, sizeof(line), "YAW %+04d.%d", yaw10 / 10, abs(yaw10 % 10)); OLEDWriteLine(0, line);
  snprintf(line, sizeof(line), "PIT %+03d.%d ROL %+03d.%d", pitch10 / 10, abs(pitch10 % 10),
           roll10 / 10, abs(roll10 % 10)); OLEDWriteLine(1, line);
  snprintf(line, sizeof(line), "TL %+04d TR %+04d", (int)(state.target_left * 1000.0f),
           (int)(state.target_right * 1000.0f)); OLEDWriteLine(2, line);
  snprintf(line, sizeof(line), "VL %+04d VR %+04d", (int)(state.speed_left * 1000.0f),
           (int)(state.speed_right * 1000.0f)); OLEDWriteLine(3, line);
  snprintf(line, sizeof(line), "ER %+04d %+04d", (int)(state.error_left * 1000.0f),
           (int)(state.error_right * 1000.0f)); OLEDWriteLine(4, line);
  snprintf(line, sizeof(line), "EA %+07ld", (long)state.encoder_left); OLEDWriteLine(5, line);
  snprintf(line, sizeof(line), "EB %+07ld", (long)state.encoder_right); OLEDWriteLine(6, line);
  snprintf(line, sizeof(line), "BAT %u.%02uV MPU %u", (unsigned int)state.battery_voltage,
           (unsigned int)(state.battery_voltage * 100.0f) % 100U, state.mpu_ok);
  OLEDWriteLine(7, line);
}

void Vehicle_Init(void)
{
  MotorStop();
  pid_left.kp = pid_right.kp = DEFAULT_PID_KP;
  pid_left.ki = pid_right.ki = DEFAULT_PID_KI;
  pid_left.kd = pid_right.kd = DEFAULT_PID_KD;
  state.max_speed = DEFAULT_MAX_SPEED_MPS;
  pid_yaw.kp = DEFAULT_YAW_KP;
  pid_yaw.ki = DEFAULT_YAW_KI;
  pid_yaw.kd = DEFAULT_YAW_KD;
  state.max_yaw_rate = DEFAULT_MAX_YAW_RATE_DPS;
  state.yaw_control_enabled = 0U;
  HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim10, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim11, TIM_CHANNEL_1);
  HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
  HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
  MotorStop();

  OLEDInit();
  OLEDWriteLine(0, "C30D VEHICLE");
  OLEDWriteLine(1, "CALIBRATING IMU");
  state.mpu_ok = MPUInitWithRetry(3U);
  UpdateBattery();
  HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);

  last_control_ms = HAL_GetTick();
  last_oled_ms = last_control_ms;
  last_telemetry_ms = last_control_ms;
  OLEDUpdate();
}

void Vehicle_Loop(void)
{
  uint32_t now = HAL_GetTick();
  ProcessUart();

  if (!state.mpu_ok && state.throttle == 0 && state.steering == 0 &&
      (now - last_mpu_retry_ms) >= MPU_RETRY_PERIOD_MS)
  {
    last_mpu_retry_ms = now;
    MotorStop();
    state.mpu_ok = MPUInitWithRetry(1U);
    now = HAL_GetTick();
  }

  if ((now - last_control_ms) >= CONTROL_PERIOD_MS)
  {
    last_control_ms += CONTROL_PERIOD_MS;
    ControlUpdate(now);
  }
  if ((now - last_telemetry_ms) >= TELEMETRY_PERIOD_MS)
  {
    last_telemetry_ms = now;
    UARTSendTelemetry(now);
  }
  if ((now - last_oled_ms) >= OLED_PERIOD_MS)
  {
    last_oled_ms = now;
    OLEDUpdate();
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    uint16_t next = (uint16_t)((uart_rx_head + 1U) % UART_RX_BUFFER_SIZE);
    if (next != uart_rx_tail)
    {
      uart_rx_buffer[uart_rx_head] = uart_rx_byte;
      uart_rx_head = next;
    }
    HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
  }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2) uart_tx_busy = 0U;
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    uart_tx_busy = 0U;
    HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
  }
}
