from pathlib import Path

from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER, TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import cm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (
    BaseDocTemplate,
    Frame,
    PageBreak,
    PageTemplate,
    Paragraph,
    Spacer,
    Table,
    TableStyle,
)


ROOT = Path(__file__).resolve().parent
OUT_PDF = ROOT / "信号与系统公式大全.pdf"
OUT_MD = ROOT / "信号与系统公式大全.md"


FONT_DIR = Path(r"C:\Windows\Fonts")
FONT_REGULAR = FONT_DIR / "NotoSansSC-VF.ttf"
FONT_SERIF = FONT_DIR / "NotoSerifSC-VF.ttf"
FONT_MONO = FONT_DIR / "consola.ttf"
FONT_MONO_BOLD = FONT_DIR / "consolab.ttf"


def register_fonts():
    pdfmetrics.registerFont(TTFont("CN", str(FONT_REGULAR)))
    pdfmetrics.registerFont(TTFont("CNSerif", str(FONT_SERIF)))
    pdfmetrics.registerFont(TTFont("Mono", str(FONT_MONO)))
    pdfmetrics.registerFont(TTFont("MonoBold", str(FONT_MONO_BOLD)))


SECTIONS = [
    {
        "title": "0. 约定与常用符号",
        "items": [
            {
                "name": "单位阶跃、冲激、符号函数",
                "note": "本册把单位阶跃函数统一记作 u(t)。离散单位阶跃记作 u[n]。",
                "formula": [
                    r"u(t)=\begin{cases}0,&t<0\\1,&t>0\end{cases},",
                    r"\delta(t)=\frac{d}{dt}u(t),\qquad u(t)=\int_{-\infty}^{t}\delta(\tau)\,d\tau",
                    r"\operatorname{sgn}(t)=2u(t)-1,\qquad u(t)=\frac{1+\operatorname{sgn}(t)}{2}",
                    r"\int_{-\infty}^{\infty}x(t)\delta(t-t_0)\,dt=x(t_0)",
                ],
            },
            {
                "name": "尺度、时移、翻转",
                "note": "先做括号内的线性变换，再观察图形位置和宽度。",
                "formula": [
                    r"x(t-t_0):\ \text{右移 }t_0,\qquad x(t+t_0):\ \text{左移 }t_0",
                    r"x(at):\ |a|>1\text{ 压缩},\quad 0<|a|<1\text{ 展宽}",
                    r"x(-t):\ \text{关于 }t=0\text{ 翻转}",
                    r"\delta(at)=\frac{1}{|a|}\delta(t),\qquad u(at)=\begin{cases}u(t),&a>0\\u(-t),&a<0\end{cases}",
                ],
            },
            {
                "name": "能量、功率、内积",
                "note": "能量信号平均功率为 0；功率信号总能量一般无穷大。",
                "formula": [
                    r"E_x=\int_{-\infty}^{\infty}|x(t)|^2\,dt",
                    r"P_x=\lim_{T\to\infty}\frac{1}{2T}\int_{-T}^{T}|x(t)|^2\,dt",
                    r"\langle x,y\rangle=\int_{-\infty}^{\infty}x(t)y^*(t)\,dt",
                    r"E_{x+y}=E_x+E_y+2\operatorname{Re}\langle x,y\rangle",
                ],
            },
            {
                "name": "奇偶分解",
                "note": "任何信号都能唯一分成偶分量和奇分量。",
                "formula": [
                    r"x_e(t)=\frac{x(t)+x(-t)}{2}",
                    r"x_o(t)=\frac{x(t)-x(-t)}{2}",
                    r"x(t)=x_e(t)+x_o(t)",
                ],
            },
        ],
    },
    {
        "title": "1. 连续时间系统的时域分析",
        "items": [
            {
                "name": "线性时不变系统 LTI",
                "note": "LTI 系统完全由冲激响应 h(t) 描述。",
                "formula": [
                    r"y(t)=x(t)*h(t)",
                    r"x(t)*h(t)=\int_{-\infty}^{\infty}x(\tau)h(t-\tau)\,d\tau",
                    r"x(t)*\delta(t-t_0)=x(t-t_0)",
                    r"\delta(t)*h(t)=h(t)",
                ],
            },
            {
                "name": "由微分方程求零输入与零状态",
                "note": "总响应 = 零输入响应 + 零状态响应。",
                "formula": [
                    r"\sum_{k=0}^{N}a_k\frac{d^k y(t)}{dt^k}",
                    r"\qquad=\sum_{m=0}^{M}b_m\frac{d^m x(t)}{dt^m}",
                    r"y(t)=y_{zi}(t)+y_{zs}(t)",
                    r"y_{zs}(t)=x(t)*h(t)",
                ],
            },
            {
                "name": "阶跃输入 u(t) 与阶跃响应",
                "note": "为避免混淆：u(t) 是单位阶跃函数；s(t) 常表示系统对 u(t) 的响应。",
                "formula": [
                    r"s(t)=h(t)*u(t)=\int_{-\infty}^{t}h(\tau)\,d\tau",
                    r"h(t)=\frac{d}{dt}s(t)",
                    r"x(t)=\int_{-\infty}^{\infty}x'(\tau)u(t-\tau)\,d\tau",
                ],
            },
            {
                "name": "因果性、稳定性、可逆性",
                "note": "考试常用 h(t) 的支撑区间和绝对可积性判断。",
                "formula": [
                    r"\text{因果 LTI:}\quad h(t)=0,\ t<0",
                    r"\text{BIBO 稳定:}\quad \int_{-\infty}^{\infty}|h(t)|\,dt<\infty",
                    r"\text{可逆:}\quad h(t)*h_i(t)=\delta(t)",
                ],
            },
        ],
    },
    {
        "title": "2. 连续时间傅里叶级数 CTFS",
        "items": [
            {
                "name": "三角形式与指数形式",
                "note": "周期为 T，基波角频率 omega_0=2pi/T。",
                "formula": [
                    r"x(t)=a_0+\sum_{n=1}^{\infty}\big[a_n\cos(n\omega_0 t)+b_n\sin(n\omega_0 t)\big]",
                    r"a_0=\frac{1}{T}\int_T x(t)\,dt",
                    r"a_n=\frac{2}{T}\int_T x(t)\cos(n\omega_0t)\,dt",
                    r"b_n=\frac{2}{T}\int_T x(t)\sin(n\omega_0t)\,dt",
                    r"x(t)=\sum_{k=-\infty}^{\infty}C_k e^{jk\omega_0t}",
                    r"C_k=\frac{1}{T}\int_T x(t)e^{-jk\omega_0t}\,dt",
                ],
            },
            {
                "name": "系数对称性",
                "note": "实信号满足共轭对称，偶/奇性可快速判断系数。",
                "formula": [
                    r"x(t)\in\mathbb{R}\quad\Rightarrow\quad C_{-k}=C_k^*",
                    r"x(t)\text{ real even}\quad\Rightarrow\quad C_k\in\mathbb{R},\ b_n=0",
                    r"x(t)\text{ real odd}\quad\Rightarrow\quad C_k\text{ pure imaginary},\ a_n=0",
                ],
            },
            {
                "name": "Parseval 等式",
                "note": "周期信号的平均功率可在频域求。",
                "formula": [
                    r"P=\frac{1}{T}\int_T |x(t)|^2\,dt",
                    r"\qquad=\sum_{k=-\infty}^{\infty}|C_k|^2",
                    r"\qquad=a_0^2+\frac{1}{2}\sum_{n=1}^{\infty}(a_n^2+b_n^2)",
                ],
            },
        ],
    },
    {
        "title": "3. 连续时间傅里叶变换 CTFT",
        "items": [
            {
                "name": "定义与反变换",
                "note": "本册采用角频率 omega 的工程常用约定。",
                "formula": [
                    r"X(j\omega)=\mathcal{F}\{x(t)\}",
                    r"\qquad=\int_{-\infty}^{\infty}x(t)e^{-j\omega t}\,dt",
                    r"x(t)=\mathcal{F}^{-1}\{X(j\omega)\}",
                    r"\qquad=\frac{1}{2\pi}\int_{-\infty}^{\infty}X(j\omega)e^{j\omega t}\,d\omega",
                ],
            },
            {
                "name": "基本变换对",
                "note": "常用变换对建议直接背，尤其是冲激、指数、矩形、抽样函数。",
                "formula": [
                    r"\delta(t)\ \longleftrightarrow\ 1",
                    r"1\ \longleftrightarrow\ 2\pi\delta(\omega)",
                    r"e^{j\omega_0t}\ \longleftrightarrow\ 2\pi\delta(\omega-\omega_0)",
                    r"\cos\omega_0t\ \longleftrightarrow\ \pi[\delta(\omega-\omega_0)+\delta(\omega+\omega_0)]",
                    r"\sin\omega_0t\ \longleftrightarrow\ \frac{\pi}{j}[\delta(\omega-\omega_0)-\delta(\omega+\omega_0)]",
                    r"e^{-at}u(t)\ \longleftrightarrow\ \frac{1}{a+j\omega},\quad a>0",
                    r"u(t)\ \longleftrightarrow\ \pi\delta(\omega)+\frac{1}{j\omega}",
                    r"\operatorname{rect}\!\left(\frac{t}{\tau}\right)\ \longleftrightarrow\ \tau\,\operatorname{Sa}\!\left(\frac{\omega\tau}{2}\right)",
                    r"\operatorname{Sa}(t)=\frac{\sin t}{t}",
                ],
            },
            {
                "name": "性质总表",
                "note": "时域操作和频域操作常成对出现。",
                "formula": [
                    r"ax_1(t)+bx_2(t)\ \longleftrightarrow\ aX_1(j\omega)+bX_2(j\omega)",
                    r"x(t-t_0)\ \longleftrightarrow\ e^{-j\omega t_0}X(j\omega)",
                    r"e^{j\omega_0t}x(t)\ \longleftrightarrow\ X[j(\omega-\omega_0)]",
                    r"x(at)\ \longleftrightarrow\ \frac{1}{|a|}X\!\left(j\frac{\omega}{a}\right)",
                    r"\frac{d^n x(t)}{dt^n}\ \longleftrightarrow\ (j\omega)^nX(j\omega)",
                    r"t^n x(t)\ \longleftrightarrow\ j^n\frac{d^nX(j\omega)}{d\omega^n}",
                    r"x(t)*h(t)\ \longleftrightarrow\ X(j\omega)H(j\omega)",
                    r"x(t)h(t)\ \longleftrightarrow\ \frac{1}{2\pi}X(j\omega)*H(j\omega)",
                ],
            },
            {
                "name": "Parseval 与能量谱",
                "note": "能量谱密度为 |X(jomega)|^2。",
                "formula": [
                    r"E=\int_{-\infty}^{\infty}|x(t)|^2\,dt",
                    r"\qquad=\frac{1}{2\pi}\int_{-\infty}^{\infty}|X(j\omega)|^2\,d\omega",
                ],
            },
        ],
    },
    {
        "title": "4. 连续时间系统的频域分析",
        "items": [
            {
                "name": "频率响应",
                "note": "正弦稳态中，系统只改变幅度和相位。",
                "formula": [
                    r"H(j\omega)=\mathcal{F}\{h(t)\}",
                    r"Y(j\omega)=X(j\omega)H(j\omega)",
                    r"x(t)=A\cos(\omega_0t+\varphi)",
                    r"\Rightarrow\ y_{ss}(t)=A|H(j\omega_0)|",
                    r"\qquad\cdot\cos[\omega_0t+\varphi+\angle H(j\omega_0)]",
                ],
            },
            {
                "name": "无失真传输",
                "note": "输出只允许幅度缩放和整体延时。",
                "formula": [
                    r"y(t)=Kx(t-t_d)",
                    r"H(j\omega)=K e^{-j\omega t_d}",
                    r"|H(j\omega)|=K,\qquad \angle H(j\omega)=-\omega t_d",
                ],
            },
            {
                "name": "理想滤波器",
                "note": "理想低通的冲激响应是 sinc/Sa 形状，非因果。",
                "formula": [
                    r"H_{LP}(j\omega)=\begin{cases}1,&|\omega|\le \omega_c\\0,&|\omega|>\omega_c\end{cases}",
                    r"h_{LP}(t)=\frac{1}{2\pi}\int_{-\omega_c}^{\omega_c}e^{j\omega t}\,d\omega",
                    r"\qquad=\frac{\sin\omega_c t}{\pi t}=\frac{\omega_c}{\pi}\operatorname{Sa}(\omega_ct)",
                ],
            },
            {
                "name": "采样定理",
                "note": "带限到 |omega| <= omega_m 时，采样角频率需大于两倍最高角频率。",
                "formula": [
                    r"x_s(t)=x(t)\sum_{n=-\infty}^{\infty}\delta(t-nT_s)",
                    r"X_s(j\omega)=\frac{1}{T_s}\sum_{k=-\infty}^{\infty}X[j(\omega-k\omega_s)]",
                    r"\omega_s=\frac{2\pi}{T_s},\qquad \omega_s>2\omega_m",
                    r"x(t)=\sum_{n=-\infty}^{\infty}x(nT_s)",
                    r"\qquad\cdot\operatorname{Sa}\!\left[\omega_m(t-nT_s)\right]\quad(\omega_s=2\omega_m)",
                ],
            },
        ],
    },
    {
        "title": "5. 拉普拉斯变换与复频域分析",
        "items": [
            {
                "name": "单边/双边拉普拉斯变换",
                "note": "连续因果系统常用单边拉普拉斯处理初值。",
                "formula": [
                    r"X(s)=\int_{-\infty}^{\infty}x(t)e^{-st}\,dt",
                    r"X_+(s)=\int_{0^-}^{\infty}x(t)e^{-st}\,dt",
                    r"s=\sigma+j\omega",
                ],
            },
            {
                "name": "常用变换对",
                "note": "右边信号一般乘 u(t)，收敛域在最右极点右侧。",
                "formula": [
                    r"\delta(t)\ \longleftrightarrow\ 1",
                    r"u(t)\ \longleftrightarrow\ \frac{1}{s},\quad \operatorname{Re}s>0",
                    r"e^{-at}u(t)\ \longleftrightarrow\ \frac{1}{s+a},\quad \operatorname{Re}s>-a",
                    r"t^n e^{-at}u(t)\ \longleftrightarrow\ \frac{n!}{(s+a)^{n+1}}",
                    r"\cos\omega_0t\,u(t)\ \longleftrightarrow\ \frac{s}{s^2+\omega_0^2}",
                    r"\sin\omega_0t\,u(t)\ \longleftrightarrow\ \frac{\omega_0}{s^2+\omega_0^2}",
                ],
            },
            {
                "name": "性质与初终值",
                "note": "初值/终值定理使用前要检查极点条件。",
                "formula": [
                    r"\frac{dx(t)}{dt}\ \longleftrightarrow\ sX_+(s)-x(0^-)",
                    r"\int_{0^-}^{t}x(\tau)d\tau\ \longleftrightarrow\ \frac{X_+(s)}{s}",
                    r"x(t-t_0)u(t-t_0)\ \longleftrightarrow\ e^{-st_0}X(s)",
                    r"x(0^+)=\lim_{s\to\infty}sX(s)",
                    r"x(\infty)=\lim_{s\to 0}sX(s)",
                ],
            },
            {
                "name": "系统函数与稳定性",
                "note": "频率响应是系统函数在 jomega 轴上的取值，前提是 jomega 轴在 ROC 内。",
                "formula": [
                    r"H(s)=\frac{Y_{zs}(s)}{X(s)}",
                    r"H(s)=\frac{\sum_{m=0}^{M}b_ms^m}{\sum_{k=0}^{N}a_ks^k}",
                    r"h(t)=\mathcal{L}^{-1}\{H(s)\}",
                    r"H(j\omega)=H(s)\big|_{s=j\omega}",
                    r"\text{因果稳定: all poles in left half-plane}",
                ],
            },
        ],
    },
    {
        "title": "6. 离散时间系统的时域分析",
        "items": [
            {
                "name": "基本序列",
                "note": "离散单位阶跃写作 u[n]，单位样值写作 delta[n]。",
                "formula": [
                    r"\delta[n]=\begin{cases}1,&n=0\\0,&n\ne0\end{cases}",
                    r"u[n]=\begin{cases}1,&n\ge0\\0,&n<0\end{cases}",
                    r"u[n]-u[n-1]=\delta[n]",
                    r"u[n]=\sum_{k=-\infty}^{n}\delta[k]",
                ],
            },
            {
                "name": "离散卷积",
                "note": "换元、翻转、平移、相乘、求和。",
                "formula": [
                    r"y[n]=x[n]*h[n]",
                    r"\qquad=\sum_{k=-\infty}^{\infty}x[k]h[n-k]",
                    r"x[n]*\delta[n-n_0]=x[n-n_0]",
                    r"x[n]*u[n]=\sum_{k=-\infty}^{n}x[k]",
                ],
            },
            {
                "name": "差分方程",
                "note": "求解思路与连续系统相同：齐次解 + 特解，或 z 域。",
                "formula": [
                    r"\sum_{k=0}^{N}a_k y[n-k]=\sum_{m=0}^{M}b_m x[n-m]",
                    r"y[n]=y_{zi}[n]+y_{zs}[n]",
                    r"y_{zs}[n]=x[n]*h[n]",
                ],
            },
            {
                "name": "离散 LTI 性质",
                "note": "判断规则与连续系统完全对应。",
                "formula": [
                    r"\text{因果:}\quad h[n]=0,\ n<0",
                    r"\text{BIBO 稳定:}\quad \sum_{n=-\infty}^{\infty}|h[n]|<\infty",
                    r"\text{可逆:}\quad h[n]*h_i[n]=\delta[n]",
                ],
            },
        ],
    },
    {
        "title": "7. z 变换",
        "items": [
            {
                "name": "定义与反变换",
                "note": "z 变换必须连同收敛域 ROC 一起看。",
                "formula": [
                    r"X(z)=\mathcal{Z}\{x[n]\}=\sum_{n=-\infty}^{\infty}x[n]z^{-n}",
                    r"x[n]=\mathcal{Z}^{-1}\{X(z)\}",
                    r"z=re^{j\Omega}",
                ],
            },
            {
                "name": "常用变换对",
                "note": "右边序列和左边序列形式相同，ROC 不同。",
                "formula": [
                    r"\delta[n]\ \longleftrightarrow\ 1",
                    r"u[n]\ \longleftrightarrow\ \frac{1}{1-z^{-1}}=\frac{z}{z-1},\quad |z|>1",
                    r"a^n u[n]\ \longleftrightarrow\ \frac{1}{1-az^{-1}}=\frac{z}{z-a},\quad |z|>|a|",
                    r"-a^n u[-n-1]\ \longleftrightarrow\ \frac{1}{1-az^{-1}},\quad |z|<|a|",
                    r"n a^n u[n]\ \longleftrightarrow\ \frac{az^{-1}}{(1-az^{-1})^2}",
                ],
            },
            {
                "name": "z 变换性质",
                "note": "移位性质要注意初始条件；双边形式最干净。",
                "formula": [
                    r"x[n-n_0]\ \longleftrightarrow\ z^{-n_0}X(z)",
                    r"a^n x[n]\ \longleftrightarrow\ X\!\left(\frac{z}{a}\right)",
                    r"n x[n]\ \longleftrightarrow\ -z\frac{dX(z)}{dz}",
                    r"x[n]*h[n]\ \longleftrightarrow\ X(z)H(z)",
                    r"x[0]=\lim_{z\to\infty}X(z)\quad(\text{right-sided})",
                    r"x[\infty]=\lim_{z\to1}(1-z^{-1})X(z)",
                ],
            },
            {
                "name": "系统函数、频率响应、稳定性",
                "note": "单位圆在 ROC 内时，H(e^{jOmega}) 存在。",
                "formula": [
                    r"H(z)=\frac{Y_{zs}(z)}{X(z)}",
                    r"H(z)=\frac{\sum_{m=0}^{M}b_m z^{-m}}{1+\sum_{k=1}^{N}a_k z^{-k}}",
                    r"H(e^{j\Omega})=H(z)\big|_{z=e^{j\Omega}}",
                    r"\text{因果稳定: all poles inside unit circle}",
                ],
            },
        ],
    },
    {
        "title": "8. 考前速查",
        "items": [
            {
                "name": "卷积常用技巧",
                "note": "遇到阶跃函数时先把积分/求和上下限画出来。",
                "formula": [
                    r"u(t-a)u(b-t)=1\quad \text{only for } a\le t\le b",
                    r"\int_{-\infty}^{\infty}f(\tau)u(\tau-a)u(t-\tau)\,d\tau",
                    r"\qquad=\int_{a}^{t}f(\tau)\,d\tau\quad(t\ge a)",
                    r"e^{-at}u(t)*e^{-bt}u(t)",
                    r"\qquad=\frac{e^{-bt}-e^{-at}}{a-b}u(t)\quad(a\ne b)",
                ],
            },
            {
                "name": "判断系统性质",
                "note": "先看是否有时变参数、未来输入、非线性运算，再看 h。",
                "formula": [
                    r"\text{memoryless: } y(t_0)\text{ only depends on }x(t_0)",
                    r"\text{linear: }T\{ax_1+bx_2\}=aT\{x_1\}+bT\{x_2\}",
                    r"\text{time invariant: }x(t-t_0)\Rightarrow y(t-t_0)",
                    r"\text{causal: no future input is used}",
                ],
            },
            {
                "name": "变换域解题路线",
                "note": "微分/差分方程优先转到 s 域或 z 域，再反变换。",
                "formula": [
                    r"\text{continuous: differential equation}\Rightarrow H(s)\Rightarrow h(t),y(t)",
                    r"\text{discrete: difference equation}\Rightarrow H(z)\Rightarrow h[n],y[n]",
                    r"\text{frequency response: }H(j\omega)\text{ or }H(e^{j\Omega})",
                ],
            },
        ],
    },
]


