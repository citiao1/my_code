
#include "LQ_SBUS.h"
#include <stdio.h>
#include "include.h"

/***********************************************************
*函 数 名:UpDate_SBUS
*设计时间: 2023年10月12日
*作    者:  LQ008
*功能说明: 解析HotRC DS600 遥控器SUBS模块信息
***********************************************************/




//typedef struct
//{
/*                通道值             键名          通道号       默认值范围  */
    signed int rc_RLR  = 0;     //摇杆左右       CH1          432  992  1552
    signed int rc_RUD  = 0;     //左手上下       CH2          432  992  1552
    signed int rc_LUD  = 0;     //左手上下       CH3          432  992  1552
    signed int rc_LLR  = 0;     //左手左右       CH4          432  992  1552
    signed int rc_SW5  = 0;     //三档拨杆5      CH5          192  992  1792
    signed int rc_SW6  = 0;     //两档拨杆6      CH6          192↓ -   1792↑
    signed int rc_SW7  = 0;     //两档拨杆7      CH7          192   -   1792
    signed int rc_SW8  = 0;     //三档拨杆8      CH8          192  992  1792    
//}hotRC_data;

/* - - - - */                          //****     CHn
unsigned char SReadBuff[64];


unsigned int Status_flag    = 0;     //状态标志  
unsigned char sbus_Rec_Fini_Flag = 0;  //接收状态控制
//unsigned char sbus_receive_flag[5] = {0x00, 0x00, 0x03, 0x00, 0x0f};
unsigned char receive_flag = 0;

/***********************************************************************
 * 函数名：void UpDate_SBUS()
 * 功  能：SBUS数据解析
 * 返回值：无
 *********************************************************************/
void UpDate_SBUS()
{
//    CHDATA[0]= ReadBuff4[0];
    rc_RLR= ((SReadBuff[1] | SReadBuff[2] << 8 ) & 0x07FF); 
    rc_RUD= ((SReadBuff[2] >> 3 | SReadBuff[3] << 5 ) & 0x07FF);
    rc_LUD= (SReadBuff[3] >> 6 | SReadBuff[4] << 2 | SReadBuff[5] << 10 ) & 0x07FF;
    rc_LLR= (SReadBuff[5] >> 1 | SReadBuff[6] << 7 ) & 0x07FF;
    rc_SW5= (SReadBuff[6] >> 4 | SReadBuff[7] << 4 ) & 0x07FF;
    rc_SW6= (SReadBuff[7] >> 7 | SReadBuff[8] << 1 | SReadBuff[9] << 9 ) & 0x07FF; 
    rc_SW7= (SReadBuff[9] >> 2 | SReadBuff[10]<< 6) & 0x07FF;
    rc_SW8= (SReadBuff[10]>> 5 | SReadBuff[11]<< 3) & 0x07FF;
    
    receive_flag = 1;
    // Status_flag=(ReadBuff4[23] | ReadBuff4[24] << 8 ) & 0x07FF;

//		Usart3_Rec_Fini_Flag = 0;
	
	
//ReadBuff4[23]为遥控器状态标志,失控时 ReadBuff4[23]=0x0f，正常时 tmpbuf[23]=0x03

}


/**********************************************串口3中断服务函数中的代码，配合上面测试函数，以防丢失 在这里备份****************************************/
//u8 num3=0;
//void SUBS_RX_IRQHandler()
//{

//    unsigned char buf;

//    /* 用户代码 */
//    buf = UART_GetChar(USART_3);
//    if(Usart3_Rec_Fini_Flag == 1)       //可以接收
//    {
//        if (num3 == 0 && buf == 0x0F)   //帧头
//        {
//            ReadBuff4[num3] = buf;
//            num3++;
////            LED_Ctrl(LED0, RVS);        // LED闪烁 指示程序运行状态
//        }
//        else if(num3 != 0)
//        {
////            LED_Ctrl(LED0, RVS);        // LED闪烁 指示程序运行状态
//            ReadBuff4[num3] = buf;
//            num3++;
//            if (num3 > 24)              //接收数据超过24，表示一帧接收完成
//            {
//                Usart3_Rec_Fini_Flag = 0;   //停止接收

//                if (ReadBuff4[0] == 0x0F && ReadBuff4[24] == 0x00 && num3 == 25)    //接受完一帧数据
//                {
//                    UpDate_SBUS();      //数据解析
//                }
//                num3 = 0;               //清空
//            }

//        }
//    }
//}
/********************************************** 串口4中断服务函数中的代码，配合上面测试函数，以防丢失 在这里备份****************************************/

























