/*---------------------------------------------------------------------*/
/* --- Web: www.STCAI.com ---------------------------------------------*/
/*---------------------------------------------------------------------*/

#ifndef __AI8051U_COMPARE_H
#define __AI8051U_COMPARE_H

#include "config.h"

//========================================================================
//                              定义声明
//========================================================================

#define    CMP_P_P46    0x00    //输入正极性选择, 0: 选择内部P4.6做正输入
#define    CMP_P_P50    0x01    //输入正极性选择, 1: 选择内部P5.0做正输入
#define    CMP_P_P51    0x02    //输入正极性选择, 2: 选择内部P5.1做正输入
#define    CMP_P_ADC    0x03    //输入正极性选择, 3: 由ADC_CHS[3:0]所选择的ADC输入端做正输入.
#define    CMP_N_P44    0x00    //输入负极性选择, 0: 选择外部P4.4做输入.
#define    CMP_N_GAP    0x01    //输入负极性选择, 1: 选择内部BandGap电压BGv做负输入.

#define    CMPO_P45()    CMPO_S = 0    //结果输出到P4.5.
#define    CMPO_P41()    CMPO_S = 1    //结果输出到P4.1.

typedef struct
{ 
    u8    CMP_EN;           //比较器允许或禁止,   ENABLE,DISABLE
    u8    CMP_P_Select;     //比较器输入正极选择, CMP_P_P46/CMP_P_P50/CMP_P_P51/CMP_P_ADC.
    u8    CMP_N_Select;     //比较器输入负极选择, CMP_N_GAP: 选择内部BandGap经过OP后的电压做负输入, CMP_N_P44: 选择P4.4做负输入.
    u8    CMP_Outpt_En;     //允许比较结果输出,   ENABLE,DISABLE
    u8    CMP_InvCMPO;      //比较器输出取反, ENABLE,DISABLE
    u8    CMP_100nsFilter;  //内部0.1us滤波,  ENABLE,DISABLE
    u8    CMP_OutDelayDuty; //0~63, 比较结果变化延时周期数
} CMP_InitDefine; 

void CMP_Inilize(CMP_InitDefine *CMPx);

#endif
