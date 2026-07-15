/*******************************************************************************
 * @file                LQ_pid.c
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
 * @author              LQ_012
 * @email               chiusir@163.com
 * @version             V2.0.0
 * @update              2026年4月24日
 *******************************************************************************/
#ifndef __LQ_PID_H__
#define __LQ_PID_H__

#include "include.h"

typedef struct
{
  float                kp;         // P
  float                ki;         // I
  float                kd;         // D
  float                imax;       // 积分限幅

  float                out_p;       // KP输出
  float                out_i;       // KI输出
  float                out_d;       // KD输出
  float                out;         // pid输出

  float                integrator; 		//< 积分值
  float                last_error; 		//< 上次误差
  float                last_derivative; //< 上次误差与上上次误差之差
  unsigned long        last_t;			//< 上次时间
}LQConfig_PID_Param_t;

// PID 参数初始化函数
void LQ_PID_Init(LQConfig_PID_Param_t * pid);

// 限幅函数
float constrain_float(float amt, float low, float high);

// PID 位置式控制器输出
float LQ_PID_LocCtrl(LQConfig_PID_Param_t * pid, float error);

// PID 增量式控制
float LQ_PID_IncCtrl(LQConfig_PID_Param_t * pid, float error);

// 串级式 PID 控制（转向环）
float LQ_PID_CasCtrl(LQConfig_PID_Param_t *external_pid, LQConfig_PID_Param_t *interior_pid, float error, int16_t gyro_z1);

#endif
