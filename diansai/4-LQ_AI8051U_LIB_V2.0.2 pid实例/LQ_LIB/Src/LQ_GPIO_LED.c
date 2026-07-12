/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
 【平    台】北京龙邱智能科技STC32位核心板
 【编    写】龙邱科技
 【E-mail  】chiusir@163.com
 【软件版本】V1.0 版权所有，单位使用请先联系授权
 【相关信息参考下列地址】
 【网    站】http://www.lqist.cn
 【淘宝店铺】http://longqiu.taobao.com
 --------------------------------------------------------------------------------
 【  IDE  】 keil C251 V5.60
 【Target 】 STC32G/STC8051U/AI8051U 32位模式
 【SYS CLK】 40 MHz使用内部晶振
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/

#include "include.h"
 #include "LQ_GPIO_LED.h"
/*************************************************************************
 * @code    void GPIO_LED_Init(void);
 * @brief   用户调用 LED初始化程序. LED0 -->在LQ_BEEP_LED.h中宏定义
 * @return  none.
 * @date    2025-10-22.
 *************************************************************************/
void GPIO_LED_Init(void)
{
	gpio_init_pin(LED_0P, GPIO_Mode_Out_PP); // LED0 推挽输出
	gpio_init_pin(LED_1P, GPIO_Mode_Out_PP); // LED1
	gpio_init_pin(LED_2P, GPIO_Mode_Out_PP); // LED2

	gpio_write_pin(LED_0P, HIGH); // 初始化关闭LED0 拉高
	gpio_write_pin(LED_1P, HIGH); // 关闭LED1
	gpio_write_pin(LED_2P, HIGH); // 关闭LED2
}


/*************************************************************************
 * @code    void LED_Ctrl(LED_n ledp, LED_s sta)
 * @brief   用户调用 LED状态控制程序.
 * @param   ledp ：LED编号， LED0/LED1/LED2/LED_ALL
 * @param   sta  ：LED状态， ON/ OFF/ RVS
 * @return  none.
 * @date    2025-10-22.
 * @note    LED管脚在 头文件中宏定义，这里直接使用传统51的位操作效率更高
 *************************************************************************/
void LED_Ctrl(LED_n ledp, LED_s sta)
{
	switch (ledp)
	{
	case LED0:
		if (sta == ON) 			LED_PIN0 = LOW;
		else if (sta == OFF) 	LED_PIN0 = HIGH;
		else if (sta == RVS)	LED_PIN0 = !LED_PIN0;
		break;
    case LED1:
		if (sta == ON)			LED_PIN1 = LOW;
		else if (sta == OFF)	LED_PIN1 = HIGH;
		else if (sta == RVS)	LED_PIN1 = !LED_PIN1;
		break;
	case LED2:
		if (sta == ON)			LED_PIN2 = LOW;
		else if (sta == OFF)	LED_PIN2 = HIGH;
		else if (sta == RVS)	LED_PIN2 = !LED_PIN2;
		break;
	case LED_ALL:
		if (sta == ON)
		{
			LED_PIN0 = LOW;   LED_PIN1 = LOW; 	LED_PIN2 = LOW;
		}
		else if (sta == OFF)
		{
			LED_PIN0 = HIGH;   LED_PIN1 = HIGH; 	LED_PIN2 = HIGH;
		}
		else if (sta == RVS)
		{
			LED_PIN0 = !LED_PIN0; LED_PIN1 = !LED_PIN1; LED_PIN2 = !LED_PIN2;
		}
		break;
	default:;
		break;
	}
}


