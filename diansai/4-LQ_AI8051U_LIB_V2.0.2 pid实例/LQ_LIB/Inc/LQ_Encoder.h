/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
 ��ƽ    ̨�������������ܿƼ�STC32λ���İ�
 ����    д������Ƽ�
 ��E-mail  ��chiusir@163.com
 �������汾��V1.0 ��Ȩ���У���λʹ��������ϵ��Ȩ
 �������Ϣ�ο����е�ַ��
 ����    վ��http://www.lqist.cn
 ���Ա����̡�http://longqiu.taobao.com
 --------------------------------------------------------------------------------
 ��  IDE  �� keil C251 V5.60
 ��Target �� STC32G/STC8051U/AI8051U 32λģʽ
 ��SYS CLK�� 42 MHzʹ���ڲ�����
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
#ifndef __LQ_ENC_H_
#define __LQ_ENC_H_

#include "config.h"

#define Enc_1 1
#define Enc_2 2

short Read_Encoder(u8 encno);
void Timer_EncInit(u8 tim_x);



#endif
