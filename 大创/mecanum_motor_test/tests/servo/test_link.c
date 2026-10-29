#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "servo_test.h"
#include "servo_link.h"
#include "serial_dma.h"
static SerialLineCallback callback;
static char output[128];
static int moves,runs,autos,stops,keeps;
static int16_t last_angle;
static uint16_t last_speed;
static uint32_t faults;
uint8_t SerialDma_InitBluetooth(SerialLineCallback cb){callback=cb;return 1;}
uint32_t SerialDma_GetRxErrors(void){return faults;}
void SerialDma_Process(void){}
uint8_t SerialDma_Write(const char *s){strcpy(output,s);return 1;}
uint8_t ServoTest_Move(int16_t a,uint16_t s){++moves;last_angle=a;last_speed=s;return 1;}
uint8_t ServoTest_Automatic(int16_t a,uint16_t s){(void)a;(void)s;++autos;return 1;}
uint8_t ServoTest_Continuous(int8_t d,uint16_t s){(void)d;(void)s;++runs;return 1;}
uint8_t ServoTest_Stop(void){++stops;return 1;}
void ServoTest_KeepAlive(void){++keeps;}
ServoTestSnapshot ServoTest_GetSnapshot(void){ServoTestSnapshot s={0};return s;}
const char *ServoTest_GetModeText(void){return "PAUSE";}
static void send(const char *body){char line[80];unsigned char crc=0;for(const char *p=body;*p;++p)crc^=(unsigned char)*p;sprintf(line,"@%s*%02X",body,crc);callback(line);}
int main(void)
{
 ServoLink_Init();send("1,V");assert(strstr(output,"1,OK,SERVO BLE R7,PAUSE,0,0,0"));
 send("2,M,-450,600");assert(moves==1&&last_angle==-450&&last_speed==600);
 char saved[128];strcpy(saved,output);send("2,M,-450,600");assert(moves==1&&!strcmp(output,saved));
 send("2,M,300,600");assert(moves==1&&strstr(output,"ID_COLLISION"));
 send("3,M,0,600");send("4,M,8000,600");send("5,M,300,99");assert(moves==1);
 send("6,C,-1,700");assert(runs==1);send("7,C,2,700");assert(runs==1);
 send("8,A,300,500");assert(autos==1);send("9,K");assert(keeps==1);send("10,S");assert(stops==1);
 callback("@11,M,300,500*00");send("12,,M,300,500");send("13,V,");assert(moves==1);
 ++faults;ServoLink_Process();assert(stops==2);
 puts("PASS: version/status/checksum/ranges/signed commands/duplicate suppression/id collision/STOP/KEEP/UART faults");return 0;
}
