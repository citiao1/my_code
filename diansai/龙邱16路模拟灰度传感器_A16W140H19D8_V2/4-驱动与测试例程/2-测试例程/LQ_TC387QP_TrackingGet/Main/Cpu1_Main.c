#include "lq_include.h"

extern IfxCpu_syncEvent g_cpuSyncEvent;

int core1_main(void)
{

    SafetyWatchdog();         // 关闭看门狗
    enableInterruptLatency(); // 开启全局中断

    IfxCpu_emitEvent(&g_cpuSyncEvent); // 等待cpu同步
    IfxCpu_waitEvent(&g_cpuSyncEvent, 1);

    /* 用户代码 */
    while (1) // 主循环
    {
        // 核心1的任务代码
    }
}
