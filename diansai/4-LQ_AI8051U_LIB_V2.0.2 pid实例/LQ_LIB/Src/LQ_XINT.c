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


/***********************************************************************************
 * 函 数 名：void Exti_Init(void)
 * 功    能：STC传统外部中断初始化，管脚初始化和中断模式、使能等设置
 * 参    数：EXT_INTx，外部中断号
 * 返 回 值：无
 * 说    明：INT脚位于P3口：EXT_INT0_P32、EXT_INT1_P33、EXT_INT2_P36、EXT_INT3_P37
 *           其中：EXT_INT2、EXT_INT3、EXT_INT4仅支持下降沿触发中断
 **********************************************************************************/
void Exti_Init(u8 EXT_INTx, u8 Priority_n)
{
	EXTI_InitTypeDef	Exti_InitStructure;		//结构定义
    //中断IO配置
    switch(EXT_INTx)
    {
        case EXT_INT0_P32:
        {
            //INT0中断管脚设置
            gpio_init_pin(P3_2,GPIO_Mode_IPU);	            //设置P3.2为上拉输入模式
            //中断模式初始化配置
            Exti_InitStructure.EXTI_Mode = EXT_MODE_Fall;   //中断模式设为下降沿触发, EXT_MODE_RiseFall,EXT_MODE_Fall
            Ext_Inilize(EXT_INT0_P32,&Exti_InitStructure);  //初始化，EXT_INT0连接外部P3.2管脚
            NVIC_INT0_Init(ENABLE,Priority_n);              //中断使能, ENABLE/DISABLE; 优先级(低到高) Priority_0~Priority_3/NULL:无优先级
        }break;
        case EXT_INT1_P33:
        {
            //INT1中断管脚设置
            gpio_init_pin(P3_3,GPIO_Mode_IPU);	            //设置P3.3为上拉输入模式
            //中断模式初始化配置
            Exti_InitStructure.EXTI_Mode = EXT_MODE_Fall;   //中断模式设为下降沿触发, EXT_MODE_RiseFall,EXT_MODE_Fall
            Ext_Inilize(EXT_INT1_P33,&Exti_InitStructure);  //初始化，  EXT_INT1连接外部P3.3管脚
            NVIC_INT1_Init(ENABLE,Priority_n);              //中断使能, ENABLE/DISABLE; 优先级(低到高) Priority_0~Priority_3/NULL:无优先级
        }break;
        case EXT_INT2_P36:
        {
            //INT1中断管脚设置
            gpio_init_pin(P3_6,GPIO_Mode_IPU);	            //设置P3.6为上拉输入模式
            //中断初始化配置
            Ext_Inilize(EXT_INT2_P36,&Exti_InitStructure);  //仅进入函数清标志位，使能，默认下降沿触发，用户不可配置
            NVIC_INT2_Init(ENABLE,Priority_n);              //中断使能, ENABLE/DISABLE; 优先级(低到高)  Priority_0~Priority_3/NULL:无优先级
        }break;
        case EXT_INT3_P37:
        {
            //INT1中断管脚设置
            gpio_init_pin(P3_7,GPIO_Mode_IPU);	            //设置P3.7为上拉输入模式
            //中断初始化配置
            Ext_Inilize(EXT_INT3_P37,&Exti_InitStructure);  //初始化仅进入函数清标志位，使能，默认下降沿触发，用户不可配置
            NVIC_INT3_Init(ENABLE,Priority_n);              //中断使能, ENABLE/DISABLE; 优先级(低到高)  Priority_0~Priority_3/NULL:无优先级
        }break;
        case EXT_INT4_P30:
        {
            //INT1中断管脚设置
            gpio_init_pin(P3_0,GPIO_Mode_IPU);	            //设置P3.0为上拉输入模式
            //中断初始化配置
            Ext_Inilize(EXT_INT4_P30,&Exti_InitStructure);  //初始化仅进入函数清标志位，使能，默认下降沿触发，用户不可配置
            NVIC_INT4_Init(ENABLE,Priority_n);              //中断使能, ENABLE/DISABLE; 优先级(低到高)  Priority_0~Priority_3/NULL:无优先级
        }break;
        default: //非法字段
            break;
    }
}


