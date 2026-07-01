#include "motor.h"

// 电机控制实现：
// - 统一封装 GM6020/M3508/M2006 三类电机
// - 支持角度/速度/电流多环嵌套控制
// - CAN 报文按电机类型分组批量发送

static uint8_t idx = 0;                 // 已注册电机数量
MOTORInstance *motor_instance[10];      // 电机实例表（最多 10 个）

// 发送 CAN 分组（1FF/200/2FF）：不同类型/编号映射到不同报文与位号
static CANInstance sender_assignment[3] =
    {
        [0] = {.tx_config.StdId = 0x1ff, .tx_config.DLC = 0x08, .tx_config.IDE = CAN_ID_STD, .tx_config.RTR = CAN_RTR_DATA, .txbuff = {0}},
        [1] = {.tx_config.StdId = 0x200, .tx_config.DLC = 0x08, .tx_config.IDE = CAN_ID_STD, .tx_config.RTR = CAN_RTR_DATA, .txbuff = {0}},
        [2] = {.tx_config.StdId = 0x2ff, .tx_config.DLC = 0x08, .tx_config.IDE = CAN_ID_STD, .tx_config.RTR = CAN_RTR_DATA, .txbuff = {0}},
    };

static uint8_t send_enable_flag[6] = {0};

/**
 * @brief  电机CAN分组设置函数
 * @param  motor: 电机实例指针
 * @param  config: CAN初始化配置结构体指针（会被修改）
 * @retval None
 * 
 * 功能说明：
 * 根据电机类型和ID，将电机分配到不同的CAN发送组
 * 
 * CAN发送组分配规则：
 * 1. M3508/M2006电机：
 *    - ID 1-4：组1 (0x200)，位号0-3
 *    - ID 5-8：组0 (0x1FF)，位号0-3
 *    - 接收ID：0x201-0x208
 * 
 * 2. GM6020电机：
 *    - ID 1-4：组0 (0x1FF)，位号0-3
 *    - ID 5-8：组2 (0x2FF)，位号0-3
 *    - 接收ID：0x205-0x208
 * 
 * 设计原因：
 * - 将多个电机的控制数据打包在一个CAN报文中发送，减少总线负载
 * - 每个CAN报文最多8字节，可以包含4个电机的控制数据（每个2字节）
 * - 通过分组发送，可以将8个电机的控制数据压缩到2-3个CAN报文中
 */
static void MotorSendgroup(MOTORInstance *motor, CAN_Init_Config_s *config)
{
    uint8_t motor_id = config->tx_id - 1;  // 电机ID转换为索引（0-7）
    uint8_t motor_send_num;                 // 电机在CAN报文中的位置（0-3）
    uint8_t motor_grouping;                 // CAN发送组号（0-2）

    switch (motor->motor_type)
    {
    case M2006:  // M2006和M3508使用相同的分组规则
    case M3508:
        {
            // M3508/M2006电机分组
            if(motor_id < 4)  // ID 1-4：分配到组1
            {
                motor_send_num = motor_id;      // 位号：0-3
                motor_grouping = 1;             // 组号：1（发送ID 0x200）
            }
            else  // ID 5-8：分配到组0
            {
                motor_send_num = motor_id - 4;  // 位号：0-3（相对于组内）
                motor_grouping = 0;             // 组号：0（发送ID 0x1FF）
            }

            // 设置接收ID：M3508/M2006的接收ID为0x201-0x208
            config->rx_id = 0x200 + motor_id + 1;
            
            // 标记该组有电机，需要发送
            send_enable_flag[motor_grouping] = 1;
            
            // 保存分组信息到电机实例
            motor->senter_group = motor_grouping;  // 保存组号
            motor->message_num = motor_send_num;   // 保存位号
        }
        break;
    case GM6020:  // GM6020电机分组
        {
            // GM6020电机分组
            if(motor_id < 4)  // ID 1-4：分配到组0
            {
                motor_send_num = motor_id;      // 位号：0-3
                motor_grouping = 0;             // 组号：0（发送ID 0x1FF）
            }
            else  // ID 5-8：分配到组2
            {
                motor_send_num = motor_id - 4;  // 位号：0-3（相对于组内）
                motor_grouping = 2;             // 组号：2（发送ID 0x2FF）
            }

            // 设置接收ID：GM6020的接收ID为0x205-0x208
            config->rx_id = 0x204 + motor_id + 1;
            
            // 保存分组信息到电机实例
            motor->senter_group = motor_grouping;  // 保存组号
            motor->message_num = motor_send_num;   // 保存位号
            
            // 标记该组有电机，需要发送
            send_enable_flag[motor_grouping] = 1;
        }
        break;
    default :
        break;  // 其他类型电机暂不支持
    } 
}



