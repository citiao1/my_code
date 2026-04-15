#ifndef __FLASH_BOOT_H__
#define __FLASH_BOOT_H__


#include "flash_config.h"
/*
bulid time:Wed Dec 19 15:43:34 2018
*/

//flash boot uart tx only
//BOOT_UART_OPEN,open:0x55,close:0xff
#define BOOT_UART_OPEN     0x55

//BOOT_UART_TX_PIN,only use GPIOA13 or GPIOB7
#define BOOT_UART_TX_A13   0x00
#define BOOT_UART_TX_B7    0x01
#define BOOT_UART_TX_PIN    BOOT_UART_TX_A13

#if FLASH_BOOT_EN
extern const unsigned char flash_data[];
#endif


#endif

