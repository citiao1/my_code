#include "AI8051U_EEPROM.h"
#include "include.h"
// #include "LQ_EEPROM.h"
/**********************************************************************************
 * 函 数 名：void EEPROM_Write_Data(eeprom_Addr ID_addr, uint16_t u16data)
 * 功    能：向EEPROM指定地址，写入数据，数据类型支持16位数据
 * 参    数：ID_addr：要写入的eeprom地址， u16data：写入该前面地址的数据
 * 返 回 值：无
 *           保存16位的数据，必须明确地址ID和范围
 * 调用示例: IAP_Write_u16Data(Page_0,12345); //将12345 保存到 eeprom地址Page_0 中
 *********************************************************************************/
void IAP_Write_u16Data(eeprom_Addr ID_addr, u16 u16data)
{
	u8 u8data[2];
	u8data[0] = (u8)(u16data >> 8);			 // 得到高八位
	u8data[1] = (u8)u16data;				 // 低八位
	EEPROM_write_n((u32)ID_addr, u8data, 2); // 将两字节数据写入EEPROM 地址，EE_ADDRESS   tmp数据地址
}

/**********************************************************************************
 * 函 数 名：uint16_t EEPROM_Read_Data(eeprom_Addr ID_addr)
 * 功    能：向EEPROM指定地址，读取数据，数据类型支持16位数据，两个字节
 * 参    数：eeprom_Addr ID_addr：目标的地址， uint16_t u16data：要保存的数据
 * 返 回 值：读取的16位数据值
 *********************************************************************************/
// 读取16位数据，明确要读取的地址ID
u16 IAP_Read_u16Data(eeprom_Addr ID_addr)
{
	u8 _temp[2];
	EEPROM_read_n((u32)ID_addr, _temp, 2); // 读出2字节
	return ((u16)((_temp[0] << 8) + _temp[1]));
}

/**********************************************************************************
 * 函 数 名：void IAP_Write_Byte(u32 eeprom_addr,u8 Byte_data)
 * 功    能：向EEPROM指定地址，写入一个字节的数据
 * 参    数：u32 eeprom_addr：要写入的eeprom地址， u8 Byte_data：写入该前面地址的数据
 * 返 回 值：无
 *           保存16位的数据，必须明确地址ID和范围
 * 调用示例: u8 testData = 241;IAP_Write_Byte(Page_0,testData); //testData 保存到 eeprom地址Page_0开始的地址空间中
 *********************************************************************************/
void IAP_Write_Byte(u32 eeprom_addr, u8 Byte_data)
{
	IAP_ENABLE(); // 设置等待时间，允许IAP操作，送一次就够
	IAP_WRITE();  // 宏调用, 送字节写命令

	IAP_ADDRE = (u8)(eeprom_addr >> 16); // 送地址高字节（地址需要改变时才需重新送地址）
	IAP_ADDRH = (u8)(eeprom_addr >> 8);	 // 送地址中字节（地址需要改变时才需重新送地址）
	IAP_ADDRL = (u8)eeprom_addr;		 // 送地址低字节（地址需要改变时才需重新送地址）
	IAP_DATA = Byte_data;				 // 送数据到IAP_DATA，只有数据改变时才需重新送

	EEPROM_Trig(); // 触发EEPROM操作，内含中断保存 挂起和恢复

	DisableEEPROM();
}

/**********************************************************************************
 * 函 数 名：u8 IAP_Read_Byte(u32 eeprom_addr)
 * 功    能：从EEPROM指定地址，读取一个字节的数据
 * 参    数：u32 eeprom_addr：要读取的eeprom起始地址
 * 返 回 值：读取的一字节数据
 * 调用示例: u8 testData; testData = IAP_Read_Byte(Page_0);
 *********************************************************************************/
u8 IAP_Read_Byte(u32 eeprom_addr)
{
	u8 OenByte;
	IAP_ENABLE(); // 设置等待时间，允许IAP操作，送一次就够
	IAP_READ();	  // 送字节读命令，命令不需改变时，不需重新送命令

	IAP_ADDRE = (u8)(eeprom_addr >> 16); // 送地址高字节（地址需要改变时才需重新送地址）
	IAP_ADDRH = (u8)(eeprom_addr >> 8);	 // 送地址中字节（地址需要改变时才需重新送地址）
	IAP_ADDRL = (u8)eeprom_addr;		 // 送地址低字节（地址需要改变时才需重新送地址）
	EEPROM_Trig();						 // 触发EEPROM操作
	OenByte = IAP_DATA;					 // 读出的数据送往
	DisableEEPROM();

	return OenByte;
}

