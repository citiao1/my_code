#ifndef VEHICLE_CONTROL_H
#define VEHICLE_CONTROL_H

#include "vehicle_types.h"

void VehicleControl_InitDefaults(void);
void VehicleControl_EnableDefaultLoops(void);
void VehicleControl_Update(uint32_t now);
uint8_t VehicleControl_StartSquare(uint8_t throttle, int8_t direction, uint32_t now);
void VehicleControl_CancelSquare(SquarePhase phase);

#endif