//----------------------- 以下为 IO中断初始化相关内容 ，所有管脚皆可中断-------------------------

//  使用时可以注释掉用不到的，节省空间，若时STC32G12K128则无需担心空间问题


/***********************************************************************************
 * 函 数 名：u8 GPIO_INT_Config(u8 GPIO, GPIO_InitTypeDef *GPIOx)
 * 功    能：初始化GPIO 端口中断,精确到每个IO口，初始化后立即使能，eg: P0INTE  的7个位对应P0.0到P0.7
 * 参    数：GPIO端口，I/O结构体（管脚和中断模式配置）
 * 返 回 值：无
 * 时    间：2024-11-18
 * 软件版本：V1.0
 **********************************************************************************/
u8 GPIO_INT_Config(u8 GPIO, GPIO_InitTypeDef *GPIOx)
{
	if(GPIO > GPIO_P7)	return FAIL;	                //错误
	if(GPIOx->Mode > PxINT_MODE_HIGH)	return FAIL;	//错误
	if(GPIO == GPIO_P0)
	{
		if(GPIOx->Mode == PxINT_MODE_Fall)		{P0IM1 &= ~(GPIOx->Pin),   P0IM0 &= ~(GPIOx->Pin);}	 //下降沿中断
		if(GPIOx->Mode == PxINT_MODE_Rise)		{P0IM1 &= ~(GPIOx->Pin),   P0IM0 |=  (GPIOx->Pin);}	 //上升沿中断
		if(GPIOx->Mode == PxINT_MODE_LOW)		{P0IM1 |=  (GPIOx->Pin),   P0IM0 &= ~(GPIOx->Pin);}	 //低电平中断
		if(GPIOx->Mode == PxINT_MODE_HIGH)		{P0IM1 |=  (GPIOx->Pin),   P0IM0 |=  (GPIOx->Pin);}	 //高点平中断
        P0INTE |= GPIOx->Pin;        //使能P0.Pin中断
    }
	if(GPIO == GPIO_P1)
	{
		if(GPIOx->Mode == PxINT_MODE_Fall)		{P1IM1 &= ~(GPIOx->Pin),   P1IM0 &= ~(GPIOx->Pin);}	 //下降沿中断
		if(GPIOx->Mode == PxINT_MODE_Rise)		{P1IM1 &= ~(GPIOx->Pin),   P1IM0 |=  (GPIOx->Pin);}	 //上升沿中断
		if(GPIOx->Mode == PxINT_MODE_LOW)		{P1IM1 |=  (GPIOx->Pin),   P1IM0 &= ~(GPIOx->Pin);}	 //低电平中断
		if(GPIOx->Mode == PxINT_MODE_HIGH)		{P1IM1 |=  (GPIOx->Pin),   P1IM0 |=  (GPIOx->Pin);}	 //高点平中断
        P1INTE |= GPIOx->Pin;        //使能P1.Pin中断
	}
	if(GPIO == GPIO_P2)
	{
		if(GPIOx->Mode == PxINT_MODE_Fall)		{P2IM1 &= ~(GPIOx->Pin),   P2IM0 &= ~(GPIOx->Pin);}	 //下降沿中断
		if(GPIOx->Mode == PxINT_MODE_Rise)		{P2IM1 &= ~(GPIOx->Pin),   P2IM0 |=  (GPIOx->Pin);}	 //上升沿中断
		if(GPIOx->Mode == PxINT_MODE_LOW)		{P2IM1 |=  (GPIOx->Pin),   P2IM0 &= ~(GPIOx->Pin);}	 //低电平中断
		if(GPIOx->Mode == PxINT_MODE_HIGH)		{P2IM1 |=  (GPIOx->Pin),   P2IM0 |=  (GPIOx->Pin);}	 //高点平中断
        P2INTE |= GPIOx->Pin;        //使能P2.Pin中断
	}
	if(GPIO == GPIO_P3)
	{
		if(GPIOx->Mode == PxINT_MODE_Fall)		{P3IM1 &= ~(GPIOx->Pin),   P3IM0 &= ~(GPIOx->Pin);}	 //下降沿中断
		if(GPIOx->Mode == PxINT_MODE_Rise)		{P3IM1 &= ~(GPIOx->Pin),   P3IM0 |=  (GPIOx->Pin);}	 //上升沿中断
		if(GPIOx->Mode == PxINT_MODE_LOW)		{P3IM1 |=  (GPIOx->Pin),   P3IM0 &= ~(GPIOx->Pin);}	 //低电平中断
		if(GPIOx->Mode == PxINT_MODE_HIGH)		{P3IM1 |=  (GPIOx->Pin),   P3IM0 |=  (GPIOx->Pin);}	 //高点平中断
        P3INTE |= GPIOx->Pin;        //使能P3.Pin中断
	}
	if(GPIO == GPIO_P4)
	{
		if(GPIOx->Mode == PxINT_MODE_Fall)		{P4IM1 &= ~(GPIOx->Pin),   P4IM0 &= ~(GPIOx->Pin);}	 //下降沿中断
		if(GPIOx->Mode == PxINT_MODE_Rise)		{P4IM1 &= ~(GPIOx->Pin),   P4IM0 |=  (GPIOx->Pin);}	 //上升沿中断
		if(GPIOx->Mode == PxINT_MODE_LOW)		{P4IM1 |=  (GPIOx->Pin),   P4IM0 &= ~(GPIOx->Pin);}	 //低电平中断
		if(GPIOx->Mode == PxINT_MODE_HIGH)		{P4IM1 |=  (GPIOx->Pin),   P4IM0 |=  (GPIOx->Pin);}	 //高点平中断
        P4INTE |= GPIOx->Pin;        //使能P4.Pin中断
	}
	if(GPIO == GPIO_P5)
	{
        if(GPIOx->Pin > 6||GPIOx->Pin > 7) return FAIL;  //AI8051U 没有P5.6  P5.7
		if(GPIOx->Mode == PxINT_MODE_Fall)		{P5IM1 &= ~(GPIOx->Pin),   P5IM0 &= ~(GPIOx->Pin);}	 //下降沿中断
		if(GPIOx->Mode == PxINT_MODE_Rise)		{P5IM1 &= ~(GPIOx->Pin),   P5IM0 |=  (GPIOx->Pin);}	 //上升沿中断
		if(GPIOx->Mode == PxINT_MODE_LOW)		{P5IM1 |=  (GPIOx->Pin),   P5IM0 &= ~(GPIOx->Pin);}	 //低电平中断
		if(GPIOx->Mode == PxINT_MODE_HIGH)		{P5IM1 |=  (GPIOx->Pin),   P5IM0 |=  (GPIOx->Pin);}	 //高点平中断
        P5INTE |= GPIOx->Pin;        //使能P5.Pin中断
	}
   	if(GPIO == GPIO_P6)
	{
		if(GPIOx->Mode == PxINT_MODE_Fall)		{P6IM1 &= ~(GPIOx->Pin),   P6IM0 &= ~(GPIOx->Pin);}	 //下降沿中断
		if(GPIOx->Mode == PxINT_MODE_Rise)		{P6IM1 &= ~(GPIOx->Pin),   P6IM0 |=  (GPIOx->Pin);}	 //上升沿中断
		if(GPIOx->Mode == PxINT_MODE_LOW)		{P6IM1 |=  (GPIOx->Pin),   P6IM0 &= ~(GPIOx->Pin);}	 //低电平中断
		if(GPIOx->Mode == PxINT_MODE_HIGH)		{P6IM1 |=  (GPIOx->Pin),   P6IM0 |=  (GPIOx->Pin);}	 //高点平中断
        P6INTE |= GPIOx->Pin;        //使能P6.Pin中断
	}
    if(GPIO == GPIO_P7)
    {
        if(GPIOx->Mode == PxINT_MODE_Fall)		{P7IM1 &= ~(GPIOx->Pin),   P7IM0 &= ~(GPIOx->Pin);}	 //下降沿中断
        if(GPIOx->Mode == PxINT_MODE_Rise)		{P7IM1 &= ~(GPIOx->Pin),   P7IM0 |=  (GPIOx->Pin);}	 //上升沿中断
        if(GPIOx->Mode == PxINT_MODE_LOW)		{P7IM1 |=  (GPIOx->Pin),   P7IM0 &= ~(GPIOx->Pin);}	 //低电平中断
        if(GPIOx->Mode == PxINT_MODE_HIGH)		{P7IM1 |=  (GPIOx->Pin),   P7IM0 |=  (GPIOx->Pin);}	 //高点平中断
        P7INTE |= GPIOx->Pin;        //使能P7.Pin中断
    }
    return SUCCESS;	        //成功
}


