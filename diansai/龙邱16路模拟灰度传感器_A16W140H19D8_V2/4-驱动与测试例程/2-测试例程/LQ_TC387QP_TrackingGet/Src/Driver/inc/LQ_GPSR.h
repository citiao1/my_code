#ifndef __LQ_GPSR_H__
#define __LQ_GPSR_H__

#include "lq_include.h"

/**
 * 软件中的序号枚举
 */
typedef enum
{
    SoftIRQ0,
    SoftIRQ1,
    SoftIRQ2,
    SoftIRQ3,
    SoftIRQ4
} SOFT_IRQ;

void GPSR_InitConfig(IfxSrc_Tos cpu, SOFT_IRQ index);
void CPSR_Trig(IfxSrc_Tos cpu, SOFT_IRQ index);

#endif /* __LQ_GPSR_H__ */