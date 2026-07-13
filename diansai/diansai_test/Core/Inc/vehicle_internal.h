#ifndef VEHICLE_INTERNAL_H
#define VEHICLE_INTERNAL_H

#include "vehicle_config.h"
#include "vehicle_types.h"

extern VehicleState state;
extern SpeedPidState pid_left;
extern SpeedPidState pid_right;
extern YawPidState pid_yaw;
extern HeadingPidState pid_heading;
extern SquareTestState square_test;

extern uint32_t last_command_ms;
extern uint32_t last_heading_ms;

float Vehicle_WrapAngle(float angle);

#endif
