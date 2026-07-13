/*******************************************************************************
 * @file                LQ_clock.c
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
#include "LQ_clock.h"
#include "LQ_uart.h"

/********************************************************************************
 * @brief	PLL 配置.
 * @param	none.
 * @return	none.
 * @note	LQ_System_Clock_Init 函数内部调用, 无需在意, 请勿随意改动.
 ********************************************************************************/
#ifdef LQ_CLK_SRC_HFXT_40MHZ
static const DL_SYSCTL_SYSPLLConfig gSYSPLLConfig = {
    .inputFreq              = DL_SYSCTL_SYSPLL_INPUT_FREQ_32_48_MHZ,
    .rDivClk2x              = 1,
    .rDivClk1               = 0,
    .rDivClk0               = 0,
    .enableCLK2x            = DL_SYSCTL_SYSPLL_CLK2X_ENABLE,
    .enableCLK1             = DL_SYSCTL_SYSPLL_CLK1_ENABLE,
    .enableCLK0             = DL_SYSCTL_SYSPLL_CLK0_DISABLE,
    .sysPLLMCLK             = DL_SYSCTL_SYSPLL_MCLK_CLK2X,
    .sysPLLRef              = DL_SYSCTL_SYSPLL_REF_HFCLK,
    .qDiv                   = 1,
    .pDiv                   = DL_SYSCTL_SYSPLL_PDIV_1
};
#endif

#ifdef LQ_CLK_SRC_HFXT_16MHZ
static const DL_SYSCTL_SYSPLLConfig gSYSPLLConfig = {
    .inputFreq              = DL_SYSCTL_SYSPLL_INPUT_FREQ_8_16_MHZ,
	.rDivClk2x              = 1,
	.rDivClk1               = 0,
	.rDivClk0               = 0,
	.enableCLK2x            = DL_SYSCTL_SYSPLL_CLK2X_ENABLE,
	.enableCLK1             = DL_SYSCTL_SYSPLL_CLK1_ENABLE,
	.enableCLK0             = DL_SYSCTL_SYSPLL_CLK0_DISABLE,
	.sysPLLMCLK             = DL_SYSCTL_SYSPLL_MCLK_CLK2X,
	.sysPLLRef              = DL_SYSCTL_SYSPLL_REF_HFCLK,
	.qDiv                   = 9,
	.pDiv                   = DL_SYSCTL_SYSPLL_PDIV_2
};
#endif

#ifdef LQ_CLK_SRC_SYSOSC_4MHZ
static const DL_SYSCTL_SYSPLLConfig gSYSPLLConfig = {
    .inputFreq              = DL_SYSCTL_SYSPLL_INPUT_FREQ_8_16_MHZ,
	.rDivClk2x              = 1,
	.rDivClk1               = 0,
	.rDivClk0               = 0,
	.enableCLK2x            = DL_SYSCTL_SYSPLL_CLK2X_ENABLE,
	.enableCLK1             = DL_SYSCTL_SYSPLL_CLK1_ENABLE,
	.enableCLK0             = DL_SYSCTL_SYSPLL_CLK0_DISABLE,
	.sysPLLMCLK             = DL_SYSCTL_SYSPLL_MCLK_CLK2X,
	.sysPLLRef              = DL_SYSCTL_SYSPLL_REF_SYSOSC,
	.qDiv                   = 9,
	.pDiv                   = DL_SYSCTL_SYSPLL_PDIV_4
};
#endif

/********************************************************************************
 * @brief	LFCLK 配置.
 * @param	none.
 * @return	none.
 * @note	LQ_System_Clock_Init 函数内部调用, 无需在意, 请勿随意改动.
 ********************************************************************************/
#ifdef LQ_CLK_SRC_LFXT_32KHZ
static const DL_SYSCTL_LFCLKConfig gLFCLKConfig = {
    .lowCap   = false,
    .monitor  = false,
    .xt1Drive = DL_SYSCTL_LFXT_DRIVE_STRENGTH_HIGHEST,
};
#endif

/********************************************************************************
 * @brief	初始化 GPIO 及其电源模块.
 * @param	none.
 * @return	none.
 * @note	none.
 * @note	System_Clock_Init 函数内部调用, 无需在意, 请勿随意改动.
 ********************************************************************************/
