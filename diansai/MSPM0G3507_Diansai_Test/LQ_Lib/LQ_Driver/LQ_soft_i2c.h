/*******************************************************************************
 * @file                LQ_soft_i2c.h
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
#ifndef __LQ_SOFT_I2C_H__
#define __LQ_SOFT_I2C_H__

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#include "LQ_common.h"
#include "LQ_gpio.h"

/**
 * @brief       软件I2C配置结构体
 * 
 * @param       scl  : SCL引脚
 * @param       sda  : SDA引脚
 * @param       addr : 从设备地址
 */
typedef struct {
	
    LQEnum_GPIO_Pin_t	scl;      	/* SCL引脚 */

    LQEnum_GPIO_Pin_t	sda;     	/* SDA引脚 */   

	uint8_t				addr;		/* 从设备地址 */ 
	
} LQConfig_SoftI2C_InitTypeDef_t;

/****************************************************************************************************
 * @brief   函数定义
 ****************************************************************************************************/

void LQ_Soft_I2C_Init(LQConfig_SoftI2C_InitTypeDef_t *cfg);	/*! @brief	软件 I2C 初始化 */

uint8_t LQ_Soft_I2C_RecvByte  (const LQConfig_SoftI2C_InitTypeDef_t *cfg, const uint8_t reg, uint8_t *buf);						/*! @brief	读取 1 字节数据 */
uint8_t LQ_Soft_I2C_RecvBuffer(const LQConfig_SoftI2C_InitTypeDef_t *cfg, const uint8_t reg, uint8_t *data_t, uint8_t length);	/*! @brief	读取 n 字节数据 */

uint8_t LQ_Soft_I2C_SendByte  (const LQConfig_SoftI2C_InitTypeDef_t *cfg, const uint8_t reg, uint8_t  data_t);					/*! @brief	发送 1 字节数据 */
uint8_t LQ_Soft_I2C_SendBuffer(const LQConfig_SoftI2C_InitTypeDef_t *cfg, const uint8_t reg, uint8_t *data_t, uint8_t length);	/*! @brief	发送 n 字节数据 */

#endif
