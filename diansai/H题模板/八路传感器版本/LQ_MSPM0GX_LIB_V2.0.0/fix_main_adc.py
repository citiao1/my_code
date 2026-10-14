# -*- coding: utf-8 -*-
"""
修复 main.c 中 ADC 函数名错误。
简化方案：直接用 LQ_ADC_GetValue(ADC0_Channel_7_Pin_A_22) 代替所有 DL_ADC12_* 调用。
LQ_ADC_GetValue 内部已经做了完整的 ADC 配置和阻塞式采集。
"""

# 新版 AngleSensor_Init - 只配 IOMUX，不动 ADC（LQ_ADC_GetValue 内部会配）
# 新版 ReadAngleSensor - 调用 LQ_ADC_GetValue
# 注释用 GB2312/GBK 编码写入（用户项目要求）

NEW_INIT_BLOCK = (
    b"\n"
    b"/* \xc8\xbe\xb5\xe3\xb4\xfa\xc1\xbf\xb6\xfe\xb3\xa3\xba\xcd\xd0\xc5\xc4\xe3\xb3\xcc\xca\xbc\xa3\xba\n"
    b" *   PA22 \xd7\xe9\xa1\xa1LQ_ADC_GetValue \xc4\xda\xc4\xe3\xb5\xc4 ADC \xb4\xa5\xb7\xa2\xc5\xe4\xd6\xc3\n"
    b" *   \xc8\xbe\xb5\xe3\xb4\xfa\xc1\xbf\xb6\xfe\xb3\xa3\xba\xd2\xd4\xd7\xe9\xd2\xd1\xb5\xc3 PA22 \xd2\xd4\xca\xc7\n"
    b" *   \xc4\xa3\xca\xbd\xca\xe4\xc8\xeb\xca\xf8\xd0\xc5\xc4\xe3\xd4\xf6\xc3\xc5\xb5\xc7\n"
    b" *   \xc7\xeb\xc1\xcb\xf6\xce\xd2\xd1\xb6\xd4\xc6\xf4\xb6\xaf\xb2\xa2\xd0\xa3\xc1\xcb\xb5\xc8\xf1\xca\xd2\xd4\xbe\xad\xa1\xa3\n"
    b" */\n"
    b"static void AngleSensor_Init(void)\n"
    b"{\n"
    b"    /* PA22 \xc5\xe4\xd6\xc3\xce\xaa\xc4\xa3\xc4\xe2\xca\xe4\xc8\xeb\xc4\xa3\xca\xbd\xa3\xa8\xb9\xd8\xb1\xd5\xca\xfd\xd7\xd6\xca\xe4\xc8\xeb\xbb\xba\xb3\xe5\xa3\xa9 */\n"
    b"    DL_GPIO_initPeripheralAnalogFunction(IOMUX_PINCM47);\n"
    b"}\n"
    b"\n"
    b"/* \xbc\xec\xb6\xc1 PA22 ADC \xca\xd5\xbf\xd8\xd6\xb5\xa1\xa3\xb1\xe9\xc2\xeb\xce\xaa 0~345 \xb6\xd0\xc1\xa2\xcb\xb5\n"
    b" * @return  \xb6\xd0\xc1\xa2\xa3\xa8\xc1\xf9\xa3\xa90~345\xa1\xa3\n"
    b" */\n"
    b"static float ReadAngleSensor(void)\n"
    b"{\n"
    b"    /* \xd5\xe2\xc0\xe8\xd6\xd0\xb1\xe9\xc2\xeb\xb5\xc4 ADC \xd5\xfd\xb5\xc0 7\xa3\xba\n"
    b"     *   PA22 <=> ADC0_Channel_7_Pin_A_22 \xa3\xa8LQ_Lib \xc6\xe4\xd3\xef\xc6\xb5\xb2\xfb\xb3\xc6\xa3\xa9\n"
    b"     *   LQ_ADC_GetValue \xbb\xf2\xc5\xfa\xd0\xd4\xc6\xf4\xb6\xaf\xbb\xf2\xc5\xfa\xb5\xc8\xb4\xfd\xcd\xea\xb3\xc9\xc7\xbb\xd6\xd0\xb1\xe4\xc1\xbf\n"
    b"     */\n"
    b"    uint16_t adc = LQ_ADC_GetValue(ADC0_Channel_7_Pin_A_22);\n"
    b"    g_angle_adc = adc;\n"
    b"    /* 12-bit ADC \xbf\xed\xb4\xa5\xa3\xba0~4095 -> 0~3.3V -> 0~345\xb6\xd0 */\n"
    b"    g_angle_sensor = ((float)adc / 4095.0f) * 345.0f;\n"
    b"    return g_angle_sensor;\n"
    b"}\n"
)

# 读 main.c
with open(r'D:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\User\main.c', 'rb') as f:
    raw = f.read()

# 找 AngleSensor_Init 函数（包括前面的注释行）
# 从 main.c 看：\n\n/* 角度传感器原始 ADC 值（0~4095）和缓存的角度（0~345度） */\nvolatile uint16_t g_angle_adc = 0u;\n\n/* ADC 通道 12（PA22）初始化（PA22 -> ADC 模拟输入） */\nstatic void AngleSensor_Init(void)
idx = raw.find(b'static void AngleSensor_Init(void)')
if idx < 0:
    raise RuntimeError("AngleSensor_Init not found")

# 找到这个块前一个"volatile uint16_t g_angle_adc"前一行
# 这样替换不会破坏 volatile 变量声明
# 实际看 raw bytes：
#   ...\n/* 角度传感器原始 ADC 值... */\nvolatile uint16_t g_angle_adc = 0u;\nfloat g_angle_sensor = 0.0f;\n\n/* ADC 通道 12（PA22）初始化... */\nstatic void AngleSensor_Init(void)\n{ ... }\n\n/* 读取 PA22... */\nstatic float ReadAngleSensor(void)\n{ ... }\n

# 替换：从"/* ADC 通道 12（PA22）初始化"开始
start_marker = b'/* ADC \xcd\xa8\xb5\xc0 12\xa3\xba'
start = raw.find(start_marker)
if start < 0:
    raise RuntimeError("start marker not found")

# 找到 ReadAngleSensor 函数结束的 "}" 后面
# 找 ReadAngleSensor 函数开始
r_start = raw.find(b'static float ReadAngleSensor(void)', start)
# 找 ReadAngleSensor 函数结束 "}\n"
r_end = raw.find(b'\n}\n', r_start)
if r_end < 0:
    r_end = raw.find(b'\n}\r\n', r_start)
end = r_end + 3

print(f"Replace [{start} .. {end}) ({end - start} bytes)")

# 拼接
new_raw = raw[:start] + NEW_INIT_BLOCK + raw[end:]

# 写回
with open(r'D:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\User\main.c', 'wb') as f:
    f.write(new_raw)

print(f"Written {len(new_raw)} bytes (was {len(raw)})")
