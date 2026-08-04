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
    HRFlowable,
    KeepTogether,
    ListFlowable,
    ListItem,
    PageBreak,
    Paragraph,
    Preformatted,
    SimpleDocTemplate,
    Spacer,
    Table,
    TableStyle,
)


PROJECT_DIR = Path(__file__).resolve().parents[2]
SOURCE = PROJECT_DIR / "说明书" / "BLE_DUAL_MOTOR_CONTROL_MANUAL.md"
OUTPUT = PROJECT_DIR / "说明书" / "BLE_DUAL_MOTOR_CONTROL_MANUAL.pdf"

FONT_REGULAR = Path(r"C:\Windows\Fonts\msyh.ttc")
FONT_BOLD = Path(r"C:\Windows\Fonts\msyhbd.ttc")

PAGE_WIDTH, PAGE_HEIGHT = A4
LEFT_MARGIN = 19 * mm
RIGHT_MARGIN = 19 * mm
TOP_MARGIN = 20 * mm
BOTTOM_MARGIN = 18 * mm
CONTENT_WIDTH = PAGE_WIDTH - LEFT_MARGIN - RIGHT_MARGIN

INK = colors.HexColor("#17212B")
MUTED = colors.HexColor("#52606D")
BLUE = colors.HexColor("#1F5D78")
GREEN = colors.HexColor("#2A6B57")
LIGHT_BLUE = colors.HexColor("#EAF2F6")
LIGHT_GREEN = colors.HexColor("#EAF3EF")
LIGHT_GRAY = colors.HexColor("#F3F5F7")
GRID = colors.HexColor("#C9D1D8")


def register_fonts() -> None:
    if not FONT_REGULAR.exists() or not FONT_BOLD.exists():
        raise FileNotFoundError("Microsoft YaHei fonts were not found")
    pdfmetrics.registerFont(TTFont("YaHei", str(FONT_REGULAR), subfontIndex=0))
    pdfmetrics.registerFont(TTFont("YaHeiBold", str(FONT_BOLD), subfontIndex=0))


def inline_markup(text: str) -> str:
    placeholders: list[str] = []

    def hold_code(match: re.Match[str]) -> str:
        placeholders.append(
            f'<font name="YaHei" color="#A23B32">{html.escape(match.group(1))}</font>'
        )
        return f"@@CODE{len(placeholders) - 1}@@"

    text = re.sub(r"`([^`]+)`", hold_code, text)
    text = html.escape(text)
    text = re.sub(r"\*\*([^*]+)\*\*", r"<b>\1</b>", text)
    for index, value in enumerate(placeholders):
        text = text.replace(f"@@CODE{index}@@", value)
    return text


def make_styles() -> dict[str, ParagraphStyle]:
    base = getSampleStyleSheet()
    return {
        "title": ParagraphStyle(
            "TitleCN",
            parent=base["Title"],
            fontName="YaHeiBold",
            fontSize=25,
            leading=34,
            textColor=INK,
            alignment=TA_CENTER,
            spaceAfter=15 * mm,
            wordWrap="CJK",
        ),
        "h2": ParagraphStyle(
            "Heading2CN",
            parent=base["Heading2"],
            fontName="YaHeiBold",
            fontSize=15,
            leading=21,
            textColor=BLUE,
            spaceBefore=7 * mm,
            spaceAfter=3 * mm,
            keepWithNext=True,
            wordWrap="CJK",
        ),
        "h3": ParagraphStyle(
            "Heading3CN",
            parent=base["Heading3"],
            fontName="YaHeiBold",
            fontSize=11.5,
            leading=17,
            textColor=GREEN,
            spaceBefore=4 * mm,
            spaceAfter=2 * mm,
            keepWithNext=True,
            wordWrap="CJK",
        ),
        "body": ParagraphStyle(
            "BodyCN",
            parent=base["BodyText"],
            fontName="YaHei",
            fontSize=9.2,
            leading=15,
            textColor=INK,
            alignment=TA_LEFT,
            spaceAfter=2.5 * mm,
            wordWrap="CJK",
        ),
        "body_keep": ParagraphStyle(
            "BodyKeepCN",
            parent=base["BodyText"],
            fontName="YaHei",
            fontSize=9.2,
            leading=15,
            textColor=INK,
            alignment=TA_LEFT,
            spaceAfter=2.5 * mm,
            keepWithNext=True,
            wordWrap="CJK",
        ),
        "meta": ParagraphStyle(
            "MetaCN",
            parent=base["BodyText"],
            fontName="YaHei",
            fontSize=10.5,
            leading=18,
            textColor=MUTED,
            alignment=TA_CENTER,
            spaceAfter=1 * mm,
            wordWrap="CJK",
        ),
        "quote": ParagraphStyle(
            "QuoteCN",
            parent=base["BodyText"],
            fontName="YaHei",
            fontSize=9.2,
            leading=15,
            textColor=INK,
            leftIndent=7 * mm,
            rightIndent=7 * mm,
            borderColor=BLUE,
            borderWidth=0.8,
            borderPadding=8,
            backColor=LIGHT_BLUE,
            spaceBefore=3 * mm,
            spaceAfter=4 * mm,
            wordWrap="CJK",
        ),
        "code": ParagraphStyle(
            "CodeCN",
            parent=base["Code"],
            fontName="YaHei",
            fontSize=7.6,
            leading=11,
            textColor=colors.HexColor("#202B33"),
            leftIndent=4 * mm,
            rightIndent=4 * mm,
            borderColor=GRID,
            borderWidth=0.5,
            borderPadding=7,
            backColor=LIGHT_GRAY,
            spaceBefore=1.5 * mm,
            spaceAfter=3 * mm,
        ),
        "table": ParagraphStyle(
            "TableCN",
            parent=base["BodyText"],
            fontName="YaHei",
            fontSize=7.5,
            leading=10.5,
            textColor=INK,
            wordWrap="CJK",
        ),
        "table_head": ParagraphStyle(
            "TableHeadCN",
            parent=base["BodyText"],
            fontName="YaHeiBold",
            fontSize=7.5,
            leading=10.5,
            textColor=colors.white,
            wordWrap="CJK",
        ),
        "footer": ParagraphStyle(
            "FooterCN",
            parent=base["BodyText"],
            fontName="YaHei",
            fontSize=7.5,
            leading=9,
            textColor=MUTED,
        ),
    }


