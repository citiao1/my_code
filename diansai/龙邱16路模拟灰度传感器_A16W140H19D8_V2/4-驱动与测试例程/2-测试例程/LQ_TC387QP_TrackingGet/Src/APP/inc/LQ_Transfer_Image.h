#ifndef _LQ_TRANSFER_IMAGE_H_
#define _LQ_TRANSFER_IMAGE_H_
#include "lq_include.h"

#define TR_CS P00_9
#define TR_CLK   QSPI4_CLK_P33_11
#define TR_MISO  QSPI4_MISO_P33_13
#define TR_MOSI  QSPI4_MOSI_P33_12
#define IO2 P00_7

#define TR_IMG_W 188
#define TR_IMG_H 120

#define TR_CS_H PIN_Write(TR_CS, 1)
#define TR_CS_L PIN_Write(TR_CS, 0)

#define TR_IO2 PIN_Read(IO2)

extern unsigned char FH[4];
extern unsigned char FE[4];

void TR_driver_init(void);
void IR_Write_byte_4000(unsigned char *dat);
void IR_Wirte_byte(unsigned char *dat, uint16_t len);

void TR_Write_Image(unsigned char high, unsigned char wide, unsigned char *dat);
void TR_Write_Image_Pixle(unsigned char height, unsigned char width, unsigned char *Pixle);
void Test_CAMERA_TR(void);

#endif
