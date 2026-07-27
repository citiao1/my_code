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

void motor_init(void);
void motor_add_30(void);
uint8_t motor_set_angles(int16_t motor_1_angle_10, int16_t motor_2_angle_10);
void motor_pro(void);

#endif
