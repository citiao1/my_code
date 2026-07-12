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

#include "LQ_MotorServo.h"
#include "include.h"

s16 Motor_PWM_MAX = 0;   /* 在电机初始化后，自动计算最小，最大占空比范围*/
u16 BlSrevo_PWM_MAX = 0; /* 在舵机初始化后，自动计算最小，最大占空比范围*/
#ifdef USE_MIT7971

/**************************************************************************************
 * 函 数 名：void Motor_Init(u16 freq)
 * 功    能：电机PWM初始化 适用于7971驱动
 * 参    数：u16 freq：设置产生PWM的频率，也决定输出PWM的满占空比时间
 * 作    者：LQ008
 * 最后更新: 2024年11月17日
 * 软件版本: V1.0
 * 调用举例: MotorInit(12500)  //12.5KHZ   @45.1584MHz时最大占空比时间设置值0~3613
 *           PWM频率 (计算方式：Period或freq = (u16)(MAIN_Fosc/freq)
 **************************************************************************************/
void Motor_Init(u16 freq)
{
    PWMx_InitDefine PWMx_InitStructure;

    PWMx_Duty PWMB_Duty;
    
    u16 PWMx_Period = (u16)(MAIN_Fosc / freq);
    
    PWMB_Duty.PWM5_Duty = 0;
    PWMB_Duty.PWM6_Duty = 0;
    PWMB_Duty.PWM7_Duty = 0;
    PWMB_Duty.PWM8_Duty = 0;
    
    Motor_PWM_MAX = PWMx_Period;  // 最大值更新

    // PWM5设置
    PWMx_InitStructure.PWM_Mode = CCMRn_PWM_MODE1; // 模式,
    PWMx_InitStructure.PWM_Duty = 0;               // PWM占空比时间, 0~Period
    PWMx_InitStructure.PWM_EnoSelect = ENO5P;      // 输出通道选择
    PWM_Configuration(PWM5, &PWMx_InitStructure);  // 始化PWM5,  PWMB
    // PWM6设置
    PWMx_InitStructure.PWM_Mode = CCMRn_PWM_MODE1; // 模式,
    PWMx_InitStructure.PWM_Duty = 0;               // PWM占空比时间, 0~Period
    PWMx_InitStructure.PWM_EnoSelect = ENO6P;      // 输出通道选择,
    PWM_Configuration(PWM6, &PWMx_InitStructure);  // 始化PWM6,   PWMB
    // PWM7设置
    PWMx_InitStructure.PWM_Mode = CCMRn_PWM_MODE1; // 模式,
    PWMx_InitStructure.PWM_Duty = 0;               // PWM占空比时间, 0~Period
    PWMx_InitStructure.PWM_EnoSelect = ENO7P;      // 输出通道选择,
    PWM_Configuration(PWM7, &PWMx_InitStructure);  // 始化PWM7,   PWMB
    // PWM8设置
    PWMx_InitStructure.PWM_Mode = CCMRn_PWM_MODE1; // 模式,
    PWMx_InitStructure.PWM_Duty = 0;               // PWM占空比时间, 0~Period
    PWMx_InitStructure.PWM_EnoSelect = ENO8P;      // 输出通道选择,
    PWM_Configuration(PWM8, &PWMx_InitStructure);  // 始化PWM8,  PWMB
    // PWMB 通道5~8设置
    PWMx_InitStructure.PWM_Period = PWMx_Period;   // 周期时间,  0~Period   (计算方式：Period = MAIN_Fosc/freq)
    PWMx_InitStructure.PWM_DeadTime = 0;           // 区发生器设置, 0~255
    PWMx_InitStructure.PWM_MainOutEnable = ENABLE; // 输出使能, ENABLE,DISABLE
    PWMx_InitStructure.PWM_CEN_Enable = ENABLE;    // 能计数器, ENABLE,DISABLE
    PWM_Configuration(PWMB, &PWMx_InitStructure);  // 始化PWM通用寄存器,  PWMA,PWMB

    // PWMB 通道5-8 输出管脚选择
    PWM5_USE_P50();
    PWM6_USE_P51();
    PWM7_USE_P52();
    PWM8_USE_P53();

    // PWM输出管脚设置（启动PWM功能后输出脚自动设置为推挽输出模式 AI8051U需要手动设置推挽输出）
    GPIO_Init(GPIO_P5, GPIO_Pin_LOW, GPIO_Mode_Out_PP); // P50~P53四个管脚PWM输出模式设为 推挽输出
    UpdatePwm(PWMB, &PWMB_Duty);
}

