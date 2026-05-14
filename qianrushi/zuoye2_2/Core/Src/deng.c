#include "deng.h"
#include "gpio.h"
void LED_Init(void){
	HAL_GPIO_WritePin(HC573_GPIO_Port, HC573_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, LD6_Pin|LD7_Pin|LD8_Pin|LD1_Pin|LD2_Pin|LD3_Pin|LD4_Pin|LD5_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, LD6_Pin|LD7_Pin|LD8_Pin|LD1_Pin|LD2_Pin|LD3_Pin|LD4_Pin|LD5_Pin, GPIO_PIN_RESET);
	HAL_Delay(100);
	HAL_GPIO_WritePin(GPIOC, LD6_Pin|LD7_Pin|LD8_Pin|LD1_Pin|LD2_Pin|LD3_Pin|LD4_Pin|LD5_Pin, GPIO_PIN_SET);
}
void LED_ON(unsigned short int Pin){
	HAL_GPIO_WritePin(HC573_GPIO_Port, HC573_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(HC573_GPIO_Port, HC573_Pin, GPIO_PIN_RESET);
}
void LED_OFF(unsigned short int Pin){
	HAL_GPIO_WritePin(HC573_GPIO_Port, HC573_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(HC573_GPIO_Port, HC573_Pin, GPIO_PIN_RESET);
}
void LED_To(unsigned short int Pin){
	HAL_GPIO_WritePin(HC573_GPIO_Port, HC573_Pin, GPIO_PIN_SET);
	HAL_GPIO_TogglePin(GPIOC, Pin);
	HAL_GPIO_WritePin(HC573_GPIO_Port, HC573_Pin, GPIO_PIN_RESET);
}

