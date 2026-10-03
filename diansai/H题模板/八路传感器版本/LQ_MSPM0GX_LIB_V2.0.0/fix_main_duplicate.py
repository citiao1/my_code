# -*- coding: utf-8 -*-
"""
修复 main.c：
- 删除旧版残留的 AngleSensor_Init（包括前面那行 /* ADC 通道 12... 注释）
- 合并到新版（NEW_INIT_BLOCK）
"""
import os

# 找 main.c 中第二个 AngleSensor_Init 位置，删除它和前面所有内容直到第一个 AngleSensor_Init
with open(r'D:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\User\main.c', 'rb') as f:
    raw = f.read()

# 找两个 AngleSensor_Init
positions = []
idx = 0
while True:
    idx = raw.find(b'static void AngleSensor_Init(void)', idx)
    if idx < 0:
        break
    positions.append(idx)
    idx += 1

print(f"Found AngleSensor_Init at: {positions}")

if len(positions) >= 2:
    first = positions[0]
    second = positions[1]
    # 第一个 AngleSensor_Init 函数从 "/* ADC 通道 12" 注释开始
    # 找第一个 "/* ADC 通道 12" 注释位置
    start_2nd = raw.find(b'/* ADC', first)
    print(f"start_2nd (/* ADC 通道 12) at: {start_2nd}")
    # 找第二个 AngleSensor_Init 函数结束
    end_2nd = raw.find(b'\n}\n', second)
    if end_2nd < 0:
        end_2nd = raw.find(b'\n}\r\n', second)
    end_pos = end_2nd + 3
    print("end_2nd at:", end_pos)

    # 删除 [start_2nd, end_pos) 区间
    new_raw = raw[:start_2nd] + raw[end_pos:]
    with open(r'D:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\User\main.c', 'wb') as f:
        f.write(new_raw)
    print(f"Written {len(new_raw)} bytes (was {len(raw)})")
else:
    print("Only one AngleSensor_Init found, no merge needed")