def md_escape(text):
    return text.replace("\\", "\\\\")


GREEK_AND_SYMBOLS = {
    r"\delta": "δ",
    r"\Delta": "Δ",
    r"\tau": "τ",
    r"\omega": "ω",
    r"\Omega": "Ω",
    r"\varphi": "φ",
    r"\phi": "φ",
    r"\pi": "π",
    r"\sigma": "σ",
    r"\infty": "∞",
    r"\sum": "∑",
    r"\int": "∫",
    r"\lim": "lim",
    r"\le": "≤",
    r"\ge": "≥",
    r"\ne": "≠",
    r"\to": "→",
    r"\Rightarrow": "⇒",
    r"\longleftrightarrow": "↔",
    r"\leftrightarrow": "↔",
    r"\cdot": "·",
    r"\pm": "±",
    r"\angle": "∠",
    r"\operatorname{Re}": "Re",
    r"\operatorname{sgn}": "sgn",
    r"\operatorname{rect}": "rect",
    r"\operatorname{Sa}": "Sa",
    r"\mathbb{R}": "ℝ",
    r"\mathcal{F}^{-1}": "F⁻¹",
    r"\mathcal{F}": "F",
    r"\mathcal{L}^{-1}": "L⁻¹",
    r"\mathcal{L}": "L",
    r"\mathcal{Z}^{-1}": "Z⁻¹",
    r"\mathcal{Z}": "Z",
}


