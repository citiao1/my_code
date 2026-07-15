/*******************************************************************************
 * @file                LQ_spi.h
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
#ifndef __LQ_SPI_H__
#define __LQ_SPI_H__

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#include "LQ_common.h"

/**
 * @brief       SPI通道枚举
 * 
 * @note        SPI输入时钟均为80Mhz
 */
typedef enum {
	
    LQ_SPI0,        /*!< SPI0 */

    LQ_SPI1,        /*!< SPI1 */

} LQEnum_SPI_t;

/**
 * @brief       通道所支持的引脚枚举
 * 
 * @note        选择通道后, 引脚只能选择通道所对应的引脚
 * 
 *              SPI0_MISO_Pin_A_4
 *              如果使用外部低速晶振, 则不能使用这个引脚
 *              SPI0_MOSI_Pin_A_5 | SPI0_SCLK_Pin_A_6
 *              如果使用外部高速晶振, 则不能使用这两个引脚
 */
typedef enum {

    /*! 不初始化 */
    SPI_NO_PIN,

    /*! SPI0 SCLK */            /*! SPI0 MOSI */            /*! SPI0 MISO */            /*! SPI0 CS */
    SPI0_SCLK_Pin_A_6 ,         SPI0_MOSI_Pin_A_5 ,         SPI0_MISO_Pin_A_4 ,        SPI0_CS0_Pin_A_2 ,
    SPI0_SCLK_Pin_A_11,         SPI0_MOSI_Pin_A_9 ,         SPI0_MISO_Pin_A_10,        SPI0_CS0_Pin_A_8 ,
    SPI0_SCLK_Pin_A_12,         SPI0_MOSI_Pin_A_14,         SPI0_MISO_Pin_A_13,        SPI0_CS0_Pin_B_25,
    SPI0_SCLK_Pin_B_18,         SPI0_MOSI_Pin_B_17,         SPI0_MISO_Pin_B_19,

    /*! SPI1 SCLK */            /*! SPI1 MOSI */            /*! SPI1 MISO */            /*! SPI1 CS */
    SPI1_SCLK_Pin_B_9 ,         SPI1_MOSI_Pin_B_8 ,         SPI1_MISO_Pin_B_7 ,        SPI1_CS0_Pin_B_6 ,
    SPI1_SCLK_Pin_B_16,         SPI1_MOSI_Pin_B_15,         SPI1_MISO_Pin_B_14,        SPI1_CS0_Pin_B_20,
    SPI1_SCLK_Pin_A_17,         SPI1_MOSI_Pin_A_18,         SPI1_MISO_Pin_A_16,        SPI1_CS0_Pin_A_26,
    SPI1_SCLK_Pin_B_23,         SPI1_MOSI_Pin_B_22,         SPI1_MISO_Pin_B_21,

} LQEnum_SPI_Pin_t;

/**
 * @brief       SPI初始化配置结构体
 */
typedef struct {
	
    DL_SPI_CLOCK_DIVIDE_RATIO	DivideRatio;		/* 系统时钟分割器 */

    uint32_t 					Prescaler;			/* SPI时钟分频器 值在0 - 1023之间 SPI速率 =  输入时钟 / ((1 + Prescaler) * 2) */

    LQEnum_SPI_Pin_t			SCLK;              	/* 时钟引脚 不需要传入SPI_NO_PIN (你怎么连时钟都不要了) */
    
    LQEnum_SPI_Pin_t			MOSI;              	/* 主发从收引脚 不需要传入SPI_NO_PIN */
    
    LQEnum_SPI_Pin_t			MISO;             	/* 主收从发引脚 不需要传入SPI_NO_PIN */  
    
    LQEnum_SPI_Pin_t			CS;                	/* 硬件片选引脚 不需要传入SPI_NO_PIN */  

    DL_SPI_MODE 				mode;              	/* 主从模式选择 */

    DL_SPI_DATA_SIZE 			dataSize;         	/* 数据传输大小 */

    DL_SPI_BIT_ORDER 			bitOrder;          	/* MSB/LSB 选择 */

    DL_SPI_CHIP_SELECT  		chipSelectPin; 		/* 硬件片选选择 主模式也可以不选*/

    DL_SPI_FRAME_FORMAT 		frameFormat;		/* 帧格式选择 使用硬件片选择 MOTO4 */

} LQConfig_SPI_InitTypeDef_t;

/**
 * @brief       SPI寄存器组
 */
extern SPI_Regs * const LQ_SPI_Regs[2];

void LQ_SPI_Init(LQEnum_SPI_t SPI, LQConfig_SPI_InitTypeDef_t *SPI_Init);	/*! @brief	SPI 初始化 */

void    LQ_SPI_SendByte(LQEnum_SPI_t SPI, uint8_t byte);    /*! @brief	SPI 发送 1 字节 */
uint8_t LQ_SPI_RecvByte(LQEnum_SPI_t SPI);			        /*! @brief	SPI 接收 1 字节 */

void LQ_SPI_EnableDMA_Recv(LQEnum_SPI_t SPI);			    /*! @brief	SPI 使能 DMA 接收触发器 */
void LQ_SPI_EnableDMA_RecvTimeout(LQEnum_SPI_t SPI);	    /*! @brief	SPI 使能 DMA 接收超时触发器 */
void LQ_SPI_EnableDMA_Send(LQEnum_SPI_t SPI);		        /*! @brief	SPI 使能 DMA 发送触发器 */

uint32_t LQ_SPI_GetRXRegister(LQEnum_SPI_t SPI);		    /*! @brief	SPI 获取接收寄存器地址 */
uint32_t LQ_SPI_GetTXRegister(LQEnum_SPI_t SPI);		    /*! @brief	SPI 获取发送寄存器地址 */

#endif
