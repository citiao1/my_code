#ifndef VEHICLE_LINE_H
#define VEHICLE_LINE_H

#include <stdint.h>

void VehicleLine_Init(uint32_t now);
void VehicleLine_Update(uint32_t now);
void VehicleLine_Stop(void);
void VehicleLine_SetSpeedPercent(uint8_t percent);
uint8_t VehicleLine_GetSpeedPercent(void);
void VehicleLine_SetDirectionGains(float kp, float kd);
float VehicleLine_GetDirectionKp(void);
float VehicleLine_GetDirectionKd(void);
void VehicleLine_CaptureWhite(void);
uint8_t VehicleLine_CaptureBlack(void);
void VehicleLine_SetCornerAdvanceMm(uint16_t millimeters);
uint16_t VehicleLine_GetCornerAdvanceMm(void);
void VehicleLine_SetCornerTurnDeg(uint16_t degrees);
uint16_t VehicleLine_GetCornerTurnDeg(void);
uint8_t VehicleLine_IsEngaged(void);
uint8_t VehicleLine_IsControlActive(void);
uint8_t VehicleLine_IsRunning(void);
float VehicleLine_GetTargetYawRate(void);

#endif
