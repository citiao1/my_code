/*---------------------------------------------------------------------*/
/* ------------------- Web: www.STCAI.com -----------------------------*/
/*---------------------------------------------------------------------*/

#include	"AI8051U_GPIO.h"

//========================================================================
// 函数: void GPIO_Config(GPIO_port GPIO, GPIO_InitTypeDef *GPIOx)
// 描述: 初始化IO口.
// 参数: GPIOx: 结构参数,请参考头文件里的定义.
// 版本: V1.0, 2012-10-22
//========================================================================
void GPIO_Config(GPIO_port GPIO, GPIO_InitTypeDef *GPIOx)
{
	if(GPIO > GPIO_P7)	return;	//错误
	if(GPIO == GPIO_P0)
	{
		if(GPIOx->Mode == GPIO_Mode_Out_IN)         P0M0 &= ~GPIOx->Pin, P0M1 &= ~GPIOx->Pin;   // 上拉准双向口   00
        if(GPIOx->Mode == GPIO_Mode_IN_FLOATING)    P0M0 &= ~GPIOx->Pin, P0M1 |=  GPIOx->Pin;   // 高阻浮空输入   01
		if(GPIOx->Mode == GPIO_Mode_Out_PP)         P0M0 |=  GPIOx->Pin, P0M1 &= ~GPIOx->Pin;   // 推挽输出       10
		if(GPIOx->Mode == GPIO_Mode_Out_OD)         P0M0 |=  GPIOx->Pin, P0M1 |=  GPIOx->Pin;   // 开漏输出       11
	}
    if(GPIO == GPIO_P1)
	{
        if(GPIOx->Mode == GPIO_Mode_Out_IN)         P1M0 &= ~GPIOx->Pin, P1M1 &= ~GPIOx->Pin;
        if(GPIOx->Mode == GPIO_Mode_IN_FLOATING)    P1M0 &= ~GPIOx->Pin, P1M1 |=  GPIOx->Pin;
        if(GPIOx->Mode == GPIO_Mode_Out_PP)         P1M0 |=  GPIOx->Pin, P1M1 &= ~GPIOx->Pin;
        if(GPIOx->Mode == GPIO_Mode_Out_OD)         P1M0 |=  GPIOx->Pin, P1M1 |=  GPIOx->Pin;
	}
    if(GPIO == GPIO_P2)
    {
		if(GPIOx->Mode == GPIO_Mode_Out_IN)         P2M0 &= ~GPIOx->Pin, P2M1 &= ~GPIOx->Pin;
        if(GPIOx->Mode == GPIO_Mode_IN_FLOATING)    P2M0 &= ~GPIOx->Pin, P2M1 |=  GPIOx->Pin;
		if(GPIOx->Mode == GPIO_Mode_Out_PP)         P2M0 |=  GPIOx->Pin, P2M1 &= ~GPIOx->Pin;
		if(GPIOx->Mode == GPIO_Mode_Out_OD)         P2M0 |=  GPIOx->Pin, P2M1 |=  GPIOx->Pin;
    }
    if(GPIO == GPIO_P3)
    {
		if(GPIOx->Mode == GPIO_Mode_Out_IN)         P3M0 &= ~GPIOx->Pin, P3M1 &= ~GPIOx->Pin;
        if(GPIOx->Mode == GPIO_Mode_IN_FLOATING)    P3M0 &= ~GPIOx->Pin, P3M1 |=  GPIOx->Pin;
		if(GPIOx->Mode == GPIO_Mode_Out_PP)         P3M0 |=  GPIOx->Pin, P3M1 &= ~GPIOx->Pin;
		if(GPIOx->Mode == GPIO_Mode_Out_OD)         P3M0 |=  GPIOx->Pin, P3M1 |=  GPIOx->Pin;
    }
    if(GPIO == GPIO_P4)
    {
		if(GPIOx->Mode == GPIO_Mode_Out_IN)         P4M0 &= ~GPIOx->Pin, P4M1 &= ~GPIOx->Pin;
        if(GPIOx->Mode == GPIO_Mode_IN_FLOATING)    P4M0 &= ~GPIOx->Pin, P4M1 |=  GPIOx->Pin;
		if(GPIOx->Mode == GPIO_Mode_Out_PP)         P4M0 |=  GPIOx->Pin, P4M1 &= ~GPIOx->Pin;
		if(GPIOx->Mode == GPIO_Mode_Out_OD)         P4M0 |=  GPIOx->Pin, P4M1 |=  GPIOx->Pin;
    }
    if(GPIO == GPIO_P5)
    {
		if(GPIOx->Mode == GPIO_Mode_Out_IN)         P5M0 &= ~GPIOx->Pin, P5M1 &= ~GPIOx->Pin;
        if(GPIOx->Mode == GPIO_Mode_IN_FLOATING)    P5M0 &= ~GPIOx->Pin, P5M1 |=  GPIOx->Pin;
		if(GPIOx->Mode == GPIO_Mode_Out_PP)         P5M0 |=  GPIOx->Pin, P5M1 &= ~GPIOx->Pin;
		if(GPIOx->Mode == GPIO_Mode_Out_OD)         P5M0 |=  GPIOx->Pin, P5M1 |=  GPIOx->Pin;
    }
    if(GPIO == GPIO_P6)
    {
		if(GPIOx->Mode == GPIO_Mode_Out_IN)         P6M0 &= ~GPIOx->Pin, P6M1 &= ~GPIOx->Pin;
        if(GPIOx->Mode == GPIO_Mode_IN_FLOATING)    P6M0 &= ~GPIOx->Pin, P6M1 |=  GPIOx->Pin;
		if(GPIOx->Mode == GPIO_Mode_Out_PP)         P6M0 |=  GPIOx->Pin, P6M1 &= ~GPIOx->Pin;
		if(GPIOx->Mode == GPIO_Mode_Out_OD)         P6M0 |=  GPIOx->Pin, P6M1 |=  GPIOx->Pin;
    }
    if(GPIO == GPIO_P7)
    {
		if(GPIOx->Mode == GPIO_Mode_Out_IN)         P7M0 &= ~GPIOx->Pin, P7M1 &= ~GPIOx->Pin;
        if(GPIOx->Mode == GPIO_Mode_IN_FLOATING)    P7M0 &= ~GPIOx->Pin, P7M1 |=  GPIOx->Pin;
		if(GPIOx->Mode == GPIO_Mode_Out_PP)         P7M0 |=  GPIOx->Pin, P7M1 &= ~GPIOx->Pin;
		if(GPIOx->Mode == GPIO_Mode_Out_OD)         P7M0 |=  GPIOx->Pin, P7M1 |=  GPIOx->Pin;
    }
}
/*************************************************************************
 * @code    void GPIO_Init(GPIO_port GPIO_Port,u8 GPIO_Pin_Bit, GPIO_Mode Mode_e)
 * @brief   一次设置GPIO端口的多个IO模式
 * @param   GPIO_Port:端口号（0~7）；GPIO_Pin_Bit：引脚位号；Mode_e：模式
 *          模式选择：GPIO_Mode_Out_IN/GPIO_Mode_IN_FLOATING/GPIO_Mode_Out_PP/GPIO_Mode_Out_OD
 * @return  无
 * @date    2025-10-22.
 * @example GPIO_Init(GPIO_P2,GPIO_Pin_0|GPIO_Pin_1,GPIO_Mode_Out_IN);
 *          设置 P2.0 P2.1 管脚准双向模式，即可以输出也可以输入
 * ***********************************************************************/
