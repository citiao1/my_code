from pathlib import Path

from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER, TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.pdfgen import canvas
from reportlab.platypus import (
    Flowable,
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
ANSWER_PDF = str(ROOT / "微机原理期末题_答案解析.pdf")
SUMMARY_PDF = str(ROOT / "微机原理期末题_知识点总结.pdf")

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


def make_styles():
    base = getSampleStyleSheet()
    return {
        "title": ParagraphStyle(
            "title",
            parent=base["Title"],
            fontName=FONT_BOLD,
            fontSize=22,
            leading=30,
            alignment=TA_CENTER,
            spaceAfter=12,
        ),
        "h1": ParagraphStyle(
            "h1",
            parent=base["Heading1"],
            fontName=FONT_BOLD,
            fontSize=16,
            leading=22,
            spaceBefore=10,
            spaceAfter=7,
            textColor=colors.HexColor("#143642"),
        ),
        "h2": ParagraphStyle(
            "h2",
            parent=base["Heading2"],
            fontName=FONT_BOLD,
            fontSize=13,
            leading=18,
            spaceBefore=8,
            spaceAfter=5,
            textColor=colors.HexColor("#255957"),
        ),
        "body": ParagraphStyle(
            "body",
            parent=base["BodyText"],
            fontName=FONT_REG,
            fontSize=10.5,
            leading=17,
            alignment=TA_LEFT,
            spaceAfter=4,
        ),
        "small": ParagraphStyle(
            "small",
            parent=base["BodyText"],
            fontName=FONT_REG,
            fontSize=9,
            leading=14,
            spaceAfter=3,
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
        "center": ParagraphStyle(
            "center",
            parent=base["BodyText"],
            fontName=FONT_REG,
            fontSize=10.5,
            leading=16,
            alignment=TA_CENTER,
            spaceAfter=4,
        ),
    }


def p(text, style):
    return Paragraph(text, style)


def bullets(items, styles):
    return ListFlowable(
        [
            ListItem(Paragraph(item, styles["body"]), leftIndent=12)
            for item in items
        ],
        bulletType="bullet",
        start="circle",
        leftIndent=16,
        bulletFontName=FONT_REG,
        bulletFontSize=7,
    )


class MathBlock(Flowable):
    def __init__(self, kind, data, width=420, height=34):
        super().__init__()
        self.kind = kind
        self.data = data
        self.width = width
        self.height = height

    def draw(self):
        c = self.canv
        c.saveState()
        c.setStrokeColor(colors.HexColor("#34545c"))
        c.setFillColor(colors.HexColor("#263238"))
        if self.kind == "frac":
            left, numerator, denominator, right = self.data
            x = 8
            y = self.height / 2 - 2
            c.setFont(FONT_REG, 10.5)
            c.drawString(x, y, left)
            x += c.stringWidth(left, FONT_REG, 10.5) + 8
            num_w = c.stringWidth(numerator, FONT_REG, 10.5)
            den_w = c.stringWidth(denominator, FONT_REG, 10.5)
            bar_w = max(num_w, den_w) + 10
            c.drawCentredString(x + bar_w / 2, y + 9, numerator)
            c.line(x, y + 6, x + bar_w, y + 6)
            c.drawCentredString(x + bar_w / 2, y - 7, denominator)
            c.drawString(x + bar_w + 8, y, right)
        elif self.kind == "radical":
            prefix, inside, suffix = self.data
            x = 8
            y = self.height / 2 - 3
            c.setFont(FONT_REG, 10.5)
            c.drawString(x, y, prefix)
            x += c.stringWidth(prefix, FONT_REG, 10.5) + 6
            c.setFont(FONT_REG, 17)
            c.drawString(x, y - 4, "√")
            x += 13
            w = c.stringWidth(inside, FONT_REG, 10.5) + 5
            c.line(x - 1, y + 12, x + w, y + 12)
            c.setFont(FONT_REG, 10.5)
            c.drawString(x + 2, y, inside)
            c.drawString(x + w + 7, y, suffix)
        elif self.kind == "subsup":
            base, sub, sup, suffix = self.data
            x = 8
            y = self.height / 2 - 2
            c.setFont(FONT_REG, 11)
            c.drawString(x, y, base)
            x += c.stringWidth(base, FONT_REG, 11) + 1
            if sup:
                c.setFont(FONT_REG, 7.5)
                c.drawString(x, y + 8, sup)
            if sub:
                c.setFont(FONT_REG, 7.5)
                c.drawString(x, y - 6, sub)
            x += max(c.stringWidth(sub or "", FONT_REG, 7.5), c.stringWidth(sup or "", FONT_REG, 7.5)) + 7
            c.setFont(FONT_REG, 11)
            c.drawString(x, y, suffix)
        c.restoreState()


class MemoryDiagram(Flowable):
    def __init__(self):
        super().__init__()
        self.width = 470
        self.height = 255

    def draw(self):
        c = self.canv
        c.saveState()
        c.setFont(FONT_BOLD, 10)
        c.setStrokeColor(colors.HexColor("#264653"))
        c.setFillColor(colors.HexColor("#f6fbfa"))
        c.roundRect(10, 84, 92, 106, 5, fill=1)
        c.setFillColor(colors.HexColor("#263238"))
        c.drawCentredString(56, 174, "CPU")
        for label, y in [
            ("A0-A16", 156),
            ("A17-A18", 134),
            ("D0-D7", 112),
            ("RD, WR", 90),
        ]:
            c.setFont(FONT_REG, 8.5)
            c.drawCentredString(56, y, label)
        label_data = [
            ("片内地址线接所有芯片 A0-A16", 162),
            ("A17-A18 经 2-4 译码器产生 CS0-CS3", 140),
            ("数据线按位连接到每组 8 片芯片", 118),
            ("读写控制线并联到各芯片 OE/WE", 96),
        ]
        for y in [162, 140, 118, 96]:
            c.line(102, y, 430, y)
            c.line(424, y + 3, 430, y)
            c.line(424, y - 3, 430, y)
        c.setFillColor(colors.HexColor("#fffaf2"))
        c.roundRect(270, 194, 100, 36, 5, fill=1)
        c.setFillColor(colors.HexColor("#263238"))
        c.setFont(FONT_BOLD, 8.5)
        c.drawCentredString(320, 216, "2-4 译码器")
        c.setFont(FONT_REG, 7.5)
        c.drawCentredString(320, 204, "输入 A17,A18; 输出 CS0-CS3")
        c.line(215, 140, 270, 212)
        c.line(370, 212, 420, 212)
        colors_by_group = ["#e8f3ff", "#edf8ea", "#fff0f0", "#f5eefb"]
        for g in range(4):
            x = 120 + g * 82
            y = 12
            c.setFillColor(colors.HexColor(colors_by_group[g]))
            c.roundRect(x, y, 72, 50, 4, fill=1)
            c.setFillColor(colors.HexColor("#263238"))
            c.setFont(FONT_BOLD, 7.5)
            c.drawCentredString(x + 36, y + 38, f"第{g + 1}组")
            c.setFont(FONT_REG, 6.5)
            c.drawCentredString(x + 36, y + 27, "8片 128K×1")
            c.drawCentredString(x + 36, y + 17, f"CS{g}")
            c.drawCentredString(x + 36, y + 8, "D0-D7 各接一片")
            c.line(320, 194, x + 36, y + 58)
        c.setFont(FONT_REG, 8)
        for text, y in label_data:
            c.setFillColor(colors.white)
            c.rect(106, y - 5, c.stringWidth(text, FONT_REG, 8) + 8, 11, fill=1, stroke=0)
            c.setFillColor(colors.HexColor("#263238"))
            c.drawString(110, y - 3, text)
        c.restoreState()


def make_table(data, col_widths, header=True):
    table = Table(data, colWidths=col_widths, repeatRows=1 if header else 0)
    style = [
        ("FONTNAME", (0, 0), (-1, -1), FONT_REG),
        ("FONTSIZE", (0, 0), (-1, -1), 8.8),
        ("LEADING", (0, 0), (-1, -1), 12),
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("GRID", (0, 0), (-1, -1), 0.35, colors.HexColor("#a7b5b5")),
        ("LEFTPADDING", (0, 0), (-1, -1), 5),
        ("RIGHTPADDING", (0, 0), (-1, -1), 5),
        ("TOPPADDING", (0, 0), (-1, -1), 4),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 4),
    ]
    if header:
        style += [
            ("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#dfeeed")),
            ("FONTNAME", (0, 0), (-1, 0), FONT_BOLD),
        ]
    table.setStyle(TableStyle(style))
    return table


