/*******************************************************************************
 * @file                LQ_1306_motor.c
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
 
#include "LQ_1306_motor.h"

uint8_t encoder_1306_500ms[] = {0x00, 0x01, 0x00, 0x02, 0x0A};          // 每500ms获取一次编码器数据
uint8_t encoder_1306_1s[] = {0x00, 0x01, 0x00, 0x04, 0x0A};             // 每1s获取一次编码器数据
uint8_t encoder_1306_2s[] = {0x00, 0x01, 0x00, 0x06, 0x0A};             // 每2s获取一次编码器数据
uint8_t pid_1306_close_encoder_send[] = {0x00, 0x02, 0x00, 0x00, 0x0A}; // 关闭编码器发送
uint8_t pid_1306_pid[] = {0x00, 0x06, 0x00, 0x00, 0x0A};                // 获取pid参数
uint16_t data_len_1306 = 5;                                             // 串口帧数据大小

// 1306数据接收
uint8_t uart_recv_buf[UART_RECV_BUF_LEN]; // 字符缓存
uint8_t buf_index = 0;                    // 缓存下标
int16_t recv_num = 0;                     // 最终解析出的数值(-450 ~ 450)
uint8_t frame_ok = 0;                     // 帧接收完成标志

static void LQ_UART0_IT_Handler(void);   // 串口中断回调函数声明 仅限于本文件作用


/*************************************************************************
 * @brief   初始化PWM 串口 GPIO
 *
 * @param    none
 * @retval   none
 *************************************************************************/
void board_1306_init()
{
    // 初始化PWM
    LQConfig_PWM_InitTypeDef_t pwm_init = {
        .DivideRatio = DL_TIMER_CLOCK_DIVIDE_1,     // 输入时钟分割器 1 分频 80MHz / 1 = 80MHz
        .Prescaler = 4 - 1,                         // 分频器 4 分频 80MHz / 4 = 20MHz
        .Period = 1000 - 1,                         // 重载值 1000
        .PwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN_UP, // 边沿对齐 上升沿有效
        .startTimer = false                         // 不启动定时器
    };
    // 计算出 PWM 频率为 20MHz / 1000 = 20000Hz = 20KHz

    LQ_TIMER_PWMInit(LQ_TIMERA_1, &pwm_init);              // 初始化 PWM
    LQ_TIMER_EnablePWMChannel(LQ_TIMERA1_PWM_CH1_Pin_B_3); // 使能 PWM 输出引脚
    LQ_TIMER_Start(LQ_TIMERA_1);                           // 启动定时器

    // DIR初始化
    LQ_GPIO_Pin_Init(GPIO_Pin_B_4, GPIO_MODE_OUTPUT_PP, GPIO_RESISTOR_PULL_UP);

    // 串口0初始化
    LQConfig_UART_InitTypeDef_t uart0_init = {
        .Tx = UART0_TX_Pin_A_10,                  // 串口0 TX引脚
        .Rx = UART0_RX_Pin_A_11,                  // 串口0 RX引脚
        .BaudRate = 115200,                       // 波特率
        .Mode = DL_UART_MODE_NORMAL,              // 串口模式设置为正常模式即可
        .Direction = DL_UART_DIRECTION_TX_RX,     // 通信方向设置 发送和接收
        .StopBits = DL_UART_STOP_BITS_ONE,        // 停止位设置为 1 位
        .Parity = DL_UART_PARITY_NONE,            // 无奇偶校验
        .FlowControl = DL_UART_FLOW_CONTROL_NONE, // 无硬件流控制
        .WordLength = DL_UART_WORD_LENGTH_8_BITS, // 数据位设置为 8 位
    };
    // 串口初始化
    LQ_UART_Init(LQ_UART0, &uart0_init);
    // 串口中断配置
    LQ_UART_ITConfig(LQ_UART0, DL_UART_INTERRUPT_RX, NVIC_Priority_NONE);
    // 使能串口中断接收
    LQ_UART_EnableIT(LQ_UART0);

    // 设置串口0接收中断回调
    LQ_UART_SetRxCallback(LQ_UART0, LQ_UART0_IT_Handler);
}

/*************************************************************************
 * @brief    UART0串口中断函数
 *
 * @param    none
 * @retval   字符数组
 *************************************************************************/
static void LQ_UART0_IT_Handler(void)
{
    uint8_t data = LQ_UART_IT_RecvByte(LQ_UART0);

    // 检测换行/回车：代表一帧数据接收完毕
    if (data == '\r' || data == '\n')
    {
        if (buf_index > 0)
        {
            frame_ok = 1; // 置帧完成标志
        }
    }
    // 非结束符：存入缓存，防止溢出
    else if (buf_index < UART_RECV_BUF_LEN - 1)
    {
        uart_recv_buf[buf_index++] = data;
    }
}

/*************************************************************************
 * @brief    ASCII字符串转int16_t数值（支持正负号）
 *
 * @param    str:   字符串首地址
 * @retval   转换后的数值
 * @example     StrToInt(uart_recv_buf[])
 *************************************************************************/
int16_t StrToInt(uint8_t *str)
{
    int16_t num = 0;
    uint8_t sign = 0; // 0:正数  1:负数
    uint8_t i = 0;

    // 判断正负号
    if (str[0] == '-')
    {
        sign = 1;
        i = 1;
    }
    else if (str[0] == '+')
    {
        sign = 0;
        i = 1;
    }

    // 逐位计算数值
    while (str[i] != '\0' && str[i] >= '0' && str[i] <= '9')
    {
        num = num * 10 + (str[i] - '0');
        i++;
    }

    if (sign)
    {
        num = -num;
    }
    return num;
}

