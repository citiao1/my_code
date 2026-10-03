# -*- coding: utf-8 -*-
import os
with open(r'D:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\User\main.c', 'rb') as f:
    raw = f.read()

# 找 "static void AngleSensor_Init" 出现位置
idx = 0
while True:
    idx = raw.find(b'static void AngleSensor_Init(void)', idx)
    if idx < 0:
        break
    # 找前一个 "*/\n"
    prev_marker = raw.rfind(b'*/', 0, idx)
    print(f"At {idx}, prev */ at {prev_marker}")
    # 打印该处往后 200 字节
    print("Context:", repr(raw[prev_marker:prev_marker+250]))
    print("---")
    idx += 1

# 找 ReadAngleSensor
idx = 0
while True:
    idx = raw.find(b'static float ReadAngleSensor(void)', idx)
    if idx < 0:
        break
    print(f"\nReadAngleSensor at {idx}")
    prev_marker = raw.rfind(b'*/', 0, idx)
    print(f"prev */ at {prev_marker}")
    print("Context:", repr(raw[prev_marker:prev_marker+200]))
    idx += 1
