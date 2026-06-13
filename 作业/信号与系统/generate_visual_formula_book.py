from pathlib import Path
import html
import re
from io import BytesIO

from PIL import Image, ImageDraw, ImageFont
from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import cm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (
    BaseDocTemplate,
    Frame,
    Image as RLImage,
    KeepTogether,
    PageBreak,
    PageTemplate,
    Paragraph,
    Spacer,
    Table,
    TableStyle,
)


ROOT = Path(__file__).resolve().parent
OUT_HTML = ROOT / "信号与系统公式大全-直观版.html"
OUT_PDF = ROOT / "信号与系统公式大全-直观版.pdf"
IMG_DIR = ROOT / ".formula_images"
FONT_DIR = Path(r"C:\Windows\Fonts")
FONT_CN = FONT_DIR / "NotoSerifSC-VF.ttf"
FONT_SANS = FONT_DIR / "NotoSansSC-VF.ttf"


def frac(num, den):
    return f'<span class="frac"><span>{num}</span><span>{den}</span></span>'


def cases(rows):
    body = "".join(
        f"<tr><td>{value}</td><td>{cond}</td></tr>" for value, cond in rows
    )
    return f'<span class="cases"><span class="brace">{{</span><table>{body}</table></span>'


def formula(*parts):
    return "".join(parts)


