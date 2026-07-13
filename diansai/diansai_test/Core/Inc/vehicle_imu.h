#ifndef VEHICLE_IMU_H
#define VEHICLE_IMU_H

#include <stdint.h>

uint8_t VehicleImu_InitWithRetry(uint8_t attempts);
void VehicleImu_Update(uint32_t now);

#endif
