#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

typedef enum
{
  MOTOR_A = 0,
  MOTOR_B,
  MOTOR_C,
  MOTOR_D,
  MOTOR_COUNT
} MotorId;

uint8_t Motor_Init(void);
uint8_t Motor_IsEnabled(void);
void Motor_Set(MotorId id, int16_t percent);
void Motor_SetAll(int16_t a, int16_t b, int16_t c, int16_t d);
void Motor_StopAll(void);
int16_t Motor_GetOutput(MotorId id);
void Motor_GetAll(int16_t output[MOTOR_COUNT]);

#endif
