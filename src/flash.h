/*
 * flash.h
 *
 *  Created on: 2013-9-22
 *      Author: Administrator
 */

#ifndef FLASH_H_
#define FLASH_H_

#include <stdio.h>


#undef FLASH_DEBUG

#ifdef FLASH_DEBUG
#define DBF(x) x
#else
#define DBF(x)
#endif

#define   SECSIZE   65536         //sector size

#define LOG_SECTOR_FILE				255
#define LOG_SECTOR_HEAD			4

int Erase_Flash(int sectorNum);
/*!!!wr_num是写入的字数，不是字节数*/
//void Write_Flash(u16 *dst_addr,u16 *src_addr,int wr_num);
//void Write_Sector(int sectorNum,int offset,u16 *src_addr,int wr_num);
//void Read_Sector(int sectorNum,int rd_num,u16 *buf);
void WriteLogFlashFinish();
void WriteLogFile(unsigned char *buf,unsigned int length);
void OpenLogFile();

void flash_test(int sectorNum);

extern short g_LogFileSwitch;
#endif /* FLASH_H_ */
