#ifndef __LQ_EEPROM_H
#define __LQ_EEPROM_H

/*****************************************************************************************
    设置EEPROM的地址，从0开始，实际是从flash后面往前开辟一片空间；本程序使用IAP方式
    若使用MOVC指令读写，则需要地址偏移，且其实地址不再是0，需要根据实际使用空间计算
        缺点是空间把握麻烦，优点是读写操作效率高。
-----------------------------------------------------------------------
|   EEPROM   | 扇区数 | IAP方式下 读/写/擦除   |       MOV方式        |
|    大小    |        | 起始地址   | 结束地址  | 起始地址 | 结束地址  |
|------------|--------|------------|-----------|----------|-----------|
|1K          |  2     | 0:0000h    | 0:03FFh   | FF:FCOOh | FF:FFFFh  |
|2K          |  4     | 0:0000h    | 0:07FFh   | FF:F800h | FF:FFFFh  |
|4K          |  8     | 0:0000h    | 0:OFFFh   | FF:FO0Oh | FF:FFFFh  |
|8K          |  16    | 0:0000h    | 0:1FFFh   | FF:EO00h | FF:FFFFh  |
|16K         |  32    | 0:0000h    | 0:3FFFh   | FF:C000h | FF:FFFFh  |
|32K         |  64    | 0:0000h    | 0:7FFFh   | FF:8000h | FF:FFFFh  |
-----------------------------------------------------------------------

用户可用根据自己的需要在整个 FLASH 空间中规划出任意不超过 FLASH 大小的 EEPROM 空间,
但需要注意:EEPROM 总是从后向前进行规划的(与AI/STC32GH系列类似)，擦除是按照扇区操作的，每扇区512字节，详细信息请参考数据手册内容
*****************************************************************************************/


/* 用户保存数据的地址，自定义数据保存空间的地址,注意选择的空间大小，不能超出 */
/* 用户保存数据的地址，自定义数据保存空间的地址,注意选择的空间大小，不能超出 */

typedef enum
{
//   Start_Addr = 0x000000,       //保存的EEPROM起始地址
//   User_Addr0 = 0x000002,       //设置每个地址区间为2Byte位数据,存放16位的数，按需求设置
//   User_Addr1 = 0x000004,       //但注意，擦除是按扇区擦除的，因此如果保存有的需要擦除有的不需要，请分开存放放到不同扇区，或者进行 数据覆盖代替擦除
   
   Page_0   = 0x0000, //0x0000 --> 0x01FF  //每个字节偏移1
   Page_1   = 0x0200, //0x0200 --> 0x03FF   //如果在下载程序时选择大小为1K则，最大为0x3FF,往后以此类推
//   Page_2   = 0x0400, //0x0400 --> 0x05FF 
//   Page_3   = 0x0600, //0x0600 --> 0x07FF 
}eeprom_Addr;



void IAP_Write_u16Data(eeprom_Addr ID_addr, u16 u16data);
u16 IAP_Read_u16Data(eeprom_Addr ID_addr);
void IAP_Write_Byte(u32 eeprom_addr, u8 Byte_data);
u8 IAP_Read_Byte(u32 eeprom_addr);
s8 IAP_Read_sByte(u32 eeprom_addr);
void IAP_Write_nByte(u32 eeprom_addr, u8 *nByte_addr, u16 length);
void IAP_Read_nByte(u32 eeprom_addr, u8 *nByte_addr, u16 length);
void IAP_Write_Buff(u32 eeprom_addr, u8 *buff, u16 length);
void IAP_Read_nByte(u32 eeprom_addr, u8 *nByte_addr, u16 length);
// void IAP_Read_Buff(u32 eeprom_addr, u8 *buff, u16 len);
void IAP_Erase_Page(u32 eeprom_addr);           //注意,擦除是按扇区擦除的，每扇区512字节，详细信息请参考数据手册内容





#endif

