/*******************************************************************************
 * @file                LQ_servo.h
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
#ifndef __LQ_SERVO_H__
#define __LQ_SERVO_H__

#include "include.h"

/****************************************************************************************************
 * @brief   宏定义
 ****************************************************************************************************/

#define SERVO_PWM_SYS_CLOCK         CPUCLK_FREQ /* 总时钟频率 */

#define SERVO_PWM_DUTY_MAX          ( 10000 )   /* 占空比最大值映射, 0=0%, 10000=100% */

#define SERVO_PWM_CH2_CH3           LQ_TIMERG_0 /* 通道2和通道3所使用的定时器 */
#define SERVO_PWM_CH1_CH4_CH5       LQ_TIMERA_0 /* 通道1、通道4和通道5所使用的定时器 */

/*!
 * @brief       舵机 PWM 输出通道
 *
 * @note        注意：若使用以下程序用来控制舵机, 则 TIMERA0 和 TIMERG0 两个定时器被占用.
 *              若要使用定时器中断, 请选择其他定时器.
 */
typedef enum
{
    SERVO_CH1 = LQ_TIMERA0_PWM_CH1_Pin_B_9,     // 对应引脚 PB9
    SERVO_CH2 = LQ_TIMERG0_PWM_CH0_Pin_B_10,    // 对应引脚 PB10
    SERVO_CH3 = LQ_TIMERG0_PWM_CH1_Pin_B_11,    // 对应引脚 PB11
    SERVO_CH4 = LQ_TIMERA0_PWM_CH2_Pin_B_12,    // 对应引脚 PB12
    SERVO_CH5 = LQ_TIMERA0_PWM_CH3_Pin_B_13,    // 对应引脚 PB13
} LQEnum_Servo_CH_t;

/****************************************************************************************************
 * @brief   函数定义
 ****************************************************************************************************/

void LQ_Servo_Init(LQEnum_Servo_CH_t ch, uint32_t period, uint32_t duty);   /*! @brief  初始化舵机, 并设置频率和初始占空比 */

void LQ_Servo_SetDuty(LQEnum_Servo_CH_t ch, uint32_t duty);                 /*! @brief  设置占空比 */

#endif
