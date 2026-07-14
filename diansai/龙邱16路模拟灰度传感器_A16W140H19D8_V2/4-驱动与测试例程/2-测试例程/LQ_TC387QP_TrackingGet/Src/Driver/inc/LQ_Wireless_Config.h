#ifndef __LQ_WIFI_CONFIG_H
#define __LQ_WIFI_CONFIG_H

#include "lq_include.h"
/* MCU 接口配置 */
#define WLS_UART_PORT       (UART0)             // 配置串口
#define WLS_UART_RX_PIN     (UART0_RX_P14_1)    // 配置串口引脚
#define WLS_UART_TX_PIN     (UART0_TX_P14_0)
#define WLS_UART_BAUD       (115200ul)          // 配置串口波特率
#define WLS_WIFI_IO1        (P15_0)             // 图传模块 IO1 引脚定义
#define WLS_BUF_LEN         (32)                // 发送数组的长度，默认固定为 32 字节，用户无需修改
/* WiFi WLAN 连接配置 */
#define WLS_WIFI_SSID       ("LQwifi001")       // 将要连接的 WIFI 名称
#define WLS_WIFI_PASSWORD   ("longqiu123")      // 将要连接的 WIFI 密码 必须长度8字符以上.
/* 通信方式配置 */
#define WLS_SEND_MODE       ("WIFI")            // 设置模块无线工作模式. "WIFI":设置发送模式为 WIFI,  "BLUEATOOTH": 设置发送模式为蓝牙.
#define WLS_REV_MODE        ("SPI")             // 设置模块接收通信模式. "SPI":设置接收模式为SPI,  "UART": 设置接收模式为UART.
#define WLS_BT_NAME         ("LQ_BLUETOOTH")    // 将要连接的蓝牙名称.
#define WLS_UART_BAUDRATE   ("115200")          // 将要连接的 UART 波特率.
#define WLS_WIFI_TRAN_MODE  ("udp")             // 设置WIFI传输模式. "tcp_client":TCP 客户端模式, "tcp_server":TCP 服务器模式, "udp":UDP 传输模式。
/* UDP通信配置 */
#define WLS_UDP_IP          ("192.168.137.1")   // 将要连接的 UDP IP
#define WLS_UDP_PORT        ("8080")            // 将要连接的 UDP 远端端口
#define WLS_UDP_MY_PORT     ("4321")            // 将要连接的 UDP 本地(模块上)端口
/* TCP通信配置 */
#define WLS_CLIENT_IP       ("192.168.137.1")   // 将要连接的 TCP 客户端 IP
#define WLS_CLIENT_PORT     ("8080")            // 将要连接的 TCP 客户端端口
#define WLS_SERVER_PORT     ("8080")            // 将要连接的 TCP 服务器端端口


// ----以上为配置模块可能需要根据自己的情况修改的配置项，以下为模块代码实现，用户无需修改----

void lq_WLS_config(void);

#endif
