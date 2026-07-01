#ifndef CHASSIC_H
#define CHASSIC_H

#include "main.h"
#include "motor.h"
#include "IMU.h"
#include "ecoder.h"
#include "contor.h"


#define pi                  3.1415926536f
#define CHASSICLENGH        0.37f
#define CHASSICWHEIGH       0.37f
#define DEGREE_2_RAD        0.0174529252f
#define RAD_2_DEGREE        57.216847881f
#define CHASSISOFFSET       84.0f
#define radius              0.150f
#define RAD_PS_2_RPM        (30.0f/pi)

typedef enum
{
    CHASSIS_ZERO_FORCE,
    CHASSIS_NO_FOLLOW,
    CHASSIS_FOLLOW_GIMBLE_YAW,
    CHASSIS_ROTATE
} Chassis_Mode_e;

typedef struct
{
    float vx;
    float vy;
    float wz;
    float offset_angle;
    int chassic_speed_buff;
    Chassis_Mode_e Chassis_Mode;

} Chassic_Ctrl_Cmd;

void Chassistask();
void ChassisInit();
Chassic_Ctrl_Cmd *GetChassisCmd();


#endif 