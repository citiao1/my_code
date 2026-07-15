/*******************************************************************************
 * @file                LQ_spi.c
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
#include "LQ_spi.h"
#include "LQ_gpio.h"

/**
 * @brief       SPI寄存器组
 */
SPI_Regs * const LQ_SPI_Regs[] = {SPI0, SPI1};

static const uint8_t LQ_SPI_MAP[][2] = {    
    /*! 不初始化 */
    {NULL, NULL},

    {GPIO_Pin_A_6 , IOMUX_PINCM11_PF_SPI0_SCLK},    // SPI0_SCLK_Pin_A_6  -> GPIO_Pin_A_6
    {GPIO_Pin_A_5 , IOMUX_PINCM10_PF_SPI0_PICO},    // SPI0_MOSI_Pin_A_5  -> GPIO_Pin_A_5
    {GPIO_Pin_A_4 , IOMUX_PINCM9_PF_SPI0_POCI },    // SPI0_MISO_Pin_A_4  -> GPIO_Pin_A_4
    {GPIO_Pin_A_2 , IOMUX_PINCM7_PF_SPI0_CS0  },    // SPI0_CS0_Pin_A_2   -> GPIO_Pin_A_2

    {GPIO_Pin_A_11, IOMUX_PINCM22_PF_SPI0_SCLK},    // SPI0_SCLK_Pin_A_11 -> GPIO_Pin_A_11
    {GPIO_Pin_A_9 , IOMUX_PINCM20_PF_SPI0_PICO},    // SPI0_MOSI_Pin_A_9  -> GPIO_Pin_A_9
    {GPIO_Pin_A_10, IOMUX_PINCM21_PF_SPI0_POCI},    // SPI0_MISO_Pin_A_10 -> GPIO_Pin_A_10
    {GPIO_Pin_A_8 , IOMUX_PINCM19_PF_SPI0_CS0 },    // SPI0_CS0_Pin_A_8   -> GPIO_Pin_A_8

    {GPIO_Pin_A_12, IOMUX_PINCM34_PF_SPI0_SCLK},    // SPI0_SCLK_Pin_A_12 -> GPIO_Pin_A_12
    {GPIO_Pin_A_14, IOMUX_PINCM36_PF_SPI0_PICO},    // SPI0_MOSI_Pin_A_14 -> GPIO_Pin_A_14
    {GPIO_Pin_A_13, IOMUX_PINCM35_PF_SPI0_POCI},    // SPI0_MISO_Pin_A_13 -> GPIO_Pin_A_13
    {GPIO_Pin_B_25, IOMUX_PINCM56_PF_SPI0_CS0 },    // SPI0_CS0_Pin_B_25  -> GPIO_Pin_B_25

    {GPIO_Pin_B_18, IOMUX_PINCM44_PF_SPI0_SCLK},    // SPI0_SCLK_Pin_B_18 -> GPIO_Pin_B_18
    {GPIO_Pin_B_17, IOMUX_PINCM43_PF_SPI0_PICO},    // SPI0_MOSI_Pin_B_17 -> GPIO_Pin_B_17
    {GPIO_Pin_B_19, IOMUX_PINCM45_PF_SPI0_POCI},    // SPI0_MISO_Pin_B_19 -> GPIO_Pin_B_19

    {GPIO_Pin_B_9 , IOMUX_PINCM26_PF_SPI1_SCLK},    // SPI1_SCLK_Pin_B_9  -> GPIO_Pin_B_9
    {GPIO_Pin_B_8 , IOMUX_PINCM25_PF_SPI1_PICO},    // SPI1_MOSI_Pin_B_8  -> GPIO_Pin_B_8
    {GPIO_Pin_B_7 , IOMUX_PINCM24_PF_SPI1_POCI},    // SPI1_MISO_Pin_B_7  -> GPIO_Pin_B_7
    {GPIO_Pin_B_6 , IOMUX_PINCM23_PF_SPI1_CS0 },    // SPI1_CS0_Pin_B_6   -> GPIO_Pin_B_6

    {GPIO_Pin_B_16, IOMUX_PINCM33_PF_SPI1_SCLK},    // SPI1_SCLK_Pin_B_16 -> GPIO_Pin_B_16
    {GPIO_Pin_B_15, IOMUX_PINCM32_PF_SPI1_PICO},    // SPI1_MOSI_Pin_B_15 -> GPIO_Pin_B_15
    {GPIO_Pin_B_14, IOMUX_PINCM31_PF_SPI1_POCI},    // SPI1_MISO_Pin_B_14 -> GPIO_Pin_B_14
    {GPIO_Pin_B_20, IOMUX_PINCM48_PF_SPI1_CS0 },    // SPI1_CS0_Pin_B_20  -> GPIO_Pin_B_20

    {GPIO_Pin_A_17, IOMUX_PINCM39_PF_SPI1_SCLK},    // SPI1_SCLK_Pin_A_17 -> GPIO_Pin_A_17
    {GPIO_Pin_A_18, IOMUX_PINCM40_PF_SPI1_PICO},    // SPI1_MOSI_Pin_A_18 -> GPIO_Pin_A_18
    {GPIO_Pin_A_16, IOMUX_PINCM38_PF_SPI1_POCI},    // SPI1_MISO_Pin_A_16 -> GPIO_Pin_A_16
    {GPIO_Pin_A_26, IOMUX_PINCM59_PF_SPI1_CS0 },    // SPI1_CS0_Pin_A_26  -> GPIO_Pin_A_26

    {GPIO_Pin_B_23, IOMUX_PINCM51_PF_SPI1_SCLK},    // SPI1_SCLK_Pin_B_23 -> GPIO_Pin_B_23
    {GPIO_Pin_B_22, IOMUX_PINCM50_PF_SPI1_PICO},    // SPI1_MOSI_Pin_B_22 -> GPIO_Pin_B_22
    {GPIO_Pin_B_21, IOMUX_PINCM49_PF_SPI1_POCI},    // SPI1_MISO_Pin_B_21 -> GPIO_Pin_B_21
};

