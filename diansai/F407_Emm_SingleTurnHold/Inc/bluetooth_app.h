#ifndef __BLUETOOTH_APP_H
#define __BLUETOOTH_APP_H

#include "main.h"

extern volatile uint32_t bluetooth_rx_count;
extern volatile uint8_t bluetooth_last_byte;
extern volatile uint32_t bluetooth_error_count;

void bluetooth_init(void);
void bluetooth_pro(void);
void bluetooth_uart_error(void);

#endif
