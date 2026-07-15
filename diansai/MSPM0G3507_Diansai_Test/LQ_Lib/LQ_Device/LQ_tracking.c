/*******************************************************************************
 * @file                LQ_tracking.c
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
#include "LQ_tracking.h"

uint16_t LQ_Tracking_Value[8] = {0};

/* ======================================== 单 ADC 轮询采集模式 ======================================== */

/*************************************************************************
 * @name     LQ_Tracking_Polling_Init
 *
 * @brief    循迹模块初始化，配置ADC与通道选择IO
 * @param    none
 * @return   none
 *
 * @note     初始化ADC采样、多路开关控制引脚S0/S1/S2，启动ADC转换
 *************************************************************************/
void LQ_Tracking_Polling_Init(void)
{
    // ADC 初始化
    LQConfig_ADC_InitTypeDef_t adc_init = {
        .clockSel     = DL_ADC12_CLOCK_SYSOSC,                  // SYSOSC 时钟频率为 32MHz
        .freqRange    = DL_ADC12_CLOCK_FREQ_RANGE_24_TO_32,     // 设置频率范围为 24MHz 到 32MHz
        .divideRatio  = DL_ADC12_CLOCK_DIVIDE_1,                // 时钟分频比为 1，即时钟频率为 32MHz / 1 = 32MHz
        .adcclks      = 100,                                    // ADC 采样周期为 100 个时钟周期，即 1 / 32MHz * 100
        .resolution   = DL_ADC12_SAMP_CONV_RES_12_BIT,          // 12 位分辨率
    };
    LQ_ADC_Init(Tracking_ADC_Port, &adc_init);                  // 初始化 ADC

    LQ_ADC_EnableConversions(Tracking_ADC_Port);                // 使能 ADC 转换通道
    LQ_ADC_StartConversions(Tracking_ADC_Port);                 // 启动 ADC 转换
    // GPIO 初始化
    LQConfig_GPIO_InitTypeDef_t gpio_init = {
        .Mode  = GPIO_MODE_OUTPUT_PP,               // 推挽输出模式
        .Pull  = GPIO_RESISTOR_PULL_DOWN,           // 下拉电阻
        .Speed = GPIO_SPEED_HIGH,                   // 低速
    };
    LQ_GPIO_Init(Tracking_S0_PIN, &gpio_init);      //初始化 S0 引脚为推挽输出模式
    LQ_GPIO_Init(Tracking_S1_PIN, &gpio_init);      //初始化 S1 引脚为推挽输出模式
    LQ_GPIO_Init(Tracking_S2_PIN, &gpio_init);      //初始化 S2 引脚为推挽输出模式
}

/*************************************************************************
 * @name     LQ_Tracking_IO_Set
 *
 * @brief    设置循迹通道选择引脚S2/S1/S0电平
 * @param    S2 : 通道选择位2
 * @param    S1 : 通道选择位1
 * @param    S0 : 通道选择位0
 * @return   none
 *
 * @note     3位二进制组合选择0~7号循迹通道
 *************************************************************************/
static void LQ_Tracking_IO_Set(uint8_t S2, uint8_t S1, uint8_t S0)
{
    // 根据 S2 的输入值设置 S2 引脚电平
    if      (S2 == 1) Tracking_S2_HIGH;
    else if (S2 == 0) Tracking_S2_LOW;
    // 根据 S1 的输入值设置 S1 引脚电平
    if      (S1 == 1) Tracking_S1_HIGH;
    else if (S1 == 0) Tracking_S1_LOW;
    // 根据 S0 的输入值设置 S0 引脚电平
    if      (S0 == 1) Tracking_S0_HIGH;
    else if (S0 == 0) Tracking_S0_LOW;
}

/*************************************************************************
 * @name     LQ_Tracking_Polling_GetValue
 *
 * @brief    轮询读取8路循迹传感器值
 * @param    none
 * @return   none
 *
 * @note     通过 I/O 的电平变化依次切换通道，多次采样取平均，结果存入LQ_Tracking_Value[8]
 *************************************************************************/
