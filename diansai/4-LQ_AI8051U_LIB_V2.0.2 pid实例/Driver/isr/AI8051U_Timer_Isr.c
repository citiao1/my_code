/*---------------------------------------------------------------------*/
/* ------------------- Web: www.STCAI.com -----------------------------*/
/*---------------------------------------------------------------------*/

#include "AI8051U_Timer.h"
#include "include.h"
#include "inductance.h"
#include "motor.h"
#include "init.h"

extern volatile bit flag_20ms_lcd;
//========================================================================
// 函数: Timer0_ISR_Handler
// 描述: Timer0中断函数.
// 参数: none.
// 返回: none.
// 版本: V1.0, 2024-11-23
//========================================================================
void Timer0_ISR_Handler (void) interrupt TMR0_VECTOR		//进中断时已经清除标志
{
	ScanInductance();
}

//========================================================================
// 函数: Timer1_ISR_Handler
// 描述: Timer1中断函数.
// 参数: none.
// 返回: none.
// 版本: V1.0, 2020-09-23
//========================================================================
void Timer1_ISR_Handler (void) interrupt TMR1_VECTOR		//进中断时已经清除标志
{
	// 使用chassis控制: 外环差比和+方向PID，内环速度环PID
		Chassis_Control();
}

//========================================================================
// 函数: Timer2_ISR_Handler
// 描述: Timer2中断函数.
// 参数: none.
// 返回: none.
// 版本: V1.0, 2020-09-23
//========================================================================
void Timer2_ISR_Handler (void) interrupt TMR2_VECTOR		//进中断时已经清除标志
{
	// TODO: 在此处添加用户代码
//	Roundabout_Detect_Task();
//	LED_Ctrl(LED0,RVS);
}

//========================================================================
// 函数: Timer3_ISR_Handler
// 描述: Timer3中断函数.
// 参数: none.
// 返回: none.
// 版本: V1.0, 2020-09-23
//========================================================================
void Timer3_ISR_Handler (void) interrupt TMR3_VECTOR		//进中断时已经清除标志
{
	// TODO: 在此处添加用户代码
//	LED_Ctrl(LED0,RVS);
}

//========================================================================
// 函数: Timer4_ISR_Handler
// 描述: Timer4中断函数.
// 参数: none.
// 返回: none.
// 版本: V1.0, 2020-09-23
//========================================================================
void Timer4_ISR_Handler (void) interrupt TMR4_VECTOR		//进中断时已经清除标志
{
	// TODO: 在此处添加用户代码
//	LED_Ctrl(LED1,RVS);
}


//========================================================================
// 函数: Timer4_ISR_Handler
// 描述: Timer4中断函数.
// 参数: none.
// 返回: none.
// 版本: V1.0, 2020-09-23
//========================================================================
void Timer11_ISR_Handler (void) interrupt TMR11_VECTOR		//进中断时已经清除标志
{
	// TODO: 在此处添加用户代码
	// ?? 软件分频计数器，用于生成 200ms 的 LCD 刷新节拍
    static uint8 lcd_timer_cnt = 0; 
    static uint8 vofa_timer_cnt = 0; // ?? 新增：VOFA 专属计数器
    // 1. 每次进中断 (每隔 3ms) 必定执行一次苯环检测
    Roundabout_Detect_Task(); 
    
    // 2. 软件计数分频
    lcd_timer_cnt++;
    if (lcd_timer_cnt >= 50) // 40次 * 3ms = 200ms
    {
        lcd_timer_cnt = 0;   // 清零计数器
        flag_20ms_lcd = 1;   // 置位 LCD 刷新标志 (你的主循环 while(1) 里会检测到并刷新屏幕)
    }
		
		vofa_timer_cnt++;
    if (vofa_timer_cnt >= 1)
    {
        vofa_timer_cnt = 0;
        flag_20ms_vofa = 1;
    }
	
//	LED_Ctrl(LED0,RVS);
//    LED_Ctrl(Beep0,RVS);
}


