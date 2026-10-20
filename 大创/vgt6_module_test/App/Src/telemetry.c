#include "telemetry.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "battery.h"
#include "attitude_control.h"
#include "chassis_test.h"
#include "encoder.h"
#include "gyro.h"
#include "serial_dma.h"
#include "speed_control.h"
#include "stm32f4xx_hal.h"

/* 9600 baud provides ~960 bytes/s. Leave room for command ACKs and for
 * BLE notification buffering instead of saturating UART at 5 Hz. */
#define TELEMETRY_PERIOD_MS 300U

static uint8_t telemetry_periodic;
static uint32_t telemetry_tick;
static uint8_t attitude_stream;
static TelemetryHeadingLogRecord heading_log[TELEMETRY_HLOG_CAPACITY];
static uint16_t heading_log_start;
static uint16_t heading_log_count;
static uint32_t heading_log_sequence;
static uint8_t heading_log_active;
static uint8_t heading_log_overflow;

void Telemetry_Init(void)
{
  telemetry_periodic = 0U;
  attitude_stream = 0U;
  telemetry_tick = HAL_GetTick();
  Telemetry_Hlog_Clear();
}

void Telemetry_Process(void)
{
  uint32_t now = HAL_GetTick();

  if ((telemetry_periodic != 0U) &&
      ((now - telemetry_tick) >= (attitude_stream ? 200U : TELEMETRY_PERIOD_MS)))
  {
    telemetry_tick = now;
    if (attitude_stream) Telemetry_SendAttitude();
    else Telemetry_SendSpeedStatus();
  }
}

void Telemetry_SetPeriodic(uint8_t enable)
{
  attitude_stream = 0U;
  telemetry_periodic = (enable != 0U) ? 1U : 0U;
  telemetry_tick = HAL_GetTick();
}

void Telemetry_SetAttitude(uint8_t enable)
{
  Telemetry_SetPeriodic(enable);
  attitude_stream = 1U;
}

void Telemetry_SendAttitude(void)
{
  static char message[256];
  AttitudeControlSnapshot a = AttitudeControl_GetSnapshot();
  GyroSnapshot g = Gyro_GetSnapshot();
  (void)snprintf(message, sizeof(message),
      "ATEL,%lu,%u,%u,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%u,%u,%u\r\n",
      (unsigned long)HAL_GetTick(), a.mode, g.ready && !g.read_failures,
      (long)(a.target_dps * 1000), (long)(g.rate_dps * 1000),
      (long)(a.output_rpm * 1000), (long)(a.target_deg * 1000),
      (long)(g.yaw_deg * 1000), (long)(a.error_deg * 1000),
      (long)(a.output_dps * 1000), a.saturated,
      SpeedControl_GetSnapshot().enabled, Battery_GetSnapshot().voltage_mv);
  SerialDma_Write(message);
}

void Telemetry_Hlog_Record(const AttitudeControlSnapshot *attitude)
{
  TelemetryHeadingLogRecord *record;
  uint16_t index;

  if ((heading_log_active == 0U) || (attitude == NULL) ||
      ((attitude->mode != 1U) && (attitude->mode != 2U)))
  {
    return;
  }
  if (heading_log_count < TELEMETRY_HLOG_CAPACITY)
  {
    index = (uint16_t)((heading_log_start + heading_log_count) %
                       TELEMETRY_HLOG_CAPACITY);
    ++heading_log_count;
  }
  else
  {
    index = heading_log_start;
    heading_log_start = (uint16_t)((heading_log_start + 1U) %
                                   TELEMETRY_HLOG_CAPACITY);
    heading_log_overflow = 1U;
  }
  record = &heading_log[index];
  record->sequence = heading_log_sequence++;
  record->tick = HAL_GetTick();
  record->target_heading_mdeg = (int32_t)lroundf(attitude->target_deg * 1000.0f);
  record->actual_heading_mdeg = (int32_t)lroundf(attitude->feedback_deg * 1000.0f);
  record->heading_error_mdeg = (int32_t)lroundf(attitude->error_deg * 1000.0f);
  record->target_rate_mdps = (int32_t)lroundf(attitude->target_dps * 1000.0f);
  record->actual_rate_mdps = (int32_t)lroundf(attitude->feedback_dps * 1000.0f);
  record->heading_output_mdps = (int32_t)lroundf(attitude->output_dps * 1000.0f);
  record->rate_output_mrpm = (int32_t)lroundf(attitude->output_rpm * 1000.0f);
  record->saturated = attitude->saturated;
  record->mode = attitude->mode;
  record->forward_rpm = attitude->forward_rpm;
  record->left_rpm = attitude->left_rpm;
}

void Telemetry_Hlog_Clear(void)
{
  heading_log_start = 0U;
  heading_log_count = 0U;
  heading_log_sequence = 0U;
  heading_log_active = 0U;
  heading_log_overflow = 0U;
}

uint8_t Telemetry_Hlog_Start(void)
{
  if (heading_log_active != 0U)
  {
    return 0U;
  }
  heading_log_active = 1U;
  return 1U;
}

uint8_t Telemetry_Hlog_Stop(void)
{
  heading_log_active = 0U;
  return 1U;
}

uint8_t Telemetry_Hlog_IsActive(void)
{
  return heading_log_active;
}

