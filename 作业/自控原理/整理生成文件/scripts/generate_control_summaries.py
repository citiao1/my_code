from __future__ import annotations

from pathlib import Path
import re
from typing import Iterable, Sequence

from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER, TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import mm
from reportlab.pdfbase.cidfonts import UnicodeCIDFont
from reportlab.pdfbase.pdfmetrics import registerFont, stringWidth
from reportlab.platypus import (
    Flowable,
    ListFlowable,
    ListItem,
    PageBreak,
    Paragraph,
    SimpleDocTemplate,
    Spacer,
    Table,
    TableStyle,
)


ROOT = Path(__file__).resolve().parents[2]
OUT_FORMULAS = ROOT / "自控原理_知识点与公式总结.pdf"
OUT_PROBLEMS = ROOT / "自控原理_大题题型与解法总结.pdf"

FONT = "STSong-Light"
FORMULA_FONT = "STSong-Light"
registerFont(UnicodeCIDFont(FONT))


FormulaToken = str | tuple[str, str, str]


class FormulaLine(Flowable):
    """Draw a single display formula with simple stacked fractions."""

    def __init__(
        self,
        tokens: Sequence[FormulaToken],
        font_size: int = 11,
        leading: int = 18,
        align: str = "center",
    ):
        super().__init__()
        self.tokens = tokens
        self.font_size = font_size
        self.leading = leading
        self.align = align
        self.width = 0
        self.height = leading

    def _parse_script_text(self, text: str) -> list[tuple[str, str]]:
        parts: list[tuple[str, str]] = []
        i = 0
        normal: list[str] = []
        while i < len(text):
            ch = text[i]
            if ch in "^_" and i + 1 < len(text):
                if normal:
                    parts.append(("normal", "".join(normal)))
                    normal = []
                kind = "super" if ch == "^" else "sub"
                i += 1
                if i < len(text) and text[i] == "(":
                    depth = 1
                    i += 1
                    start = i
                    while i < len(text) and depth:
                        if text[i] == "(":
                            depth += 1
                        elif text[i] == ")":
                            depth -= 1
                            if depth == 0:
                                break
                        i += 1
                    parts.append((kind, text[start:i]))
                    if i < len(text) and text[i] == ")":
                        i += 1
                else:
                    start = i
                    if i < len(text) and text[i].isalnum():
                        while i < len(text) and text[i].isalnum():
                            i += 1
                    else:
                        i += 1
                    parts.append((kind, text[start:i]))
                continue
            normal.append(ch)
            i += 1
        if normal:
            parts.append(("normal", "".join(normal)))
        return parts

    def _script_text_width(self, text: str, font_size: int | None = None) -> float:
        fs = font_size or self.font_size
        small = max(6, fs * 0.68)
        total = 0.0
        for kind, part in self._parse_script_text(text):
            size = small if kind in {"super", "sub"} else fs
            total += stringWidth(part, FORMULA_FONT, size)
        return total

    def _token_size(self, token: FormulaToken) -> tuple[float, float]:
        if isinstance(token, tuple) and token[0] == "frac":
            _, num, den = token
            w = max(
                self._script_text_width(num, self.font_size - 1),
                self._script_text_width(den, self.font_size - 1),
            ) + 8
            return w, self.leading
        if isinstance(token, tuple) and token[0] == "supfrac":
            _, num, den = token
            w = max(
                self._script_text_width(num, self.font_size - 3),
                self._script_text_width(den, self.font_size - 3),
            ) + 6
            return w, self.leading
        return self._script_text_width(str(token)), self.leading

    def wrap(self, availWidth: float, availHeight: float) -> tuple[float, float]:
        self.width = sum(self._token_size(t)[0] for t in self.tokens)
        self.height = self.leading
        return min(availWidth, self.width), self.height

    def draw(self) -> None:
        c = self.canv
        c.setFont(FORMULA_FONT, self.font_size)
        total = sum(self._token_size(t)[0] for t in self.tokens)
        if self.align == "center":
            x = max(0, (self._availWidth - total) / 2)
        else:
            x = 0
        baseline = self.leading * 0.39
        for token in self.tokens:
            if isinstance(token, tuple) and token[0] == "frac":
                _, num, den = token
                w, _ = self._token_size(token)
                num_w = self._script_text_width(num, self.font_size - 1)
                den_w = self._script_text_width(den, self.font_size - 1)
                self._draw_script_text(x + (w - num_w) / 2, baseline + 5.2, num, self.font_size - 1)
                c.setLineWidth(0.6)
                c.line(x + 2, baseline + 3.2, x + w - 2, baseline + 3.2)
                self._draw_script_text(x + (w - den_w) / 2, baseline - 6.4, den, self.font_size - 1)
                x += w
            elif isinstance(token, tuple) and token[0] == "supfrac":
                _, num, den = token
                w, _ = self._token_size(token)
                fs = self.font_size - 3
                num_w = self._script_text_width(num, fs)
                den_w = self._script_text_width(den, fs)
                top = baseline + 8.5
                self._draw_script_text(x + (w - num_w) / 2, top + 4.2, num, fs)
                c.setLineWidth(0.45)
                c.line(x + 2, top + 2.6, x + w - 2, top + 2.6)
                self._draw_script_text(x + (w - den_w) / 2, top - 5.4, den, fs)
                x += w
            else:
                text = str(token)
                self._draw_script_text(x, baseline, text, self.font_size)
                x += self._script_text_width(text, self.font_size)

    def _draw_script_text(self, x: float, baseline: float, text: str, font_size: int | float) -> None:
        c = self.canv
        small = max(6, font_size * 0.68)
        cursor = x
        for kind, part in self._parse_script_text(text):
            if kind == "normal":
                c.setFont(FORMULA_FONT, font_size)
                y = baseline
                size = font_size
            elif kind == "super":
                c.setFont(FORMULA_FONT, small)
                y = baseline + font_size * 0.42
                size = small
            else:
                c.setFont(FORMULA_FONT, small)
                y = baseline - font_size * 0.30
                size = small
            c.drawString(cursor, y, part)
            cursor += stringWidth(part, FORMULA_FONT, size)