void GPIO_Init(GPIO_port GPIO_Port,u8 GPIO_Pin_Bit, GPIO_Mode Mode_e)
{
    GPIO_InitTypeDef GPIO_Initstu;

    GPIO_Initstu.Pin  = GPIO_Pin_Bit;        //要初始化的I/O  eg: GPIO_Pin_0|GPIO_Pin_2;
    GPIO_Initstu.Mode = Mode_e;              //要初始化的I/O 模式
    GPIO_Config(GPIO_Port,&GPIO_Initstu);
}

/*************************************************************************
 * @code    void GPIO_Set_Out_IN(Pin_list Pin_x);
 * @brief   设置引脚准双向模式，该函数用户不需要动
 * @param   Pin_x:引脚号
 * @return  无
 * @date    2025-10-22.
 * @example GPIO_Set_Out_IN(P2_0); // 设置 P2_0 管脚准双向模式，即可以输出也可以输入
 * ***********************************************************************/
void GPIO_Set_Out_IN(Pin_list Pin_x)
{
    u8 Pin_Bit = GPIO_Get_Pin(Pin_x);
    switch (GPIO_Get_Port(Pin_x))
    {
        case GPIO_P0: P0M0 &= ~Pin_Bit, P0M1 |= Pin_Bit; break;
        case GPIO_P1: P1M0 &= ~Pin_Bit, P1M1 |= Pin_Bit; break;
        case GPIO_P2: P2M0 &= ~Pin_Bit, P2M1 |= Pin_Bit; break;
        case GPIO_P3: P3M0 &= ~Pin_Bit, P3M1 |= Pin_Bit; break;
        case GPIO_P4: P4M0 &= ~Pin_Bit, P4M1 |= Pin_Bit; break;
        case GPIO_P5: P5M0 &= ~Pin_Bit, P5M1 |= Pin_Bit; break;
        case GPIO_P6: P6M0 &= ~Pin_Bit, P6M1 |= Pin_Bit; break;
        case GPIO_P7: P7M0 &= ~Pin_Bit, P7M1 |= Pin_Bit; break;
        default: break;
    }
}
/*************************************************************************
 * @code    void GPIO_Set_IN_FLOATING(Pin_list Pin_x);
 * @brief   设置引脚为浮空输入模式，该函数用户不需要动
 * @param   Pin_x:引脚号
 * @return  无
 * @date    2025-10-22.
 * @example GPIO_Set_IN_FLOATING(P2_0); // 设置 P2_0 管脚为浮空输入
 * ***********************************************************************/
