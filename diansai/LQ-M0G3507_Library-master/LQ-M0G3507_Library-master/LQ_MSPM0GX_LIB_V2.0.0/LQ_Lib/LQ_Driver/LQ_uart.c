/*******************************************************************************
 * @file                LQ_uart.c
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
#include "LQ_uart.h"
#include <stdio.h>

/*!
 * @brief       UART 接收回调函数数组
 */
LQ_INT_CallBack_t LQ_UART_IT_CallBack[LQ_UART_ALL] = {NULL};

/**
 * @brief       UART 寄存器 映射表
 * 
 * @note
 */
UART_Regs* const LQ_UART_Regs[] = {
    UART0,  UART1,  UART2,  UART3
};

/**
 * @brief       UART 配置寄存器 映射表
 * 
 * @note        
 */
const uint32_t LQ_UART_MAP[][2] = {
    /*! UART0 */
    {GPIO_Pin_A_0 , IOMUX_PINCM1_PF_UART0_TX },     // UART0_TX_Pin_A_0 -> GPIO_Pin_A_0
    {GPIO_Pin_A_1 , IOMUX_PINCM2_PF_UART0_RX },     // UART0_RX_Pin_A_1 -> GPIO_Pin_A_1
    {GPIO_Pin_B_0 , IOMUX_PINCM12_PF_UART0_TX},     // UART0_TX_Pin_B_0 -> GPIO_Pin_B_0
    {GPIO_Pin_B_1 , IOMUX_PINCM13_PF_UART0_RX},     // UART0_RX_Pin_B_1 -> GPIO_Pin_B_1
    {GPIO_Pin_A_10, IOMUX_PINCM21_PF_UART0_TX},     // UART0_TX_Pin_A_10 -> GPIO_Pin_A_10
    {GPIO_Pin_A_11, IOMUX_PINCM22_PF_UART0_RX},     // UART0_RX_Pin_A_11 -> GPIO_Pin_A_11
    {GPIO_Pin_A_28, IOMUX_PINCM3_PF_UART0_TX },     // UART0_TX_Pin_A_28 -> GPIO_Pin_A_28
    {GPIO_Pin_A_31, IOMUX_PINCM6_PF_UART0_RX },     // UART0_RX_Pin_A_31 -> GPIO_Pin_A_31

    /*! UART1 */
    {GPIO_Pin_B_4 , IOMUX_PINCM17_PF_UART1_TX},     // UART1_TX_Pin_B_4 -> GPIO_Pin_B_4
    {GPIO_Pin_B_5 , IOMUX_PINCM18_PF_UART1_RX},     // UART1_RX_Pin_B_5 -> GPIO_Pin_B_5
    {GPIO_Pin_B_6 , IOMUX_PINCM23_PF_UART1_TX},     // UART1_TX_Pin_B_6 -> GPIO_Pin_B_6
    {GPIO_Pin_B_7 , IOMUX_PINCM24_PF_UART1_RX},     // UART1_RX_Pin_B_7 -> GPIO_Pin_B_7
    {GPIO_Pin_A_8 , IOMUX_PINCM19_PF_UART1_TX},     // UART1_TX_Pin_A_8 -> GPIO_Pin_A_8
    {GPIO_Pin_A_9 , IOMUX_PINCM20_PF_UART1_RX},     // UART1_RX_Pin_A_9 -> GPIO_Pin_A_9
    {GPIO_Pin_A_17, IOMUX_PINCM39_PF_UART1_TX},     // UART1_TX_Pin_A_17 -> GPIO_Pin_A_17
    {GPIO_Pin_A_18, IOMUX_PINCM40_PF_UART1_RX},     // UART1_RX_Pin_A_18 -> GPIO_Pin_A_18

    /*! UART2 */
    {GPIO_Pin_B_15, IOMUX_PINCM32_PF_UART2_TX},     // UART2_TX_Pin_B_15 -> GPIO_Pin_B_15
    {GPIO_Pin_B_16, IOMUX_PINCM33_PF_UART2_RX},     // UART2_RX_Pin_B_16 -> GPIO_Pin_B_16
    {GPIO_Pin_B_17, IOMUX_PINCM43_PF_UART2_TX},     // UART2_TX_Pin_B_17 -> GPIO_Pin_B_17
    {GPIO_Pin_B_18, IOMUX_PINCM44_PF_UART2_RX},     // UART2_RX_Pin_B_18 -> GPIO_Pin_B_18
    {GPIO_Pin_A_21, IOMUX_PINCM46_PF_UART2_TX},     // UART2_TX_Pin_A_21 -> GPIO_Pin_A_21
    {GPIO_Pin_A_22, IOMUX_PINCM47_PF_UART2_RX},     // UART2_RX_Pin_A_22 -> GPIO_Pin_A_22
    {GPIO_Pin_A_23, IOMUX_PINCM53_PF_UART2_TX},     // UART2_TX_Pin_A_23 -> GPIO_Pin_A_23
    {GPIO_Pin_A_24, IOMUX_PINCM54_PF_UART2_RX},     // UART2_RX_Pin_A_24 -> GPIO_Pin_A_24

    /*! UART3 */
    {GPIO_Pin_B_2 , IOMUX_PINCM15_PF_UART3_TX},     // UART3_TX_Pin_B_2 -> GPIO_Pin_B_2
    {GPIO_Pin_B_3 , IOMUX_PINCM16_PF_UART3_RX},     // UART3_RX_Pin_B_3 -> GPIO_Pin_B_3
    {GPIO_Pin_B_12, IOMUX_PINCM29_PF_UART3_TX},     // UART3_TX_Pin_B_12 -> GPIO_Pin_B_12
    {GPIO_Pin_B_13, IOMUX_PINCM30_PF_UART3_RX},     // UART3_RX_Pin_B_13 -> GPIO_Pin_B_13
    {GPIO_Pin_A_14, IOMUX_PINCM36_PF_UART3_TX},     // UART3_TX_Pin_A_14 -> GPIO_Pin_A_14
    {GPIO_Pin_A_13, IOMUX_PINCM35_PF_UART3_RX},     // UART3_RX_Pin_A_13 -> GPIO_Pin_A_13
    {GPIO_Pin_A_26, IOMUX_PINCM59_PF_UART3_TX},     // UART3_TX_Pin_A_26 -> GPIO_Pin_A_26
    {GPIO_Pin_A_25, IOMUX_PINCM55_PF_UART3_RX},     // UART3_RX_Pin_A_25 -> GPIO_Pin_A_25
};

