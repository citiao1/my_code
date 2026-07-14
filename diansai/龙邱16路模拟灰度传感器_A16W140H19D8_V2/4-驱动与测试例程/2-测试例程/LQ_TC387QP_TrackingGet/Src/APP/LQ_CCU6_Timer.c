#include "LQ_CCU6_Timer.h"
#include "LQ_CCU6.h"

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@void LQ_CCU6_Timer (void)
@功能说明：测试程序
@参数说明：无
@函数返回：无
@备   注：核心板上的LED灯闪烁，中断时P10.5/P10.6闪灯
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
void Test_CCU6_Timer(void)
{
    GPIO_LED_Init();
    CCU6_InitConfig(CCU60, CCU6_Channel0, 1000 * 1000); // CCU6初始化 LED0

    CCU6_InitConfig(CCU61, CCU6_Channel0, 500 * 1000); // CCU6初始化

    // 中断服务函数中翻转LED
    while (1)
    {
        ;
    }
}
