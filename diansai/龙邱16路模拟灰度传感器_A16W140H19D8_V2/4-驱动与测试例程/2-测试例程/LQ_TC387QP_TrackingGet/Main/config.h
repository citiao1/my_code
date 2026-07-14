#ifndef CONFIG_H
#define CONFIG_H


//========================== 关于板子类型 ===============================//
// 若使用LQ_TC2&3x7    v7通用母板 则选择关闭 USE_LQTC3X7_mVA宏定义
// 若使用_LQTC3X7_mVA  v7通用母板 则选择打开 USE_LQTC3X7_mVA宏定义

//#define USE_LQTC3X7_mVA   // 打开则表示使用 V7驱控mini一体板

//========================== 关于电机 ==================================//
// 若要使用7843或7971电机驱动，取消下面这行的注释，并注释掉#define USEDRV8701这行
// #define USE7843or7971
#define USEDRV8701
//======================== 关于printf重定向 ============================//
#define PRINT_COM           UART0

//========================== 关于屏幕 ==================================//
// 屏幕显示方向选择,V7.0.3已修改为初始化时传参选择
extern unsigned char USE_HORIZONTAL;  // 默认为0
// 屏幕类型选择
// 不同IPS屏幕定义不同的分辨率
#define USE_QSPI 1 // 0不使用硬件SPI接线方式,1硬SPI   如果使用硬件SPI  则CS接P21_2  SCK接P00_2   SDI接P00_1  DC接P20_12  RST接P20_13

// 以下是各种屏幕类型的定义，根据实际需求取消对应屏幕类型的注释，并注释掉其他屏幕类型的定义
#define IPS114
// #define IPS130
// #define IPS154
// #define IPS200
// #define TFT18
// #define TFT20
// #define OLED

#endif
