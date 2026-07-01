#include "chassic.h"

/*
 *****************************************************************************************
 * 麦克纳姆底盘控制（chassic.c）
 *
 * 功能概览：
 * 1. 初始化四个M3508底盘电机，配置速度环+电流环控制策略
 * 2. 读取遥控器/键鼠指令，并结合云台偏角（编码器反馈）完成坐标转换
 * 3. 执行麦轮运动学解算，得到四个轮子的目标转速
 * 4. 将目标转速写入电机控制器，在MotorControl()中统一进行PID计算与CAN下发
 *
 * 数据流：
 * ControlTask -> 写入 chassic_cmd_recv -> Chassistask -> MotorControl -> CAN 下发
 *
 * 注意事项：
 * - 本文件仅负责控制逻辑，底层CAN发送在 motor.c 中统一处理
 * - 编码器用于计算云台相对底盘的偏角，以实现底盘/云台解耦
 * - cmd_mode = 0 时，为遥控器模式，需要额外的底盘模式补偿
 *****************************************************************************************
 */

// ------------------------- 静态变量定义 -------------------------
static Motor_Init_Config_s ChassicMotorsConfig; // 底盘电机初始化配置
static MOTORInstance *motor[4];                 // 底盘四个电机实例指针：右前、左前、左后、右后
static AHRS_FEED *AHRSFeed;                     // IMU姿态数据指针（暂未直接使用）
static float motor_speedset[5];                 // 四个电机目标转速（RPM）
static float chassic_vx,chassic_vy;             // 机体坐标系速度分量（m/s）
static Chassic_Ctrl_Cmd chassic_cmd_recv;       // 底盘控制命令缓存
static EcoderInstance *ecoderdfeed;             // 编码器实例（用于云台偏角）
extern uint8_t cmd_mode;                        // 控制模式：0-遥控器，1-键鼠

// ------------------------- 静态函数声明 -------------------------
static void OMNICALCULATE(void);
static void CalcOffsetAngle(void);
static void ChassisOutput(void);

// ------------------------- 底盘初始化 -------------------------
/**
 * @brief  底盘初始化
 * @param  None
 * @retval None
 *
 * 配置内容：
 * - 四个M3508电机：速度环为最外层，闭环包含速度+电流
 * - 速度/电流环PID参数
 * - 注册电机到CAN并保存实例
 * - 初始化IMU与编码器数据指针
 */
void ChassisInit(void)
{
    // 1) 底盘电机控制设置
    ChassicMotorsConfig.motor_type = M3508;
    ChassicMotorsConfig.contorller_setting_init_config.outer_loop_type = SPEED_LOOP;
    ChassicMotorsConfig.contorller_setting_init_config.close_loop_type = SPEED_LOOP | ANGLE_LOOP;
    ChassicMotorsConfig.contorller_setting_init_config.motor_reverse_flag = MOTOR_DIRECTION_NORMAL;
    ChassicMotorsConfig.contorller_setting_init_config.feedback_reserve_flag = FEEDBACK_DIRCTION_NORMAL;
    ChassicMotorsConfig.contorller_setting_init_config.Feedfoward_flag = FEEDFORWARD_NONE;
    ChassicMotorsConfig.contorller_setting_init_config.angle_feedback_source = MOTOR_FEED;
    ChassicMotorsConfig.contorller_setting_init_config.speed_feedback_source = MOTOR_FEED;

    // 速度环PID参数（针对M3508底盘电机调参结果）
    ChassicMotorsConfig.contorller_param_init_config.speed_PID = (PID_Init_Config_s)
    {
        .Kp = 1.80f,
        .Ki = 0.005f,
        .Kd = 0.0f,
        .KF = 0.0f,
        .MaxOut = 15000.0f,
        .DeadBand = 200.0f,
        .IntegralLimit = 3000.0f,
        .Improve = PID_Integral_limit,
        .Output_LPF_RC = 2.0f,
        .Derivative_LPF_RC = 0.0f,
        .CoefA = 0.0f,
        .CoefB = 0.0f
    };

    // 电流环PID参数：抑制过流，提高响应
    ChassicMotorsConfig.contorller_param_init_config.current_PID = (PID_Init_Config_s)
    {
        .Kp = 1.00f,
        .Ki = 0.00f,
        .Kd = 0.01f,
        .KF = 0.0f,
        .MaxOut = 5000.0f,
        .DeadBand = 200.0f,
        .IntegralLimit = 0.0f,
        .Improve = PID_Integral_limit,
        .Output_LPF_RC = 2.0f,
        .Derivative_LPF_RC = 0.0f,
        .CoefA = 0.0f,
        .CoefB = 0.0f
    };

    // 2) 注册四个电机：CAN ID = 1~4
    for(int i = 1; i <= 4; i++)
    {
        ChassicMotorsConfig.can_init_config.tx_id = i;
        ChassicMotorsConfig.contorller_setting_init_config.motor_reverse_flag = MOTOR_DIRECTION_RESERVE; // 根据实车安装确定正反转
        motor[i - 1] = MotorInit(&ChassicMotorsConfig);
    }

    // 3) 绑定姿态与编码器数据
    AHRSFeed = GetAHRSFeed();         // 获取IMU数据（用于扩展云台补偿）
    ecoderdfeed = EcoderInit();       // 初始化编码器，获取云台相对底盘偏角
}


