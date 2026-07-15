/*******************************************************************************
 * @file                LQ_pwm.c
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
#include "LQ_pwm.h"

/*!
 * @brief       PWM引脚映射表
 * 
 * @note        内部使用, 外部禁止修改
 */
static const uint8_t LQ_PWM_MAP[][4] = {

    /* TIMA0 CH0 Pin */
    {GPIO_Pin_A_0 , IOMUX_PINCM1_PF_TIMA0_CCP0 , DL_TIMER_CC_0_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_A_8 , IOMUX_PINCM19_PF_TIMA0_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_A_21, IOMUX_PINCM46_PF_TIMA0_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_B_8 , IOMUX_PINCM25_PF_TIMA0_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_B_14, IOMUX_PINCM31_PF_TIMA0_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERA_0},

    /* TIMA0 CH1 Pin */
    {GPIO_Pin_A_1 , IOMUX_PINCM2_PF_TIMA0_CCP1 , DL_TIMER_CC_1_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_A_3 , IOMUX_PINCM8_PF_TIMA0_CCP1 , DL_TIMER_CC_1_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_A_7 , IOMUX_PINCM14_PF_TIMA0_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_A_9 , IOMUX_PINCM20_PF_TIMA0_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_A_22, IOMUX_PINCM47_PF_TIMA0_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_B_9 , IOMUX_PINCM26_PF_TIMA0_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_B_12, IOMUX_PINCM29_PF_TIMA0_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_B_20, IOMUX_PINCM48_PF_TIMA0_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERA_0},

    /* TIMA0 CH2 Pin */
    {GPIO_Pin_A_3 , IOMUX_PINCM8_PF_TIMA0_CCP2 , DL_TIMER_CC_2_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_A_7 , IOMUX_PINCM14_PF_TIMA0_CCP2, DL_TIMER_CC_2_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_A_10, IOMUX_PINCM21_PF_TIMA0_CCP2, DL_TIMER_CC_2_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_A_15, IOMUX_PINCM37_PF_TIMA0_CCP2, DL_TIMER_CC_2_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_B_0 , IOMUX_PINCM12_PF_TIMA0_CCP2, DL_TIMER_CC_2_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_B_4 , IOMUX_PINCM17_PF_TIMA0_CCP2, DL_TIMER_CC_2_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_B_12, IOMUX_PINCM29_PF_TIMA0_CCP2, DL_TIMER_CC_2_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_B_17, IOMUX_PINCM43_PF_TIMA0_CCP2, DL_TIMER_CC_2_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_B_20, IOMUX_PINCM48_PF_TIMA0_CCP2, DL_TIMER_CC_2_INDEX, LQ_TIMERA_0},

    /* TIMA0 CH3 Pin */
    {GPIO_Pin_A_4 , IOMUX_PINCM9_PF_TIMA0_CCP3 , DL_TIMER_CC_3_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_A_12, IOMUX_PINCM34_PF_TIMA0_CCP3, DL_TIMER_CC_3_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_A_17, IOMUX_PINCM39_PF_TIMA0_CCP3, DL_TIMER_CC_3_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_A_23, IOMUX_PINCM53_PF_TIMA0_CCP3, DL_TIMER_CC_3_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_A_25, IOMUX_PINCM55_PF_TIMA0_CCP3, DL_TIMER_CC_3_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_A_28, IOMUX_PINCM3_PF_TIMA0_CCP3 , DL_TIMER_CC_3_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_B_2 , IOMUX_PINCM15_PF_TIMA0_CCP3, DL_TIMER_CC_3_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_B_13, IOMUX_PINCM30_PF_TIMA0_CCP3, DL_TIMER_CC_3_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_B_24, IOMUX_PINCM52_PF_TIMA0_CCP3, DL_TIMER_CC_3_INDEX, LQ_TIMERA_0},
    {GPIO_Pin_B_26, IOMUX_PINCM57_PF_TIMA0_CCP3, DL_TIMER_CC_3_INDEX, LQ_TIMERA_0},

    /* TIMA1 CH0 Pin */
    {GPIO_Pin_A_10, IOMUX_PINCM21_PF_TIMA1_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERA_1},
    {GPIO_Pin_A_15, IOMUX_PINCM37_PF_TIMA1_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERA_1},
    {GPIO_Pin_A_17, IOMUX_PINCM39_PF_TIMA1_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERA_1},
    {GPIO_Pin_A_28, IOMUX_PINCM3_PF_TIMA1_CCP0 , DL_TIMER_CC_0_INDEX, LQ_TIMERA_1},
    {GPIO_Pin_B_0 , IOMUX_PINCM12_PF_TIMA1_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERA_1},
    {GPIO_Pin_B_2 , IOMUX_PINCM15_PF_TIMA1_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERA_1},
    {GPIO_Pin_B_4 , IOMUX_PINCM17_PF_TIMA1_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERA_1},
    {GPIO_Pin_B_17, IOMUX_PINCM43_PF_TIMA1_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERA_1},
    {GPIO_Pin_B_26, IOMUX_PINCM57_PF_TIMA1_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERA_1},

    /* TIMA1 CH1 Pin */
    {GPIO_Pin_A_11, IOMUX_PINCM22_PF_TIMA1_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERA_1},
    {GPIO_Pin_A_16, IOMUX_PINCM38_PF_TIMA1_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERA_1},
    {GPIO_Pin_A_18, IOMUX_PINCM40_PF_TIMA1_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERA_1},
    {GPIO_Pin_A_24, IOMUX_PINCM54_PF_TIMA1_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERA_1},
    {GPIO_Pin_A_31, IOMUX_PINCM6_PF_TIMA1_CCP1 , DL_TIMER_CC_1_INDEX, LQ_TIMERA_1},
    {GPIO_Pin_B_1 , IOMUX_PINCM13_PF_TIMA1_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERA_1},
    {GPIO_Pin_B_3 , IOMUX_PINCM16_PF_TIMA1_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERA_1},
    {GPIO_Pin_B_5 , IOMUX_PINCM18_PF_TIMA1_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERA_1},
    {GPIO_Pin_B_18, IOMUX_PINCM44_PF_TIMA1_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERA_1},
    {GPIO_Pin_B_27, IOMUX_PINCM58_PF_TIMA1_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERA_1},

    /* TIMG0 CH0 Pin */
    {GPIO_Pin_A_5 , IOMUX_PINCM10_PF_TIMG0_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_0},
    {GPIO_Pin_A_12, IOMUX_PINCM34_PF_TIMG0_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_0},
    {GPIO_Pin_A_23, IOMUX_PINCM53_PF_TIMG0_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_0},
    {GPIO_Pin_B_10, IOMUX_PINCM27_PF_TIMG0_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_0},

    /* TIMG0 CH1 Pin */
    {GPIO_Pin_A_6 , IOMUX_PINCM11_PF_TIMG0_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_0},
    {GPIO_Pin_A_13, IOMUX_PINCM35_PF_TIMG0_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_0},
    {GPIO_Pin_A_24, IOMUX_PINCM54_PF_TIMG0_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_0},
    {GPIO_Pin_B_11, IOMUX_PINCM28_PF_TIMG0_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_0},

    /* TIMG6 CH0 Pin */
    {GPIO_Pin_A_5 , IOMUX_PINCM10_PF_TIMG6_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_6},
    {GPIO_Pin_A_21, IOMUX_PINCM46_PF_TIMG6_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_6},
    {GPIO_Pin_A_29, IOMUX_PINCM4_PF_TIMG6_CCP0 , DL_TIMER_CC_0_INDEX, LQ_TIMERG_6},
    {GPIO_Pin_B_2 , IOMUX_PINCM15_PF_TIMG6_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_6},
    {GPIO_Pin_B_6 , IOMUX_PINCM23_PF_TIMG6_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_6},
    {GPIO_Pin_B_10, IOMUX_PINCM27_PF_TIMG6_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_6},
    {GPIO_Pin_B_26, IOMUX_PINCM57_PF_TIMG6_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_6},

    /* TIMG6 CH1 Pin */
    {GPIO_Pin_A_6 , IOMUX_PINCM11_PF_TIMG6_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_6},
    {GPIO_Pin_A_22, IOMUX_PINCM47_PF_TIMG6_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_6},
    {GPIO_Pin_A_30, IOMUX_PINCM5_PF_TIMG6_CCP1 , DL_TIMER_CC_1_INDEX, LQ_TIMERG_6},
    {GPIO_Pin_B_3 , IOMUX_PINCM16_PF_TIMG6_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_6},
    {GPIO_Pin_B_7 , IOMUX_PINCM24_PF_TIMG6_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_6},
    {GPIO_Pin_B_11, IOMUX_PINCM28_PF_TIMG6_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_6},
    {GPIO_Pin_B_27, IOMUX_PINCM58_PF_TIMG6_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_6},

    /* TIMG7 CH0 Pin */
    {GPIO_Pin_A_3 , IOMUX_PINCM8_PF_TIMG7_CCP0 , DL_TIMER_CC_0_INDEX, LQ_TIMERG_7},
    {GPIO_Pin_A_17, IOMUX_PINCM39_PF_TIMG7_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_7},
    {GPIO_Pin_A_23, IOMUX_PINCM53_PF_TIMG7_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_7},
    {GPIO_Pin_A_26, IOMUX_PINCM59_PF_TIMG7_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_7},
    {GPIO_Pin_A_28, IOMUX_PINCM3_PF_TIMG7_CCP0 , DL_TIMER_CC_0_INDEX, LQ_TIMERG_7},
    {GPIO_Pin_B_15, IOMUX_PINCM32_PF_TIMG7_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_7},

    /* TIMG7 CH1 Pin */
    {GPIO_Pin_A_2 , IOMUX_PINCM7_PF_TIMG7_CCP1 , DL_TIMER_CC_1_INDEX, LQ_TIMERG_7},
    {GPIO_Pin_A_4 , IOMUX_PINCM9_PF_TIMG7_CCP1 , DL_TIMER_CC_1_INDEX, LQ_TIMERG_7},
    {GPIO_Pin_A_7 , IOMUX_PINCM14_PF_TIMG7_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_7},
    {GPIO_Pin_A_18, IOMUX_PINCM40_PF_TIMG7_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_7},
    {GPIO_Pin_A_24, IOMUX_PINCM54_PF_TIMG7_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_7},
    {GPIO_Pin_A_27, IOMUX_PINCM60_PF_TIMG7_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_7},
    {GPIO_Pin_A_31, IOMUX_PINCM6_PF_TIMG7_CCP1 , DL_TIMER_CC_1_INDEX, LQ_TIMERG_7},
    {GPIO_Pin_B_16, IOMUX_PINCM33_PF_TIMG7_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_7},
    {GPIO_Pin_B_19, IOMUX_PINCM45_PF_TIMG7_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_7},

    /* TIMG8 CH0 Pin */
    {GPIO_Pin_A_1 , IOMUX_PINCM2_PF_TIMG8_CCP0 , DL_TIMER_CC_0_INDEX, LQ_TIMERG_8},
    {GPIO_Pin_A_3 , IOMUX_PINCM8_PF_TIMG8_CCP0 , DL_TIMER_CC_0_INDEX, LQ_TIMERG_8},
    {GPIO_Pin_A_5 , IOMUX_PINCM10_PF_TIMG8_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_8},
    {GPIO_Pin_A_7 , IOMUX_PINCM14_PF_TIMG8_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_8},
    {GPIO_Pin_A_21, IOMUX_PINCM46_PF_TIMG8_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_8},
    {GPIO_Pin_A_23, IOMUX_PINCM53_PF_TIMG8_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_8},
    {GPIO_Pin_A_26, IOMUX_PINCM59_PF_TIMG8_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_8},
    {GPIO_Pin_A_29, IOMUX_PINCM4_PF_TIMG8_CCP0 , DL_TIMER_CC_0_INDEX, LQ_TIMERG_8},
    {GPIO_Pin_B_6 , IOMUX_PINCM23_PF_TIMG8_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_8},
    {GPIO_Pin_B_10, IOMUX_PINCM27_PF_TIMG8_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_8},
    {GPIO_Pin_B_15, IOMUX_PINCM32_PF_TIMG8_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_8},
    {GPIO_Pin_B_21, IOMUX_PINCM49_PF_TIMG8_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_8},

    /* TIMG8 CH1 Pin */
    {GPIO_Pin_A_0 , IOMUX_PINCM1_PF_TIMG8_CCP1 , DL_TIMER_CC_1_INDEX, LQ_TIMERG_8},
    {GPIO_Pin_A_2 , IOMUX_PINCM7_PF_TIMG8_CCP1 , DL_TIMER_CC_1_INDEX, LQ_TIMERG_8},
    {GPIO_Pin_A_4 , IOMUX_PINCM9_PF_TIMG8_CCP1 , DL_TIMER_CC_1_INDEX, LQ_TIMERG_8},
    {GPIO_Pin_A_6 , IOMUX_PINCM11_PF_TIMG8_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_8},
    {GPIO_Pin_A_22, IOMUX_PINCM47_PF_TIMG8_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_8},
    {GPIO_Pin_A_27, IOMUX_PINCM60_PF_TIMG8_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_8},
    {GPIO_Pin_A_30, IOMUX_PINCM5_PF_TIMG8_CCP1 , DL_TIMER_CC_1_INDEX, LQ_TIMERG_8},
    {GPIO_Pin_B_7 , IOMUX_PINCM24_PF_TIMG8_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_8},
    {GPIO_Pin_B_11, IOMUX_PINCM28_PF_TIMG8_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_8},
    {GPIO_Pin_B_16, IOMUX_PINCM33_PF_TIMG8_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_8},
    {GPIO_Pin_B_19, IOMUX_PINCM45_PF_TIMG8_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_8},
    {GPIO_Pin_B_22, IOMUX_PINCM50_PF_TIMG8_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_8},

    /* TIMG12 CH0 Pin */
    {GPIO_Pin_A_10, IOMUX_PINCM21_PF_TIMG12_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_12},
    {GPIO_Pin_A_14, IOMUX_PINCM36_PF_TIMG12_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_12},
    {GPIO_Pin_B_13, IOMUX_PINCM30_PF_TIMG12_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_12},
    {GPIO_Pin_B_20, IOMUX_PINCM48_PF_TIMG12_CCP0, DL_TIMER_CC_0_INDEX, LQ_TIMERG_12},

    /* TIMG12 CH1 Pin */
    {GPIO_Pin_A_25, IOMUX_PINCM55_PF_TIMG12_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_12},
    {GPIO_Pin_A_31, IOMUX_PINCM6_PF_TIMG12_CCP1 , DL_TIMER_CC_1_INDEX, LQ_TIMERG_12},
    {GPIO_Pin_B_14, IOMUX_PINCM31_PF_TIMG12_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_12},
    {GPIO_Pin_B_24, IOMUX_PINCM52_PF_TIMG12_CCP1, DL_TIMER_CC_1_INDEX, LQ_TIMERG_12},

};