uint16_t Telemetry_Hlog_Count(void)
{
  return heading_log_count;
}

uint8_t Telemetry_Hlog_SendStatus(void)
{
  char message[96];
  int length = snprintf(message, sizeof(message),
                        "HLOG,STATUS,%u,%u,%u,%u,%lu\r\n",
                        heading_log_active, heading_log_count,
                        TELEMETRY_HLOG_CAPACITY, heading_log_overflow,
                        (unsigned long)heading_log_sequence);
  if ((length <= 0) || ((size_t)length >= sizeof(message)))
  {
    return 0U;
  }
  return SerialDma_Write(message);
}

uint8_t Telemetry_Hlog_SendRead(uint32_t offset, uint32_t count)
{
  static char message[224];
  uint32_t index;
  uint32_t available;
  uint32_t actual_count;
  int length;
  TelemetryHeadingLogRecord record;

  if (offset >= heading_log_count)
  {
    actual_count = 0U;
  }
  else
  {
    available = (uint32_t)heading_log_count - offset;
    actual_count = (count < available) ? count : available;
  }
  for (index = 0U; index < actual_count; ++index)
  {
    record = heading_log[(heading_log_start + (uint16_t)offset +
                          (uint16_t)index) % TELEMETRY_HLOG_CAPACITY];
    length = snprintf(
        message, sizeof(message),
        "HLOG,DATA,%lu,%lu,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%u,%d,%d,%u\r\n",
        (unsigned long)record.sequence, (unsigned long)record.tick,
        (long)record.target_heading_mdeg, (long)record.actual_heading_mdeg,
        (long)record.heading_error_mdeg, (long)record.target_rate_mdps,
        (long)record.actual_rate_mdps, (long)record.heading_output_mdps,
        (long)record.rate_output_mrpm, record.saturated,
        (int)record.forward_rpm, (int)record.left_rpm, record.mode);
    if ((length <= 0) || ((size_t)length >= sizeof(message)) ||
        (SerialDma_Write(message) == 0U))
    {
      return 0U;
    }
  }
  length = snprintf(message, sizeof(message),
                    "HLOG,END,%lu,%lu,%u\r\n",
                    (unsigned long)offset, (unsigned long)actual_count,
                    heading_log_count);
  if ((length <= 0) || ((size_t)length >= sizeof(message)))
  {
    return 0U;
  }
  return SerialDma_Write(message);
}

uint8_t Telemetry_IsPeriodic(void)
{
  return telemetry_periodic;
}

/* Formatting is main-loop-only; SerialDma_Write copies before reuse.
 * Keep large work buffers out of nested command/telemetry stack frames. */
static void Telemetry_SendImu(void)
{
  static char message[256];
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
}

void Telemetry_SendNow(void)
{
  Telemetry_SendImu();
  Telemetry_SendEncoders();
  Telemetry_SendBattery();
  Telemetry_SendDriver();
  Telemetry_SendSpeedStatus();
}

void Telemetry_SendEncoders(void)
{
  static char message[256];
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

void Telemetry_SendSpeedStatus(void)
{
  /*
   * Keep TEL within the UART TX queue's 256-byte per-message limit.
   * The pure BM50 console only consumes the 27-field speed format; the
   * previous 48-field vehicle format could be silently rejected by
   * SerialDma_Write() after the tick/diagnostic values grew.
   */
  static char message[256];
  GyroSnapshot gyro = Gyro_GetSnapshot();
  BatterySnapshot battery = Battery_GetSnapshot();
  SpeedControlSnapshot speed = SpeedControl_GetSnapshot();
  static int32_t fields[27];
  int32_t gyro_raw_mdps = (int32_t)(gyro.raw_rate_dps * 1000.0f);
  int32_t gyro_rate_mdps = (int32_t)(gyro.rate_dps * 1000.0f);
  int32_t yaw_mdeg = (int32_t)(gyro.yaw_deg * 1000.0f);
  uint32_t index;
  int length;

  memset(fields, 0, sizeof(fields));
  fields[1] = (int32_t)HAL_GetTick();
  fields[3] = speed.enabled;
  for (index = 0U; index < 4U; ++index)
  {
    fields[4U + index] = speed.target_rpm[index];
    fields[8U + index] = speed.output_percent[index];
    fields[12U + index] = speed.actual_rpm_x10[index];
    fields[16U + index] = speed.target_rpm[index];
  }
  fields[20] = battery.voltage_mv;
  fields[21] = gyro_raw_mdps;
  fields[22] = gyro_rate_mdps;
  fields[23] = gyro.ready;
  fields[24] = gyro_rate_mdps;
  fields[25] = yaw_mdeg;
  fields[26] = speed.enabled;

  length = snprintf(message, sizeof(message), "TEL");
  for (index = 1U; (index < 27U) && (length > 0) &&
                   ((uint32_t)length < sizeof(message)); ++index)
  {
    length += snprintf(&message[length], sizeof(message) - (size_t)length,
                       ",%ld", (long)fields[index]);
  }
  if ((length > 0) && ((size_t)length < (sizeof(message) - 3U)))
  {
    (void)snprintf(&message[length], sizeof(message) - (size_t)length,
                   "\r\n");
  }
  SerialDma_Write(message);
}
