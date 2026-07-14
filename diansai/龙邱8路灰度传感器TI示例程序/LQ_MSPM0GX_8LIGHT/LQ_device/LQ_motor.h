#ifndef _LQ_MOTOR_H
#define _LQ_MOTOR_H

#include "include.h"


/*  
 * 当前电机 PWM 的自动重装载值减 1，此值需与 SysConfig 配置中的设置保持一致。
 * 该值定义了 PWM 信号一个周期内的计数值上限，用于确定 PWM 信号的周期。
 */
#define PWM_Period	2560

/*  
 * 该宏定义用于将 PWM 的原始值进行缩放，使其范围转换到 0 至 10000。
 * 通过该缩放因子，可将 PWM 的实际值映射到 0 - 10000 的标准范围，方便进行统一的占空比设置和控制。
 */
#define PWM_Denom	(float)(10000/(float)PWM_Period)


#define Motor1	1
#define Motor2	2


void PWM_Set(GPTIMER_Regs *INST, int32_t Duty, DL_TIMER_CC_INDEX IDX);
void Motor_Ctrl(int32_t Duty2, int32_t Duty1);
void LQ_Test_Motor();

#endif
