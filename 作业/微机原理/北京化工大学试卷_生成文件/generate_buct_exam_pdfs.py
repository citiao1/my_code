from pathlib import Path
import re

import numpy as np
from PIL import Image
from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER, TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.pdfgen import canvas
from reportlab.platypus import (
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


WORK_DIR = Path(__file__).resolve().parent
ROOT = WORK_DIR.parent
INPUT_RENDER_DIR = WORK_DIR / "rendered_input"
CLEAN_DIR = WORK_DIR / "cleaned_pages"
RENDERED_DIR = WORK_DIR / "rendered_output"

QUESTION_PDF = ROOT / "北京化工大学微机原理2017-2019_题目版.pdf"
ANSWER_PDF = ROOT / "北京化工大学微机原理2017-2019_答案解析.pdf"
SUMMARY_PDF = ROOT / "北京化工大学微机原理2017-2019_知识点总结.pdf"

FONT_REG = "NotoSansSC"
FONT_BOLD = "NotoSansSC-Bold"
FONT_MONO = "SimSun"


def register_fonts():
    candidates = [
        (FONT_REG, r"C:\Windows\Fonts\NotoSansSC-VF.ttf"),
        (FONT_BOLD, r"C:\Windows\Fonts\msyhbd.ttc"),
        (FONT_MONO, r"C:\Windows\Fonts\simsun.ttc"),
    ]
    for name, path in candidates:
        pdfmetrics.registerFont(TTFont(name, path))


def styles():
    base = getSampleStyleSheet()
    return {
        "title": ParagraphStyle(
            "title",
            parent=base["Title"],
            fontName=FONT_BOLD,
            fontSize=20,
            leading=28,
            alignment=TA_CENTER,
            spaceAfter=10,
        ),
        "h1": ParagraphStyle(
            "h1",
            parent=base["Heading1"],
            fontName=FONT_BOLD,
            fontSize=15,
            leading=21,
            spaceBefore=10,
            spaceAfter=6,
            textColor=colors.HexColor("#12343b"),
        ),
        "h2": ParagraphStyle(
            "h2",
            parent=base["Heading2"],
            fontName=FONT_BOLD,
            fontSize=12.5,
            leading=18,
            spaceBefore=7,
            spaceAfter=4,
            textColor=colors.HexColor("#1d5c63"),
        ),
        "body": ParagraphStyle(
            "body",
            parent=base["BodyText"],
            fontName=FONT_REG,
            fontSize=10.2,
            leading=16,
            alignment=TA_LEFT,
            spaceAfter=4,
        ),
        "small": ParagraphStyle(
            "small",
            parent=base["BodyText"],
            fontName=FONT_REG,
            fontSize=8.8,
            leading=13,
            textColor=colors.HexColor("#475569"),
            spaceAfter=4,
        ),
        "code": ParagraphStyle(
            "code",
            parent=base["Code"],
            fontName=FONT_MONO,
            fontSize=8.8,
            leading=12,
            leftIndent=0,
            spaceBefore=4,
            spaceAfter=6,
        ),
    }


def para(text, style):
    return Paragraph(text, style)


def bullet_list(items, st):
    return ListFlowable(
        [ListItem(Paragraph(item, st["body"]), leftIndent=11) for item in items],
        bulletType="bullet",
        start="circle",
        leftIndent=16,
        bulletFontName=FONT_REG,
        bulletFontSize=7,
    )


def numbered(items, st):
    return ListFlowable(
        [ListItem(Paragraph(item, st["body"]), leftIndent=14) for item in items],
        bulletType="1",
        leftIndent=18,
        bulletFontName=FONT_REG,
        bulletFontSize=9,
    )


def answer_items(items, st):
    rows = []
    for item in items:
        rows.append(Paragraph(item, st["body"]))
    return rows


def header_footer(c: canvas.Canvas, doc):
    c.saveState()
    c.setFont(FONT_REG, 8.5)
    c.setFillColor(colors.HexColor("#64748b"))
    c.drawString(18 * mm, 12 * mm, "北京化工大学《微机原理及接口技术》2017-2019 复习整理")
    c.drawRightString(A4[0] - 18 * mm, 12 * mm, f"{doc.page}")
    c.restoreState()


def doc(path, title):
    return SimpleDocTemplate(
        str(path),
        pagesize=A4,
        rightMargin=18 * mm,
        leftMargin=18 * mm,
        topMargin=16 * mm,
        bottomMargin=18 * mm,
        title=title,
    )


def clean_page(src: Path, dst: Path):
    im = Image.open(src).convert("RGB")
    a = np.asarray(im).astype(np.int16)
    mx = a.max(axis=2)
    mn = a.min(axis=2)
    sat = mx - mn
    mean = a.mean(axis=2)

    # Keep black/neutral printed strokes and page rules. The source PDFs are
    # flattened scans: colored notes can be removed reliably, while dark pencil
    # that overlaps the question text cannot be fully separated from printing.
    neutral_dark = (sat < 34) & (mx < 135)
    very_dark = mx < 80
    printed = neutral_dark | very_dark

    out = np.full_like(a, 255, dtype=np.uint8)
    out[printed] = a[printed].clip(0, 255).astype(np.uint8)

    # Strengthen remaining dark text for legibility after aggressive cleaning.
    out_gray = out.mean(axis=2)
    blacken = out_gray < 150
    out[blacken] = [0, 0, 0]
    Image.fromarray(out).save(dst, optimize=True)


def build_question_pdf():
    CLEAN_DIR.mkdir(exist_ok=True)
    page_files = []
    for src in sorted(INPUT_RENDER_DIR.glob("*.png"), key=lambda p: natural_key(p.name)):
        dst = CLEAN_DIR / src.name
        clean_page(src, dst)
        page_files.append(dst)

    c = canvas.Canvas(str(QUESTION_PDF), pagesize=A4)
    w, h = A4
    for idx, png in enumerate(page_files, 1):
        year = "2017-2018" if "2017-2018" in png.name else "2018-2019"
        c.setFont(FONT_BOLD, 12)
        c.drawCentredString(w / 2, h - 10 * mm, f"{year} 题目版")
        c.drawImage(str(png), 12 * mm, 12 * mm, width=w - 24 * mm, height=h - 28 * mm, preserveAspectRatio=True, anchor="c")
        c.setFont(FONT_REG, 8)
        c.setFillColor(colors.HexColor("#64748b"))
        c.drawRightString(w - 12 * mm, 8 * mm, f"{idx}")
        c.setFillColor(colors.black)
        c.showPage()
    c.save()


def natural_key(name):
    return [int(s) if s.isdigit() else s for s in re.split(r"(\d+)", name)]


ANSWERS_2017 = [
    ("一、填空题", [
        "1. 主机包括 CPU、存储器、I/O 接口和总线。",
        "2. x=+22, y=-39, n=8: [x+y]补 = 11101111B；[y-x]补 = 11000011B。",
        "3. NMI 引脚产生非屏蔽中断；另一个中断请求引脚是 INTR。",
        "4. 系统总线按功能分为数据总线、地址总线和控制总线。",
        "5. 中断向量是中断服务程序入口地址；8086 中断向量表物理地址 00000H-003FFH；n=10 时向量从 00028H 开始。",
        "6. CPU 完成一次基本存储器操作所需时间称为总线周期。",
        "7. BIU 为总线接口部件，EU 为执行部件，通过 6 字节指令队列并行工作。",
        "8. RAM 包括 SRAM 和 DRAM。",
        "9. ALE 为地址锁存允许信号。",
        "10. 数据传送方式：无条件传送、查询传送、中断传送、DMA 传送。",
        "11. [BP+SI] 为基址变址寻址，默认 SS，物理地址 = 1000H*10H + (00A0H+0050H) = 100F0H；[BX] 为寄存器间接寻址，默认 DS，物理地址 = 15000H + 0500H = 15500H。",
        "12. 清 BX 的 D3、D8: AND BX, 0FEF7H；100H 字节低 2 位取反: XOR BYTE PTR [100H], 03H。",
    ]),
    ("二、选择题", [
        "1 C；2 D；3 A；4 B；5 B；6 G、F；7 B、F；8 D；9 D；10 B；11 C；12 B；13 C；14 C。",
    ]),
    ("三、编程和读程题", [
        "1. 33H-4FH = E4H，CF=1，OF=0。",
        "2. STR1 DW 'AB' 在小端下 AX=4241H；CNT 从 STR1 到当前位置，包含 STR1 与 STR2，共 12。",
        "3. 串操作类题重点：REP STOSB 等价于循环执行 ES:[DI] <- AL, DI <- DI+1, CX <- CX-1。",
        "4. 堆栈和 CALL/RET 类题按小端、先减 SP 后写入、RET 从栈顶弹出 IP 的规则计算。",
        "5. 存储器扩展题按总容量/芯片容量求芯片数，再按位宽分组。",
    ]),
    ("四、接口与综合题", [
        "8255 方式控制字、端口地址译码、LED/开关连接、8259 中断向量初始化是重点；解题按端口地址、控制字、读写顺序、屏蔽字四步写。",
    ]),
]


ANSWERS_2018 = [
    ("一、填空题", [
        "1. x=+92, y=-79, n=8: [x+y]补 = 00001101B；[x-y]补 = 10101011B。",
        "2. NMI 为非屏蔽中断；响应不受 FLAGS 的 IF 位影响。",
        "3. 系统总线分为数据总线、地址总线、控制总线。",
        "4. 中断向量是中断服务程序入口地址；8086 向量表地址 00000H-003FFH，共 256 个向量；0060H/4 = 18H。",
        "5. 执行一条指令所需时间为指令周期；8086 基本总线周期通常含 4 个时钟周期；每个时钟周期称为 T 状态。",
        "6. [BP+SI] 为基址变址寻址，默认 SS，物理地址 = 40E0H*10H + 10F0H + 39F1H = 459E1H；[100H] 为直接寻址，默认 DS，物理地址 = 398A0H + 0100H = 399A0H。",
        "7. 清 BX 的 D4、D7: AND BX, 0FF6FH；2000H 字节低 4 位取反: XOR BYTE PTR [2000H], 0FH。",
        "8. 8086 CPU 由 BIU 和 EU 组成，通过 6 字节指令队列并行工作。",
        "9. 数据传送方式：无条件、查询、中断、DMA。",
    ]),
    ("二、选择题", [
        "1 D；2 B；3 C；4 C；5 C；6 D；7 B；8 A；9 D；10 B；11 D；12 C；13 D；14 B；15 B、F。",
    ]),
    ("三、编程和读程题", [
        "1. 93H-4DH = 46H，CF=0，OF=1。",
        "2. MOV WORD PTR [BX],2024H 将 24H 写入 DS:1800H，20H 写入 DS:1801H；执行 MOV SP,1150H 后栈顶逻辑地址 SS:1150H，物理地址 11150H。",
        "3. 汇编片段题检查寄存器变化、内存小端顺序、FLAGS 变化以及 LOOP 对 CX 的影响。",
        "4. 接口程序题先写初始化控制字，再按端口读状态/读数据/写数据，注意 I/O 端口宽度和端口地址。",
    ]),
    ("四、接口与综合题", [
        "常见答案包括 8255 方式 0 控制字设置、8259 ICW1-ICW4 与 OCW1 设置、中断向量表写入、开中断 STI、以及中断服务程序末尾发送 EOI。",
    ]),
]


KNOWLEDGE = [
    ("8086 基础结构", [
        "8086 由 BIU 和 EU 组成。BIU 负责总线访问、取指和形成 20 位物理地址；EU 负责译码和执行。",
        "8086 地址线 20 根，寻址空间 1MB；数据总线 16 位；8088 外部数据总线 8 位。",
        "物理地址公式：物理地址 = 段地址 * 10H + 偏移地址。CS:IP 指向下一条待取指令，SS:SP 指向栈顶。",
        "8086 中一个基本总线周期通常含 T1-T4 四个 T 状态，可插入 Tw 等待状态。",
    ]),
    ("数制、补码和标志位", [
        "n 位补码范围：-2^(n-1) 到 2^(n-1)-1。正数补码等于原码；负数补码为绝对值按位取反加 1。",
        "减法 A-B 可看成 A+[-B]补。CF 表示无符号借位/进位，OF 表示有符号溢出。",
        "ZF=1 表示结果为 0；SF 取结果最高位；PF 表示低 8 位 1 的个数为偶数。",
    ]),
    ("寻址方式和小端存储", [
        "立即寻址、寄存器寻址、直接寻址、寄存器间接寻址、基址/变址/基址变址/相对基址变址寻址要能识别。",
        "含 BP 的存储器操作默认段寄存器为 SS；不含 BP 的 [BX]/[SI]/[DI] 默认 DS，可用段超越前缀改变。",
        "8086 小端存储：低地址存放低字节，高地址存放高字节。例如 2024H 写入内存时低地址为 24H。",
    ]),
    ("指令和汇编程序", [
        "IN/OUT 使用 AL/AX 与端口传送，直接端口地址只能是 8 位立即数；大于 FFH 的端口地址要放入 DX。",
        "LEA 装入偏移地址，不访问内存；MOV 两个操作数不能同时为内存。",
        "LOOP 先 CX <- CX-1，再判断 CX 是否为 0；REP 与串操作配合，由 CX 控制重复次数。",
        "PUSH 先 SP <- SP-2 后写入字；POP 先读栈顶字再 SP <- SP+2。",
    ]),
    ("存储器和总线", [
        "系统总线按功能分为数据总线、地址总线、控制总线。",
        "存储器芯片扩展：位扩展满足数据总线宽度，字扩展满足容量深度；芯片总数 = 系统总容量 / 单片容量。",
        "SRAM 速度快、无需刷新；DRAM 集成度高、需要刷新。",
    ]),
    ("中断系统", [
        "NMI 为非屏蔽中断，不受 IF 影响；INTR 为可屏蔽中断，响应条件通常为 IF=1 且 INTR 有效。",
        "8086 中断向量表位于 00000H-003FFH，共 256 个向量，每个向量 4 字节，存放 IP 低字、CS 高字。",
        "中断向量地址公式：向量 n 的入口存放地址 = 4*n。若向量从 0060H 开始，则类型号 = 0060H/4 = 18H。",
        "中断响应过程：关中断、压 FLAGS、CS、IP，取向量转入 ISR；IRET 弹回 IP、CS、FLAGS。",
    ]),
    ("I/O 数据传送方式", [
        "无条件传送适用于外设总是就绪；查询传送通过状态位判断；中断传送由外设主动请求；DMA 由 DMAC 控制内存与外设直接交换。",
        "最小模式下常见控制信号：内存读 RD=0 且 M/IO 表示内存；I/O 写 WR=0 且 M/IO 表示 I/O。",
    ]),
]


CODE_8255 = r"""
; 8255 control word format for mode set:
; D7=1
; D6-D5: Group A mode, 00=mode0, 01=mode1, 1x=mode2
; D4: PA direction, 1=input, 0=output
; D3: PC upper direction, 1=input, 0=output
; D2: Group B mode, 0=mode0, 1=mode1
; D1: PB direction, 1=input, 0=output
; D0: PC lower direction, 1=input, 0=output
;
; Formula:
; control = 80H
;         + GA_mode*20H
;         + PA_in*10H
;         + PCU_in*08H
;         + GB_mode*04H
;         + PB_in*02H
;         + PCL_in
;
; Example: PA input, PB output, PC upper/lower output, all mode 0:
MOV DX, P8255_CTRL
MOV AL, 90H        ; 1001 0000B
OUT DX, AL
;
; Bit set/reset (BSR) for PC bit:
; D7=0, D3-D1=bit number 0-7, D0=1 set / 0 reset
; Example: set PC3, then reset PC3:
MOV DX, P8255_CTRL
MOV AL, 00000111B  ; set PC3
OUT DX, AL
MOV AL, 00000110B  ; reset PC3
OUT DX, AL
"""


CODE_8259 = r"""
; 8259A initialization sequence:
; ICW1 -> command port
; ICW2 -> data port, interrupt vector base
; ICW3 -> data port, cascade wiring
; ICW4 -> data port when IC4=1
; OCW1 -> data port, interrupt mask register
;
; ICW1 bits:
; D4=1 fixed, D3=1 level / 0 edge, D2=1 interval4 / 0 interval8,
; D1=1 single / 0 cascade, D0=1 need ICW4
;
; Common 8086 edge-triggered cascade mode:
PIC_CMD  EQU 20H
PIC_DATA EQU 21H
MOV AL, 11H        ; ICW1: edge, cascade, need ICW4
OUT PIC_CMD, AL
MOV AL, 08H        ; ICW2: vector base, IRQ0 type=08H
OUT PIC_DATA, AL
MOV AL, 04H        ; ICW3 master: slave connected to IR2
OUT PIC_DATA, AL
MOV AL, 01H        ; ICW4: 8086/8088 mode, normal EOI
OUT PIC_DATA, AL
MOV AL, 0F8H       ; OCW1: unmask IR0-IR2, mask IR3-IR7
OUT PIC_DATA, AL
STI
;
; ISR end, non-specific EOI:
MOV AL, 20H
OUT PIC_CMD, AL
IRET
;
; Formula:
; interrupt_type = vector_base + IRQ_number
; vector_table_address = 4 * interrupt_type
; IVT stores IP at [4n], CS at [4n+2]
"""


def build_answer_pdf():
    st = styles()
    story = [para("北京化工大学微机原理 2017-2019 答案解析", st["title"])]
    story.append(para("说明：本答案依据扫描卷面中可辨认的批注、题干信息和微机原理常规解法整理。个别综合大题原卷手写遮挡较重，按题型给出可复用的解题规则。", st["small"]))
    for year, sections in [("2017-2018", ANSWERS_2017), ("2018-2019", ANSWERS_2018)]:
        story.append(para(year, st["h1"]))
        for title, items in sections:
            story.append(para(title, st["h2"]))
            story.extend(answer_items(items, st))
    doc(ANSWER_PDF, "答案解析").build(story, onFirstPage=header_footer, onLaterPages=header_footer)


def build_summary_pdf():
    st = styles()
    story = [para("北京化工大学微机原理 2017-2019 知识点总结", st["title"])]
    story.append(para("覆盖两份试卷反复出现的 8086、存储器、总线、中断、8255、8259 和汇编程序题。", st["small"]))
    for title, items in KNOWLEDGE:
        story.append(para(title, st["h1"]))
        story.append(bullet_list(items, st))
    story.append(PageBreak())
    story.append(para("8255 初始化公式与模板", st["h1"]))
    story.append(para("8255 控制字分为方式选择控制字和 PC 位设置/复位 BSR 控制字。考试中通常先确定 PA/PB/PC 的输入输出方向，再代入控制字位。", st["body"]))
    story.append(Preformatted(CODE_8255.strip(), st["code"]))
    story.append(para("8259 初始化公式与模板", st["h1"]))
    story.append(para("8259 初始化必须按 ICW1、ICW2、ICW3、ICW4 的顺序写入，之后用 OCW1 设置中断屏蔽。服务程序末尾通常向命令端口写 20H 发送 EOI。", st["body"]))
    story.append(Preformatted(CODE_8259.strip(), st["code"]))
    doc(SUMMARY_PDF, "知识点总结").build(story, onFirstPage=header_footer, onLaterPages=header_footer)


def main():
    CLEAN_DIR.mkdir(exist_ok=True)
    RENDERED_DIR.mkdir(exist_ok=True)
    register_fonts()
    build_question_pdf()
    build_answer_pdf()
    build_summary_pdf()
    print(f"created: {QUESTION_PDF}")
    print(f"created: {ANSWER_PDF}")
    print(f"created: {SUMMARY_PDF}")


if __name__ == "__main__":
    main()
