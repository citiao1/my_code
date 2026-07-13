/*******************************************************************************
 * @file                LQ_ccd.c
 * @brief               本文件是 LQ_MSPM0GX_LIB 软件开源库文件的一部分
 * @copyright           版权所有 (C) 2025-2026 北京龙邱科技有限公司
 * @website             http://www.lqist.cn
 * @taobao              http://longqiu.taobao.com
 *
 * @description         龙邱科技 MSPM0G3507 核心板驱动库声明
 *
 * 开发环境配置:
 *   - 使用环境 : Keil5
 *   - 目标芯片 : MSPM0G3507
 *   - 外置晶振 : 16.000MHz
 *   - 系统时钟 : 80MHz
 *
 * 本文件遵循GPL-3.0开源协议发布，旨在为 MSPM0G3507 芯片嵌入式系统设计提供快速上手开发基于 MSPM0G3507 的应用程序的参考实现
 * 商业用途（包括单位使用）需提前联系作者获得授权
 *
 * GPL-3.0 许可证声明摘要:
 * 1. 允许自由使用、修改、分发本软件
 * 2. 分发修改后的版本时，必须以相同许可证发布
 * 3. 必须保留原始版权声明和许可证信息
 * 4. 不提供任何担保，使用风险自负
 * 5. 完整协议文本请参见项目根目录 LICENSE 文件
 *
 * @author              LQ_012
 * @email               chiusir@163.com
 * @version             V2.0.0
 * @update              2026年4月24日
 *******************************************************************************/
#include "LQ_ccd.h"

uint8_t CCD_Bin[CCD_MAX][128];   //二值化滤波后的（滤波覆盖二值化的数组）

/*************************************************************************
 * @name     LQ_CCD_Init
 *
 * @brief    CCD 管脚初始化
 * @param    ccd_num    CCD 端口号
 * @return   none
 * 
 * @example  LQ_CCD_Init(CCD1);
 *************************************************************************/
void LQ_CCD_Init(LQEnum_CCD_t ccd_num) 
{
    // ADC 初始化结构体
    LQConfig_ADC_InitTypeDef_t adc_init = {
        .clockSel     = DL_ADC12_CLOCK_SYSOSC,                  // SYSOSC 时钟频率为 32MHz
        .freqRange    = DL_ADC12_CLOCK_FREQ_RANGE_24_TO_32,     // 设置频率范围为 24MHz 到 32MHz
        .divideRatio  = DL_ADC12_CLOCK_DIVIDE_1,                // 时钟分频比为 1，即时钟频率为 32MHz / 1 = 32MHz
        .adcclks      = 100,                                    // ADC 采样周期为 100 个时钟周期，即 1 / 32MHz * 100
        .resolution   = DL_ADC12_SAMP_CONV_RES_12_BIT,          // 12 位分辨率
    };
    // 初始化 ADC
    LQ_ADC_Init((ccd_num == CCD1) ? CCD1_AO_PORT : CCD2_AO_PORT, &adc_init);
    // 使能 ADC 转换通道
    LQ_ADC_EnableConversions((ccd_num == CCD1) ? CCD1_AO_PORT : CCD2_AO_PORT);
    // 启动 ADC 转换
    LQ_ADC_StartConversions((ccd_num == CCD1) ? CCD1_AO_PORT : CCD2_AO_PORT);

	// 初始化SCL和SDA引脚
    LQConfig_GPIO_InitTypeDef_t gpio_init = {
		.Mode  = GPIO_MODE_OUTPUT_OD,       // 推挽输出
		.Pull  = GPIO_RESISTOR_PULL_UP,     // 上拉
		.Speed = GPIO_SPEED_HIGH,           // 高速
	};
	LQ_GPIO_Init((ccd_num == CCD1) ? CCD1_CLK_PIN : CCD2_CLK_PIN, &gpio_init);
	LQ_GPIO_Init((ccd_num == CCD1) ? CCD1_SI_PIN  : CCD2_SI_PIN , &gpio_init);
}

/*************************************************************************
 * @name     LQ_Bin_CCD_Filter
 *
 * @brief    对二值化后的CCD数据进行简单滤波处理
 * @param    ccd_num    CCD 端口号
 * @return   none
 * 
 * @example  LQ_Bin_CCD_Filter(CCD1);
 *************************************************************************/
static void LQ_Bin_CCD_Filter(LQEnum_CCD_t ccd_num)
{
    uint8_t i = 0;
    for (i = 1; i < 128; i++)
    {
        // 如果前一个像素点和后一个像素点都为0 则此像素点为0
        if (CCD_Bin[ccd_num][i] == 1 && CCD_Bin[ccd_num][i - 1] == 0 && CCD_Bin[ccd_num][i + 1] == 0)
        {
            CCD_Bin[ccd_num][i] = 0;
        }
        // 如果前一个像素点和后一个像素点都为1 则此像素点为1
        else if (CCD_Bin[ccd_num][i] == 0 && CCD_Bin[ccd_num][i - 1] == 1 && CCD_Bin[ccd_num][i + 1] == 1)
        {
            CCD_Bin[ccd_num][i] = 1;
        }
    }
}