void GPIO_Set_IN_FLOATING(Pin_list Pin_x)
{
    u8 Pin_Bit = GPIO_Get_Pin(Pin_x);
    switch (GPIO_Get_Port(Pin_x))
    {
        case GPIO_P0: P0M0 &= ~Pin_Bit, P0M1 |= Pin_Bit; break;
        case GPIO_P1: P1M0 &= ~Pin_Bit, P1M1 |= Pin_Bit; break;
        case GPIO_P2: P2M0 &= ~Pin_Bit, P2M1 |= Pin_Bit; break;
        case GPIO_P3: P3M0 &= ~Pin_Bit, P3M1 |= Pin_Bit; break;
        case GPIO_P4: P4M0 &= ~Pin_Bit, P4M1 |= Pin_Bit; break;
        case GPIO_P5: P5M0 &= ~Pin_Bit, P5M1 |= Pin_Bit; break;
        case GPIO_P6: P6M0 &= ~Pin_Bit, P6M1 |= Pin_Bit; break;
        case GPIO_P7: P7M0 &= ~Pin_Bit, P7M1 |= Pin_Bit; break;
        default: break;
    }
}

/*************************************************************************
 * @code    void GPIO_Set_Out_PP(Pin_list Pin_x);
 * @brief   设置引脚为推挽输出模式，该函数用户不需要动
 * @param   Pin_x:引脚号
 * @return  无
 * @date    2025-10-22.
 * @example GPIO_Set_Out_PP(P2_0); // 设置 P2_0 管脚为推挽输出
 * ***********************************************************************/
