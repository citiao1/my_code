#ifndef __APP_DEBUG_H__
#define __APP_DEBUG_H__

/*******************************************************************************
 * @file    app_debug.h
 * @brief   VOFA+ ����ģ��ͷ�ļ���App Layer��
 *
 * @note    ��ģ��ͨ�� UART0��PA10/PA11, 9600bps������λ��ͨ�ţ�����
 *          "��ѭ����ѯ RX + DMA ������ TX" �ܹ������� B����
 *          ���ױ������ж���ִ������ʽ���͵���ϵͳ������
 *
 *          1. FireWater Э�飨���գ�����λ���·� "kp_pos=0.5\n" �Ȳ���ָ��
 *          2. �ı� CSV Э�飨���ͣ�����Ƭ����ʱ�ϱ� "1.2,3.4,5.6\n" �� VOFA+ ��ͼ
 *
 *          �ɵ�����һ����FireWater Э���е� key����
 *          λ�û���kp_pos, kd_pos, om_pos������޷���
 *          ���ٶȻ���kp_yaw, ki_yaw, om_yaw������޷���, imax_yaw�������޷���
 *          �ٶȻ���kp_sl, ki_sl����, kp_sr, ki_sr���ң�
 *          ֱ�У�vx��Ŀ���ٶȣ�, vmax���ٶ����ޣ�, deadzone��ƫ��������,
 *                gyro_off����������Ư��
 *          ʹ�ܿ��أ�en_pos, en_yaw, en_speed��0/1��
 *******************************************************************************/

#include "include.h"
#include "mid_pid.h"

/* ���Դ������ã�����ͨ��ʹ�ã� */
#define APP_DEBUG_UART                 (LQ_UART0)
#define APP_DEBUG_UART_TX              (UART0_TX_Pin_A_10)
#define APP_DEBUG_UART_RX              (UART0_RX_Pin_A_11)
#define APP_DEBUG_UART_BAUD            (9600u)

/**
 * @brief  ��ʼ�����Դ��ڣ�UART0, 9600bps, 8N1��+ TX DMA ͨ��
 * @note   �ܹ�˵����
 *         - RX����ѭ����ѯ FIFO����ʹ���жϣ��ƿ� LQ �ⲻ�ȶ����ж�·����
 *         - TX��DMA + 1024 �ֽڻ��ζ��У���ѭ������ ServiceTx �ƽ�
 *         ���裺
 *         1. LQ_UART_Init ���� UART0 ���������������ʡ����š�8N1��
 *         2. ���� TX/RX FIFO���� RX FIFO ��ֵΪ 1 �ֽڡ�TX FIFO 3/4 ��
 *         3. ���� DMA ͨ�� 0��UART0 TX ���������δ��䡢�ֽڿ��ȡ�Դ������Ŀ��̶�
 *         4. UART0 ���� DMA ����
 */
void APP_DEBUG_Init(void);

/**
 * @brief  ��ѯ UART0 TX DMA ״̬���ӻ��ζ����ύ��һ����������
 * @note   ��������ѭ����Ƶ�����ã����ܱ�֤ TX ���������
 *         ���裺
 *         1. ����һ�� DMA ���ڽ��У�ֱ�ӷ��أ������ƽ� tail ������ dma_len
 *         2. �����ζ��пգ�ֱ�ӷ���
 *         3. ȡ"�� tail �����Ƶ����������"��Ϊ���� DMA ���ȣ����� DMA
 */
void APP_DEBUG_ServiceTx(void);

/**
 * @brief  ��������ȡ RX FIFO��ÿ�յ� '\n' ��β�������н��� FireWater Э��
 * @note   ��������ѭ���и�Ƶ���ã����� 9600 ��������·�Ľ����ӳ�
 *         - '\r' ֱ�Ӷ���
 *         - '\n' �����д��������� key=value ���޸� g_param��
 *         - ������ֱ�Ӷ������ȴ���һ����������ͬ��
 */
void APP_DEBUG_PollRx(void);

/**
 * @brief  ���Ͷ�ͨ�� CSV ���ݣ������������뻷�ζ��У�
 * @param  data  ��������ָ��
 * @param  n     ͨ������
 * @note   ��ʽ��1.2,3.4,5.6\n��VOFA+ �ı�Э��ģʽ��
 *         �����ȸ�ʽ�����ڲ�������������֡���
 */
void APP_DEBUG_SendPlot(float *data, uint8_t n);

/**
 * @brief  ��ʽ�������ַ����������������뻷�ζ��У�
 * @param  fmt  printf ��ʽ�ַ���
 * @note   ���ڴ�ӡ��־���������
 */
void APP_DEBUG_Printf(const char *fmt, ...);

/**
 * @brief  ��ѯ������ VOFA+ ��ͼ���ݣ��� 20ms ����ˢ��
 * @note   Ӧ����ѭ���и�Ƶ���ã��ڲ��ж��Ƿ񵽴����ڣ�δ��ֱ�ӷ���
 *         11 ·���ݣ�
 *           0:�����ۼ�����  1:�����ۼ�����  2:ƫ�����
 *           3:����Ŀ���ٶ�   4:����ʵ���ٶ�   5:����Ŀ���ٶ�
 *           6:����ʵ���ٶ�   7:���� PWM      8:���� PWM
 */
void APP_DEBUG_PollPlot(void);

#endif