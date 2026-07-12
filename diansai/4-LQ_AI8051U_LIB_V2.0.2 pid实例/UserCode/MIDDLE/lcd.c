#include "lcd.h"
#include "inductance.h"
#include "chassis.h"
#include "motor.h"

const char* State_Str[6] = {"NORM", "APPR", "TURN", "IN  ", "EXIT"};
const char* Type_Str[4]  = {"NONE", "LEFT", "RGfHT"};
char txt[30];
int *inductance_values; // 用于存储电感值的全局变量
uint8 selected_param = 0;
float tune_step = 5.0f;  // 默认步进值，可以通过左右键改变
/**
 * @brief 显示电磁传感器数据
 * 显示归一化后的五个电感值
 * @param values 电感值数组 [L1, L2, M, R1, R2]
 */
void Lcd_Display(void)
{
		const PID_TypeDef *dir_pid = PID_GetController(2);
		const PID_TypeDef *v_pid = PID_GetController(1);
		const PID_TypeDef *pid = PID_GetController(3);
    inductance_values = GetInductance(); // 将只读电感值赋值给局部变量

	  sprintf(txt, "L1:%03d %03d M:%03d  ", inductance_values[0],inductance_values[1],inductance_values[2]);
    LCD_ShowStr_16(0, 0, txt, u16_RED_IPS, u16_BLACK);
    
		sprintf(txt, "R1:%03d %03d BV: %04d  ", inductance_values[3],inductance_values[4],current_adc);
    LCD_ShowStr_16(0, 1, txt, u16_RED_IPS, u16_BLACK);
    
//		sprintf(txt,"battery:%05d  ",current_adc);
//		LCD_ShowStr_16(0, 0, txt, u16_RED_IPS, u16_BLACK);
	// 第 1 行：显示 Kp。如果选中了 Kp，前面显示小箭头
//    if (selected_param == 0)
//			sprintf(txt, "-> Kp: %4.3f  Tspeed:%d ", dir_pid->Kp,chassis.target_speed);
//    else
//        sprintf(txt, "   Kp: %4.3f  Tspeed:%d ",dir_pid->Kp,chassis.target_speed);
//    LCD_ShowStr_16(0, 2, txt, u16_RED_IPS, u16_BLACK);
//    
//    // 第 2 行：显示 Kd。如果选中了 Kd，前面显示小箭头
//    if (selected_param == 2) // 为了逻辑清晰，我们用 0 代表 P，2 代表 D
//        sprintf(txt, "-> Kd: %4.3f yr:%4d ", dir_pid->Kd, actual_yaw_rate);
//    else
//        sprintf(txt, "   Kd: %4.3f yr:%4d    ", dir_pid->Kd, actual_yaw_rate);
//    LCD_ShowStr_16(0, 3, txt, u16_RED_IPS, u16_BLACK);
//		/*偏差*/
//		sprintf(txt, "Dev:%d Tyaw:%d Dout:%d   ", chassis.current_deviation,target_yaw_rate,direction_output);
//    LCD_ShowStr_16(0, 4, txt, u16_RED_IPS, u16_BLACK);
//		/*偏航角*/
////		if (selected_param == 1) 
////    {
////        // 选中 1 时，箭头指向 target_yr
////        sprintf(txt, "-> trg_yr:%4d  yr:%4d ", target_yaw_rate, actual_yaw_rate);
////    }
////    else 
////    {
////        // 没选中时，隐藏箭头
////        sprintf(txt, "   trg_yr:%4d  yr:%4d ", target_yaw_rate, actual_yaw_rate);
////    }
////		LCD_ShowStr_16(0, 5, txt, u16_RED_IPS, u16_BLACK);
//		/*左右目标速度*/
		sprintf(txt,"LTS:%d RTS:%d stop:%d   ",chassis.left_speed,chassis.right_speed,stop_flag);
		LCD_ShowStr_16(0, 5, txt, u16_RED_IPS, u16_BLACK);
//		sprintf(txt,"EL:%d ER:%d   ",_encoder_L,_encoder_R);
//		LCD_ShowStr_16(0, 6, txt, u16_RED_IPS, u16_BLACK);
//		sprintf(txt,"LPWM:%04d RPWM:%04d ",left_pwm,right_pwm);
//		LCD_ShowStr_16(0, 7, txt, u16_RED_IPS, u16_BLACK);
		sprintf(txt,"dist:%d Cal_angle:%d",(int)car_distance,(int)car_angle);
    LCD_ShowStr_16(0, 7, txt, u16_RED_IPS, u16_BLACK); // 用绿色显示，极其醒目！
		
		sprintf(txt,"State:%s Type:%s   ", State_Str[loop_state], Type_Str[loop_type]);
    LCD_ShowStr_16(0, 6, txt, u16_RED_IPS, u16_BLACK); // 用绿色显示，极其醒目！

/*以下用来测电感值与显示*/
//			sprintf(txt, "L1:%03d %05d  ", inductance_values[0],L1);
//			LCD_ShowStr_16(0, 0, txt, u16_RED_IPS, u16_BLACK);			
//			sprintf(txt, "L2:%03d %05d  ", inductance_values[1],L2);
//			LCD_ShowStr_16(0, 1, txt, u16_RED_IPS, u16_BLACK);
//			sprintf(txt, "M:%03d %05d   ", inductance_values[2],M);
//			LCD_ShowStr_16(0, 2, txt, u16_RED_IPS, u16_BLACK);
//			sprintf(txt, "R2:%03d %05d   ", inductance_values[4],R2);
//			LCD_ShowStr_16(0, 3, txt, u16_RED_IPS, u16_BLACK);
//			sprintf(txt, "R1:%03d %05d   ", inductance_values[3],R1);
//			LCD_ShowStr_16(0, 4, txt, u16_RED_IPS, u16_BLACK);
}

