

/*******************************************************************************
 * @file    mid_oled.c
 * @brief   0.96 寸 OLED 显示中间层实现（Middle Layer）
 *
 * @note    实现原理：
 *          - GRAM 缓存：128×8 页 s_oled_gram[128][8] = 1024 字节
 *          - 渲染 API（ShowString/ShowNum/Clear/ShowFloat）只改 GRAM + 置脏标志
 *          - 刷新：MID_OLED_Task() 轮询式分片通过软件 SPI 把 GRAM 发送到屏幕
 *            每次最多 1 页 128 字节，按 main loop 8 次 = 40ms = 25Hz
 *            5ms tick 频率下，每 Task 阻塞 ~0.3ms 占 6% 占空比，不影响 5ms 调度
 *
 *          性能优化：
 *          - dirty=0 时 Task 0 开销
 *          - 任何渲染后 dirty=1 + page=0 从头刷
 *          - 8 页全刷完 dirty=0、page=0，等待下一帧渲染
 *
 *          代码优化：
 *          - 使用 s_oled_pow() 替代 <math.h> pow()，避免浮点栈
 *          - ShowFloat 使用 snprintf 而非自己拼接
 * @author  LQ_012
 * @date    2026-07-27
 *******************************************************************************/

#include "mid_oled.h"
#include "LQ_font.h"
#include <stdio.h>
#include <string.h>

/* ============================================================
 *  静态状态（文件私有）
 * ============================================================ */

/* OLED 显示缓存：128 列 × 8 页 = 1024 字节
 *  渲染 API 只修改 GRAM + 置 dirty 标志
 *  Task() 轮询式分片把 GRAM 发送到屏幕
 */
static uint8_t s_oled_gram[MID_OLED_W][MID_OLED_PAGES];

/* 脏标志：1=GRAM 与屏幕不一致，需要刷新；0=已经一致
 *  注意：main loop 单线程访问，线程安全
 *  加 volatile 防止编译器优化
 */
static volatile uint8_t s_oled_dirty = 0U;

/* 当前正在刷的页号 0~7，由 Task 推进
 *  8 页全刷完自动归 0 + dirty=0
 */
static volatile uint8_t s_oled_page = 0U;

/* ============================================================
 *  内部函数前置声明
 * ============================================================ */

/* 写一个字节到 OLED（cmd=0 命令，cmd=1 数据）
 *  使用软件 SPI 位翻转 GPIO
 *  约 30 次 GPIO 翻转 @80MHz ≈ 0.4us
 */
static void s_oled_wr_byte(uint8_t dat, uint8_t cmd);

/* 设置 OLED 光标位置（col, page）
 *  发送 3 字节命令（0xB0+page, 0x10+col>>4, 0x00+col&0x0F）
 */
static void s_oled_set_pos(uint8_t col, uint8_t page);

/* 在 GRAM 指定的 (x, y) 画或清一个点（1=白，0=黑） */
static void s_oled_draw_point(uint8_t x, uint8_t y, uint8_t t);

/* 在 GRAM 指定的 (x, y) 显示一个字符（6x8 或 6x12）
 *  y 为像素单位
 */
static void s_oled_show_char(uint8_t x, uint8_t y, uint8_t chr,
                             uint8_t size, uint8_t mode);

/* 计算 m^n，uint32_t 版，避免 <math.h> 浮点开销 */
static uint32_t s_oled_pow(uint8_t m, uint8_t n);

/* ============================================================
 *  内部函数实现
 * ============================================================ */

/* 写一个字节到 OLED
 * @param dat  要写入的字节
 * @param cmd  0=命令（DC=0），1=数据（DC=1）
 */
static void s_oled_wr_byte(uint8_t dat, uint8_t cmd)
{
    uint8_t i;

    /* 1) DC 控制：0=命令，1=数据 */
    LQ_GPIO_WritePin(MID_OLED_DC_PIN, cmd ? 1u : 0u);

    /* 2) CS 拉低：选中 OLED */
    LQ_GPIO_WritePin(MID_OLED_CS_PIN, 0u);

    /* 3) 软件 SPI：MSB first，位翻转 */
    for (i = 0u; i < 8u; i++) {
        LQ_GPIO_WritePin(MID_OLED_SCL_PIN, 0u);             /* 时钟拉低 */
        LQ_GPIO_WritePin(MID_OLED_SDA_PIN, (dat & 0x80u) ? 1u : 0u); /* 输出位 */
        LQ_GPIO_WritePin(MID_OLED_SCL_PIN, 1u);             /* 时钟拉高（上升沿采样） */
        dat <<= 1u;
    }

    /* 4) CS 拉高：本次传输结束 */
    LQ_GPIO_WritePin(MID_OLED_CS_PIN, 1u);
}

