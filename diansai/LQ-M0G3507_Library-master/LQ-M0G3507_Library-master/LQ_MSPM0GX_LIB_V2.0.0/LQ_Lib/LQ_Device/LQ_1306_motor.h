/*******************************************************************************
 * @file                LQ_1306_motor.h
 * @brief               本文件是 LQ_MSPM0GX_LIB 软件开源库文件的一部分
 * @copyright           版权所有 (C) 2025-2026 北京龙邱科技有限公司
 * @website             http://www.lqist.cn
 * @taobao              http://longqiu.taobao.com
 *
 * @description         龙邱科技 MSPM0G3507 核心板驱动库声明
 *
 * 开发环境配置:
 *   - 使用环境 : Keil5
 *   - 目标芯片 : MSPM0G3507
 *   - 外置晶振 : 16.000MHz
 *   - 系统时钟 : 80MHz
 *
 * 本文件遵循GPL-3.0开源协议发布，旨在为 MSPM0G3507 芯片嵌入式系统设计提供快速上手开发基于 MSPM0G3507 的应用程序的参考实现
 * 商业用途（包括单位使用）需提前联系作者获得授权
 *
 * GPL-3.0 许可证声明摘要:
 * 1. 允许自由使用、修改、分发本软件
 * 2. 分发修改后的版本时，必须以相同许可证发布
 * 3. 必须保留原始版权声明和许可证信息
 * 4. 不提供任何担保，使用风险自负
 * 5. 完整协议文本请参见项目根目录 LICENSE 文件
 *
 * @author              Guard_Byte
 * @email               chiusir@163.com
 * @version             V2.0.0
 * @update              2026年7月02日
 *******************************************************************************/

#ifndef __LQ_IDAC_MOTOR_H_
#define __LQ_IDAC_MOTOR_H_

#include "include.h"

// 以下宏定义主要用于串口收发命令的判断
#define UART_RECV_BUF_LEN                      (32) // 接收缓存大小，足够存数值字符串

#define DIR_POSITIVE                           (1) // 反转
#define DIR_NEGATIVE                           (0) // 正转

#define encoder_receive_500ms                  (0)
#define encoder_receive_1s                     (1)
#define encoder_receive_2s                     (2)
#define pid_receive                            (3)
#define encoder_close_receive                  (4)

#define pid_change_P                           (5)
#define pid_change_I                           (6)
#define pid_change_D                           (7)

#define motor1306_speed                        (8)
#define motor1306_stop                         (9)

extern uint8_t uart_recv_buf[UART_RECV_BUF_LEN]; // 字符缓存
extern uint8_t buf_index;                        // 缓存下标
extern int16_t recv_num;                         // 最终解析出的数值(-450 ~ 450)
extern uint8_t frame_ok;                         // 帧接收完成标志


void board_1306_init(void);                    // 初始化1306板驱动板
int16_t StrToInt(uint8_t *str);                 // 字符串转整型

void uart0_receive();                          // 串口0接收中断
void uart0_1306_sub_data(uint8_t cmd);         // 串口0接收中断子函数

void motor_1306_set(uint8_t dir, uint16_t pwm);     // 1306板驱动电机
void uart0_1306_pid_change(uint8_t cmd, float kx); // 串口0接收中断子函数，PID参数
void uart0_1306_speed(uint8_t cmd, uint8_t speed);  // 串口0接收中断子函数, 电机转速


#endif