def column_widths(column_count: int) -> list[float]:
    ratios = {
        2: [0.31, 0.69],
        3: [0.16, 0.28, 0.56],
        4: [0.19, 0.22, 0.23, 0.36],
        5: [0.13, 0.20, 0.20, 0.20, 0.27],
    }
    if column_count == 13:
        return [CONTENT_WIDTH / 13.0] * 13
    selected = ratios.get(column_count, [1.0 / column_count] * column_count)
    return [CONTENT_WIDTH * ratio for ratio in selected]


def make_table(raw_rows: list[list[str]], styles: dict[str, ParagraphStyle]) -> Table:
    cells = []
    for row_index, row in enumerate(raw_rows):
        style = styles["table_head"] if row_index == 0 else styles["table"]
        cells.append([Paragraph(inline_markup(cell.strip()), style) for cell in row])

    table = Table(
        cells,
        colWidths=column_widths(len(raw_rows[0])),
        repeatRows=1,
        hAlign="LEFT",
    )
    table.setStyle(
        TableStyle(
            [
                ("BACKGROUND", (0, 0), (-1, 0), BLUE),
                ("BACKGROUND", (0, 1), (-1, -1), colors.white),
                ("ROWBACKGROUNDS", (0, 1), (-1, -1), [colors.white, LIGHT_GRAY]),
                ("GRID", (0, 0), (-1, -1), 0.35, GRID),
                ("VALIGN", (0, 0), (-1, -1), "TOP"),
                ("LEFTPADDING", (0, 0), (-1, -1), 4),
                ("RIGHTPADDING", (0, 0), (-1, -1), 4),
                ("TOPPADDING", (0, 0), (-1, -1), 4),
                ("BOTTOMPADDING", (0, 0), (-1, -1), 4),
            ]
        )
    )
    return table


