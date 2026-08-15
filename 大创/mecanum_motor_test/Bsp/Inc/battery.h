#ifndef BATTERY_H
#define BATTERY_H

#include <stdint.h>

uint8_t Battery_Init(void);
void Battery_Process(void);
uint8_t Battery_IsReady(void);
uint16_t Battery_GetMillivolts(void);
uint16_t Battery_GetRaw(void);

#endif
