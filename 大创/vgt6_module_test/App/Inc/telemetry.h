#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stdint.h>

#include "attitude_control.h"
#include "speed_control.h"
#include "encoder.h"

#define TELEMETRY_HLOG_CAPACITY 1024U
#define TELEMETRY_HLOG_READ_MAX 8U

typedef struct
{
  uint32_t sequence;
  uint32_t tick;
  uint32_t tick_us;
  int32_t target_heading_mdeg;
  int32_t actual_heading_mdeg;
  int32_t heading_error_mdeg;
  int32_t target_rate_mdps;
  int32_t actual_rate_mdps;
  int32_t heading_output_mdps;
  int32_t rate_output_mrpm;
  uint8_t saturated;
  uint8_t mode; /* 1 rate PI; 2 heading PD -> rate PI. */
  int16_t forward_rpm;
  int16_t left_rpm;
} TelemetryHeadingLogRecord;

typedef struct
{
  uint32_t sequence, tick, tick_us;
  int32_t target[4], actual_x10[4], raw_x10[4], pwm_mpercent[4];
  uint16_t battery_mv;
  int16_t distance_mm;
  uint8_t enabled, armed, mode, saturated_mask, tof_limited, tof_valid, encoder_ready;
} TelemetryWheelLogRecord;

void Telemetry_Init(void);
void Telemetry_Process(void);
void Telemetry_SetPeriodic(uint8_t enable);
uint8_t Telemetry_IsPeriodic(void);
void Telemetry_SendNow(void);
void Telemetry_SendEncoders(void);
void Telemetry_SendBattery(void);
void Telemetry_SendDriver(void);
void Telemetry_SendSpeedStatus(void);
void Telemetry_SetAttitude(uint8_t enable);
void Telemetry_SendAttitude(void);
void Telemetry_SendTof(void);
void Telemetry_SetTof(uint8_t enable);
void Telemetry_SetFollow(uint8_t enable);
void Telemetry_SendFollow(void);
void Telemetry_Hlog_Record(const AttitudeControlSnapshot *attitude,
                           uint32_t tick_ms, uint32_t tick_us);
void Telemetry_Hlog_Clear(void);
uint8_t Telemetry_Hlog_Start(void);
uint8_t Telemetry_Hlog_Stop(void);
uint8_t Telemetry_Hlog_IsActive(void);
uint16_t Telemetry_Hlog_Count(void);
uint8_t Telemetry_Hlog_SendStatus(void);
uint8_t Telemetry_Hlog_SendRead(uint32_t offset, uint32_t count);
void Telemetry_Wlog_Record(const SpeedControlSnapshot *speed,
    const EncoderSnapshot *encoder, uint32_t tick_ms, uint32_t tick_us);
void Telemetry_Wlog_Clear(void);
uint8_t Telemetry_Wlog_Start(void);
uint8_t Telemetry_Wlog_SendStatus(void);
uint8_t Telemetry_Wlog_SendRead(uint32_t offset, uint32_t count);

#endif
