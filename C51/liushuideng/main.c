/* 流水灯：P1.0～P1.7 控制八个 LED，低电平点亮。 */
#include <reg51.h>

typedef unsigned char u8;
typedef unsigned int u16;

#define LED_OFF 0xff

/* 软件延时，实际时长与晶振及编译设置有关。 */
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

    P1 = LED_OFF;  /* 先熄灭所有 LED。 */

    while (1)
    {
        pattern = 0xfe;  /* 从 P1.0 开始点亮。 */
        for (i = 0; i < 8; i++)
        {
            P1 = pattern;
            delay_ms(500);
            pattern = (u8)((pattern << 1) | 0x01);  /* 亮灯位置向高位移动。 */
        }

        
    }
}
