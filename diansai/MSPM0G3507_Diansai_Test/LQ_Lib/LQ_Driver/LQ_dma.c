/*******************************************************************************
 * @file                LQ_dma.c
 * @brief               本文件是 LQ_MSPM0GX_LIB 软件开源库文件的一部分
 * @copyright           版权所有 (C) 2025-2026 北京龙邱科技有限公司
 * @website             http://www.lqist.cn
 * @taobao              http://longqiu.taobao.com
 *
 * @description         龙邱科技 MSPM0G3507 核心板驱动库声明
 *
 * 开发环境配置:
 *   - 使用环境 : Keil5
 *   - 目标芯片 : MSPM0G3507
 *   - 外置晶振 : 16.000MHz
 *   - 系统时钟 : 80MHz
 *
 * 本文件遵循GPL-3.0开源协议发布，旨在为 MSPM0G3507 芯片嵌入式系统设计提供快速上手开发基于 MSPM0G3507 的应用程序的参考实现
 * 商业用途（包括单位使用）需提前联系作者获得授权
 *
 * GPL-3.0 许可证声明摘要:
 * 1. 允许自由使用、修改、分发本软件
 * 2. 分发修改后的版本时，必须以相同许可证发布
 * 3. 必须保留原始版权声明和许可证信息
 * 4. 不提供任何担保，使用风险自负
 * 5. 完整协议文本请参见项目根目录 LICENSE 文件
 *
 * @author				wuwu	(接口层编写)
 * @author              LQ_012	(优化与应用层编写)
 * @email               chiusir@163.com
 * @version             V2.0.0
 * @update              2026年4月24日
 *******************************************************************************/
#include "LQ_dma.h"

/*************************************************************************
 * @brief       初始化DMA通道
 * 
 * @param       channel     DMA通道
 * @param       pInit       DMA配置结构体指针
 *
 * @note        初始化DMA通道, 配置DMA通道的参数
 *************************************************************************/
void LQ_DMA_Init(LQEnum_DMA_Channel_t channel, LQConfig_DMA_InitTypeDef_t *DMA_Init)
{
    DMA->DMACHAN[channel].DMACTL = DMA_Init->transferMode | DL_DMA_NORMAL_MODE |
        (DMA_Init->destIncrement << 4) | DMA_Init->srcIncrement | 
        (DMA_Init->destWidth << 4) | DMA_Init->srcWidth;

    DL_Common_updateReg(&DMA->DMATRIG[channel].DMATCTL,
        DMA_Init->trigger | (uint32_t)DMA_Init->triggerType,
        DMA_DMATCTL_DMATSEL_MASK | DMA_DMATCTL_DMATINT_MASK);
}

/*************************************************************************
 * @brief       启动DMA通道传输
 * 
 * @param       channel     DMA通道
 *************************************************************************/
void LQ_DMA_Start(LQEnum_DMA_Channel_t channel)
{
    DMA->DMACHAN[channel].DMACTL |= DMA_DMACTL_DMAEN_ENABLE;
    DMA->DMACHAN[channel].DMACTL |= DMA_DMACTL_DMAREQ_REQUEST;
}

/*************************************************************************
 * @brief       设置源地址
 * 
 * @param       channel     DMA通道
 * @param       addr        源地址
 *************************************************************************/
void LQ_DMA_SetSrcAddr(LQEnum_DMA_Channel_t channel, uint32_t addr)
{
    DMA->DMACHAN[channel].DMASA = addr;
}

/*************************************************************************
 * @brief       设置目标地址
 * 
 * @param       channel     DMA通道
 * @param       addr        目标地址
 *************************************************************************/
void LQ_DMA_SetDstAddr(LQEnum_DMA_Channel_t channel, uint32_t addr)
{
    DMA->DMACHAN[channel].DMADA = addr;
}

/*************************************************************************
 * @brief       设置传输数据量
 * 
 * @param       channel     DMA通道
 * @param       size        传输数据量
 *************************************************************************/
void LQ_DMA_SetTransferSize(LQEnum_DMA_Channel_t channel, uint32_t size)
{
    DMA->DMACHAN[channel].DMASZ = size;
}

/*************************************************************************
 * @brief       检查DMA通道是否使能
 * 
 * @param       channel     DMA通道
 * 
 * @return      true        使能
 * @return      false       未使能
 *************************************************************************/
bool LQ_DMA_IsEnable(LQEnum_DMA_Channel_t channel)
{
    return (DMA->DMACHAN[channel].DMACTL & DMA_DMACTL_DMAEN_MASK) == DMA_DMACTL_DMAEN_DISABLE;
}
