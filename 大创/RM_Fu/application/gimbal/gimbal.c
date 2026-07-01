#include "gimbal.h"

/*
 *****************************************************************************************
 * 云台控制模块（gimbal.c）
 *
 * 功能概览：
 * 1. 控制GM6020云台电机（Yaw/Pitch），采用角度-速度-电流三环控制
 * 2. 支持遥控器与键鼠两种控制模式（cmd_mode由ControlTask设置）
 * 3. 提供小陀螺模式/键鼠模式下的特殊前馈补偿，降低云台抖动
 * 4. 结合IMU姿态，累计Yaw总角度，多圈控制云台
 *
 * 数据流：
 * ControlTask -> gimbal_cd（指令） -> GimbalTask -> MotorControl -> CAN
 *****************************************************************************************
 */

// ------------------------- 静态变量定义 -------------------------
static MOTORInstance *motor_yaw;          // 云台Yaw电机实例
static MOTORInstance *motor_pitch;        // 云台Pitch电机实例
static AHRS_FEED *imu_feed;               // IMU姿态数据（Yaw/Pitch角度、速度）
static Gimbal_Ctrl_Cmd gimbal_cd;         // 云台控制命令缓存
static float motor_yaw_set;               // Yaw电机目标角度（或目标值）
static float motor_pitch_set;             // Pitch电机目标角度
static int16_t changle_count;             // Pitch角度变化增量
static int16_t total_angle = 3200;        // Pitch累计角度（编码值范围）
static float fw_value;                    // Yaw前馈补偿值
static RC_Ctl_t *rc;                      // 遥控器数据指针
extern float yawcotor;                    // 外部标志位：1表示需要同步IMU角度
uint8_t yaw_enable_flag = 0;              // 防止上电初始角度失控

// ------------------------- 工具函数 -------------------------
static float Abs(float value)
{
    return (value >= 0.0f) ? value : -value;
}


/**
 * @brief  遥控器模式下的Yaw前馈补偿
 * @param  motor: Yaw电机实例
 * @retval float: 前馈值
 *
 * 功能说明：
 * - 根据电机速度与小陀螺状态，动态调整前馈量，减小惯性
 * - rc->rc.s2 == 2 表示小陀螺模式，需要反向补偿
 */
static float GimbalYawForward(MOTORInstance *motor)
{
    static float KF = 0.060f;     // 前馈增益
    static float rspeed;          // 当前速度
    static float lrspeed;         // 上一次速度
    static float symbol;          // 旋转方向符号
    static uint64_t tick;         // 小陀螺状态时间戳
    static uint8_t state = 0;     // 小陀螺模式状态机

    rspeed = motor->measure.rspead;
    lrspeed = motor->measure.lastrspead; 
    symbol = rspeed / (Abs(rspeed) + 1.0f);

    if(rc->rc.s2 == 2)  // 小陀螺模式
    {
        if(state == 0)
        {   
            tick = uwTick;
            state = 1;
        }
        symbol = -symbol;  // 旋转方向取反
        if(uwTick - tick >= 50 && state == 1)
        {
            symbol = 0;
            KF = 0.0f;      // 50ms后取消前馈，避免抖动
        }
    }
    else
    {
        KF = 0.06f;
        state = 0;
    }

    if (rc->rc.s2 == 2)
    {
        fw_value = -56.0f;        // 小陀螺固定前馈
    }
    else
    {
        fw_value = (rspeed / (Abs(rspeed) + 1.0f)) * KF * (Abs(rspeed * 0.50f + lrspeed * 0.50f - symbol * 150.0f));
    }
    return fw_value;
}


/**
 * @brief  键鼠模式下的Yaw前馈补偿
 * @param  motor: Yaw电机实例
 * @retval float: 前馈值
 *
 * 与遥控器模式类似，只是判断条件使用键鼠指令（gimbal_cd.rotatemode）
 */
