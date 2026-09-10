from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.enums import TA_CENTER
from reportlab.lib.units import mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, Preformatted

pdfmetrics.registerFont(TTFont('CN', r'C:\Windows\Fonts\Deng.ttf'))
s = getSampleStyleSheet()
s.add(ParagraphStyle(name='T', parent=s['Title'], fontName='CN', fontSize=20, leading=25, alignment=TA_CENTER, textColor=colors.HexColor('#16324F')))
s.add(ParagraphStyle(name='H', parent=s['Heading1'], fontName='CN', fontSize=14, leading=18, textColor=colors.HexColor('#16324F'), spaceBefore=8, spaceAfter=4))
s.add(ParagraphStyle(name='B', parent=s['BodyText'], fontName='CN', fontSize=9, leading=13, spaceAfter=4))
s.add(ParagraphStyle(name='C', fontName='CN', fontSize=8.5, leading=12, leftIndent=6, rightIndent=6, backColor=colors.HexColor('#F1F4F6'), borderPadding=6))

def p(x, st='B'): return Paragraph(x, s[st])
def tb(rows, widths):
    t=Table(rows, colWidths=widths, repeatRows=1)
    t.setStyle(TableStyle([('FONTNAME',(0,0),(-1,-1),'CN'),('FONTSIZE',(0,0),(-1,-1),8),('LEADING',(0,0),(-1,-1),10),('BACKGROUND',(0,0),(-1,0),colors.HexColor('#DCEAF4')),('GRID',(0,0),(-1,-1),.35,colors.HexColor('#AAB8C2')),('VALIGN',(0,0),(-1,-1),'MIDDLE'),('TOPPADDING',(0,0),(-1,-1),3),('BOTTOMPADDING',(0,0),(-1,-1),3)]))
    return t

story=[p('数电部分接线总结','T'), p('目标：以 555 的 12 kHz 为主时钟，得到 6 kHz、3 kHz 和 4 kHz 三路信号。')]
story += [p('总信号路径','H'), Preformatted('555 3脚 OUT（12 kHz）\n       ├──→ 第一片 74LS163：QA=6 kHz，QB=3 kHz\n       └──→ 第二片 74LS163：模3计数，QA或QB=4 kHz', s['C']), p('所有数字芯片共用 +5 V 和 GND；每片芯片电源旁建议放置 0.1 μF 去耦电容。')]
story += [p('一、555 基准时钟','H'), tb([['引脚','功能','接线'],['1','GND','接地'],['3','OUT','输出 12 kHz，并接两片 163 的 CLK'],['4','RST','接 +5 V'],['8','VCC','接 +5 V']], [18*mm,25*mm,125*mm]), p('先用频率探针确认 555 的 3 脚为 12.00 kHz。')]
story += [p('二、第一片 74LS163：得到 6 kHz 和 3 kHz','H'), tb([['引脚','名称','接线'],['1','CLR（低有效）','+5 V'],['2','CLK','接 555 的 3 脚 OUT（12 kHz）'],['3~6','A、B、C、D','全部接 GND'],['7','ENP','+5 V'],['8','GND','接地'],['9','LOAD（低有效）','+5 V'],['10','ENT','+5 V'],['16','VCC','+5 V']], [20*mm,35*mm,113*mm]), Preformatted('11脚 QA → 6 kHz（12 kHz ÷ 2）\n12脚 QB → 3 kHz（12 kHz ÷ 4）', s['C']), p('3 kHz 滤波器的输入取第一片 163 的 12 脚 QB，不要误接 11 脚 QA。')]
story += [p('三、第二片 74LS163：模3分频得到 4 kHz','H'), tb([['引脚','名称','接线'],['1','CLR（低有效）','先接 +5 V；不要悬空'],['2','CLK','接 555 的 3 脚 OUT（12 kHz），不能接 6 kHz'],['3~6','A、B、C、D','全部接 GND'],['7','ENP','+5 V'],['8','GND','接地'],['9','LOAD（低有效）','接 7400 的 6 脚输出'],['10','ENT','+5 V'],['16','VCC','+5 V']], [20*mm,35*mm,113*mm]), p('计数到 2（QA=0、QB=1）时，用 7400 令 LOAD=0；下一个时钟沿同步装入 0000，计数序列为：'), Preformatted('0 → 1 → 2 → 0 → 1 → 2 → …', s['C']), p('7400 接线：', 'H'), tb([['7400引脚','连接'],['14、7','14脚→+5 V；7脚→GND'],['1、2、3','1、2脚都接第二片 163 的 QA（11脚）；3脚得到 /QA'],['4、5、6','4脚接第二片 163 的 QB（12脚）；5脚接 7400 的 3脚；6脚接第二片 163 的 LOAD（9脚）']], [42*mm,128*mm]), p('第二片 163 的 QA（11脚）或 QB（12脚）都可作为 4 kHz 输出，频率相同。')]
story += [p('四、数电检查顺序','H'), tb([['检查点','应测频率','说明'],['555 的 3脚 OUT','12.00 kHz','主时钟'],['第一片 163 的 11脚 QA','6.00 kHz','÷2'],['第一片 163 的 12脚 QB','3.00 kHz','÷4，接 3 kHz 滤波器'],['第二片 163 的 QA或QB','4.00 kHz','模3分频，接 4 kHz 滤波器']], [48*mm,42*mm,80*mm]), p('排查时一次只测一个检查点。若第一片正常而第二片没有 4 kHz，重点检查 7400 的 /QA、QB 以及第二片 163 的 9 脚 LOAD。')]
story += [p('五、最终输出','H'), Preformatted('3 kHz：第一片 163 的 12脚 QB → 3 kHz滤波器\n4 kHz：第二片 163 的 QA或QB → 4 kHz滤波器\n6 kHz：第一片 163 的 11脚 QA（若需要保留）', s['C']), p('7474 若只用于消除尖峰，应放在相应输出之后作重新定时/整形，不要接成再次二分频。频率比为 3 kHz : 4 kHz = 3 : 4；幅度比和相位差属于后级模拟电路。')]

def footer(c, d):
    c.saveState(); c.setFont('CN',8); c.setFillColor(colors.HexColor('#666666')); c.drawString(15*mm,8*mm,'电子技术课程设计 · 数字分频部分'); c.drawRightString(A4[0]-15*mm,8*mm,f'第 {d.page} 页'); c.restoreState()

SimpleDocTemplate('数电部分接线总结.pdf', pagesize=A4, rightMargin=15*mm, leftMargin=15*mm, topMargin=13*mm, bottomMargin=13*mm, title='数电部分接线总结').build(story, onFirstPage=footer, onLaterPages=footer)
print('数电部分接线总结.pdf')
