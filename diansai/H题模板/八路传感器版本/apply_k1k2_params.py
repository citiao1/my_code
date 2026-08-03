# -*- coding: utf-8 -*-
"""
K1/K2 独立参数集改造脚本
统一用 gb18030 读写（gb2312 超集，对 gb2312 字符产生相同字节）
"""
import re
import os

BASE = r'd:\桌面\八路传感器版本\LQ_MSPM0GX_LIB_V2.0.0\Code\Middle'
ENC = 'gb18030'

# 保存每个文件的原始行尾风格，写回时恢复
_file_newline = {}

def read_f(name):
    """读取文件，统一将行尾归一化为 \\n 便于模式匹配，同时记录原始行尾风格"""
    with open(os.path.join(BASE, name), 'rb') as f:
        raw = f.read()
    # 检测原始行尾：优先 CRLF，否则 LF
    if b'\r\n' in raw:
        _file_newline[name] = '\r\n'
    else:
        _file_newline[name] = '\n'
    # 解码并归一化行尾为 \n
    return raw.decode(ENC).replace('\r\n', '\n')

def write_f(name, content):
    """写入文件，恢复原始行尾风格"""
    nl = _file_newline.get(name, '\n')
    if nl != '\n':
        content = content.replace('\n', nl)
    with open(os.path.join(BASE, name), 'w', encoding=ENC, newline='') as f:
        f.write(content)

def check(content, *keys):
    """验证关键字符串是否存在"""
    for k in keys:
        if k not in content:
            print(f'  WARN: 缺少 [{k}]')
    for bad in ['__chassis_vx', '__kp']:
        if bad in content:
            # 仅警告，由调用方决定是否允许
            pass

# ============================================================
# 1. mid_pid.h
# ============================================================
print('=== mid_pid.h ===')
c = read_f('mid_pid.h')

# 1a. 删除 extern 声明
c_before = c
c = re.sub(r'extern float __chassis_vx;[^\n]*\n', '', c)
c = re.sub(r'extern float __kp;\n', '', c)
assert c != c_before, 'mid_pid.h: extern 未删除'
assert '__chassis_vx' not in c and '__kp' not in c, 'mid_pid.h: 仍有残留'
print('  extern 声明已删除')

# 1b. 新增 Load 函数声明
old_decl = 'void MID_Param_Init(void);'
assert old_decl in c, 'mid_pid.h: 找不到 MID_Param_Init 声明'
new_decl = (old_decl + '\n\n'
    '/* 加载 K1 模式默认参数集到 g_param（发车时调用，覆盖当前活动参数） */\n'
    'void MID_Param_LoadK1Defaults(void);\n\n'
    '/* 加载 K2 模式默认参数集到 g_param（发车时调用，覆盖当前活动参数） */\n'
    'void MID_Param_LoadK2Defaults(void);')
c = c.replace(old_decl, new_decl, 1)
assert 'MID_Param_LoadK1Defaults' in c, 'mid_pid.h: LoadK1 声明未添加'
print('  Load 函数声明已添加')

write_f('mid_pid.h', c)
print('  mid_pid.h 写入完成')

# ============================================================
# 2. mid_pid.c
# ============================================================
print('=== mid_pid.c ===')
c = read_f('mid_pid.c')

# 2a. 添加 <string.h>
assert '#include "mid_line.h"' in c, 'mid_pid.c: 找不到 mid_line.h include'
c = c.replace('#include "mid_line.h"', '#include "mid_line.h"\n#include <string.h>  /* memcpy */', 1)
assert '#include <string.h>' in c, 'mid_pid.c: string.h 未添加'
print('  #include <string.h> 已添加')

# 2b. 删除全局定义
old_globals = 'float __chassis_vx = 120.0f;\nfloat __kp = 39.0f;\n'
assert old_globals in c, 'mid_pid.c: 找不到全局定义'
c = c.replace(old_globals, '', 1)
print('  全局定义 __chassis_vx/__kp 已删除')

