#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stdint.h>

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

#endif
