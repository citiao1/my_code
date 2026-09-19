/***********************************************
公司：轮趣科技（东莞）有限公司
品牌：WHEELTEC
官网：wheeltec.net
淘宝店铺：shop114407458.taobao.com 
速卖通: https://minibalance.aliexpress.com/store/4455017
版本：5.7
修改时间：2021-04-29

 
Brand: WHEELTEC
Website: wheeltec.net
Taobao shop: shop114407458.taobao.com 
Aliexpress: https://minibalance.aliexpress.com/store/4455017
Version:5.7
Update：2021-04-29

All rights reserved
***********************************************/
#include "encoder.h"
#include "stm32f10x_gpio.h"



/************************************************************
Function: Initialize TIM2 to encoder interface mode
Input   : none
Output  : none
函数功能：把TIM8初始化为编码器接口模式
入口参数：无
返回  值：无
**************************************************************************/
void Encoder_Init_TIM2(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;  
	//NVIC_InitTypeDef NVIC_InitStruct;
  TIM_ICInitTypeDef TIM_ICInitStructure;  
  GPIO_InitTypeDef GPIO_InitStructure;
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);//使能定时器2的时钟
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);//使能PB端口时钟
	
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1;	//端口配置
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; //浮空输入
  GPIO_Init(GPIOA, &GPIO_InitStructure);					      //根据设定参数初始化GPIOC
  
  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
  TIM_TimeBaseStructure.TIM_Prescaler = 0x0; // 预分频器 
  TIM_TimeBaseStructure.TIM_Period = ENCODER_TIM_PERIOD; //设定计数器自动重装值
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;//选择时钟分频：不分频
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;////TIM向上计数  
  TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
  TIM_EncoderInterfaceConfig(TIM2, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);//使用编码器模式3
  TIM_ICStructInit(&TIM_ICInitStructure);
  TIM_ICInitStructure.TIM_ICFilter = 10;	//滤波10
  TIM_ICInit(TIM2, &TIM_ICInitStructure);//根据 TIM_ICInitStruct 的参数初始化外设	TIMx
  TIM_ClearFlag(TIM2, TIM_FLAG_Update);//清除TIM的更新标志位
  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
  //Reset counter
  TIM_SetCounter(TIM2,0);
  TIM_Cmd(TIM2, ENABLE); 
	
	
}

/**************************************************************************
Function: TIMING_TIM_Init
Input   : TIM_Period,TIM_Prescaler
Output  : none
函数功能：定时中断初始化
入口参数: 预装载值和预分频器 
返回  值：无
**************************************************************************/	 	
//定时器TIM5
void TIMING_TIM_Init(u16 arr,u16 psc)
{
	//定时时间：(arr+1)*(psc+1)/CLK
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
	NVIC_InitTypeDef NVIC_InitStructure; 		
		// 设置中断来源
    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn ;	
		// 设置主优先级为 1
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	 
		// 设置抢占优先级为0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;	
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

	// 开启定时器时钟,即内部时钟CK_INT=72M
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);

	/*--------------------时基结构体初始化-------------------------*/
	TIM_TimeBaseInitStruct.TIM_Period = arr;              			//设定计数器自动重装值 
	TIM_TimeBaseInitStruct.TIM_Prescaler  = psc;          			//设定预分频器
	TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;	//TIM向上计数模式
	TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;        //设置时钟分割
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStruct);      		//初始化定时器
		// 清除计数器中断标志位
    TIM_ClearFlag(TIM3, TIM_FLAG_Update);
		// 开启计数器中断
    TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);

	TIM_Cmd(TIM3,ENABLE);                              		//使能定时器

}

/**************************************************************************
Function: Read encoder count per unit time
Input   : TIMX：Timer
Output  : none
函数功能：单位时间读取编码器计数
入口参数：TIMX：定时器
返回  值：速度值
**************************************************************************/
int Read_Encoder(u8 TIMX)
{
   int Encoder_TIM;    
   switch(TIMX)
	 {
		 case 2:  Encoder_TIM= (short)TIM2 -> CNT;  TIM2 -> CNT=0;break;	
		 default: Encoder_TIM=0;
	 }
		return Encoder_TIM;
}
/**************************************************************************
Function: TIM4 interrupt service function
Input   : none
Output  : none
函数功能：TIM4中断服务函数
入口参数：无
返回  值：无
**************************************************************************/
void TIM4_IRQHandler(void)
{ 		    		  			    
	if(TIM_GetFlagStatus(TIM4,TIM_FLAG_Update)==SET)//溢出中断
	{
	 
	} 
	TIM_ClearITPendingBit(TIM4,TIM_IT_Update); 	//清除中断标志位 	    
}
/**************************************************************************
Function: TIM2 interrupt service function
Input   : none
Output  : none
函数功能：TIM8中断服务函数
入口参数：无
返回  值：无
**************************************************************************/
void TIM2_UP_IRQHandler(void)
{ 		    		  			    
	if(TIM_GetFlagStatus(TIM2,TIM_FLAG_Update)==SET)//溢出中断
	{
	 
	} 
	TIM_ClearITPendingBit(TIM2,TIM_IT_Update); 	//清除中断标志位 	    
}

/**************************************************************************
Function: TIM2 interrupt service function
Input   : none
Output  : none
函数功能：TIM8中断服务函数
入口参数：无
返回  值：无
**************************************************************************/
void TIM3_UP_IRQHandler(void)
{ 		    		  			    
	if(TIM_GetFlagStatus(TIM3,TIM_FLAG_Update)==SET)//溢出中断
	{
	 
	} 
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update); 	//清除中断标志位 	    
}



