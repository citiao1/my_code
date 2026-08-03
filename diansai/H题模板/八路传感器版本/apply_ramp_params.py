# -*- coding: utf-8 -*-
"""
将 MID_KEY_K1_LAUNCH_OFFSET / MID_KEY_K1_DECEL_END_OFFSET /
    MID_KEY_K2_LAUNCH_OFFSET / MID_KEY_RAMP_STEP 四个宏
迁移为 MID_Param_t 字段（launch_offset / decel_end_offset / ramp_step），
由 K1/K2 默认参数集各自赋值。
统一用 gb18030 读写，保持原文件行尾风格。
"""
import os
import re

BASE = r'd:\桌面\八路传感器版本\LQ_MSPM0GX_LIB_V2.0.0\Code\Middle'
ENC = 'gb18030'

# 保存每个文件的原始行尾风格
_file_newline = {}

def read_f(name):
    with open(os.path.join(BASE, name), 'rb') as f:
        raw = f.read()
    nl = '\r\n' if b'\r\n' in raw else '\n'
    _file_newline[name] = nl
    return raw.decode(ENC).replace('\r\n', '\n')

def write_f(name, content):
    nl = _file_newline.get(name, '\n')
    if nl != '\n':
        content = content.replace('\n', nl)
    with open(os.path.join(BASE, name), 'w', encoding=ENC, newline='') as f:
        f.write(content)

# ============================================================
# 1. mid_pid.h：在 MID_Param_t 中新增 3 个字段
# ============================================================
print('=== mid_pid.h ===')
c = read_f('mid_pid.h')

# 在 speed_max 行之后、gyro_offset 之前插入新字段
# 用正则匹配，不依赖具体中文注释内容
pattern = r'(    float chassis_vx;[^\n]*\n    float speed_max;[^\n]*\n)'
m = re.search(pattern, c)
assert m is not None, 'mid_pid.h: 找不到 chassis_vx/speed_max 字段'
old_block = m.group(1)

new_block = (old_block +
    '\n'
    '    /* 速度渐变参数（K1/K2 默认集中填不同值，发车时随 LoadXxxDefaults 加载）*/\n'
    '    float launch_offset;      /* 发车偏移：chassis_vx = 巡航速度 + launch_offset */\n'
    '    float decel_end_offset;   /* 减速终点偏移：decel_end = 巡航速度 + decel_end_offset（仅 K1 用，K2 填 0）*/\n'
    '    float ramp_step;          /* 渐变步进量（带符号：负=减速 K1，正=加速 K2，代码统一用 +=）*/\n')
c = c[:m.start()] + new_block + c[m.end():]
assert 'float launch_offset;' in c, 'mid_pid.h: launch_offset 字段未添加'
assert 'float decel_end_offset;' in c, 'mid_pid.h: decel_end_offset 字段未添加'
assert 'float ramp_step;' in c, 'mid_pid.h: ramp_step 字段未添加'
print('  3 个新字段已添加到 MID_Param_t')

write_f('mid_pid.h', c)
print('  mid_pid.h 写入完成')

# ============================================================
# 2. mid_pid.c：在两个默认集中新增字段值
# ============================================================
print('=== mid_pid.c ===')
c = read_f('mid_pid.c')

# K1 默认集：在 .speed_max 行之后插入
old_k1 = ('    .speed_max   = 550.0f,\n'
          '    .gyro_offset = 0.0f,\n'
          '    .enable_pos  = 1, .enable_yaw = 1, .enable_speed = 1,\n'
          '};\n'
          '\n'
          '/* K2')
assert old_k1 in c, 'mid_pid.c: 找不到 K1 默认集 speed_max 行'
new_k1 = ('    .speed_max        = 550.0f,\n'
          '    .gyro_offset      = 0.0f,\n'
          '    .launch_offset    = 100.0f,    /* K1 发车偏移 = +100 */\n'
          '    .decel_end_offset = -200.0f,   /* K1 减速终点偏移 = -200 */\n'
          '    .ramp_step        = -1.0f,     /* K1 渐变步进 = -1（减速）*/\n'
          '    .enable_pos  = 1, .enable_yaw = 1, .enable_speed = 1,\n'
          '};\n'
          '\n'
          '/* K2')
