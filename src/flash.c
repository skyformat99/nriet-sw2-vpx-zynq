#include "xil_io.h"
#include "flash.h"

#define SECSIZE   65536
#define XPAR_AXI_EMC_0_S_AXI_MEM0_BASEADDR 0x80000000
#define WAIT_TICKS	0xffffffff

int Erase_Flash(int sectorNum)
{
	int i=0;
	int val=0;
	unsigned short *base_addr;

	base_addr = (unsigned short*)(XPAR_AXI_EMC_0_S_AXI_MEM0_BASEADDR+SECSIZE*sectorNum*2);

	Xil_Out16((unsigned int)(base_addr+0x555),0x00AA);
	Xil_Out16((unsigned int)(base_addr+0x2AA),0x0055);
	Xil_Out16((unsigned int)(base_addr+0x555),0x0080);
	Xil_Out16((unsigned int)(base_addr+0x555),0x00AA);
	Xil_Out16((unsigned int)(base_addr+0x2AA),0x0055);
	Xil_Out16((unsigned int)(base_addr+0),0x0030);

	val=Xil_In16((unsigned int)(base_addr+0));

	while((val&0x80)!=0x80)
	{
		i++;
		if( i>=WAIT_TICKS)
		{
			 printf("addr %x Erase_Flash failed!\n",(unsigned int)base_addr);
			 Xil_Out16((unsigned int)(base_addr+0x555),0x0090);
			 Xil_Out16((unsigned int)(base_addr+0x000),0x0000);
			 return -1;
		}

		val=Xil_In16((unsigned int)base_addr);
	}
	Xil_Out16((unsigned int)(base_addr+0x555),0x0090);
	Xil_Out16((unsigned int)(base_addr+0x000),0x0000);

	return 0;  
}

int Write_Sector(int sectorNum,int offset,unsigned short *src_addr,int wr_num)
{	
        int i,j;
        int val = 0;
        unsigned short *dst_addr;
		
		dst_addr = (unsigned short*)(XPAR_AXI_EMC_0_S_AXI_MEM0_BASEADDR+SECSIZE*sectorNum*2+offset);
		/* Unlock Bypass program */
		Xil_Out16((unsigned int)(dst_addr+0x555),0x00AA);
		Xil_Out16((unsigned int)(dst_addr+0x2AA),0x0055);


		for(i=0;i<wr_num;i++)
		{
			Xil_Out16((unsigned int)(dst_addr+0x555),0x0020);
			Xil_Out16((unsigned int)(dst_addr+0),0x00A0);

			Xil_Out16((unsigned int)(dst_addr+i),*(src_addr+i));
			j=0;

			val = Xil_In16((unsigned int)(dst_addr+i));
			while(val  !=  *(src_addr+i))
			{
				j++;
				if(j>=WAIT_TICKS)
				{
					Xil_Out16((unsigned int)(dst_addr+0x555),0x0090);
					Xil_Out16((unsigned int)(dst_addr+0x000),0x0000);
					return -1;
				}
				val = Xil_In16((unsigned int)(dst_addr+i));
			 }
		}

		Xil_Out16((unsigned int)(dst_addr+0x555),0x0090);
		Xil_Out16((unsigned int)(dst_addr+0x000),0x0000);
		return 0;
}

int Read_Sector(int sectorNum,int rd_num,unsigned short *buf)
{
	unsigned short *sector_addr;
	int i;

	if( rd_num <= 0)
	   return -1;

	sector_addr = (unsigned short*)(XPAR_AXI_EMC_0_S_AXI_MEM0_BASEADDR+SECSIZE*sectorNum*2);
	
	if( rd_num <= SECSIZE )
	{
		for(i=0;i<rd_num;i++)
		{
			*buf = Xil_In16((unsigned int)(sector_addr+i));
			buf++;
		}
	}
	else
	{
	   printf("error\n");
	}
	return 0;
}


