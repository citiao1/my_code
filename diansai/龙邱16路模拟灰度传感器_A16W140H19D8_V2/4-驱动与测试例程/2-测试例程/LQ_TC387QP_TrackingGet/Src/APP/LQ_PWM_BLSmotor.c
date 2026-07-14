#include "LQ_PWM_BLSmotor.h"

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：void BLSmotorInit(void)
@功能说明：无感无刷初始化
@参数说明：无
@函数返回：无
@调用方法：BLSmotorInit();
@备    注：主函数调用
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
void BLSmotorInit(void)
{
    ATOM_PWM_InitConfig(ATOMBLS1, 500, Blsmotor_Frequency); // 无刷频率为50HZ，初始值为1ms
    ATOM_PWM_InitConfig(ATOMBLS2, 500, Blsmotor_Frequency); // 无刷理论范围为：1ms--2ms
    ATOM_PWM_InitConfig(ATOMBLS3, 500, Blsmotor_Frequency); // 无刷理论范围为：1ms--2ms
}
/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：void BlsmotorCtrl(Bls_e BLS, uint32 duty)
@功能说明：无感无刷控制函数
@参数说明：BLS:选择无刷1还是2  duty：舵机占空比
@函数返回：无
@调用方法：BlsmotorCtrl (Servo1,duty);
@备    注：
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
void BlsmotorCtrl(Bls_e BLS, uint32 duty)
{
    if (duty >= Blsmotor_Max) // 限制幅值
        duty = Blsmotor_Max;
    else if (duty <= Blsmotor_Min) // 限制幅值
        duty = Blsmotor_Min;
    switch (BLS)
    {
    case BLS1:
        ATOM_PWM_InitConfig(ATOMBLS1, duty, Blsmotor_Frequency); // 舵机频率为50HZ，初始值为1ms中值
        break;
    case BLS2:
        ATOM_PWM_InitConfig(ATOMBLS2, duty, Blsmotor_Frequency); // 无刷理论范围为：1ms--2ms
        break;
    case BLS3:
        ATOM_PWM_InitConfig(ATOMBLS3, duty, Blsmotor_Frequency); // 无刷理论范围为：1ms--2ms
        break;
    case BLSALL:
        ATOM_PWM_InitConfig(ATOMBLS1, duty, Blsmotor_Frequency); // 无刷理论范围为：1ms--2ms
        ATOM_PWM_InitConfig(ATOMBLS2, duty, Blsmotor_Frequency); // 无刷理论范围为：1ms--2ms
        ATOM_PWM_InitConfig(ATOMBLS3, duty, Blsmotor_Frequency); // 无刷理论范围为：1ms--2ms
        break;
    }
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：void TestBlsMotor(void)
@功能说明：无感无刷测试函数
@参数说明：无
@函数返回：无
@调用方法：TestBlsMotor();    BLS3 --》 P21_2
@备    注：
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
void Test_BlsMotor(void)
{
    char txt[16];
    signed short duty = 510;  
    GPIO_LED_Init();
    GPIO_KEY_Init();

    Display_Init(2);         // LCD初始化 
    Display_CLS(U16_BLACK);  // 黑色屏幕 
    Display_showString(0, 0, "   LQ BLSmotor Test      ", U16_RED, U16_BLACK, 16);
    BLSmotorInit();
    while (1)
    {
        if (!KEY_Read(KEY0))
        {
            if (duty > 490) // 防止duty超
            {
                duty -= 10;
            }
        }
        if (!KEY_Read(KEY1))
        {
            duty = 510;
        }
        if (!KEY_Read(KEY2))
        {
            if (duty < 1510) // 防止duty超
            {
                duty += 10;
            }
        }
        BlsmotorCtrl(BLS3, duty);
        sprintf(txt, "BLS duty:%04d ", duty);
        Display_showString(1, 2, txt, U16_BLACK, U16_GREEN, 16); // 显示出库实际脉冲数，以便灵活调整
        LED_Ctrl(LEDALL, RVS);                                   // 四个LED同时闪烁;
        Delay_Ms(100);
    }
}
