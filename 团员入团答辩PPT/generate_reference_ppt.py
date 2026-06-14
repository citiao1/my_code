from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED

from PIL import Image, ImageDraw, ImageFont

BASE = Path(__file__).resolve().parent
REF_ASSETS = BASE / "ai_ref_assets"
OUT_ASSETS = BASE / "assets_reference"
PPTX_PATH = BASE / "团员入团答辩-傅思雄-参考版.pptx"

W, H = 1920, 1080
RED = "#C8102E"
DARK = "#151515"
TEXT = "#333333"
MUTED = "#666666"
LIGHT = "#F7F7F7"
BORDER = "#E8E8E8"
SOFT_RED = "#FBE6EA"
WHITE = "#FFFFFF"

SLIDES = [
    {
        "kind": "cover",
        "title": "入团答辩汇报",
        "subtitle": "青春心向党 · 建功新时代",
        "meta": "汇报人：傅思雄    自控2401",
    },
    {
        "kind": "three",
        "num": "01",
        "title": "个人基本情况",
        "items": [
            ("学业基础", "绩点 3.3 / 4.33，重视专业学习与实践结合。", "image6.png"),
            ("实践经历", "担任实验室助理，协助日常整理、设备维护和实验准备。", "image8.png"),
            ("个人特点", "任职经历不多，但做事踏实，注重执行、协作与责任。", "image7.png"),
        ],
    },
    {
        "kind": "three",
        "num": "02",
        "title": "竞赛获奖与实践经历",
        "items": [
            ("蓝桥杯省一等奖", "已取得省级一等奖，国赛成绩目前等待公布。", "image4.png"),
            ("智能车实践", "正在参与智能车相关训练，在调试和复盘中提升能力。", "image5.png"),
            ("持续挑战", "通过竞赛训练编程、调试、动手和解决问题能力。", "image9.png"),
        ],
    },
    {
        "kind": "two",
        "num": "03",
        "title": "对共青团的认识",
        "items": [
            ("思想上的引领", "共青团是党领导的先进青年的群团组织，是青年在实践中学习中国特色社会主义的重要平台。它引导我们树立正确的世界观、人生观和价值观。"),
            ("行动上的标准", "成为共青团员意味着要在学习、工作和生活中发挥模范作用，不只追求个人进步，也要心系集体、服务同学、勇于承担。"),
        ],
    },
    {
        "kind": "statement",
        "num": "04",
        "title": "入团志愿与自我要求",
        "quote": "我志愿加入中国共产主义青年团。",
        "body": [
            "我希望以更高标准要求自己，把个人成长和集体需要结合起来。",
            "在学习上继续保持稳定进步，在科创实践中不断锻炼能力，在集体事务中主动承担力所能及的工作。",
        ],
    },
    {
        "kind": "thanks",
        "title": "感谢聆听",
        "subtitle": "敬请各位老师、同学批评指正！",
    },
]


def rgb(hex_value):
    hex_value = hex_value.replace("#", "")
    return tuple(int(hex_value[i : i + 2], 16) for i in (0, 2, 4))


def font(size, bold=False):
    candidates = [
        "C:/Windows/Fonts/msyhbd.ttc" if bold else "C:/Windows/Fonts/msyh.ttc",
        "C:/Windows/Fonts/simhei.ttf",
        "C:/Windows/Fonts/simsun.ttc",
    ]
    for item in candidates:
        try:
            return ImageFont.truetype(item, size)
        except OSError:
            continue
    return ImageFont.load_default()


F = {
    "cover": font(94, True),
    "title": font(60, True),
    "h2": font(34, True),
    "body": font(29),
    "small": font(24),
    "tiny": font(20),
    "num": font(34, True),
    "quote": font(42, True),
}


def draw_text(draw, xy, content, fnt, color=TEXT, anchor=None):
    draw.text(xy, content, font=fnt, fill=rgb(color), anchor=anchor)


def wrap(draw, content, fnt, max_width):
    lines = []
    current = ""
    for ch in content:
        candidate = current + ch
        if draw.textlength(candidate, font=fnt) <= max_width:
            current = candidate
        else:
            if current:
                lines.append(current)
            current = ch
    if current:
        lines.append(current)
    return lines