void GPIO_Set_Out_PP(Pin_list Pin_x)
{
    u8 Pin_Bit = GPIO_Get_Pin(Pin_x);
    switch (GPIO_Get_Port(Pin_x))
    {
        case GPIO_P0: P0M0 |= Pin_Bit, P0M1 &= ~Pin_Bit; break;
        case GPIO_P1: P1M0 |= Pin_Bit, P1M1 &= ~Pin_Bit; break;
        case GPIO_P2: P2M0 |= Pin_Bit, P2M1 &= ~Pin_Bit; break;
        case GPIO_P3: P3M0 |= Pin_Bit, P3M1 &= ~Pin_Bit; break;
        case GPIO_P4: P4M0 |= Pin_Bit, P4M1 &= ~Pin_Bit; break;
        case GPIO_P5: P5M0 |= Pin_Bit, P5M1 &= ~Pin_Bit; break;
        case GPIO_P6: P6M0 |= Pin_Bit, P6M1 &= ~Pin_Bit; break;
        case GPIO_P7: P7M0 |= Pin_Bit, P7M1 &= ~Pin_Bit; break;
        default: break;
    }
}

/*************************************************************************
 * @code    void GPIO_Set_Out_OD(Pin_list Pin_x);
 * @brief   设置引脚为开漏模式，该函数用户不需要动
 * @param   Pin_x:引脚号
 * @return  无
 * @date    2025-10-22.
 * @example GPIO_Set_Out_OD(P2_0); // 设置 P2_0 管脚为开漏输出
 * ***********************************************************************/
void GPIO_Set_Out_OD(Pin_list Pin_x)
{
    u8 Pin_Bit = GPIO_Get_Pin(Pin_x);
    switch (GPIO_Get_Port(Pin_x))
    {
        case GPIO_P0: P0M0 |= Pin_Bit, P0M1 |= Pin_Bit; break;
        case GPIO_P1: P1M0 |= Pin_Bit, P1M1 |= Pin_Bit; break;
        case GPIO_P2: P2M0 |= Pin_Bit, P2M1 |= Pin_Bit; break;
        case GPIO_P3: P3M0 |= Pin_Bit, P3M1 |= Pin_Bit; break;
        case GPIO_P4: P4M0 |= Pin_Bit, P4M1 |= Pin_Bit; break;
        case GPIO_P5: P5M0 |= Pin_Bit, P5M1 |= Pin_Bit; break;
        case GPIO_P6: P6M0 |= Pin_Bit, P6M1 |= Pin_Bit; break;
        case GPIO_P7: P7M0 |= Pin_Bit, P7M1 |= Pin_Bit; break;
        default: break;
    }
}
/*************************************************************************
 * @code    void gpio_init_pin(Pin_list Pin_x, GPIO_Mode mode);
 * @brief   用户调用 GPIO引脚初始化函数.
 * @param   Pin_x  ：管脚标号.
 * @param   mode   ：IO模式, ->  GPIO_Mode
 * @return  none.
 * @date    2025-10-22.
 * @example gpio_init_pin(P2_0, GPIO_Mode_Out_PP); // 初始化 P2_0 管脚为推挽输出
 *************************************************************************/
void gpio_init_pin(Pin_list Pin_x, GPIO_Mode mode)
{
    switch (mode)
    {
        case GPIO_Mode_Out_IN:
		    GPIO_Set_Out_IN(Pin_x);
			break;
        case GPIO_Mode_IN_FLOATING:
            GPIO_Set_IN_FLOATING(Pin_x); /* 浮空输入 高阻态 无上下拉*/
            break;
        case GPIO_Mode_IPU:
            GPIO_Set_IN_FLOATING(Pin_x); /* 先设置为浮空输入 再使能上拉 */
            gpio_pull_up(Pin_x, ENABLE);
            break;
        case GPIO_Mode_IPD:
            GPIO_Set_IN_FLOATING(Pin_x); /* 先设置为浮空输入 再使能下拉 */
            gpio_pull_down(Pin_x, ENABLE);
            break;
        case GPIO_Mode_Out_PP:
            GPIO_Set_Out_PP(Pin_x);     /* 推挽输出，驱动能力用户选择是否强驱动和IO速度 */
            break;
        case GPIO_Mode_Out_OD:
            GPIO_Set_Out_OD(Pin_x);    /* 通用开漏模式，开漏即可读取外部状态 也可对外输出高低电平 */
            break;
    default:  break;
    }
}

