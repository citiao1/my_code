#include "LQ_lsm6dsr.h"




/********************************************************************************************************************

 * @模块名称：LSM6DSR 六轴陀螺仪模块

 * @模块概述：
 
    本模块聚焦于实现三轴加速度和三轴角速度的精准采集。加速度数据代表物体在三个不同空间轴上的加速情况，而角速度数据则体现了物体绕三个轴的旋转速度。
		
		· 角速度数据可用于角度积分运算，通过对时间的累积，将角速度转换为角度信息，为进一步的姿态分析提供基础。
		· 借助四元数等数学方法，结合加速度和角速度数据，可以进行复杂的姿态解算。这在平衡控制场景（如平衡车、无人机的稳定飞行）以及惯性导航系统中起着关键作用，能够实时准确地确定物体在空间中的姿态和位置。
		· 在 PID（比例 - 积分 - 微分）控制环节中，加速度和角速度数据可作为阻尼因子，帮助系统更快地达到稳定状态，减少超调并提高控制精度；
			 同时，加速度和角速度也可以应用于串级 PID 控制，提升系统的控制性能和响应速度。


 * @使用方法：
 
    1. 初始化阶段：
       在程序开始运行的初始化阶段，调用 “ LQ_LSM6DSR_init(); ” 函数。
			 
			 // 注：设备 ID 已在初始化函数中完成读取操作。在示例程序 “ LQ_Test_LSM6DSR() ” 里读取的设备 ID 仅用于屏幕显示，在实际开发使用时，若无显示设备 ID 的需求，无需额外再次读取设备 ID，以节省系统资源和运行时间。
			 
    2. 数据采集阶段：
       在 主循环 或者 定时器中断 中调用 “ LQ_LSM6DSR_Read6AxisData(&LQ_LSM6DSR_Acc_X, &LQ_LSM6DSR_Acc_Y, &LQ_LSM6DSR_Acc_Z, &LQ_LSM6DSR_Gyro_X, &LQ_LSM6DSR_Gyro_Y, &LQ_LSM6DSR_Gyro_Z); ” 函数。
			 
    3. 数据访问阶段：
       采集到的数据会存储在全局变量 “ LQ_LSM6DSR_Acc_X、 LQ_LSM6DSR_Acc_Y、 LQ_LSM6DSR_Acc_Z、 LQ_LSM6DSR_Gyro_X、 LQ_LSM6DSR_Gyro_Y、 LQ_LSM6DSR_Gyro_Z ” 中，用户可以直接访问这些变量来获取相应的加速度和角速度数据。
			 


 * @注意事项：
 
    使用方法中未涉及的函数或变量主要供内部调用。非必要情况下，用户无需深入了解其实现细节，只需按照上述使用方法进行操作即可。
		若确实需要对模块进行定制或优化，建议在充分了解模块整体架构和硬件原理的基础上进行。

 ******************************************************************************************************************/




//-----------------LSM6DSR 六轴陀螺仪最终数据-----------------
signed short LQ_LSM6DSR_Acc_X = 0, LQ_LSM6DSR_Acc_Y = 0, LQ_LSM6DSR_Acc_Z = 0;
signed short LQ_LSM6DSR_Gyro_X = 0, LQ_LSM6DSR_Gyro_Y = 0, LQ_LSM6DSR_Gyro_Z = 0;



/********************************************************************************************************************
 * 【函数名称】 LQ_LSM6DSR_SPI_ReadWriteBytes
 * 【功能概述】 陀螺仪 SPI 读写指定长度的数据
 * 【输入参数】 unsigned char *lq_buff：数据指针；unsigned int len：数据长度
 * 【返 回 值】 无
 * 【使用示例】 LQ_LSM6DSR_SPI_ReadWriteBytes(buff, len + 1);
 * 【注意事项】 此函数会进行 SPI 通信操作，在操作过程中会拉低片选信号开始通信，通信结束后拉高片选信号。
 ******************************************************************************************************************/
void LQ_LSM6DSR_SPI_ReadWriteBytes(unsigned char *lqbuff, unsigned int len)
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


/********************************************************************************************************************
 * 【函数名称】 LQ_LSM6DSR_SPI_ReadData
 * 【功能概述】 陀螺仪 SPI 从设备读取指定长度的数据
 * 【输入参数】 unsigned char reg：设备起始地址；unsigned int len：待读取的数据长度；unsigned char *buf：数据存放地址
 * 【返 回 值】 无
 * 【使用示例】 LQ_LSM6DSR_SPI_ReadData(reg, len, buf);
 * 【注意事项】 此函数会设置读取标志，调用读写函数进行数据读取，并将读取的数据复制到目标缓冲区。
 ******************************************************************************************************************/
void LQ_LSM6DSR_SPI_ReadData(unsigned char reg, unsigned int len, unsigned char* buf)
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


