#ifndef LQ_HARD_SPI_H_
#define LQ_HARD_SPI_H_

#include "lq_include.h"

void LQ_QSPI_Init(IfxQspi_Sclk_Out sck_pin, IfxQspi_Mtsr_Out mosi_pin, IfxQspi_Mrst_In miso_pin, IfxQspi_Slso_Out cs_pin, uint8 mode, uint32 baud);
void LQ_QSPI_Wirte_Read(uint8 *modata, uint8 *midata, uint32 len, uint8 continuous);

#endif /* LQ_HARD_SPI_H_ */