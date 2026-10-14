#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""在 main.c 中添加 g_angle_sensor 角度传感器相关代码（PA22 ADC 读取 WDD35D4）"""
import os

TARGET = r"d:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\User\main.c"

with open(TARGET, "rb") as f:
    data = f.read()

# 1) 在 include 区域添加 LQ_adc.h
old_inc = b'#include "include.h"\n#include "LQ_device.h"\n#include "LQ_lsm6dsv16x.h"\n'
new_inc = (b'#include "include.h"\n'
           b'#include "LQ_device.h"\n'
           b'#include "LQ_lsm6dsv16x.h"\n'
           b'#include "LQ_adc.h"\n')
assert old_inc in data, "include block not found"
data = data.replace(old_inc, new_inc, 1)

# 2) 在 main 之前添加 g_angle_sensor 全局变量 + 辅助函数
helper_code_lines = [
    "",
    "/* ============================================================",
    " *  WDD35D4 角度传感器（PA22 ADC）",
    " *  - WDD35D4-5K 精密电位器：VCC=3.3V, GND, 中间抽头接 PA22",
    " *  - 阻值 5KΩ，电气角度 0~345°，机械角度 0~360°",
    " *  - PA22 通过 DL_GPIO_initPeripheralAnalogFunction(IOMUX_PINCM47) 配置为模拟输入",
    " *  - 12-bit ADC 满量程 0~4095 对应 0~3.3V，电压 = (adc/4095) * 3.3V",
    " *  - 角度 = (adc / 4095.0f) * 345.0f",
    " *  - app_debug.c 引用 g_angle_sensor（VOFA+ 通道 10 显示）",
    " * ============================================================ */",
    "",
    "/* 角度传感器原始 ADC 值（0~4095）和换算后的角度（0~345°） */",
    "volatile uint16_t g_angle_adc = 0u;",
    "float g_angle_sensor = 0.0f;",
    "",
    "/* ADC 通道 12（PA22）初始化：PA22 -> ADC 模拟输入 */",
    "static void AngleSensor_Init(void)",
    "{",
    "    /* PA22 配置为模拟输入模式（关闭数字输入缓冲） */",
    "    DL_GPIO_initPeripheralAnalogFunction(IOMUX_PINCM47);",
    "    /* ADC 通道 12：单次转换、12-bit、软件触发 */",
    "    DL_ADC12_enablePower(ADC0);",
    "    DL_ADC12_initSingleShot(ADC0, DL_ADC12_REPEAT_MODE_DISABLED,",
    "                            DL_ADC12_SAMPLING_SOURCE_AUTO, DL_ADC12_TRIG_SRC_SOFTWARE,",
    "                            DL_ADC12_SAMPLING_SOURCE_AUTO);",
    "    DL_ADC12_configChannelSingleEnded(ADC0, DL_ADC12_INPUT_CHAN_12,",
    "                                      DL_ADC12_REFERENCE_VOLTAGE_VDDA,",
    "                                      DL_ADC12_SAMPLE_TIMER_SOURCE_AUTO);",
    "    /* 启动并校准 ADC */",
    "    DL_ADC12_startCalibration(ADC0);",
    "    while (DL_ADC12_isCalibrationBusy(ADC0)) { /* 等待校准完成 */ }",
    "    DL_ADC12_enableConversions(ADC0);",
    "}",
    "",
    "/* 读取 PA22 ADC 原始值 + 换算为角度（0~345°）",
    " * @return  角度（度，0~345）",
    " */",
    "static float ReadAngleSensor(void)",
    "{",
    "    /* 软件触发一次 ADC 转换 */",
    "    DL_ADC12_startConversion(ADC0);",
    "    while (DL_ADC12_isConversionStarted(ADC0) == false) { /* 等待 */ }",
    "    /* 等待转换完成 */",
    "    while (DL_ADC12_getRawStatus(ADC0) != DL_ADC12_RAW_STATUS_READY) { /* 等待 */ }",
    "    /* 读取结果 */",
    "    uint16_t adc = DL_ADC12_getMemResult(ADC0, DL_ADC12_MEM_IDX_0);",
    "    g_angle_adc = adc;",
    "    /* 12-bit ADC 满量程 0~4095 -> 0~3.3V -> 0~345° */",
    "    g_angle_sensor = ((float)adc / 4095.0f) * 345.0f;",
    "    return g_angle_sensor;",
    "}",
    "",
    "/* ============================================================",
    " *  主函数",
    " * ============================================================ */",
    "",
]
helper_code = "\n".join(helper_code_lines) + "\n"
helper_bytes = helper_code.encode("gb2312")

# 找到 "int main(void)" 之前的位置插入 helper
marker = b'int main(void)\n'
idx = data.find(marker)
assert idx >= 0, "main function not found"
data = data[:idx] + helper_bytes + data[idx:]

# 3) 在 main 中 IMU 初始化后调用 AngleSensor_Init
# 使用实际字节: /* 8. 底盘三环 PID 状态初始化 */
old_init = (b'    delay_ms(100);\n'
            b'\n'
            b'    /* 8. \xb5\xd7\xc5\xcc\xc8\xfd\xbb\xb7 PID \xd7\xb4\xcc\xac\xb3\xf5\xca\xbc\xbb\xaf */\n'
            b'    MID_Chassis_Init();\n')
assert old_init in data, "init block not found"

new_init = ("    delay_ms(100);\n"
            "\n"
            "    /* 7.5 WDD35D4 角度传感器初始化（PA22 ADC） */\n"
            "    AngleSensor_Init();\n"
            "\n"
            "    /* 8. 底盘三环 PID 状态初始化 */\n"
            "    MID_Chassis_Init();\n")
new_init_bytes = new_init.encode("gb2312")
data = data.replace(old_init, new_init_bytes, 1)

# 4) 在主循环里调用 ReadAngleSensor（在 APP_Scheduler_Run 后）
old_in_loop = b'        APP_Scheduler_Run();\n'
new_in_loop = ("        APP_Scheduler_Run();\n"
               "\n"
               "        /* 读取 WDD35D4 角度传感器（用于 VOFA+ 通道 10 显示） */\n"
               "        (void)ReadAngleSensor();\n")
new_in_loop_bytes = new_in_loop.encode("gb2312")
# Replace only the first occurrence (in main loop)
assert data.count(old_in_loop) >= 1, "APP_Scheduler_Run line not found in loop"
data = data.replace(old_in_loop, new_in_loop_bytes, 1)

with open(TARGET, "wb") as f:
    f.write(data)
print("main.c 角度传感器添加完成")
