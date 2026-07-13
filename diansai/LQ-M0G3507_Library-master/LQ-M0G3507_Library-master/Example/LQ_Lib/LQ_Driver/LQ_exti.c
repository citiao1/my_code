/*******************************************************************************
 * @file                LQ_exti.c
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
#include "LQ_exti.h"

static const IRQn_Type LQ_EXIT_IRQns[] = {
    GPIOA_INT_IRQn,
    GPIOB_INT_IRQn,
};

/*************************************************************************
 * @brief       外部中断初始化
 * 
 * @param       Pin                 GPIO引脚
 * @param       Pull                上下拉配置
 * @param       Trigger             触发方式
 *              
 * @example     LQ_EXIT_Init(GPIO_Pin_A_1, GPIO_RESISTOR_PULL_UP, LQ_EXIT_TRIGGER_FALLING);
 *************************************************************************/
void LQ_EXIT_Init(LQEnum_GPIO_Pin_t Pin, LQEnum_GPIO_Pull_t Pull, LQEnum_EXIT_Trigger_t Trigger)
{
    uint8_t port = LQ_GPIO_MAP[Pin][0];
    uint8_t gpio_pin = LQ_GPIO_MAP[Pin][1];

    IOMUX->SECCFG.PINCM[Pin] =
        IOMUX_PINCM_INENA_ENABLE | IOMUX_PINCM_PC_CONNECTED |
        ((uint32_t) 0x00000001) | (uint32_t) DL_GPIO_INVERSION_DISABLE |
        (uint32_t) Pull | (uint32_t) DL_GPIO_HYSTERESIS_DISABLE |
        ((uint32_t) DL_GPIO_WAKEUP_DISABLE & IOMUX_PINCM_WCOMP_MASK);
    IOMUX->SECCFG.PINCM[Pin] |=
        ((uint32_t) DL_GPIO_WAKEUP_DISABLE & IOMUX_PINCM_WUEN_MASK);

    uint32_t polarity_val[] = {
        0x1,    /* LQ_EXIT_TRIGGER_RISING  → 01 */
        0x2,    /* LQ_EXIT_TRIGGER_FALLING → 10 */
        0x3,    /* LQ_EXIT_TRIGGER_BOTH    → 11 */
    };

    if (gpio_pin < 16) {
        LQ_GPIO_Regs[port]->POLARITY15_0 |= (uint32_t)(polarity_val[Trigger] << (gpio_pin * 2));
    } else {
        LQ_GPIO_Regs[port]->POLARITY31_16 |= (uint32_t)(polarity_val[Trigger] << ((gpio_pin-16) * 2));
    }
    
    LQ_GPIO_Regs[port]->CPU_INT.ICLR |= 1 << gpio_pin;
    
    LQ_GPIO_Regs[port]->CPU_INT.IMASK |= 1 << gpio_pin;
}

/*************************************************************************
 * @brief       外部中断使能
 * 
 * @param       port                外部中断端口
 * @param       priority            NVIC中断优先级
 *
 * @note		LQ_EXIT_Enable(LQ_EXIT_PORT_A, NVIC_Priority_Highest);
 *************************************************************************/
void LQ_EXIT_Enable(LQEnum_EXIT_Port_t port, LQEnum_NVIC_Priority_t priority)
{
    NVIC_EnableIRQ(LQ_EXIT_IRQns[port]);

    if(priority != NVIC_Priority_NONE)
    NVIC_SetPriority(LQ_EXIT_IRQns[port], priority);
}
