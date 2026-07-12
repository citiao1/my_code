#include "chassis.h"
#include "inductance.h"
#include "motor.h"
#include "LQ_Encoder.h"
#include "LQ_LSM6DSR_Hard.h"

// 设定单节 3.3V (总压 9.9V) 的极限死亡线阈值
#define BAT_SAFE_ADC_THRESHOLD  2240
// ==========================================
// 🌟 物理标定常数 (根据实际测试结果设定)
// ==========================================
#define ROUNDABOUT_ZONE_MIN  600000  // ���뱽��Ԥ���� (��ǰһ������)
#define ROUNDABOUT_ZONE_MAX  900000 // �뿪����Ԥ���� (���껷���󳹵ױ���)
#define PITCH_THRESHOLD  1500
// 距离标定：1m ≈ 64000 
#define LOOP_ENTRY_DIST  15000  // 入环前盲开距离 (约 0.5m)，如果进环太早，可增大此值
#define LOOP_IN_DIST 13000
#define LOOP_COOLDOWN_DIST  12000 // 出环后的冷却距离 (约 30cm, 64000*0.3)
// 角度标定：1° ≈ 14.21
#define LOOP_EXIT_ANGLE  5300   // 环岛出口触发角度g (约 310°)
#define LOOP_IN_ANGLE  600   // 环岛入口触发角度 (约 30°)
#define LOOP_EXIT_EXTRA_ANGLE  420   // 出环额外要求的 30° 旋转
Chassis_TypeDef chassis;
IMU_Data_t imu_data; // 实例化全局缓存变量
int16 direction_output;          // 内环输出：最终给车轮的差速修正量
int16 actual_yaw_rate;            // 内环输入：真实的偏航角速度
int16 current_adc;
int16 target_yaw_rate = 0;
int set_angle_calculation = 0;
RoundaboutState_e loop_state = LOOP_NORMAL;
LoopType_e loop_type = LOOP_NONE;

double car_angle = 0.0f;       // 车辆绝对角度
/**
 * @brief 底盘初始化
 * 初始化底盘结构体中的速度和目标偏差变量
 */
void Chassis_Init(void)
{
		chassis.target_speed = 300;       // 初始化目标速度为100   max : 1100
    chassis.current_deviation = 0;  // 初始化当前偏差为0
    chassis.left_speed = 0;         // 初始化左轮速度为0
    chassis.right_speed = 0;        // 初始化右轮速度为0
}

void Clear_Car_Angle(void)
{
	car_angle = 0.0f;
}

/**
 * @brief 电池低压保护监控任务
 * 建议放在 2ms 的 Timer1 中断中执行
 */
void Battery_Protection_Task(void)
{
    static uint16 low_vol_timer = 0;
    
    // 1. 直接读取原始 ADC 值 (无需消耗算力转换成浮点电压)
    current_adc = Get_ADCResult(ADC_CH9_P01);
    
		if(stop_flag == 1) return;
    // 2. 检测是否低于警戒线
    if (current_adc < BAT_SAFE_ADC_THRESHOLD) 
    {
        low_vol_timer++;
        
        // 3. 时间滤波（防抖）：2ms * 500 = 1秒
        // 防止电机瞬间加速抽血导致的误判
        if (low_vol_timer >= 500) 
        {
            stop_flag = 1;  // 触发全局急停
        }
    }
    else 
    {
        // 电压弹回安全线以上，清零计时器
        low_vol_timer = 0;
    }
}
/**
 * @brief 统一更新 IMU 全部 6 轴数据
 * 建议在控制主循环 (2ms) 的最开头调用一次即可
 */
void IMU_Update(void)
{
#ifdef HARDWARE_SPI
    LSM6DSR_Read_Data(&imu_data.ax, &imu_data.ay, &imu_data.az, &imu_data.gx, &imu_data.gy, &imu_data.gz);
#elif defined HARDWARE_IIC
    LQ_HARD_IIC_LSM60DSR_Read(&imu_data.ax, &imu_data.ay, &imu_data.az, &imu_data.gx, &imu_data.gy, &imu_data.gz);
#else
    imu_data.gy = 0; 
    imu_data.gz = 0; 
#endif
}

