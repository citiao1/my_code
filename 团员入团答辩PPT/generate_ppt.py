from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
from xml.sax.saxutils import escape

from PIL import Image, ImageDraw, ImageFont

OUT_DIR = Path(__file__).resolve().parent
ASSET_DIR = OUT_DIR / "assets"
PPTX_PATH = OUT_DIR / "团员入团答辩-傅思雄-美化版.pptx"

EMU = 914400
SLIDE_W = 13.333333
SLIDE_H = 7.5

RED = "C1121F"
DARK_RED = "7F1D1D"
INK = "111827"
MUTED = "64748B"
BLUE = "1D4ED8"
LIGHT_BG = "F8FAFC"
CARD = "FFFFFF"
PALE_RED = "FEE2E2"
PALE_BLUE = "DBEAFE"

SLIDES = [
    {
        "kind": "cover",
        "title": "团员入团答辩",
        "subtitle": "以青年担当融入集体，以实践行动靠近团组织",
        "meta": "自控2401  傅思雄",
        "image": "cover.png",
    },
    {
        "no": "01",
        "title": "个人基本情况",
        "lead": "从基础事务做起，在学习与实践中靠近先进集体。",
        "bullets": ["绩点：3.3 / 4.33", "实践身份：实验室助理", "正式任职不多，但重视执行与协作", "希望用稳定行动服务集体"],
        "tag": "踏实 · 主动 · 责任感",
        "image": "profile.png",
    },
    {
        "no": "02",
        "title": "学习与科创实践",
        "lead": "把课堂知识带到真实问题里，在竞赛中训练解决问题能力。",
        "bullets": ["蓝桥杯：省级一等奖", "国赛成绩：等待公布", "正在参与智能车相关实践", "持续训练编程、调试和复盘能力"],
        "tag": "专业学习 · 科创竞赛 · 实践能力",
        "image": "competition.png",
    },
    {
        "no": "03",
        "title": "实验室助理经历",
        "lead": "在具体事务中学习服务意识，也在细节里培养责任心。",
        "bullets": ["协助实验室日常整理与维护", "配合设备、资料和实验准备", "在基础事务中保持耐心和细致", "理解“小事做好”也是服务集体"],
        "tag": "服务意识 · 协作意识 · 执行力",
        "image": "lab.png",
    },
    {
        "no": "04",
        "title": "对共青团的认识",
        "lead": "共青团不只是身份，更是先进青年应有的责任标准。",
        "bullets": ["思想上积极向上，主动靠近组织", "学习中追求进步，保持自我要求", "集体中愿意服务他人、承担事务", "实践中敢于尝试，把成长融入需要"],
        "tag": "先进性 · 服务性 · 实践性",
        "image": "youth.png",
    },
    {
        "no": "05",
        "title": "入团后的努力方向",
        "lead": "用更高标准要求自己，把认识落实到学习、实践和服务中。",
        "bullets": ["保持学习稳定进步，补齐不足", "继续投入智能车、蓝桥杯等科创实践", "积极参与班级、实验室和学校活动", "主动承担任务，接受组织和同学监督"],
        "tag": "汇报完毕，谢谢大家",
        "image": "plan.png",
    },
]


def e(value):
    return int(value * EMU)


def c(value):
    return value.replace("#", "").upper()


def try_font(size, bold=False):
    candidates = [
        "C:/Windows/Fonts/msyhbd.ttc" if bold else "C:/Windows/Fonts/msyh.ttc",
        "C:/Windows/Fonts/simhei.ttf",
        "C:/Windows/Fonts/simsun.ttc",
    ]
    for candidate in candidates:
        try:
            return ImageFont.truetype(candidate, size)
        except OSError:
            continue
    return ImageFont.load_default()


def hex_to_rgb(value):
    value = value.replace("#", "")
    return tuple(int(value[i : i + 2], 16) for i in (0, 2, 4))


def rounded_rect(draw, xy, radius, fill, outline=None, width=1):
    draw.rounded_rectangle(xy, radius=radius, fill=fill, outline=outline, width=width)


