#include "motor.h"
#include "stdio.h"

#define ONE_LOOP 2150000
int16 _encoder_L;
int16 _encoder_R;
int16 left_pwm;
int16 right_pwm;

int16 vofa_target_speed = 0;
int16 vofa_current_speed = 0;
int16 vofa_out_pwm = 0;

int16 stop_flag = 1;
// 负压风扇全局控制变量
int16 target_fan_pwm;  // 期望的负压PWM (目标值)
int16 current_fan_pwm; // 当前正在输出的PWM (现值)

/*pwm输出限幅*/
int16 max_forward_pwm = (int16)(3200 * 0.8);  // 正转限制在约 85%
int16 max_reverse_pwm = (int16)(-3200 * 0.8) ; // 反转限制在约 -40% (刹车力度够用就行)
double car_distance = 0.0f;    // ������ʻ���ۼ����
double global_distance = 0.0f; // ?? ����ȫ����̣��ӷ�����ʼȫ���ۼӣ����ڻ�����������
int set_distance_calculation = 0;

// ������㺯��������Ѱ�����ʱ��Ҫ�õ���
void Clear_Car_Distance(void)
{
    car_distance = 0.0f;
}
// ?? ������ȫ��������㣨����һȦ���ص����ʱ���ã�
void Clear_Global_Distance(void)
{
    global_distance = 0.0f;
}
void Car_Distance_Calculate(void)
{
	double step_dist = (double)(_encoder_L + _encoder_R) / 2.0f;
    
    // 1. ȫ����̣�ֻҪ�����ܣ���Զ�����ۼӣ������κα�־λ���ƣ�
    global_distance += step_dist;
		if(global_distance > ONE_LOOP) global_distance = 0.0f;
	if(set_distance_calculation)
	{
		car_distance += step_dist;
	}
}

void Motor_Overload_Protection_Task(int16 lpwm, int16 rpwm)
{
    static uint16 overload_timer = 0;
    
    // 动态计算 90% 限幅阈值：2560 * 0.9 = 2304
    int16 threshold = (int16)(max_forward_pwm * 0.9f); 

    // 如果车子本身就是急停状态，重置计数器并退出
    if (stop_flag == 1)
    {
        overload_timer = 0;
        return;
    }

    // 检查左轮【或】右轮的绝对值是否触及了 90% 的死亡红线
    if (abs(lpwm) >= 2000 || abs(rpwm) >= 2000)
    {
        overload_timer++;
        
        // ⏱️ 时间滤波计算：2ms * 500次 = 1000ms = 1秒
        // 允许高速过弯时瞬间满载拉满，但绝对不允许持续憋满 1 秒！
        // (如果你觉得 1 秒太长，可以改成 250次，对应 500ms)
        if (overload_timer >= 500) 
        {
            stop_flag = 1; // 🚨 触发全局急停，切断动力！
        }
    }
    else
    {
        // 只要有一瞬间脱离了高负载（比如过完弯了），计时器立刻清零，防止误判
        overload_timer = 0; 
    }
}

/**
 * @brief 电机速度控制函数：输入目标速度，读取当前速度，通过PID计算占据比，控制电机转速
 * @param Left_Target_Speed 左轮目标速度 Right_Target_Speed 右轮目标速度
 * @param 无返回值，但可能会出错
 */
void Motor_Control(int16 Left_Target_Speed, int16 Right_Target_Speed)
{
//		static uint8 Motor_stop_Time = 0; // 堵转计时器
//    static int16 left_pwm_last = 0;   // 记录上一次的 PWM (用于滤波)
//    static int16 right_pwm_last = 0;
		_encoder_L = -1 * Read_Encoder(1);//读取编码器数值(真实速度) ,左编码器前进为负值因此前面加上负号
		_encoder_R = Read_Encoder(2);//读取编码器数值(真实速度)
		Car_Distance_Calculate();
		/*堵转检测机制 (撞墙保护)*/
//		if ((Left_Target_Speed != 0 || Right_Target_Speed != 0) && 
//         (abs(_encoder_L) < 5 || abs(_encoder_R) < 5) && 
//         stop_flag == 0)
//		{
//				Motor_stop_Time++;
//        if (Motor_stop_Time >= 50) // 假设2ms周期，50次就是持续100ms堵转
//        {
//            stop_flag = 1;         // 触发急停！
//        }
//		}
//		else 
//		{
//				Motor_stop_Time = 0;       // 恢复正常，清零计数器
//		}

	//速度环占据比，分别用PID_LEFT和PID_RIGHT索引来实现左右轮控制
	left_pwm = PID_Calculate_ByIndex(PID_LEFT, _encoder_L, Left_Target_Speed);
	right_pwm = PID_Calculate_ByIndex(PID_RIGHT, _encoder_R, Right_Target_Speed);
		
			/*输出低通滤波  防止电流突变*/
//		left_pwm = (int16)(left_pwm * 0.8f + left_pwm_last * 0.2f);
//    right_pwm = (int16)(right_pwm * 0.8f + right_pwm_last * 0.2f);
//    left_pwm_last = left_pwm;
//    right_pwm_last = right_pwm;
		
		left_pwm = left_pwm > max_forward_pwm ? max_forward_pwm : left_pwm;
    left_pwm = left_pwm < max_reverse_pwm ? max_reverse_pwm : left_pwm;
    
    right_pwm = right_pwm > max_forward_pwm ? max_forward_pwm : right_pwm;
    right_pwm = right_pwm < max_reverse_pwm ? max_reverse_pwm : right_pwm;
		
		
//		Motor_Overload_Protection_Task(left_pwm, right_pwm);
		
		if (stop_flag == 1)
    {
        left_pwm = 0;
        right_pwm = 0;
    }
//		left_pwm = 0;
//    right_pwm = 0;
//		else 
//		{
//			left_pwm = 1500;
//        right_pwm = 1500;
//		}
		Motor_Ctrl(right_pwm, -left_pwm);
	/*
	当3000占空比时对应编码器输出为1100，则认为左右轮最大转速为1100
	*/
//	Motor_Ctrl(-right_pwm, left_pwm); //输出需要自己确保正确
	//printf("%d,%d,%d,%d,%d\n", chassis.target_speed, _encoder_L,_encoder_R, left_pwm,-right_pwm);
	
//	vofa_target_speed = Left_Target_Speed;
//  vofa_current_speed = _encoder_L;
//  vofa_out_pwm = left_pwm;
}


/**
 * @brief 负压风扇平滑控制后台任务 (软着陆)
 * 放在 main 的 while(1) 中执行
 */
void Fan_Smooth_Task(void)
{
    static uint16 fan_tick = 0;
    
    // 1. 根据当前车模状态，下达【期望目标】
    if (stop_flag == 1) 
    {
        target_fan_pwm = 1000; // 死机/待机：期望 900
    }
    
    // 2. 利用 while(1) 的循环周期，进行非阻塞【现值追踪】
    fan_tick++;
    if (fan_tick >= 5) // 决定平滑的速度，数值越大变化越慢
    {
        fan_tick = 0;
        
        // 现值 < 期望值：缓加速
        if (current_fan_pwm < target_fan_pwm) 
        {
            current_fan_pwm += 50; 
        }
        // 现值 > 期望值：缓减速 (防撞墙着陆)
        else if (current_fan_pwm > target_fan_pwm) 
        {
            current_fan_pwm -= 50; 
        }
        
        // 3. 永远只用现值来驱动电机
        BLmotor_Ctrl_w1(current_fan_pwm);
    }
}