/*************************************************************************
 * @code    void gpio_pull_up(Pin_list Pin_x, u8 enable)
 * @brief   GPIO引脚上拉使能设置.
 * @param   Pin_x  ：管脚标号.
 * @param   enable ：使能标志, ENABLE-使能, DISABLE-禁止.
 * @return  none.
 * @date    2025-12-22.
 * @example gpio_pull_up(P2_0, ENABLE); // 设置 P2_0 管脚上拉
 *************************************************************************/
void gpio_pull_up(Pin_list Pin_x, u8 _enable)
{
    u8 port_offset = GPIO_Get_Port(Pin_x), pin_num = GPIO_Get_Pin(Pin_x);
    if (Pin_x <= P7_7)
	{
		(_enable == ENABLE) ? ((*(&P0PU + port_offset) |= pin_num)) : ((*(&P0PU + port_offset) &= ~pin_num));
	}
}

/*************************************************************************
 * @code    void gpio_pull_down(Pin_list pin, u8 enable)
 * @brief   GPIO引脚下拉使能设置.
 * @param   Pin_x  ：管脚标号.
 * @param   enable ：使能标志, ENABLE-使能, DISABLE-禁止.
 * @return  none.
 * @date    2025-12-22.
 * @example GPIO_down_up(P2_0, ENABLE); // 设置 P2_0 管脚下拉
 *************************************************************************/
void gpio_pull_down(Pin_list Pin_x, u8 _enable)
{
    u8 port_offset = GPIO_Get_Port(Pin_x), pin_num = GPIO_Get_Pin(Pin_x);
    if (Pin_x <= P7_7)
	{
    	(_enable == ENABLE) ? ((*(&P0PD + port_offset)) |= pin_num) : ((*(&P0PD + port_offset) &= ~pin_num));
	}
}

/*************************************************************************
 * @code    void GPIO_ncs(Pin_list pin, u8 enable)
 * @brief   GPIO引脚施密特触发控制器使能设置.
 * @param   Pin_x  ：管脚标号.
 * @param   enable ：使能标志, ENABLE-使能, DISABLE-禁止.
 * @return  none.
 * @date    2025-10-22.
 * @example GPIO_ncs(P2_0, ENABLE); // 设置 P2_0 管脚使能施密特触发控制器
 *************************************************************************/
void gpio_en_ncs(Pin_list Pin_x, u8 _enable)
{
    u8 port_offSet = GPIO_Get_Port(Pin_x), pin_num = GPIO_Get_Pin(Pin_x);
    if (Pin_x <= P7_7)
    {
        (_enable == ENABLE) ? ((*(&P0NCS + port_offSet)) |= pin_num) : ((*(&P0NCS + port_offSet) &= ~pin_num));
    }
}

/*************************************************************************
 * @code    void gpio_speed(Pin_list pin, u8 enable)
 * @brief   GPIO引脚电平转换速度使能设置.
 * @param   Pin_x  ：管脚标号.
 * @param   enable ：使能标志, ENABLE-使能, DISABLE-禁止.
 * @return  none.
 * @date    2025-12-22.
 * @example GPIO_speed(P2_0, ENABLE); // 设置 P2_0 管脚电平转换速度快
 *************************************************************************/
void gpio_speed(Pin_list Pin_x, u8 _enable)
{
    u8 port_offSet = GPIO_Get_Port(Pin_x), pin_num = GPIO_Get_Pin(Pin_x);
    if (Pin_x <= P7_7)
    {
        (_enable == ENABLE) ? ((*(&P0SR + port_offSet)) |= pin_num) : ((*(&P0SR + port_offSet) &= ~pin_num));
    }
}

