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

#include "LQ_IPSLCD.h"
#include "include.h"
#include "font.h"


/*!
  * @brief    IPS1.14初始化
  * @return   无
  * @note     如果修改管脚 需要修改初始化的管脚
  * @see      IPS_LCD_Init();
  */
void IPS_LCD_Init(void)
{
#if Use_HARDSPI
//    SPIM_Init(SPI2,baud_72M,SPI2_0_SCK_PB13,SPI_MISO_NULL,SPI2_0_MOSI_PB15,PA8);
#else   //软件SPI管脚初始化
    GPIO_Init(GPIO_P3,GPIO_Pin_2|GPIO_Pin_4,GPIO_Mode_Out_PP);            //SCLK,MOSI 推挽输出
    GPIO_Init(GPIO_P2,GPIO_Pin_1|GPIO_Pin_5|GPIO_Pin_6,GPIO_Mode_Out_PP); //RST,DC,CS 推挽输出
#endif
    /*复位*/
    IPS_RST_L;
    delay_ms(100);
    IPS_RST_H;
    delay_ms(100);

    IPS_CS_L;
    delay_ms(150);
    LCD_Write_Cmd(0x11);
    delay_ms(120);
    LCD_Write_Cmd(0x36);
    if(USE_HORIZONTAL==0)LCD_Write_Byte(0x00);
    else if(USE_HORIZONTAL==1)LCD_Write_Byte(0xC0);
    else if(USE_HORIZONTAL==2)LCD_Write_Byte(0x70);
    else LCD_Write_Byte(0xA0);

    LCD_Write_Cmd(0x3A);
    LCD_Write_Byte(0x05);
    LCD_Write_Cmd(0xB2);
    LCD_Write_Byte(0x0C);
    LCD_Write_Byte(0x0C);
    LCD_Write_Byte(0x00);
    LCD_Write_Byte(0x33);
    LCD_Write_Byte(0x33);
    LCD_Write_Cmd(0xB7);
    LCD_Write_Byte(0x35);
    LCD_Write_Cmd(0xBB);
    LCD_Write_Byte(0x19);
    LCD_Write_Cmd(0xC0);
    LCD_Write_Byte(0x2C);
    LCD_Write_Cmd(0xC2);
    LCD_Write_Byte(0x01);
    LCD_Write_Cmd(0xC3);
    LCD_Write_Byte(0x12);
    LCD_Write_Cmd(0xC4);
    LCD_Write_Byte(0x20);
    LCD_Write_Cmd(0xC6);
    LCD_Write_Byte(0x0F);
    LCD_Write_Cmd(0xD0);
    LCD_Write_Byte(0xA4);
    LCD_Write_Byte(0xA1);
    LCD_Write_Cmd(0xE0);
    LCD_Write_Byte(0xD0);
    LCD_Write_Byte(0x04);
    LCD_Write_Byte(0x0D);
    LCD_Write_Byte(0x11);
    LCD_Write_Byte(0x13);
    LCD_Write_Byte(0x2B);
    LCD_Write_Byte(0x3F);
    LCD_Write_Byte(0x54);
    LCD_Write_Byte(0x4C);
    LCD_Write_Byte(0x18);
    LCD_Write_Byte(0x0D);
    LCD_Write_Byte(0x0B);
    LCD_Write_Byte(0x1F);
    LCD_Write_Byte(0x23);
    LCD_Write_Cmd(0xE1);
    LCD_Write_Byte(0xD0);
    LCD_Write_Byte(0x04);
    LCD_Write_Byte(0x0C);
    LCD_Write_Byte(0x11);
    LCD_Write_Byte(0x13);
    LCD_Write_Byte(0x2C);
    LCD_Write_Byte(0x3F);
    LCD_Write_Byte(0x44);
    LCD_Write_Byte(0x51);
    LCD_Write_Byte(0x2F);
    LCD_Write_Byte(0x1F);
    LCD_Write_Byte(0x1F);
    LCD_Write_Byte(0x20);
    LCD_Write_Byte(0x23);
    LCD_Write_Cmd(0x21);
    LCD_Write_Cmd(0x29);
    delay_ms(20);
}

/*!
  * @brief    简单延时函数
  *
  * @param    Del ：延时时间
  *
  * @return   无
  *
  * @note     内部调用
  *
  * @see      IPSdelay_1us(1);
  *
  */
void IPSdelay_1us(unsigned int Del)     //
{
    delay_us(Del);
//  while(Del--)
//  {
//    __asm("NOP");
//  }
}

