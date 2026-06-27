from pathlib import Path

from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER, TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.cidfonts import UnicodeCIDFont
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (
    BaseDocTemplate,
    Frame,
    KeepTogether,
    ListFlowable,
    ListItem,
    PageTemplate,
    Paragraph,
    Preformatted,
    Spacer,
    Table,
    TableStyle,
)


ROOT = Path(__file__).resolve().parent
OUT = ROOT / "8259A初始化程序编写方法.pdf"


def register_fonts():
    font_path = Path(r"C:\Windows\Fonts\msyh.ttc")
    bold_path = Path(r"C:\Windows\Fonts\msyhbd.ttc")
    mono_path = Path(r"C:\Windows\Fonts\simhei.ttf")
    try:
        pdfmetrics.registerFont(TTFont("MSYH", str(font_path)))
        pdfmetrics.registerFont(TTFont("MSYH-Bold", str(bold_path)))
        pdfmetrics.registerFont(TTFont("SIMHEI", str(mono_path)))
        return "MSYH", "MSYH-Bold", "SIMHEI"
    except Exception:
        pdfmetrics.registerFont(UnicodeCIDFont("STSong-Light"))
        return "STSong-Light", "STSong-Light", "STSong-Light"


FONT, BOLD, MONO = register_fonts()


def para(text, style):
    return Paragraph(text, style)


def code(text, style):
    return Preformatted(text.strip("\n"), style)


def bullet(items, style):
    return ListFlowable(
        [ListItem(Paragraph(item, style), leftIndent=0) for item in items],
        bulletType="1",
        start="1",
        leftIndent=14,
        bulletFontName=FONT,
        bulletFontSize=9,
        bulletColor=colors.HexColor("#315b7d"),
    )


def table(data, widths, style):
    t = Table(data, colWidths=widths, repeatRows=1, hAlign="LEFT")
    t.setStyle(
        TableStyle(
            [
                ("FONTNAME", (0, 0), (-1, -1), FONT),
                ("FONTNAME", (0, 0), (-1, 0), BOLD),
                ("FONTSIZE", (0, 0), (-1, -1), 8.2),
                ("TEXTCOLOR", (0, 0), (-1, 0), colors.white),
                ("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#25506d")),
                ("BACKGROUND", (0, 1), (-1, -1), colors.HexColor("#f7fbfd")),
                ("GRID", (0, 0), (-1, -1), 0.35, colors.HexColor("#aebdc7")),
                ("VALIGN", (0, 0), (-1, -1), "TOP"),
                ("LEFTPADDING", (0, 0), (-1, -1), 5),
                ("RIGHTPADDING", (0, 0), (-1, -1), 5),
                ("TOPPADDING", (0, 0), (-1, -1), 4),
                ("BOTTOMPADDING", (0, 0), (-1, -1), 4),
            ]
            + style
        )
    )
    return t


def page(canvas, doc):
    canvas.saveState()
    width, height = A4
    canvas.setFillColor(colors.HexColor("#24475e"))
    canvas.rect(0, height - 16 * mm, width, 16 * mm, stroke=0, fill=1)
    canvas.setFillColor(colors.white)
    canvas.setFont(BOLD, 10)
    canvas.drawString(18 * mm, height - 10.5 * mm, "8259A 初始化程序编写方法")
    canvas.setFillColor(colors.HexColor("#6d8796"))
    canvas.setFont(FONT, 8)
    canvas.drawRightString(width - 18 * mm, 10 * mm, f"第 {doc.page} 页")
    canvas.restoreState()


styles = getSampleStyleSheet()
styles.add(
    ParagraphStyle(
        name="TitleCN",
        fontName=BOLD,
        fontSize=19,
        leading=23,
        alignment=TA_CENTER,
        textColor=colors.HexColor("#18364c"),
        spaceAfter=7,
    )
)
styles.add(
    ParagraphStyle(
        name="SubtitleCN",
        fontName=FONT,
        fontSize=9,
        leading=13,
        alignment=TA_CENTER,
        textColor=colors.HexColor("#53636f"),
        spaceAfter=8,
    )
)
styles.add(
    ParagraphStyle(
        name="H1CN",
        fontName=BOLD,
        fontSize=13.4,
        leading=18,
        textColor=colors.HexColor("#1f4d68"),
        spaceBefore=7,
        spaceAfter=5,
        keepWithNext=True,
    )
)
styles.add(
    ParagraphStyle(
        name="BodyCN",
        fontName=FONT,
        fontSize=9.1,
        leading=13.7,
        textColor=colors.HexColor("#1f2933"),
        alignment=TA_LEFT,
        spaceAfter=4,
    )
)
styles.add(
    ParagraphStyle(
        name="SmallCN",
        fontName=FONT,
        fontSize=8.1,
        leading=11.4,
        textColor=colors.HexColor("#26323d"),
        spaceAfter=4,
    )
)
styles.add(
    ParagraphStyle(
        name="CodeCN",
        fontName=MONO,
        fontSize=7.8,
        leading=9.8,
        leftIndent=5,
        rightIndent=5,
        borderWidth=0.4,
        borderColor=colors.HexColor("#bdd2df"),
        borderPadding=4,
        backColor=colors.HexColor("#f4f8fa"),
        textColor=colors.HexColor("#173143"),
        spaceBefore=3,
        spaceAfter=5,
    )
)
styles.add(
    ParagraphStyle(
        name="CalloutCN",
        fontName=BOLD,
        fontSize=9.3,
        leading=13.2,
        leftIndent=6,
        rightIndent=6,
        borderWidth=0.6,
        borderColor=colors.HexColor("#8aa9b8"),
        borderPadding=6,
        backColor=colors.HexColor("#eef7f6"),
        textColor=colors.HexColor("#24475e"),
        spaceBefore=5,
        spaceAfter=6,
    )
)


