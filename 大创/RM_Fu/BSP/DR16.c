#include "DR16.h"
#include "usart.h"
#include "string.h"
#include "iwdg.h"

/*
 *****************************************************************************************
 * DR16 遥控器解析模块（SBUS协议 18字节有效数据）
 *
 * 数据结构：RC_Ctl_t
 * - rc.ch0~ch3：摇杆数据（范围：364~1684，对应-660~+660）
 * - rc.s1/s2 ：拨杆开关（1/3/2 对应 上/中/下）
 * - mouse：鼠标三轴与左右键
 * - key：键盘按键状态（W、A、S、D、Shift、Ctrl等）
 *
 * 函数：
 * 1. GiveSbusBuff：返回DMA接收缓存指针
 * 2. Dbus_to_rc：解析SBUS数据帧，填充RC_CtrlData
 * 3. GetRCData：返回解析后的数据结构指针
 *****************************************************************************************
 */

RC_Ctl_t RC_CtrlData[2];          // [0] 当前帧，[1] 上一帧（用于按键计数）
uint8_t sbus_buff[256];           // SBUS接收缓冲区（DMA使用）
uint16_t temp[6];                 // 临时变量，保存解析中间值
int16_t vx;
int16_t vy;
int16_t vz;
uint16_t key_watch;


/**
 * @brief  获取SBUS接收缓冲区指针（供DMA使用）
 */
uint8_t *GiveSbusBuff(void)
{
    return sbus_buff;
}

/**
 * @brief  将SBUS原始数据解析为RC_Ctl_t结构
 * @param  pData: 指向17字节SBUS数据的指针
 * @retval RC_Ctl_t*: 解析后的遥控器数据结构体指针
 */
RC_Ctl_t *Dbus_to_rc(uint8_t pData[])
{
    // 重新启动DMA接收，准备下一帧数据
    HAL_UARTEx_ReceiveToIdle_DMA(&huart3, sbus_buff, 100);
    __HAL_DMA_DISABLE_IT(huart3.hdmarx, DMA_IT_HT);
    RC_CtrlData[0].rc.ch0 = ((int16_t)pData[0] | ((int16_t)pData[1] << 8)) & 0x07FF;
    RC_CtrlData[0].rc.ch1 = (((int16_t)pData[1] >> 3) | ((int16_t)pData[2] << 5)) & 0x07FF;
    RC_CtrlData[0].rc.ch2 = (((int16_t)pData[2] >> 6) | ((int16_t)pData[3] << 2) |
                          ((int16_t)pData[4] << 10)) &
                         0x07FF;
    RC_CtrlData[0].rc.ch3 = (((int16_t)pData[4] >> 1) | ((int16_t)pData[5] << 7)) &
                         0x07FF;

    RC_CtrlData[0].rc.s1 = ((pData[5] >> 4) & 0x000C) >> 2;
    RC_CtrlData[0].rc.s2 = ((pData[5] >> 4) & 0x0003);

    RC_CtrlData[0].mouse.x = ((int16_t)pData[6]) | ((int16_t)pData[7] << 8);                    //鼠标左右动（右为正）
    temp[0] = ((int16_t)pData[6]) | ((int16_t)pData[7] << 8);
    vx = temp[0];
    RC_CtrlData[0].mouse.y = ((int16_t)pData[8]) | ((int16_t)pData[9] << 8);                    //鼠标上下动（下为正）
    temp[1] = ((int16_t)pData[8]) | ((int16_t)pData[9] << 8);
    vy = temp[1];
    RC_CtrlData[0].mouse.z = ((int16_t)pData[10]) | ((int16_t)pData[11] << 8);                  //鼠标滚轮上下动（上为正）
    temp[2] = ((int16_t)pData[10]) | ((int16_t)pData[11] << 8);
    vz = temp[2];
    RC_CtrlData[0].mouse.press_l = pData[12];
    temp[3] = pData[12];
    RC_CtrlData[0].mouse.press_r = pData[13];
    temp[4] = pData[13];
    *(uint16_t *)&RC_CtrlData[PASSING].key[KEY_PASSING] = (uint16_t)((pData[14]) | (pData[15] << 8));
    temp[5] = (uint16_t)((pData[14]) | (pData[15] << 8));
    if(RC_CtrlData[PASSING].key[KEY_PASSING].shift)
    {
        RC_CtrlData[PASSING].key[KEY_WITH_SHIFT] = RC_CtrlData[PASSING].key[KEY_PASSING];
    }
    else
    {
        memset(&RC_CtrlData[PASSING].key[KEY_WITH_SHIFT], 0 ,sizeof(RC_Ctl_t));
    }
    if(RC_CtrlData[PASSING].key[KEY_PASSING].ctrl)
    {
        RC_CtrlData[PASSING].key[KEY_WITH_CTRL] = RC_CtrlData[PASSING].key[KEY_PASSING];
    }
    else
    {
        memset(&RC_CtrlData[PASSING].key[KEY_WITH_CTRL], 0, sizeof(RC_Ctl_t));
    }

    uint16_t key_now = RC_CtrlData[PASSING].key[KEY_PASSING].keys,
             key_with_shift = RC_CtrlData[PASSING].key[KEY_WITH_SHIFT].keys,
             key_with_ctrl = RC_CtrlData[PASSING].key[KEY_WITH_CTRL].keys,
             key_last_with_shift = RC_CtrlData[LAST].key[KEY_WITH_SHIFT].keys,
             key_last_with_ctrl = RC_CtrlData[LAST].key[KEY_WITH_CTRL].keys;                        //联合体存在bug，暂无法使用


    for(uint16_t i = 0, j = 0x01; i < 16 ; j <<= 1,i++)
    {
        if(i == 4 || i == 5)
        {
            continue;
        }
        if((temp[5] & j) && !(key_with_ctrl & j) && !(key_with_shift & j))
        {
            RC_CtrlData[PASSING].key_count[KEY_PASSING][i]++;
        }
        if ((temp[5] & j) && (key_with_ctrl & j) && !(key_with_shift & j))
        {
            RC_CtrlData[PASSING].key_count[KEY_WITH_CTRL][i]++;
        }
        if ((temp[5] & j) && !(key_with_ctrl & j) && (key_with_shift & j))
        {
            RC_CtrlData[PASSING].key_count[KEY_WITH_SHIFT][i]++;
        }
    }

    RC_CtrlData[LAST].key[KEY_PASSING] = RC_CtrlData[PASSING].key[KEY_PASSING];
    RC_CtrlData[LAST].key[KEY_WITH_CTRL] = RC_CtrlData[PASSING].key[KEY_WITH_CTRL];
    RC_CtrlData[LAST].key[KEY_WITH_SHIFT] = RC_CtrlData[PASSING].key[KEY_WITH_SHIFT];

    return RC_CtrlData;
}

/**
 * @brief  获取解析后的遥控器数据
 */
RC_Ctl_t *GetRCData(void)
{
    return RC_CtrlData;
}


