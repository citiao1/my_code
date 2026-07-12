/*---------------------------------------------------------------------*/
/* ------------------- Web: www.STCAI.com -----------------------------*/
/*---------------------------------------------------------------------*/

#ifndef	__STC32G_GPIO_H
#define	__STC32G_GPIO_H

#include "config.h"

// 为高效操作，保留传统直接操作寄存器的宏定义
#define  GPIO_Set_Bit(_x, val)  ((_x)=(val))          //设置引脚电平,eg:GPIO_Set_Bit(P10, HIGH)
#define  GPIO_Get_Bit(_x)       (_x)                   //读取引脚电平,VAL=GPIO_Get_Bit(P10)

// 以下为GPIO相关的宏定义，用户无需关心
#define  GPIO_Get_Port(Pins)         ((Pins & 0xF0) >> 4)         //获取端口 0~F0 (目前到B)
#define  GPIO_Get_Pin(Pins)          (1 << (Pins & 0x0F))         //获取引脚 0~0F (限定0到7位)

#define	GPIO_Pin_0		0x01	//IO引脚 Px.0   <<0
#define	GPIO_Pin_1		0x02	//IO引脚 Px.1   <<1
#define	GPIO_Pin_2		0x04	//IO引脚 Px.2   <<2
#define	GPIO_Pin_3		0x08	//IO引脚 Px.3   <<3
#define	GPIO_Pin_4		0x10	//IO引脚 Px.4   <<4 
#define	GPIO_Pin_5		0x20	//IO引脚 Px.5   <<5 
#define	GPIO_Pin_6		0x40	//IO引脚 Px.6   <<6
#define	GPIO_Pin_7		0x80	//IO引脚 Px.7   <<7
#define	GPIO_Pin_LOW	0x0F	//IO低4位引脚
#define	GPIO_Pin_HIGH	0xF0	//IO高4位引脚
#define	GPIO_Pin_All	0xFF	//IO所有引脚

typedef enum
{
    GPIO_P0 = 0,
    GPIO_P1,
    GPIO_P2,
    GPIO_P3,
    GPIO_P4,
    GPIO_P5,
    GPIO_P6,
    GPIO_P7,
}GPIO_port;


typedef enum
{
	P0_0 = 0x00, P0_1, P0_2, P0_3, P0_4, P0_5, P0_6, P0_7,
	P1_0 = 0x10, P1_1, P1_2, P1_3, P1_4, P1_5, P1_6, P1_7,
	P2_0 = 0x20, P2_1, P2_2, P2_3, P2_4, P2_5, P2_6, P2_7,
	P3_0 = 0x30, P3_1, P3_2, P3_3, P3_4, P3_5, P3_6, P3_7,
	P4_0 = 0x40, P4_1, P4_2, P4_3, P4_4, P4_5, P4_6, P4_7,
	P5_0 = 0x50, P5_1, P5_2, P5_3, P5_4, P5_5, P5_6, P5_7,
	P6_0 = 0x60, P6_1, P6_2, P6_3, P6_4, P6_5, P6_6, P6_7,
	P7_0 = 0x70, P7_1, P7_2, P7_3, P7_4, P7_5, P7_6, P7_7,
	PX_0 = 0xFF,
}Pin_list;


// IO模式定义
typedef enum
{
    GPIO_Mode_Out_IN        = 0x00,     /* 弱上拉 准双向口 */
    GPIO_Mode_IN_FLOATING   = 0x10,     /* 高阻态  浮空输入 */
    GPIO_Mode_IPU           = 0x11,     /* 上拉输入=高阻态+内部上拉 */
    GPIO_Mode_IPD           = 0x12,     /* 下拉输入=高阻态+内部下拉 */
    GPIO_Mode_Out_PP        = 0x20,     /* 推挽输出, 注意该模式默认输出高电平！ */
    GPIO_Mode_Out_OD        = 0x21,     /* 开漏模式 */
}GPIO_Mode;

typedef struct
{
	u8	Mode;		//IO模式,  		GPIO_PullUp,GPIO_HighZ,GPIO_OUT_OD,GPIO_OUT_PP
	u8	Pin;		//要设置的端口	
} GPIO_InitTypeDef;


/* 内部GPIO配置函数声明，用户一般不需要管 */
void GPIO_Config(GPIO_port GPIO, GPIO_InitTypeDef *GPIOx); /* 初始化GPIO */
void GPIO_Set_Out_IN(Pin_list Pin_x);               /* 弱上拉 准双向口 */
void GPIO_Set_IN_FLOATING(Pin_list Pin_x);          /* 高阻态 浮空输入 */
void GPIO_Set_Out_PP(Pin_list Pin_x);               /* 推挽输出 */
void GPIO_Set_Out_OD(Pin_list Pin_x);               /* 开漏输出 */

void gpio_pull_up(Pin_list Pin_x, u8 _enable);      /* 上拉使能设置 */
void gpio_pull_down(Pin_list Pin_x, u8 _enable);    /* 下拉使能设置 */
void gpio_en_ncs(Pin_list Pin_x, u8 _enable);       /* 施密特触发器使能设置 */
void gpio_speed(Pin_list Pin_x, u8 _enable);        /* IO速度模式设置 */
void gpio_electricity(Pin_list Pin_x, u8 _enable);  /* 强驱动能力设置 */
void gpio_en_digital(Pin_list Pin_x, u8 _enable);   /* 数字功能使能设置 */
void gpio_AF_mode(Pin_list Pin_x, u8 _mode);        /* 外设IO复用模式设置 */


//========================================================================
//                      外部 GPIO用户 调用函数声明
//========================================================================
void GPIO_Init(GPIO_port GPIO_Port,u8 GPIO_Pin_Bit, GPIO_Mode Mode_e);// 可一次初始化整个端口8个IO
void gpio_init_pin(Pin_list Pin_x, GPIO_Mode Mode_e); // IO初始化
void gpio_write_pin(Pin_list Pin_x,u8 value);         // IO状态写
u8 gpio_read_pin(Pin_list Pin_x);                     // IO状态读
void gpio_toggle_pin(Pin_list Pin_x);                 // IO状态反转

#endif
