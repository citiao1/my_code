/*******************************************************************************
 * @file                LQ_lsm6dsr.c
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
#include "LQ_lsm6dsr.h"

/*************************************************************************
 * @code    	static void LQ_LSM6DSR_SPI_ReadWriteBytes
 *
 * @brief   	陀螺仪SPI读写指定长度的数据.
 * @param   	lq_buff : 数据指针.
 * @param   	len : 数据长度.
 * @return  	none.
 *
 * @note    	此函数会进行SPI通信操作，在操作过程中会拉低片选信号开始通信，通信结束后拉高片选信号.
 *************************************************************************/
static void LQ_LSM6DSR_SPI_ReadWriteBytes(unsigned char *lqbuff, unsigned int len)
{
	unsigned char i = 0;

	// 片选拉低，时钟线拉高
    LSM6DSR_CS_Clr();
    LSM6DSR_SCL_Set();

    do
    {
		for(i=0;i<8;i++)
		{
			if((*lqbuff)>=0x80) LSM6DSR_SDA_Set();
			else LSM6DSR_SDA_Clr();
					
			// 时钟线拉低
			LSM6DSR_SCL_Clr();
			// 数据左移一位
			(*lqbuff) <<= 1;
			// 时钟线拉高
			LSM6DSR_SCL_Set();
					
			// 读取数据
			(*lqbuff) |= LSM6DSR_SDO_READ;
		}
				
		// 指针指向下一个数据
		lqbuff++;
				
	} while(--len);
	
	// 片选拉高
	LSM6DSR_CS_Set();
}

/*************************************************************************
 * @code    	static void LQ_LSM6DSR_SPI_ReadData
 *
 * @brief   	陀螺仪SPI从设备读取指定长度的数据.
 * @param   	reg : 设备起始地址.
 * @param   	len : 待读取的数据长度.
 * @param   	buf : 数据存放地址.
 * @return  	none.
 *
 * @note    	此函数会设置读取标志，调用读写函数进行数据读取，并将读取的数据复制到目标缓冲区.
 *************************************************************************/
static void LQ_LSM6DSR_SPI_ReadData(unsigned char reg, unsigned int len, unsigned char* buf)
{
	unsigned short i = 0;
    unsigned char buff[32] = {0};
		
	// 设置读取标志
    buff[0] = reg | 0x80;

	// 调用读写函数
    LQ_LSM6DSR_SPI_ReadWriteBytes(buff, len+1);
		
	// 将读取的数据复制到目标缓冲区
    for(i=0;i<len;i++)
	{
		buf[i] = buff[i+1];
	}
}

/*************************************************************************
 * @code    	static void LQ_LSM6DSR_SPI_WriteData
 *
 * @brief   	陀螺仪SPI向设备写入数据.
 * @param   	reg : 设备起始地址.
 * @param   	value : 待写入的数据.
 * @return  	none.
 *
 * @note    	此函数会清除最高位设置为写入模式，然后调用读写函数进行数据写入.
 *************************************************************************/
static void LQ_LSM6DSR_SPI_WriteData(unsigned char reg, unsigned char value)
{
    unsigned char buff[2] = {0};

	// 清除最高位，设置为写入模式
    buff[0] = reg & 0x7f;
	// 待写入的数据
    buff[1] = value;
	// 调用读写函数
    LQ_LSM6DSR_SPI_ReadWriteBytes(buff, 2);
}


/*************************************************************************
 * @code    	unsigned char LQ_LSM6DSR_GetDeviceID
 *
 * @brief   	获取陀螺仪设备ID.
 * @param   	reg : 设备ID寄存器地址.
 * @return  	设备ID.
 *
 * @note    	此函数通过调用读取函数来获取设备ID.
 *************************************************************************/
static unsigned char LQ_LSM6DSR_GetDeviceID(unsigned char reg)
{  
    unsigned char buff = 0;
	
	// 调用读取函数
    LQ_LSM6DSR_SPI_ReadData(reg, 1, &buff);

    return buff;  
}


/*************************************************************************
 * @code    	void LQ_LSM6DSR_WriteRegister
 *
 * @brief   	向陀螺仪寄存器中写入数据.
 * @param   	reg : 寄存器地址.
 * @param   	data : 待写入的数据.
 * @return  	none.
 *
 * @note    	此函数通过调用写入函数来向寄存器写入数据.
 *************************************************************************/
static void LQ_LSM6DSR_WriteRegister(unsigned char reg,unsigned char dat)
{
    LQ_LSM6DSR_SPI_WriteData(reg, dat);
}


/*************************************************************************
 * @code    	unsigned char LQ_LSM6DSR_ReadRegister
 *
 * @brief   	从寄存器中读取数据.
 * @param   	reg : 寄存器地址.
 * @return  	读取到的数据.
 *
 * @note    	此函数通过调用读取函数来从寄存器读取数据.
 *************************************************************************/
static unsigned char LQ_LSM6DSR_ReadRegister(unsigned char reg)
{
    unsigned char buff;

	// 调用读取函数
    LQ_LSM6DSR_SPI_ReadData(reg, 1, &buff);
	
    return buff;
}


/*************************************************************************
 * @code    	void LQ_LSM6DSR_ReadMultipleRegisters
 *
 * @brief   	从寄存器中连续读取多个数据.
 * @param   	reg : 起始寄存器地址.
 * @param   	buf : 数据存放地址.
 * @param   	len : 待读取的数据长度.
 * @return  	none.
 *
 * @note    	此函数通过调用读取函数来连续读取多个寄存器的数据.
 *************************************************************************/
