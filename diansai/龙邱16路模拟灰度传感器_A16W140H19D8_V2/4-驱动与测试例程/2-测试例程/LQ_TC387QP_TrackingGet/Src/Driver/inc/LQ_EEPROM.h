#ifndef __LQ_EEPROM_H__
#define __LQ_EEPROM_H__

#include "lq_include.h"

void EEPROM_EraseSector(unsigned char sector);
void EEPROM_Write(unsigned char sector, unsigned short page, unsigned long *buff, unsigned short len);
void EEPROM_Read(unsigned char sector, unsigned short page, unsigned long *rbuff, unsigned short len);

#endif /* __LQ_EEPROM_H__ */
