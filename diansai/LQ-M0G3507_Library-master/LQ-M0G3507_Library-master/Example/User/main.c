/*******************************************************************************
 * @file                main.c
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
 * @author				wuwu	(接口层编写)
 * @author              LQ_012	(优化与应用层编写)
 * @email               chiusir@163.com
 * @version             V2.0.0
 * @update              2026年7月02日
 *******************************************************************************/

#include "LQ_Demo.h"  // 测试函数头文件
#include "include.h"

int main(void)
{
    // 时钟及系统初始化,,若使用非龙邱核心板，注意自行修改时钟初始化参数和延时（滴答定时器）参数
	LQ_System_Init();
	delay_ms(10);
    
//	=================================== LQ_decive模块示例演示函数，内含死循环 ========================================

	// LQ_GPIO_Output_Demo();			// 测试 GPIO 输出
	// LQ_GPIO_Input_Demo();			// 测试 GPIO 输入
	// LQ_EXTI_Demo();					// 测试 GPIO 外部中断
	// LQ_UART_Rx_IT_Demo();			// 测试 UART 接收中断
	// LQ_UART_Tx_Demo();				// 测试 UART 发送
	// LQ_UART_Tx_DMA_Demo();			// 测试 UART DMA 发送
	// LQ_OLED_Demo();					// 测试 OLED 显示
	// LQ_PWM_Demo();					// 测试 PWM 输出
	// LQ_TIM_IT_Demo();				// 测试定时器中断
	// LQ_Motor_Demo();					// 测试电机输出
	// LQ_Servo_Demo();					// 测试舵机输出
	// LQ_Encoder_Demo();				// 测试编码器输出
	// LQ_LSM6DSR_Demo();				// 测试 LSM6DSR 六轴传感器输出
	// LQ_Tracking_Demo();				// 测试 8 路模拟量灰度循迹模块
	// LQ_MPU6050_Demo();				// 测试 MPU6050 六轴传感器输出
	// LQ_CCD_Demo();					// 测试 CCD 图像获取与处理
    // LQ_1306Motor_Demo();             // 测试 1306自闭环电机，PWM控制，串口可以订阅速度信息，接线参考电机使用手册

//	=========================== 若以上有测试函数未被注释，则程序不会运行到本行以下位置 ================================
	
	while(1)
	{
		delay_ms(50);
	}
}
