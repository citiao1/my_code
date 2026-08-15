#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>

typedef struct
{
  int16_t x;
  int16_t y;
  int16_t z;
} Mpu6050GyroRaw;

uint8_t Mpu6050_Init(void);
uint8_t Mpu6050_ReadGyro(Mpu6050GyroRaw *gyro);

#endif
