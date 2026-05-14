#ifndef LIB_VERSION_H
#define LIB_VERSION_H

#include "uni_iot.h"

const char* const Mon_str[13]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

int firmware_ver = 0; //firmware_ver format is: YYYYMMDDHH

static char build_date[] = __DATE__;
static char build_time[] = __TIME__;

static inline void uni_print_version(void)
{
    char tmp_mon[4];
    char tmp_year[8];
    char tmp_2 [4];
    int tmp_val = 0;
    int i = 0;
   
    //get the year
    tmp_year[0]=build_date[7];
    tmp_year[1]=build_date[8];
    tmp_year[2]=build_date[9];
    tmp_year[3]=build_date[10];
    tmp_year[4]='\0';
    tmp_val = uni_strtol(tmp_year, NULL, 10);
    firmware_ver += tmp_val*1000000;
    
    //get the month
    tmp_val = 0;
    uni_strncpy(tmp_mon,build_date,3);
    tmp_mon[3] = '\0';
    
    for(i = 0;i < 12; i++)
    {
        if(!uni_strcasecmp(Mon_str[i],tmp_mon))
        {
            tmp_val = i+1;
            break;
        }
    }
    firmware_ver += tmp_val*10000;

    //get the date
    tmp_val = 0;
    tmp_2[0]=build_date[4];
    tmp_2[1]=build_date[5];
    tmp_2[2]='\0';
    tmp_val = uni_strtol(tmp_2, NULL, 10);
    firmware_ver += tmp_val*100;

    //get the hour
    tmp_val = 0;
    tmp_2[0]=build_time[0];
    tmp_2[1]=build_time[1];
    tmp_2[2]='\0';
    tmp_val = uni_strtol(tmp_2, NULL, 10);
    firmware_ver += tmp_val;
    printf("\r\n");
    printf("*|**||*|\\*||**||**/'\\   >>Unione Version : %s \r\n", UNI_CROW_VERSION);
    printf("*\\_//**|*\\||**||**\\_/   >>Build Data     : %d \r\n", firmware_ver);
    printf("\r\n");
}

#endif
