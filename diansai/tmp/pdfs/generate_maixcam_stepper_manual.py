from __future__ import annotations

import html
from pathlib import Path

from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER, TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (
    BaseDocTemplate,
    Flowable,
    Frame,
    KeepTogether,
    ListFlowable,
    ListItem,
    NextPageTemplate,
    PageBreak,
    PageTemplate,
    Paragraph,
    Preformatted,
    Spacer,
    Table,
    TableStyle,
)
from reportlab.platypus.tableofcontents import TableOfContents


ROOT = Path(__file__).resolve().parents[2]
OUTPUT_DIR = ROOT / "output" / "pdf"
MD_PATH = OUTPUT_DIR / "maixcam_stepper_motor_manual.md"
PDF_PATH = OUTPUT_DIR / "maixcam_stepper_motor_manual.pdf"

TITLE = "MaixCAM Pro 闭环步进电机使用说明书"
SUBTITLE = "ZDT X42S / Emm V5 串口协议 - 零点保存、断电回零与绝对角度控制"
VERSION = "V1.0"
DATE = "2026-07-31"

CHARCOAL = colors.HexColor("#20272B")
TEAL = colors.HexColor("#087E8B")
TEAL_DARK = colors.HexColor("#075E66")
RUST = colors.HexColor("#B33A2B")
AMBER = colors.HexColor("#E0A12A")
INK = colors.HexColor("#263238")
MUTED = colors.HexColor("#5F6B70")
LINE = colors.HexColor("#CBD5D8")
PAPER = colors.HexColor("#F5F7F7")
PALE_TEAL = colors.HexColor("#E5F3F4")
PALE_RED = colors.HexColor("#FCECE9")
WHITE = colors.white