/*!
  * @brief    写命令
  * @param    cmd ：命令
  * @return   无
  * @note     内部调用
  * @see      LCD_Write_Cmd(0xb7); //LCD Driveing control
  */
void LCD_Write_Cmd(unsigned char cmd)
{
    unsigned char i;
    IPS_DC_L;// A0=0 ILI9163_A0=0;     //A0=0发送命令
#if Use_HARDSPI
    SPI_Write_8Bit(SPI2,cmd);
#else

    for(i=0;i<8;i++)
    {
        IPS_CK_L;                   // SCK=0
        if(cmd&0x80)
            {
                IPS_DI_H;               // SDI=1
            }else{
                IPS_DI_L;               // SDI=0
            }
        IPS_CK_H;                   // SCK=1
        cmd <<= 1;
    }
#endif
    IPS_DC_H;
}

/*!
  * @brief    写字节
  * @param    dat ：数据
  * @return   无
  * @note     内部调用
  * @see      LCD_Write_Byte(0x00); //CRL=0
  */
void LCD_Write_Byte(unsigned char dat)
{
    unsigned char i;
    IPS_DC_H;// A0=1  ILI9163_A0=1;         //A0=1发送数据
#if Use_HARDSPI
    SPI_Write_8Bit(SPI2,dat);
#else

    for(i=0;i<8;i++)
    {
        IPS_CK_L;   // SCK=0  ILI9163_SCK=0;
        if(dat&0x80){IPS_DI_H;}// SDI=1
        else{IPS_DI_L;}// SDI=0
        IPS_CK_H;   // SCK=1 ILI9163_SCK = 1;
        dat = (dat<<1);
    }
#endif
}

/*!
  * @brief    写半字
  * @param    dat ：数据
  * @return   无
  * @note     无
  * @see      LCD_Write_Word(0xFFFF);
  */
void LCD_Write_Word(unsigned int dat)
{
    unsigned char i;
    IPS_DC_H;// A0=1  ILI9163_A0=1;
#if Use_HARDSPI
    SPI_Write_16Bit(SPI2,dat);
#else

    for(i=0;i<16;i++)
    {
        IPS_CK_L;   // SCK=0  ILI9163_SCK=0;
        if(dat&0x8000){ IPS_DI_H;}// SDI=1
        else{IPS_DI_L;}// SDI=0
        IPS_CK_H;   // SCK=1  ILI9163_SCK=1;
        dat<<=1;
    }
#endif

}

/******************************************************************************
      函数说明：设置起始和结束地址
      入口数据：x1,x2 设置列的起始和结束地址
             y1,y2 设置行的起始和结束地址
      返回值：  无
******************************************************************************/
void LCD_Address_Set(unsigned int x1,unsigned int y1,unsigned int x2,unsigned int y2)
{
    int W_offset = 0, H_offset = 0;
    if(USE_HORIZONTAL==0)
    {
#ifdef IPS114
        W_offset = 52; H_offset = 40;
#endif
#ifdef IPS130
        W_offset = 0; H_offset = 0;
#endif
#ifdef IPS154
        W_offset = 0; H_offset = 0;
#endif
#ifdef IPS200
        W_offset = 0; H_offset = 0;
#endif
    }
    else if(USE_HORIZONTAL==1)
    {
#ifdef IPS114
        W_offset = 53; H_offset = 40;
#endif
#ifdef IPS130
        W_offset = 0; H_offset = 80;
#endif
#ifdef IPS154
        W_offset = 0; H_offset = 80;
#endif
#ifdef IPS200
        W_offset = 0; H_offset = 0;
#endif
    }
    else if(USE_HORIZONTAL==2)
    {
#ifdef IPS114
        W_offset = 40; H_offset = 53;
#endif
#ifdef IPS130
        W_offset = 0; H_offset = 0;
#endif
#ifdef IPS154
        W_offset = 0; H_offset = 0;
#endif
#ifdef IPS200
        W_offset = 0; H_offset = 0;
#endif
    }
    else
    {
#ifdef IPS114
        W_offset = 40; H_offset = 52;
#endif
#ifdef IPS130
        W_offset = 80; H_offset = 0;
#endif
#ifdef IPS154
        W_offset = 80; H_offset = 0;
#endif
#ifdef IPS200
        W_offset = 0; H_offset = 0;
#endif
    }
    LCD_Write_Cmd(0x2a);//列地址设置
    LCD_Write_Word(x1+W_offset);
    LCD_Write_Word(x2+W_offset);
    LCD_Write_Cmd(0x2b);//行地址设置
    LCD_Write_Word(y1+H_offset);
    LCD_Write_Word(y2+H_offset);
    LCD_Write_Cmd(0x2c);//储存器写
}

