/*******************************************************************************
 * @file                4-LQ_UART_Demo.c
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
#include "LQ_Demo.h"

/*================================================== 串口接收中断例程 ==================================================*/

/*************************************************************************
 * @name     LQ_UART_IT_Handler
 *
 * @brief    UART 串口接收中断回调函数
 * @param    none
 * @return   none
 *************************************************************************/
void LQ_UART_IT_Handler(void)
{
    // 收到一个字符，直接发送回发送端
    uint8_t data = LQ_UART_IT_RecvByte(LQ_UART0);
    LQ_UART_SendByte(LQ_UART0, data);
}

/*************************************************************************
 * @name     LQ_UART_Rx_IT_Demo
 *
 * @brief    UART 接收中断例程
 * @param    none
 * @return   none
 * 
 * @note     本例程演示了 UART 接收中断的使用方法，在接收到一个字符时，立即发送回发送端。
 *************************************************************************/
void LQ_UART_Rx_IT_Demo(void)
{
    // 串口配置
    LQConfig_UART_InitTypeDef_t uart_init = {
        .Tx          = UART0_TX_Pin_A_10,               // 串口0 TX引脚
        .Rx          = UART0_RX_Pin_A_11,               // 串口0 RX引脚
        .BaudRate    = 115200,                          // 波特率
        .Mode        = DL_UART_MODE_NORMAL,             // 串口模式设置为正常模式即可
        .Direction   = DL_UART_DIRECTION_TX_RX,         // 通信方向设置 发送和接收
        .StopBits    = DL_UART_STOP_BITS_ONE,           // 停止位设置为 1 位
        .Parity      = DL_UART_PARITY_NONE,             // 无奇偶校验
        .FlowControl = DL_UART_FLOW_CONTROL_NONE,       // 无硬件流控制
        .WordLength  = DL_UART_WORD_LENGTH_8_BITS,      // 数据位设置为 8 位
    };
    // 串口初始化
    LQ_UART_Init(LQ_UART0, &uart_init);
    // 串口中断配置
    LQ_UART_ITConfig(LQ_UART0, DL_UART_INTERRUPT_RX, NVIC_Priority_NONE);
    // 使能串口中断接收
    LQ_UART_EnableIT(LQ_UART0);

    // 设置串口0接收中断回调
    LQ_UART_SetRxCallback(LQ_UART0, LQ_UART_IT_Handler);

    while (1)
    {
        delay_ms(10);
    }
}

/*================================================== 串口普通发送例程 ==================================================*/

/*************************************************************************
 * @name     LQ_UART_Tx_Demo
 *
 * @brief    UART 普通发送例程
 * @param    none
 * @return   none
 * 
 * @note     本例程演示了 UART 普通发送的使用方法，在发送一个字符串时，立即发送。
 *************************************************************************/
void LQ_UART_Tx_Demo(void)
{
    // 串口配置
    LQConfig_UART_InitTypeDef_t uart_init = {
        .Tx          = UART0_TX_Pin_A_10,               // 串口0 TX引脚
        .Rx          = UART0_RX_Pin_A_11,               // 串口0 RX引脚
        .BaudRate    = 115200,                          // 波特率
        .Mode        = DL_UART_MODE_NORMAL,             // 串口模式设置为正常模式即可
        .Direction   = DL_UART_DIRECTION_TX_RX,         // 通信方向设置 发送和接收
        .StopBits    = DL_UART_STOP_BITS_ONE,           // 停止位设置为 1 位
        .Parity      = DL_UART_PARITY_NONE,             // 无奇偶校验
        .FlowControl = DL_UART_FLOW_CONTROL_NONE,       // 无硬件流控制
        .WordLength  = DL_UART_WORD_LENGTH_8_BITS,      // 数据位设置为 8 位
    };
    // 串口初始化
    LQ_UART_Init(LQ_UART0, &uart_init);

    char str[] = "Hello World!\r\n";

    while (1)
    {
        LQ_UART_SendBuffer(LQ_UART0, (uint8_t*)str, sizeof(str));
        delay_ms(500);
    }
}

/*================================================== 串口DMA发送例程 ==================================================*/

/*************************************************************************
 * @name     LQ_UART_Tx_DMA_Demo
 *
 * @brief    UART DMA发送例程
 * @param    none
 * @return   none
 * 
 * @note     本例程演示了 UART DMA 发送的使用方法。
 *************************************************************************/
void LQ_UART_Tx_DMA_Demo(void)
{
    char str[] = "Hello World!\r\n";

    // 串口配置
    LQConfig_UART_InitTypeDef_t uart_init = {
        .Tx          = UART0_TX_Pin_A_10,               // 串口0 TX引脚
        .Rx          = UART0_RX_Pin_A_11,               // 串口0 RX引脚
        .BaudRate    = 115200,                          // 波特率
        .Mode        = DL_UART_MODE_NORMAL,             // 串口模式设置为正常模式即可
        .Direction   = DL_UART_DIRECTION_TX_RX,         // 通信方向设置 发送和接收
        .StopBits    = DL_UART_STOP_BITS_ONE,           // 停止位设置为 1 位
        .Parity      = DL_UART_PARITY_NONE,             // 无奇偶校验
        .FlowControl = DL_UART_FLOW_CONTROL_NONE,       // 无硬件流控制
        .WordLength  = DL_UART_WORD_LENGTH_8_BITS,      // 数据位设置为 8 位
    };
    // 串口初始化
    LQ_UART_Init(LQ_UART0, &uart_init);
    // 使能串口 DMA 发送事件
    LQ_UART_EnableDMATransmit(LQ_UART0);

    LQConfig_DMA_InitTypeDef_t dma_init = {
        .trigger       = DMA_Trigger_UART0_TX,          // 触发源为 UART0 的 TX 事件
        .triggerType   = DL_DMA_TRIGGER_TYPE_EXTERNAL,  // 外部触发源
        .transferMode  = DL_DMA_SINGLE_TRANSFER_MODE,   // 单次传输模式
        .srcWidth      = DL_DMA_WIDTH_BYTE,             // 8 位源数据宽度
        .destWidth     = DL_DMA_WIDTH_BYTE,             // 8 位目的数据宽度
        .srcIncrement  = DL_DMA_ADDR_INCREMENT,         // 源地址递增
        .destIncrement = DL_DMA_ADDR_UNCHANGED,         // 目的地址不递增
    };
    // 初始化 DMA 通道
    LQ_DMA_Init(DMA_Channel_0, &dma_init);
    // 配置目的地址为 UART0_TX 寄存器
    LQ_DMA_SetDstAddr(DMA_Channel_0, (uint32_t)LQ_UART_GetTXRegister(LQ_UART0));

    while (1)
    {
        LQ_DMA_SetSrcAddr(DMA_Channel_0, (uint32_t)str);        // 设置源地址为 str 的起始地址
        LQ_DMA_SetTransferSize(DMA_Channel_0, sizeof(str));     // 设置传输长度为 str 的长度
        LQ_DMA_Start(DMA_Channel_0);                            // 启动 DMA 传输
        delay_ms(500);                                          // 延时 500ms
    }
}
