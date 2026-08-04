from pathlib import Path
import sys

import pypdfium2 as pdfium


pdf_path = Path(sys.argv[1])
output_dir = Path(sys.argv[2])
output_dir.mkdir(parents=True, exist_ok=True)

document = pdfium.PdfDocument(pdf_path)
for index, page in enumerate(document):
    bitmap = page.render(scale=2.0)
    image = bitmap.to_pil()
    output = output_dir / f"page-{index + 1}.png"
    image.save(output)
    print(output)
print(f"pages={len(document)}")
