/***********************************************
公司：轮趣科技（东莞）有限公司
品牌：WHEELTEC
官网：wheeltec.net
淘宝店铺：shop114407458.taobao.com 
速卖通: https://minibalance.aliexpress.com/store/4455017
版本：5.7
修改时间：2025-06-05

 
Brand: WHEELTEC
Website: wheeltec.net
Taobao shop: shop114407458.taobao.com 
Aliexpress: https://minibalance.aliexpress.com/store/4455017
Version:5.7
Update：2025-06-05

All rights reserved
***********************************************/
#include "stm32f10x.h"
#include "sys.h"                     
u8 delay_50,delay_flag; //延时相关变量
extern int Target_Encoder,Current_Encoder;
int main(void)
{ 
    MY_NVIC_PriorityGroupConfig(2);	//设置中断分组
	delay_init();	    	        //延时函数初始化	
	JTAG_Set(JTAG_SWD_DISABLE);     //关闭JTAG接口
	uart1_init(115200);             //串口1初始化
	JTAG_Set(SWD_ENABLE);           //打开SWD接口 可以利用主板的SWD接口调试
	MiniBalance_PWM_Init(7199,0);   //初始化PWM 10KHZ与电机硬件接口，用于驱动电机
	Encoder_Init_TIM2();
	Motor_Init();                   //电机使能引脚和方向引脚的初始化
    TIMING_TIM_Init(7199,99);       //定时器3初始化，10ms中断
	while(1)
	{		
		printf("Target=%d Current=%d\n\r",Target_Encoder,Current_Encoder);
	}
}

