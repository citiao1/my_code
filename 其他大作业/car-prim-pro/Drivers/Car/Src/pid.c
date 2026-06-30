#include "pid.h"

void PID_Init(PID_t *pid,float Kp,float Ki,float Kd,float out_max, float integral_max)
{
    pid->Kp=Kp; 
    pid->Ki=Ki; 
    pid->Kd=Kd;
		pid->out_max = out_max;
    pid->integral_max = integral_max;
    pid->error=pid->last_error=pid->integral=0;
}

float PID_Compute(PID_t *pid,float target,float measure)
{
    pid->error = target-measure;
    pid->integral += pid->error;

    if (pid->integral > pid->integral_max) {
        pid->integral = pid->integral_max;
    } else if (pid->integral < -pid->integral_max) {
        pid->integral = -pid->integral_max;
    }
    
    float out = pid->Kp*pid->error
              + pid->Ki*pid->integral
              + pid->Kd*(pid->error-pid->last_error);

    pid->last_error = pid->error;

    if (out > pid->out_max) {
        out = pid->out_max;
    } else if (out < -pid->out_max) {
        out = -pid->out_max;
    }

    return out;
}
void PID_Clear(PID_t *pid) {
    pid->error = pid->last_error = pid->integral = 0;
}