/*******************************************************************************
 * @file                LQ_servo.c
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
#include "LQ_servo.h"

/*************************************************************************
 * @name     LQ_Servo_Init
 *
 * @brief    舵机PWM初始化，自动计算参数并启动PWM输出
 * @param    ch     : 舵机通道号
 * @param    period : 目标PWM周期/频率
 * @param    duty   : 初始占空比(0~PWM_DUTY_MAX)
 * @return   none
 *
 * @note     自动区分不同通道的定时器与时钟配置
 *************************************************************************/
void LQ_Servo_Init(LQEnum_Servo_CH_t ch, uint32_t period, uint32_t duty)
{
    PWM_ConfigTypeDef cfg = {0};
    if (ch == SERVO_CH2 || ch == SERVO_CH3)
        cfg = LQ_PWM_CalcOptimal(SERVO_PWM_CH2_CH3, period);
    else
        cfg = LQ_PWM_CalcOptimal(SERVO_PWM_CH1_CH4_CH5, period);
    
    LQConfig_PWM_InitTypeDef_t pwm_init = {
        .DivideRatio = (DL_TIMER_CLOCK_DIVIDE)(cfg.DivideRatio-1),
        .Prescaler   = cfg.Prescaler,
        .Period      = cfg.Period,
        .PwmMode     = DL_TIMER_PWM_MODE_EDGE_ALIGN_UP,
        .startTimer  = false,
    };
    if (ch == SERVO_CH2 || ch == SERVO_CH3)
        LQ_TIMER_PWMInit(SERVO_PWM_CH2_CH3, &pwm_init);
    else
        LQ_TIMER_PWMInit(SERVO_PWM_CH1_CH4_CH5, &pwm_init); //初始化 PWM
    LQ_TIMER_EnablePWMChannel((LQEnum_PWM_Pin_t)ch);        //使能 PWM 通道
    if (ch == SERVO_CH2 || ch == SERVO_CH3)
        LQ_TIMER_PWM_Start(SERVO_PWM_CH2_CH3);
    else
        LQ_TIMER_PWM_Start(SERVO_PWM_CH1_CH4_CH5);          //启动定时器
    LQ_Servo_SetDuty(ch, duty);
}

/*************************************************************************
 * @name     LQ_Servo_SetDuty
 *
 * @brief    设置舵机PWM占空比
 * @param    ch   : 舵机通道号
 * @param    duty : 目标占空比(范围:0 ~ SERVO_PWM_DUTY_MAX)
 * @return   none
 *
 * @note     自动读取当前重装载值，计算并设置CCR比较值
 *************************************************************************/
void LQ_Servo_SetDuty(LQEnum_Servo_CH_t ch, uint32_t duty)
{
    uint32_t load = 0;
    if (duty > SERVO_PWM_DUTY_MAX)
        return;
    if (ch ==SERVO_CH2 || ch ==SERVO_CH3)
        load = LQ_TIMER_Regs[SERVO_PWM_CH2_CH3]->COUNTERREGS.LOAD;
    else
        load = LQ_TIMER_Regs[SERVO_PWM_CH1_CH4_CH5]->COUNTERREGS.LOAD;
    LQ_TIMER_PWMSetCaptureCompare((LQEnum_PWM_Pin_t)ch, load * duty / SERVO_PWM_DUTY_MAX);
}
