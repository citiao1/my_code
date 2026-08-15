#ifndef YAW_RATE_CONTROL_H
#define YAW_RATE_CONTROL_H

#include <stdint.h>

typedef struct
{
  float kp;
  float ki;
  float kff;
  float integral_limit;
  float target_dps;
  float feedback_dps;
  float error_dps;
  float output_rpm;
  uint8_t enabled;
} YawRateControlSnapshot;

void YawRateControl_Init(void);
void YawRateControl_Reset(void);
float YawRateControl_Update(float target_dps, float feedback_dps,
                            float dt_seconds, float output_limit_rpm);
void YawRateControl_SetEnabled(uint8_t enabled);
YawRateControlSnapshot YawRateControl_GetSnapshot(void);

#endif
