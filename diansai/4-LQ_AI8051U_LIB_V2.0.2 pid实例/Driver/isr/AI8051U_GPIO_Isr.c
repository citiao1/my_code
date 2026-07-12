/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
 【平    台】北京龙邱智能科技STC32位核心板
 【编    写】龙邱科技
 【E-mail  】chiusir@163.com
 【软件版本】V1.1 版权所有，单位使用请先联系授权
 【最后更新】2024年11月8日
 【相关信息参考下列地址】
 【网    站】http://www.lqist.cn
 【淘宝店铺】http://longqiu.taobao.com
 --------------------------------------------------------------------------------
 【  IDE  】 keil C251 5.60.0
 【Target 】 STC32G/STC8051U/AI8051U 32位模式
 【SYS CLK】 42 MHz使用内部晶振
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/

#include "include.h"

//========================================================================
// 函数: void P0xINT_ISR_Handler
// 描述: GPIO P0端口中断函数.
// 参数: none.
// 返回: none.
// 版本: V1.0, 2024-11-18
//      如果if判断，将中断服务函数写在其中，不美观，可单独设立一个函数，来写中断服务内容，在进中断后调用
//========================================================================
void P0INT_ISR_Handler (void) interrupt P0INT_VECTOR		//进中断后手动经清除标志位
{
    u8 P0x_IRQ;
    P0x_IRQ = P0INTF;       //P0端口中断标志寄存器
    if(P0x_IRQ)
    {
        P0INTF = 0x00;      //清除P0端口中断标志位
        if(P0x_IRQ&GPIO_Pin_0)    /* P0.0中断服务*/
        {
            // TODO: 在此处添加用户代码
//            LED_Ctrl(LED0,RVS);
//            OLED_P6x8Str(5,3, "P0_INT ->P0.0");
        }

        if(P0x_IRQ&GPIO_Pin_3)    /* P0.3中断服务*/
        {
            // TODO: 在此处添加用户代码
//            LED_Ctrl(LED0,RVS);
//            OLED_P6x8Str(5,3, "P0_INT ->P0.3");
        }
/*  还可以继续添加其他管脚的中断服务程序  */
    }
}


//========================================================================
// 函数: void P1INT_ISR_Handler
// 描述: GPIO P1端口中断函数.
// 参数: none.
// 返回: none.
// 版本: V1.0, 2024-11-18
//========================================================================
void P1INT_ISR_Handler (void) interrupt P1INT_VECTOR		//进中断后手动经清除标志位
{
    u8 P1x_IRQ;
    P1x_IRQ = P2INTF;
    if(P1x_IRQ)
    {
        P1INTF = 0x00;      //清除P1端口中断标志位
        if(P1x_IRQ&GPIO_Pin_0)    /* P1.0中断服务*/
        {
            // TODO: 在此处添加用户代码
//            LED_Ctrl(LED0,RVS);
        }
/*  还可以继续添加其他管脚的中断服务程序  */
    }
}


//========================================================================
// 函数: void P2INT_ISR_Handler
// 描述: GPIO P2端口中断函数.
// 参数: none.
// 返回: none.
// 版本: V1.0, 2024-11-18
//========================================================================
//void P2INT_ISR_Handler (void) interrupt P2INT_VECTOR		//进中断后手动经清除标志位
//{
//    u8 P2x_IRQ;
//    P2x_IRQ = P2INTF;
//    if(P2x_IRQ)
//    {
//        P2INTF = 0x00;      //清除P2端口中断标志位
//        if(P2x_IRQ&GPIO_Pin_0)    /* P2.0中断服务*/
//        {
//            // TODO: 在此处添加用户代码
////            LED_Ctrl(LED0,RVS);
//        }
///*  还可以继续添加其他管脚的中断服务程序  */

//    }
//}