story = []
story.append(Spacer(1, 2 * mm))
story.append(para("8259A 初始化程序编写方法", styles["TitleCN"]))
story.append(
    para(
        "根据课件《中断.pdf》7.3 节整理: 把 8259A 初始化题拆成“读题 - 填位 - 算十六进制 - 写 OUT 程序”的固定流程。",
        styles["SubtitleCN"],
    )
)

story.append(para("一、初始化程序的固定骨架", styles["H1CN"]))
story.append(para("8259A 初始化必须按照 ICW 的顺序写入。单片 8259A 且需要 ICW4 时，最常见骨架如下:", styles["BodyCN"]))
story.append(
    code(
        """
; 设 8259A 的偶地址端口为 PORT0，奇地址端口为 PORT1
MOV AL, ICW1
OUT PORT0, AL
MOV AL, ICW2
OUT PORT1, AL
MOV AL, ICW4
OUT PORT1, AL
MOV AL, OCW1
OUT PORT1, AL
""",
        styles["CodeCN"],
    )
)
story.append(para("级联方式要在 ICW2 与 ICW4 之间增加 ICW3:", styles["BodyCN"]))
story.append(
    code(
        """
MOV AL, ICW1
OUT PORT0, AL
MOV AL, ICW2
OUT PORT1, AL
MOV AL, ICW3
OUT PORT1, AL
MOV AL, ICW4
OUT PORT1, AL
MOV AL, OCW1
OUT PORT1, AL
""",
        styles["CodeCN"],
    )
)
story.append(para("公式: 偶端口写 ICW1 和 OCW2/OCW3；奇端口写 ICW2、ICW3、ICW4、OCW1。", styles["CalloutCN"]))

story.append(para("二、从题目条件提取 6 个变量", styles["H1CN"]))
story.append(
    bullet(
        [
            "端口地址: 偶地址端口 PORT0，奇地址端口 PORT1。",
            "单片还是级联: 单片 SNGL=1；级联 SNGL=0，并且要写 ICW3。",
            "触发方式: 电平触发 LTIM=1；边沿触发 LTIM=0。",
            "8086/8088 模式: 通常 uPM=1，因此 ICW1 的 IC4=1，需要继续写 ICW4。",
            "中断类型码基值: 题目说“IR0 的中断类型码为 xxH”，这个值直接作为 ICW2。",
            "屏蔽哪些 IR 输入: 屏蔽 IRi 就令 OCW1 的 Di=1，不屏蔽则 Di=0。",
        ],
        styles["BodyCN"],
    )
)

story.append(para("三、ICW1 的公式", styles["H1CN"]))
story.append(para("ICW1 写入偶端口，D4 必须为 1，表示正在写初始化命令字。", styles["BodyCN"]))
story.append(
    table(
        [
            ["位", "名称", "取值规则"],
            ["D7-D5", "A7-A5", "8086/8088 系统中不用，通常写 000"],
            ["D4", "固定标志", "固定为 1"],
            ["D3", "LTIM", "电平触发为 1，边沿触发为 0"],
            ["D2", "ADI", "8086/8088 中不用，通常写 0"],
            ["D1", "SNGL", "单片为 1，级联为 0"],
            ["D0", "IC4", "需要 ICW4 为 1，不需要为 0"],
        ],
        [22 * mm, 30 * mm, 104 * mm],
        [],
    )
)
story.append(code("ICW1 = 10H + LTIM*08H + SNGL*02H + IC4", styles["CodeCN"]))
story.append(
    table(
        [
            ["条件", "ICW1 二进制", "ICW1 十六进制"],
            ["单片、边沿触发、需要 ICW4", "00010011B", "13H"],
            ["单片、电平触发、需要 ICW4", "00011011B", "1BH"],
            ["级联、边沿触发、需要 ICW4", "00010001B", "11H"],
            ["级联、电平触发、需要 ICW4", "00011001B", "19H"],
        ],
        [75 * mm, 42 * mm, 39 * mm],
        [],
    )
)
story.append(para("课件例题中“单片、电平触发、要 ICW4”，所以 ICW1=10H+08H+02H+01H=1BH。", styles["BodyCN"]))

