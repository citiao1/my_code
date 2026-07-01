#ifndef CONTROL_H
#define CONTROL_H

#include "main.h"
#include "chassic.h"
#include "DR16.h"

// 控制模式枚举
typedef enum
{
    RCCONTROLMODE = 0,      // 遥控器控制模式
    MOUSECONTROLMODE = 1    // 键鼠控制模式（精简版不支持）
} Control_Mode_e;

// 全局变量声明
extern uint8_t cmd_mode;            // 当前控制模式（0: 遥控器, 1: 键鼠）
extern RC_Ctl_t *rc_data;           // 遥控器数据指针
extern Chassic_Ctrl_Cmd *chassis_cmd;  // 底盘控制命令指针

// 函数声明
void ControlInit(void);             // 控制初始化函数
void RoboCmdTask(void);             // 机器人控制任务（主控制逻辑）

// 遥控器开关状态判断宏
#define switch_is_up(s)     (s == 1)    // 开关上
#define switch_is_mid(s)    (s == 3)    // 开关中
#define switch_is_down(s)   (s == 2)    // 开关下

#endif

