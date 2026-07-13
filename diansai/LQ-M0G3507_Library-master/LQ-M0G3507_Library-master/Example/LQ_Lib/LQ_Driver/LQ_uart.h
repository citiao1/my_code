/*******************************************************************************
 * @file                LQ_uart.h
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
#ifndef __LQ_UART_H__
#define __LQ_UART_H__

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#include "LQ_common.h"
#include "LQ_gpio.h"
#include "LQ_exti.h"

/**
 * @brief       串口端口枚举
 * 
 * @note        
 */
typedef enum {
    LQ_UART0,       /* UART0 */

    LQ_UART1,       /* UART1 */

    LQ_UART2,       /* UART2 */

    LQ_UART3,       /* UART3 */

    LQ_UART_ALL,    /* 全部串口 */
} LQEnum_UART_t;

/**
 * @brief       通道所支持的引脚枚举
 * 
 * @note        选择通道后, 引脚只能选择通道所对应的引脚
 */
typedef enum {
    /*! UART0 TX */             /*! UART0 RX */
    UART0_TX_Pin_A_0 ,          UART0_RX_Pin_A_1 ,
    UART0_TX_Pin_B_0 ,          UART0_RX_Pin_B_1 ,
    UART0_TX_Pin_A_10,          UART0_RX_Pin_A_11,
    UART0_TX_Pin_A_28,          UART0_RX_Pin_A_31,

    /*! UART1 TX */             /*! UART1 RX */
    UART1_TX_Pin_B_4 ,          UART1_RX_Pin_B_5 ,
    UART1_TX_Pin_B_6 ,          UART1_RX_Pin_B_7 ,
    UART1_TX_Pin_A_8 ,          UART1_RX_Pin_A_9 ,
    UART1_TX_Pin_A_17,          UART1_RX_Pin_A_18,

    /*! UART2 TX */             /*! UART2 RX */
    UART2_TX_Pin_B_15,          UART2_RX_Pin_B_16,
    UART2_TX_Pin_B_17,          UART2_RX_Pin_B_18,
    UART2_TX_Pin_A_21,          UART2_RX_Pin_A_22,
    UART2_TX_Pin_A_23,          UART2_RX_Pin_A_24,

    /*! UART3 TX */             /*! UART3 RX */
    UART3_TX_Pin_B_2 ,          UART3_RX_Pin_B_3 ,
    UART3_TX_Pin_B_12,          UART3_RX_Pin_B_13,
    UART3_TX_Pin_A_14,          UART3_RX_Pin_A_13,
    UART3_TX_Pin_A_26,          UART3_RX_Pin_A_25,
    
} LQEnum_UART_Pin_t;

/**
 * @brief       UART 配置结构体
 * 
 * @note
 */
typedef struct {
	
    /* UART TX Pin */
    LQEnum_UART_Pin_t			Tx;

    /* UART RX Pin */
    LQEnum_UART_Pin_t			Rx;

    /* 串口波特率 */
    uint32_t					BaudRate;

    /* 串口模式 */
    DL_UART_MODE				Mode;

    /* 通信方向 */
    DL_UART_DIRECTION			Direction;

    /* 流控控制 */
    DL_UART_FLOW_CONTROL		FlowControl;

    /* 奇偶校验位设置 */
    DL_UART_PARITY				Parity;

    /* 数据传输大小 */
    DL_UART_WORD_LENGTH			WordLength;

    /* 停止位 */
    DL_UART_STOP_BITS			StopBits;

} LQConfig_UART_InitTypeDef_t;

extern LQ_INT_CallBack_t LQ_UART_IT_CallBack[LQ_UART_ALL];

void LQ_UART_Init(LQEnum_UART_t UART, LQConfig_UART_InitTypeDef_t *UART_Init);
void LQ_UART_ITConfig(LQEnum_UART_t UART, uint32_t UART_IT, LQEnum_NVIC_Priority_t Priority);

void LQ_UART_SendByte(LQEnum_UART_t UART, uint8_t data);
void LQ_UART_SendBuffer(LQEnum_UART_t UART, uint8_t *buf, uint32_t len);

void LQ_UART_SetRxCallback(LQEnum_UART_t UART, LQ_INT_CallBack_t cb);

uint8_t LQ_UART_IT_RecvByte(LQEnum_UART_t UART);
uint8_t LQ_UART_RecvByte(LQEnum_UART_t UART);

void LQ_UART_EnableIT(LQEnum_UART_t UART);
void LQ_UART_DisableIT(LQEnum_UART_t UART);

void LQ_UART_EnableDMAReceive(LQEnum_UART_t UART);
void LQ_UART_EnableDMAReceiveTimeout(LQEnum_UART_t UART);
void LQ_UART_EnableDMATransmit(LQEnum_UART_t UART);

uint32_t LQ_UART_GetRXRegister(LQEnum_UART_t UART);
uint32_t LQ_UART_GetTXRegister(LQEnum_UART_t UART);

#endif
