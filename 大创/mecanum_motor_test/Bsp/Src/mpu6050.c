#include "mpu6050.h"

#include "i2c.h"

#define MPU6050_ADDRESS             (0x68U << 1)
#define MPU6050_REG_SMPLRT_DIV      0x19U
#define MPU6050_REG_CONFIG          0x1AU
#define MPU6050_REG_GYRO_CONFIG     0x1BU
#define MPU6050_REG_ACCEL_CONFIG    0x1CU
#define MPU6050_REG_GYRO_XOUT_H     0x43U
#define MPU6050_REG_PWR_MGMT_1      0x6BU
#define MPU6050_REG_WHO_AM_I        0x75U
#define MPU6050_WHO_AM_I_VALUE      0x68U
#define MPU6050_I2C_TIMEOUT_MS      5U

static uint8_t Mpu6050_Write(uint8_t reg, uint8_t value)
{
  return HAL_I2C_Mem_Write(&hi2c2, MPU6050_ADDRESS, reg, I2C_MEMADD_SIZE_8BIT,
                           &value, 1U, MPU6050_I2C_TIMEOUT_MS) == HAL_OK;
}

static uint8_t Mpu6050_Read(uint8_t reg, uint8_t *data, uint16_t length)
{
  return HAL_I2C_Mem_Read(&hi2c2, MPU6050_ADDRESS, reg, I2C_MEMADD_SIZE_8BIT,
                          data, length, MPU6050_I2C_TIMEOUT_MS) == HAL_OK;
}

uint8_t Mpu6050_Init(void)
{
  uint8_t who = 0U;

  if (HAL_I2C_IsDeviceReady(&hi2c2, MPU6050_ADDRESS, 3U, 20U) != HAL_OK)
  {
    return 0U;
  }
  if ((Mpu6050_Read(MPU6050_REG_WHO_AM_I, &who, 1U) == 0U) ||
      (who != MPU6050_WHO_AM_I_VALUE))
  {
    return 0U;
  }

  if (Mpu6050_Write(MPU6050_REG_PWR_MGMT_1, 0x80U) == 0U)
  {
    return 0U;
  }
  HAL_Delay(100U);

  if (Mpu6050_Write(MPU6050_REG_PWR_MGMT_1, 0x01U) == 0U)
  {
    return 0U;
  }
  HAL_Delay(10U);

  /* DLPF_CFG=3, +/-250 dps, 1 kHz / (19 + 1) = 50 Hz. */
  if ((Mpu6050_Write(MPU6050_REG_SMPLRT_DIV, 19U) == 0U) ||
      (Mpu6050_Write(MPU6050_REG_CONFIG, 3U) == 0U) ||
      (Mpu6050_Write(MPU6050_REG_GYRO_CONFIG, 0U) == 0U) ||
      (Mpu6050_Write(MPU6050_REG_ACCEL_CONFIG, 0U) == 0U))
  {
    return 0U;
  }
  return 1U;
}

uint8_t Mpu6050_ReadGyro(Mpu6050GyroRaw *gyro)
{
  uint8_t raw[6];

  if ((gyro == 0) || (Mpu6050_Read(MPU6050_REG_GYRO_XOUT_H, raw, sizeof(raw)) == 0U))
  {
    return 0U;
  }

  gyro->x = (int16_t)((raw[0] << 8) | raw[1]);
  gyro->y = (int16_t)((raw[2] << 8) | raw[3]);
  gyro->z = (int16_t)((raw[4] << 8) | raw[5]);
  return 1U;
}