# 2c. 替换 MID_Param_Init 函数为：默认集 + Load 函数 + 新 MID_Param_Init
new_block = (
'/* K1 模式默认参数集（快发+里程减速模式专用）\n'
' * 与 K2 默认集的差异：pid_pos.kp=52（K1 实测值）\n'
' * enables=1（发射就绪态，MID_Chassis_Start 也会置 1） */\n'
'static const MID_Param_t s_param_k1_default = {\n'
'    .pid_pos   = { .kp = 52.0f,  .kd = 0.0f, .out_max = 300.0f },\n'
'    .pid_yaw   = { .kp = 17.5f, .ki = 15.0f, .kd = 0.0f, .imax = 80.0f, .out_max = 300.0f },\n'
'    .pid_speed_l = { .kp = 1.0f, .ki = 80.0f, .kd = 0.0f, .out_max = 9000.0f, .inc_max = 1000.0f },\n'
'    .pid_speed_r = { .kp = 1.0f, .ki = 80.0f, .kd = 0.0f, .out_max = 9000.0f, .inc_max = 1000.0f },\n'
'    .chassis_vx  = 120.0f,\n'
'    .speed_max   = 550.0f,\n'
'    .gyro_offset = 0.0f,\n'
'    .enable_pos  = 1, .enable_yaw = 1, .enable_speed = 1,\n'
'};\n'
'\n'
'/* K2 模式默认参数集（缓发+渐进加速模式专用）\n'
' * 字段值与原 MID_Param_Init 一致，enables=1（发射就绪） */\n'
'static const MID_Param_t s_param_k2_default = {\n'
'    .pid_pos   = { .kp = 39.0f,  .kd = 0.0f, .out_max = 300.0f },\n'
'    .pid_yaw   = { .kp = 17.5f, .ki = 15.0f, .kd = 0.0f, .imax = 80.0f, .out_max = 300.0f },\n'
'    .pid_speed_l = { .kp = 1.0f, .ki = 80.0f, .kd = 0.0f, .out_max = 9000.0f, .inc_max = 1000.0f },\n'
'    .pid_speed_r = { .kp = 1.0f, .ki = 80.0f, .kd = 0.0f, .out_max = 9000.0f, .inc_max = 1000.0f },\n'
'    .chassis_vx  = 120.0f,\n'
'    .speed_max   = 550.0f,\n'
'    .gyro_offset = 0.0f,\n'
'    .enable_pos  = 1, .enable_yaw = 1, .enable_speed = 1,\n'
'};\n'
'\n'
'/* 加载 K1 默认参数集到 g_param（memcpy 覆盖当前活动参数） */\n'
'void MID_Param_LoadK1Defaults(void)\n'
'{\n'
'    memcpy(&g_param, &s_param_k1_default, sizeof(MID_Param_t));\n'
'}\n'
'\n'
'/* 加载 K2 默认参数集到 g_param（memcpy 覆盖当前活动参数） */\n'
'void MID_Param_LoadK2Defaults(void)\n'
'{\n'
'    memcpy(&g_param, &s_param_k2_default, sizeof(MID_Param_t));\n'
'}\n'
'\n'
'/* 初始化所有参数为默认值（上电调用）\n'
' * @note  复用 K2 默认集（字段值与原实现一致），再强制 enables=0 安全态 */\n'
'void MID_Param_Init(void)\n'
'{\n'
'    MID_Param_LoadK2Defaults();\n'
'    g_param.enable_pos   = 0;\n'
'    g_param.enable_yaw   = 0;\n'
'    g_param.enable_speed = 0;\n'
'}')

pattern = r'void MID_Param_Init\(void\)\s*\{[\s\S]*?\n\}'
m = re.search(pattern, c)
assert m is not None, 'mid_pid.c: 未匹配到 MID_Param_Init 函数'
old_func = m.group(0)
print(f'  旧 MID_Param_Init 匹配长度: {len(old_func)} 字符')
c = c[:m.start()] + new_block + c[m.end():]
assert 's_param_k1_default' in c, 'mid_pid.c: K1 默认集未添加'
assert 's_param_k2_default' in c, 'mid_pid.c: K2 默认集未添加'
assert 'MID_Param_LoadK1Defaults' in c, 'mid_pid.c: LoadK1 未添加'
assert 'MID_Param_LoadK2Defaults' in c, 'mid_pid.c: LoadK2 未添加'
assert '__kp' not in c and '__chassis_vx' not in c, 'mid_pid.c: 仍有 __kp/__chassis_vx 残留'
print('  MID_Param_Init 已重构，默认集 + Load 函数已添加')

write_f('mid_pid.c', c)
print('  mid_pid.c 写入完成')

# ============================================================
# 3. mid_key.h
# ============================================================
print('=== mid_key.h ===')
c = read_f('mid_key.h')

