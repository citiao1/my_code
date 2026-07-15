/*******************************************************************************
 * @file                LQ_adc.c
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
#include "LQ_adc.h"

ADC12_Regs * const LQ_ADC_Regs[2] = {ADC0, ADC1};

const IRQn_Type LQ_ADC_IRQns[2] = {ADC0_INT_IRQn, ADC1_INT_IRQn};

static DL_ADC12_ClockConfig adc_clock[2] = {0};
/*************************************************************************
 * @brief       ADC初始化
 * 
 * @param       ADC         ADC端口号
 * @param       ADC_Init    ADC配置结构体指针
 * 
 * @example     LQ_ADC_Init(ADC_Port_0, &ADC_Init);
 *************************************************************************/
void LQ_ADC_Init(LQEnum_ADC_t ADC, LQConfig_ADC_InitTypeDef_t *ADC_Init)
{
    adc_clock[ADC].clockSel    = ADC_Init->clockSel;    // 设置时钟源
    adc_clock[ADC].divideRatio = ADC_Init->divideRatio; // 设置分频
    adc_clock[ADC].freqRange   = ADC_Init->freqRange;   // 设置时钟频率范围
    // 关闭 ADC 转换
    LQ_ADC_DisableConversions(ADC);
    // 设置时钟
    DL_ADC12_setClockConfig(LQ_ADC_Regs[ADC], &adc_clock[ADC]);
    // 设置分辨率和电源模式
    DL_Common_updateReg(&LQ_ADC_Regs[ADC]->ULLMEM.CTL2, 0, ADC12_CTL2_RES_MASK);
    DL_Common_updateReg(&LQ_ADC_Regs[ADC]->ULLMEM.CTL2, ADC_Init->resolution, ADC12_CTL2_RES_MASK);
    DL_ADC12_setPowerDownMode(LQ_ADC_Regs[ADC], DL_ADC12_POWER_DOWN_MODE_MANUAL);
    LQ_ADC_Regs[ADC]->ULLMEM.SCOMP0 = ADC_Init->adcclks;
    // 使能 ADC 转换
    LQ_ADC_EnableConversions(ADC);
}

/*************************************************************************
 * @brief       获取 ADC 转换结果
 * 
 * @param       Channel     ADC通道号
 * 
 * @return      ADC转换结果
 * 
 * @example     uint16_t value = LQ_ADC_GetValue(ADC0_Channel_0_Pin_A_27);
 *************************************************************************/
uint16_t LQ_ADC_GetValue(LQEnum_ADC_Channel_t adc_pin)
{
    uint16_t return_value    = 0;
    uint32_t ch   = (uint32_t)adc_pin / 2;
    uint32_t port = (uint32_t)adc_pin % 2;

    LQ_ADC_DisableConversions(port);
    DL_ADC12_setStartAddress(LQ_ADC_Regs[port], DL_ADC12_SEQ_START_ADDR_00);
    DL_ADC12_configConversionMem
    (
        LQ_ADC_Regs[port],                      // 选择寄存器
        DL_ADC12_MEM_IDX_0,                     // 选择采样存储器
        ch,                                     // 选择通道
        DL_ADC12_REFERENCE_VOLTAGE_VDDA,        // 参考电压
        DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0,    // 采样时钟
        DL_ADC12_AVERAGING_MODE_DISABLED,       // 平均功能
        DL_ADC12_BURN_OUT_SOURCE_DISABLED,      // 烧毁检测
        DL_ADC12_TRIGGER_MODE_AUTO_NEXT,        // 自动触发下一个通道
        DL_ADC12_WINDOWS_COMP_MODE_DISABLED     // 窗口比较器
    );
    LQ_ADC_EnableConversions(port);

    LQ_ADC_StartConversions(port);
    while(!(LQ_ADC_Regs[port]->ULLMEM.CPU_INT.RIS & ADC12_CPU_INT_RIS_MEMRESIFG0_SET));
    return_value = DL_ADC12_getMemResult(LQ_ADC_Regs[port], DL_ADC12_MEM_IDX_0);
    LQ_ADC_Regs[port]->ULLMEM.CPU_INT.ICLR = ADC12_CPU_INT_ICLR_MEMRESIFG0_CLR;
    LQ_ADC_EndConversions(port);

    return return_value;
}

/*************************************************************************
 * @brief       使能 ADC 转换
 * 
 * @param       ADC         ADC 端口号
 * 
 * @example     Lq_ADC_EnableConversions(ADC_Port_0);
 *************************************************************************/
void LQ_ADC_EnableConversions(LQEnum_ADC_t ADC)
{
    LQ_ADC_Regs[ADC]->ULLMEM.CTL0 |= (ADC12_CTL0_ENC_ON);
}

/*************************************************************************
 * @brief       失能 ADC 转换
 * 
 * @param       ADC         ADC端口号
 * 
 * @example     Lq_ADC_DisableConversions(ADC_Port_0);
 *************************************************************************/
void LQ_ADC_DisableConversions(LQEnum_ADC_t ADC)
{
    LQ_ADC_Regs[ADC]->ULLMEM.CTL0 &= ~(ADC12_CTL0_ENC_ON);
}

/*************************************************************************
 * @brief       启动ADC转换
 * 
 * @param       ADC         ADC端口号
 * 
 * @example     LQ_ADC_StartConversions(ADC_Port_0);
 *************************************************************************/
void LQ_ADC_StartConversions(LQEnum_ADC_t ADC)
{
    LQ_ADC_Regs[ADC]->ULLMEM.CTL1 |= (ADC12_CTL1_SC_START);
}

/*************************************************************************
 * @brief       停止 ADC 转换
 * 
 * @param       ADC         ADC端口号
 * 
 * @example     LQ_ADC_EndConversions(ADC_Port_0);
 *************************************************************************/
void LQ_ADC_EndConversions(LQEnum_ADC_t ADC)
{
    LQ_ADC_Regs[ADC]->ULLMEM.CTL1 &= ~(ADC12_CTL1_SC_START);
}
