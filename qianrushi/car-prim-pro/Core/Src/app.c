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

typedef enum {
    STATE_FORWARD,
    STATE_STOP,
    STATE_SCAN,
    STATE_TURN_TO_MAX
} CarState_t;

static CarState_t state = STATE_FORWARD;

static PID_t pid;
static float base_speed = 50;

static float D_left=0, D_front=0, D_right=0;
static float last_filtered = 0;

float max_distance = 0;
uint32_t max_time = 0;

uint32_t state_start_time = 0;

#define SAMPLE_COUNT 5
#define LIMIT_THRESH 10
#define TURN_DELAY 500
#define THRESHOLD_STOP  40.0f   // 碰撞距离（cm）
#define SCAN_SPEED      120     // 扫描转速
#define BASE_SPEED_L    100     // 前进速度
#define BASE_SPEED_R		100
#define SCAN_TIME       1500    // 扫描时间 ms（可调）
#define TURN_TOLERANCE  50      // 对准允许误差 ms
#define VMAX 150
#define D0 20.0f
#define A 10

void App_Init(void)
{
    PID_Init(&pid, 1.5f, 0, 0.5f);

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

void App_Update(float dist_in)
{
    static uint32_t last = 0;
		int culculate_speed;

    // 控制循环周期（20ms）
    /*if (HAL_GetTick() - last < 20) return;
    last = HAL_GetTick();
	*/
    // ====== 触发测距 ======
    

    // ====== OLED显示 ======
    

    // ====== 状态机 ======
    switch(state)
    {
    // ======================
    case STATE_FORWARD:
				culculate_speed = speed_trans(dist_in);
        Motor_SetSpeed(culculate_speed, culculate_speed);

        if (dist_in < THRESHOLD_STOP)
        {
            Motor_Stop();
            state = STATE_SCAN;
            state_start_time = uwTick;
        }
        break;

    // ======================
    case STATE_STOP:

        // 稍微停一下（稳定）
        /*if (HAL_GetTick() - state_start_time > 200)
        {
            // 初始化扫描
            max_distance = 0;
            max_time = HAL_GetTick();

            state = STATE_SCAN;
            state_start_time = HAL_GetTick();
        }*/
				if(uwTick-state_start_time<1000)return;
				state = STATE_SCAN;
        break;

    // ======================
    case STATE_SCAN:

        // 原地左转扫描
        Motor_SetSpeed(0, SCAN_SPEED);

        if (dist_in > 70.0f)
        {
            Motor_Stop();
            state = STATE_FORWARD;
            state_start_time = HAL_GetTick();
        }
				/*// 记录最大距离
        if (dist_in > max_distance)
        {
            max_distance = distance;
            max_time = HAL_GetTick();
        }

        // 扫描结束
        if (HAL_GetTick() - state_start_time > SCAN_TIME)
        {
            state = STATE_TURN_TO_MAX;
        }*/
        break;

    // ======================
    case STATE_TURN_TO_MAX:

        // 继续转，直到转回最大距离方向
        Motor_SetSpeed(-SCAN_SPEED, SCAN_SPEED);

        // 时间对齐
        if (HAL_GetTick() >= max_time &&
            HAL_GetTick() - max_time < TURN_TOLERANCE)
        {
            Motor_Stop();
            state = STATE_FORWARD;
        }
        break;
				
    }
}