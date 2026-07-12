/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
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

#include "include.h"
//#include "LQ_CCD.h"


u8 xdata CCD1_Bin[128];   //二值化滤波后的（滤波覆盖二值化的数组）
/**
  * @brief    CCD管脚初始化
  * @return   无
  * @note     
  * @example  
  * @date     2023/11/24
*/
void CCD1_Init() 
{
	ADC_InitTypeDef		ADC_InitStructure;		//结构定义

    gpio_init_pin(P1_7, GPIO_Mode_IN_FLOATING);     //ADC管脚 P17，高阻态 浮空输入

    //ADC配置
	ADC_InitStructure.ADC_SMPduty   = 12;		//ADC 模拟信号采样时间控制, 0~31（注意： SMPDUTY 一定不能设置小于 10）
	ADC_InitStructure.ADC_CsSetup   = 0;		//ADC 通道选择时间控制 0(默认),1
	ADC_InitStructure.ADC_CsHold    = 1;		//ADC 通道选择保持时间控制 0,1(默认),2,3
	ADC_InitStructure.ADC_Speed     = ADC_SPEED_2X12T;		//设置 ADC 工作时钟频率	ADC_SPEED_2X1T~ADC_SPEED_2X16T
	ADC_InitStructure.ADC_AdjResult = ADC_RIGHT_JUSTIFIED;	//ADC结果调整,	ADC_LEFT_JUSTIFIED,ADC_RIGHT_JUSTIFIED
	ADC_Inilize(&ADC_InitStructure);		//初始化
	ADC_PowerControl(ENABLE);				//ADC电源开关, ENABLE或DISABLE
	NVIC_ADC_Init(DISABLE,Priority_0);		//中断使能, ENABLE/DISABLE; 优先级(低到高) Priority_0,Priority_1,Priority_2,Priority_3

    GPIO_Init(GPIO_P1,GPIO_Pin_3|GPIO_Pin_5,GPIO_Mode_Out_PP);  // P13,P15 推挽输出
}

void CCD_Delayus(uint16_t us)
{
    delay_us(us);
}

/**
  * @brief    CCD采集函数
  *
  * @param    p  ：  存放采集数据的首地址
  *
  * @return   无
  *
  * @note     
  *
  * @example  uint16_t ccd_buf[128];
  *           CCD_Collect(ccd_buf); 
  *
*/
void CCD_Collect(uint16_t *p)
{
	unsigned char i;
	unsigned int  temp = 0;
    {
        CCD1_SI_H;              // SI  = 1 
        CCD_Delayus(1);
        CCD1_CLK_H;             // CLK = 1 
        CCD_Delayus(1);
        CCD1_SI_L;              // SI  = 0 
        CCD_Delayus(1);

        for(i=0; i<128; i++) 
        {
            CCD_Delayus(1);
            CCD1_CLK_H;         // CLK = 1 
            CCD_Delayus(1);
            temp = CCD1_AO_GET();       
            *p++ = temp;     
            CCD1_CLK_L;        // CLK = 0 
        }
        CCD_Delayus(1);
        CCD1_CLK_H;            // CLK = 1 
        CCD_Delayus(1);
        CCD1_CLK_L;            // CLK = 0 
    }

}


/*******************************************************************
 * 函数名：void ccd_binaryzation(uint16_t threshold, uint16_t *ccd_data, uint8_t *bin_ccd)
 * 功  能：CCD二值化处理,并滤波
 * 参  数：无
 * 返回值: 无
 ******************************************************************/
void ccd_binaryzation(uint16_t threshold, uint16_t *ccd_data, uint8_t *bin_ccd)
{
    u8 i = 0;
    for (i = 0; i < 128; i++)  //二值化
    {
        if (*ccd_data++ >= threshold)
        {
            *bin_ccd++ = 1;
        }
        else
        {
            *bin_ccd++ = 0;
        }
    }

    Bin_CCD_Filter();//将二值化后的数据进行滤波，并保存到全局数据中
}

