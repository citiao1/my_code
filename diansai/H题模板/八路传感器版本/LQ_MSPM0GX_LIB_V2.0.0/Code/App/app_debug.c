#include "app_debug.h"

/*******************************************************************************
 * @file    app_debug.c
 * @brief   VOFA+ 调试模块（应用层）
 *
 * @note    UART0 (PA10/PA11, 9600 8N1) 与上位机通信
 *          生产者/消费者模型：
 *          - 生产者：ISR / 应用代码通过 SendPlot / Printf 将文本写入 1024 字节环形缓冲区
 *          - 消费者：主循环 ServiceTx 将环形缓冲区数据交给 UART0 DMA 发送
 *
 *          RX 侧：主循环 PollRx 逐字节读取 RX FIFO，组装成行，
 *          然后 s_process_line 解析 FireWater 协议的 key=value 命令。
 *
 *          实时绘图：PollPlot 每 20ms 发送 10 通道 CSV 数据。
 *          具体通道布局见 PollPlot() 函数。
 *******************************************************************************/

#include "app_scheduler.h"
#include "mid_encoder.h"
#include "mid_chassis.h"
#include "mid_line.h"             /* MID_Line_CalcError() */
#include "LQ_dma.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>


/* ============================================================
 *  应用层常量定义
 * ============================================================ */

/* UART0 TX 使用的 DMA 通道，定义为 DMA 通道 0 */
#define UART_TX_DMA_CHANNEL    (DMA_Channel_0)

/* TX 环形缓冲区大小，实际可用空间为 TX_RING_SIZE - 1 */
#define TX_RING_SIZE            (1024u)

/* RX 一行数据的最大长度，防止缓冲区溢出 */
#define RX_LINE_SIZE            (64u)

/* 格式化缓冲区大小，用于 snprintf 拼装 CSV 或 Printf 内容 */
#define FMT_BUF_SIZE             (128u)

/* 绘图刷新周期，50Hz 即 20ms */
#define PLOT_PERIOD_MS            (20u)

/* ============================================================
 *  应用层静态变量
 * ============================================================ */

/** @brief TX 环形缓冲区；实际可用空间为 TX_RING_SIZE - 1，预留 1 字节用于区分空/满状态 */
static uint8_t s_tx_ring[TX_RING_SIZE];
/** @brief 写头指针，DMA 读取位置 */
static uint16_t s_tx_head = 0u;
/** @brief 写尾指针，应用写入位置 */
static uint16_t s_tx_tail = 0u;
/** @brief 当前 DMA 正在传输的长度，0 表示没有 DMA 传输进行中 */
static uint16_t s_tx_dma_len = 0u;

/** @brief RX 行拼接缓冲区 */
static char s_rx_line[RX_LINE_SIZE];
/** @brief s_rx_line 当前写入位置，同时也是已接收但未处理的行长度 */
static uint8_t s_rx_len = 0u;

/** @brief 格式化缓冲区，用于 SendPlot 或 Printf 拼装内容 */
static char s_fmt_buf[FMT_BUF_SIZE];

/** @brief 上次绘图时间戳（毫秒），用于周期轮询判断 */
static uint32_t s_last_plot_ms = 0u;

/* ============================================================
 *  内部函数前向声明
 * ============================================================ */

/* 查询 TX 环形缓冲区剩余可用空间 */
static uint16_t s_tx_free(void);
/* 将一个字符串加入 TX 环形缓冲区，空间不足时丢弃 */
static void s_tx_enqueue(const char *text);
/* 处理一行 FireWater 协议命令 */
static void s_process_line(const char *line);

/* ============================================================
 *  内部函数实现
 * ============================================================ */

/**
 * @brief  查询 TX 环形缓冲区当前剩余可用空间
 * @return 剩余可用字节数，范围 0 到 TX_RING_SIZE - 1
 * @note   通过 head 和 tail 的关系判断：
 *         - head > tail：剩余空间为 SIZE - (head - tail) - 1
 *         - head < tail：剩余空间为 tail - head - 1
 *         初始空状态时，head == tail 为空，head + 1 == tail 为满
 */
static uint16_t s_tx_free(void)
{
    if (s_tx_head >= s_tx_tail) {
        return (uint16_t)(TX_RING_SIZE - (s_tx_head - s_tx_tail) - 1u);
    }
    return (uint16_t)(s_tx_tail - s_tx_head - 1u);
}

/**
 * @brief  将一个字符串加入 TX 环形缓冲区
 * @param  text  源字符串
 * @note   空间不足时不进行任何操作，字符串会被丢弃
 */