/*********************************************************************************
 * 函 数 名：void Motor_Ctrl(int16 Motor1, int16 Motor2)
 * 功    能：7971双路电机控制函数
 * 参 数 值：Motor1：电机1占空比；Motor2：电机2占空比
 * 返 回 值：无
 * 作    者：LQ008
 * 最后更新：2024年11月17日
 * 软件版本：V1.0
 * 调用举例：MotorCtrl (1000,-2000); //两个电机一正一反转动
 **********************************************************************************/
void Motor_Ctrl(int16 Motor1, int16 Motor2)
{
    if (Motor1 < 0)
    {
        Motor1 = 0 - Motor1;
        UpdatePwmCh(PWM5, 0);
        UpdatePwmCh(PWM6, Motor1);
    }
    else
    {
        UpdatePwmCh(PWM5, Motor1);
        UpdatePwmCh(PWM6, 0);
    }

    if (Motor2 < 0)
    {
        Motor2 = 0 - Motor2;
        UpdatePwmCh(PWM7, 0);
        UpdatePwmCh(PWM8, Motor2);
    }
    else
    {
        UpdatePwmCh(PWM7, Motor2);
        UpdatePwmCh(PWM8, 0);
    }
}

/*  USE7971*/

#elif defined USE_DRV8701
/**************************************************************************************
 * 函 数 名：void Motor_Init(u16 freq)
 * 功    能：电机PWM初始化   适用于8701驱动
 * 返 回 值：无
 * 作    者：LQ008
 * 最后更新: 2024年11月17日
 * 软件版本: V1.0
 * 调用举例: MotorInit();  //12.5KHz @45.1584MHz时最大占空比时间设置值为0~3613
 *           PWM频率 计算方式：Period = (u16)(MAIN_Fosc/freq)  自适应主频
 **************************************************************************************/
void Motor_Init(u16 freq)
{
    PWMx_InitDefine PWMx_InitStructure;

    u16 PWMx_Period = (u16)(MAIN_Fosc / freq);
    Motor_PWM_MAX = PWMx_Period;
    // PWMB_CH5设置
    PWMx_InitStructure.PWM_Mode = CCMRn_PWM_MODE1; // 模式,
    PWMx_InitStructure.PWM_Duty = 0;               // PWM占空比时间, 0~Period
    PWMx_InitStructure.PWM_EnoSelect = ENO5P;      // 输出通道选择
    PWM_Configuration(PWM5, &PWMx_InitStructure);  // 始化PWM5,  PWMB
    // PWMB_CH7设置
    PWMx_InitStructure.PWM_Mode = CCMRn_PWM_MODE1; // 模式,
    PWMx_InitStructure.PWM_Duty = 0;               // PWM占空比时间, 0~Period
    PWMx_InitStructure.PWM_EnoSelect = ENO7P;      // 输出通道选择
    PWM_Configuration(PWM7, &PWMx_InitStructure);  // 始化PWM7,   PWMB

    // PWMA 通道1~4设置
    PWMx_InitStructure.PWM_Period = PWMx_Period;   // 周期时间,  0~Period   (计算方式：Period = MAIN_Fosc/freq)
    PWMx_InitStructure.PWM_DeadTime = 0;           // 区发生器设置, 0~255
    PWMx_InitStructure.PWM_MainOutEnable = ENABLE; // 输出使能, ENABLE,DISABLE
    PWMx_InitStructure.PWM_CEN_Enable = ENABLE;    // 能计数器, ENABLE,DISABLE
    PWM_Configuration(PWMB, &PWMx_InitStructure);  // 始化PWM通用寄存器,  PWMA,PWMB

#ifdef AI8051U_32Bit // 如果是STC32,选择PWM管脚后 需要对应修改GIO初始化，以下主要针对AI8051U，使用STC32G的自行修改
    // PWMB 通道5-8 输出管脚选择
    PWM5_USE_P50();
    //    PWM6_USE_P51();       //设为IO,做8701方向控制IO1
    PWM7_USE_P52();
//    PWM8_USE_P53();       //设为IO,做8701方向控制IO2
#endif

    GPIO_Init(GPIO_P5, GPIO_Pin_LOW, GPIO_Mode_Out_PP); // P50/P52管脚PWM输出模式设为 推挽输出.8701方向控制IO-P51/P53,推挽输出
    gpio_write_pin(Motor_Dir1, LOW);                    // 电机1方向初始化
    gpio_write_pin(Motor_Dir2, LOW);                    // 电机2方向初始化

    UpdatePwmCh(PWM5, 0); // 要更新的通道以及占空比时间
    UpdatePwmCh(PWM7, 0); // 要更新的通道以及占空比时间
}

