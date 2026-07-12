#include "inductance.h"

// 电磁传感器变量定义
static int ADC_values[5];//L1, L2, M, R1, R2
int L1, L2, M, R1, R2;
/**
 * @brief ADC 值归一化
 * 将 ADC 值归一化到指定范围
 * @param value ADC 值
 * @param min 最小值
 * @param max 最大值
 * @return 归一化后的值
 */
int _adcNormalize(int value, double min, double max)
{
    double normalized = (double)(value - min) / (max - min)* 100.0;
    if(normalized > 100.0) normalized = 100.0;
    if(normalized < 1.0) normalized = 1.0;
    return (int)normalized;
}
/**
 * @brief 去极值平均滤波函数
 * 连续采样 5 次，去掉最大值和最小值，其余取平均
 * @param ch ADC通道号
 * @return 滤波后的有效ADC值
 */
int Get_Filtered_ADC(uint8 ch)
{
    int i;
    int temp;
    int sum = 0;
    int max_val = 0;
    int min_val = 65535; // 赋初值为理论最大可能值

    // 连续采样 5 次
    for (i = 0; i < 3; i++)
    {
        temp = Get_ADCResult(ch);
        sum += temp;
        
        // 筛选最大值和最小值
        if (temp > max_val) max_val = temp;
        if (temp < min_val) min_val = temp;
    }

    // 减去极值后取平均 (总和 - 最大 - 最小) / 3
    return (sum - max_val - min_val) / 3;
}
/**
 * @brief 扫描电磁传感器数据
 * 放在定时器读取五个电磁传感器ADC值
 */
void ScanInductance(void)
{
    // 原ADC值，调试用
     L1 = Get_ADCResult(5);//L1
     L2 = Get_ADCResult(1);//L2
     M = Get_ADCResult(2);//M
     R1 = Get_ADCResult(3);//R1
     R2 = Get_ADCResult(4);//R2
//		 L1 = Get_Filtered_ADC(5); // L1
//    L2 = Get_Filtered_ADC(1); // L2
//    M  = Get_Filtered_ADC(2); // M
//    R1 = Get_Filtered_ADC(3); // R1
//    R2 = Get_Filtered_ADC(4); // R2
		
	
    //归一化后的ADC值
    ADC_values[0] = _adcNormalize(L1, 50, 2500);//L1
    ADC_values[1] = _adcNormalize(L2, 50, 2500);//L2
    ADC_values[2] = _adcNormalize(M, 0, 2500);//M
    ADC_values[3] = _adcNormalize(R1, 20, 2500);//R1
    ADC_values[4] = _adcNormalize(R2, 100, 2500);//R2
}

int* GetInductance()
{
    return ADC_values;  // 返回数组首地址
}
