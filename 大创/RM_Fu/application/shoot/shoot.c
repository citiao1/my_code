#include "shoot.h"

/*
 *****************************************************************************************
 * 射击控制模块（shoot.c）
 *
 * 组件说明：
 * - Rammermotor：拨弹盘电机（M2006）
 * - Frictiongear1/2motor：左/右摩擦轮电机（M3508）
 *
 * 功能：
 * 1. 初始化拨弹盘与摩擦轮电机，配置速度+电流双环控制
 * 2. 根据不同射击模式，设置摩擦轮/拨盘目标速度
 * 3. 提供反转（疏弹）逻辑，避免卡弹
 *****************************************************************************************
 */

// ------------------------- 静态变量定义 -------------------------
static MOTORInstance *Rammermotor;         // 拨弹盘电机
static MOTORInstance *Frictiongear1motor;  // 左摩擦轮电机
static MOTORInstance *Frictiongear2motor;  // 右摩擦轮电机

static Shoot_Ctrl_cmd shoot_cmd;           // 射击控制命令（由ControlTask写入）
static float motor_Rammerset;              // 拨盘目标速度
static float motor_Frictionset1;           // 左摩擦轮目标速度
static float motor_Frictionset2;           // 右摩擦轮目标速度

/**
 * @brief  射击模块初始化
 * - 拨弹盘：M2006，速度+电流双环
 * - 摩擦轮：两颗M3508，速度+电流双环，其中一颗反向
 */
void ShootInit(void)
{
    Motor_Init_Config_s motor_Rammerset_init = {
        .motor_type = M2006,
        .contorller_setting_init_config = {
            .close_loop_type = SPEED_LOOP | ANGLE_LOOP,
            .outer_loop_type = SPEED_LOOP,
            .speed_feedback_source = MOTOR_FEED,
        },
        .contorller_param_init_config = {
            .speed_PID = {
                .Kp = 1.15f,
                .Ki = 0.01f,
                .Kd = 100.0f,
                .MaxOut = 50000.0f,
                .DeadBand = 10.0f,
                .IntegralLimit = 2880.0f,
                .Output_LPF_RC = 0.1f,
                .Improve = PID_Integral_limit,
            },
            .current_PID = {
                .Kp = 2.3f,
                .Ki = 0.005f,
                .Kd = 140.0f,
                .MaxOut = 80000.0f,
                .DeadBand = 10.0f,
                .IntegralLimit = 2000.0f,
                .Output_LPF_RC = 0.2f,
                .Improve = PID_Integral_limit,
            },
        },
        .can_init_config.tx_id = 7,
    };

    Motor_Init_Config_s motor_Frictionset1_init = {
        .motor_type = M3508,
        .contorller_setting_init_config = {
            .close_loop_type = SPEED_LOOP | ANGLE_LOOP,
            .outer_loop_type = SPEED_LOOP,
            .speed_feedback_source = MOTOR_FEED,
        },
        .contorller_param_init_config = {
            .speed_PID = {
                .Kp = 1.0f,
                .Ki = 0.0f,
                .Kd = 0.0f,
                .MaxOut = 30000.0f,
                .DeadBand = 10.0f,
                .IntegralLimit = 1500.0f,
                .Output_LPF_RC = 3.0f,
                .Improve = PID_Integral_limit,
            },
            .current_PID = {
                .Kp = 2.0f,
                .Ki = 0.005f,
                .Kd = 120.0f,
                .MaxOut = 30000.0f,
                .DeadBand = 10.0f,
                .IntegralLimit = 800.0f,
                .Output_LPF_RC = 3.0f,
                .Improve = PID_Integral_limit,
            },
        },
        .can_init_config.tx_id = 5,
    };

    Motor_Init_Config_s motor_Frictionset2_init = {
        .motor_type = M3508,
        .contorller_setting_init_config = {
            .close_loop_type = SPEED_LOOP | ANGLE_LOOP,
            .outer_loop_type = SPEED_LOOP,
            .speed_feedback_source = MOTOR_FEED,
            .motor_reverse_flag = MOTOR_DIRECTION_RESERVE,
        },
        .contorller_param_init_config = {
            .speed_PID = {
                .Kp = 1.0f,
                .Ki = 0.0f,
                .Kd = 0.0f,
                .MaxOut = 30000.0f,
                .DeadBand = 10.0f,
                .IntegralLimit = 5500.0f,
                .Output_LPF_RC = 3.0f,
                .Improve = PID_Integral_limit,
            },
            .current_PID = {
                .Kp = 2.0f,
                .Ki = 0.0f,
                .Kd = 0.0f,
                .MaxOut = 30000.0f,
                .DeadBand = 10.0f,
                .IntegralLimit = 5500.0f,
                .Output_LPF_RC = 3.0f,
                .Improve = PID_Integral_limit,
            },
        },
        .can_init_config.tx_id = 6,
    };

    Rammermotor = MotorInit(&motor_Rammerset_init);
    Frictiongear1motor = MotorInit(&motor_Frictionset1_init);
    Frictiongear2motor = MotorInit(&motor_Frictionset2_init);
}

