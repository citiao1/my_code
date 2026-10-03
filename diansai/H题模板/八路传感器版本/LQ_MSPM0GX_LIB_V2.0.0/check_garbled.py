#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""检查所有相关文件的实际内容"""
import os

ROOT = r"d:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0"

files = [
    r"Code\Middle\mid_line.h",
    r"Code\Middle\mid_line.c",
    r"Code\Middle\mid_key.c",
    r"Code\Middle\mid_oled.c",
    r"Code\Middle\mid_oled.h",
    r"Code\Middle\mid_pid.c",
    r"Code\Middle\mid_pid.h",
    r"User\main.c",
    r"Code\App\app_scheduler.c",
    r"User\include.h",
    r"User\include.c",
]

for relpath in files:
    full = os.path.join(ROOT, relpath)
    if not os.path.exists(full):
        print(f"NOT FOUND: {relpath}")
        continue
    with open(full, "rb") as f:
        data = f.read()
    # check for UTF-8 replacement character
    rep_count = data.count(b'\xef\xbf\xbd')
    has_garbled = '锟' in data.decode('gb2312', errors='ignore') or '锛' in data.decode('gb2312', errors='ignore') or '鈥' in data.decode('gb2312', errors='ignore')
    has_ffd = rep_count > 0
    print(f"== {relpath}")
    print(f"   size={len(data)}  rep_count={rep_count}  has_garbled={has_garbled}")
    if has_ffd:
        print(f"   *** 文件含 UTF-8 替换字符 (U+FFFD)，中文已损坏 ***")