/**
 * @brief  电机反馈报文解析函数（CAN接收回调）
 * @param  _instance: CAN实例指针
 * @retval None
 * 
 * 功能说明：
 * 当CAN接收到电机反馈数据时，自动调用此函数
 * 解析电机反馈的CAN报文，提取角度、速度、电流、温度等信息
 * 
 * CAN报文格式（8字节）：
 * - 字节0-1：编码器值（ECD，0-8191，16位）
 * - 字节2-3：速度（RPM，有符号16位）
 * - 字节4-5：实际电流（mA，有符号16位）
 * - 字节6：温度（℃）
 * - 字节7：保留
 * 
 * 处理内容：
 * 1. 解析编码器值（ECD）：转换为单圈角度
 * 2. 解析速度：转换为角度/秒，并进行平滑滤波
 * 3. 解析电流：进行平滑滤波
 * 4. 解析温度
 * 5. 计算多圈累计角度（处理编码器溢出）
 * 
 * 平滑滤波：
 * - 速度滤波系数：0.85（一阶低通滤波）
 * - 电流滤波系数：0.90（一阶低通滤波）
 * - 目的是减少噪声，提高数据稳定性
 */
static void DecodeMotor(CANInstance *_instance)
{
    uint8_t *rx_buff = _instance->rxbuff;              // 获取接收缓冲区
    MOTORInstance *motor = (MOTORInstance *)_instance->id;  // 获取电机实例指针
    motor_measure_s *measure = &motor->measure;        // 获取测量数据指针

    // 计算时间差（用于速度计算，当前未使用）
    //motor->dt = TickCount(&motor->feed_cnt);

    // 解析编码器值（ECD）：0-8191，对应0-360度
    measure->last_ecd = measure->ecd;                  // 保存上次编码值
    measure->ecd = ((uint16_t)rx_buff[0]) << 8 | rx_buff[1];  // 读取当前编码值（16位，大端序）
    measure->angle_single_round = ECD_ANGLE_COEF*(float)measure->ecd;  // 转换为单圈角度（度）
    
    // 解析速度：RPM转换为度/秒，并进行平滑滤波
    measure->lastrspead = measure->rspead;             // 保存上次速度
    // 速度解析：读取16位有符号数（大端序），转换为RPM，再转换为度/秒
    // 平滑滤波：新速度 = 旧速度×0.15 + 新速度×0.85（一阶低通滤波）
    measure->rspead = (1.0f - SPEED_SMOOTH_COEF) * measure->rspead +
        RPM_2_ANGLE_PER_SEC * SPEED_SMOOTH_COEF * (float)((int16_t)(rx_buff[2] << 8 | rx_buff[3]));
    
    // 解析电流：读取16位有符号数（大端序），并进行平滑滤波
    // 平滑滤波：新电流 = 旧电流×0.10 + 新电流×0.90（一阶低通滤波）
    measure->real_current = (1.0f - CURREANT_SMOOTH_COEF)*measure->real_current + 
        CURREANT_SMOOTH_COEF*(float)((int16_t)(rx_buff[4] << 8 | rx_buff[5]));
    
    // 解析温度：读取温度值（℃）
    measure->temperature = rx_buff[6];

    // 多圈角度计算：处理编码器溢出（0和8191之间的跳变）
    // 编码器值范围：0-8191（对应0-360度）
    // 当编码器值从8191跳到0时，说明转了一圈，total_round++
    // 当编码器值从0跳到8191时，说明反向转了一圈，total_round--
    // 阈值4096用于判断是否发生溢出（而不是正常的角度变化）
    if(measure->ecd - measure->last_ecd > 4096)  // 从高值跳到低值（反向转一圈）
    {
        measure->total_round --;  // 圈数减1
    }
    if(measure->ecd - measure->last_ecd < -4096)  // 从低值跳到高值（正向转一圈）
    {
        measure->total_round ++;  // 圈数加1
    }
    // 计算累计角度：圈数×360 + 单圈角度
    measure->total_angle = measure->total_round*360 + measure->angle_single_round;
}

