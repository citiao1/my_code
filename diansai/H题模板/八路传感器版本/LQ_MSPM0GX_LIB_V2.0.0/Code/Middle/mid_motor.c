/*******************************************************************************
 * @file    mid_motor.c
 * @brief   电机中间层实现
 *
 * @note    封装 LQ_Motor_Init / LQ_Motor_SetDuty
 *          自动完成:限幅、占空比符号处理
 *          底层 LQ_Motor 已改为 AT8236 慢衰减驱动(2 路 PWM/电机),
 *          中间层不再需要软件死区补偿,故删除 SetDead 接口
 *******************************************************************************/

#include "mid_motor.h"

void MID_Motor_Init(uint32_t freq_hz)
{
    /* 初始化两路 H 桥,初始占空比 0(静止) */
    LQ_Motor_Init(freq_hz, 0);
}

void MID_Motor_SetDuty(MID_Motor_t motor, int32_t duty)
{
    /* 限幅 */
    if (duty >  MOTOR_PWM_DUTY_MAX)  duty =  MOTOR_PWM_DUTY_MAX;
    if (duty < -MOTOR_PWM_DUTY_MAX)  duty = -MOTOR_PWM_DUTY_MAX;

    /* LQ_Motor_SetDuty 内部已处理方向(正/负/零)和 AT8236 慢衰减逻辑,
     * 这里直接透传即可
     */
    if (motor == MID_MOTOR_LEFT) {
        LQ_Motor_SetDuty(MOTOR_CH1, duty);
    } else {
        LQ_Motor_SetDuty(MOTOR_CH2, duty);
    }
}

void MID_Motor_Stop(void)
{
    LQ_Motor_SetDuty(MOTOR_CH1, 0);
    LQ_Motor_SetDuty(MOTOR_CH2, 0);
}
