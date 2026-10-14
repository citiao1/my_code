#include <reg51.h>

typedef unsigned char u8;
typedef unsigned int u16;

#define LED_OFF 0xff

static void delay_ms(u16 milliseconds)
{
    u16 i;

    while (milliseconds-- != 0)
    {
        for (i = 0; i < 120; i++)
        {
            ;
        }
    }
}

void main(void)
{
    u8 pattern;
    u8 i;

    P1 = LED_OFF;

    while (1)
    {
        pattern = 0xfe;
        for (i = 0; i < 8; i++)
        {
            P1 = pattern;
            delay_ms(500);
            pattern = (u8)((pattern << 1) | 0x01);
        }

        /*pattern = 0x7f;
        for (i = 0; i < 8; i++)
        {
            P1 = pattern;
            delay_ms(500);
            pattern = (u8)((pattern >> 1) | 0x80);
        }
		*/
    }
}
