#include "delay.h"
static u8 fac_us=0;
static u16 fac_ms=0;
void delay_init(){
	SysTick->CTRL&=~(1<<2);
	fac_us=9;
	fac_ms=(u16)fac_us*1000;
}
void delay_us(u32 nus){
	u32 temp;
	SysTick->LOAD=nus*fac_us;
	SysTick->VAL=0x00;
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk;
	do{
		temp=SysTick->CTRL;
	}while((temp&0x01)&&!(temp&(1<<16)));
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;
	SysTick->VAL=0x00;
}
void delay_ms(u16 nms){
	u32 temp;
	SysTick->LOAD=nms*fac_ms;
	SysTick->VAL=0X00;
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk;
	do{
		temp=SysTick->CTRL;
	}while((temp&0x01)&&!(temp&(1<<16)));
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;
	SysTick->VAL=0x00;
}
