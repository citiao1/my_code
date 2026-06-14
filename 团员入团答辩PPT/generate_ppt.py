from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
from xml.sax.saxutils import escape

from PIL import Image, ImageDraw, ImageFont

OUT_DIR = Path(__file__).resolve().parent
ASSET_DIR = OUT_DIR / "assets_refined"
PPTX_PATH = OUT_DIR / "团员入团答辩-傅思雄-精修版.pptx"

SLIDE_W_PX = 1920
SLIDE_H_PX = 1080

RED = "#B91C1C"
DARK_RED = "#7F1D1D"
INK = "#111827"
TEXT = "#334155"
MUTED = "#64748B"
LINE = "#E5E7EB"
SOFT_RED = "#FEE2E2"
SOFT_BLUE = "#DBEAFE"
BLUE = "#2563EB"
BG = "#F8FAFC"
WHITE = "#FFFFFF"

SLIDES = [
    {
        "title": "团员入团答辩",
        "eyebrow": "申请入团 · 个人汇报",
        "subtitle": "以青年担当融入集体，以实践行动靠近团组织",
        "meta": "自控2401  傅思雄",
        "type": "cover",
    },
    {
        "num": "01",
        "title": "个人基本情况",
        "lead": "从基础事务做起，在学习与实践中靠近先进集体。",
        "points": [
            ("学习情况", "绩点 3.3 / 4.33"),
            ("实践身份", "实验室助理"),
            ("个人特点", "踏实完成任务，重视执行与协作"),
        ],
        "quote": "任职经历不多，但希望用稳定行动服务集体。",
        "type": "profile",
    },
    {
        "num": "02",
        "title": "学习与科创实践",
        "lead": "把课堂知识带到真实问题里，在竞赛训练中提升解决问题能力。",
        "points": [
            ("蓝桥杯", "省级一等奖，国赛成绩待公布"),
            ("智能车", "正在参与相关训练与实践"),
            ("能力提升", "编程、调试、复盘和持续学习"),
        ],
        "quote": "竞赛让我更清楚地认识到，青年学生既要会学，也要会用。",
        "type": "competition",
    },
    {
        "num": "03",
        "title": "实验室助理经历",
        "lead": "在具体事务中学习服务意识，也在细节里培养责任心。",
        "points": [
            ("日常协助", "整理、维护、配合实验准备"),
            ("协作意识", "服从安排，及时反馈问题"),
            ("责任理解", "基础事务同样需要耐心和标准"),
        ],
        "quote": "把小事做好，也是服务集体的一种方式。",
        "type": "lab",
    },
    {
        "num": "04",
        "title": "对共青团的认识",
        "lead": "共青团不只是一个身份，更是先进青年应有的责任标准。",
        "points": [
            ("先进性", "思想上积极向上，主动靠近组织"),
            ("服务性", "集体中愿意服务他人、承担事务"),
            ("实践性", "实践中敢于尝试，把成长融入需要"),
        ],
        "quote": "申请入团，是希望用更高标准要求自己。",
        "type": "youth",
    },
    {
        "num": "05",
        "title": "入团后的努力方向",
        "lead": "把认识落实到学习、实践和服务中，接受组织和同学监督。",
        "points": [
            ("学习上", "保持稳定进步，补齐不足"),
            ("实践上", "继续投入智能车、蓝桥杯等科创训练"),
            ("集体中", "积极参与活动，在需要时主动承担任务"),
        ],
        "quote": "汇报完毕，谢谢大家。",
        "type": "plan",
    },
]


def rgb(value):
    value = value.replace("#", "")
    return tuple(int(value[i : i + 2], 16) for i in (0, 2, 4))


def font(size, bold=False):
    choices = [
        "C:/Windows/Fonts/msyhbd.ttc" if bold else "C:/Windows/Fonts/msyh.ttc",
        "C:/Windows/Fonts/simhei.ttf",
        "C:/Windows/Fonts/simsun.ttc",
    ]
    for item in choices:
        try:
            return ImageFont.truetype(item, size)
        except OSError:
            pass
    return ImageFont.load_default()


