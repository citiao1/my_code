#ifndef PS2_CONTROL_H
#define PS2_CONTROL_H

#include <stdint.h>

void Ps2Control_Init(void);
void Ps2Control_Process(void);
uint8_t Ps2Control_IsConnected(void);
uint8_t Ps2Control_IsArmed(void);
char Ps2Control_GetStatusChar(void);

#endif
