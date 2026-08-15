#ifndef SPEED_CONTROL_H
#define SPEED_CONTROL_H

#include <stdint.h>

#include "motor.h"

typedef struct
{
  float kp;
  float ki;
  float kd;
  uint8_t output_limit;
  uint8_t integral_limit;
  uint8_t enabled;
} SpeedControlConfig;

typedef struct
{
  int16_t rpm[MOTOR_COUNT];
  int16_t target_rpm[MOTOR_COUNT];
} SpeedControlSnapshot;

void SpeedControl_Init(void);
void SpeedControl_Process(uint32_t elapsed_ms);
void SpeedControl_SetTargets(int16_t a, int16_t b, int16_t c, int16_t d);
void SpeedControl_Stop(void);
void SpeedControl_Reset(void);
void SpeedControl_SetEnabled(uint8_t enabled);
uint8_t SpeedControl_IsEnabled(void);
SpeedControlConfig SpeedControl_GetConfig(void);
SpeedControlSnapshot SpeedControl_GetSnapshot(void);

#endif