static void s_tx_enqueue(const char *text)
{
    size_t len = strlen(text);

    if (len > s_tx_free()) return;  /* 空间不足，丢弃 */
    while (len-- > 0u) {
        s_tx_ring[s_tx_head] = (uint8_t)*text++;
        s_tx_head = (uint16_t)((s_tx_head + 1u) % TX_RING_SIZE);
    }
}

/* ============================================================
 *  应用层：初始化
 * ============================================================ */

/**
 * @brief  初始化调试串口，UART0, 9600bps, 8N1，以及 TX DMA 通道
 * @note   详细说明：
 *         - RX 采用循环轮询 FIFO 方式，不使用中断，因为 LQ 库不支持中断卸载
 *         - TX 使用 DMA，配合 1024 字节环形缓冲区，由 ServiceTx 轮询发送
 *         实现思路：
 *         先调用 LQ_UART_Init 初始化 UART0 硬件，波特率模式选 8N1，
 *         然后设置 TX 和 RX FIFO，RX FIFO 阈值为 1 字节，TX FIFO 3/4 空触发
 *         再初始化 DMA 通道 0，用于 UART0 TX 专用，目标地址为 TX 数据寄存器
 *         最后使能 UART0 的 DMA 发送功能
 */
void APP_DEBUG_Init(void)
{
    LQConfig_UART_InitTypeDef_t uart_init = {
        .Tx         = APP_DEBUG_UART_TX,
        .Rx         = APP_DEBUG_UART_RX,
        .BaudRate   = APP_DEBUG_UART_BAUD,
        .Mode       = DL_UART_MODE_NORMAL,
        .Direction  = DL_UART_DIRECTION_TX_RX,
        .FlowControl= DL_UART_FLOW_CONTROL_NONE,
        .Parity     = DL_UART_PARITY_NONE,
        .WordLength = DL_UART_WORD_LENGTH_8_BITS,
        .StopBits   = DL_UART_STOP_BITS_ONE,
    };
    LQConfig_DMA_InitTypeDef_t dma_init = {
        .trigger       = (uint8_t)DMA_Trigger_UART0_TX,
        .triggerType   = DL_DMA_TRIGGER_TYPE_EXTERNAL,
        .transferMode  = DL_DMA_SINGLE_TRANSFER_MODE,
        .srcWidth      = DL_DMA_WIDTH_BYTE,
        .destWidth     = DL_DMA_WIDTH_BYTE,
        .srcIncrement  = DL_DMA_ADDR_INCREMENT,
        .destIncrement = DL_DMA_ADDR_UNCHANGED,
    };

    /* UART0 硬件初始化 */
    LQ_UART_Init(APP_DEBUG_UART, &uart_init);

    /* 设置 FIFO：RX 阈值为 1 字节，实现轮询响应 */
    DL_UART_disable(UART0);
    DL_UART_enableFIFOs(UART0);
    DL_UART_setRXFIFOThreshold(UART0, DL_UART_RX_FIFO_LEVEL_ONE_ENTRY);
    DL_UART_setTXFIFOThreshold(UART0, DL_UART_TX_FIFO_LEVEL_3_4_EMPTY);
    DL_UART_enable(UART0);

    /* DMA 通道 0 初始化为 UART0 TX 专用，目标地址为 TX 数据寄存器 */
    LQ_DMA_Init(UART_TX_DMA_CHANNEL, &dma_init);
    LQ_DMA_SetDstAddr(UART_TX_DMA_CHANNEL, LQ_UART_GetTXRegister(APP_DEBUG_UART));
    LQ_UART_EnableDMATransmit(APP_DEBUG_UART);
}

/* ============================================================
 *  应用层：TX DMA 状态查询与帧发送
 * ============================================================ */

/**
 * @brief  查询 UART0 TX DMA 状态，并将环形缓冲区数据发送一帧
 * @note   应用需要循环频繁调用此函数，确保 TX 数据能及时发出
 *         实现思路：
 *         首先检查上一帧 DMA 传输是否完成，如果完成则更新 tail 指针
 *         然后检查是否有新数据需要发送（tail != head）
 *         计算 tail 到 head 之间的连续数据长度（注意环回边界）
 *         设置 DMA 源地址和传输长度，启动 DMA 传输
 */
