#ifndef PID_H
#define PID_H

typedef struct{
    float Kp,Ki,Kd;
    float error,last_error,integral;
    float out_max;     //输出限幅
    float integral_max;//积分限幅
}PID_t;

void PID_Init(PID_t *pid,float Kp,float Ki,float Kd,float out_max, float integral_max);
float PID_Compute(PID_t *pid,float target,float measure);
void PID_Clear(PID_t *pid);
#endif