/**
 * @brief  写入当前射击电机参考值
 */
static void ShootOutPut(void)
{
    MotorSetRef(Rammermotor, motor_Rammerset);
    MotorSetRef(Frictiongear1motor, motor_Frictionset1);
    MotorSetRef(Frictiongear2motor, motor_Frictionset2);
}

/**
 * @brief  拨弹盘反转逻辑（疏弹）
 * 当拨盘PID输出超过阈值时，短时间反转退出
 */
static void ShootReserve(void)
{
    static uint8_t sreserve1 = 0;
    static uint8_t stickreserver = 0;
    static uint64_t srtick = 0;

    if(Rammermotor->motor_contorller.speed_PID.Output >= 75000.0f && sreserve1 == 0)
    {
        sreserve1 = 1;
    }
    if(sreserve1 == 1 && stickreserver == 0)
    {
        srtick = uwTick;
        stickreserver = 1;
    }

    if(sreserve1 == 1 && uwTick - srtick < 500)
    {
        motor_Rammerset = -1500.0f;
    }

    if(sreserve1 == 1 && uwTick - srtick >= 500)
    {
        sreserve1 = 0;
        stickreserver = 0;
    }
}




/**
 * @brief  射击任务
 */
void ShootTask(void)
{
    MotorEnable(Rammermotor);
    MotorEnable(Frictiongear1motor);
    MotorEnable(Frictiongear2motor);

    switch (shoot_cmd.shoot_mode)
    {
    case MOVENONE:
        motor_Rammerset = 0.0f;
        motor_Frictionset1 = 0.0f;
        motor_Frictionset2 = 0.0f;
        break;

    case MOVEHEAD:  // 只启动摩擦轮
        motor_Frictionset1 = 50000.0f;
        motor_Frictionset2 = 50000.0f;
        motor_Rammerset = 0.0f;
        break;

    case MOVEALL:   // 摩擦轮+拨盘
        motor_Frictionset1 = 50000.0f;
        motor_Frictionset2 = 50000.0f;
        motor_Rammerset = 10500.0f;
        break;

    case MOVEMOUSE: // 键鼠模式：摩擦轮恒速，拨盘缓升
        motor_Frictionset1 = 50000.0f;
        motor_Frictionset2 = 50000.0f;
        motor_Rammerset += 65.0f;
        if(motor_Rammerset >= 10500.0f)
        {
            motor_Rammerset = 10500.0f;
        }
        break;

    case MOVERESERVE: // 反转疏弹
        motor_Rammerset = -2300.0f;
        ShootReserve();
        break;

    default:
        break;
    }

    ShootOutPut();
    MotorControl();
}






/**
 * @brief  获取射击控制命令结构体
 */
Shoot_Ctrl_cmd *GetShootCmd(void)
{
    return &shoot_cmd;
}