static float MouseGimbalYawForward(MOTORInstance *motor)
{
    static float KF = 0.060f;
    static float rspeed;
    static float lrspeed;
    static float symbol;
    static uint64_t tick;
    static uint8_t state = 0;

    rspeed = motor->measure.rspead;
    lrspeed = motor->measure.lastrspead;
    symbol = rspeed / (Abs(rspeed) + 1.0f);

    if (gimbal_cd.rotatemode == 1)  // 键鼠小陀螺模式
    {
        if (state == 0)
        {
            tick = uwTick;
            state = 1;
        }
        symbol = -symbol;
        if (uwTick - tick >= 50 && state == 1)
        {
            symbol = 0;
            KF = 0.0f;
        }
    }
    else
    {
        KF = 0.06f;
        state = 0;
    }

    if (gimbal_cd.rotatemode == 1)
    {
        fw_value = -54.0f;
    }
    else
    {
        fw_value = (rspeed / (Abs(rspeed) + 1.0f)) * KF * (Abs(rspeed * 0.50f + lrspeed * 0.50f - symbol * 150.0f));
    }

    return fw_value;
}

/**
 * @brief  云台初始化
 * @details 初始化Yaw/Pitch电机（GM6020），配置PID参数并绑定IMU/遥控器数据
 */
void GimbalInit(void)
{
    imu_feed = GetAHRSFeed();

    // Yaw电机配置：角度环（外部IMU反馈）+速度环，含前馈
    Motor_Init_Config_s motor_yaw_init =
    {
        .motor_type = GM6020,
        .contorller_setting_init_config = {
            .outer_loop_type = ANGLE_LOOP,
            .close_loop_type = ANGLE_LOOP | SPEED_LOOP,
            .angle_feedback_source = OTHER_FEED,
            .speed_feedback_source = MOTOR_FEED,
            .feedback_reserve_flag = FEEDBACK_DIRCTION_NORMAL,
            .Feedfoward_flag = SPEED_FEEDFORWARD | ANGLE_FEEDFORWARD,
            .motor_reverse_flag = MOTOR_DIRECTION_NORMAL
        },
        .contorller_param_init_config = {
            .angle_PID = {
                .Kp = 13.3f,
                .Ki = 0.35f,
                .Kd = 0.042f,
                .KF = 3.0f,
                .MaxOut = 13500.0f,
                .DeadBand = 10.0f,
                .IntegralLimit = 163.0f,
                .Improve = PID_Integral_limit | PID_ChangingIntegrationRate,
                .Output_LPF_RC = 0.5f,
                .Derivative_LPF_RC = 2.0f,
                .CoefA = 1000.0f,
                .CoefB = 2.0f
            },
            .speed_PID = {
                .Kp = 15.7f,
                .Ki = 0.2f,
                .Kd = 225.0f,
                .MaxOut = 26000.0f,
                .DeadBand = 50.0f,
                .IntegralLimit = 1100.0f,
                .Improve = PID_Integral_limit | PID_Derivative_DerivativeFilter | PID_ChangingIntegrationRate,
                .Output_LPF_RC = 0.1f,
                .Derivative_LPF_RC = 2.0f,
                .CoefA = 120.0f,
                .CoefB = 20.0f
            },
            .other_angle_feedback_prt = &imu_feed->YawTotalDegree,
            .angle_feedforward_prt = &fw_value,
        },
        .can_init_config.tx_id = 6,
    };

    // Pitch电机配置：角度环+速度环（均采用电机内部反馈）
    Motor_Init_Config_s motor_pitch_init =
    {
        .motor_type = GM6020,
        .contorller_setting_init_config = {
            .outer_loop_type = ANGLE_LOOP,
            .close_loop_type = ANGLE_LOOP | SPEED_LOOP,
            .angle_feedback_source = MOTOR_FEED,
            .speed_feedback_source = MOTOR_FEED,
            .feedback_reserve_flag = FEEDBACK_DIRCTION_NORMAL,
            .motor_reverse_flag = MOTOR_DIRECTION_NORMAL,
            .Feedfoward_flag = FEEDFORWARD_NONE
        },
        .contorller_param_init_config = {
            .angle_PID = {
                .Kp = 1.68f,
                .Ki = 0.078f,
                .Kd = 0.0f,
                .MaxOut = 80000.0f,
                .DeadBand = 0.0f,
                .IntegralLimit = 428.0f,
                .Improve = PID_Integral_limit,
                .Output_LPF_RC = 3.0f,
                .Derivative_LPF_RC = 0.0f,
                .CoefA = 0.0f,
                .CoefB = 0.0f
            },
            .speed_PID = {
                .Kp = 7.05f,
                .Ki = 0.0f,
                .Kd = 0.23f,
                .MaxOut = 100000.0f,
                .DeadBand = 10.0f,
                .IntegralLimit = 2020.0f,
                .Improve = PID_Derivative_DerivativeFilter | PID_Integral_limit,
                .Output_LPF_RC = 3.0f,
                .Derivative_LPF_RC = 1.8f,
                .CoefA = 0.0f,
                .CoefB = 0.0f
            },
        },
        .can_init_config.tx_id = 5,
    };

    motor_yaw = MotorInit(&motor_yaw_init);
    motor_pitch = MotorInit(&motor_pitch_init);

    rc = GetRCData();
    gimbal_cd.pitch_total_degree = 3200.0f;                     // Pitch初始角度（编码计数）
    gimbal_cd.yaw_total_degree = imu_feed->YawTotalDegree;      // 从IMU同步Yaw总角度
}

