#include "ecoder.h"

/*
 *****************************************************************************************
 * 编码器解析模块（基于CAN反馈）
 *
 * 功能：读取电机编码器反馈（GM6020偏角编码器），计算：
 * 1. 单圈角度（single_angle）
 * 2. 编码值差（d_ecode）
 * 3. 多圈累计角度（total_angle）
 *****************************************************************************************
 */

static float single_angle;   // 单圈角度（度）
static float single_ecode;   // 当前编码值
static float last_ecode;     // 上一次编码值
static float total_angle;    // 多圈累计角度
static int round = 0;        // 编码器圈数计数
static float d_ecode[1];     // 编码器差值（数组形式，便于外部引用）
static float totald_ecode;
static float averaged_ecode;

static EcoderInstance *ecoder_instance;
static CAN_Init_Config_s ecoder_can_config = 
{
    .rx_id = 0x001,
};

/**
 * @brief  编码器CAN回调：解析编码器反馈
 */
static void EncodeHandle(CANInstance *_instance)
{
    last_ecode = single_ecode;
    single_ecode = (uint16_t)(_instance->rxbuff[3] | (_instance->rxbuff[4] << 8));
    single_angle = single_ecode * ENCODE_2_DEGREE;
    d_ecode[0] = single_ecode - last_ecode;


    if(d_ecode[0] < -7000)
    {
        round ++;
    }
    if(d_ecode[0] > 7000)
    {
        round --;
    }
    total_angle = round * 360 + single_angle;
}



/**
 * @brief  编码器初始化
 */
EcoderInstance *EcoderInit(void)
{

    EcoderInstance *_instance = (EcoderInstance *)malloc(sizeof(EcoderInstance));
    memset(_instance, 0 ,sizeof(EcoderInstance));
    _instance->single_angle = &single_angle;
    _instance->single_ecode = &single_ecode;
    _instance->d_ecode = d_ecode;
    _instance->round = &round;
    _instance->total_angle = &total_angle;
    _instance->last_ecode = &last_ecode;
    _instance->averaged_ecode = &averaged_ecode;
    ecoder_can_config.can_moudle_callback = EncodeHandle;
    ecoder_can_config.id = _instance;
    _instance->ecoder_can_instance = CANRegister(&ecoder_can_config);

    ecoder_instance = _instance;

    return _instance;

}




