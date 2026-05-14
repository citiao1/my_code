#include "liushui.h"
#include "gpio.h"
#include "deng.h"
#include "anjian.h"
#include "shanshuo.h"
uint8_t a=0;
uint16_t count=0;
uint16_t temp=0;
uint16_t liushui_pinlv=100;
uint16_t shanshuo_pinlv=100;
void start(void){
	if(Read_KEY()==2){
		count++;
		HAL_Delay(200);
		if(count%2==0){
			a=0;
		}else{
			a=1;
		}
	}
	else if(Read_KEY()==3){
		temp=a;
		a=3;
	}
	else if(Read_KEY()==4){
		temp=a;
		a=4;
	}
	switch(a){
		case 0:
			LED_ON(LD1_Pin);
			shanshuo(shanshuo_pinlv);
			HAL_Delay(liushui_pinlv);
			LED_OFF(LD1_Pin);
			shanshuo(shanshuo_pinlv);
			LED_ON(LD2_Pin);
			shanshuo(shanshuo_pinlv);
			HAL_Delay(liushui_pinlv);
			LED_OFF(LD2_Pin);
			shanshuo(shanshuo_pinlv);
			LED_ON(LD3_Pin);
			shanshuo(shanshuo_pinlv);
			HAL_Delay(liushui_pinlv);
			LED_OFF(LD3_Pin);
			shanshuo(shanshuo_pinlv);
			LED_ON(LD4_Pin);
			shanshuo(shanshuo_pinlv);
			HAL_Delay(liushui_pinlv);
			shanshuo(shanshuo_pinlv);
			LED_OFF(LD4_Pin);
			break;
		case 1:
			LED_ON(LD4_Pin);
			shanshuo(shanshuo_pinlv);
			HAL_Delay(liushui_pinlv);
			LED_OFF(LD4_Pin);
			shanshuo(shanshuo_pinlv);
			LED_ON(LD3_Pin);
			shanshuo(shanshuo_pinlv);
			HAL_Delay(liushui_pinlv);
			LED_OFF(LD3_Pin);
			shanshuo(shanshuo_pinlv);
			LED_ON(LD2_Pin);
			shanshuo(shanshuo_pinlv);
			HAL_Delay(liushui_pinlv);
			LED_OFF(LD2_Pin);
			shanshuo(shanshuo_pinlv);
			LED_ON(LD1_Pin);
			shanshuo(shanshuo_pinlv);
			HAL_Delay(liushui_pinlv);
			LED_OFF(LD1_Pin);
			shanshuo(shanshuo_pinlv);
			break;
		case 3:
			liushui_pinlv+=100;
			a=temp;
			break;
		case 4:
			shanshuo_pinlv+=100;
			a=temp;
			break;
	}
}
void stop(void){
	LED_OFF(LD1_Pin);
	LED_OFF(LD2_Pin);
	LED_OFF(LD3_Pin);
	LED_OFF(LD4_Pin);
	LED_OFF(LD5_Pin);
	LED_OFF(LD6_Pin);
	LED_OFF(LD7_Pin);
	LED_OFF(LD8_Pin);
}