/**************************************************************************************
 * 函 数 名：void Motor_Ctrl(int16 Motor1,int16 Motor2)
 * 功    能：LQDRV8701双路电机控制函数
 * 参    数：Motor1：电机1占空比；Motor2：电机2占空比
 * 返 回 值：无
 * 作    者：LQ008
 * 最后更新: 2024年11月17日
 * 软件版本: V1.0
 * 调用举例: MotorInit(12500)  //12.5KHz @45.1584MHz
 *           PWM频率 计算方式：Period = (u16)(MAIN_Fosc/freq)
 **************************************************************************************/
void Motor_Ctrl(int16 Motor1, int16 Motor2)
{
    if (abs(Motor1) > Motor_PWM_MAX)   return;  // 范围超出直接退出
    if (abs(Motor2) > Motor_PWM_MAX)   return;  // 范围超出直接退出

    if (Motor1 < 0)
    {
        Motor1 = 0 - Motor1;
        M1_DIR_PIN1 = HIGH;
        UpdatePwmCh(PWM5, Motor1); // 0~Motor_PWM_MAX
    }
    else
    {
        M1_DIR_PIN1 = LOW;
        UpdatePwmCh(PWM5, Motor1); // 0~Motor_PWM_MAX
    }

    if (Motor2 < 0)
    {
        Motor2 = 0 - Motor2;
        M1_DIR_PIN2 = HIGH;
        UpdatePwmCh(PWM7, Motor2);
    }
    else
    {
        M1_DIR_PIN2 = LOW;
        UpdatePwmCh(PWM7, Motor2);
    }
}

#endif /*  USE8701*/

/**********************************************************************************
 * 函 数 名：void Servo_Init(u16 freq)
 * 功    能：舵机PWM初始化
 * 参 数 值：舵机PWM的频率  一般50或100 Hz
 * 返 回 值：无
 * 作    者：chiusir
 * 最后更新：2024年11月17日
 * 软件版本：V1.0
 * 调用举例：驱动舵机 ServoInit(100);  //100Hz
 *           PWM频率 计算方式：Period = (u16)(MAIN_Fosc/freq)  自适应主频
 ***********************************************************************************/
void Servo_Init(u16 freq)
{
    PWMx_InitDefine PWMx_InitStructure;
    /* 注意这里的管脚选择与输出使能，PWA具有互补输出功能，因此用不到互补管脚输出，则需要ENOxN禁止输出*/

    PWMA_Prescaler((MAIN_Fosc / 1000000));     // PWM时钟 预分频器设置 PWM分频到1M范围
    // PWM1设置
    PWMx_InitStructure.PWM_Mode = CCMRn_PWM_MODE1; // 模式
    PWMx_InitStructure.PWM_Duty = 0;               // PWM占空比时间, 0~Period
    PWMx_InitStructure.PWM_EnoSelect = ENO1P;      // 输出通道选择
    PWM_Configuration(PWM1, &PWMx_InitStructure);  // 初始化PWM1,  PWMA
    // PWM2设置
    PWMx_InitStructure.PWM_Mode = CCMRn_PWM_MODE1; // 模式
    PWMx_InitStructure.PWM_Duty = 0;               // PWM占空比时间, 0~Period
    PWMx_InitStructure.PWM_EnoSelect = ENO2P;      // 输出通道选择
    PWM_Configuration(PWM2, &PWMx_InitStructure);  // 初始化PWM2,  PWMA

    // PWMA 通道1~4设置
    PWMx_InitStructure.PWM_Period = (u16)(1000000 / freq); // 周期时间,   0~Period
    PWMx_InitStructure.PWM_DeadTime = 0;                   // 死区发生器设置, 0~255
    PWMx_InitStructure.PWM_MainOutEnable = ENABLE;         // 主输出使能, ENABLE,DISABLE
    PWMx_InitStructure.PWM_CEN_Enable = ENABLE;            // 使能计数器, ENABLE,DISABLE
    PWM_Configuration(PWMA, &PWMx_InitStructure);          // 初始化PWM通用寄存器,  PWMA

    // PWMA 通道1-4 输出管脚选择
    PWM1_USE_P10P11(); // 仅使能P10输出即可，不需要互补管脚P11的输出，互补管脚还可做其他普通IO使用
    PWM2_USE_P12P13();
    // PWM输出管脚设置（启动PWM功能后输出脚自动设置为推挽输出模式 AI8051U需要手动设置推挽输出）
    GPIO_Init(GPIO_P1, GPIO_Pin_0 | GPIO_Pin_2, GPIO_Mode_Out_PP); // P10/P12管脚PWM输出模式设为 推挽输出
    UpdatePwmCh(PWM1, 0);                                          // 要更新的通道以及占空比时间
    UpdatePwmCh(PWM2, 0);
}
/**********************************************************************************
 * 函 数 名：void Servo_Ctrl (u16 duty)
 * 功    能：舵机转角函数，由于小车拉杆范围限制，较小
 * 参 数 值：short Sduty，占空比，范围0--10000；
 * 作    者：LQ008
 * 最后更新：2024年11月17日
 * 软件版本：V1.0
 * 调用举例：ServoCtrl(1500);//舵机中值
 **********************************************************************************/