/**********************************************************************************
 * 函 数 名：s8 IAP_Read_sByte(u32 eeprom_addr)
 * 功    能：从EEPROM指定地址，读取一个字节的数据
 * 参    数：u32 eeprom_addr：要读取的eeprom起始地址
 * 返 回 值：读取的一字节数据有符号数
 * 调用示例: s8 testData; testData = IAP_Read_sByte(Page_0);
 *********************************************************************************/
s8 IAP_Read_sByte(u32 eeprom_addr)
{
	s8 OenByte;
	IAP_ENABLE(); // 设置等待时间，允许IAP操作，送一次就够
	IAP_READ();	  // 送字节读命令，命令不需改变时，不需重新送命令

	IAP_ADDRE = (u8)(eeprom_addr >> 16); // 送地址高字节（地址需要改变时才需重新送地址）
	IAP_ADDRH = (u8)(eeprom_addr >> 8);	 // 送地址中字节（地址需要改变时才需重新送地址）
	IAP_ADDRL = (u8)eeprom_addr;		 // 送地址低字节（地址需要改变时才需重新送地址）
	EEPROM_Trig();						 // 触发EEPROM操作
	OenByte = (s8)IAP_DATA;				 // 读出的数据送往
	DisableEEPROM();

	return OenByte;
}

/**********************************************************************************
 * 函 数 名：void IAP_Write_nByte(u32 eeprom_addr,u8 *nByte,u16 length)
 * 功    能：向EEPROM指定地址开始，连续写入n字节的数据
 * 参    数：u32 eeprom_addr：要写入的eeprom地址， u8 *nByte：写入该前面地址的数据；u16 length：数据长度
 * 返 回 值：无
 *           保存16位的数据，必须明确地址ID和范围
 * 调用示例: IAP_Write_nByte(Page_0,(u8 *)12345,5); //12345 保存到 eeprom地址Page_0开始的地址空间中
 *********************************************************************************/
void IAP_Write_nByte(u32 eeprom_addr, u8 *nByte_addr, u16 length)
{
	IAP_ENABLE(); // 设置等待时间，允许IAP操作，送一次就够
	IAP_WRITE();  // 宏调用, 送字节写命令
	do
	{
		IAP_ADDRE = (u8)(eeprom_addr >> 16); // 送地址高字节（地址需要改变时才需重新送地址）
		IAP_ADDRH = (u8)(eeprom_addr >> 8);	 // 送地址中字节（地址需要改变时才需重新送地址）
		IAP_ADDRL = (u8)eeprom_addr;		 // 送地址低字节（地址需要改变时才需重新送地址）
		IAP_DATA = *nByte_addr;				 // 送数据到IAP_DATA，只有数据改变时才需重新送

		EEPROM_Trig(); // 触发EEPROM操作

		eeprom_addr++; // 下一个地址
		nByte_addr++;  // 下一个数据
	} while (--length); // 直到结束

	DisableEEPROM();
}

/**********************************************************************************
 * 函 数 名：void IAP_Read_nByte(u32 eeprom_addr,u8 *nByte,u16 length)
 * 功    能：从EEPROM指定地址开始，连续读出n字节的数据
 * 参    数：u32 eeprom_addr：要读取的eeprom起始地址， u8 *nByte保存读取到的数据；u16 length：数据长度
 * 返 回 值：无
 *           保存16位的数据，必须明确地址ID和范围
 * 调用示例:
	Test_Data[2];    IAP_Read_nByte(Page_0,Test_Data,2); // 从eeprom地址Page_0开始读取2字节数据保存到Test_Data；
 *********************************************************************************/
void IAP_Read_nByte(u32 eeprom_addr, u8 *nByte_addr, u16 length)
{
	IAP_ENABLE(); // 设置等待时间，允许IAP操作，送一次就够
	IAP_READ();	  // 送字节读命令，命令不需改变时，不需重新送命令
	do
	{
		IAP_ADDRE = (u8)(eeprom_addr >> 16); // 送地址高字节（地址需要改变时才需重新送地址）
		IAP_ADDRH = (u8)(eeprom_addr >> 8);	 // 送地址中字节（地址需要改变时才需重新送地址）
		IAP_ADDRL = (u8)eeprom_addr;		 // 送地址低字节（地址需要改变时才需重新送地址）
		EEPROM_Trig();						 // 触发EEPROM操作
		*nByte_addr = IAP_DATA;				 // 读出的数据送往
		eeprom_addr++;
		nByte_addr++;
	} while (--length);

	DisableEEPROM();
}