class FormulaBox(Flowable):
    def __init__(
        self,
        lines: Sequence[Sequence[FormulaToken]],
        title: str | None = None,
        note: str | None = None,
        width: float = 170 * mm,
    ):
        super().__init__()
        self.lines = [FormulaLine(line) for line in lines]
        self.title = title
        self.note = note
        self.box_width = width
        self.height = 0

    def wrap(self, availWidth: float, availHeight: float) -> tuple[float, float]:
        self.box_width = min(self.box_width, availWidth)
        h = 10
        if self.title:
            h += 15
        h += len(self.lines) * 20
        if self.note:
            h += 18
        self.height = h
        return self.box_width, self.height

    def draw(self) -> None:
        c = self.canv
        c.saveState()
        c.setStrokeColor(colors.HexColor("#7fb3bd"))
        c.setFillColor(colors.HexColor("#f4fbfc"))
        c.roundRect(0, 0, self.box_width, self.height, 5, stroke=1, fill=1)
        y = self.height - 14
        if self.title:
            c.setFillColor(colors.HexColor("#155e68"))
            c.setFont(FONT, 10.5)
            c.drawString(8, y, self.title)
            y -= 16
        else:
            y -= 5
        for line in self.lines:
            line._availWidth = self.box_width - 16
            line.canv = c
            c.saveState()
            c.translate(8, y - 12)
            line.draw()
            c.restoreState()
            y -= 20
        if self.note:
            c.setFillColor(colors.HexColor("#555555"))
            c.setFont(FONT, 8.5)
            c.drawString(8, 8, self.note)
        c.restoreState()


def styles():
    ss = getSampleStyleSheet()
    return {
        "title": ParagraphStyle(
            "ChineseTitle",
            parent=ss["Title"],
            fontName=FONT,
            fontSize=21,
            leading=28,
            alignment=TA_CENTER,
            textColor=colors.HexColor("#12343b"),
            spaceAfter=10,
        ),
        "subtitle": ParagraphStyle(
            "Subtitle",
            parent=ss["Normal"],
            fontName=FONT,
            fontSize=10,
            leading=16,
            alignment=TA_CENTER,
            textColor=colors.HexColor("#555555"),
            spaceAfter=14,
        ),
        "h1": ParagraphStyle(
            "H1",
            parent=ss["Heading1"],
            fontName=FONT,
            fontSize=15,
            leading=20,
            textColor=colors.HexColor("#11636c"),
            spaceBefore=10,
            spaceAfter=8,
        ),
        "h2": ParagraphStyle(
            "H2",
            parent=ss["Heading2"],
            fontName=FONT,
            fontSize=12.5,
            leading=17,
            textColor=colors.HexColor("#20444a"),
            spaceBefore=8,
            spaceAfter=5,
        ),
        "body": ParagraphStyle(
            "Body",
            parent=ss["BodyText"],
            fontName=FONT,
            fontSize=9.5,
            leading=15,
            alignment=TA_LEFT,
            spaceAfter=5,
        ),
        "small": ParagraphStyle(
            "Small",
            parent=ss["BodyText"],
            fontName=FONT,
            fontSize=8.5,
            leading=13,
            textColor=colors.HexColor("#555555"),
        ),
        "bullet": ParagraphStyle(
            "Bullet",
            parent=ss["BodyText"],
            fontName=FONT,
            fontSize=9.2,
            leading=14,
            leftIndent=12,
            firstLineIndent=-8,
            spaceAfter=3,
        ),
    }