/******************************************************************************
      函数说明：在指定位置画点
      入口数据：x,y 画点坐标
                color 点的颜色
      返回值：  无
******************************************************************************/
void LCD_DrawPoint(unsigned int x,unsigned int y,unsigned int color)
{
    LCD_Address_Set(x,y,x,y);//设置光标位置
    LCD_Write_Word(color);
}

/******************************************************************************
      函数说明：显示单个字符
      入口数据：x,y显示坐标
                num 要显示的字符
                fc 字的颜色
                bc 字的背景色
                sizey 字号：可选12 16 24 32
                mode:  0非叠加模式  1叠加模式
      返回值：  无
******************************************************************************/
void LCD_ShowChar(unsigned int x,unsigned int y,unsigned char num,unsigned int fc,unsigned int bc,unsigned char sizey,unsigned char mode)
{
    unsigned char temp,sizex,t,m=0;
    unsigned int i,TypefaceNum;//一个字符所占字节大小
    unsigned int x0=x;
    sizex=sizey/2;
    TypefaceNum=(sizex/8+((sizex%8)?1:0))*sizey;
    num=num-' ';    //得到偏移后的值
    LCD_Address_Set(x,y,x+sizex-1,y+sizey-1);  //设置光标位置
    for(i=0;i<TypefaceNum;i++)
    {
//        if(sizey==12)temp=ascii_1206[num][i];              //调用6x12字体  太小，字库中删除，想用看可以在添加，但会增加编译后的文件大小
//        else
            if(sizey==16)temp=ascii_1608[num][i];       //调用8x16字体
        else if(sizey==24)temp=ascii_2412[num][i];       //调用12x24字体
//        else if(sizey==32)temp=ascii_3216[num][i];       //调用16x32字体
        else return;
        for(t=0;t<8;t++)
        {
            if(!mode)//非叠加模式
            {
                if(temp&(0x01<<t))LCD_Write_Word(fc);
                else LCD_Write_Word(bc);
                m++;
                if(m%sizex==0)
                {
                    m=0;
                    break;
                }
            }
            else//叠加模式
            {
                if(temp&(0x01<<t))LCD_DrawPoint(x,y,fc);//画一个点
                x++;
                if((x-x0)==sizex)
                {
                    x=x0;
                    y++;
                    break;
                }
            }
        }
    }
}

/******************************************************************************
      函数说明：显示字符串
      入口数据：x,y显示坐标
                *p 要显示的字符串
                fc 字的颜色
                bc 字的背景色
                sizey 字号：可选12 16 24 32
                mode:  0非叠加模式  1叠加模式
      返回值：  无
******************************************************************************/
void LCD_ShowString(unsigned int x,unsigned int y, char *p,unsigned int fc,unsigned int bc,unsigned char sizey,unsigned char mode)
{
    while(*p!='\0')
    {
        LCD_ShowChar(x,y,*p,fc,bc,sizey,mode);
        x+=sizey/2;
        p++;
    }
}