SECTIONS = [
    (
        "0. 约定与常用符号",
        [
            (
                "单位阶跃、冲激、符号函数",
                "本册把单位阶跃函数统一记作 u(t)，离散单位阶跃记作 u[n]。",
                [
                    formula("u(t) = ", cases([("0", "t &lt; 0"), ("1", "t &gt; 0")])),
                    formula("δ(t) = ", frac("d u(t)", "dt")),
                    "u(t) = ∫<sub>-∞</sub><sup>t</sup> δ(τ) dτ",
                    "sgn(t) = 2u(t) - 1",
                    formula("u(t) = ", frac("1 + sgn(t)", "2")),
                    "∫<sub>-∞</sub><sup>∞</sup> x(t)δ(t - t<sub>0</sub>) dt = x(t<sub>0</sub>)",
                ],
            ),
            (
                "尺度、时移、翻转",
                "先看括号里的 t 怎么变，再判断移动、压缩或翻转。",
                [
                    "x(t - t<sub>0</sub>)：右移 t<sub>0</sub>",
                    "x(t + t<sub>0</sub>)：左移 t<sub>0</sub>",
                    "|a| &gt; 1 时，x(at) 压缩；0 &lt; |a| &lt; 1 时，x(at) 展宽",
                    "x(-t)：关于 t = 0 翻转",
                    formula("δ(at) = ", frac("1", "|a|"), " δ(t)"),
                ],
            ),
            (
                "能量、功率、奇偶分解",
                "能量信号平均功率为 0；功率信号总能量一般无穷大。",
                [
                    "E<sub>x</sub> = ∫<sub>-∞</sub><sup>∞</sup> |x(t)|<sup>2</sup> dt",
                    formula(
                        "P<sub>x</sub> = lim<sub>T→∞</sub> ",
                        frac("1", "2T"),
                        " ∫<sub>-T</sub><sup>T</sup> |x(t)|<sup>2</sup> dt",
                    ),
                    formula("x<sub>e</sub>(t) = ", frac("x(t) + x(-t)", "2")),
                    formula("x<sub>o</sub>(t) = ", frac("x(t) - x(-t)", "2")),
                    "x(t) = x<sub>e</sub>(t) + x<sub>o</sub>(t)",
                ],
            ),
        ],
    ),
    (
        "1. 连续时间系统的时域分析",
        [
            (
                "LTI 系统与卷积",
                "线性时不变系统完全由冲激响应 h(t) 描述。",
                [
                    "y(t) = x(t) * h(t)",
                    "x(t) * h(t) = ∫<sub>-∞</sub><sup>∞</sup> x(τ)h(t - τ)dτ",
                    "x(t) * δ(t - t<sub>0</sub>) = x(t - t<sub>0</sub>)",
                    "δ(t) * h(t) = h(t)",
                ],
            ),
            (
                "零输入、零状态、阶跃输入",
                "总响应 = 零输入响应 + 零状态响应；阶跃输入统一写作 u(t)。",
                [
                    "y(t) = y<sub>zi</sub>(t) + y<sub>zs</sub>(t)",
                    "y<sub>zs</sub>(t) = x(t) * h(t)",
                    "s(t) = h(t) * u(t)",
                    "s(t) = ∫<sub>-∞</sub><sup>t</sup> h(τ)dτ",
                    formula("h(t) = ", frac("d s(t)", "dt")),
                ],
            ),
            (
                "因果性、稳定性、可逆性",
                "判断 LTI 系统时，优先看 h(t)。",
                [
                    "因果：h(t) = 0，t &lt; 0",
                    "BIBO 稳定：∫<sub>-∞</sub><sup>∞</sup> |h(t)|dt &lt; ∞",
                    "可逆：h(t) * h<sub>i</sub>(t) = δ(t)",
                ],
            ),
        ],
    ),
    (
        "2. 连续时间傅里叶级数 CTFS",
        [
            (
                "三角形式",
                "周期为 T，基波角频率 ω<sub>0</sub> = 2π / T。",
                [
                    "x(t) = a<sub>0</sub> + Σ<sub>n=1</sub><sup>∞</sup>[a<sub>n</sub>cos(nω<sub>0</sub>t) + b<sub>n</sub>sin(nω<sub>0</sub>t)]",
                    formula("a<sub>0</sub> = ", frac("1", "T"), " ∫<sub>T</sub> x(t)dt"),
                    formula("a<sub>n</sub> = ", frac("2", "T"), " ∫<sub>T</sub> x(t)cos(nω<sub>0</sub>t)dt"),
                    formula("b<sub>n</sub> = ", frac("2", "T"), " ∫<sub>T</sub> x(t)sin(nω<sub>0</sub>t)dt"),
                ],
            ),
            (
                "指数形式与 Parseval",
                "实信号满足 C<sub>-k</sub> = C<sub>k</sub><sup>*</sup>。",
                [
                    "x(t) = Σ<sub>k=-∞</sub><sup>∞</sup> C<sub>k</sub>e<sup>jkω₀t</sup>",
                    formula("C<sub>k</sub> = ", frac("1", "T"), " ∫<sub>T</sub> x(t)e<sup>-jkω₀t</sup>dt"),
                    formula("P = ", frac("1", "T"), " ∫<sub>T</sub>|x(t)|<sup>2</sup>dt"),
                    "P = Σ<sub>k=-∞</sub><sup>∞</sup>|C<sub>k</sub>|<sup>2</sup>",
                ],
            ),
        ],
    ),
    (
        "3. 连续时间傅里叶变换 CTFT",
        [
            (
                "定义",
                "采用角频率 ω 的工程常用约定。",
                [
                    "X(jω) = ∫<sub>-∞</sub><sup>∞</sup> x(t)e<sup>-jωt</sup>dt",
                    formula("x(t) = ", frac("1", "2π"), " ∫<sub>-∞</sub><sup>∞</sup> X(jω)e<sup>jωt</sup>dω"),
                ],
            ),
            (
                "常用变换对",
                "冲激、阶跃、指数、正余弦是最高频考点。",
                [
                    "δ(t) ↔ 1",
                    "1 ↔ 2πδ(ω)",
                    "e<sup>jω₀t</sup> ↔ 2πδ(ω - ω<sub>0</sub>)",
                    "cos(ω<sub>0</sub>t) ↔ π[δ(ω - ω<sub>0</sub>) + δ(ω + ω<sub>0</sub>)]",
                    formula("sin(ω<sub>0</sub>t) ↔ ", frac("π", "j"), "[δ(ω - ω<sub>0</sub>) - δ(ω + ω<sub>0</sub>)]"),
                    formula("e<sup>-at</sup>u(t) ↔ ", frac("1", "a + jω"), "，a &gt; 0"),
                    formula("u(t) ↔ πδ(ω) + ", frac("1", "jω")),
                ],
            ),
            (
                "性质",
                "时域操作和频域操作成对出现。",
                [
                    "x(t - t<sub>0</sub>) ↔ e<sup>-jωt₀</sup>X(jω)",
                    "e<sup>jω₀t</sup>x(t) ↔ X[j(ω - ω<sub>0</sub>)]",
                    formula("x(at) ↔ ", frac("1", "|a|"), "X(jω/a)"),
                    formula(frac("d<sup>n</sup>x(t)", "dt<sup>n</sup>"), " ↔ (jω)<sup>n</sup>X(jω)"),
                    "x(t) * h(t) ↔ X(jω)H(jω)",
                    formula("x(t)h(t) ↔ ", frac("1", "2π"), "X(jω) * H(jω)"),
                    formula("E = ∫<sub>-∞</sub><sup>∞</sup>|x(t)|<sup>2</sup>dt = ", frac("1", "2π"), "∫<sub>-∞</sub><sup>∞</sup>|X(jω)|<sup>2</sup>dω"),
                ],
            ),
        ],
    ),
    (
        "4. 连续时间系统的频域分析",
        [
            (
                "频率响应",
                "正弦稳态中，LTI 系统只改变幅度和相位。",
                [
                    "H(jω) = F{h(t)}",
                    "Y(jω) = X(jω)H(jω)",
                    "x(t)=Acos(ω<sub>0</sub>t+φ)",
                    "y<sub>ss</sub>(t)=A|H(jω<sub>0</sub>)|cos[ω<sub>0</sub>t+φ+∠H(jω<sub>0</sub>)]",
                ],
            ),
            (
                "无失真传输、理想低通、采样",
                "无失真只允许幅度缩放和整体延时。",
                [
                    "y(t)=Kx(t-t<sub>d</sub>)",
                    "H(jω)=Ke<sup>-jωt<sub>d</sub></sup>",
                    formula("h<sub>LP</sub>(t) = ", frac("sin(ω<sub>c</sub>t)", "πt"), " = ", frac("ω<sub>c</sub>", "π"), "Sa(ω<sub>c</sub>t)"),
                    "x<sub>s</sub>(t)=x(t)Σ<sub>n=-∞</sub><sup>∞</sup>δ(t-nT<sub>s</sub>)",
                    formula("X<sub>s</sub>(jω)=", frac("1", "T<sub>s</sub>"), "Σ<sub>k=-∞</sub><sup>∞</sup>X[j(ω-kω<sub>s</sub>)]"),
                    "采样定理：ω<sub>s</sub> &gt; 2ω<sub>m</sub>",
                ],
            ),
        ],
    ),
    (
        "5. 拉普拉斯变换与复频域分析",
        [
            (
                "定义与常用变换对",
                "右边信号通常乘 u(t)，ROC 在最右极点右侧。",
                [
                    "X(s)=∫<sub>-∞</sub><sup>∞</sup>x(t)e<sup>-st</sup>dt，s=σ+jω",
                    formula("u(t) ↔ ", frac("1", "s")),
                    formula("e<sup>-at</sup>u(t) ↔ ", frac("1", "s+a")),
                    formula("t<sup>n</sup>e<sup>-at</sup>u(t) ↔ ", frac("n!", "(s+a)<sup>n+1</sup>")),
                    formula("cos(ω<sub>0</sub>t)u(t) ↔ ", frac("s", "s<sup>2</sup>+ω<sub>0</sub><sup>2</sup>")),
                    formula("sin(ω<sub>0</sub>t)u(t) ↔ ", frac("ω<sub>0</sub>", "s<sup>2</sup>+ω<sub>0</sub><sup>2</sup>")),
                ],
            ),
            (
                "性质、初终值、系统函数",
                "使用初值/终值定理前要检查极点条件。",
                [
                    formula(frac("dx(t)", "dt"), " ↔ sX<sub>+</sub>(s) - x(0<sup>-</sup>)"),
                    "x(t-t<sub>0</sub>)u(t-t<sub>0</sub>) ↔ e<sup>-st₀</sup>X(s)",
                    "x(0<sup>+</sup>) = lim<sub>s→∞</sub>sX(s)",
                    "x(∞) = lim<sub>s→0</sub>sX(s)",
                    formula("H(s)=", frac("Y<sub>zs</sub>(s)", "X(s)")),
                    "因果稳定：全部极点在左半平面",
                ],
            ),
        ],
    ),
    (
        "6. 离散时间系统的时域分析",
        [
            (
                "基本序列与卷积",
                "离散单位阶跃写作 u[n]，单位样值写作 δ[n]。",
                [
                    formula("δ[n] = ", cases([("1", "n = 0"), ("0", "n ≠ 0")])),
                    formula("u[n] = ", cases([("1", "n ≥ 0"), ("0", "n &lt; 0")])),
                    "u[n] - u[n-1] = δ[n]",
                    "y[n] = x[n] * h[n] = Σ<sub>k=-∞</sub><sup>∞</sup>x[k]h[n-k]",
                    "x[n] * δ[n-n<sub>0</sub>] = x[n-n<sub>0</sub>]",
                    "x[n] * u[n] = Σ<sub>k=-∞</sub><sup>n</sup>x[k]",
                ],
            ),
            (
                "差分方程与系统性质",
                "求解思路：齐次解 + 特解，或直接转 z 域。",
                [
                    "Σ<sub>k=0</sub><sup>N</sup>a<sub>k</sub>y[n-k] = Σ<sub>m=0</sub><sup>M</sup>b<sub>m</sub>x[n-m]",
                    "y[n] = y<sub>zi</sub>[n] + y<sub>zs</sub>[n]",
                    "因果：h[n] = 0，n &lt; 0",
                    "BIBO 稳定：Σ<sub>n=-∞</sub><sup>∞</sup>|h[n]| &lt; ∞",
                ],
            ),
        ],
    ),
    (
        "7. z 变换",
        [
            (
                "定义与常用变换对",
                "z 变换必须连同收敛域 ROC 一起看。",
                [
                    "X(z)=Σ<sub>n=-∞</sub><sup>∞</sup>x[n]z<sup>-n</sup>",
                    "δ[n] ↔ 1",
                    formula("u[n] ↔ ", frac("1", "1-z<sup>-1</sup>"), " = ", frac("z", "z-1"), "，|z| &gt; 1"),
                    formula("a<sup>n</sup>u[n] ↔ ", frac("1", "1-az<sup>-1</sup>"), " = ", frac("z", "z-a"), "，|z| &gt; |a|"),
                    formula("n a<sup>n</sup>u[n] ↔ ", frac("az<sup>-1</sup>", "(1-az<sup>-1</sup>)<sup>2</sup>")),
                ],
            ),
            (
                "性质、系统函数、稳定性",
                "单位圆在 ROC 内时，H(e^{jΩ}) 存在。",
                [
                    "x[n-n<sub>0</sub>] ↔ z<sup>-n₀</sup>X(z)",
                    "a<sup>n</sup>x[n] ↔ X(z/a)",
                    "n x[n] ↔ -z dX(z)/dz",
                    "x[n] * h[n] ↔ X(z)H(z)",
                    formula("H(z)=", frac("Y<sub>zs</sub>(z)", "X(z)")),
                    "H(e<sup>jΩ</sup>) = H(z)|<sub>z=e<sup>jΩ</sup></sub>",
                    "因果稳定：全部极点在单位圆内",
                ],
            ),
        ],
    ),
]


