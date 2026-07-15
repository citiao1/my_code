/*******************************************************************************
 * @file                LQ_oled.c
 * @brief               本文件是 LQ_MSPM0GX_LIB 软件开源库文件的一部分
 * @copyright           版权所有 (C) 2025-2026 北京龙邱科技有限公司
 * @website             http://www.lqist.cn
 * @taobao              http://longqiu.taobao.com
 *
 * @description         龙邱科技 MSPM0G3507 核心板驱动库声明
 *
 * 开发环境配置:
 *   - 使用环境 : Keil5
 *   - 目标芯片 : MSPM0G3507
 *   - 外置晶振 : 16.000MHz
 *   - 系统时钟 : 80MHz
 *
 * 本文件遵循GPL-3.0开源协议发布，旨在为 MSPM0G3507 芯片嵌入式系统设计提供快速上手开发基于 MSPM0G3507 的应用程序的参考实现
 * 商业用途（包括单位使用）需提前联系作者获得授权
 *
 * GPL-3.0 许可证声明摘要:
 * 1. 允许自由使用、修改、分发本软件
 * 2. 分发修改后的版本时，必须以相同许可证发布
 * 3. 必须保留原始版权声明和许可证信息
 * 4. 不提供任何担保，使用风险自负
 * 5. 完整协议文本请参见项目根目录 LICENSE 文件
 *
 * @author              LQ_012
 * @email               chiusir@163.com
 * @version             V2.0.0
 * @update              2026年4月24日
 *******************************************************************************/
#include "LQ_oled.h"
#include "LQ_font.h"
#include "LQ_spi.h"

/********************************************************************************************************************

 * @模块名称：0.96 英寸 OLED 显示屏 （SPI 7针）

 * @模块概述：
 
    · 显示屏分辨率为 128 * 64。


 * @使用方法：
 
    1. 初始化阶段：
       在程序开始运行的初始化阶段，调用 “ OLED_Init(); ” 函数。
			 
			 
    2. 显示方法：
		
			 int oled_data1 = 123;
			 float oled_data2 = 123.45;
			 int oled_data3 = 0xABCD;

			 // 整型：(data1)
			 sprintf(txt, "data1: %4d", oled_data1);
			 OLED_ShowString(1, 0, (uint8_t *)txt, 8);
			 
			 // 浮点型：(data2)
			 sprintf(txt, "data2: %6.2f", oled_data2);
			 OLED_ShowString(2, 0, (uint8_t *)txt, 8);
			 
			 // 字符串：
			 sprintf(txt, "lqkj");
			 OLED_ShowString(3, 0, (uint8_t *)txt, 8);
			 
			 // 十六进制：(data3)
			 sprintf(txt, "data3: 0x%04X", oled_data3);
			 OLED_ShowString(4, 0, (uint8_t *)txt, 8);
			 
			 // 汉字
			 OLED_ShowChinese(3, 0, 0, 1);   // 北
			 OLED_ShowChinese(3, 14, 1, 1);  // 京
			 OLED_ShowChinese(3, 28, 2, 1);  // 龙
			 OLED_ShowChinese(3, 42, 3, 1);  // 邱
			 
			 
    3. 最后调用 “ OLED_Refresh(); ” 函数，更新显存到 OLED。


 * @注意事项：
 
		· 建议将相关显示操作放置在主函数的 “ while(1) ” 循环中调用。
		· 若屏幕刷新速度过快导致显示内容无法看清，可适当添加 5 - 50 ms 的延时，以调整显示效果。

 ******************************************************************************************************************/

/*************************************************************************
 * @code    	void LQ_OLED_WR_Byte
 *
 * @brief   	OLED写入一个字节数据/命令.
 * @param   	dat : 待写入的数据.
 * @param   	cmd : 1=命令，0=数据.
 * @return  	none.
 *
 * @note    	模拟SPI时序写入.
 *************************************************************************/
static void LQ_OLED_WR_Byte(unsigned char dat,unsigned char cmd)
{	
	unsigned char i;			  
	if(cmd) OLED_DC_Set();
	else    OLED_DC_Clr();		  
	OLED_CS_Clr();

//	LQ_SPI_SendByte(LQ_SPI1, cmd);
	
	for(i=0;i<8;i++)
	{			  
		OLED_SCL_Clr();
		if(dat&0x80) OLED_SDA_Set();
		else         OLED_SDA_Clr();
		OLED_SCL_Set();
		dat<<=1;   
	}
	
	OLED_CS_Set();
	OLED_DC_Set();   	  
}

