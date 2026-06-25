from pathlib import Path

import pdfplumber


ROOT = Path(__file__).resolve().parent.parent
PDFS = [
    "北京化工大学微机原理2017-2019_重排题目版.pdf",
    "北京化工大学微机原理2017-2019_答案解析.pdf",
    "北京化工大学微机原理2017-2019_知识点总结.pdf",
]
NOISE = ["¥", "÷", "cid:", "GOH", "XCHGAL", "MOVA l"]

for name in PDFS:
    path = ROOT / name
    with pdfplumber.open(str(path)) as pdf:
        text = "\n".join(page.extract_text() or "" for page in pdf.pages)
        print(f"{name}: pages={len(pdf.pages)}, chars={len(text)}, noise={any(s in text for s in NOISE)}")
