#ifndef VEHICLE_DISPLAY_H
#define VEHICLE_DISPLAY_H

#include <stdint.h>

void VehicleDisplay_Init(void);
void VehicleDisplay_WriteLine(uint8_t page, const char *text);
void VehicleDisplay_Update(void);

#endif
