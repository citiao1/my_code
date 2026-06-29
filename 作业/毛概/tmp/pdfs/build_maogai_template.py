from __future__ import annotations

from html import escape
from pathlib import Path

from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER, TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.cidfonts import UnicodeCIDFont
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (
    BaseDocTemplate,
    Frame,
    HRFlowable,
    ListFlowable,
    ListItem,
    PageBreak,
    PageTemplate,
    Paragraph,
    Spacer,
    Table,
    TableStyle,
)


ROOT = Path(__file__).resolve().parents[2]
OUT_DIR = ROOT / "output" / "pdf"
OUT_PDF = OUT_DIR / "毛概答题模板.pdf"


def register_fonts() -> tuple[str, str]:
    pdfmetrics.registerFont(UnicodeCIDFont("STSong-Light"))
    body = "STSong-Light"
    heading = body
    simhei = Path(r"C:\Windows\Fonts\simhei.ttf")
    if simhei.exists():
        pdfmetrics.registerFont(TTFont("SimHei", str(simhei)))
        heading = "SimHei"
    return body, heading


BODY_FONT, HEADING_FONT = register_fonts()

PALETTE = {
    "ink": colors.HexColor("#1F2933"),
    "muted": colors.HexColor("#52616B"),
    "line": colors.HexColor("#CCD6DD"),
    "blue": colors.HexColor("#245C7A"),
    "green": colors.HexColor("#3F6B4F"),
    "red": colors.HexColor("#8A3A3A"),
    "amber": colors.HexColor("#8A6A28"),
    "paper": colors.HexColor("#F8FAFB"),
    "soft_blue": colors.HexColor("#EAF4F7"),
    "soft_green": colors.HexColor("#EEF7F0"),
    "soft_amber": colors.HexColor("#FAF4E4"),
    "soft_red": colors.HexColor("#F8ECEC"),
}


styles = getSampleStyleSheet()
styles.add(
    ParagraphStyle(
        "CnTitle",
        parent=styles["Title"],
        fontName=HEADING_FONT,
        fontSize=24,
        leading=32,
        alignment=TA_CENTER,
        textColor=PALETTE["ink"],
        spaceAfter=8,
        wordWrap="CJK",
    )
)
styles.add(
    ParagraphStyle(
        "CnSubtitle",
        parent=styles["Normal"],
        fontName=BODY_FONT,
        fontSize=10.5,
        leading=16,
        alignment=TA_CENTER,
        textColor=PALETTE["muted"],
        spaceAfter=16,
        wordWrap="CJK",
    )
)
styles.add(
    ParagraphStyle(
        "H1",
        parent=styles["Heading1"],
        fontName=HEADING_FONT,
        fontSize=16,
        leading=22,
        textColor=PALETTE["blue"],
        spaceBefore=8,
        spaceAfter=7,
        wordWrap="CJK",
    )
)
styles.add(
    ParagraphStyle(
        "H2",
        parent=styles["Heading2"],
        fontName=HEADING_FONT,
        fontSize=12.5,
        leading=18,
        textColor=PALETTE["green"],
        spaceBefore=6,
        spaceAfter=5,
        wordWrap="CJK",
    )
)
styles.add(
    ParagraphStyle(
        "BodyCn",
        parent=styles["BodyText"],
        fontName=BODY_FONT,
        fontSize=9.6,
        leading=15,
        textColor=PALETTE["ink"],
        spaceAfter=4,
        wordWrap="CJK",
    )
)
styles.add(
    ParagraphStyle(
        "SmallCn",
        parent=styles["BodyText"],
        fontName=BODY_FONT,
        fontSize=8.6,
        leading=13,
        textColor=PALETTE["ink"],
        spaceAfter=3,
        wordWrap="CJK",
    )
)
styles.add(
    ParagraphStyle(
        "NoteCn",
        parent=styles["BodyText"],
        fontName=BODY_FONT,
        fontSize=8.5,
        leading=13,
        textColor=PALETTE["muted"],
        spaceAfter=3,
        wordWrap="CJK",
    )
)
styles.add(
    ParagraphStyle(
        "CellHead",
        parent=styles["BodyText"],
        fontName=HEADING_FONT,
        fontSize=8.8,
        leading=12,
        textColor=colors.white,
        alignment=TA_CENTER,
        wordWrap="CJK",
    )
)
styles.add(
    ParagraphStyle(
        "CellCn",
        parent=styles["BodyText"],
        fontName=BODY_FONT,
        fontSize=8,
        leading=11.4,
        textColor=PALETTE["ink"],
        wordWrap="CJK",
    )
)
styles.add(
    ParagraphStyle(
        "QuoteCn",
        parent=styles["BodyText"],
        fontName=BODY_FONT,
        fontSize=9.2,
        leading=14.2,
        leftIndent=8,
        rightIndent=8,
        textColor=PALETTE["ink"],
        spaceBefore=3,
        spaceAfter=5,
        wordWrap="CJK",
    )
)


