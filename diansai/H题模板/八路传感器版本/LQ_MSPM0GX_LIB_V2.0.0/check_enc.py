#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""检查指定文件的编码"""
import os
import sys

ROOT = r"d:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0"

files_to_check = [
    r"Code\Middle\mid_line.h",
    r"Code\Middle\mid_line.c",
    r"Code\Middle\mid_key.c",
    r"Code\Middle\mid_oled.c",
    r"Code\Middle\mid_oled.h",
    r"User\main.c",
    r"Code\App\app_scheduler.c",
]

for relpath in files_to_check:
    full = os.path.join(ROOT, relpath)
    if not os.path.exists(full):
        print(f"NOT FOUND: {relpath}")
        continue
    with open(full, "rb") as f:
        data = f.read()
    # detect BOM
    bom = ""
    if data.startswith(b"\xef\xbb\xbf"):
        bom = "UTF-8-BOM"
    elif data.startswith(b"\xff\xfe"):
        bom = "UTF-16-LE-BOM"
    elif data.startswith(b"\xfe\xff"):
        bom = "UTF-16-BE-BOM"
    # try utf-8 decode
    try:
        data.decode("utf-8")
        utf8_ok = True
    except UnicodeDecodeError as e:
        utf8_ok = False
        utf8_err = str(e)
    # try gb2312 decode
    try:
        data.decode("gb2312")
        gb_ok = True
    except UnicodeDecodeError as e:
        gb_ok = False
        gb_err = str(e)
    print(f"== {relpath}")
    print(f"   size={len(data)}  bom={bom}  utf8_ok={utf8_ok}  gb2312_ok={gb_ok}")
    if not utf8_ok:
        print(f"   utf8_err={utf8_err}")
    if not gb_ok:
        print(f"   gb_err={gb_err}")
