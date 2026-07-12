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
#ifndef __LQ_IPSLCD_H__
#define __LQ_IPSLCD_H__

#include "include.h"

//使用软件SPI或还是硬件SPI
#define Use_HARDSPI  0   //置1使用硬件SPI，置0使用软件模拟SPI

//龙邱IPS 模块管脚定义,此定义为软件SPI引脚
#define IPS_CK        P32       // SCLK管脚
#define IPS_DI        P34       // MOSI管脚
#define IPS_RST       P21       // RST管脚
#define IPS_DC        P25       // D/C管脚
#define IPS_CS        P26       // CS管脚

//管脚操作宏定义

#define IPS_DC_H        GPIO_Set_Bit(IPS_DC, HIGH);
#define IPS_DC_L        GPIO_Set_Bit(IPS_DC, LOW);
#define IPS_RST_H       GPIO_Set_Bit(IPS_RST, HIGH);
#define IPS_RST_L       GPIO_Set_Bit(IPS_RST, LOW);
#define IPS_DI_H        GPIO_Set_Bit(IPS_DI, HIGH);
#define IPS_DI_L        GPIO_Set_Bit(IPS_DI, LOW);
#define IPS_CK_H        GPIO_Set_Bit(IPS_CK, HIGH);
#define IPS_CK_L        GPIO_Set_Bit(IPS_CK, LOW);
#define IPS_CS_H        GPIO_Set_Bit(IPS_CS, HIGH);
#define IPS_CS_L        GPIO_Set_Bit(IPS_CS, LOW);
//定义显示图像的大小
#define LCDH 60
#define LCDW 94


/*屏幕型号选择，四选一，使用哪款屏幕则取消哪款屏幕的注释*/
#define IPS114
//#define IPS130
//#define IPS154
//#define IPS200
/*
USE_HORIZONTAL 0:  竖屏，排针在屏幕的左侧
USE_HORIZONTAL 1:  竖屏，排针在屏幕的右侧
USE_HORIZONTAL 2:  横屏，排针在屏幕的下侧
USE_HORIZONTAL 3:  横屏，排针在屏幕的上侧
*/
#define USE_HORIZONTAL 3  //设置横屏或者竖屏显示 0或1为竖屏 2或3为横屏

#ifdef IPS114
#if USE_HORIZONTAL==0||USE_HORIZONTAL==1
#define LCD_W 135
#define LCD_H 240
#else
#define LCD_W 240
#define LCD_H 135
#endif
#endif

#ifdef IPS130
#define LCD_W 240
#define LCD_H 240
#endif

#ifdef IPS154
#define LCD_W 240
#define LCD_H 240
#endif

#ifdef IPS200
#if USE_HORIZONTAL==0||USE_HORIZONTAL==1
#define LCD_W 240
#define LCD_H 320
#else
#define LCD_W 320
#define LCD_H 240
#endif
#endif


//画笔颜色
#define u16_WHITE            0xFFFF //白色
#define u16_BLACK            0x0000 //黑色
#define u16_BLUE             0x001F //蓝色
#define u16_BRED             0XF81F //
#define u16_GRED             0XFFE0 //
#define u16_GBLUE            0X07FF //
#define u16_RED_IPS          0xF800 //红色
#define u16_MAGENTA          0xF81F //品红
#define u16_GREEN            0x07E0 //绿色
#define u16_CYAN             0x7FFF //蓝绿色
#define u16_YELLOW           0xFFE0 //黄色
#define u16_BROWN            0XBC40 //棕色
#define u16_BRRED            0XFC07 //棕红色
#define u16_GRAY             0X8430 //灰色
#define u16_DARKBLUE         0X01CF //深蓝色
#define u16_LIGHTBLUE        0X7D7C //浅蓝色
#define u16_GRAYBLUE         0X5458 //灰蓝色
#define u16_LIGHTGREEN       0X841F //浅绿色
#define u16_LGRAY            0XC618 //浅灰色(PANNEL),窗体背景色
#define u16_LGRAYBLUE        0XA651 //浅灰蓝色(中间层颜色)
#define u16_LBBLUE           0X2B12 //浅棕蓝色(选择条目的反色)

void LCD_Write_Cmd(unsigned char cmd);/*写入命令*/
void LCD_Write_Byte(unsigned char dat);/*写入8位数据*/
void LCD_Write_Word(unsigned int dat);/*写入16位数据*/
void LCD_Address_Set(unsigned int x1,unsigned int y1,unsigned int x2,unsigned int y2);/*设置起始和结束地址*/

void LCD_DrawPoint(unsigned int x,unsigned int y,unsigned int color);/*在指定位置画点*/
/*指定位置填充指定颜色*/
void LCD_Fill(unsigned int xsta,unsigned int ysta,unsigned int xend,unsigned int yend,unsigned int color);
/*清屏*/
void LCD_CLS(unsigned int color);
/*画线*/
void LCD_Draw_Line(unsigned char xs,unsigned char ys,unsigned char xe,unsigned char ye,unsigned short color_dat);
/*画矩形框*/
void LCD_Draw_Rectangle(unsigned char xs,unsigned char ys,unsigned char xe,unsigned char ye,unsigned short color_dat);
/*画圆*/
void LCD_Draw_Circle(unsigned char x,unsigned char y,unsigned char r,unsigned short color_dat);
/*显示图片*/
void LCD_ShowPicture(unsigned int x, unsigned int y, unsigned int length, unsigned int width,const unsigned char pic[]);

/*显示单个字符*/
void LCD_ShowChar(unsigned int x,unsigned int y,unsigned char num,unsigned int fc,unsigned int bc,unsigned char sizey,unsigned char mode);
/*显示字符串*/
void LCD_ShowString(unsigned int x,unsigned int y,char *p,unsigned int fc,unsigned int bc,unsigned char sizey,unsigned char mode);
/*显示12*6大小的字符串*/
//void LCD_ShowStr_12(unsigned int x,unsigned int y, char *p,unsigned int fc,unsigned int bc);
/*显示16*8大小的字符串*/
void LCD_ShowStr_16(unsigned int x,unsigned int y, char *p,unsigned int fc,unsigned int bc);
/*显示24*12大小的字符串*/
void LCD_ShowStr_24(unsigned int x,unsigned int y, char *p,unsigned int fc,unsigned int bc);
/*显示32*16大小的字符串*/
//void LCD_ShowStr_32(unsigned int x,unsigned int y, char *p,unsigned int fc,unsigned int bc);
/*显示汉字*/
void LCD_ShowChinese(unsigned int x,unsigned int y,char *s,unsigned int fc,unsigned int bc,unsigned char sizey);


void LCDSPI_Road_RGB565(unsigned char wide_start, unsigned char high_start, unsigned char high, unsigned char wide, uint16_t *Pixle);
void LCDSPI_Road( unsigned char wide_start, unsigned char high_start,unsigned char high, unsigned char wide, unsigned char *Pixle);
void LCDSPI_BinRoad( unsigned char wide_start, unsigned char high_start,unsigned char high, unsigned char wide, unsigned char *Pixle);

void IPS_LCD_Init(void);  /*LQ IPS 屏幕初始化*/



#endif /*__LQ_IPSLCD_H__*/
