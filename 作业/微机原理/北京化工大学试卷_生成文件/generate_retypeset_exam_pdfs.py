from pathlib import Path
import shutil

from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER, TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (
    Flowable,
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

QUESTION_PDF = ROOT / "北京化工大学微机原理2017-2019_重排题目版.pdf"
ANSWER_PDF = ROOT / "北京化工大学微机原理2017-2019_答案解析.pdf"
SUMMARY_PDF = ROOT / "北京化工大学微机原理2017-2019_知识点总结.pdf"

LEGACY_QUESTION_PDF = QUESTION_PDF.with_name(
    QUESTION_PDF.name.replace("_重排题目版.pdf", "_题目版.pdf")
)

FONT_REG = "NotoSansSC"
FONT_BOLD = "NotoSansSC-Bold"
FONT_MONO = "SimSun"


def register_fonts():
    for name, path in [
        (FONT_REG, r"C:\Windows\Fonts\NotoSansSC-VF.ttf"),
        (FONT_BOLD, r"C:\Windows\Fonts\msyhbd.ttc"),
        (FONT_MONO, r"C:\Windows\Fonts\simsun.ttc"),
    ]:
        pdfmetrics.registerFont(TTFont(name, path))


def make_styles():
    base = getSampleStyleSheet()
    return {
        "title": ParagraphStyle(
            "title",
            parent=base["Title"],
            fontName=FONT_BOLD,
            fontSize=20,
            leading=28,
            alignment=TA_CENTER,
            spaceAfter=8,
        ),
        "subtitle": ParagraphStyle(
            "subtitle",
            parent=base["BodyText"],
            fontName=FONT_REG,
            fontSize=9,
            leading=13,
            alignment=TA_CENTER,
            textColor=colors.HexColor("#64748b"),
            spaceAfter=8,
        ),
        "h1": ParagraphStyle(
            "h1",
            parent=base["Heading1"],
            fontName=FONT_BOLD,
            fontSize=15,
            leading=21,
            textColor=colors.HexColor("#12343b"),
            spaceBefore=9,
            spaceAfter=6,
        ),
        "h2": ParagraphStyle(
            "h2",
            parent=base["Heading2"],
            fontName=FONT_BOLD,
            fontSize=12.5,
            leading=17,
            textColor=colors.HexColor("#1f5f64"),
            spaceBefore=7,
            spaceAfter=4,
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
            spaceAfter=3,
        ),
        "code": ParagraphStyle(
            "code",
            parent=base["Code"],
            fontName=FONT_MONO,
            fontSize=8.8,
            leading=12,
            spaceBefore=4,
            spaceAfter=6,
        ),
    }


def p(text, st):
    return Paragraph(text, st)


class AnswerLines(Flowable):
    def __init__(self, lines=3, width=470, gap=16):
        super().__init__()
        self.lines = lines
        self.width = width
        self.gap = gap
        self.height = lines * gap + 4

    def draw(self):
        c = self.canv
        c.saveState()
        c.setStrokeColor(colors.HexColor("#b7c1ca"))
        for i in range(self.lines):
            y = self.height - 4 - i * self.gap
            c.line(0, y, self.width, y)
        c.restoreState()


class BlankBox(Flowable):
    def __init__(self, label="作答区", width=470, height=110):
        super().__init__()
        self.label = label
        self.width = width
        self.height = height

    def draw(self):
        c = self.canv
        c.saveState()
        c.setStrokeColor(colors.HexColor("#94a3b8"))
        c.roundRect(0, 0, self.width, self.height, 4, stroke=1, fill=0)
        c.setFont(FONT_REG, 9)
        c.setFillColor(colors.HexColor("#64748b"))
        c.drawString(8, self.height - 15, self.label)
        c.restoreState()


class MemoryConnectBlank(Flowable):
    def __init__(self, groups=8, chip="RAM 组"):
        super().__init__()
        self.groups = groups
        self.chip = chip
        self.width = 470
        self.height = 150

    def draw(self):
        c = self.canv
        c.saveState()
        c.setStrokeColor(colors.HexColor("#475569"))
        c.setFillColor(colors.white)
        c.setFont(FONT_REG, 8)
        c.rect(12, 44, 56, 58)
        c.drawCentredString(40, 75, "CPU")
        for i, label in enumerate(["A0-A?", "A?-A?", "D0-D7", "RD/WR"]):
            y = 94 - i * 15
            c.drawString(74, y, label)
            c.line(68, y + 2, 440, y + 2)
        start_x = 110
        chip_w = 36
        gap = min(13, (320 - self.groups * chip_w) / max(1, self.groups - 1))
        for i in range(self.groups):
            x = start_x + i * (chip_w + gap)
            c.rect(x, 18, chip_w, 38)
            c.drawCentredString(x + chip_w / 2, 43, f"{i+1}")
            c.drawCentredString(x + chip_w / 2, 31, self.chip)
            c.drawCentredString(x + chip_w / 2, 20, "CS WE")
        c.setFillColor(colors.HexColor("#64748b"))
        c.drawString(12, 126, "请补全：片内地址线、片选译码线、数据线、读写控制线和地址范围")
        c.restoreState()


class MemoryAnswerDiagram(Flowable):
    def __init__(self, chip_label, inner_addr, select_lines, ranges):
        super().__init__()
        self.chip_label = chip_label
        self.inner_addr = inner_addr
        self.select_lines = select_lines
        self.ranges = ranges
        self.width = 470
        self.height = 220

    def draw(self):
        c = self.canv
        c.saveState()
        c.setStrokeColor(colors.HexColor("#334155"))
        c.setFillColor(colors.white)
        c.setFont(FONT_REG, 8)
        c.rect(12, 90, 58, 68)
        c.setFillColor(colors.HexColor("#0f172a"))
        c.drawCentredString(41, 148, "8086/CPU")
        for i, label in enumerate([self.inner_addr, self.select_lines, "D0-D7", "RD/WR"]):
            y = 136 - i * 15
            c.drawString(75, y, label)
            c.line(70, y + 2, 430, y + 2)
        c.rect(115, 145, 72, 34)
        c.setFillColor(colors.HexColor("#0f172a"))
        c.drawCentredString(151, 165, "74LS138")
        c.drawCentredString(151, 153, "3-8 译码")
        start_x, chip_w, gap = 92, 40, 9
        for i in range(8):
            x = start_x + i * (chip_w + gap)
            c.setFillColor(colors.white)
            c.rect(x, 58, chip_w, 42)
            c.setFillColor(colors.HexColor("#0f172a"))
            c.drawCentredString(x + chip_w / 2, 86, f"组{i}")
            c.drawCentredString(x + chip_w / 2, 74, self.chip_label)
            c.drawCentredString(x + chip_w / 2, 62, "8片x1")
            c.line(151, 145, x + chip_w / 2, 100)
        c.setFillColor(colors.HexColor("#475569"))
        c.drawString(12, 31, "第一组地址范围：")
        y = 18
        for i, rng in enumerate(self.ranges):
            c.drawString(88 + (i % 2) * 185, y - (i // 2) * 12, rng)
        c.restoreState()


class SevenSegCircuitBlank(Flowable):
    def __init__(self):
        super().__init__()
        self.width = 470
        self.height = 170

    def draw(self):
        c = self.canv
        c.saveState()
        c.setStrokeColor(colors.HexColor("#475569"))
        c.setFont(FONT_REG, 8.5)
        c.rect(24, 62, 90, 74)
        c.drawCentredString(69, 128, "8259A")
        c.drawString(34, 111, "IR3 <- 脉冲")
        c.drawString(34, 96, "INT/INTA")
        c.drawString(34, 81, "CS RD WR A0")
        c.rect(190, 62, 98, 74)
        c.drawCentredString(239, 128, "8255A")
        c.drawString(204, 111, "PA0-PA7 -> 段码")
        c.drawString(204, 96, "PC0 -> 位码")
        c.drawString(204, 81, "方式 0 输出")
        c.rect(360, 78, 52, 44)
        c.drawCentredString(386, 102, "数码管")
        c.line(114, 99, 190, 99)
        c.line(288, 113, 360, 113)
        c.line(288, 91, 360, 91)
        c.drawString(12, 148, "请补全：8255/8259 初始化、向量表、ISR、显示子程序")
        c.restoreState()


class SevenSegAnswerDiagram(Flowable):
    def __init__(self, pc0_active="0"):
        super().__init__()
        self.pc0_active = pc0_active
        self.width = 470
        self.height = 185

    def draw(self):
        c = self.canv
        c.saveState()
        c.setStrokeColor(colors.HexColor("#334155"))
        c.setFont(FONT_REG, 8)
        c.rect(16, 68, 84, 76)
        c.drawCentredString(58, 134, "8259A")
        for i, label in enumerate(["IR3 <- 外部脉冲", "INT -> CPU INTR", "INTA <- CPU", "CS/RD/WR/A0"]):
            c.drawString(26, 118 - i * 14, label)
        c.rect(144, 68, 88, 76)
        c.drawCentredString(188, 134, "8086 CPU")
        c.drawString(154, 114, "响应 IR3")
        c.drawString(154, 100, "执行 INT3")
        c.drawString(154, 86, "刷新显示")
        c.rect(282, 68, 96, 76)
        c.drawCentredString(330, 134, "8255A")
        c.drawString(294, 116, "PA0-PA7 -> a-g,dp")
        c.drawString(294, 102, f"PC0={self.pc0_active} 点亮")
        c.drawString(294, 88, "方式0 输出")
        c.rect(410, 84, 42, 44)
        c.drawCentredString(431, 110, "数码管")
        c.drawCentredString(431, 96, "0-9")
        c.line(100, 116, 144, 116)
        c.drawString(111, 120, "INT")
        c.line(144, 96, 100, 96)
        c.drawString(110, 100, "INTA")
        c.line(232, 106, 282, 106)
        c.drawString(245, 110, "I/O")
        c.line(378, 116, 410, 116)
        c.drawString(382, 121, "段码")
        c.line(378, 94, 410, 94)
        c.drawString(382, 99, "位码")
        c.setFillColor(colors.HexColor("#475569"))
        c.drawString(16, 44, "向量表：0000:004C 写 INT3 的 IP，0000:004E 写 INT3 的 CS。ISR 末尾向 8259 命令端口写 20H 发 EOI。")
        c.restoreState()


def doc(path, title):
    return SimpleDocTemplate(
        str(path),
        pagesize=A4,
        leftMargin=18 * mm,
        rightMargin=18 * mm,
        topMargin=16 * mm,
        bottomMargin=18 * mm,
        title=title,
    )


def footer(c, d):
    c.saveState()
    c.setFont(FONT_REG, 8)
    c.setFillColor(colors.HexColor("#64748b"))
    c.drawString(18 * mm, 11 * mm, "北京化工大学《微机原理及接口技术》2017-2019 重排整理")
    c.drawRightString(A4[0] - 18 * mm, 11 * mm, str(d.page))
    c.restoreState()


def make_score_table(st, columns):
    data = [["题号", *columns, "总分"], ["得分", *["" for _ in columns], ""]]
    t = Table(data, colWidths=[32 * mm] + [22 * mm] * len(columns) + [28 * mm])
    t.setStyle(TableStyle([
        ("FONTNAME", (0, 0), (-1, -1), FONT_REG),
        ("FONTSIZE", (0, 0), (-1, -1), 9),
        ("GRID", (0, 0), (-1, -1), 0.6, colors.HexColor("#64748b")),
        ("ALIGN", (0, 0), (-1, -1), "CENTER"),
        ("VALIGN", (0, 0), (-1, -1), "MIDDLE"),
    ]))
    return t


def add_header(story, year, points):
    st = make_styles()
    story.append(p(f"北京化工大学 {year} 学年第一学期", st["title"]))
    story.append(p("《微机原理与接口技术》（电科）期末考试试卷  重排题目版", st["subtitle"]))
    story.append(p("班级：__________   姓名：__________   学号：__________   分数：__________", st["body"]))
    story.append(make_score_table(st, ["一", "二", "三", "四"]))
    story.append(Spacer(1, 6))


def q(story, text, lines=1):
    st = make_styles()
    story.append(p(text, st["body"]))
    if lines:
        story.append(AnswerLines(lines=lines))


def code_block(story, text):
    st = make_styles()
    story.append(Preformatted(text.strip(), st["code"]))


def build_questions():
    st = make_styles()
    story = []
    add_header(story, "2017-2018", 100)
    story.append(p("一、填空题（每空 1 分，共 29 分）", st["h1"]))
    for item in [
        "1. 微型计算机的主机包括 ________、________、________ 和总线。",
        "2. 设 x=+22，y=-39，计算机字长 n=8，则：[x+y]补=________B，[y-x]补=________B。",
        "3. 从 8086 CPU 的 NMI 引脚产生的中断叫做________，另一个中断请求引脚是________引脚。",
        "4. 计算机中的系统总线按功能分为数据总线、地址总线和________。",
        "5. 中断向量是指________；8086 CPU 的中断向量表存于物理地址________；n=10 的向量从________H 地址开始。",
        "6. CPU 完成一次基本存储器操作所需的时间叫做________。",
        "7. 8086 CPU 由两部分组成，其中 BIU 叫做________，EU 叫做________；它们之间利用________队列实现并行工作。",
        "8. 随机存储器 RAM 包括 SRAM 和________。",
        "9. 8086 CPU 的 ALE 引脚叫做________。",
        "10. 计算机与外设进行数据传送的方式包括________、________、________和________。",
        "11. 已知 CS=1200H，DS=1500H，ES=FC91H，BX=0500H，SS=1000H，SP=3C3AH，IP=537DH，SI=0050H，BP=00A0H。指出下列目的操作数的寻址方式并计算物理地址：ADD BYTE PTR [BP+SI],10H；MOV [BX],AX。",
        "12. 用一条指令完成：将 BX 中 D3、D8 位清 0，其余位不变；将内存 100H 字节单元低 2 位取反，高 6 位不变。",
    ]:
        q(story, item, lines=1 if len(item) < 90 else 2)
    story.append(p("二、选择题（每空 1 分，共 14 分）", st["h1"]))
    mc2017 = [
        "1. CPU 与外设的数据传送方式中，哪一种不需要 CPU 干预？ A 查询 B 中断 C DMA D 都需要",
        "2. 下列指令正确的是：A IN AX,180H  B LEA SI,1000H  C MOV [SI],BUFFER  D XLAT",
        "3. 在 8086 CPU 中，PUSH AX 是____的数据传送指令。A 16 位 B 8 或16 位 C 8 位 D 32 位",
        "4. DATA1 DW 12H,34H；DATA2 DB 3AH,0F3H。语法错误的是：A LEA SI,DATA1 B MOV AX,DATA2 C MOV AX,DATA1+2 D INC DATA2+1",
        "5. 8086 CPU 的中断向量表：A 存放类型号 B 存放 ISR 入口地址 C 存放向量地址 D 是返回地址",
        "6. 下一条将要取出的指令地址存放在____:____。A SP B IF C SS D DS E BP F IP G CS H BX",
        "7. 8086CPU 地址线有____根，寻址范围是____。A 8 B 16 C 32 D 64 E 64KB F 1MB G 32MB H 2GB",
        "8. 8086 最小模式下对 I/O 写操作时，有效控制信号为____。",
        "9. 16Kx1 位芯片组成 64KB 存储器，芯片组数和每组芯片数为____。A 2和8 B 1和16 C 4和16 D 4和8",
        "10. 8086 CPU 包括____。A 运算器、控制器和存储器 B 运算器、控制器和寄存器 C 运算器、控制器和接口部件 D 运算器、控制器和累加器",
        "11. AGAIN: MOV ES:[DI],AL / INC DI / LOOP AGAIN。等价指令为：A REP MOVSB B REP LODSB C REP STOSB D REPE SCASB",
        "12. 8086/8088 系统中，一个堆栈可使用的最大空间是____。A 1MB B 64KB C 由 SP 决定 D 由 SS 决定",
        "13. CPU 响应 INTR 中断请求时，中断类型号由____提供。A CPU 内部 B 中断指令 C 中断源 D 固定",
        "14. 直接通过芯片级总线与 CPU 相连的是____。A 键盘 B 磁盘驱动器 C 内存 D 显示器",
    ]
    for item in mc2017:
        q(story, item, lines=1)
    story.append(p("三、编程和读程题（21 分）", st["h1"]))
    q(story, "1. 执行 MOV AL,33H / MOV BL,4FH / SUB AL,BL 后，写出 (AL)、CF、OF。", 2)
    q(story, "2. 已知 STR0 DB 12H,34H,56H,78H；STR1 DW 'AB'；STR2 DB 10 DUP(?)；CNT EQU $-STR1。执行 MOV CX,CNT；MOV AX,STR1；MOV BL,STR0+2 后，求 CX、AX、BL。", 3)
    code_block(story, """
DSEG SEGMENT
ORG 3H
A   DB 10H
B   DW 'EF','CD'
C   DB 2 DUP(3)
SUM DD 80906050H
DSEG ENDS
""")
    q(story, "3. 根据上面的数据段定义画出内存示意图。'A' 的 ASCII 为 41H。", 0)
    story.append(BlankBox("内存示意图作答区", height=86))
    code_block(story, """
MOV AL,81H
MOV BL,22H
MOV CL,33H
MOV SI,100H
____ AL,BL
____ low1
________________
low1: ____ AL,CL
____ low2
________________
low2: ________________
""")
    q(story, "4. 补全程序：比较 AL、BL、CL 中无符号数大小，将最小数存到 100H 单元。", 0)
    story.append(p("四、综合题（36 分）", st["h1"]))
    q(story, "1. 用 16Kx1 RAM 芯片组成 128Kx8 内存储器，CPU 地址线 20 根、数据线 8 根，可采用 74LS138 译码器。求芯片数、片内地址线数、最少片选地址线数；画 CPU-存储器连接图；判断是否地址覆盖并写出第一组地址范围。", 0)
    story.append(MemoryConnectBlank(groups=8, chip="16Kx8"))
    q(story, "2. 8255A 作输入接口：PA 为数据输入，方式 0，查询方式；PC3=1 表示 READY。端口地址 80H-83H。补全程序，从 PA 输入 20 字节存到 1000H 开始。", 0)
    code_block(story, """
MOV AL,91H
OUT __________
MOV CX,20
MOV DI,1000H
XML: __________
     __________
     ____ XML
     __________
     MOV [DI],AL
     INC DI
     __________
""")
    q(story, "3. 秒表电路：8259 IR3 接收 100Hz 脉冲，计数成 1 秒；8255 PA 接数码管段码，PC0 接位码，PC0=0 点亮。8255 地址 280H-283H，8259 地址 80H/81H，IR3 类型号 13H。补全 8255 初始化、向量表、8259 初始化、ISR 和显示子程序。", 0)
    story.append(SevenSegCircuitBlank())
    story.append(BlankBox("程序补全作答区", height=125))

    story.append(PageBreak())
    add_header(story, "2018-2019", 100)
    story.append(p("一、填空题（第 9 小题每空 2 分，其余每空 1 分，共 27 分）", st["h1"]))
    for item in [
        "1. 设 x=+92，y=-79，计算机字长 n=8，则：[x+y]补=________B，[x-y]补=________B。",
        "2. 从 8086 CPU 的 NMI 引脚产生的中断叫做________，它的响应不受 FLAGS 的________位影响。",
        "3. 系统总线按功能分为________、________和________。",
        "4. 中断向量是指________；8086 向量表物理地址为 00000H-003FFH，共________个向量。若向量存放在 0060H-0063H，则类型号为________。",
        "5. CPU 执行一条指令所需时间称为________；8086 基本总线周期通常包含________个时钟周期，每个时钟周期称为一个________。",
        "6. 已知 CS=7A05H，DS=398AH，ES=FC91H，BX=1EF3H，SS=40E0H，SP=3C3AH，IP=537DH，SI=39F1H，BP=10F0H。求 ADD BYTE PTR [BP+SI],10H 和 CMP BYTE PTR [100H],0 的寻址方式及物理地址。",
        "7. 用一条指令完成：将 BX 的 D4、D7 清 0；将 2000H 字节单元低 4 位取反，高 4 位不变。",
        "8. Intel 8086 CPU 由________和________组成，它们之间利用________队列实现并行工作。",
        "9. 计算机与外设数据传送方式包括________、________、________和________。",
    ]:
        q(story, item, lines=1 if len(item) < 90 else 2)
    story.append(p("二、选择题（1×15=15 分）", st["h1"]))
    mc2018 = [
        "1. 下列哪一种不是 CPU 与外设的数据传送方式？A 查询 B 中断 C DMA D 串行传送",
        "2. 下列指令正确的是：A OUT AX,80H B SHL AX,CL C LEA SI,1000H D MOV [SI],BUFFER",
        "3. 8086 中堆栈操作指令是____的数据传送指令。A 8 位 B 8或16 位 C 16 位 D 32 位",
        "4. DATA1 DW 12H,34H；DATA2 DB 3AH,0F3H。语法错误的是：A LEA SI,DATA1 B MOV AX,DATA1+2 C MOV AX,DATA2 D INC DATA2+1",
        "5. 地址/数据分时复用线可通过____分离地址信息。A 寄存器 B 缓冲器 C 锁存器 D 译码器",
        "6. IP 中存放的是____。A 当前指令 B 下一条要执行的指令 C 操作数地址 D 下一条要执行指令的地址",
        "7. 微处理器寻址范围与____有关。A 数据总线宽度 B 地址总线宽度 C 控制总线宽度 D 寄存器个数",
        "8. 8086 最小模式下对存储器读操作，有效控制信号为____。",
        "9. 8086/8088 响应可屏蔽中断条件是____。A IF=0,INTR=0 B IF=0,INTR=1 C IF=1,INTR=0 D IF=1,INTR=1",
        "10. 8086 CPU 包括____。A 运算器、控制器和存储器 B 运算器、控制器和寄存器 C 运算器、控制器和接口部件 D 运算器、控制器和累加器",
        "11. DMA 控制方式下由____控制数据传送。A CPU B 软件 C 存储器管理部件 D DMAC",
        "12. 地址译码器输入端应接到 CPU 的____上。A 控制总线 B 数据总线 C 地址总线 D 外部总线",
        "13. CPU 响应可屏蔽中断请求时，中断类型号由____提供。A CPU 内部 B 中断指令 C 固定 D 中断控制器",
        "14. 8086 中断向量表____。A 存类型号 B 存 ISR 入口地址 C 存向量地址 D 是返回地址",
        "15. 8086 地址线有____根，寻址范围____。A 8 B 16 C 32 D 64 E 64KB F 1MB G 32MB H 2GB",
    ]
    for item in mc2018:
        q(story, item, 1)
    story.append(p("三、编程和读程题（30 分）", st["h1"]))
    q(story, "1. 执行 MOV AL,93H / MOV BL,4DH / SUB AL,BL 后，写出 (AL)、CF、OF。", 2)
    code_block(story, """
已知执行前：CS=1200H, SS=1000H, SP=2CB0H, IP=26C5H
MOV AX,123DH
MOV BX,1800H
MOV WORD PTR [BX],2024H
MOV SP,1150H
XCHG AX,[BX]
PUSH AX
PUSH [BX]
POP AX
""")
    q(story, "2. 写出执行结果：AX、栈顶逻辑地址、DS:1800H 中的字数据等。", 3)
    code_block(story, """
DSEG SEGMENT
A   DB 20H
B   DW 'AB'
C   DB 3 DUP(2)
SUM DD 12340320H
DSEG ENDS
""")
    q(story, "3. 根据上面的数据段定义画出内存示意图。", 0)
    story.append(BlankBox("内存示意图作答区", height=82))
    q(story, "4. 统计字符串 'Never too old to learn$' 中字符 'o' 的个数并将其删除，形成的新字符串送至 NEWSTR 处存放。补全程序。", 0)
    code_block(story, """
DATAS SEGMENT
STR DB 'Never too old to learn$'
LEN EQU $-STR
NEWSTR DB 40 DUP(?)
DATAS ENDS
CODES SEGMENT
ASSUME CS:CODES,DS:DATAS
START:
    MOV AX,DATAS
    MOV DS,AX
    __________
    LEA DI,NEWSTR
    MOV CX,LEN
    MOV DL,0
CHECK:
    MOV AL,[SI]
    __________
    JZ NEXT
    __________
    INC DI
    JMP COMMON
NEXT:
    __________
COMMON:
    INC SI
    __________
    MOV AH,4CH
    INT 21H
CODES ENDS
END START
""")
    story.append(p("四、综合题（32 分）", st["h1"]))
    q(story, "1. 用 32Kx1 RAM 芯片组成 256Kx8 内存储器，CPU 地址线 20 根、数据线 8 根，可采用 74LS138 译码器。求芯片数、片内地址线数、最少片选地址线数；画连接图；判断是否地址覆盖并写出第一组地址范围。", 0)
    story.append(MemoryConnectBlank(groups=8, chip="32Kx8"))
    q(story, "2. 输出端口 4CH，输入状态端口 4DH。从输入口读外部状态，若不为 80H，输出 FFH 关闭设备；否则输出 00H 开启设备。补全程序。", 0)
    code_block(story, """
    ____ AL,4DH
    ____ AL,80H
    JNZ GOFF
GON:
    MOV AL,00H
    OUT ____,AL
    JMP TMP
GOFF:
    MOV AL,0FFH
    OUT ____,AL
TMP:
""")
    q(story, "3. 玩具计数系统：光电检测脉冲接 8259A IR3；8255 PA 接数码管段码，PC0 接位码，PC0=1 点亮。8255 地址 0FF20H-0FF23H，8259 地址 0FF80H/0FF81H，IR3 类型号 13H。补全 8255 初始化、向量表、8259 初始化、ISR 和显示子程序，实现 0-9 循环计数显示。", 0)
    story.append(SevenSegCircuitBlank())
    story.append(BlankBox("程序补全作答区", height=125))
    doc(QUESTION_PDF, "重排题目版").build(story, onFirstPage=footer, onLaterPages=footer)
    shutil.copyfile(QUESTION_PDF, LEGACY_QUESTION_PDF)


CODE_2017_MIN = """
; 比较 AL、BL、CL 中无符号数，将最小值存入 DS:0100H
MOV AL,81H
MOV BL,22H
MOV CL,33H
MOV SI,0100H
CMP AL,BL
JB  LOW1
XCHG AL,BL
LOW1:
CMP AL,CL
JB  LOW2
XCHG AL,CL
LOW2:
MOV [SI],AL
"""


CODE_2017_8255_INPUT = """
; 8255: PA 输入，PC3 状态输入，方式 0；端口 80H-83H
MOV AL,91H          ; 1001 0001B: PA 输入，PC 上半输入，PB/PC 下半输出
OUT 83H,AL
MOV CX,20
MOV DI,1000H
XML:
    IN  AL,82H      ; 读 PC
    TEST AL,08H     ; PC3=1 ?
    JZ  XML
    IN  AL,80H      ; 读 PA 数据
    MOV [DI],AL
    INC DI
    LOOP XML
"""


CODE_2017_STOPWATCH = """
DATA SEGMENT
TAB    DB 3FH,06H,5BH,4FH,66H,6DH,7DH,07H,7FH,6FH
Second DB 0
Tick   DB 0
DATA ENDS

CODE SEGMENT
ASSUME CS:CODE,DS:DATA

INT3 PROC FAR
    PUSH AX
    PUSH DS
    MOV AX,DATA
    MOV DS,AX
    INC Tick
    CMP Tick,100
    JB  INT_DONE
    MOV Tick,0
    INC Second
    CMP Second,10
    JB  INT_DONE
    MOV Second,0
INT_DONE:
    MOV AL,20H
    OUT 80H,AL      ; 8259 EOI
    POP DS
    POP AX
    IRET
INT3 ENDP

DISP PROC FAR
    PUSH AX
    PUSH BX
    PUSH DX
    MOV BL,Second
    MOV BH,0
    MOV AL,TAB[BX]
    MOV DX,280H
    OUT DX,AL       ; PA 段码
    MOV AL,0FEH     ; PC0=0 点亮
    MOV DX,282H
    OUT DX,AL
    POP DX
    POP BX
    POP AX
    RET
DISP ENDP

START:
    MOV AX,DATA
    MOV DS,AX
    CLI
    MOV AL,80H      ; 8255: PA、PC 输出，方式 0
    MOV DX,283H
    OUT DX,AL
    MOV AX,0
    MOV ES,AX
    MOV DI,13H*4
    MOV AX,OFFSET INT3
    MOV ES:[DI],AX
    MOV AX,SEG INT3
    MOV ES:[DI+2],AX
    MOV AL,13H      ; 8259 初始化示意：IR3 类型号 13H
    OUT 80H,AL
    MOV AL,10H
    OUT 81H,AL
    MOV AL,0F7H     ; 开 IR3
    OUT 81H,AL
    STI
BEGIN:
    CALL DISP
    JMP BEGIN
CODE ENDS
END START
"""


CODE_2018_STRING = """
DATAS SEGMENT
STR    DB 'Never too old to learn$'
LEN    EQU $-STR
NEWSTR DB 40 DUP(?)
COUNT  DB 0
DATAS ENDS

CODES SEGMENT
ASSUME CS:CODES,DS:DATAS
START:
    MOV AX,DATAS
    MOV DS,AX
    LEA SI,STR
    LEA DI,NEWSTR
    MOV CX,LEN
    MOV DL,0
CHECK:
    MOV AL,[SI]
    CMP AL,'o'
    JZ  NEXT
    MOV [DI],AL
    INC DI
    JMP COMMON
NEXT:
    INC DL
COMMON:
    INC SI
    LOOP CHECK
    MOV COUNT,DL
    MOV AH,4CH
    INT 21H
CODES ENDS
END START
"""


CODE_2018_DEVICE = """
IN  AL,4DH
CMP AL,80H
JNZ GOFF
GON:
    MOV AL,00H
    OUT 4CH,AL
    JMP TMP
GOFF:
    MOV AL,0FFH
    OUT 4CH,AL
TMP:
"""


CODE_2018_COUNTER = """
DATA SEGMENT
TAB     DB 3FH,06H,5BH,4FH,66H,6DH,7DH,07H,7FH,6FH
SheepCN DB 0
DATA ENDS

CODE SEGMENT
ASSUME CS:CODE,DS:DATA

INT3 PROC FAR
    PUSH AX
    PUSH DS
    MOV AX,DATA
    MOV DS,AX
    INC SheepCN
    CMP SheepCN,10
    JB  NEXT
    MOV SheepCN,0
NEXT:
    MOV AL,20H
    MOV DX,0FF80H
    OUT DX,AL       ; EOI
    POP DS
    POP AX
    IRET
INT3 ENDP

DISP PROC FAR
    PUSH AX
    PUSH BX
    PUSH DX
    MOV BL,SheepCN
    MOV BH,0
    MOV AL,TAB[BX]
    MOV DX,0FF20H
    OUT DX,AL       ; PA 段码
    MOV AL,01H      ; PC0=1 点亮
    MOV DX,0FF22H
    OUT DX,AL
    POP DX
    POP BX
    POP AX
    RET
DISP ENDP

START:
    MOV AX,DATA
    MOV DS,AX
    CLI
    MOV AL,80H      ; 8255: PA、PC 输出，方式 0
    MOV DX,0FF23H
    OUT DX,AL
    MOV AX,0
    MOV ES,AX
    MOV DI,13H*4
    MOV AX,OFFSET INT3
    MOV ES:[DI],AX
    MOV AX,SEG INT3
    MOV ES:[DI+2],AX
    MOV AL,13H      ; 按题给出的类型号/端口初始化
    MOV DX,0FF80H
    OUT DX,AL
    MOV AL,10H
    MOV DX,0FF81H
    OUT DX,AL
    MOV AL,0F7H     ; 允许 IR3
    OUT DX,AL
    STI
MAIN:
    CALL DISP
    JMP MAIN
CODE ENDS
END START
"""


def build_answers():
    st = make_styles()
    story = [p("北京化工大学微机原理 2017-2019 答案解析（重排版配套）", st["title"])]
    story.append(p("说明：编程题给出可运行/可参考的完整汇编框架；画图题给出连接关系和地址计算，实际画图按题目空白作图区完成。", st["small"]))

    story.append(p("2017-2018", st["h1"]))
    for text in [
        "填空：1 CPU、存储器、I/O 接口；2 11101111B、11000011B；3 非屏蔽中断、INTR；4 控制总线；5 ISR 入口地址、00000H-003FFH、00028H；6 总线周期；7 总线接口部件、执行部件、指令；8 DRAM；9 地址锁存允许；10 无条件、查询、中断、DMA。",
        "寻址：ADD BYTE PTR[BP+SI] 为基址变址寻址，默认 SS，PA=10000H+00A0H+0050H=100F0H；MOV [BX],AX 为寄存器间接寻址，默认 DS，PA=15000H+0500H=15500H。",
        "位操作：AND BX,0FEF7H；XOR BYTE PTR [100H],03H。",
        "选择题：1 C；2 D；3 A；4 B；5 B；6 G,F；7 B,F；8 D；9 D；10 B；11 C；12 B；13 C；14 C。",
        "SUB 33H-4FH=E4H。无符号发生借位 CF=1；两个正数相减结果为负但仍在有符号范围内，OF=0。",
        "STR1 DW 'AB' 小端存储，AX=4241H；CNT 从 STR1 到当前位置，包含 STR1 两字节和 STR2 十字节，CX=12；BL=STR0+2=56H。",
        "数据段图：ORG 3H 后 A 在偏移 0003H；B 的 'EF' 存 45H,46H，'CD' 存 43H,44H；C 为 03H,03H；SUM=80906050H 小端存 50H,60H,90H,80H。",
    ]:
        story.append(p(text, st["body"]))
    story.append(p("最小值程序：", st["h2"]))
    story.append(Preformatted(CODE_2017_MIN.strip(), st["code"]))
    story.append(p("存储器综合：128Kx8 / 16Kx1 = 64 片；每组 8 片组成 16Kx8，共 8 组。片内地址线 14 根；8 组选 3 根地址线接 74LS138。若只译 A16-A14 而高位 A19-A17 未参与全译码，会有地址覆盖；第一组在 A16-A14=000 时包含 00000H-03FFFH、20000H-23FFFH、40000H-43FFFH、60000H-63FFFH、80000H-83FFFH、A0000H-A3FFFH、C0000H-C3FFFH、E0000H-E3FFFH。", st["body"]))
    story.append(MemoryAnswerDiagram(
        "16Kx8",
        "A0-A13 -> 片内地址",
        "A14-A16 -> 74LS138",
        [
            "00000H-03FFFH",
            "20000H-23FFFH",
            "40000H-43FFFH",
            "60000H-63FFFH",
            "80000H-83FFFH",
            "A0000H-A3FFFH",
            "C0000H-C3FFFH",
            "E0000H-E3FFFH",
        ],
    ))
    story.append(p("8255 查询输入程序：", st["h2"]))
    story.append(Preformatted(CODE_2017_8255_INPUT.strip(), st["code"]))
    story.append(p("秒表完整参考程序：", st["h2"]))
    story.append(SevenSegAnswerDiagram(pc0_active="0"))
    story.append(Preformatted(CODE_2017_STOPWATCH.strip(), st["code"]))

    story.append(PageBreak())
    story.append(p("2018-2019", st["h1"]))
    for text in [
        "填空：1 00001101B、10101011B；2 非屏蔽中断、IF；3 数据总线、地址总线、控制总线；4 ISR 入口地址、256、18H；5 指令周期、4、T 状态；6 [BP+SI] 基址变址默认 SS，PA=40E00H+10F0H+39F1H=459E1H；[100H] 直接寻址默认 DS，PA=398A0H+0100H=399A0H；7 AND BX,0FF6FH；XOR BYTE PTR [2000H],0FH；8 BIU、EU、指令；9 无条件、查询、中断、DMA。",
        "选择题：1 D；2 B；3 C；4 C；5 C；6 D；7 B；8 A；9 D；10 B；11 D；12 C；13 D；14 B；15 B,F。",
        "SUB 93H-4DH=46H，CF=0；93H 是负数、4DH 是正数，负数减正数得到正数，发生有符号溢出，OF=1。",
        "程序读程：MOV WORD PTR [BX],2024H 后 DS:1800H 低字节 24H、高字节 20H；MOV SP,1150H 后栈顶 SS:1150H；XCHG AX,[BX] 后 AX=2024H，内存字=123DH；PUSH/POP 后 AX=123DH。",
        "数据段图：A=20H；B DW 'AB' 小端按字存 41H,42H；C 为 02H,02H,02H；SUM=12340320H 小端存 20H,03H,34H,12H。",
    ]:
        story.append(p(text, st["body"]))
    story.append(p("删除字符 o 完整程序：", st["h2"]))
    story.append(Preformatted(CODE_2018_STRING.strip(), st["code"]))
    story.append(p("存储器综合：256Kx8 / 32Kx1 = 64 片；每组 8 片组成 32Kx8，共 8 组。片内地址线 15 根；片选 3 根。若仅用 A17-A15 译码而高位 A19-A18 未参与，会地址覆盖；第一组地址为 00000H-07FFFH、40000H-47FFFH、80000H-87FFFH、C0000H-C7FFFH。", st["body"]))
    story.append(MemoryAnswerDiagram(
        "32Kx8",
        "A0-A14 -> 片内地址",
        "A15-A17 -> 74LS138",
        [
            "00000H-07FFFH",
            "40000H-47FFFH",
            "80000H-87FFFH",
            "C0000H-C7FFFH",
        ],
    ))
    story.append(p("端口控制程序：", st["h2"]))
    story.append(Preformatted(CODE_2018_DEVICE.strip(), st["code"]))
    story.append(p("玩具计数完整参考程序：", st["h2"]))
    story.append(SevenSegAnswerDiagram(pc0_active="1"))
    story.append(Preformatted(CODE_2018_COUNTER.strip(), st["code"]))
    doc(ANSWER_PDF, "答案解析").build(story, onFirstPage=footer, onLaterPages=footer)


def build_summary():
    st = make_styles()
    story = [p("北京化工大学微机原理 2017-2019 知识点总结", st["title"])]
    sections = [
        ("8086 结构", "BIU 负责总线访问、取指和形成物理地址；EU 负责执行。物理地址=段地址*10H+偏移。8086 20 根地址线、1MB 空间、16 位数据总线。"),
        ("补码与标志", "n 位补码范围 -2^(n-1) 到 2^(n-1)-1。CF 看无符号进/借位，OF 看有符号溢出。"),
        ("寻址", "含 BP 默认 SS；BX/SI/DI 默认 DS。直接、寄存器间接、基址变址等要会判断并计算物理地址。"),
        ("小端和堆栈", "低地址存低字节。PUSH 先 SP-=2 再写；POP 先读再 SP+=2。"),
        ("中断", "向量表 00000H-003FFH，每向量 4 字节，地址=4*n，低字 IP、高字 CS。INTR 可屏蔽，NMI 不受 IF 控制。"),
        ("存储器扩展", "先按位宽分组，再按容量分组。片内地址线由单片深度决定，片选线由组数决定；未参与译码的高位会造成地址覆盖。"),
        ("8255", "方式控制字 D7=1；D6-D5 A 组方式，D4 PA 方向，D3 PC 高 4 位方向，D2 B 组方式，D1 PB 方向，D0 PC 低 4 位方向。方式 0 全输出常为 80H；PA 输入、PC3 状态输入常见为 91H。"),
        ("8259", "初始化顺序 ICW1、ICW2、ICW3、ICW4，再写 OCW1 屏蔽字。ISR 结束发 EOI: MOV AL,20H / OUT command,AL。类型号=基址+IRQ。"),
    ]
    for title, body in sections:
        story.append(p(title, st["h1"]))
        story.append(p(body, st["body"]))
    story.append(p("8255 初始化模板", st["h1"]))
    story.append(Preformatted("""
; control = 80H + GA_mode*20H + PA_in*10H + PCU_in*08H
;         + GB_mode*04H + PB_in*02H + PCL_in
MOV DX, P8255_CTRL
MOV AL, 80H        ; PA/PB/PC 全输出，方式 0
OUT DX, AL
MOV AL, 91H        ; PA 输入，PC 高位输入，其余输出，方式 0
OUT DX, AL
""".strip(), st["code"]))
    story.append(p("8259 初始化模板", st["h1"]))
    story.append(Preformatted("""
PIC_CMD  EQU 20H
PIC_DATA EQU 21H
MOV AL,11H       ; ICW1: edge, cascade, ICW4 needed
OUT PIC_CMD,AL
MOV AL,08H       ; ICW2: vector base
OUT PIC_DATA,AL
MOV AL,04H       ; ICW3: slave on IR2, example
OUT PIC_DATA,AL
MOV AL,01H       ; ICW4: 8086 mode
OUT PIC_DATA,AL
MOV AL,0F7H      ; OCW1: unmask IR3
OUT PIC_DATA,AL
STI
; ISR end
MOV AL,20H
OUT PIC_CMD,AL
IRET
""".strip(), st["code"]))
    doc(SUMMARY_PDF, "知识点总结").build(story, onFirstPage=footer, onLaterPages=footer)


def main():
    register_fonts()
    build_questions()
    build_answers()
    build_summary()
    print(QUESTION_PDF)
    print(ANSWER_PDF)
    print(SUMMARY_PDF)


if __name__ == "__main__":
    main()
