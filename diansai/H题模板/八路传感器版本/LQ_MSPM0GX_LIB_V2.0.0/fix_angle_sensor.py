#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""在 main.c 中使用 LQ_ADC API 重写 g_angle_sensor 角度传感器代码"""
import os

TARGET = r"d:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\User\main.c"

with open(TARGET, "rb") as f:
    data = f.read()

# 用基于 LQ 库的简化版替换原 helper 代码块
old_helper = (b'/* ============================================================\n'
              b' *  WDD35D4 \xbd\xd8\xb6\xfe\xb4\xab\xc6\xf7\xa3\xa8PA22 ADC\xa3\xa9\n'
              b' *  - WDD35D4-5K \xbe\xab\xc7\xd0\xb5\xe7\xd4\xb4\xc6\xf7\xa3\xbaVCC=3.3V, GND, \xd6\xd0\xbc\xe4\xb3\xe5\xbd\xd8\xc8\xcf PA22\n'
              b' *  - \xd6\xd8\xd6\xb5 5K\xce\xa8\xa3\xac\xb5\xe7\xc1\xaa\xbd\xc7\xb6\xfe 0~345\xc2\xb0\xa3\xac\xbb\xfa\xbd\xcc\xbd\xc7\xb6\xfe 0~360\xc2\xb0\n'
              b' *  - PA22 \xcd\xf9\xd5\xfb DL_GPIO_initPeripheralAnalogFunction(IOMUX_PINCM47) \xc5\xe4\xd6\xc3\xce\xaa\xc4\xa3\xbb\xe1\xca\xe4\xc8\xeb\n'
              b' *  - 12-bit ADC \xc3\xfc\xc1\xbf\xb3\xa1 0~4095 \xb5\xd4\xd3\xa6 0~3.3V\xa3\xac\xb5\xe7\xc1\xaa = (adc/4095) * 3.3V\n'
              b' *  - \xbd\xc7\xb6\xfe = (adc / 4095.0f) * 345.0f\n'
              b' *  - app_debug.c \xd2\xd4\xd3\xc3 g_angle_sensor\xa3\xa8VOFA+ \xcd\xa8\xd6\xda 10 \xcf\xd4\xca\xbe\xa3\xa9\n'
              b' * ============================================================ */\n'
              b'\n'
              b'/* \xbd\xc7\xb6\xfe\xb4\xab\xc6\xf7\xd4\xad\xc1\xbf ADC \xd6\xb5\xa3\xa80~4095\xa3\xa9\xba\xcd\xbb\xbb\xc1\xbf\xba\xf3\xb5\xc4\xbd\xc7\xb6\xfe\xa3\xa80~345\xc2\xb0\xa3\xa9 */\n'
              b'volatile uint16_t g_angle_adc = 0u;\n'
              b'float g_angle_sensor = 0.0f;\n'
              b'\n'
              b'/* ADC \xcd\xa8\xd6\xda 12\xa3\xa8PA22\xa3\xa9\xc5\xfa\xbb\xaf\xa3\xbaPA22 -> ADC \xc4\xa3\xbb\xe1\xca\xe4\xc8\xeb */\n'
              b'static void AngleSensor_Init(void)\n'
              b'{\n'
              b'    /* PA22 \xc5\xe4\xd6\xc3\xce\xaa\xc4\xa3\xbb\xe1\xca\xe4\xc8\xeb\xc4\xa3\xbc\xb6\xa3\xa8\xb9\xd8\xb1\xd5\xca\xe4\xc8\xeb\xbb\xba\xcd\xbf\xa3\xa9 */\n'
              b'    DL_GPIO_initPeripheralAnalogFunction(IOMUX_PINCM47);\n'
              b'    /* ADC \xcd\xa8\xd6\xda 12\xa3\xba\xb5\xa5\xb4\xce\xd7\xaa\xbb\xbb\xa3\xac12-bit\xa3\xac\xc8\xed\xbc\xfe\xb6\xaf\xb0\xe5 */\n'
              b'    DL_ADC12_enablePower(ADC0);\n'
              b'    DL_ADC12_initSingleShot(ADC0, DL_ADC12_REPEAT_MODE_DISABLED,\n'
              b'                            DL_ADC12_SAMPLING_SOURCE_AUTO, DL_ADC12_TRIG_SRC_SOFTWARE,\n'
              b'                            DL_ADC12_SAMPLING_SOURCE_AUTO);\n'
              b'    DL_ADC12_configChannelSingleEnded(ADC0, DL_ADC12_INPUT_CHAN_12,\n'
              b'                                      DL_ADC12_REFERENCE_VOLTAGE_VDDA,\n'
              b'                                      DL_ADC12_SAMPLE_TIMER_SOURCE_AUTO);\n'
              b'    /* \xcd\xf9\xd5\xfb\xb2\xbb\xbc\xcc\xc1\xaa ADC */\n'
              b'    DL_ADC12_startCalibration(ADC0);\n'
              b'    while (DL_ADC12_isCalibrationBusy(ADC0)) { /* \xb5\xc8\xd3\xa6\xbc\xcc\xc1\xaa\xcd\xea\xb3\xc9 */ }\n'
              b'    DL_ADC12_enableConversions(ADC0);\n'
              b'}\n'
              b'\n'
              b'/* \xcc\xe1\xca\xd5 PA22 ADC \xd4\xad\xc1\xbf\xd6\xb5 + \xbb\xbb\xc1\xbf\xce\xaa\xbd\xc7\xb6\xfe\xa3\xa80~345\xc2\xb0\xa3\xa9\n'
              b' * @return  \xbd\xc7\xb6\xfe\xa3\xa8\xc2\xb0\xa3\xac0~345\xa3\xa9\n'
              b' */\n'
              b'static float ReadAngleSensor(void)\n'
              b'{\n'
              b'    /* \xc8\xed\xbc\xfe\xb6\xaf\xb0\xe5\xb4\xce\xb4\xce ADC \xd7\xaa\xbb\xbb */\n'
              b'    DL_ADC12_startConversion(ADC0);\n'
              b'    while (DL_ADC12_isConversionStarted(ADC0) == false) { /* \xb5\xc8\xd3\xa6 */ }\n'
              b'    /* \xb5\xc8\xd3\xa6\xd7\xaa\xbb\xbb\xcd\xea\xb3\xc9 */\n'
              b'    while (DL_ADC12_getRawStatus(ADC0) != DL_ADC12_RAW_STATUS_READY) { /* \xb5\xc8\xd3\xa6 */ }\n'
              b'    /* \xcc\xe1\xca\xd5\xbd\xe1\xca\xf8 */\n'
              b'    uint16_t adc = DL_ADC12_getMemResult(ADC0, DL_ADC12_MEM_IDX_0);\n'
              b'    g_angle_adc = adc;\n'
              b'    /* 12-bit ADC \xc3\xfc\xc1\xbf\xb3\xa1 0~4095 -> 0~3.3V -> 0~345\xc2\xb0 */\n'
              b'    g_angle_sensor = ((float)adc / 4095.0f) * 345.0f;\n'
              b'    return g_angle_sensor;\n'
              b'}\n'
              b'\n')