/*************************************************************************
 * @brief       SPI初始化
 * 
 * @param       SPI                 SPI通道
 * @param       init                SPI初始化配置结构体
 * 
 * @example     LQConfig_SPI_InitTypeDef_t spi = {
 *                  .DivideRatio = DL_SPI_CLOCK_DIVIDE_RATIO_1,
 *                  .Prescaler   = 0,
 *                  .mode        = DL_SPI_MODE_CONTROLLER,
 *                  .dataSize    = DL_SPI_DATA_SIZE_8,
 *                  .bitOrder    = DL_SPI_BIT_ORDER_MSB_FIRST,
 *                  .frameFormat = DL_SPI_FRAME_FORMAT_MOTO3_POL0_PHA0,
 *                  .SCLK        = SPI1_SCLK_Pin_B_9,
 *                  .MOSI        = SPI1_MOSI_Pin_B_8,
 *              };
 *              LQ_SPI_Init(LQ_SPI1, &spi);
 *************************************************************************/
void LQ_SPI_Init(LQEnum_SPI_t SPI, LQConfig_SPI_InitTypeDef_t *SPI_Init)
{
    if(SPI_Init->SCLK != SPI_NO_PIN)
    IOMUX->SECCFG.PINCM[LQ_SPI_MAP[SPI_Init->SCLK][0]] = LQ_SPI_MAP[SPI_Init->SCLK][1] | IOMUX_PINCM_PC_CONNECTED;

    if(SPI_Init->MOSI != SPI_NO_PIN)
    IOMUX->SECCFG.PINCM[LQ_SPI_MAP[SPI_Init->MOSI][0]] = LQ_SPI_MAP[SPI_Init->MOSI][1] | IOMUX_PINCM_PC_CONNECTED;

    if(SPI_Init->MISO != SPI_NO_PIN)
    IOMUX->SECCFG.PINCM[LQ_SPI_MAP[SPI_Init->MISO][0]] = LQ_SPI_MAP[SPI_Init->MISO][1] | IOMUX_PINCM_PC_CONNECTED | IOMUX_PINCM_INENA_ENABLE;

    if(SPI_Init->CS != SPI_NO_PIN)
    IOMUX->SECCFG.PINCM[LQ_SPI_MAP[SPI_Init->CS][0]] = LQ_SPI_MAP[SPI_Init->CS][1] | IOMUX_PINCM_PC_CONNECTED;

    LQ_SPI_Regs[SPI]->CLKSEL = (uint32_t) DL_SPI_CLOCK_BUSCLK;

    LQ_SPI_Regs[SPI]->CLKDIV = (uint32_t) SPI_Init->DivideRatio;
    
    DL_Common_updateReg(&LQ_SPI_Regs[SPI]->CTL0,
        (uint32_t)SPI_Init->chipSelectPin | (uint32_t)SPI_Init->frameFormat | (uint32_t)SPI_Init->dataSize,
        SPI_CTL0_CSSEL_MASK | SPI_CTL0_FRF_MASK | SPI_CTL0_SPO_MASK |
        SPI_CTL0_SPH_MASK | SPI_CTL0_DSS_MASK);

    DL_Common_updateReg(&LQ_SPI_Regs[SPI]->CTL1,
        (uint32_t)DL_SPI_PARITY_NONE | (uint32_t)SPI_Init->bitOrder | (uint32_t)SPI_Init->mode,
        SPI_CTL1_PES_MASK | SPI_CTL1_PREN_MASK | SPI_CTL1_PTEN_MASK |
        SPI_CTL1_MSB_MASK | SPI_CTL1_CP_MASK);

    DL_Common_updateReg(&LQ_SPI_Regs[SPI]->CLKCTL, SPI_Init->Prescaler, SPI_CLKCTL_SCR_MASK);

    DL_SPI_setFIFOThreshold(LQ_SPI_Regs[SPI], DL_SPI_RX_FIFO_LEVEL_1_2_FULL, DL_SPI_TX_FIFO_LEVEL_1_2_EMPTY);

    DL_SPI_enable(LQ_SPI_Regs[SPI]);
}