def generate_images():
    ASSET_DIR.mkdir(exist_ok=True)
    font_big = try_font(64, True)
    font_mid = try_font(42, True)
    font_small = try_font(30)
    red = hex_to_rgb(RED)
    dark = hex_to_rgb(DARK_RED)
    blue = hex_to_rgb(BLUE)
    pale_red = hex_to_rgb(PALE_RED)
    pale_blue = hex_to_rgb(PALE_BLUE)
    ink = hex_to_rgb(INK)
    muted = hex_to_rgb(MUTED)

    def save(name, drawer):
        image = Image.new("RGBA", (1600, 1000), (0, 0, 0, 0))
        draw = ImageDraw.Draw(image)
        drawer(draw)
        image.save(ASSET_DIR / name)

    def draw_cover(draw):
        draw.rounded_rectangle((120, 120, 1480, 880), radius=70, fill=(255, 255, 255, 235))
        draw.polygon([(300, 210), (1230, 170), (1370, 610), (450, 690)], fill=red + (235,))
        draw.polygon([(360, 270), (1180, 240), (1280, 530), (470, 575)], fill=(185, 28, 28, 235))
        for x, y, r in [(410, 320, 22), (480, 285, 12), (520, 360, 10), (575, 315, 8)]:
            draw.ellipse((x-r, y-r, x+r, y+r), fill=(255, 255, 255, 235))
        draw.line((400, 690, 410, 830), fill=dark + (255,), width=18)
        draw.rounded_rectangle((280, 820, 560, 860), radius=18, fill=pale_red + (255,))
        for i in range(5):
            x = 720 + i * 80
            draw.rounded_rectangle((x, 700, x + 44, 810), radius=12, fill=pale_blue + (255,))
            draw.rectangle((x + 10, 730, x + 34, 760), fill=(255, 255, 255, 255))
        draw.text((650, 390), "Youth League", font=font_big, fill=(255, 255, 255, 235))

    def draw_profile(draw):
        rounded_rect(draw, (170, 180, 1080, 760), 48, (255, 255, 255, 255), (226, 232, 240, 255), 4)
        draw.rounded_rectangle((250, 260, 500, 510), radius=40, fill=pale_red + (255,))
        draw.ellipse((315, 305, 435, 425), fill=red + (255,))
        draw.rounded_rectangle((285, 438, 465, 520), radius=35, fill=red + (255,))
        draw.text((580, 270), "个人基本情况", font=font_mid, fill=ink + (255,))
        for i, text in enumerate(["GPA 3.3/4.33", "实验室助理", "科创竞赛实践"]):
            y = 390 + i * 90
            draw.rounded_rectangle((580, y, 980, y + 56), radius=22, fill=(248, 250, 252, 255))
            draw.ellipse((606, y + 17, 628, y + 39), fill=blue + (255,))
            draw.text((650, y + 10), text, font=font_small, fill=muted + (255,))
        draw.arc((1060, 130, 1420, 490), 210, 25, fill=pale_red + (255,), width=28)
        draw.arc((1120, 260, 1500, 660), 200, 30, fill=pale_blue + (255,), width=20)

    def draw_competition(draw):
        for x, h, fill in [(320, 250, pale_blue), (500, 360, red), (680, 300, pale_red)]:
            draw.rounded_rectangle((x, 690 - h, x + 130, 690), radius=24, fill=fill + (255,))
        draw.text((520, 260), "省一", font=font_big, fill=red + (255,))
        draw.text((825, 360), "{ code }", font=font_mid, fill=blue + (255,))
        draw.line((250, 760, 1180, 760), fill=(203, 213, 225, 255), width=14)
        draw.rounded_rectangle((900, 610, 1220, 720), radius=55, fill=ink + (255,))
        draw.rectangle((980, 560, 1145, 625), fill=ink + (255,))
        for x in [970, 1150]:
            draw.ellipse((x, 700, x + 92, 792), fill=(15, 23, 42, 255))
            draw.ellipse((x + 28, 728, x + 64, 764), fill=(226, 232, 240, 255))
        draw.line((250, 585, 410, 510, 590, 540, 750, 430), fill=blue + (255,), width=16)
        draw.ellipse((735, 415, 780, 460), fill=blue + (255,))

    def draw_lab(draw):
        rounded_rect(draw, (210, 230, 1280, 760), 42, (255, 255, 255, 255), (226, 232, 240, 255), 4)
        draw.rounded_rectangle((340, 330, 760, 600), radius=28, fill=pale_blue + (255,))
        for x in [405, 520, 635]:
            for y in [385, 500]:
                draw.ellipse((x, y, x + 42, y + 42), fill=blue + (255,))
        draw.line((447, 406, 520, 406, 520, 521, 635, 521), fill=blue + (255,), width=10)
        draw.line((447, 521, 635, 406), fill=blue + (255,), width=10)
        draw.rounded_rectangle((870, 330, 1130, 610), radius=28, fill=pale_red + (255,))
        draw.rectangle((925, 385, 1075, 540), fill=(255, 255, 255, 255))
        draw.text((915, 645), "整理  调试  协作", font=font_small, fill=muted + (255,))
        draw.rectangle((250, 760, 1320, 800), fill=dark + (255,))

    def draw_youth(draw):
        centers = [(420, 480, "先进性", red), (760, 480, "服务性", blue), (1100, 480, "实践性", dark)]
        for x, y, label, fill in centers:
            draw.ellipse((x - 135, y - 135, x + 135, y + 135), fill=fill + (240,))
            draw.ellipse((x - 92, y - 92, x + 92, y + 92), fill=(255, 255, 255, 255))
            bbox = draw.textbbox((0, 0), label, font=font_mid)
            draw.text((x - (bbox[2] - bbox[0]) / 2, y - 26), label, font=font_mid, fill=fill + (255,))
        draw.line((550, 480, 625, 480), fill=(203, 213, 225, 255), width=10)
        draw.line((895, 480, 970, 480), fill=(203, 213, 225, 255), width=10)
        draw.polygon([(460, 230), (1030, 210), (1120, 330), (530, 350)], fill=pale_red + (255,))
        draw.text((600, 250), "青年责任", font=font_big, fill=red + (255,))

    def draw_plan(draw):
        points = [(260, 720), (520, 590), (760, 600), (1010, 430), (1250, 300)]
        draw.line(points, fill=blue + (255,), width=18, joint="curve")
        for i, (x, y) in enumerate(points, start=1):
            draw.ellipse((x - 45, y - 45, x + 45, y + 45), fill=(255, 255, 255, 255), outline=blue + (255,), width=10)
            draw.text((x - 12, y - 22), str(i), font=font_small, fill=blue + (255,))
        draw.polygon([(1240, 260), (1360, 295), (1265, 370)], fill=red + (255,))
        labels = ["学习", "竞赛", "服务", "担当", "进步"]
        for (x, y), label in zip(points, labels):
            draw.rounded_rectangle((x - 70, y + 70, x + 70, y + 118), radius=18, fill=(255, 255, 255, 235), outline=(226, 232, 240, 255), width=3)
            bbox = draw.textbbox((0, 0), label, font=font_small)
            draw.text((x - (bbox[2] - bbox[0]) / 2, y + 78), label, font=font_small, fill=muted + (255,))

    save("cover.png", draw_cover)
    save("profile.png", draw_profile)
    save("competition.png", draw_competition)
    save("lab.png", draw_lab)
    save("youth.png", draw_youth)
    save("plan.png", draw_plan)