void LQ_Tracking_Polling_GetValue(void)
{
    // 定义变量，存储每次采样得到的原始数据、累加有效的采样数据、最终经过处理后的采样数据
    uint32_t data = 0, sum = 0;
    // 定义变量，对该通道采样的总次数、要抛弃的前几次数据、循环变量
    uint8_t num_samples = 5, discard_samples = 3, i = 0, j = 0;
    for (i = 0; i < 8; i++)
    {
        LQ_Tracking_IO_Set((i>>2)&1, (i>>1)&1, (i>>0)&1);
        for (j = 0; j < num_samples; j++)
        {
            // 采样并缩放范围为 0-100
            data = LQ_ADC_GetValue(Tracking_ADC_CH) * 0.02442;
            // 限幅
            data = data >= 100 ? 100 : (data <= 0 ? 0 : data);
            // 抛弃前几次数据
            if (j >= discard_samples)
                sum += data;
        }
        LQ_Tracking_Value[i] = sum / (num_samples - discard_samples);
        sum = 0;
    }
}

/* =========================================== 并行采集模式 ============================================ */

/*************************************************************************
 * @name     LQ_Tracking_Parallel_Init
 *
 * @brief    循迹模块初始化，配置ADC通道
 * @param    none
 * @return   none
 *
 * @note     初始化ADC采样启动ADC转换
 *************************************************************************/
void LQ_Tracking_Parallel_Init(void)
{
    // ADC 初始化
    LQConfig_ADC_InitTypeDef_t adc_init = {
        .clockSel     = DL_ADC12_CLOCK_SYSOSC,                  // SYSOSC 时钟频率为 32MHz
        .freqRange    = DL_ADC12_CLOCK_FREQ_RANGE_24_TO_32,     // 设置频率范围为 24MHz 到 32MHz
        .divideRatio  = DL_ADC12_CLOCK_DIVIDE_1,                // 时钟分频比为 1，即时钟频率为 32MHz / 1 = 32MHz
        .adcclks      = 100,                                    // ADC 采样周期为 100 个时钟周期，即 1 / 32MHz * 100
        .resolution   = DL_ADC12_SAMP_CONV_RES_12_BIT,          // 12 位分辨率
    };
    LQ_ADC_Init(Tracking_ADC_Port, &adc_init);                  // 初始化 ADC

    LQ_ADC_EnableConversions(Tracking_ADC_Port);                // 使能 ADC 转换通道
    LQ_ADC_StartConversions(Tracking_ADC_Port);                 // 启动 ADC 转换
}

/*************************************************************************
 * @name     LQ_Tracking_Parallel_GetValue
 *
 * @brief    并行读取8路循迹传感器值
 * @param    none
 * @return   none
 *
 * @note     依次切换通道，多次采样取平均，结果存入LQ_Tracking_Value[8]
 *************************************************************************/
void LQ_Tracking_Parallel_GetValue(void)
{
    // 定义变量，存储每次采样得到的原始数据、累加有效的采样数据、最终经过处理后的采样数据
    uint32_t data = 0, sum = 0;
    // 定义变量，对该通道采样的总次数、要抛弃的前几次数据、循环变量
    uint8_t num_samples = 5, discard_samples = 3, i = 0, j = 0;
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < num_samples; j++)
        {
            // 采样并缩放范围为 0-100
            data = LQ_ADC_GetValue(Tracking_ADC_Start_CH + i*2) * 0.02442;
            // 限幅
            data = data >= 100 ? 100 : (data <= 0 ? 0 : data);
            // 抛弃前几次数据
            if (j >= discard_samples)
                sum += data;
        }
        LQ_Tracking_Value[i] = sum / (num_samples - discard_samples);
        sum = 0;
    }
}