/******************************************************************************
      函数说明：显示12*06大小的字符串
      入口数据：x    IPS114: 横坐标（横屏时：0-39     竖屏时：0-21） y 纵坐标（横屏时：0-10     竖屏时：0-19）
             x  IPS130: 横坐标（横屏时：0-39      竖屏时：0-39） y 纵坐标（横屏时：0-19     竖屏时：0-19）
             x  IPS154: 横坐标（横屏时：0-39      竖屏时：0-39） y 纵坐标（横屏时：0-19     竖屏时：0-19）
             x  IPS200: 横坐标（横屏时：0-52      竖屏时：0-21） y 纵坐标（横屏时：0-19     竖屏时：0-21）
                *p  要显示的字符串
                fc  字的颜色
                bc  字的背景色
      返回值：  无
******************************************************************************/
void LCD_ShowStr_12(unsigned int x,unsigned int y, char *p,unsigned int fc,unsigned int bc)
{
    while(*p!='\0')
    {
        LCD_ShowChar(x*6,y*12,*p,fc,bc,12,0);
        x++;
        p++;
    }
}
/******************************************************************************
      函数说明：显示16*08大小的字符串
      入口数据：x    IPS114: 横坐标（横屏时：0-29     竖屏时：0-15） y 纵坐标（横屏时：0-7     竖屏时：0-14）
             x  IPS130: 横坐标（横屏时：0-29      竖屏时：0-29） y 纵坐标（横屏时：0-14     竖屏时：0-14）
             x  IPS154: 横坐标（横屏时：0-29      竖屏时：0-29） y 纵坐标（横屏时：0-14     竖屏时：0-14）
             x  IPS200: 横坐标（横屏时：0-39      竖屏时：0-21） y 纵坐标（横屏时：0-14     竖屏时：0-21）
      返回值：  无
******************************************************************************/
void LCD_ShowStr_16(unsigned int x,unsigned int y, char *p,unsigned int fc,unsigned int bc)
{
    while(*p!='\0')
    {
        LCD_ShowChar(x*8,y*16,*p,fc,bc,16,0);
        x++;
        p++;
    }
}
/******************************************************************************
      函数说明：显示24*12大小的字符串
      入口数据：x    IPS114: 横坐标（横屏时：0-19     竖屏时：0-10） y 纵坐标（横屏时：0-4     竖屏时：0-9）
             x  IPS130: 横坐标（横屏时：0-19      竖屏时：0-21） y 纵坐标（横屏时：0-9     竖屏时：0-21）
             x  IPS154: 横坐标（横屏时：0-39      竖屏时：0-21） y 纵坐标（横屏时：0-19     竖屏时：0-21）
             x  IPS200: 横坐标（横屏时：0-25      竖屏时：0-21） y 纵坐标（横屏时：0-9     竖屏时：0-21）
                *p  要显示的字符串
                fc  字的颜色
                bc  字的背景色
      返回值：  无
******************************************************************************/
void LCD_ShowStr_24(unsigned int x,unsigned int y, char *p,unsigned int fc,unsigned int bc)
{
    while(*p!='\0')
    {
        LCD_ShowChar(x*12,y*24,*p,fc,bc,24,0);
        x++;
        p++;
    }
}
/******************************************************************************
      函数说明：x    IPS114: 横坐标（横屏时：0-14     竖屏时：0-7） y 纵坐标（横屏时：0-3     竖屏时：0-6）
             x  IPS130: 横坐标（横屏时：0-14      竖屏时：0-21） y 纵坐标（横屏时：0-6     竖屏时：0-21）
             x  IPS154: 横坐标（横屏时：0-14      竖屏时：0-21） y 纵坐标（横屏时：0-6     竖屏时：0-21）
             x  IPS200: 横坐标（横屏时：0-19      竖屏时：0-21） y 纵坐标（横屏时：0-6     竖屏时：0-21）
                *p  要显示的字符串
                fc  字的颜色
                bc  字的背景色
      返回值：  无
******************************************************************************/
void LCD_ShowStr_32(unsigned int x,unsigned int y, char *p,unsigned int fc,unsigned int bc)
{
    while(*p!='\0')
    {
        LCD_ShowChar(x*16,y*32,*p,fc,bc,32,0);
        x++;
        p++;
    }
}
/******************************************************************************
      函数说明：在指定区域填充颜色
      入口数据：xsta,ysta   起始坐标
                xend,yend   终止坐标
                                color       要填充的颜色
      返回值：  无
******************************************************************************/
void LCD_Fill(unsigned int xsta,unsigned int ysta,unsigned int xend,unsigned int yend,unsigned int color)
{
    unsigned int i,j;
    LCD_Address_Set(xsta,ysta,xend-1,yend-1);//设置显示范围
    for(i=ysta;i<yend;i++)
    {
        for(j=xsta;j<xend;j++)
        {
            LCD_Write_Word(color);
        }
    }
}
/******************************************************************************
      函数说明：清屏
      入口数据：color       要填充的颜色
      返回值：  无
******************************************************************************/
void LCD_CLS(unsigned int color)
{
    LCD_Fill(0,0,LCD_W,LCD_H,color);
}


