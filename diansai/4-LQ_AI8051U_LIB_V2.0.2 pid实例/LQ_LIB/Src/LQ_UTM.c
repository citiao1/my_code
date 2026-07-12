#include "include.h"

#include "LQ_UTM.h"
#include "LQ_BDGPS.h"

double rad(double _d) {
    return _d * PI / 180.0;
}

int get_zone(double lon)
{
    int zone = (int)floor((lon + 180.0) / 6.0) + 1;
    if (zone < 1) zone = 1;
    if (zone > 60) zone = 60;
    return zone;
}

struct UTM latlon2utm(struct Point _p) 
{
    double _a = 6378137.0;              // WGS84椭球长半轴
    double _f = 1.0 / 298.257223563;    // WGS84椭球扁率
    double k0 = 0.9996;                 // UTM投影标准纵缩比
    double lon0, _x0, _y0, _e, _e2, _n, _t, _c, _A, _M;   //下划线命名，避免字母与寄存器重名
    int zone;
    struct UTM utm;
    
    zone = get_zone(_p.lon);
    lon0 = (zone - 1) * 6 - 180 + 3;
    _x0 = 500000.0;
    _y0 = _p.lat >= 0 ? 0 : 10000000.0;

    _e = sqrt(_f * (2 - _f));
    _e2 = _e * _e;                                                              //此处使用，8051U的TPFU硬件三角函数加速
    _n = _a / sqrt(1 - _e2 * sin(rad(_p.lat)) * sin(rad(_p.lat)));
    _t = tan(rad(_p.lat)) * tan(rad(_p.lat));
    _c = _e2 * cos(rad(_p.lat)) * cos(rad(_p.lat));
    _A = cos(rad(_p.lat)) * (rad(_p.lon) - rad(lon0));
    _M = _a * ((1 - _e2 / 4 - 3 * _e2 * _e2 / 64 - 5 * _e2 * _e2 * _e2 / 256) * rad(_p.lat)
            - (3 * _e2 / 8 + 3 * _e2 * _e2 / 32 + 45 * _e2 * _e2 * _e2 / 1024) * sin(2 * rad(_p.lat))
            + (15 * _e2 * _e2 / 256 + 45 * _e2 * _e2 * _e2 / 1024) * sin(4 * rad(_p.lat))
            - (35 * _e2 * _e2 * _e2 / 3072) * sin(6 * rad(_p.lat)));


    utm.zone = zone;
    utm.ux = k0 * _n * (_A + (1 - _t + _c) * _A * _A * _A / 6
                     + (5 - 18 * _t + _t * _t + 72 * _c - 58 * _e2) * _A * _A * _A * _A * _A / 120)
            + _x0;
    utm.uy = k0 * (_M + _n * tan(rad(_p.lat)) * (_A * _A / 2
              + (5 - _t + 9 * _c + 4 * _c * _c) * _A * _A * _A * _A / 24
              + (61 - 58 * _t + _t * _t + 600 * _c - 330 * _e2) * _A * _A * _A * _A * _A * _A / 720));

    if (_p.lat < 0) {
        utm.uy += 10000000.0;
    }

    return utm;
}


