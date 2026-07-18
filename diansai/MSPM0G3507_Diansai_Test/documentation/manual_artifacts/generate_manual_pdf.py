from __future__ import annotations

import html
import re
from pathlib import Path

from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER, TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (
    BaseDocTemplate,
    Frame,
    HRFlowable,
    NextPageTemplate,
    PageBreak,
    PageTemplate,
    Paragraph,
    Spacer,
    Table,
    TableStyle,
    XPreformatted,
)
from reportlab.platypus.tableofcontents import TableOfContents


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "documentation" / "MSPM0G3507_智能车完整说明书.md"
OUTPUT = ROOT / "output" / "pdf" / "MSPM0G3507_智能车完整说明书.pdf"
FONT_REGULAR = Path(r"C:\Windows\Fonts\simhei.ttf")

SAFETY_TEXT = """1.严禁故意短路任何电路，调试过程中请随时关注电池电量，不要过放！不要过放！
2.所有电路连接工作必须断电操作，严禁带电插拔线路
3.对电源线路的更改，必须最先把电池的中转线移除再进行操作
4.湿手不碰车！
5.更改线路连接后，必须将电池拔出，使用万用表蜂鸣档测量短路，严禁带电使用蜂鸣档
6.使用万用表测量电压时，请随时注意不要短路，尽量由硬件队员进行此操作
后续内容待更新。"""


class ManualDocTemplate(BaseDocTemplate):
    def afterFlowable(self, flowable):
        if not isinstance(flowable, Paragraph):
            return
        levels = {"ManualH1": 0, "ManualH2": 1, "ManualH3": 2}
        level = levels.get(flowable.style.name)
        if level is not None:
            self.notify("TOCEntry", (level, flowable.getPlainText(), self.page))


def register_fonts() -> None:
    if not FONT_REGULAR.exists():
        raise FileNotFoundError("未找到 Windows 中文字体 simhei.ttf")
    pdfmetrics.registerFont(TTFont("Chinese", str(FONT_REGULAR)))
    pdfmetrics.registerFontFamily(
        "Chinese", normal="Chinese", bold="Chinese", italic="Chinese", boldItalic="Chinese"
    )


def make_styles():
    sample = getSampleStyleSheet()
    styles = {
        "body": ParagraphStyle(
            "ManualBody",
            parent=sample["BodyText"],
            fontName="Chinese",
            fontSize=9.5,
            leading=15,
            textColor=colors.HexColor("#20252B"),
            spaceAfter=5,
            wordWrap="CJK",
        ),
        "h1": ParagraphStyle(
            "ManualH1",
            parent=sample["Heading1"],
            fontName="Chinese",
            fontSize=20,
            leading=27,
            textColor=colors.HexColor("#12202B"),
            spaceBefore=7,
            spaceAfter=12,
            keepWithNext=True,
            wordWrap="CJK",
        ),
        "h2": ParagraphStyle(
            "ManualH2",
            parent=sample["Heading2"],
            fontName="Chinese",
            fontSize=14,
            leading=21,
            textColor=colors.HexColor("#0F5962"),
            spaceBefore=12,
            spaceAfter=7,
            keepWithNext=True,
            wordWrap="CJK",
        ),
        "h3": ParagraphStyle(
            "ManualH3",
            parent=sample["Heading3"],
            fontName="Chinese",
            fontSize=11.5,
            leading=18,
            textColor=colors.HexColor("#253A45"),
            spaceBefore=9,
            spaceAfter=5,
            keepWithNext=True,
            wordWrap="CJK",
        ),
        "list": ParagraphStyle(
            "ManualList",
            parent=sample["BodyText"],
            fontName="Chinese",
            fontSize=9.3,
            leading=14.5,
            leftIndent=13,
            firstLineIndent=-11,
            spaceAfter=3,
            wordWrap="CJK",
        ),
        "code": ParagraphStyle(
            "ManualCode",
            parent=sample["Code"],
            fontName="Chinese",
            fontSize=8.2,
            leading=12,
            textColor=colors.HexColor("#1C2830"),
            leftIndent=0,
            rightIndent=0,
            wordWrap="CJK",
        ),
        "table": ParagraphStyle(
            "ManualTable",
            parent=sample["BodyText"],
            fontName="Chinese",
            fontSize=7.8,
            leading=11.2,
            textColor=colors.HexColor("#20252B"),
            wordWrap="CJK",
        ),
        "table_header": ParagraphStyle(
            "ManualTableHeader",
            parent=sample["BodyText"],
            fontName="Chinese",
            fontSize=8,
            leading=11.5,
            textColor=colors.white,
            wordWrap="CJK",
        ),
        "safety_title": ParagraphStyle(
            "SafetyTitle",
            parent=sample["Title"],
            fontName="Chinese",
            fontSize=24,
            leading=32,
            alignment=TA_CENTER,
            textColor=colors.HexColor("#A71919"),
            spaceAfter=18,
        ),
        "safety": ParagraphStyle(
            "SafetyLine",
            parent=sample["BodyText"],
            fontName="Chinese",
            fontSize=13.5,
            leading=22,
            textColor=colors.HexColor("#711414"),
            spaceAfter=7,
            wordWrap="CJK",
        ),
        "toc_title": ParagraphStyle(
            "TocTitle",
            parent=sample["Heading1"],
            fontName="Chinese",
            fontSize=17,
            leading=23,
            textColor=colors.HexColor("#12202B"),
            spaceBefore=12,
            spaceAfter=10,
        ),
    }
    return styles


