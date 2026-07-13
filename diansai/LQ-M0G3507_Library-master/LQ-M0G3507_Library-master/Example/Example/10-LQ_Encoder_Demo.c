/*******************************************************************************
 * @file                10-LQ_Encoder_Demo.c
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

#include "LQ_device.h"
#include "LQ_Demo.h"

/*************************************************************************
 * @name     LQ_Servo_Demo
 *
 * @brief    舵机控制例程
 *
 * @param    none
 * @return   none
 * 
 * @note     本例程演示了舵机控制的基本方法, 包括舵机初始化、舵机控制等。
 * 
 * @note     舵机输出所使用的定时器和其他配置可在 LQ_servo.h 文件中修改。
 *************************************************************************/
void LQ_Encoder_Demo(void)
{
    LQConfig_Encoder_InitTypeDef_t encoder_cfg = {
		.count = 0,
		.encoder_cnt = 0,
		.pinA = GPIO_Pin_A_7,   // 脉冲引脚
		.pinB = GPIO_Pin_A_3    // 方向引脚
	};
	LQ_Encoder_Init(5, &encoder_cfg);
	LQConfig_Encoder_InitTypeDef_t encoder_cfg1 = {
		.count = 0,
		.encoder_cnt = 0,
		.pinA = GPIO_Pin_A_8,   // 脉冲引脚
		.pinB = GPIO_Pin_B_7    // 方向引脚
	};
	LQ_Encoder_Init(5, &encoder_cfg1);  // 初始化定时器, 并设置 5ms 获取一次值

    while (1)
    {
        printf("count1:%04d\t"  , encoder_cfg.count);
		printf("count2:%04d\r\n", encoder_cfg1.count);
        delay_ms(10);
    }
}
