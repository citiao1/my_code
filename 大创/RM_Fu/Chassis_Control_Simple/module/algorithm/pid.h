#ifndef PID_H
#define PID_H

#include "main.h"
#include "stdlib.h"
#include "stdio.h"


#ifndef abs
#define abs(x) ((x > 0) ? x : -x)
#endif

typedef enum
{
    PID_IMPROVE_NONE = 0b000000,
    PID_Integral_limit = 0b000001,
    PID_Trapezoid_Intergral = 0b000010,
    PID_ChangingIntegrationRate = 0b000100,
    PID_Derivative_DerivativeFilter = 0b001000,

}PID_Improvement_e;


typedef struct 
{
    float Kp;
    float Ki;
    float Kd;
    float KF;
    float MaxOut;
    float DeadLimit;
    

    float Last_Measure;
    float Measure;
    float Ref;
    float Error;
    float Last_Error;

    PID_Improvement_e Improve;
    float IntegralLimit;
    float Output_LPF_RC;
    float Derivative_LPF_RC;
    float CoefA;
    float CoefB;


    float Pout;
    float Iout;
    float ITerm;
    float Dout;

    float Last_Pout;
    float Last_Iout;
    float Last_Dout;
    float Last_ITerm;

    float Last_Out;
    float Output;

    float dt;
}PIDInstance;


typedef struct 
{
    float Kp;
    float Ki;
    float Kd;
    float KF;
    float MaxOut;
    float DeadBand;
    float IntegralLimit;
    PID_Improvement_e Improve;
    float Output_LPF_RC;
    float Derivative_LPF_RC;
    float CoefA;
    float CoefB;

}PID_Init_Config_s;

PIDInstance *PID_Init(PIDInstance *pid, PID_Init_Config_s *config);
float PIDCalculate(PIDInstance *pid, float measure, float ref);
PIDInstance **Getpid();

#endif 