SUB = str.maketrans("0123456789+-=()aeijmnorstx", "₀₁₂₃₄₅₆₇₈₉₊₋₌₍₎ₐₑᵢⱼₘₙₒᵣₛₜₓ")
SUP = str.maketrans("0123456789+-=()n", "⁰¹²³⁴⁵⁶⁷⁸⁹⁺⁻⁼⁽⁾ⁿ")


def parse_braced(text, start):
    if start >= len(text) or text[start] != "{":
        return "", start
    depth = 0
    for idx in range(start, len(text)):
        char = text[idx]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return text[start + 1 : idx], idx + 1
    return text[start + 1 :], len(text)


def replace_frac(text):
    result = []
    i = 0
    while i < len(text):
        if text.startswith(r"\frac", i):
            num, j = parse_braced(text, i + 5)
            den, k = parse_braced(text, j)
            result.append(f"(({num}) / ({den}))")
            i = k
        else:
            result.append(text[i])
            i += 1
    return "".join(result)


def strip_command_with_braces(text, command):
    needle = command + "{"
    while needle in text:
        start = text.index(needle)
        inner, end = parse_braced(text, start + len(command))
        text = text[:start] + inner + text[end:]
    return text


def convert_bounds(text):
    import re

    text = re.sub(r"([∫∑])_\{([^{}]+)\}\^\{([^{}]+)\}", r"\1[\2 → \3]", text)
    text = re.sub(r"([∫∑])_([^{}\s]+)\^([^{}\s]+)", r"\1[\2 → \3]", text)
    text = re.sub(r"lim_\{([^{}]+)\}", r"lim[\1]", text)
    return text


