#include "contor.h"

// 控制层共享指针（均指向各模块的命令/数据结构体）
RC_Ctl_t *rc_data;
Chassic_Ctrl_Cmd *chassis_cmd;
Gimbal_Ctrl_Cmd *gimbal_rc_cmd;
Shoot_Ctrl_cmd *shoot_rc_cmd;
WatchingRecive_t *watch_cmd;
uint16_t g_ccount;
uint8_t cmd_mode = 0;                    // 当前控制模式（0: 遥控器, 1: 键鼠）
PIDInstance *pid;
uint8_t watchingcontorl;                 // 视觉自锁模式切换
static float temp;
uint8_t rotatemode;                      // 小陀螺模式，按键 C 切换
static uint8_t cflag = 0;
static uint64_t ctick = 0;               // C 键按下时间戳
static uint8_t c_count = 0;              // C 键短按计数（奇偶切换）
Jkey v;


    // 整车控制初始化：
    // - 抓取各数据/命令指针
    // - 初始化底盘/云台/射击/视觉模块
void ControlInit()
{
    __disable_irq();
    rc_data = GetRCData();
    chassis_cmd = GetChassisCmd();
    gimbal_rc_cmd = GetGimbalCmd();
    shoot_rc_cmd = GetShootCmd();
    watch_cmd = GetWatchingRev();
    ChassisInit();
    GimbalInit();
    ShootInit();
    WatchingInit();
    __enable_irq();
}

// 键鼠控制模式：
// - WASD 平滑叠加 vx/vy
// - 鼠标左右键控制射击模式
// - C 键切换小陀螺
// - 鼠标移动叠加 yaw/pitch
static void MouseControlSet()                                           //键鼠控制模式
{
    static float wvx;
    static float svx;
    static float avy;
    static float dvy;           

    

    if (rc_data[0].key[PASSING].a)
    {
        avy += -(float)rc_data[0].key[PASSING].a * 1.2;
        if (avy <= -150)
        {
            avy = -150;
        }
    }
    else
    {
        if(avy < 0)
        {    
            avy +=  5;
        }
        if(avy >= 0)
            avy = 0;
    }

    if (rc_data[0].key[PASSING].d)
    {
        dvy += (float)rc_data[0].key[PASSING].d * 1.2;
        if (dvy >= 150)
        {
            dvy = 150;
        }
    }
    else 
    {
        if (dvy > 0)
        {
            dvy -=   5;
        }
        if (dvy <= 0)
            dvy = 0;
    }
    if (rc_data[0].key[PASSING].s)
    {
        svx += -(float)rc_data[0].key[PASSING].s * 1.2;
        if (svx <= -150)
        {
            svx = -150;
        }
    }
    else 
    {
        if (svx < 0)
        {
            svx +=  5;
        }
        if (svx >= 0)
            svx = 0;
    }
    if (rc_data[0].key[PASSING].w)
    {
        wvx += (float)rc_data[0].key[PASSING].w * 1.2;
        if (wvx >= 150)
        {
            wvx = 150;
        }
    }
    else
    {
        if (wvx > 0)
        {
            wvx -=  5.0;
        }
        if (wvx <= 0)
            wvx = 0;
    }

    chassis_cmd->vy = avy + dvy;
    chassis_cmd->vx = wvx + svx;


    if (!rc_data[0].mouse.press_l)
    {
        shoot_rc_cmd->shoot_mode = MOVENONE;
    }
    if (rc_data[0].mouse.press_r && !rc_data[0].mouse.press_l)
    {
        shoot_rc_cmd->shoot_mode = MOVERESERVE;
    }
        if(rc_data[0].mouse.press_l)
    {
        shoot_rc_cmd->shoot_mode = MOVEMOUSE;
    }
    

        if (rc_data[0].key[0].b)
        {
            NVIC_SystemReset();
        }
    

             //键鼠云台底盘控制部分

    if (rc_data[0].key [0].c)
    {
        if (cflag == 0)
        {
            ctick = uwTick;
            cflag = 1;
        }
    }

    if (ctick - uwTick <= -10 && !rc_data[0].key[0].c && cflag == 1)
    {
        c_count += 1;
        cflag = 0;
    } // 实现遥控器键鼠切换(注意,键鼠操作要在选手端开启时执行)

    rotatemode = c_count % 2;
    gimbal_rc_cmd->rotatemode = rotatemode;
    switch (rotatemode)
    {
    case ROTATESTOP:
    {
        if(chassis_cmd->wz > 0)
        {
            chassis_cmd->wz -= 8;
        }
        if(chassis_cmd->wz <= 0)
        {
            chassis_cmd->wz = 0;
        }
    }
    break;
    case ROTATESTART:
    {
        chassis_cmd->wz += 6;
        if (chassis_cmd->wz >= 239)
        {
            chassis_cmd->wz = 239;
        }
    }
    break;

    default:
        break;
    
    }


        if (rc_data[0].mouse.x >= 5000)
        {
            rc_data[0].mouse.x = 5000;
        }
        if (rc_data[0].mouse.x <= -5000)
        {
            rc_data[0].mouse.x = -5000;
        }
        gimbal_rc_cmd->yaw_total_degree += rc_data[0].mouse.x / 25;
        
        gimbal_rc_cmd->pitch_change_degree = -(rc_data[0].mouse.y * 10);


}





