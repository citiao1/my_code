#ifndef VEHICLE_BUZZER_H
#define VEHICLE_BUZZER_H

#include <stdint.h>

void VehicleBuzzer_Init(void);
void VehicleBuzzer_Set(uint8_t enabled);
void VehicleBuzzer_Beep(uint32_t duration_ms);
void VehicleBuzzer_Update(uint32_t now);

#endif
