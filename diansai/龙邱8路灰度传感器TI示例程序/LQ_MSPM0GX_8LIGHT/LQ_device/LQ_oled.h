#ifndef _LQ_OLED_H
#define _LQ_OLED_H

#include "include.h"

extern char txt[32];

//-----------------OLED端口定义-----------------

#define OLED_SCL_Clr() 	DL_GPIO_clearPins(OLED_OLED_CK_PORT,OLED_OLED_CK_PIN)//CK
#define OLED_SCL_Set() 	DL_GPIO_setPins(OLED_OLED_CK_PORT,OLED_OLED_CK_PIN)

#define OLED_SDA_Clr() 	DL_GPIO_clearPins(OLED_OLED_DI_PORT,OLED_OLED_DI_PIN)//DI
#define OLED_SDA_Set() 	DL_GPIO_setPins(OLED_OLED_DI_PORT,OLED_OLED_DI_PIN)

#define OLED_RES_Clr()  DL_GPIO_clearPins(OLED_OLED_RST_PORT,OLED_OLED_RST_PIN)//RST
#define OLED_RES_Set()  DL_GPIO_setPins(OLED_OLED_RST_PORT,OLED_OLED_RST_PIN)

#define OLED_DC_Clr()   DL_GPIO_clearPins(OLED_OLED_DC_PORT,OLED_OLED_DC_PIN)//DC
#define OLED_DC_Set()   DL_GPIO_setPins(OLED_OLED_DC_PORT,OLED_OLED_DC_PIN)

#define OLED_CS_Clr()   DL_GPIO_clearPins(OLED_OLED_CS_PORT,OLED_OLED_CS_PIN)//CS
#define OLED_CS_Set()   DL_GPIO_setPins(OLED_OLED_CS_PORT,OLED_OLED_CS_PIN)


#define OLED_CMD  0	//写命令
#define OLED_DATA 1	//写数据



void OLED_WR_Byte(unsigned char dat,unsigned char cmd);
	
extern unsigned char OLED_GRAM[144][8];

void OLED_ColorTurn(unsigned char i);
void OLED_DisplayTurn(unsigned char i);
void OLED_DisPlay_On(void);
void OLED_DisPlay_Off(void);
void OLED_Refresh(void);
void OLED_Clear(void);
void OLED_DrawPoint(unsigned char x,unsigned char y,unsigned char t);
void OLED_DrawLine(unsigned char x1,unsigned char y1,unsigned char x2,unsigned char y2,unsigned char mode);
void OLED_DrawCircle(unsigned char x,unsigned char y,unsigned char r);
void OLED_ShowChar(unsigned char x,unsigned char y,unsigned char chr,unsigned char size1,unsigned char mode);
void OLED_ShowString(unsigned char y,unsigned char x,unsigned char *chr,unsigned char size1);
unsigned int OLED_Pow(unsigned char m,unsigned char n);
void OLED_ShowNum(unsigned char x,unsigned char y,unsigned int num,unsigned char len,unsigned char size1,unsigned char mode);
void OLED_ShowChinese(unsigned char x,unsigned char y,unsigned char num,unsigned char mode);
void OLED_ScrollDisplay(unsigned char num,unsigned char space,unsigned char mode);
void OLED_ShowPicture(unsigned char x,unsigned char y,unsigned char sizex,unsigned char sizey,unsigned char BMP[],unsigned char mode);
void OLED_Init(void);

void LQ_Test_OLED(void);

#endif
