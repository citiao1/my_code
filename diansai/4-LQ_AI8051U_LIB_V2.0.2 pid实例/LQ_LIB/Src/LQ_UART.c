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
#include "string.h"
#include "LQ_UART.h"

u8 xdata DMA_URT1_Buf[128];	   //收发缓存，同时使用多路串口时每个串口需分别定义缓存，以免相互干扰
u8 xdata DMA_URX1_Buf[128];

/******************** DMA 配置 ********************/
void UART1_DMA_config()
{
	DMA_UART_InitTypeDef		DMA_UART_InitStructure;		    //结构定义

	DMA_UART_InitStructure.DMA_TX_Length = 128;				    //DMA传输总字节数  	(0~65535) + 1
	DMA_UART_InitStructure.DMA_TX_Buffer = (u16)DMA_URT1_Buf;	//发送数据存储地址
	DMA_UART_InitStructure.DMA_RX_Length = 128;				    //DMA传输总字节数  	(0~65535) + 1
	DMA_UART_InitStructure.DMA_RX_Buffer = (u16)DMA_URX1_Buf;	//接收数据存储地址
	DMA_UART_InitStructure.DMA_TX_Enable = ENABLE;		        //DMA使能  	ENABLE,DISABLE
	DMA_UART_InitStructure.DMA_RX_Enable = ENABLE;		        //DMA使能  	ENABLE,DISABLE
	DMA_UART_Inilize(UART1, &DMA_UART_InitStructure);	        //初始化

	NVIC_DMA_UART1_Tx_Init(ENABLE,Priority_0,Priority_0);		//中断使能, ENABLE/DISABLE; 优先级(低到高) Priority_0~Priority_3; 总线优先级(低到高) Priority_0~Priority_3
	NVIC_DMA_UART1_Rx_Init(ENABLE,Priority_0,Priority_0);		//中断使能, ENABLE/DISABLE; 优先级(低到高) Priority_0~Priority_3; 总线优先级(低到高) Priority_0~Priority_3

	DMA_UR1R_CLRFIFO();		//清空 DMA FIFO
//    DMA_UR4R_CLRFIFO();		//清空 DMA FIFO
}

/**************************************************************************************
 * 函数：void UART_Init(UARTn_RxTx_Pin UARTx_Pin,u32 BaudRate)
 * 描述：UART初始化
 * 参数：UARTn_RxTx_Pin UARTx_Pin,u32 BaudRate--》串口号引脚，波特率
 * 返回：无
 * 方法：UART_Init(UART1_P30_P31, 115200ul);  //初始化串口1，其他参数配置详见如下
 * 注意：若切换串口默认波特率发生 定时器、中断优先级，请跳到函数内部具体位置修改。
 **************************************************************************************/