/**
 * @brief       UART中断 IRQ 映射表
 *
 * @note
 */
const uint32_t LQ_UART_IRQns[] = {
    UART0_INT_IRQn,
    UART1_INT_IRQn,
    UART2_INT_IRQn,
    UART3_INT_IRQn
};

/********************************************************************************
 * @brief       UART初始化
 * 
 * @param       Uart            串口通道
 * @param       UART_Init       串口初始化结构体
 * 
 * @example     LQConfig_UART_InitTypeDef_t uart_init = {
 *					.Tx          = UART0_TX_Pin_B_0,
 *					.Rx          = UART0_RX_Pin_B_1,
 *					.BaudRate    = 115200,
 *					.Mode        = DL_UART_MODE_NORMAL,
 *					.Direction   = DL_UART_DIRECTION_TX_RX,
 *					.StopBits    = DL_UART_STOP_BITS_ONE,
 *					.Parity      = DL_UART_PARITY_NONE,
 *					.FlowControl = DL_UART_FLOW_CONTROL_NONE,
 *					.WordLength  = DL_UART_WORD_LENGTH_8_BITS
 *				};
 *				LQ_UART_Init(LQ_UART0, &uart_init);
 ********************************************************************************/
void LQ_UART_Init(LQEnum_UART_t UART, LQConfig_UART_InitTypeDef_t *UART_Init)
{
    UART_Regs *Uart = LQ_UART_Regs[UART];

    IOMUX->SECCFG.PINCM[LQ_UART_MAP[UART_Init->Tx][0]] = 
        IOMUX_PINCM_PC_CONNECTED | LQ_UART_MAP[UART_Init->Tx][1];

    IOMUX->SECCFG.PINCM[LQ_UART_MAP[UART_Init->Rx][0]] = 
        IOMUX_PINCM_PC_CONNECTED | IOMUX_PINCM_INENA_ENABLE | 
        LQ_UART_MAP[UART_Init->Rx][1];

    Uart->CLKSEL = (uint32_t)DL_UART_CLOCK_BUSCLK;

    Uart->CLKDIV = (uint32_t)DL_UART_CLOCK_DIVIDE_RATIO_1;
    
    DL_UART_disable(Uart);

    DL_Common_updateReg(&Uart->CTL0,
        (uint32_t) UART_Init->Mode | (uint32_t) UART_Init->Direction |
        (uint32_t) UART_Init->FlowControl,
        UART_CTL0_RXE_MASK | UART_CTL0_TXE_MASK | UART_CTL0_MODE_MASK |
        UART_CTL0_RTSEN_MASK | UART_CTL0_CTSEN_MASK | UART_CTL0_FEN_MASK);

    DL_Common_updateReg(&Uart->LCRH,
        (uint32_t) UART_Init->Parity | (uint32_t) UART_Init->WordLength |
        (uint32_t) UART_Init->StopBits,
        UART_LCRH_PEN_ENABLE | UART_LCRH_EPS_MASK | UART_LCRH_SPS_MASK |
        UART_LCRH_WLEN_MASK | UART_LCRH_STP2_MASK);

    DL_UART_setOversampling(Uart, DL_UART_OVERSAMPLING_RATE_16X);

    double BUSClock = (Uart == UART3) ? 80000000 : 40000000;

    double Divider = BUSClock / (16.0 * UART_Init->BaudRate);

    double IntegerDivider = (uint32_t)Divider;

    uint32_t FractionalDivider = (uint32_t)((Divider - IntegerDivider) * 64 + 0.5);

    DL_UART_setBaudRateDivisor(Uart, IntegerDivider, FractionalDivider);

    DL_UART_enable(Uart);
}