CSS = """
@page { size: A4; margin: 13mm 13mm 14mm 13mm; }
* { box-sizing: border-box; }
body {
  margin: 0;
  color: #172033;
  font-family: "Noto Serif SC", "Microsoft YaHei", "SimSun", serif;
  line-height: 1.42;
  background: #fff;
}
.cover {
  border-bottom: 3px solid #27496d;
  padding: 3mm 0 5mm;
  margin-bottom: 5mm;
}
h1 {
  margin: 0;
  font-size: 28px;
  letter-spacing: 0;
  text-align: center;
}
.subtitle {
  margin-top: 5px;
  color: #526071;
  text-align: center;
  font-size: 12px;
}
h2 {
  margin: 9mm 0 3mm;
  padding: 2mm 3mm;
  color: #fff;
  background: #27496d;
  font-size: 17px;
  break-after: avoid;
}
h3 {
  margin: 4.5mm 0 1mm;
  color: #1c314b;
  font-size: 14px;
  break-after: avoid;
}
.note {
  margin: 0 0 1.5mm;
  color: #58657a;
  font-size: 10.5px;
}
.block {
  margin: 0 0 3mm;
  padding: 2mm 3mm;
  border: 1px solid #d6deea;
  border-left: 4px solid #5a7fa8;
  background: #f7f9fc;
  break-inside: avoid;
}
.eq {
  margin: 1.6mm 0;
  font-size: 15px;
  color: #111827;
  font-family: "Cambria Math", "Noto Serif SC", "Times New Roman", serif;
}
.frac {
  display: inline-grid;
  grid-template-rows: auto auto;
  align-items: center;
  justify-items: center;
  vertical-align: -0.45em;
  margin: 0 0.12em;
  line-height: 1.05;
}
.frac > span:first-child {
  display: block;
  padding: 0 0.25em 1px;
  border-bottom: 1px solid currentColor;
}
.frac > span:last-child {
  display: block;
  padding: 1px 0.25em 0;
}
.cases {
  display: inline-flex;
  align-items: center;
  gap: 0.12em;
  vertical-align: middle;
}
.brace {
  font-size: 2.9em;
  line-height: 0.82;
}
.cases table {
  border-collapse: collapse;
  font-size: 0.98em;
}
.cases td {
  padding: 0.05em 0.28em;
  white-space: nowrap;
}
sup, sub { line-height: 0; }
"""