static unsigned int m_LogSectorLength = 0;
static unsigned int m_LogSectorOffset = 8;
short g_LogFileSwitch = 0;


static void readdata(int sectorNum,int offset,int len,unsigned char *buf)
{
	unsigned short *sector_addr,*readBuf,_data;
	int i,rd_num;

	rd_num = len/2;
	readBuf = (unsigned short*)buf;
	sector_addr = (unsigned short*)(XPAR_AXI_EMC_0_S_AXI_MEM0_BASEADDR+SECSIZE*sectorNum*2+offset);

	for(i=0;i<rd_num;i++)
	{
		*readBuf = Xil_In16((unsigned int)(sector_addr+i));
		readBuf++;
	}
	if( len%2 != 0 )
	{
		_data = Xil_In16((unsigned int)(sector_addr+i));
		*(buf+len-1) = (unsigned char)_data&0xff;
	}
}


static int writedata(int sectorNum,int offset,const unsigned char *src,int len)
{
	int i,j,wr_num;
	unsigned short *dst_addr,*data_addr,*src_addr,_data;
	int _val;

	wr_num = len/2;
	dst_addr = (unsigned short*)(XPAR_AXI_EMC_0_S_AXI_MEM0_BASEADDR+SECSIZE*sectorNum*2);
	data_addr = (unsigned short*)(XPAR_AXI_EMC_0_S_AXI_MEM0_BASEADDR+SECSIZE*sectorNum*2+offset);
	src_addr = (unsigned short*)src;

	/* Unlock Bypass program */
	Xil_Out16((unsigned int)(dst_addr+0x555),0x00AA);
	Xil_Out16((unsigned int)(dst_addr+0x2AA),0x0055);

	for(i=0;i<wr_num;i++)
	{
		Xil_Out16((unsigned int)(dst_addr+0x555),0x0020);
		Xil_Out16((unsigned int)(dst_addr+0x000),0x00A0);

		_data = *(src_addr+i);
		Xil_Out16((unsigned int)(data_addr+i),_data);
		usleep(100);
		j=0;

		_val = Xil_In16((unsigned int)(data_addr+i));
		while(_val != _data)
		{
			j++;
			if(j>=WAIT_TICKS)
			{
				printf("Error-1!!!--i = %d, addr %x sector_write failed ! --\r\n",i,Xil_In16((unsigned int)(dst_addr+i)));
				Xil_Out16((unsigned int)(dst_addr+0x555),0x0090);
				Xil_Out16((unsigned int)(dst_addr+0x000),0x0000);
				return -1;
			}
			_val = Xil_In16((unsigned int)(data_addr+i));
		 }
	}

	if( len%2 != 0 )
	{
		Xil_Out16((unsigned int)(dst_addr+0x555),0x0020);
		Xil_Out16((unsigned int)(dst_addr+0x000),0x00A0);

		_data = Xil_In16((unsigned int)(data_addr+i));
		_data = (_data&0xff00)|(*(src_addr+i)&0x00ff);

		Xil_Out16((unsigned int)(data_addr+i),_data);
		usleep(100);
		j=0;

		_val = Xil_In16((unsigned int)(data_addr+i));
		while(_val != _data)
		{
			j++;
			if(j>=0xffffff)
			{
				printf("Error-2!!!--i = %d, addr %x sector_write failed ! --\r\n",i, Xil_In16((unsigned int)(dst_addr+i)));
				Xil_Out16((unsigned int)(dst_addr+0x555),0x0090);
				Xil_Out16((unsigned int)(dst_addr+0x000),0x0000);
				return -1;
			}
			_val = Xil_In16((unsigned int)(data_addr+i));
		}
	}
	Xil_Out16((unsigned int)(dst_addr+0x555),0x0090);
	Xil_Out16((unsigned int)(dst_addr+0x000),0x0000);

	return 0;
}