/**
 * @brief 获取陀螺仪Z轴角速度 (偏航角速度)
 * @return int16 Z轴原始角速度数据
 */
int16 Get_Yaw_Rate(void)
{
    int16 ax, ay, az, gx, gy, gz;
    
    // 调用底层函数读取6轴数据
    // 注意：传入的是变量的地址(&)，底层函数会把读到的值放进这些变量里
#ifdef HARDWARE_SPI
    LSM6DSR_Read_Data(&ax, &ay, &az, &gx, &gy, &gz);
#elif defined HARDWARE_IIC
    LQ_HARD_IIC_LSM60DSR_Read(&ax, &ay, &az, &gx, &gy, &gz);
#else
    // 如果都没有定义，默认给0防止报错，请检查你的工程宏定义
    gz = 0; 
#endif

    // 我们只需要 Z轴的角速度 (Gyro Z)
    return gz; 
}

/**
 * @brief 获取陀螺仪Y轴角速度 (通常为俯仰或横滚角速度，取决于安装方向)
 * @return int16 Y轴原始角速度数据
 */
int16 Get_Pitch_Rate(void)
{
    int16 ax, ay, az, gx, gy, gz;
    
    // 调用底层函数读取6轴数据
    // 注意：传入的是变量的地址(&)，底层函数会把读到的值放进这些变量里
#ifdef HARDWARE_SPI
    LSM6DSR_Read_Data(&ax, &ay, &az, &gx, &gy, &gz);
#elif defined HARDWARE_IIC
    LQ_HARD_IIC_LSM60DSR_Read(&ax, &ay, &az, &gx, &gy, &gz);
#else
    // 如果都没有定义，默认给0防止报错，请检查你的工程宏定义
    gy = 0;  // 🌟 注意这里改成了给 gy 赋默认值
#endif

    // 我们只需要 Y轴的角速度 (Gyro Y)
    return gy; 
}

///**
// * @brief 计算循迹偏差
// * 使用差比和算法计算车辆偏离中心线的程度
// * 公式：( (左感+左感) - (右感+右感) ) / (总电感 + 补偿值) * 100
// * @return int 归一化后的偏差值，范围 -100 到 100
// */
//int Calculate_Deviation(void) 
//{
//    // 获取电感ADC数组指针 (假设顺序为: 左外, 左内, 中, 右内, 右外)
//    int* adc_values = GetInductance();
//    
//    // 计算左右两侧电感值的差值 (分子)，反映偏向哪一侧
//    int eleSub = (adc_values[0] + adc_values[1]) - (adc_values[3] + adc_values[4]);
//    
//    // 计算所有电感值的总和 (分母)，+10是为了防止分母为0导致除零错误
//    // 使用“差比和”可以消除由于速度变化导致的磁场强度整体变化影响
//    int eleAdd = adc_values[0] + adc_values[1] + adc_values[2] + adc_values[3] + adc_values[4] + 10;
//	
//    // 计算归一化偏差值
//    float eleValue = (float)eleSub / (float)eleAdd * 100.0f;
//	
//		static uint16 off_track_timer = 0;   // 丢线持续时间计数器
//		if(stop_flag == 1) return 0;
//	
//		if(eleAdd < 20.0)
//		{
//			off_track_timer++;
//			if(off_track_timer >= 250) stop_flag = 1;
//		}
//		else
//		{
//			off_track_timer = 0;
//		}
//    
//    // 限幅处理：将偏差值限制在 -100 到 100 之间
//    if(eleValue > 100.0f) eleValue = 100.0f;
//    else if(eleValue < -100.0f) eleValue = -100.0f;

//    // 转换为整型返回给控制层
//    return (int)(eleValue);
//}

/**
 * @brief 计算循迹偏差
 * 根据苯环状态机，动态切换差比和权重，欺骗 PID 做出特定转向动作
 */
