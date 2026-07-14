#include "LQ_Wireless_Config.h"
#include "lq_include.h"
/********************************************************************************
 * @brief   发送单条 AT 指令.
 * @param   temp_buf : 缓存数组.
 * @param   cmd      : AT 指令.
 * @param   param    : 参数.
 * @return  none.
 * @date    2026-01-15
 ********************************************************************************/
void send_single_at_cmd(char *temp_buf, const char *cmd, const char *param)
{
    memset(temp_buf, 0, WLS_BUF_LEN);     // 清空缓存数组
    sprintf(temp_buf, cmd, param);        // 拼接AT指令
    UART_PutStr(WLS_UART_PORT, temp_buf); // 发送AT指令
    Delay_Ms(300);                        // 延时
}

/********************************************************************************
 * @brief   图传模块 AT 指令配置.
 * @param   none.
 * @return  none.
 * @date    2026-01-15
 ********************************************************************************/
void lq_WLS_config(void)
{
    char temp[WLS_BUF_LEN];

    PIN_InitConfig(WLS_WIFI_IO1, PIN_MODE_OUTPUT, 1);          // 初始化 IO1 引脚
    UART_InitConfig(WLS_UART_RX_PIN, WLS_UART_TX_PIN, WLS_UART_BAUD); //   初始化 UART1下载口的串口
    PIN_Write(WLS_WIFI_IO1, 0);                                // 拉低 IO1 引脚, 进入配置模式
    Delay_Ms(500);
    // 发送 AT 指令
    send_single_at_cmd(temp, "AT+REV_MODE=%s\r\n",          WLS_REV_MODE);          // 设置 回传模式
    send_single_at_cmd(temp, "AT+SEND_MODE=%s\r\n",         WLS_SEND_MODE);         // 设置 发送模式
    send_single_at_cmd(temp, "AT+WIFI_SSID=%s\r\n",         WLS_WIFI_SSID);         // 设置 WIFI 名称
    send_single_at_cmd(temp, "AT+WIFI_PASSWORD=%s\r\n",     WLS_WIFI_PASSWORD);     // 设置 WIFI 密码
    send_single_at_cmd(temp, "AT+WIFI_TRAN_MODE=%s\r\n",    WLS_WIFI_TRAN_MODE);    // 设置 WIFI 传输模式
    send_single_at_cmd(temp, "AT+UDP_IP=%s\r\n",            WLS_UDP_IP);            // 设置 UDP IP
    send_single_at_cmd(temp, "AT+UDP_PORT=%s\r\n",          WLS_UDP_PORT);          // 设置 UDP 端口号
    send_single_at_cmd(temp, "AT+UDP_MY_PORT=%s\r\n",       WLS_UDP_MY_PORT);       // 设置 UDP 本地端口
    send_single_at_cmd(temp, "AT+BLUETOOTH_NAME=%s\r\n",    WLS_BT_NAME);           // 设置 蓝牙名称
    send_single_at_cmd(temp, "AT+UART_BAUD=%s\r\n",         WLS_UART_BAUDRATE);     // 设置 UART 波特率
    /* 一般用不上这三个配置*/
    // send_single_at_cmd(temp, "AT+CLIENT_IP=%s\r\n", WLS_CLIENT_IP);
    // send_single_at_cmd(temp, "AT+CLIENT_PORT=%s\r\n", WLS_CLIENT_PORT);
    // send_single_at_cmd(temp, "AT+SERVER_PORT=%s\r\n", WLS_SERVER_PORT);

    // 拉高 IO1 引脚, 退出配置模式
    PIN_Write(WLS_WIFI_IO1, 1);
}
