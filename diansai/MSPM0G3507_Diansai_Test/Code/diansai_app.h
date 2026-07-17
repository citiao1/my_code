#ifndef DIANSAI_APP_H
#define DIANSAI_APP_H

/*
 * 初始化完整车辆应用。调用前必须先执行 LQ_System_Init()。
 * 本函数会完成外设模块、串级控制器和 1 ms SysTick 的初始化；IMU 零偏
 * 标定约需 3 秒，期间车辆必须保持静止。
 */
void DiansaiApp_Init(void);

/*
 * 主循环轮询入口，应当尽可能频繁调用。函数内部自行完成 10 ms 周期调度，
 * 不要求调用者延时；两次调用之间可使用 __WFI() 等待中断以降低空闲功耗。
 */
void DiansaiApp_Run(void);

#endif