/********************************************************************************************************************
 * 【函数名称】 LQ_LSM6DSR_SPI_WriteData
 * 【功能概述】 陀螺仪 SPI 向设备写入数据
 * 【输入参数】 unsigned char reg：设备起始地址；unsigned char value：待写入的数据
 * 【返 回 值】 无
 * 【使用示例】 LQ_LSM6DSR_SPI_WriteData(reg, value);
 * 【注意事项】 此函数会清除最高位设置为写入模式，然后调用读写函数进行数据写入。
 ******************************************************************************************************************/
void LQ_LSM6DSR_SPI_WriteData(unsigned char reg, unsigned char value)
{
    unsigned char buff[2] = {0};

		// 清除最高位，设置为写入模式
    buff[0] = reg & 0x7f;
		// 待写入的数据
    buff[1] = value;
		// 调用读写函数
    LQ_LSM6DSR_SPI_ReadWriteBytes(buff, 2);
}


/********************************************************************************************************************
 * 【函数名称】 LQ_LSM6DSR_GetDeviceID
 * 【功能概述】 获取陀螺仪设备 ID
 * 【输入参数】 unsigned char reg：设备 ID 寄存器地址
 * 【返 回 值】 unsigned char：设备 ID
 * 【使用示例】 unsigned char id = LQ_LSM6DSR_GetDeviceID(reg);
 * 【注意事项】 此函数通过调用读取函数来获取设备 ID。
 ******************************************************************************************************************/
unsigned char LQ_LSM6DSR_GetDeviceID(unsigned char reg)
{  
    unsigned char buff = 0;
	
		// 调用读取函数
    LQ_LSM6DSR_SPI_ReadData(reg, 1, &buff);

    return buff;  
}


/********************************************************************************************************************
 * 【函数名称】 LQ_LSM6DSR_WriteRegister
 * 【功能概述】 向陀螺仪寄存器中写入数据
 * 【输入参数】 unsigned char reg：寄存器地址；unsigned char data：待写入的数据
 * 【返 回 值】 无
 * 【使用示例】 LQ_LSM6DSR_WriteRegister(reg, data);
 * 【注意事项】 此函数通过调用写入函数来向寄存器写入数据。
 ******************************************************************************************************************/
void LQ_LSM6DSR_WriteRegister(unsigned char reg,unsigned char dat)
{
    LQ_LSM6DSR_SPI_WriteData(reg, dat);
}


/********************************************************************************************************************
 * 【函数名称】 LQ_LSM6DSR_ReadRegister
 * 【功能概述】 从寄存器中读取数据
 * 【输入参数】 unsigned char reg：寄存器地址
 * 【返 回 值】 unsigned char：读取到的数据
 * 【使用示例】 unsigned char data = LQ_LSM6DSR_ReadRegister(reg);
 * 【注意事项】 此函数通过调用读取函数来从寄存器读取数据。
 ******************************************************************************************************************/
unsigned char LQ_LSM6DSR_ReadRegister(unsigned char reg)
{
    unsigned char buff;

		// 调用读取函数
    LQ_LSM6DSR_SPI_ReadData(reg, 1, &buff);
	
    return buff;
}


/********************************************************************************************************************
 * 【函数名称】 LQ_LSM6DSR_ReadMultipleRegisters
 * 【功能概述】 从寄存器中连续读取多个数据
 * 【输入参数】 
 *     - unsigned char reg：起始寄存器地址
 *     - unsigned char *buf：数据存放地址
 *     - unsigned char len：待读取的数据长度
 * 【返 回 值】 无
 * 【使用示例】 LQ_LSM6DSR_ReadMultipleRegisters(reg, buf, len);
 * 【注意事项】 此函数通过调用读取函数来连续读取多个寄存器的数据。
 ******************************************************************************************************************/
void LQ_LSM6DSR_ReadMultipleRegisters(unsigned char reg, unsigned char* buf, unsigned char len)
{
		// 调用读取函数
    LQ_LSM6DSR_SPI_ReadData(reg, len, buf);
}


/********************************************************************************************************************
 * 【函数名称】 LQ_LSM6DSR_CheckDeviceID
 * 【功能概述】 陀螺仪 LSM6DSR 设备 ID 检测
 * 【输入参数】 无
 * 【返 回 值】 unsigned char：检测结果，返回读取到的设备 ID
 * 【使用示例】 unsigned char res = LQ_LSM6DSR_CheckDeviceID();
 * 【注意事项】 会进行一系列配置操作，用于切换寄存器连接挂载的 3 轴地磁
 ******************************************************************************************************************/
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


/********************************************************************************************************************
 * 【函数名称】 LQ_LSM6DSR_Init
 * 【功能概述】 初始化 LSM6DSR 陀螺仪
 * 【输入参数】 无
 * 【返 回 值】 无
 * 【使用示例】 LQ_LSM6DSR_Init();
 * 【注意事项】 会进行一系列寄存器配置操作，并进行设备检测
 ******************************************************************************************************************/
