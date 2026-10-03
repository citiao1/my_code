#ifndef __MID_MOTOR_H__
#define __MID_MOTOR_H__

/*******************************************************************************
 * @file    mid_motor.h
 * @brief   电机中间层(Middle Layer)
 *
 * @note    封装 LQ_Motor_Init / LQ_Motor_SetDuty,功能:
 *          - 占空比限幅保护
 *          - 电机通道枚举对外统一
 *
 *          底层 LQ_Motor 已改为 AT8236 慢衰减驱动(2 路 PWM/电机),
 *          中间层不再需要软件死区补偿,故删除 SetDead 接口
 *******************************************************************************/

#include "include.h"
#include "LQ_motor.h"

/* 电机通道枚举 */
typedef enum {
    MID_MOTOR_LEFT  = 0,
    MID_MOTOR_RIGHT = 1,
} MID_Motor_t;

/* 电机初始化
 * @param freq_hz  PWM 频率(Hz),推荐 10000 或 20000
 * @note  内部调用 LQ_Motor_Init,初始占空比为 0
 */
void MID_Motor_Init(uint32_t freq_hz);

/* 设置电机占空比
 * @param motor   左/右电机
 * @param duty    占空比,范围 -10000 ~ +10000(正=正转,负=反转)
 * @note  自动限幅,底层自动处理方向和 AT8236 慢衰减逻辑
 */
void MID_Motor_SetDuty(MID_Motor_t motor, int32_t duty);

/* 停止所有电机 */
void MID_Motor_Stop(void);

#endif
