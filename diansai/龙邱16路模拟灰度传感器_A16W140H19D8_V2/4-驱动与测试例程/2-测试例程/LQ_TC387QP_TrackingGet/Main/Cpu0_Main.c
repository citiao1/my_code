#include "lq_include.h"

IfxCpu_syncEvent g_cpuSyncEvent = 0;

int core0_main(void)
{
    //================================ 系统代码 ================================//
    cpu_init();                        // 核心CPU初始化
    IfxCpu_emitEvent(&g_cpuSyncEvent); // 等待CPU同步
    IfxCpu_waitEvent(&g_cpuSyncEvent, 1);

    UART_InitConfig(UART0_RX_P14_1, UART0_TX_P14_0, 115200);  // 下载口的串口

//    ================================ 用户代码 ================================//
    Test_Tracking();     // PASS,测试(函数内含死循环)龙邱16路灰度模拟_光电循迹传感器所有通道读取,下载串口打印，并在LCD屏上显示
    while (1) // 主循环
    {
        Delay_Ms(50);
        // 核心0的任务代码
    }
}
