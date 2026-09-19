#ifndef TELEMETRY_H
#define TELEMETRY_H

void Telemetry_Init(void);
void Telemetry_Process(void);
void Telemetry_SendNow(void);
void Telemetry_SendEncoders(void);
void Telemetry_SendBattery(void);
void Telemetry_SendDriver(void);

#endif