/***********************************************************************************
 * 函 数 名: NVIC_PxINT_Init
 * 描    述: PxINT嵌套向量中断控制器初始化.
 * 参    数: Priority: 中断优先级, Priority_0,Priority_1,Priority_2,Priority_3.
 * 返    回: 执行结果 SUCCESS/FAIL.
 * 版    本: V1.0, 2024-11-20
 **********************************************************************************/
u8 NVIC_PxINT_Init(u8 GPIO_Px, u8 Priority)
{
    if(GPIO_Px > GPIO_P7)	  return FAIL;
    if(Priority > Priority_3) return FAIL;
    //Px端口的中断优先级

    if(0 == Priority) PINIPH &= ~(0x01<<GPIO_Px), PINIPL &= ~(0x01<<GPIO_Px);
    if(1 == Priority) PINIPH &= ~(0x01<<GPIO_Px), PINIPL |=  (0x01<<GPIO_Px);
	if(2 == Priority) PINIPH |=  (0x01<<GPIO_Px), PINIPL &= ~(0x01<<GPIO_Px);
	if(3 == Priority) PINIPH |=  (0x01<<GPIO_Px), PINIPL |=  (0x01<<GPIO_Px);

	return SUCCESS;
}


/***********************************************************************************
 * 函 数 名：GPIO_PxINT_Init(Pin_list Pinx, GPIO_INT_Mode_e INT_Mode,u8 Priority_n)
 * 功    能：IO端口中断，初始化，设置中断IO，以及中断模式、端口优先级等
 * 参    数：Pinx - 要初始化的IO端口；INT_Mode - 中断模式；Priority_n - 中断优先级
 * 返 回 值：无
 * 说    明：AI8051U或STC32G支持所有普通I/O可中断，切记需要手动清除中断标志位（详见中断服务函数）
 * 例    子：GPIO_PxINT_Init(P0_0,PxINT_MODE_Fall,Priority_0); //P0.0 下降沿触发中断，优先级为0
 **********************************************************************************/