static void LQ_System_GPIO_Init(void)
{
	/* 复位, 使其寄存器重置 */
	DL_GPIO_reset(GPIOA);
    DL_GPIO_reset(GPIOB);
    DL_UART_reset(UART0);
    DL_UART_reset(UART1);
    DL_UART_reset(UART2);
    DL_UART_reset(UART3);
    DL_Timer_reset(TIMA0);
    DL_Timer_reset(TIMA1);
    DL_Timer_reset(TIMG0);
    DL_Timer_reset(TIMG6);
    DL_Timer_reset(TIMG7);
    DL_Timer_reset(TIMG8);
    DL_Timer_reset(TIMG12);
    DL_SPI_reset(SPI0);
    DL_SPI_reset(SPI1);
    DL_ADC12_reset(ADC0);
    DL_ADC12_reset(ADC1);

	/* 开启电源 */
    DL_GPIO_enablePower(GPIOA);
    DL_GPIO_enablePower(GPIOB);
    DL_UART_enablePower(UART0);
    DL_UART_enablePower(UART1);
    DL_UART_enablePower(UART2);
    DL_UART_enablePower(UART3);
    DL_Timer_enablePower(TIMA0);
    DL_Timer_enablePower(TIMA1);
    DL_Timer_enablePower(TIMG0);
    DL_Timer_enablePower(TIMG6);
    DL_Timer_enablePower(TIMG7);
    DL_Timer_enablePower(TIMG8);
    DL_Timer_enablePower(TIMG12);
    DL_SPI_enablePower(SPI0);
    DL_SPI_enablePower(SPI1);
    DL_ADC12_enablePower(ADC0);
    DL_ADC12_enablePower(ADC1);

	/* 等待电源稳定*/
	delay_cycles(POWER_STARTUP_DELAY);
}

/********************************************************************************
 * @brief	SYSPLL 系统时钟校准与有效性检测.
 * @param	none.
 * @return	true-时钟正常 false-时钟异常.
 * @note	使用FCC电路测量并校验SYSPLL时钟频率是否合法.
 * @note	系统初始化调用，请勿随意改动.
 ********************************************************************************/
static bool SYSCFG_DL_SYSCTL_SYSPLL_init(void)
{
    bool fFCCRatioStatus = false;
    uint32_t fFCCSysoscCount;
    uint32_t fFCCPllCount;
    uint32_t fFCCRatio;
    uint32_t fccTimeOutCounter;

    DL_SYSCTL_setFCCPeriods(DL_SYSCTL_FCC_TRIG_CNT_01);

    DL_SYSCTL_configFCC(DL_SYSCTL_FCC_TRIG_TYPE_RISE_RISE,
                        DL_SYSCTL_FCC_TRIG_SOURCE_LFCLK,
                        DL_SYSCTL_FCC_CLOCK_SOURCE_SYSPLLCLK1);

    fccTimeOutCounter = 0;
    DL_SYSCTL_startFCC();
    while (DL_SYSCTL_isFCCDone() == 0) {
        delay_cycles(977);
        fccTimeOutCounter++;
        if (fccTimeOutCounter > 65) {
            break;
        }
    }

    fFCCPllCount = DL_SYSCTL_readFCC();

#ifdef LQ_CLK_SRC_SYSOSC_4MHZ
    DL_SYSCTL_configFCC(DL_SYSCTL_FCC_TRIG_TYPE_RISE_RISE,
                        DL_SYSCTL_FCC_TRIG_SOURCE_LFCLK,
                        DL_SYSCTL_FCC_CLOCK_SOURCE_SYSOSC);
#else
    DL_SYSCTL_configFCC(DL_SYSCTL_FCC_TRIG_TYPE_RISE_RISE,
                        DL_SYSCTL_FCC_TRIG_SOURCE_LFCLK,
                        DL_SYSCTL_FCC_CLOCK_SOURCE_HFCLK);
#endif
    /* Get SYSPLL frequency using FCC */
    fccTimeOutCounter = 0;
    DL_SYSCTL_startFCC();
    while (DL_SYSCTL_isFCCDone() == 0) {
        delay_cycles(977);  /* 1x LFCLK cycle = 32MHz/32.768kHz = 977, 30.5us */
        fccTimeOutCounter++;
        if (fccTimeOutCounter > 65) {
            /* Timeout set to approximately 2ms (user-customizable) */
            break;
        }
    }

    /* get measB= SYSOSC freq wrt LFOSC*/
    fFCCSysoscCount = DL_SYSCTL_readFCC();

    /* Get ratio of both measurements*/
    fFCCRatio = (fFCCPllCount * FLOAT_TO_INT_SCALE) / fFCCSysoscCount;
    /* Check ratio is within bounds*/
    if ((FCC_LOWER_BOUND < fFCCRatio) && (fFCCRatio < FCC_UPPER_BOUND)) 
    {
         /* ratio is good for proceeding into application code. */
        fFCCRatioStatus = true;
    }

    return fFCCRatioStatus;
}

/********************************************************************************
 * @brief	系统时钟初始化.
 * @param	none.
 * @return	none.
 * @note	配置系统输出时钟为 80MHz.
 * @note	系统初始化时调用, 无需在意, 请勿随意改动.
 ********************************************************************************/
