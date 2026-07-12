/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
 【平    台】北京龙邱智能科技STC32位核心板
 【编    写】龙邱科技
 【E-mail  】chiusir@163.com
 【软件版本】V1.1 版权所有，单位使用请先联系授权
 【相关信息参考下列地址】
 【网    站】http://www.lqist.cn
 【淘宝店铺】http://longqiu.taobao.com
 --------------------------------------------------------------------------------
 【  IDE  】 keil C251 V5.60
 【Target 】 STC32G/STC8051U/AI8051U 32位模式
 【SYS CLK】 42 MHz使用内部晶振
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/

#include "LQ_HSPI.h"
#include "include.h"

/***************************************************************
 *  函数名：void HSPI_Init()
 *  功  能：硬件SPI应用初始化
 *  参  数：无
 *  返回值：无
 *  说  明：这里初始化的模式是对应SPI接口 陀螺仪的 ，其他SPI使用注意重新初始化
 *          同时注意，修改对应使用的SPI管脚 和初始化参数
 **************************************************************/
void HSPI_Init()
{
    GPIO_InitTypeDef    SPI_GPIO_InitStr;
    SPI_InitTypeDef		SPI_InitStructure;
    
    SPI_SW(SPI_P40_P41_P42_P43);	        // SPI管脚选择，按组
	SPI_SS_3 = 1;                           //对应上面选择的管脚，同步修改
    SPI_IOSW_Enable();                      //交换MMSI和MISO脚位
    
     //初始化串口使用收发管脚，需要与实际选择的串口保持一致
    SPI_GPIO_InitStr.Pin  = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3;      //指定要初始化的SPI管脚
    SPI_GPIO_InitStr.Mode = GPIO_PullUp;	                //指定IO的输入或输出方式,GPIO_PullUp 准双向口
    GPIO_Inilize(GPIO_P4,&SPI_GPIO_InitStr);	            //初始化，结构体配置

	SPI_InitStructure.SPI_Mode      = SPI_Mode_Master;		//主从选择   SPI_Mode_Master, SPI_Mode_Slave
	SPI_InitStructure.SPI_CPOL      = SPI_CPOL_Low;			//时钟相位   SPI_CPOL_High,   SPI_CPOL_Low
	SPI_InitStructure.SPI_CPHA      = SPI_CPHA_1Edge;		//数据边沿   SPI_CPHA_1Edge,  SPI_CPHA_2Edge
	SPI_InitStructure.SPI_FirstBit  = SPI_MSB;				//移位方向   SPI_MSB, SPI_LSB
	SPI_InitStructure.SPI_Speed     = SPI_Speed_4;			//SPI速度    SPI_Speed_4, SPI_Speed_8, SPI_Speed_16, SPI_Speed_2
	SPI_InitStructure.SPI_SSIG      = DISABLE;				//片选位     ENABLE, DISABLE
    SPI_InitStructure.SPI_Enable    = ENABLE;				//SPI启动    ENABLE, DISABLE
	SPI_Init(&SPI_InitStructure);
	NVIC_SPI_Init(ENABLE,Priority_3);		//中断使能, ENABLE/DISABLE; 优先级(低到高) Priority_0,Priority_1,Priority_2,Priority_3
	
}


/**************************************************************
 *  函数名：void SPI_Write_Nbytes(u32 addr, u8 *buffer, u8 size)
 *  功  能：向设备某地址写数据
 *  入参数:
 *  addr   : 地址参数
 *  buffer : 缓冲需要写入设备的数据
 *  size   : 数据块大小
 *出参数: 无
 *************************************************************/
void SPI_Write_NByte(unsigned char addr, unsigned char* buffer, unsigned char sizel)
{
    SPI_SS_3  = 0;                       // enable device
    SPI_WriteByte(addr);                //发送写命令设置起始地址
    do{
        SPI_WriteByte(*buffer++);       //
        buffer++;
    }while(--sizel);
    SPI_SS_3  = 1;                        // disable device
}

/**************************************************************
 *  函数名：void SPI_Write_Nbytes(u32 addr, u8 *buffer, u8 size)写数据到设备中
 *  功  能：从设备某地址读取数据
 *  入参数:
 *  addr   : 地址参数
 *  buffer : 缓冲需要读取设备的数据
 *  size   : 数据块大小
 *出参数: 无
 *************************************************************/
void SPI_Read_NByte(unsigned char addr, unsigned char* buffer, unsigned int sizel)
{
    SPI_SS_3  = 0;                        //enable device
    SPI_WriteByte(addr);                 //发送读命令
    do{
        *buffer = SPI_ReadByte();       //receive byte and store at buffer
        buffer++;
    }while(--sizel);                     //read until no_bytes is reached
    SPI_SS_3  = 1;                        //disable device
}