# 3a. 新增 MID_Key_GetCruiseVx 声明（在 MID_Key_GetLaunchMode 声明之后）
old_getmode = ('MID_LaunchMode_t MID_Key_GetLaunchMode(void);')
assert old_getmode in c, 'mid_key.h: 找不到 GetLaunchMode 声明'
new_getmode = (old_getmode + '\n\n'
    '/* 查询当前模式的巡航速度（ramp 收敛目标）\n'
' * @return K1/K2 发车时保存的模式巡航速度；IDLE 时返回 0\n'
' * @note  供 MID_Chassis_RampStep 作为减速终点/加速目标的基准\n'
' */\n'
'float MID_Key_GetCruiseVx(void);')
c = c.replace(old_getmode, new_getmode, 1)
assert 'MID_Key_GetCruiseVx' in c, 'mid_key.h: GetCruiseVx 声明未添加'
print('  MID_Key_GetCruiseVx 声明已添加')

# 3b. 注释中 __chassis_vx -> 模式巡航速度
cnt = c.count('__chassis_vx')
c = c.replace('__chassis_vx', '模式巡航速度')
print(f'  注释中 __chassis_vx 替换 {cnt} 处')
assert '__chassis_vx' not in c, 'mid_key.h: 仍有 __chassis_vx 残留'

write_f('mid_key.h', c)
print('  mid_key.h 写入完成')

# ============================================================
# 4. mid_key.c
# ============================================================
print('=== mid_key.c ===')
c = read_f('mid_key.c')

# 4a. 新增 s_cruise_vx 静态变量（在 s_launch_mode 声明之后）
old_lm = 'static MID_LaunchMode_t s_launch_mode = MID_MODE_IDLE;'
assert old_lm in c, 'mid_key.c: 找不到 s_launch_mode 声明'
new_lm = (old_lm + '\n\n'
'/* 当前模式的巡航速度（ramp 收敛目标）\n'
' * K1/K2 发车时由默认集的 chassis_vx 保存，供 MID_Chassis_RampStep 读取 */\n'
'static float s_cruise_vx = 0.0f;')
c = c.replace(old_lm, new_lm, 1)
assert 's_cruise_vx' in c, 'mid_key.c: s_cruise_vx 未添加'
print('  s_cruise_vx 静态变量已添加')

# 4b. 新增 MID_Key_GetCruiseVx 实现（在 MID_Key_GetLaunchMode 实现之后）
old_impl = ('MID_LaunchMode_t MID_Key_GetLaunchMode(void)\n'
'{\n'
'    return s_launch_mode;\n'
'}')
assert old_impl in c, 'mid_key.c: 找不到 GetLaunchMode 实现'
new_impl = (old_impl + '\n\n'
'/* 查询当前模式的巡航速度（ramp 收敛目标）\n'
' * @return K1/K2 发车时保存的模式巡航速度；IDLE 时返回 0\n'
' * @note  供 MID_Chassis_RampStep 作为减速终点/加速目标的基准\n'
' */\n'
'float MID_Key_GetCruiseVx(void)\n'
'{\n'
'    return s_cruise_vx;\n'
'}')
c = c.replace(old_impl, new_impl, 1)
assert 'float MID_Key_GetCruiseVx(void)' in c, 'mid_key.c: GetCruiseVx 实现未添加'
print('  MID_Key_GetCruiseVx 实现已添加')

# 4c. 修改 s_handle_start (K1)：替换 kp=52 硬编码
old_k1_kp = '\t\tg_param.pid_pos.kp=52;\n'
assert old_k1_kp in c, 'mid_key.c: 找不到 K1 kp=52 行'
new_k1_kp = ('    /* 加载 K1 完整默认参数集（覆盖当前 g_param，含 PID/速度/使能） */\n'
'    MID_Param_LoadK1Defaults();\n'
'    /* 保存 K1 巡航速度（ramp 收敛目标），再加发车偏移 */\n'
'    s_cruise_vx = g_param.chassis_vx;\n')
c = c.replace(old_k1_kp, new_k1_kp, 1)
print('  K1 kp=52 已替换为 LoadK1Defaults + cruise 保存')

# 4d. K1 chassis_vx 赋值改用 s_cruise_vx
old_k1_vx = 'g_param.chassis_vx = __chassis_vx + MID_KEY_K1_LAUNCH_OFFSET;'
assert old_k1_vx in c, 'mid_key.c: 找不到 K1 chassis_vx 行'
c = c.replace(old_k1_vx, 'g_param.chassis_vx = s_cruise_vx + MID_KEY_K1_LAUNCH_OFFSET;', 1)
print('  K1 chassis_vx 改用 s_cruise_vx')