/*************************************************************************
 * @code    void gpio_electricity(Pin_list pin, u8 enable)
 * @brief   GPIO引脚端口驱动电流使能控制.
 * @param   Pin_x  ：管脚标号.
 * @param   enable ：使能标志, ENABLE-强驱动能力, DISABLE-弱驱动能力.
 * @return  none.
 * @date    2025-12-22.
 * @example GPIO_electricity(P2_0, ENABLE); // 设置 P2_0 管脚增强控制端口的驱动能力
 *************************************************************************/
void gpio_electricity(Pin_list Pin_x, u8 _enable)
{
    u8 port_offSet = GPIO_Get_Port(Pin_x), pin_num = GPIO_Get_Pin(Pin_x);
    if (Pin_x <= P7_7)
    {
        (_enable == ENABLE) ? ((*(&P0DR + port_offSet) &= ~pin_num)) : ((*(&P0DR + port_offSet)) |= pin_num);
    }
}

/*************************************************************************
 * @code    void gpio_en_digital(Pin_list Pin_x, u8 _enable)
 * @brief   GPIO引脚数字信号输入使能控制.
 * @param   Pin_x  ：管脚标号.
 * @param   enable ：使能标志, ENABLE-使能, DISABLE-禁止.
 * @return  none.
 * @date    2025-12-22.
 * @example gpio_en_digital(P2_0, ENABLE); // 使能 P2_0 管脚数字信号输入
 *************************************************************************/
void gpio_en_digital(Pin_list Pin_x, u8 _enable)
{
    u8 port_offSet = GPIO_Get_Port(Pin_x), pin_num = GPIO_Get_Pin(Pin_x);
    if (Pin_x <= P7_7)
    {
        (_enable == ENABLE) ? ((*(&P0IE + port_offSet)) |= pin_num) : ((*(&P0IE + port_offSet) &= ~pin_num));
    }
}

/*************************************************************************
 * @code    void gpio_AF_mode(Pin_list Pin_x, u8 _mode)
 * @brief   GPIO引脚配置模式设置.默认用户配置
 * @param   Pin_x  ：管脚标号.
 * @param   _mode ：配置模式, ENABLE-外设自动配置-即复用模式. DISABLE-用户自行配置,
 * @return  none.
 * @date    2025-12-22.
 * @example gpio_AF_mode(P2_0, ENABLE); // 设置 P2_0 管脚为外设自动配置模式
 *************************************************************************/
void gpio_AF_mode(Pin_list Pin_x, u8 _mode)
{
    u8 port_offSet = GPIO_Get_Port(Pin_x), pin_num = GPIO_Get_Pin(Pin_x);
    if (Pin_x <= P7_7)
    {
        (_mode == ENABLE) ? ((*(&P0BP + port_offSet) &= ~pin_num)) : ((*(&P0BP + port_offSet)) |= pin_num);
    }
}

/*************************************************************************
 * @code    void gpio_write_pin(Pin_list Pin_x,u8 value)
 * @brief   GPIO引脚电平状态.
 * @param   Pin_x ：管脚标号.
 * @param   value ：电平状态设置，1-使能，0-禁止.
 * @return  none.
 * @date    2025-11-12.
 * @example gpio_write_pin(P0_0, ENABLE);  // P0_0上拉使能
 *************************************************************************/