int Calculate_Deviation(void) 
{
    int* adc = GetInductance(); // 简化指针名称，方便书写
    float eleSub = 0;
    float eleAdd = 0;
    float eleValue = 0;
    static uint16 off_track_timer = 0;

    if(stop_flag == 1) return 0;

    // =========================================================================
    // 🌟 核心控制层：状态机多路复用权重分配
    // =========================================================================
    if (loop_state == LOOP_NORMAL)
    {
        // 【正常巡线】原汁原味的差比和
        eleSub = (adc[0] + adc[1]) - (adc[3] + adc[4]);
        eleAdd = adc[0] + adc[1] + adc[2] + adc[3] + adc[4] + 10;
    }
    else if (loop_state == LOOP_APPROACH)
    {
        // 【逼近阶段】屏蔽最外侧电感 (adc[0]和adc[3])，防止被侧边圆环的磁场提早“吸”偏
        eleSub = adc[1] - adc[4];
//        eleAdd = adc[1] + adc[2] + adc[4] + 10;
//					eleSub = (adc[0] + adc[1]) - (adc[3] + adc[4]);
					eleAdd = adc[0] + adc[1] + adc[2] + adc[3] + adc[4] + 10;
    }
    else if (loop_state == LOOP_TURN_IN)
    {
        // 【强制入环】制造巨大假偏差，瞬间拉满 PID！
        if (loop_type == LOOP_RIGHT)
        {
            // 进左环：极度削弱左侧，放大右侧。制造“严重偏右”的假象，强迫车猛向左打死！
            eleSub = (0.2f * adc[0] + 0.5f * adc[1]) - (1.8f * adc[3] + 1.5f * adc[4]);
        }
        else if (loop_type == LOOP_LEFT)
        {
            // 进右环：极度放大左侧，削弱右侧。制造“严重偏左”的假象，强迫车猛向右打死！
            eleSub = (1.8f * adc[0] + 1.5f * adc[1]) - (0.2f * adc[3] + 0.5f * adc[4]);
        }
        eleAdd = adc[0] + adc[1] + adc[2] + adc[3] + adc[4] + 10;
    }
    else if (loop_state == LOOP_IN_LOOP)
    {
        // 【环内巡线】微调权重，防止被离心力甩出
//        if (loop_type == LOOP_RIGHT)
//        {
//            // 左环内，适度放大右侧，让车有持续向左靠拢（贴内侧）的趋势
//            eleSub = (0.8f * adc[0] + 0.8f * adc[1]) - (1.2f * adc[3] + 1.2f * adc[4]);
//        }
//        else 
//        {
//            // 右环内，适度放大左侧，让车有持续向右靠拢的趋势
//            eleSub = (1.2f * adc[0] + 1.2f * adc[1]) - (0.8f * adc[3] + 0.8f * adc[4]);
//        }
				eleSub = (adc[0] + adc[1]) - (adc[3] + adc[4]);
        eleAdd = adc[0] + adc[1] + adc[2] + adc[3] + adc[4] + 10;
    }
    else if (loop_state == LOOP_EXITING)
    {
        // 【强制出环】反向拉扯，把车头强制拽出圆环
        if (loop_type == LOOP_RIGHT)
        {
            // 左环出环：放大左侧权重，送给 PID 正偏差，强迫车头向右转（拉直出环）
            eleSub = (1.5f * adc[0] + 1.5f * adc[1]) - (0.5f * adc[3] + 0.5f * adc[4]);
        }
        else 
        {
            // 右环出环：放大右侧权重，强迫车头向左转
            eleSub = (0.5f * adc[0] + 0.5f * adc[1]) - (1.5f * adc[3] + 1.5f * adc[4]);
        }
        eleAdd = adc[0] + adc[1] + adc[2] + adc[3] + adc[4] + 10;
    }
    // =========================================================================

    // 丢线保护逻辑 (保持原样不动)
    if(eleAdd < 20.0)
    {
        off_track_timer++;
        if(off_track_timer >= 250) stop_flag = 1;
    }
    else
    {
        off_track_timer = 0;
    }

    // 归一化偏差值计算
    eleValue = (float)eleSub / (float)eleAdd * 100.0f;

    // 限幅处理：将偏差值限制在 -100 到 100 之间
    if(eleValue > 100.0f) eleValue = 100.0f;
    else if(eleValue < -100.0f) eleValue = -100.0f;

    // 转换为整型返回给控制层
    return (int)(eleValue);
}


