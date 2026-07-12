#ifndef _LQ_SBUS_H_
#define _LQ_SBUS_H_


#define ENABLE_SBUS   1   //0:Disable SBUS function, 1:Enable SBUS function

extern unsigned char SReadBuff[64];
extern signed int RLR, RUD, LLR, LUD, SW5, SW6, SW7, SW8;
//extern signed int old_RLR, old_RUD, old_LLR, old_LUD, old_SW5, old_SW6, old_SW7, old_SW8;

extern unsigned int Status_flag;
//extern unsigned int CHDATA[18];
extern unsigned char sbus_Rec_Fini_Flag;
//extern unsigned char sbus_receive_flag[5];
extern unsigned char receive_flag;


void UpDate_SBUS(void);



#endif


