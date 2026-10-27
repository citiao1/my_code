/* 共阴极数码管接 P0，K1 接 P3.3，按下接地。 */
#include <reg51.h>
#include <intrins.h>

typedef unsigned char u8;
typedef unsigned int u16;

sbit K1 = P3^3;

/* P0.0～P0.7 依次接 a、b、c、d、e、f、g、dp。 */
static const u8 code segment_code[10] =
{
    0x3f, 0x06, 0x5b, 0x4f, 0x66,
    0x6d, 0x7d, 0x07, 0x7f, 0x6f
};

static volatile bit key_pending;
static bit counter_done;
static u8 number;
static u8 release_ms;

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

void int1_isr(void) interrupt 2
{
    EX1 = 0;
    key_pending = 1;  /* 暂停 INT1，避免按键抖动产生重复计数。 */
}

static void counter_init(void)
{
    EA = 0;
    number = 0;
    release_ms = 0;
    key_pending = 0;
    counter_done = 0;
    P0 = segment_code[0];
    K1 = 1;
    IT1 = 1;  /* INT1 下降沿触发。 */
    IE1 = 0;
    EX1 = 1;
    EA = 1;
}

static void counter_process(void)
{
    delay_ms(1);
    if (key_pending)
    {
        key_pending = 0;
        delay_ms(20);
        if (K1 == 0)
        {
            if (++number == 10)
            {
                number = 0;
                counter_done = 1;  /* 第 10 次显示 0，保持 INT1 关闭。 */
            }
            P0 = segment_code[number];
        }
    }

    if (counter_done || EX1)
        return;

    if (K1 == 0)
    {
        release_ms = 0;
        return;
    }

    if (++release_ms >= 20)
    {
        release_ms = 0;
        IE1 = 0;
        EX1 = 1;  /* 按键松开稳定后，才接受下一次按下。 */
    }
}

void main(void)
{
    counter_init();
    while (1)
        counter_process();
}
