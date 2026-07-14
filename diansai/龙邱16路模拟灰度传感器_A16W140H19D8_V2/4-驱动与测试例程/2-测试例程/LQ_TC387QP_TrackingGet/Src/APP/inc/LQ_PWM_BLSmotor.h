#ifndef _LQ_PWM_BLSMOTOR_H_
#define _LQ_PWM_BLSMOTOR_H_

#include "lq_include.h"

#define Blsmotor_Max                1510     // PWM最大值
#define Blsmotor_Min                510      // PWM最小值

#define Blsmotor_Frequency          50 // PWM频率
#define ATOMBLS1 IfxGtm_ATOM0_5_TOUT15_P00_6_OUT
#define ATOMBLS2 IfxGtm_ATOM1_6_TOUT16_P00_7_OUT
#define ATOMBLS3 IfxGtm_ATOM0_0_TOUT53_P21_2_OUT  // MINI 负压风机pwm

// 定义模块号
typedef enum
{
    BLS1   = 0,  // 电调1
    BLS2   = 1,  // 电调2
    BLS3   = 2,  // 电调3，MINI负压风机
    BLSALL = 3   // 电调1&2

} Bls_e;

void BLSmotorInit(void);
void BlsmotorCtrl(Bls_e BLS, uint32 duty);
void Test_BlsMotor(void);
#endif
