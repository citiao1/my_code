/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
 【平    台】北京龙邱智能科技STC32位核心板
 【编    写】龙邱科技
 【E-mail  】chiusir@163.com
 【软件版本】V1.0 版权所有，单位使用请先联系授权
 【相关信息参考下列地址】
 【网    站】http://www.lqist.cn
 【淘宝店铺】http://longqiu.taobao.com
 --------------------------------------------------------------------------------
 【  IDE  】 keil C251 V5.60
 【Target 】 STC32G/STC8051U/AI8051U 32位模式
 【SYS CLK】 42 MHz使用内部晶振
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
#ifndef   _LQ_BD_H_
#define   _LQ_BD_H_

#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define  ENABLE_GPS      0 //使能GPS模块接收，0为关闭

extern  char *  strstr ( const char *, const char * );     /* 由于C251 C库中没有该函数原型，需要声明在外部 */

typedef enum
{
    gN=0,
    gS=1,
}lat;

typedef enum
{
    gE=0,
    gW=1,
}lon;

typedef struct {
        double PX;                 //坐标值
        double PY;
        double Direction;         //车头朝向（方向）
}Position_t;

typedef struct
{
  char GPS_Buffer[128];         //完整数据
  char isGetData;               //是否获取到GPS数据
  char UTCTime[11];             //UTC时间
  char isParseData;             //是否解析完成
  char latitude[11];            //纬度
  char N_S[2];                  //N/S
  char longitude[12];           //经度
  char E_W[2];                  //E/W
  char isUsefull;               //定位信息是否有效
  char speed[6];                //速度 单位：节
  char direction[6];            //方向
} _SaveData;

extern _SaveData Save_Data;
extern Position_t point_p;

char Get_DoubleData(_SaveData *BDdata, double *Lon ,double *Lat);
char Get_IntData(int* Lon_Z, int* Lon_X, int* Lat_Z, int* Lat_X);
int BD_getdata(Position_t *Current_Point);




#endif