void gpio_write_pin(Pin_list Pin_x, u8 value)
{
    switch (GPIO_Get_Port(Pin_x))
    {
        case GPIO_P0: if (value) P0 |= GPIO_Get_Pin(Pin_x); else P0 &= ~GPIO_Get_Pin(Pin_x); break;
        case GPIO_P1: if (value) P1 |= GPIO_Get_Pin(Pin_x); else P1 &= ~GPIO_Get_Pin(Pin_x); break;
        case GPIO_P2: if (value) P2 |= GPIO_Get_Pin(Pin_x); else P2 &= ~GPIO_Get_Pin(Pin_x); break;
        case GPIO_P3: if (value) P3 |= GPIO_Get_Pin(Pin_x); else P3 &= ~GPIO_Get_Pin(Pin_x); break;
        case GPIO_P4: if (value) P4 |= GPIO_Get_Pin(Pin_x); else P4 &= ~GPIO_Get_Pin(Pin_x); break;
        case GPIO_P5: if (value) P5 |= GPIO_Get_Pin(Pin_x); else P5 &= ~GPIO_Get_Pin(Pin_x); break;
        case GPIO_P6: if (value) P6 |= GPIO_Get_Pin(Pin_x); else P6 &= ~GPIO_Get_Pin(Pin_x); break;
        case GPIO_P7: if (value) P7 |= GPIO_Get_Pin(Pin_x); else P7 &= ~GPIO_Get_Pin(Pin_x); break;
        default: break;
    }
}

/*************************************************************************
 * @code    u8 gpio_read_pin(Pin_list Pin_x)
 * @brief   获取其 GPIO 引脚电平状态.
 * @param   Pin_x ：管脚标号.
 * @return  管脚电平状态.1/0
 * @date    2025-11-12.
 * @example u8 val = gpio_read_pin(P0_0);  // 获取P0_0电平状态
 *************************************************************************/
u8 gpio_read_pin(Pin_list Pin_x)
{
    u8 val = FAIL;    // 默认返回错误状态
     switch (GPIO_Get_Port(Pin_x))
     {
         case GPIO_P0: val = (P0 & GPIO_Get_Pin(Pin_x)) ? 1 : 0; break;
         case GPIO_P1: val = (P1 & GPIO_Get_Pin(Pin_x)) ? 1 : 0; break;
         case GPIO_P2: val = (P2 & GPIO_Get_Pin(Pin_x)) ? 1 : 0; break;
         case GPIO_P3: val = (P3 & GPIO_Get_Pin(Pin_x)) ? 1 : 0; break;
         case GPIO_P4: val = (P4 & GPIO_Get_Pin(Pin_x)) ? 1 : 0; break;
         case GPIO_P5: val = (P5 & GPIO_Get_Pin(Pin_x)) ? 1 : 0; break;
         case GPIO_P6: val = (P6 & GPIO_Get_Pin(Pin_x)) ? 1 : 0; break;
         case GPIO_P7: val = (P7 & GPIO_Get_Pin(Pin_x)) ? 1 : 0; break;
         default: break;
     }
    return val;
}

/*************************************************************************
 * @code    void gpio_toggle_pin(Pin_list Pin_x)
 * @brief   GPIO引脚电平输出状态反转.
 * @param   Pin_x ：管脚标号.
 * @return  none.
 * @date    2025-11-12.
 * @example gpio_toggle_pin(P0_0);  // P0_0电平状态反转
 *************************************************************************/
void gpio_toggle_pin(Pin_list Pin_x)
{
    switch (GPIO_Get_Port(Pin_x))
    {
        case GPIO_P0: P0 ^= GPIO_Get_Pin(Pin_x); break;
        case GPIO_P1: P1 ^= GPIO_Get_Pin(Pin_x); break;
        case GPIO_P2: P2 ^= GPIO_Get_Pin(Pin_x); break;
        case GPIO_P3: P3 ^= GPIO_Get_Pin(Pin_x); break;
        case GPIO_P4: P4 ^= GPIO_Get_Pin(Pin_x); break;
        case GPIO_P5: P5 ^= GPIO_Get_Pin(Pin_x); break;
        case GPIO_P6: P6 ^= GPIO_Get_Pin(Pin_x); break;
        case GPIO_P7: P7 ^= GPIO_Get_Pin(Pin_x); break;
        default: break;
    }
}
