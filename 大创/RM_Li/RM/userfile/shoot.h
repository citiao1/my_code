#ifndef _shoot__H__
#define _shoot__H__
#include "main.h"
#include "mycan.h"
#include "pid.h"
#define BULLETSPEED    -15000
#define FRICTIONLEFTSPEED   -5000
#define FRICTIONRIGHTSPEED   5000
typedef enum
{
    MOVENONE = 2,
    MOVEHEAD = 3,
    MOVEALL = 1,
    MOVEMOUSE = 4,
    MOVERESERVE = 5
}ShootMode;

typedef struct
{
	ShootMode shoot_mood;
}ShootCmd;

void ShootControl();
ShootCmd *GetShoot(void);
#endif