assert old_helper in data, "old helper block not found"

# 替换为基于 LQ_ADC 的简化版
new_helper_lines = [
    "/* ============================================================",
    " *  WDD35D4 角度传感器（PA22 ADC 通道 7）",
    " *  - WDD35D4-5K 精密电位器：VCC=3.3V, GND, 中间抽头接 PA22",
    " *  - 阻值 5KΩ，电气角度 0~345°",
    " *  - PA22 -> ADC0 Channel 7（LQ_ADC 库通道：ADC0_Channel_7_Pin_A_22）",
    " *  - 12-bit ADC 满量程 0~4095 对应 0~3.3V",
    " *  - 角度 = (adc / 4095.0f) * 345.0f",
    " *  - app_debug.c 引用 g_angle_sensor（VOFA+ 通道 10 显示）",
    " * ============================================================ */",
    "",
    "/* 角度传感器原始 ADC 值（0~4095）和换算后的角度（0~345°） */",
    "volatile uint16_t g_angle_adc = 0u;",
    "float g_angle_sensor = 0.0f;",
    "",
    "/* WDD35D4 角度传感器初始化：调用 LQ_ADC 库初始化 ADC0 */",
    "static void AngleSensor_Init(void)",
    "{",
    "    LQConfig_ADC_InitTypeDef_t adc_init = {0};",
    "    /* 默认 12-bit、VDDA 参考、80MHz 系统时钟下 ADC 时钟 40MHz（最大） */",
    "    LQ_ADC_Init(ADC_Port_0, &adc_init);",
    "}",
    "",
    "/* 读取 WDD35D4 角度（0~345°）",
    " * @return  角度（度，0~345）",
    " */",
    "static float ReadAngleSensor(void)",
    "{",
    "    /* PA22 对应 LQ_ADC 通道：ADC0_Channel_7_Pin_A_22 */",
    "    uint16_t adc = LQ_ADC_GetValue(ADC0_Channel_7_Pin_A_22);",
    "    g_angle_adc = adc;",
    "    /* 12-bit ADC 满量程 0~4095 -> 0~3.3V -> 0~345° */",
    "    g_angle_sensor = ((float)adc / 4095.0f) * 345.0f;",
    "    return g_angle_sensor;",
    "}",
    "",
]
new_helper = "\n".join(new_helper_lines) + "\n"
new_helper_bytes = new_helper.encode("gb2312")

data = data.replace(old_helper, new_helper_bytes, 1)

# 移除 LQ_adc.h 引用，因为 LQ_ADC_xx 已通过 include.h 引入
old_inc = b'#include "LQ_adc.h"\n'
if old_inc in data:
    data = data.replace(old_inc, b'', 1)

with open(TARGET, "wb") as f:
    f.write(data)
print("main.c 角度传感器改用 LQ_ADC 完成")
