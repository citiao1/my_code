// 支持多组PID参数
#ifndef __PID_H_
#define __PID_H_

#include "DEF.h"

typedef enum {
	PID_POSITION = 0,
	PID_INCREMENT = 1,
} PID_Mode;

typedef enum {
	PID_LEFT = 0,
	PID_RIGHT = 1,
	PID_DIRECTION = 2,
	PID_YAW_RATE = 3
} PID_Index;

typedef struct
{
	float Kp;
	float Ki;
	float Kd;
	float prev_error;
	float prev_prev_error;
	float integral;
	float output_max;
	float output_min;
	float output;
	PID_Mode mode;
} PID_TypeDef;

void PID_AllInit(void);
void PID_Init(PID_TypeDef *pid, float Kp, float Ki, float Kd, float out_max, float out_min,  PID_Mode mode);

// 串级控制专用函数（高性能版本）
int16 PID_CascadePosition(PID_TypeDef *pid, int16 current, int16 target);
int16 PID_CascadeIncrement(PID_TypeDef *pid, int16 current, int16 target);

// 通过索引调用PID计算函数
int16 PID_Calculate_ByIndex(PID_Index index, int16 current, int16 target);

// 获取特定PID控制器的只读指针
const PID_TypeDef* PID_GetController(PID_Index index);

// 向后兼容的通用PID计算函数
int16 PID_Calculate(PID_TypeDef *pid, int16 current, int16 target);
void PID_SetKp(PID_Index index, float Kp);
void PID_SetKi(PID_Index index, float Ki);
void PID_SetKd(PID_Index index, float Kd);
#endif