/**
 * @brief  使能电机（允许输出）
 * @param  motor: 电机实例指针
 * @retval None
 * 
 * 功能说明：
 * 允许电机输出，将stop_flag设置为MOTOR_ENABLE
 * 在MotorControl()中，只有当stop_flag为MOTOR_ENABLE时，电机才会输出控制值
 */
void MotorEnable(MOTORInstance *motor)
{
    motor->stop_flag = MOTOR_ENABLE;  // 设置使能标志
}



/**
 * @brief  电机初始化函数
 * @param  config: 电机初始化配置结构体指针
 * @retval MOTORInstance*: 返回初始化后的电机实例指针
 * 
 * 功能说明：
 * 1. 分配电机实例内存
 * 2. 初始化电机类型和控制设置
 * 3. 初始化三个PID控制器（角度、速度、电流）
 * 4. 设置外部反馈指针
 * 5. 配置电机CAN分组（发送组和接收ID）
 * 6. 注册CAN回调函数（用于接收电机反馈数据）
 * 7. 默认使能电机
 * 8. 将电机实例加入全局电机表
 */
MOTORInstance *MotorInit(Motor_Init_Config_s *config)
{
    // 动态分配电机实例内存
    MOTORInstance *instance = (MOTORInstance *)malloc(sizeof(MOTORInstance));
    memset(instance, 0, sizeof(MOTORInstance));

    // 电机基本设置：设置电机类型和控制配置
    instance->motor_type = config->motor_type;                               // 电机类型（GM6020/M3508/M2006）
    instance->motor_settings = config->contorller_setting_init_config;       // 复制控制设置（闭环类型、反馈来源等）

    // 电机控制器初始化：初始化三个PID控制器
    PID_Init(&instance->motor_contorller.angle_PID, &config->contorller_param_init_config.angle_PID);    // 角度环PID
    PID_Init(&instance->motor_contorller.current_PID, &config->contorller_param_init_config.current_PID); // 电流环PID
    PID_Init(&instance->motor_contorller.speed_PID, &config->contorller_param_init_config.speed_PID);     // 速度环PID

    // 电机分组：根据电机类型和ID配置CAN发送组和接收ID
    MotorSendgroup(instance, &config->can_init_config);

    // 注册电机到CAN总线：设置CAN回调函数和数据指针
    config->can_init_config.can_module_callback = DecodeMotor;  // 设置接收回调函数（解析电机反馈数据）
    config->can_init_config.id = instance;                      // 设置用户数据指针（指向电机实例）
    instance->motor_can_instance = CANRegister(&config->can_init_config);  // 注册CAN实例

    MotorEnable(instance);           // 默认使能电机
    motor_instance[idx ++] = instance;  // 将电机实例加入全局表
    return instance;                 // 返回电机实例指针
}


