/*******************************************************************************
 * @file                LQ_clock.h
 * @brief               本文件是 LQ_MSPM0GX_LIB 软件开源库文件的一部分
 * @copyright           版权所有 (C) 2025-2026 北京龙邱科技有限公司
 * @website             http://www.lqist.cn
 * @taobao              http://longqiu.taobao.com
 *
 * @description         龙邱科技 MSPM0G3507 核心板驱动库声明
 *
 * 开发环境配置:
 *   - 使用环境 : Keil5
 *   - 目标芯片 : MSPM0G3507
 *   - 外置晶振 : 16.000MHz
 *   - 系统时钟 : 80MHz
 *
 * 本文件遵循GPL-3.0开源协议发布，旨在为 MSPM0G3507 芯片嵌入式系统设计提供快速上手开发基于 MSPM0G3507 的应用程序的参考实现
 * 商业用途（包括单位使用）需提前联系作者获得授权
 *
 * GPL-3.0 许可证声明摘要:
 * 1. 允许自由使用、修改、分发本软件
 * 2. 分发修改后的版本时，必须以相同许可证发布
 * 3. 必须保留原始版权声明和许可证信息
 * 4. 不提供任何担保，使用风险自负
 * 5. 完整协议文本请参见项目根目录 LICENSE 文件
 *
 * @author				wuwu	(接口层编写)
 * @author              LQ_012	(优化与应用层编写)
 * @email               chiusir@163.com
 * @version             V2.0.0
 * @update              2026年4月24日
 *******************************************************************************/
#ifndef __LQ_CLOCK_H__
#define __LQ_CLOCK_H__

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

/*========================== HFCLK 源选择 ==========================*/

/*!
 * @brief		时钟源选择：取消注释你需要的配置（仅保留一个）
 */
//#define LQ_CLK_SRC_HFXT_40MHZ			/* 外部 40MHz 晶振 */
#define LQ_CLK_SRC_HFXT_16MHZ			/* 外部 16MHz 晶振 */
//#define LQ_CLK_SRC_SYSOSC_4MHZ			/* 内部 4MHz 振荡器（默认） */

/*========================== LFCLK 源选择 ==========================*/

//#define LQ_CLK_SRC_LFXT_32KHZ      		/* 外部 32kHz 晶振 */
#define LQ_CLK_SRC_LFOSC_32KHZ    		/* 内部 32kHz 振荡器（默认） */

/*=========================== 时钟配置宏 ===========================*/

/* 开启电源后等待稳定的时长 */
#define POWER_STARTUP_DELAY                           			( 16 )

/* 浮点数转整数比例系数(放大 1000 倍, 避免浮点运算) */
#define FLOAT_TO_INT_SCALE										( 1000U )

#ifdef LQ_CLK_SRC_HFXT_40MHZ
#define FCC_EXPECTED_RATIO										( 1000 )						/* FCC 预期频率比值 */
#endif

#ifdef LQ_CLK_SRC_HFXT_16MHZ
#define FCC_EXPECTED_RATIO										( 2500 )
#endif

#ifdef LQ_CLK_SRC_SYSOSC_4MHZ
#define FCC_EXPECTED_RATIO										( 1250 )
#endif

/* FCC频率比值上下限(允许 ±0.3% 误差) */
#define FCC_UPPER_BOUND											( FCC_EXPECTED_RATIO * (1 + 0.003) )
#define FCC_LOWER_BOUND											( FCC_EXPECTED_RATIO * (1 - 0.003) )

/* 主时钟频率 */
#define CPUCLK_FREQ                                           	( 80000000 )

/*  函数声明  */

void LQ_System_Init(void);				// 系统初始化

#endif
