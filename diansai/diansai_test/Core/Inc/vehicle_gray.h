#ifndef VEHICLE_GRAY_H
#define VEHICLE_GRAY_H

#include "vehicle_config.h"

#include <stdint.h>

void VehicleGray_Init(void);
uint16_t VehicleGray_ReadChannel(uint8_t channel);
void VehicleGray_ReadAll(uint16_t values[GRAY_SENSOR_CHANNELS]);

#endif
