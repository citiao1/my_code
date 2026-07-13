/*******************************************************************************
 * @file                12-LQ_Tracking_Demo.c
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
 * @name     LQ_Tracking_Demo
 *
 * @brief    8 路模拟量灰度循迹模块使用示例
 *
 * @param    none
 * 
 * @note     本例程演示了 8 路模拟量灰度循迹模块的使用方法, 包括模块初始化、模块配置、模块控制等。
 * 
 * @note     本模块主要用于实现 8 路模拟量灰度循迹功能，通过对 8 个不同通道的模拟量进行采集和处理，
		     能够感知外部环境的灰度变化，常用于智能小车等设备的循迹应用场景。
 *************************************************************************/
void LQ_Tracking_Demo(void)
{
#if 0
    // 初始化为轮询采集模式
    LQ_Tracking_Polling_Init();

    while (1)
    {
        LQ_Tracking_Polling_GetValue();
		printf("a1:%03d,a2:%03d,a3:%03d,a4:%03d,a5:%03d,a6:%03d,a7:%03d,a8:%03d\r\n",
				LQ_Tracking_Value[0],LQ_Tracking_Value[1],LQ_Tracking_Value[2],LQ_Tracking_Value[3],
				LQ_Tracking_Value[4],LQ_Tracking_Value[5],LQ_Tracking_Value[6],LQ_Tracking_Value[7]);
    }
#else
    // 初始化为并行采集模式
    LQ_Tracking_Parallel_Init();

    while (1)
    {
        LQ_Tracking_Parallel_GetValue();
		printf("a1:%03d,a2:%03d,a3:%03d,a4:%03d,a5:%03d,a6:%03d,a7:%03d,a8:%03d\r\n",
				LQ_Tracking_Value[0],LQ_Tracking_Value[1],LQ_Tracking_Value[2],LQ_Tracking_Value[3],
				LQ_Tracking_Value[4],LQ_Tracking_Value[5],LQ_Tracking_Value[6],LQ_Tracking_Value[7]);
    }
#endif
}