def build_html():
    body = [
        "<!doctype html><html><head><meta charset='utf-8'>",
        "<title>信号与系统公式大全-直观版</title>",
        f"<style>{CSS}</style>",
        "</head><body>",
        "<section class='cover'>",
        "<h1>信号与系统公式大全</h1>",
        "<div class='subtitle'>直观版：单位阶跃函数统一写作 u(t)，离散单位阶跃写作 u[n]</div>",
        "</section>",
    ]
    for section_title, groups in SECTIONS:
        body.append(f"<h2>{section_title}</h2>")
        for title, note, equations in groups:
            body.append("<div class='block'>")
            body.append(f"<h3>{title}</h3>")
            body.append(f"<p class='note'>{note}</p>")
            for eq in equations:
                body.append(f"<div class='eq'>{eq}</div>")
            body.append("</div>")
    body.append("</body></html>")
    OUT_HTML.write_text("\n".join(body), encoding="utf-8")
    print(f"Wrote {OUT_HTML}")


def strip_tags(value):
    value = re.sub(r"<br\s*/?>", " ", value)
    value = re.sub(r"<[^>]+>", "", value)
    return html.unescape(value)


def cases_to_text(match):
    rows = re.findall(r"<tr><td>(.*?)</td><td>(.*?)</td></tr>", match.group(0), re.S)
    parts = [f"{strip_tags(v)}，{strip_tags(c)}" for v, c in rows]
    return "{ " + "； ".join(parts) + " }"


