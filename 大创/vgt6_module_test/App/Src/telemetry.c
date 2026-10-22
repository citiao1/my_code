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
#include "tof.h"

/* 9600 baud provides ~960 bytes/s. Leave room for command ACKs and for
 * BLE notification buffering instead of saturating UART at 5 Hz. */
#define TELEMETRY_PERIOD_MS 300U

static uint8_t telemetry_periodic;
static uint32_t telemetry_tick;
static uint8_t attitude_stream;
/* One capture at a time; union keeps four-wheel 100 Hz recording inside SRAM.
 * CLEAR explicitly selects the format. No dynamic allocation or ISR logging. */
static union
{
  TelemetryHeadingLogRecord heading[TELEMETRY_HLOG_CAPACITY];
  TelemetryWheelLogRecord wheel[TELEMETRY_HLOG_CAPACITY];
} log_storage;
#define heading_log log_storage.heading
static uint8_t wheel_log_selected;
static uint16_t heading_log_start;
static uint16_t heading_log_count;
static uint32_t heading_log_sequence;
static uint8_t heading_log_active;
static uint8_t heading_log_overflow;
static uint8_t tof_stream;
static uint32_t tof_tick, hlog_read_tick;
static uint8_t hlog_read_seen;
static uint8_t periodic_send;
static TofSnapshot last_tof_report;
static uint32_t tof_report_tick;
static uint8_t tof_report_seen;

static uint8_t write_live(const char *message)
{
  if (periodic_send) return SerialDma_WriteTelemetry(message);
  return SerialDma_Write(message);
}

void Telemetry_Init(void)
{
  telemetry_periodic = 0U;
  periodic_send = 0U;
  tof_report_seen = 0U;
  attitude_stream = 0U;
  telemetry_tick = HAL_GetTick();
  Telemetry_Hlog_Clear();
  tof_stream = hlog_read_seen = 0U;
  tof_tick = HAL_GetTick();
}