def p(text: str, style: str = "BodyCn") -> Paragraph:
    return Paragraph(escape(text).replace("\n", "<br/>"), styles[style])


def strong(label: str, body: str) -> Paragraph:
    return Paragraph(f"<b>{escape(label)}</b>{escape(body)}", styles["BodyCn"])


def bullets(items: list[str], level: str = "BodyCn") -> ListFlowable:
    return ListFlowable(
        [ListItem(p(item, level), leftIndent=0) for item in items],
        bulletType="bullet",
        leftIndent=14,
        bulletFontName=BODY_FONT,
        bulletFontSize=7,
        bulletColor=PALETTE["blue"],
    )


def numbered(items: list[str], level: str = "BodyCn") -> ListFlowable:
    return ListFlowable(
        [ListItem(p(item, level), leftIndent=0) for item in items],
        bulletType="1",
        start="1",
        leftIndent=18,
        bulletFontName=BODY_FONT,
        bulletFontSize=8.4,
        bulletColor=PALETTE["blue"],
    )


def box(title: str, items: list[str], fill=PALETTE["soft_blue"]):
    body = [[p(title, "H2")]]
    for item in items:
        body.append([p(item, "QuoteCn")])
    table = Table(body, colWidths=[170 * mm], hAlign="LEFT")
    table.setStyle(
        TableStyle(
            [
                ("BACKGROUND", (0, 0), (-1, -1), fill),
                ("BOX", (0, 0), (-1, -1), 0.6, PALETTE["line"]),
                ("LEFTPADDING", (0, 0), (-1, -1), 8),
                ("RIGHTPADDING", (0, 0), (-1, -1), 8),
                ("TOPPADDING", (0, 0), (-1, -1), 5),
                ("BOTTOMPADDING", (0, 0), (-1, -1), 5),
            ]
        )
    )
    return table


def make_table(headers: list[str], rows: list[list[str]], widths: list[float]):
    data = [[p(h, "CellHead") for h in headers]]
    data += [[p(c, "CellCn") for c in row] for row in rows]
    table = Table(data, colWidths=[w * mm for w in widths], repeatRows=1, hAlign="LEFT")
    table.setStyle(
        TableStyle(
            [
                ("BACKGROUND", (0, 0), (-1, 0), PALETTE["blue"]),
                ("TEXTCOLOR", (0, 0), (-1, 0), colors.white),
                ("GRID", (0, 0), (-1, -1), 0.35, PALETTE["line"]),
                ("VALIGN", (0, 0), (-1, -1), "TOP"),
                ("LEFTPADDING", (0, 0), (-1, -1), 4),
                ("RIGHTPADDING", (0, 0), (-1, -1), 4),
                ("TOPPADDING", (0, 0), (-1, -1), 4),
                ("BOTTOMPADDING", (0, 0), (-1, -1), 4),
                ("BACKGROUND", (0, 1), (-1, -1), colors.white),
                ("ROWBACKGROUNDS", (0, 1), (-1, -1), [colors.white, PALETTE["paper"]]),
            ]
        )
    )
    return table