def apply_scripts(text):
    import re

    def sub_repl(match):
        raw = match.group(1) or match.group(2)
        raw = raw.replace("{", "").replace("}", "")
        if raw and all(ch in "0123456789+-=()aeijmnorstx" for ch in raw):
            return raw.translate(SUB)
        return "_(" + raw + ")"

    def sup_repl(match):
        raw = match.group(1) or match.group(2)
        raw = raw.replace("{", "").replace("}", "")
        if raw and all(ch in "0123456789+-=()n" for ch in raw):
            return raw.translate(SUP)
        return "^(" + raw + ")"

    text = re.sub(r"_\{([^{}]+)\}|_([A-Za-z0-9+\-=()]+)", sub_repl, text)
    text = re.sub(r"\^\{([^{}]+)\}|\^([A-Za-z0-9+\-=()]+)", sup_repl, text)
    return text


def cleanup_visual(text):
    import re

    text = replace_frac(text)
    text = text.replace(r"\langle", "<")
    text = text.replace(r"\rangle", ">")
    for key, value in GREEK_AND_SYMBOLS.items():
        text = text.replace(key, value)
    text = strip_command_with_braces(text, r"\text")
    text = text.replace(r"\left", "").replace(r"\right", "")
    text = text.replace(r"\begin{cases}", "{")
    text = text.replace(r"\end{cases}", "}")
    text = text.replace(r"\qquad", "    ")
    text = text.replace(r"\quad", "  ")
    text = text.replace(r"\,", " ")
    text = text.replace(r"\!", "")
    text = text.replace(r"\ ", " ")
    text = text.replace(r"\\", "; ")
    text = convert_bounds(text)
    text = text.replace("&", "，")
    text = text.replace(r"\big", "")
    text = text.replace(r"\Big", "")
    text = text.replace(r"\bigg", "")
    text = text.replace(r"\Bigg", "")
    text = re.sub(r"_\{([^{}]+)\}", r"_\1", text)
    text = re.sub(r"\^\{([^{}]+)\}", r"^(\1)", text)
    text = text.replace("{", "").replace("}", "")
    text = text.replace("\\", "")
    text = text.replace("mathrm", "")
    text = re.sub(r"\s+", " ", text)
    text = text.replace(" ,", ",").replace("， ", "，")
    text = text.replace("，,", "，").replace(",，", "，")
    return text.strip()