void APP_DEBUG_ServiceTx(void)
{
    uint16_t length;

    /* 上一帧 DMA 传输未完成则等待 */
    if (s_tx_dma_len > 0u) {
        if (DL_DMA_getTransferSize(DMA, UART_TX_DMA_CHANNEL) > 0u) return;
        DL_DMA_disableChannel(DMA, UART_TX_DMA_CHANNEL);
        s_tx_tail = (uint16_t)((s_tx_tail + s_tx_dma_len) % TX_RING_SIZE);
        s_tx_dma_len = 0u;
    }

    /* 缓冲区为空，没有数据可发 */
    if (s_tx_tail == s_tx_head) return;

    /* 计算 tail 到 head 之间的连续数据长度（环回边界处理） */
    length = (s_tx_head > s_tx_tail) ?
             (uint16_t)(s_tx_head - s_tx_tail) :
             (uint16_t)(TX_RING_SIZE - s_tx_tail);

    LQ_DMA_SetSrcAddr(UART_TX_DMA_CHANNEL, (uint32_t)&s_tx_ring[s_tx_tail]);
    LQ_DMA_SetTransferSize(UART_TX_DMA_CHANNEL, length);
    s_tx_dma_len = length;
    DL_DMA_enableChannel(DMA, UART_TX_DMA_CHANNEL);
    DL_DMA_startTransfer(DMA, UART_TX_DMA_CHANNEL);
}

/* ============================================================
 *  应用层：RX 轮询与信息拼接
 * ============================================================ */

/**
 * @brief  轮询读取 RX FIFO，逐字节拼接，遇到换行符时调用 s_process_line
 * @note   应用需要循环频繁调用此函数，波特率 9600 时每字节约 1ms
 *         - 忽略回车符 \r
 *         - 遇到换行符 \n 则处理完整行
 *         - 普通字符直接追加到行缓冲区
 *         - 行缓冲区溢出时丢弃整行
 */
void APP_DEBUG_PollRx(void)
{
    while (!DL_UART_isRXFIFOEmpty(UART0)) {
        char byte = (char)DL_UART_receiveData(UART0);

        if (byte == '\r') continue;

        if (byte == '\n') {
            s_rx_line[s_rx_len] = '\0';
            s_rx_len = 0u;
            s_process_line(s_rx_line);
        } else if (s_rx_len < RX_LINE_SIZE - 1u) {
            s_rx_line[s_rx_len++] = byte;
        } else {
            /* 行太长，丢弃整行 */
            s_rx_len = 0u;
        }
    }
}

/* ============================================================
 *  应用层：数据发送接口
 * ============================================================ */

/**
 * @brief  发送 CSV 格式数据，用于 VOFA 上位机实时绘图
 * @param  data  数据数组指针
 * @param  n     通道数量
 * @note   格式如 1.2,3.4,5.6 加上换行符，VOFA 自动识别绘图模式
 *         使用 s_fmt_buf 格式化，然后加入发送队列
 */
void APP_DEBUG_SendPlot(float *data, uint8_t n)
{
    uint16_t offset = 0u;
    uint8_t i;

    for (i = 0u; i < n; i++) {
        if (i > 0u) {
            s_fmt_buf[offset++] = ',';
        }
        /* 浮点数转字符串，Keil 需勾选 Use MicroLIB */
        offset += (uint16_t)snprintf(&s_fmt_buf[offset], FMT_BUF_SIZE - offset,
                                      "%.2f", data[i]);
        if (offset >= FMT_BUF_SIZE - 4u) break;
    }
    s_fmt_buf[offset++] = '\n';
    s_fmt_buf[offset] = '\0';
    s_tx_enqueue(s_fmt_buf);
}

/**
 * @brief  格式化字符串发送，功能类似于 printf
 * @param  fmt  printf 格式字符串
 * @note   用于打印日志信息
 */
void APP_DEBUG_Printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(s_fmt_buf, FMT_BUF_SIZE, fmt, args);
    va_end(args);
    if (len > 0) {
        s_tx_enqueue(s_fmt_buf);
    }
}

/* ============================================================
 *  应用层：实时数据绘图
 * ============================================================ */

/**
 * @brief  轮询发送 VOFA 实时绘图数据，每 20ms 强制刷新一次
 * @note   应用需要循环频繁调用此函数，检查时间戳是否到达周期时间，到达则发送一帧
 *         10 通道数据布局，对应 VOFA 的 11 个绘图通道：
 *           0: 目标角速度 (deg/s) - 位置环输出
 *           1: 实际角速度 (deg/s) - 陀螺仪测量值减去零漂
 *           2: 归一化位置偏差 [-1, +1]
 *           3: 偏差积分值 (每5ms累加一次)
 *           4: 左轮目标速度
 *           5: 左轮实际速度 (编码器换算)
 *           6: 左轮 PWM
 *           7: 右轮目标速度
 *           8: 右轮实际速度 (编码器换算)
 *           9: 右轮 PWM
 */
