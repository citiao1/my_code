/*******************************************************************************
 * @file                include.h
 * @brief               通用头文件汇总 - 包含 LQ_MSPM0GX_LIB 全部外设与公共库
 * @copyright           龙邱科技 (C) 2025-2026 版权所有，未经许可不得用于商业用途
 * @website             http://www.lqist.cn
 * @taobao              http://longqiu.taobao.com
 *
 * @description         适用于 MSPM0G3507 芯片的开发板通用工程头文件
 *
 * 工程环境：
 *   - 开发工具 : Keil5
 *   - 主控芯片 : MSPM0G3507
 *   - 输入频率 : 16.000MHz（外部晶振）
 *   - 主频     : 80MHz（PLL 倍频后）
 *
 * 本文件遵循 GPL-3.0 开源协议：
 * 二次开发时请尊重原作者版权，引用本工程的 MSPM0G3507 相关代码请注明来源于龙邱科技。
 * 不得用于任何商业用途，违者必究。
 *
 * GPL-3.0 协议主要内容：
 * 1. 你可以自由使用、修改本工程的源代码
 * 2. 你可以自由分发本工程的修改版本
 * 3. 修改后的代码必须同样开源
 * 4. 不得用于任何商业用途
 * 5. 详细协议内容请参考本工程根目录下的 LICENSE 文件
 *
 * @author              wuwu    (龙邱科技)
 * @author              LQ_012  (本工程移植修改)
 * @email               chiusir@163.com
 * @version             V2.0.0
 * @update              2026年04月24日
 *******************************************************************************/
#ifndef __INCLUDE_H_
#define __INCLUDE_H_


/*  C 标准库头文件  */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*  TI 官方驱动库  */
#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

/*  龙邱外设驱动库  */
#include "LQ_clock.h"
#include "LQ_gpio.h"
#include "LQ_uart.h"
#include "LQ_exti.h"
#include "LQ_spi.h"
#include "LQ_time.h"
#include "LQ_pwm.h"
#include "LQ_dma.h"
#include "LQ_adc.h"
#include "LQ_soft_i2c.h"
#include "LQ_soft_spi.h"

#include "LQ_common.h"
#include "LQ_1306_motor.h"

/*  用户模块头文件  */


#endif
