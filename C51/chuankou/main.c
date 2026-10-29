/* AT89C51, 11.0592 MHz, 12T; UART mode 1, 9600 bps, 8N1. */
#include <reg51.h>

typedef unsigned char u8;
typedef unsigned long u32;

#define RX_BUFFER_SIZE 32
#define RX_BUFFER_MASK (RX_BUFFER_SIZE - 1)

static volatile u8 rx_buffer[RX_BUFFER_SIZE];
static volatile u8 rx_head;
static volatile u8 rx_tail;
static volatile bit rx_overflow;
static volatile bit tx_done;
static volatile u32 elapsed_seconds;
static u8 timer_ticks;
static u32 last_reported;

/* 接收和发送完成分别处理，支持发送计数时继续接收数据。 */
void serial_isr(void) interrupt 4
{
    u8 value;
    u8 next;

    if (RI)
    {
        value = SBUF;
        RI = 0;
        next = (rx_head + 1) & RX_BUFFER_MASK;
        if (next != rx_tail)
        {
            rx_buffer[rx_head] = value;
            rx_head = next;
        }
        else
            rx_overflow = 1;  /* 缓冲区满时丢弃新字节，主循环报告。 */
    }

    if (TI)
    {
        TI = 0;
        tx_done = 1;
    }
}

void timer0_isr(void) interrupt 1
{
    /* 11059200 / 12 * 0.05 = 46080; 65536 - 46080 = 0x4C00. */
    TR0 = 0;
    TH0 = 0x4c;
    TL0 = 0x00;
    TR0 = 1;

    if (++timer_ticks == 20)
    {
        timer_ticks = 0;
        ++elapsed_seconds;
    }
}

static void uart_send_byte(u8 value)
{
    tx_done = 0;
    SBUF = value;
    while (!tx_done)
        ;  /* 等待期间中断保持开启，接收和定时继续工作。 */
}

static void uart_send_text(const char code *text)
{
    while (*text != '\0')
        uart_send_byte(*text++);
}

static void uart_send_number(u32 value)
{
    u8 digits[10];
    u8 length;

    length = 0;
    do
    {
        digits[length++] = '0' + (u8)(value % 10);
        value /= 10;
    } while (value != 0);

    while (length != 0)
        uart_send_byte(digits[--length]);
}

static void app_init(void)
{
    EA = 0;
    TR0 = 0;
    TR1 = 0;
    rx_head = 0;
    rx_tail = 0;
    rx_overflow = 0;
    tx_done = 0;
    timer_ticks = 0;
    elapsed_seconds = 0;
    last_reported = 0;
    P3 |= 0x03;  /* 释放 P3.0/RXD、P3.1/TXD。 */

    TMOD = 0x21;  /* T1 方式 2 产生波特率；T0 方式 1 定时。 */
    PCON &= 0x7f; /* SMOD = 0。 */
    SCON = 0x50;  /* 串口方式 1，REN = 1，允许接收。 */
    TH1 = 0xfd;   /* 11059200 / (12 * 32 * (256 - 253)) = 9600. */
    TL1 = 0xfd;
    TH0 = 0x4c;
    TL0 = 0x00;
    TF0 = 0;
    TF1 = 0;
    ET1 = 0;      /* T1 只提供波特率，不开启 T1 中断。 */
    ET0 = 1;
    ES = 1;
    TR1 = 1;
    TR0 = 1;
    EA = 1;
}

static void app_process(void)
{
    u8 value;
    u32 seconds;

    if (rx_head != rx_tail)
    {
        value = rx_buffer[rx_tail];
        rx_tail = (rx_tail + 1) & RX_BUFFER_MASK;
        uart_send_byte(value);  /* 原样回显，不改变大小写或换行。 */
    }

    if (rx_overflow)
    {
        rx_overflow = 0;
        uart_send_text("\r\n[RX overflow]\r\n");
    }

    EA = 0;  /* 8051 读取 32 位计数需要防止被定时中断打断。 */
    seconds = elapsed_seconds;
    EA = 1;
    if (seconds == last_reported)
        return;

    last_reported = seconds;
    uart_send_text("\r\nCount = ");
    uart_send_number(seconds);
    uart_send_text("\r\n");
}

void main(void)
{
    app_init();
    while (1)
        app_process();
}
