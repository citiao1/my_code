from pathlib import Path

import pdfplumber


ROOT = Path(__file__).resolve().parent.parent
OUT = Path(__file__).resolve().parent / "extracted_text_utf8.txt"


with OUT.open("w", encoding="utf-8") as f:
    for path in sorted(ROOT.glob("北京化工大学《微机原理及接口技术》201*-201*.pdf")):
        f.write(f"\n==== {path.name} ====\n")
        with pdfplumber.open(str(path)) as pdf:
            for i, page in enumerate(pdf.pages, 1):
                text = page.extract_text(x_tolerance=1, y_tolerance=3) or ""
                f.write(f"\n-- page {i} --\n{text}\n")

print(OUT)