void Servo_Ctrl(u8 Sx, u16 Sduty)
{
    if (Sduty <= Servo_Median - 1000) // 限制幅值,根据实际情况进行调整
    {
        Sduty = Servo_Median - 1000;
    }
    else if (Sduty >= Servo_Median + 1000) // 限制幅值
    {
        Sduty = Servo_Median + 1000;
    }
    if (Servo1 == Sx)
        UpdatePwmCh(PWM1, Sduty);
    if (Servo1 == Sx)
        UpdatePwmCh(PWM2, Sduty);
}

/******************************************************************************************
 * 函 数 名：void BLmotor_Init(u16 freq)
 * 功    能：无刷电机控制引脚初始化，频率需要和舵机控制频率保持一致，推荐50Hz
 * 参 数 值：u16 freq：无感无刷PWM输出的频率，范围40--300，建议50Hz
 * 返 回 值：无
 * 作    者：LQ008
 * 最后更新：2024年11月17日
 * 软件版本：V1.0
 * 调用举例：BLmotor_Init(50);  //初始化P20 P22引脚为PWM，频率50Hz,满占空比20000
 ******************************************************************************************/
void BLmotor_Init(u16 freq)
{
    PWMx_InitDefine PWMx_InitStructure;
    /* 注意这里的管脚选择与输出使能，PWA具有互补输出功能，因此用不到互补管脚输出，则需要ENOxN禁止输出*/
    PWMA_Prescaler((MAIN_Fosc / 1000000 - 1)); // PWM时钟 预分频器设置 PWM分频到1M范围

    PWMx_InitStructure.PWM_Mode = CCMRn_PWM_MODE1; // 模式,
    PWMx_InitStructure.PWM_Duty = 0;               // PWM占空比时间, 0~Period
    PWMx_InitStructure.PWM_EnoSelect = ENO3P;      // 输出通道选择
    PWM_Configuration(PWM3, &PWMx_InitStructure);  // 初始化PWM3,  PWMA
    // PWM4设置
    PWMx_InitStructure.PWM_Mode = CCMRn_PWM_MODE1; // 模式
    PWMx_InitStructure.PWM_Duty = 0;               // PWM占空比时间, 0~Period
    PWMx_InitStructure.PWM_EnoSelect = ENO4P;      // 输出通道选择
    PWM_Configuration(PWM4, &PWMx_InitStructure);  // 初始化PWM4,  PWMA
    // PWMA 通道1~4设置
    PWMx_InitStructure.PWM_Period = 1000000 / freq; // 周期时间,   0~Period，最高可设置65535
    PWMx_InitStructure.PWM_DeadTime = 0;            // 死区发生器设置, 0~255
    PWMx_InitStructure.PWM_MainOutEnable = ENABLE;  // 主输出使能, ENABLE,DISABLE
    PWMx_InitStructure.PWM_CEN_Enable = ENABLE;     // 使能计数器, ENABLE,DISABLE
    PWM_Configuration(PWMA, &PWMx_InitStructure);   // 初始化PWM通用寄存器,  PWMA

    // PWMA 通道1-4 输出管脚选择
    PWM3_USE_P14P15();
    PWM4_USE_P16P17();

    GPIO_Init(GPIO_P1, GPIO_Pin_4 | GPIO_Pin_6, GPIO_Mode_Out_PP); // P14/P16管脚PWM输出模式设为 推挽输出
    UpdatePwmCh(PWM3, 0);                                          // 要更新的通道以及占空比时间,初始化后设为0
    UpdatePwmCh(PWM4, 0);
}