//****************************************************************************************
// 函 数 名：void IAP_Write_Buff(u32 eeprom_addr, const u8 *buff)
// 功    能：写buff到EEPROM
// 参    数：u32 eeprom_addr：要写入的eeprom地址， const u8 *buff：写入该前面地址的数据
// 返 回 值：无
// 调用示例: IAP_Write_Buff(Page_0, (u8 *)"Hello World!",12); //将字符串"Hello World!"保存到 eeprom地址Page_0开始的地址空间中
//****************************************************************************************
void IAP_Write_Buff(u32 eeprom_addr, u8 *buff, u16 length)
{
	u8 txt[24] = {0};
#if 1
	IAP_ENABLE(); // 设置等待时间，允许IAP操作，送一次就够
	IAP_WRITE();  // 宏调用, 送字节写命令
	do
	{
		IAP_ADDRE = (eeprom_addr >> 16); // 送地址高字节（地址需要改变时才需重新送地址）
		IAP_ADDRH = (eeprom_addr >> 8);	 // 送地址中字节（地址需要改变时才需重新送地址）
		IAP_ADDRL = eeprom_addr;		 // 送地址低字节（地址需要改变时才需重新送地址）
		IAP_DATA = *buff;				 // 送数据到IAP_DATA，只有数据改变时才需重新送

		sprintf(txt, "IAP_E:%#X ", eeprom_addr >> 16);
		OLED_P6x8Str(0, 1, txt); // 显示读出的数据
		sprintf(txt, "IAP_H:%#X ", eeprom_addr >> 8);
		OLED_P6x8Str(0, 2, txt); // 显示读出的数据
		sprintf(txt, "IAP_L:%#X ", eeprom_addr);
		OLED_P6x8Str(0, 3, txt); // 显示读出的数据

		EEPROM_Trig(); // 触发EEPROM操作

		eeprom_addr++; // 下一个地址
		buff++;		   // 下一个数据
	} while (--length); // 直到结束

	DisableEEPROM();
#else
	while (length--)
	{
		IAP_Write_Byte(eeprom_addr, *buff); // 写 IAP 数据
		eeprom_addr++;
		buff++;
	}
#endif
}

//****************************************************************************************
// 函 数 名：void IAP_Read_Buff(u32 eeprom_addr, u8 *buff, u16 len)
// 功    能：从EEPROM读取len个字节到buff
// 参    数：u32 eeprom_addr：要读取的eeprom地址， u8 *buff：读取到的数据， u16 len：读取的数据长度
// 返 回 值：无
// 调用示例: IAP_Read_Buff(Page_0, buff, 10); // 从eeprom地址Page_0开始读取10字节数据保存到buff；
// ***************************************************************************************
void IAP_Read_Buff(u32 eeprom_addr, u8 *buff, u16 length)
{
#if 1
	IAP_ENABLE(); // 设置等待时间，允许IAP操作，送一次就够
	IAP_READ();	  // 送字节读命令，命令不需改变时，不需重新送命令
	do
	{
		IAP_ADDRE = (eeprom_addr >> 16); // 送地址高字节（地址需要改变时才需重新送地址）
		IAP_ADDRH = (eeprom_addr >> 8);	 // 送地址中字节（地址需要改变时才需重新送地址）
		IAP_ADDRL = eeprom_addr;		 // 送地址低字节（地址需要改变时才需重新送地址）
		EEPROM_Trig();					 // 触发EEPROM操作
		*buff = IAP_DATA;				 // 读出的数据送往
		eeprom_addr++;
		buff++;
	} while (--length);

	DisableEEPROM();
#else

	while (length--)
	{
		*buff++ = IAP_Read_Byte(eeprom_addr); // 读 IAP 数据
		eeprom_addr++;
	}
#endif
}

void IAP_Erase_Page(u32 eeprom_addr)
{
	EEPROM_SectorErase(eeprom_addr);
	delay_ms(6); // 擦除一整个扇区512字节，传入参数只要在该扇区内，每扇区耗时约5ms
}


