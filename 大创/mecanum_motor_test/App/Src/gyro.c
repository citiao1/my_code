#include "gyro.h"

#include <string.h>

#include "mpu6050.h"
#include "stm32f4xx_hal.h"

#define GYRO_SCALE_LSB_PER_DPS          131.0f
#define GYRO_Z_SIGN                       1.0f
#define GYRO_WARMUP_SAMPLES              25U
#define GYRO_CALIBRATION_SAMPLES        200U
#define GYRO_CAL_VARIANCE_LIMIT_RAW   17161.0f
#define GYRO_RATE_DEADBAND_DPS             0.5f
#define GYRO_BIAS_TRACK_LIMIT_DPS          1.5f
#define GYRO_BIAS_TRACK_ALPHA              0.002f
#define GYRO_LPF_CUTOFF_HZ                  5.0f
#define GYRO_TWO_PI                         6.2831853f
#define GYRO_MAX_READ_FAILURES              5U
#define GYRO_RECONNECT_INTERVAL_MS       1000U

static GyroSnapshot gyro_state;
static uint16_t warmup_remaining;
static float calibration_mean_z;
static float calibration_m2_z;
static uint32_t reconnect_tick;

static float Gyro_Abs(float value)
{
  return (value >= 0.0f) ? value : -value;
}

static float Gyro_WrapAngle(float angle)
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

static void Gyro_ResetCalibration(uint16_t warmup_samples)
{
  gyro_state.ready = 0U;
  gyro_state.calibrating = gyro_state.connected;
  gyro_state.calibration_samples = 0U;
  gyro_state.bias_z = 0.0f;
  gyro_state.raw_rate_dps = 0.0f;
  gyro_state.rate_dps = 0.0f;
  warmup_remaining = warmup_samples;
  calibration_mean_z = 0.0f;
  calibration_m2_z = 0.0f;
}

static void Gyro_AddCalibrationSample(int16_t raw_z)
{
  float delta;
  float delta_after_mean;

  ++gyro_state.calibration_samples;
  delta = (float)raw_z - calibration_mean_z;
  calibration_mean_z += delta / (float)gyro_state.calibration_samples;
  delta_after_mean = (float)raw_z - calibration_mean_z;
  calibration_m2_z += delta * delta_after_mean;
}

static uint8_t Gyro_CalibrationIsStable(void)
{
  float variance;

  if (gyro_state.calibration_samples < 2U)
  {
    return 0U;
  }
  variance = calibration_m2_z / (float)(gyro_state.calibration_samples - 1U);
  return variance <= GYRO_CAL_VARIANCE_LIMIT_RAW;
}

static void Gyro_MarkOffline(void)
{
  gyro_state.connected = 0U;
  gyro_state.ready = 0U;
  gyro_state.calibrating = 0U;
  gyro_state.read_failures = 0U;
  gyro_state.raw_rate_dps = 0.0f;
  gyro_state.rate_dps = 0.0f;
  reconnect_tick = HAL_GetTick();
}

static uint8_t Gyro_TryReconnect(void)
{
  uint32_t now = HAL_GetTick();

  if ((now - reconnect_tick) < GYRO_RECONNECT_INTERVAL_MS)
  {
    return 0U;
  }

  reconnect_tick = now;
  if (Mpu6050_Init() == 0U)
  {
    return 0U;
  }

  gyro_state.connected = 1U;
  gyro_state.read_failures = 0U;
  Gyro_ResetCalibration(GYRO_WARMUP_SAMPLES);
  return 1U;
}

uint8_t Gyro_Init(void)
{
  memset(&gyro_state, 0, sizeof(gyro_state));
  gyro_state.connected = Mpu6050_Init();
  Gyro_ResetCalibration(GYRO_WARMUP_SAMPLES);
  reconnect_tick = HAL_GetTick();
  return gyro_state.connected;
}

void Gyro_Process(float dt_seconds, uint8_t vehicle_stationary)
{
  Mpu6050GyroRaw raw;
  float corrected_rate;
  float filter_rc;
  float filter_alpha;

  if (dt_seconds <= 0.0f)
  {
    return;
  }
  if (gyro_state.connected == 0U)
  {
    (void)Gyro_TryReconnect();
    return;
  }
  if (Mpu6050_ReadGyro(&raw) == 0U)
  {
    if (gyro_state.read_failures < 0xFFFFU)
    {
      ++gyro_state.read_failures;
    }
    if (gyro_state.read_failures >= GYRO_MAX_READ_FAILURES)
    {
      Gyro_MarkOffline();
    }
    return;
  }

  gyro_state.read_failures = 0U;
  gyro_state.raw_z = raw.z;

  if (warmup_remaining > 0U)
  {
    --warmup_remaining;
    return;
  }

  if (gyro_state.calibrating != 0U)
  {
    if (vehicle_stationary == 0U)
    {
      Gyro_ResetCalibration(GYRO_WARMUP_SAMPLES / 2U);
      return;
    }

    Gyro_AddCalibrationSample(raw.z);
    if (gyro_state.calibration_samples < GYRO_CALIBRATION_SAMPLES)
    {
      return;
    }
    if (Gyro_CalibrationIsStable() == 0U)
    {
      Gyro_ResetCalibration(GYRO_WARMUP_SAMPLES / 2U);
      return;
    }

    gyro_state.bias_z = calibration_mean_z;
    gyro_state.ready = 1U;
    gyro_state.calibrating = 0U;
    return;
  }

  corrected_rate = GYRO_Z_SIGN * ((float)raw.z - gyro_state.bias_z) /
                   GYRO_SCALE_LSB_PER_DPS;
  if ((vehicle_stationary != 0U) &&
      (Gyro_Abs(corrected_rate) < GYRO_BIAS_TRACK_LIMIT_DPS))
  {
    gyro_state.bias_z = (1.0f - GYRO_BIAS_TRACK_ALPHA) * gyro_state.bias_z +
                        GYRO_BIAS_TRACK_ALPHA * (float)raw.z;
    corrected_rate = GYRO_Z_SIGN * ((float)raw.z - gyro_state.bias_z) /
                     GYRO_SCALE_LSB_PER_DPS;
  }

  if (Gyro_Abs(corrected_rate) < GYRO_RATE_DEADBAND_DPS)
  {
    corrected_rate = 0.0f;
  }
  gyro_state.raw_rate_dps = corrected_rate;

  filter_rc = 1.0f / (GYRO_TWO_PI * GYRO_LPF_CUTOFF_HZ);
  filter_alpha = dt_seconds / (filter_rc + dt_seconds);
  gyro_state.rate_dps += filter_alpha * (corrected_rate - gyro_state.rate_dps);
  gyro_state.yaw_deg = Gyro_WrapAngle(gyro_state.yaw_deg +
                                      gyro_state.rate_dps * dt_seconds);
}

uint8_t Gyro_StartCalibration(void)
{
  if (gyro_state.connected == 0U)
  {
    gyro_state.connected = Mpu6050_Init();
  }
  Gyro_ResetCalibration(GYRO_WARMUP_SAMPLES);
  reconnect_tick = HAL_GetTick();
  return gyro_state.connected;
}

void Gyro_ZeroYaw(void)
{
  gyro_state.yaw_deg = 0.0f;
}

GyroSnapshot Gyro_GetSnapshot(void)
{
  return gyro_state;
}