def run_xml(text, size=2200, bold=False, fill=INK):
    bold_xml = "<a:b/>" if bold else ""
    return (
        f'<a:r><a:rPr lang="zh-CN" sz="{size}" dirty="0">'
        f'<a:solidFill><a:srgbClr val="{c(fill)}"/></a:solidFill>{bold_xml}'
        f'<a:latin typeface="Microsoft YaHei"/><a:ea typeface="Microsoft YaHei"/></a:rPr>'
        f'<a:t>{escape(text)}</a:t></a:r>'
    )


def p_xml(text, size=2200, bold=False, fill=INK, align="l"):
    return f'<a:p><a:pPr algn="{align}"/>{run_xml(text, size, bold, fill)}</a:p>'


def bullet_xml(items):
    body = []
    for item in items:
        body.append(
            '<a:p><a:pPr marL="260000" indent="-155000"><a:buChar char="•"/></a:pPr>'
            f'{run_xml(item, 1800, False, "334155")}</a:p>'
        )
    return "".join(body)


def shape_xml(shape_id, x, y, w, h, fill, line=None, prst="rect", alpha=None):
    line = line or fill
    alpha_xml = f'<a:alpha val="{alpha}"/>' if alpha else ""
    return f'''
<p:sp>
  <p:nvSpPr><p:cNvPr id="{shape_id}" name="Shape {shape_id}"/><p:cNvSpPr/><p:nvPr/></p:nvSpPr>
  <p:spPr><a:xfrm><a:off x="{e(x)}" y="{e(y)}"/><a:ext cx="{e(w)}" cy="{e(h)}"/></a:xfrm><a:prstGeom prst="{prst}"><a:avLst/></a:prstGeom><a:solidFill><a:srgbClr val="{c(fill)}">{alpha_xml}</a:srgbClr></a:solidFill><a:ln><a:solidFill><a:srgbClr val="{c(line)}"/></a:solidFill></a:ln></p:spPr>
  <p:txBody><a:bodyPr/><a:lstStyle/><a:p/></p:txBody>
</p:sp>'''