SECTIONS = [
    {
        "title": "1. 使用范围与结论",
        "blocks": [
            {
                "type": "lead",
                "text": "本说明书适用于 MaixCAM Pro 通过 UART 控制 ZDT X42S 或兼容 Emm V5 串口协议的闭环步进电机。当前工程使用电机内部保存的单圈零点，不增加限位开关或霍尔传感器。",
            },
            {
                "type": "warning",
                "title": "适用前提",
                "items": [
                    "机构正常运动范围必须小于一整圈，当前平衡机构软件限幅为 -12° 到 +12°。",
                    "电机驱动器必须支持 `0x93` 保存单圈零点、`0x9A` 单圈回零和 `0x36` 实际位置查询。",
                    "若机构可能连续旋转多圈，必须改用限位/霍尔回零或驱动器的多圈回零方案，不能照搬本说明书。",
                ],
            },
            {
                "type": "table",
                "headers": ["当前结论", "实际含义"],
                "rows": [
                    ["零点只保存一次", "机械位置摆正后发送 `01 93 88 01 6B`，随后关闭标定开关。"],
                    ["正常上电只回零", "发送 `01 9A 00 00 6B`，再读取真实位置确认到达 0°。"],
                    ["保留原串口", "程序继续使用 `/dev/ttyS0`、115200 bit/s，不改用户现有 UART 端口。"],
                    ["回零成功有反馈依据", "实际位置连续 2 次进入 ±1.5° 才算成功，最长等待 10 s。"],
                ],
                "widths": [43, 125],
            },
        ],
    },
    {
        "title": "2. 上电前安全检查",
        "blocks": [
            {
                "type": "danger",
                "title": "必须执行",
                "items": [
                    "断电接线。禁止带电插拔电机相线、驱动器电源线或 UART 线。",
                    "电机和驱动器使用独立电源，禁止从 MaixCAM GPIO 或板载小电源接口直接给电机供电。",
                    "MaixCAM GND 与电机驱动器信号 GND 必须共地。",
                    "连接 MaixCAM RX 前，先测量电机 TTL_TX 高电平，确认不会超过 MaixCAM 可接受的 3.3 V 逻辑电平；不确定时使用电平转换。",
                    "首次测试卸除重负载，预留急停和断电空间，确认机构不会撞击机械端点。",
                ],
            },
            {
                "type": "checklist",
                "items": [
                    "电机地址为 `0x01`。",
                    "驱动器串口波特率为 115200，数据格式与当前驱动器设置一致。",
                    "驱动器细分与程序的 3200 脉冲/圈一致。",
                    "TX/RX 已交叉连接，GND 已共地。",
                    "机械零点位置已人工确认，机构在该位置没有预紧或干涉。",
                ],
            },
        ],
    },
    {
        "title": "3. 硬件连接",
        "blocks": [
            {"type": "wiring"},
            {
                "type": "table",
                "headers": ["MaixCAM Pro", "连接到电机驱动器", "说明"],
                "rows": [
                    ["UART0 TX（通常为 A16）", "TTL_RX", "发送 Emm V5 指令。"],
                    ["UART0 RX（通常为 A17）", "TTL_TX", "接收普通应答和 8 字节位置帧。"],
                    ["GND", "信号 GND", "必须共地。"],
                    ["不连接", "电机主电源输入", "电机独立供电，不从 MaixCAM 取电。"],
                ],
                "widths": [50, 52, 66],
            },
            {
                "type": "note",
                "title": "端口保持不变",
                "text": "当前代码明确使用 `/dev/ttyS0`。如果你的现有接线已经能正常收发，不需要改到 UART1，也不需要增加 `pinmap` 配置。实体引脚仍应以所用 MaixCAM Pro 版本的丝印和原理图为准。",
            },
        ],
    },
    {
        "title": "4. 软件文件与当前参数",
        "blocks": [
            {
                "type": "table",
                "headers": ["文件", "用途", "正常使用方式"],
                "rows": [
                    ["`motor_calibration(1).py`", "独立保存零点和回零验证", "推荐先用它完成首次标定和断电复测。"],
                    ["`diansai/main.py`", "视觉检测、串级控制和电机角度输出", "确认独立标定成功后再运行。"],
                    ["F407 `Emm_V5.c`", "厂商协议函数参考", "用于核对命令字节，不部署到 MaixCAM。"],
                ],
                "widths": [52, 55, 61],
            },
            {
                "type": "table",
                "headers": ["参数", "当前值", "作用"],
                "rows": [
                    ["`UART_DEV`", "`/dev/ttyS0`", "MaixCAM 串口设备。"],
                    ["`UART_BAUD`", "115200", "串口波特率。"],
                    ["`MOTOR_ID`", "`0x01`", "电机地址。"],
                    ["`CHECKSUM`", "`0x6B`", "Emm V5 固定帧尾。"],
                    ["`PULSES_PER_DEGREE`", "3200 / 360", "按 3200 脉冲/圈换算角度。"],
                    ["`MOTOR_POSITION_LIMIT_DEG`", "12.0°", "集成程序的机械角度软件限幅。"],
                    ["`MOTOR_SPEED_RPM`", "100 r/min", "集成程序位置命令速度。"],
                    ["`MOTOR_ACC`", "80", "集成程序位置命令加速度参数。"],
                    ["`MOTOR_HOME_TIMEOUT_MS`", "10000 ms", "回零最长等待时间。"],
                    ["`MOTOR_HOME_TOLERANCE_DEG`", "1.5°", "回零到位容差。"],
                    ["`MOTOR_HOME_SETTLE_SAMPLES`", "2", "连续到位样本数。"],
                ],
                "widths": [61, 37, 70],
            },
        ],
    },
    {
        "title": "5. 零点、视觉 O 点与位置坐标",
        "blocks": [
            {
                "type": "table",
                "headers": ["概念", "保存位置", "用途", "是否相同"],
                "rows": [
                    ["电机机械零点", "电机驱动器内部存储", "断电后让机构回到固定机械角度。", "不是视觉 O 点"],
                    ["视觉 O 点", "MaixCAM 程序变量 `X_ZERO_PX`", "把图像中的球位置换算成厘米。", "不是电机零点"],
                    ["当前命令角", "Python 变量 `current_motor_angle`", "减少重复发送相近目标。", "不等于实时反馈"],
                    ["实际反馈角", "`0x36` 返回帧", "确认回零是否真正完成。", "来自编码器"],
                ],
                "widths": [40, 51, 55, 30],
            },
            {
                "type": "warning",
                "title": "最常见误区",
                "items": [
                    "`01 0A 6D 6B` 只是将当前坐标清零，不等同于保存单圈回零零点。",
                    "收到 `0x9A` 普通应答只表示命令帧被接收，不表示电机已经回到零点。",
                    "Python 中把角度变量写成 0，也不能证明电机真实位置为 0。",
                ],
            },
        ],
    },
    {
        "title": "6. Emm V5 关键指令",
        "blocks": [
            {
                "type": "table",
                "headers": ["功能", "发送帧（地址 1）", "程序函数", "回复/判定"],
                "rows": [
                    ["读取实际位置", "`01 36 6B`", "`read_position()`", "8 字节 `0x36` 帧"],
                    ["使能", "`01 F3 AB 01 00 6B`", "`enable()`", "4 字节普通应答"],
                    ["禁能", "`01 F3 AB 00 00 6B`", "`disable()`", "4 字节普通应答"],
                    ["保存单圈零点", "`01 93 88 01 6B`", "`save_origin()`", "普通应答 + 断电复测"],
                    ["单圈就近回零", "`01 9A 00 00 6B`", "`home()`", "实际角度连续到位"],
                    ["立即停止", "`01 FE 98 00 6B`", "`stop()`", "普通应答"],
                    ["绝对位置", "13 字节 `0xFD` 帧", "`goto_angle()`", "目标由脉冲数给出"],
                ],
                "widths": [34, 51, 44, 47],
            },
            {
                "type": "code",
                "title": "实际位置回复格式",
                "text": "[ADDR, 0x36, SIGN, POS3, POS2, POS1, POS0, 0x6B]\nraw = POS3<<24 | POS2<<16 | POS1<<8 | POS0\ndeg = raw * 360 / 65536\nSIGN != 0 时角度取负值",
            },
            {
                "type": "note",
                "title": "应答状态说明",
                "text": "普通命令应答为 4 字节：地址、功能码、状态字节、`0x6B`。当前程序主要校验地址、功能码和帧尾；保存零点是否真正持久化，最终必须通过断电后回零测试确认。",
            },
        ],
    },
    {
        "title": "7. 第一次保存机械零点",
        "pagebreak": True,
        "blocks": [
            {
                "type": "lead",
                "text": "推荐使用独立的 `motor_calibration(1).py` 完成首次标定。不要先运行完整视觉控制程序，以免尚未确认零点时机构突然运动。",
            },
            {
                "type": "steps",
                "items": [
                    "关闭电机和 MaixCAM 电源，检查 UART 交叉连接、共地和独立电机电源。",
                    "人工把机构放到希望长期保存的机械零点，例如球杆完全水平的位置。",
                    "打开 `motor_calibration(1).py`，确认 `UART_DEV = \"/dev/ttyS0\"`、`MOTOR_ID = 0x01`。",
                    "把 `CALIBRATION_MODE = 0` 临时改为 `CALIBRATION_MODE = 1`。",
                    "上电后运行脚本。程序先查询位置、使能电机，再发送 `01 93 88 01 6B`。",
                    "看到“零点保存命令已应答”和“零点标定成功”后停止程序。",
                    "立即把 `CALIBRATION_MODE` 改回 `0`。不要带着 `1` 进入日常运行。",
                ],
            },
            {
                "type": "code",
                "title": "标定时唯一需要改的参数",
                "text": "# first calibration run only\nCALIBRATION_MODE = 1\n\n# restore immediately after calibration\nCALIBRATION_MODE = 0",
            },
            {
                "type": "danger",
                "title": "禁止重复覆盖",
                "items": [
                    "如果每次启动都保持 `CALIBRATION_MODE = 1`，当前任意位置都会成为新零点，原零点将失去意义。",
                    "调整机械结构后确实需要重标定时，才重新执行一次本节流程。",
                ],
            },
        ],
    },
    {
        "title": "8. 断电回零验证",
        "blocks": [
            {
                "type": "steps",
                "items": [
                    "确认 `CALIBRATION_MODE = 0`。",
                    "完全断电，等待驱动器和 MaixCAM 停止。",
                    "在允许的单圈范围内手动把机构转离机械零点，避免撞到端点。",
                    "重新上电并运行独立脚本。",
                    "程序发送 `01 9A 00 00 6B`，随后每 150 ms 查询一次真实位置。",
                    "只有实际角度连续 2 次进入 ±1.5°，程序才打印“回零完成”。",
                    "重复至少 3 次断电测试，确认每次都回到同一机械位置。",
                ],
            },
            {
                "type": "flow",
                "nodes": ["读取位置", "使能电机", "发送 0x9A", "轮询 0x36", "连续到位", "禁能退出"],
            },
            {
                "type": "table",
                "headers": ["结果", "判定"],
                "rows": [
                    ["合格", "三次断电后都能回到同一位置，日志有完整 `0x9A` 应答和连续到位反馈。"],
                    ["不合格", "只打印应答但不运动、回到不同位置、位置一直未知或 10 s 超时。"],
                ],
                "widths": [34, 134],
            },
        ],
    },
    {
        "title": "9. 集成到视觉控制 main.py",
        "blocks": [
            {
                "type": "table",
                "headers": ["运行场景", "`MOTOR_SAVE_ORIGIN_ON_BOOT`", "`MOTOR_HOME_ON_BOOT`", "说明"],
                "rows": [
                    ["首次标定", "`True`", "`True` 或 `False`", "只运行一次，机构必须已人工摆正。"],
                    ["正常运行", "`False`", "`True`", "推荐配置：保留原零点，上电回零。"],
                    ["调试视觉但不回零", "`False`", "`False`", "仅在已保证机构安全时使用。"],
                ],
                "widths": [35, 47, 41, 53],
            },
            {
                "type": "code",
                "title": "当前正常运行配置",
                "text": "UART_DEV = \"/dev/ttyS0\"\nMOTOR_SAVE_ORIGIN_ON_BOOT = False\nMOTOR_HOME_ON_BOOT = True",
            },
            {
                "type": "flow",
                "nodes": ["创建 UART", "读取位置", "使能电机", "上电回零", "视觉控制", "退出禁能"],
            },
            {
                "type": "note",
                "title": "故障降级",
                "text": "`initialize_motor()` 如果读不到有效位置、使能无完整应答或回零失败，会把 `motor.connected` 设为 `False`。视觉仍可运行，但 `goto_motor_angle()` 不会继续输出运动命令。",
            },
        ],
    },
    {
        "title": "10. 绝对角度控制与参数换算",
        "blocks": [
            {
                "type": "paragraph",
                "text": "位置命令使用绝对模式 `raF = 1`。正角度方向字节为 `0x00`，负角度方向字节为 `0x01`。程序先做软件限幅，再把角度换算成脉冲数。",
            },
            {
                "type": "code",
                "title": "换算公式",
                "text": "pulses = round(abs(angle_deg) * 3200 / 360)\n12 deg -> round(106.667) = 107 pulses\n30 deg -> round(266.667) = 267 pulses",
            },
            {
                "type": "warning",
                "title": "细分必须匹配",
                "items": [
                    "如果驱动器实际一圈不是 3200 脉冲，角度会按比例错误。",
                    "例如实际为 6400 脉冲/圈而程序仍写 3200，命令 12° 只会运动约 6°。",
                    "修改驱动器细分后，必须同步修改 `PULSES_PER_DEGREE`。",
                ],
            },
            {
                "type": "note",
                "title": "机构映射",
                "text": "完整视觉程序通过 `PIPE_MOTOR_CALIBRATION` 把真实球杆角度映射成电机绝对角度。先完成电机零点和回零，再测量机构映射；不要用机构映射误差去补偿一个尚未标定的电机零点。",
            },
        ],
    },
    {
        "title": "11. 日志判读与故障排查",
        "blocks": [
            {
                "type": "table",
                "headers": ["现象/日志", "优先原因", "处理顺序"],
                "rows": [
                    ["`电机无响应`", "串口或接线不通", "确认 `/dev/ttyS0`、115200、地址 1；检查 TX/RX 交叉、共地、TTL 电平。"],
                    ["能发送但无位置帧", "RX 路径或驱动器回复设置异常", "检查电机 TTL_TX 到 MaixCAM RX；用逻辑分析仪确认是否有 8 字节 `0x36` 回复。"],
                    ["`0x93` 有应答但断电不回零", "零点未持久化或随后又被覆盖", "确认保存标志为 1；标定后立即恢复模式 0；重新做完整断电复测。"],
                    ["`0x9A` 有应答但电机不动", "已在零点、未保存原点、驱动器参数或使能异常", "先读实际角度；确认使能；重新标定；核对驱动器回零功能。"],
                    ["回零到不同位置", "超出单圈假设或零点被重复保存", "限制手动转动范围；检查标定开关；多圈机构改用传感器回零。"],
                    ["`homing timeout`", "机械卡滞、位置不收敛或反馈中断", "立即停机；检查机构、供电、位置帧、10 s 超时和 ±1.5° 容差。"],
                    ["角度比例错误", "3200 脉冲/圈不匹配", "核对驱动器细分并同步修改换算参数。"],
                    ["正负方向相反", "机械安装方向与软件符号不同", "断电后确认机构方向；需要时统一修改方向约定，不要带电交换相线。"],
                    ["完整程序只运行视觉", "初始化把电机标记为离线", "查看前序日志是位置查询、使能还是回零失败；修复后重新启动。"],
                ],
                "widths": [43, 55, 78],
            },
            {
                "type": "warning",
                "title": "不要用延时伪造完成",
                "items": [
                    "固定等待 1 s 后直接打印成功不是有效验证。",
                    "判断依据应当是实际位置反馈，或进一步接入驱动器正式回零状态 `S_OFLAG`。",
                ],
            },
        ],
    },
    {
        "title": "12. 参数调整建议",
        "blocks": [
            {
                "type": "table",
                "headers": ["需要调整的现象", "参数", "建议"],
                "rows": [
                    ["正常回零超过 10 s", "`HOME_TIMEOUT_MS` / `MOTOR_HOME_TIMEOUT_MS`", "先排除卡滞和速度问题，再适当增大，不能靠无限延时掩盖故障。"],
                    ["到零附近抖动、难以完成", "`HOME_TOLERANCE_DEG`", "可从 1.5° 小幅增加，但必须满足机构精度要求。"],
                    ["偶发误判到位", "`HOME_SETTLE_SAMPLES`", "从 2 增加到 3 或 4，提高稳定性但延长完成时间。"],
                    ["串口偶发慢回复", "`COMMAND_TIMEOUT_MS`", "从 500 ms 小幅增加；同时排查缓冲和接线。"],
                    ["机构动作过猛", "`MOTOR_SPEED_RPM`、`MOTOR_ACC`", "降低后无负载测试，再逐步增加。回零速度通常由驱动器回零参数控制。"],
                    ["角度超出安全范围", "`MOTOR_POSITION_LIMIT_DEG`", "按机械结构收紧，不要为了追求控制量盲目放大。"],
                ],
                "widths": [46, 58, 72],
            },
        ],
    },
    {
        "title": "13. 最终验收清单",
        "blocks": [
            {
                "type": "checklist",
                "items": [
                    "串口仍为 `/dev/ttyS0`，没有擅自切换 UART。",
                    "电机地址、波特率、帧尾与驱动器一致。",
                    "电机独立供电，MaixCAM 与驱动器共地，RX 电平安全。",
                    "首次标定只执行一次 `0x93` 保存命令。",
                    "标定后所有正常运行配置都关闭保存零点开关。",
                    "至少完成 3 次断电、手动转开、重新回零测试。",
                    "每次回零都有实际 `0x36` 位置反馈并连续进入容差。",
                    "绝对角度方向、比例和 ±12° 软件限幅已在轻载下验证。",
                    "机械零点与视觉 O 点分别标定，没有混为同一个参数。",
                    "故障时程序停止发送运动命令，人工急停路径可用。",
                ],
            },
            {
                "type": "danger",
                "title": "硬件验证状态",
                "items": [
                    "本说明书依据当前源码和 Emm V5 命令映射生成；语法与模拟串口分包测试已通过。",
                    "真实电机的 EEPROM 零点保存、断电回零精度、负载能力和 TTL 电平必须在你的 MaixCAM Pro 与电机实物上完成验收。",
                ],
            },
        ],
    },
    {
        "title": "附录 A. 当前配置速查",
        "blocks": [
            {
                "type": "code",
                "title": "独立标定脚本 - 正常回零",
                "text": "UART_DEV = \"/dev/ttyS0\"\nUART_BAUD = 115200\nMOTOR_ID = 0x01\nCALIBRATION_MODE = 0",
            },
            {
                "type": "code",
                "title": "完整视觉程序 - 正常启动",
                "text": "UART_DEV = \"/dev/ttyS0\"\nMOTOR_SAVE_ORIGIN_ON_BOOT = False\nMOTOR_HOME_ON_BOOT = True\nMOTOR_POSITION_LIMIT_DEG = 12.0",
            },
            {
                "type": "table",
                "headers": ["资料", "位置"],
                "rows": [
                    ["完整视觉程序", "`D:/my_code/my_code/diansai/main.py`"],
                    ["独立标定脚本", "`D:/xwechat_files_1/.../2026-07/motor_calibration(1).py`"],
                    ["F407 协议参考", "`F407_Emm_SingleTurnHold/Src/Emm_V5.c`"],
                    ["MaixPy UART 文档", "`https://wiki.sipeed.com/maixpy/doc/zh/peripheral/uart.html`"],
                ],
                "widths": [45, 123],
            },
        ],
    },
]