/******************************************************************************
      函数说明：画线
      入口数据：xs ：起始x
                                ys ：起始y
                                xe ：结束x IPS114   (横屏x:0-240, 竖屏x:0-135)
                                xe ：结束x IPS130  (横屏x:0-240, 竖屏x:0-240)
                                xe ：结束x IPS154  (横屏x:0-240, 竖屏x:0-240)
                                xe ：结束x IPS200  (横屏x:0-320, 竖屏x:0-240)
                                ys ：结束y IPS114   (横屏y:0-135, 竖屏x:0-240)
                                ys ：结束y IPS130  (横屏y:0-240, 竖屏x:0-240)
                                ys ：结束y IPS154  (横屏y:0-240, 竖屏x:0-240)
                                ys ：结束y IPS200  (横屏y:0-240, 竖屏x:0-320)
                                color_dat ：颜色
      返回值：  无
******************************************************************************/
void LCD_Draw_Line(unsigned char xs,unsigned char ys,unsigned char xe,unsigned char ye,unsigned short color_dat)
{
  int i,ds;
  int dx,dy,inc_x, inc_y;
  int xerr = 0, yerr = 0;                       //初始化变量

  if(xs==xe)                            //如果是画垂直线则只需对竖直坐标计数
  {
    LCD_Address_Set(xs,ys,xe,ye);
    for(i=0;i<(ye-ys+1);i++)
    {
      LCD_Write_Word(color_dat);
    }
  }
  else if(ys==ye)                       //如果是水平线则只需要对水平坐标计数
  {
    LCD_Address_Set(xs,ys,xe,ye);
    for(i=0;i<(xe-xs+1);i++)
    {
      LCD_Write_Word(color_dat);
    }
  }
  else                              //如果是斜线，则重新计算，使用画点函数画出直线
  {
    dx = xe - xs;                       //计算坐标增量
    dy = ye - ys;

    if(dx > 0) inc_x = 1;                   //设置单步方向
    else
    {
      inc_x = -1; dx = -dx;
    }
    if(dy > 0) inc_y = 1;                   //设置单步方向
    else
    {
      inc_y = -1; dy = -dy;
    }

    if(dx > dy) {ds = dx;}                  //选取基本增量坐标轴
    else        {ds = dy;}

    for(i = 0; i <= ds+1; i++)                  //画线输出
    {
      LCD_DrawPoint(xs, ys,color_dat);  //画点
      xerr += dx;
      yerr += dy;
      if(xerr > ds)
      {
        xerr -= ds;
        xs   += inc_x;
      }
      if(yerr > ds)
      {
        yerr -= ds;
        ys   += inc_y;
      }
    }
  }
}

/******************************************************************************
      函数说明：画矩形框
      入口数据：xs ：起始x
                                ys ：起始y
                                xe ：结束x IPS114   (横屏x:0-240, 竖屏x:0-135)
                                xe ：结束x IPS130  (横屏x:0-240, 竖屏x:0-240)
                                xe ：结束x IPS154  (横屏x:0-240, 竖屏x:0-240)
                                xe ：结束x IPS200  (横屏x:0-320, 竖屏x:0-240)
                                ys ：结束y IPS114   (横屏y:0-135, 竖屏x:0-240)
                                ys ：结束y IPS130  (横屏y:0-240, 竖屏x:0-240)
                                ys ：结束y IPS154  (横屏y:0-240, 竖屏x:0-240)
                                ys ：结束y IPS200  (横屏y:0-240, 竖屏x:0-320)
                                color_dat ：颜色
      返回值：  无
******************************************************************************/
void LCD_Draw_Rectangle(unsigned char xs,unsigned char ys,unsigned char xe,unsigned char ye,unsigned short color_dat)
{
  LCD_Draw_Line(xs,ys,xs,ye,color_dat);   //画矩形左边
  LCD_Draw_Line(xe,ys,xe,ye,color_dat);   //画矩形右边
  LCD_Draw_Line(xs,ys,xe,ys,color_dat);   //画矩形上边
  LCD_Draw_Line(xs,ye,xe,ye,color_dat);   //画矩形下边
}

