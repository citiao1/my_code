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
#define TARGET_DISTANCE 30.0f // 目标右侧贴墙距离：10cm

static PID_t right_wall_pid;
typedef enum {
    STATE_FOLLOW_WALL, // 沿墙巡航状态
    STATE_DECIDE,      // 停车决策状态
    STATE_TURN_LEFT,   // 原地左转状态
    STATE_TURN_RIGHT,  // 原地右转状态
    STATE_TURN_BACK,    // 原地掉头状态
		STATE_ESCAPE_LEFT,   // <-- ??
    STATE_ESCAPE_RIGHT,
		    // <-- ??:????????
    STATE_CROSSROAD_TURN,   // <-- ??:?????????
    STATE_BLIND_WALK_OUT
} CarState_t;


static CarState_t car_state = STATE_FOLLOW_WALL;
static uint32_t state_timer = 0;

static PID_t pid;
static float base_speed = 50;

static float D_left=0, D_front=0, D_right=0;
static float last_filtered = 0;

float max_distance = 0;
uint32_t max_time = 0;

uint32_t state_start_time = 0;
int left_speed=0;
int right_speed=0;
#define SAMPLE_COUNT 5
#define LIMIT_THRESH 10
#define TURN_DELAY 500
#define THRESHOLD_STOP  20.0f   // 碰撞距离（cm）
#define SCAN_SPEED      120     // 扫描转速
#define BASE_SPEED_L    100     // 前进速度
#define BASE_SPEED_R		100
#define SCAN_TIME       1500    // 扫描时间 ms（可调）
#define TURN_TOLERANCE  50      // 对准允许误差 ms
#define VMAX 150
#define D0 35.0f
#define A 10
#define TURN_SPEED 180

uint8_t huandao_flg=0;
char state[30]="forward";
#define KP 2.0f   // 比例系数：决定转向力度
#define KD 0.0f
#define KI 0.0f
float kp=KP;
float last_error=0;
void App_Init(void)
{
    

}

int speed_trans(float d)
{
	if(d<=D0)
		return 0;
	uint16_t x = d-D0;
	uint16_t denom = x+A;
	
	int speed = VMAX*x/denom;
	return speed;
}