F = {
    "title": font(76, True),
    "cover": font(104, True),
    "h2": font(38, True),
    "body": font(30),
    "body_bold": font(30, True),
    "small": font(24),
    "tiny": font(20),
    "num": font(54, True),
}


def text(draw, xy, content, fnt, fill=INK, anchor=None, align="left"):
    draw.text(xy, content, font=fnt, fill=rgb(fill), anchor=anchor, align=align)


def rounded(draw, box, radius=28, fill=WHITE, outline=None, width=2):
    draw.rounded_rectangle(box, radius=radius, fill=rgb(fill), outline=rgb(outline) if outline else None, width=width)


def wrap_lines(draw, content, fnt, max_width):
    lines = []
    current = ""
    for char in content:
        candidate = current + char
        if draw.textlength(candidate, font=fnt) <= max_width:
            current = candidate
        else:
            if current:
                lines.append(current)
            current = char
    if current:
        lines.append(current)
    return lines


def draw_grid(draw):
    for x in range(120, SLIDE_W_PX, 120):
        draw.line((x, 0, x, SLIDE_H_PX), fill=(241, 245, 249), width=1)
    for y in range(120, SLIDE_H_PX, 120):
        draw.line((0, y, SLIDE_W_PX, y), fill=(241, 245, 249), width=1)


def draw_header(draw, slide):
    text(draw, (140, 90), slide["num"], F["num"], RED)
    draw.line((140, 158, 220, 158), fill=rgb(RED), width=8)
    text(draw, (260, 86), slide["title"], F["title"], INK)
    for idx, line in enumerate(wrap_lines(draw, slide["lead"], F["body"], 880)):
        text(draw, (264, 190 + idx * 42), line, F["body"], MUTED)


def draw_footer(draw, page_no):
    draw.rectangle((0, 1018, SLIDE_W_PX, SLIDE_H_PX), fill=rgb(DARK_RED))
    text(draw, (140, 1038), "共青团入团答辩 · 个人汇报", F["tiny"], "#FEE2E2")
    text(draw, (1780, 1038), f"{page_no:02d}", F["tiny"], "#FEE2E2")


def draw_side_mark(draw, label):
    rounded(draw, (1370, 112, 1690, 168), 20, SOFT_RED)
    text(draw, (1530, 124), label, F["small"], RED, anchor="ma")


def draw_points(draw, points):
    y = 330
    for idx, (label, value) in enumerate(points):
        top = y + idx * 152
        rounded(draw, (150, top, 1030, top + 104), 26, WHITE, LINE, 2)
        draw.ellipse((186, top + 32, 226, top + 72), fill=rgb(RED if idx == 0 else BLUE if idx == 1 else DARK_RED))
        text(draw, (252, top + 22), label, F["body_bold"], INK)
        text(draw, (252, top + 62), value, F["body"], TEXT)


def draw_quote(draw, quote):
    rounded(draw, (150, 825, 1180, 925), 28, "#FFF7ED", "#FED7AA", 2)
    draw.line((190, 850, 190, 900), fill=rgb(RED), width=7)
    text(draw, (220, 852), quote, F["body"], DARK_RED)


def draw_badge(draw, cx, cy, title, subtitle, fill):
    draw.ellipse((cx - 150, cy - 150, cx + 150, cy + 150), fill=rgb(fill))
    draw.ellipse((cx - 112, cy - 112, cx + 112, cy + 112), fill=rgb(WHITE))
    text(draw, (cx, cy - 38), title, F["h2"], fill, anchor="ma")
    text(draw, (cx, cy + 28), subtitle, F["small"], MUTED, anchor="ma")


def visual_cover(draw):
    draw.polygon([(1180, 120), (1710, 90), (1785, 460), (1240, 500)], fill=rgb(RED))
    draw.polygon([(1240, 182), (1660, 162), (1715, 390), (1288, 420)], fill=rgb("#DC2626"))
    draw.line((1238, 500, 1268, 830), fill=rgb(DARK_RED), width=22)
    for x, y, r in [(1325, 250, 23), (1375, 220, 10), (1395, 285, 9), (1440, 247, 8), (1460, 300, 7)]:
        draw.ellipse((x - r, y - r, x + r, y + r), fill=rgb(WHITE))
    rounded(draw, (1135, 760, 1660, 830), 28, SOFT_RED)
    text(draw, (1398, 778), "2 MIN PRESENTATION", F["small"], RED, anchor="ma")


