#ifndef PID_H
#define PID_H

#include <stdint.h>

typedef struct
{
  float kp;
  float ki;
  float kd;
  float output;
  float previous_error;
  float previous_previous_error;
  float output_limit;
} PidController;

void Pid_Init(PidController *pid, float kp, float ki, float kd,
              float output_limit, float integral_limit);
void Pid_SetGains(PidController *pid, float kp, float ki, float kd);
void Pid_SetLimits(PidController *pid, float output_limit, float integral_limit);
void Pid_Reset(PidController *pid);
float Pid_Update(PidController *pid, float target, float feedback, float dt_seconds);

#endif
