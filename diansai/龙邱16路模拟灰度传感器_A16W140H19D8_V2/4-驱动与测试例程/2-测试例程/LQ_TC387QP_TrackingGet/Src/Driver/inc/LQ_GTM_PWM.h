#ifndef LQ_GTM_PWM_H
#define LQ_GTM_PWM_H

#include "lq_include.h"

// ATOM 时钟 100MHz
#define ATOM_PWM_CLK 100000000

// ATOM 最大占空比 可自行修改
#define ATOM_PWM_MAX 10000.0f

// TOM 时钟 6.25MHz/2
#define TOM_PWM_CLK 6250000

// TOM 最大占空比 可自行修改
#define TOM_PWM_MAX 10000.0f

////////////////ATOM_PWM//////////////////////
void ATOM_PWM_InitConfig(IfxGtm_Atom_ToutMap pin, unsigned long duty, unsigned long pwmFreq_Hz);
void ATOM_PWM_SetDuty(IfxGtm_Atom_ToutMap pin, unsigned long duty, unsigned long pwmFreq_Hz);

////////////////TOM_TIM//////////////////////
void TOM_PWM_InitConfig(IfxGtm_Tom_ToutMap pin, unsigned long duty, unsigned long pwmFreq_Hz);
void TOM_PWM_SetDuty(IfxGtm_Tom_ToutMap pin, unsigned long duty, unsigned long pwmFreq_Hz);


////////////////TIM_IN//////////////////////
void TIM_InitConfig(IfxGtm_Tim_TinMap pin);
void TIM_GetPwm(IfxGtm_Tim_TinMap pin, float32 *Period, float32 *Duty);

#endif /* LQ_GTM_PWM_H */
