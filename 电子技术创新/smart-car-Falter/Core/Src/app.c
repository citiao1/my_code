#include "app.h"
#include "motor.h"
#include "servo.h"
#include "ultrasonic.h"
#include "filter.h"
#include "pid.h"
#include "maze.h"
#include "oled.h"
#include "tim.h"
#include "stdio.h"

#define BASE_SPEED 100        // 小车的基础直行速度
#define TARGET_DISTANCE 30.0f // 目标右侧贴墙距离：30cm

static PID_t right_wall_pid;

// 小车状态机枚举定义
typedef enum {
    STATE_FOLLOW_WALL,   // 沿墙巡航状态：正常行驶，保持右侧距离
    STATE_DECIDE,        // 停车决策状态：遇障停车，判断左右空间
    STATE_TURN_LEFT,     // 原地左转状态
    STATE_TURN_RIGHT,    // 原地右转状态
    STATE_TURN_BACK,     // 原地掉头状态
    STATE_ESCAPE_LEFT,   // 左侧紧急避让状态
    STATE_ESCAPE_RIGHT,  // 右侧紧急避让状态
    STATE_CROSSROAD_TURN,// 十字路口左转状态
    STATE_BLIND_WALK_OUT // 盲走冲出状态（用于环岛或特殊场景）
} CarState_t;

static CarState_t car_state = STATE_FOLLOW_WALL; // 当前小车状态
static uint32_t state_timer = 0;                 // 状态持续时间计时器

static PID_t pid;
static float base_speed = 50;

static float D_left=0, D_front=0, D_right=0; // 原始距离数据（未使用滤波前）
static float last_filtered = 0;              // 上次滤波值

float max_distance = 0;                      // 记录最大探测距离
uint32_t max_time = 0;                       // 记录最大距离对应的时间

uint32_t state_start_time = 0;               // 状态开始时间戳
int left_speed=0;                            // 左轮目标速度
int right_speed=0;                           // 右轮目标速度

#define SAMPLE_COUNT 5
#define LIMIT_THRESH 10
#define TURN_DELAY 500
#define THRESHOLD_STOP  20.0f   // 碰撞停止距离阈值（cm）
#define SCAN_SPEED      120     // 扫描时的电机转速
#define BASE_SPEED_L    100     // 基础前进速度左轮
#define BASE_SPEED_R		100     // 基础前进速度右轮
#define SCAN_TIME       1500    // 扫描持续时间 ms
#define TURN_TOLERANCE  50      // 转弯对准允许误差 ms
#define VMAX 150                // 速度映射最大输出
#define D0 35.0f                // 速度映射起始距离
#define A 10                    // 速度映射调节系数
#define TURN_SPEED 180          // 原地转弯速度

uint8_t huandao_flg=0;         // 环岛/特殊路口标志位
char state[30]="forward";      // 当前状态字符串，用于OLED显示

#define KP 2.0f   // PD控制比例系数：决定转向纠正力度
#define KD 0.0f   // PD控制微分系数
#define KI 0.0f   // PD控制积分系数（暂未使用）
float kp=KP;
float last_error=0;

/**
 * @brief 应用初始化函数
 * @note 目前为空，可在其中初始化PID参数或其他模块
 */
void App_Init(void)
{
    

}

/**
 * @brief 距离-速度映射转换函数
 * @param d 当前距离值
 * @return 计算后的速度值
 * @note 当距离小于D0时返回0，否则根据非线性公式计算速度，距离越远速度越快并趋于VMAX
 */
int speed_trans(float d)
{
	if(d<=D0)
		return 0;
	uint16_t x = d-D0;
	uint16_t denom = x+A;
	
	int speed = VMAX*x/denom;
	return speed;
}

/**
 * @brief 应用主更新循环，处理传感器数据并控制电机
 * @param dist_1 左侧超声波距离
 * @param dist_2 前方超声波距离
 * @param dist_3 右侧超声波距离
 */