/* 设置 OLED 光标位置
 * @param col  列 0~127
 * @param page 页 0~7
 */
static void s_oled_set_pos(uint8_t col, uint8_t page)
{
    s_oled_wr_byte((uint8_t)(0xB0u | (page & 0x07u)), 0u);            /* 页地址 */
    s_oled_wr_byte((uint8_t)(0x10u | ((col >> 4) & 0x0Fu)), 0u);      /* 列高 4 位 */
    s_oled_wr_byte((uint8_t)(col & 0x0Fu), 0u);                       /* 列低 4 位 */
}

/* 在 GRAM 指定的 (x, y) 画或清一个点
 * @param x 列 0~127
 * @param y 行 0~63
 * @param t 1=点亮（白底），0=熄灭（黑底）
 */
static void s_oled_draw_point(uint8_t x, uint8_t y, uint8_t t)
{
    uint8_t page = y >> 3u;          /* 计算 y 所在页号（每页 8 行） */
    uint8_t bit  = (uint8_t)(1u << (y & 0x07u));   /* 计算 y 在页内 bit 位 */
    if (t) {
        s_oled_gram[x][page] |= bit;
    } else {
        s_oled_gram[x][page] &= (uint8_t)~bit;
    }
}

/* 在 GRAM 指定的 (x, y) 显示一个字符
 * @param x    列 0~127
 * @param y    行 0~63（像素单位）
 * @param chr  ASCII 编码（基于 ' ' 偏移）
 * @param size MID_OLED_FONT_8 (6x8) 或 MID_OLED_FONT_12 (6x12)
 * @param mode 1=白字，0=黑字
 */
static void s_oled_show_char(uint8_t x, uint8_t y, uint8_t chr,
                             uint8_t size, uint8_t mode)
{
    uint8_t i, m, temp, chr1;
    uint8_t x0 = x, y0 = y;
    chr1 = (uint8_t)(chr - ' ');          /* 字库偏移 */

    if (size == MID_OLED_FONT_8) {
        /* 6x8 字模：每字符 6 字节，6x8 = 6 列 × 8 行 */
        for (i = 0u; i < 6u; i++) {
            temp = asc2_0806[chr1][i];
            for (m = 0u; m < 8u; m++) {
                if (temp & 0x01u) {
                    s_oled_draw_point(x, y, mode);
                } else {
                    s_oled_draw_point(x, y, (uint8_t)(mode ? 0u : 1u));
                }
                temp >>= 1u;
                y++;
            }
            x++;
            y = y0;
        }
    } else if (size == MID_OLED_FONT_12) {
        /* 6x12 字模：每字符 12 字节，6x12 = 6 列 × 12 行（横跨 2 页） */
        for (i = 0u; i < 6u; i++) {
            temp = asc2_1206[chr1][i];      /* 前 8 行（y=y0 到 y0+7） */
            for (m = 0u; m < 8u; m++) {
                if (temp & 0x01u) {
                    s_oled_draw_point(x, y, mode);
                } else {
                    s_oled_draw_point(x, y, (uint8_t)(mode ? 0u : 1u));
                }
                temp >>= 1u;
                y++;
            }
            x++;
            y = y0;
        }
        /* 后 4 行需要画到第二页（y+8 到 y+11） */
        x = x0;
        y = (uint8_t)(y0 + 8u);
        for (i = 6u; i < 12u; i++) {
            temp = asc2_1206[chr1][i];
            for (m = 0u; m < 4u; m++) {
                if (temp & 0x01u) {
                    s_oled_draw_point(x, y, mode);
                } else {
                    s_oled_draw_point(x, y, (uint8_t)(mode ? 0u : 1u));
                }
                temp >>= 1u;
                y++;
            }
            x++;
            y = (uint8_t)(y0 + 8u);
        }
    }
}

/* 计算 m^n（uint32_t 版） */
static uint32_t s_oled_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1u;
    while (n-- > 0u) {
        result *= m;
    }
    return result;
}

/* ============================================================
 *  公共 API 实现
 * ============================================================ */

/* 初始化 OLED GPIO + 复位 + 发送初始化命令 + 清 GRAM
 * @note  整个调用约 100ms 左右，期间 main loop 未启动
 *        后续 main loop 第一次 MID_OLED_Task() 把 GRAM 推到屏幕
 */