def markdown_text(value: str) -> str:
    return value


def write_markdown() -> None:
    lines = [
        f"# {TITLE}",
        "",
        f"> {SUBTITLE}",
        "",
        f"- 版本：{VERSION}",
        f"- 日期：{DATE}",
        "- 当前串口：`/dev/ttyS0`，115200 bit/s",
        "- 当前电机地址：`0x01`",
        "- 适用运动范围：单圈内；当前集成程序限幅 ±12°",
        "",
        "## 目录",
        "",
    ]
    for section in SECTIONS:
        lines.append(f"- [{section['title']}](#{section['title'].lower().replace(' ', '-').replace('.', '')})")
    lines.extend(["", "---", ""])

    for section in SECTIONS:
        lines.extend([f"## {section['title']}", ""])
        for block in section["blocks"]:
            kind = block["type"]
            if kind in {"lead", "paragraph"}:
                lines.extend([markdown_text(block["text"]), ""])
            elif kind in {"warning", "danger", "note"}:
                marker = "警告" if kind == "danger" else "注意"
                lines.append(f"> **{marker}：{block['title']}**")
                if "text" in block:
                    lines.append(f"> {block['text']}")
                else:
                    for item in block["items"]:
                        lines.append(f"> - {item}")
                lines.append("")
            elif kind == "checklist":
                for item in block["items"]:
                    lines.append(f"- [ ] {item}")
                lines.append("")
            elif kind == "steps":
                for index, item in enumerate(block["items"], 1):
                    lines.append(f"{index}. {item}")
                lines.append("")
            elif kind == "table":
                lines.append("| " + " | ".join(block["headers"]) + " |")
                lines.append("| " + " | ".join(["---"] * len(block["headers"])) + " |")
                for row in block["rows"]:
                    escaped = [str(cell).replace("|", "\\|") for cell in row]
                    lines.append("| " + " | ".join(escaped) + " |")
                lines.append("")
            elif kind == "code":
                lines.extend([f"### {block['title']}", "", "```text", block["text"], "```", ""])
            elif kind == "wiring":
                lines.extend([
                    "```text",
                    "MaixCAM Pro                         ZDT X42S / Emm V5",
                    "UART0 TX (/dev/ttyS0)  ----------> TTL_RX",
                    "UART0 RX (/dev/ttyS0)  <---------- TTL_TX",
                    "GND                     ----------- GND",
                    "                                        + 独立电机电源",
                    "```",
                    "",
                ])
            elif kind == "flow":
                lines.extend(["`" + " -> ".join(block["nodes"]) + "`", ""])

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    MD_PATH.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


