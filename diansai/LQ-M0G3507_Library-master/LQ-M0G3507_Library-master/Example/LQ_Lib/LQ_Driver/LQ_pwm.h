/*******************************************************************************
 * @file                LQ_pwm.h
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
#ifndef __LQ_PWM_H__
#define __LQ_PWM_H__

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#include "LQ_common.h"

#include "LQ_gpio.h"
#include "LQ_time.h"

/*!
 * @brief       PWM信号引脚枚举
 * 
 * @note        只有 TIMA0 支持 4 通道输出 PWM
 *              其余 TIM   支持 2 通道输出 PWM
 *              一个定时器通道只能选用一个引脚
 *              一个定时器只能有一个 PWM 频率
 * 
 * @note        默认总线时钟为80M
 * @note        注意 LQ_TIMERG_0, LQ_TIMERG_6 总线时钟只有40Mhz
 */
typedef enum {
	
    /*! ------------------------------TIMERA0-------------------------------- !*/

    /* TIMA0 CH0 Pin */
    LQ_TIMERA0_PWM_CH0_Pin_A_0 , LQ_TIMERA0_PWM_CH0_Pin_A_8 , LQ_TIMERA0_PWM_CH0_Pin_A_21,
    LQ_TIMERA0_PWM_CH0_Pin_B_8 , LQ_TIMERA0_PWM_CH0_Pin_B_14,

    /* TIMA0 CH1 Pin */
    LQ_TIMERA0_PWM_CH1_Pin_A_1 , LQ_TIMERA0_PWM_CH1_Pin_A_3 , LQ_TIMERA0_PWM_CH1_Pin_A_7 ,
    LQ_TIMERA0_PWM_CH1_Pin_A_9 , LQ_TIMERA0_PWM_CH1_Pin_A_22, 
    LQ_TIMERA0_PWM_CH1_Pin_B_9 , LQ_TIMERA0_PWM_CH1_Pin_B_12, LQ_TIMERA0_PWM_CH1_Pin_B_20,

    /* TIMA0 CH2 Pin */
    LQ_TIMERA0_PWM_CH2_Pin_A_3 , LQ_TIMERA0_PWM_CH2_Pin_A_7 , LQ_TIMERA0_PWM_CH2_Pin_A_10, 
    LQ_TIMERA0_PWM_CH2_Pin_A_15, 
    LQ_TIMERA0_PWM_CH2_Pin_B_0 , LQ_TIMERA0_PWM_CH2_Pin_B_4 , LQ_TIMERA0_PWM_CH2_Pin_B_12, 
    LQ_TIMERA0_PWM_CH2_Pin_B_17, LQ_TIMERA0_PWM_CH2_Pin_B_20,

    /* TIMA0 CH3 Pin */
    LQ_TIMERA0_PWM_CH3_Pin_A_4 , LQ_TIMERA0_PWM_CH3_Pin_A_12, LQ_TIMERA0_PWM_CH3_Pin_A_17,
    LQ_TIMERA0_PWM_CH3_Pin_A_23, LQ_TIMERA0_PWM_CH3_Pin_A_25, LQ_TIMERA0_PWM_CH3_Pin_A_28, 
    LQ_TIMERA0_PWM_CH3_Pin_B_2 , LQ_TIMERA0_PWM_CH3_Pin_B_13, LQ_TIMERA0_PWM_CH3_Pin_B_24,
    LQ_TIMERA0_PWM_CH3_Pin_B_26,

    /*! ------------------------------TIMERA1-------------------------------- !*/

    /* TIMA1 CH0 Pin */
    LQ_TIMERA1_PWM_CH0_Pin_A_10, LQ_TIMERA1_PWM_CH0_Pin_A_15, LQ_TIMERA1_PWM_CH0_Pin_A_17,
    LQ_TIMERA1_PWM_CH0_Pin_A_28, 
    LQ_TIMERA1_PWM_CH0_Pin_B_0 , LQ_TIMERA1_PWM_CH0_Pin_B_2 , LQ_TIMERA1_PWM_CH0_Pin_B_4 , 
    LQ_TIMERA1_PWM_CH0_Pin_B_17, LQ_TIMERA1_PWM_CH0_Pin_B_26,

    /* TIMA1 CH1 Pin */
    LQ_TIMERA1_PWM_CH1_Pin_A_11, LQ_TIMERA1_PWM_CH1_Pin_A_16, LQ_TIMERA1_PWM_CH1_Pin_A_18,
    LQ_TIMERA1_PWM_CH1_Pin_A_24, LQ_TIMERA1_PWM_CH1_Pin_A_31, 
    LQ_TIMERA1_PWM_CH1_Pin_B_1 , LQ_TIMERA1_PWM_CH1_Pin_B_3 , LQ_TIMERA1_PWM_CH1_Pin_B_5 , 
    LQ_TIMERA1_PWM_CH1_Pin_B_18, LQ_TIMERA1_PWM_CH1_Pin_B_27,

    /*! ------------------------------TIMERG0-------------------------------- !*/

    /* TIMG0 CH0 Pin */
    LQ_TIMERG0_PWM_CH0_Pin_A_5 , LQ_TIMERG0_PWM_CH0_Pin_A_12, LQ_TIMERG0_PWM_CH0_Pin_A_23,
    LQ_TIMERG0_PWM_CH0_Pin_B_10,

    /* TIMG0 CH1 Pin */
    LQ_TIMERG0_PWM_CH1_Pin_A_6 , LQ_TIMERG0_PWM_CH1_Pin_A_13, LQ_TIMERG0_PWM_CH1_Pin_A_24,
    LQ_TIMERG0_PWM_CH1_Pin_B_11,

    /*! ------------------------------TIMERG6-------------------------------- !*/

    /* TIMG6 CH0 Pin */
    LQ_TIMERG6_PWM_CH0_Pin_A_5 , LQ_TIMERG6_PWM_CH0_Pin_A_21, LQ_TIMERG6_PWM_CH0_Pin_A_29,
    LQ_TIMERG6_PWM_CH0_Pin_B_2 , LQ_TIMERG6_PWM_CH0_Pin_B_6 , LQ_TIMERG6_PWM_CH0_Pin_B_10,
    LQ_TIMERG6_PWM_CH0_Pin_B_26,

    /* TIMG6 CH1 Pin */
    LQ_TIMERG6_PWM_CH1_Pin_A_6 , LQ_TIMERG6_PWM_CH1_Pin_A_22, LQ_TIMERG6_PWM_CH1_Pin_A_30,
    LQ_TIMERG6_PWM_CH1_Pin_B_3 , LQ_TIMERG6_PWM_CH1_Pin_B_7 , LQ_TIMERG6_PWM_CH1_Pin_B_11,
    LQ_TIMERG6_PWM_CH1_Pin_B_27,

    /*! ------------------------------TIMERG7-------------------------------- !*/

    /* TIMG7 CH0 Pin */
    LQ_TIMERG7_PWM_CH0_Pin_A_3 , LQ_TIMERG7_PWM_CH0_Pin_A_17, LQ_TIMERG7_PWM_CH0_Pin_A_23,
    LQ_TIMERG7_PWM_CH0_Pin_A_26, LQ_TIMERG7_PWM_CH0_Pin_A_28, 
    LQ_TIMERG7_PWM_CH0_Pin_B_15,

    /* TIMG7 CH1 Pin */
    LQ_TIMERG7_PWM_CH1_Pin_A_2 , LQ_TIMERG7_PWM_CH1_Pin_A_4 , LQ_TIMERG7_PWM_CH1_Pin_A_7 ,
    LQ_TIMERG7_PWM_CH1_Pin_A_18, LQ_TIMERG7_PWM_CH1_Pin_A_24, LQ_TIMERG7_PWM_CH1_Pin_A_27,
    LQ_TIMERG7_PWM_CH1_Pin_A_31, 
    LQ_TIMERG7_PWM_CH1_Pin_B_16, LQ_TIMERG7_PWM_CH1_Pin_B_19,

    /*! ------------------------------TIMERG8-------------------------------- !*/

    /* TIMG8 CH0 Pin */
    LQ_TIMERG8_PWM_CH0_Pin_A_1 , LQ_TIMERG8_PWM_CH0_Pin_A_3 , LQ_TIMERG8_PWM_CH0_Pin_A_5 ,
    LQ_TIMERG8_PWM_CH0_Pin_A_7 , LQ_TIMERG8_PWM_CH0_Pin_A_21, LQ_TIMERG8_PWM_CH0_Pin_A_23,
    LQ_TIMERG8_PWM_CH0_Pin_A_26, LQ_TIMERG8_PWM_CH0_Pin_A_29, 
    LQ_TIMERG8_PWM_CH0_Pin_B_6 , LQ_TIMERG8_PWM_CH0_Pin_B_10, LQ_TIMERG8_PWM_CH0_Pin_B_15, 
    LQ_TIMERG8_PWM_CH0_Pin_B_21,

    /* TIMG8 CH1 Pin */
    LQ_TIMERG8_PWM_CH1_Pin_A_0 , LQ_TIMERG8_PWM_CH1_Pin_A_2 , LQ_TIMERG8_PWM_CH1_Pin_A_4 ,
    LQ_TIMERG8_PWM_CH1_Pin_A_6 , LQ_TIMERG8_PWM_CH1_Pin_A_22, LQ_TIMERG8_PWM_CH1_Pin_A_27, 
    LQ_TIMERG8_PWM_CH1_Pin_A_30, 
    LQ_TIMERG8_PWM_CH1_Pin_B_7 , LQ_TIMERG8_PWM_CH1_Pin_B_11, LQ_TIMERG8_PWM_CH1_Pin_B_16, 
    LQ_TIMERG8_PWM_CH1_Pin_B_19, LQ_TIMERG8_PWM_CH1_Pin_B_22,

    /*! ------------------------------TIMERG12-------------------------------- !*/

    /* TIMG12 CH0 Pin */
    LQ_TIMERG12_PWM_CH0_Pin_A_10, LQ_TIMERG12_PWM_CH0_Pin_A_14,
    LQ_TIMERG12_PWM_CH0_Pin_B_13, LQ_TIMERG12_PWM_CH0_Pin_B_20,

    /* TIMG12 CH1 Pin */
    LQ_TIMERG12_PWM_CH1_Pin_A_25, LQ_TIMERG12_PWM_CH1_Pin_A_31,
    LQ_TIMERG12_PWM_CH1_Pin_B_14, LQ_TIMERG12_PWM_CH1_Pin_B_24,

} LQEnum_PWM_Pin_t;

