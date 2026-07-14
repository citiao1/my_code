

#ifndef __LQ_CAMERA_H
#define __LQ_CAMERA_H
#include "config.h"
#include "LQ_MT9V034.h"

#define LQMT9V034 // 默认为神眼摄像头
#define USETFT1_8

#define IMAGEH MT9V034_IMAGEH /*!< 摄像头采集高度 */
#define IMAGEW MT9V034_IMAGEW /*!< 摄像头采集宽度 */

/* 使用数组宽高 修改这里即可 */
#ifdef TFT18
#define LCDH 120 /*!< TFT显示高度（用户使用）高度 */
#define LCDW 160 /*!< TFT显示宽度（用户使用）宽度 */
#else
#define LCDH 120 /*!< TFT显示高度（用户使用）高度 */
#define LCDW 188 /*!< TFT显示宽度（用户使用）宽度 */
#endif

/** 图像原始数据存放 */
extern unsigned char Image_Data[IMAGEH][IMAGEW];

/** 压缩后之后用于存放屏幕显示数据  */
extern unsigned char Image_Use[LCDH][LCDW];

/** 二值化后用于OLED显示的数据 */
extern unsigned char Pixle[LCDH][LCDW];
void CAMERA_Reprot(void);                       // 串口上报上位机
void CAMERA_Init(unsigned char fps);            // 摄像头测试例程
void Get_Use_Image(void);                       // 获取需要使用的图像数组
void Get_Bin_Image(unsigned char mode);         // 二值化
int GetOSTU(unsigned char tmImage[LCDH][LCDW]); // 大津法求阈值大小
void Test_CAMERA(void);                         // 摄像头测试例程
// 基于soble边沿检测算子的一种边沿检测
void lq_sobel(unsigned char imageIn[LCDH][LCDW], unsigned char imageOut[LCDH][LCDW], unsigned char Threshold);
// 基于soble边沿检测算子的一种自动阈值边沿检测
void lq_sobelAutoThreshold(unsigned char imageIn[LCDH][LCDW], unsigned char imageOut[LCDH][LCDW]);

#endif
