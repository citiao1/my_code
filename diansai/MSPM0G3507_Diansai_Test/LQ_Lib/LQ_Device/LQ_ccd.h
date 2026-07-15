/*******************************************************************************
 * @file                LQ_ccd.h
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
#ifndef __LQ_CCD_H__
#define __LQ_CCD_H__

#include "include.h"

// -------------------- CCD1 相关配置 --------------------

#define CCD1_AO_PORT            ADC_Port_1
#define CCD1_AO_CHANNEL         ADC1_Channel_5_Pin_B_18   // ADC1 通道 5 PB18 引脚
#define CCD1_CLK_PIN            GPIO_Pin_B_26
#define CCD1_SI_PIN             GPIO_Pin_B_27

// -------------------- CCD2 相关配置 --------------------

#define CCD2_AO_PORT            ADC_Port_1
#define CCD2_AO_CHANNEL         ADC1_Channel_4_Pin_B_17   // ADC2 通道 4 PB17 引脚
#define CCD2_CLK_PIN            GPIO_Pin_A_29
#define CCD2_SI_PIN             GPIO_Pin_A_30

// -------------------- 其他配置 --------------------

#define CCD1_AO_GET(n)          LQ_ADC_GetValue(n);     // 获取 CCDn 采集值

#define CCD_CLK_H(n)            LQ_GPIO_WritePin(n ,1)  // CCDn 时钟 HIGH
#define CCD_CLK_L(n)            LQ_GPIO_WritePin(n ,0)  // CCDn 时钟 LOW

#define CCD_SI_H(n)             LQ_GPIO_WritePin(n ,1)  // CCDn 数据输出 HIGH
#define CCD_SI_L(n)             LQ_GPIO_WritePin(n ,0)  // CCDn 数据输出 LOW

#define THRESHOLD               800     // OLED 上显示亮灭的阈值   范围0-4095

// CCD 端口枚举
typedef enum
{
    CCD1,
    CCD2,
    CCD_MAX   
} LQEnum_CCD_t;

// -------------------- 相关函数 --------------------

/** 二值化滤波后的CCD值 */
extern uint8_t CCD_Bin[CCD_MAX][128];

/*! @brief      CCD 初始化函数 */
void LQ_CCD_Init(LQEnum_CCD_t ccd_num);

/*! @brief      CCD 采集函数 */
void LQ_CCD_Collect(LQEnum_CCD_t ccd_num, uint16_t *p);

/*! @brief      CCD 二值化函数 */
void LQ_CCD_Binaryzation(LQEnum_CCD_t ccd_num, uint16_t threshold, uint16_t *ccd_data, uint8_t *bin_ccd);

/*! @brief      CCD 获取阈值函数 */
uint16_t LQ_CCD_Get_Threshold(uint16_t ccd_data[128]);

/*! @brief      CCD 获取中线偏差函数 */
uint16_t LQ_CCD_Get_Error(uint8_t bin_ccd[128]);

#endif
