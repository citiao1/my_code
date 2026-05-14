#include "anjian.h"
#include "gpio.h"
#include "deng.h"
unsigned char Read_KEY(void){
	unsigned char value=0;
	if(HAL_GPIO_ReadPin(B1_GPIO_Port,B1_Pin)==GPIO_PIN_RESET){
			HAL_Delay(20);
			if(HAL_GPIO_ReadPin(B1_GPIO_Port,B1_Pin)==GPIO_PIN_RESET){
				value=1;
				while(HAL_GPIO_ReadPin(B1_GPIO_Port,B1_Pin)==GPIO_PIN_RESET);
			}
		}
	if(HAL_GPIO_ReadPin(B2_GPIO_Port,B2_Pin)==GPIO_PIN_RESET){
			HAL_Delay(20);
			if(HAL_GPIO_ReadPin(B2_GPIO_Port,B2_Pin)==GPIO_PIN_RESET){
				value=2;
				//while(HAL_GPIO_ReadPin(B2_GPIO_Port,B2_Pin)==GPIO_PIN_RESET);
			}
		}
	if(HAL_GPIO_ReadPin(B3_GPIO_Port,B3_Pin)==GPIO_PIN_RESET){
			HAL_Delay(20);
			if(HAL_GPIO_ReadPin(B3_GPIO_Port,B3_Pin)==GPIO_PIN_RESET){
				value=3;
				//while(HAL_GPIO_ReadPin(B3_GPIO_Port,B3_Pin)==GPIO_PIN_RESET);
			}
		}
	if(HAL_GPIO_ReadPin(B4_GPIO_Port,B4_Pin)==GPIO_PIN_RESET){
			HAL_Delay(20);
			if(HAL_GPIO_ReadPin(B4_GPIO_Port,B4_Pin)==GPIO_PIN_RESET){
				value=4;
				//while(HAL_GPIO_ReadPin(B4_GPIO_Port,B4_Pin)==GPIO_PIN_RESET);
			}
		}
	return value;
}
