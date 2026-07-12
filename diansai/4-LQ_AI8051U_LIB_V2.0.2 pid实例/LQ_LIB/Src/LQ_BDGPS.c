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
#include "LQ_BDGPS.h"

#include "include.h"


Position_t point_p;

_SaveData Save_Data;   //在串口中断中接收数据



double my_strtod(const char *str)
{
  double result = 0.0;
  double fraction = 1.0;
//  int sign = 1;
//  int exponent = 0;
//  int exp_sign = 1;
//  int exp_num = 0;

  signed char sign = 1;
  signed char exponent = 0;
  signed char exp_sign = 1;
  signed char exp_num = 0;
    
  while (isspace(*str)) str++;  // 跳过空白字符

  // 处理正负号
  if (*str == '-' || *str == '+')
  {
    sign = (*str++ == '-' ? -1 : 1);
  }

  // 解析整数部分
  while (isdigit(*str))
  {
    result = result * 10 + (*str++ - '0');
  }

  // 处理小数点
  if (*str == '.')
  {
    str++;
    while (isdigit(*str))
    {
      fraction /= 10;
      result += (*str++ - '0') * fraction;
    }
  }

  // 处理指数部分
  if (*str == 'e' || *str == 'E')
  {
    str++;
    // 处理指数的正负号
    if (*str == '-' || *str == '+')
    {
      exp_sign = (*str++ == '-' ? -1 : 1);
    }
    exp_num = 0;
    while (isdigit(*str))
    {
      exp_num = exp_num * 10 + (*str++ - '0');
    }
    exponent = exp_sign * exp_num;
  }

  while (exponent > 0)
  {
    result *= 10;
    exponent--;
  }
  while (exponent < 0)
  {
    result /= 10;
    exponent++;
  }

  return sign * result;
}

 
 

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：void parseGpsBuffer(void);
@功能说明：数据解析
@参数说明：无
@函数返回：无
@修改时间：2022/02/24
@备    注：将串口收到的数据分门别类的保存到结构体中
@           $GNRMC,130416.000,A,2236.91843,N,11359.19128,E,0.001,306.28,090720,,,A*4F
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
void parseGpsBuffer(void)
{
  char *subString;
  char *subStringNext;
  char i = 0;
  if (Save_Data.isGetData)      //如果字符串不为空
  {
    for (i = 0 ; i <= 8 ; i++)  //循环7次
    {
      if (i == 0)
      {
        if ((subString = strchr(Save_Data.GPS_Buffer, ',')) == NULL)  //没有检测到逗号
          while(1);             //解析错误
      }
      else  //检测到逗号，返回逗号的位置
      {
        subString++;    //位置加1（定位到逗号的后一位）
        if ((subStringNext = strchr(subString, ',')) != NULL)//定位下一个逗号的位置
        {
          char usefullBuffer[2];
          switch(i)
          {
          case 1:
              memcpy(Save_Data.UTCTime, subString, subStringNext - subString);       //两个逗号之间为 时间信息 并转换成北京时间
              Save_Data.UTCTime[1] = Save_Data.UTCTime[1]+ 8;
              if(Save_Data.UTCTime[1] > '9')
              {
                Save_Data.UTCTime[0]++;
                if(Save_Data.UTCTime[0] == '3')
                  Save_Data.UTCTime[0] = '0';
                Save_Data.UTCTime[1] = (Save_Data.UTCTime[1] % '9') + '0'-1;
              }
              break;//结束switch
          case 2:memcpy(usefullBuffer,            subString, subStringNext - subString);break; //数据是否有效标志
          case 3:memcpy(Save_Data.latitude,       subString, subStringNext - subString);break;  //获取纬度信息
          case 4:memcpy(Save_Data.N_S,            subString, subStringNext - subString);break;  //获取N/S
          case 5:memcpy(Save_Data.longitude,      subString, subStringNext - subString);break;  //获取经度信息
          case 6:memcpy(Save_Data.E_W,            subString, subStringNext - subString);break;  //获取E/W
          case 7:memcpy(Save_Data.speed,          subString, subStringNext - subString);break;  //获取速度
          case 8:memcpy(Save_Data.direction,      subString, subStringNext - subString);break;  //获取方向
          default:break;
          }
          subString = subStringNext;    //下一个逗号位置给第一个指针，
          Save_Data.isParseData = 1; //手动给真值，（数据是否解析完成）
          //          if(usefullBuffer[0] == 'A')
          //            Save_Data.isUsefull = true;
          //          else if(usefullBuffer[0] == 'V')
          //            Save_Data.isUsefull = false;
        }
        else
        {
//          while(1); //解析错误
           UART1_PutStr("北斗数据解析错误\n");
        }
      }
    }
  }
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：char Get_IntData(int* Lon_Z, int* Lon_X, int* Lat_Z, int* Lat_X);
@功能说明：经纬度原始值
@参数说明：Lon_Z（经度整数），Lon_X（经度小数），Lat_Z（纬度整数）,Lat_X（纬度小数）
@函数返回：无
@修改时间：2022/02/24
@备    注：将结构体中的字符串转换成数据，有两种方式，#if 1：以小数点为分界。#if 0：自己手动调节
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
char Get_IntData(int* Lon_Z, int* Lon_X, int* Lat_Z, int* Lat_X)
{
#if 1
  //判断纬度值中是否有‘.'有:说明有数据 例如：3946.99715  变化范围在后六位中变化，最后一位不稳定可舍去。取值为3946和99715
  if((strstr(Save_Data.latitude, ".")) != NULL)
  {
    sscanf(Save_Data.latitude, "%d.%d",&(* Lat_Z), &(* Lat_X));
  }
  else
    return 0;
  //经度原理同上 例如：11628.32198  取值为11628 和 32198
  if((strstr(Save_Data.longitude, ".")) != NULL)
  {
    sscanf(Save_Data.longitude, "%d.%d",&(* Lon_Z), &(* Lon_X));
  }
  else
    return 0;
  return 1;
#else
  unsigned num=0;
  //判断纬度值中是否有‘.'有:说明有数据 例如：3946.99715  变化范围在后六位中变化，最后一位不稳定可舍去。
  if((strstr(Save_Data.latitude, ".")) != NULL)
  {
    *Lat_Z = (Save_Data.latitude[0] - '0')*100 + (Save_Data.latitude[1] - '0')*10 + (Save_Data.latitude[2] - '0');
    num    = (Save_Data.latitude[3] - '0')*100000 + (Save_Data.latitude[5] - '0')*10000 + (Save_Data.latitude[6] - '0')*1000 + \
             (Save_Data.latitude[7] - '0')*100 + (Save_Data.latitude[8] - '0')*10 + (Save_Data.latitude[9] - '0');
    *Lat_X = num;       //取值为394 和 699715
  }
  else
    return 0;
  //经度原理同上 例如：11628.32198
  if((strstr(Save_Data.longitude, ".")) != NULL)
  {
    *Lon_Z = (Save_Data.longitude[0] - '0')*1000 + (Save_Data.longitude[1] - '0')*100 + \
             (Save_Data.longitude[2] - '0')*10 + (Save_Data.longitude[3] - '0');
    num    = (Save_Data.longitude[4] - '0')*100000 + (Save_Data.longitude[6] - '0')*10000 + (Save_Data.longitude[7] - '0')*1000 + \
             (Save_Data.longitude[8] - '0')*100 + (Save_Data.longitude[9] - '0')*10 + (Save_Data.longitude[10] - '0');
    *Lon_X = num;       //取值为1162 和 832198
  }
  else
    return 0;
  return 1;
#endif
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：char Get_DoubleData(double* Lon, double* Lat)
@功能说明：经纬度字符串转化成浮点数
@参数说明：Lon（经度），Lat（纬度）
@函数返回：0:转化失败 1：转化成功
@修改时间：2022/02/24
@备    注：
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
char Get_DoubleData(_SaveData * BDdata, double *Lon ,double *Lat)
{
    double gW,gJ;
    gJ = my_strtod(BDdata->longitude);
    gW = my_strtod(BDdata->latitude);
    if(gW == 0.0)
        return 0;
    else
        *Lon = gJ/100.0;
    if(gJ == 0.0)
        return 0;
    else
        *Lat = gW/100.0;
    return 1;
}

