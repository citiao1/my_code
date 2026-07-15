#include "include.h"
#include "diansai_app.h"

int main(void)
{
    LQ_System_Init();
    delay_ms(20);
    DiansaiApp_Init();

    while (1)
    {
        DiansaiApp_Run();
        __WFI();
    }
}
