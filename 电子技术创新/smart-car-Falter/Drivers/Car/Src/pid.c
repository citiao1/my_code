#include "pid.h"

void PID_Init(PID_t *pid,float Kp,float Ki,float Kd)
{
    pid->Kp=Kp; pid->Ki=Ki; pid->Kd=Kd;
    pid->error=pid->last=pid->integral=0;
}

float PID_Compute(PID_t *pid,float target,float measure)
{
    pid->error = target-measure;
    pid->integral += pid->error;

    float out = pid->Kp*pid->error
              + pid->Ki*pid->integral
              + pid->Kd*(pid->error-pid->last);

    pid->last = pid->error;
    return out;
}