/*************************************************************************
 * @brief       SPI发送字节
 * 
 * @param       SPI                 SPI通道
 * @param       byte                要发送的字节
 * 
 * @example     LQ_SPI_SendByte(LQ_SPI1, 0x55);
 *************************************************************************/
void LQ_SPI_SendByte(LQEnum_SPI_t SPI, uint8_t byte)
{
    while ( (LQ_SPI_Regs[SPI]->STAT & SPI_STAT_TNF_MASK) == SPI_STAT_TNF_FULL ) {};
    
    LQ_SPI_Regs[SPI]->TXDATA = byte;

    while ( (LQ_SPI_Regs[SPI]->STAT & SPI_STAT_BUSY_MASK) == SPI_STAT_BUSY_ACTIVE ) {};
}

/*************************************************************************
 * @brief       SPI接收字节
 * 
 * @param       SPI                 SPI通道
 * @param       byte                接收的字节
 * 
 * @example     uint8_t byte = LQ_SPI_RecvByte(LQ_SPI1);
 *************************************************************************/
uint8_t LQ_SPI_RecvByte(LQEnum_SPI_t SPI)
{
    while ( (LQ_SPI_Regs[SPI]->STAT & SPI_STAT_RFE_MASK) == SPI_STAT_RFE_EMPTY ) {};

    return ((uint8_t)(LQ_SPI_Regs[SPI]->RXDATA));
}

/*************************************************************************
 * @brief       SPI使能DMA接收触发器
 * 
 * @param       SPI                 SPI通道
 *************************************************************************/
void LQ_SPI_EnableDMA_Recv(LQEnum_SPI_t SPI)
{
    LQ_SPI_Regs[SPI]->DMA_TRIG_RX.IMASK = DL_SPI_DMA_INTERRUPT_RX;
}

/*************************************************************************
 * @brief       SPI使能DMA接收超时触发器
 * 
 * @param       SPI                 SPI通道
 *************************************************************************/
void LQ_SPI_EnableDMA_RecvTimeout(LQEnum_SPI_t SPI)
{
    LQ_SPI_Regs[SPI]->DMA_TRIG_RX.IMASK = DL_SPI_DMA_INTERRUPT_RX_TIMEOUT;
}

/*************************************************************************
 * @brief       SPI使能DMA发送触发器
 * 
 * @param       SPI                 SPI通道
 *************************************************************************/
void LQ_SPI_EnableDMA_Send(LQEnum_SPI_t SPI)
{
    LQ_SPI_Regs[SPI]->DMA_TRIG_TX.IMASK = DL_SPI_DMA_INTERRUPT_TX;
}

/*************************************************************************
 * @brief       SPI获取接收寄存器地址
 * 
 * @param       SPI                 SPI通道
 * @return      uint32_t            接收寄存器地址
 * 
 * @example     uint32_t rxReg = Lq_SPI_GetRXRegister(LQ_SPI1);
 *************************************************************************/
uint32_t LQ_SPI_GetRXRegister(LQEnum_SPI_t SPI)
{
    return (uint32_t)&LQ_SPI_Regs[SPI]->RXDATA;
}

/*************************************************************************
 * @brief       SPI获取发送寄存器地址
 * 
 * @param       SPI                 SPI通道
 * @return      uint32_t            发送寄存器地址
 * 
 * @example     uint32_t txReg = Lq_SPI_GetTXRegister(LQ_SPI1);
 *************************************************************************/
uint32_t LQ_SPI_GetTXRegister(LQEnum_SPI_t SPI)
{
    return (uint32_t)&LQ_SPI_Regs[SPI]->TXDATA;
}
