#ifndef __LCD_H_
#define __LCD_H_

#include "include.h"
#include "LQ_IPSLCD.h"
#include "string.h"
#include "stdio.h"

/**
 * @brief 显示电磁传感器数据
 * 显示归一化后的五个电感值
 * @param values 电感值数组 [L1, L2, M, R1, R2]
 */
void Lcd_Display(void);
extern uint8 selected_param;
extern float tune_step;
#endif