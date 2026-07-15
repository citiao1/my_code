/*******************************************************************************
 * @file                LQ_tracking.h
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
#ifndef __LQ_TRACKING_H__
#define __LQ_TRACKING_H__

#include "include.h"

/****************************************************************************************************
 * @brief   宏定义
 ****************************************************************************************************/

#define Tracking_ADC_Port           ADC_Port_0                              /* 使用 ADC 通道 0 */

// ------------------------------ 单 ADC 轮询采集模式 ------------------------------

#define Tracking_ADC_CH             ADC0_Channel_0_Pin_A_27                 /* ADC 采集通道 */

#define Tracking_ADC_IT             ADC_Trigger_Channel_0_Result            /* ADC 中断触发掩码(此处为 ADC0 的通道 0 ) */

#define Tracking_S0_PIN             GPIO_Pin_A_26                           /* ADC 采集通道切换引脚 */
#define Tracking_S1_PIN             GPIO_Pin_A_25
#define Tracking_S2_PIN             GPIO_Pin_A_24

#define Tracking_S0_LOW             LQ_GPIO_WritePin(Tracking_S0_PIN, 0)    /* 控制引脚 */
#define Tracking_S1_LOW             LQ_GPIO_WritePin(Tracking_S1_PIN, 0)
#define Tracking_S2_LOW             LQ_GPIO_WritePin(Tracking_S2_PIN, 0)

#define Tracking_S0_HIGH            LQ_GPIO_WritePin(Tracking_S0_PIN, 1)
#define Tracking_S1_HIGH            LQ_GPIO_WritePin(Tracking_S1_PIN, 1)
#define Tracking_S2_HIGH            LQ_GPIO_WritePin(Tracking_S2_PIN, 1)

// ---------------------------------- 并行采集模式 ---------------------------------

#define Tracking_ADC_Start_CH       ADC0_Channel_0_Pin_A_27                 /* ADC 采集起始通道 */
#define Tracking_ADC_End_CH         ADC0_Channel_7_Pin_A_22                 /* ADC 采集结束通道 */

extern uint16_t LQ_Tracking_Value[8];

/****************************************************************************************************
 * @brief   函数定义
 ****************************************************************************************************/

// ------------------------------ 单 ADC 轮询采集模式 ------------------------------

void LQ_Tracking_Polling_Init(void);        /* 初始化为轮询采集模式(也就是 1 个 ADC 轮询采集) */

void LQ_Tracking_Polling_GetValue(void);    /* 轮询读取8路循迹传感器值 */

// ---------------------------------- 并行采集模式 ---------------------------------

void LQ_Tracking_Parallel_Init(void);       /* 初始化为并行采集模式(也就是 8 个 ADC 一起采集) */

void LQ_Tracking_Parallel_GetValue(void);   /* 并行读取8路循迹传感器值 */

#endif