/******************************************************************************
      函数说明：画圆
      入口数据：x ：圆心x IPS114    (横屏x:0-240, 竖屏x:0-135)
                      IPS130  (横屏x:0-240, 竖屏x:0-240)
                      IPS154  (横屏x:0-240, 竖屏x:0-240)
                       IPS200  (横屏x:0-320, 竖屏x:0-240)
            y ：圆心y  IPS114   (横屏y:0-135, 竖屏x:0-240)
                     IPS130  (横屏y:0-240, 竖屏x:0-240)
                     IPS154  (横屏y:0-240, 竖屏x:0-240)
                     IPS200  (横屏y:0-240, 竖屏x:0-320)
                                r ：半径
                                color_dat ：颜色
      返回值：  无
******************************************************************************/
void LCD_Draw_Circle(unsigned char x,unsigned char y,unsigned char r,unsigned short color_dat)
{
  unsigned char dx, dy = r;

  if((x>=r) &&((LCD_W-x)>=r) && (y>=r) && ((LCD_H-y)>=r))       //确定所画圆在屏幕范围内，没有超出最外边，(暂不支持与屏幕边相交)
  {
    for(dx = 0; dx <= r; dx++)
    {
      while((r * r + 1 - dx * dx) < (dy * dy)) dy--;
      LCD_DrawPoint(x + dx, y - dy, color_dat);
      LCD_DrawPoint(x - dx, y - dy, color_dat);
      LCD_DrawPoint(x - dx, y + dy, color_dat);
      LCD_DrawPoint(x + dx, y + dy, color_dat);

      LCD_DrawPoint(x + dy, y - dx, color_dat);
      LCD_DrawPoint(x - dy, y - dx, color_dat);
      LCD_DrawPoint(x - dy, y + dx, color_dat);
      LCD_DrawPoint(x + dy, y + dx, color_dat);
    }
  }
}

/******************************************************************************
      函数说明：显示图片
      入口数据：x,y起点坐标
                length 图片长度
                width  图片宽度
                pic[]  图片数组
      返回值：  无
******************************************************************************/
void LCD_ShowPicture(unsigned int x, unsigned int y, unsigned int length, unsigned int width,const unsigned char pic[])
{
    unsigned int i,j;
    unsigned long k=0;
    LCD_Address_Set(x,y,x+length-1,y+width-1);
    for(i=0;i<length;i++)
    {
        for(j=0;j<width;j++)
        {
            LCD_Write_Byte(pic[k*2]);
            LCD_Write_Byte(pic[k*2+1]);
            k++;
        }
    }
}
/******************************************************************************
      函数说明：显示单个12x12汉字
      入口数据：x,y显示的行数和列数
                *s_dat 要显示的汉字
                word_color 字的颜色
                back_color 字的背景色
      返回值：  无
******************************************************************************/
//void LCD_ShowChinese12x12(unsigned int x, unsigned int y,  char *s_dat,unsigned int word_color,unsigned int back_color)
//{
//    unsigned char wm=0,ii = 0,i,j;
//    int adder=1;
//    while(s_dat[ii] != '\0')
//    {
//        wm = 0;
//        adder = 1;
//        while(tfont_Idx[wm] > 127)
//        {
//          if(tfont_Idx[wm] == (uint8_t)s_dat[ii])
//          {
//            if(tfont_Idx[wm + 1] == (uint8_t)s_dat[ii + 1])
//            {
//              adder = wm * 12;
//              break;
//            }
//          }
//          wm += 2;
//        }

//        if(adder != 1)// 显示汉字
//        {
//            LCD_Address_Set(x*12,y*12,(x+1)*12-1,(y+1)*12-1);         //定位字符显示区域
//          for(j=0; j <24; j++)
//          {
//              if((j%2) == 0){
//                  for(i=0; i<8; i++)
//                  {
//                      if((tfont_12x12[adder]) &  (0x80>>i))
//                          LCD_Write_Word(word_color);
//                      else
//                          LCD_Write_Word(back_color);
//                  }
//              }
//              else{
//                  for(i=0; i<4; i++)
//                  {
//                      if((tfont_12x12[adder]) &  (0x80>>i))
//                          LCD_Write_Word(word_color);
//                      else
//                          LCD_Write_Word(back_color);
//                  }
//              }
//            adder += 1;
//          }
//        }
//        else              //显示空白字符
//        {}
//        //y+=1;//左右方向
//        x+=1;//上下方向
//        ii += 2;
//      }
//}

