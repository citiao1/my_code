from pathlib import Path
import re

from reportlab.lib.pagesizes import A4
from reportlab.pdfbase.cidfonts import UnicodeCIDFont
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfgen import canvas


ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / "打钩题答案.md"
OUTPUT_DIR = ROOT / "output" / "pdf"
OUTPUT = OUTPUT_DIR / "打钩题答案.pdf"


def clean_markdown(line: str) -> str:
    line = line.rstrip()
    if line.startswith("# "):
        return line[2:]
    if line.startswith("## "):
        return line[3:]
    if line.startswith("### "):
        return line[4:]
    return latex_to_text(line.replace("$$", ""))


def replace_frac(text: str) -> str:
    marker = "\\frac"
    while marker in text:
        start = text.find(marker)
        num_start = start + len(marker)
        if num_start >= len(text) or text[num_start] != "{":
            break
        num_end = matching_brace(text, num_start)
        den_start = num_end + 1
        if num_end == -1 or den_start >= len(text) or text[den_start] != "{":
            break
        den_end = matching_brace(text, den_start)
        if den_end == -1:
            break
        numerator = replace_frac(text[num_start + 1 : num_end])
        denominator = replace_frac(text[den_start + 1 : den_end])
        text = text[:start] + f"({numerator})/({denominator})" + text[den_end + 1 :]
    return text


def matching_brace(text: str, open_index: int) -> int:
    depth = 0
    for i in range(open_index, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return i
    return -1


def latex_to_text(text: str) -> str:
    text = replace_frac(text)
    replacements = {
        r"\int": "int",
        r"\infty": "inf",
        r"\lim": "lim",
        r"\to": "->",
        r"\delta": "delta",
        r"\operatorname{Re}": "Re",
        r"\operatorname": "",
        r"\left": "",
        r"\right": "",
        r"\quad": "    ",
        r"\qquad": "        ",
        r"\,": "",
        r"\;": "",
        r"\ ": " ",
    }
    for old, new in replacements.items():
        text = text.replace(old, new)

    text = re.sub(r"\^\{([^{}]+)\}", r"^(\1)", text)
    text = re.sub(r"_\{([^{}]+)\}", r"_(\1)", text)
    text = text.replace("$", "")
    text = text.replace("\\", "")
    text = text.replace("{", "(").replace("}", ")")
    return text


def wrap_text(text: str, max_units: int = 52) -> list[str]:
    if not text:
        return [""]

    lines: list[str] = []
    current = ""
    units = 0.0

    for ch in text:
        width = 1.0 if ord(ch) < 128 else 2.0
        if units + width > max_units and current:
            lines.append(current)
            current = ch
            units = width
        else:
            current += ch
            units += width

    if current:
        lines.append(current)
    return lines


def draw_pdf() -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    pdfmetrics.registerFont(UnicodeCIDFont("STSong-Light"))

    c = canvas.Canvas(str(OUTPUT), pagesize=A4)
    width, height = A4
    margin_x = 52
    y = height - 52

    for raw in SOURCE.read_text(encoding="utf-8").splitlines():
        line = clean_markdown(raw)
        if raw.startswith("# "):
            font_size = 18
            leading = 26
        elif raw.startswith("## "):
            font_size = 14
            leading = 22
        elif raw.startswith("### "):
            font_size = 12
            leading = 20
        else:
            font_size = 10.5
            leading = 17

        c.setFont("STSong-Light", font_size)
        for part in wrap_text(line):
            if y < 46:
                c.showPage()
                y = height - 52
                c.setFont("STSong-Light", font_size)
            c.drawString(margin_x, y, part)
            y -= leading

    c.save()


if __name__ == "__main__":
    draw_pdf()
    print(OUTPUT)