void UART_Init(UARTn_RxTx_Pin UARTx_Pin,u32 BaudRate)
{
    COMx_InitDefine		COMx_InitStructure;	    // 结构体定义

    const uart_port = (UARTx_Pin >> 12) & 0x0F; // 0x0 -> UART0// 提取UART编号
    const pin_group = (UARTx_Pin >> 8) & 0x0F;  // 0x1 -> 组1
    const rx_pin = UARTx_Pin & 0xFF;            // 0x30 -> 48(十进制)// 提取引脚号

    gpio_init_pin(rx_pin, GPIO_Mode_IPU);           // RX  若上拉，输入模式
    gpio_init_pin((rx_pin+1), GPIO_Mode_Out_PP);	// TX  推挽输出

    switch(uart_port)
    {
        case UART1:
        {
            UART1_SW(pin_group);
            //配置串口通信模式，中断优先级等
            COMx_InitStructure.UART_Mode      = UART_8bit_BRTx;		//模式,   UART_ShiftRight,UART_8bit_BRTx,UART_9bit,UART_9bit_BRTx
            COMx_InitStructure.UART_BRT_Use   = BRT_Timer2;			//选择波特率发生器, BRT_Timer2 (注意: 串口2固定使用BRT_Timer2)
            COMx_InitStructure.UART_BaudRate  = BaudRate;			//波特率,     110 ~ 115200
            COMx_InitStructure.UART_RxEnable  = ENABLE;				//接收允许,   ENABLE或DISABLE
            UART_Configuration(UART1, &COMx_InitStructure);		    //初始化串口 UART1
            NVIC_UART1_Init(ENABLE,Priority_1);		//中断使能, ENABLE/DISABLE; 优先级(低到高) Priority_0,Priority_1,Priority_2,Priority_3
        }break;

    //    case UART2:
    //    {
    //        UART2_SW(pin_group);
    //        //配置串口通信模式，中断优先级等
    //        COMx_InitStructure.UART_Mode      = UART_8bit_BRTx;		//模式,   UART_ShiftRight,UART_8bit_BRTx,UART_9bit,UART_9bit_BRTx
    //        COMx_InitStructure.UART_BRT_Use   = BRT_Timer2;			//选择波特率发生器, BRT_Timer2 (注意: 串口2固定使用BRT_Timer2)
    //        COMx_InitStructure.UART_BaudRate  = BaudRate;			//波特率,     110 ~ 115200
    //        COMx_InitStructure.UART_RxEnable  = ENABLE;				//接收允许,   ENABLE或DISABLE
    //        UART_Configuration(UART2, &COMx_InitStructure);		    //初始化串口 UART2
    //        NVIC_UART2_Init(ENABLE,Priority_1);		//中断使能, ENABLE/DISABLE; 优先级(低到高) Priority_0,Priority_1,Priority_2,Priority_3
    //    }break;

        case UART3:
        {
            UART3_SW(pin_group);
            //配置串口通信模式，中断优先级等
            COMx_InitStructure.UART_Mode      = UART_8bit_BRTx;		//模式,   UART_ShiftRight,UART_8bit_BRTx,UART_9bit,UART_9bit_BRTx
            COMx_InitStructure.UART_BRT_Use   = BRT_Timer3;			//选择波特率发生器, BRT_Timer2 (注意: 串口2固定使用BRT_Timer2)
            COMx_InitStructure.UART_BaudRate  = BaudRate;			//波特率,     110 ~ 115200
            COMx_InitStructure.UART_RxEnable  = ENABLE;				//接收允许,   ENABLE或DISABLE
            UART_Configuration(UART3, &COMx_InitStructure);		    //初始化串口 UART3
            NVIC_UART3_Init(ENABLE,Priority_1);		//中断使能, ENABLE/DISABLE; 优先级(低到高) Priority_0,Priority_1,Priority_2,Priority_3
        }break;

        case UART4:
        {
            UART4_SW(pin_group);
            //配置串口通信模式，中断优先级等
            COMx_InitStructure.UART_Mode      = UART_8bit_BRTx;		//模式,   UART_ShiftRight,UART_8bit_BRTx,UART_9bit,UART_9bit_BRTx
            COMx_InitStructure.UART_BRT_Use   = BRT_Timer2;			//选择波特率发生器, BRT_Timer2 (注意: 串口2固定使用BRT_Timer2)
            COMx_InitStructure.UART_BaudRate  = BaudRate;			//波特率,     110 ~ 115200
            COMx_InitStructure.UART_RxEnable  = ENABLE;				//接收允许,   ENABLE或DISABLE
            UART_Configuration(UART4, &COMx_InitStructure);		    //初始化串口UART4
            NVIC_UART4_Init(ENABLE,Priority_1);		//中断使能, ENABLE/DISABLE; 优先级(低到高) Priority_0,Priority_1,Priority_2,Priority_3
        }break;

       default: //非法字段
            break;

    }

}


/**************************************************************************************
 *函数: void Test_UART(void)
 *描述: 串口收发送测试程序.串口接收什么便回发什么
 *参数: None.
 *返回: None.
 *版本: V1.0, 2024-12-21
 *************************************************************************************/
void Test_UART_DMA()
{
 	u8	i,len;
	char strs[40]=("AI8051U UART DMA Test Programme!\r\n");  //UART发送一个字符串
    OLED_P6x8Str(19,2,"LQ OLED Test...");		//字符串显示
    UART_Init(UART1_P30_P31, 115200ul);//初始化串口1 波特率115200 8位数据,
    Global_IRQ_Enable();            //使能全局中断
	UART1_DMA_config();


	DmaTx1Flag = 0;
	DmaRx1Flag = 0;

	DMA_UR1R_TRIG();	//触发UART1接收功能

    len=strlen(strs);
    for (i = 0; i < len; i++)
    {
        DMA_URT1_Buf[i]=strs[i];
    }
	DMA_UR1T_TRIG();	//触发UART1发送功能,一次性发送字符串strs


	while (1)
	{
        DmaTx1Flag = 0;
        DmaRx1Flag = 0;

        DMA_UR1R_TRIG();	//触发UART1接收功能

        if(DmaRx1Flag)            //接收缓存满了溢出标志位1
        {
            len=strlen(DMA_URX1_Buf);
//            DMA_UR1R_CLRFIFO();
            for (i = 0; i < len; i++)
            {
                DMA_URT1_Buf[i]=DMA_URX1_Buf[i];
            }
            DMA_UR1T_TRIG();	//触发UART1发送功能
        }
	}

}





