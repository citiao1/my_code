#include "gyro.h"

#include <string.h>

#include "imu660rc.h"
#include "stm32f4xx_hal.h"

#define GYRO_CALIBRATION_SAMPLES 200U
#define GYRO_WARMUP_SAMPLES       20U
#define GYRO_SCALE_MDPS_PER_LSB   17.50f
#define ACCEL_SCALE_MG_PER_LSB     0.122f
#define GYRO_DEADBAND_DPS          0.05f
#define GYRO_RECONNECT_INTERVAL_MS 1000U
#define GYRO_MAX_READ_FAILURES     5U
#define GYRO_TWO_PI                6.2831853f
#define GYRO_LPF_CUTOFF_HZ         5.0f

static GyroSnapshot gyro_state;
static uint16_t warmup_remaining;
static float calibration_mean[3];
static uint32_t reconnect_tick;

static void Gyro_UpdateDiagnostics(void)
{
  Imu660rcDiagnostics diagnostics;

  Imu660rc_GetDiagnostics(&diagnostics);
  gyro_state.who_am_i = diagnostics.who_am_i;
  gyro_state.sensor_status = diagnostics.status_reg;
  gyro_state.spi_transfer_ok = diagnostics.transfer_ok;
  gyro_state.spi_hal_status = diagnostics.hal_status;
}

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

static int32_t Gyro_RoundToInt(float value)
{
  return (int32_t)((value >= 0.0f) ? (value + 0.5f) : (value - 0.5f));
}

static void Gyro_ResetCalibration(uint16_t warmup_samples)
{
  uint8_t axis;

  gyro_state.ready = 0U;
  gyro_state.calibrating = gyro_state.connected;
  gyro_state.calibration_samples = 0U;
  gyro_state.raw_rate_dps = 0.0f;
  gyro_state.rate_dps = 0.0f;
  warmup_remaining = warmup_samples;
  for (axis = 0U; axis < 3U; ++axis)
  {
    calibration_mean[axis] = 0.0f;
    gyro_state.bias[axis] = 0.0f;
  }
  gyro_state.bias_z = 0.0f;
}

static void Gyro_AddCalibrationSample(const Imu660rcAxis3 *raw)
{
  float sample[3];
  float delta;
  uint8_t axis;

  sample[0] = (float)raw->x;
  sample[1] = (float)raw->y;
  sample[2] = (float)raw->z;
  ++gyro_state.calibration_samples;
  for (axis = 0U; axis < 3U; ++axis)
  {
    delta = sample[axis] - calibration_mean[axis];
    calibration_mean[axis] += delta /
                              (float)gyro_state.calibration_samples;
  }
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
  if (Imu660rc_Init() == 0U)
  {
    Gyro_UpdateDiagnostics();
    return 0U;
  }

  Gyro_UpdateDiagnostics();
  gyro_state.connected = 1U;
  gyro_state.read_failures = 0U;
  Gyro_ResetCalibration(GYRO_WARMUP_SAMPLES);
  return 1U;
}

uint8_t Gyro_Init(void)
{
  uint8_t who_am_i = 0U;

  memset(&gyro_state, 0, sizeof(gyro_state));
  (void)Imu660rc_ReadWhoAmI(&gyro_state.who_am_i);
  Gyro_UpdateDiagnostics();
  gyro_state.connected = Imu660rc_Init();
  (void)Imu660rc_ReadWhoAmI(&who_am_i);
  if (gyro_state.connected != 0U)
  {
    gyro_state.who_am_i = who_am_i;
  }
  Gyro_UpdateDiagnostics();
  Gyro_ResetCalibration(GYRO_WARMUP_SAMPLES);
  reconnect_tick = HAL_GetTick();
  return gyro_state.connected;
}

void Gyro_Process(float dt_seconds, uint8_t stationary)
{
  Imu660rcAxis3 raw_gyro;
  Imu660rcAxis3 raw_accel;
  float corrected_rate;
  float filter_rc;
  float filter_alpha;
  float raw_rate[3];
  uint8_t axis;

  if (dt_seconds <= 0.0f)
  {
    return;
  }
  if (gyro_state.connected == 0U)
  {
    (void)Gyro_TryReconnect();
    return;
  }
  if (Imu660rc_ReadRaw(&raw_gyro, &raw_accel) == 0U)
  {
    Gyro_UpdateDiagnostics();
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
  gyro_state.raw_gyro[0] = raw_gyro.x;
  gyro_state.raw_gyro[1] = raw_gyro.y;
  gyro_state.raw_gyro[2] = raw_gyro.z;
  gyro_state.raw_accel[0] = raw_accel.x;
  gyro_state.raw_accel[1] = raw_accel.y;
  gyro_state.raw_accel[2] = raw_accel.z;
  gyro_state.raw_z = raw_gyro.z;
  (void)Imu660rc_ReadStatus(&gyro_state.sensor_status);
  Gyro_UpdateDiagnostics();

  for (axis = 0U; axis < 3U; ++axis)
  {
    raw_rate[axis] = (float)gyro_state.raw_gyro[axis];
    gyro_state.accel_mg[axis] = Gyro_RoundToInt(
        (float)gyro_state.raw_accel[axis] * ACCEL_SCALE_MG_PER_LSB);
    gyro_state.gyro_mdps[axis] = Gyro_RoundToInt(
        raw_rate[axis] * GYRO_SCALE_MDPS_PER_LSB);
  }

  if (warmup_remaining > 0U)
  {
    --warmup_remaining;
    return;
  }

  if (gyro_state.calibrating != 0U)
  {
    if (stationary == 0U)
    {
      Gyro_ResetCalibration(GYRO_WARMUP_SAMPLES / 2U);
      return;
    }
    Gyro_AddCalibrationSample(&raw_gyro);
    if (gyro_state.calibration_samples < GYRO_CALIBRATION_SAMPLES)
    {
      return;
    }
    for (axis = 0U; axis < 3U; ++axis)
    {
      gyro_state.bias[axis] = calibration_mean[axis];
    }
    gyro_state.bias_z = gyro_state.bias[2];
    gyro_state.ready = 1U;
    gyro_state.calibrating = 0U;
    return;
  }

  for (axis = 0U; axis < 3U; ++axis)
  {
    gyro_state.gyro_mdps[axis] = Gyro_RoundToInt(
        (raw_rate[axis] - gyro_state.bias[axis]) *
        GYRO_SCALE_MDPS_PER_LSB);
  }
  corrected_rate = (raw_rate[2] - gyro_state.bias[2]) *
                   (GYRO_SCALE_MDPS_PER_LSB / 1000.0f);
  if ((stationary != 0U) && (Gyro_Abs(corrected_rate) < 1.5f))
  {
    gyro_state.bias[2] = 0.998f * gyro_state.bias[2] + 0.002f * raw_rate[2];
    gyro_state.bias_z = gyro_state.bias[2];
    corrected_rate = (raw_rate[2] - gyro_state.bias[2]) *
                     (GYRO_SCALE_MDPS_PER_LSB / 1000.0f);
  }

  if (Gyro_Abs(corrected_rate) < GYRO_DEADBAND_DPS)
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
    gyro_state.connected = Imu660rc_Init();
    Gyro_UpdateDiagnostics();
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
