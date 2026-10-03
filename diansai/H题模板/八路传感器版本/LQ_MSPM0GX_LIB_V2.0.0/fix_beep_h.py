#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""添加 MID_Beep_Beep 声明到 mid_beep.h"""
import os

TARGET = r"d:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\Code\Middle\mid_beep.h"

with open(TARGET, "rb") as f:
    data = f.read()

# 在 MID_Beep_Toggle(void); 之后、#endif 之前插入新声明
old_block = ('/* 翻转蜂鸣器状态（响<->停）\n'
             ' * @note  内部调用 LQ_GPIO_TogglePin，适合主循环周期性翻转做"滴滴"报警\n'
             ' */\n'
             'void MID_Beep_Toggle(void);\n'
             '\n'
             '#endif\n')
old_bytes = old_block.encode('gb2312')
assert old_bytes in data, "未找到目标块"

new_block = ('/* 翻转蜂鸣器状态（响<->停）\n'
             ' * @note  内部调用 LQ_GPIO_TogglePin，适合主循环周期性翻转做"滴滴"报警\n'
             ' */\n'
             'void MID_Beep_Toggle(void);\n'
             '\n'
             '/* 阻塞式蜂鸣：响 count 次，每次 on_seconds 秒，间隔 gap_seconds 秒\n'
             ' * @param  count        响声次数，0 直接返回\n'
             ' * @param  on_seconds   每次响声持续时长（秒），<=0 直接返回\n'
             ' * @param  gap_seconds  相邻响声之间的静音间隔（秒），<0 直接返回\n'
             ' *                      =0 表示连响不间断\n'
             ' * @return  none\n'
             ' * @note   阻塞式实现：on -> delay -> off -> [delay gap] -> ...\n'
             ' *         占用主循环，仅适合短促提示音（< 1~2s）\n'
             ' *         长期报警应使用 MID_Beep_On / MID_Beep_Off 自行实现状态机\n'
             ' *         末次响声之后不再追加间隔\n'
             ' *\n'
             ' * @example  MID_Beep_Beep(3, 0.5f,  0.1f);  // 滴0.5s 静0.1s 滴0.5s 静0.1s 滴0.5s\n'
             ' *           MID_Beep_Beep(1, 1.0f,  0.0f);  // 长鸣 1 秒（无间隔参数）\n'
             ' *           MID_Beep_Beep(5, 0.05f, 0.05f); // 紧凑快响\n'
             ' */\n'
             'void MID_Beep_Beep(uint8_t count, float on_seconds, float gap_seconds);\n'
             '\n'
             '#endif\n')
new_bytes = new_block.encode('gb2312')

data = data.replace(old_bytes, new_bytes, 1)
with open(TARGET, "wb") as f:
    f.write(data)
print("mid_beep.h 修改完成")