static void LQ_LSM6DSR_ReadMultipleRegisters(unsigned char reg, unsigned char* buf, unsigned char len)
{
	// 调用读取函数
    LQ_LSM6DSR_SPI_ReadData(reg, len, buf);
}


/*************************************************************************
 * @code    	unsigned char LQ_LSM6DSR_CheckDeviceID
 *
 * @brief   	陀螺仪LSM6DSR设备ID检测.
 * @param   	none.
 * @return  	检测结果，返回读取到的设备ID.
 *
 * @note    	会进行一系列配置操作，用于切换寄存器连接挂载的3轴地磁.
 *************************************************************************/
unsigned char LQ_LSM6DSR_CheckDeviceID(void)
{
    unsigned char res = 0, i = 0;

    LQ_LSM6DSR_WriteRegister(LSM6DSR_FUNC_CFG, 0x00);  // 切换 LSM6D 寄存器来连接挂载的 3 轴地磁
    res = LQ_LSM6DSR_GetDeviceID(WHO_AM_LSM6D);
    
    if(res == DRV_ID_LSM6D)
    {
        for(i=0;i<=3;i++)
        {
            LQ_LSM6DSR_WriteRegister(0x14, 0x80);
            delay_ms(5);
            LQ_LSM6DSR_WriteRegister(0x14, 0x00);
            delay_ms(5);
            LQ_LSM6DSR_WriteRegister(LSM6DSR_CTRL1_XL, 0X20);
        }
        LQ_LSM6DSR_WriteRegister(LSM6DSR_FUNC_CFG, 0x00);
    }
    return res;
}


/*************************************************************************
 * @code    	void LQ_LSM6DSR_Init
 *
 * @brief   	初始化LSM6DSR陀螺仪.
 * @param   	none.
 * @return  	none.
 *
 * @note    	会进行一系列寄存器配置操作，并进行设备检测.
 *************************************************************************/
void LQ_LSM6DSR_Init(void)
{
	LQConfig_GPIO_InitTypeDef_t lsm6dsr_gpio;
	
	lsm6dsr_gpio.Mode  = GPIO_MODE_OUTPUT_PP;	// 输出模式
	lsm6dsr_gpio.Pull  = GPIO_RESISTOR_NO_PULL;	// 浮空模式
	lsm6dsr_gpio.Speed = GPIO_SPEED_HIGH;		// 高驱模式
	LQ_GPIO_Init(LSM6DSR_SCK, &lsm6dsr_gpio);
	LQ_GPIO_Init(LSM6DSR_SDA, &lsm6dsr_gpio);
	LQ_GPIO_Init(LSM6DSR_CS, &lsm6dsr_gpio);
	lsm6dsr_gpio.Mode  = GPIO_MODE_INPUT;		// 输入模式
	LQ_GPIO_Init(LSM6DSR_SDO, &lsm6dsr_gpio);
	delay_ms(10);
	
	unsigned char res = 0;
	unsigned int count = 0;

	LQ_LSM6DSR_WriteRegister(LSM6DSR_CTRL1_XL, 0X20);        // 加速度计 52HZ（倾斜角检测功能工作在 26HZ，因此加速度计 ODR 必须设置为 >= 26hz），2g 量程
	LQ_LSM6DSR_WriteRegister(LSM6DSR_CTRL9_XL, 0X38);        // 使能加速度计 x, y, z 轴
	LQ_LSM6DSR_WriteRegister(LSM6DSR_CTRL6_C, 0X40|0x10);    // 陀螺仪电平触发，加速度计高性能使能
	LQ_LSM6DSR_WriteRegister(LSM6DSR_CTRL7_G, 0X80);         // 陀螺仪高性能使能
	LQ_LSM6DSR_WriteRegister(LSM6DSR_INT2_CTRL, 0X03);       // 加速度计 INT2 引脚失能，陀螺仪数据 INT2 使能
	LQ_LSM6DSR_WriteRegister(LSM6DSR_CTRL2_G, 0X1C);         // 陀螺仪 12.5hz  2000dps
	LQ_LSM6DSR_WriteRegister(LSM6DSR_CTRL10_C, 0X38);        // 使能陀螺仪 x, y, z 轴
	delay_ms(5);

	res = LQ_LSM6DSR_CheckDeviceID();
}


/*************************************************************************
 * @code    	void LQ_LSM6DSR_Read6AxisData
 *
 * @brief   	读取6轴陀螺仪数据.
 * @param   	ax : 加速度计X轴数据指针.
 * @param   	ay : 加速度计Y轴数据指针.
 * @param   	az : 加速度计Z轴数据指针.
 * @param   	gx : 陀螺仪X轴数据指针.
 * @param   	gy : 陀螺仪Y轴数据指针.
 * @param   	gz : 陀螺仪Z轴数据指针.
 * @return  	none.
 *
 * @note    	none.
 *************************************************************************/
void LQ_LSM6DSR_Read6AxisData(signed short *ax, signed short *ay, signed short *az, signed short *gx, signed short *gy, signed short *gz)
{
	unsigned char buf[12] = {0};
    
	LQ_LSM6DSR_ReadMultipleRegisters(LSM6DSR_OUTX_L_GYRO, buf, 12);
    
	*gx = ((unsigned int)buf[1]<<8) | buf[0];
	*gy = ((unsigned int)buf[3]<<8) | buf[2];
	*gz = ((unsigned int)buf[5]<<8) | buf[4];
	*ax = ((unsigned int)buf[7]<<8) | buf[6];
	*ay = ((unsigned int)buf[9]<<8) | buf[8];
	*az = ((unsigned int)buf[11]<<8)| buf[10];
}
