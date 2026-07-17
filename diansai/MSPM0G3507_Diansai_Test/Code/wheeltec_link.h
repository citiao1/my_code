#ifndef WHEELTEC_LINK_H
#define WHEELTEC_LINK_H

#include <stdint.h>

/* 收到一整行命令后由传输层回调应用层；line 只在回调返回前有效。 */
typedef void (*WheeltecLineHandler)(char *line);

/* 初始化 PA10/PA11 UART0：9600、8N1、收发 FIFO。 */
void WheeltecLink_Init(void);

/* 非阻塞读取 RX FIFO；每收到一个 CR/LF 结尾的完整命令便调用 handler。 */
void WheeltecLink_Poll(WheeltecLineHandler handler);

/* 轮询 UART0 TX DMA 状态，并从软件环形队列提交下一段连续数据。 */
void WheeltecLink_ServiceTx(void);

/*
 * 将一个完整文本帧放入 1024 字节环形队列。空间不足时整帧拒绝并返回 0，
 * 从而避免半帧破坏上位机协议同步。
 */
uint8_t WheeltecLink_SendText(const char *text);

#endif
