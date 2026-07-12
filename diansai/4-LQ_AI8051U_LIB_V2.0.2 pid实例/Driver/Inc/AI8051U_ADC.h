/*---------------------------------------------------------------------*/
/* ------------------- Web: www.STCAI.com -----------------------------*/
/*---------------------------------------------------------------------*/

#ifndef	__AI8051U_ADC_H
#define	__AI8051U_ADC_H

#include	"config.h"
#include	"AI8051U_GPIO.h"
//========================================================================
//                              ADC设置
//========================================================================

#define 	ADC_PowerOn(n)			(n==0?(ADC_POWER = 0):(ADC_POWER = 1))	/* ADC电源开关 */
#define 	ADC_Justify(n)			(n==0?(RESFMT = 0):(RESFMT = 1))				/* ADC转换结果格式控制, 左对齐/右对齐 */

//========================================================================
//                              定义声明
//========================================================================

#define ADC_SPEED_2X1T		0			//SYSclk/2/1
#define ADC_SPEED_2X2T		1			//SYSclk/2/2
#define ADC_SPEED_2X3T		2			//SYSclk/2/3
#define ADC_SPEED_2X4T		3			//SYSclk/2/4
#define ADC_SPEED_2X5T		4			//SYSclk/2/5
#define ADC_SPEED_2X6T		5			//SYSclk/2/6
#define ADC_SPEED_2X7T		6			//SYSclk/2/7
#define ADC_SPEED_2X8T		7			//SYSclk/2/8
#define ADC_SPEED_2X9T		8			//SYSclk/2/9
#define ADC_SPEED_2X10T		9			//SYSclk/2/10
#define ADC_SPEED_2X11T		10		    //SYSclk/2/11
#define ADC_SPEED_2X12T		11		    //SYSclk/2/12
#define ADC_SPEED_2X13T		12		    //SYSclk/2/13
#define ADC_SPEED_2X14T		13		    //SYSclk/2/14
#define ADC_SPEED_2X15T		14		    //SYSclk/2/15
#define ADC_SPEED_2X16T		15		    //SYSclk/2/16

typedef enum
{
     ADC_CH0_P10=0,
     ADC_CH1_P11,  //左竖   1
     ADC_CH2_P12,  //中间   2
     ADC_CH3_P13,	 //右横   3
     ADC_CH4_P14,  //右竖   4
     ADC_CH5_P15,  //左横   5
     ADC_CH6_P16,
     ADC_CH7_P17,
     ADC_CH8_P00,
     ADC_CH9_P01,
     ADC_CH10_P02,
     ADC_CH11_P03,
     ADC_CH12_P04,
     ADC_CH13_P05,
     ADC_CH14_P06,
     ADC_CH15_P07,
}ADC_CHx;		//ADC通道选择



#define ADC_LEFT_JUSTIFIED		0		//ADC Result left-justified
#define ADC_RIGHT_JUSTIFIED		1		//ADC Result right-justified


typedef struct
{
	u8	ADC_SMPduty;		//ADC 模拟信号采样时间控制, 0~31（注意： SMPDUTY 一定不能设置小于 10）
	u8	ADC_Speed;			//设置 ADC 工作时钟频率	ADC_SPEED_2X1T~ADC_SPEED_2X16T
	u8	ADC_AdjResult;		//ADC结果调整,	ADC_LEFT_JUSTIFIED,ADC_RIGHT_JUSTIFIED
	u8	ADC_CsSetup;		//ADC 通道选择时间控制 0(默认),1
	u8	ADC_CsHold;			//ADC 通道选择保持时间控制 0,1(默认),2,3
} ADC_InitTypeDef;

void ADC_GPIO_Init(ADC_CHx CHn_IOx);
u8 ADC_Inilize(ADC_InitTypeDef *ADCx);
void ADC_PowerControl(u8 pwr);
u16 Get_ADCResult(ADC_CHx channel);	//channel = 0~15

#endif