/********************************************************************************
 * @brief       UART中断设置
 * 
 * @param       Uart            串口通道
 * @param       UART_IT         串口中断标志位, 从下面列出的中断中选择
 *              @arg    DL_UART_INTERRUPT_TX				发送寄存器空中断（可发送）
 *              @arg    DL_UART_INTERRUPT_RX				接收数据就绪中断（可读取）
 *              @arg    DL_UART_INTERRUPT_DMA_DONE_TX		发送 DMA 完成中断
 *              @arg    DL_UART_INTERRUPT_DMA_DONE_RX		接收 DMA 完成中断
 *              @arg    DL_UART_INTERRUPT_CTS_DONE			CTS 硬件流控中断
 *              @arg    DL_UART_INTERRUPT_ADDRESS_MATCH		9 位地址匹配中断
 *              @arg    DL_UART_INTERRUPT_LINC0_MATCH		LIN 总线 LINC0 匹配中断
 *              @arg    DL_UART_INTERRUPT_EOT_DONE			一帧数据发送完成中断
 *              @arg    ...
 * @param       Priority        中断优先级
 ********************************************************************************/
void LQ_UART_ITConfig(LQEnum_UART_t UART, uint32_t UART_IT, LQEnum_NVIC_Priority_t Priority)
{
    LQ_UART_Regs[UART]->CPU_INT.IMASK |= UART_IT;

    NVIC_ClearPendingIRQ(LQ_UART_IRQns[UART]);

    if(Priority == NVIC_Priority_NONE) return;
    
    NVIC_SetPriority(LQ_UART_IRQns[UART], Priority);

}

/********************************************************************************
 * @brief       UART设置接收中断回调函数
 * 
 * @param       Uart            串口通道
 * @param       cb              回调函数
 * 
 * @example     LQ_UART_SetRxCallback(UART0, uart0_rx_callback);
 ********************************************************************************/
void LQ_UART_SetRxCallback(LQEnum_UART_t UART, LQ_INT_CallBack_t cb)
{
    LQ_UART_IT_CallBack[UART] = cb;
}

/********************************************************************************
 * @brief       UART中断使能
 * 
 * @param       Uart            串口通道
 * 
 * @example     LQ_UART_EnableIT(UART0);
 ********************************************************************************/
void LQ_UART_EnableIT(LQEnum_UART_t UART)
{
    NVIC_EnableIRQ(LQ_UART_IRQns[UART]);
}

/********************************************************************************
 * @brief       UART中断禁用
 * 
 * @param       Uart            串口通道
 * 
 * @example     LQ_UART_DisableIT(UART0);
 ********************************************************************************/