/*************************************************************************
 * @brief       定时器复用PWM初始化
 * 
 * @param       TIMER           定时器枚举
 * @param       TimerPWM_Init   PWM配置结构体
 * 
 * @example     LQConfig_PWM_InitTypeDef pwm_config = {
 *                  .DivideRatio = DL_TIMER_CLOCK_DIVIDE_8,
 *                  .Prescaler   = 100 - 1,
 *                  .Period      = 49999,
 *                  .PwmMode     = DL_TIMER_PWM_MODE_EDGE_ALIGN_UP,
 *                  .startTimer  = true
 *              };
 *              LQ_TIMER_PWMInit(LQ_TIMERA_0, &pwm_config);
 *************************************************************************/
void LQ_TIMER_PWMInit(LQEnum_Timer_t TIMER, LQConfig_PWM_InitTypeDef_t *PWM_Init)
{
    GPTIMER_Regs *timer = LQ_TIMER_Regs[TIMER];

    timer->CLKSEL = (uint32_t)DL_TIMER_CLOCK_BUSCLK;
    
    timer->CLKDIV = (uint32_t)PWM_Init->DivideRatio;
    
    timer->COMMONREGS.CPS = (uint32_t)PWM_Init->Prescaler;

    if( TIMER == LQ_TIMERA_0 )
    {
        switch (PWM_Init->PwmMode)
        {
        case DL_TIMER_PWM_MODE_EDGE_ALIGN:
            DL_Timer_setCaptureCompareAction(timer,
                (DL_TIMER_CC_LACT_CCP_HIGH | DL_TIMER_CC_CDACT_CCP_LOW),
                DL_TIMER_CC_2_INDEX);
            DL_Timer_setCaptureCompareAction(timer,
                (DL_TIMER_CC_LACT_CCP_HIGH | DL_TIMER_CC_CDACT_CCP_LOW),
                DL_TIMER_CC_3_INDEX);
        break;

        case DL_TIMER_PWM_MODE_EDGE_ALIGN_UP:
            DL_Timer_setCaptureCompareAction(timer,
                (DL_TIMER_CC_ZACT_CCP_HIGH | DL_TIMER_CC_CUACT_CCP_LOW),
                DL_TIMER_CC_2_INDEX);
            DL_Timer_setCaptureCompareAction(timer,
                (DL_TIMER_CC_ZACT_CCP_HIGH | DL_TIMER_CC_CUACT_CCP_LOW),
                DL_TIMER_CC_3_INDEX);
        break;

        default:
            DL_Timer_setCaptureCompareAction(timer,
                (DL_TIMER_CC_CUACT_CCP_HIGH | DL_TIMER_CC_CDACT_CCP_LOW),
                DL_TIMER_CC_2_INDEX);
            DL_Timer_setCaptureCompareAction(timer,
                (DL_TIMER_CC_CUACT_CCP_HIGH | DL_TIMER_CC_CDACT_CCP_LOW),
                DL_TIMER_CC_3_INDEX);
            break;
        }

        DL_Timer_setCaptureCompareCtl(
            timer, DL_TIMER_CC_MODE_COMPARE, 0, DL_TIMER_CC_2_INDEX);

        DL_Timer_setCaptureCompareCtl(
            timer, DL_TIMER_CC_MODE_COMPARE, 0, DL_TIMER_CC_3_INDEX);

        DL_Timer_setCaptureCompareOutCtl(timer,
            DL_TIMER_CC_OCTL_INIT_VAL_LOW, DL_TIMER_CC_OCTL_INV_OUT_DISABLED,
            DL_TIMER_CC_OCTL_SRC_FUNCVAL, DL_TIMER_CC_2_INDEX);

        DL_Timer_setCaptureCompareOutCtl(timer,
            DL_TIMER_CC_OCTL_INIT_VAL_LOW, DL_TIMER_CC_OCTL_INV_OUT_DISABLED,
            DL_TIMER_CC_OCTL_SRC_FUNCVAL, DL_TIMER_CC_3_INDEX);

        DL_Timer_setCaptureCompareInput(timer,
            DL_TIMER_CC_INPUT_INV_NOINVERT, DL_TIMER_CC_IN_SEL_CCPX,
            DL_TIMER_CC_2_INDEX);

        DL_Timer_setCaptureCompareInput(timer,
            DL_TIMER_CC_INPUT_INV_NOINVERT, DL_TIMER_CC_IN_SEL_CCPX,
            DL_TIMER_CC_3_INDEX);
    }

    switch (PWM_Init->PwmMode) {
        case DL_TIMER_PWM_MODE_EDGE_ALIGN:
            DL_Timer_setLoadValue(timer, (PWM_Init->Period - (uint32_t) 1));
            DL_Timer_setCaptureCompareAction(timer,
                (DL_TIMER_CC_LACT_CCP_HIGH | DL_TIMER_CC_CDACT_CCP_LOW),
                DL_TIMER_CC_0_INDEX);
            DL_Timer_setCaptureCompareAction(timer,
                (DL_TIMER_CC_LACT_CCP_HIGH | DL_TIMER_CC_CDACT_CCP_LOW),
                DL_TIMER_CC_1_INDEX);
            break;
        case DL_TIMER_PWM_MODE_EDGE_ALIGN_UP:
            DL_Timer_setLoadValue(timer, (PWM_Init->Period - (uint32_t) 1));
            DL_Timer_setCaptureCompareAction(timer,
                (DL_TIMER_CC_ZACT_CCP_HIGH | DL_TIMER_CC_CUACT_CCP_LOW),
                DL_TIMER_CC_0_INDEX);
            DL_Timer_setCaptureCompareAction(timer,
                (DL_TIMER_CC_ZACT_CCP_HIGH | DL_TIMER_CC_CUACT_CCP_LOW),
                DL_TIMER_CC_1_INDEX);
            DL_Timer_setCounterValueAfterEnable(
                timer, DL_TIMER_COUNT_AFTER_EN_ZERO);
            break;
        default:  // DL_TIMER_PWM_MODE_CENTER_ALIGN
            DL_Timer_setLoadValue(timer, (PWM_Init->Period >> 1));
            DL_Timer_setCaptureCompareAction(timer,
                (DL_TIMER_CC_CUACT_CCP_HIGH | DL_TIMER_CC_CDACT_CCP_LOW),
                DL_TIMER_CC_0_INDEX);
            DL_Timer_setCaptureCompareAction(timer,
                (DL_TIMER_CC_CUACT_CCP_HIGH | DL_TIMER_CC_CDACT_CCP_LOW),
                DL_TIMER_CC_1_INDEX);
            break;
    }

    DL_Timer_setCaptureCompareCtl(
        timer, DL_TIMER_CC_MODE_COMPARE, 0, DL_TIMER_CC_0_INDEX);

    DL_Timer_setCaptureCompareCtl(
        timer, DL_TIMER_CC_MODE_COMPARE, 0, DL_TIMER_CC_1_INDEX);

    DL_Timer_setCaptureCompareOutCtl(timer, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
        DL_TIMER_CC_OCTL_INV_OUT_DISABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
        DL_TIMER_CC_0_INDEX);

    DL_Timer_setCaptureCompareOutCtl(timer, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
        DL_TIMER_CC_OCTL_INV_OUT_DISABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
        DL_TIMER_CC_1_INDEX);

    DL_Timer_setCaptureCompareInput(timer, DL_TIMER_CC_INPUT_INV_NOINVERT,
        DL_TIMER_CC_IN_SEL_CCPX, DL_TIMER_CC_0_INDEX);

    DL_Timer_setCaptureCompareInput(timer, DL_TIMER_CC_INPUT_INV_NOINVERT,
        DL_TIMER_CC_IN_SEL_CCPX, DL_TIMER_CC_1_INDEX);

    DL_Common_updateReg(&timer->COUNTERREGS.CTRCTL,
        (GPTIMER_CTRCTL_REPEAT_REPEAT_1 | (uint32_t) PWM_Init->PwmMode |
            (uint32_t) PWM_Init->startTimer),
        (GPTIMER_CTRCTL_CZC_MASK | GPTIMER_CTRCTL_CAC_MASK |
            GPTIMER_CTRCTL_CLC_MASK | GPTIMER_CTRCTL_CVAE_MASK |
            GPTIMER_CTRCTL_CM_MASK | GPTIMER_CTRCTL_REPEAT_MASK |
            GPTIMER_CTRCTL_EN_MASK));

    DL_Timer_setCounterControl(timer,
        DL_TIMER_CZC_CCCTL0_ZCOND, 
        DL_TIMER_CAC_CCCTL0_ACOND, 
        DL_TIMER_CLC_CCCTL0_LCOND);

    DL_Timer_enableClock(timer);

    if(TIMER == LQ_TIMERA_0) {
        DL_TimerG_setCCPDirection(timer , 
        DL_TIMER_CC0_OUTPUT | DL_TIMER_CC1_OUTPUT |
        DL_TIMER_CC2_OUTPUT | DL_TIMER_CC3_OUTPUT);
    }
    else  {
        DL_TimerG_setCCPDirection(timer , 
        DL_TIMER_CC0_OUTPUT | DL_TIMER_CC1_OUTPUT);
    }
}

