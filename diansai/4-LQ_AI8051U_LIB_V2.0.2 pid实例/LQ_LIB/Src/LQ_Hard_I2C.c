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


#include "include.h"
#include "LQ_Hard_I2C.h"
#include "AI8051U_I2C.h"

 u16 IIC_Wait_time_out;
 //等待响应
 void Hard_IIC_Wait()
 {
 	while (!(I2CMSST & 0x40))
     {
        delay_us(1);
        if(IIC_Wait_time_out++>5000) break;  //5ms超时退出
     }
    IIC_Wait_time_out = 0;
 	I2CMSST &= ~0x40;
 }

 //起始信号
 void Hard_IIC_Start()
 {
 	I2CMSCR = 0x01; //发送 START 命令
 	Hard_IIC_Wait();
 }

 //发送数据
 void Hard_IIC_SendData(unsigned char dat)
 {
 	I2CTXD = dat; //写数据到数据缓冲区
 	I2CMSCR = 0x02; //发送 SEND 命令
 	Hard_IIC_Wait();
 }

 //等待响应
 void Hard_IIC_RecvACK()
 {
 	I2CMSCR = 0x03; //发送读 ACK 命令
 	Hard_IIC_Wait();
 }

 //接收数据
 unsigned char Hard_IIC_RecvData()
 {
 	I2CMSCR = 0x04; //发送 RECV 命令
 	Hard_IIC_Wait();
 	return I2CRXD;
 }

 //发送ACK
 void Hard_IIC_SendACK()
 {
 	I2CMSST = 0x00; //设置 ACK 信号
 	I2CMSCR = 0x05; //发送 ACK 命令
 	Hard_IIC_Wait();
 }

 //发送NACK
 void Hard_IIC_SendNAK()
 {
 	I2CMSST = 0x01; //设置 NAK 信号
 	I2CMSCR = 0x05; //发送 ACK 命令
 	Hard_IIC_Wait();
 }

 //IIC停止信号
 void Hard_IIC_Stop()
 {
 	I2CMSCR = 0x06; //发送 STOP 命令
 	Hard_IIC_Wait();
 }

//========================================================================
// 函数: void I2C_WriteNbyte(u8 dev_addr, u8 mem_addr, u8 *p, u8 number)
// 描述: I2C写入数据函数.
// 参数: dev_addr: 设备地址, mem_addr: 存储地址, *p写入数据存储位置, number写入数据个数.
// 返回: none.
// 版本: V1.0, 2020-09-15
//========================================================================
void Hard_I2C_WriteNbyte(u8 dev_addr, u8 mem_addr, u8 *p, u8 number)
{
	Start();			// 发送起始命令
	SendData(dev_addr); // 发送设备地址+写命令
	RecvACK();
	SendData(mem_addr); // 发送存储地址
	RecvACK();
	do
	{
		SendData(*p++);
		RecvACK();
	} while (--number);
	Stop(); // 发送停止命令
}

//========================================================================
// 函数: void I2C_ReadNbyte(u8 dev_addr, u8 mem_addr, u8 *p, u8 number)
// 描述: I2C读取数据函数.
// 参数: dev_addr: 设备地址, mem_addr: 存储地址, *p读取数据存储位置, number读取数据个数.
// 返回: none.
// 版本: V1.0, 2020-09-15
//========================================================================
void Hard_I2C_ReadNbyte(u8 dev_addr, u8 mem_addr, u8 *p, u8 number) 
{
	Start();			// 发送起始命令
	SendData(dev_addr); // 发送设备地址+写命令
	// RecvACK();
	SendData(mem_addr); // 发送存储地址
	// RecvACK();
	Start();				// 发送起始命令
	SendData(dev_addr | 1); // 发送设备地址+读命令
	// RecvACK();
	do
	{
		*p = RecvData();
		p++;
		if (number != 1)
			SendACK(); // send ACK
	} while (--number);
	SendNAK(); // send no ACK
	Stop();	   // 发送停止命令
}


/************************************************************
 * @Description: 初始化硬件IIC
 * @param   type:选择IIC引脚     tepy  SCL  SDA
                                （0：P2.4 P2.3）
                                （1：P1.5 P1.4）
                                （2：         ）
                                （3：P3.2 P3.3）
  * @see    void Hard_IIC_Init();
  *
  * @date  2024-09-15
*************************************************************/
void Hard_IIC_Init(unsigned char type)
{
    I2C_InitTypeDef I2C_InitStructure;
    
    if (type == 0) // 选择SCL:P2.4与SDA:P2.3为IIC引脚
    {
        I2C_SW(I2C_P24_P23);                    // 选择管脚 I2C_P23_P24
        GPIO_Init(GPIO_P2,GPIO_Pin_3 | GPIO_Pin_4,GPIO_Mode_Out_IN);// 将I2C管脚设为准双向口
    }
    else if (type == 1) // 选择P1.5与P1.4为IIC引脚
    {
        I2C_SW(I2C_P15_P14);
        GPIO_Init(GPIO_P1,GPIO_Pin_4 | GPIO_Pin_5,GPIO_Mode_Out_IN);
    }
#ifdef STC32G12K_32Bit
    else if (type == 2)
    {
        I2C_SW(I2C_P76_P77); // STC32G支持的管脚        // 8051U仅三组，I2C_S[1:0]  可选值0，1，3 无2
    }
#endif
    else if (type == 3) // 选择P3.2与P3.3为IIC引脚
    {
        I2C_SW(I2C_P32_P33); // I2C_P24_P23; I2C_P15_P14; I2C_P32_P33
        GPIO_Init(GPIO_P3,GPIO_Pin_2 | GPIO_Pin_3,GPIO_Mode_Out_IN);
    }

    I2C_InitStructure.I2C_Mode = I2C_Mode_Master; // 主从选择   I2C_Mode_Master, I2C_Mode_Slave
    I2C_InitStructure.I2C_Enable = ENABLE;        // I2C功能使能,   ENABLE, DISABLE
    I2C_InitStructure.I2C_MS_WDTA = ENABLE;      // 主机使能自动发送,  ENABLE, DISABLE
    I2C_InitStructure.I2C_Speed = 31;             // 总线速度=Fosc/2/(Speed*2+4),      0~63
    I2C_Init(&I2C_InitStructure);
    NVIC_I2C_Init(I2C_Mode_Master, DISABLE, Priority_0); // 主从模式, I2C_Mode_Master, I2C_Mode_Slave; 中断使能, ENABLE/DISABLE; 优先级(低到高) Priority_0,Priority_1,Priority_2,Priority_3
}
