/*
 * udp_net.c
 *
 *  Created on: 2013-9-26
 *      Author: Administrator
 */
#include <stdio.h>
#include <string.h>

#if 0
unsigned short g_startFileSector = 10;
unsigned short g_curSector = 10;
unsigned short g_maxSector = 0;
unsigned int g_FileLen = 0;
static unsigned char g_file_buf[SECSIZE*2];
char switch_authority = 0;

void IIC_SrioPortStaticShow();

/*bSetGroup表示是否需要将组播设置到交换芯片中去*/
/*bSend是否需要将打印信息发送给主机*/
void ReadFromFile(char bSetGroup)
{
	int i,j,num;
	unsigned short groupIdNum;
	unsigned short groupname;
	unsigned short group[100];

	num = 0;
	for( i=0; i<7; i++)
	{
		Read_Sector(i,1,(unsigned short *)(&g_file_buf[i*1024]));
		memcpy(&groupIdNum,(void*)&g_file_buf[i*1024],2);
		groupIdNum = groupIdNum&0xff;

		if( groupIdNum == 0 || groupIdNum == 0xff )
			continue;

		num++;
		Read_Sector(i,groupIdNum+1,(unsigned short *)(&g_file_buf[i*1024]));
		for( j=0; j<groupIdNum; j++)
		{
			if(j == 0)
			{
				memcpy(&groupname,&g_file_buf[i*1024+j*2+2],2);
				xil_printf("group name = 0x%x\n\r",groupname);
				group[0] = groupname;
				continue;
			}
		}
		if( bSetGroup == 1)
		{
			group[j*2+2] = 0xffff;
			hlSrioSetGroup(group);
		}
	}
}


void SetSrioDelay(void *buf,int len)
{
	int status,delaySector;
	unsigned char _delayBuf[6];
	unsigned int srioDelay;
	unsigned short _bSrio;
	char rd_wr,send_buf[1024];

	delaySector = 8;
	memcpy((void*)&rd_wr,buf,1);
	if( rd_wr == 0 )	/*read*/
	{
		Read_Sector(delaySector,3,(unsigned short*)&_delayBuf);
		memcpy((void*)&srioDelay,(void*)_delayBuf,4);
		memcpy((void*)&_bSrio,(void*)(_delayBuf+4),2);

		xil_printf("read srio enum delay time, hlSrio is %d, delay time is %d-0x%x ok!\n\r",_bSrio,srioDelay,srioDelay);
		sprintf(send_buf,"read srio enum delay time, hlSrio is %d, delay time is %d-0x%x ok!\n\r",_bSrio,srioDelay,srioDelay);
	}
	else if( rd_wr == 1 )	/*write*/
	{
		status = Erase_Flash(delaySector);
		if( status == -1 )
		{
			xil_printf("!!!Error,erase flash error, set srio enum delay time failed!\n\r");
			return;
		}

		memcpy((void*)&srioDelay,buf+1,4);
		memcpy((void*)&_bSrio,buf+5,2);
		xil_printf("_bSrio = %d, srioDelay = 0x%x!\n\r",_bSrio,srioDelay);
		memcpy((void*)_delayBuf,buf+1,6);
		Write_Sector(delaySector,0,(unsigned short *)_delayBuf,3);
		xil_printf("update srio enum delay time ok! hlSrio is %d, the new delay time is %d-0x%x !\n\r",_bSrio,srioDelay,srioDelay);
		sprintf(send_buf,"update srio enum delay time ok! hlSrio is %d, the new delay time is %d-0x%x !\n\r",_bSrio,srioDelay,srioDelay);
	}
}
#endif

void UartSetSrioDelay(unsigned short bEnum,int delay)
{
	FILE *pFile = NULL;
	unsigned char _buf[8];
	unsigned short _bSrio;
	unsigned int _srioDelay;

	pFile = fopen("/mnt/delayTime.txt","wb+");
	if( pFile == NULL )
	{
		printf("open delayTime.txt file failed,set  delayTime failed.\n");
	}
	else
	{
		memcpy((void*)&_buf[0],(void*)&delay,4);
		memcpy((void*)&_buf[4],(void*)&bEnum,2);
		fwrite((void*)&_buf[0],6,1,pFile);
	}
	fclose( pFile );

	pFile = fopen("/mnt/delayTime.txt","rb");
	if( pFile == NULL )
	{

	}
	else
	{
		fread((void*)&_buf[0],1,6,pFile);
		memcpy((void*)&_srioDelay,(void*)&_buf[0],4);
		memcpy((void*)&_bSrio,(void*)&_buf[4],2);
	}
	fclose( pFile );

	printf("read srio enum delay time, hlSrio is %d, delay time is %d-0x%x ok!\n\r",_bSrio,_srioDelay,_srioDelay);
}


void OpenSysReset()
{
	FILE *pFile = NULL;
	unsigned short _bSysReset = 1;

	pFile = fopen("/mnt/sysResetFile.txt","wb+");
	if( pFile == NULL )
	{
		printf("打开系统复位配置文件失败，设置打开系统复位功能失败.\n");
	}
	else
	{
		fwrite((void*)&_bSysReset,2,1,pFile);
	}
	fclose( pFile );

	_bSysReset = 0;
	pFile = fopen("/mnt/sysResetFile.txt","rb");
	if( pFile == NULL )
	{

	}
	else
	{
		fread((void*)&_bSysReset,1,2,pFile);
	}
	fclose( pFile );

	printf("读取系统复位配置文件，当前设置系统复位功能为 %d.\n\r",_bSysReset);

}

void CloseSysReset()
{
	FILE *pFile = NULL;
	unsigned short _bSysReset = 0;

	pFile = fopen("/mnt/sysResetFile.txt","wb+");
	if( pFile == NULL )
	{
		printf("打开系统复位配置文件失败，设置关闭系统复位功能失败.\n");
	}
	else
	{
		fwrite((void*)&_bSysReset,2,1,pFile);
	}
	fclose( pFile );

	_bSysReset = 1;
	pFile = fopen("/mnt/sysResetFile.txt","rb");
	if( pFile == NULL )
	{

	}
	else
	{
		fread((void*)&_bSysReset,1,2,pFile);
	}
	fclose( pFile );

	printf("读取系统复位配置文件，当前设置系统复位功能为 %d.\n\r",_bSysReset);
}