void App_Update(float dist_1,float dist_2,float dist_3)
{
		/*static uint32_t last_time = 0;
    if (HAL_GetTick() - last_time < 20) return; 
    last_time = HAL_GetTick();
		*/
    float left_dist_raw  = dist_1;  //左边距离
    float front_dist_raw     = dist_2;  // 正前方
    float right_dist_raw = dist_3;  // 右边距离

    // 拦截右侧死角乱码: 无效数据替换为极大值
		if (right_dist_raw <= 0.0f) right_dist_raw = 400.0f;
    if (left_dist_raw <= 0.0f)  left_dist_raw = 400.0f;

    

    // 右侧距离低通滤波
    static float right_dist_filtered = 20.0f; 
    if (right_dist_raw > 0.0f && right_dist_raw <= 400.0f) {
        right_dist_filtered = Filter_LowPass(right_dist_raw, right_dist_filtered, 0.2f);
    }

    // 左侧距离低通滤波
    static float left_dist_filtered = 20.0f;
    if (left_dist_raw > 0.0f && left_dist_raw <= 400.0f) {
        left_dist_filtered = Filter_LowPass(left_dist_raw, left_dist_filtered, 0.2f);
    } else {
        left_dist_filtered = 400.0f; 
    }


     // 前方距离特殊滤波策略：跌落后立即响应，回升时缓慢跟随以防误判
     static float front_dist_filtered = 400.0f;
    if (front_dist_raw > 0.0f && front_dist_raw <= 400.0f) 
    {
        if (front_dist_raw < front_dist_filtered) {
            front_dist_filtered = front_dist_raw; 
        } else {
            front_dist_filtered = Filter_LowPass(front_dist_raw, front_dist_filtered, 0.1f);
        }
    }
    
    /*if (right_dist_filtered > 50.0f) 
    {
        Motor_SetSpeed(BASE_SPEED, BASE_SPEED - 40);
        return;
    }*/


    switch (car_state) 
    {
        // ------------------------------------------
        // 状态 1：正常沿墙巡航
        // ------------------------------------------
        case STATE_FOLLOW_WALL:
        {
            // 【触发事件】：左侧过近，进入左侧避让
						if (left_dist_filtered < 5.0f) 
            {
                car_state = STATE_ESCAPE_LEFT;
                state_timer = HAL_GetTick();
                break; 
            } 
            // 【触发事件】：右侧过近，进入右侧避让
            else if (right_dist_filtered < 5.0f) 
            {
                car_state = STATE_ESCAPE_RIGHT;
                state_timer = HAL_GetTick();
                break;
            }
            // 【触发事件】：前方有障碍，停车并进入决策状态
            if (front_dist_filtered < 30.0f) 
            {
                Motor_Stop();
                car_state = STATE_DECIDE; // 切入决策状态
                state_timer = HAL_GetTick(); // 记录当前时间，用于延时停顿
                break;
            }
            // 【触发事件】：左侧和前方空旷，可能进入环岛或特殊路口
						if (left_dist_filtered > 50.0f && front_dist_filtered > 50.0f&&uwTick>5000&&huandao_flg<3) 
            {
                
								huandao_flg+=1;
								if(huandao_flg==1){
									car_state = STATE_BLIND_WALK_OUT; // 第一次触发：盲走冲出
								}
								if(huandao_flg==2){
									car_state = STATE_CROSSROAD_TURN; // 第二次触发：路口转弯
								}
								
                state_timer = HAL_GetTick();
                break;
            }
            // 【触发事件】：左侧空旷且前方较近，右侧空旷，执行左转
						if(left_dist_filtered>50.0f&&front_dist_filtered<40.0f&&right_dist_filtered>50.0f){
							car_state=STATE_TURN_LEFT;
							state_timer=HAL_GetTick();
							break;
						}
						sprintf(state,"forward    "); // 更新显示状态
            
            // 防丢墙逻辑：如果右墙突然没了，给个差速慢慢向右靠
            if (right_dist_filtered > 50.0f) 
            {
                Motor_SetSpeed(BASE_SPEED, BASE_SPEED - 40);
                break;
            }

            // --- 正常的防抖 PD 贴墙算法 ---
            static float last_error = 0.0f;
            float error = right_dist_filtered - TARGET_DISTANCE; // 计算与目标距离的误差
            
            // 死区控制：微小误差不处理，防止电机抖动
            if (error > -1.0f && error < 1.0f) {
                error = 0.0f;
            }

            float derivative = error - last_error; // 计算微分项
            last_error = error;

            float adjust = (KP * error) + (KD * derivative); // 计算PD输出调整量


            // adjust < 0 表示想往左转，但如果左侧小于 5cm，没收左转权利，强制向右修正
            if (adjust < 0.0f && left_dist_filtered < 5.0f) 
            {
                adjust = 10.0f; // 强行拉直车身，甚至轻微向右硬挤
            }

            // 限幅刹车：限制调整量的范围
            if (adjust > 100.0f)  adjust = 100.0f;
            if (adjust < -100.0f) adjust = -100.0f;

            left_speed  = BASE_SPEED + (int)adjust;  // 左轮速度 = 基础速度 + 调整量
            right_speed = BASE_SPEED - (int)adjust;  // 右轮速度 = 基础速度 - 调整量

            // PWM 越界保护
            if (left_speed > 1000) left_speed = 1000;
            if (left_speed < 0)    left_speed = 0;
            if (right_speed > 1000) right_speed = 1000;
            if (right_speed < 0)    right_speed = 0;

            Motor_SetSpeed(left_speed, right_speed); // 设置电机速度
            break;
        }
        
        // ------------------------------------------
        // 状态：十字路口左转处理
        // ------------------------------------------
				case STATE_CROSSROAD_TURN:
        {
            sprintf(state,"cross L   ");
            
            // 执行左转动作
            Motor_SetSpeed(0, TURN_SPEED); 
            
            // 判断退出条件：时间超过500ms 且 前方距离大于40cm（确认转过去了）
            if (HAL_GetTick() - state_timer >500 && front_dist_filtered > 40.0f ) 
            {
                // 转入普通左转状态进行微调或继续行驶
                car_state = STATE_TURN_LEFT;
								
                state_timer = HAL_GetTick();
            }
            break;
        }
        
        // ------------------------------------------
        // 状态：盲走冲出（用于环岛等无墙区域）
        // ------------------------------------------
				case STATE_BLIND_WALK_OUT:
        {
            sprintf(state,"blind out ");
            
            // 直线全速前进，不依赖侧墙PD
            Motor_SetSpeed(BASE_SPEED, BASE_SPEED); 

            // 持续盲走2000ms后，尝试回归沿墙模式
            if (HAL_GetTick() - state_timer > 2000) 
            {
                // 时间到，恢复正常的 PD 贴墙控制
                car_state = STATE_FOLLOW_WALL; 
            }
            break;
        }
				
        // ------------------------------------------
        // 状态：左侧紧急避让
        // ------------------------------------------
				case STATE_ESCAPE_LEFT:
        {
            sprintf(state,"esc left   ");
            // 向右急转以避开左侧障碍
            Motor_SetSpeed(180, 0);
            
            // 退出条件：时间超过100ms 或 左侧距离大于10cm
            if (HAL_GetTick() - state_timer > 100 || left_dist_filtered > 10.0f) {
                car_state = STATE_FOLLOW_WALL;
            }
            break;
        }

        // ------------------------------------------
        // 状态：右侧紧急避让
        // ------------------------------------------
        case STATE_ESCAPE_RIGHT:
        {
            sprintf(state,"esc right  ");
            // 向左急转以避开右侧障碍
            Motor_SetSpeed(0, 180);
            
            // 退出条件：时间超过100ms 或 右侧距离大于10cm
            if (HAL_GetTick() - state_timer > 100 || right_dist_filtered > 10.0f) {
                car_state = STATE_FOLLOW_WALL;
            }
            break;
        }

        // ------------------------------------------
        // 状态 2：停车决策 (遇障后观察左右)
        // ------------------------------------------
        case STATE_DECIDE:
        {
						
						HAL_Delay(100);
            // 急刹车后，停顿等待车身停止晃动，并且让超声波数据刷新为稳态
						sprintf(state,"decide    ");
            if (HAL_GetTick() - state_timer < 500) {
                Motor_Stop();
                break;
            }

            // 决策逻辑：对比左右两边的数据，哪边宽敞去哪边
            if (left_dist_filtered < 10.0f && right_dist_filtered < 10.0f) 
            {
                // 两边都是死胡同（或者距离太近），原地掉头
                car_state = STATE_TURN_BACK;
            }
            else if (left_dist_filtered > right_dist_filtered) 
            {
                // 左边比右边宽敞，向左转
                car_state = STATE_TURN_LEFT;
            }
            else 
            {
                // 右边更宽敞，向右转
                car_state = STATE_TURN_RIGHT;
            }
            break;
        }

        // ------------------------------------------
        // 状态 3：原地左转
        // ------------------------------------------
        case STATE_TURN_LEFT:
        {
						sprintf(state,"left   ");
            Motor_SetSpeed(0, TURN_SPEED); // 左轮停，右轮转
            
            // 退出条件：时间超过100ms 且 前方距离大于30cm（确保转出障碍物范围）
            if (HAL_GetTick() - state_timer > 100 && front_dist_filtered > 30.0f) 
            {
                car_state = STATE_FOLLOW_WALL;
            }
            break;
        }

        // ------------------------------------------
        // 状态：原地右转
        // ------------------------------------------
        case STATE_TURN_RIGHT:
        {
            Motor_SetSpeed(TURN_SPEED, 0); // 右轮停，左轮转
            sprintf(state,"right   ");
            // 退出条件：时间超过100ms 且 前方距离大于40cm
            if (HAL_GetTick() - state_timer > 100 && front_dist_filtered > 40.0f) 
            {
                car_state = STATE_FOLLOW_WALL;
            }
            break;
        }

        // ------------------------------------------
        // 状态：原地掉头
        // ------------------------------------------
        case STATE_TURN_BACK:
        {
            Motor_SetSpeed(0, TURN_SPEED); // 此处逻辑同左转，实际可能需要更复杂的掉头逻辑
            sprintf(state,"back   ");
            // 退出条件：时间超过100ms 且 前方距离大于40cm
            if (HAL_GetTick() - state_timer > 100 && front_dist_filtered > 40.0f) 
            {
                car_state = STATE_FOLLOW_WALL;
            }
            break;
        }
    }
}