//========================================================================
// 函数: void P3INT_ISR_Handler
// 描述: GPIO P3端口中断函数.
// 参数: none.
// 返回: none.
// 版本: V1.0, 2024-11-18
//========================================================================
//void P3INT_ISR_Handler (void) interrupt P3INT_VECTOR		//进中断后手动经清除标志位
//{
//    u8 P3x_IRQ;
//    P3x_IRQ = P3INTF;
//    if(P3x_IRQ)
//    {
//        P3INTF = 0x00;      //清除P3端口中断标志位
//        if(P3x_IRQ&GPIO_Pin_0)    /* P3.0中断服务*/
//        {
//            // TODO: 在此处添加用户代码
////            LED_Ctrl(LED0,RVS);
//        }
/*  还可以继续添加其他管脚的中断服务程序  */
//    }
//}

//========================================================================
// 函数: void P4INT_ISR_Handler
// 描述: GPIO P4端口中断函数.
// 参数: none.
// 返回: none.
// 版本: V1.0, 2024-11-18
//========================================================================
//void P4INT_ISR_Handler (void) interrupt P4INT_VECTOR		//进中断后手动经清除标志位
//{
//    u8 P4x_IRQ;
//    P4x_IRQ = P4INTF;
//    if(P4x_IRQ)
//    {
//        P4INTF = 0x00;      //清除P4端口中断标志位
//        if(P4x_IRQ&GPIO_Pin_0)    /* P4.0中断服务*/
//        {
//            // TODO: 在此处添加用户代码
////            LED_Ctrl(LED0,RVS);
//        }
/*  还可以继续添加其他管脚的中断服务程序  */
//    }
//}

//========================================================================
// 函数: void P5INT_ISR_Handler
// 描述: GPIO P5端口中断函数.
// 参数: none.
// 返回: none.
// 版本: V1.0, 2024-11-18
//========================================================================
//void P5INT_ISR_Handler (void) interrupt P5INT_VECTOR		//进中断后手动经清除标志位
//{
//    u8 P5x_IRQ;
//    P5x_IRQ = P5INTF;
//    if(P5x_IRQ)
//    {
//        P7INTF = 0x00;      //清除P5端口中断标志位
//        if(P5x_IRQ&GPIO_Pin_0)    /* P5.0中断服务*/
//        {
//            // TODO: 在此处添加用户代码
////            LED_Ctrl(LED0,RVS);
//        }
/*  还可以继续添加其他管脚的中断服务程序  */

//    }
//}

//========================================================================
// 函数: void P6INT_ISR_Handler
// 描述: GPIO P6端口中断函数.
// 参数: none.
// 返回: none.
// 版本: V1.0, 2024-11-18
//========================================================================
//void P6INT_ISR_Handler (void) interrupt P6INT_VECTOR		//进中断后手动经清除标志位
//{
//    u8 P6x_IRQ;
//    P6x_IRQ = P6INTF;
//    if(P6x_IRQ)
//    {
//        P7INTF = 0x00;      //清除P6端口中断标志位
//        if(P6x_IRQ&GPIO_Pin_0)    /* P6.0中断服务*/
//        {
//            // TODO: 在此处添加用户代码
////            LED_Ctrl(LED0,RVS);
//        }
/*  还可以继续添加其他管脚的中断服务程序  */

//    }
//}

//========================================================================
// 函数: void P7xINT_ISR_Handler
// 描述: GPIO P7端口中断函数.
// 参数: none.
// 返回: none.
// 版本: V1.0, 2024-11-18
//========================================================================
//void P7INT_ISR_Handler (void) interrupt P7INT_VECTOR		//进中断后手动经清除标志位
//{
//    u8 P7x_IRQ;
//    P7x_IRQ = P7INTF;
//    if(P7x_IRQ)
//    {
//        P7INTF = 0x00;      //清除P7端口中断标志位
//        if(P7x_IRQ&GPIO_Pin_0)    /* P7.0中断服务*/
//        {
//            // TODO: 在此处添加用户代码
////            LED_Ctrl(LED0,RVS);
//        }

//    }
//}

