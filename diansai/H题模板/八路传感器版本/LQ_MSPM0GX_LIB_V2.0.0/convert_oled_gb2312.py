"""
把 mid_oled.h/c 从 UTF-8 转 GB2312（与其他模块风格一致）
"""
import codecs

for fname in ['mid_oled.h', 'mid_oled.c']:
    path = r'd:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\Code\Middle\\' + fname
    with open(path, 'rb') as f:
        raw = f.read()
    # 先按 utf-8 解码（Write 工具默认 utf-8）
    text = raw.decode('utf-8')
    # 再以 gb2312 编码写回
    with codecs.open(path, 'w', encoding='gb2312') as f:
        f.write(text)
    # 验证
    data = open(path, 'rb').read()
    try:
        data.decode('gb2312')
        print(f'{fname}: OK, size={len(data)}')
    except Exception as e:
        print(f'{fname}: FAIL, {e}')
