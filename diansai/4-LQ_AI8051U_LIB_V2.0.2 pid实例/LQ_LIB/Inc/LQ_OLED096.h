/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
 【平    台】北京龙邱智能科技STC32位核心板
 【编    写】龙邱科技
 【E-mail  】chiusir@163.com
 【软件版本】V1.0 版权所有，单位使用请先联系授权
 【相关信息参考下列地址】
 【网    站】http://www.lqist.cn
 【淘宝店铺】http://longqiu.taobao.com
 --------------------------------------------------------------------------------
 【  IDE  】 keil C251 V5.60
 【Target 】 STC32G/STC8051U/AI8051U 32位模式
 【SYS CLK】 42 MHz使用内部晶振
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/

#ifndef _LQOLED_H
#define _LQOLED_H

// OLED 引脚定义
#define OLED_CK    P32     //OLED CK管脚
#define OLED_DI	   P34     //OLED DI管脚
#define OLED_RST   P21     //OLED RST管脚
#define OLED_DC	   P25     //OLED DC管脚
#define OLED_CS	   P26     //OLED CS管脚 默认拉低，可以不用

// 同步以上管脚，用于初始化,注意上下必须一致
#define OLED_CLK_PIN  P3_2
#define OLED_DI_PIN   P3_4
#define OLED_RST_PIN  P2_1
#define OLED_DC_PIN   P2_5
#define OLED_CS_PIN   P2_6

//------------------------------------------------------------------------------

#define OLED_DC_H  OLED_DC = 1;
#define OLED_DC_L  OLED_DC = 0;

#define OLED_RST_H OLED_RST = 1;
#define OLED_RST_L OLED_RST = 0;

#define OLED_DI_H  OLED_DI = 1;
#define OLED_DI_L  OLED_DI = 0;

#define OLED_CK_H  OLED_CK = 1;
#define OLED_CK_L  OLED_CK = 0;

#define OLED_CS_H  OLED_CS = 1;
#define OLED_CS_L  OLED_CS = 0;

#define X_WIDTH 132
#define Y_WIDTH 64


void OLED_Init(void);
void OLED_Fill(void);
void OLED_CLS(void);
void OLED_PutPixel(unsigned char x,unsigned char y);
void OLED_ClrDot(unsigned char x,unsigned char y);
void OLED_Rectangle(unsigned char x1,unsigned char y1,unsigned char x2,unsigned char y2,unsigned char gif);
void OLED_P6x8Str(unsigned char x,unsigned char y, char ch[]);
void OLED_P8x16Str(unsigned char x,unsigned char y, char ch[]);
void OLED_P14x16Str(unsigned char x,unsigned char y,char ch[]);
void OLED_Show_BMP(unsigned char x0,unsigned char y0,unsigned char wide,unsigned char high,const unsigned char * bmp);
void OLED_Show_LQLogo(void);
void OLED_Show_Frame80(void);
void OLED_Show_Frame94(void);
void OLED_Road(unsigned short high, unsigned short wide, unsigned char *Pixle);
void OLED_WrCmd(unsigned char cmd);
void OLED_Set_Pos(unsigned char x, unsigned char y);
void OLED_WrDat(unsigned char dat);
void OLED_Set_Pos(unsigned char x, unsigned char y);


#endif


