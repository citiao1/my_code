import math
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont


OUT = Path(__file__).resolve().parent


def font(size, bold=False):
    candidates = [
        r"C:\Windows\Fonts\simhei.ttf",
        r"C:\Windows\Fonts\msyhbd.ttc" if bold else r"C:\Windows\Fonts\msyh.ttc",
        r"C:\Windows\Fonts\simsun.ttc",
    ]
    for path in candidates:
        try:
            return ImageFont.truetype(path, size)
        except OSError:
            pass
    return ImageFont.load_default()


FONT = font(22)
SMALL = font(17)
TINY = font(14)
TITLE = font(28, bold=True)


def canvas(w=1400, h=900):
    return Image.new("RGB", (w, h), "white")


def draw_text(draw, xy, text, fill=(20, 20, 20), fnt=None, anchor=None):
    draw.text(xy, text, font=fnt or FONT, fill=fill, anchor=anchor)


class Plot:
    def __init__(self, draw, box, xlim, ylim, logx=False):
        self.draw = draw
        self.box = box
        self.xlim = xlim
        self.ylim = ylim
        self.logx = logx

    def tx(self, x):
        x0, y0, x1, y1 = self.box
        if self.logx:
            lx = math.log10(x)
            a = math.log10(self.xlim[0])
            b = math.log10(self.xlim[1])
            return x0 + (lx - a) / (b - a) * (x1 - x0)
        return x0 + (x - self.xlim[0]) / (self.xlim[1] - self.xlim[0]) * (x1 - x0)

    def ty(self, y):
        x0, y0, x1, y1 = self.box
        return y1 - (y - self.ylim[0]) / (self.ylim[1] - self.ylim[0]) * (y1 - y0)

    def pt(self, x, y):
        return (self.tx(x), self.ty(y))

    def axes(self, xticks=None, yticks=None, xlabel="", ylabel=""):
        x0, y0, x1, y1 = self.box
        self.draw.rectangle(self.box, outline=(40, 40, 40), width=2)
        if xticks:
            for x in xticks:
                if not (self.xlim[0] <= x <= self.xlim[1]):
                    continue
                px = self.tx(x)
                self.draw.line((px, y0, px, y1), fill=(225, 225, 225))
                self.draw.line((px, y1, px, y1 + 7), fill=(40, 40, 40), width=2)
                label = f"{x:g}"
                draw_text(self.draw, (px, y1 + 10), label, fnt=TINY, anchor="ma")
        if yticks:
            for y in yticks:
                if not (self.ylim[0] <= y <= self.ylim[1]):
                    continue
                py = self.ty(y)
                self.draw.line((x0, py, x1, py), fill=(225, 225, 225))
                self.draw.line((x0 - 7, py, x0, py), fill=(40, 40, 40), width=2)
                draw_text(self.draw, (x0 - 12, py), f"{y:g}", fnt=TINY, anchor="rm")
        if xlabel:
            draw_text(self.draw, ((x0 + x1) / 2, y1 + 42), xlabel, fnt=SMALL, anchor="ma")
        if ylabel:
            draw_text(self.draw, (x0 - 55, (y0 + y1) / 2), ylabel, fnt=SMALL, anchor="mm")

    def line(self, xs, ys, fill=(0, 80, 180), width=3, dash=False):
        pts = []
        for x, y in zip(xs, ys):
            if self.xlim[0] <= x <= self.xlim[1] and self.ylim[0] <= y <= self.ylim[1]:
                pts.append(self.pt(float(x), float(y)))
            else:
                if len(pts) >= 2:
                    self._draw_poly(pts, fill, width, dash)
                pts = []
        if len(pts) >= 2:
            self._draw_poly(pts, fill, width, dash)

    def _draw_poly(self, pts, fill, width, dash):
        if not dash:
            self.draw.line(pts, fill=fill, width=width, joint="curve")
            return
        for i in range(len(pts) - 1):
            x0, y0 = pts[i]
            x1, y1 = pts[i + 1]
            dist = math.hypot(x1 - x0, y1 - y0)
            if dist == 0:
                continue
            steps = max(1, int(dist / 14))
            for k in range(steps):
                if k % 2 == 0:
                    a = k / steps
                    b = min(1, (k + 0.55) / steps)
                    self.draw.line((x0 + (x1 - x0) * a, y0 + (y1 - y0) * a,
                                    x0 + (x1 - x0) * b, y0 + (y1 - y0) * b),
                                   fill=fill, width=width)

    def hline(self, y, fill=(80, 80, 80), width=2):
        if self.ylim[0] <= y <= self.ylim[1]:
            self.draw.line((self.tx(self.xlim[0]), self.ty(y), self.tx(self.xlim[1]), self.ty(y)), fill=fill, width=width)

    def vline(self, x, fill=(80, 80, 80), width=2, dash=False):
        if self.xlim[0] <= x <= self.xlim[1]:
            if dash:
                self._draw_poly([(self.tx(x), self.ty(self.ylim[0])), (self.tx(x), self.ty(self.ylim[1]))], fill, width, True)
            else:
                self.draw.line((self.tx(x), self.ty(self.ylim[0]), self.tx(x), self.ty(self.ylim[1])), fill=fill, width=width)

    def marker(self, x, y, r=6, fill=(220, 0, 0)):
        px, py = self.pt(x, y)
        self.draw.ellipse((px-r, py-r, px+r, py+r), fill=fill, outline=(0, 0, 0))