class TemplateDoc(BaseDocTemplate):
    def __init__(self, filename: Path):
        super().__init__(
            str(filename),
            pagesize=A4,
            rightMargin=18 * mm,
            leftMargin=18 * mm,
            topMargin=18 * mm,
            bottomMargin=16 * mm,
            title="毛概答题模板",
            author="Codex",
        )
        frame = Frame(
            self.leftMargin,
            self.bottomMargin,
            self.width,
            self.height,
            id="normal",
        )
        self.addPageTemplates(
            [PageTemplate(id="page", frames=[frame], onPage=self.header_footer)]
        )

    def header_footer(self, canvas, doc):
        canvas.saveState()
        canvas.setFont(BODY_FONT, 8)
        canvas.setFillColor(PALETTE["muted"])
        if doc.page > 1:
            canvas.drawString(18 * mm, A4[1] - 11 * mm, "毛概答题模板")
            canvas.drawRightString(A4[0] - 18 * mm, A4[1] - 11 * mm, "基于2026知识点1-2原句整理")
            canvas.setStrokeColor(PALETTE["line"])
            canvas.line(18 * mm, A4[1] - 13 * mm, A4[0] - 18 * mm, A4[1] - 13 * mm)
        canvas.drawCentredString(A4[0] / 2, 8 * mm, str(doc.page))
        canvas.restoreState()


