#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""修改 mid_key.c 在 s_handle_start() 中添加清除紧急停车标志的代码"""
import os

TARGET = r"d:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\Code\Middle\mid_key.c"

# GB2312 编码读写
with open(TARGET, "rb") as f:
    data = f.read()

# 1) 添加 #include "mid_line.h"
old_inc = b'#include "mid_beep.h"           /* MID_Beep_Beep                       */\n'
new_inc = (b'#include "mid_beep.h"           /* MID_Beep_Beep                       */\n'
           b'#include "mid_line.h"           /* g_line_emergency_stopped            */\n')
assert old_inc in data, "include 替换位置未找到"
data = data.replace(old_inc, new_inc, 1)

# 2) 在 MID_Chassis_Start() 之前插入清紧急停车标志
old_block = b'    /* 2) \xd7\xbb\xbb\xb6\xc4\xac\xc8\xcf\xb4\xe6\xd6\xb5\xcb\xd9\xd6\xb5\xd2\xbb\x80\xa3\x40\x76\x61\x6c\x75\x65\x5f\x76\x78\x00\xa3\xa1\x70\x61\x72\x61\x6d\x2e\x63\x20\xc4\xac\xc8\xcf\x20\x34\x30\x20\x6d\x6d\x2f\x73\x0a'
# Actually this is the comment line. Let me find the unique block "3) \xd4\xf2\xbf\xaa\xcb\xfb\xc8\xba" which means "3) 打开三环"
# Open three-loop enable
old_start_block = b'    /* 3) \xbf\xaa\xbf\xd8\xcb\xfb\xc8\xba\xca\xb9\xd3\xc3 */\n    MID_Chassis_Start();\n'
new_start_block = (b'    /* 3) \xbf\xaa\xbf\xd8\xcb\xfb\xc8\xba\xca\xb9\xd3\xc3 */\n'
                   b'    MID_Chassis_Start();\n')
# Just add new step 3) for clearing emergency, and renumber

# The simpler approach: find unique pattern around MID_Chassis_Start()
# Old numbering: 1) 校准 2) 恢复速度 3) 打开三环 4) 标记 5) 提示音
# New numbering: 1) 校准 2) 恢复速度 3) 清紧急标志 4) 打开三环 5) 标记 6) 提示音

# Let me find the exact bytes
# "    /* 3) 打开三环使能 */\n    MID_Chassis_Start();\n\n    /* 4) 标记为运行中 */\n    s_running = 1U;\n\n    /* 5) 启动成功提示音：1 声短\"滴\" */\n    MID_Beep_Beep(1, 0.08f, 0.0f);\n}"
# I'll search for the exact 3) 打开三环 marker
marker = '    /* 3)'.encode('gb2312')
idx = data.find(marker)
print(f"找到 '3)' 标记位置: {idx}")
if idx >= 0:
    # Print context
    print("Context (decoded):")
    print(data[idx:idx+200].decode('gb2312', errors='replace'))

# Find the function body and replace
old_func = ('    /* 2) 恢复默认目标速度（param.c 默认 40 mm/s，Stop 时已被清 0） */\n'
            '    g_param.chassis_vx = __chassis_vx;\n'
            '\n'
            '    /* 3) 打开三环使能 */\n'
            '    MID_Chassis_Start();\n'
            '\n'
            '    /* 4) 标记为运行中 */\n'
            '    s_running = 1U;\n'
            '\n'
            '    /* 5) 启动成功提示音：1 声短"滴" */\n'
            '    MID_Beep_Beep(1, 0.08f, 0.0f);\n'
            '}')
old_func_bytes = old_func.encode('gb2312')
new_func = ('    /* 2) 恢复默认目标速度（param.c 默认 40 mm/s，Stop 时已被清 0） */\n'
            '    g_param.chassis_vx = __chassis_vx;\n'
            '\n'
            '    /* 3) 清除 5 路红外紧急停车标志（5 路全 0/全 1 触发的停车状态） */\n'
            '    g_line_emergency_stopped = 0U;\n'
            '\n'
            '    /* 4) 打开三环使能 */\n'
            '    MID_Chassis_Start();\n'
            '\n'
            '    /* 5) 标记为运行中 */\n'
            '    s_running = 1U;\n'
            '\n'
            '    /* 6) 启动成功提示音：1 声短"滴" */\n'
            '    MID_Beep_Beep(1, 0.08f, 0.0f);\n'
            '}')
new_func_bytes = new_func.encode('gb2312')

assert old_func_bytes in data, f"old_func 未找到，old_func:\n{old_func}"
data = data.replace(old_func_bytes, new_func_bytes, 1)

with open(TARGET, "wb") as f:
    f.write(data)

print("修改完成")
