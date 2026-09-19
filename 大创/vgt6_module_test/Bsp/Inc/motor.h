#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

typedef enum
{
  MOTOR_CHANNEL_A = 0,
  MOTOR_CHANNEL_B,
  MOTOR_CHANNEL_C,
  MOTOR_CHANNEL_D,
  MOTOR_CHANNEL_COUNT
} MotorChannel;

uint8_t Motor_Init(void);
void Motor_Stop(MotorChannel channel);
void Motor_StopAll(void);
uint8_t Motor_Run(MotorChannel channel, int8_t direction, uint8_t duty_percent);
uint8_t Motor_Recover(MotorChannel channel);

#endif /* MOTOR_H */
