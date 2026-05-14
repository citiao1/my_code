#include "servo.h"
#include "tim.h"

void Servo_SetAngle(float angle)
{
    if(angle<0) angle=0;
    if(angle>180) angle=180;

    uint32_t ccr = (angle/180.0f)*2000 + 500;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, ccr);
}