/*************************************************************************
 * @brief       使能PWM引脚
 * 
 * @param       PWM_PIN         PWM引脚枚举
 *************************************************************************/
void LQ_TIMER_EnablePWMChannel(LQEnum_PWM_Pin_t PWM_PIN)
{
    IOMUX->SECCFG.PINCM[LQ_PWM_MAP[PWM_PIN][0]] = LQ_PWM_MAP[PWM_PIN][1] | IOMUX_PINCM_PC_CONNECTED;
    
    DL_GPIO_enableOutput(LQ_GPIO_Regs[LQ_GPIO_MAP[LQ_PWM_MAP[PWM_PIN][0]][0]], LQ_GPIO_MAP[LQ_PWM_MAP[PWM_PIN][0]][1]);
    
    DL_Timer_setCaptureCompareOutCtl(LQ_TIMER_Regs[LQ_PWM_MAP[PWM_PIN][3]], 
        DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_DISABLED, 
        DL_TIMER_CC_OCTL_SRC_FUNCVAL,
	    LQ_PWM_MAP[PWM_PIN][2]);

    DL_Timer_setCaptCompUpdateMethod(LQ_TIMER_Regs[LQ_PWM_MAP[PWM_PIN][3]], 
        DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, 
        LQ_PWM_MAP[PWM_PIN][2]);
}