void Telemetry_Process(void)
{
  uint32_t now = HAL_GetTick();
  uint8_t ranging_stream = tof_stream && !heading_log_active &&
      (!hlog_read_seen || now - hlog_read_tick >= 1500U);
  uint32_t tof_period = telemetry_periodic ? 200U : 100U;
  uint32_t status_period = ranging_stream ? 500U :
      (attitude_stream ? 200U : TELEMETRY_PERIOD_MS);
  periodic_send = 1U;

  /* Distance: 10 Hz alone, 5 Hz alongside 2 Hz TEL/ATEL. At 9600 baud,
   * even 55-byte TOF + 255-byte TEL stays below 800 bytes/s. Captures and
   * downloads retain their UART budget; the local guard never pauses. */
  if (ranging_stream && now - tof_tick >= tof_period)
  {
    TofSnapshot tof = Tof_GetSnapshot();
    tof_tick = now;
    /* Repeating one invalid result at 5..10 Hz wastes BLE UART capacity.
     * Report every transition promptly, then a 1 Hz invalid-state heartbeat.
     * Valid ranges retain their normal rate; local ranging never pauses. */
    if (tof.valid || !tof_report_seen || last_tof_report.valid ||
        tof.connected != last_tof_report.connected ||
        tof.range_status != last_tof_report.range_status ||
        tof.errors != last_tof_report.errors || now - tof_report_tick >= 1000U)
      Telemetry_SendTof();
  }

  if ((telemetry_periodic != 0U) &&
      ((now - telemetry_tick) >= status_period))
  {
    telemetry_tick = now;
    if (attitude_stream) Telemetry_SendAttitude();
    else Telemetry_SendSpeedStatus();
  }
  periodic_send = 0U;
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

void Telemetry_SetTof(uint8_t enable)
{
  tof_stream = enable != 0U;
  tof_report_seen = 0U;
  tof_tick = HAL_GetTick();
}

void Telemetry_SendTof(void)
{
  char message[112];
  TofSnapshot tof = Tof_GetSnapshot();
  (void)snprintf(message, sizeof(message), "TOF,%lu,%u,%u,%d,%u,%lu,%lu\r\n",
      (unsigned long)HAL_GetTick(), tof.connected, tof.valid,
      tof.valid ? (int)tof.distance_mm : -1, tof.range_status,
      (unsigned long)tof.age_ms, (unsigned long)tof.errors);
  if (write_live(message))
  {
    last_tof_report = tof;
    tof_report_tick = HAL_GetTick();
    tof_report_seen = 1U;
  }
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
  write_live(message);
}

void Telemetry_Hlog_Record(const AttitudeControlSnapshot *attitude,
                           uint32_t tick_ms, uint32_t tick_us)
{
  TelemetryHeadingLogRecord *record;
  uint16_t index;

  if (wheel_log_selected || (heading_log_active == 0U) || (attitude == NULL) ||
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
  record->tick = tick_ms;
  record->tick_us = tick_us;
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
  wheel_log_selected = 0U;
  heading_log_start = 0U;
  heading_log_count = 0U;
  heading_log_sequence = 0U;
  heading_log_active = 0U;
  heading_log_overflow = 0U;
}

uint8_t Telemetry_Hlog_Start(void)
{
  if (wheel_log_selected || heading_log_active != 0U)
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
                        wheel_log_selected ? 0U : heading_log_active,
                        wheel_log_selected ? 0U : heading_log_count,
                        TELEMETRY_HLOG_CAPACITY,
                        wheel_log_selected ? 0U : heading_log_overflow,
                        (unsigned long)(wheel_log_selected ? 0U : heading_log_sequence));
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
  hlog_read_seen = 1U;
  hlog_read_tick = HAL_GetTick();

  if (wheel_log_selected) return SerialDma_Write("ERR,HLOG,WLOG_SELECTED\r\n");

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
        "HLOG,DATA,%lu,%lu,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%u,%d,%d,%u,%lu\r\n",
        (unsigned long)record.sequence, (unsigned long)record.tick,
        (long)record.target_heading_mdeg, (long)record.actual_heading_mdeg,
        (long)record.heading_error_mdeg, (long)record.target_rate_mdps,
        (long)record.actual_rate_mdps, (long)record.heading_output_mdps,
        (long)record.rate_output_mrpm, record.saturated,
        (int)record.forward_rpm, (int)record.left_rpm, record.mode,
        (unsigned long)record.tick_us);
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

void Telemetry_Wlog_Clear(void)
{
  Telemetry_Hlog_Clear();
  wheel_log_selected = 1U;
}

uint8_t Telemetry_Wlog_Start(void)
{
  if (!wheel_log_selected || heading_log_active) return 0U;
  heading_log_active = 1U;
  return 1U;
}

void Telemetry_Wlog_Record(const SpeedControlSnapshot *speed,
    const EncoderSnapshot *encoder, uint32_t tick_ms, uint32_t tick_us)
{
  TelemetryWheelLogRecord *record;
  TofSnapshot tof;
  uint32_t i;
  uint16_t index;
  if (!wheel_log_selected || !heading_log_active) return;
  if (heading_log_count < TELEMETRY_HLOG_CAPACITY)
  {
    index = (uint16_t)((heading_log_start + heading_log_count++) % TELEMETRY_HLOG_CAPACITY);
  }
  else
  {
    index = heading_log_start;
    heading_log_start = (uint16_t)((heading_log_start + 1U) % TELEMETRY_HLOG_CAPACITY);
    heading_log_overflow = 1U;
  }
  record = &log_storage.wheel[index];
  record->sequence = heading_log_sequence++;
  record->tick = tick_ms;
  record->tick_us = tick_us;
  for (i = 0U; i < 4U; ++i)
  {
    record->target[i] = speed->target_rpm[i];
    record->actual_x10[i] = speed->actual_rpm_x10[i];
    record->raw_x10[i] = encoder->rpm_x10[i];
    record->pwm_mpercent[i] = speed->output_mpercent[i];
  }
  record->enabled = speed->enabled;
  record->armed = ChassisTest_GetSnapshot().armed;
  record->mode = AttitudeControl_GetSnapshot().mode;
  record->encoder_ready = encoder->ready;
  record->saturated_mask = speed->saturated_mask;
  record->tof_limited = speed->tof_limited;
  record->battery_mv = Battery_GetSnapshot().voltage_mv;
  tof = Tof_GetSnapshot();
  record->tof_valid = tof.connected && tof.valid;
  record->distance_mm = record->tof_valid ? (int16_t)tof.distance_mm : -1;
}

uint8_t Telemetry_Wlog_SendStatus(void)
{
  char message[96];
  (void)snprintf(message, sizeof(message), "WLOG,STATUS,%u,%u,1024,%u,%lu\r\n",
      wheel_log_selected ? heading_log_active : 0U,
      wheel_log_selected ? heading_log_count : 0U,
      wheel_log_selected ? heading_log_overflow : 0U,
      (unsigned long)(wheel_log_selected ? heading_log_sequence : 0U));
  return SerialDma_Write(message);
}

uint8_t Telemetry_Wlog_SendRead(uint32_t offset, uint32_t count)
{
  static char message[256];
  uint32_t n, i, count_read = 0U;
  int length;
  hlog_read_seen = 1U;
  hlog_read_tick = HAL_GetTick();
  if (!wheel_log_selected) return SerialDma_Write("ERR,WLOG,USE_CLEAR_FIRST\r\n");
  if (offset < heading_log_count)
  {
    count_read = heading_log_count - offset;
    if (count_read > count) count_read = count;
  }
  for (n = 0U; n < count_read; ++n)
  {
    const TelemetryWheelLogRecord *r = &log_storage.wheel[
        (heading_log_start + offset + n) % TELEMETRY_HLOG_CAPACITY];
    length = snprintf(message, sizeof(message), "WLOG,DATA,%lu,%lu,%lu,%u,%u,%u",
        (unsigned long)r->sequence, (unsigned long)r->tick,
        (unsigned long)r->tick_us, r->enabled, r->armed, r->mode);
    for (i = 0U; i < 4U; ++i)
      length += snprintf(message + length, sizeof(message) - (size_t)length,
          ",%ld,%ld,%ld,%ld", (long)r->target[i], (long)r->actual_x10[i],
          (long)r->raw_x10[i], (long)r->pwm_mpercent[i]);
    length += snprintf(message + length, sizeof(message) - (size_t)length,
        ",%u,%u,%u,%d,%u,%u\r\n", r->saturated_mask, r->tof_limited,
        r->battery_mv, r->distance_mm, r->tof_valid, r->encoder_ready);
    if (length <= 0 || (size_t)length >= sizeof(message) ||
        !SerialDma_Write(message)) return 0U;
  }
  (void)snprintf(message, sizeof(message), "WLOG,END,%lu,%lu,%u\r\n",
      (unsigned long)offset, (unsigned long)count_read, heading_log_count);
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
  write_live(message);
}