class ManualDocTemplate(BaseDocTemplate):
    def afterFlowable(self, flowable):
        if isinstance(flowable, Paragraph) and flowable.style.name == "ManualH1":
            text = flowable.getPlainText()
            key = f"section-{self.seq.nextf('section')}"
            self.canv.bookmarkPage(key)
            self.canv.addOutlineEntry(text, key, level=0, closed=False)
            self.notify("TOCEntry", (0, text, self.page, key))


class WiringDiagram(Flowable):
    def __init__(self, width=168 * mm, height=44 * mm):
        super().__init__()
        self.width = width
        self.height = height

    def draw(self):
        c = self.canv
        box_w = 47 * mm
        box_h = 27 * mm
        y = 8 * mm
        right_x = self.width - box_w
        c.setFillColor(PALE_TEAL)
        c.setStrokeColor(TEAL)
        c.roundRect(0, y, box_w, box_h, 3 * mm, fill=1, stroke=1)
        c.roundRect(right_x, y, box_w, box_h, 3 * mm, fill=1, stroke=1)
        c.setFillColor(CHARCOAL)
        c.setFont("CNBold", 11)
        c.drawCentredString(box_w / 2, y + 17 * mm, "MaixCAM Pro")
        c.drawCentredString(right_x + box_w / 2, y + 17 * mm, "ZDT X42S")
        c.setFont("CNBody", 8.5)
        c.drawCentredString(box_w / 2, y + 10 * mm, "/dev/ttyS0")
        c.drawCentredString(right_x + box_w / 2, y + 10 * mm, "Emm V5")

        start_x = box_w + 3 * mm
        end_x = right_x - 3 * mm
        rows = [
            (y + 22 * mm, "TX", "TTL_RX", True, TEAL),
            (y + 15 * mm, "RX", "TTL_TX", False, TEAL),
            (y + 8 * mm, "GND", "GND", True, CHARCOAL),
        ]
        c.setLineWidth(1.4)
        for line_y, left, right, forward, color in rows:
            c.setStrokeColor(color)
            c.line(start_x, line_y, end_x, line_y)
            if forward:
                c.line(end_x - 3 * mm, line_y + 1.5 * mm, end_x, line_y)
                c.line(end_x - 3 * mm, line_y - 1.5 * mm, end_x, line_y)
            else:
                c.line(start_x + 3 * mm, line_y + 1.5 * mm, start_x, line_y)
                c.line(start_x + 3 * mm, line_y - 1.5 * mm, start_x, line_y)
            c.setFillColor(INK)
            c.setFont("CNBody", 7.5)
            c.drawString(start_x + 2 * mm, line_y + 1.5 * mm, left)
            c.drawRightString(end_x - 2 * mm, line_y + 1.5 * mm, right)
        c.setFillColor(RUST)
        c.setFont("CNBold", 8.5)
        c.drawCentredString(self.width / 2, 2 * mm, "电机独立供电；只连接 UART 信号和共地")