def normalize_math_html(expr):
    expr = re.sub(
        r'<span class="cases">.*?</span>\s*</span>',
        cases_to_text,
        expr,
        flags=re.S,
    )

    def frac_repl(match):
        num = normalize_inline(match.group(1))
        den = normalize_inline(match.group(2))
        return f"§FRAC§{num}§OVER§{den}§END§"

    expr = re.sub(
        r'<span class="frac"><span>(.*?)</span><span>(.*?)</span></span>',
        frac_repl,
        expr,
        flags=re.S,
    )
    expr = normalize_inline(expr)
    return expr


def normalize_inline(expr):
    expr = re.sub(
        r"<sub>(.*?)</sub>",
        lambda m: f"§SUB§{normalize_inline(m.group(1))}§ENDSUB§",
        expr,
        flags=re.S,
    )
    expr = re.sub(
        r"<sup>(.*?)</sup>",
        lambda m: f"§SUP§{normalize_inline(m.group(1))}§ENDSUP§",
        expr,
        flags=re.S,
    )
    expr = strip_tags(expr)
    expr = re.sub(r"\s+", " ", expr)
    return expr.strip()


def load_font(size):
    return ImageFont.truetype(str(FONT_CN), size=size)


def text_bbox(draw, text, font):
    if not text:
        return (0, 0, 0, 0)
    box = draw.textbbox((0, 0), text, font=font)
    return box


def plain_for_fraction(expr):
    expr = re.sub(r"§SUB§(.*?)§ENDSUB§", r"_\1", expr)
    expr = re.sub(r"§SUP§(.*?)§ENDSUP§", r"^\1", expr)
    return expr


