#ifndef HEADING_CONTROL_H
#define HEADING_CONTROL_H

#include <stdint.h>

typedef struct
{
  float kp;
  float kd;
  float max_rate_dps;
  float target_deg;
  float feedback_deg;
  float error_deg;
  float output_dps;
  uint8_t enabled;
  uint8_t holding;
} HeadingControlSnapshot;

void HeadingControl_Init(void);
void HeadingControl_Reset(void);
void HeadingControl_Track(float current_heading_deg);
uint8_t HeadingControl_StepTarget(float current_heading_deg, float delta_deg);
uint8_t HeadingControl_HoldTarget(float current_heading_deg, float target_heading_deg);
float HeadingControl_Update(float current_heading_deg, float current_rate_dps,
                            float output_limit_dps);
void HeadingControl_SetEnabled(uint8_t enabled);
HeadingControlSnapshot HeadingControl_GetSnapshot(void);

#endif
