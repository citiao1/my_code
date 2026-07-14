#ifndef __LQ_GPIO_IRQ_H__
#define __LQ_GPIO_IRQ_H__

#include "lq_include.h"

/* GPIO 中断触发模式 */
#define PIN_IRQ_MODE_RISING IfxPort_InputMode_pullDown             /*!< 上升沿（下拉）触发中断 */
#define PIN_IRQ_MODE_FALLING IfxPort_InputMode_pullUp              /*!< 下降沿（上拉）触发中断 */
#define PIN_IRQ_MODE_RISING_FALLING IfxPort_InputMode_noPullDevice /*!< 双边沿（开漏）触发中断 */

#endif /* __LQ_GPIO_IRQ_H__ */
