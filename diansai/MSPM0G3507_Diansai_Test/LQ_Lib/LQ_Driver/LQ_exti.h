/*******************************************************************************
 * @file                LQ_exti.h
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
 * @update              2026年4月24日
 *******************************************************************************/
#ifndef __LQ_EXTI_H__
#define __LQ_EXTI_H__

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#include "LQ_gpio.h"
#include "LQ_common.h"

#define LQ_EXIT_GPIOA_ALL					( 32 )
#define LQ_EXIT_GPIOB_ALL					( 32 )

/**
 * @brief       外部中断端口枚举
 * 
 * @note        GROUP1_IRQHandler 为中断服务函数
 */
typedef enum {
    LQ_EXIT_PORT_A,                 /* EXIT端口A */

    LQ_EXIT_PORT_B,                 /* EXIT端口B */
} LQEnum_EXIT_Port_t;

/**
 * @brief       外部中断触发方式枚举
 */
typedef enum {
    LQ_EXIT_TRIGGER_RISING,         /* 上升沿触发 */

    LQ_EXIT_TRIGGER_FALLING,        /* 下降沿触发 */

    LQ_EXIT_TRIGGER_BOTH            /* 上升沿和下降沿触发 */
} LQEnum_EXIT_Trigger_t;

void LQ_EXIT_Init(LQEnum_GPIO_Pin_t Pin, LQEnum_GPIO_Pull_t Pull, LQEnum_EXIT_Trigger_t Trigger);
void LQ_EXIT_Enable(LQEnum_EXIT_Port_t port, LQEnum_NVIC_Priority_t priority);

#endif