static unsigned char OLED_GRAM[144][8];

/*************************************************************************
 * @code    	void LQ_OLED_ColorTurn
 *
 * @brief   	OLED反显/正常显示控制.
 * @param   	i : 0=正常显示，1=反色显示.
 * @return  	none.
 *
 * @note    	none.
 *************************************************************************/
void LQ_OLED_ColorTurn(unsigned char i)
{
	if(i==0)
	{
		LQ_OLED_WR_Byte(0xA6,OLED_CMD);//正常显示
	}
	if(i==1)
	{
		LQ_OLED_WR_Byte(0xA7,OLED_CMD);//反色显示
	}
}

/*************************************************************************
 * @code    	void LQ_OLED_DisplayTurn
 *
 * @brief   	OLED屏幕180度旋转显示控制.
 * @param   	i : 0=正常，1=旋转180度.
 * @return  	none.
 *
 * @note    	none.
 *************************************************************************/
void LQ_OLED_DisplayTurn(unsigned char i)
{
	if(i==0)
	{
		LQ_OLED_WR_Byte(0xC8,OLED_CMD);//正常显示
		LQ_OLED_WR_Byte(0xA1,OLED_CMD);
	}
	if(i==1)
	{
		LQ_OLED_WR_Byte(0xC0,OLED_CMD);//反转显示
		LQ_OLED_WR_Byte(0xA0,OLED_CMD);
	}
}

/*************************************************************************
 * @code    	void LQ_OLED_DisPlay_On
 *
 * @brief   	开启OLED显示.
 * @param   	none.
 * @return  	none.
 *
 * @note    	使能电荷泵并点亮屏幕.
 *************************************************************************/
void LQ_OLED_DisPlay_On(void)
{
	LQ_OLED_WR_Byte(0x8D,OLED_CMD);//电荷泵使能
	LQ_OLED_WR_Byte(0x14,OLED_CMD);//开启电荷泵
	LQ_OLED_WR_Byte(0xAF,OLED_CMD);//点亮屏幕
}

/*************************************************************************
 * @code    	void LQ_OLED_DisPlay_Off
 *
 * @brief   	关闭OLED显示.
 * @param   	none.
 * @return  	none.
 *
 * @note    	关闭电荷泵并关闭屏幕.
 *************************************************************************/
void LQ_OLED_DisPlay_Off(void)
{
	LQ_OLED_WR_Byte(0x8D,OLED_CMD);//电荷泵使能
	LQ_OLED_WR_Byte(0x10,OLED_CMD);//关闭电荷泵
	LQ_OLED_WR_Byte(0xAE,OLED_CMD);//关闭屏幕
}

/*************************************************************************
 * @code    	void LQ_OLED_Refresh
 *
 * @brief   	更新显存数据到OLED屏幕.
 * @param   	none.
 * @return  	none.
 *
 * @note    	修改GRAM后必须调用才能显示.
 *************************************************************************/
void LQ_OLED_Refresh(void)
{
	unsigned char i,n;
	for(i=0;i<8;i++)
	{
		LQ_OLED_WR_Byte(0xb0+i,OLED_CMD); //设置行起始地址
		LQ_OLED_WR_Byte(0x00,OLED_CMD);   //设置低列起始地址
		LQ_OLED_WR_Byte(0x10,OLED_CMD);   //设置高列起始地址
		for(n=0;n<128;n++) LQ_OLED_WR_Byte(OLED_GRAM[n][i],OLED_DATA);
	}
}

/*************************************************************************
 * @code    	void LQ_OLED_Clear
 *
 * @brief   	OLED清屏函数.
 * @param   	none.
 * @return  	none.
 *
 * @note    	清空GRAM并刷新屏幕.
 *************************************************************************/
void LQ_OLED_Clear(void)
{
	unsigned char i,n;
	for(i=0;i<8;i++)
	{
		for(n=0;n<128;n++)
		{
			OLED_GRAM[n][i]=0;//清除所有数据
		}
	}
	LQ_OLED_Refresh();//更新显示
}

