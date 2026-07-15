/*******************************************************************************
 * @file                LQ_oled.h
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
 * @author              LQ_012
 * @email               chiusir@163.com
 * @version             V2.0.0
 * @update              2026年4月24日
 *******************************************************************************/
#ifndef __LQ_OLED_H__
#define __LQ_OLED_H__

#include "include.h"

/****************************************************************************************************
 * @brief   宏定义
 ****************************************************************************************************/

//-----------------OLED引脚定义-----------------

#define OLED_SCL_PIN			GPIO_Pin_A_17
#define OLED_SDA_PIN			GPIO_Pin_A_16
#define OLED_CS_PIN				GPIO_Pin_B_22
#define OLED_RES_PIN			GPIO_Pin_B_21
#define OLED_DC_PIN				GPIO_Pin_B_23

//-----------------OLED端口定义-----------------

#define OLED_SCL_Clr() 			LQ_GPIO_WritePin(OLED_SCL_PIN, 0)	// SCK
#define OLED_SCL_Set() 			LQ_GPIO_WritePin(OLED_SCL_PIN, 1)

#define OLED_SDA_Clr() 			LQ_GPIO_WritePin(OLED_SDA_PIN, 0)	// DI
#define OLED_SDA_Set() 			LQ_GPIO_WritePin(OLED_SDA_PIN, 1)

#define OLED_RES_Clr()  		LQ_GPIO_WritePin(OLED_RES_PIN, 0)//RST
#define OLED_RES_Set()  		LQ_GPIO_WritePin(OLED_RES_PIN, 1)

#define OLED_DC_Clr()   		LQ_GPIO_WritePin(OLED_DC_PIN, 0)//DC
#define OLED_DC_Set()   		LQ_GPIO_WritePin(OLED_DC_PIN, 1)

#define OLED_CS_Clr()   		LQ_GPIO_WritePin(OLED_CS_PIN, 0)//CS
#define OLED_CS_Set()   		LQ_GPIO_WritePin(OLED_CS_PIN, 1)

typedef enum
{
	OLED_CMD = 0x00,	// 写命令
	OLED_DATA,			// 写数据
} LQ_OLED_Instruct_t;
	
/****************************************************************************************************
 * @brief   函数定义
 ****************************************************************************************************/

void LQ_OLED_Init(void);					/*! @brief	初始化 OLED 屏幕 */

void LQ_OLED_ColorTurn(unsigned char i);	/*! @brief	OLED 反显/正常显示控制*/
void LQ_OLED_DisplayTurn(unsigned char i);	/*! @brief	OLED 屏幕 180 度旋转显示控制 */
void LQ_OLED_DisPlay_On(void);				/*! @brief	开启 OLED 显示 */
void LQ_OLED_DisPlay_Off(void);				/*! @brief	关闭 OLED 显示 */
void LQ_OLED_Refresh(void);					/*! @brief	更新显存数据到 OLED 屏幕 */
void LQ_OLED_Clear(void);					/*! @brief	OLED 清屏函数 */

/*! @brief	在 OLED 指定坐标画点 */
void LQ_OLED_DrawPoint(unsigned char x,unsigned char y,unsigned char t);

/*! @brief	在 OLED 两点之间画线 */
void LQ_OLED_DrawLine(unsigned char x1,unsigned char y1,unsigned char x2,unsigned char y2,unsigned char mode);

/*! @brief	在 OLED 指定位置画圆 */
void LQ_OLED_DrawCircle(unsigned char x,unsigned char y,unsigned char r);

/*! @brief	在 OLED 指定位置显示一个 ASCII 字符 */
void LQ_OLED_ShowChar(unsigned char x,unsigned char y,unsigned char chr,unsigned char size1,unsigned char mode);

/*! @brief	在 OLED 指定位置显示字符串 */
void LQ_OLED_ShowString(unsigned char y,unsigned char x,unsigned char *chr,unsigned char size1);

/*! @brief	计算 m 的 n 次方 */
unsigned int LQ_OLED_Pow(unsigned char m,unsigned char n);

/*! @brief	在 OLED 指定位置显示数字 */
void LQ_OLED_ShowNum(unsigned char x,unsigned char y,unsigned int num,unsigned char len,unsigned char size1,unsigned char mode);

/*! @brief	在 OLED 指定位置显示汉字 */
void LQ_OLED_ShowChinese(unsigned char x,unsigned char y,unsigned char num,unsigned char mode);

/*! @brief	汉字左滚动显示 */
void LQ_OLED_ScrollDisplay(unsigned char num,unsigned char space,unsigned char mode);

/*! @brief	在 OLED 显示图片 */
void LQ_OLED_ShowPicture(unsigned char x,unsigned char y,unsigned char sizex,unsigned char sizey,unsigned char BMP[],unsigned char mode);

#endif
