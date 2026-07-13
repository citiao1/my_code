/*******************************************************************************
 * @file                3-LQ_EXTI_Demo.c
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

static LQEnum_GPIO_Pin_t  exti = GPIO_Pin_B_0;      // 外部中断检测引脚
static LQEnum_EXIT_Port_t port = LQ_EXIT_PORT_B;    // 外部中断端口
static LQEnum_GPIO_Pin_t  led  = GPIO_Pin_A_15;     // LED 引脚选择

/*************************************************************************
 * @name     LQ_EXTI_Demo_Handler
 *
 * @brief    EXTI 示例中断处理函数
 * @param    none
 * @return   none
 * 
 * @note     通过外部中断检测 GPIO 引脚的状态，并切换 LED 引脚的状态。
 * @note     该函数在 LQ_isr.c 文件中被 GROUP1_IRQHandler 函数调用。
 *************************************************************************/
void LQ_EXTI_Demo_Handler(void)
{
    static int gpio_flag = 0;
    gpio_flag = DL_GPIO_getEnabledInterruptStatus(LQ_GPIO_Regs[LQ_GPIO_MAP[exti][0]], (1U << LQ_GPIO_MAP[exti][1]));
    if (gpio_flag != 0)
    {
        LQ_GPIO_TogglePin(led);
        DL_GPIO_clearInterruptStatus(LQ_GPIO_Regs[LQ_GPIO_MAP[exti][0]], (1U << LQ_GPIO_MAP[exti][1]));
    }
}

/*************************************************************************
 * @name     LQ_EXTI_Demo
 *
 * @brief    EXTI 示例 (外部中断)
 * @param    none
 * @return   none
 * 
 * @note     初始化指定 GPIO 引脚并检测其输入状态，并通过 LED 灯显示当前状态。
 *************************************************************************/
void LQ_EXTI_Demo(void)
{
    // 初始化 LED 引脚
    LQConfig_GPIO_InitTypeDef_t gpio_init = {
        .Mode  = GPIO_MODE_OUTPUT_PP,       // 设置推挽输出模式
        .Pull  = GPIO_RESISTOR_PULL_DOWN,   // 设置下拉电阻
        .Speed = GPIO_SPEED_LOW,            // 设置 GPIO 速度为低速模式 
    };
    LQ_GPIO_Init(led, &gpio_init);          // 初始化 led 引脚
    LQ_GPIO_WritePin(led, 1);               // 输出高电平

    // 初始化并使能外部中断
    LQ_EXIT_Init(exti,                          // 初始化外部中断引脚
                 GPIO_RESISTOR_PULL_UP,         // 引脚上拉
                 LQ_EXIT_TRIGGER_FALLING);      // 下降沿触发中断
    LQ_EXIT_Enable(port, NVIC_Priority_Highest);// 使能外部中断

    while (1)
    {
        delay_ms(10);    // 延时 10ms
    }
}