def text_xml(shape_id, x, y, w, h, text, fill="FFFFFF", line="FFFFFF", prst="rect"):
    return f'''
<p:sp>
  <p:nvSpPr><p:cNvPr id="{shape_id}" name="TextBox {shape_id}"/><p:cNvSpPr txBox="1"/><p:nvPr/></p:nvSpPr>
  <p:spPr><a:xfrm><a:off x="{e(x)}" y="{e(y)}"/><a:ext cx="{e(w)}" cy="{e(h)}"/></a:xfrm><a:prstGeom prst="{prst}"><a:avLst/></a:prstGeom><a:solidFill><a:srgbClr val="{c(fill)}"/></a:solidFill><a:ln><a:solidFill><a:srgbClr val="{c(line)}"/></a:solidFill></a:ln></p:spPr>
  <p:txBody><a:bodyPr wrap="square" rtlCol="0"><a:spAutoFit/></a:bodyPr><a:lstStyle/>{text}</p:txBody>
</p:sp>'''


def image_xml(shape_id, rel_id, name, x, y, w, h):
    return f'''
<p:pic>
  <p:nvPicPr><p:cNvPr id="{shape_id}" name="{escape(name)}"/><p:cNvPicPr><a:picLocks noChangeAspect="1"/></p:cNvPicPr><p:nvPr/></p:nvPicPr>
  <p:blipFill><a:blip r:embed="{rel_id}"/><a:stretch><a:fillRect/></a:stretch></p:blipFill>
  <p:spPr><a:xfrm><a:off x="{e(x)}" y="{e(y)}"/><a:ext cx="{e(w)}" cy="{e(h)}"/></a:xfrm><a:prstGeom prst="rect"><a:avLst/></a:prstGeom></p:spPr>
</p:pic>'''


def common_slide_bg(page_num):
    return "".join([
        shape_xml(2, 0, 0, SLIDE_W, SLIDE_H, LIGHT_BG),
        shape_xml(3, 0, 0, SLIDE_W, 0.12, RED),
        shape_xml(4, 0, 7.05, SLIDE_W, 0.45, DARK_RED),
        shape_xml(5, 0.62, 0.58, 0.18, 5.95, RED),
        shape_xml(6, 10.95, -0.48, 2.8, 2.8, "FCA5A5", prst="ellipse", alpha="17000"),
        shape_xml(7, 11.78, 0.55, 1.2, 1.2, "DBEAFE", prst="ellipse", alpha="45000"),
        text_xml(92, 0.65, 7.17, 5.4, 0.18, p_xml("共青团入团答辩 · 个人汇报", 760, False, "FEE2E2"), DARK_RED, DARK_RED),
        text_xml(93, 11.95, 7.17, 0.5, 0.18, p_xml(str(page_num).zfill(2), 760, False, "FEE2E2", "r"), DARK_RED, DARK_RED),
    ])


