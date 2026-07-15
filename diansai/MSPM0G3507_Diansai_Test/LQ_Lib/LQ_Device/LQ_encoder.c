/*******************************************************************************
 * @file                LQ_encoder.c
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
#include "LQ_encoder.h"

/*!
 * @brief   编码器相关参数结构体数组
 */
static LQConfig_Encoder_InitTypeDef_t *LQ_Encoder_fd[LQ_ENCODER_MAX];
/*!
 * @brief   编码器初始化数量
 */
static uint8_t LQ_Encoder_cnt = 0;

/*************************************************************************
 * @name    LQ_Encoder_Init
 *
 * @brief   编码器初始化函数
 * @param   time   : 编码器速度获取间隔时间，单位为 ms
 * @param   encoder: 编码器相关参数结构体
 * @return  none
 * 
 * @note    该函数用于初始化编码器相关参数，并开启编码器脉冲引脚的中断，并开启编码器计数器的中断
 *************************************************************************/
void LQ_Encoder_Init(uint32_t time, LQConfig_Encoder_InitTypeDef_t *encoder)
{
    // 初始化脉冲检测引脚(外部中断)
    LQ_EXIT_Init(encoder->pinA, GPIO_RESISTOR_PULL_UP, LQ_EXIT_TRIGGER_FALLING);
    LQ_EXIT_Enable((LQ_GPIO_MAP[encoder->pinA][0] == 0) ? LQ_EXIT_PORT_A : LQ_EXIT_PORT_B, NVIC_Priority_Highest);
    // 初始化方向引脚(普通输入模式)
    LQConfig_GPIO_InitTypeDef_t gpio_input = {
        .Mode  = GPIO_MODE_INPUT,
        .Pull  = GPIO_RESISTOR_PULL_DOWN,
        .Speed = GPIO_SPEED_HIGH,
    };
    LQ_GPIO_Init(encoder->pinB, &gpio_input);
    // 初始化定时器(获取编码器检测到的平均速度)
    LQConfig_Timer_InitTypeDef_t timer_init = {
        .DivideRatio = DL_TIMER_CLOCK_DIVIDE_8,
        .Prescaler   = 100 - 1,
        .Period      = (100 * time) - 1,
        .TimerMode   = DL_TIMER_TIMER_MODE_PERIODIC_UP,
        .startTimer  = false,
    };
    LQ_TIMER_BaseInit(LQ_TIMERA_0, &timer_init);
    LQ_TIMER_ITConfig(LQ_TIMERA_0, DL_TIMER_INTERRUPT_LOAD_EVENT, NVIC_Priority_NONE);
    LQ_TIMER_ITEnable(LQ_TIMERA_0);
    LQ_TIMER_Start(LQ_TIMERA_0);
    // 保存编码器相关参数
    LQ_Encoder_fd[LQ_Encoder_cnt] = encoder;
    LQ_Encoder_cnt++;
    // 设置定时器回调函数
    LQ_TIMER_SetLoadCallback(LQ_TIMERA_0, LQ_Encoder_TIMER_IRQHandler);
}

/*************************************************************************
 * @name    LQ_Encoder_IRQHandler
 *
 * @brief   编码器脉冲引脚中断处理函数 (私有函数)
 * @param   none
 * @return  none
 * 
 * @note    该函数在检测到编码器 LSB 引脚的中断时被调用，用于更新编码器计数器的值
 *************************************************************************/
static void LQ_Encoder_IRQHandler(LQConfig_Encoder_InitTypeDef_t *encoder)
{
    // 获取编码器 LSB 引脚的脉冲状态
    encoder->gpio_flag = DL_GPIO_getEnabledInterruptStatus(
        LQ_GPIO_Regs[LQ_GPIO_MAP[encoder->pinA][0]],
        (1U << LQ_GPIO_MAP[encoder->pinA][1])
    );
    if (encoder->gpio_flag != 0)
    {
        if (LQ_GPIO_ReadPin(encoder->pinB) == 0)
        {
            encoder->encoder_cnt++;
        }
        else
        {
            encoder->encoder_cnt--;
        }
        // 清除编码器 LSB 引脚的中断状态
	    DL_GPIO_clearInterruptStatus(LQ_GPIO_Regs[LQ_GPIO_MAP[encoder->pinA][0]], (1U << LQ_GPIO_MAP[encoder->pinA][1]));
    }
}

/*************************************************************************
 * @name    LQ_Encoder_IRQHandlers
 *
 * @brief   编码器脉冲引脚中断处理函数
 * @param   none
 * @return  none
 * 
 * @note    该函数在检测到编码器 LSB 引脚的中断时被调用，用于更新编码器计数器的值
 *************************************************************************/
void LQ_Encoder_IRQHandlers(void)
{
    for (uint8_t i = 0; i < LQ_Encoder_cnt; i++)
    {
        LQ_Encoder_IRQHandler(LQ_Encoder_fd[i]);
    }
}

/*************************************************************************
 * @name    LQ_Encoder_TIMER_IRQHandler
 *
 * @brief   编码器计数器中断处理函数
 * @param   none
 * @return  none
 * 
 * @note    该函数在编码器计数器中断发生时被调用，用于更新编码器计数器的值
 *************************************************************************/
void LQ_Encoder_TIMER_IRQHandler(void)
{
    for (uint8_t i = 0; i < LQ_Encoder_cnt; i++)
    {
        LQ_Encoder_fd[i]->count = LQ_Encoder_fd[i]->encoder_cnt;
        LQ_Encoder_fd[i]->encoder_cnt = 0;
    }
}