void LQ_UART_DisableIT(LQEnum_UART_t UART)
{
    NVIC_DisableIRQ(LQ_UART_IRQns[UART]);
}

/********************************************************************************
 * @brief       UART阻塞发送1字节
 * 
 * @param       Uart            串口通道
 * @param       data            发送内容
 ********************************************************************************/
void LQ_UART_SendByte(LQEnum_UART_t UART, uint8_t data)
{
    while( (LQ_UART_Regs[UART]->STAT & UART_STAT_TXFF_MASK) == UART_STAT_TXFF_MASK );

    LQ_UART_Regs[UART]->TXDATA = data;
}

/********************************************************************************
 * @brief       UART发送Buffer
 * 
 * @param       Uart            串口通道
 * @param       buf             发送缓冲数组
 * @param       len             发送数组长度
 ********************************************************************************/
void LQ_UART_SendBuffer(LQEnum_UART_t UART, uint8_t *buf, uint32_t len)
{
    while(len --)
    {
        LQ_UART_SendByte(UART, *buf);
        buf++;
    }
}

/********************************************************************************
 * @brief       UART接收1字节(中断中使用)
 * 
 * @param       Uart            串口通道
 * @param       data            发送内容
 ********************************************************************************/
uint8_t LQ_UART_IT_RecvByte(LQEnum_UART_t UART)
{
    return ((uint8_t)(LQ_UART_Regs[UART]->RXDATA & UART_RXDATA_DATA_MASK));
}

/********************************************************************************
 * @brief       UART阻塞接收1字节
 * 
 * @param       Uart            串口通道
 * @param       data            发送内容
 ********************************************************************************/
uint8_t LQ_UART_RecvByte(LQEnum_UART_t UART)
{
    while( (LQ_UART_Regs[UART]->STAT & UART_STAT_RXFE_MASK) == UART_STAT_RXFE_SET );

    return ((uint8_t)(LQ_UART_Regs[UART]->RXDATA & UART_RXDATA_DATA_MASK));
}

/********************************************************************************
 * @brief       UART使能DMA接收触发器
 * 
 * @param       Uart            串口通道
 ********************************************************************************/
void LQ_UART_EnableDMAReceive(LQEnum_UART_t UART)
{
    LQ_UART_Regs[UART]->DMA_TRIG_RX.IMASK = DL_UART_DMA_INTERRUPT_RX;
}

/********************************************************************************
 * @brief       UART使能DMA接收超时触发器
 * 
 * @param       Uart            串口通道
 ********************************************************************************/
void LQ_UART_EnableDMAReceiveTimeout(LQEnum_UART_t UART)
{
    LQ_UART_Regs[UART]->DMA_TRIG_RX.IMASK = DL_UART_DMA_INTERRUPT_RX_TIMEOUT;
}

/********************************************************************************
 * @brief       UART使能DMA发送触发器
 * 
 * @param       Uart            串口通道
 ********************************************************************************/
void LQ_UART_EnableDMATransmit(LQEnum_UART_t UART)
{
    LQ_UART_Regs[UART]->DMA_TRIG_TX.IMASK = UART_DMA_TRIG_TX_IMASK_TXINT_SET;
}

/********************************************************************************
 * @brief       UART获取接收寄存器地址
 * 
 * @param       Uart            串口通道
 * 
 * @example     uint32_t rxReg = LQ_UART_GetRXRegister(UART0);
 ********************************************************************************/
uint32_t LQ_UART_GetRXRegister(LQEnum_UART_t UART)
{
    return (uint32_t)&LQ_UART_Regs[UART]->RXDATA;
}

/********************************************************************************
 * @brief       UART获取发送寄存器地址
 * 
 * @param       Uart            串口通道
 * 
 * @example     uint32_t txReg = LQ_UART_GetTXRegister(UART0);
 ********************************************************************************/
uint32_t LQ_UART_GetTXRegister(LQEnum_UART_t UART)
{
    return (uint32_t)&LQ_UART_Regs[UART]->TXDATA;
}

/********************************************************************************
 * @brief		printf函数重定义
 ********************************************************************************/
int fputc(int ch, FILE *stream)
{
	// 当串口0忙的时候等待，不忙的时候再发送传进来的字符
	LQ_UART_SendByte(LQ_UART0, ch);

	return ch;
}
