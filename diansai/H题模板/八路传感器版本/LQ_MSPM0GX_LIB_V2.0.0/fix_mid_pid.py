#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""在 mid_pid.c 中标记 line_threshold / line_hysteresis 为不再使用"""
import os

TARGET = r"d:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\Code\Middle\mid_pid.c"

with open(TARGET, "rb") as f:
    data = f.read()

# 直接以字节匹配需要修改的块
old_bytes = (b'    /* ---------- \xd1\xad\xcf\xdf\xb2\xce\xca\xfd ---------- */\n'
             b'    g_param.line_threshold = 80.0f;         /* ADC \xe3\xd0\xd6\xb5\xb7\xb6\xce\xa7 0~100 */\n'
             b'    g_param.line_deadzone  = 0.1f;            /* \xcb\xc0\xc7\xf8\xa3\xba\xc6\xab\xb2\xee\xd0\xa1\xd3\xda 0.05 \xca\xb1\xb2\xbb\xcf\xec\xd3\xa6 */\n'
             b'    g_param.line_hysteresis = 10.0f;           /* \xe3\xd0\xd6\xb5\xd6\xcd\xbb\xd8*/\n')

new_block_text = """    /* ---------- 循迹参数 ---------- */
    /* line_threshold / line_hysteresis 已不再使用
     * 原 8 路模拟灰度 ADC 巡线 -> 已替换为 5 路数字量红外（IR）巡线
     * 5 路 IR 直接读 GPIO，使用电平有效 + 施密特滞回
     * 见 mid_line.c 的 bits[] 0/1 状态 + MID_Line_CalcError()
     * 仍使用 line_deadzone 做小偏度归零
     * 以下两参数在 VOFA+ 面板已移除作为可调字段，保留赋值以兼容旧代码
     */
    g_param.line_threshold  = 80.0f;          /* 不再使用 - 5 路 IR 不读 ADC */
    g_param.line_deadzone   = 0.1f;           /* 偏度小于此值时强制归零 */
    g_param.line_hysteresis = 10.0f;          /* 不再使用 - 5 路 IR 不读 ADC */
"""
new_bytes = new_block_text.encode('gb2312')

assert old_bytes in data, "old_bytes not found"
data = data.replace(old_bytes, new_bytes, 1)
with open(TARGET, "wb") as f:
    f.write(data)
print("mid_pid.c 修改完成")
