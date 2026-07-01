#include "control.h"

/*
 *****************************************************************************************
 * 机器人控制模块（control.c）- 精简版
 *
 * 功能概览：
 * 1. 初始化底盘控制模块
 * 2. 读取遥控器数据，解析控制指令
 * 3. 根据遥控器开关切换底盘模式
 * 4. 将遥控器摇杆数据转换为底盘速度指令
 *
 * 数据流：
 * 遥控器SBUS -> DR16解析 -> RoboCmdTask -> 写入chassis_cmd -> Chassistask
 *
 * 注意事项：
 * - 精简版仅支持遥控器控制模式
 * - 不支持键鼠控制模式
 * - 不支持云台、射击、视觉模块
 *****************************************************************************************
 */

// 全局变量定义
RC_Ctl_t *rc_data;                  // 遥控器数据指针
Chassic_Ctrl_Cmd *chassis_cmd;      // 底盘控制命令指针
uint8_t cmd_mode = 0;               // 当前控制模式（0: 遥控器, 1: 键鼠，精简版固定为0）

/**
 * @brief  控制初始化函数
 * @param  None
 * @retval None
 *
 * 功能：
 * 1. 获取遥控器数据指针
 * 2. 获取底盘控制命令指针
 * 3. 初始化底盘模块
 */
void ControlInit(void)
{
    __disable_irq();                // 关闭中断，保证初始化过程不被中断
    rc_data = GetRCData();          // 获取遥控器数据指针
    chassis_cmd = GetChassisCmd();  // 获取底盘控制命令指针
    ChassisInit();                  // 初始化底盘模块（注册电机、配置PID等）
    __enable_irq();                 // 重新开启中断
}

/**
 * @brief  遥控器控制模式处理函数
 * @param  None
 * @retval None
 *
 * 功能：
 * 1. 根据s2开关设置底盘模式：
 *    - 上：不跟随模式（CHASSIS_NO_FOLLOW）
 *    - 中：不跟随模式（CHASSIS_NO_FOLLOW）
 *    - 下：旋转模式（CHASSIS_ROTATE）
 * 2. 将遥控器摇杆数据转换为底盘速度指令：
 *    - ch3：X方向速度（前进/后退）
 *    - ch2：Y方向速度（左移/右移）
 *    - ch0：预留（可用于角速度控制）
 *    - ch1：预留
 * 3. 摇杆数据范围：364~1684，中值为1024，转换为-250~+250
 */
static void RcControlSet(void)
{
    // 1) 根据s2开关设置底盘模式
    if(switch_is_down(rc_data->rc.s2))        // 开关下：旋转模式（小陀螺）
    {
        chassis_cmd->Chassis_Mode = CHASSIS_ROTATE;
    }
    else if(switch_is_mid(rc_data->rc.s2))    // 开关中：不跟随模式
    {
        chassis_cmd->Chassis_Mode = CHASSIS_NO_FOLLOW;
    }
    else if(switch_is_up(rc_data->rc.s2))     // 开关上：不跟随模式
    {
        chassis_cmd->Chassis_Mode = CHASSIS_NO_FOLLOW;
    }

    // 2) 将遥控器摇杆数据转换为底盘速度指令
    // ch3：X方向速度（前进为正，后退为负）
    // 摇杆范围：364~1684，中值1024，映射到-250~+250（单位：cm/s，转换为m/s需除以100）
    chassis_cmd->vx = ((float)(rc_data->rc.ch3 - 1024) * 250.0f / 660.0f) / 100.0f;  // 转换为m/s
    
    // ch2：Y方向速度（右移为正，左移为负）
    chassis_cmd->vy = ((float)(rc_data->rc.ch2 - 1024) * 250.0f / 660.0f) / 100.0f;  // 转换为m/s
    
    // ch0：预留（可用于角速度控制）
    // chassis_cmd->wz = ((float)(rc_data->rc.ch0 - 1024) * 1.0f / 660.0f);
    
    // 角速度由底盘模式决定（在Chassistask中设置）
    chassis_cmd->wz = 0;  // 默认角速度为0
}

/**
 * @brief  机器人控制任务（主控制逻辑）
 * @param  None
 * @retval None
 *
 * 功能：
 * 1. 精简版固定使用遥控器控制模式
 * 2. 调用遥控器控制处理函数
 * 3. 处理系统复位按键（B键）
 */
void RoboCmdTask(void)
{
    // 精简版固定使用遥控器控制模式
    cmd_mode = RCCONTROLMODE;
    
    // 调用遥控器控制处理函数
    RcControlSet();
    
    // 处理系统复位按键（B键）：按下B键后系统复位
    if (rc_data->key[0].b)
    {
        NVIC_SystemReset();
    }
}

