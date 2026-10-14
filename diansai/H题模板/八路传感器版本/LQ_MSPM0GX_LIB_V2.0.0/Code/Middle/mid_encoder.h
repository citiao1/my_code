#ifndef __MID_ENCODER_H__
#define __MID_ENCODER_H__

/*******************************************************************************
 * @file    mid_encoder.h
 * @brief   编码器中间层封装头文件（Middle Layer）
 *
 * @note    封装 LQ_Encoder，将原始计数值转换为实际速度 (mm/s)
 *          每个编码器对应一个 LQConfig_Encoder_InitTypeDef_t 实例
 *
 *          计算公式：
 *          speed = delta * 1e6 / (COUNTS_PER_METER * SAMPLE_MS)  (mm/s)
 *          其中 COUNTS_PER_METER = 实际标定值（1米对应的编码器脉冲数）
 *          已包含 4 倍频系数、减速比，是最终有效的值
 *******************************************************************************/

#include "include.h"
#include "LQ_encoder.h"

/* 编码器引脚（按实际接线修改） */
#define MID_ENCODER_LEFT_PINA          (GPIO_Pin_A_7)
#define MID_ENCODER_LEFT_PINB          (GPIO_Pin_A_3)
#define MID_ENCODER_RIGHT_PINA         (GPIO_Pin_A_8)
#define MID_ENCODER_RIGHT_PINB         (GPIO_Pin_B_7)

/* 编码器线数：500 线 x 4 倍频 = 2000 脉冲/圈 */
#define MID_ENCODER_LINES              (500u)
#define MID_ENCODER_QUAD               (4u)
#define MID_ENCODER_PULSES_PER_REV     (MID_ENCODER_LINES * MID_ENCODER_QUAD)  /* 2000 */

/* 采样周期（必须与 LQ_Encoder_Init 的 time 一致） */
#define MID_ENCODER_SAMPLE_MS          (5u)

/* 实际标定值：1米对应的编码器脉冲数（让车跑刚好1米后看编码器计数）
 * = 脉冲/圈 x 周长 = 2000 x (2*pi*0.033) = 414
 * 约 3726 为实际值（考虑了减速比或不同直径的车轮）
 */
#define MID_ENCODER_COUNTS_PER_METER   (3726.0f)

/* 一阶 IIR 低通滤波器：alpha 为新值权重（0~1，越大滤波越弱） */
#define MID_ENCODER_IIR_ALPHA          (0.35f)

/* 编码器枚举实例 */
typedef enum {
    MID_ENCODER_LEFT  = 0,
    MID_ENCODER_RIGHT = 1,
} MID_Encoder_t;

/* 初始化编码器
 * @note  内部调用 LQ_Encoder_Init，使用 mid_encoder.h 中的引脚定义
 *        初始化结构体并指定每一路实例
 */
void MID_Encoder_Init(void);

/* 每 5ms 调用一次，从编码器读取累计计数值，转 mm/s、IIR 滤波、累计脉冲
 * @note  可由调度器 (app_scheduler.c) 在 5ms 任务周期内调用
 *        必须在调用 PID 计算之前调用
 */
void MID_Encoder_Update(void);

/* 获取滤波后速度，单位 mm/s
 * @param enc  左/右编码器
 * @return     速度（正=前进，负=后退）
 * @note       必须先调用 MID_Encoder_Update()
 */
float MID_Encoder_GetSpeed(MID_Encoder_t enc);

/* 获取累计脉冲数（跑完标定刚好1米后看编码器 = 脉冲/米）
 * @param enc  左/右编码器
 * @return     从开机累计的总脉冲数
 * @note       单位为编码器脉冲
 */
int32_t MID_Encoder_GetTotal(MID_Encoder_t enc);


/* 里程归零：记录当前左右轮累计脉冲为基准
 * @note  供 K1 发车模式调用，发车时里程从 0 开始累计
 *        内部记录 MID_Encoder_GetTotal(LEFT/RIGHT) 当前值作为基准
 */
void MID_Encoder_OdomReset(void);

/* 查询累计行驶里程（mm）
 * @return 从上次 OdomReset 起的累计里程（mm），左右轮取平均
 *         计算式：((total_l - start_l) + (total_r - start_r)) / 2 / COUNTS_PER_METER * 1000
 * @note  基于脉冲差分，无积分漂移，精度高
 *        未调用 OdomReset 前基准为 0，返回的是绝对累计里程
 */
float MID_Encoder_GetOdomMm(void);

#endif