def visual_formula_lines(lines):
    visual = []
    for line in lines:
        parts = cleanup_visual(line).split("\n")
        for part in parts:
            part = part.strip()
            if part:
                visual.append(part)
    return visual


def build_markdown():
    lines = [
        "# 信号与系统公式大全",
        "",
        "整理依据：当前文件夹中的《信号与系统不挂科》1-7 讲义体系。",
        "",
        "约定：单位阶跃函数统一记作 $u(t)$；离散单位阶跃记作 $u[n]$。",
        "",
    ]
    for sec in SECTIONS:
        lines.append(f"## {sec['title']}")
        lines.append("")
        for item in sec["items"]:
            lines.append(f"### {item['name']}")
            lines.append("")
            if item.get("note"):
                lines.append(f"> {item['note']}")
                lines.append("")
            for formula in visual_formula_lines(item["formula"]):
                lines.append(f"- {formula}")
            lines.append("")
    OUT_MD.write_text("\n".join(lines), encoding="utf-8")


def on_page(canvas, doc):
    canvas.saveState()
    canvas.setFont("CN", 8)
    canvas.setFillColor(colors.HexColor("#666666"))
    page = canvas.getPageNumber()
    canvas.drawCentredString(A4[0] / 2, 0.9 * cm, f"信号与系统公式大全  ·  {page}")
    canvas.restoreState()