S = styles()


def p(text: str, style: str = "body") -> Paragraph:
    return Paragraph(rich_scripts(text), S[style])


def rich_scripts(text: str) -> str:
    text = text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")

    def repl_super(match: re.Match[str]) -> str:
        body = match.group(1) or match.group(2)
        return f"<super>{body}</super>"

    def repl_sub(match: re.Match[str]) -> str:
        body = match.group(1) or match.group(2)
        return f"<sub>{body}</sub>"

    text = re.sub(r"\^\(([^)]*)\)|\^([A-Za-z0-9+\-]+)", repl_super, text)
    text = re.sub(r"_\(([^)]*)\)|_([A-Za-z0-9]+)", repl_sub, text)
    return text


def bullets(items: Iterable[str]) -> ListFlowable:
    return ListFlowable(
        [ListItem(p(item, "body"), bulletColor=colors.HexColor("#11636c")) for item in items],
        bulletType="bullet",
        leftIndent=14,
        bulletFontName=FONT,
        bulletFontSize=7,
    )


def page_footer(canvas, doc):
    canvas.saveState()
    canvas.setFont(FONT, 8)
    canvas.setFillColor(colors.HexColor("#777777"))
    canvas.drawString(18 * mm, 12 * mm, "自动控制原理期末速成课整理")
    canvas.drawRightString(192 * mm, 12 * mm, f"第 {doc.page} 页")
    canvas.restoreState()


def table(rows: list[list[str]], col_widths: list[float]) -> Table:
    data = [[p(cell, "small") for cell in row] for row in rows]
    t = Table(data, colWidths=col_widths, repeatRows=1)
    t.setStyle(
        TableStyle(
            [
                ("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#e2f2f4")),
                ("TEXTCOLOR", (0, 0), (-1, 0), colors.HexColor("#12343b")),
                ("GRID", (0, 0), (-1, -1), 0.35, colors.HexColor("#b7c9cc")),
                ("VALIGN", (0, 0), (-1, -1), "TOP"),
                ("LEFTPADDING", (0, 0), (-1, -1), 5),
                ("RIGHTPADDING", (0, 0), (-1, -1), 5),
                ("TOPPADDING", (0, 0), (-1, -1), 4),
                ("BOTTOMPADDING", (0, 0), (-1, -1), 4),
            ]
        )
    )
    return t


def title_page(title: str, subtitle: str) -> list:
    return [
        Spacer(1, 40 * mm),
        p(title, "title"),
        p(subtitle, "subtitle"),
        FormulaBox(
            [["复习原则：先识别模型和输入，再选公式；先算特征方程，再谈稳定和性能。"]],
            note="整理对象：自控期末速成课-5.21.pdf；本讲义按考试知识点重组，不逐页复刻课件。",
        ),
        Spacer(1, 10 * mm),
        p("阅读提示：公式下方都给出物理含义或使用条件；涉及分式的核心公式采用上下分层排版，便于直接辨认分子和分母。", "body"),
        PageBreak(),
    ]


