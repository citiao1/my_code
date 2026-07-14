#include "LQ_STM_Timer.h"
#include "LQ_STM.h"

/*************************************************************************
 *  函数名称：void Test_STM(void)
 *  功能说明：测试程序
 *  参数说明：无
 *  函数返回：无
 *  备    注：核心板上的LED灯闪烁，中断时P10.5/P10.6闪灯
 *************************************************************************/
void Test_STM_Timer(void)
{
    uint32 time = 0;
    char txt[50];
    Display_Init(0);
    Delay_Ms(200);
    Display_CLS(U16_BLACK);

    GPIO_LED_Init(); // 初始化LED

    // 中断服务函数中翻转LED
    while (1)
    {
        time = STM_GetNowUs(STM0);
        Delay_Us(1000);
        LED_Ctrl(LEDALL, RVS);
        time = STM_GetNowUs(STM0) - time;

        sprintf(txt, "time:%07ldUs", time);                       // 将变量填充到字符串的对应位置，并将字符串存放到txt[]中
        Display_showString(0, 1, txt, U16_WHITE, U16_BLACK, 24); // 将txt中 内容显示出来
    }
}