def parse_markdown(source: str, styles: dict[str, ParagraphStyle]):
    lines = source.splitlines()
    flowables = []
    index = 0
    title_seen = False

    while index < len(lines):
        line = lines[index].rstrip()
        stripped = line.strip()

        if not stripped:
            index += 1
            continue

        if stripped.startswith("# "):
            flowables.extend(
                [Spacer(1, 25 * mm), Paragraph(inline_markup(stripped[2:]), styles["title"])]
            )
            title_seen = True
            index += 1
            continue

        if stripped.startswith("## "):
            if title_seen and stripped.startswith("## 1."):
                flowables.extend(
                    [
                        Spacer(1, 13 * mm),
                        HRFlowable(
                            width=CONTENT_WIDTH * 0.42,
                            thickness=1.2,
                            color=BLUE,
                            spaceBefore=0,
                            spaceAfter=0,
                        ),
                        PageBreak(),
                    ]
                )
                title_seen = False
            flowables.append(Paragraph(inline_markup(stripped[3:]), styles["h2"]))
            index += 1
            continue

        if stripped.startswith("### "):
            flowables.append(Paragraph(inline_markup(stripped[4:]), styles["h3"]))
            index += 1
            continue

        if stripped.startswith("> "):
            flowables.append(Paragraph(inline_markup(stripped[2:]), styles["quote"]))
            index += 1
            continue

        if stripped.startswith("```"):
            language = stripped[3:].strip()
            code_lines: list[str] = []
            index += 1
            while index < len(lines) and not lines[index].strip().startswith("```"):
                code_lines.append(lines[index].rstrip())
                index += 1
            index += 1
            label = f"[{language.upper()}]\n" if language else ""
            flowables.append(Preformatted(label + "\n".join(code_lines), styles["code"], maxLineLength=100))
            continue

        if stripped.startswith("|") and index + 1 < len(lines):
            separator = lines[index + 1].strip()
            if separator.startswith("|") and re.fullmatch(r"[|:\- ]+", separator):
                rows: list[list[str]] = []
                rows.append([cell.strip() for cell in stripped.strip("|").split("|")])
                index += 2
                while index < len(lines) and lines[index].strip().startswith("|"):
                    row = [cell.strip() for cell in lines[index].strip().strip("|").split("|")]
                    if len(row) == len(rows[0]):
                        rows.append(row)
                    index += 1
                flowables.extend([make_table(rows, styles), Spacer(1, 3 * mm)])
                continue

        list_match = re.match(r"^(\d+)\.\s+(.*)$", stripped)
        bullet_match = re.match(r"^-\s+(.*)$", stripped)
        if list_match or bullet_match:
            ordered = list_match is not None
            items = []
            while index < len(lines):
                current = lines[index].strip()
                match = re.match(r"^(\d+)\.\s+(.*)$", current) if ordered else re.match(r"^-\s+(.*)$", current)
                if match is None:
                    break
                body = match.group(2) if ordered else match.group(1)
                items.append(ListItem(Paragraph(inline_markup(body), styles["body"]), leftIndent=5 * mm))
                index += 1
            list_flowable = ListFlowable(
                    items,
                    bulletType="1" if ordered else "bullet",
                    start="1" if ordered else "•",
                    leftIndent=8 * mm,
                    bulletFontName="YaHei",
                    bulletFontSize=8,
                    spaceAfter=2 * mm,
                )
            flowables.append(KeepTogether([list_flowable]))
            continue

        paragraph_lines = [stripped]
        index += 1
        while index < len(lines):
            candidate = lines[index].strip()
            if not candidate:
                break
            if (
                candidate.startswith(("#", ">", "```", "|", "- "))
                or re.match(r"^\d+\.\s+", candidate)
            ):
                break
            paragraph_lines.append(candidate)
            index += 1

        paragraph_text = " ".join(paragraph_lines)
        if title_seen:
            style = styles["meta"]
        elif paragraph_text.endswith(("：", ":")):
            style = styles["body_keep"]
        else:
            style = styles["body"]
        flowables.append(Paragraph(inline_markup(paragraph_text), style))

    return flowables


def page_decoration(canvas, document) -> None:
    canvas.saveState()
    page_number = canvas.getPageNumber()
    if page_number > 1:
        canvas.setStrokeColor(GRID)
        canvas.setLineWidth(0.45)
        canvas.line(LEFT_MARGIN, PAGE_HEIGHT - 14 * mm, PAGE_WIDTH - RIGHT_MARGIN, PAGE_HEIGHT - 14 * mm)
        canvas.setFont("YaHei", 7.5)
        canvas.setFillColor(MUTED)
        canvas.drawString(LEFT_MARGIN, PAGE_HEIGHT - 11 * mm, "STM32F407 双步进电机蓝牙控制说明书")

    canvas.setStrokeColor(GRID)
    canvas.setLineWidth(0.45)
    canvas.line(LEFT_MARGIN, 12 * mm, PAGE_WIDTH - RIGHT_MARGIN, 12 * mm)
    canvas.setFont("YaHei", 7.5)
    canvas.setFillColor(MUTED)
    canvas.drawString(LEFT_MARGIN, 8 * mm, "F407_Emm_SingleTurnHold")
    canvas.drawRightString(PAGE_WIDTH - RIGHT_MARGIN, 8 * mm, f"第 {page_number} 页")
    canvas.restoreState()


def main() -> None:
    register_fonts()
    styles = make_styles()
    source_text = SOURCE.read_text(encoding="utf-8")
    story = parse_markdown(source_text, styles)

    document = SimpleDocTemplate(
        str(OUTPUT),
        pagesize=A4,
        leftMargin=LEFT_MARGIN,
        rightMargin=RIGHT_MARGIN,
        topMargin=TOP_MARGIN,
        bottomMargin=BOTTOM_MARGIN,
        title="STM32F407 双步进电机蓝牙控制说明书",
        author="Codex",
        subject="F407_Emm_SingleTurnHold 控制协议、接线和程序说明",
    )
    document.build(story, onFirstPage=page_decoration, onLaterPages=page_decoration)
    print(OUTPUT)


if __name__ == "__main__":
    main()
