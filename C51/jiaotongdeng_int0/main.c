/* 交通灯低电平点亮，K1 接 P3.2，按下接地。 */
#include <reg51.h>
#include <intrins.h>

typedef unsigned char u8;
typedef unsigned int u16;

sbit K1 = P3^2;

#define ALL_RED 0xdb
#define ALL_OFF 0xff

/* P1.0～P1.5：南北绿、黄、红，东西绿、黄、红。 */
static const u8 code phase_output[4] = {0xde, 0xdd, 0xf3, 0xeb};
static volatile bit key_pending;
static u8 phase;
static u8 release_ms;
static u16 phase_ms;

/* 12 MHz、12T 下约 1 ms 的软件延时。 */
static void delay_ms(u16 milliseconds)
{
    u8 count;

    while (milliseconds != 0)
    {
        count = 165;
        do
        {
            _nop_();
            _nop_();
        } while (--count != 0);
        --milliseconds;
    }
}

void int0_isr(void) interrupt 0
{
    EX0 = 0;
    key_pending = 1;  /* 中断只记录请求，消抖和闪灯由主程序处理。 */
}

static void emergency_blink(void)
{
    u8 i;

    for (i = 0; i < 4; ++i)
    {
        P1 = ALL_RED;
        delay_ms(250);
        P1 = ALL_OFF;
        delay_ms(250);
    }
    P1 = phase_output[phase];  /* 闪烁约 2 秒后，继续原来的交通灯状态。 */
}

static void key_process(void)
{
    if (key_pending)
    {
        key_pending = 0;
        delay_ms(20);
        if (K1 == 0)
            emergency_blink();
    }

    if (EX0)
        return;

    if (K1 == 0)
    {
        release_ms = 0;
        return;
    }

    if (++release_ms >= 20)
    {
        release_ms = 0;
        IE0 = 0;
        EX0 = 1;  /* 松开稳定后再允许下一次按键中断。 */
    }
}

static void traffic_init(void)
{
    EA = 0;
    phase = 0;
    phase_ms = 0;
    release_ms = 0;
    key_pending = 0;
    P1 = phase_output[phase];
    K1 = 1;
    IT0 = 1;  /* INT0 下降沿触发。 */
    IE0 = 0;
    EX0 = 1;
    EA = 1;
}

static void traffic_process(void)
{
    delay_ms(1);
    key_process();
    if (++phase_ms >= 1000)
    {
        phase_ms = 0;
        phase = (phase + 1) & 0x03;
        P1 = phase_output[phase];
    }
}

void main(void)
{
    traffic_init();
    while (1)
        traffic_process();
}
