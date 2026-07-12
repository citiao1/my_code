/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
 【平    台】北京龙邱智能科技STC32位核心板
 【编    写】龙邱科技
 【E-mail  】chiusir@163.com
 【软件版本】V1.1 版权所有，单位使用请先联系授权
 【相关信息参考下列地址】
 【网    站】http://www.lqist.cn
 【淘宝店铺】http://longqiu.taobao.com
 --------------------------------------------------------------------------------
 【  IDE  】 keil C251 V5.60
 【Target 】 STC32G/STC8051U/AI8051U 32位模式
 【SYS CLK】 40 MHz使用内部晶振
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/

// 下载时, 选择时钟频率 与"config.h"中宏定义保持一致 默认使用40M主频


#include "include.h"
#include "init.h"
#include "motor.h"
#include "lcd.h"
#include "chassis.h"
#include "inductance.h"

volatile bit flag_20ms_lcd = 0;
volatile bit flag_20ms_vofa;
void main(void)
{
	int vofa_timer = 0;
    System_Init();  /* 系统初始化 必须保留 */
    Global_IRQ_Enable(); // 使能全局中断
    GPIO_LED_Init();
    User_Init(); // 统一初始化所有外设和模块
		Wait_For_Start();
    while (1)
    {
			Emergency_Stop_Task();
			Fan_Smooth_Task();
			if (flag_20ms_lcd == 1)
        {
            flag_20ms_lcd = 0;
            Lcd_Display();
            Key_Tuning_Task(); 
        }
				
				// ===============================================
        // ?? 2. VOFA+ 上位机蓝牙发送任务 (20ms 极速刷新)
        // ===============================================
        if (flag_20ms_vofa == 1)
        {
            flag_20ms_vofa = 0;
            
            // ?? 严格按照 CSV 格式打印，结尾必须带 \n
            // 通道顺序: 
            // CH1: 环岛状态 (0:NORM, 1:APPR, 2:TURN, 3:IN, 4:EXIT)
            // CH2: 环岛类型 (0:NONE, 1:LEFT, 2:RIGHT)
            // CH3: 循迹误差 (-100 ~ 100)
            // CH4: 左轮速
            // CH5: 右轮速
            // CH6: 里程积分
            // CH7: 角度积分
//            printf("%d,%d,%d,%d,%ld,%ld\n", 
//                   loop_state, 
//                   loop_type, 
//                   imu_data.ax, 
//                    imu_data.gy,
//                   (long)car_distance, 
//                   (long)car_angle);
//						printf("%06d,%06d,%06d,%06d,%06d,%06d\n",imu_data.ax,imu_data.ay,imu_data.az,imu_data.gx,imu_data.gy,imu_data.gz);
//							printf("%05d,%05d,%05d,%05d,%05d\n",L1,L2,M,R2,R1);
									 			//printf("%d,%d,%d,%d,%d,%d\n",0,chassis.current_deviation,target_yaw_rate,direction_output,left_pwm,right_pwm);
					//printf("%d,%d\n", current_adc,stop_flag);
					printf("%d,%d,%ld\n",loop_state,loop_type,(long)global_distance);
//					printf("%d,%d,%d,%d,%d,%d,%d,%d,%d\n", chassis.left_speed,chassis.right_speed ,direction_output,_encoder_L,_encoder_R,left_pwm,right_pwm,current_adc,stop_flag);
//					printf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", target_yaw_rate,actual_yaw_rate ,direction_output,_encoder_L,_encoder_R,left_pwm,right_pwm,current_adc,current_fan_pwm,stop_flag);
//						printf("%ld,%ld\n", (long)car_distance, (long)car_angle);//					  printf("%d,%d,%d,%d,%d\n",chassis.target_speed,_encoder_L,_encoder_R,left_pwm,right_pwm);
        }

			LED_Ctrl(LED0, RVS);
    }
}

