/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
 【平    台】北京龙邱智能科技STC32位核心板
 【编    写】龙邱科技
 【E-mail  】chiusir@163.com
 【软件版本】V1.1 版权所有，单位使用请先联系授权
 【相关信息参考下列地址】
 【网    站】http://www.lqist.cn
 【淘宝店铺】http://longqiu.taobao.com
 --------------------------------------------------------------------------------
 【  IDE  】 keil C251 V5.60
 【Target 】 STC32G/STC8051U/AI8051U 32位模式
 【SYS CLK】 42 MHz使用内部晶振
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
#ifndef __LQ_CCD_H_
#define __LQ_CCD_H_

#include "include.h"


#define  CCD1_AO_CHANNEL    ADC_CH7_P17   //ADC通道7 P1.7引脚
#define  CCD1_CLK_PIN       P15
#define  CCD1_SI_PIN        P13

#define  CCD1_AO_GET()    Get_ADCResult(CCD1_AO_CHANNEL);

#define  CCD1_CLK_H      CCD1_CLK_PIN = 1
#define  CCD1_CLK_L      CCD1_CLK_PIN = 0

#define  CCD1_SI_H       CCD1_SI_PIN = 1 
#define  CCD1_SI_L       CCD1_SI_PIN = 0


#define THRESHOLD          800         //OLED上显示亮灭的阈值   范围0-4095



/** 二值化滤波后的CCD值 */
extern u8 xdata CCD1_Bin[128];


void ccd_binaryzation(uint16_t threshold, uint16_t *ccd_data, uint8_t *bin_ccd);
uint16_t ccd_get_threshold(uint16_t ccd_data[128]);
uint16_t ccd_get_error(uint8_t bin_ccd[128]);
void Bin_CCD_Filter(void);

void CCD1_Init(void);
void CCD_Collect(uint16_t *p);


#endif

