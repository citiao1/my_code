#ifndef IMU660RC_H
#define IMU660RC_H

#include <stdint.h>

#define IMU660RC_WHO_AM_I_VALUE 0x70U

typedef struct
{
  int16_t x;
  int16_t y;
  int16_t z;
} Imu660rcAxis3;

typedef struct
{
  uint8_t who_am_i;
  uint8_t status_reg;
  uint8_t transfer_ok;
  uint8_t hal_status;
} Imu660rcDiagnostics;

/* Read the primary UI-channel output registers of the LSM6DSV16X used by
 * IMU660RC.  The selected configuration is +/-500 dps and +/-4 g. */
uint8_t Imu660rc_Init(void);
uint8_t Imu660rc_ReadWhoAmI(uint8_t *who_am_i);
uint8_t Imu660rc_ReadStatus(uint8_t *status);
uint8_t Imu660rc_ReadRaw(Imu660rcAxis3 *gyro, Imu660rcAxis3 *accel);
void Imu660rc_GetDiagnostics(Imu660rcDiagnostics *diagnostics);

#endif /* IMU660RC_H */