/*************************************************************************
 * @brief       启动 PWM 输出
 * 
 * @param       TIMER           定时器枚举
 * 
 * @example     LQ_TIMER_Start(LQ_TIMERA_0);
 *************************************************************************/
void LQ_TIMER_PWM_Start(LQEnum_Timer_t TIMER)
{
    LQ_TIMER_Regs[TIMER]->COUNTERREGS.CTRCTL |= (GPTIMER_CTRCTL_EN_ENABLED);
}

/*************************************************************************
 * @brief       停止 PWM 输出
 * 
 * @param       TIMER           定时器枚举
 * 
 * @example     LQ_TIMER_Start(LQ_TIMERA_0);
 *************************************************************************/
void LQ_TIMER_PWM_Stop(LQEnum_Timer_t TIMER)
{
    LQ_TIMER_Regs[TIMER]->COUNTERREGS.CTRCTL &= ~(GPTIMER_CTRCTL_EN_ENABLED);
}

/*************************************************************************
 * @brief       设置PWM捕获比较值（占空比）
 * 
 * @param       PWM_PIN         PWM引脚枚举
 * @param       Value           PWM值
 *************************************************************************/
void LQ_TIMER_PWMSetCaptureCompare(LQEnum_PWM_Pin_t PWM_PIN, uint32_t Value)
{
    volatile uint32_t *pReg;

    pReg = &LQ_TIMER_Regs[LQ_PWM_MAP[PWM_PIN][3]]->COUNTERREGS.CC_01[0];
    pReg += (uint32_t) LQ_PWM_MAP[PWM_PIN][2];

    *pReg = (Value);
}

