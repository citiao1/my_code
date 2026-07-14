#ifndef _LQ_BD_H_
#define _LQ_BD_H_

typedef enum
{
    N = 0,
    S
} lat;
typedef enum
{
    E = 0,
    W
} lon;

typedef struct
{
    double PX; // 坐标值
    double PY;
    double Direction; // 车头朝向（方向）
} Position_t;

typedef struct SaveData
{
    char GPS_Buffer[128]; // 完整数据
    char isGetData;       // 是否获取到GPS数据
    char UTCTime[11];     // UTC时间
    char isParseData;     // 是否解析完成
    char latitude[11];    // 纬度
    char N_S[2];          // N/S
    char longitude[12];   // 经度
    char E_W[2];          // E/W
    char isUsefull;       // 定位信息是否有效
    char speed[6];        // 速度 单位：节
    char direction[6];    // 方向
} _SaveData;

extern _SaveData Save_Data;
extern Position_t point_p;
char Get_DoubleData(_SaveData *data, double *Lon, double *Lat);
char Get_IntData(int *Lon_Z, int *Lon_X, int *Lat_Z, int *Lat_X);
int BD_getdata(Position_t *Current_Point);
void Test_BD1202(void);

#endif