def build_formula_doc() -> list:
    story = title_page("自控原理知识点与公式总结", "概念、公式、适用条件和直观解释")

    story += [
        p("1. 自动控制系统基本概念", "h1"),
        p("控制系统的核心是比较“希望值”和“实际反馈值”，再利用控制器改变被控对象。闭环系统牺牲一部分结构复杂度，换来抗扰、减小误差和改善动态性能。", "body"),
        FormulaBox(
            [
                ["E(s) = R(s) - B(s)"],
                ["B(s) = H(s) C(s)"],
                ["误差 = 给定输入 - 反馈信号"],
            ],
            title="反馈比较点",
            note="H(s) 是反馈通道；单位反馈时 H(s)=1。",
        ),
        bullets(
            [
                "开环控制：输出不参与修正，结构简单，但抗扰和精度较差。",
                "闭环控制：输出经反馈参与修正，能减小偏差，但可能引入稳定性问题。",
                "稳定性、快速性、准确性是期末大题里最常被同时考察的三类性能。",
            ]
        ),
        p("常用输入信号及拉氏变换", "h2"),
        table(
            [
                ["时域输入", "拉氏变换", "直观含义"],
                ["单位阶跃 1(t)", "1 / s", "突然给一个常值目标，最常用于看稳态误差和过渡过程。"],
                ["单位斜坡 t", "1 / s^2", "目标匀速变化，用来考速度误差系数。"],
                ["单位抛物线 t^2 / 2", "1 / s^3", "目标匀加速变化，用来考加速度误差系数。"],
                ["e^(-at)", "1 / (s+a)", "指数衰减信号，极点在 -a。"],
                ["sin ωt", "ω / (s^2+ω^2)", "正弦输入，频域分析的基础。"],
            ],
            [36 * mm, 42 * mm, 86 * mm],
        ),
        p("2. 传递函数和闭环结构", "h1"),
        FormulaBox(
            [
                ["G(s) = ", ("frac", "C(s)", "R(s)")],
                ["Φ(s) = ", ("frac", "C(s)", "R(s)"), " = ", ("frac", "G(s)", "1 + G(s)H(s)")],
                ["Φ_e(s) = ", ("frac", "E(s)", "R(s)"), " = ", ("frac", "1", "1 + G(s)H(s)")],
            ],
            title="传递函数与负反馈闭环",
            note="传递函数只描述零初始条件下输入到输出的关系；闭环特征方程是 1+G(s)H(s)=0。",
        ),
        bullets(
            [
                "串联：总传递函数等于各环节相乘，G = G1 G2 ... Gn。",
                "并联：总传递函数等于各支路相加，G = G1 + G2 + ... + Gn。",
                "负反馈：前向通道在分子，1+开环传递函数在分母；正反馈把加号换成减号。",
                "方框图化简的底层目标只有一个：求出 C(s)/R(s)、C(s)/D(s) 或 E(s)/R(s)。",
            ]
        ),
        FormulaBox(
            [
                ["P = ", ("frac", "Σ P_k Δ_k", "Δ")],
                ["Δ = 1 - ΣL_i + ΣL_iL_j - ΣL_iL_jL_m + ..."],
            ],
            title="梅逊增益公式",
            note="P_k 是第 k 条前向通路增益；Δ_k 删除与第 k 条前向通路接触的回路后再求 Δ。",
        ),
        p("梅逊公式适合信号流图题。若图里有多个相互不接触回路，Δ 的正负号按“一个回路减、两个不接触回路加、三个不接触回路减”交替。", "body"),
        p("3. 时域分析", "h1"),
        FormulaBox(
            [
                ["G(s) = ", ("frac", "1", "Ts + 1")],
                ["c(t) = 1 - e^(-t/T)"],
                ["t_s(5%) ≈ 3T，t_s(2%) ≈ 4T"],
            ],
            title="一阶系统单位阶跃响应",
            note="T 是时间常数；T 越小，响应越快。",
        ),
        FormulaBox(
            [
                ["Φ(s) = ", ("frac", "ω_n^2", "s^2 + 2ζω_n s + ω_n^2")],
                ["M_p = e", ("supfrac", "-πζ", "√(1-ζ^2)"), " × 100%"],
                ["t_p = ", ("frac", "π", "ω_n√(1-ζ^2)")],
                ["t_s(5%) ≈ ", ("frac", "3", "ζω_n"), "，t_s(2%) ≈ ", ("frac", "4", "ζω_n")],
            ],
            title="标准二阶系统",
            note="ζ 控制振荡和超调；ω_n 控制时间尺度；ζω_n 是衰减速度。",
        ),
        bullets(
            [
                "0 < ζ < 1：欠阻尼，有振荡和超调；ζ 越小超调越大。",
                "ζ = 1：临界阻尼，不振荡且通常较快。",
                "ζ > 1：过阻尼，不振荡但响应慢。",
                "主导极点越靠近虚轴，系统响应越慢；越靠左，衰减越快。",
            ]
        ),
        p("劳斯稳定判据", "h2"),
        FormulaBox(
            [
                ["D(s)=a_n s^n+a_(n-1)s^(n-1)+...+a_0"],
                ["稳定条件：劳斯表第一列元素同号且无零"],
            ],
            title="不求根判断稳定",
            note="第一列变号次数等于右半平面特征根个数。",
        ),
        table(
            [
                ["特殊情况", "处理方式", "含义"],
                ["第一列首元素为 0", "用小正数 ε 替代，继续列劳斯表，再看符号变化。", "说明根分布处在临界情形附近。"],
                ["整行全 0", "由上一行构造辅助多项式 A(s)，求导后替换全零行。", "存在关于原点对称的根，常见于虚轴根。"],
            ],
            [36 * mm, 80 * mm, 48 * mm],
        ),
        p("稳态误差", "h2"),
        FormulaBox(
            [
                ["e_ss = lim_(t→∞) e(t) = lim_(s→0) sE(s)"],
                ["E(s) = ", ("frac", "R(s)", "1 + G(s)H(s)")],
                ["K_p = lim_(s→0) G(s)H(s)"],
                ["K_v = lim_(s→0) sG(s)H(s)"],
                ["K_a = lim_(s→0) s^2G(s)H(s)"],
            ],
            title="终值定理与误差系数",
            note="使用终值定理前，sE(s) 的极点必须都在左半平面或原点处满足条件。",
        ),
        FormulaBox(
            [
                ["阶跃输入：e_ss = ", ("frac", "1", "1 + K_p")],
                ["斜坡输入：e_ss = ", ("frac", "1", "K_v")],
                ["抛物线输入：e_ss = ", ("frac", "1", "K_a")],
            ],
            title="单位反馈常用结果",
            note="系统型别由开环 G(s)H(s) 中原点极点个数决定。",
        ),
        table(
            [
                ["系统型别", "阶跃误差", "斜坡误差", "抛物线误差"],
                ["0 型", "有限", "∞", "∞"],
                ["I 型", "0", "有限", "∞"],
                ["II 型", "0", "0", "有限"],
            ],
            [36 * mm, 42 * mm, 42 * mm, 42 * mm],
        ),
        p("4. 根轨迹分析", "h1"),
        FormulaBox(
            [
                ["1 + K G_0(s)H(s) = 0"],
                ["幅值条件：|G_0(s)H(s)| = ", ("frac", "1", "K")],
                ["相角条件：∠G_0(s)H(s) = (2q+1)π"],
            ],
            title="根轨迹基本条件",
            note="根轨迹描述 K 从 0 到 ∞ 时闭环极点的位置变化。",
        ),
        FormulaBox(
            [
                ["渐近线交点：σ_a = ", ("frac", "Σp_i - Σz_i", "n - m")],
                ["渐近线角：θ_q = ", ("frac", "(2q+1)π", "n - m"), "，q=0,1,..."],
                ["分离点：", ("frac", "dK", "ds"), " = 0"],
            ],
            title="绘制根轨迹的常用公式",
            note="n 是开环极点数，m 是开环零点数；实轴上右侧开环零极点总数为奇数的区间在根轨迹上。",
        ),
        bullets(
            [
                "起点：开环极点；终点：开环零点，若零点不足则终点在无穷远。",
                "根轨迹关于实轴对称。",
                "与虚轴交点通常用劳斯表求临界 K，再代回求交点频率。",
                "题目若让判断参数范围，先写特征方程，再用劳斯判据，比凭图判断稳。",
            ]
        ),
        p("5. 频域分析", "h1"),
        FormulaBox(
            [
                ["G(jω) = Re(ω) + j Im(ω)"],
                ["A(ω)=|G(jω)| = √(Re^2+Im^2)"],
                ["φ(ω)=∠G(jω)"],
                ["L(ω)=20 lg A(ω)"],
            ],
            title="频率特性",
            note="幅频看放大/衰减，相频看相位滞后或超前。",
        ),
        table(
            [
                ["典型环节", "幅值渐近特性", "相位特性"],
                ["比例 K", "20 lg K，水平线", "K>0 时 0°"],
                ["积分 1/s", "-20 dB/dec", "-90°"],
                ["微分 s", "+20 dB/dec", "+90°"],
                ["惯性 1/(Ts+1)", "ω=1/T 后斜率 -20 dB/dec", "0° 到 -90°"],
                ["一阶微分 Ts+1", "ω=1/T 后斜率 +20 dB/dec", "0° 到 +90°"],
                ["振荡环节", "转折频率附近可能出现谐振峰", "0° 到 -180°"],
            ],
            [38 * mm, 68 * mm, 58 * mm],
        ),
        FormulaBox(
            [
                ["相位裕度：γ = 180° + φ(ω_c)"],
                ["幅值裕度：h = ", ("frac", "1", "|G(jω_g)H(jω_g)|")],
                ["dB 表示：20 lg h"],
            ],
            title="相对稳定性指标",
            note="ω_c 为幅值穿越频率 |G(jω_c)H(jω_c)|=1；ω_g 为相位穿越频率 φ=-180°。",
        ),
        FormulaBox(
            [
                ["闭环右半平面根数：Z = P - N"],
                ["稳定要求：Z = 0"],
            ],
            title="奈奎斯特判据",
            note="P 是开环右半平面极点数；N 按顺时针包围 (-1,j0) 的次数计数。若教材采用反向计数，公式符号随约定改变，但稳定目标仍是 Z=0。",
        ),
        bullets(
            [
                "Bode 图大题先拆成典型环节，再叠加幅值斜率和相位。",
                "Nyquist 图大题先看开环右半平面极点数 P，再看曲线对 -1 点的包围。",
                "相位裕度和幅值裕度不是越大越好，太大常意味着响应变慢；考试通常只需判断是否满足稳定裕度。",
            ]
        ),
    ]
    return story


