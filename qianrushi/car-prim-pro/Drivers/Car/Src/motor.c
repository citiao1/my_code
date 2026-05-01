#include "motor.h"
#include "gpio.h"
#include "tim.h"
/*行进，参数包括左侧速度及右侧速度，速度取值+-0~1000*/
void Motor_SetSpeed(int right, int left)
{
    if(left >= 0){
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
    }else{
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
        left = -left;
    }

    if(right >= 0){
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);
    }else{
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);
        right = -right;
    }

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, left);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, right);
}

void Motor_Stop(void)
{
    Motor_SetSpeed(0,0);
}