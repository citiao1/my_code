# -*- coding: utf-8 -*-
import os
with open(r'D:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\User\main.c', 'rb') as f:
    raw = f.read()

# 直接看 start = 5099 附近
print("Around 5099:")
print(repr(raw[5080:5300]))
