/*******************************************************************************
 * @file                LQ_motor.c
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
#include "LQ_motor.h"

/*************************************************************************
 * @name     LQ_Motor_Init
 *
 * @brief    电机PWM初始化，自动计算PWM参数并启动PWM输出
 * @param    period : 目标PWM频率/周期
 * @param    duty : 电机初始占空比(-MOTOR_PWM_DUTY_MAX ~ MOTOR_PWM_DUTY_MAX)
 * @return   none
 *
 * @note     初始化指定定时器的两路电机PWM通道并启动输出
 *************************************************************************/
void LQ_Motor_Init(uint32_t period, int32_t duty)
{
    // 初始化 PWM
    PWM_ConfigTypeDef cfg = LQ_PWM_CalcOptimal(MOTOR_PWM_TIMER, period);
    LQConfig_PWM_InitTypeDef_t pwm_init = {
        .DivideRatio = (DL_TIMER_CLOCK_DIVIDE)(cfg.DivideRatio-1),
        .Prescaler   = cfg.Prescaler,
        .Period      = cfg.Period,
        .PwmMode     = DL_TIMER_PWM_MODE_EDGE_ALIGN_UP,
        .startTimer  = false,
    };
    LQ_TIMER_PWMInit(MOTOR_PWM_TIMER, &pwm_init);           //初始化 PWM
    LQ_TIMER_EnablePWMChannel((LQEnum_PWM_Pin_t)MOTOR_CH1); //使能 PWM 通道
    LQ_TIMER_EnablePWMChannel((LQEnum_PWM_Pin_t)MOTOR_CH2);
    LQ_TIMER_PWM_Start(MOTOR_PWM_TIMER);                    //启动定时器
    // 初始化方向引脚
    LQConfig_GPIO_InitTypeDef_t gpio_init = {
        .Mode  = GPIO_MODE_OUTPUT_PP,       // 设置推挽输出模式
        .Pull  = GPIO_RESISTOR_PULL_DOWN,   // 设置下拉电阻
        .Speed = GPIO_SPEED_HIGH,           // 设置 GPIO 速度为高速模式 
    };
    LQ_GPIO_Init(MOTOR_CH1_DIR, &gpio_init);    // 初始化电机一方向引脚
    LQ_GPIO_Init(MOTOR_CH2_DIR, &gpio_init);    // 初始化电机二方向引脚
    if (duty > 0) {
        LQ_GPIO_WritePin(MOTOR_CH1_DIR, 1);
        LQ_GPIO_WritePin(MOTOR_CH2_DIR, 1);
    } else {
        LQ_GPIO_WritePin(MOTOR_CH1_DIR, 0);
        LQ_GPIO_WritePin(MOTOR_CH2_DIR, 0);
    }
    LQ_Motor_SetDuty(MOTOR_CH1, abs(duty));     //设置占空比
    LQ_Motor_SetDuty(MOTOR_CH2, abs(duty));
}

/*************************************************************************
 * @name     LQ_Motor_SetDuty
 *
 * @brief    设置指定电机通道的PWM占空比
 * @param    ch   : 电机通道号(MOTOR_CH1/MOTOR_CH2)
 * @param    duty : 目标占空比(-MOTOR_PWM_DUTY_MAX ~ MOTOR_PWM_DUTY_MAX，10000对应100%)
 * @return   none
 *
 * @note     自动读取重装载值，计算并输出对应占空比
 *************************************************************************/
void LQ_Motor_SetDuty(LQEnum_Motor_CH_t ch, int32_t duty)
{
    uint32_t load = LQ_TIMER_Regs[MOTOR_PWM_TIMER]->COUNTERREGS.LOAD;
    if (ch == MOTOR_CH1) {
        if (duty > 0) {
            LQ_GPIO_WritePin(MOTOR_CH1_DIR, 1);
        } else {
            LQ_GPIO_WritePin(MOTOR_CH1_DIR, 0);
        }
    } else if (ch == MOTOR_CH2) {
        if (duty > 0) {
            LQ_GPIO_WritePin(MOTOR_CH2_DIR, 1);
        } else {
            LQ_GPIO_WritePin(MOTOR_CH2_DIR, 0);
        }
    }
    LQ_TIMER_PWMSetCaptureCompare((LQEnum_PWM_Pin_t)ch, load * abs(duty) / MOTOR_PWM_DUTY_MAX);
}
