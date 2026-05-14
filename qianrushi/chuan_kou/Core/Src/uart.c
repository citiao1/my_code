#include "gpio.h"
#include "uart.h"
#include "usart.h"
#include "string.h"
extern char Rx_flg;
extern char Rx_Buff;
extern UART_HandleTypeDef huart1;
unsigned char flag[10]="hello";
unsigned char flag1[10]="nohello";
void Uart_Tx(unsigned char *data,unsigned short int leng){
	HAL_UART_Transmit(&huart1,data,leng,100);
}
void Uart_Rx(unsigned char *data,unsigned short int leng){
	HAL_UART_Receive(&huart1,data,leng,100);
}