/******************************************************************************
      函数说明：显示单个16x16汉字
      入口数据：x,y显示的行数和列数
                *s_dat 要显示的汉字
                word_color 字的颜色
                back_color 字的背景色
      返回值：  无
******************************************************************************/
void LCD_ShowChinese16x16(unsigned int x, unsigned int y,  char *s_dat,unsigned int word_color,unsigned int back_color)
{
    unsigned char wm=0,ii = 0,i,j;
    int adder=1;
    while(s_dat[ii] != '\0')
    {
        wm = 0;
        adder = 1;
        while(tfont_Idx[wm] > 127)
        {
          if(tfont_Idx[wm] == (uint8_t)s_dat[ii])
          {
            if(tfont_Idx[wm + 1] == (uint8_t)s_dat[ii + 1])
            {
              adder = wm * 16;
              break;
            }
          }
          wm += 2;
        }

        if(adder != 1)// 显示汉字
        {
            LCD_Address_Set(x*16,y*16,(x+1)*16-1,(y+1)*16-1);         //定位字符显示区域
          for(j=0; j <32; j++)
          {
            for(i=0; i<8; i++)
            {
              if((tfont_16x16[adder]) &  (0x80>>i))
                  LCD_Write_Word(word_color);
              else
                  LCD_Write_Word(back_color);
            }
            adder += 1;
          }
        }
        else              //显示空白字符
        {}
        //y+=1;//左右方向
        x+=1;//上下方向
        ii += 2;
      }
}

/******************************************************************************
      函数说明：显示单个24x24汉字
      入口数据：x,y显示的行数和列数
                *s_dat 要显示的汉字
                word_color 字的颜色
                back_color 字的背景色
      返回值：  无
******************************************************************************/
//void LCD_ShowChinese24x24(unsigned int x, unsigned int y,  char *s_dat,unsigned int word_color,unsigned int back_color)
//{
//    unsigned char wm=0,ii = 0,i,j;
//    int adder=1;
//    while(s_dat[ii] != '\0')
//    {
//        wm = 0;
//        adder = 1;
//        while(tfont_Idx[wm] > 127)
//        {
//          if(tfont_Idx[wm] == (uint8_t)s_dat[ii])
//          {
//            if(tfont_Idx[wm + 1] == (uint8_t)s_dat[ii + 1])
//            {
//              adder = wm * 36;
//              break;
//            }
//          }
//          wm += 2;
//        }

//        if(adder != 1)// 显示汉字
//        {
//            LCD_Address_Set(x*24,y*24,(x+1)*24-1,(y+1)*24-1);         //定位字符显示区域
//          for(j=0; j <72; j++)
//          {
//            for(i=0; i<8; i++)
//            {
//              if((tfont_24x24[adder]) &  (0x80>>i))
//                  LCD_Write_Word(word_color);
//              else
//                  LCD_Write_Word(back_color);
//            }
//            adder += 1;
//          }
//        }
//        else              //显示空白字符
//        {}
//        //y+=1;//左右方向
//        x+=1;//上下方向
//        ii += 2;
//      }
//}


/******************************************************************************
      函数说明：显示单个32x32汉字
      入口数据：x,y显示的行数和列数
                *s_dat 要显示的汉字
                word_color 字的颜色
                back_color 字的背景色
      返回值：  无
******************************************************************************/
//void LCD_ShowChinese32x32(unsigned int x, unsigned int y,  char *s_dat,unsigned int word_color,unsigned int back_color)
//{
//    unsigned char wm=0,ii = 0,i,j;
//    int adder=1;
//    while(s_dat[ii] != '\0')
//    {
//        wm = 0;
//        adder = 1;
//        while(tfont_Idx[wm] > 127)
//        {
//          if(tfont_Idx[wm] == (uint8_t)s_dat[ii])
//          {
//            if(tfont_Idx[wm + 1] == (uint8_t)s_dat[ii + 1])
//            {
//              adder = wm * 64;
//              break;
//            }
//          }
//          wm += 2;
//        }

//        if(adder != 1)// 显示汉字
//        {
//            LCD_Address_Set(x*32,y*32,(x+1)*32-1,(y+1)*32-1);         //定位字符显示区域
//          for(j=0; j <128; j++)
//          {
//            for(i=0; i<8; i++)
//            {
//              if((tfont_32x32[adder]) &  (0x80>>i))
//                  LCD_Write_Word(word_color);
//              else
//                  LCD_Write_Word(back_color);
//            }
//            adder += 1;
//          }
//        }
//        else              //显示空白字符
//        {}
//        x+=1;//上下方向
//        ii += 2;
//      }
//}
/******************************************************************************
      函数说明：显示汉字串
      入口数据：x,y显示的起始行数和列数
                *s 要显示的汉字串
                fc 字的颜色
                bc 字的背景色
                sizey 字号 可选 12 16 24 32
      返回值：  无
            备注  ：  字库在font.h中，取模选项：宋体，阴码，逐行式，点阵16*16，顺向，十六进制，C51格式
******************************************************************************/
void LCD_ShowChinese(unsigned int x,unsigned int y,char *s,unsigned int fc,unsigned int bc,unsigned char sizey)
{
    while(*s!=0)
    {
//        if(sizey==12) LCD_ShowChinese12x12(x,y,s,fc,bc);
//        else
            if(sizey==16) LCD_ShowChinese16x16(x,y,s,fc,bc);
//        else if(sizey==24) LCD_ShowChinese24x24(x,y,s,fc,bc);
//        else if(sizey==32) LCD_ShowChinese32x32(x,y,s,fc,bc);
        else return;
        s+=2;
        x++;
    }
}