/******************************************************************************************
 * 函 数 名：void BLmotor_Ctrl(u16 BLmotor1,u16 BLmotor2)
 * 功    能：BLmotor 无感无刷电调控制程序，当PWM高电平时间在1-2ms区间内，无刷电机旋转，2ms时转速最高
 * 参 数 值：u16 BLmotor1，占空比，范围0--10000；
 * 作    者：LQ008
 * 最后更新：2024年11月17日
 * 软件版本：V1.0
 * 调用举例：BLmotor_Ctrl(1000，1000);//当频率为50时，设置高电平时间为1ms
 ******************************************************************************************/
void BLmotor_Ctrl(u16 BLmotor1, u16 BLmotor2)
{
    if (BLmotor1 <= 900) // 限制幅值,根据实际情况进行调整
    {
        BLmotor1 = 900;
    }
    else if (BLmotor2 >= 1990) // 限制幅值
    {
        BLmotor2 = 1990;
    }
    UpdatePwmCh(PWM3, BLmotor1);
    UpdatePwmCh(PWM4, BLmotor2);
}

/******************************************************************************************
 * 函 数 名：void BLmotor_Init_1(u16 freq)   
 * 功    能：无刷电机控制引脚初始化，适用于一体板P23引脚
 ******************************************************************************************/
void BLmotor_Init_1(u16 freq)
{
    PWMx_InitDefine PWMx_InitStructure;
    /* 注意这里的管脚选择与输出使能，PWA具有互补输出功能，因此用不到互补管脚输出，则需要ENOxN禁止输出*/
    PWMA_Prescaler((MAIN_Fosc / 1000000 - 1)); // PWM时钟 预分频器设置 PWM分频到1M范围

    PWMx_InitStructure.PWM_Mode = CCMRn_PWM_MODE1; // 模式,
    PWMx_InitStructure.PWM_Duty = 0;               // PWM占空比时间, 0~Period
    PWMx_InitStructure.PWM_EnoSelect = ENO2N;      // 输出通道选择 PWM2_N通道 P23
    PWM_Configuration(PWM2, &PWMx_InitStructure);  // 初始化PWM2,  PWMA

    // PWMA 通道
    PWMx_InitStructure.PWM_Period = 1000000 / freq; // 周期时间,   0~Period，最高可设置65535
    PWMx_InitStructure.PWM_DeadTime = 0;            // 死区发生器设置, 0~255
    PWMx_InitStructure.PWM_MainOutEnable = ENABLE; // 主输出使能, ENABLE,DISABLE
    PWMx_InitStructure.PWM_CEN_Enable = ENABLE;     // 使能计数器, ENABLE,DISABLE
    PWM_Configuration(PWMA, &PWMx_InitStructure);   // 初始化PWM通用寄存器,  PWMA

    // PWMA 通道 输出管脚选择
    PWM2_USE_P22P23();   // 仅使用P23

    GPIO_Init(GPIO_P2, GPIO_Pin_3, GPIO_Mode_Out_PP); // P23管脚PWM输出模式设为 推挽输出
    UpdatePwmCh(PWM2, 0);                                          // 要更新的通道以及占空比时间,初始化后设为0
}

/******************************************************************************************
 * 函 数 名：void BLmotor_Ctrl_w1(u16 BLmotor_duty)
 * 功    能：BLmotor 无感无刷电调控制程序 (单通道安全版)
 ******************************************************************************************/
void BLmotor_Ctrl_w1(u16 BLmotor_duty)
{
    // 完美的限幅保护，防止传入错误数据导致电调死机或风扇狂转
    if (BLmotor_duty < 900)  
    {
        BLmotor_duty = 900;
    }
    if (BLmotor_duty > 2000) 
    {
        BLmotor_duty = 2000;
    }
    UpdatePwmCh(PWM2, BLmotor_duty);
}