def inline_markup(text: str) -> str:
    parts = text.split("`")
    rendered = []
    for index, part in enumerate(parts):
        escaped = html.escape(part, quote=False)
        if index % 2:
            rendered.append(
                f'<font name="Chinese" color="#8B3A20">{escaped}</font>'
            )
        else:
            escaped = re.sub(r"\*\*(.+?)\*\*", r"<b>\1</b>", escaped)
            rendered.append(escaped)
    return "".join(rendered).replace("  ", "<br/>")


def table_widths(column_count: int, available: float):
    if column_count == 2:
        ratios = [0.31, 0.69]
    elif column_count == 3:
        ratios = [0.28, 0.42, 0.30]
    elif column_count == 4:
        ratios = [0.10, 0.31, 0.16, 0.43]
    else:
        ratios = [1.0 / column_count] * column_count
    return [available * ratio for ratio in ratios]


def make_table(rows, styles, available):
    parsed = []
    for row_index, row in enumerate(rows):
        style = styles["table_header"] if row_index == 0 else styles["table"]
        parsed.append([Paragraph(inline_markup(cell), style) for cell in row])
    table = Table(
        parsed,
        colWidths=table_widths(len(rows[0]), available),
        repeatRows=1,
        hAlign="LEFT",
    )
    table.setStyle(
        TableStyle(
            [
                ("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#17636B")),
                ("VALIGN", (0, 0), (-1, -1), "TOP"),
                ("GRID", (0, 0), (-1, -1), 0.35, colors.HexColor("#AAB5B8")),
                ("ROWBACKGROUNDS", (0, 1), (-1, -1), [colors.white, colors.HexColor("#F2F6F6")]),
                ("LEFTPADDING", (0, 0), (-1, -1), 4),
                ("RIGHTPADDING", (0, 0), (-1, -1), 4),
                ("TOPPADDING", (0, 0), (-1, -1), 4),
                ("BOTTOMPADDING", (0, 0), (-1, -1), 4),
            ]
        )
    )
    return table


def parse_markdown(text: str, styles, available: float):
    lines = text.splitlines()
    story = []
    paragraph = []
    index = 0

    def flush_paragraph():
        if paragraph:
            joined = " ".join(part.strip() for part in paragraph)
            story.append(Paragraph(inline_markup(joined), styles["body"]))
            paragraph.clear()

    while index < len(lines):
        line = lines[index]
        stripped = line.strip()

        if stripped.startswith("```"):
            flush_paragraph()
            index += 1
            code_lines = []
            while index < len(lines) and not lines[index].strip().startswith("```"):
                code_lines.append(lines[index])
                index += 1
            block = XPreformatted(html.escape("\n".join(code_lines)), styles["code"])
            box = Table([[block]], colWidths=[available], hAlign="LEFT")
            box.setStyle(
                TableStyle(
                    [
                        ("BACKGROUND", (0, 0), (-1, -1), colors.HexColor("#F1F3F4")),
                        ("BOX", (0, 0), (-1, -1), 0.5, colors.HexColor("#BCC5C8")),
                        ("LEFTPADDING", (0, 0), (-1, -1), 8),
                        ("RIGHTPADDING", (0, 0), (-1, -1), 8),
                        ("TOPPADDING", (0, 0), (-1, -1), 6),
                        ("BOTTOMPADDING", (0, 0), (-1, -1), 6),
                    ]
                )
            )
            story.extend([box, Spacer(1, 6)])
        elif stripped.startswith("|") and stripped.endswith("|"):
            flush_paragraph()
            raw_rows = []
            while index < len(lines):
                candidate = lines[index].strip()
                if not (candidate.startswith("|") and candidate.endswith("|")):
                    break
                cells = [cell.strip() for cell in candidate.strip("|").split("|")]
                if not all(re.fullmatch(r":?-{3,}:?", cell) for cell in cells):
                    raw_rows.append(cells)
                index += 1
            if raw_rows:
                story.extend([make_table(raw_rows, styles, available), Spacer(1, 7)])
            continue
        elif stripped.startswith("### "):
            flush_paragraph()
            story.append(Paragraph(inline_markup(stripped[4:]), styles["h3"]))
        elif stripped.startswith("## "):
            flush_paragraph()
            story.append(Paragraph(inline_markup(stripped[3:]), styles["h2"]))
        elif stripped.startswith("# "):
            flush_paragraph()
            story.append(Paragraph(inline_markup(stripped[2:]), styles["h1"]))
        elif stripped == "---":
            flush_paragraph()
            story.extend(
                [HRFlowable(width="100%", thickness=0.7, color=colors.HexColor("#90A0A4")), Spacer(1, 5)]
            )
        elif re.match(r"^-\s+", stripped):
            flush_paragraph()
            story.append(Paragraph("- " + inline_markup(stripped[2:]), styles["list"]))
        elif re.match(r"^\d+\.\s+", stripped):
            flush_paragraph()
            match = re.match(r"^(\d+\.)\s+(.*)$", stripped)
            story.append(
                Paragraph(inline_markup(match.group(1) + " " + match.group(2)), styles["list"])
            )
        elif not stripped:
            flush_paragraph()
        else:
            paragraph.append(line)
        index += 1

    flush_paragraph()
    return story


