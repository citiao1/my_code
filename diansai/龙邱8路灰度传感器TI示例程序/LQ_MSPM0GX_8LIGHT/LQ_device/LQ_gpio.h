#ifndef _LQ_GPIO_H
#define _LQ_GPIO_H

#include "include.h"



#define LED_ON				DL_GPIO_clearPins(LED_PORT, LED_LED0_PIN)  	   // 点亮指定的 LED 灯
#define LED_OFF				DL_GPIO_setPins(LED_PORT, LED_LED0_PIN)  			 // 熄灭指定的 LED 灯
#define LED_TOGGLE		DL_GPIO_togglePins(LED_PORT, LED_LED0_PIN)   	 // 翻转指定 LED 灯的亮灭状态


#define Buzz_ON				DL_GPIO_setPins(BUZZ_PORT, BUZZ_Buzzer_PIN)  	 // 开启蜂鸣器
#define Buzz_OFF			DL_GPIO_clearPins(BUZZ_PORT, BUZZ_Buzzer_PIN)   // 关闭蜂鸣器
#define Buzz_TOGGLE		DL_GPIO_togglePins(BUZZ_PORT, BUZZ_Buzzer_PIN)  // 翻转蜂鸣器的开关状态




uint32_t LQ_GPIO_readPins(GPIO_Regs* GPIO, uint32_t pins);

void LQ_Test_LED(void);

#endif