void APP_DEBUG_PollPlot(void)
{
    uint32_t now_ms = APP_Scheduler_GetTickMs();
    if (now_ms - s_last_plot_ms < PLOT_PERIOD_MS) {
        return;
    }
    s_last_plot_ms = now_ms;

    float plot[10];

    plot[0] = g_chassis.target_omega;    /* 0: 目标角速度 (deg/s) - 位置环输出，对应 VOFA 的 omega=X */
    plot[1] = g_chassis.gyro_z_actual;   /* 1: 实际角速度 (deg/s) = 原始值 - g_param.gyro_offset */
    plot[2] = MID_Line_CalcError();      /* 2: 归一化位置偏差 [-1, +1] */
    plot[3] = g_chassis.target_diff;     /* 3: 偏差积分值 (每5ms累加一次) */
    plot[4] = g_chassis.target_speed_l;  /* 4: 左轮目标速度 */
    plot[5] = MID_Encoder_GetSpeed(MID_ENCODER_LEFT);   /* 5: 左轮实际速度 (编码器换算) */
    plot[6] = g_chassis.pwm_l;           /* 6: 左轮 PWM */
    plot[7] = g_chassis.target_speed_r;  /* 7: 右轮目标速度 */
    plot[8] = MID_Encoder_GetSpeed(MID_ENCODER_RIGHT);  /* 8: 右轮实际速度 (编码器换算) */
    plot[9] = g_chassis.pwm_r;           /* 9: 右轮 PWM */

    APP_DEBUG_SendPlot(plot, 10);
}

/* ============================================================
 *  FireWater 协议命令解析
 * ============================================================ */

/**
 * @brief  从字符串 buf 中解析 key=value 形式的浮点数值
 * @param  buf  源字符串
 * @param  key  要查找的键名，如 "kp_pos"
 * @param  def  默认值，未找到时返回此值
 * @return 解析到的浮点数值，未找到则返回 def
 */
static float s_parse_float(const char *buf, const char *key, float def)
{
    const char *p = strstr(buf, key);
    if (p == NULL) return def;
    p += strlen(key);
    if (*p != '=') return def;
    p++;
    return (float)atof(p);
}

/**
 * @brief  处理一行 FireWater 协议命令（key=value 格式）
 * @param  line  完整的命令行字符串
 * @note   解析参数并更新全局参数 g_param
 *         支持的 key：
 *         - 位置环参数：kp_pos, kd_pos, om_pos（位置环输出限幅）
 *         - 角度环参数：kp_yaw, ki_yaw, om_yaw（角度环输出限幅）, imax_yaw（积分限幅）
 *         - 速度环参数：kp_sl, ki_sl（左轮）, kp_sr, ki_sr（右轮）
 *         - 直行参数：vx（目标速度）, vmax（速度限幅）, deadzone（偏差死区）,
 *           gyro_off（陀螺仪零漂补偿）
 *         - 使能开关：en_pos, en_yaw, en_speed（0 或 1）
 */
static void s_process_line(const char *line)
{
    /* 回显收到的原始字符串，方便调试 */
    APP_DEBUG_Printf("RX: %s\n", line);

    /* 依次解析并更新 g_param */
    /* 位置环 PD */
    g_param.pid_pos.kp = s_parse_float(line, "kp_pos", g_param.pid_pos.kp);
    g_param.pid_pos.kd = s_parse_float(line, "kd_pos", g_param.pid_pos.kd);
    /* 角度环 PI */
    g_chassis.target_omega = s_parse_float(line, "omega",  g_chassis.target_omega);
    g_param.pid_yaw.kp = s_parse_float(line, "kp_yaw", g_param.pid_yaw.kp);
    g_param.pid_yaw.ki = s_parse_float(line, "ki_yaw", g_param.pid_yaw.ki);
    g_param.pid_yaw.kd = s_parse_float(line, "kd_yaw", g_param.pid_yaw.kd);
    /* 左轮速度环 PI */
    g_param.pid_speed_l.kp = s_parse_float(line, "kp_sl", g_param.pid_speed_l.kp);
    g_param.pid_speed_l.ki = s_parse_float(line, "ki_sl", g_param.pid_speed_l.ki);
    /* 右轮速度环 PI */
    g_param.pid_speed_r.kp = s_parse_float(line, "kp_sr", g_param.pid_speed_r.kp);
    g_param.pid_speed_r.ki = s_parse_float(line, "ki_sr", g_param.pid_speed_r.ki);
    /* 直行参数 */
    g_param.chassis_vx      = s_parse_float(line, "vx",       g_param.chassis_vx);
    
    /* 使能开关：0 或 1，解析失败时保持当前值 */
    g_param.enable_pos   = (uint8_t)s_parse_float(line, "en_pos",   g_param.enable_pos);
    g_param.enable_yaw   = (uint8_t)s_parse_float(line, "en_yaw",   g_param.enable_yaw);
    g_param.enable_speed = (uint8_t)s_parse_float(line, "en_speed", g_param.enable_speed);
}