def para(text, style):
    text = (
        text.replace("&", "&amp;")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
        .replace(" ", "&nbsp;")
    )
    return Paragraph(text, style)


def formula_block(lines, styles):
    rows = []
    for line in visual_formula_lines(lines):
        line = line.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
        rows.append([Paragraph(line, styles["Formula"])])
    table = Table(rows, colWidths=[16.2 * cm], hAlign="LEFT")
    table.setStyle(
        TableStyle(
            [
                ("BACKGROUND", (0, 0), (-1, -1), colors.HexColor("#F7F8FA")),
                ("BOX", (0, 0), (-1, -1), 0.6, colors.HexColor("#D9DDE6")),
                ("LEFTPADDING", (0, 0), (-1, -1), 8),
                ("RIGHTPADDING", (0, 0), (-1, -1), 8),
                ("TOPPADDING", (0, 0), (-1, -1), 5),
                ("BOTTOMPADDING", (0, 0), (-1, -1), 5),
                ("LINEBELOW", (0, 0), (-1, -2), 0.25, colors.HexColor("#ECEFF4")),
            ]
        )
    )
    return table


def build_pdf():
    register_fonts()

    base = getSampleStyleSheet()
    styles = {
        "Title": ParagraphStyle(
            "Title",
            parent=base["Title"],
            fontName="CN",
            fontSize=23,
            leading=31,
            alignment=TA_CENTER,
            textColor=colors.HexColor("#1B2430"),
            spaceAfter=16,
        ),
        "Subtitle": ParagraphStyle(
            "Subtitle",
            parent=base["BodyText"],
            fontName="CN",
            fontSize=10,
            leading=16,
            alignment=TA_CENTER,
            textColor=colors.HexColor("#4A5568"),
            spaceAfter=20,
        ),
        "Section": ParagraphStyle(
            "Section",
            parent=base["Heading1"],
            fontName="CN",
            fontSize=15,
            leading=20,
            textColor=colors.HexColor("#12355B"),
            spaceBefore=12,
            spaceAfter=8,
            keepWithNext=True,
        ),
        "Item": ParagraphStyle(
            "Item",
            parent=base["Heading2"],
            fontName="CN",
            fontSize=11.5,
            leading=15,
            textColor=colors.HexColor("#1F2937"),
            spaceBefore=7,
            spaceAfter=3,
            keepWithNext=True,
        ),
        "Note": ParagraphStyle(
            "Note",
            parent=base["BodyText"],
            fontName="CN",
            fontSize=8.8,
            leading=13,
            textColor=colors.HexColor("#4B5563"),
            leftIndent=6,
            spaceAfter=4,
        ),
        "Formula": ParagraphStyle(
            "Formula",
            parent=base["Code"],
            fontName="CNSerif",
            fontSize=10.4,
            leading=15.2,
            alignment=TA_LEFT,
            wordWrap="CJK",
            textColor=colors.HexColor("#111827"),
        ),
    }

    doc = BaseDocTemplate(
        str(OUT_PDF),
        pagesize=A4,
        leftMargin=1.75 * cm,
        rightMargin=1.75 * cm,
        topMargin=1.55 * cm,
        bottomMargin=1.35 * cm,
        title="信号与系统公式大全",
        author="Codex",
    )
    frame = Frame(doc.leftMargin, doc.bottomMargin, doc.width, doc.height, id="normal")
    doc.addPageTemplates([PageTemplate(id="FormulaBook", frames=[frame], onPage=on_page)])

    story = [
        Paragraph("信号与系统公式大全", styles["Title"]),
        Paragraph(
            "按当前讲义体系整理；单位阶跃函数统一写作 u(t)，离散单位阶跃写作 u[n]。"
            "<br/>公式已转换成直观数学符号排版，便于考前速查。",
            styles["Subtitle"],
        ),
    ]

    for idx, sec in enumerate(SECTIONS):
        if idx in {3, 5, 7}:
            story.append(PageBreak())
        story.append(Paragraph(sec["title"], styles["Section"]))
        for item in sec["items"]:
            story.append(Paragraph(item["name"], styles["Item"]))
            if item.get("note"):
                story.append(Paragraph(item["note"], styles["Note"]))
            story.append(formula_block(item["formula"], styles))
            story.append(Spacer(1, 5))

    doc.build(story)


if __name__ == "__main__":
    build_markdown()
    build_pdf()
    print(f"Wrote {OUT_MD}")
    print(f"Wrote {OUT_PDF}")
