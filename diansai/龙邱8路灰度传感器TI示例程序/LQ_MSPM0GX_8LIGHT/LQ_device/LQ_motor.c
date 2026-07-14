#include "LQ_motor.h"


/**
  * PB2：
  * PWM 引脚，频率是 12.5k，使用 TIMG6 的 C0 通道
	* PWM 占空比设置范围 0~10000


  * PB3：
	* PWM 引脚，频率是 12.5k，使用 TIMA1 的 C1 通道
	* PWM 占空比设置范围 0~10000



  * 都是边缘对齐模式，且为向上计数模式，默认低电平输出，默认占空比为 0
*/


/**
  * PA15：方向控制引脚，与 PB2 是一对儿


  * PA9：方向控制引脚，与 PB3 是一对儿
*/


/********************************************************************************************************************
 * 【函数名称】 PWM_Set
 * 【功能概述】 该函数用于设置指定定时器通道的 PWM 占空比。会对输入的占空比进行换算和限幅保护，确保占空比在有效范围内。
 * 【输入参数】
 *            - GPTIMER_Regs *INST：定时器寄存器指针，指向要操作的定时器。
 *            - int32_t Duty：要设置的占空比，范围为 0 - 10000。
 *            - DL_TIMER_CC_INDEX IDX：定时器通道索引，指定要设置的通道。
 * 【返 回 值】 无
 * 【使用示例】 PWM_Set(PWM_L_INST, 1000, GPIO_PWM_L_C0_IDX);
 * 【注意事项】 占空比范围为 0 - 10000，函数会自动对超出范围的占空比进行限幅处理。
 ******************************************************************************************************************/
void PWM_Set(GPTIMER_Regs *INST, int32_t Duty, DL_TIMER_CC_INDEX IDX)
{
	
		Duty = (int32_t)(Duty / (float)PWM_Denom);  // PWM 换算
		Duty = Duty >= PWM_Period ? PWM_Period : (Duty <= 0 ? 0 : Duty);  // 限幅保护
		
		DL_TimerG_setCaptureCompareValue(INST, (int32_t)Duty, IDX);
	
}


/********************************************************************************************************************
 * 【函数名称】 Motor_Ctrl
 * 【功能概述】 该函数用于控制 drv8701e 双路电机的转速和方向。根据输入的占空比的正负来确定电机的正反转，绝对值决定电机的转速。
 * 【输入参数】
 *            - int32_t Duty2：电机 2 的占空比，正负决定方向，绝对值决定转速。
 *            - int32_t Duty1：电机 1 的占空比，正负决定方向，绝对值决定转速。
 * 【返 回 值】 无
 * 【使用示例】 Motor_Ctrl(1000, -1000);
 * 【注意事项】 Duty 的绝对值决定电机的转速，正负决定电机的正反转。
 ******************************************************************************************************************/
void Motor_Ctrl(int32_t Duty2, int32_t Duty1)
{
		/*  Motor1 方向控制 */
		if(Duty1 >= 0)  // 正转
		{
				// Motor1 方向引脚输出高电平
				DL_GPIO_setPins(Motor_PORT, Motor_Motor_IO1_PIN);
		}
		else  // 反转
		{
				// 取占空比的绝对值
				Duty1 = - Duty1;
				// Motor1 方向引脚输出低电平
				DL_GPIO_clearPins(Motor_PORT, Motor_Motor_IO1_PIN);
		}
		
		/*  Motor2 方向控制 */
		if( Duty2 >= 0 )  // 正转
		{
				// Motor2 方向引脚输出高电平
				DL_GPIO_setPins(Motor_PORT, Motor_Motor_IO2_PIN);
		}
		else  // 反转
		{
				// 取占空比的绝对值
				Duty2 = - Duty2;
				// Motor2 方向引脚输出低电平
				DL_GPIO_clearPins(Motor_PORT, Motor_Motor_IO2_PIN);
		}
		
		// 设置 Motor1 的 PWM 占空比
		PWM_Set(PWM_Motor_INST, (int32_t)Duty1, GPIO_PWM_Motor_C0_IDX);
		// 设置 Motor2 的 PWM 占空比
		PWM_Set(PWM_Motor_INST, (int32_t)Duty2, GPIO_PWM_Motor_C1_IDX);
}


/********************************************************************************************************************
 * 【函数名称】 LQ_Test_Motor
 * 【功能概述】 该函数用于测试 drv8701e 双路电机驱动功能。初始化 OLED 屏幕，显示电机的占空比信息，并控制电机运行。同时使 LED 闪烁。
 * 【输入参数】 无
 * 【返 回 值】 无
 * 【使用示例】 LQ_Test_Motor();
 * 【注意事项】 Duty 的绝对值决定电机的转速，正负决定电机的正反转。
 ******************************************************************************************************************/
void LQ_Test_Motor()
{
		/*  电机1 和 电机2 的占空比  */
		int Duty1 = 1000, Duty2 = -1000;
		
		/*  初始化 OLED  */
		OLED_Init();
		
		while(1)
		{          
			/*  屏幕显示  */
			sprintf(txt, "LQ_Test_Motor");
			OLED_ShowString(0, 23, (uint8_t *)txt, 12);
			sprintf(txt, "Duty1: %6d", Duty1);
			OLED_ShowString(3, 0, (uint8_t *)txt, 8);
			sprintf(txt, "Duty2: %6d", Duty2);
			OLED_ShowString(5, 0, (uint8_t *)txt, 8);
			OLED_Refresh();
				
			/*  drv8701e 双路电机驱动控制函数  */
			Motor_Ctrl(Duty1, Duty2);
			
			/*  LED 闪烁  */
			LED_TOGGLE;
			delay_ms(50);	
		}
}