/**
 * @brief 动态角速度阶跃测试序列 (状态机)
 * @note 依赖 Timer1 的 2ms 周期。500次 tick = 1秒。
 */
void Dynamic_Yaw_Test_Task(void)
{
    static uint16 time_tick = 0;   // 时间计数器
    static uint8 current_stage = 0; // 当前所处的测试阶段

    // 1. 如果车子处于急停/未发车状态，重置测试进度
    if (stop_flag == 1) 
    {
        time_tick = 0;
        current_stage = 0;
        target_yaw_rate = 0;
        return;
    }

    // 2. 车子启动后，开始计时 (2ms 进一次)
    time_tick++;

    // 3. 设定每个阶段持续的时间：例如 1.5 秒 (750 ticks) 切换一次状态
    if (time_tick >= 2500) 
    {
        time_tick = 0;       // 清零计时器
        current_stage++;     // 进入下一个测试阶段
    }

    // 4. 执行状态机序列
    switch (current_stage)
    {
        case 0: target_yaw_rate = 4000;  break;  // 阶段0：正转 3000
        case 1: target_yaw_rate = -4000; break;  // 阶段1：反转 3000
        case 2: target_yaw_rate = 7000;  break;  // 阶段2：正转 4000
       case 3: target_yaw_rate = -7000; break;  // 阶段3：反转 4000
//        case 4: target_yaw_rate = 8500;  break;  // 阶段4：正转 5000
//        case 5: target_yaw_rate = 5500; break;  // 阶段5：反转 5000
//        case 6: target_yaw_rate = 6000;  break;  // 阶段6：正转 6000
//        case 7: target_yaw_rate = -6000; break;  // 阶段7：反转 6000
//			case 8: target_yaw_rate = 9000;break;
//			case 9: target_yaw_rate = -9000;break;
//			case 10:target_yaw_rate = 10000;break;
//			case 11:target_yaw_rate = -10000;break;
        default:
            // 测试结束：归零并强行触发停车保护
            target_yaw_rate = 0;
            stop_flag = 1; 
            break;
    }
}

