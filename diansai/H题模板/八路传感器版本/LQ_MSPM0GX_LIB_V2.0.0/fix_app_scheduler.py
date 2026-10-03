# -*- coding: utf-8 -*-
# 修正：emergency 检测取消注释（位置环保持注释）

with open(r'd:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\Code\App\app_scheduler.c', 'r', encoding='gbk') as f:
    content = f.read()

# 把 emergency 检测取消注释
old = '''//    MID_Chassis_PosStep(e_line, dt_s);
//
//    /* 1.5) 5 路全 0/全 1 紧急检测（即使位置环未启用也要检测）
//     *     若 5 路同电平 -> MID_Chassis_Stop() + 置 g_line_emergency_stopped=1
//     *     K1 启动时由 mid_key.c 清标志解除
//     */
//    MID_Line_CheckEmergency(ir_levels);'''

new = '''//    MID_Chassis_PosStep(e_line, dt_s);

    /* 1.5) 5 路全 0/全 1 紧急检测（位置环未启用也必须检测）
     *     若 5 路同电平 -> MID_Chassis_Stop() + 置 g_line_emergency_stopped=1
     *     K1 启动时由 mid_key.c 清标志解除
     */
    MID_Line_CheckEmergency(ir_levels);'''

assert old in content, "未找到 emergency 注释块"
content = content.replace(old, new)

with open(r'd:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\Code\App\app_scheduler.c', 'w', encoding='gbk') as f:
    f.write(content)
print("OK: app_scheduler.c emergency uncommented")