def rounded(draw, box, radius=24, fill=WHITE, outline=BORDER, width=2):
    draw.rounded_rectangle(box, radius=radius, fill=rgb(fill), outline=rgb(outline), width=width)


def icon_image(name, size=54):
    path = REF_ASSETS / name
    if not path.exists():
        return None
    image = Image.open(path).convert("RGBA")
    image.thumbnail((size, size))
    return image


def base_page():
    image = Image.new("RGB", (W, H), WHITE)
    draw = ImageDraw.Draw(image)
    draw.rectangle((0, 0, W, 10), fill=rgb(RED))
    draw.line((0, H - 8, W, H - 8), fill=rgb(RED), width=5)
    draw.line((0, 0, 0, 58), fill=rgb(DARK), width=2)
    draw.line((W - 2, 0, W - 2, 58), fill=rgb(DARK), width=2)
    draw.line((0, H - 60, 0, H), fill=rgb(DARK), width=2)
    draw.line((W - 2, H - 60, W - 2, H), fill=rgb(DARK), width=2)
    return image, draw


def section_title(draw, slide):
    draw_text(draw, (130, 120), slide["num"], F["num"], RED)
    draw.line((130, 165, 220, 165), fill=rgb(RED), width=6)
    draw_text(draw, (130, 220), slide["title"], F["title"], DARK)
    draw.line((130, 305, 610, 305), fill=rgb(RED), width=5)


def draw_card(draw, x, y, w, h, title, body, icon=None):
    rounded(draw, (x, y, x + w, y + h), 28, WHITE, BORDER, 2)
    if icon:
        im = icon_image(icon, 56)
        if im:
            draw.bitmap((x + 34, y + 32), im, fill=None)
        else:
            draw.ellipse((x + 36, y + 34, x + 86, y + 84), fill=rgb(RED))
    else:
        draw.ellipse((x + 36, y + 34, x + 86, y + 84), fill=rgb(RED))
    draw_text(draw, (x + 112, y + 30), title, F["h2"], DARK)
    yy = y + 88
    for line in wrap(draw, body, F["body"], w - 160):
        draw_text(draw, (x + 112, yy), line, F["body"], TEXT)
        yy += 42


def draw_cover(slide):
    image, draw = base_page()
    draw_text(draw, (170, 250), slide["title"], F["cover"], DARK)
    draw.line((170, 378, 660, 378), fill=rgb(RED), width=9)
    draw_text(draw, (170, 450), slide["subtitle"], F["h2"], RED)
    draw_text(draw, (170, 775), slide["meta"], F["body"], TEXT)
    draw.ellipse((1370, 225, 1710, 565), fill=rgb(SOFT_RED))
    draw.line((1210, 655, 1690, 655), fill=rgb(DARK), width=5)
    draw.line((1450, 185, 1450, 720), fill=rgb(RED), width=18)
    draw.polygon([(1450, 190), (1715, 270), (1450, 350)], fill=rgb(RED))
    draw.polygon([(1488, 245), (1648, 287), (1488, 325)], fill=rgb("#E11D48"))
    draw_text(draw, (1700, 900), "Youth League", F["small"], MUTED, anchor="ra")
    return image


def draw_three(slide):
    image, draw = base_page()
    section_title(draw, slide)
    y = 380
    for title, body, icon in slide["items"]:
        draw_card(draw, 170, y, 1100, 145, title, body, icon)
        y += 185
    draw.ellipse((1425, 305, 1665, 545), fill=rgb(SOFT_RED))
    draw_text(draw, (1545, 365), slide["num"], F["cover"], RED, anchor="ma")
    draw_text(draw, (1545, 492), "KEY", F["h2"], DARK, anchor="ma")
    draw.line((1380, 680, 1720, 680), fill=rgb(RED), width=6)
    draw_text(draw, (1550, 730), "踏实 · 实践 · 进步", F["h2"], TEXT, anchor="ma")
    return image


