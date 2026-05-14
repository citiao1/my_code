#ifndef PID_H
#define PID_H

typedef struct{
    float Kp,Ki,Kd;
    float error,last,integral;
}PID_t;

void PID_Init(PID_t *pid,float Kp,float Ki,float Kd);
float PID_Compute(PID_t *pid,float target,float measure);

#endif