def story() -> list:
    flow: list = []

    flow.append(p("毛概答题模板", "CnTitle"))
    flow.append(p("基于《2-2026知识点1（简答题）》与《3-2026知识点2（论述题）》原句整理", "CnSubtitle"))
    flow.append(
        box(
            "总原则：先把题目放进马克思主义中国化时代化的理论链条",
            [
                "马克思主义中国化时代化，就是立足中国国情和时代特点，坚持把马克思主义基本原理同中国具体实际相结合、同中华优秀传统文化相结合，深入研究和解决中国革命、建设、改革不同历史时期的实际问题，真正搞懂面临的时代课题，不断吸收新的时代内容，科学回答时代提出的重大理论和实践课题，创造新的理论成果。",
                "马克思主义中国化时代化的理论成果是一脉相承又与时俱进的关系。一方面，毛泽东思想所蕴含的马克思主义的立场、观点和方法，为中国特色社会主义理论体系提供了基本遵循。另一方面，中国特色社会主义理论体系在新的历史条件下进一步丰富和发展了毛泽东思想。",
                "它们都是马克思列宁主义在中国的运用和发展，都以独创性的理论成果丰富和发展了马克思主义的理论宝库，都是党和国家必须长期坚持的指导思想，是全国各族人民团结奋斗的共同思想基础。",
            ],
            PALETTE["soft_blue"],
        )
    )
    flow.append(Spacer(1, 6))
    flow.append(p("一、最常用的万能开头与收束", "H1"))
    flow.append(
        numbered(
            [
                "开头先写历史进程：在新民主主义革命时期，我们党创立了毛泽东思想，是马克思主义中国化时代化的第一次历史性飞跃。",
                "接着写理论体系：在改革开放和社会主义现代化建设新时期，我们党创立了邓小平理论，形成了“三个代表”重要思想、科学发展观，形成中国特色社会主义理论体系，实现了马克思主义中国化时代化新的飞跃。",
                "最后写新时代：在中国特色社会主义新时代，我们党创立了习近平新时代中国特色社会主义思想，实现了马克思主义中国化时代化新的飞跃。",
                "收束时扣关系：马克思主义中国化时代化的理论成果是一脉相承又与时俱进的关系。",
            ]
        )
    )
    flow.append(
        p(
            "用法：任何问到某一个理论、某一个人物、某一个历史地位的题，都可以先用以上四句搭桥，再回到题目要求。这样既能显示理论脉络，也方便自然扩写。",
            "NoteCn",
        )
    )

    flow.append(p("二、五人五理论总表", "H1"))
    flow.append(
        make_table(
            ["人物", "理论", "关键词原句", "可套答题点"],
            [
                [
                    "毛泽东",
                    "毛泽东思想",
                    "毛泽东思想是马克思主义中国化时代化的第一个重大理论成果；毛泽东思想是中国革命和建设的科学指南；毛泽东思想是中国共产党和中国人民的宝贵精神财富。",
                    "新民主主义革命理论；社会主义革命和社会主义建设理论；活的灵魂：实事求是、群众路线、独立自主。",
                ],
                [
                    "邓小平",
                    "邓小平理论",
                    "邓小平理论首要的基本的理论问题：什么是社会主义、怎样建设社会主义。",
                    "社会主义本质：解放生产力，发展生产力，消灭剥削，消除两极分化，最终达到共同富裕。",
                ],
                [
                    "江泽民",
                    "“三个代表”重要思想",
                    "进一步回答了什么是社会主义、怎样建设社会主义的问题，创造性的回答了建设什么样的党、怎样建设党的问题。",
                    "始终代表中国先进生产力的发展要求；始终代表中国先进文化的前进方向；始终代表中国最广大人民的根本利益。",
                ],
                [
                    "胡锦涛",
                    "科学发展观",
                    "创造性地回答了新形势下实现什么样的发展、怎样发展等重大问题。",
                    "推动经济社会发展是第一要义；以人为本是核心立场；全面协调可持续是基本要求；统筹兼顾是根本方法。",
                ],
                [
                    "习近平",
                    "习近平新时代中国特色社会主义思想",
                    "在中国特色社会主义新时代，我们党创立了习近平新时代中国特色社会主义思想，实现了马克思主义中国化时代化新的飞跃。",
                    "这个新时代，是承前启后、继往开来、在新的历史条件下继续夺取中国特色社会主义伟大胜利的时代。",
                ],
            ],
            [18, 31, 63, 58],
        )
    )

    flow.append(p("三、夸夸题模板：历史地位、意义、评价", "H1"))
    flow.append(
        box(
            "三段式答法",
            [
                "第一段写贡献：先点出理论内容、回答了什么问题、解决了什么时代课题。",
                "第二段写评价：直接套“理论成果、科学指南、精神财富”“继承和发展”“开篇之作”“接续发展”等原句。",
                "第三段写意义：从理论宝库、国家民族、社会发展、人民生活、国际影响五个方向扩写。",
            ],
            PALETTE["soft_amber"],
        )
    )
    flow.append(Spacer(1, 4))
    flow.append(p("常用意义句库", "H2"))
    flow.append(
        bullets(
            [
                "理论意义：极大地丰富了马克思主义的理论宝库。",
                "国家民族：完成了新民主主义革命，建立了中华人民共和国，实现了民族独立、人民解放。",
                "制度意义：社会主义基本制度的确立，为当代中国一切发展进步奠定了制度基础。",
                "社会发展：极大地促进了我国社会生产力的发展。",
                "人民维度：使中国人民当家作主，建立自己的伟大国家。",
                "国际影响：进一步改变了世界政治经济格局，增强了社会主义的力量，对维护世界和平产生了积极影响。",
                "复兴维度：为实现中华民族伟大复兴提供充满新的活力的体制保证和快速发展的物质条件。",
            ]
        )
    )
    flow.append(p("四、五个领导人的具体内容", "H1"))

    flow.append(p("1. 毛泽东思想", "H2"))
    flow.append(
        bullets(
            [
                "主要内容：新民主主义革命理论；社会主义革命和社会主义建设理论；革命军队建设和军事战略的理论；政策和策略的理论；思想政治工作和文化工作的理论；党的建设理论。",
                "活的灵魂：毛泽东思想的活的灵魂，是贯穿于毛泽东思想各个组成部分的立场、观点和方法，他们有三个基本方面，即实事求是，群众路线，独立自主。",
                "历史地位：第一，毛泽东思想是马克思主义中国化时代化的第一个重大理论成果；第二，毛泽东思想是中国革命和建设的科学指南；第三，毛泽东思想是中国共产党和中国人民的宝贵精神财富。",
                "本人评价：毛泽东是伟大的马克思主义者，是伟大的无产阶级革命家、战略家和理论家。就他的一生来看，他的功绩远远大于他的过失。他的功绩是第一位的，错误是第二位的。",
            ]
        )
    )
    flow.append(p("新民主主义革命可套原句", "H2"))
    flow.append(
        bullets(
            [
                "总路线：无产阶级领导的，人民大众的，反对帝国主义、封建主义和官僚资本主义的革命。",
                "道路：中国革命走农村包围城市、武装夺取政权的道路。",
                "三大法宝：统一战线、武装斗争和党的建设。",
                "三大法宝关系：统一战线和武装斗争是中国革命的两个基本特点，是战胜敌人的两个基本武器。统一战线是实行武装斗争的统一战线，武装斗争是统一战线的中心支柱，党的组织则是掌握统一战线和武装斗争这两个武器以实行对敌冲锋陷阵的英勇战士。",
                "意义：揭示了近代中国革命发展的客观规律，解决了中国革命的一系列理论问题，科学地回答了中国革命向何处去的问题，以及中国革命的发展阶段问题，极大地丰富了马克思主义的理论宝库。",
            ]
        )
    )
    flow.append(p("社会主义革命和建设可套原句", "H2"))
    flow.append(
        bullets(
            [
                "过渡时期总路线：总路线的主要内容被概括为“一化三改”。“一化”即社会主义工业化；“三改”即对个体农业、手工业和对资本主义工商业的社会主义改造。",
                "社会主义改造经验：第一，坚持社会主义工业化建设与社会主义改造同时并举；第二，采取积极引导、逐步过渡的方式；第三，用和平方法进行改造。",
                "制度意义：社会主义基本制度的确立，为当代中国一切发展进步奠定了制度基础。",
                "初步探索意义：为开创中国特色社会主义提供了宝贵经验、理论准备、物质基础；丰富了科学社会主义的理论和实践。",
                "经验教训：必须把马克思主义与中国实际相结合，探索符合中国特点的社会主义建设道路；必须正确认识社会主义社会的主要矛盾和根本任务，集中力量发展生产力；必须从实际出发进行社会主义建设。",
            ]
        )
    )

    flow.append(p("2. 邓小平理论", "H2"))
    flow.append(
        bullets(
            [
                "首要的基本的理论问题：什么是社会主义、怎样建设社会主义。",
                "社会主义本质：解放生产力，发展生产力，消灭剥削，消除两极分化，最终达到共同富裕。",
                "精髓：解放思想、实事求是的思想路线，是邓小平理论的活的灵魂，是邓小平理论的精髓。",
                "历史地位：1、马克思列宁主义、毛泽东思想的继承和发展。2、中国特色社会主义理论体系的开篇之作。3、改革开放和社会主义现代化建设的科学指南。",
            ]
        )
    )
    flow.append(
        bullets(
            [
                "社会主义初级阶段两层含义：我国社会已经是社会主义社会。我们必须坚持而不能离开社会主义；我国的社会主义还在初级阶段。我们必须从这个实际出发，而不能超越这个阶段。",
                "基本路线：领导和团结全国各族人民，以经济建设为中心，坚持四项基本原则，坚持改革开放，自力更生，艰苦创业，为把我国建设成为富强、民主、文明、和谐、美丽的社会主义现代化国家而奋斗。",
                "市场经济理论：计划经济和市场经济不是划分社会制度的标志，计划经济不等于社会主义，市场经济也不等于资本主义；计划和市场都是经济手段。",
                "两个毫不动摇：必须毫不动摇巩固和发展公有制经济；必须毫不动摇鼓励、支持、引导非公有制经济发展。",
            ]
        )
    )
    flow.append(
        box(
            "邓小平理论“开篇之作”写法",
            [
                "邓小平作为中国特色社会主义理论的创立者，紧紧抓住“什么是社会主义、怎样建设社会主义”这个基本问题，响亮提出“走自己的道路，建设有中国特色的社会主义”的伟大号召，从此中国特色社会主义成为我们党全部理论和实践一以贯之的主题。",
                "邓小平理论第一次比较系统地初步回答了中国社会主义的一系列基本问题，指导我们党制定了在社会主义初级阶段的基本路线。这一科学理论体系为我们坚持走自己的路，建设中国特色社会主义提供了根本遵循。",
            ],
            PALETTE["soft_green"],
        )
    )

    flow.append(p("3. “三个代表”重要思想", "H2"))
    flow.append(
        bullets(
            [
                "核心观点：1、始终代表中国先进生产力的发展要求。2、始终代表中国先进文化的前进方向。3、始终代表中国最广大人民的根本利益。",
                "历史地位：“三个代表”重要思想是中国特色社会主义理论体系的丰富发展。",
                "问题意识：三个代表重要思想在邓小平理论的基础上，进一步回答了什么是社会主义、怎样建设社会主义的问题，创造性的回答了建设什么样的党、怎样建设党的问题，进一步深化了对中国特色社会主义的认识。",
                "理论武器：反映了当代世界和中国的发展变化对党和国家工作的新要求，是加强和改进党的建设、推进我国社会主义自我完善和发展的强大理论武器，是党和国家必须长期坚持的指导思想，是党和人民的宝贵精神财富。",
            ]
        )
    )
    flow.append(
        bullets(
            [
                "改革发展稳定：改革是动力，发展是目的，稳定是前提。",
                "处理关系：要把改革的力度、发展的速度和社会可承受的程度统一起来，把不断改善人民生活作为处理改革发展稳定关系的重要结合点，在社会稳定中推进改革发展，通过改革发展促进社会稳定。",
                "政治文明：建设社会主义政治文明，必须发展社会主义民主；必须坚持和完善中国特色社会主义政治制度；必须坚持依法治国，建设社会主义法治国家。",
            ]
        )
    )

    flow.append(p("4. 科学发展观", "H2"))
    flow.append(
        bullets(
            [
                "科学内涵：1、推动经济社会发展是科学发展观的第一要义。2、以人为本是科学发展观的核心立场。3、全面协调可持续是科学发展观的基本要求。4、统筹兼顾是科学发展观的根本方法。",
                "以人为本：以人为本是科学发展观的核心立场和根本立场，集中体现了马克思主义历史唯物论的基本原理，体现了我们党全心全意为人民服务的根本宗旨和推动经济社会发展的根本目的，是社会主义的本质特征。",
                "全面协调可持续：全面是指发展要有全面性、整体性；协调是指发展要有协调性、均衡性；可持续是指发展要有持久性、连续性。",
                "历史地位：科学发展观是对经济社会发展一般规律认识的深化，是马克思主义关于发展的世界观和方法论的集中体现，是中国特色社会主义理论体系的重要组成部分。",
                "精神实质：科学发展观最鲜明的精神实质是解放思想、实事求是、与时俱进、求真务实。",
            ]
        )
    )
    flow.append(
        bullets(
            [
                "继续扩写：科学发展观在邓小平理论和“三个代表”重要思想的基础上，用一系列具有鲜明时代特点的新思想、新观点、新论断，进一步回答了什么是社会主义、怎样建设社会主义和建设什么样的党、怎样建设党的问题，创造性地回答了新形势下实现什么样的发展、怎样发展等重大问题。",
                "指针地位：科学发展观是全面建设小康社会、加快推进社会主义现代化的根本指针。",
                "统筹兼顾：坚持统筹兼顾，必须正确认识和妥善处理中国特色社会主义事业中的重大关系。",
            ]
        )
    )

    flow.append(p("5. 习近平新时代中国特色社会主义思想和新时代", "H2"))
    flow.append(
        bullets(
            [
                "历史进程句：在中国特色社会主义新时代，我们党创立了习近平新时代中国特色社会主义思想，实现了马克思主义中国化时代化新的飞跃。",
                "新时代内涵：这个新时代，是承前启后、继往开来、在新的历史条件下继续夺取中国特色社会主义伟大胜利的时代，是决胜全面建成小康社会、进而全面建设社会主义现代化强国的时代。",
                "人民维度：是全国各族人民团结奋斗、不断创造美好生活、逐步实现全体人民共同富裕的时代。",
                "民族复兴：是全体中华儿女勠力同心、奋力实现中华民族伟大复兴中国梦的时代。",
                "世界维度：是我国日益走近世界舞台中央、不断为人类作出更大贡献的时代。",
                "两句收束：中国特色社会主义道路是实现中华民族伟大复兴的必由之路；中国特色社会主义理论体系是立足时代前沿、与时俱进的科学理论。",
            ]
        )
    )

    flow.append(PageBreak())
    flow.append(p("五、方法论速背", "H1"))
    flow.append(
        make_table(
            ["场景", "可直接写的原句"],
            [
                [
                    "马克思主义中国化时代化",
                    "第一，运用马克思主义的立场、观点和方法，观察时代、把握时代引领时代，解决中国革命、建设、改革中的实际问题。第二，总结和提炼中国革命、建设、改革的实践经验并将其上升为理论，不断丰富和发展马克思主义的理论宝库。第三，运用中国人民喜闻乐见的民族语言来阐述马克思主义，使其植根于中华优秀传统文化的土壤之中，具有中国特色、中国风格、中国气派。",
                ],
                [
                    "毛泽东思想方法论",
                    "毛泽东思想的活的灵魂，是贯穿于毛泽东思想各个组成部分的立场、观点和方法，他们有三个基本方面，即实事求是，群众路线，独立自主。",
                ],
                [
                    "邓小平理论方法论",
                    "解放思想、实事求是的思想路线，是邓小平理论的活的灵魂，是邓小平理论的精髓。",
                ],
                [
                    "科学发展观方法论",
                    "科学发展观最鲜明的精神实质是解放思想、实事求是、与时俱进、求真务实；统筹兼顾是科学发展观的根本方法。",
                ],
                [
                    "社会主义建设经验",
                    "必须把马克思主义与中国实际相结合，探索符合中国特点的社会主义建设道路；必须正确认识社会主义社会的主要矛盾和根本任务，集中力量发展生产力；必须从实际出发进行社会主义建设。",
                ],
                [
                    "改革发展稳定",
                    "改革是动力，发展是目的，稳定是前提。要把改革的力度、发展的速度和社会可承受的程度统一起来。",
                ],
            ],
            [38, 132],
        )
    )

    flow.append(p("六、题型套法", "H1"))
    flow.append(
        box(
            "A. 问“历史地位/指导意义/评价”",
            [
                "先答该理论的核心内容或回答的问题。",
                "再套历史地位原句，例如“第一个重大理论成果”“开篇之作”“丰富发展”“接续发展”。",
                "最后从理论宝库、科学指南、精神财富、国家民族、人民生活、世界影响中选三到五句。",
            ],
            PALETTE["soft_red"],
        )
    )
    flow.append(Spacer(1, 5))
    flow.append(
        box(
            "B. 问“为什么/如何理解”",
            [
                "先写背景：立足中国国情和时代特点，真正搞懂面临的时代课题。",
                "再写内容：把该题关键词对应到“总路线、基本路线、核心观点、科学内涵、方法论”。",
                "最后写意义：科学回答时代提出的重大理论和实践课题，创造新的理论成果。",
            ],
            PALETTE["soft_green"],
        )
    )
    flow.append(Spacer(1, 5))
    flow.append(
        box(
            "C. 问“五人中任意一个”",
            [
                "第一句点名理论；第二句写它回答了什么问题；第三句写它在谁的基础上继承发展；第四句写一脉相承又与时俱进；第五句回到题目关键词。",
                "万能收尾：都是党和国家必须长期坚持的指导思想，是全国各族人民团结奋斗的共同思想基础。",
            ],
            PALETTE["soft_blue"],
        )
    )

    flow.append(p("七、考前最后一页：硬背清单", "H1"))
    flow.append(
        make_table(
            ["必须会", "原句"],
            [
                ["毛泽东思想活的灵魂", "实事求是，群众路线，独立自主。"],
                ["新民主主义革命三大法宝", "统一战线、武装斗争和党的建设。"],
                ["新民主主义革命总路线", "无产阶级领导的，人民大众的，反对帝国主义、封建主义和官僚资本主义的革命。"],
                ["邓小平理论首要问题", "什么是社会主义、怎样建设社会主义。"],
                ["社会主义本质", "解放生产力，发展生产力，消灭剥削，消除两极分化，最终达到共同富裕。"],
                ["两个毫不动摇", "必须毫不动摇巩固和发展公有制经济；必须毫不动摇鼓励、支持、引导非公有制经济发展。"],
                ["三个代表核心观点", "始终代表中国先进生产力的发展要求；始终代表中国先进文化的前进方向；始终代表中国最广大人民的根本利益。"],
                ["科学发展观科学内涵", "推动经济社会发展是第一要义；以人为本是核心立场；全面协调可持续是基本要求；统筹兼顾是根本方法。"],
                ["理论关系", "马克思主义中国化时代化的理论成果是一脉相承又与时俱进的关系。"],
                ["新时代", "在中国特色社会主义新时代，我们党创立了习近平新时代中国特色社会主义思想，实现了马克思主义中国化时代化新的飞跃。"],
            ],
            [43, 127],
        )
    )
    flow.append(Spacer(1, 8))
    flow.append(
        p(
            "资料来源：D:/my_code/my_code/作业/毛概/2-2026知识点1（简答题）.pdf；D:/my_code/my_code/作业/毛概/3-2026知识点2（论述题）.pdf。",
            "NoteCn",
        )
    )
    flow.append(HRFlowable(width="100%", thickness=0.4, color=PALETTE["line"], spaceBefore=6, spaceAfter=4))
    flow.append(p("提示：本模板按“先搭理论链条，再落到题目关键词，再用意义句扩写”的顺序设计。正式答题时优先写题目直接要求的点，再使用通用句补充。", "NoteCn"))
    return flow


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    doc = TemplateDoc(OUT_PDF)
    doc.build(story())
    print(OUT_PDF)


if __name__ == "__main__":
    main()
