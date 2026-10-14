# -*- coding: utf-8 -*-
import os
with open(r'D:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\User\main.c', 'rb') as f:
    raw = f.read()

# 找所有 "/* ADC" 位置
idx = 0
print("All '/* ADC' occurrences:")
while True:
    idx = raw.find(b'/* ADC', idx)
    if idx < 0:
        break
    print(f"At {idx}: {repr(raw[idx:idx+30])}")
    idx += 1
