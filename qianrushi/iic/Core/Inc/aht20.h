
#ifndef IIC_AHT20_H
#define IIC_AHT20_H
#include "i2c.h"
#include "my_function.h"
void AHT20_Init(void);
void AHT20_Read(float *Temperature,float *Humidity);
void AHT20_Measure();
void AHT20_Get();
void AHT20_Analysis(float *Temperature,float *Humidity);
#endif //IIC_AHT20_H