def slide_xml(slide, idx, media_rel):
    shapes = [common_slide_bg(idx + 1)]
    if slide.get("kind") == "cover":
        shapes.append(text_xml(8, 0.95, 1.2, 1.9, 0.38, p_xml("申请入团", 1180, True, RED, "ctr"), PALE_RED, PALE_RED, "roundRect"))
        shapes.append(text_xml(9, 0.95, 1.92, 6.1, 0.74, p_xml(slide["title"], 3800, True, INK), LIGHT_BG, LIGHT_BG))
        shapes.append(text_xml(10, 0.98, 2.9, 6.45, 0.55, p_xml(slide["subtitle"], 1750, False, "475569"), LIGHT_BG, LIGHT_BG))
        shapes.append(shape_xml(11, 0.98, 3.78, 1.35, 0.05, BLUE))
        shapes.append(text_xml(12, 0.98, 5.52, 3.0, 0.34, p_xml(slide["meta"], 1350, False, "475569"), LIGHT_BG, LIGHT_BG))
        shapes.append(text_xml(13, 4.2, 5.42, 2.2, 0.52, p_xml("2分钟陈述", 1250, True, "FFFFFF", "ctr"), RED, RED, "roundRect"))
        shapes.append(shape_xml(14, 7.15, 0.9, 5.45, 5.45, "FFFFFF", "E2E8F0", "roundRect"))
        shapes.append(image_xml(15, media_rel, slide["image"], 7.45, 1.24, 4.86, 3.04))
    else:
        shapes.append(text_xml(8, 0.95, 0.72, 0.75, 0.45, p_xml(slide["no"], 1550, True, "FFFFFF", "ctr"), RED, RED, "roundRect"))
        shapes.append(text_xml(9, 1.95, 0.65, 5.65, 0.52, p_xml(slide["title"], 2500, True, INK), LIGHT_BG, LIGHT_BG))
        shapes.append(text_xml(10, 0.98, 1.48, 6.0, 0.63, p_xml(slide["lead"], 1450, False, "475569"), LIGHT_BG, LIGHT_BG))
        shapes.append(shape_xml(11, 0.98, 2.38, 6.25, 3.25, CARD, "E2E8F0", "roundRect"))
        shapes.append(text_xml(12, 1.22, 2.72, 5.65, 2.3, bullet_xml(slide["bullets"]), CARD, CARD))
        shapes.append(text_xml(13, 1.22, 5.38, 4.95, 0.42, p_xml(slide["tag"], 1180, True, RED), CARD, CARD))
        shapes.append(shape_xml(14, 7.8, 1.25, 4.55, 4.35, "FFFFFF", "E2E8F0", "roundRect"))
        shapes.append(image_xml(15, media_rel, slide["image"], 8.03, 1.55, 4.08, 2.55))
        shapes.append(shape_xml(16, 7.52, 5.88, 0.72, 0.08, RED))
        shapes.append(text_xml(17, 8.38, 5.66, 3.35, 0.48, p_xml("答辩关键词", 1050, True, "475569"), LIGHT_BG, LIGHT_BG))
    return f'''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<p:sld xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main">
  <p:cSld><p:spTree><p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="0" cy="0"/><a:chOff x="0" y="0"/><a:chExt cx="0" cy="0"/></a:xfrm></p:grpSpPr>{''.join(shapes)}</p:spTree></p:cSld>
  <p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>
</p:sld>'''


def theme_xml():
    return '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<a:theme xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" name="YouthLeague"><a:themeElements><a:clrScheme name="YouthLeague"><a:dk1><a:srgbClr val="111827"/></a:dk1><a:lt1><a:srgbClr val="FFFFFF"/></a:lt1><a:dk2><a:srgbClr val="334155"/></a:dk2><a:lt2><a:srgbClr val="F8FAFC"/></a:lt2><a:accent1><a:srgbClr val="C1121F"/></a:accent1><a:accent2><a:srgbClr val="1D4ED8"/></a:accent2><a:accent3><a:srgbClr val="7F1D1D"/></a:accent3><a:accent4><a:srgbClr val="FEE2E2"/></a:accent4><a:accent5><a:srgbClr val="DBEAFE"/></a:accent5><a:accent6><a:srgbClr val="64748B"/></a:accent6><a:hlink><a:srgbClr val="2563EB"/></a:hlink><a:folHlink><a:srgbClr val="7C3AED"/></a:folHlink></a:clrScheme><a:fontScheme name="Microsoft YaHei"><a:majorFont><a:latin typeface="Microsoft YaHei"/><a:ea typeface="Microsoft YaHei"/><a:cs typeface="Microsoft YaHei"/></a:majorFont><a:minorFont><a:latin typeface="Microsoft YaHei"/><a:ea typeface="Microsoft YaHei"/><a:cs typeface="Microsoft YaHei"/></a:minorFont></a:fontScheme><a:fmtScheme name="Office"><a:fillStyleLst><a:solidFill><a:schemeClr val="phClr"/></a:solidFill><a:solidFill><a:schemeClr val="phClr"/></a:solidFill><a:solidFill><a:schemeClr val="phClr"/></a:solidFill></a:fillStyleLst><a:lnStyleLst><a:ln w="6350"><a:solidFill><a:schemeClr val="phClr"/></a:solidFill></a:ln><a:ln w="12700"><a:solidFill><a:schemeClr val="phClr"/></a:solidFill></a:ln><a:ln w="19050"><a:solidFill><a:schemeClr val="phClr"/></a:solidFill></a:ln></a:lnStyleLst><a:effectStyleLst><a:effectStyle><a:effectLst/></a:effectStyle><a:effectStyle><a:effectLst/></a:effectStyle><a:effectStyle><a:effectLst/></a:effectStyle></a:effectStyleLst><a:bgFillStyleLst><a:solidFill><a:schemeClr val="phClr"/></a:solidFill><a:solidFill><a:schemeClr val="phClr"/></a:solidFill><a:solidFill><a:schemeClr val="phClr"/></a:solidFill></a:bgFillStyleLst></a:fmtScheme></a:themeElements><a:objectDefaults/><a:extraClrSchemeLst/></a:theme>'''