// ======================== 电机控制部分  ==========================

/*************************************************************************
 * @brief      电机方向占空比控制
 *
 * @param       cmd         命令格式
 *
 * @example     motor_1306_set(1,500)  dir取0（反转）或1（正转） pwm取0-1000
 * @note        dir 取 0（反转）或1（正转），pwm 取 0-1000，超出范围将自动截断。
 *************************************************************************/
void motor_1306_set(uint8_t dir, uint16_t pwm)
{
    LQ_GPIO_WritePin(GPIO_Pin_B_4, dir);
    LQ_TIMER_PWMSetCaptureCompare(LQ_TIMERA1_PWM_CH1_Pin_B_3, pwm);  
}

/*************************************************************************
 * @brief       uart0命令接收 具体接收代码在UAER0接收中断里，此处仅作参考
 *
 * @param       none
 *
 * @example    uart0_receive()
 *************************************************************************/
void uart0_receive()
{
    if (frame_ok == 1)
    {
        uart_recv_buf[buf_index] = '\0'; // 字符串补结束符
        recv_num = StrToInt(uart_recv_buf);
        frame_ok = 0;
        buf_index = 0; // 重置下标，准备下一帧
    }
}

/*************************************************************************
 * @brief       uart0命令发送 获取电机各项数据
 *
 * @param       cmd         命令格式
 *
 * @example     uart0_1306_send(encoder_receive_500ms)
 *************************************************************************/
void uart0_1306_sub_data(uint8_t cmd)
{
    if (cmd == encoder_receive_500ms)
        LQ_UART_SendBuffer(LQ_UART0, encoder_1306_500ms, data_len_1306);
    else if (cmd == encoder_receive_1s)
        LQ_UART_SendBuffer(LQ_UART0, encoder_1306_1s, data_len_1306);
    else if (cmd == encoder_receive_2s)
        LQ_UART_SendBuffer(LQ_UART0, encoder_1306_2s, data_len_1306);
    else if (cmd == pid_receive)
        LQ_UART_SendBuffer(LQ_UART0, pid_1306_pid, data_len_1306);
    else if (cmd == encoder_close_receive)
        LQ_UART_SendBuffer(LQ_UART0, pid_1306_close_encoder_send, data_len_1306);
}
/*************************************************************************
 * @brief       uart0命令发送 修改PID各项数据
 * @param       cmd         命令格式
                                kx          修改的数据
 * @example    uart0_1306_pid_change(pid_change_P,0.56)将KP修改为0.56
 *************************************************************************/
void uart0_1306_pid_change(uint8_t cmd, float kx)
{
    if (cmd == pid_change_P)
    {
        uint16_t kx_int = (uint16_t)(kx * 100);
        // 二进制帧，每个元素1字节，对应你说的00、03、数值、0A
        uint8_t frame[5] = {
            0x00,
            0x03,
            (kx_int >> 8) & 0xFF, // 高8位
            kx_int & 0xFF,        // 低8位
            0x0A};
        // 固定5个字节直接发送
        LQ_UART_SendBuffer(LQ_UART0, frame, data_len_1306);
    }
    else if (cmd == pid_change_I)
    {
        uint16_t kx_int = (uint16_t)(kx * 100);
        // 二进制帧，每个元素1字节，对应你说的00、03、数值、0A
        uint8_t frame[5] = {
            0x00,
            0x04,
            (kx_int >> 8) & 0xFF, // 高8位
            kx_int & 0xFF,        // 低8位
            0x0A};
        // 固定5个字节直接发送
        LQ_UART_SendBuffer(LQ_UART0, frame, data_len_1306);
    }
    else if (cmd == pid_change_D)
    {
        uint16_t kx_int = (uint16_t)(kx * 100);
        // 二进制帧，每个元素1字节，对应你说的00、03、数值、0A
        uint8_t frame[5] = {
            0x00,
            0x05,
            (kx_int >> 8) & 0xFF, // 高8位
            kx_int & 0xFF,        // 低8位
            0x0A};
        // 固定5个字节直接发送
        LQ_UART_SendBuffer(LQ_UART0, frame, data_len_1306);
    }
}
/*************************************************************************
 * @brief       uart0命令发送 在线修改转速
 *
 * @param       cmd         命令格式
                                speed       修改的转速  limit 0-127
 *
 * @example    uart0_1306_speed(motor_speed,20)将转速设置为20rps
                             uart0_1306_speed(motor_stop,0)停止电机 此时speed参数无效
 *************************************************************************/
void uart0_1306_speed(uint8_t cmd, uint8_t speed)
{
    if (cmd == motor1306_speed)
    {
        // 二进制帧，每个元素1字节，对应你说的00、03、数值、0A
        uint8_t frame[5] = {
            0x00,
            0x09,
            (speed >> 8) & 0xFF, // 高8位
            speed & 0xFF,        // 低8位
            0x0A};
        // 固定5个字节直接发送
        LQ_UART_SendBuffer(LQ_UART0, frame, data_len_1306);
    }
    else if (cmd == motor1306_stop)
    {
        // 二进制帧，每个元素1字节，对应你说的00、03、数值、0A
        uint8_t frame[5] = {
            0x00,
            0x0A,
            0x00,
            0x00,
            0x0A};
        // 固定5个字节直接发送
        LQ_UART_SendBuffer(LQ_UART0, frame, data_len_1306);
    }
}
