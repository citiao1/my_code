# -*- coding: utf-8 -*-
"""
清理 main.c 中的重复函数。
- 删除多余的 AngleSensor_Init（旧版简短版 + 旧版注释）
- 删除多余的 main 函数（旧版）
- 只保留正确的新版（NEW_INIT_BLOCK）部分
"""
import os

with open(r'D:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\User\main.c', 'rb') as f:
    raw = f.read()

# 策略：
# 1) 找第一个 "static void AngleSensor_Init(void)" 位置
# 2) 找 "int main(void)" 位置（新版）
# 3) 删除 [first AngleSensor_Init 位置, 第一个 main 位置) 中所有内容
# 4) 重新插入正确的新版 AngleSensor_Init + ReadAngleSensor

idx_ang = raw.find(b'static void AngleSensor_Init(void)')
idx_main = raw.find(b'int main(void)')
print(f"AngleSensor_Init at: {idx_ang}")
print(f"int main at: {idx_main}")

# 找第一个 AngleSensor_Init 之前的注释 "/* ADC 通道 12" 开始位置
# 但要保留前面 "volatile uint16_t g_angle_adc" 那一段
start_to_remove = raw.find(b'/* ADC', idx_ang)
print(f"/* ADC at: {start_to_remove}")

# 删除 [start_to_remove, idx_main) 中所有内容
new_raw = raw[:start_to_remove] + raw[idx_main:]

# 重新插入 NEW_INIT_BLOCK
NEW_INIT_BLOCK = (
    b"\n"
    b"/* WDD35D4 \xbd\xc7\xb6\xc8\xb4\xab\xc1\xcf\xbb\xfa\xa3\xbaPA22 ADC \xb5\xc4\xb3\xcc\xca\xbc\xca\xc7\xbd\xe1\xca\xf8\n"
    b" *   PA22 \xd7\xe9\xa1\xa1LQ_ADC_GetValue \xc4\xda\xc4\xe3\xb5\xc4 ADC \xb4\xa5\xb7\xa2\xc5\xe4\xd6\xc3\n"
    b" *   \xd7\xee\xbf\xc6\xc4\xa3\xca\xbd\xca\xe4\xc8\xeb\xca\xf8\xd0\xc5\xc4\xe3\xd4\xf6\xc3\xc5\xb5\xc7\xd2\xd4\xbe\xed\n"
    b" */\n"
    b"static void AngleSensor_Init(void)\n"
    b"{\n"
    b"    /* PA22 \xc5\xe4\xd6\xc3\xce\xaa\xc4\xa3\xc4\xe2\xca\xe4\xc8\xeb\xc4\xa3\xca\xbd\xa3\xa8\xb9\xd8\xb1\xd5\xca\xfd\xd7\xd6\xca\xe4\xc8\xeb\xbb\xba\xb3\xe5\xa3\xa9 */\n"
    b"    DL_GPIO_initPeripheralAnalogFunction(IOMUX_PINCM47);\n"
    b"}\n"
    b"\n"
    b"/* \xbc\xec\xb6\xc1 PA22 ADC \xca\xd5\xbf\xd8\xd6\xb5\xa1\xa3\xb1\xe9\xc2\xeb\xce\xaa 0~345 \xb6\xd0\xc1\xa2\n"
    b" * @return  \xb6\xd0\xc1\xa2\xa3\xa8\xc1\xf9\xa3\xa90~345\xa1\xa3\n"
    b" */\n"
    b"static float ReadAngleSensor(void)\n"
    b"{\n"
    b"    /* \xd5\xe2\xc0\xe8\xd6\xd0\xb1\xe9\xc2\xeb\xb5\xc4 ADC \xd5\xfd\xb5\xc0 7\xa3\xbaPA22 <=> ADC0_Channel_7_Pin_A_22 */\n"
    b"    uint16_t adc = LQ_ADC_GetValue(ADC0_Channel_7_Pin_A_22);\n"
    b"    g_angle_adc = adc;\n"
    b"    /* 12-bit ADC \xbf\xed\xb4\xa5\xa3\xba0~4095 -> 0~3.3V -> 0~345\xb6\xd0 */\n"
    b"    g_angle_sensor = ((float)adc / 4095.0f) * 345.0f;\n"
    b"    return g_angle_sensor;\n"
    b"}\n"
)

# 在 main 前面插入 NEW_INIT_BLOCK
# 找到 main 之前的 "/* ============================================================\n *  main 函数\n * ============================================================ */" 块
# 简单点：直接在 main 前插入
# main 之前会有一个空行 + 注释块，先找 main 注释
# 找 main(void) 之前最近的 "/* =" 块
main_comment_start = raw.rfind(b'/* ====', 0, idx_main)
print(f"main comment block at: {main_comment_start}")

# 拼起来
# raw[:start_to_remove] 是 angle sensor 之前的所有
# raw[idx_main:] 是 main 之后的所有
# 插入：保留 main 上面的注释块，从 angle sensor 开始替换
# 实际上 NEW_INIT_BLOCK 包含了 angle sensor 的注释
# 让我用 main 之前的注释块开始
final_raw = raw[:main_comment_start] + NEW_INIT_BLOCK + b"\n" + raw[main_comment_start:]

with open(r'D:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\User\main.c', 'wb') as f:
    f.write(final_raw)
print(f"Written {len(final_raw)} bytes (was {len(raw)})")