def master_xml():
    return '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<p:sldMaster xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main"><p:cSld><p:spTree><p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="0" cy="0"/><a:chOff x="0" y="0"/><a:chExt cx="0" cy="0"/></a:xfrm></p:grpSpPr></p:spTree></p:cSld><p:clrMap bg1="lt1" tx1="dk1" bg2="lt2" tx2="dk2" accent1="accent1" accent2="accent2" accent3="accent3" accent4="accent4" accent5="accent5" accent6="accent6" hlink="hlink" folHlink="folHlink"/><p:sldLayoutIdLst><p:sldLayoutId id="2147483649" r:id="rId1"/></p:sldLayoutIdLst><p:txStyles><p:titleStyle/><p:bodyStyle/><p:otherStyle/></p:txStyles></p:sldMaster>'''


def layout_xml():
    return '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<p:sldLayout xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main" type="blank" preserve="1"><p:cSld name="Blank"><p:spTree><p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="0" cy="0"/><a:chOff x="0" y="0"/><a:chExt cx="0" cy="0"/></a:xfrm></p:grpSpPr></p:spTree></p:cSld><p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr></p:sldLayout>'''


def rels(entries):
    body = "".join(f'<Relationship Id="{rid}" Type="{typ}" Target="{target}"/>' for rid, typ, target in entries)
    return f'<?xml version="1.0" encoding="UTF-8" standalone="yes"?><Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">{body}</Relationships>'