/**
 * @brief  麦轮运动学解算（底盘坐标系）
 * @param  None
 * @retval None
 *
 * 输入：
 *  - chassic_vx, chassic_vy：机体坐标系线速度（m/s）
 *  - chassic_cmd_recv.wz：机体角速度（rad/s 对应的等效值）
 * 输出：motor_speedset[0..3]（单位：RPM）
 *
 * 公式：以机体中心为坐标原点，采用标准麦克纳姆轮运动学模型
 */
static void OMNICALCULATE(void)
{
    const float coefficient = -(RAD_PS_2_RPM) * (1.0f / radius);  // 通用转换系数

    motor_speedset[0] = coefficient * (-chassic_vx + chassic_vy + chassic_cmd_recv.wz); // 右前轮
    motor_speedset[1] = coefficient * ( chassic_vx + chassic_vy + chassic_cmd_recv.wz); // 左前轮
    motor_speedset[2] = coefficient * ( chassic_vx - chassic_vy + chassic_cmd_recv.wz); // 左后轮
    motor_speedset[3] = coefficient * (-chassic_vx - chassic_vy + chassic_cmd_recv.wz); // 右后轮
}


/**
 * @brief  计算底盘相对云台的偏角
 * @param  None
 * @retval None
 *
 * 编码器输出：
 *  - single_angle：将编码值转换成角度（0~360）
 *  - single_ecode：原始编码值，可用于判断跨零
 */
static void CalcOffsetAngle(void)
{
    static float angle;
    static float ecode;

    angle = *(ecoderdfeed->single_angle);
    ecode = *(ecoderdfeed->single_ecode);

    if(ecode >= 16383)  // 接近满量程，可能跨零
    {
        if(angle > CHASSISOFFSET && angle < CHASSISOFFSET + 180.0f)
        {
            chassic_cmd_recv.offset_angle = angle - CHASSISOFFSET;
        }
        else if(angle > chassic_cmd_recv.offset_angle + 180.0f)
        {
            chassic_cmd_recv.offset_angle = angle - CHASSISOFFSET - 360.0f;
        }
        else
        {
            chassic_cmd_recv.offset_angle = angle - CHASSISOFFSET;
        }
    }
    else
    {
        if (angle > CHASSISOFFSET)
        {
            chassic_cmd_recv.offset_angle = angle - CHASSISOFFSET;
        }
        else if (angle <= CHASSISOFFSET && angle >= CHASSISOFFSET - 180.f)
        {
            chassic_cmd_recv.offset_angle = angle - CHASSISOFFSET;
        }
        else 
        {
            chassic_cmd_recv.offset_angle = angle - CHASSISOFFSET + 360.f;
        }
    }
}

/**
 * @brief  将电机参考值写入控制器
 */
static void ChassisOutput(void)
{
    MotorSetRef(motor[0], motor_speedset[0]);
    MotorSetRef(motor[1], motor_speedset[1]);
    MotorSetRef(motor[2], motor_speedset[2]);
    MotorSetRef(motor[3], motor_speedset[3]);
}




/**
 * @brief  底盘任务主体：每个控制周期调用
 */
void Chassistask(void)
{
    // 1) 根据模式决定是否输出零力
    if(chassic_cmd_recv.Chassis_Mode == CHASSIS_ZERO_FORCE)
    {
        for(int i = 0;i < 4;i++)
        {
            MotorStop(motor[i]);
        }
    }
    else
    {
        for(int i = 0;i < 4;i++)
        {
            MotorEnable(motor[i]);
        }
    }

    // 2) 遥控器模式：根据底盘模式进行角速度补偿
    if(cmd_mode == 0)
    {
        switch(chassic_cmd_recv.Chassis_Mode)
        {
        case CHASSIS_NO_FOLLOW:
            chassic_cmd_recv.wz = 0;  // 云台不跟随 -> 底盘角速度为0
            break;

        case CHASSIS_FOLLOW_GIMBLE_YAW:
        {
            // 简单平方律调节偏角，逼近云台方向
            chassic_cmd_recv.wz = 0.045f * (chassic_cmd_recv.offset_angle + 53) * abs(chassic_cmd_recv.offset_angle + 53);
            chassic_cmd_recv.wz = fminf(269.0f, fmaxf(-269.0f, chassic_cmd_recv.wz)); // 限幅
        }
        break;

        case CHASSIS_ROTATE:
            chassic_cmd_recv.wz = 239.0f; // 小陀螺固定角速度
            break;

        default:
            break;
        }
    }

    // 3) 更新云台偏角
    CalcOffsetAngle();

    // 4) 坐标转换：云台系 -> 机体系
    static float sin_theta, cos_theta;
    cos_theta = (float)cos(chassic_cmd_recv.offset_angle * DEGREE_2_RAD);
    sin_theta = (float)sin(chassic_cmd_recv.offset_angle * DEGREE_2_RAD);
    chassic_vx = (chassic_cmd_recv.vx * cos_theta - chassic_cmd_recv.vy * sin_theta);
    chassic_vy = (chassic_cmd_recv.vx * sin_theta + chassic_cmd_recv.vy * cos_theta);

    // 5) 运动学解算 + 写入电机参考值
    OMNICALCULATE();
    ChassisOutput();

    // 6) 调用底层电机控制（PID计算 + CAN发送）
    MotorControl();
}

/**
 * @brief  获取底盘命令结构体指针
 * @return Chassic_Ctrl_Cmd* ：用于外部模块写入底盘控制量
 */
Chassic_Ctrl_Cmd *GetChassisCmd(void)
{
    return &chassic_cmd_recv;
}