def code_block(text, styles):
    return Preformatted(text.rstrip(), styles["code"])


def page_footer(c, doc):
    c.saveState()
    c.setFont(FONT_REG, 8)
    c.setFillColor(colors.HexColor("#667276"))
    c.drawCentredString(A4[0] / 2, 10 * mm, f"{doc.title}  ·  第 {doc.page} 页")
    c.restoreState()


def build_doc(path, title, story):
    doc = SimpleDocTemplate(
        path,
        pagesize=A4,
        rightMargin=18 * mm,
        leftMargin=18 * mm,
        topMargin=17 * mm,
        bottomMargin=17 * mm,
        title=title,
    )
    doc.build(story, onFirstPage=page_footer, onLaterPages=page_footer)


def answer_story(styles):
    story = [p("微机原理期末题 答案与详细解析", styles["title"])]
    story += [
        p("说明：本答案按题目顺序给出。涉及公式的位置使用上下结构分式、右上角/右下角角标和完整根号绘制；汇编题给出编程思路与逐行注释。", styles["body"]),
        p("一、填空题", styles["h1"]),
    ]
    fill_rows = [
        ["题号", "答案", "解析"],
        ["1", "10110100B；01100100B；10000000B", "8 位补码：正数直接写原码，负数用 2<super>8</super>-|x|。-76: 256-76=180=B4H=10110100B；100 为 01100100B；-128 为 10000000B。"],
        ["2", "字；先进后出；减 2", "8086 栈以字为单位操作，遵循 LIFO。PUSH 时 SP 先减 2，再写入 SS:SP。"],
        ["3", "Cache；主存；辅存", "层次化存储利用局部性原理，在速度、容量、成本之间折中。"],
        ["4", "CS；IP", "取指时逻辑地址为 CS:IP，物理地址由段基址加偏移得到。"],
        ["5", "158DEH；103FH；54EEH", "物理地址 = 103FH×10H+54EEH=103F0H+54EEH=158DEH。"],
        ["6", "总线周期；4；指令周期", "8086/8088 的基本总线周期通常由 T1-T4 四个时钟周期组成。"],
        ["7(1)", "寄存器相对寻址；1A80AH", "源操作数 [SI+1F00H] 默认 DS，偏移=590AH+1F00H=780AH，物理地址=13000H+780AH=1A80AH。注意这里加的是 780AH，不是 0780H。"],
        ["7(2)", "题目指令非法；若只算目的地址为 2CCFEH", "MOV BYTE PTR [BP],1234H 中，BYTE PTR 指明目的操作数为字节，而 1234H 是 16 位立即数，8086 汇编会报类型不匹配。若改成 WORD PTR [BP] 或把立即数改成 8 位数，目的地址按 SS:BP 计算，为 2BB00H+11FEH=2CCFEH。"],
        ["8", "字位同时扩展；16 片", "容量从 16K 到 128K 需 8 倍字扩展；位宽从 4 到 8 需 2 倍位扩展，共 8×2=16 片。"],
        ["9", "38", "压缩 BCD 每 4 位表示一位十进制数，0011 为 3，1000 为 8。"],
    ]
    story.append(make_table([[p(c, styles["small"]) for c in row] for row in fill_rows], [22 * mm, 42 * mm, 104 * mm]))
    story += [
        Spacer(1, 6),
        p("常用地址公式：", styles["body"]),
        MathBlock("subsup", ("物理地址", "20位", "", " = 段地址 × 10H + 偏移地址"), height=28),
        p("二、选择题", styles["h1"]),
    ]
    choice_rows = [
        ["1", "C", "机器中带符号整数通常采用补码，便于加减统一。"],
        ["2", "D", "IP 由取指流程自动维护，编程人员不能直接用 MOV 等指令读写。"],
        ["3", "B", "IF=0 屏蔽可屏蔽中断 INTR；NMI、内部中断、INT n 不受 IF 屏蔽。"],
        ["4", "A", "16K=2<super>14</super>，需要 14 根地址线。"],
        ["5", "C", "1 ms 周期性采集适合中断方式，既及时又不必 CPU 一直查询。"],
        ["6", "C", "计算机与外设之间通常通过设备总线/接口总线传送信息。"],
        ["7", "B", "T1 状态先输出地址信息，随后复用线才传数据。"],
        ["8", "B", "标号和变量常见属性有段属性、偏移属性、类型属性，不含“地址属性”这一项。"],
        ["9", "D", "DMA 方式由 DMA 控制器 DMAC 接管总线完成数据传送。"],
        ["10", "B", "完整中断过程：中断请求、中断响应、中断服务、中断返回。"],
    ]
    story.append(make_table([[p(c, styles["small"]) for c in row] for row in [["题号", "答案", "解析"]] + choice_rows], [18 * mm, 18 * mm, 132 * mm]))
    story += [p("三、分析题", styles["h1"]), p("1. 指令执行结果", styles["h2"])]
    analysis1 = [
        ["指令", "结果", "解析"],
        ["ADD AL,[BX]", "(AX)=13CEH，OF=0", "DS:BX = 10AAH:5DE0H，物理地址=16880H，内存字节 D0H。AL=FEH+D0H=CEH，有进位但两个负数相加结果仍为负，OF=0。"],
        ["MOV AX,BX", "AL=E0H", "AX 变为 5DE0H，因此低 8 位 AL=E0H。"],
        ["LEA DX,[BX]", "DX=5DE0H", "LEA 只取有效地址，不访问内存，故 DX=BX。"],
        ["SHL BL,CL", "AL=FEH，CF=1", "CL=02H，BL=E0H=11100000B，左移两位得到 80H，最后移出的位为 1。该指令不改 AL，因此 AL 仍为 FEH。"],
    ]
    story.append(make_table([[p(c, styles["small"]) for c in row] for row in analysis1], [34 * mm, 40 * mm, 94 * mm]))
    story += [
        p("2. 程序执行结果与功能", styles["h2"]),
        p("循环把 BUF 中 1 到 10 每个数乘 2 后累加，结果为 2×(1+2+...+10)=110=006EH。随后用 LEN=10 除该和，商为 11，余数为 0。DIV CL 后 AL=0BH、AH=00H，但程序为了使用 DOS 6 号调用执行 MOV AH,6，这一步把 AH 中的余数 00H 覆盖成 06H。于是第一次输出 DL=0BH+30H=3BH，即分号；第二次执行 MOV DL,AH 得到 06H，再加 30H 变成 36H，输出字符 '6'。所以实际屏幕输出为 ;6，而不是 ;0。", styles["body"]),
    ]
    story.append(MathBlock("frac", ("数学上商 = ", "2×(1+2+...+10)", "10", " = 11，余数 = 0；程序实际输出 ;6"), height=42))
    prog_rows = [
        ["项目", "结果"],
        ["(BX)", "006EH"],
        ["(CX)", "0000H（LOOP 结束后 CX=0，随后 MOV CL,LEN 使 CL=0AH；DIV 后 CX 不变，为 000AH。若填最终寄存器值，应写 000AH）"],
        ["LEN", "000AH"],
        ["(AX)", "060BH（DIV 后为 000BH；随后 MOV AH,6 覆盖余数，最终 AH=06H、AL=0BH）"],
        ["程序功能", "设计意图是求数组 BUF 中各元素乘 2 后的平均值及余数并输出；实际程序存在覆盖 AH 的缺陷，屏幕输出为 ;6。"],
    ]
    story.append(make_table([[p(c, styles["small"]) for c in row] for row in prog_rows], [34 * mm, 134 * mm], header=False))
    story += [
        p("3. 编程：找 6 个字数据中的最小值及其偏移地址", styles["h2"]),
        p("编程思路：数据是字数据，逐个用有符号比较更合理，因为 0FFFDH 表示 -3。先把第一个数设为当前最小值和地址，然后从第二个字开始循环 5 次；若发现更小值，就同时更新 MIN 和 MINADDR。", styles["body"]),
        code_block(
            """
DATA SEGMENT
    ORG 1000H
ARY     DW 102AH,3840H,0FFFDH,137EH,0,835DH
MIN     DW ?
MINADDR DW ?
DATA ENDS

CODE SEGMENT
ASSUME CS:CODE,DS:DATA
START:
    MOV AX,DATA          ; 初始化数据段寄存器 DS
    MOV DS,AX

    LEA SI,ARY           ; SI 指向当前被检查的数据
    MOV AX,[SI]          ; 先假定第 1 个字是最小值
    MOV MIN,AX
    MOV MINADDR,SI       ; 保存最小值所在偏移地址

    ADD SI,2             ; 转到第 2 个字
    MOV CX,5             ; 剩余 5 个字待比较
NEXT:
    MOV AX,[SI]          ; 取当前字数据
    CMP AX,MIN           ; 按有符号数比较当前值和最小值
    JGE SKIP             ; 当前值 >= MIN，则不更新
    MOV MIN,AX           ; 当前值更小，更新最小值
    MOV MINADDR,SI       ; 同步保存其偏移地址
SKIP:
    ADD SI,2             ; 指向下一个字
    LOOP NEXT

    MOV AH,4CH
    INT 21H
CODE ENDS
END START
            """,
            styles,
        ),
        p("执行结果：最小值为 0FFFDH（有符号数为 -3），最小值偏移地址为 1004H。", styles["body"]),
        PageBreak(),
        p("四、综合题", styles["h1"]),
        p("1. 128K×1 RAM 组成 512K×8 存储器", styles["h2"]),
    ]
    story += [
        p("（1）容量与地址线：512K×8 需要 8 位数据宽度，128K×1 每片只提供 1 位，因此每组要 8 片并联扩展位宽；512K/128K=4 组，共需 4×8=32 片。片内容量 128K=2<super>17</super>，故片内地址线需 17 根；4 组片选至少需要 2 根高位地址线进入 2-4 译码器。", styles["body"]),
        p("128K = 2<super>17</super>，因此片内地址线为 A0-A16。", styles["body"]),
        MathBlock("frac", ("芯片片数 = ", "512K×8", "128K×1", " = 32 片"), height=40),
        p("（2）连接示意图：", styles["body"]),
        MemoryDiagram(),
        p("（3）若用 A17、A18 作片选，A0-A16 作片内地址，而 CPU 有 20 根地址线，则 A19 未参与译码，存在地址覆盖。第一组芯片在 A18A17=00 时被选中，A19 可为 0 或 1，因此第一组有两个地址范围：00000H-1FFFFH 和 80000H-9FFFFH。", styles["body"]),
        p("2. 8259 的 IR2 中断服务程序与中断向量设置", styles["h2"]),
        p("IR0 类型号为 60H，所以 IR2 类型号为 62H。普通 EOI 需要向 8259 命令端口 20H 输出 20H。显示字符串建议以 '$' 结束，题面给出的 STRING DB 'IR2 Interrupt' 应补为 STRING DB 'IR2 Interrupt$'。", styles["body"]),
        code_block(
            """
DATA SEGMENT
STRING DB 'IR2 Interrupt$'
DATA ENDS

CODE SEGMENT
ASSUME CS:CODE,DS:DATA

IR2PROC PROC FAR
    PUSH AX              ; 保护中断服务程序会用到的寄存器
    PUSH DX
    PUSH DS

    MOV AX,DATA          ; DOS 9 号调用要求 DS:DX 指向字符串
    MOV DS,AX
    LEA DX,STRING
    MOV AH,09H
    INT 21H              ; 显示 IR2 Interrupt

    MOV AL,20H
    OUT 20H,AL           ; 普通 EOI，通知 8259 中断服务结束

    POP DS               ; 恢复现场
    POP DX
    POP AX
    IRET                 ; 中断返回
IR2PROC ENDP

START:
    MOV AX,DATA
    MOV DS,AX

    PUSH DS              ; 设置中断向量时 DS 要临时指向服务程序段
    MOV AX,SEG IR2PROC
    MOV DS,AX
    MOV DX,OFFSET IR2PROC
    MOV AH,25H
    MOV AL,62H           ; IR2 对应中断类型号 60H+2
    INT 21H              ; 写入中断向量表
    POP DS

    ; 如需开放 IR2，还应对 8259 屏蔽寄存器清 IR2 对应位。
    MOV AH,4CH
    INT 21H
CODE ENDS
END START
            """,
            styles,
        ),
        p("3. 8255 连接 ADC0809 程序填空", styles["h2"]),
        p("根据题意，8255 端口地址为 A 口 180H、B 口 181H、C 口 182H、控制口 183H。原题文字写“B 口方式 1 输出，C 口方式 1 输出”，但 8255 的 C 口没有独立的“方式 1 输出”；且下面程序是普通查询流程，没有方式 1 握手协议。因此应按代码注释和硬件需求理解为方式 0：A 口输入、B 口输出、PC2 输入。对应方式控制字为 91H。硬件连接图标的是 IN4，虽然程序注释写 IN3 且预填了 0BH，但应以连接图为准。IN4 的通道地址为 ADDC ADDB ADDA=100B，PB3=ALE，所以锁存通道时 PB3=1、PB2PB1PB0=100，写入 0CH；启动时 PB4=1，写入 1CH；撤销 START 后回到 0CH；检测 EOC 在 PC2，测试掩码为 04H。", styles["body"]),
        code_block(
            """
START:  MOV  AL,91H       ; 方式0：A口输入，B口输出，PC2所在的C口低4位输入
        MOV  DX,183H      ; 8255 控制字端口地址
        OUT  DX,AL        ; 送 8255 方式控制字

        MOV  AL,0CH       ; IN4: ADDC ADDB ADDA=100，且 PB3(ALE)=1 锁存地址
        MOV  DX,181H      ; 8255 的 B 口地址
        OUT  DX,AL        ; 送 IN4 通道地址并锁存

        MOV  AL,1CH       ; PB4=1，产生 START=1，启动 A/D 转换
        OUT  DX,AL
        MOV  CX,1000H
DELAY:  LOOP DELAY        ; 延时，保证启动脉冲宽度

        MOV  AL,0CH       ; PB4=0，撤销 START 信号，保持 IN4 地址
        OUT  DX,AL

        MOV  DX,182H      ; 8255 的 C 口地址
TEST1:  IN   AL,DX        ; 读 C 口状态
        TEST AL,04H       ; 检测 EOC，即 PC2
        JZ   TEST1        ; EOC 未有效则继续等待

        MOV  DX,180H      ; 8255 的 A 口地址
        IN   AL,DX        ; 读取 ADC0809 转换结果
        MOV  [1000H],AL   ; 保存到偏移地址 1000H 的内存单元
            """,
            styles,
        ),
        p("注：ADC0809 通常在转换期间 EOC=0、转换结束 EOC=1，因此 TEST AL,04H 后用 JZ TEST1 等待是合理的。若某实验箱另行规定 EOC 低有效，则需把 JZ TEST1 改为 JNZ TEST1。", styles["body"]),
    ]
    return story