class FlowDiagram(Flowable):
    def __init__(self, nodes, width=168 * mm, height=24 * mm):
        super().__init__()
        self.nodes = nodes
        self.width = width
        self.height = height

    def draw(self):
        c = self.canv
        gap = 3 * mm
        node_w = (self.width - gap * (len(self.nodes) - 1)) / len(self.nodes)
        node_h = 11 * mm
        y = 6 * mm
        for index, label in enumerate(self.nodes):
            x = index * (node_w + gap)
            c.setFillColor(PALE_TEAL if index not in {2, 3} else colors.HexColor("#FFF4D6"))
            c.setStrokeColor(TEAL if index not in {2, 3} else AMBER)
            c.roundRect(x, y, node_w, node_h, 2 * mm, fill=1, stroke=1)
            c.setFillColor(INK)
            c.setFont("CNBody", 7.2)
            c.drawCentredString(x + node_w / 2, y + 4.1 * mm, label)
            if index < len(self.nodes) - 1:
                line_start = x + node_w
                line_end = line_start + gap
                line_y = y + node_h / 2
                c.setStrokeColor(MUTED)
                c.line(line_start, line_y, line_end, line_y)
                c.line(line_end - 1.5 * mm, line_y + 1 * mm, line_end, line_y)
                c.line(line_end - 1.5 * mm, line_y - 1 * mm, line_end, line_y)


