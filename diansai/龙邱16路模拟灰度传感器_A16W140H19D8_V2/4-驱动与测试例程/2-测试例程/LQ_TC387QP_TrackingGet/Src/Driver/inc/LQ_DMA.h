#ifndef __LQ_DMA_H__
#define __LQ_DMA_H__

#include "lq_include.h"

#include "IfxCpu.h"
#include "IfxDma_Dma.h"
#include "IfxDma.h"
#include "IfxDma_regdef.h"

/**
 * 摄像头 DMA 配置结构体
 */
typedef struct
{
    Ifx_DMA_CH linkedList[2]; // 总是报错，去掉了IFX_ALIGN(256)，实测没有影响
    // IFX_ALIGN(256) Ifx_DMA_CH linkedList[2]; //链表存储。必须与256位地址对齐，否则DMA无法读取
    IfxDma_Dma_Channel chn; // Dma channel handle
} Dma_Camera_t;

/** 摄像头 数据采集完成标志位 */
extern unsigned char Camera_Flag;

/**
 * 摄像头 DMA 配置结构体
 */
extern Dma_Camera_t g_DmaCameraLinkedList;

/* DMA 中断服务函数  */
void DMA_IRQHandler(void);

/* 摄像头DMA传输初始化*/
void DMA_CameraInitConfig(unsigned long srcStartAddr, unsigned long dstStartAddr, unsigned long channel);

/*  摄像头DMA传输开启 */
void DMA_CameraStart(unsigned long channel);

/* DMA传输停止*/
void DMA_CameraStop(unsigned long channel);

/*  摄像头DMA传输中断处理函数*/
void DMA_CameraIRQ(unsigned long channel);

#endif /* __LQ_DMA_H__ */