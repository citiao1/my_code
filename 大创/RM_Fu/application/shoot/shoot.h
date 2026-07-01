#ifndef SHOOT_H
#define SHOOT_H

#include "contor.h"
#include "motor.h"
#include "main.h"

typedef enum
{
    MOVENONE = 2,
    MOVEHEAD = 3,
    MOVEALL = 1,
    MOVEMOUSE = 4,
    MOVERESERVE = 5
}SHOOT_MODE_e;





typedef struct
{
    SHOOT_MODE_e shoot_mode;
    
}Shoot_Ctrl_cmd;

void ShootInit();
void ShootTask();
Shoot_Ctrl_cmd *GetShootCmd();

#endif // !