def esc(value: str) -> str:
    return html.escape(str(value)).replace("\n", "<br/>")


def make_styles():
    pdfmetrics.registerFont(TTFont("CNBody", r"C:\Windows\Fonts\Deng.ttf"))
    pdfmetrics.registerFont(TTFont("CNBold", r"C:\Windows\Fonts\simhei.ttf"))
    pdfmetrics.registerFont(TTFont("Code", r"C:\Windows\Fonts\consola.ttf"))

    base = getSampleStyleSheet()
    styles = {
        "cover_title": ParagraphStyle(
            "CoverTitle", parent=base["Title"], fontName="CNBold", fontSize=27,
            leading=36, textColor=WHITE, alignment=TA_LEFT, spaceAfter=8 * mm,
        ),
        "cover_subtitle": ParagraphStyle(
            "CoverSubtitle", parent=base["Normal"], fontName="CNBody", fontSize=12,
            leading=19, textColor=colors.HexColor("#D9E7E8"), alignment=TA_LEFT,
        ),
        "cover_meta": ParagraphStyle(
            "CoverMeta", parent=base["Normal"], fontName="CNBody", fontSize=10,
            leading=16, textColor=INK,
        ),
        "h1": ParagraphStyle(
            "ManualH1", parent=base["Heading1"], fontName="CNBold", fontSize=17,
            leading=23, textColor=TEAL_DARK, spaceBefore=5 * mm, spaceAfter=3.2 * mm,
            keepWithNext=True,
        ),
        "h2": ParagraphStyle(
            "ManualH2", parent=base["Heading2"], fontName="CNBold", fontSize=11,
            leading=16, textColor=CHARCOAL, spaceBefore=2 * mm, spaceAfter=1.5 * mm,
            keepWithNext=True,
        ),
        "body": ParagraphStyle(
            "ManualBody", parent=base["BodyText"], fontName="CNBody", fontSize=9.4,
            leading=15.2, textColor=INK, alignment=TA_LEFT, spaceAfter=2.2 * mm,
            wordWrap="CJK",
        ),
        "lead": ParagraphStyle(
            "ManualLead", parent=base["BodyText"], fontName="CNBody", fontSize=10.4,
            leading=17, textColor=CHARCOAL, backColor=PALE_TEAL, borderPadding=8,
            borderColor=TEAL, borderWidth=0.8, borderRadius=2, spaceAfter=3 * mm,
            wordWrap="CJK",
        ),
        "small": ParagraphStyle(
            "ManualSmall", parent=base["BodyText"], fontName="CNBody", fontSize=8,
            leading=11.5, textColor=INK, wordWrap="CJK",
        ),
        "table_header": ParagraphStyle(
            "TableHeader", parent=base["BodyText"], fontName="CNBold", fontSize=8,
            leading=10.5, textColor=WHITE, alignment=TA_CENTER, wordWrap="CJK",
        ),
        "table_cell": ParagraphStyle(
            "TableCell", parent=base["BodyText"], fontName="CNBody", fontSize=7.8,
            leading=11, textColor=INK, wordWrap="CJK",
        ),
        "callout_title": ParagraphStyle(
            "CalloutTitle", parent=base["BodyText"], fontName="CNBold", fontSize=9,
            leading=13, textColor=CHARCOAL, wordWrap="CJK",
        ),
        "callout_body": ParagraphStyle(
            "CalloutBody", parent=base["BodyText"], fontName="CNBody", fontSize=8.5,
            leading=13, textColor=INK, wordWrap="CJK",
        ),
        "code": ParagraphStyle(
            "CodeBlock", parent=base["Code"], fontName="Code", fontSize=8,
            leading=11, textColor=colors.HexColor("#E8F0F1"), leftIndent=0,
        ),
        "toc_title": ParagraphStyle(
            "TOCTitle", parent=base["Heading1"], fontName="CNBold", fontSize=20,
            leading=26, textColor=TEAL_DARK, spaceAfter=6 * mm,
        ),
        "toc_entry": ParagraphStyle(
            "TOCEntry", parent=base["Normal"], fontName="CNBody", fontSize=9.5,
            leading=15, textColor=INK, leftIndent=5 * mm, firstLineIndent=-5 * mm,
        ),
    }
    return styles