def visual_profile(draw):
    rounded(draw, (1260, 310, 1720, 780), 38, WHITE, LINE)
    draw.ellipse((1405, 385, 1575, 555), fill=rgb(SOFT_RED))
    draw.ellipse((1454, 420, 1526, 492), fill=rgb(RED))
    rounded(draw, (1428, 505, 1552, 590), 38, RED)
    for i, label in enumerate(["GPA", "LAB", "TEAM"]):
        rounded(draw, (1335, 625 + i * 58, 1645, 666 + i * 58), 16, BG)
        text(draw, (1360, 631 + i * 58), label, F["tiny"], BLUE if i == 0 else RED)


def visual_competition(draw):
    draw.line((1260, 745, 1745, 745), fill=rgb(LINE), width=12)
    for x, h, color in [(1310, 210, SOFT_BLUE), (1440, 310, RED), (1570, 245, SOFT_RED)]:
        rounded(draw, (x, 745 - h, x + 85, 745), 18, color)
    text(draw, (1485, 330), "省一", F["title"], RED, anchor="ma")
    text(draw, (1485, 425), "蓝桥杯", F["h2"], INK, anchor="ma")
    draw.line((1235, 610, 1330, 560, 1440, 575, 1545, 505, 1695, 470), fill=rgb(BLUE), width=10)
    for x, y in [(1235, 610), (1330, 560), (1440, 575), (1545, 505), (1695, 470)]:
        draw.ellipse((x - 14, y - 14, x + 14, y + 14), fill=rgb(BLUE))


def visual_lab(draw):
    rounded(draw, (1235, 320, 1740, 760), 36, WHITE, LINE)
    rounded(draw, (1300, 390, 1510, 585), 24, SOFT_BLUE)
    for x in [1348, 1410, 1472]:
        for y in [430, 510]:
            draw.ellipse((x - 16, y - 16, x + 16, y + 16), fill=rgb(BLUE))
    draw.line((1364, 430, 1410, 510, 1472, 430), fill=rgb(BLUE), width=7)
    rounded(draw, (1560, 390, 1685, 610), 24, SOFT_RED)
    draw.rectangle((1588, 425, 1658, 560), fill=rgb(WHITE))
    text(draw, (1490, 668), "整理 · 维护 · 协作", F["small"], MUTED, anchor="ma")


def visual_youth(draw):
    draw_badge(draw, 1325, 500, "先进性", "向上", RED)
    draw_badge(draw, 1545, 500, "服务性", "集体", BLUE)
    draw_badge(draw, 1435, 710, "实践性", "行动", DARK_RED)
    draw.line((1380, 580, 1490, 580), fill=rgb(LINE), width=8)
    draw.line((1385, 640, 1420, 610), fill=rgb(LINE), width=8)
    draw.line((1490, 640, 1460, 610), fill=rgb(LINE), width=8)


def visual_plan(draw):
    points = [(1265, 735), (1380, 630), (1500, 660), (1615, 520), (1730, 410)]
    draw.line(points, fill=rgb(BLUE), width=10)
    for idx, (x, y) in enumerate(points, start=1):
        draw.ellipse((x - 32, y - 32, x + 32, y + 32), fill=rgb(WHITE), outline=rgb(BLUE), width=7)
        text(draw, (x, y - 16), str(idx), F["tiny"], BLUE, anchor="ma")
    draw.polygon([(1730, 360), (1810, 405), (1740, 455)], fill=rgb(RED))
    text(draw, (1500, 310), "持续进步", F["h2"], RED, anchor="ma")


VISUALS = {
    "profile": visual_profile,
    "competition": visual_competition,
    "lab": visual_lab,
    "youth": visual_youth,
    "plan": visual_plan,
}


