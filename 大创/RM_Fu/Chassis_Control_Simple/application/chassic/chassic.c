#include "chassic.h"
#include "math.h"

/*
 *****************************************************************************************
 * 麦克纳姆底盘控制（chassic.c）- 精简版
 *
 * 功能概览：
 * 1. 初始化四个M3508底盘电机，配置速度环+电流环控制策略
 * 2. 读取遥控器指令，执行麦轮运动学解算，得到四个轮子的目标转速
 * 3. 将目标转速写入电机控制器，在MotorControl()中统一进行PID计算与CAN下发
 *
 * 数据流：
 * ControlTask -> 写入 chassic_cmd_recv -> Chassistask -> MotorControl -> CAN 下发
 *
 * 注意事项：
 * - 本文件仅负责控制逻辑，底层CAN发送在 motor.c 中统一处理
 * - 精简版不支持云台跟随模式（已去除编码器和IMU相关代码）
 * - 支持零力模式、不跟随模式、旋转模式
 *****************************************************************************************
 */

// ------------------------- 静态变量定义 -------------------------
static Motor_Init_Config_s ChassicMotorsConfig; // 底盘电机初始化配置
static MOTORInstance *motor[4];                 // 底盘四个电机实例指针：右前、左前、左后、右后
static float motor_speedset[4];                 // 四个电机目标转速（RPM）
static float chassic_vx, chassic_vy;            // 机体坐标系速度分量（m/s）
static Chassic_Ctrl_Cmd chassic_cmd_recv;       // 底盘控制命令缓存

// ------------------------- 静态函数声明 -------------------------
static void OMNICALCULATE(void);
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
 */
void ChassisInit(void)
{
    // 1) 底盘电机控制设置
    ChassicMotorsConfig.motor_type = M3508;
    ChassicMotorsConfig.contorller_setting_init_config.outer_loop_type = SPEED_LOOP;
    ChassicMotorsConfig.contorller_setting_init_config.close_loop_type = SPEED_AND_CURRENT_LOOP;
    ChassicMotorsConfig.contorller_setting_init_config.motor_reverse_flag = MOTOR_DIRECTION_NORMAL;
    ChassicMotorsConfig.contorller_setting_init_config.feedback_reserve_flag = FEEDBACK_DIRCTION_NORMAL;
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
    // 注意：根据实际硬件安装，可能需要调整 motor_reverse_flag
    for(int i = 1; i <= 4; i++)
    {
        ChassicMotorsConfig.can_init_config.tx_id = i;
        // 根据实际安装方向确定正反转标志
        // 如果电机反转，设置为 MOTOR_DIRECTION_RESERVE
        ChassicMotorsConfig.contorller_setting_init_config.motor_reverse_flag = MOTOR_DIRECTION_RESERVE;
        motor[i - 1] = MotorInit(&ChassicMotorsConfig);
    }
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
 * 轮子顺序：0-右前，1-左前，2-左后，3-右后
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
 * @brief  将电机参考值写入控制器
 * @param  None
 * @retval None
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
 * @param  None
 * @retval None
 *
 * 功能：
 * 1. 根据模式决定是否输出零力
 * 2. 根据底盘模式进行角速度设置
 * 3. 直接使用遥控器指令的速度值（无需坐标转换）
 * 4. 执行运动学解算
 * 5. 将目标速度写入电机控制器
 * 6. 调用底层电机控制（PID计算 + CAN发送）
 */
void Chassistask(void)
{
    // 1) 根据模式决定是否输出零力
    if(chassic_cmd_recv.Chassis_Mode == CHASSIS_ZERO_FORCE)
    {
        for(int i = 0; i < 4; i++)
        {
            MotorStop(motor[i]);
        }
        return;  // 零力模式直接返回，不执行后续控制
    }
    else
    {
        for(int i = 0; i < 4; i++)
        {
            MotorEnable(motor[i]);
        }
    }

    // 2) 根据底盘模式进行角速度设置（精简版，不支持云台跟随）
    switch(chassic_cmd_recv.Chassis_Mode)
    {
    case CHASSIS_NO_FOLLOW:
        chassic_cmd_recv.wz = 0;  // 不跟随模式 -> 底盘角速度为0
        break;

    case CHASSIS_ROTATE:
        chassic_cmd_recv.wz = 239.0f; // 旋转模式：固定角速度（小陀螺）
        break;

    case CHASSIS_FOLLOW_GIMBLE_YAW:
        // 精简版不支持云台跟随模式，将其视为不跟随模式
        chassic_cmd_recv.wz = 0;
        break;

    default:
        chassic_cmd_recv.wz = 0;
        break;
    }

    // 3) 直接使用遥控器指令的速度值（精简版，无需坐标转换）
    chassic_vx = chassic_cmd_recv.vx;
    chassic_vy = chassic_cmd_recv.vy;

    // 4) 运动学解算 + 写入电机参考值
    OMNICALCULATE();
    ChassisOutput();

    // 5) 调用底层电机控制（PID计算 + CAN发送）
    MotorControl();
}

/**
 * @brief  获取底盘命令结构体指针
 * @return Chassic_Ctrl_Cmd*：用于外部模块写入底盘控制量
 */
Chassic_Ctrl_Cmd *GetChassisCmd(void)
{
    return &chassic_cmd_recv;
}

