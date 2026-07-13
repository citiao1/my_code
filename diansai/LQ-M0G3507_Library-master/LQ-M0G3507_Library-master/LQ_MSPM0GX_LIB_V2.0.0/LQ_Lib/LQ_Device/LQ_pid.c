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
#include "LQ_pid.h"

/*************************************************************************
 * @name     constrain_float
 *
 * @brief    限幅函数
 *
 * @param    amt        需要限幅的参数
 * @param    low        最低值
 * @param    high       最高值
 * 
 * @return   限幅后的参数
 * 
 * @example  constrain_float(pid->integrator, -pid->imax, pid->imax);
 *************************************************************************/
float constrain_float(float amt, float low, float high)
{
    return ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)));
}

/*************************************************************************
 * @name     LQ_PID_Init
 *
 * @brief    PID 参数初始化函数
 *
 * @param    pid        PID 参数结构体
 * 
 * @return   none
 *************************************************************************/
void LQ_PID_Init(LQConfig_PID_Param_t *pid)
{
    pid->kp              = 0;
    pid->ki              = 0;
    pid->kd              = 0;
    pid->imax            = 0;
    pid->out_p           = 0;
    pid->out_i           = 0;
    pid->out_d           = 0;
    pid->out             = 0;
    pid->integrator      = 0;
    pid->last_error      = 0;
    pid->last_derivative = 0;
    pid->last_t          = 0;
}

/*************************************************************************
 * @name     LQ_PID_LocCtrl
 *
 * @brief    PID 位置式控制器输出
 *
 * @param    pid        PID 参数结构体
 * @param    error      误差值
 * 
 * @return   none
 *************************************************************************/
float LQ_PID_LocCtrl(LQConfig_PID_Param_t *pid, float error)
{
    /* 累积误差 */
    pid->integrator += error;

    /* 误差限幅 */
    constrain_float(pid->integrator, -pid->imax, pid->imax);

    pid->out_p = pid->kp * error;
    pid->out_i = pid->ki * pid->integrator;
    pid->out_d = pid->kd * (error - pid->last_error);

    pid->last_error = error;

    pid->out = pid->out_p + pid->out_i + pid->out_d;

    return pid->out;
}

/*************************************************************************
 * @name     LQ_PID_IncCtrl
 *
 * @brief    PID 增量式控制
 *
 * @param    pid        PID 参数结构体
 * @param    error      误差值
 * 
 * @return   none
 *************************************************************************/
float LQ_PID_IncCtrl(LQConfig_PID_Param_t *pid, float error)
{

    pid->out_p = pid->kp * (error - pid->last_error);
    pid->out_i = pid->ki * error;
    pid->out_d = pid->kd * ((error - pid->last_error) - pid->last_derivative);

    pid->last_derivative = error - pid->last_error;
    pid->last_error = error;

    pid->out += pid->out_p + pid->out_i + pid->out_d;

    return pid->out;
}

/*************************************************************************
 * @name     LQ_PID_CasCtrl
 *
 * @brief    串级式 PID 控制（转向环）
 *
 * @param    external_pid       外环 PID 参数结构体
 * @param    interior_pid       内环 PID 参数结构体
 * @param    error              误差值
 * @param    gyro_z1            陀螺仪数据
 * 
 * @return   none
 *************************************************************************/
float LQ_PID_CasCtrl(LQConfig_PID_Param_t *external_pid, LQConfig_PID_Param_t *interior_pid, float error, int16_t gyro_z1)
{

    external_pid->out_p = external_pid->kp * error;

    external_pid->out_d = external_pid->kd * gyro_z1;

    external_pid->out = external_pid->out_p + external_pid->out_d;

    external_pid->last_error = error;

    error = external_pid->out - gyro_z1;

    interior_pid->out_p = interior_pid->kp * error;

    interior_pid->out_d = interior_pid->kd * external_pid->last_error;

    interior_pid->out = interior_pid->out_p + interior_pid->out_d;

    return interior_pid->out;
}

