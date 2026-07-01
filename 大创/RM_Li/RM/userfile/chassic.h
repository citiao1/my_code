#ifndef _chassic__H__
#define _chassic__H__
#include "main.h"
#include "conter.h"
#include "imu.h"
#include "mycan.h"
#include "pid.h"

#define DEGREE_2_RAD        0.0174529252f
#define pi                  3.1415926536f
#define radius              0.150f
#define RAD_PS_2_RPM        (30.0f/pi)
#define ANGLEOFFSET         35.0f //(实际在20到40度) 

typedef enum
{
	CHASSIS_ZERO_FORCE,
	CHASSIS_NO_FOLLOW,
	CHASSIS_FOLLOW_GIMBLE_YAW,
	CHASSIS_ROTATE
}ChassicMode;

typedef struct
{
	float vx;
	float vy;
	float vz;//绕Z垂直轴角速度
	float offset_angle;
	ChassicMode Mode;

}ChassicCmd;

ChassicCmd *GetChassic(void);
void ChassicControl(void);

#endif
