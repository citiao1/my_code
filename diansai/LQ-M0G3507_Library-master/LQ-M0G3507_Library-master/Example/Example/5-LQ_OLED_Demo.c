/*******************************************************************************
 * @file                5-LQ_OLED_Demo.c
 * @brief               本文件是 LQ_MSPM0GX_LIB 软件开源库文件的一部分
 * @copyright           版权所有 (C) 2025-2026 北京龙邱科技有限公司
 * @website             http://www.lqist.cn
 * @taobao              http://longqiu.taobao.com
 *
 * @description         龙邱科技 MSPM0G3507 核心板驱动库声明
 *
 * 开发环境配置:
 *   - 使用环境 : Keil5
 *   - 目标芯片 : MSPM0G3507
 *   - 外置晶振 : 16.000MHz
 *   - 系统时钟 : 80MHz
 *
 * 本文件遵循GPL-3.0开源协议发布，旨在为 MSPM0G3507 芯片嵌入式系统设计提供快速上手开发基于 MSPM0G3507 的应用程序的参考实现
 * 商业用途（包括单位使用）需提前联系作者获得授权
 *
 * GPL-3.0 许可证声明摘要:
 * 1. 允许自由使用、修改、分发本软件
 * 2. 分发修改后的版本时，必须以相同许可证发布
 * 3. 必须保留原始版权声明和许可证信息
 * 4. 不提供任何担保，使用风险自负
 * 5. 完整协议文本请参见项目根目录 LICENSE 文件
 *
 * @author              LQ_012
 * @email               chiusir@163.com
 * @version             V2.0.0
 * @update              2026年4月24日
 *******************************************************************************/
#include "LQ_Demo.h"

/*************************************************************************
 * @name     LQ_OLED_Demo
 *
 * @brief    OLED显示例程
 *
 * @param    none
 * @return   none
 * 
 * @note     本例程演示了 OLED 显示功能。
 *************************************************************************/
void LQ_OLED_Demo(void)
{
    char  txt[32]    = {0};
	int   oled_data1 = 123;
	float oled_data2 = 123.45;
	int   oled_data3 = 0xABCD;
	
	LQ_OLED_Init();  // 初始化OLED

	while (1)
	{
		// 整型：(data1)
		sprintf(txt, "data1: %4d", oled_data1);
		LQ_OLED_ShowString(1, 0, (uint8_t *)txt, 8);

		// 浮点型：(data2)
		sprintf(txt, "data2: %6.2f", oled_data2);
		LQ_OLED_ShowString(2, 0, (uint8_t *)txt, 8);

		// 字符串：
		sprintf(txt, "lqkj");
		LQ_OLED_ShowString(3, 0, (uint8_t *)txt, 8);

		// 十六进制：(data3)
		sprintf(txt, "data3: 0x%04X", oled_data3);
		LQ_OLED_ShowString(4, 0, (uint8_t *)txt, 8);
			
		// 汉字
		LQ_OLED_ShowChinese(3, 0, 0, 1);   // 北
		LQ_OLED_ShowChinese(3, 14, 1, 1);  // 京
		LQ_OLED_ShowChinese(3, 28, 2, 1);  // 龙
		LQ_OLED_ShowChinese(3, 42, 3, 1);  // 邱

		LQ_OLED_Refresh();
				
		delay_ms(50);
	}
}
