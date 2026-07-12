/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
 【平    台】北京龙邱智能科技STC32位核心板
 【编    写】龙邱科技
 【E-mail  】chiusir@163.com
 【软件版本】V1.0 版权所有，单位使用请先联系授权
 【相关信息参考下列地址】
 【网    站】http://www.lqist.cn
 【淘宝店铺】http://longqiu.taobao.com
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/

#include <LQ_PID.h>
extern int16_t gyro_z1;
/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：float constrain_float(float amt, float low, float high)
@功能说明：限幅函数
@参数说明：amt：参数 low：最低值 high：最高值
@函数返回：无
@修改时间：2022/02/24
@调用方法：constrain_float(pid->integrator, -pid->imax, pid->imax);
@备    注：
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
float constrain_float(float amt, float low, float high)
{
    return ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)));
}

// pid参数初始化函数
void PidInit(pid_param_t *pid)
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

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：float constrain_float(float amt, float low, float high)
@功能说明：pid位置式控制器输出
@参数说明：
@函数返回：
@修改时间：2022/02/24
@调用方法：
@备    注：
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
float PidLocCtrl(pid_param_t *pid, float error)
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

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：PidIncCtrl(pid_param_t * pid, float error)
@功能说明：增量式pid控制
@参数说明：
@函数返回：
@修改时间：2022/02/24
@调用方法：
@备    注：
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
float PidIncCtrl(pid_param_t *pid, float error)
{

    pid->out_p = pid->kp * (error - pid->last_error);
    pid->out_i = pid->ki * error;
    pid->out_d = pid->kd * ((error - pid->last_error) - pid->last_derivative);

    pid->last_derivative = error - pid->last_error;
    pid->last_error = error;

    pid->out += pid->out_p + pid->out_i + pid->out_d;

    return pid->out;
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：PidCasCtrl(pid_param_t * pid, float error)
@功能说明：串级式pid控制（转向环）
@参数说明：
@函数返回：
@修改时间：2024/01/09
@调用方法：
@备    注：
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
float PidCasCtrl(pid_param_t *external_pid, pid_param_t *interior_pid, float error)
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