/*!
  * @brief    显示龙邱LOG
  *
  * @param    无
  *
  * @return   无
  *
  * @note     无
  *
  * @see      LCD_Show_Logo();
  */
//void LCD_Show_Logo(unsigned char xs,unsigned char ys)
//{
//    LCD_ShowPicture(xs,ys,160,54,gImage_logo);
//}

/*!
  * @brief    IPS屏 unsigned int RGB数据显示
  *
  * @param    high_start ： 显示图像开始位置
  * @param    wide_start ： 显示图像开始位置
  * @param    high ： 显示图像高度
  * @param    wide ： 显示图像宽度
  * @param    Pixle： 显示图像数据地址
  *
  * @return   无
  *
  * @note     注意 屏幕左上为 （0，0）
  *
  * @see
  *
  * @date     2024/7/31
  */
void LCDSPI_Road_RGB565(unsigned char wide_start, unsigned char high_start, unsigned char high, unsigned char wide, uint16_t *Pixle)
{
    unsigned long i,j;
    /* 设置显示范围 */
    LCD_Address_Set(wide_start, high_start, wide_start + wide - 1, high_start + high - 1);
    for(i = 0; i < LCDH; i++)
        {
            for(j = 0; j < LCDW; j ++)
            {
                /* 显示 */
//                color = Image_RGB565[i][j].R *2*2*2*2*2*2*2*2*2*2*2 +  Image_RGB565[i][j].G *2*2*2*2*2 + Image_RGB565[i][j].B;
                LCD_Write_Word (*Pixle++);
//                LCD_Write_Word (color);
            }
        }
}

/*!
  * @brief    IPS114屏 unsigned char 灰度数据显示
  *
  * @param    high_start ： 显示图像开始位置
  * @param    wide_start ： 显示图像开始位置
  * @param    high ： 显示图像高度
  * @param    wide ： 显示图像宽度
  * @param    Pixle： 显示图像数据地址
  *
  * @return   无
  *
  * @note     注意 屏幕左上为 （0，0）
  *
  * @see
  *
  * @date     2019/12/3 星期二
  */
void LCDSPI_Road(unsigned char wide_start, unsigned char high_start, unsigned char high, unsigned char wide, unsigned char *Pixle)
{
    uint32_t i;
    uint16_t color;
    uint32_t temp = high * wide;
    /* 设置显示范围 */
    LCD_Address_Set(wide_start, high_start, wide_start - 1 + wide, high_start - 1 + high);

    /* 显示图像 */
    for(i = 0; i < temp; i++)
    {
            /* 将灰度转化为 RGB565 */
            color = (Pixle[i] >> 3) << 11;
            color |= (Pixle[i] >> 2) << 5;
            color |= Pixle[i] >> 3;
//            /* 显示 */
            LCD_Write_Word (color);
    }
}

/*!
  * @brief    IPS114屏 unsigned char 二值化数据显示
  *
  * @param    high_start ： 显示图像开始位置
  * @param    wide_start ： 显示图像开始位置
  * @param    high ： 显示图像高度
  * @param    wide ： 显示图像宽度
  * @param    Pixle： 显示图像数据地址
  *
  * @return   无
  *
  * @note     注意 屏幕左上为 （0，0）
  *
  * @see
  *
  * @date     2019/12/3 星期二
  */
void LCDSPI_BinRoad(unsigned char wide_start, unsigned char high_start, unsigned char high, unsigned char wide, unsigned char *Pixle)
{
    unsigned long i;
    unsigned long temp = high * wide;
    /* 设置显示范围 */
    LCD_Address_Set(wide_start, high_start, wide_start - 1 + wide, high_start - 1 + high);


    /* 显示图像 */
    for(i = 0; i < temp; i++)
    {
        if(Pixle[i])
        {
            /* 显示 */
            LCD_Write_Word (0xffff);
        }
        else
        {
            /* 显示 */
            LCD_Write_Word (0);
        }
    }
}



