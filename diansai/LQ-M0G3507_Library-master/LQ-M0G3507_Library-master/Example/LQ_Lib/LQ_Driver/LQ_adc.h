/*******************************************************************************
 * @file                LQ_adc.h
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
#ifndef __LQ_ADC_H__
#define __LQ_ADC_H__

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#include "LQ_common.h"

/*!
 * @brief       ADC端口号
 * 
 */
typedef enum {
	
    ADC_Port_0,
    
	ADC_Port_1,

} LQEnum_ADC_t;

/*!
 * @brief       ADC通道号
 * 
 */
typedef enum {
   
	/* ADC0的各通道 */               /* ADC1的各通道 */
    ADC0_Channel_0_Pin_A_27,        ADC1_Channel_0_Pin_A_15,
    ADC0_Channel_1_Pin_A_26,        ADC1_Channel_1_Pin_A_16,
    ADC0_Channel_2_Pin_A_25,        ADC1_Channel_2_Pin_A_17,
    ADC0_Channel_3_Pin_A_24,        ADC1_Channel_3_Pin_A_18,
    ADC0_Channel_4_Pin_B_25,        ADC1_Channel_4_Pin_B_17,
    ADC0_Channel_5_Pin_B_24,        ADC1_Channel_5_Pin_B_18,
    ADC0_Channel_6_Pin_B_20,        ADC1_Channel_6_Pin_B_19,
    ADC0_Channel_7_Pin_A_22,        ADC1_Channel_7_Pin_A_21,
    ADC0_Channel_8_Pin_A_21,        ADC1_Channel_8_Pin_A_22,
    ADCx_Channel_Max,
} LQEnum_ADC_Channel_t;

/*!
 * @brief       ADC初始化结构体
 * 
 */
typedef struct {
    
	DL_ADC12_CLOCK 				clockSel;    	/* 时钟选择 */

    DL_ADC12_CLOCK_FREQ_RANGE 	freqRange;    	/* 时钟频率范围 */

    DL_ADC12_CLOCK_DIVIDE 		divideRatio; 	/* 时钟分频比 */

    uint16_t 					adcclks;     	/* ADC采集时钟周期 */

    DL_ADC12_SAMP_CONV_RES 		resolution;  	/* 转换分辨率 */
	
} LQConfig_ADC_InitTypeDef_t;

/*! @brief      ADC 初始化 */
void LQ_ADC_Init(LQEnum_ADC_t port, LQConfig_ADC_InitTypeDef_t *ADC_Init);

/*! @brief      ADC 通道数据读取 */
uint16_t LQ_ADC_GetValue(LQEnum_ADC_Channel_t Channel);

/*! @brief      ADC 使能转换 */
void LQ_ADC_EnableConversions(LQEnum_ADC_t ADC);
/*! @brief      ADC 失能转换 */
void LQ_ADC_DisableConversions(LQEnum_ADC_t ADC);

/*! @brief      ADC 启动转换 */
void LQ_ADC_StartConversions(LQEnum_ADC_t ADC);
/*! @brief      ADC 停止转换 */
void LQ_ADC_EndConversions(LQEnum_ADC_t ADC);

#endif