def make_table(block, styles):
    data = [[Paragraph(esc(cell), styles["table_header"]) for cell in block["headers"]]]
    for row in block["rows"]:
        data.append([Paragraph(esc(cell), styles["table_cell"]) for cell in row])
    widths = [value * mm for value in block["widths"]]
    table = Table(data, colWidths=widths, repeatRows=1, hAlign="LEFT", splitByRow=1)
    table.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, 0), TEAL_DARK),
        ("TEXTCOLOR", (0, 0), (-1, 0), WHITE),
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("GRID", (0, 0), (-1, -1), 0.35, LINE),
        ("ROWBACKGROUNDS", (0, 1), (-1, -1), [WHITE, PAPER]),
        ("LEFTPADDING", (0, 0), (-1, -1), 5),
        ("RIGHTPADDING", (0, 0), (-1, -1), 5),
        ("TOPPADDING", (0, 0), (-1, -1), 4.5),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 4.5),
    ]))
    return [table, Spacer(1, 3 * mm)]


def make_callout(block, styles):
    kind = block["type"]
    if kind == "danger":
        background, stroke, title_color = PALE_RED, RUST, RUST
    elif kind == "warning":
        background, stroke, title_color = colors.HexColor("#FFF6DE"), AMBER, colors.HexColor("#8A5A00")
    else:
        background, stroke, title_color = PALE_TEAL, TEAL, TEAL_DARK

    title_style = ParagraphStyle(
        f"{kind}Title", parent=styles["callout_title"], textColor=title_color,
    )
    contents = [Paragraph(esc(block["title"]), title_style)]
    if "text" in block:
        contents.extend([Spacer(1, 1 * mm), Paragraph(esc(block["text"]), styles["callout_body"])])
    else:
        bullets = ListFlowable(
            [ListItem(Paragraph(esc(item), styles["callout_body"]), leftIndent=4 * mm) for item in block["items"]],
            bulletType="bullet", start="circle", leftIndent=6 * mm, bulletFontName="CNBody",
            bulletFontSize=6, spaceBefore=1 * mm,
        )
        contents.extend([Spacer(1, 1 * mm), bullets])
    inner = Table([[contents]], colWidths=[164 * mm])
    inner.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, -1), background),
        ("BOX", (0, 0), (-1, -1), 0.8, stroke),
        ("LINEBEFORE", (0, 0), (0, -1), 3, stroke),
        ("LEFTPADDING", (0, 0), (-1, -1), 9),
        ("RIGHTPADDING", (0, 0), (-1, -1), 8),
        ("TOPPADDING", (0, 0), (-1, -1), 7),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 7),
    ]))
    return [KeepTogether(inner), Spacer(1, 3 * mm)]


def make_steps(items, styles):
    rows = []
    for index, item in enumerate(items, 1):
        number = Paragraph(str(index), ParagraphStyle(
            f"Step{index}", parent=styles["small"], fontName="CNBold", fontSize=9,
            textColor=WHITE, alignment=TA_CENTER,
        ))
        rows.append([number, Paragraph(esc(item), styles["body"])])
    table = Table(rows, colWidths=[9 * mm, 157 * mm], hAlign="LEFT", splitByRow=1)
    table.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (0, -1), TEAL),
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LINEBELOW", (1, 0), (1, -2), 0.35, LINE),
        ("LEFTPADDING", (0, 0), (0, -1), 2),
        ("RIGHTPADDING", (0, 0), (0, -1), 2),
        ("TOPPADDING", (0, 0), (-1, -1), 5),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 5),
        ("LEFTPADDING", (1, 0), (1, -1), 7),
    ]))
    return [table, Spacer(1, 3 * mm)]


def make_checklist(items, styles):
    rows = []
    for item in items:
        box = Paragraph("□", ParagraphStyle(
            "CheckBox", parent=styles["body"], fontName="CNBold", fontSize=11,
            textColor=TEAL_DARK,
        ))
        rows.append([box, Paragraph(esc(item), styles["body"])])
    table = Table(rows, colWidths=[7 * mm, 159 * mm], hAlign="LEFT", splitByRow=1)
    table.setStyle(TableStyle([
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("TOPPADDING", (0, 0), (-1, -1), 2),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 2),
        ("LEFTPADDING", (0, 0), (-1, -1), 1),
        ("RIGHTPADDING", (0, 0), (-1, -1), 2),
    ]))
    return [table, Spacer(1, 2 * mm)]


def cover_page(canvas, doc):
    canvas.saveState()
    width, height = A4
    canvas.setFillColor(CHARCOAL)
    canvas.rect(0, 0, width, height, fill=1, stroke=0)
    canvas.setFillColor(TEAL)
    canvas.rect(0, height - 14 * mm, width, 14 * mm, fill=1, stroke=0)
    canvas.setFillColor(colors.HexColor("#2C363B"))
    canvas.rect(0, 0, 17 * mm, height, fill=1, stroke=0)
    canvas.setFillColor(colors.HexColor("#7ED0D6"))
    canvas.setFont("CNBold", 8.5)
    canvas.drawString(25 * mm, 14 * mm, f"{VERSION}  |  {DATE}")
    canvas.setFillColor(colors.HexColor("#AFC0C3"))
    canvas.drawRightString(width - 20 * mm, 14 * mm, "MaixCAM Pro / Emm V5")
    canvas.restoreState()