story.append(para("四、ICW2 的公式", styles["H1CN"]))
story.append(para("ICW2 写入奇端口，用来设置中断类型码的高 5 位。8086/8088 系统中，IR0-IR7 的类型码连续排列:", styles["BodyCN"]))
story.append(
    code(
        """
IR0 类型码 = ICW2
IR1 类型码 = ICW2 + 1
...
IRi 类型码 = ICW2 + i
IR7 类型码 = ICW2 + 7
""",
        styles["CodeCN"],
    )
)
story.append(para("如果题目直接给“IR0 的中断类型码为 38H”，则 ICW2=38H。", styles["BodyCN"]))
story.append(para("如果题目给的是“IR5 的中断类型码为 1BH”，可先按连续类型码反推 ICW2=1BH-5=16H；但考试题最常见、也最稳的是直接给 IR0 的类型码。", styles["SmallCN"]))

story.append(para("五、ICW3 的公式", styles["H1CN"]))
story.append(para("只有级联时才写 ICW3。单片时不写 ICW3。", styles["BodyCN"]))
story.append(
    table(
        [
            ["对象", "计算方法", "例子"],
            ["主片", "如果主片 IRi 接有从片，则 Di=1；否则 Di=0", "从片接在 IR3 和 IR6: 01001000B=48H"],
            ["从片", "写入该从片连接到主片的 IR 编号", "接在 IR3: 03H；接在 IR6: 06H"],
        ],
        [24 * mm, 83 * mm, 49 * mm],
        [],
    )
)

story.append(para("六、ICW4 的公式", styles["H1CN"]))
story.append(para("ICW4 写入奇端口。只要 ICW1 的 D0=1，就必须继续写 ICW4。", styles["BodyCN"]))
story.append(
    table(
        [
            ["位", "名称", "取值规则"],
            ["D7-D5", "固定", "通常为 000"],
            ["D4", "SFNM", "特殊全嵌套为 1，普通全嵌套为 0"],
            ["D3", "BUF", "缓冲方式为 1，非缓冲方式为 0"],
            ["D2", "M/S", "缓冲方式下主片为 1，从片为 0；非缓冲方式通常为 0"],
            ["D1", "AEOI", "自动 EOI 为 1，普通/正常 EOI 为 0"],
            ["D0", "uPM", "8086/8088 模式为 1，MCS-80/85 模式为 0"],
        ],
        [22 * mm, 30 * mm, 104 * mm],
        [],
    )
)
story.append(code("ICW4 = SFNM*10H + BUF*08H + MS*04H + AEOI*02H + uPM", styles["CodeCN"]))
story.append(para("普通完全嵌套、非缓冲方式、普通 EOI、8086/8088 模式时，ICW4=01H。自动 EOI 时加 02H；特殊全嵌套时加 10H。", styles["BodyCN"]))

story.append(para("七、OCW1 的公式", styles["H1CN"]))
story.append(para("OCW1 是中断屏蔽字，写入奇端口。规则: 屏蔽 IRi -> Di=1；允许 IRi -> Di=0。", styles["BodyCN"]))
story.append(
    code(
        """
屏蔽 IR1 和 IR5:
OCW1 = 00100010B = 22H

屏蔽 IR3 和 IR7:
OCW1 = 10001000B = 88H
""",
        styles["CodeCN"],
    )
)

story.append(para("八、把题目套进模板", styles["H1CN"]))
story.append(para("题目若给: 端口 P0H/P1H、单片、正常完全嵌套、普通 EOI、高电平有效、IR0 类型码 TH、屏蔽 IRa 和 IRb，可直接套:", styles["BodyCN"]))
story.append(
    bullet(
        [
            "PORT0=P0H，PORT1=P1H。",
            "单片: SNGL=1；高电平有效按电平触发: LTIM=1。",
            "8086/8088: IC4=1，uPM=1。",
            "普通全嵌套、普通 EOI、非缓冲: ICW4=01H。",
            "ICW2=TH。",
            "OCW1=2^a+2^b。",
        ],
        styles["BodyCN"],
    )
)
story.append(
    code(
        """
MOV AL, ICW1
OUT P0H, AL
MOV AL, TH
OUT P1H, AL
MOV AL, 01H
OUT P1H, AL
MOV AL, OCW1
OUT P1H, AL
""",
        styles["CodeCN"],
    )
)

