# -*- coding: utf-8 -*-
# app_scheduler.c: 移除 LQ_tracking.h 依赖，调度 5 路红外 emergency 检测（GBK）

with open(r'd:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\Code\App\app_scheduler.c', 'r', encoding='gbk') as f:
    content = f.read()

# 1) 删 LQ_tracking.h include
old_include = '#include "LQ_tracking.h"'
new_include = '/* LQ_tracking.h 已移除（5 路红外替代原 8 路模拟灰度） */'
assert old_include in content, "未找到 LQ_tracking.h include"
content = content.replace(old_include, new_include)

# 2) 在文件头注释中加一句"5 路红外"说明
old_header_note = "          1) 8 路灰度采集 + 偏度计算 + 位置环 PD"
new_header_note = "          1) 5 路红外采集 + 偏度计算 + 位置环 PD（当前未启用）"
content = content.replace(old_header_note, new_header_note)

# 3) 位置环注释里"8 路灰"->"5 路红外"
old_pos_line1 = "//    LQ_Tracking_Polling_GetValue();"
new_pos_line1 = "//    MID_IR_ReadRaw(ir_levels);          // 5 路原始 H/L 电平"
content = content.replace(old_pos_line1, new_pos_line1)

old_pos_line2 = "//    float e_line = MID_Line_CalcError();"
new_pos_line2 = "//    MID_IR_UpdateBits(ir_levels);       // 5 路 -> bits[5]"
content = content.replace(old_pos_line2, new_pos_line2)

# 在 MID_Chassis_PosStep 后插入 emergency 检测（位置环下面）
old_pos_step = "//    MID_Chassis_PosStep(e_line, dt_s);"
new_pos_step = '''//    MID_Chassis_PosStep(e_line, dt_s);
//
//    /* 1.5) 5 路全 0/全 1 紧急检测（即使位置环未启用也要检测）
//     *     若 5 路同电平 -> MID_Chassis_Stop() + 置 g_line_emergency_stopped=1
//     *     K1 启动时由 mid_key.c 清标志解除
//     */
//    MID_Line_CheckEmergency(ir_levels);'''
assert old_pos_step in content, "未找到位置环 MID_Chassis_PosStep 注释行"
content = content.replace(old_pos_step, new_pos_step)

# 4) 在 s_scheduler_step 开头添加 5 路红外读取
old_step_signature = "static void s_scheduler_step(float dt_s)\n{"
new_step_signature = '''static void s_scheduler_step(float dt_s)
{
    /* 0) 5 路红外原始电平读取（每 5ms 一次）
     *    5 路 GPIO 总耗时 < 1us，不影响 5ms 调度实时性
     */
    uint8_t ir_levels[MID_IR_SENSOR_COUNT];
    MID_IR_ReadRaw(ir_levels);'''
assert old_step_signature in content, "未找到 s_scheduler_step 函数签名"
content = content.replace(old_step_signature, new_step_signature)

# 5) 文件头注释里"8 路循迹采集" -> "5 路红外采集"
old_header_ch1 = "                1) 5 路红外采集 + 偏度计算 + 位置环 PD（当前未启用）"
new_header_ch1 = "                1) 5 路红外原始电平采集 + 全 0/全 1 紧急检测（每 5ms）"
content = content.replace(old_header_ch1, new_header_ch1)

with open(r'd:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\Code\App\app_scheduler.c', 'w', encoding='gbk') as f:
    f.write(content)
print("OK: app_scheduler.c modified (GBK)")