def body_page(canvas, doc):
    canvas.saveState()
    width, height = A4
    canvas.setStrokeColor(LINE)
    canvas.setLineWidth(0.5)
    canvas.line(18 * mm, height - 13 * mm, width - 18 * mm, height - 13 * mm)
    canvas.setFillColor(MUTED)
    canvas.setFont("CNBody", 7.8)
    canvas.drawString(18 * mm, height - 9.5 * mm, TITLE)
    canvas.drawRightString(width - 18 * mm, height - 9.5 * mm, "MaixCAM 专用")
    canvas.line(18 * mm, 13 * mm, width - 18 * mm, 13 * mm)
    canvas.drawString(18 * mm, 8.5 * mm, f"{VERSION}  |  {DATE}")
    canvas.drawRightString(width - 18 * mm, 8.5 * mm, f"第 {canvas.getPageNumber() - 1} 页")
    canvas.restoreState()


def build_pdf() -> None:
    styles = make_styles()
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    doc = ManualDocTemplate(
        str(PDF_PATH), pagesize=A4,
        leftMargin=18 * mm, rightMargin=18 * mm,
        topMargin=18 * mm, bottomMargin=18 * mm,
        title=TITLE, author="Codex", subject=SUBTITLE,
    )
    cover_frame = Frame(25 * mm, 28 * mm, 160 * mm, 225 * mm, id="cover_frame", showBoundary=0)
    body_frame = Frame(18 * mm, 16 * mm, 174 * mm, 263 * mm, id="body_frame", showBoundary=0)
    doc.addPageTemplates([
        PageTemplate(id="cover", frames=[cover_frame], onPage=cover_page),
        PageTemplate(id="body", frames=[body_frame], onPage=body_page),
    ])

    story = [
        Spacer(1, 44 * mm),
        Paragraph(TITLE, styles["cover_title"]),
        Paragraph(SUBTITLE, styles["cover_subtitle"]),
        Spacer(1, 24 * mm),
        Table([
            [Paragraph("当前串口", styles["cover_meta"]), Paragraph("/dev/ttyS0 · 115200 bit/s", styles["cover_meta"])],
            [Paragraph("电机地址", styles["cover_meta"]), Paragraph("0x01", styles["cover_meta"])],
            [Paragraph("控制范围", styles["cover_meta"]), Paragraph("单圈内 · 软件限幅 ±12°", styles["cover_meta"])],
            [Paragraph("核心流程", styles["cover_meta"]), Paragraph("保存一次零点 · 正常启动回零 · 实际位置确认", styles["cover_meta"])],
        ], colWidths=[34 * mm, 107 * mm], style=TableStyle([
            ("BACKGROUND", (0, 0), (-1, -1), colors.HexColor("#F3F7F7")),
            ("GRID", (0, 0), (-1, -1), 0.5, colors.HexColor("#9FB3B7")),
            ("LEFTPADDING", (0, 0), (-1, -1), 8),
            ("RIGHTPADDING", (0, 0), (-1, -1), 8),
            ("TOPPADDING", (0, 0), (-1, -1), 7),
            ("BOTTOMPADDING", (0, 0), (-1, -1), 7),
            ("TEXTCOLOR", (0, 0), (0, -1), TEAL_DARK),
        ])),
        Spacer(1, 14 * mm),
        Paragraph("硬件安全提示：电机独立供电、UART 共地、确认 TTL 电平、断电接线。", ParagraphStyle(
            "CoverWarning", parent=styles["cover_meta"], fontName="CNBold", textColor=colors.HexColor("#FFBCAD"),
        )),
        NextPageTemplate("body"),
        PageBreak(),
        Paragraph("目录", styles["toc_title"]),
    ]

    toc = TableOfContents()
    toc.levelStyles = [styles["toc_entry"]]
    story.extend([toc, PageBreak()])

    for section in SECTIONS:
        if section.get("pagebreak"):
            story.append(PageBreak())
        story.append(Paragraph(esc(section["title"]), styles["h1"]))
        for block in section["blocks"]:
            kind = block["type"]
            if kind == "lead":
                story.append(Paragraph(esc(block["text"]), styles["lead"]))
            elif kind == "paragraph":
                story.append(Paragraph(esc(block["text"]), styles["body"]))
            elif kind in {"warning", "danger", "note"}:
                story.extend(make_callout(block, styles))
            elif kind == "table":
                story.extend(make_table(block, styles))
            elif kind == "steps":
                story.extend(make_steps(block["items"], styles))
            elif kind == "checklist":
                story.extend(make_checklist(block["items"], styles))
            elif kind == "code":
                story.append(Paragraph(esc(block["title"]), styles["h2"]))
                code = Preformatted(block["text"], styles["code"])
                code_box = Table([[code]], colWidths=[164 * mm])
                code_box.setStyle(TableStyle([
                    ("BACKGROUND", (0, 0), (-1, -1), CHARCOAL),
                    ("BOX", (0, 0), (-1, -1), 0.5, CHARCOAL),
                    ("LEFTPADDING", (0, 0), (-1, -1), 8),
                    ("RIGHTPADDING", (0, 0), (-1, -1), 8),
                    ("TOPPADDING", (0, 0), (-1, -1), 7),
                    ("BOTTOMPADDING", (0, 0), (-1, -1), 7),
                ]))
                story.extend([KeepTogether(code_box), Spacer(1, 3 * mm)])
            elif kind == "wiring":
                story.extend([WiringDiagram(), Spacer(1, 2 * mm)])
            elif kind == "flow":
                story.extend([FlowDiagram(block["nodes"]), Spacer(1, 2 * mm)])

    doc.multiBuild(story)


if __name__ == "__main__":
    write_markdown()
    build_pdf()
    print(MD_PATH)
    print(PDF_PATH)
