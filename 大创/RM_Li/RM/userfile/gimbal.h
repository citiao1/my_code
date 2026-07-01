#ifndef _gimbal__H__
#define _gimbal__H__
#include "main.h"
#include "mycan.h"
#include "IMU.h"
#include "PID.h"

#define PITCHMAXDEGREE   35.0f
#define PITCHMINDEGREE   90.0f
#define PITCHMIDDLEDEGREE 50.0f
typedef struct
{
	float Yawdegree;
	float Pitchdegree;
	float PitchChangedegree;

}GimbalCmd;

GimbalCmd *GetGimbal(void);
void GimbalControl(void);


#endif