void GPIO_PxINT_Init(Pin_list Pinx, GPIO_INT_Mode_e INT_Mode,u8 Priority_n)
{
    GPIO_InitTypeDef  GPIO_PxINT_Structure;
    u8  GPIO_Port = GPIO_Get_Port(Pinx);
    switch (INT_Mode)
    {
    case PxINT_MODE_Fall:gpio_init_pin(Pinx,GPIO_Mode_IPU);   //下降沿触发中断,IO初始化为上拉输入模式
        break;
    case PxINT_MODE_Rise:gpio_init_pin(Pinx,GPIO_Mode_IPD);   //上升沿触发中断,IO初始化为下拉输入模式
        break;
    case PxINT_MODE_LOW:gpio_init_pin(Pinx,GPIO_Mode_IPU);    //低电平触发中断,IO初始化为上拉输入模式
        break;
    case PxINT_MODE_HIGH:gpio_init_pin(Pinx,GPIO_Mode_IPD);   //高电平触发中断,IO初始化为下拉输入模式
        break;
    }
    GPIO_PxINT_Structure.Pin  = (0x01 << GPIO_Get_Pin(Pinx));                 //要初始化的I/O  eg: GPIO_Pin_0|GPIO_Pin_2;
    GPIO_PxINT_Structure.Mode = INT_Mode;                 //初始化的I/O 模式 下降沿触发中断
    GPIO_INT_Config(GPIO_Port,&GPIO_PxINT_Structure);      //初始化管脚中断模式行 并 >使能< 其管脚中断！！！
    NVIC_PxINT_Init(GPIO_Port,Priority_n);                //中断优先级配置,对象是端口 PO_INT_Priority(Priority_0)；
}


