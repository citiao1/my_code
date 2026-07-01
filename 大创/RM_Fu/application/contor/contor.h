#ifndef CONTOR_H
#define CONTOR_H

#include "chassic.h"
#include "DR16.h"
#include "gimbal.h"
#include "shoot.h"
#include "FreeRTOS.h"
#include "pid.h"
#include "watching.h"



// 遥控器三档开关值（与 DR16 解码一致）
#define RC_SW_UP    ((uint16_t)1)
#define RC_SW_MID   ((uint16_t)3)  
#define RC_SW_DOWN  ((uint16_t)2)

#define switch_is_down(s)   (s == RC_SW_DOWN)
#define switch_is_mid(s)    (s == RC_SW_MID)
#define switch_is_up(s)     (s == RC_SW_UP)

// 小陀螺模式开关
#define ROTATESTART         1
#define ROTATESTOP         0

// 控制模式：0 遥控器，1 键鼠
#define RCCONTROLMODE       0
#define MOUSECONTROLMODE    1







typedef struct 
{
    /* data */
}Shoot_Cmd_s;

typedef struct 
{
    /* data */
}Gimble_Cmd_s;

// 键值按下去抖/计数结构体（用于双击/短按判断）
typedef struct
{
    uint64_t tIck;
    uint16_t count;
    uint8_t state;
    uint8_t mode;
}Jkey;


// 初始化整车控制：
// - 获取各模块命令结构体指针
// - 调用底盘/云台/射击/视觉初始化
void ControlInit();

// 主控制任务：
// - 处理模式切换（R 键）
// - 根据模式分发到遥控器/键鼠控制函数
void RoboCmdTask();

#endif 

























