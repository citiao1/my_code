/*******************************************************************************
 * @file                LQ_soft_spi.h
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
#ifndef __LQ_SOFT_SPI_H__
#define __LQ_SOFT_SPI_H__

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#include "LQ_common.h"
#include "LQ_gpio.h"

#define SOFT_SPI_MAX_TRANS_LEN				( 64 )		/*! @brief	软件 SPI 一次性最大传输长度 */

typedef enum
{
	
	SOFT_SPI_MODE_0 = 0x00,
	
	SOFT_SPI_MODE_1,
	
	SOFT_SPI_MODE_2,
	
	SOFT_SPI_MODE_3,
	
} LQEnum_SoftSPI_Mode_t;

/**
 * @brief       软件I2C配置结构体
 * 
 * @param       sck             SCK引脚
 * @param       miso         	MISO引脚
 * @param		mosi			MOSI引脚
 * @param		cs				CS引脚
 */
typedef struct {
	
    LQEnum_GPIO_Pin_t		sck;  	/* SCK引脚 */
	
	LQEnum_GPIO_Pin_t		miso;	/* MISO引脚 */
	
	LQEnum_GPIO_Pin_t		mosi;	/* MOSI引脚 */

    LQEnum_GPIO_Pin_t		cs;    	/* CS引脚 */
	
	LQEnum_SoftSPI_Mode_t	mode;	/* SPI模式 */
	
} LQConfig_SoftSPI_InitTypeDef_t;

/****************************************************************************************************
 * @brief   函数定义
 ****************************************************************************************************/

void LQ_Soft_SPI_Init(LQConfig_SoftSPI_InitTypeDef_t *cfg);	/*! @brief	软件 SPI 初始化 */

uint8_t LQ_Soft_SPI_ReadByte (const LQConfig_SoftSPI_InitTypeDef_t *cfg, const uint8_t reg);								/*! @brief	读取 1 字节 */
void    LQ_Soft_SPI_Read     (const LQConfig_SoftSPI_InitTypeDef_t *cfg, const uint8_t reg, uint8_t *buf, uint16_t len);	/*! @brief	读取 n 字节 */

void    LQ_Soft_SPI_WriteByte(const LQConfig_SoftSPI_InitTypeDef_t *cfg, const uint8_t reg, uint8_t value);					/*! @brief	发送 1 字节 */
void    LQ_Soft_SPI_Write    (const LQConfig_SoftSPI_InitTypeDef_t *cfg, const uint8_t reg, uint8_t *buf, uint16_t len);	/*! @brief	发送 n 字节 */

#endif