void WriteLogFlash()
{
	int status;
	unsigned short _buf[4];
	unsigned short _bFinish;

	Read_Sector(LOG_SECTOR_FILE,LOG_SECTOR_HEAD,_buf);
	memcpy((void*)&_bFinish,(void*)&_buf[0],2);

	if( _bFinish != 0xABCD )			//日志没有正常完成
	{
		printf("日志记录未开启或枚举非正常结束.\r\n");
		printf("如需查看日志，在枚举结束后，在串口中先输入enter激活配置功能，再输入小写c进行查看.\r\n");
		printf("如需重新开启日志记录功能，在枚举结束后，在串口中先输入enter激活配置功能，再输入大写C重新打开日志记录功能.\r\n");
		g_LogFileSwitch = 0;
		return;
	}
	else
	{
			printf("日志功能已开启，枚举过程将记录在sector-%d中。。。\r\n",LOG_SECTOR_FILE);
			status = Erase_Flash(LOG_SECTOR_FILE);
			if( status == -1 )
			{
				printf("!!!Error,Erase log flash sector error!\r\n");
				g_LogFileSwitch = 0;
				return;
			}
			g_LogFileSwitch = 1;
	}
}

void OpenLogFile()
{
	unsigned char _buf[8];
	unsigned short _head;

	Erase_Flash(LOG_SECTOR_FILE);

	_head= 0xABCD;
	m_LogSectorLength = 65536;

	memcpy((void*)&_buf[0],(void*)&_head,2);
	memcpy((void*)&_buf[2],(void*)&m_LogSectorLength,4);
	writedata(LOG_SECTOR_FILE,0,_buf,8);
}

void CloseLogFile()
{
	Erase_Flash(LOG_SECTOR_FILE);
}

void WriteLogFlashFinish()
{
	unsigned char _buf[8];
	unsigned short _head;

	if( g_LogFileSwitch == 0 )
			return;

	_head= 0xABCD;

	memcpy((void*)&_buf[0],(void*)&_head,2);
	memcpy((void*)&_buf[2],(void*)&m_LogSectorLength,4);
	writedata(LOG_SECTOR_FILE,0,_buf,8);
}

void WriteLogFile(unsigned char *buf,unsigned int length)
{
	unsigned int _dataLen;

	if( g_LogFileSwitch == 0 )
		return;

	if( m_LogSectorOffset+length >= 65536*2 )
	{
		printf("Error!!!WriteLogFlash:: m_LogSectorOffset+length >= 128K.\r\n");
		return;
	}

	if( length == 0 )
	{
		printf("Error!!!WriteLogFlash:: length == 0.\r\n");
		return;
	}

	if( buf == NULL )
	{
		printf("Error!!!WriteLogFlash:: buf == NULL.\r\n");
		return;
	}

	_dataLen = length;

	if( length % 2 == 0 )
		_dataLen = length;
	else
		_dataLen = length+1;

	writedata(LOG_SECTOR_FILE,m_LogSectorOffset,buf, length);

	m_LogSectorOffset = m_LogSectorOffset + _dataLen;
	m_LogSectorLength += _dataLen;

}


void ReadLogInfo()
{
	unsigned char _buf[8];
	unsigned char _logFileBuf[65535];
	unsigned char *p;
	unsigned int _logFileLength;
	unsigned short _bFinish;
	int i;

	readdata(LOG_SECTOR_FILE,0,8,_buf);
	memcpy((void*)&_logFileLength,(void*)&_buf[2],4);
	memcpy((void*)&_bFinish,(void*)&_buf[0],2);

	if( _bFinish != 0xABCD )			//日志没有正常完成
		_logFileLength = 65536;

	readdata(LOG_SECTOR_FILE,8,_logFileLength,_logFileBuf);
	p = _logFileBuf;

	_logFileBuf[_logFileLength] = '\0';

	for( i=0; i<_logFileLength; i++ )
	{
		putchar(*p);
		p++;
	}
	putchar('\n');
	printf("------------------------------------------------------------\r\n");
}