/*******************************************************************
 * 函数名：uint16_t ccd_get_threshold(uint16_t ccd_data[CCD_W])
 * 功  能：CCD计算动态阈值
 * 参  数：ccd_data[CCD_W]
 * 返回值: 阈值((value1_max + value1_min) / 2)
 ******************************************************************/
uint16_t ccd_get_threshold(uint16_t ccd_data[128])
{
    uint8_t i = 0;
    uint16_t value1_max, value1_min;
    value1_max = ccd_data[0];
    value1_min = ccd_data[0];
    ccd_data += 5;
    for (i = 5; i < 123; i++)
    {
        if (value1_max <= ccd_data[i])
            value1_max = ccd_data[i];
        else if (value1_min >= ccd_data[i])
            value1_min = ccd_data[i];
    }

    return ((value1_max + value1_min) / 2);
}


/*******************************************************************
 * 函数名：uint16_t ccd_get_error(uint8_t bin_ccd[CCD_W], uint8_t ccd_x)
 * 功  能：获取CCD边线，计算赛道偏差值
 * 参  数：二值化的CCD数据数组 ccd_data[CCD_W],CCD区分，一个函数实现获取边线和中线偏差获取
 * 返回值: 根据左右边线计算中线偏差
 ******************************************************************/
uint16_t ccd_get_error(uint8_t bin_ccd[128])
{
    uint8_t i = 0, j = 0;
    char Left, Right;
    j = 64; // 从中间向两边找边线
    if (bin_ccd[64] == 0)
    {
        for (i = 0; i < (64 - 3); i++)
        {
            if ((bin_ccd[j - i] + bin_ccd[j - i - 1] + bin_ccd[j - i - 2]) == 3) // 左丢线
            {
                return -64;
            }
            if ((bin_ccd[j + i] + bin_ccd[j + i + 1] + bin_ccd[j + i + 2]) == 3) // 右丢线
            {
                return 64;
            }
        }
        return 200; // 　两边都丢线
    }
    else
    {
//        LED_Ctrl(LED0, RVS);
        i = j = 64;
        for (i = 64; i >= 3; i--) // 从中间向左找边线 寻找跳变点提取边线
        {
            if ((bin_ccd[i - 1] + bin_ccd[i - 2] + bin_ccd[i - 3]) == 0) // i左边全为0
            {
                if ((bin_ccd[i + 1] + bin_ccd[i + 2] + bin_ccd[i + 3]) == 3) // i右边全为1 则i为左边线
                {
                    Left = i;
                    break;
                }
            }
            if (i <= 3) // 最小边线为3
                Left = 3;
        }

        for (j = 64; j <= 125; j++) // 从中间向右找边线 寻找跳变点提取边线
        {
            if ((bin_ccd[j + 1] + bin_ccd[j + 2] + bin_ccd[j + 3]) == 0) // 右边全为0
            {
                if ((bin_ccd[j - 1] + bin_ccd[j - 2] + bin_ccd[j - 3]) == 3) // j左边全为1，i为右边线
                {
                    Right = j;
                    break;
                }
            }
            if (j >= 125) // 最大边线为125
                Right = 125;
        }
        return (Left + Right - 128); // 根据左右边线计算赛道中线
    }
}

/*******************************************************************
 * 函数名：void Bin_CCD_Filter(char ccd_n)
 * 功  能：对二值化后的CCD数据进行简单滤波处理
 * 返回值: 无
 ******************************************************************/
void Bin_CCD_Filter()
{
    u8 i = 0;
    for (i = 1; i < 128; i++)
    {
        if (CCD1_Bin[i] == 1 && CCD1_Bin[i - 1] == 0 && CCD1_Bin[i + 1] == 0) // 如果前一个像素点和后一个像素点都为0 则此像素点为0
        {
            CCD1_Bin[i] = 0;
        }
        else if (CCD1_Bin[i] == 0 && CCD1_Bin[i - 1] == 1 && CCD1_Bin[i + 1] == 1) // 如果前一个像素点和后一个像素点都为1 则此像素点为1
        {
            CCD1_Bin[i] = 1;
        }
    }
}