def render_slide(slide, index):
    image = Image.new("RGB", (SLIDE_W_PX, SLIDE_H_PX), rgb(BG))
    draw = ImageDraw.Draw(image)
    draw_grid(draw)
    draw_footer(draw, index + 1)

    if slide["type"] == "cover":
        rounded(draw, (96, 94, 1824, 972), 46, WHITE, LINE, 2)
        text(draw, (155, 170), slide["eyebrow"], F["small"], RED)
        text(draw, (155, 292), slide["title"], F["cover"], INK)
        draw.line((160, 435, 500, 435), fill=rgb(RED), width=10)
        for idx, line in enumerate(wrap_lines(draw, slide["subtitle"], F["body"], 760)):
            text(draw, (160, 500 + idx * 48), line, F["body"], TEXT)
        text(draw, (160, 830), slide["meta"], F["h2"], INK)
        visual_cover(draw)
    else:
        draw_header(draw, slide)
        draw_side_mark(draw, "2分钟陈述")
        draw_points(draw, slide["points"])
        draw_quote(draw, slide["quote"])
        VISUALS[slide["type"]](draw)

    path = ASSET_DIR / f"slide_{index + 1:02d}.png"
    image.save(path, quality=95)
    return path


def generate_slide_images():
    ASSET_DIR.mkdir(exist_ok=True)
    return [render_slide(slide, idx) for idx, slide in enumerate(SLIDES)]


def rels(entries):
    body = "".join(f'<Relationship Id="{rid}" Type="{typ}" Target="{target}"/>' for rid, typ, target in entries)
    return f'<?xml version="1.0" encoding="UTF-8" standalone="yes"?><Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">{body}</Relationships>'


def image_slide_xml(rel_id):
    return f'''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<p:sld xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main">
  <p:cSld><p:spTree>
    <p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>
    <p:grpSpPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="0" cy="0"/><a:chOff x="0" y="0"/><a:chExt cx="0" cy="0"/></a:xfrm></p:grpSpPr>
    <p:pic>
      <p:nvPicPr><p:cNvPr id="2" name="slide image"/><p:cNvPicPr><a:picLocks noChangeAspect="1"/></p:cNvPicPr><p:nvPr/></p:nvPicPr>
      <p:blipFill><a:blip r:embed="{rel_id}"/><a:stretch><a:fillRect/></a:stretch></p:blipFill>
      <p:spPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="12192000" cy="6858000"/></a:xfrm><a:prstGeom prst="rect"><a:avLst/></a:prstGeom></p:spPr>
    </p:pic>
  </p:spTree></p:cSld>
  <p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>
</p:sld>'''


def theme_xml():
    return '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?><a:theme xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" name="Refined"><a:themeElements><a:clrScheme name="Refined"><a:dk1><a:srgbClr val="111827"/></a:dk1><a:lt1><a:srgbClr val="FFFFFF"/></a:lt1><a:dk2><a:srgbClr val="334155"/></a:dk2><a:lt2><a:srgbClr val="F8FAFC"/></a:lt2><a:accent1><a:srgbClr val="B91C1C"/></a:accent1><a:accent2><a:srgbClr val="2563EB"/></a:accent2><a:accent3><a:srgbClr val="7F1D1D"/></a:accent3><a:accent4><a:srgbClr val="FEE2E2"/></a:accent4><a:accent5><a:srgbClr val="DBEAFE"/></a:accent5><a:accent6><a:srgbClr val="64748B"/></a:accent6><a:hlink><a:srgbClr val="2563EB"/></a:hlink><a:folHlink><a:srgbClr val="7C3AED"/></a:folHlink></a:clrScheme><a:fontScheme name="Microsoft YaHei"><a:majorFont><a:latin typeface="Microsoft YaHei"/><a:ea typeface="Microsoft YaHei"/></a:majorFont><a:minorFont><a:latin typeface="Microsoft YaHei"/><a:ea typeface="Microsoft YaHei"/></a:minorFont></a:fontScheme><a:fmtScheme name="Office"><a:fillStyleLst><a:solidFill><a:schemeClr val="phClr"/></a:solidFill><a:solidFill><a:schemeClr val="phClr"/></a:solidFill><a:solidFill><a:schemeClr val="phClr"/></a:solidFill></a:fillStyleLst><a:lnStyleLst><a:ln w="6350"><a:solidFill><a:schemeClr val="phClr"/></a:solidFill></a:ln><a:ln w="12700"><a:solidFill><a:schemeClr val="phClr"/></a:solidFill></a:ln><a:ln w="19050"><a:solidFill><a:schemeClr val="phClr"/></a:solidFill></a:ln></a:lnStyleLst><a:effectStyleLst><a:effectStyle><a:effectLst/></a:effectStyle><a:effectStyle><a:effectLst/></a:effectStyle><a:effectStyle><a:effectLst/></a:effectStyle></a:effectStyleLst><a:bgFillStyleLst><a:solidFill><a:schemeClr val="phClr"/></a:solidFill><a:solidFill><a:schemeClr val="phClr"/></a:solidFill><a:solidFill><a:schemeClr val="phClr"/></a:solidFill></a:bgFillStyleLst></a:fmtScheme></a:themeElements><a:objectDefaults/><a:extraClrSchemeLst/></a:theme>'''


