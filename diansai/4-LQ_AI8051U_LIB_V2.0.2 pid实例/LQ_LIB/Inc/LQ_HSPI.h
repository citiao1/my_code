#ifndef  __LQ_HSPI_H
#define  __LQ_HSPI_H
#include "config.h"

void HSPI_Init(void);

void SPI_Write_NByte(unsigned char addr, unsigned char* buffer, unsigned char sizel);
void SPI_Read_NByte(unsigned char addr, unsigned char* buffer, unsigned int sizel);

#endif



