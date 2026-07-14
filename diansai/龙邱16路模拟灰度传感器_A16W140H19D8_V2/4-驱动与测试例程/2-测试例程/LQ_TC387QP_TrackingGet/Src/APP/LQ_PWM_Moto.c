#include "LQ_PWM_Moto.h"


/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：void MotorInit(void)
@功能说明：电机PWM初始化
@参数说明：无
@函数返回：无
@调用方法：MotorInit(100);
@备    注：驱动4个电机
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
void MotorInit(void)
{
    ATOM_PWM_InitConfig(MOTOR1_P, 0, MOTOR_FREQUENCY);
    ATOM_PWM_InitConfig(MOTOR2_P, 0, MOTOR_FREQUENCY);
    ATOM_PWM_InitConfig(MOTOR3_P, 0, MOTOR_FREQUENCY);
    ATOM_PWM_InitConfig(MOTOR4_P, 0, MOTOR_FREQUENCY);

    PIN_InitConfig(MOTOR1_I, PIN_MODE_OUTPUT, 0);
    PIN_InitConfig(MOTOR2_I, PIN_MODE_OUTPUT, 0);
    PIN_InitConfig(MOTOR3_I, PIN_MODE_OUTPUT, 0);
    PIN_InitConfig(MOTOR4_I, PIN_MODE_OUTPUT, 0);
}

#ifdef USE7843or7971
void MotorCtrL(Motor_e Motor, sint32 motor)
{
    switch (Motor)
    {
    case Motor1:
        if (motor > 0)
        {
            ATOM_PWM_SetDuty(MOTOR1_P, motor, MOTOR_FREQUENCY);
            PIN_Write(MOTOR1_I, 0);
        }
        else
        {
            ATOM_PWM_SetDuty(MOTOR1_P, (ATOM_PWM_MAX + motor), MOTOR_FREQUENCY);
            PIN_Write(MOTOR1_I, 1);
        }
        break;
    case Motor2:
        if (motor > 0)
        {
            ATOM_PWM_SetDuty(MOTOR2_P, motor, MOTOR_FREQUENCY);
            PIN_Write(MOTOR2_I, 0);
        }
        else
        {
            ATOM_PWM_SetDuty(MOTOR2_P, (ATOM_PWM_MAX + motor), MOTOR_FREQUENCY);
            PIN_Write(MOTOR2_I, 1);
        }
        break;
    case Motor3:
        if (motor > 0)
        {
            ATOM_PWM_SetDuty(MOTOR3_P, motor, MOTOR_FREQUENCY);
            PIN_Write(MOTOR3_I, 0);
        }
        else
        {
            ATOM_PWM_SetDuty(MOTOR3_P, (ATOM_PWM_MAX + motor), MOTOR_FREQUENCY);
            PIN_Write(MOTOR3_I, 1);
        }
        break;
    case Motor4:
        if (motor > 0)
        {
            ATOM_PWM_SetDuty(MOTOR4_P, motor, MOTOR_FREQUENCY);
            PIN_Write(MOTOR4_I, 0);
        }
        else
        {
            ATOM_PWM_SetDuty(MOTOR4_P, (ATOM_PWM_MAX + motor), MOTOR_FREQUENCY);
            PIN_Write(MOTOR4_I, 1);
        }
        break;
    }
}

#else // USEDRV8701

void MotorCtrL(Motor_e Motor, sint32 motor)
{
    switch (Motor)
    {
    case Motor1:
        if (motor > 0)
        {
            ATOM_PWM_SetDuty(MOTOR1_P, motor, MOTOR_FREQUENCY);
            PIN_Write(MOTOR1_I, 0);
        }
        else
        {
            ATOM_PWM_SetDuty(MOTOR1_P, (0 - motor), MOTOR_FREQUENCY);
            PIN_Write(MOTOR1_I, 1);
        }
        break;
    case Motor2:
        if (motor > 0)
        {
            ATOM_PWM_SetDuty(MOTOR2_P, motor, MOTOR_FREQUENCY);
            PIN_Write(MOTOR2_I, 0);
        }
        else
        {
            ATOM_PWM_SetDuty(MOTOR2_P, (0 - motor), MOTOR_FREQUENCY);
            PIN_Write(MOTOR2_I, 1);
        }
        break;
    case Motor3:
        if (motor > 0)
        {
            ATOM_PWM_SetDuty(MOTOR3_P, motor, MOTOR_FREQUENCY);
            PIN_Write(MOTOR3_I, 0);
        }
        else
        {
            ATOM_PWM_SetDuty(MOTOR3_P, (0 - motor), MOTOR_FREQUENCY);
            PIN_Write(MOTOR3_I, 1);
        }
        break;
    case Motor4:
        if (motor > 0)
        {
            ATOM_PWM_SetDuty(MOTOR4_P, motor, MOTOR_FREQUENCY);
            PIN_Write(MOTOR4_I, 0);
        }
        else
        {
            ATOM_PWM_SetDuty(MOTOR4_P, (0 - motor), MOTOR_FREQUENCY);
            PIN_Write(MOTOR4_I, 1);
        }
        break;
    }
}
#endif
void Test_Motor(void)
{
    short duty = 1500;
    char txt[16];
    GPIO_LED_Init();
    GPIO_KEY_Init();
    MotorInit();
    Display_Init(0);         // LCD初始化
    Display_CLS(U16_BLACK); // 蓝色屏幕
    Display_CLS(U16_BLACK); // 清屏
    Display_showString(0, 0, "   LQ Motor Test      ", U16_WHITE, U16_BLACK, 16);

    while (1)
    {
//        if (KEY_Read(KEY0) == 0) // 按下KEY0键，占空比减小
//        {
//            if (duty > -ATOM_PWM_MAX)
//                duty -= 100;
//        }
//        if (KEY_Read(KEY2) == 0) // 按下KEY2键，占空比加大
//        {
//            if (duty < ATOM_PWM_MAX) // 满占空比为12500
//                duty += 100;
//        }
//        if (KEY_Read(KEY1) == 0) // 按下KEY1键，占空比中值
//        {
//            duty = 1500;
//        }
for(duty=0;duty<10000;duty+=50){
        sprintf(txt, "PWM: %05d;", duty);
        printf(txt);
        Display_showString(0, 5, txt, U16_WHITE, U16_BLACK, 16); // 字符串显示
//        MotorCtrL(Motor1, duty);
//        MotorCtrL(Motor2, duty);
        MotorCtrL(Motor3, duty);
        MotorCtrL(Motor4, duty);

        LED_Ctrl(LED0, RVS); // 电平翻转,LED闪烁
        Delay_Ms(100);       // 延时等待
    }
for(duty=0;duty>-10000;duty-=50){
        sprintf(txt, "PWM: %05d;", duty);
        printf(txt);
        Display_showString(0, 5, txt, U16_WHITE, U16_BLACK, 16); // 字符串显示
//        MotorCtrL(Motor1, duty);
//        MotorCtrL(Motor2, duty);
        MotorCtrL(Motor3, duty);
        MotorCtrL(Motor4, duty);

        LED_Ctrl(LED0, RVS); // 电平翻转,LED闪烁
        Delay_Ms(100);       // 延时等待
    }
    }
}
