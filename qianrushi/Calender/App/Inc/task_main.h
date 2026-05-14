//
// Created by 123 on 2025/12/4.
//

#ifndef CALENDER_TASK_MAIN_H
#define CALENDER_TASK_MAIN_H
#include "my_rtc.h"
#include "oled.h"
#include "usart.h"
#include "stdio.h"
#include "string.h"
#include "my_knob.h"
#include "gpio.h"
void MainTask();
void MainTaskInit();
void showTime(struct tm* time);
void key_pro();
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void blink();
void Buzzer();
#endif //CALENDER_TASK_MAIN_H