void MID_OLED_Init(void)
{
    /* 1) 5 个 GPIO 初始化为推挽输出 */
    LQConfig_GPIO_InitTypeDef_t gpio_init = {
        .Mode  = GPIO_MODE_OUTPUT_PP,        /* 推挽输出 */
        .Pull  = GPIO_RESISTOR_NO_PULL,      /* 无上下拉 */
        .Speed = GPIO_SPEED_HIGH,            /* 高速，软件 SPI 频率约 1MHz，足够 */
    };
    LQ_GPIO_Init(MID_OLED_SCL_PIN, &gpio_init);
    LQ_GPIO_Init(MID_OLED_SDA_PIN, &gpio_init);
    LQ_GPIO_Init(MID_OLED_CS_PIN , &gpio_init);
    LQ_GPIO_Init(MID_OLED_RES_PIN, &gpio_init);
    LQ_GPIO_Init(MID_OLED_DC_PIN , &gpio_init);

    /* 2) 硬件复位：RES 拉低 50ms 再释放 */
    delay_ms(10);
    LQ_GPIO_WritePin(MID_OLED_RES_PIN, 0u);
    delay_ms(50);
    LQ_GPIO_WritePin(MID_OLED_RES_PIN, 1u);

    /* 3) SSD1306 初始化命令 */
    s_oled_wr_byte(0xAEu, 0u);   /* 关显示 */
    s_oled_wr_byte(0x00u, 0u);   /* 设置低 4 位列地址 */
    s_oled_wr_byte(0x10u, 0u);   /* 设置高 4 位列地址 */
    s_oled_wr_byte(0x40u, 0u);   /* 设置显示起始行 (0x00~0x3F) */
    s_oled_wr_byte(0x81u, 0u);   /* 对比度控制 */
    s_oled_wr_byte(0xCFu, 0u);   /* SEG 输出电流设置 */
    s_oled_wr_byte(0xA1u, 0u);   /* 正常 SEG/Column 映射 */
    s_oled_wr_byte(0xC8u, 0u);   /* 正常 COM/Row 扫描方向 */
    s_oled_wr_byte(0xA6u, 0u);   /* 正常显示 */
    s_oled_wr_byte(0xA8u, 0u);   /* 设置多路率 (1~64) */
    s_oled_wr_byte(0x3Fu, 0u);   /* 1/64 duty */
    s_oled_wr_byte(0xD3u, 0u);   /* 设置显示偏移 */
    s_oled_wr_byte(0x00u, 0u);   /* 无偏移 */
    s_oled_wr_byte(0xD5u, 0u);   /* 设置时钟分频/频率 */
    s_oled_wr_byte(0x80u, 0u);   /* 100 帧/秒 */
    s_oled_wr_byte(0xD9u, 0u);   /* 设置预充电周期 */
    s_oled_wr_byte(0xF1u, 0u);   /* 预充电 15 clocks，放电 1 clock */
    s_oled_wr_byte(0xDAu, 0u);   /* 设置 COM 引脚硬件配置 */
    s_oled_wr_byte(0x12u, 0u);   /* 顺序 COM 引脚配置 */
    s_oled_wr_byte(0xDBu, 0u);   /* 设置 VCOMH */
    s_oled_wr_byte(0x40u, 0u);   /* VCOMH 反选电平 */
    s_oled_wr_byte(0x20u, 0u);   /* 设置内存寻址模式 */
    s_oled_wr_byte(0x02u, 0u);   /* 页寻址模式 (0x02) */
    s_oled_wr_byte(0x8Du, 0u);   /* 设置电荷泵使能 */
    s_oled_wr_byte(0x14u, 0u);   /* 启用电荷泵 */
    s_oled_wr_byte(0xA4u, 0u);   /* 关闭全显 (0xA4) */
    s_oled_wr_byte(0xA6u, 0u);   /* 关闭反显显示 */

    MID_OLED_Clear();

    /* 4) 开显示 */
    s_oled_wr_byte(0xAFu, 0u);

    /* 5) 此时 Clear 已让 dirty=1 + page=0，下次 Task 会把 8 页全刷 */
}

/* 清空 GRAM 内容（dirty=1 + page=0 从头刷）
 * @note  仅清 GRAM 内存，SPI 发送交由 MID_OLED_Task() 完成
 *        调用本函数后 Task 累计带来 0.3ms × 8 = 2.4ms 的外部 GPIO 翻转
 */
