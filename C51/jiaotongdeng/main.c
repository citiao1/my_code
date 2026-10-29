/* 交通灯：P1.0～P1.5 依次控制南北、东西的绿黄红灯，低电平点亮。 */
#include <reg51.h>
#include <intrins.h>

typedef unsigned char u8;
typedef unsigned int u16;

#define NS_GREEN  0x01
#define NS_YELLOW 0x02
#define NS_RED    0x04
#define EW_GREEN  0x08
#define EW_YELLOW 0x10
#define EW_RED    0x20
#define PHASE_COUNT 4

/* 按表循环切换四种灯光状态。 */
static const u8 code phase_output[PHASE_COUNT] =
{
    (u8)~(NS_GREEN  | EW_RED),
    (u8)~(NS_YELLOW | EW_RED),
    (u8)~(NS_RED    | EW_GREEN),
    (u8)~(NS_RED    | EW_YELLOW)
};

static const u16 code phase_time_ms[PHASE_COUNT] =
{
    1500, 500, 1500, 500  /* 绿灯约 1.5 秒，黄灯约 0.5 秒。 */
};

/* 12 MHz、12T 下的软件延时。 */
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

static void traffic_init(void)
{
    P1 = (u8)~(NS_RED | EW_RED);  /* 初始时两个方向均亮红灯。 */
}

static void traffic_cycle(void)
{
    u8 phase;

    for (phase = 0; phase < PHASE_COUNT; ++phase)
    {
        P1 = phase_output[phase];
        delay_ms(phase_time_ms[phase]);
    }
}

void main(void)
{
    traffic_init();
    while (1)
    {
        traffic_cycle();
    }
}
