# -*- coding: utf-8 -*-
# main.c: 5 路红外屏显 + RUN/STOP 状态 + K1 按键 + OLED（HEAD 基础上修改）

with open(r'D:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\User\main.c', 'rb') as f:
    raw = f.read()
print(f"Original size: {len(raw)}")

# 1) 替换 includes（ASCII）
raw = raw.replace(
    b'#include "LQ_tracking.h"',
    b'#include "mid_line.h"\n#include "mid_oled.h"\n#include "mid_key.h"\n#include "mid_imu.h"'
)
print("OK: includes replaced")

# 2) 替换 LQ_Tracking_Polling_Init 调用
raw = raw.replace(
    b'    LQ_Tracking_Polling_Init();',
    b'    MID_IR_Init();'
)
print("OK: LQ_Tracking_Polling_Init -> MID_IR_Init")

# 4) 在 MID_Beep_Init 前加 OLED + Key 初始化（用 GBK 字节）
comment_oled_init = '/* 屏幕初始化（必须在 s_oled_ir_refresh 前） */'.encode('gbk')
comment_key_init = '/* K1 按键初始化，用于 K1 启动时清 emergency 标志 */'.encode('gbk')
new_init = b'    ' + comment_oled_init + b'\n    MID_OLED_Init();\n    ' + comment_key_init + b'\n    MID_Key_Init();\n\n    MID_Beep_Init();\n    MID_Beep_Beep(2, 0.7f, 0.5f);'
old_init = b'    MID_Beep_Init();\n    MID_Beep_Beep(2, 0.7f, 0.5f);'
if old_init in raw:
    raw = raw.replace(old_init, new_init)
    print("OK: MID_OLED_Init + MID_Key_Init added")
else:
    print("WARNING: MID_Beep_Init not found")

# 5) s_oled_ir_refresh 函数（GBK 编码）
s_oled_ir_func_str = """/* ============================================================
 *  OLED 5 路红外 + 状态刷新
 *  - 50ms 一次（~20Hz 屏幕刷新）
 *  - 调用 MID_IR_ReadRaw 读 5 路原始 H/L 电平
 *  - 第 0 行：状态 STATUS: RUN 或 STATUS: STOP
 *  - 第 1~5 行：IR1~IR5 原始 H/L 电平
 *  - 状态变化时（RUN->STOP）连续响 3 声报警
 * ============================================================ */
static uint32_t s_oled_ir_last_ms = 0U;     /* 上次刷新的 SysTick tick（ms） */
#define OLED_IR_REFRESH_MS    (50U)         /* 刷新周期 */
static uint8_t s_oled_last_emergency = 0U;  /* 上次状态，边沿蜂鸣用 */

static void s_oled_ir_refresh(void)
{
    uint32_t now_ms;
    char buf[12];
    uint8_t i;
    uint8_t levels[MID_IR_SENSOR_COUNT];
    const char *status_str;

    now_ms = APP_Scheduler_GetTickMs();
    if ((now_ms - s_oled_ir_last_ms) < OLED_IR_REFRESH_MS) {
        return;     /* 未到刷新周期，直接返回 */
    }
    s_oled_ir_last_ms = now_ms;

    /* 读 5 路原始 H/L 电平 */
    MID_IR_ReadRaw(levels);

    /* 第 0 行：状态 */
    if (g_line_emergency_stopped) {
        status_str = "STATUS:STOP";   /* 5 路全 0/全 1 触发紧急停车 */
    } else {
        status_str = "STATUS:RUN ";   /* 正常运行 */
    }
    MID_OLED_ShowString(0, 0, status_str, MID_OLED_FONT_8);

    /* 第 1~5 行：IR1~IR5 原始 H/L 电平 */
    for (i = 0U; i < MID_IR_SENSOR_COUNT; i++) {
        snprintf(buf, sizeof(buf), "IR%u=%s",
                 (unsigned)(i + 1U),
                 (levels[i] != 0U) ? "H" : "L");
        MID_OLED_ShowString((uint8_t)(i + 1U), 0, buf, MID_OLED_FONT_8);
    }

    /* 状态变化边沿报警：RUN -> STOP 瞬间响 3 声 */
    if (g_line_emergency_stopped && !s_oled_last_emergency) {
        /* 紧急停车报警：3 声哔哔哔（阻塞约 400ms） */
        MID_Beep_Beep(3, 0.08f, 0.08f);
    }
    s_oled_last_emergency = g_line_emergency_stopped;
}

"""
s_oled_ir_func = s_oled_ir_func_str.encode('gbk')

# 在 int main(void) 之前插入
old_main = b'int main(void)'
new_main = s_oled_ir_func + b'int main(void)'
if old_main in raw:
    raw = raw.replace(old_main, new_main)
    print("OK: s_oled_ir_refresh function added")
else:
    print("WARNING: int main(void) not found")

# 6) main loop 加 K1 + s_oled_ir_refresh + MID_OLED_Task
#    现状：__WFI() 是最后一行
#    在 __WFI() 前插入
key_comment = '/* K1 按键轮询（20ms 一次） */'.encode('gbk')
ir_comment = '/* 5 路红外 + 状态刷新（50ms 一次） */'.encode('gbk')
oled_task_comment = '/* OLED 分片刷新（dirty=0 时 0 开销） */'.encode('gbk')

old_wfi = b'        __WFI();'
new_wfi = (
    b'        ' + key_comment + b'\n'
    b'        MID_Key_Scan();\n\n'
    b'        ' + ir_comment + b'\n'
    b'        s_oled_ir_refresh();\n\n'
    b'        ' + oled_task_comment + b'\n'
    b'        MID_OLED_Task();\n\n'
    b'        __WFI();'
)
raw = raw.replace(old_wfi, new_wfi)
print("OK: main loop updated")

# 7) 加 stdio.h 用于 snprintf
if b'#include <stdio.h>' not in raw:
    raw = raw.replace(
        b'#include "include.h"\n',
        b'#include "include.h"\n#include <stdio.h>  /* snprintf for IR H/L display */\n'
    )
    print("OK: stdio.h added")

with open(r'D:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\User\main.c', 'wb') as f:
    f.write(raw)

print(f"Final size: {len(raw)}")
print("OK: main.c modified")