def summary_story(styles):
    story = [p("微机原理期末题 相关知识点总结", styles["title"])]
    story += [
        p("本总结围绕本套题涉及的补码、8086 地址形成、寻址方式、指令执行、存储器扩展、中断、8255/ADC 接口和常用 DOS 调用整理。", styles["body"]),
        p("1. 数制、补码与 BCD", styles["h1"]),
        bullets(
            [
                "n 位无符号数范围为 0 到 2<super>n</super>-1；n 位补码范围为 -2<super>n-1</super> 到 2<super>n-1</super>-1。",
                "正数补码等于原码；负数补码可用 2<super>n</super>-|x| 求得，也可按“取反加 1”得到。",
                "压缩 BCD 用 4 位表示 1 个十进制数位，例如 0011 1000B 表示十进制 38。",
            ],
            styles,
        ),
        p("n 位补码范围：最小值 = -2<super>n-1</super>，最大值 = 2<super>n-1</super>-1。", styles["body"]),
        p("2. 8086 逻辑地址与物理地址", styles["h1"]),
        bullets(
            [
                "8086 有 20 位地址总线，可寻址 1MB 空间；逻辑地址写作 段地址:偏移地址。",
                "物理地址由段地址左移 4 位后加偏移地址得到。",
                "取指令默认使用 CS:IP；普通数据多数默认 DS；以 BP 为基址的存储器操作默认 SS。",
            ],
            styles,
        ),
        MathBlock("subsup", ("PA", "20位", "", " = Segment × 10H + Offset"), height=30),
        p("3. 寻址方式速记", styles["h1"]),
    ]
    addr_rows = [
        ["寻址方式", "形式", "要点"],
        ["立即寻址", "MOV AX,1234H", "操作数在指令中。"],
        ["寄存器寻址", "MOV AX,BX", "操作数在寄存器中。"],
        ["直接寻址", "MOV AX,[2000H]", "偏移地址直接给出，默认 DS。"],
        ["寄存器间接", "MOV AX,[SI]", "偏移由 BX/BP/SI/DI 给出；BP 默认 SS。"],
        ["基址/变址/相对", "MOV AX,[BX+SI+10H]", "多个寄存器和位移相加形成有效地址 EA。"],
        ["取有效地址", "LEA DX,[BX+SI]", "只计算 EA，不访问内存。"],
    ]
    story.append(make_table([[p(c, styles["small"]) for c in row] for row in addr_rows], [34 * mm, 50 * mm, 84 * mm]))
    story += [
        p("4. 指令与标志位", styles["h1"]),
        bullets(
            [
                "ADD/SUB/CMP 会影响 CF、ZF、SF、OF 等标志；MOV、LEA 通常不影响标志。",
                "CF 反映无符号进位/借位；OF 反映带符号运算溢出。",
                "SHL/SAL 左移时低位补 0，最后移出的最高位进入 CF；移位次数由 1 或 CL 指定。",
                "DIV r/m8：AX 除以 8 位操作数，商入 AL，余数入 AH。",
            ],
            styles,
        ),
        p("5. 循环与条件转移", styles["h1"]),
        bullets(
            [
                "LOOP 标号：先 CX=CX-1，若 CX 不为 0 则转移。",
                "无符号比较常用 JA/JAE/JB/JBE；带符号比较常用 JG/JGE/JL/JLE。",
                "找最大/最小值时要先判断题目数据应按有符号还是无符号解释。",
            ],
            styles,
        ),
        p("6. 存储器扩展", styles["h1"]),
        bullets(
            [
                "位扩展：芯片容量深度相同、位宽不够时，多片并联接不同数据位。",
                "字扩展：总容量深度不够时，用高位地址线译码产生片选。",
                "字位同时扩展：容量深度和数据位宽都不够，需要同时分组和并联。",
                "若 CPU 地址线没有全部参与译码，会产生地址覆盖。",
            ],
            styles,
        ),
        MathBlock("frac", ("芯片总数 = ", "目标容量×目标位宽", "单片容量×单片位宽", ""), height=38),
        p("7. 中断与 8259", styles["h1"]),
        bullets(
            [
                "完整中断过程包括中断请求、中断响应、中断服务和中断返回。",
                "IF=0 只屏蔽 INTR 可屏蔽中断，不屏蔽 NMI、内部异常和 INT n 软件中断。",
                "8259 中 IRn 的类型号通常为基准类型号+n；普通 EOI 常向命令端口写 20H。",
                "中断服务程序要保护现场，结束时执行 IRET。",
            ],
            styles,
        ),
        p("8. 8255 与 ADC0809 接口", styles["h1"]),
        bullets(
            [
                "8255 常见地址连续分配：A 口、B 口、C 口、控制口依次递增。",
                "方式控制字最高位为 1；端口方向位决定输入/输出。若要从 PC2 读取 EOC，C 口低 4 位必须配置为输入。",
                "ADC0809 常用流程：按连接图确定 INn 通道号，送通道地址并用 ALE 锁存，给 START 脉冲，查询 EOC，读取转换结果。",
                "ADC0809 的通道选择通常按 ADDC ADDB ADDA 表示二进制通道号。例如 IN4 对应 100B；若 PB3 接 ALE，则锁存控制字低 4 位为 1100B，即 0CH。",
                "检测 EOC 时要注意实验系统规定的有效电平；常见 ADC0809 时序是转换中 EOC=0，转换结束 EOC=1。",
                "题目若同时写“方式 1 输出”和给出普通查询程序，要识别是否为笔误；8255 的 C 口在方式 1 下主要承担握手联络信号，不是独立的方式 1 输出口。",
            ],
            styles,
        ),
        p("9. DOS 常用调用", styles["h1"]),
    ]
    dos_rows = [
        ["调用", "入口参数", "功能"],
        ["INT 21H/AH=25H", "AL=类型号，DS:DX=中断服务程序入口", "设置中断向量。"],
        ["INT 21H/AH=06H", "输出时 DL=字符 ASCII，且 DL≠0FFH", "直接控制台输入/输出。"],
        ["INT 21H/AH=09H", "DS:DX 指向以 '$' 结束的字符串", "显示字符串。"],
        ["INT 21H/AH=4CH", "AL 可作返回码", "返回 DOS。"],
    ]
    story.append(make_table([[p(c, styles["small"]) for c in row] for row in dos_rows], [36 * mm, 72 * mm, 60 * mm]))
    story += [
        p("10. 答题检查清单", styles["h1"]),
        bullets(
            [
                "地址题先判断默认段寄存器，再算 EA，最后算物理地址。",
                "代码题先写数据段、代码段和 ASSUME，再初始化 DS。",
                "中断题不要忘记保护现场、发送 EOI、IRET 返回。",
                "接口题要写清端口地址、控制字、控制信号位和状态检测位。",
            ],
            styles,
        ),
        p("补充：常见公式展示", styles["h1"]),
        MathBlock("radical", ("若用容量位数表达地址线：地址线数 = ", "存储单元数", " 的二进制指数"), height=34),
        MathBlock("subsup", ("A", "0-16", "17根", " 接片内地址；A17-A18 作片选译码"), height=30),
    ]
    return story


def main():
    register_fonts()
    styles = make_styles()
    build_doc(ANSWER_PDF, "答案解析", answer_story(styles))
    build_doc(SUMMARY_PDF, "知识点总结", summary_story(styles))
    print(ANSWER_PDF)
    print(SUMMARY_PDF)


if __name__ == "__main__":
    main()