story.append(para("九、课件例题完整拆解", styles["H1CN"]))
story.append(para("题目: 一片 8259A，端口 60H、61H，普通完全嵌套，普通 EOI，高电平有效，IR0 类型号为 38H，IR1、IR5 屏蔽。", styles["BodyCN"]))
story.append(
    table(
        [
            ["条件", "推出"],
            ["端口 60H、61H", "60H 为偶端口，61H 为奇端口"],
            ["一片 8259A", "SNGL=1，不写 ICW3"],
            ["高电平有效", "LTIM=1"],
            ["8086/8088", "IC4=1，写 ICW4，uPM=1"],
            ["普通完全嵌套", "SFNM=0"],
            ["普通 EOI", "AEOI=0"],
            ["IR0 类型号 38H", "ICW2=38H"],
            ["屏蔽 IR1、IR5", "OCW1=00100010B=22H"],
        ],
        [55 * mm, 101 * mm],
        [],
    )
)
story.append(
    code(
        """
ICW1 = 10H + 08H + 02H + 01H = 1BH
ICW2 = 38H
ICW4 = 01H
OCW1 = 02H + 20H = 22H

MOV AL, 1BH
OUT 60H, AL
MOV AL, 38H
OUT 61H, AL
MOV AL, 01H
OUT 61H, AL
MOV AL, 22H
OUT 61H, AL
""",
        styles["CodeCN"],
    )
)

story.append(para("十、综合题中的 8259A 初始化", styles["H1CN"]))
story.append(para("课件综合题: 8259A 端口为 170H 和 171H；单片；正常完全嵌套；普通 EOI；高电平有效；IR0 类型码为 38H；IR3 和 IR7 被屏蔽。", styles["BodyCN"]))
story.append(
    code(
        """
ICW1 = 10H + 08H + 02H + 01H = 1BH
ICW2 = 38H
ICW4 = 01H
OCW1 = 10001000B = 88H

MOV AL, 1BH
OUT 170H, AL
MOV AL, 38H
OUT 171H, AL
MOV AL, 01H
OUT 171H, AL
MOV AL, 88H
OUT 171H, AL
""",
        styles["CodeCN"],
    )
)

story.append(para("十一、普通 EOI 的收尾程序", styles["H1CN"]))
story.append(para("如果 ICW4 中 AEOI=0，表示正常/普通 EOI。中断服务程序返回前必须向 8259A 偶端口送 EOI 命令 20H。", styles["BodyCN"]))
story.append(
    code(
        """
MOV AL, 20H
OUT PORT0, AL
IRET
""",
        styles["CodeCN"],
    )
)
story.append(para("注意: EOI 命令不是初始化命令字，而是中断服务程序结束时发给 8259A 的操作命令。初始化程序里通常写 ICW 和 OCW1；服务程序里才写 EOI。", styles["CalloutCN"]))

story.append(para("十二、考试快速检查表", styles["H1CN"]))
story.append(
    bullet(
        [
            "ICW1 是否写到了偶端口。",
            "ICW2 是否写到了奇端口。",
            "单片时是否没有多写 ICW3。",
            "级联时是否写了 ICW3，并且主片和从片含义没有写反。",
            "ICW1 的 D4 是否为 1。",
            "电平触发是否令 ICW1 的 D3=1。",
            "单片是否令 ICW1 的 D1=1。",
            "8086/8088 模式是否令 ICW1 的 D0=1，并继续写 ICW4。",
            "普通 EOI 是否令 ICW4 的 D1=0。",
            "OCW1 是否是“屏蔽为 1，允许为 0”，不要写反。",
            "普通 EOI 的中断服务程序末尾是否 OUT PORT0, 20H。",
        ],
        styles["BodyCN"],
    )
)
story.append(para("最核心的一句话: 先算 ICW1，再填 ICW2，单片跳过 ICW3，8086 写 ICW4，最后用 OCW1 屏蔽中断源。", styles["CalloutCN"]))


def build():
    doc = BaseDocTemplate(
        str(OUT),
        pagesize=A4,
        leftMargin=18 * mm,
        rightMargin=18 * mm,
        topMargin=22 * mm,
        bottomMargin=18 * mm,
        title="8259A 初始化程序编写方法",
        author="Codex",
    )
    frame = Frame(doc.leftMargin, doc.bottomMargin, doc.width, doc.height, id="normal")
    doc.addPageTemplates([PageTemplate(id="main", frames=[frame], onPage=page)])
    doc.build(story)


if __name__ == "__main__":
    build()
    print(OUT)
