#include "telemetry.h"

#include <stdio.h>

#include "battery.h"
#include "chassis_test.h"
#include "encoder.h"
#include "gyro.h"
#include "serial_dma.h"
#include "stm32f4xx_hal.h"

void Telemetry_Init(void)
{
}

void Telemetry_Process(void)
{
  /* Deliberately empty: telemetry is command-driven to keep the 9600-baud
   * Bluetooth link clear for interactive motor testing. */
}

void Telemetry_SendNow(void)
{
  char message[256];
  GyroSnapshot gyro = Gyro_GetSnapshot();

  (void)snprintf(message, sizeof(message),
                 "IMU,%lu,%u,%u,%u,0x%02X,%u,%d,%d,%d,%ld,%ld,%ld,"
                 "%d,%d,%d,%ld,%ld,%ld,%ld,0x%02X,%u,%u\r\n",
                 (unsigned long)HAL_GetTick(), gyro.connected, gyro.ready,
                 gyro.calibrating, gyro.who_am_i, gyro.read_failures,
                 gyro.raw_gyro[0], gyro.raw_gyro[1], gyro.raw_gyro[2],
                 (long)gyro.gyro_mdps[0], (long)gyro.gyro_mdps[1],
                 (long)gyro.gyro_mdps[2], gyro.raw_accel[0],
                 gyro.raw_accel[1], gyro.raw_accel[2],
                 (long)gyro.accel_mg[0], (long)gyro.accel_mg[1],
                 (long)gyro.accel_mg[2], (long)(gyro.yaw_deg * 1000.0f),
                 gyro.sensor_status, gyro.spi_transfer_ok,
                 gyro.spi_hal_status);
  SerialDma_Write(message);
  Telemetry_SendEncoders();
  Telemetry_SendBattery();
  Telemetry_SendDriver();
}

void Telemetry_SendEncoders(void)
{
  char message[256];
  EncoderSnapshot encoder = Encoder_GetSnapshot();

  (void)snprintf(message, sizeof(message),
                 "ENC,%lu,%u,%u,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,"
                 "%ld,%ld,%ld,%ld\r\n",
                 (unsigned long)HAL_GetTick(), encoder.ready,
                 encoder.counts_per_revolution,
                 (long)encoder.total[0], (long)encoder.total[1],
                 (long)encoder.total[2], (long)encoder.total[3],
                 (long)encoder.delta[0], (long)encoder.delta[1],
                 (long)encoder.delta[2], (long)encoder.delta[3],
                 (long)encoder.rpm_x10[0], (long)encoder.rpm_x10[1],
                 (long)encoder.rpm_x10[2], (long)encoder.rpm_x10[3]);
  SerialDma_Write(message);
}

void Telemetry_SendBattery(void)
{
  char message[96];
  BatterySnapshot battery = Battery_GetSnapshot();

  (void)snprintf(message, sizeof(message), "BAT,%lu,%u,%u,%u,%u\r\n",
                 (unsigned long)HAL_GetTick(), battery.ready,
                 battery.raw_adc, battery.voltage_mv,
                 battery.read_failures);
  SerialDma_Write(message);
}

void Telemetry_SendDriver(void)
{
  char message[96];
  ChassisTestSnapshot driver = ChassisTest_GetSnapshot();

  (void)snprintf(message, sizeof(message),
                 "DRV,%lu,%u,%u,%u,%c,%d,%u,%lu\r\n",
                 (unsigned long)HAL_GetTick(), driver.motor_ready,
                 driver.armed, driver.active,
                 (char)('A' + (uint8_t)driver.channel), driver.direction,
                 driver.duty_percent, (unsigned long)driver.remaining_ms);
  SerialDma_Write(message);
}
