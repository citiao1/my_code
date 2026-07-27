#ifndef __MOTOR_UART_H
#define __MOTOR_UART_H

#include "main.h"

extern float uart_angle[2];
extern uint8_t uart_position_ready[2];
extern volatile uint32_t uart_error_count;

void uart_init(void);
void uart_pro(void);

#endif