/*************************************************************************
 * @name     LQ_PWM_CalcOptimal
 *
 * @brief    根据目标频率计算最优PWM参数(分频比、预分频器、重装载值)
 * @param    ch          : 定时器通道枚举，用于区分时钟源
 * @param    target_freq : 目标PWM频率
 * @return   PWM_ConfigTypeDef : 最优PWM配置参数
 *
 * @note     自动选择最优组合，使重装载值Period最大，PWM控制最精细
 *************************************************************************/
PWM_ConfigTypeDef LQ_PWM_CalcOptimal(LQEnum_Timer_t ch, uint32_t target_freq)
{
    PWM_ConfigTypeDef best = {0};
    uint32_t max_period = 65535U;
    uint32_t best_period = 0;
    uint32_t pwm_system_clk = 0;
    if (ch == LQ_TIMERG_0 || ch == LQ_TIMERG_6)
        pwm_system_clk = CPUCLK_FREQ / 2;
    else
        pwm_system_clk= CPUCLK_FREQ;

    // 遍历所有分频组合，找 Period 最大的合法方案
    for (uint8_t dr = 1; dr <= 8; dr++) {
        // 用 int 而不是 uint8_t，彻底避免溢出死循环！！！
        for (int psc = 0; psc <= 255; psc++) {
            uint32_t denominator = (uint32_t)dr * (psc + 1) * target_freq;
            uint32_t period = pwm_system_clk / denominator;
            // 必须满足 period <= 最大值，且越大越好
            if (period >= 1 && period <= max_period) {
                if (period > best_period) {
                    best_period = period;
                    best.DivideRatio = dr;
                    best.Prescaler = (uint8_t)psc;
                    best.Period = period;
                }
            }
        }
    }
    return best;
}