static void LQ_System_Clock_Init(void)
{
#ifndef LQ_CLK_SRC_SYSOSC_4MHZ
    DL_GPIO_initPeripheralAnalogFunction(IOMUX_PINCM10);
    DL_GPIO_initPeripheralAnalogFunction(IOMUX_PINCM11);
#endif

#ifndef LQ_CLK_SRC_LFOSC_32KHZ
    DL_GPIO_initPeripheralAnalogFunction(IOMUX_PINCM8);
    DL_GPIO_initPeripheralAnalogFunction(IOMUX_PINCM9);
#endif

    DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);
    DL_SYSCTL_setFlashWaitState(DL_SYSCTL_FLASH_WAIT_STATE_2);

    DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);
    /* Set default configuration */
    DL_SYSCTL_disableHFXT();
    DL_SYSCTL_disableSYSPLL();

#ifdef LQ_CLK_SRC_HFXT_40MHZ
    DL_SYSCTL_setHFCLKSourceHFXTParams(DL_SYSCTL_HFXT_RANGE_32_48_MHZ, 0x0F, true);
#endif

#ifdef LQ_CLK_SRC_HFXT_16MHZ
    DL_SYSCTL_setHFCLKSourceHFXTParams(DL_SYSCTL_HFXT_RANGE_8_16_MHZ, 0x0F, true);
#endif

    DL_SYSCTL_configSYSPLL((DL_SYSCTL_SYSPLLConfig *)&gSYSPLLConfig);

    while (SYSCFG_DL_SYSCTL_SYSPLL_init() == false) 
    {
        /* Toggle SYSPLL enable to re-enable SYSPLL and re-check incorrect locking */
        DL_SYSCTL_disableSYSPLL();
        DL_SYSCTL_enableSYSPLL();

        /* Wait until SYSPLL startup is stabilized*/
        while ((DL_SYSCTL_getClockStatus() & SYSCTL_CLKSTATUS_SYSPLLGOOD_MASK) != DL_SYSCTL_CLK_STATUS_SYSPLL_GOOD) {}
    }

    DL_SYSCTL_setULPCLKDivider(DL_SYSCTL_ULPCLK_DIV_2);
    DL_SYSCTL_setHFCLKDividerForMFPCLK(DL_SYSCTL_HFCLK_MFPCLK_DIVIDER_10);
#ifdef LQ_CLK_SRC_LFXT_32KHZ
    DL_SYSCTL_setLFCLKSourceLFXT((DL_SYSCTL_LFCLKConfig *)&gLFCLKConfig);
#endif
    DL_SYSCTL_enableMFCLK();
    DL_SYSCTL_enableMFPCLK();
    DL_SYSCTL_setMFPCLKSource(DL_SYSCTL_MFPCLK_SOURCE_SYSOSC);
    DL_SYSCTL_setMCLKSource(SYSOSC, HSCLK, DL_SYSCTL_HSCLK_SOURCE_SYSPLL);
}

/********************************************************************************
 * @brief	系统滴答时钟初始化.
 * @param	none.
 * @return	none.
 * @note	配置系统滴答时钟为 1.00微秒，用于精准延时函数使用
 * @note	系统初始化时调用, 无需在意, 请勿随意改动.
 ********************************************************************************/
static void LQ_System_Tick_Init(void)
{
    /* 将周期初始化为1.00微秒 */
    DL_SYSTICK_init(80);
    /* 启用SysTick并开始计数 */
    DL_SYSTICK_enable();
}

/********************************************************************************
 * @brief	系统调试串口初始化.
 * @param	none.
 * @return	none.
 * @note	初始化后可使用 printf 进行信息打印.
 * @note	直接连接使用下载器上的串口即可.
 ********************************************************************************/
static void LQ_System_UART_Init(void)
{
	// 串口配置
	LQConfig_UART_InitTypeDef_t uart_init = {
		.Tx          = UART0_TX_Pin_A_10,
		.Rx          = UART0_RX_Pin_A_11,
		.BaudRate    = 115200,
		.Mode        = DL_UART_MODE_NORMAL,
		.Direction   = DL_UART_DIRECTION_TX_RX,
		.StopBits    = DL_UART_STOP_BITS_ONE,
		.Parity      = DL_UART_PARITY_NONE,
		.FlowControl = DL_UART_FLOW_CONTROL_NONE,
		.WordLength  = DL_UART_WORD_LENGTH_8_BITS
	};
	LQ_UART_Init(LQ_UART0, &uart_init);
}

/********************************************************************************
 * @brief	系统初始化.
 * @param	none.
 * @return	none.
 * @example	System_Init();
 * @note	程序上电后最初调用的程序.
 ********************************************************************************/
void LQ_System_Init(void)
{
	LQ_System_GPIO_Init();
	LQ_System_Tick_Init();
	LQ_System_Clock_Init();
	LQ_System_UART_Init();
}