/**
 * @brief  将目标角度写入电机控制器
 */
static void GimbalOutPut(void)
{
    MotorSetRef(motor_yaw, motor_yaw_set);
    MotorSetRef(motor_pitch, motor_pitch_set);
}

/**
 * @brief  Pitch角度积分控制（根据键鼠/遥控输入调整累计角度）
 * @retval int16_t : pitch累计角度（限制在安全范围内）
 */
static int16_t AngleContol(void)
{
    changle_count = gimbal_cd.pitch_change_degree;

    if (changle_count >= -50 && changle_count <= 50)
    {
        // 微小输入忽略，防止抖动
        total_angle -= 0;
    }
    else
    {
        total_angle -= (changle_count) / 50;  // 将键鼠输入映射到角度变化
    }

    // 角度限制，防止撞击
    if (total_angle >= 3500)
    {
        total_angle = 3500;
    }
    if (total_angle <= 1960)
    {
        total_angle = 1960;
    }
    return total_angle;
}






/**
 * @brief  云台任务：控制Yaw/Pitch电机
 */
void GimbalTask(void)
{
    MotorEnable(motor_yaw);
    MotorEnable(motor_pitch);
    gimbal_cd.pitch_total_degree = (float)AngleContol();

    // 第一次启动时同步IMU角度
    if(yawcotor == 1)
    {
        gimbal_cd.yaw_total_degree = imu_feed->YawTotalDegree;
        yaw_enable_flag = 1;
        yawcotor = 2;
    }

    motor_pitch_set = gimbal_cd.pitch_total_degree;
    motor_yaw_set = gimbal_cd.yaw_total_degree;

    // 根据控制模式选择不同的前馈算法
    if(gimbal_cd.cmd_mode == 0)
    {
        GimbalYawForward(motor_yaw);
    }
    else
    {
        MouseGimbalYawForward(motor_yaw);
    }

    // 上电1秒内限制输出，防止初始抖动
    if(uwTick <= 1000)
    {
        motor_yaw->motor_contorller.angle_PID.MaxOut = 500.0f;
    }
    else
    {
        motor_yaw->motor_contorller.angle_PID.MaxOut = 2500.0f;
    }

    GimbalOutPut();
    MotorControl();
}


/**
 * @brief  获取云台控制命令结构体
 */
Gimbal_Ctrl_Cmd *GetGimbalCmd(void)
{
    return &gimbal_cd;
}

/**
 * @brief  获取Yaw电机实例
 */
MOTORInstance *GetYawMotor(void)
{
    return motor_yaw;
}

/**
 * @brief  获取Pitch电机实例
 */
MOTORInstance *GetPitchMotor(void)
{
    return motor_pitch;
}


