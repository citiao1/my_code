/*******************************************************************************
 * @file                LQ_enums.h
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
#ifndef __LQ_ENUMS_H__
#define __LQ_ENUMS_H__

/**
 * @brief       NVIC中断优先级配置
 * 
 * @param       
 */
typedef enum {
    NVIC_Priority_Highest = 0,

    NVIC_Priority_High,

    NVIC_Priority_Low,

    NVIC_Priority_Lowest,

    NVIC_Priority_NONE
} LQEnum_NVIC_Priority_t;


/**
 * @brief       DMA触发源
 * 
 * @note        外部触发源
 */
typedef enum {
    DMA_Trigger_SOFTWARE,
    DMA_Trigger_GENERIC_SUB0,
    DMA_Trigger_GENERIC_SUB1,
    DMA_Trigger_AES_AES_0,
    DMA_Trigger_AES_AES_1,
    DMA_Trigger_AES_AES_2,
    DMA_Trigger_DAC0_EVT_BD_1,
    DMA_Trigger_I2C0_TX,
    DMA_Trigger_I2C0_RX,
    DMA_Trigger_I2C1_TX,
    DMA_Trigger_I2C1_RX,
    DMA_Trigger_SPI0_RX,
    DMA_Trigger_SPI0_TX,
    DMA_Trigger_SPI1_RX,
    DMA_Trigger_SPI1_TX,
    DMA_Trigger_UART3_RX,
    DMA_Trigger_UART3_TX,
    DMA_Trigger_UART0_RX,
    DMA_Trigger_UART0_TX,
    DMA_Trigger_UART1_RX,
    DMA_Trigger_UART1_TX,
    DMA_Trigger_UART2_RX,
    DMA_Trigger_UART2_TX,
    DMA_Trigger_ADC0_EVT_GEN_BD,
    DMA_Trigger_ADC1_EVT_GEN_BD,
} LQEnum_DMA_ExternalTrigger_t;

/**
 * @brief       DMA触发源
 * 
 * @note        内部触发源
 */
typedef enum {
    DMA_Trigger_CH_0,
    DMA_Trigger_CH_1,
    DMA_Trigger_CH_2,
    DMA_Trigger_CH_3,
    DMA_Trigger_CH_4,
    DMA_Trigger_CH_5,
    DMA_Trigger_CH_6,
} LQEnum_DMA_InternalTrigger_t;

#endif
