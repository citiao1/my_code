#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

#include "motor.h"

uint8_t Encoder_Init(void);
void Encoder_Sample(int32_t delta[MOTOR_COUNT]);
void Encoder_ResetAll(void);
int32_t Encoder_Get(MotorId id);
void Encoder_GetAll(int32_t count[MOTOR_COUNT]);

#endif