void MID_OLED_Clear(void)
{
    uint8_t i, n;
    for (i = 0u; i < MID_OLED_PAGES; i++) {
        for (n = 0u; n < MID_OLED_W; n++) {
            s_oled_gram[n][i] = 0u;
        }
    }
    s_oled_dirty = 1U;
    s_oled_page  = 0U;
}

/* 显示 ASCII 字符串
 * @note  仅改 GRAM + 置脏标志
 */
void MID_OLED_ShowString(uint8_t y, uint8_t x, const char *str, uint8_t size)
{
    uint8_t mode = 1u;             /* 1=白字（黑底白字） */
    uint8_t row_y;                 /* 当前行的起始 y 坐标（根据 size 计算） */

    if (str == NULL) {
        return;
    }

    if (size == MID_OLED_FONT_8) {
        row_y = (uint8_t)(y * 8u);     /* 6x8 字模每行 8 像素 */
    } else if (size == MID_OLED_FONT_12) {
        row_y = (uint8_t)(y * 16u);    /* 6x12 字模每行 16 像素 */
    } else {
        return;                        /* 不支持的字体（GRID 约定） */
    }

    while ((*str >= ' ') && (*str <= '~')) {
        s_oled_show_char(x, row_y, (uint8_t)*str, size, mode);
        x = (uint8_t)(x + 6u);         /* 步进：6x8 与 6x12 每字符都是 6 像素宽 */
        str++;
    }

    s_oled_dirty = 1U;
    s_oled_page  = 0U;                 /* 任何 GRAM 变化都从 page=0 重刷 */
}

/* 显示无符号整数
 * @note  仅改 GRAM + 置脏标志
 */
void MID_OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len,
                      uint8_t size, uint8_t mode)
{
    uint8_t t, temp, m = 0u;
    uint8_t step;                     /* 每个字符的 x 步进：size/2 + 间距 */

    if (len == 0u) {
        return;
    }
    if (size == MID_OLED_FONT_8) {
        m = 2u;                       /* 6x8 字间距 2 像素 */
    }
    step = (uint8_t)((size >> 1) + m);

    for (t = 0u; t < len; t++) {
        /* 取 num 的第 (len-t) 位，再转成 '0' 的字符 */
        temp = (uint8_t)((num / s_oled_pow(10u, (uint8_t)(len - t - 1u))) % 10u);
        s_oled_show_char((uint8_t)(x + step * t), y, (uint8_t)('0' + temp), size, mode);
    }

    s_oled_dirty = 1U;
    s_oled_page  = 0U;
}

/* 显示浮点数（带符号）
 * @note  使用 snprintf 格式化为 "-12.34"
 *        栈上 16 字节缓冲，需要 <stdio.h>，但是已经引入
 */
void MID_OLED_ShowFloat(uint8_t y, uint8_t x, float val, uint8_t prec, uint8_t size)
{
    char buf[16];
    if (prec > 4u) prec = 4u;         /* 最多补 4 位小数，防止溢出 */
    snprintf(buf, sizeof(buf), "%.*f", (int)prec, (double)val);
    MID_OLED_ShowString(y, x, buf, size);
}

/* main loop 分片刷屏任务
 * @note  **必须在 main loop 调用**，禁止进 5ms 调度器 / 中断
 *        执行流程：
 *          1) dirty=0 时直接返回（0 开销）
 *          2) 设置当前页 (s_oled_page) 光标
 *          3) 发送该页 128 字节 GRAM 数据到 OLED
 *          4) page++；==8 时 page=0 + dirty=0
 *        阻塞时间：
 *          1 页 = 3 命令 + 128 字节 ≈ 131 字节 × ~30 GPIO 翻转
 *                ≈ 4000 GPIO 翻转 × ~10 cycles = 40000 cycles
 *                ≈ 80MHz 下 0.5ms
 *          8 页 = 4ms 累计（Task 每次 0.5ms / 5ms tick = 10% 占空比）
 */
void MID_OLED_Task(void)
{
    uint8_t page, n;

    /* dirty=0 时 0 开销 */
    if (s_oled_dirty == 0U) {
        return;
    }

    /* 刷新当前页 */
    page = s_oled_page;
    s_oled_set_pos(0u, page);
    for (n = 0u; n < MID_OLED_W; n++) {
        s_oled_wr_byte(s_oled_gram[n][page], 1u);   /* 写数据 */
    }

    /* 推进到下一页 */
    page++;
    if (page >= MID_OLED_PAGES) {
        page = 0U;
        s_oled_dirty = 0U;        /* 8 页全刷完 dirty=0 */
    }
    s_oled_page = page;
}
