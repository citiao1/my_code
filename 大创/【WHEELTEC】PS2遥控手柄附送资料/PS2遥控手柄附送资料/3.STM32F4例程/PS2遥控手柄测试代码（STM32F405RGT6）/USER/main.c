/***********************************************
公司：轮趣科技（东莞）有限公司
品牌：WHEELTEC
官网：wheeltec.net
淘宝店铺：shop114407458.taobao.com 
速卖通: https://minibalance.aliexpress.com/store/4455017
版本：V5.0
修改时间：2021-11-05

Brand: WHEELTEC
Website: wheeltec.net
Taobao shop: shop114407458.taobao.com 
Aliexpress: https://minibalance.aliexpress.com/store/4455017
Version: V5.0
Update：2021-11-05

All rights reserved
***********************************************/
#include "stm32f4xx.h"
#include "sys.h"  

int PS2_LX,PS2_LY,PS2_RX,PS2_RY,PS2_KEY;     //
int main(void)
{
	delay_init(168);                //=====主频168M
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//=====设置系统中断优先级分组2
  uart_init(9600);              //=====延时初始化
	PS2_Init();									    //=====ps2驱动端口初始化
	PS2_SetInit();		 					    //=====ps2配置初始化,配置“红绿灯模式”，并选择是否可以修改
	delay_ms(500);
  while(1)
		{
    	PS2_LX=PS2_AnologData(PSS_LX);    
			PS2_LY=PS2_AnologData(PSS_LY);
			PS2_RX=PS2_AnologData(PSS_RX);
			PS2_RY=PS2_AnologData(PSS_RY);
			PS2_KEY=PS2_DataKey();	
			printf("%d     PS2_LX:",PS2_LX);
			printf("%d     PS2_LY:",PS2_LY);
		  printf("%d     PS2_RX:",PS2_RX);
			printf("%d     PS2_RY:",PS2_RY);
			printf("%d \r\nPS2_KEY:",PS2_KEY);
			delay_ms(100);
	}
}
