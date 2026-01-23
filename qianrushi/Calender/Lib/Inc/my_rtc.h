#ifndef RTC_MY_RTC_H
#define RTC_MY_RTC_H
#include "stm32f1xx_hal.h"
#include "rtc.h"
#include "time.h"
HAL_StatusTypeDef MY_RTC_SetTime(struct tm *time);

struct tm* MY_RTC_GetTime();
void MY_RTC_Init();
#endif //RTC_MY_RTC_H