# 4e. 修改 s_handle_k2_start (K2)：替换 kp=__kp
old_k2_kp = '\t\tg_param.pid_pos.kp=__kp;\n'
assert old_k2_kp in c, 'mid_key.c: 找不到 K2 kp=__kp 行'
new_k2_kp = ('    /* 加载 K2 完整默认参数集（覆盖当前 g_param，含 PID/速度/使能） */\n'
'    MID_Param_LoadK2Defaults();\n'
'    /* 保存 K2 巡航速度（ramp 收敛目标），再加发车偏移 */\n'
'    s_cruise_vx = g_param.chassis_vx;\n')
c = c.replace(old_k2_kp, new_k2_kp, 1)
print('  K2 kp=__kp 已替换为 LoadK2Defaults + cruise 保存')

# 4f. K2 chassis_vx 赋值改用 s_cruise_vx
old_k2_vx = 'g_param.chassis_vx = __chassis_vx + MID_KEY_K2_LAUNCH_OFFSET;'
assert old_k2_vx in c, 'mid_key.c: 找不到 K2 chassis_vx 行'
c = c.replace(old_k2_vx, 'g_param.chassis_vx = s_cruise_vx + MID_KEY_K2_LAUNCH_OFFSET;', 1)
print('  K2 chassis_vx 改用 s_cruise_vx')

# 4g. s_handle_stop 中添加 s_cruise_vx 归零
old_stop = 's_launch_mode = MID_MODE_IDLE;'
assert old_stop in c, 'mid_key.c: 找不到 stop 中 IDLE 赋值'
new_stop = old_stop + '\n    s_cruise_vx = 0.0f;            /* 清零巡航速度 */'
c = c.replace(old_stop, new_stop, 1)
print('  s_handle_stop 中 s_cruise_vx 归零已添加')

# 4h. 注释中剩余 __chassis_vx -> 巡航速度
cnt = c.count('__chassis_vx')
c = c.replace('__chassis_vx', '巡航速度')
print(f'  注释中 __chassis_vx 替换 {cnt} 处')
assert '__chassis_vx' not in c, 'mid_key.c: 仍有 __chassis_vx 残留'
assert '__kp' not in c, 'mid_key.c: 仍有 __kp 残留'

write_f('mid_key.c', c)
print('  mid_key.c 写入完成')

# ============================================================
# 5. mid_chassis.c
# ============================================================
print('=== mid_chassis.c ===')
c = read_f('mid_chassis.c')

# 5a. 添加 cruise 局部变量
old_mode = 'MID_LaunchMode_t mode = MID_Key_GetLaunchMode();\n'
assert old_mode in c, 'mid_chassis.c: 找不到 mode 声明行'
new_mode = old_mode + '    float cruise = MID_Key_GetCruiseVx();   /* 当前模式巡航速度（ramp 目标） */\n'
c = c.replace(old_mode, new_mode, 1)
assert 'float cruise = MID_Key_GetCruiseVx()' in c, 'mid_chassis.c: cruise 变量未添加'
print('  cruise 局部变量已添加')

# 5b. 代码中 __chassis_vx -> cruise
reps = [
    ('float decel_end = __chassis_vx + MID_KEY_K1_DECEL_END_OFFSET;',
     'float decel_end = cruise + MID_KEY_K1_DECEL_END_OFFSET;'),
    ('g_param.chassis_vx < __chassis_vx', 'g_param.chassis_vx < cruise'),
    ('g_param.chassis_vx > __chassis_vx', 'g_param.chassis_vx > cruise'),
    ('g_param.chassis_vx = __chassis_vx;', 'g_param.chassis_vx = cruise;'),
]
for old_s, new_s in reps:
    assert old_s in c, f'mid_chassis.c: 找不到 [{old_s}]'
    c = c.replace(old_s, new_s, 1)
    print(f'  代码替换: {old_s[:40]}... -> cruise')

# 5c. 注释中剩余 __chassis_vx -> 巡航速度
cnt = c.count('__chassis_vx')
c = c.replace('__chassis_vx', '巡航速度')
print(f'  注释中 __chassis_vx 替换 {cnt} 处')
assert '__chassis_vx' not in c, 'mid_chassis.c: 仍有 __chassis_vx 残留'

write_f('mid_chassis.c', c)
print('  mid_chassis.c 写入完成')

# ============================================================
# 6. mid_chassis.h
# ============================================================
print('=== mid_chassis.h ===')
c = read_f('mid_chassis.h')

# 6a. 注释中 __chassis_vx -> 巡航速度
cnt = c.count('__chassis_vx')
c = c.replace('__chassis_vx', '巡航速度')
print(f'  注释中 __chassis_vx 替换 {cnt} 处')
assert '__chassis_vx' not in c, 'mid_chassis.h: 仍有 __chassis_vx 残留'

write_f('mid_chassis.h', c)
print('  mid_chassis.h 写入完成')

print('\n=== 全部完成 ===')
