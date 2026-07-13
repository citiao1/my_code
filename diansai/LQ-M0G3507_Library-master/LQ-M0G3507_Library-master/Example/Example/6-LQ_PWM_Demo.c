/*******************************************************************************
 * @file                6-LQ_PWM_Demo.c
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

/*************************************************************************
 * @name     LQ_PWM_Demo
 *
 * @brief    PWM 输出例程
 *
 * @param    none
 * @return   none
 * 
 * @note     本例程演示了 PWM 输出功能。
 * @note     注意例程中使用的 TIMERG0 定时器原始频率只有 40MHz
 * @note     注意区别不同定时器的频率, 在 LQ_time.h 中可以查询
 *************************************************************************/
void LQ_PWM_Demo(void)
{
    LQConfig_PWM_InitTypeDef_t pwm_init = {
        .DivideRatio = DL_TIMER_CLOCK_DIVIDE_1,         // 输入时钟分割器 1 分频 40MHz / 1 = 40MHz
        .Prescaler   = 1 - 1,                           // 分频器 1 分频 40MHz / 1 = 40MHz
        .Period      = 10000 - 1,                       // 重载值 10000
        .PwmMode     = DL_TIMER_PWM_MODE_EDGE_ALIGN_UP, // 边沿对齐 上升沿有效
        .startTimer  = false                            // 不启动定时器
    };
    // 计算出 PWM 频率为 40MHz / 10000 = 4000Hz = 4KHz

    LQ_TIMER_PWMInit(LQ_TIMERG_0, &pwm_init);               // 初始化 PWM
    LQ_TIMER_EnablePWMChannel(LQ_TIMERG0_PWM_CH1_Pin_B_11); // 使能 PWM 输出引脚
    LQ_TIMER_Start(LQ_TIMERG_0);                            // 启动定时器

    uint16_t count = 0;

    while (1)
    {
        // 修改比较值(占空比)
        LQ_TIMER_PWMSetCaptureCompare(LQ_TIMERG0_PWM_CH1_Pin_B_11, count);
        delay_ms(100);
        count += 20;
        if (count > 10000)
        {
            count = 0;
        }
    }
}