def crossing(x, y, level):
    z = y - level
    idxs = np.where(z[:-1] * z[1:] <= 0)[0]
    i = int(idxs[0])
    lx0, lx1 = np.log(x[i]), np.log(x[i + 1])
    y0, y1 = y[i], y[i + 1]
    t = (level - y0) / (y1 - y0)
    return float(np.exp(lx0 + t * (lx1 - lx0)))


def bisection(fn, lo, hi, n=100):
    flo = fn(lo)
    for _ in range(n):
        mid = (lo + hi) / 2
        fm = fn(mid)
        if flo * fm <= 0:
            hi = mid
        else:
            lo = mid
            flo = fm
    return (lo + hi) / 2


def make_5_8():
    w = np.logspace(-2, 2, 1600)
    G = 10 / ((1 + 0.2j*w) * (2 + 1j*w) * (0.5 + 1j*w))
    mag = 20 * np.log10(np.abs(G))
    phase = np.unwrap(np.angle(G)) * 180 / np.pi
    asym = np.full_like(w, 20.0)
    for c in [0.5, 2, 5]:
        idx = w > c
        asym[idx] -= 20 * np.log10(w[idx] / c)
    wc = crossing(w, np.abs(G), 1)
    phi_wc = np.interp(np.log(wc), np.log(w), phase)
    gamma = 180 + phi_wc
    fphase = lambda x: -(math.atan(0.2*x) + math.atan(0.5*x) + math.atan(2*x)) * 180 / math.pi + 180
    wp = bisection(fphase, 2, 8)
    mag_wp = abs(10 / ((1 + 0.2j*wp) * (2 + 1j*wp) * (0.5 + 1j*wp)))
    kg = 1 / mag_wp

    im = canvas()
    d = ImageDraw.Draw(im)
    draw_text(d, (700, 24), "5-8 频率特性曲线", fnt=TITLE, anchor="ma")
    p1 = Plot(d, (120, 90, 1320, 420), (0.03, 60), (-85, 25), logx=True)
    p1.axes([0.05, 0.1, 0.5, 2, 5, 10, 50], [-80, -60, -40, -20, 0, 20], "omega / rad/s", "dB")
    p1.hline(0, (0, 0, 0))
    p1.line(w, mag, (0, 83, 170), 3)
    p1.line(w, asym, (215, 60, 55), 3, dash=True)
    for c in [0.5, 2, 5]:
        p1.vline(c, (90, 90, 90), 2, dash=True)
        draw_text(d, (p1.tx(c), p1.ty(18)), f"omega={c:g}", fnt=TINY, anchor="ma")
    p1.vline(wc, (140, 0, 160), 3, dash=True)
    p1.vline(wp, (0, 130, 80), 3, dash=True)
    draw_text(d, (170, 100), "实线：精确幅频；红虚线：渐近线", fnt=SMALL)
    draw_text(d, (p1.tx(wc)+6, p1.ty(3)), f"wc≈{wc:.2f}", fnt=TINY)
    draw_text(d, (p1.tx(wp)+6, p1.ty(-8)), f"wg≈{wp:.2f}", fnt=TINY)

    p2 = Plot(d, (120, 510, 1320, 800), (0.03, 60), (-280, 10), logx=True)
    p2.axes([0.05, 0.1, 0.5, 2, 5, 10, 50], [-270, -225, -180, -135, -90, -45, 0], "omega / rad/s", "deg")
    p2.hline(-180, (0, 0, 0))
    p2.line(w, phase, (0, 83, 170), 3)
    p2.vline(wc, (140, 0, 160), 3, dash=True)
    p2.vline(wp, (0, 130, 80), 3, dash=True)
    draw_text(d, (p2.tx(wc)+8, p2.ty(phi_wc)-20), f"gamma≈{gamma:.1f}deg", fnt=SMALL)
    draw_text(d, (p2.tx(wp)+8, p2.ty(-180)+12), f"Kg≈{kg:.3g} ({20*math.log10(kg):.2f}dB)", fnt=SMALL)
    im.save(OUT / "fig_5_8_reference.png")
    return wc, gamma, wp, kg


