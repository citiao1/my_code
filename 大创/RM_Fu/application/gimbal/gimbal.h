#ifndef GIMBAL_H
#define GIMBAL_H

#include "IMU.h"
#include "motor.h"
#include "contor.h"
#include "DR16.h"

#define WATCHINGSTOP 0
#define WATCHINGSTART 1

typedef enum 
{
    MOTOR_ZERO,
    MMTOR_NAL
}GMotor_Mod_e;




typedef struct 
{
    uint8_t cmd_mode;                //切换键鼠与遥控器模式判断标志位,0为键盘,1为键鼠
    uint8_t rotatemode;
    uint8_t watchingcmd;
    GMotor_Mod_e gimbal_mode;
    int16_t pitch_change_degree;
    float yaw_total_degree;
    float pitch_total_degree;

}Gimbal_Ctrl_Cmd;

Gimbal_Ctrl_Cmd *GetGimbalCmd();
void GimbalTask();
void GimbalInit();
MOTORInstance *GetYawMotor();
MOTORInstance *GetPitchMotor();


#endif // 
