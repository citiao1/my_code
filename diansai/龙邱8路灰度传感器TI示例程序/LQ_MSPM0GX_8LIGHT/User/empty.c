#include "ti_msp_dl_config.h"

#include "include.h"
/**
 *   @ file main.c as empty
 *   @ 本文件即相当于main.c文件
 *   @ 注：1、<empty.syscfg> 为图形化配置信息文件使用时在当前页面选中其打开keil
 *         在工具栏tool选项中打开添加好的图形化工具进行配置，然后Ctrl+S或点击文件保存
 *         2、图形化配置生成的所有代码在<ti_msp_dl_config.c>和<ti_msp_dl_config.h>文件中
 *           配置保存后，代码文件需要编译一次后更新，如果TI System Configuration Tool和SDK的路劲不对
 *           可能无法使用图形化配置，参考教程：https://www.bilibili.com/video/BV1dBEjzTEPs/
 *           教程所需所有文件均已打包，只参考其安装配置步骤即可！！！
 *         3、若要使用串口功能，该库中串口的波特率设置为115200，具体使用步骤请看LQ_uart.c文件注释内容
 *         4、编译完成后会出现几条info信息，其原因是当前程序配置，如果在低功耗模式被唤醒的情况下，info
 *           信息中提到的几个相关内容需要重新对寄存器进行配置，才能恢复使用，建议大家对这一块进行优化，
 *           正常情况下可忽略这部分内容，不影响大家正常使用，如果是需要使用低功耗功能的各位，并且也想要
 *           去除重新配置的麻烦，可根据情况自行优化
 */

 /***************************************  8路循灰度续集模块控制说明   *********************************************
 * 【功能概述】 该函数用于控制 8 路模拟量灰度循迹模块的三个 IO 引脚的电平状态，通过设置这些引脚的电平组合，
 *             可以决定循迹模块 ADC 引脚输出哪个通道的数据。
 * 【输入参数】
 *            - unsigned char S2：代表循迹模块 S2 引脚的电平状态。
 *            - unsigned char S1：代表循迹模块 S1 引脚的电平状态。
 *            - unsigned char S0：代表循迹模块 S0 引脚的电平状态。
 * 【返 回 值】 无
 * 【使用示例】 Tracking_IO_Set(0, 0, 1);  // 将 S2 设为低电平，S1 设为低电平，S0 设为高电平，选择通道 2
 * 【注意事项】 每个参数可输入 0 或 1，其中 0 表示将对应引脚设置为低电平，1 表示将对应引脚设置为高电平。
 *             不同的电平组合对应不同的循迹模块通道选择。具体对应关系如下：
 *            - (S2, S1, S0) = (0, 0, 0) 对应通道 1
 *            - (S2, S1, S0) = (0, 0, 1) 对应通道 2
 *            - (S2, S1, S0) = (0, 1, 0) 对应通道 3
 *            - (S2, S1, S0) = (0, 1, 1) 对应通道 4
 *            - (S2, S1, S0) = (1, 0, 0) 对应通道 5
 *            - (S2, S1, S0) = (1, 0, 1) 对应通道 6
 *            - (S2, S1, S0) = (1, 1, 0) 对应通道 7
 *            - (S2, S1, S0) = (1, 1, 1) 对应通道 8
 ******************************************************************************************************************/
int main(void)
{
	SYSCFG_DL_init(); // TI 系统GPIO等初始化，由图形化配置生成，无需手动更改
	delay_ms(10);

	// 初始化OLED
	OLED_Init();
	// 8 路灰度循迹模块初始化
	Tracking_Adc_Init();	// 八路灰度循迹模块初始化  一路ADC读取+3路IO控制输出


	while (1)
	{
		// 获取 8 路模拟量灰度循迹模块各通道 ADC 数据
		Tracking_Value_Acquire();

		// 屏幕显示
		sprintf(txt, "LQ_Test_Tracking");
		OLED_ShowString(0, 18, (uint8_t *)txt, 12);

		// LQ_Tracking_Value[0] - LQ_Tracking_Value[7] 表示循迹模块从左至右各个通道的 ADC 数据
		sprintf(txt, "a1: %04d   a2: %04d", LQ_Tracking_Value[0], LQ_Tracking_Value[1]);
		OLED_ShowString(3, 0, (uint8_t *)txt, 8);
		sprintf(txt, "a3: %04d   a4: %04d", LQ_Tracking_Value[2], LQ_Tracking_Value[3]);
		OLED_ShowString(4, 0, (uint8_t *)txt, 8);
		sprintf(txt, "a5: %04d   a6: %04d", LQ_Tracking_Value[4], LQ_Tracking_Value[5]);
		OLED_ShowString(5, 0, (uint8_t *)txt, 8);
		sprintf(txt, "a7: %04d   a8: %04d", LQ_Tracking_Value[6], LQ_Tracking_Value[7]);
		OLED_ShowString(6, 0, (uint8_t *)txt, 8);

		OLED_Refresh();
	}
}
