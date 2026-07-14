#include "LQ_servo.h"




/********************************************************************************************************************

 * @模块名称：180°舵机

 * @模块概述：
 
    · 通过配置 180°舵机的占空比，可实现对舵机转动角度的控制。
		· 舵机频率为 50 Hz
		· 0.5ms 对应计数 250
		· 1.5ms （舵机中值）约为 750
		· 2.5ms 对应计数 1250
		
		// 不同占空比时间与舵机转动角度对应关系如下：
		·t = 0.5ms ——————- 舵机会转动 0 °
		·t = 1.0ms ——————- 舵机会转动 45°
		·t = 1.5ms ——————- 舵机会转动 90°
		·t = 2.0ms ——————- 舵机会转动 135°
		·t = 2.5ms ——————- 舵机会转动 180°


 * @使用方法：
 
    无需进行初始化操作，直接在 主循环 或 定时器中断服务函数 中调用 “ Servo_Ctrl(ch, Duty); ” 函数即可。
		
		编号对应的 引脚 及 定时器通道 在函数的注意事项中有说明。
		
		示例：
				Servo_Ctrl(1, 750);  // 设置编号为 1 的舵机占空比为 750
		


 * @注意事项：
 
     1. 设置占空比时，计数范围需在 250 - 1250 之间，避免超出舵机有效控制范围。
		 2. 由于实际情况可能存在误差，使用过程中可能需根据具体情况对占空比进行微调。

 ******************************************************************************************************************/




/********************************************************************************************************************
 * 【函数名称】 Servo_Ctrl
 * 【功能概述】 该函数用于设置指定舵机的 PWM 占空比，以控制舵机的转动角度。
 * 【输入参数】
 *            - unsigned char ch：选择对应舵机的编号，范围为 1 - 5。
 *            - unsigned int Duty：舵机的 PWM 占空比，取值范围为 250 - 1250。
 * 【返 回 值】 无
 * 【使用示例】 Servo_Ctrl(1, 750);  // 设置编号为 1 的舵机占空比为 750
 * 【注意事项】
 *            - 该函数会对输入的占空比进行限幅保护，确保其在有效范围内。
 *            - 不同舵机编号对应的定时器和通道信息如下：
 *              - 舵机编号 1：使用 PWM_Servo_A0_INST 定时器，通过 GPIO_PWM_Servo_A0_C1_IDX 通道设置占空比（对应引脚 PB9）。
 *              - 舵机编号 2：使用 PWM_Servo_G0_INST 定时器，通过 GPIO_PWM_Servo_G0_C0_IDX 通道设置占空比（对应引脚 PB10）。
 *              - 舵机编号 3：使用 PWM_Servo_G0_INST 定时器，通过 GPIO_PWM_Servo_G0_C1_IDX 通道设置占空比（对应引脚 PB11）。
 *              - 舵机编号 4：使用 PWM_Servo_A0_INST 定时器，通过 GPIO_PWM_Servo_A0_C2_IDX 通道设置占空比（对应引脚 PB12）。
 *              - 舵机编号 5：使用 PWM_Servo_A0_INST 定时器，通过 GPIO_PWM_Servo_A0_C3_IDX 通道设置占空比（对应引脚 PB13）。
 ******************************************************************************************************************/
void Servo_Ctrl(unsigned char ch, unsigned int Duty)
{
	
    // 占空比限幅保护
    // 若输入的占空比大于等于 1250，则将其限制为 1250
    // 若输入的占空比小于等于 250，则将其限制为 250
    // 否则，使用输入的占空比
    Duty = Duty >= 1250 ? 1250 : (Duty <= 250 ? 250 : Duty);

	
	
    // 根据选择的舵机编号，设置对应的 PWM 占空比
    // 舵机编号 1：使用 PWM_Servo_A0_INST 定时器，通过 GPIO_PWM_Servo_A0_C1_IDX 通道设置占空比
    if (ch == 1)
        DL_TimerG_setCaptureCompareValue(PWM_Servo_A0_INST, (unsigned int)Duty, GPIO_PWM_Servo_A0_C1_IDX);  // PB9

		// 舵机编号 2：使用 PWM_Servo_G0_INST 定时器，通过 GPIO_PWM_Servo_G0_C0_IDX 通道设置占空比
    if (ch == 2)
        DL_TimerG_setCaptureCompareValue(PWM_Servo_G0_INST, (unsigned int)Duty, GPIO_PWM_Servo_G0_C0_IDX);  // PB10

    // 舵机编号 3：使用 PWM_Servo_G0_INST 定时器，通过 GPIO_PWM_Servo_G0_C1_IDX 通道设置占空比
    if (ch == 3)
        DL_TimerG_setCaptureCompareValue(PWM_Servo_G0_INST, (unsigned int)Duty, GPIO_PWM_Servo_G0_C1_IDX);  // PB11
		
    // 舵机编号 4：使用 PWM_Servo_A0_INST 定时器，通过 GPIO_PWM_Servo_A0_C2_IDX 通道设置占空比
    if (ch == 4)
        DL_TimerG_setCaptureCompareValue(PWM_Servo_A0_INST, (unsigned int)Duty, GPIO_PWM_Servo_A0_C2_IDX);  // PB12

    // 舵机编号 5：使用 PWM_Servo_A0_INST 定时器，通过 GPIO_PWM_Servo_A0_C3_IDX 通道设置占空比
    if (ch == 5)
        DL_TimerG_setCaptureCompareValue(PWM_Servo_A0_INST, (unsigned int)Duty, GPIO_PWM_Servo_A0_C3_IDX);  // PB13

}

/********************************************************************************************************************
 * 【函数名称】 LQ_Test_Servo
 * 【功能概述】 该函数用于测试舵机驱动功能。初始化 OLED 屏幕，显示舵机的占空比信息，并控制舵机转动。同时使 LED 闪烁。
 * 【输入参数】 无
 * 【返 回 值】 无
 * 【使用示例】 LQ_Test_Servo();
 * 【注意事项】 注意 Servo_Ctrl 函数使用说明，不要设置参数超出范围
 ******************************************************************************************************************/
void LQ_Test_Servo()
{
        /* 舵机编号 1 - 5 的占空比 */
        int Duty1 = 750, Duty2 = 750, Duty3 = 750, Duty4 = 750, Duty5 = 750;
    
        // 初始化 OLED
        OLED_Init();
        while (1)
        {
            /* 屏幕显示 */
            sprintf(txt, "LQ_Test_Servo");
			OLED_ShowString(0, 23, (uint8_t *)txt, 12);
			sprintf(txt, "D_1: %5d;D_2: %5d", Duty1, Duty2);
            OLED_ShowString(3, 0, (uint8_t *)txt, 8);
			sprintf(txt, "D_3: %5d;D_4: %5d", Duty3, Duty4);
			OLED_ShowString(5, 0, (uint8_t *)txt, 8);
            sprintf(txt, "D_5: %5d;", Duty5);
			OLED_ShowString(7, 0, (uint8_t *)txt, 8);
			OLED_Refresh();
            
            /* 舵机控制 */
            Servo_Ctrl(1, Duty1);
            Servo_Ctrl(2, Duty2);
            Servo_Ctrl(3, Duty3);
            Servo_Ctrl(4, Duty4);
            Servo_Ctrl(5, Duty5);
            
            /* LED 闪烁*/
            LED_TOGGLE;
            delay_ms(50);
        }
}