/**
 * @brief  设置电机控制参考值
 * @param  motor: 电机实例指针
 * @param  ref: 参考值（单位取决于最外层控制环）
 * @retval None
 * 
 * 功能说明：
 * 设置电机的目标值，具体含义取决于电机的最外层控制环：
 * - ANGLE_LOOP: ref为角度（度）
 * - SPEED_LOOP: ref为速度（度/秒 或 RPM）
 * - CURRENT_LOOP: ref为电流（mA）
 * 
 * 注意：该函数只是设置参考值，实际控制由MotorControl()完成
 */
void MotorSetRef(MOTORInstance *motor, float ref)
{
    motor->motor_contorller.pid_ref = ref;  // 设置PID参考值
}

/**
 * @brief  修改电机的最外层控制环
 * @param  motor: 电机实例指针
 * @param  outer_loop: 最外层控制环类型
 * @retval None
 * 
 * 功能说明：
 * 动态切换电机的最外层控制环，例如：
 * - 从速度环切换到角度环
 * - 从角度环切换到速度环
 * 
 * 注意：只有配置了相应的闭环类型，才能切换到该控制环
 */
void MotorOuterLoop(MOTORInstance *motor, Closeloop_Type_e outer_loop)
{
    motor->motor_settings.outer_loop_type = outer_loop;  // 设置最外层控制环
}

/**
 * @brief  关闭电机（停止输出）
 * @param  motor: 电机实例指针
 * @retval None
 * 
 * 功能说明：
 * 停止电机输出，将stop_flag设置为MOTOR_STOP
 * 在MotorControl()中，如果stop_flag为MOTOR_STOP，输出会被强制为0
 */
void MotorStop(MOTORInstance *motor)
{
    motor->stop_flag = MOTOR_STOP;  // 设置停止标志
}


/**
 * @brief  电机控制主函数：为所有电机计算PID并发送CAN报文
 * @param  None
 * @retval None
 * 
 * 功能说明：
 * 这是电机控制的核心函数，在ChassisTask中周期性调用（5ms周期，200Hz）
 * 
 * 处理流程：
 * 1. 遍历所有已注册的电机
 * 2. 对每个电机：
 *    a. 保存上次参考值
 *    b. 处理电机反转标志（如果需要反转，参考值取反）
 *    c. 根据配置执行多环PID控制：
 *       - 角度环PID（最外层）
 *       - 速度环PID（中间层）
 *       - 电流环PID（最内层）
 *    d. 处理反馈反转标志
 *    e. 检查停止标志（如果停止，输出为0）
 *    f. 将输出值打包到CAN发送缓冲区
 * 3. 批量发送所有CAN报文（按组发送，减少总线负载）
 * 
 * 多环控制说明：
 * - 角度环输出 -> 速度环参考值
 * - 速度环输出 -> 电流环参考值
 * - 电流环输出 -> 最终电机电流值
 * 
 * CAN分组发送：
 * - 组0 (0x1FF): M3508 ID 1-4 或 GM6020 ID 1-4
 * - 组1 (0x200): M3508 ID 5-8
 * - 组2 (0x2FF): GM6020 ID 5-8
 * - 每组最多4个电机，每个电机2字节（高8位+低8位）
 */

 
