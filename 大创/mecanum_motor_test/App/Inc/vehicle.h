#ifndef VEHICLE_H
#define VEHICLE_H

#include <stdint.h>

#include "motor.h"

typedef enum
{
  VEHICLE_STOP = 0,
  VEHICLE_FORWARD,
  VEHICLE_BACKWARD,
  VEHICLE_LEFT,
  VEHICLE_RIGHT,
  VEHICLE_ROTATE_LEFT,
  VEHICLE_ROTATE_RIGHT,
  VEHICLE_DRIVE,
  VEHICLE_HEADING_HOLD,
  VEHICLE_WHEEL_TEST
} VehicleMode;

uint8_t Vehicle_Init(void);
void Vehicle_Process(void);
uint8_t Vehicle_Move(VehicleMode mode);
uint8_t Vehicle_Drive(int16_t forward_rpm, int16_t left_rpm, int16_t yaw_rate_dps);
uint8_t Vehicle_StepHeading(int16_t delta_deg);
uint8_t Vehicle_HoldHeading(void);
uint8_t Vehicle_Jog(MotorId id, int8_t direction);
void Vehicle_Stop(void);
uint8_t Vehicle_SetSpeed(uint16_t rpm);
uint16_t Vehicle_GetSpeed(void);
uint8_t Vehicle_IsReady(void);
const char *Vehicle_GetModeText(void);

#endif