def master_xml():
    return '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?><p:sldMaster xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main"><p:cSld><p:spTree><p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="0" cy="0"/><a:chOff x="0" y="0"/><a:chExt cx="0" cy="0"/></a:xfrm></p:grpSpPr></p:spTree></p:cSld><p:clrMap bg1="lt1" tx1="dk1" bg2="lt2" tx2="dk2" accent1="accent1" accent2="accent2" accent3="accent3" accent4="accent4" accent5="accent5" accent6="accent6" hlink="hlink" folHlink="folHlink"/><p:sldLayoutIdLst><p:sldLayoutId id="2147483649" r:id="rId1"/></p:sldLayoutIdLst><p:txStyles><p:titleStyle/><p:bodyStyle/><p:otherStyle/></p:txStyles></p:sldMaster>'''


def layout_xml():
    return '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?><p:sldLayout xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main" type="blank" preserve="1"><p:cSld name="Blank"><p:spTree><p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="0" cy="0"/><a:chOff x="0" y="0"/><a:chExt cx="0" cy="0"/></a:xfrm></p:grpSpPr></p:spTree></p:cSld><p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr></p:sldLayout>'''


def build_pptx(slide_images):
    count = len(slide_images)
    overrides = "".join(f'<Override PartName="/ppt/slides/slide{i}.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slide+xml"/>' for i in range(1, count + 1))
    content_types = f'''<?xml version="1.0" encoding="UTF-8" standalone="yes"?><Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/><Default Extension="xml" ContentType="application/xml"/><Default Extension="png" ContentType="image/png"/><Override PartName="/ppt/presentation.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml"/><Override PartName="/ppt/slideMasters/slideMaster1.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slideMaster+xml"/><Override PartName="/ppt/slideLayouts/slideLayout1.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slideLayout+xml"/><Override PartName="/ppt/theme/theme1.xml" ContentType="application/vnd.openxmlformats-officedocument.theme+xml"/><Override PartName="/ppt/presProps.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.presProps+xml"/><Override PartName="/ppt/viewProps.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.viewProps+xml"/><Override PartName="/ppt/tableStyles.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.tableStyles+xml"/><Override PartName="/docProps/core.xml" ContentType="application/vnd.openxmlformats-package.core-properties+xml"/><Override PartName="/docProps/app.xml" ContentType="application/vnd.openxmlformats-officedocument.extended-properties+xml"/>{overrides}</Types>'''
    slide_ids = "".join(f'<p:sldId id="{255 + i}" r:id="rId{i + 1}"/>' for i in range(1, count + 1))
    presentation = f'''<?xml version="1.0" encoding="UTF-8" standalone="yes"?><p:presentation xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main"><p:sldMasterIdLst><p:sldMasterId id="2147483648" r:id="rId1"/></p:sldMasterIdLst><p:sldIdLst>{slide_ids}</p:sldIdLst><p:sldSz cx="12192000" cy="6858000" type="wide"/><p:notesSz cx="6858000" cy="9144000"/></p:presentation>'''
    pres_rels = [("rId1", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster", "slideMasters/slideMaster1.xml")]
    pres_rels += [(f"rId{i + 1}", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide", f"slides/slide{i}.xml") for i in range(1, count + 1)]
    pres_rels += [
        (f"rId{count + 2}", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/presProps", "presProps.xml"),
        (f"rId{count + 3}", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/viewProps", "viewProps.xml"),
        (f"rId{count + 4}", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/tableStyles", "tableStyles.xml"),
    ]

    files = {
        "[Content_Types].xml": content_types.encode("utf-8"),
        "_rels/.rels": rels([
            ("rId1", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument", "ppt/presentation.xml"),
            ("rId2", "http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties", "docProps/core.xml"),
            ("rId3", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties", "docProps/app.xml"),
        ]).encode("utf-8"),
        "ppt/presentation.xml": presentation.encode("utf-8"),
        "ppt/_rels/presentation.xml.rels": rels(pres_rels).encode("utf-8"),
        "ppt/slideMasters/slideMaster1.xml": master_xml().encode("utf-8"),
        "ppt/slideMasters/_rels/slideMaster1.xml.rels": rels([
            ("rId1", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout", "../slideLayouts/slideLayout1.xml"),
            ("rId2", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme", "../theme/theme1.xml"),
        ]).encode("utf-8"),
        "ppt/slideLayouts/slideLayout1.xml": layout_xml().encode("utf-8"),
        "ppt/slideLayouts/_rels/slideLayout1.xml.rels": rels([("rId1", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster", "../slideMasters/slideMaster1.xml")]).encode("utf-8"),
        "ppt/theme/theme1.xml": theme_xml().encode("utf-8"),
        "ppt/presProps.xml": b'<?xml version="1.0" encoding="UTF-8" standalone="yes"?><p:presentationPr xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main"/>',
        "ppt/viewProps.xml": b'<?xml version="1.0" encoding="UTF-8" standalone="yes"?><p:viewPr xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main"/>',
        "ppt/tableStyles.xml": b'<?xml version="1.0" encoding="UTF-8" standalone="yes"?><a:tblStyleLst xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" def="{5C22544A-7EE6-4342-B048-85BDC9FD1C3A}"/>',
        "docProps/app.xml": f'<?xml version="1.0" encoding="UTF-8" standalone="yes"?><Properties xmlns="http://schemas.openxmlformats.org/officeDocument/2006/extended-properties" xmlns:vt="http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes"><Application>Codex</Application><PresentationFormat>宽屏</PresentationFormat><Slides>{count}</Slides><Company/></Properties>'.encode("utf-8"),
        "docProps/core.xml": '<?xml version="1.0" encoding="UTF-8" standalone="yes"?><cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:dcterms="http://purl.org/dc/terms/" xmlns:dcmitype="http://purl.org/dc/dcmitype/" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"><dc:title>团员入团答辩-傅思雄-精修版</dc:title><dc:creator>Codex</dc:creator><cp:lastModifiedBy>Codex</cp:lastModifiedBy></cp:coreProperties>'.encode("utf-8"),
    }

    for i, image_path in enumerate(slide_images, start=1):
        files[f"ppt/slides/slide{i}.xml"] = image_slide_xml("rId2").encode("utf-8")
        files[f"ppt/slides/_rels/slide{i}.xml.rels"] = rels([
            ("rId1", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout", "../slideLayouts/slideLayout1.xml"),
            ("rId2", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/image", f"../media/slide_{i:02d}.png"),
        ]).encode("utf-8")
        files[f"ppt/media/slide_{i:02d}.png"] = image_path.read_bytes()

    with ZipFile(PPTX_PATH, "w", ZIP_DEFLATED) as archive:
        for name, data in files.items():
            archive.writestr(name, data)


def main():
    slide_images = generate_slide_images()
    build_pptx(slide_images)
    print(PPTX_PATH)


if __name__ == "__main__":
    main()