def split_formula_tokens(expr):
    parts = []
    pos = 0
    pattern = re.compile(
        r"§FRAC§(.*?)§OVER§(.*?)§END§|§SUB§(.*?)§ENDSUB§|§SUP§(.*?)§ENDSUP§"
    )
    for match in pattern.finditer(expr):
        if match.start() > pos:
            parts.append(("text", expr[pos : match.start()]))
        if match.group(1) is not None:
            parts.append(("frac", plain_for_fraction(match.group(1)), plain_for_fraction(match.group(2))))
        elif match.group(3) is not None:
            parts.append(("sub", match.group(3)))
        else:
            parts.append(("sup", match.group(4)))
        pos = match.end()
    if pos < len(expr):
        parts.append(("text", expr[pos:]))
    return parts


def measure_tokens(tokens, font, small_font):
    dummy = Image.new("RGB", (10, 10), "white")
    draw = ImageDraw.Draw(dummy)
    asc, desc = font.getmetrics()
    comps = []
    total_w = 0
    max_above = asc
    max_below = desc
    for token in tokens:
        if token[0] == "text":
            text = token[1]
            box = text_bbox(draw, text, font)
            w = max(1, box[2] - box[0])
            h = asc + desc
            above, below = asc, desc
            comps.append((token, w, h, above, below))
        elif token[0] in {"sub", "sup"}:
            text = token[1]
            box = text_bbox(draw, text, small_font)
            w = max(1, box[2] - box[0] + 1)
            sasc, sdesc = small_font.getmetrics()
            if token[0] == "sub":
                above, below = asc, max(desc + 12, int((sasc + sdesc) * 0.62))
            else:
                above, below = max(asc, sasc + sdesc + 8), desc
            comps.append((token, w, above + below, above, below))
        else:
            num, den = token[1], token[2]
            nbox = text_bbox(draw, num, small_font)
            dbox = text_bbox(draw, den, small_font)
            nw = nbox[2] - nbox[0]
            dw = dbox[2] - dbox[0]
            sasc, sdesc = small_font.getmetrics()
            w = max(nw, dw) + 18
            above = sasc + sdesc + 8
            below = sasc + sdesc + 8
            h = above + below
            comps.append((token, w, h, above, below))
        total_w += w
        max_above = max(max_above, above)
        max_below = max(max_below, below)
    return comps, total_w, max_above, max_below


def render_formula(expr, path, max_width=1540):
    expr = normalize_math_html(expr)
    font_size = 40
    while font_size >= 26:
        font = load_font(font_size)
        small_font = load_font(max(18, int(font_size * 0.72)))
        tokens = split_formula_tokens(expr)
        comps, width, above, below = measure_tokens(tokens, font, small_font)
        if width <= max_width - 42:
            break
        font_size -= 2

    pad_x, pad_y = 22, 14
    img_w = min(max_width, int(width + 2 * pad_x))
    img_h = int(above + below + 2 * pad_y)
    image = Image.new("RGB", (img_w, img_h), "#F7F9FC")
    draw = ImageDraw.Draw(image)
    baseline = pad_y + above
    x = pad_x
    for token, w, h, token_above, token_below in comps:
        if token[0] == "text":
            draw.text((x, baseline - token_above), token[1], fill="#111827", font=font)
        elif token[0] == "sub":
            draw.text((x, baseline - 15), token[1], fill="#111827", font=small_font)
        elif token[0] == "sup":
            draw.text((x, baseline - token_above), token[1], fill="#111827", font=small_font)
        else:
            num, den = token[1], token[2]
            nbox = text_bbox(draw, num, small_font)
            dbox = text_bbox(draw, den, small_font)
            n_w = nbox[2] - nbox[0]
            d_w = dbox[2] - dbox[0]
            center = x + w / 2
            sasc, sdesc = small_font.getmetrics()
            num_y = baseline - token_above + 1
            line_y = baseline - 3
            den_y = baseline + 5
            draw.text((center - n_w / 2, num_y), num, fill="#111827", font=small_font)
            draw.line((x + 4, line_y, x + w - 4, line_y), fill="#111827", width=2)
            draw.text((center - d_w / 2, den_y), den, fill="#111827", font=small_font)
        x += w
    image.save(path)
    return path


def on_page(canvas, doc):
    canvas.saveState()
    canvas.setFont("CN", 8)
    canvas.setFillColor(colors.HexColor("#697386"))
    canvas.drawCentredString(A4[0] / 2, 0.75 * cm, f"信号与系统公式大全-直观版 · {canvas.getPageNumber()}")
    canvas.restoreState()


