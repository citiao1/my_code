#ifndef VEHICLE_MOTOR_H
#define VEHICLE_MOTOR_H

#include "vehicle_types.h"

void VehicleMotor_Init(void);
uint8_t VehicleMotor_IsEnabled(void);
void VehicleMotor_Stop(void);
void VehicleMotor_UpdateFeedback(void);
void VehicleMotor_SetLeftPwm(int16_t physical_pwm);
void VehicleMotor_SetRightPwm(int16_t physical_pwm);
int16_t VehicleMotor_SpeedPidStep(SpeedPidState *pid, float target, float measured, float *error_out);

#endif