def draw_two(slide):
    image, draw = base_page()
    section_title(draw, slide)
    draw_card(draw, 170, 395, 720, 330, slide["items"][0][0], slide["items"][0][1], "image15.png")
    draw_card(draw, 1030, 395, 720, 330, slide["items"][1][0], slide["items"][1][1], "image16.png")
    draw.rectangle((170, 810, 1750, 900), fill=rgb(LIGHT))
    draw.line((220, 832, 220, 878), fill=rgb(RED), width=8)
    draw_text(draw, (250, 835), "理解共青团，就是理解先进青年应承担的责任。", F["body"], DARK)
    return image


def draw_statement(slide):
    image, draw = base_page()
    section_title(draw, slide)
    draw_text(draw, (W // 2, 405), f"“{slide['quote']}”", F["quote"], RED, anchor="ma")
    draw.line((520, 505, 1400, 505), fill=rgb(BORDER), width=3)
    y = 610
    for body in slide["body"]:
        rounded(draw, (340, y, 1580, y + 110), 26, WHITE, BORDER, 2)
        draw.ellipse((390, y + 38, 424, y + 72), fill=rgb(RED))
        yy = y + 35
        for line in wrap(draw, body, F["body"], 1080):
            draw_text(draw, (455, yy), line, F["body"], TEXT)
            yy += 42
        y += 150
    return image


def draw_thanks(slide):
    image, draw = base_page()
    draw_text(draw, (W // 2, 360), slide["title"], F["cover"], DARK, anchor="ma")
    draw.line((660, 500, 1260, 500), fill=rgb(RED), width=9)
    draw_text(draw, (W // 2, 590), slide["subtitle"], F["h2"], TEXT, anchor="ma")
    draw.ellipse((830, 720, 1090, 980), fill=rgb(SOFT_RED))
    im = icon_image("image16.png", 115)
    if im:
        image.paste(im, (902, 792), im)
    return image


def render_slides():
    OUT_ASSETS.mkdir(exist_ok=True)
    paths = []
    for idx, slide in enumerate(SLIDES, start=1):
        if slide["kind"] == "cover":
            image = draw_cover(slide)
        elif slide["kind"] == "three":
            image = draw_three(slide)
        elif slide["kind"] == "two":
            image = draw_two(slide)
        elif slide["kind"] == "statement":
            image = draw_statement(slide)
        else:
            image = draw_thanks(slide)
        path = OUT_ASSETS / f"ref_slide_{idx:02d}.png"
        image.save(path, quality=95)
        paths.append(path)
    return paths


def rels(entries):
    body = "".join(f'<Relationship Id="{rid}" Type="{typ}" Target="{target}"/>' for rid, typ, target in entries)
    return f'<?xml version="1.0" encoding="UTF-8" standalone="yes"?><Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">{body}</Relationships>'


def slide_xml(rel_id):
    return f'''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<p:sld xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main">
  <p:cSld><p:spTree><p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="0" cy="0"/><a:chOff x="0" y="0"/><a:chExt cx="0" cy="0"/></a:xfrm></p:grpSpPr>
  <p:pic><p:nvPicPr><p:cNvPr id="2" name="slide image"/><p:cNvPicPr><a:picLocks noChangeAspect="1"/></p:cNvPicPr><p:nvPr/></p:nvPicPr><p:blipFill><a:blip r:embed="{rel_id}"/><a:stretch><a:fillRect/></a:stretch></p:blipFill><p:spPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="12192000" cy="6858000"/></a:xfrm><a:prstGeom prst="rect"><a:avLst/></a:prstGeom></p:spPr></p:pic>
  </p:spTree></p:cSld><p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>
</p:sld>'''


def theme_xml():
    return '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?><a:theme xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" name="Reference"><a:themeElements><a:clrScheme name="Reference"><a:dk1><a:srgbClr val="151515"/></a:dk1><a:lt1><a:srgbClr val="FFFFFF"/></a:lt1><a:dk2><a:srgbClr val="333333"/></a:dk2><a:lt2><a:srgbClr val="F7F7F7"/></a:lt2><a:accent1><a:srgbClr val="C8102E"/></a:accent1><a:accent2><a:srgbClr val="151515"/></a:accent2><a:accent3><a:srgbClr val="FBE6EA"/></a:accent3><a:accent4><a:srgbClr val="666666"/></a:accent4><a:accent5><a:srgbClr val="E8E8E8"/></a:accent5><a:accent6><a:srgbClr val="FFFFFF"/></a:accent6><a:hlink><a:srgbClr val="C8102E"/></a:hlink><a:folHlink><a:srgbClr val="666666"/></a:folHlink></a:clrScheme><a:fontScheme name="Microsoft YaHei"><a:majorFont><a:latin typeface="Microsoft YaHei"/><a:ea typeface="Microsoft YaHei"/></a:majorFont><a:minorFont><a:latin typeface="Microsoft YaHei"/><a:ea typeface="Microsoft YaHei"/></a:minorFont></a:fontScheme><a:fmtScheme name="Office"><a:fillStyleLst><a:solidFill><a:schemeClr val="phClr"/></a:solidFill><a:solidFill><a:schemeClr val="phClr"/></a:solidFill><a:solidFill><a:schemeClr val="phClr"/></a:solidFill></a:fillStyleLst><a:lnStyleLst><a:ln w="6350"><a:solidFill><a:schemeClr val="phClr"/></a:solidFill></a:ln><a:ln w="12700"><a:solidFill><a:schemeClr val="phClr"/></a:solidFill></a:ln><a:ln w="19050"><a:solidFill><a:schemeClr val="phClr"/></a:solidFill></a:ln></a:lnStyleLst><a:effectStyleLst><a:effectStyle><a:effectLst/></a:effectStyle><a:effectStyle><a:effectLst/></a:effectStyle><a:effectStyle><a:effectLst/></a:effectStyle></a:effectStyleLst><a:bgFillStyleLst><a:solidFill><a:schemeClr val="phClr"/></a:solidFill><a:solidFill><a:schemeClr val="phClr"/></a:solidFill><a:solidFill><a:schemeClr val="phClr"/></a:solidFill></a:bgFillStyleLst></a:fmtScheme></a:themeElements><a:objectDefaults/><a:extraClrSchemeLst/></a:theme>'''


def master_xml():
    return '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?><p:sldMaster xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main"><p:cSld><p:spTree><p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="0" cy="0"/><a:chOff x="0" y="0"/><a:chExt cx="0" cy="0"/></a:xfrm></p:grpSpPr></p:spTree></p:cSld><p:clrMap bg1="lt1" tx1="dk1" bg2="lt2" tx2="dk2" accent1="accent1" accent2="accent2" accent3="accent3" accent4="accent4" accent5="accent5" accent6="accent6" hlink="hlink" folHlink="folHlink"/><p:sldLayoutIdLst><p:sldLayoutId id="2147483649" r:id="rId1"/></p:sldLayoutIdLst><p:txStyles><p:titleStyle/><p:bodyStyle/><p:otherStyle/></p:txStyles></p:sldMaster>'''


def layout_xml():
    return '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?><p:sldLayout xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main" type="blank" preserve="1"><p:cSld name="Blank"><p:spTree><p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="0" cy="0"/><a:chOff x="0" y="0"/><a:chExt cx="0" cy="0"/></a:xfrm></p:grpSpPr></p:spTree></p:cSld><p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr></p:sldLayout>'''


def build_ppt(paths):
    count = len(paths)
    overrides = "".join(f'<Override PartName="/ppt/slides/slide{i}.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slide+xml"/>' for i in range(1, count + 1))
    content = f'''<?xml version="1.0" encoding="UTF-8" standalone="yes"?><Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/><Default Extension="xml" ContentType="application/xml"/><Default Extension="png" ContentType="image/png"/><Override PartName="/ppt/presentation.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml"/><Override PartName="/ppt/slideMasters/slideMaster1.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slideMaster+xml"/><Override PartName="/ppt/slideLayouts/slideLayout1.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slideLayout+xml"/><Override PartName="/ppt/theme/theme1.xml" ContentType="application/vnd.openxmlformats-officedocument.theme+xml"/><Override PartName="/ppt/presProps.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.presProps+xml"/><Override PartName="/ppt/viewProps.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.viewProps+xml"/><Override PartName="/ppt/tableStyles.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.tableStyles+xml"/><Override PartName="/docProps/core.xml" ContentType="application/vnd.openxmlformats-package.core-properties+xml"/><Override PartName="/docProps/app.xml" ContentType="application/vnd.openxmlformats-officedocument.extended-properties+xml"/>{overrides}</Types>'''
    ids = "".join(f'<p:sldId id="{255+i}" r:id="rId{i+1}"/>' for i in range(1, count + 1))
    pres = f'''<?xml version="1.0" encoding="UTF-8" standalone="yes"?><p:presentation xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main"><p:sldMasterIdLst><p:sldMasterId id="2147483648" r:id="rId1"/></p:sldMasterIdLst><p:sldIdLst>{ids}</p:sldIdLst><p:sldSz cx="12192000" cy="6858000" type="wide"/><p:notesSz cx="6858000" cy="9144000"/></p:presentation>'''
    pres_rels = [("rId1", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster", "slideMasters/slideMaster1.xml")]
    pres_rels += [(f"rId{i+1}", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide", f"slides/slide{i}.xml") for i in range(1, count + 1)]
    pres_rels += [(f"rId{count+2}", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/presProps", "presProps.xml"), (f"rId{count+3}", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/viewProps", "viewProps.xml"), (f"rId{count+4}", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/tableStyles", "tableStyles.xml")]
    files = {
        "[Content_Types].xml": content,
        "_rels/.rels": rels([("rId1", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument", "ppt/presentation.xml"), ("rId2", "http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties", "docProps/core.xml"), ("rId3", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties", "docProps/app.xml")]),
        "ppt/presentation.xml": pres,
        "ppt/_rels/presentation.xml.rels": rels(pres_rels),
        "ppt/slideMasters/slideMaster1.xml": master_xml(),
        "ppt/slideMasters/_rels/slideMaster1.xml.rels": rels([("rId1", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout", "../slideLayouts/slideLayout1.xml"), ("rId2", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme", "../theme/theme1.xml")]),
        "ppt/slideLayouts/slideLayout1.xml": layout_xml(),
        "ppt/slideLayouts/_rels/slideLayout1.xml.rels": rels([("rId1", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster", "../slideMasters/slideMaster1.xml")]),
        "ppt/theme/theme1.xml": theme_xml(),
        "ppt/presProps.xml": '<?xml version="1.0" encoding="UTF-8" standalone="yes"?><p:presentationPr xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main"/>',
        "ppt/viewProps.xml": '<?xml version="1.0" encoding="UTF-8" standalone="yes"?><p:viewPr xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main"/>',
        "ppt/tableStyles.xml": '<?xml version="1.0" encoding="UTF-8" standalone="yes"?><a:tblStyleLst xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" def="{5C22544A-7EE6-4342-B048-85BDC9FD1C3A}"/>',
        "docProps/app.xml": f'<?xml version="1.0" encoding="UTF-8" standalone="yes"?><Properties xmlns="http://schemas.openxmlformats.org/officeDocument/2006/extended-properties" xmlns:vt="http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes"><Application>Codex</Application><PresentationFormat>宽屏</PresentationFormat><Slides>{count}</Slides><Company/></Properties>',
        "docProps/core.xml": '<?xml version="1.0" encoding="UTF-8" standalone="yes"?><cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:dcterms="http://purl.org/dc/terms/" xmlns:dcmitype="http://purl.org/dc/dcmitype/" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"><dc:title>团员入团答辩-傅思雄-参考版</dc:title><dc:creator>Codex</dc:creator><cp:lastModifiedBy>Codex</cp:lastModifiedBy></cp:coreProperties>',
    }
    with ZipFile(PPTX_PATH, "w", ZIP_DEFLATED) as z:
        for name, data in files.items():
            z.writestr(name, data.encode("utf-8"))
        for i, path in enumerate(paths, start=1):
            z.writestr(f"ppt/slides/slide{i}.xml", slide_xml("rId2").encode("utf-8"))
            z.writestr(f"ppt/slides/_rels/slide{i}.xml.rels", rels([("rId1", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout", "../slideLayouts/slideLayout1.xml"), ("rId2", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/image", f"../media/ref_slide_{i:02d}.png")]).encode("utf-8"))
            z.writestr(f"ppt/media/ref_slide_{i:02d}.png", path.read_bytes())


def main():
    paths = render_slides()
    build_ppt(paths)
    print(PPTX_PATH)


if __name__ == "__main__":
    main()