def make_5_9():
    w = np.logspace(-2, 3, 1600)
    G = (1 + 0.2j*w) / ((1j*w)**2 * (1 + 0.02j*w))
    mag = 20 * np.log10(np.abs(G))
    phase = np.unwrap(np.angle(G)) * 180 / np.pi
    wc = crossing(w, np.abs(G), 1)
    phi = np.interp(np.log(wc), np.log(w), phase)
    gamma = 180 + phi
    D = 0.18**2 - 4 * 0.004
    wlow = (0.18 - math.sqrt(D)) / 0.008
    whigh = (0.18 + math.sqrt(D)) / 0.008
    k_low = wlow**2 * math.sqrt(1 + (0.02*wlow)**2) / math.sqrt(1 + (0.2*wlow)**2)
    k_high = whigh**2 * math.sqrt(1 + (0.02*whigh)**2) / math.sqrt(1 + (0.2*whigh)**2)

    im = canvas()
    d = ImageDraw.Draw(im)
    draw_text(d, (700, 24), "5-9 频率法求相角裕度与 K", fnt=TITLE, anchor="ma")
    p1 = Plot(d, (120, 90, 1320, 420), (0.03, 200), (-90, 90), logx=True)
    p1.axes([0.05, 0.1, 1, 6.5, 10, 50, 100], [-80, -40, 0, 40, 80], "omega / rad/s", "dB")
    p1.hline(0, (0, 0, 0))
    p1.line(w, mag, (0, 83, 170), 3)
    p1.vline(wc, (140, 0, 160), 3, dash=True)
    p1.vline(wlow, (215, 60, 55), 3, dash=True)
    draw_text(d, (p1.tx(wc)+8, p1.ty(5)), f"K=1: ωc≈{wc:.2f}", fnt=SMALL)
    draw_text(d, (p1.tx(wlow)+8, p1.ty(35)), f"PM=45°取低频解\nω≈{wlow:.2f}, K≈{k_low:.1f}", fnt=SMALL)

    p2 = Plot(d, (120, 510, 1320, 800), (0.03, 200), (-190, -80), logx=True)
    p2.axes([0.05, 0.1, 1, 6.5, 10, 50, 100], [-180, -160, -135, -120, -100], "omega / rad/s", "deg")
    p2.hline(-180, (0, 0, 0))
    p2.hline(-135, (215, 60, 55))
    p2.line(w, phase, (0, 83, 170), 3)
    p2.vline(wc, (140, 0, 160), 3, dash=True)
    p2.vline(wlow, (215, 60, 55), 3, dash=True)
    draw_text(d, (p2.tx(wc)+8, p2.ty(phi)-20), f"γ≈{gamma:.1f}°", fnt=SMALL)
    draw_text(d, (930, 822), f"严格还有高频解：ω≈{whigh:.2f}, K≈{k_high:.1f}；旧答案通常取低频解。", fnt=SMALL)
    im.save(OUT / "fig_5_9_reference.png")
    return wc, gamma, wlow, k_low, whigh, k_high


