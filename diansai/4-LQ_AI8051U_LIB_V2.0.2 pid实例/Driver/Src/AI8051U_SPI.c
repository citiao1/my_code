/*---------------------------------------------------------------------*/
/* --- Web: www.STCAI.com ---------------------------------------------*/
/*---------------------------------------------------------------------*/

#include "AI8051U_SPI.h"

bit B_SPI_Busy; //发送忙标志
bit SPI_RxTimerOut;
bit SPI_TimerOutEn;
u8 	SPI_RxCnt;
u8  SPI_BUF_type SPI_RxBuffer[SPI_BUF_LENTH];

//========================================================================
// 函数: void SPI_Init(SPI_InitTypeDef *SPIx)
// 描述: SPI初始化程序.
// 参数: SPIx: 结构参数,请参考spi.h里的定义.
// 返回: none.
// 版本: V1.0, 2012-11-22
//========================================================================
void SPI_Init(SPI_InitTypeDef *SPIx)
{
    if(SPIx->SPI_SSIG == ENABLE) SSIG = 1;     //conform Master or Slave by SPI_Mode(ignore SS)
    else                         SSIG = 0;     //conform Master or Slave by SS pin.
    SPI_Start(SPIx->SPI_Enable);
    SPI_FirstBit_Set(SPIx->SPI_FirstBit);
    SPI_Mode_Set(SPIx->SPI_Mode);
    SPI_CPOL_Set(SPIx->SPI_CPOL);
    SPI_CPHA_Set(SPIx->SPI_CPHA);
    SPI_Clock_Select(SPIx->SPI_Speed);

    if(SPIx->TimeOutScale == TO_SCALE_SYSCLK)   SPITOCR |= 0x20;  //超时计数时钟源：系统时钟
    else
    {
        SET_TPS();          //设置系统等待时间单元，用于控制EEPROM操作、SPI/I2C超时时间以及休眠唤醒等待时间
        SPITOCR &= ~0x20;   //超时计数时钟源：1us时钟(1MHz时钟)
    }

    if((SPIx->TimeOutTimer > 0) && (SPIx->TimeOutTimer <= 0xffffff))
    {
        SPITOTL = (u8)SPIx->TimeOutTimer;
        SPITOTH = (u8)(SPIx->TimeOutTimer>>8);
        SPITOTE = (u8)(SPIx->TimeOutTimer>>16); //写 SPITOTE 后，新的TM值才会生效
    }
    
    if((SPIx->SPI_Mode == SPI_Mode_Slave) && (SPIx->TimeOutEnable == ENABLE))
    {
        SPI_TimerOutEn = 1;
        SPITOCR |= 0x80;    //从机接收超时功能使能
    }
    else
    {
        SPI_TimerOutEn = 0;
        SPITOCR &= ~0x80;   //从机接收超时功能禁止
    }

    if(SPIx->TimeOutINTEnable == ENABLE) SPITOCR |= 0x40;   //从机接收超时中断使能
    else                                 SPITOCR &= ~0x40;  //从机接收超时中断禁止

    SPI_RxTimerOut = 0;
    B_SPI_Busy = 0;
    SPI_RxCnt = 0;
}

//========================================================================
// 函数: void SPI_SetMode(u8 mode)
// 描述: SPI设置主从模式函数.
// 参数: mode: 指定模式, 取值 SPI_Mode_Master 或 SPI_Mode_Slave.
// 返回: none.
// 版本: V1.0, 2012-11-22
//========================================================================
void SPI_SetMode(u8 mode)
{
    if(mode == SPI_Mode_Slave)
    {
        MSTR = 0;     //重新设置为从机待机
        SSIG = 0;     //SS引脚确定主从
        if(SPI_TimerOutEn)
        {
            SPI_TOIFClear();
            SPITOCR |= 0x80;    //从机接收超时功能使能
        }
    }
    else
    {
        SPITOCR &= ~0x80;   //从机接收超时功能禁止
        MSTR = 1;     //使能 SPI 主机模式
        SSIG = 1;     //忽略SS引脚功能
    }
}

//========================================================================
// 函数: void SPI_WriteByte(u8 dat)
// 描述: SPI发送一个字节数据.
// 参数: dat: 要发送的数据.
// 返回: none.
// 版本: V1.0, 2020-09-14
//========================================================================
void SPI_WriteByte(u8 dat)  //SPI发送一个字节数据
{
    if(ESPI)
    {
        B_SPI_Busy = 1;
        SPDAT = dat;
        while(B_SPI_Busy);  //中断模式
    }
    else
    {
        SPDAT = dat;
        while(SPIF == 0);   //查询模式
        SPI_ClearFlag();    //清除SPIF和WCOL标志
    }
}

//========================================================================
// 函数: void SPI_ReadByte(u8 dat)
// 描述: SPI查询模式读取一个字节数据.
// 参数: none.
// 返回: 读取的数据.
// 版本: V1.0, 2020-09-14
//========================================================================
u8 SPI_ReadByte(void)
{
    SPDAT = 0xff;
    if(ESPI)
    {
        B_SPI_Busy = 1;
        while(B_SPI_Busy);  //中断模式
    }
    else
    {
        while(SPIF == 0);   //查询模式
        SPI_ClearFlag();     //清除SPIF和WCOL标志
    }
    return (SPDAT);
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
void SPI_Write_NByte(u8 addr, u8* buffer, u8 sizel)
{
//    SPI_SS_3  = 0;                       // enable device
    SPI_WriteByte(addr);                //发送写命令设置起始地址
    do{
        SPI_WriteByte(*buffer++);       //
        buffer++;
    }while(--sizel);
//    SPI_SS_3  = 1;                    // disable device
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
void SPI_Read_NByte(u8 addr, u8* buffer, u8 sizel)
{
//    SPI_SS_3  = 0;                     //enable device
    SPI_WriteByte(addr|0x80);                //发送读命令
    do{
        *buffer = SPI_ReadByte();       //receive byte and store at buffer
        buffer++;
    }while(--sizel);                   //read until no_bytes is reached
//    SPI_SS_3  = 1;                    //disable device
}


