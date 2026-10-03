# -*- coding: utf-8 -*-
"""
修复 mid_pid.c 中 bits[7] 数组越界警告（5 路 IR 替代 8 路灰度）。
- bits[0] = OUT1（最左）
- bits[4] = OUT5（最右，原 bits[7] 含义）
"""

with open(r'D:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\Code\Middle\mid_pid.c', 'rb') as f:
    raw = f.read()

# 改 bits[7] -> bits[4]（5 路 IR 中最右是 OUT5=bits[4]）
# 原 8 路灰度：bits[0]=最左, bits[7]=最右
# 现 5 路 IR：bits[0]=最左, bits[4]=最右
old = b'    if(bits[0]!=1||bits[7]!=1)'
new = b'    if(bits[0]!=1||bits[4]!=1)'  # 5 路 IR：bits[4] = OUT5（最右）

if old not in raw:
    raise RuntimeError("old string not found in mid_pid.c")

count = raw.count(old)
print(f"Found {count} occurrence(s) of old string")

new_raw = raw.replace(old, new)

with open(r'D:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\Code\Middle\mid_pid.c', 'wb') as f:
    f.write(new_raw)

print(f"Written {len(new_raw)} bytes (was {len(raw)})")