void MotorControl()
{
    uint8_t group,num;         // CAN发送组号和电机在组内的位置
    int16_t set;               // 最终输出值（电流值，-32768~32767）
    MOTORInstance *motor;      // 当前处理的电机实例
    Motor_Controller_s *motor_controllers;  // 电机控制器指针
    Motor_Control_Setting_s *motor_settings;  // 电机设置指针
    motor_measure_s *measure;  // 电机测量数据指针
    float pid_measure = 0;     // PID测量值
    static float pid_ref;      // PID参考值（在多环控制中会逐步更新）

    // 遍历所有已注册的电机
    for(int i = 0; i < idx; i++)
    {
        // 获取当前电机的各个指针，方便后续使用
        motor = motor_instance[i];
        motor_controllers = &motor->motor_contorller;
        motor_settings = &motor->motor_settings;
        measure = &motor->measure;
        
        // 保存上次参考值
        motor_controllers->pid_lastref[0] = motor_controllers->pid_lastref[1];  // 上次的参考值
        pid_ref = motor_controllers->pid_ref;                                   // 当前参考值
        motor_controllers->pid_lastref[1] = pid_ref;                            // 保存当前参考值
        
        // 处理电机反转标志：如果电机需要反转，参考值取反
        if (motor_settings->motor_reverse_flag == MOTOR_DIRECTION_RESERVE)
        {
            pid_ref *= -1;  // 取反参考值
        }

        // 角度环PID控制：如果配置了角度环且为最外层控制环
        if(motor_settings->close_loop_type & ANGLE_LOOP && motor_settings->outer_loop_type == ANGLE_LOOP)
        {
            // 选择角度反馈来源：外部传感器或电机自身
            if(motor_settings->angle_feedback_source == OTHER_FEED)
            {
                pid_measure = *motor_controllers->other_angle_feedback_ptr;  // 外部反馈（如IMU）
            }
            else if(i == 4)  // 特殊处理：第5个电机使用total_angle
            {
                pid_measure = measure->total_angle;  // 使用累计角度
            }
            else if(i == 5)  // 特殊处理：第6个电机使用ecd
            {
                pid_measure = measure->ecd;          // 使用编码值
            }
            else
            {
                pid_measure = measure->total_angle;  // 默认使用累计角度
            }
            // 计算角度环PID，输出作为速度环的参考值
            pid_ref = PIDCalculate(&motor_controllers->angle_PID, pid_measure, pid_ref);
        }

        // 速度环PID控制：如果配置了速度环且为中间层或最外层控制环
        if(motor_settings->close_loop_type & SPEED_LOOP && motor_settings->outer_loop_type & (ANGLE_LOOP | SPEED_LOOP))
        {
            // 选择速度反馈来源：外部传感器或电机自身
            if(motor_settings->speed_feedback_source == OTHER_FEED)
            {
                pid_measure = *motor_controllers->other_speed_feedback_ptr;  // 外部反馈
            }
            else
            {
                pid_measure = measure->rspead;  // 电机自身速度反馈
            }

            // 计算速度环PID，输出作为电流环的参考值
            pid_ref = PIDCalculate(&motor_controllers->speed_PID, pid_measure, pid_ref);
        }
        
        // 电流环PID控制：如果配置了电流环
        if(motor_settings->close_loop_type & CURRENT_LOOP)
        {
            // 计算电流环PID，输出作为最终电机电流值
            pid_ref = PIDCalculate(&motor_controllers->current_PID, measure->real_current, pid_ref);
        }
        
        // 处理反馈反转标志：如果反馈需要反转，输出取反
        if(motor_settings->feedback_reserve_flag == FEEDBACK_DIRCTION_RESERVE)
        {
            pid_ref *= -1;  // 取反输出
        }
        
        // 检查停止标志：如果电机停止，输出强制为0
        if(motor->stop_flag == MOTOR_STOP)
        {
            pid_ref = 0;  // 输出为0
        }

        // 将浮点数输出转换为16位整数（电流值）
        set = (int16_t)pid_ref;
        
        // 将输出值打包到CAN发送缓冲区
        group = motor->senter_group;                                          // 获取CAN发送组号
        num = motor->message_num;                                             // 获取电机在组内的位置（0-3）
        sender_assignment[group].txbuff[2*num] = (uint8_t)(set >> 8);         // 高8位
        sender_assignment[group].txbuff[2*num + 1] = (uint8_t)(set & 0x00ff); // 低8位
    }

    // 批量发送各组CAN报文：只有该组有电机时才发送
    // 这样可以减少CAN总线负载和发送调用次数
    for(int i = 0;i < 3;i++)
    {
        if(send_enable_flag[i])  // 检查该组是否有电机
        {
            CANTransmit(&sender_assignment[i], 10);  // 发送CAN报文（超时10ms）
        }
    }
}