def build_pdf():
    pdfmetrics.registerFont(TTFont("CN", str(FONT_SANS)))
    pdfmetrics.registerFont(TTFont("CNSerif", str(FONT_CN)))
    IMG_DIR.mkdir(exist_ok=True)
    for old in IMG_DIR.glob("*.png"):
        old.unlink()

    sample = getSampleStyleSheet()
    styles = {
        "Title": ParagraphStyle(
            "Title",
            parent=sample["Title"],
            fontName="CN",
            fontSize=24,
            leading=31,
            alignment=TA_CENTER,
            textColor=colors.HexColor("#172033"),
            spaceAfter=6,
        ),
        "Subtitle": ParagraphStyle(
            "Subtitle",
            parent=sample["BodyText"],
            fontName="CN",
            fontSize=10,
            leading=15,
            alignment=TA_CENTER,
            textColor=colors.HexColor("#526071"),
            spaceAfter=16,
        ),
        "Section": ParagraphStyle(
            "Section",
            parent=sample["Heading1"],
            fontName="CN",
            fontSize=15,
            leading=20,
            textColor=colors.white,
            backColor=colors.HexColor("#27496D"),
            borderPadding=(4, 7, 4),
            spaceBefore=10,
            spaceAfter=7,
            keepWithNext=True,
        ),
        "Item": ParagraphStyle(
            "Item",
            parent=sample["Heading2"],
            fontName="CN",
            fontSize=12,
            leading=16,
            textColor=colors.HexColor("#1C314B"),
            spaceBefore=3,
            spaceAfter=2,
            keepWithNext=True,
        ),
        "Note": ParagraphStyle(
            "Note",
            parent=sample["BodyText"],
            fontName="CN",
            fontSize=8.6,
            leading=12,
            textColor=colors.HexColor("#58657A"),
            spaceAfter=4,
        ),
    }
    doc = BaseDocTemplate(
        str(OUT_PDF),
        pagesize=A4,
        leftMargin=1.25 * cm,
        rightMargin=1.25 * cm,
        topMargin=1.15 * cm,
        bottomMargin=1.05 * cm,
        title="信号与系统公式大全-直观版",
    )
    frame = Frame(doc.leftMargin, doc.bottomMargin, doc.width, doc.height, id="main")
    doc.addPageTemplates([PageTemplate(id="p", frames=[frame], onPage=on_page)])

    story = [
        Paragraph("信号与系统公式大全", styles["Title"]),
        Paragraph("直观版：单位阶跃函数统一写作 u(t)，离散单位阶跃写作 u[n]。", styles["Subtitle"]),
    ]

    img_index = 0
    for sec_idx, (section_title, groups) in enumerate(SECTIONS):
        if sec_idx in {3, 5, 7}:
            story.append(PageBreak())
        story.append(Paragraph(section_title, styles["Section"]))
        for title, note, equations in groups:
            block = [Paragraph(title, styles["Item"]), Paragraph(note, styles["Note"])]
            for eq in equations:
                img_index += 1
                img_path = IMG_DIR / f"eq_{img_index:03d}.png"
                render_formula(eq, img_path)
                with Image.open(img_path) as im:
                    w_px, h_px = im.size
                width_pt = min(doc.width - 18, w_px * 0.36)
                height_pt = h_px * (width_pt / w_px)
                block.append(RLImage(str(img_path), width=width_pt, height=height_pt))
                block.append(Spacer(1, 1.5))
            table = Table([[block]], colWidths=[doc.width])
            table.setStyle(
                TableStyle(
                    [
                        ("BACKGROUND", (0, 0), (-1, -1), colors.HexColor("#F7F9FC")),
                        ("BOX", (0, 0), (-1, -1), 0.6, colors.HexColor("#D6DEEA")),
                        ("LINEBEFORE", (0, 0), (-1, -1), 3, colors.HexColor("#5A7FA8")),
                        ("LEFTPADDING", (0, 0), (-1, -1), 8),
                        ("RIGHTPADDING", (0, 0), (-1, -1), 8),
                        ("TOPPADDING", (0, 0), (-1, -1), 6),
                        ("BOTTOMPADDING", (0, 0), (-1, -1), 6),
                    ]
                )
            )
            story.append(KeepTogether(table))
            story.append(Spacer(1, 5))
    doc.build(story)
    print(f"Wrote {OUT_PDF}")


if __name__ == "__main__":
    build_html()
    build_pdf()
