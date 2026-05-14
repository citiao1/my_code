#ifndef __UART_H
#define __UART_H
void Uart_Tx(unsigned char *data,unsigned short int leng);
void Uart_Rx(unsigned char *data,unsigned short int leng);
void Rx_Process(void);
#endif