// 遥控器控制模式：
// - s2 开关：底盘模式（跟随/不跟随/小陀螺）
// - 摇杆通道：
//    ch3 -> vx, ch2 -> vy, ch0 -> yaw 累计, ch1 -> pitch 改变量
// - s1 开关：射击/摩擦轮占位（待实现）
static void RcControlSet()                                          //遥控器控制模式
{
    if(switch_is_down(rc_data->rc.s2))                      //小陀螺模式
    {
        chassis_cmd->Chassis_Mode = CHASSIS_ROTATE;
        // 云台
    }
    else if(switch_is_mid(rc_data->rc.s2))                  //云台底盘分离模式
    {
        chassis_cmd->Chassis_Mode = CHASSIS_NO_FOLLOW;
        // 云台
    }
    else if(switch_is_up(rc_data->rc.s2))                   //云台底盘跟随模式
    {
        chassis_cmd->Chassis_Mode = CHASSIS_FOLLOW_GIMBLE_YAW;
        // 云台
    }
    if(switch_is_down(rc_data->rc.s1))
    {
        //拨弹盘摩擦轮均不动
    }
    else if(switch_is_mid(rc_data->rc.s1))
    {
        //摩擦轮动
    }
    else if(switch_is_up(rc_data->rc.s1))
    {
        //拨弹盘摩擦轮均动
    }
        chassis_cmd->vx = ((float)(rc_data->rc.ch3 - 1024) * 250.0f/660.0f);
        chassis_cmd->vy = ((float)(rc_data->rc.ch2 - 1024) * 250.0f/660.0f);
        gimbal_rc_cmd->yaw_total_degree += ((rc_data->rc.ch0 - 1024) * 1.0f/660.0f);
        gimbal_rc_cmd->pitch_change_degree = ((rc_data->rc.ch1 -1024) * 800.0f/660.0f);
        temp = gimbal_rc_cmd->yaw_total_degree;

        if (gimbal_rc_cmd->pitch_total_degree >= 18)
        {
            gimbal_rc_cmd->pitch_total_degree = 18;
        }
    else if (gimbal_rc_cmd->pitch_total_degree <= -20)
    {
        gimbal_rc_cmd->pitch_total_degree = -20;
    }
    shoot_rc_cmd->shoot_mode = rc_data->rc.s1;
}





// 主控制任务：处理 R 键切换并分发到不同控制模式
void RoboCmdTask()
{
    static uint16_t r_count = 0 ;

    static uint64_t rtick = 0;
    static uint8_t rflag = 0;
    if (rc_data[0].key[0].r)
    {   
        if(rflag == 0)
        {
            rtick = uwTick;
            rflag = 1;
        }
    }

    if (rtick - uwTick <= -10 && !rc_data[0].key[0].r && rflag == 1)
    {
        r_count += 1;
        rflag = 0;
    }                                                                                       //实现遥控器键鼠切换(注意,键鼠操作要在选手端开启时执行)
        
    
        
    cmd_mode = r_count % 2;
    gimbal_rc_cmd->cmd_mode = cmd_mode;
    switch (cmd_mode)
    {
        case RCCONTROLMODE:
        {
            RcControlSet();
        }
        break;
        case MOUSECONTROLMODE:
        {
            MouseControlSet();
        }
        break;

        default:
            break;
    }

    if (rc_data[0].key[0].b)
    {
        NVIC_SystemReset();
    }
}




