/*************************************************************************
 * @name     LQ_CCD_Delayus
 *
 * @brief    微秒级延时
 * @param    us    延时时长
 * @return   none
 * 
 * @example  LQ_CCD_Delayus(5);
 *************************************************************************/
static void LQ_CCD_Delayus(uint16_t us)
{
    delay_us(us);
}

/*************************************************************************
 * @name     LQ_CCD_Collect
 *
 * @brief    CCD 采集函数
 * @param    ccd_num    CCD 端口号
 * @param    p          存放采集数据的首地址
 * @return   none
 * 
 * @example  uint16_t ccd_buf[128];
 *           LQ_CCD_Collect(CCD1, ccd_buf); 
 *************************************************************************/
void LQ_CCD_Collect(LQEnum_CCD_t ccd_num, uint16_t *p)
{
	unsigned char i;
	unsigned int  temp = 0;
    {
        CCD_SI_H((ccd_num == CCD1) ? CCD1_SI_PIN : CCD2_SI_PIN);       // SI  = 1 
        LQ_CCD_Delayus(1);
        CCD_CLK_H((ccd_num == CCD1) ? CCD1_CLK_PIN : CCD2_CLK_PIN);    // CLK = 1 
        LQ_CCD_Delayus(1);
        CCD_SI_L((ccd_num == CCD1) ? CCD1_SI_PIN : CCD2_SI_PIN);       // SI  = 0 
        LQ_CCD_Delayus(1);

        for(i=0; i<128; i++) 
        {
            LQ_CCD_Delayus(1);
            CCD_CLK_H((ccd_num == CCD1) ? CCD1_CLK_PIN : CCD2_CLK_PIN);    // CLK = 1 
            LQ_CCD_Delayus(1);
            temp = CCD1_AO_GET((ccd_num == CCD1) ? CCD1_AO_CHANNEL : CCD2_AO_CHANNEL);       
            *p++ = temp;     
            CCD_CLK_L((ccd_num == CCD1) ? CCD1_CLK_PIN : CCD2_CLK_PIN);    // CLK = 0 
        }
        LQ_CCD_Delayus(1);
        CCD_CLK_H((ccd_num == CCD1) ? CCD1_CLK_PIN : CCD2_CLK_PIN);        // CLK = 1 
        LQ_CCD_Delayus(1);
        CCD_CLK_L((ccd_num == CCD1) ? CCD1_CLK_PIN : CCD2_CLK_PIN);        // CLK = 0 
    }

}

/*************************************************************************
 * @name     LQ_CCD_Binaryzation
 *
 * @brief    CCD 二值化处理,并滤波
 * @param    ccd_num    CCD 端口号
 * @param    threshold  阈值
 * @param    ccd_data   存放采集数据的首地址
 * @param    bin_ccd    存放二值化数据的首地址
 * @return   none
 * 
 * @example  uint16_t ccd_buf[128];
 *           LQ_CCD_Binaryzation(CCD1, THRESHOLD, ccd_buf, CCD_Bin[CCD1]); 
 *************************************************************************/
void LQ_CCD_Binaryzation(LQEnum_CCD_t ccd_num, uint16_t threshold, uint16_t *ccd_data, uint8_t *bin_ccd)
{
    uint8_t i = 0;
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

    LQ_Bin_CCD_Filter(ccd_num);//将二值化后的数据进行滤波，并保存到全局数据中
}

/*************************************************************************
 * @name     LQ_CCD_Get_Threshold
 *
 * @brief    CCD 计算动态阈值
 * @param    ccd_data   存放采集数据的首地址
 * @return   阈值((value1_max + value1_min) / 2)
 * 
 * @example  uint16_t ccd_buf[128];
 *           uint16_t threshold = LQ_CCD_Get_Threshold(ccd_buf); 
 *************************************************************************/
uint16_t LQ_CCD_Get_Threshold(uint16_t ccd_data[128])
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

/*************************************************************************
 * @name     LQ_CCD_Get_Threshold
 *
 * @brief    获取 CCD 边线，计算赛道偏差值
 * @param    bin_ccd    二值化后的 CCD 数据数组
 * @return   根据左右边线计算中线偏差
 * 
 * @example  uint16_t ccd_buf[128];
 *           uint16_t error = LQ_CCD_Get_Error(ccd_buf); 
 *************************************************************************/
uint16_t LQ_CCD_Get_Error(uint8_t bin_ccd[128])
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
