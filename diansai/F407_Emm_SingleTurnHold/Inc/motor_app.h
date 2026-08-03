#ifndef __MOTOR_APP_H
#define __MOTOR_APP_H

#include "main.h"

extern float motor_angle[2];
extern float motor_target_angle[2];
extern uint8_t motor_position_valid[2];
extern uint8_t motor_state;
extern uint8_t motor_pending_steps;
extern uint32_t motor_cmd_count;
extern uint32_t motor_correction_count;
extern uint32_t motor_error_count;

/*
 * 最简用法：
 *   EMM_Init();
 *   while (1) {
 *     EMM_Run();
 *   }
 *
 * 需要改变目标时只调用：EMM_SetAngle(90.0f, 120.0f);
 */
void EMM_Init(void);
void EMM_Run(void);
uint8_t EMM_SetAngle(float motor_1_degree, float motor_2_degree);
uint8_t EMM_IsOnTarget(void);

/* 板载按键使用的相对 +30 度请求。 */
void motor_add_30(void);

#endif