c = c.replace(old_k1, new_k1, 1)
assert '.launch_offset    = 100.0f' in c, 'mid_pid.c: K1 launch_offset 未添加'
print('  K1 默认集：3 字段值已添加')

# K2 默认集：在 .speed_max 行之后插入
old_k2 = ('    .speed_max   = 550.0f,\n'
          '    .gyro_offset = 0.0f,\n'
          '    .enable_pos  = 1, .enable_yaw = 1, .enable_speed = 1,\n'
          '};\n'
          '\n'
          '/* 加载 K1')
assert old_k2 in c, 'mid_pid.c: 找不到 K2 默认集 speed_max 行'
new_k2 = ('    .speed_max        = 550.0f,\n'
          '    .gyro_offset      = 0.0f,\n'
          '    .launch_offset    = -100.0f,   /* K2 发车偏移 = -100 */\n'
          '    .decel_end_offset = 0.0f,      /* K2 不使用减速终点，填 0 */\n'
          '    .ramp_step        = 1.0f,      /* K2 渐变步进 = +1（加速）*/\n'
          '    .enable_pos  = 1, .enable_yaw = 1, .enable_speed = 1,\n'
          '};\n'
          '\n'
          '/* 加载 K1')
c = c.replace(old_k2, new_k2, 1)
assert '.launch_offset    = -100.0f' in c, 'mid_pid.c: K2 launch_offset 未添加'
print('  K2 默认集：3 字段值已添加')

write_f('mid_pid.c', c)
print('  mid_pid.c 写入完成')

# ============================================================
# 3. mid_key.h：删除 4 个宏定义（含注释）
# ============================================================
print('=== mid_key.h ===')
c = read_f('mid_key.h')

# 删除 4 个宏定义块（每个宏含上方注释行 + 宏行 + 空行）
# 注释内容不含宏名，用通用模式：/* 任意 */\n#define MACRO ...\n\n
patterns_to_remove = [
    r'/\*[^\n]*\*/\n#define MID_KEY_K1_LAUNCH_OFFSET[^\n]*\n\n',
    r'/\*[^\n]*\*/\n#define MID_KEY_K1_DECEL_END_OFFSET[^\n]*\n\n',
    r'/\*[^\n]*\*/\n#define MID_KEY_K2_LAUNCH_OFFSET[^\n]*\n\n',
    r'/\*[^\n]*\*/\n#define MID_KEY_RAMP_STEP[^\n]*\n\n',
]
for p in patterns_to_remove:
    m = re.search(p, c)
    assert m is not None, f'mid_key.h: 未匹配到宏块 [{p[:40]}]'
    c = c[:m.start()] + c[m.end():]
    print(f'  已删除宏块')

# 验证 4 个宏已全部删除
for macro in ['MID_KEY_K1_LAUNCH_OFFSET', 'MID_KEY_K1_DECEL_END_OFFSET',
              'MID_KEY_K2_LAUNCH_OFFSET', 'MID_KEY_RAMP_STEP']:
    assert macro not in c, f'mid_key.h: 仍有 {macro} 残留'
print('  4 个宏定义已全部删除')

write_f('mid_key.h', c)
print('  mid_key.h 写入完成')

# ============================================================
# 4. mid_key.c：改代码引用 + 注释
# ============================================================
print('=== mid_key.c ===')
c = read_f('mid_key.c')

# K1 发车：MID_KEY_K1_LAUNCH_OFFSET -> g_param.launch_offset
old_k1_code = 'g_param.chassis_vx = s_cruise_vx + MID_KEY_K1_LAUNCH_OFFSET;'
assert old_k1_code in c, 'mid_key.c: 找不到 K1 LAUNCH_OFFSET 引用'
c = c.replace(old_k1_code, 'g_param.chassis_vx = s_cruise_vx + g_param.launch_offset;', 1)
print('  K1 发车：launch_offset 引用已修改')

# K1 注释
old_k1_note = 'K1 快发模式：发车速度 = 巡航速度 + MID_KEY_K1_LAUNCH_OFFSET'
# 注释是 gb18030 中文，用更宽松的匹配：替换含 MID_KEY_K1_LAUNCH_OFFSET 的整行注释
c = re.sub(r'(/\*[^\n]*?)MID_KEY_K1_LAUNCH_OFFSET([^\n]*\*/)',
           r'\1g_param.launch_offset\2', c)