def draw_normal_page(canvas, doc):
    canvas.saveState()
    width, height = A4
    canvas.setFont("Chinese", 7.5)
    canvas.setFillColor(colors.HexColor("#617078"))
    canvas.drawString(18 * mm, height - 11 * mm, "MSPM0G3507 智能车完整说明书 · V24")
    canvas.drawRightString(width - 18 * mm, 10 * mm, f"第 {doc.page} 页")
    canvas.setStrokeColor(colors.HexColor("#CBD2D4"))
    canvas.setLineWidth(0.4)
    canvas.line(18 * mm, height - 13 * mm, width - 18 * mm, height - 13 * mm)
    canvas.restoreState()


def build_pdf() -> None:
    register_fonts()
    styles = make_styles()
    source = SOURCE.read_text(encoding="utf-8")
    required_prefix = "# 硬件调试最高优先级安全事项\n\n" + SAFETY_TEXT + "\n\n---\n"
    if not source.startswith(required_prefix):
        raise ValueError("Markdown 开头的硬件安全原文缺失或被修改")

    body = source[len(required_prefix):].lstrip()
    split_at = body.find("\n## 1. ")
    if split_at < 0:
        raise ValueError("无法定位说明书正文第一章")
    intro = body[:split_at]
    chapters = body[split_at + 1:]

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    page_width, page_height = A4
    left = right = 18 * mm
    top = 18 * mm
    bottom = 17 * mm
    frame = Frame(
        left,
        bottom,
        page_width - left - right,
        page_height - top - bottom,
        id="content",
        leftPadding=0,
        rightPadding=0,
        topPadding=0,
        bottomPadding=0,
    )
    doc = ManualDocTemplate(
        str(OUTPUT),
        pagesize=A4,
        leftMargin=left,
        rightMargin=right,
        topMargin=top,
        bottomMargin=bottom,
        title="MSPM0G3507 智能车完整说明书",
        author="MSPM0G3507_Diansai_Test",
        subject="车辆使用、软件结构、控制算法与调参说明",
    )
    doc.addPageTemplates(
        [
            PageTemplate(id="safety", frames=[frame]),
            PageTemplate(id="normal", frames=[frame], onPage=draw_normal_page),
        ]
    )
    available = page_width - left - right

    safety_lines = [Paragraph(inline_markup(line), styles["safety"]) for line in SAFETY_TEXT.splitlines()]
    safety_box = Table([[safety_lines]], colWidths=[available - 8 * mm])
    safety_box.setStyle(
        TableStyle(
            [
                ("BACKGROUND", (0, 0), (-1, -1), colors.HexColor("#FFF0EF")),
                ("BOX", (0, 0), (-1, -1), 2.0, colors.HexColor("#B52222")),
                ("LEFTPADDING", (0, 0), (-1, -1), 13),
                ("RIGHTPADDING", (0, 0), (-1, -1), 13),
                ("TOPPADDING", (0, 0), (-1, -1), 13),
                ("BOTTOMPADDING", (0, 0), (-1, -1), 10),
            ]
        )
    )

    story = [
        Spacer(1, 12 * mm),
        Paragraph("硬件调试最高优先级安全事项", styles["safety_title"]),
        safety_box,
        Spacer(1, 7 * mm),
        Paragraph("执行任何接线、测量、烧录或落地测试前，必须先阅读并执行本页。", styles["safety"]),
        NextPageTemplate("normal"),
        PageBreak(),
    ]
    story.extend(parse_markdown(intro, styles, available))
    story.append(Paragraph("目录", styles["toc_title"]))
    toc = TableOfContents()
    toc.levelStyles = [
        ParagraphStyle(
            "TOC1", fontName="Chinese", fontSize=10, leading=16,
            leftIndent=0, firstLineIndent=0, textColor=colors.HexColor("#123B43")
        ),
        ParagraphStyle(
            "TOC2", fontName="Chinese", fontSize=9, leading=14,
            leftIndent=12, firstLineIndent=0, textColor=colors.HexColor("#26383E")
        ),
        ParagraphStyle(
            "TOC3", fontName="Chinese", fontSize=8, leading=12,
            leftIndent=24, firstLineIndent=0, textColor=colors.HexColor("#4D5A5E")
        ),
    ]
    story.extend([toc, PageBreak()])
    story.extend(parse_markdown(chapters, styles, available))
    doc.multiBuild(story)


if __name__ == "__main__":
    build_pdf()
    print(OUTPUT)
