/* AT89S51, 12 MHz / 12T. Common-cathode display on P1. */
#include <reg51.h>

typedef unsigned char u8;

#define RELEASE_SAMPLES 21         /* At least 20 ms of sampled high level. */

sbit K01 = P3^5;                   /* Hardware T1 counter input. */

/* P1.0..P1.7 -> a, b, c, d, e, f, g, dp. High level lights a segment. */
static const u8 code segment_code[10] =
{
    0x3f, 0x06, 0x5b, 0x4f, 0x66,
    0x6d, 0x7d, 0x07, 0x7f, 0x6f
};

static volatile bit count_pending;
static volatile u8 release_samples;
static u8 number;

void timer1_isr(void) interrupt 3
{
    TR1 = 0;                       /* Count the first edge, then reject bounce. */
    release_samples = 0;
    count_pending = 1;
}

void timer0_isr(void) interrupt 1
{
    TH0 = 0xfc;                    /* 65536 - 1000 = 0xfc18: 1 ms. */
    TL0 = 0x18;
    if (TR1)
        return;

    if (K01 == 0)
        release_samples = 0;
    else if (release_samples < RELEASE_SAMPLES)
        ++release_samples;
}

static void counter_init(void)
{
    EA = 0;
    IE = 0;
    IP = 0;
    TR0 = 0;
    TR1 = 0;
    number = 0;
    count_pending = 0;
    release_samples = 0;
    P1 = segment_code[0];
    K01 = 1;                       /* Release P3.5 for input. */
    TMOD = 0x61;                   /* T1: counter mode 2; T0: timer mode 1. */
    TH1 = 0xff;                    /* Auto reload: one falling edge overflows. */
    TL1 = 0xff;
    TH0 = 0xfc;
    TL0 = 0x18;
    TF0 = 0;
    TF1 = 0;
    ET0 = 1;
    ET1 = 1;                       /* INT1 is unused. */
    TR0 = 1;
    TR1 = 1;
    EA = 1;
}

static void counter_process(void)
{
    EA = 0;
    if (count_pending)
    {
        count_pending = 0;
        if (++number == 10)
            number = 0;
        P1 = segment_code[number];
    }

    if (!TR1 && release_samples >= RELEASE_SAMPLES && K01 != 0)
    {
        TL1 = 0xff;                /* Discard any edge during ISR latency. */
        TF1 = 0;
        TR1 = 1;
    }
    EA = 1;
}

void main(void)
{
    counter_init();
    while (1)
        counter_process();
}
