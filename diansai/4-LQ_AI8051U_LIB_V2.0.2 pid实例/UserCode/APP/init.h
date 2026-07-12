#ifndef __INIT_H_
#define __INIT_H_

#include "config.h"

/**
 * @brief 统一初始化所有外设和模块
 * 包含: ADC, LCD, Timer, Motor, PID, Chassis等
 */
void User_Init(void);
void Wait_For_Start(void);
void Emergency_Stop_Task(void);
void Key_Tuning_Task(void);
extern volatile bit flag_20ms_lcd;
extern volatile bit flag_20ms_vofa;
#endif