def make_5_13():
    w = np.logspace(-3, 3, 1600)
    G1 = (1 + 0.1j*w) / ((1j*w) * (1j*w - 1))
    G3 = 1 / ((1j*w) * (1 + 0.1j*w) * (1 + 0.25j*w))
    im = canvas(1500, 700)
    d = ImageDraw.Draw(im)
    draw_text(d, (750, 22), "5-13 Nyquist 草图和 K 范围", fnt=TITLE, anchor="ma")
    p1 = Plot(d, (90, 90, 710, 610), (-1.2, 0.4), (-1.0, 1.0))
    p1.axes([-1, -0.5, -0.1, 0], [-1, -0.5, 0, 0.5, 1], "Re", "Im")
    p1.line(np.real(G1), np.imag(G1), (0, 83, 170), 3)
    p1.line(np.real(G1), -np.imag(G1), (0, 83, 170), 2, dash=True)
    p1.marker(-0.1, 0)
    draw_text(d, (120, 95), "(1) P=1，临界点 -1/K=-0.1", fnt=SMALL)
    draw_text(d, (120, 125), "闭环稳定：K>10", fnt=SMALL, fill=(170, 0, 0))

    p2 = Plot(d, (810, 90, 1430, 610), (-0.25, 0.08), (-0.25, 0.25))
    p2.axes([-0.2, -1/14, -0.1, 0], [-0.2, -0.1, 0, 0.1, 0.2], "Re", "Im")
    p2.line(np.real(G3), np.imag(G3), (0, 83, 170), 3)
    p2.line(np.real(G3), -np.imag(G3), (0, 83, 170), 2, dash=True)
    p2.marker(-1/14, 0)
    draw_text(d, (840, 95), "(3) P=0，临界点 -1/K=-1/14", fnt=SMALL)
    draw_text(d, (840, 125), "闭环稳定：0<K<14", fnt=SMALL, fill=(170, 0, 0))
    im.save(OUT / "fig_5_13_reference.png")


def make_5_14():
    im = canvas(1400, 850)
    d = ImageDraw.Draw(im)
    draw_text(d, (700, 24), "5-14 Nyquist 图按 K 缩放判稳", fnt=TITLE, anchor="ma")
    # This is intentionally not to scale; it is a clean copying sketch of the
    # textbook Nyquist curve, with crowded points near the origin spread out.
    left, right, mid_y = 120, 1240, 350
    top, bottom = 90, 620
    xmap = {-50: 250, -20: 620, -1: 900, -0.05: 1110, 0: 1200}
    def sy(v):
        return mid_y - v * 8

    d.rectangle((left, top, right, bottom), outline=(40, 40, 40), width=2)
    d.line((left, mid_y, right, mid_y), fill=(0, 0, 0), width=2)
    d.line((xmap[0], bottom, xmap[0], top), fill=(0, 0, 0), width=2)
    d.polygon([(right, mid_y), (right-16, mid_y-6), (right-16, mid_y+6)], fill=(0, 0, 0))
    d.polygon([(xmap[0], top), (xmap[0]-6, top+16), (xmap[0]+6, top+16)], fill=(0, 0, 0))
    draw_text(d, (right-20, mid_y+18), "Re", fnt=SMALL)
    draw_text(d, (xmap[0]+12, top+8), "Im", fnt=SMALL)

    upper = [(xmap[-50], sy(0)), (340, sy(16)), (430, sy(24)), (560, sy(21)),
             (xmap[-20], sy(0)), (710, sy(-12)), (810, sy(-18)),
             (xmap[-1], sy(-9)), (1010, sy(12)), (xmap[-0.05], sy(0)), (xmap[0], sy(0))]
    lower = [(x, 2*mid_y-y) for x, y in upper[:-1]]
    d.line(upper, fill=(0, 83, 170), width=4, joint="curve")
    for i in range(len(lower)-1):
        if i % 2 == 0:
            d.line((lower[i], lower[i+1]), fill=(0, 83, 170), width=2)

    for val, label, dy in [(-50, "-50", 16), (-20, "-20", 16), (-1, "-1", -28), (-0.05, "-0.05", 16), (0, "0", 16)]:
        x = xmap[val]
        d.ellipse((x-6, mid_y-6, x+6, mid_y+6), outline=(0, 0, 0), fill="white", width=2)
        draw_text(d, (x, mid_y+dy), label, fnt=SMALL, anchor="ma")
    d.ellipse((xmap[-1]-9, mid_y-9, xmap[-1]+9, mid_y+9), fill=(220, 0, 0), outline=(0, 0, 0))
    draw_text(d, (xmap[-1]+18, mid_y-35), "(-1,j0)", fnt=SMALL, fill=(170, 0, 0))

    draw_text(d, (150, 650), "题图为 K=500。若改为一般 K，整条 Nyquist 曲线按 K/500 等比例缩放。", fnt=SMALL)
    draw_text(d, (150, 690), "由题图在负实轴的三处临界位置，可读出临界增益：K=10，25，10000。", fnt=SMALL)
    draw_text(d, (150, 730), "按 Nyquist 判据：不包围 (-1,j0) 时闭环稳定。", fnt=SMALL)
    draw_text(d, (150, 775), "稳定区间：0<K<10 或 25<K<10000。", fnt=FONT, fill=(170, 0, 0))
    im.save(OUT / "fig_5_14_reference.png")


