/*---------------------------------------------------------------------*/
/* ------------------- Web: www.STCAI.com -----------------------------*/
/*---------------------------------------------------------------------*/

#include "AI8051U_SPI.h"

//========================================================================
// 函数: SPI_ISR_Handler
// 描述: SPI中断函数.
// 参数: none.
// 返回: none.
// 版本: V1.0, 2020-09-23
//========================================================================
void SPI_ISR_Handler() interrupt SPI_VECTOR
{
    if(SPIF)
    {
        if(MSTR)	//主机模式
        {
            B_SPI_Busy = 0;
        }
        else        //从机模式
        {
            if(SPI_RxCnt >= SPI_BUF_LENTH) SPI_RxCnt = 0;
            SPI_RxBuffer[SPI_RxCnt++] = SPDAT;
        }
    }
    SPI_ClearFlag();	    //清 SPIF 和 WCOL 标志

    if(SPITOSR & 0x01)
    {
        SPI_RxTimerOut = 1;
        SPI_TOIFClear();     //设置 CTOCF 清除超时标志位 TOIF
    }
}
