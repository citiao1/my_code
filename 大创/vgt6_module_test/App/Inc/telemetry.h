#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stdint.h>

#include "attitude_control.h"

#define TELEMETRY_HLOG_CAPACITY 1024U
#define TELEMETRY_HLOG_READ_MAX 8U

typedef struct
{
  uint32_t sequence;
  uint32_t tick;
  int32_t target_heading_mdeg;
  int32_t actual_heading_mdeg;
  int32_t heading_error_mdeg;
  int32_t target_rate_mdps;
  int32_t actual_rate_mdps;
  int32_t heading_output_mdps;
  int32_t rate_output_mrpm;
  uint8_t saturated;
  int16_t forward_rpm;
  int16_t left_rpm;
} TelemetryHeadingLogRecord;

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
void Telemetry_Hlog_Record(const AttitudeControlSnapshot *attitude);
void Telemetry_Hlog_Clear(void);
uint8_t Telemetry_Hlog_Start(void);
uint8_t Telemetry_Hlog_Stop(void);
uint8_t Telemetry_Hlog_IsActive(void);
uint16_t Telemetry_Hlog_Count(void);
uint8_t Telemetry_Hlog_SendStatus(void);
uint8_t Telemetry_Hlog_SendRead(uint32_t offset, uint32_t count);

#endif
