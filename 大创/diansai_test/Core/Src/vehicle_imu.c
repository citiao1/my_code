#include "vehicle_imu.h"

#include "i2c.h"
#include "vehicle_config.h"
#include "vehicle_internal.h"
#include "vehicle_motor.h"

#include <math.h>

static uint32_t last_mpu_ms;
static uint8_t mpu_read_failures;

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

uint8_t VehicleImu_InitWithRetry(uint8_t attempts)
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

void VehicleImu_Update(uint32_t now)
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
      state.heading_control_enabled = 0U;
      VehicleMotor_Stop();
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

  if (state.throttle == 0 && state.steering == 0 && !state.heading_hold_active &&
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
