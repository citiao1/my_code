#include "shanshuo.h"
#include "deng.h"
#include "gpio.h"
void shanshuo(unsigned int shanshuo_pinlv){
		LED_ON(LD5_Pin);
		LED_ON(LD6_Pin);
		LED_ON(LD7_Pin);
		LED_ON(LD8_Pin);
		HAL_Delay(shanshuo_pinlv);
		LED_OFF(LD5_Pin);
		LED_OFF(LD6_Pin);
		LED_OFF(LD7_Pin);
		LED_OFF(LD8_Pin);
}
