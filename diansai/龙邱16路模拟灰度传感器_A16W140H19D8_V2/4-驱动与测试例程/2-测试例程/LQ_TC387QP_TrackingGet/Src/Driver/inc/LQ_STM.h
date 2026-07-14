#ifndef __LQ_STM_H__
#define __LQ_STM_H__

#include "lq_include.h"

/**
 * 	STM模块枚举
 */
typedef enum
{
    STM0 = 0,
    STM1
} STM_t;

/**
 * 	STM通道枚举
 */
typedef enum
{
    STM_Channel_0 = 0,
    STM_Channel_1
} STM_Channel_t;

extern IfxStm_CompareConfig g_StmCompareConfig[4];
void STM_InitConfig(STM_t STM, STM_Channel_t channel, unsigned long us);
void STM_DelayUs(STM_t stm, unsigned long us);
unsigned long STM_GetNowUs(STM_t stm);
void STM_DisableInterrupt(STM_t stm, STM_Channel_t channel);
void STM_EnableInterrupt(STM_t stm, STM_Channel_t channel);
void Delay_Ms(uint32 stmms);
void Delay_Us(uint32 stmms);

#endif /* __LQ_STM_H__ */