#ifndef GYRO_H
#define GYRO_H

#include <stdint.h>

typedef struct
{
  uint8_t connected;
  uint8_t ready;
  uint8_t calibrating;
  uint8_t who_am_i;
  uint8_t spi_transfer_ok;
  uint8_t spi_hal_status;
  uint8_t sensor_status;
  uint16_t calibration_samples;
  uint16_t read_failures;
  int16_t raw_gyro[3];
  int16_t raw_accel[3];
  int32_t gyro_mdps[3];
  int32_t accel_mg[3];
  float bias[3];
  int16_t raw_z;
  float bias_z;
  float raw_rate_dps;
  float rate_dps;
  float yaw_deg;
} GyroSnapshot;

uint8_t Gyro_Init(void);
void Gyro_Process(float dt_seconds, uint8_t stationary);
uint8_t Gyro_StartCalibration(void);
void Gyro_ZeroYaw(void);
GyroSnapshot Gyro_GetSnapshot(void);

#endif /* GYRO_H */
