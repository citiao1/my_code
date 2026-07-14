#include <stdio.h>
#include "LQ_Encoder.h"
#include "LQ_GPT12_ENC.h"
#include "LQ_STM.h"
#include "LQ_UART.h"
#include "LQ_GPIO_LED.h"
#include "LQ_OLED096.h"
#include "LQ_TFT18.h"

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：void EncInit (void)
@功能说明：编码器初始化
@参数说明：无
@函数返回：无
@调用方法：EncInit();
@备    注：
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
void EncInit(void)
{
    ENC_InitConfig(ENC1_InPut_P33_7, ENC1_Dir_P33_6);
    ENC_InitConfig(ENC2_InPut_P02_8, ENC2_Dir_P33_5);
//    ENC_InitConfig(ENC3_InPut_P10_3, ENC3_Dir_P10_1);
    ENC_InitConfig(ENC4_InPut_P20_3, ENC4_Dir_P20_0);
}

void Test_Encoder(void)
{
    char txt[32];
    sint16 ECPULSE1 = 0;
    sint16 ECPULSE2 = 0;
//    sint16 ECPULSE3 = 0;
    sint16 ECPULSE4 = 0;
    GPIO_LED_Init();
    GPIO_KEY_Init();
    Display_Init(0);         // LCD初始化
    Display_CLS(U16_BLACK); // 蓝色屏幕
    Display_CLS(U16_BLACK); // 清屏
    Display_showString(0, 0, "Test Encoder", U16_RED, U16_BLACK, 16);

    EncInit();
    while (1)
    {
        /* 获取编码器值 */
        ECPULSE1 = ENC_GetCounter(ENC1_InPut_P33_7);
        ECPULSE2 = ENC_GetCounter(ENC2_InPut_P02_8);
//        ECPULSE3 = ENC_GetCounter(ENC3_InPut_P10_3);
        ECPULSE4 = ENC_GetCounter(ENC4_InPut_P20_3);
        sprintf(txt, "Enc1: %05d;", ECPULSE1);
        Display_showString(0, 3, txt, U16_WHITE, U16_BLACK, 16); // 字符串显示
        sprintf(txt, "Enc2: %05d;", ECPULSE2);
        Display_showString(0, 4, txt, U16_WHITE, U16_BLACK, 16); // 字符串显示
//        sprintf(txt, "Enc3: %05d;", ECPULSE3);
//        Display_showString(0, 5, txt, U16_WHITE, U16_BLACK, 16); // 字符串显示
        sprintf(txt, "Enc4: %05d;", ECPULSE4);
        Display_showString(0, 6, txt, U16_WHITE, U16_BLACK, 16); // 字符串显示

        LED_Ctrl(LED0, RVS); // 电平翻转,LED闪烁
        Delay_Ms(200);       // 延时等待
    }
}
