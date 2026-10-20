#ifndef PS2_CONTROL_H
#define PS2_CONTROL_H

#include <stdint.h>

typedef struct
{
  uint8_t enabled;
  uint8_t speed_rpm;
  int16_t forward_rpm;
  int16_t left_rpm;
  int16_t rotate_rpm;
} Ps2ControlSnapshot;

void Ps2Control_Init(void);
void Ps2Control_Process(void);
void Ps2Control_Stop(void);
uint8_t Ps2Control_IsActive(void);
Ps2ControlSnapshot Ps2Control_GetSnapshot(void);

#endif /* PS2_CONTROL_H */