def build_package():
    generate_images()
    n = len(SLIDES)
    slide_overrides = "".join(f'<Override PartName="/ppt/slides/slide{i}.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slide+xml"/>' for i in range(1, n + 1))
    content_types = f'''<?xml version="1.0" encoding="UTF-8" standalone="yes"?><Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/><Default Extension="xml" ContentType="application/xml"/><Default Extension="png" ContentType="image/png"/><Override PartName="/ppt/presentation.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml"/><Override PartName="/ppt/slideMasters/slideMaster1.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slideMaster+xml"/><Override PartName="/ppt/slideLayouts/slideLayout1.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slideLayout+xml"/><Override PartName="/ppt/theme/theme1.xml" ContentType="application/vnd.openxmlformats-officedocument.theme+xml"/><Override PartName="/ppt/presProps.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.presProps+xml"/><Override PartName="/ppt/viewProps.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.viewProps+xml"/><Override PartName="/ppt/tableStyles.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.tableStyles+xml"/><Override PartName="/docProps/core.xml" ContentType="application/vnd.openxmlformats-package.core-properties+xml"/><Override PartName="/docProps/app.xml" ContentType="application/vnd.openxmlformats-officedocument.extended-properties+xml"/>{slide_overrides}</Types>'''
    slide_ids = "".join(f'<p:sldId id="{255 + i}" r:id="rId{i + 1}"/>' for i in range(1, n + 1))
    presentation = f'''<?xml version="1.0" encoding="UTF-8" standalone="yes"?><p:presentation xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main" saveSubsetFonts="1"><p:sldMasterIdLst><p:sldMasterId id="2147483648" r:id="rId1"/></p:sldMasterIdLst><p:sldIdLst>{slide_ids}</p:sldIdLst><p:sldSz cx="12192000" cy="6858000" type="wide"/><p:notesSz cx="6858000" cy="9144000"/><p:defaultTextStyle><a:defPPr><a:defRPr lang="zh-CN"><a:latin typeface="Microsoft YaHei"/><a:ea typeface="Microsoft YaHei"/></a:defRPr></a:defPPr></p:defaultTextStyle></p:presentation>'''
    pres_rels_entries = [("rId1", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster", "slideMasters/slideMaster1.xml")]
    pres_rels_entries += [(f"rId{i + 1}", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide", f"slides/slide{i}.xml") for i in range(1, n + 1)]
    pres_rels_entries += [
        (f"rId{n + 2}", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/presProps", "presProps.xml"),
        (f"rId{n + 3}", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/viewProps", "viewProps.xml"),
        (f"rId{n + 4}", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/tableStyles", "tableStyles.xml"),
    ]
    files = {
        "[Content_Types].xml": content_types,
        "_rels/.rels": rels([
            ("rId1", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument", "ppt/presentation.xml"),
            ("rId2", "http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties", "docProps/core.xml"),
            ("rId3", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties", "docProps/app.xml"),
        ]),
        "ppt/presentation.xml": presentation,
        "ppt/_rels/presentation.xml.rels": rels(pres_rels_entries),
        "ppt/slideMasters/slideMaster1.xml": master_xml(),
        "ppt/slideMasters/_rels/slideMaster1.xml.rels": rels([
            ("rId1", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout", "../slideLayouts/slideLayout1.xml"),
            ("rId2", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme", "../theme/theme1.xml"),
        ]),
        "ppt/slideLayouts/slideLayout1.xml": layout_xml(),
        "ppt/slideLayouts/_rels/slideLayout1.xml.rels": rels([("rId1", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster", "../slideMasters/slideMaster1.xml")]),
        "ppt/theme/theme1.xml": theme_xml(),
        "ppt/presProps.xml": '<?xml version="1.0" encoding="UTF-8" standalone="yes"?><p:presentationPr xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main"/>',
        "ppt/viewProps.xml": '<?xml version="1.0" encoding="UTF-8" standalone="yes"?><p:viewPr xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main"/>',
        "ppt/tableStyles.xml": '<?xml version="1.0" encoding="UTF-8" standalone="yes"?><a:tblStyleLst xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" def="{5C22544A-7EE6-4342-B048-85BDC9FD1C3A}"/>',
        "docProps/app.xml": f'<?xml version="1.0" encoding="UTF-8" standalone="yes"?><Properties xmlns="http://schemas.openxmlformats.org/officeDocument/2006/extended-properties" xmlns:vt="http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes"><Application>Codex</Application><PresentationFormat>宽屏</PresentationFormat><Slides>{n}</Slides><Company/></Properties>',
        "docProps/core.xml": '<?xml version="1.0" encoding="UTF-8" standalone="yes"?><cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:dcterms="http://purl.org/dc/terms/" xmlns:dcmitype="http://purl.org/dc/dcmitype/" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"><dc:title>团员入团答辩-傅思雄-美化版</dc:title><dc:creator>Codex</dc:creator><cp:lastModifiedBy>Codex</cp:lastModifiedBy></cp:coreProperties>',
    }
    for i, slide in enumerate(SLIDES, start=1):
        image_name = slide["image"]
        files[f"ppt/slides/slide{i}.xml"] = slide_xml(slide, i - 1, "rId2")
        files[f"ppt/slides/_rels/slide{i}.xml.rels"] = rels([
            ("rId1", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout", "../slideLayouts/slideLayout1.xml"),
            ("rId2", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/image", f"../media/image{i}.png"),
        ])
        files[f"ppt/media/image{i}.png"] = (ASSET_DIR / image_name).read_bytes()
    return files


def main():
    with ZipFile(PPTX_PATH, "w", ZIP_DEFLATED) as archive:
        for name, data in build_package().items():
            if isinstance(data, bytes):
                archive.writestr(name, data)
            else:
                archive.writestr(name, data.encode("utf-8"))
    print(PPTX_PATH)


if __name__ == "__main__":
    main()