/*************************************************************************
 * @code    	void LQ_OLED_DrawPoint
 *
 * @brief   	在OLED指定坐标画点.
 * @param   	x : 横坐标0~127.
 * @param   	y : 纵坐标0~63.
 * @param   	t : 1=填充，0=清空.
 * @return  	none.
 *
 * @note    	操作GRAM显存，需刷新才生效.
 *************************************************************************/
void LQ_OLED_DrawPoint(unsigned char x,unsigned char y,unsigned char t)
{
	unsigned char i,m,n;
	i = y/8;
	m = y%8;
	n = 1<<m;
	if(t){OLED_GRAM[x][i]|=n;}
	else
	{
		OLED_GRAM[x][i]=~OLED_GRAM[x][i];
		OLED_GRAM[x][i]|=n;
		OLED_GRAM[x][i]=~OLED_GRAM[x][i];
	}
}

/*************************************************************************
 * @code    	void LQ_OLED_DrawLine
 *
 * @brief   	在OLED两点之间画线.
 * @param   	x1,y1 : 起点坐标.
 * @param   	x2,y2 : 终点坐标.
 * @param   	mode : 显示模式.
 * @return  	none.
 *
 * @note    	使用Bresenham算法.
 *************************************************************************/
void LQ_OLED_DrawLine(unsigned char x1,unsigned char y1,unsigned char x2,unsigned char y2,unsigned char mode)
{
	unsigned int t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance;
	int incx,incy,uRow,uCol;
	delta_x=x2-x1; //计算坐标增量 
	delta_y=y2-y1;
	uRow=x1;//画线起点坐标
	uCol=y1;
	if(delta_x>0)incx=1; //设置单步方向 
	else if (delta_x==0)incx=0;//垂直线 
	else {incx=-1;delta_x=-delta_x;}
	if(delta_y>0)incy=1;
	else if (delta_y==0)incy=0;//水平线 
	else {incy=-1;delta_y=-delta_x;}
	if(delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴 
	else distance=delta_y;
	for(t=0;t<distance+1;t++)
	{
		LQ_OLED_DrawPoint(uRow,uCol,mode);//画点
		xerr+=delta_x;
		yerr+=delta_y;
		if(xerr>distance)
		{
			xerr-=distance;
			uRow+=incx;
		}
		if(yerr>distance)
		{
			yerr-=distance;
			uCol+=incy;
		}
	}
}

/*************************************************************************
 * @code    	void LQ_OLED_DrawCircle
 *
 * @brief   	在OLED指定位置画圆.
 * @param   	x : 圆心X坐标.
 * @param   	y : 圆心Y坐标.
 * @param   	r : 圆半径.
 * @return  	none.
 *
 * @note    	使用中点画圆算法.
 *************************************************************************/
void LQ_OLED_DrawCircle(unsigned char x,unsigned char y,unsigned char r)
{
	int a, b,num;
    a = 0;
    b = r;
    while(2 * b * b >= r * r)      
    {
        LQ_OLED_DrawPoint(x + a, y - b,1);
        LQ_OLED_DrawPoint(x - a, y - b,1);
        LQ_OLED_DrawPoint(x - a, y + b,1);
        LQ_OLED_DrawPoint(x + a, y + b,1);
 
        LQ_OLED_DrawPoint(x + b, y + a,1);
        LQ_OLED_DrawPoint(x + b, y - a,1);
        LQ_OLED_DrawPoint(x - b, y - a,1);
        LQ_OLED_DrawPoint(x - b, y + a,1);
        
        a++;
        num = (a * a + b * b) - r*r;//计算画的点离圆心的距离
        if(num > 0)
        {
            b--;
            a--;
        }
    }
}

/*************************************************************************
 * @code    	void LQ_OLED_ShowChar
 *
 * @brief   	在OLED指定位置显示一个ASCII字符.
 * @param   	x : 横坐标.
 * @param   	y : 纵坐标.
 * @param   	chr : 待显示字符.
 * @param   	size1 : 字体大小6x8/6x12.
 * @param   	mode : 0反色，1正常.
 * @return  	none.
 *
 * @note    	支持6x8、12x06字体.
 *************************************************************************/
void LQ_OLED_ShowChar(unsigned char x,unsigned char y,unsigned char chr,unsigned char size1,unsigned char mode)
{
	unsigned char i,m,temp,size2,chr1;
	unsigned char x0=x,y0=y;
	if(size1==8)size2=6;
	else size2=(size1/8+((size1%8)?1:0))*(size1/2);  //得到字体一个字符对应点阵集所占的字节数
	chr1=chr-' ';  //计算偏移后的值
	for(i=0;i<size2;i++)
	{
		if     (size1==8)  { temp=asc2_0806[chr1][i]; } //调用0806字体
		else if(size1==12) { temp=asc2_1206[chr1][i]; } //调用1206字体
		else return;
				
		for(m=0;m<8;m++)
		{
			if(temp&0x01)LQ_OLED_DrawPoint(x,y,mode);
			else LQ_OLED_DrawPoint(x,y,!mode);
			temp>>=1;
			y++;
		}
		x++;
		if((size1!=8)&&((x-x0)==size1/2)) { x=x0;y0=y0+8; }
		y=y0;
  }
}

/*************************************************************************
 * @code    	void LQ_OLED_ShowString
 *
 * @brief   	在OLED指定位置显示字符串.
 * @param   	y : 行坐标.
 * @param   	x : 列坐标.
 * @param   	*chr : 字符串指针.
 * @param   	size1 : 字体大小.
 * @return  	none.
 *
 * @note    	自动换行，支持ASCII字符串.
 *************************************************************************/
void LQ_OLED_ShowString(unsigned char y,unsigned char x,unsigned char *chr,unsigned char size1)
{
	char mode = 1;
	while((*chr>=' ')&&(*chr<='~'))//判断是不是非法字符!
	{
		if(size1==8) LQ_OLED_ShowChar(x,y*8,*chr,size1,mode);
		if(size1==12)LQ_OLED_ShowChar(x,y*16,*chr,size1,mode);
		if(size1==8) x += 6;
		else         x += size1/2;
		chr++;
  }
}

/*************************************************************************
 * @code    	unsigned int LQ_OLED_Pow
 *
 * @brief   	计算m的n次方.
 * @param   	m : 底数.
 * @param   	n : 指数.
 * @return  	计算结果.
 *
 * @note    	供数字显示函数使用.
 *************************************************************************/
unsigned int LQ_OLED_Pow(unsigned char m,unsigned char n)
{
	unsigned int result=1;
	while(n--)
	{
		result*=m;
	}
	return result;
}

/*************************************************************************
 * @code    	void LQ_OLED_ShowNum
 *
 * @brief   	在OLED指定位置显示数字.
 * @param   	x : 横坐标.
 * @param   	y : 纵坐标.
 * @param   	num : 待显示数字.
 * @param   	len : 数字位数.
 * @param   	size1 : 字体大小.
 * @param   	mode : 显示模式.
 * @return  	none.
 *
 * @note    	支持无符号整数显示.
 *************************************************************************/
void LQ_OLED_ShowNum(unsigned char x,unsigned char y,unsigned int num,unsigned char len,unsigned char size1,unsigned char mode)
{
	unsigned char t,temp,m=0;
	if(size1==8)m=2;
	for(t=0;t<len;t++)
	{
		temp=(num/LQ_OLED_Pow(10,len-t-1))%10;
		if(temp==0)
		{
			LQ_OLED_ShowChar(x+(size1/2+m)*t,y,'0',size1,mode);
		}
		else 
		{
			LQ_OLED_ShowChar(x+(size1/2+m)*t,y,temp+'0',size1,mode);
		}
  }
}

/*************************************************************************
 * @code    	void LQ_OLED_ShowChinese
 *
 * @brief   	在OLED指定位置显示汉字.
 * @param   	y : 行坐标.
 * @param   	x : 列坐标.
 * @param   	num : 汉字序号.
 * @param   	mode : 显示模式.
 * @return  	none.
 *
 * @note    	使用14*16汉字库.
 *************************************************************************/
void LQ_OLED_ShowChinese(unsigned char y,unsigned char x,unsigned char num,unsigned char mode)
{
	y*=16;
	unsigned char m,temp;
	unsigned char x0=x,y0=y;
	unsigned int i,size3=(14/8+((14%8)?1:0))*14;  //得到字体一个字符对应点阵集所占的字节数
	for(i=0;i<size3;i++)
	{
		temp=Hzk1[num][i];//调用14*16字体
				
		for(m=0;m<8;m++)
		{
			if(temp&0x01)LQ_OLED_DrawPoint(x,y,mode);
			else LQ_OLED_DrawPoint(x,y,!mode);
			temp>>=1;
			y++;
		}
		x++;
		if((x-x0)==14)
		{x=x0;y0=y0+8;}
		y=y0;
	}
}

/*************************************************************************
 * @code    	void LQ_OLED_ScrollDisplay
 *
 * @brief   	汉字左滚动显示.
 * @param   	num : 汉字个数.
 * @param   	space : 滚动间隔.
 * @param   	mode : 显示模式.
 * @return  	none.
 *
 * @note    	死循环函数，需单独任务调用.
 *************************************************************************/
void LQ_OLED_ScrollDisplay(unsigned char num,unsigned char space,unsigned char mode)
{
	unsigned char i,n,t=0,m=0,r;
	while(1)
	{
		if(m==0)
		{
			LQ_OLED_ShowChinese(128,24,t,mode); //写入一个汉字保存在OLED_GRAM[][]数组中
			t++;
		}
		if(t==num)
		{
			for(r=0;r<16*space;r++)      //显示间隔
			{
				for(i=1;i<144;i++)
				{
					for(n=0;n<8;n++)
					{
						OLED_GRAM[i-1][n]=OLED_GRAM[i][n];
					}
				}
				LQ_OLED_Refresh();
			}
			t=0;
		}
		m++;
		if(m==16){m=0;}
		for(i=1;i<144;i++)   //实现左移
		{
			for(n=0;n<8;n++)
			{
				OLED_GRAM[i-1][n]=OLED_GRAM[i][n];
			}
		}
		LQ_OLED_Refresh();
	}
}

/*************************************************************************
 * @code    	void LQ_OLED_ShowPicture
 *
 * @brief   	在OLED显示图片.
 * @param   	x,y   : 起点坐标.
 * @param   	sizex : 图片宽度.
 * @param   	sizey : 图片高度.
 * @param   	BMP[] : 图片数组.
 * @param   	mode  : 显示模式.
 * @return  	none.
 *
 * @note    	支持单色BMP格式图片.
 *************************************************************************/
void LQ_OLED_ShowPicture(unsigned char x,unsigned char y,unsigned char sizex,unsigned char sizey,unsigned char BMP[],unsigned char mode)
{
	unsigned int j = 0;
	unsigned char i, n, temp, m;
	unsigned char x0 = x, y0 = y;
	sizey = sizey / 8 + ((sizey % 8) ? 1 : 0);
	for(n = 0; n < sizey; n++)
	{
		 for(i = 0; i < sizex; i++)
		 {
			temp = BMP[j];
			j++;
			for(m = 0; m < 8; m++)
			{
				if (temp&0x01) LQ_OLED_DrawPoint(x,y,mode);
				else           LQ_OLED_DrawPoint(x,y,!mode);
				temp >>= 1;
				y++;
			}
			x++;
			if ((x-x0) == sizex)
			{
				x  = x0;
				y0 = y0+8;
			}
			y = y0;
		}
	 }
}

/*************************************************************************
 * @code    	void LQ_OLED_Init
 *
 * @brief   	OLED初始化.
 * @param   	none.
 * @return  	none.
 *
 * @note    	使用OLED前必须初始化.
 *************************************************************************/
void LQ_OLED_Init(void)
{
	LQConfig_GPIO_InitTypeDef_t oled_gpio = {0};
	
	oled_gpio.Mode  = GPIO_MODE_OUTPUT_PP;		// 输出模式
	oled_gpio.Pull  = GPIO_RESISTOR_NO_PULL;	// 浮空模式
	oled_gpio.Speed = GPIO_SPEED_HIGH;			// 高驱模式
	LQ_GPIO_Init(OLED_SCL_PIN, &oled_gpio);
	LQ_GPIO_Init(OLED_SDA_PIN, &oled_gpio);
	LQ_GPIO_Init(OLED_CS_PIN , &oled_gpio);
	
	LQ_GPIO_Init(OLED_RES_PIN, &oled_gpio);
	LQ_GPIO_Init(OLED_DC_PIN , &oled_gpio);
	
//	LQConfig_SPI_InitTypeDef_t oled_spi = {
//		.DivideRatio = DL_SPI_CLOCK_DIVIDE_RATIO_1,
//		.Prescaler   = 1 - 1,
//		.mode        = DL_SPI_MODE_CONTROLLER,
//		.dataSize    = DL_SPI_DATA_SIZE_8,
//		.bitOrder    = DL_SPI_BIT_ORDER_MSB_FIRST,
//		.frameFormat = DL_SPI_FRAME_FORMAT_MOTO3_POL0_PHA0,
//		.SCLK        = SPI1_SCLK_Pin_A_17,
//		.MISO        = SPI_NO_PIN,
//		.MOSI        = SPI1_MOSI_Pin_A_18,
//		.CS          = SPI_NO_PIN,
//	};
//	LQ_SPI_Init(LQ_SPI1, &oled_spi);
	
	delay_ms(10);
	
	OLED_RES_Clr();
	delay_ms(50);
	OLED_RES_Set();
	
	LQ_OLED_WR_Byte(0xAE,OLED_CMD);//--turn off oled panel
	LQ_OLED_WR_Byte(0x00,OLED_CMD);//---set low column address
	LQ_OLED_WR_Byte(0x10,OLED_CMD);//---set high column address
	LQ_OLED_WR_Byte(0x40,OLED_CMD);//--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
	LQ_OLED_WR_Byte(0x81,OLED_CMD);//--set contrast control register
	LQ_OLED_WR_Byte(0xCF,OLED_CMD);// Set SEG Output Current Brightness
	LQ_OLED_WR_Byte(0xA1,OLED_CMD);//--Set SEG/Column Mapping     0xa0左右反置 0xa1正常
	LQ_OLED_WR_Byte(0xC8,OLED_CMD);//Set COM/Row Scan Direction   0xc0上下反置 0xc8正常
	LQ_OLED_WR_Byte(0xA6,OLED_CMD);//--set normal display
	LQ_OLED_WR_Byte(0xA8,OLED_CMD);//--set multiplex ratio(1 to 64)
	LQ_OLED_WR_Byte(0x3f,OLED_CMD);//--1/64 duty
	LQ_OLED_WR_Byte(0xD3,OLED_CMD);//-set display offset	Shift Mapping RAM Counter (0x00~0x3F)
	LQ_OLED_WR_Byte(0x00,OLED_CMD);//-not offset
	LQ_OLED_WR_Byte(0xd5,OLED_CMD);//--set display clock divide ratio/oscillator frequency
	LQ_OLED_WR_Byte(0x80,OLED_CMD);//--set divide ratio, Set Clock as 100 Frames/Sec
	LQ_OLED_WR_Byte(0xD9,OLED_CMD);//--set pre-charge period
	LQ_OLED_WR_Byte(0xF1,OLED_CMD);//Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
	LQ_OLED_WR_Byte(0xDA,OLED_CMD);//--set com pins hardware configuration
	LQ_OLED_WR_Byte(0x12,OLED_CMD);
	LQ_OLED_WR_Byte(0xDB,OLED_CMD);//--set vcomh
	LQ_OLED_WR_Byte(0x40,OLED_CMD);//Set VCOM Deselect Level
	LQ_OLED_WR_Byte(0x20,OLED_CMD);//-Set Page Addressing Mode (0x00/0x01/0x02)
	LQ_OLED_WR_Byte(0x02,OLED_CMD);//
	LQ_OLED_WR_Byte(0x8D,OLED_CMD);//--set Charge Pump enable/disable
	LQ_OLED_WR_Byte(0x14,OLED_CMD);//--set(0x10) disable
	LQ_OLED_WR_Byte(0xA4,OLED_CMD);// Disable Entire Display On (0xa4/0xa5)
	LQ_OLED_WR_Byte(0xA6,OLED_CMD);// Disable Inverse Display On (0xa6/a7) 
	LQ_OLED_Clear();
	LQ_OLED_WR_Byte(0xAF,OLED_CMD);
	LQ_OLED_Clear();
}