/*!
 * @brief       PWM 最优配置结构体
 *
 * @note        存储自动计算得到的 PWM 时钟分频、预分频、重装载值
 */
typedef struct {
    uint8_t DivideRatio;   /*!< 时钟分频系数 范围:1~8 */
    uint8_t Prescaler;     /*!< 预分频器值   范围:0~255 */
    uint32_t Period;       /*!< PWM重装载值(占空比最大值) */
} PWM_ConfigTypeDef;

/*!
 * @brief       PWM配置结构体
 * 
 * @note        定时器频率 = 输入时钟 / DivideRatio / Prescaler
 *              PWM频率 = 定时器频率 / Period
 *              
 */
typedef struct {
	
    DL_TIMER_CLOCK_DIVIDE   DivideRatio;  		/* 分割器 对输入时钟先分割*/
    
    uint8_t					Prescaler;     		/* 分频器 0 - 255 对应1 - 256分频*/
    
    uint32_t 				Period;           	/* 装载值 TIMERG12 为32位 其余为16位 */
    
    DL_TIMER_PWM_MODE 		PwmMode;          	/* PWM模式 */
    
    bool startTimer;                            /* 立即开始 */
	
} LQConfig_PWM_InitTypeDef_t;

/****************************************************************************************************
 * @brief   函数定义
 ****************************************************************************************************/

void LQ_TIMER_PWMInit(LQEnum_Timer_t TIMER, LQConfig_PWM_InitTypeDef_t *PWM_Init);	/*! @brief	初始化定时器复用为 PWM 功能 */
void LQ_TIMER_EnablePWMChannel(LQEnum_PWM_Pin_t PWM_PIN);							/*! @brief	使能 PWM 输出引脚 */
void LQ_TIMER_PWM_Start(LQEnum_Timer_t TIMER);										/*! @brief	启动 PWM 输出 */
void LQ_TIMER_PWM_Stop(LQEnum_Timer_t TIMER);										/*! @brief	停止 PWM 输出 */
void LQ_TIMER_PWMSetCaptureCompare(LQEnum_PWM_Pin_t PWM_PIN, uint32_t Value);		/*! @brief	设置PWM捕获比较值（占空比） */

PWM_ConfigTypeDef LQ_PWM_CalcOptimal(LQEnum_Timer_t ch, uint32_t target_freq);      /*! @brief	计算最优 PWM 配置 */

#endif