void LQ_LSM6DSR_Init(void)
{
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


/********************************************************************************************************************
 * 【函数名称】 LQ_LSM6DSR_Read6AxisData
 * 【功能概述】 读取 6 轴陀螺仪数据
 * 【输入参数】 
 *     - signed short *ax：加速度计 X 轴数据指针
 *     - signed short *ay：加速度计 Y 轴数据指针
 *     - signed short *az：加速度计 Z 轴数据指针
 *     - signed short *gx：陀螺仪 X 轴数据指针
 *     - signed short *gy：陀螺仪 Y 轴数据指针
 *     - signed short *gz：陀螺仪 Z 轴数据指针
 * 【返 回 值】 无
 * 【使用示例】 
 *     signed short ax, ay, az, gx, gy, gz;
 *     LQ_LSM6DSR_Read6AxisData(&ax, &ay, &az, &gx, &gy, &gz);
 * 【注意事项】 无
 ******************************************************************************************************************/
void LQ_LSM6DSR_Read6AxisData(signed short *ax, signed short *ay, signed short *az, signed short *gx, signed short *gy, signed short *gz)
{
		unsigned char buf[12] = {0};
    
		LQ_LSM6DSR_ReadMultipleRegisters(LSM6DSR_OUTX_L_GYRO, buf, 12);
    
		*gx = ((unsigned int)buf[1]<<8)|buf[0];
		*gy = ((unsigned int)buf[3]<<8)|buf[2];
		*gz = ((unsigned int)buf[5]<<8)|buf[4];
		*ax = ((unsigned int)buf[7]<<8)|buf[6];
		*ay = ((unsigned int)buf[9]<<8)|buf[8];
		*az = ((unsigned int)buf[11]<<8)|buf[10];
}



/********************************************************************************************************************
 * 【函数名称】 LQ_Test_LSM6DSR
 * 【功能概述】 该函数用于测试 LSM6DSR 陀螺仪传感器，并将相关数据显示在 OLED 屏幕上。
 *             主要流程包括初始化 LSM6DSR 传感器和 OLED 屏幕，检查设备 ID 并显示，然后在循环中持续读取 6 轴陀螺仪数据并更新显示。
 * 【输入参数】 无
 * 【返 回 值】 无
 * 【使用示例】 LQ_Test_LSM6DSR();
 * 【注意事项】 无
 ******************************************************************************************************************/
void LQ_Test_LSM6DSR()
{
		LQ_LSM6DSR_Init();  // 陀螺仪初始化
		unsigned char res = LQ_LSM6DSR_CheckDeviceID();  // 获取设备 ID
	
		// 初始化OLED
		OLED_Init();
	
		sprintf(txt, "LQ_LSM6DSR_Test");
		OLED_ShowString(0, 18,(unsigned char *)txt,8);
	

		// ID 显示
		if(res == DRV_ID_LSM6D)
		{
				sprintf(txt, "ID: 0X%2X", res);
				OLED_ShowString(2, 35, (unsigned char *)txt, 8);
		}
		else
		{
				sprintf(txt, "ID: ERROR");
				OLED_ShowString(2, 35, (unsigned char *)txt, 8);
		}
		
		while(1)
		{
			// 陀螺仪六轴数据采集
			LQ_LSM6DSR_Read6AxisData(&LQ_LSM6DSR_Acc_X, &LQ_LSM6DSR_Acc_Y, &LQ_LSM6DSR_Acc_Z, &LQ_LSM6DSR_Gyro_X, &LQ_LSM6DSR_Gyro_Y, &LQ_LSM6DSR_Gyro_Z);

			// 加速度数据
			sprintf(txt, "ax:%06d", LQ_LSM6DSR_Acc_X);
			OLED_ShowString(4, 0, (unsigned char *)txt, 8);
			sprintf(txt, "ay:%06d", LQ_LSM6DSR_Acc_Y);
			OLED_ShowString(5, 0, (unsigned char *)txt, 8);
			sprintf(txt, "az:%06d", LQ_LSM6DSR_Acc_Z);
			OLED_ShowString(6, 0, (unsigned char *)txt, 8);
			
			// 角速度数据
			sprintf(txt, "gx:%06d", LQ_LSM6DSR_Gyro_X);
			OLED_ShowString(4, 70, (unsigned char *)txt, 8);
			sprintf(txt, "gy:%06d", LQ_LSM6DSR_Gyro_Y);
			OLED_ShowString(5, 70, (unsigned char *)txt, 8);
			sprintf(txt, "gz:%06d", LQ_LSM6DSR_Gyro_Z);
			OLED_ShowString(6, 70, (unsigned char *)txt, 8);

            OLED_Refresh();	
		}
}


