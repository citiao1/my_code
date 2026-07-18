from __future__ import annotations

import json
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw
from pypdf import PdfReader


ROOT = Path(__file__).resolve().parents[2]
PDF = ROOT / "output" / "pdf" / "MSPM0G3507_智能车完整说明书.pdf"
ARTIFACTS = ROOT / "documentation" / "manual_artifacts"
PAGES = ARTIFACTS / "rendered_pages"
CONTACTS = ARTIFACTS / "contact_sheets"

SAFETY_LINES = [
    "1.严禁故意短路任何电路，调试过程中请随时关注电池电量，不要过放！不要过放！",
    "2.所有电路连接工作必须断电操作，严禁带电插拔线路",
    "3.对电源线路的更改，必须最先把电池的中转线移除再进行操作",
    "4.湿手不碰车！",
    "5.更改线路连接后，必须将电池拔出，使用万用表蜂鸣档测量短路，严禁带电使用蜂鸣档",
    "6.使用万用表测量电压时，请随时注意不要短路，尽量由硬件队员进行此操作",
    "后续内容待更新。",
]

REQUIRED_TEXT = [
    "在另一台 Windows 电脑部署上位机",
    "网页上位机使用",
    "板载按键、拨码和蜂鸣器",
    "串口、DMA 与蓝牙桥",
    "从零开始调参",
    "VOFA+ 完整通道表",
    "调试故障复盘与避免方法",
    "I65",
    "SPEED_FEEDFORWARD_ENABLED=0",
]


def normalized(text: str) -> str:
    return "".join(text.split())


def main() -> None:
    reader = PdfReader(str(PDF))
    extracted_pages = [(page.extract_text() or "") for page in reader.pages]
    extracted = "\n\n".join(
        f"===== PAGE {index + 1} =====\n{text}"
        for index, text in enumerate(extracted_pages)
    )
    (ARTIFACTS / "extracted_text.txt").write_text(extracted, encoding="utf-8")

    first = normalized(extracted_pages[0])
    missing_safety = [line for line in SAFETY_LINES if normalized(line) not in first]
    all_text = normalized("\n".join(extracted_pages))
    missing_required = [text for text in REQUIRED_TEXT if normalized(text) not in all_text]

    images = sorted(PAGES.glob("page-*.png"))
    if len(images) != len(reader.pages):
        raise RuntimeError(f"渲染页数 {len(images)} 与 PDF 页数 {len(reader.pages)} 不一致")

    CONTACTS.mkdir(parents=True, exist_ok=True)
    metrics = []
    loaded = []
    for image_path in images:
        image = Image.open(image_path).convert("RGB")
        background = Image.new("RGB", image.size, "white")
        difference = ImageChops.difference(image, background).convert("L")
        mask = difference.point(lambda value: 255 if value > 12 else 0)
        bbox = mask.getbbox()
        nonwhite = mask.histogram()[255] / (image.width * image.height)
        metrics.append(
            {
                "page": len(metrics) + 1,
                "width": image.width,
                "height": image.height,
                "ink_bbox": bbox,
                "nonwhite_ratio": round(nonwhite, 5),
            }
        )
        loaded.append(image)

    for start in range(0, len(loaded), 2):
        pair = loaded[start : start + 2]
        thumb_width = 650
        thumbs = []
        for index, page in enumerate(pair, start=start + 1):
            ratio = thumb_width / page.width
            thumb = page.resize((thumb_width, round(page.height * ratio)), Image.Resampling.LANCZOS)
            canvas = Image.new("RGB", (thumb.width, thumb.height + 34), "#D8DEE0")
            canvas.paste(thumb, (0, 34))
            ImageDraw.Draw(canvas).text((10, 9), f"PDF page {index}", fill="black")
            thumbs.append(canvas)
        sheet = Image.new(
            "RGB",
            (sum(image.width for image in thumbs) + 12 * (len(thumbs) - 1), max(image.height for image in thumbs)),
            "#717A7E",
        )
        x = 0
        for thumb in thumbs:
            sheet.paste(thumb, (x, 0))
            x += thumb.width + 12
        sheet.save(CONTACTS / f"pages_{start + 1:02d}_{start + len(pair):02d}.png")

    summary = {
        "pdf": str(PDF),
        "pages": len(reader.pages),
        "characters_extracted": len("".join(extracted_pages)),
        "missing_safety_lines": missing_safety,
        "missing_required_text": missing_required,
        "page_metrics": metrics,
    }
    (ARTIFACTS / "qa_summary.json").write_text(
        json.dumps(summary, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    print(json.dumps(summary, ensure_ascii=False, indent=2))
    if missing_safety or missing_required:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