void Roundabout_Detect_Task(void)
{
    int* adc = GetInductance(); // 0:左外, 1:左内, 2:中, 3:右内, 4:右外
		int16 pitch = imu_data.gy;
    static uint16 state_timer = 0;
		static uint16 timeout_counter = 0; // 🌟 新增：超时保护计数器
		
    switch(loop_state)
    {
        case LOOP_NORMAL:
					
				if (global_distance < ROUNDABOUT_ZONE_MIN || global_distance > ROUNDABOUT_ZONE_MAX)
            {
                // ���ڱ�������ǿ�ƹ���������ؼ�ʱ������ֱ��������������ѭ����
                state_timer = 0;
                break; 
            }
    // 🌟 识别左环
				if (set_distance_calculation == 1 && car_distance < LOOP_COOLDOWN_DIST)
            {
                // 还在冷却期内，什么都不做
                state_timer = 0;
                break; 
            }
        else if (set_distance_calculation == 1 && car_distance >= LOOP_COOLDOWN_DIST)
            {
                // 跑够了冷却距离，关闭距离积分，恢复正常寻找状态
                set_distance_calculation = 0; 
            }
				if(adc[2] > 90 && ((adc[0] > 80 )|| adc[3] > 80) && abs(pitch) < PITCH_THRESHOLD) 		//(adc[0] + adc[1]) - (adc[3] + adc[4]) > 30   (adc[0] + adc[1]) - (adc[3] + adc[4]) > 15  abs(pitch) < PITCH_THRESHOLD
				{
						state_timer++;
						if(state_timer >= 2) // 20ms 防抖
						{
								loop_state = LOOP_APPROACH;
								loop_type = LOOP_NONE;
								Clear_Car_Distance(); // 距离清零，开始盲开计步
								set_distance_calculation = 1;  //开启位置计算
								state_timer = 0;
								timeout_counter = 0; // 清零超时计数
						}
				}
    // 🌟 识别右环 (必须加 else if！)
//				else if(adc[2] > 90 && adc[3] > 70  && (adc[4] + adc[3]) - (adc[0] + adc[1]) > 10)		//(adc[4] + adc[3]) - (adc[0] + adc[1]) > 25   (adc[4] + adc[3]) - (adc[0] + adc[1]) > 15
//				{
//						state_timer++;
//						if(state_timer >= 1) // 20ms 防抖
//						{
//								loop_state = LOOP_APPROACH;
//								loop_type = LOOP_RIGHT;
//								Clear_Car_Distance(); // 距离清零，开始盲开计步
//								set_distance_calculation = 1;  //开启距离积分
//								state_timer = 0;
//								timeout_counter = 0; // 清零超时计数
//						}
//				}
    // 🌟 如果都不满足，才清零防抖计时器
				else 
				{ 
						state_timer = 0;
				}
				break;

        case LOOP_APPROACH:
            // 等待车头逼近入环点（靠里程积分判断）
						timeout_counter++;
            if(car_distance >= LOOP_ENTRY_DIST && adc[2] > 70 &&(adc[0] > 60 || adc[3] > 60)) 
            {
								
								if ((adc[0] + adc[1]) >= (adc[3] + adc[4])) 
                {
                    loop_type = LOOP_LEFT;  // 左边能量大，确认是左环
                }
                else if((adc[0] + adc[1])< (adc[3] + adc[4]))
                {
                    loop_type = LOOP_RIGHT; // 右边能量大，确认是右环
                }
								
                loop_state = LOOP_TURN_IN;
								Clear_Car_Distance();
                Clear_Car_Angle(); // 🌟 角度清零，准备入环测角度！
								set_angle_calculation = 1;
								set_distance_calculation = 1;
								timeout_counter = 0; // 🌟 状态切换，清零超时计数
            }
//						else if (timeout_counter > 500)
//            {
//                loop_state = LOOP_NORMAL;
//                loop_type = LOOP_NONE;
//                set_distance_calculation = 0;
//                set_angle_calculation = 0;
//            }
            break;

        case LOOP_TURN_IN:
            // 入环一定时间，或者侧边电感回落，认为已进环
						timeout_counter++;
						if(car_angle > LOOP_IN_ANGLE && adc[2] < 60 && car_distance > LOOP_IN_DIST)
						{
								loop_state = LOOP_IN_LOOP;
							
							// 🌟 极其重要：为下一个阶段 (IN_LOOP) 准备干净的战场！
                Clear_Car_Angle();    // 清零，因为 IN_LOOP 需要重新从 0 算到 310 度来判断出口
                Clear_Car_Distance(); // 如果你 IN_LOOP 里不用距离，清不清无所谓，保险起见清掉
                
                // IN_LOOP 只需要测角度，不需要测距离了
                set_distance_calculation = 0; 
                set_angle_calculation = 1;
							  timeout_counter = 0; // 🌟 状态切换，清零超时计数
						}
//						else if (timeout_counter > 300)
//            {
//                loop_state = LOOP_NORMAL;
//                loop_type = LOOP_NONE;
//                set_distance_calculation = 0;
//                set_angle_calculation = 0;
//            }
            break;

        case LOOP_IN_LOOP:
					timeout_counter++; // 🌟 开始计时
            // 在环内，绝对不能看电感，只看陀螺仪角度积分！
            // 转够了 300 度，认为到了出口
            if(abs((int)car_angle) > LOOP_EXIT_ANGLE)
            {
                loop_state = LOOP_EXITING;
								// 🌟 极其关键：将角度清零！让 EXITING 阶段重新从 0 开始测那 30°！
                Clear_Car_Angle(); 
                set_angle_calculation = 1; // 保持角度积分开启
								timeout_counter = 0; // 🌟 状态切换，清零超时计数
            }
//						else if (timeout_counter > 700)
//            {
//                loop_state = LOOP_NORMAL;
//                loop_type = LOOP_NONE;
//                set_distance_calculation = 0;
//                set_angle_calculation = 0;
//            }
            break;

        case LOOP_EXITING:
						timeout_counter++; // 🌟 开始计时
            // 车身回正，电感恢复正常水平
            if(abs((int)car_angle) > LOOP_EXIT_EXTRA_ANGLE)
            {
                // 🌟 2. 角度转够了，再看车身有没有回正 (电感恢复平直)
                if(adc[2] < 80 && adc[0] < 60 && adc[3] < 60)
                {
                    state_timer++;
                    if(state_timer >= 1) // 20ms 防抖
                    {
                        // ✅ 出环大获全胜！
                        loop_state = LOOP_NORMAL;
                        loop_type = LOOP_NONE;
                        state_timer = 0;
                        timeout_counter = 0; // 🌟 开始计时
                        // 🌟 开启出环“冷却期”：关闭角度，开启距离，重置清零
												set_angle_calculation = 0; // 彻底关闭角度积分，防溢出
												Clear_Car_Angle();
                        Clear_Car_Distance();
                        set_distance_calculation = 1; // 开启距离，为 NORMAL 里的冷却期测距！
                    }
                }
                else { state_timer = 0; }
            }
						
//						if (timeout_counter > 300)
//            {
//                loop_state = LOOP_NORMAL;
//                loop_type = LOOP_NONE;
//                set_distance_calculation = 0;
//                set_angle_calculation = 0;
//                state_timer = 0;
//            }
            break;
    }
}

