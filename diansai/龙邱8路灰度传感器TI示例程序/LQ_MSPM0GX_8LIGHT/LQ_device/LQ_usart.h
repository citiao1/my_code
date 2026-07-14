#ifndef _LQ_USART_H
#define _LQ_USART_H

#include "include.h"

extern volatile unsigned char uart_data;

void uart_init();
void uart0_send_char(char ch);
void uart0_send_string(char* str);


#endif
