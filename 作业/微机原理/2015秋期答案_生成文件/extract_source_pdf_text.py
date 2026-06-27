from pathlib import Path

import pdfplumber


ROOT = Path(__file__).resolve().parent.parent
PDF = ROOT / "必修-CSE22500E《微机原理及应用》电科2015秋期.pdf"
OUT = Path(__file__).resolve().parent / "source_text.txt"

with pdfplumber.open(str(PDF)) as doc, OUT.open("w", encoding="utf-8") as f:
    for i, page in enumerate(doc.pages, 1):
        text = page.extract_text(x_tolerance=1, y_tolerance=3) or ""
        f.write(f"\n-- page {i} --\n{text}\n")
        print(f"page {i}: {len(text)} chars")

print(OUT)