void App_Update(float dist_1,float dist_2,float dist_3)
{
		/*static uint32_t last_time = 0;
    if (HAL_GetTick() - last_time < 20) return; 
    last_time = HAL_GetTick();
		*/
    float left_dist_raw  = dist_1;  // 左边距离
    float front_dist_raw     = dist_2;  // 正前方
    float right_dist_raw = dist_3;  // 右边距离

    // 拦截右侧死角乱码
    
		if (right_dist_raw <= 0.0f) right_dist_raw = 400.0f;
    if (left_dist_raw <= 0.0f)  left_dist_raw = 400.0f;
    // 紧急防撞
    

    static float right_dist_filtered = 20.0f; 
    if (right_dist_raw > 0.0f && right_dist_raw <= 400.0f) {
        right_dist_filtered = Filter_LowPass(right_dist_raw, right_dist_filtered, 0.2f);
    }

    static float left_dist_filtered = 20.0f;
    if (left_dist_raw > 0.0f && left_dist_raw <= 400.0f) {
        left_dist_filtered = Filter_LowPass(left_dist_raw, left_dist_filtered, 0.2f);
    } else {
        left_dist_filtered = 400.0f; 
    }


     static float front_dist_filtered = 400.0f;
    if (front_dist_raw > 0.0f && front_dist_raw <= 400.0f) 
    {
        if (front_dist_raw < front_dist_filtered) {
            // 【跌落瞬间响应】：只要看到距离变小，立刻拉低！刹车 0 延迟！
            front_dist_filtered = front_dist_raw; 
        } else {
            // 【回升防骗慢半拍】：距离突然变大（比如跳到400），用 0.1 的系数死死压住它
            // 逼它必须连续好几次测到 400，才承认路真的宽了
            front_dist_filtered = Filter_LowPass(front_dist_raw, front_dist_filtered, 0.1f);
        }
    }
    
    /*if (right_dist_filtered > 50.0f) 
    {
        Motor_SetSpeed(BASE_SPEED, BASE_SPEED - 40);
        return;
    }*/

    // ==========================================
    // 抗扰 PD 控制
    // ==========================================
    switch (car_state) 
    {
        // ------------------------------------------
        // 状态 1：正常沿墙巡航
        // ------------------------------------------
        case STATE_FOLLOW_WALL:
        {
            // 【触发事件】：前方有障碍！
						if (left_dist_filtered < 5.0f) 
            {
                car_state = STATE_ESCAPE_LEFT;
                state_timer = HAL_GetTick();
                break; // ????????,??????
            } 
            else if (right_dist_filtered < 5.0f) 
            {
                car_state = STATE_ESCAPE_RIGHT;
                state_timer = HAL_GetTick();
                break;
            }
            if (front_dist_filtered < 30.0f) 
            {
                Motor_Stop();
								Motor_SetSpeed(-100,-100);
								HAL_Delay(1000);// 紧急刹车
                car_state = STATE_DECIDE; // 切入决策状态
                state_timer = HAL_GetTick(); // 记录当前时间，用于延时停顿
                break;
            }
						if (left_dist_filtered > 50.0f && front_dist_filtered > 50.0f&&uwTick>5000&&huandao_flg<3) 
            {
                
								huandao_flg+=1;
								if(huandao_flg==1){
									car_state = STATE_BLIND_WALK_OUT;
								}
								if(huandao_flg==2){
									car_state = STATE_CROSSROAD_TURN;
								}
								
                state_timer = HAL_GetTick();
                break;
            }
						if(left_dist_filtered>50.0f&&front_dist_filtered<40.0f&&right_dist_filtered>50.0f){
							car_state=STATE_TURN_LEFT;
							state_timer=HAL_GetTick();
							break;
						}
						sprintf(state,"forward    ");
            // 防丢墙逻辑：如果右墙突然没了，给个差速慢慢向右靠
            if (right_dist_filtered > 50.0f) 
            {
                Motor_SetSpeed(BASE_SPEED, BASE_SPEED - 40);
                break;
            }

            // --- 正常的防抖 PD 贴墙算法 ---
            static float last_error = 0.0f;
            float error = right_dist_filtered - TARGET_DISTANCE;
            
            // 死区控制
            if (error > -1.0f && error < 1.0f) {
                error = 0.0f;
            }

            float derivative = error - last_error;
            last_error = error;

            float adjust = (KP * error) + (KD * derivative);

            // 【🛡️ 左侧隐形空气墙（一票否决权）】
            // adjust < 0 表示想往左转，但如果左侧小于 25cm，没收左转权利！
            if (adjust < 0.0f && left_dist_filtered < 5.0f) 
            {
                adjust = 10.0f; // 强行拉直车身，甚至轻微向右硬挤
            }

            // 限幅刹车
            if (adjust > 100.0f)  adjust = 100.0f;
            if (adjust < -100.0f) adjust = -100.0f;

            left_speed  = BASE_SPEED + (int)adjust;
            right_speed = BASE_SPEED - (int)adjust;

            // PWM 越界保护
            if (left_speed > 1000) left_speed = 1000;
            if (left_speed < 0)    left_speed = 0;
            if (right_speed > 1000) right_speed = 1000;
            if (right_speed < 0)    right_speed = 0;

            Motor_SetSpeed(left_speed, right_speed);
            break;
        }
				case STATE_CROSSROAD_TURN:
        {
            sprintf(state,"cross L   ");
            
            // ???? (??????????????,??? TURN_SPEED, -TURN_SPEED ????)
            Motor_SetSpeed(0, TURN_SPEED); 
            
            // ????:???? 150ms ????,???????
            if (HAL_GetTick() - state_timer >500 && front_dist_filtered > 40.0f ) 
            {
                // ? ????:??????????,????????!
                car_state = STATE_TURN_LEFT;
								
                state_timer = HAL_GetTick();
            }
            break;
        }
				case STATE_BLIND_WALK_OUT:
        {
            sprintf(state,"blind out ");
            
            // ??????????,???? PD ??????!
            Motor_SetSpeed(BASE_SPEED, BASE_SPEED); 

            // ??????:400ms (?????? BASE_SPEED ??,???????????)
            if (HAL_GetTick() - state_timer > 2000) 
            {
                // ?????????????,?????? PD ??
                car_state = STATE_FOLLOW_WALL; 
            }
            break;
        }
				
				case STATE_ESCAPE_LEFT:
        {
            sprintf(state,"esc left   ");
            // ?????,????????
            Motor_SetSpeed(180, 0);
            
            // ????:?? 200ms,??????????? 10cm ??
            if (HAL_GetTick() - state_timer > 100 || left_dist_filtered > 10.0f) {
                car_state = STATE_FOLLOW_WALL;
            }
            break;
        }

        // ------------------------------------------
        // ??????:??????
        // ------------------------------------------
        case STATE_ESCAPE_RIGHT:
        {
            sprintf(state,"esc right  ");
            // ?????,????????
            Motor_SetSpeed(0, 180);
            
            // ????:?? 200ms,??????????? 10cm ??
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
            // 急刹车后，停顿 200ms 等待车身停止晃动，并且让超声波数据刷新为稳态
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
            Motor_SetSpeed(0, TURN_SPEED);
            
            // 【修复 2：强制最小转弯保护期 + 滤波判断】
            // 必须先无脑转至少 300ms 避开致盲区，且滤波后的前方真实距离 > 35cm 才能退出！
            if (HAL_GetTick() - state_timer > 100 && front_dist_filtered > 30.0f) 
            {
                car_state = STATE_FOLLOW_WALL;
            }
            break;
        }

        case STATE_TURN_RIGHT:
        {
            Motor_SetSpeed(TURN_SPEED, 0);
            sprintf(state,"right   ");
            if (HAL_GetTick() - state_timer > 100 && front_dist_filtered > 40.0f) 
            {
                car_state = STATE_FOLLOW_WALL;
            }
            break;
        }

        case STATE_TURN_BACK:
        {
            Motor_SetSpeed(0, TURN_SPEED);
            sprintf(state,"back   ");
            if (HAL_GetTick() - state_timer > 100 && front_dist_filtered > 40.0f) 
            {
                car_state = STATE_FOLLOW_WALL;
            }
            break;
        }
    }
}