def make_5_15():
    gamma = 36
    x = math.tan(math.radians(90 - gamma))
    wc = 100 / math.sqrt(1 + x*x)
    T = x / wc
    zeta = 1 / (20 * math.sqrt(T))
    Mr = 1 / (2 * zeta * math.sqrt(1 - zeta*zeta))
    wn = math.sqrt(100 / T)
    wr = wn * math.sqrt(1 - 2*zeta*zeta)
    w = np.logspace(0, 3, 1200)
    Phi = 100 / (T*(1j*w)**2 + 1j*w + 100)
    mag = np.abs(Phi)
    im = canvas()
    d = ImageDraw.Draw(im)
    draw_text(d, (700, 24), "5-15 闭环幅频特性和谐振峰值", fnt=TITLE, anchor="ma")
    p = Plot(d, (120, 90, 1320, 720), (1, 500), (0, 1.9), logx=True)
    p.axes([1, 2, 5, 10, 20, 50, 100, 200, 500], [0, 0.5, 1.0, 1.5], "omega / rad/s", "|Phi(jw)|")
    p.line(w, mag, (0, 83, 170), 4)
    p.marker(wr, Mr)
    p.vline(wr, (215, 60, 55), 3, dash=True)
    p.hline(Mr, (215, 60, 55), 3)
    draw_text(d, (p.tx(wr)+12, p.ty(Mr)-38), f"Mr≈{Mr:.2f}", fnt=FONT, fill=(170, 0, 0))
    draw_text(d, (p.tx(wr)+12, p.ty(Mr)-10), f"ωr≈{wr:.1f}", fnt=SMALL)
    draw_text(d, (130, 760), f"由 γ=36° 得 arctan(ωcT)=54°，T≈{T:.4f}s，ζ≈{zeta:.3f}。", fnt=FONT)
    im.save(OUT / "fig_5_15_reference.png")
    return T, zeta, Mr, wr