def problem_template(title: str, idea: str, formulas: list[Sequence[FormulaToken]], steps: list[str], notes: list[str]) -> list:
    return [
        p(title, "h1"),
        p(idea, "body"),
        FormulaBox(formulas, title="核心公式"),
        p("解题步骤", "h2"),
        bullets(steps),
        p("解释与易错点", "h2"),
        bullets(notes),
    ]


def build_problem_doc() -> list:
    story = title_page("自控原理大题题型与解法总结", "只整理计算/分析大题，不包含选择题和填空题")

    story += problem_template(
        "题型 1：由微分方程、物理系统或电路求传递函数",
        "这类题的关键不是背答案，而是把系统先写成代数方程。机械系统用力平衡，电路用阻抗法或节点方程，最后在零初始条件下取拉氏变换。",
        [
            ["G(s) = ", ("frac", "输出量的拉氏变换", "输入量的拉氏变换")],
            ["电容阻抗：Z_C = ", ("frac", "1", "Cs"), "，电感阻抗：Z_L = Ls"],
            ["质量-弹簧-阻尼：M s^2X + B sX + KX = F"],
        ],
        [
            "先确定输入量和输出量，例如输入电压/输出电压、外力/位移、转矩/角位移。",
            "把微分方程写完整，阻尼项对应一阶导数，弹簧项对应位移，质量或转动惯量对应二阶导数。",
            "零初始条件下进行拉氏变换，把微分变成 s 的乘法。",
            "把输出放左边、输入放右边，整理成输出/输入的分式。",
        ],
        [
            "传递函数不包含初始条件；题目给初始条件时通常是在考响应，不是在考传递函数。",
            "电路题用阻抗法时，电容是 1/(Cs)，不要写成 Cs。",
            "若输出是某个元件两端电压，先用分压或节点电压表达输出，不要直接把总电压当输出。",
        ],
    )

    story += problem_template(
        "题型 2：方框图化简与梅逊公式求闭环传递函数",
        "课件第二章多次强调这一类是大题。化简法适合结构清晰的串并联反馈；交叉反馈多、回路多时，先转信号流图并用梅逊公式更省力。",
        [
            ["负反馈：", ("frac", "C(s)", "R(s)"), " = ", ("frac", "G(s)", "1 + G(s)H(s)")],
            ["梅逊公式：P = ", ("frac", "ΣP_kΔ_k", "Δ")],
            ["Δ = 1 - ΣL_i + ΣL_iL_j - ..."],
        ],
        [
            "标出输入 R(s)、输出 C(s) 和所有比较点的正负号。",
            "能先合并的串联、并联先合并，反馈环从内到外逐层化简。",
            "若使用梅逊公式，列出所有前向通路 P_k、所有单独回路 L_i、互不接触回路组。",
            "计算 Δ 与每条通路对应的 Δ_k，代入梅逊公式。",
            "最后检查分母是否对应闭环特征方程，避免漏掉反馈符号。",
        ],
        [
            "负反馈分母是 1+GH，正反馈分母是 1-GH；许多错题都错在这个符号。",
            "梅逊公式里的“互不接触”指没有公共节点，不是图上看起来没有交叉。",
            "若题目问扰动 D(s) 到输出 C(s)，要令 R(s)=0 单独求通道；不能直接套 C/R。",
        ],
    )

    story += problem_template(
        "题型 3：劳斯判据判断稳定性和参数范围",
        "这类题通常给一个含 K 或其他参数的特征方程，要求判断稳定范围。目标是让劳斯表第一列全同号。",
        [
            ["D(s)=a_n s^n+a_(n-1)s^(n-1)+...+a_0"],
            ["稳定条件：劳斯表第一列 > 0"],
            ["变号次数 = 右半平面根个数"],
        ],
        [
            "先由闭环分母写出特征方程 D(s)=0。",
            "按 s 的降幂列劳斯表，前两行直接填系数，后续行按行列式规则计算。",
            "把第一列元素列成不等式组。",
            "解不等式得到参数范围。",
            "若出现临界值，可代回特征方程判断是否有虚轴根。",
        ],
        [
            "不要用开环分母判断闭环稳定；稳定性看闭环特征方程。",
            "第一列为零或整行全零是特殊情况，需要用 ε 或辅助多项式处理。",
            "题目若问“临界稳定”或“等幅振荡频率”，通常就是让你找虚轴根。",
        ],
    )

    story += problem_template(
        "题型 4：一阶/二阶系统动态性能指标计算",
        "这类题会给闭环传递函数或阶跃响应曲线，让你求超调量、峰值时间、调节时间、阻尼比和自然频率。",
        [
            ["标准二阶：Φ(s) = ", ("frac", "ω_n^2", "s^2+2ζω_n s+ω_n^2")],
                ["M_p = e", ("supfrac", "-πζ", "√(1-ζ^2)"), " × 100%"],
            ["t_p = ", ("frac", "π", "ω_n√(1-ζ^2)")],
            ["t_s(2%) ≈ ", ("frac", "4", "ζω_n")],
        ],
        [
            "把给定闭环传递函数分母整理成 s^2+2ζω_n s+ω_n^2。",
            "对照系数求出 ω_n 和 ζ。",
            "按题目要求代入超调量、峰值时间、调节时间公式。",
            "若给的是曲线，从稳态值读出峰值和超调，再反求 ζ。",
            "若系统不是标准二阶，先判断是否可以用主导极点近似。",
        ],
        [
            "二阶标准型分子必须与分母常数项一致，若有额外比例系数，先分清稳态增益和标准动态参数。",
            "2% 与 5% 调节时间系数不同：2% 用 4/(ζω_n)，5% 用 3/(ζω_n)。",
            "ζ≥1 时没有超调峰值，不能套欠阻尼超调公式。",
        ],
    )

    story += problem_template(
        "题型 5：稳态误差与系统型别",
        "稳态误差大题通常给开环传递函数和输入类型，要求求 e_ss 或为了满足误差指标求 K。",
        [
            ["e_ss = lim_(s→0) sE(s)"],
            ["E(s) = ", ("frac", "R(s)", "1+G(s)H(s)")],
            ["K_p=lim G(s)H(s)，K_v=lim sG(s)H(s)，K_a=lim s^2G(s)H(s)"],
            ["阶跃：", ("frac", "1", "1+K_p"), "，斜坡：", ("frac", "1", "K_v"), "，抛物线：", ("frac", "1", "K_a")],
        ],
        [
            "先判断系统型别：看 G(s)H(s) 中原点极点个数。",
            "识别输入：阶跃、斜坡、抛物线分别对应 1/s、1/s^2、1/s^3。",
            "计算对应误差系数 K_p、K_v 或 K_a。",
            "代入稳态误差公式；若题目给误差上限，反解 K 的范围。",
            "最后确认闭环稳定，否则稳态误差没有意义。",
        ],
        [
            "系统型别看开环 G(s)H(s)，不是看闭环传递函数。",
            "单位反馈公式最常用；非单位反馈时优先从 E(s)=R(s)-H(s)C(s) 推导。",
            "稳态误差为 ∞ 表示不能跟踪该输入，不是计算失败。",
        ],
    )

    story += problem_template(
        "题型 6：绘制根轨迹并求参数范围",
        "根轨迹题常让画图、求分离点、渐近线、与虚轴交点，或给定性能要求求 K。先用规则确定形状，再用代数求关键点。",
        [
            ["1 + KG_0(s)H(s)=0"],
            ["σ_a = ", ("frac", "Σp_i-Σz_i", "n-m")],
            ["θ_q = ", ("frac", "(2q+1)180°", "n-m")],
            ["分离点：", ("frac", "dK", "ds"), "=0"],
        ],
        [
            "列出开环零点和极点，标在 s 平面上。",
            "确定根轨迹分支数、起点、终点和实轴区间。",
            "计算渐近线交点和角度。",
            "用 dK/ds=0 求分离点或会合点。",
            "用劳斯表求与虚轴交点及临界 K。",
            "根据题目要求，从闭环特征方程或根轨迹图读出 K 的范围。",
        ],
        [
            "实轴判别规则：测试点右侧开环实零极点总数为奇数，则该点在根轨迹上。",
            "分离点候选值必须落在根轨迹实轴区间内，否则舍去。",
            "根轨迹图只是辅助，参数范围最好由劳斯表或特征方程确认。",
        ],
    )

    story += problem_template(
        "题型 7：Bode 图绘制与频域指标计算",
        "Bode 大题的关键是拆环节、找转折频率、叠加斜率。不要一上来代很多频率点，先画渐近线骨架。",
        [
            ["L(ω)=20lg|G(jω)|"],
            ["比例 K：20lgK"],
            ["惯性环节：", ("frac", "1", "Ts+1"), "，转折频率 ω=", ("frac", "1", "T")],
            ["相位裕度：γ = 180° + φ(ω_c)"],
        ],
        [
            "把开环传递函数分解成 K、积分/微分、一阶惯性/微分、二阶振荡等典型环节。",
            "求所有转折频率并按从小到大排列。",
            "从低频段开始确定幅频斜率，每过一个环节转折频率就改变对应斜率。",
            "相频曲线按各环节相位叠加，粗略图重点看 0.1ω_b 到 10ω_b 的过渡。",
            "求穿越频率后计算相位裕度或幅值裕度。",
        ],
        [
            "积分环节从低频开始就是 -20 dB/dec，不是从某个转折点才开始。",
            "多个环节的 dB 幅值直接相加，相位也直接相加。",
            "若 K 改变，Bode 幅频图整体上下平移，相频图不变。",
        ],
    )

    story += problem_template(
        "题型 8：Nyquist 判据判断闭环稳定",
        "Nyquist 大题看起来像画图题，本质是数包围次数。先知道开环右半平面极点数 P，再看开环频率特性曲线如何围绕 -1 点。",
        [
            ["Z = P - N"],
            ["闭环稳定：Z=0"],
            ["若 P=0，稳定要求通常是不包围 (-1,j0)"],
        ],
        [
            "写出开环传递函数 G(s)H(s)，判断右半平面开环极点数 P。",
            "根据典型环节或代入关键频率画 Nyquist 曲线的大致走向。",
            "数曲线对 (-1,j0) 的顺时针包围次数 N。",
            "计算 Z=P-N；若 Z=0，则闭环稳定。",
            "若题目给 K，观察 K 改变会按比例放大/缩小曲线，进而改变是否包围 -1 点。",
        ],
        [
            "包围的是 (-1,j0)，不是原点。",
            "采用不同教材的包围方向约定时，N 的符号可能相反；只要全题约定一致即可。",
            "有虚轴开环极点时，需要绕开小半圆，这类题要特别说明路径变形。",
        ],
    )

    story += [
        p("考前大题作答顺序建议", "h1"),
        table(
            [
                ["看到题目关键词", "优先方法", "最后检查"],
                ["方框图、信号流图、求 C/R", "方框图化简或梅逊公式", "反馈正负号、是否令其他输入为 0"],
                ["稳定、K 的范围、特征方程", "劳斯判据", "第一列是否全正、临界值是否另判"],
                ["超调、峰值、调节时间", "标准二阶对照", "ζ 是否在 0 到 1 之间"],
                ["稳态误差、型别", "终值定理和误差系数", "闭环是否稳定、输入类型是否识别正确"],
                ["根轨迹、分离点、渐近线", "根轨迹规则 + 劳斯", "候选点是否在轨迹区间"],
                ["Bode、裕度、Nyquist", "频域典型环节叠加", "dB/相位单位、包围点是否为 -1"],
            ],
            [45 * mm, 60 * mm, 59 * mm],
        ),
    ]
    return story


def build_pdf(path: Path, story: list) -> None:
    doc = SimpleDocTemplate(
        str(path),
        pagesize=A4,
        rightMargin=18 * mm,
        leftMargin=18 * mm,
        topMargin=16 * mm,
        bottomMargin=18 * mm,
        title=path.stem,
        author="Codex",
    )
    doc.build(story, onFirstPage=page_footer, onLaterPages=page_footer)


def main() -> None:
    build_pdf(OUT_FORMULAS, build_formula_doc())
    build_pdf(OUT_PROBLEMS, build_problem_doc())
    print(OUT_FORMULAS)
    print(OUT_PROBLEMS)


if __name__ == "__main__":
    main()
