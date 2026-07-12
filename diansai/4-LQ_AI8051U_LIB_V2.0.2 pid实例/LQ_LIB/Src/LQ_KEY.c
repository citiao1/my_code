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
#include "LQ_KEY.h"


/*************************************************************************
 *  函数名称：void GPIO_KEY_Init(void)
 *  功能说明：GPIO初始化函数
 *  参数说明：无
 *  函数返回：无
 *  修改时间：2020年3月10日
 *  备    注：
 *************************************************************************/
void GPIO_KEY_Init(void)
{
	gpio_init_pin(KEY0_PIN, GPIO_Mode_IPU); // 按键0
	gpio_init_pin(KEY1_PIN, GPIO_Mode_IPU); // 按键1
	gpio_init_pin(KEY2_PIN, GPIO_Mode_IPU); // 按键2
	gpio_init_pin(DSW0_PIN, GPIO_Mode_IPU); // 拨码开关0
	gpio_init_pin(DSW1_PIN, GPIO_Mode_IPU); // 拨码开关1
}

/*************************************************************************
 *  函数名称：unsigned char KEY_Read(KEYn_e KEYno)
 *  功能说明：读取按键状态
 *  参数说明：KEYn_e KEYno按键编号
 *  函数返回：按键状态，按下返回1-TRUE，松开返回0
 *  修改时间：2025年1月10日
 *  备    注：
 *************************************************************************/
unsigned char KEY_Read(KEYn_e KEYno)
{
	if(KEYno == KEY0) return (!gpio_read_pin(KEY0_PIN)); // 母板上按键0
	if(KEYno == KEY1) return (!gpio_read_pin(KEY1_PIN)); // 母板上按键1
	if(KEYno == KEY2) return (!gpio_read_pin(KEY2_PIN)); // 母板上按键2
	if(KEYno == DSW0) return (!gpio_read_pin(DSW0_PIN)); // 母板上拨码开关0
	if(KEYno == DSW1) return (!gpio_read_pin(DSW1_PIN)); // 母板上拨码开关1
	if(KEYno == Tubep) return (!gpio_read_pin(Tubep_PIN)); // 母板上干簧管
	return 0;
}


