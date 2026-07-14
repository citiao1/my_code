#ifndef _LQ_PWM_BLDC_H_
#define _LQ_PWM_BLDC_H_

// 电机频率
#define MOTOR_FREQUENCY 10000

// 霍尔输入管脚 宏定义
#define BLDC_Hall_A P20_3 // U
#define BLDC_Hall_B P20_0 // V
#define BLDC_Hall_C P00_5 // W

// BLDC驱动上桥PWM 宏定义
#define BLDC_AP IfxGtm_ATOM0_6_TOUT42_P23_1_OUT
#define BLDC_BP IfxGtm_ATOM0_5_TOUT40_P32_4_OUT
#define BLDC_CP IfxGtm_ATOM0_0_TOUT53_P21_2_OUT
// BLDC驱动下桥GPIO 宏定义
#define BLDC_AI P21_4
#define BLDC_BI P21_3
#define BLDC_CI P20_8

extern int ENC, S_ENC;
extern float MPS;
extern unsigned int RFx;
void BLDC_HallInit(void);
uint8_t Get_Hall_State(void);
void Test_MotorBLDC(void);
void LQ_BLDCStop(void);
void LQ_BLDCCtrl(void);

extern int BLDC_duty;
#endif