def write_solution(vals):
    wc58, gamma58, wp58, kg58 = vals["58"]
    wc59, gamma59, wlow, klow, whigh, khigh = vals["59"]
    T15, zeta15, Mr15, wr15 = vals["515"]
    text = f"""# 5-8、5-9、5-13、5-14、5-15 誊写稿

## 5-8
G(s)=10/[(0.2s+1)(s+2)(s+0.5)]
=10/[(0.2s+1)(s/2+1)2(s/0.5+1)0.5]，低频增益为 10/(2*0.5)=10，即 20dB。

转折频率为 0.5、2、5 rad/s。
幅频渐近线：低频 20dB；过 0.5 后斜率 -20dB/dec；过 2 后 -40dB/dec；过 5 后 -60dB/dec。

相频：
φ(ω)=-arctan(0.2ω)-arctan(0.5ω)-arctan(2ω)。

令 |G(jωc)|=1，得 ωc≈{wc58:.2f} rad/s。
此时 φ(ωc)≈-{180-gamma58:.2f}°，所以相角裕度 γ=180°+φ(ωc)≈{gamma58:.2f}°。

令 φ(ωg)=-180°，得 ωg≈{wp58:.2f} rad/s。
此时 |G(jωg)|≈{1/kg58:.3f}，幅值裕度 Kg=1/|G(jωg)|≈{kg58:.3f}，约 {20*math.log10(kg58):.2f}dB。

## 5-9
G(s)=K(0.2s+1)/[s^2(0.02s+1)]。

(1) K=1 时：
|G(jω)|=sqrt(1+0.04ω²)/(ω² sqrt(1+0.0004ω²))。
令 |G(jωc)|=1，得 ωc≈{wc59:.2f} rad/s。

φ(ω)=-180°+arctan(0.2ω)-arctan(0.02ω)。
代入 ωc 得 φ(ωc)≈-{180-gamma59:.2f}°，
所以相位裕度 γ≈{gamma59:.2f}°。

(2) 要求 γ=45°，则 φ(ωc)=-135°。
即 arctan(0.2ωc)-arctan(0.02ωc)=45°。
由 tan(A-B)=1 得：
0.18ω/(1+0.004ω²)=1，
解得低频交叉 ωc≈{wlow:.2f} rad/s。

再由 |G(jωc)|=1：
K=ωc² sqrt(1+0.0004ωc²)/sqrt(1+0.04ωc²)≈{klow:.1f}。
故按旧答案取 K≈25，较精确为 K≈25.9。
注：方程还有高频解 ωc≈{whigh:.2f}，K≈{khigh:.1f}，一般本题取低频解。

## 5-13
(1) G(s)=K(0.1s+1)/[s(s-1)]。
闭环特征方程：
s(s-1)+K(0.1s+1)=0，
即 s²+(0.1K-1)s+K=0。
二阶系统稳定要求各系数同号且正：
0.1K-1>0，K>0。
所以闭环稳定条件为 K>10；K<10 时闭环不稳定，K=10 为临界稳定。

(3) G(s)=K/[s(0.1s+1)(0.25s+1)]。
闭环特征方程：
s(0.1s+1)(0.25s+1)+K=0，
即 0.025s³+0.35s²+s+K=0。
三阶 Routh 判据：
0.35*1>0.025K，且 K>0。
所以 0<K<14 时闭环稳定；K=14 为临界稳定。

## 5-14
由题给 Nyquist 图知 p=0。
改变开环增益 K 时，Nyquist 曲线只按比例缩放。
根据图上曲线与负实轴的临界位置，可得三个临界增益：
K=10，K=25，K=10000。

按 Nyquist 判据，曲线不包围 (-1,j0) 时闭环稳定。
由图判断稳定区间为：
0<K<10 或 25<K<10000。
因此不稳定区间为 10<K<25 或 K>10000；
K=10、25、10000 为临界值。

## 5-15
G(s)=100/[s(Ts+1)]。
开环相角：
φ(ω)=-90°-arctan(ωT)。
相角裕度 γ=36°，故在截止频率处：
180°+φ(ωc)=36°，
即 90°-arctan(ωcT)=36°，
所以 arctan(ωcT)=54°，ωcT=tan54°。

又 |G(jωc)|=1：
100/[ωc sqrt(1+(ωcT)²)]=1。
由此 ωc≈{100/math.sqrt(1+math.tan(math.radians(54))**2):.2f} rad/s，
T≈{T15:.4f}s。

闭环传递函数：
Φ(s)=100/(Ts²+s+100)。
化为标准二阶形式可得
ωn=sqrt(100/T)，ζ=1/(20sqrt(T))≈{zeta15:.3f}。

谐振峰值：
Mr=1/[2ζsqrt(1-ζ²)]≈{Mr15:.2f}。
"""
    (OUT / "homework_5_8_5_15_solution.md").write_text(text, encoding="utf-8-sig")


def main():
    vals = {}
    vals["58"] = make_5_8()
    vals["59"] = make_5_9()
    make_5_13()
    make_5_14()
    vals["515"] = make_5_15()
    write_solution(vals)


if __name__ == "__main__":
    main()