print('  K1 注释已更新')

# K2 发车：MID_KEY_K2_LAUNCH_OFFSET -> g_param.launch_offset
old_k2_code = 'g_param.chassis_vx = s_cruise_vx + MID_KEY_K2_LAUNCH_OFFSET;'
assert old_k2_code in c, 'mid_key.c: 找不到 K2 LAUNCH_OFFSET 引用'
c = c.replace(old_k2_code, 'g_param.chassis_vx = s_cruise_vx + g_param.launch_offset;', 1)
print('  K2 发车：launch_offset 引用已修改')

# K2 注释
c = re.sub(r'(/\*[^\n]*?)MID_KEY_K2_LAUNCH_OFFSET([^\n]*\*/)',
           r'\1g_param.launch_offset\2', c)
print('  K2 注释已更新')

# 验证
for macro in ['MID_KEY_K1_LAUNCH_OFFSET', 'MID_KEY_K2_LAUNCH_OFFSET']:
    assert macro not in c, f'mid_key.c: 仍有 {macro} 残留'

write_f('mid_key.c', c)
print('  mid_key.c 写入完成')

# ============================================================
# 5. mid_chassis.c：改代码引用 + 注释
# ============================================================
print('=== mid_chassis.c ===')
c = read_f('mid_chassis.c')

# decel_end
old_decel = 'float decel_end = cruise + MID_KEY_K1_DECEL_END_OFFSET;'
assert old_decel in c, 'mid_chassis.c: 找不到 DECEL_END_OFFSET 引用'
c = c.replace(old_decel, 'float decel_end = cruise + g_param.decel_end_offset;', 1)
print('  decel_end 引用已修改')

# K1 ramp: -= MID_KEY_RAMP_STEP -> += g_param.ramp_step
old_k1_ramp = 'g_param.chassis_vx -= MID_KEY_RAMP_STEP;'
assert old_k1_ramp in c, 'mid_chassis.c: 找不到 K1 RAMP_STEP 引用'
c = c.replace(old_k1_ramp, 'g_param.chassis_vx += g_param.ramp_step;', 1)
print('  K1 ramp 引用已修改（-= -> +=）')

# K2 ramp: += MID_KEY_RAMP_STEP -> += g_param.ramp_step
old_k2_ramp = 'g_param.chassis_vx += MID_KEY_RAMP_STEP;'
assert old_k2_ramp in c, 'mid_chassis.c: 找不到 K2 RAMP_STEP 引用'
c = c.replace(old_k2_ramp, 'g_param.chassis_vx += g_param.ramp_step;', 1)
print('  K2 ramp 引用已修改')

# 注释中的宏名引用
c = re.sub(r'(/\*[^\n]*?)MID_KEY_K1_DECEL_END_OFFSET([^\n]*\*/)',
           r'\1g_param.decel_end_offset\2', c)
c = re.sub(r'(/\*[^\n]*?)MID_KEY_RAMP_STEP([^\n]*\*/)',
           r'\1g_param.ramp_step\2', c)
print('  注释已更新')

# 验证
for macro in ['MID_KEY_K1_DECEL_END_OFFSET', 'MID_KEY_RAMP_STEP']:
    assert macro not in c, f'mid_chassis.c: 仍有 {macro} 残留'

write_f('mid_chassis.c', c)
print('  mid_chassis.c 写入完成')

# ============================================================
# 6. mid_chassis.h：更新注释（如有宏名引用）
# ============================================================
print('=== mid_chassis.h ===')
c = read_f('mid_chassis.h')

# 检查是否有宏名引用
changed = False
for macro in ['MID_KEY_K1_LAUNCH_OFFSET', 'MID_KEY_K1_DECEL_END_OFFSET',
              'MID_KEY_K2_LAUNCH_OFFSET', 'MID_KEY_RAMP_STEP']:
    if macro in c:
        c = re.sub(r'(/\*[^\n]*?)' + macro + r'([^\n]*\*/)',
                   r'\1g_param 对应字段\2', c)
        changed = True
        print(f'  注释中 {macro} 已替换')

if not changed:
    print('  无宏名引用，跳过')

write_f('mid_chassis.h', c)
print('  mid_chassis.h 写入完成')

print('\n=== 全部完成 ===')
