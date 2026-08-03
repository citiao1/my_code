#ifndef __MOTOR_UART_H
#define __MOTOR_UART_H

#include "main.h"

extern float uart_angle[2];
extern uint8_t uart_position_ready[2];
extern volatile uint32_t uart_error_count;

/* 仅供 motor_app.c 使用，业务代码不需要调用。 */
void motor_uart_init_internal(void);
void motor_uart_process_internal(void);

#endif
