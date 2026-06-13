from pathlib import Path
import re

from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import mm
from reportlab.pdfbase.cidfonts import UnicodeCIDFont
from reportlab.pdfbase import pdfmetrics
from reportlab.platypus import (
    SimpleDocTemplate,
    Paragraph,
    Spacer,
    Preformatted,
    Table,
    TableStyle,
)
from xml.sax.saxutils import escape

BASE_DIR = Path(__file__).resolve().parent
SOURCE = BASE_DIR / "Unity日系校园模拟游戏入门教程.md"
OUTPUT = BASE_DIR / "Unity日系校园模拟游戏入门教程.pdf"

pdfmetrics.registerFont(UnicodeCIDFont("STSong-Light"))

styles = getSampleStyleSheet()
styles.add(ParagraphStyle(
    name="CnTitle",
    fontName="STSong-Light",
    fontSize=22,
    leading=30,
    alignment=TA_CENTER,
    spaceAfter=10 * mm,
    textColor=colors.HexColor("#24324A"),
))
styles.add(ParagraphStyle(
    name="CnH1",
    fontName="STSong-Light",
    fontSize=17,
    leading=24,
    spaceBefore=7 * mm,
    spaceAfter=3 * mm,
    textColor=colors.HexColor("#324A67"),
))
styles.add(ParagraphStyle(
    name="CnH2",
    fontName="STSong-Light",
    fontSize=14,
    leading=20,
    spaceBefore=5 * mm,
    spaceAfter=2 * mm,
    textColor=colors.HexColor("#405A73"),
))
styles.add(ParagraphStyle(
    name="CnBody",
    fontName="STSong-Light",
    fontSize=10.5,
    leading=16,
    spaceAfter=2.2 * mm,
    firstLineIndent=0,
))
styles.add(ParagraphStyle(
    name="CnBullet",
    fontName="STSong-Light",
    fontSize=10.5,
    leading=16,
    leftIndent=6 * mm,
    firstLineIndent=-4 * mm,
    spaceAfter=1.4 * mm,
))
styles.add(ParagraphStyle(
    name="CnSmall",
    fontName="STSong-Light",
    fontSize=8.5,
    leading=12,
    spaceAfter=1.5 * mm,
))
styles.add(ParagraphStyle(
    name="CnTable",
    fontName="STSong-Light",
    fontSize=9.5,
    leading=13,
))


def inline_markup(text: str) -> str:
    text = escape(text.strip())
    text = re.sub(r"`([^`]+)`", r"<font color='#7A3E21'>\1</font>", text)
    text = text.replace("**", "")
    return text


def make_table(lines):
    rows = []
    for line in lines:
        cells = [inline_markup(cell) for cell in line.strip().strip("|").split("|")]
        rows.append([Paragraph(cell, styles["CnTable"]) for cell in cells])
    if not rows:
        return []

    page_width = A4[0] - 36 * mm
    col_count = max(len(row) for row in rows)
    col_width = page_width / col_count
    table = Table(rows, colWidths=[col_width] * col_count, hAlign="LEFT")
    table.setStyle(TableStyle([
        ("FONTNAME", (0, 0), (-1, -1), "STSong-Light"),
        ("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#EAF0F5")),
        ("TEXTCOLOR", (0, 0), (-1, 0), colors.HexColor("#24324A")),
        ("GRID", (0, 0), (-1, -1), 0.35, colors.HexColor("#C9D3DD")),
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LEFTPADDING", (0, 0), (-1, -1), 5),
        ("RIGHTPADDING", (0, 0), (-1, -1), 5),
        ("TOPPADDING", (0, 0), (-1, -1), 4),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 4),
    ]))
    return [table, Spacer(1, 3 * mm)]


def build_story(markdown: str):
    story = []
    lines = markdown.splitlines()
    index = 0
    in_code = False
    code_lines = []

    title_consumed = False
    while index < len(lines):
        raw = lines[index]
        line = raw.rstrip()

        if line.startswith("```"):
            if in_code:
                code = "\n".join(code_lines).rstrip()
                if code:
                    story.append(Preformatted(code, styles["CnSmall"], maxLineLength=86))
                    story.append(Spacer(1, 3 * mm))
                code_lines = []
                in_code = False
            else:
                in_code = True
            index += 1
            continue

        if in_code:
            code_lines.append(raw)
            index += 1
            continue

        if not line.strip() or line.strip() == "---":
            index += 1
            continue

        if line.startswith("| "):
            table_lines = []
            while index < len(lines) and lines[index].startswith("| "):
                if not re.match(r"^\|\s*-+", lines[index]):
                    table_lines.append(lines[index])
                index += 1
            story.extend(make_table(table_lines))
            continue

        if line.startswith("# "):
            text = inline_markup(line[2:])
            if not title_consumed:
                story.append(Paragraph(text, styles["CnTitle"]))
                title_consumed = True
            else:
                story.append(Paragraph(text, styles["CnH1"]))
            index += 1
            continue

        if line.startswith("## "):
            story.append(Paragraph(inline_markup(line[3:]), styles["CnH1"]))
            index += 1
            continue

        if line.startswith("### "):
            story.append(Paragraph(inline_markup(line[4:]), styles["CnH2"]))
            index += 1
            continue

        if line.startswith("- "):
            story.append(Paragraph("• " + inline_markup(line[2:]), styles["CnBullet"]))
            index += 1
            continue

        numbered = re.match(r"^(\d+)\.\s+(.*)$", line)
        if numbered:
            story.append(Paragraph(f"{numbered.group(1)}. {inline_markup(numbered.group(2))}", styles["CnBullet"]))
            index += 1
            continue

        story.append(Paragraph(inline_markup(line), styles["CnBody"]))
        index += 1

    return story


def add_page_number(canvas, doc):
    canvas.saveState()
    canvas.setFont("STSong-Light", 9)
    canvas.setFillColor(colors.HexColor("#7B8794"))
    canvas.drawCentredString(A4[0] / 2, 11 * mm, f"第 {doc.page} 页")
    canvas.restoreState()


def main():
    markdown = SOURCE.read_text(encoding="utf-8")
    doc = SimpleDocTemplate(
        str(OUTPUT),
        pagesize=A4,
        rightMargin=18 * mm,
        leftMargin=18 * mm,
        topMargin=18 * mm,
        bottomMargin=18 * mm,
        title="Unity日系校园模拟游戏入门教程",
        author="Codex",
    )
    story = build_story(markdown)
    doc.build(story, onFirstPage=add_page_number, onLaterPages=add_page_number)
    print(OUTPUT)


if __name__ == "__main__":
    main()