/**
 * @brief 底盘闭环控制
 * 获取偏差并输入方向PID，计算差速输出
 */
void Chassis_Control(void)
{
    const PID_TypeDef *direction_pid; //外环：赛道偏差PID  
		const PID_TypeDef *yaw_rate_pid;  // 内环：角速度PID (输出最终差速)
		static uint8 outer_loop_timer = 0;
		float k = 0;
    //获取方向环PID控制器句柄和偏航角速度环PID句柄
    direction_pid = PID_GetController(PID_DIRECTION);
		yaw_rate_pid = PID_GetController(PID_YAW_RATE); // 新增！获取第4组PID
	
		IMU_Update();
	
		//电池保护
		Battery_Protection_Task();
		/*角速度环测试*/
//		Dynamic_Yaw_Test_Task();
  
		outer_loop_timer++;
    if(outer_loop_timer >= 3) 
    {
        outer_loop_timer = 0;
        
        chassis.current_deviation = Calculate_Deviation();
        // 外环算出力矩，更新 direction_output
        target_yaw_rate = PID_CascadePosition((PID_TypeDef *)direction_pid, chassis.current_deviation, 0);
    }
    //读取当前的偏航角速度
		actual_yaw_rate = imu_data.gz;
		if(set_angle_calculation)
		{
			if (abs(actual_yaw_rate) > 20) 
        {
            car_angle += abs((double)actual_yaw_rate) * 0.002f;
        }
		}
		direction_output = PID_CascadePosition((PID_TypeDef *)yaw_rate_pid, actual_yaw_rate, -target_yaw_rate);
//			direction_output = 0;
		

		k = direction_output * 0.01;
		if(k >= 0)
		{
			k = k > 0.65 ? 0.65 : k;
		}
		else
		{
			k = k < -0.65 ? -0.65 : k;
		}
		if(k >= 0)
		{
			chassis.left_speed  = chassis.target_speed * (1 - k);
			chassis.right_speed = chassis.target_speed * (1 + k * 0.2);
		}
		else
		{
			k *= -1;
			chassis.left_speed  = chassis.target_speed * (1 + k * 0.2);
			chassis.right_speed = chassis.target_speed * (1 - k);
		}
//		chassis.left_speed  = chassis.target_speed - direction_output;
//    chassis.right_speed = chassis.target_speed + direction_output;
    //执行电机控制
		Motor_Control(chassis.left_speed, chassis.right_speed);
}