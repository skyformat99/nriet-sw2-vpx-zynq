/*
 * udp_net.c
 *
 *  Created on: 2013-9-26
 *      Author: Administrator
 */
#include <string.h>
#include <stdlib.h>
#include "udpapp.h"
#include "iic_1848.h"
#include "udpSocketLib.h"

extern unsigned int g_chassisNum;

unsigned short hrSlotnumbers = 99;				//如果需要交换板通知接口板发送数据，将其初始化值置为0
int hrReadynumbers=0;
int hrBackupIndex=0;
unsigned short statusArr[12]={0};
unsigned short restoreArr[12]={0};
int RebootArr[REBOOTBOARDMAX]={0};
unsigned char registerArr[12]={0};
unsigned short doorBellFlag=0;

int nSocketID0 = -1;

static void OnSocket(void * pParent,int nID,char * buf,int len)
{
	char * addr;
	int  port;
	unsigned char swIndex;
	unsigned char portNum;
	unsigned char i;
	unsigned char *arr=(unsigned char*)buf;

	addr = udpGetRecvAddress(nID);
	port = udpGetRecvPort(nID);
	if(len < 0)
		return;

	if(!parseNotifyPktStruct(&NotifyPkt,arr,len))
	{
		printf("Unknown Packet %d!\n\r",NotifyPkt.Pkt_Type);
		goto label3;
	}
	//xil_printf("Packet Type %d!\n\r",NotifyPkt.Pkt_Type);
	switch(NotifyPkt.Pkt_Type)
	{
		case STATE_NOTIFY_PKT:
			//xil_printf("Recv a state notify pkt!\n\r");
			parseStateNotifyPkt(&stateNofityPkt, arr);
			sendReplyAckPkt(addr,port,STATE_NOTIFY_ACK);
			//xil_printf("sendReplyAckPkt finish[hrReadynumbers=%d][%d]!\n\r",hrReadynumbers,hrSlotnumbers);
			/*
			if((hrReadynumbers==hrSlotnumbers*SLICEIDMAX)&&(doorBellFlag==0))
			{
				rioSendDB(0x803c, 0xf6);
				delay_SL(0x1fffff);
				rioSendDB(0x813c, 0xf6);
				delay_SL(0x1fffff);
				rioSendDB(0x823c, 0xf6);
				delay_SL(0x1fffff);
				rioSendDB(0x833c, 0xf6);
				delay_SL(0x1fffff);
				rioSendDB(0x803c, 0xf7);
				delay_SL(0x1fffff);
				rioSendDB(0x813c, 0xf7);
				delay_SL(0x1fffff);
				rioSendDB(0x823c, 0xf7);
				delay_SL(0x1fffff);
				rioSendDB(0x833c, 0xf7);

				doorBellFlag=1;
				xil_printf("********************************************");
				xil_printf("\n\r*********All Hr nodes has been started!*****\n\r");
				xil_printf("********************************************\n\r");
			}*/
			break;
		case FAULT_NOTIFY_PKT:
			printf("Recv a fault notify pkt!\n\r");
			parseFaultNotifyPkt(&faultNotifyPkt, arr);
			//clearStatusArr(Slot_Index,statusArr);//clear the corresponding bits int the statusArr
			//rioSendDB(0x88dc,0xf6);
			//rioSendDB(0x88dc,0xf7);
			break;
		case UPDATE_NOTIFY_PKT:
			//defult none
			//hrBackupIndex=0;
			break;
		case REPLY_ACK_PKT:
			//defult node
			break;
		case STOP_NOTIFY_PKT:
			//xil_printf("Recv a stop notify pkt!\n\r");
			parseStopNotifyPkt(&stopNotifyPkt, arr);
			//xil_printf("Slot_Index=%d Count=%d\n\r",stopNotifyPkt.Slot_Index,stopNotifyPkt.Pkt_count);
			if(!doorBellFlag)
			{
				//haven't send doorbell signal, and recv a type 5 packet
				for(i=0;i<4;i++)
				{
					if(((statusArr[stopNotifyPkt.Slot_Index-1]>>i)&0x01)==1)
					{
						hrReadynumbers--;
					}
				}
				clearArr(stopNotifyPkt.Slot_Index,statusArr);
				registerArr[stopNotifyPkt.Slot_Index-1]=1;
			}

			if(!stopNotifyPkt.Pkt_count)
			{
				RebootArr[stopNotifyPkt.Slot_Index-1]++;
				//xil_printf("Register stopNotify request into RebootArr!\n\r");
			}
			printRebootArr(RebootArr);
			//Clear the corresponding position of StatusArr AND hrReadynumbers
			//clearStatusArr(stopNotifyPkt.Slot_Index,statusArr);
			//hrReadynumbers--;
			if(!CheckSlotTable(stopNotifyPkt.Slot_Index,&swIndex,&portNum))
			{
				//normal programs cannot go here,just in case.
				printf("Cannot find the corresponding swIndex and portNum!\n\r");
				goto label1;
			}
			//xil_printf("Current swIndex=[%d] portNum=[%d]\n\r",swIndex,portNum);
			disable_1848_port_rxtx(swIndex,portNum);

			reset_1848_ackid(swIndex,portNum);
label1:		sendReplyAckPkt(addr,port,STOP_NOTIFY_ACK);
			break;
		case RESTORE_NOTIFY_PKT:
			//xil_printf("Recv a restore notify pkt!\n\r");
			parseRestoreNotifyPkt(&restoreNotifyPkt, arr);
			if(!doorBellFlag)
			{
				if(registerArr[restoreNotifyPkt.Slot_Index-1]==1)
				{
					registerArr[restoreNotifyPkt.Slot_Index-1]=0;
					if (!CheckSlotTable(restoreNotifyPkt.Slot_Index, &swIndex,&portNum)) {
						printf("Cannot find the corresponding swIndex and portNum!\n\r");
						goto label2;
					}
					//clearArr(restoreNotifyPkt.Slot_Index,restoreArr);
					enable_1848_port_rxtx(swIndex, portNum);
				}

				if(isRepeatPkt(restoreNotifyPkt.Slot_Index,restoreNotifyPkt.Slice_ID,statusArr))
				{
					hrReadynumbers++;
					/*
					if((hrReadynumbers==hrSlotnumbers*SLICEIDMAX)&&(doorBellFlag==0))
					{
						rioSendDB(0x803c, 0xf6);
						delay_SL(0x1fffff);
						rioSendDB(0x813c, 0xf6);
						delay_SL(0x1fffff);
						rioSendDB(0x823c, 0xf6);
						delay_SL(0x1fffff);
						rioSendDB(0x833c, 0xf6);
						delay_SL(0x1fffff);
						rioSendDB(0x803c, 0xf7);
						delay_SL(0x1fffff);
						rioSendDB(0x813c, 0xf7);
						delay_SL(0x1fffff);
						rioSendDB(0x823c, 0xf7);
						delay_SL(0x1fffff);
						rioSendDB(0x833c, 0xf7);

						doorBellFlag=1;
						printStatusArr(statusArr);
						xil_printf("********************************************");
						xil_printf("\n\r*********All Hr nodes has been started!*****\n\r");
						xil_printf("********************************************\n\r");
					}
					*/
				}


			}else{
				//Whether or not the corresponding Solt_Index was registered in RebootArr before
				//xil_printf("Slot_Index=%d Slice_ID=%d\n\r",restoreNotifyPkt.Slot_Index,restoreNotifyPkt.Slice_ID);
				//printRebootArr(RebootArr);
				/*for(i=0;i<REBOOTBOARDMAX;i++)
				{
					if(restoreNotifyPkt.Slot_Index==RebootArr[i])
					{
						CurRebootIndex=i;
						if(setStatusArr(restoreNotifyPkt.Slot_Index,restoreNotifyPkt.Slice_ID,statusArr))
						{
							hrReadynumbers++;
						}
						break;
					}
				}
				if(i==REBOOTBOARDMAX)
				{
					xil_printf("Received unknown Slot_Index that was not registered before!\n\r");
					goto label2;
				}*/
				setArr(restoreNotifyPkt.Slot_Index,restoreNotifyPkt.Slice_ID,restoreArr);
				if (checkArr(restoreNotifyPkt.Slot_Index, restoreArr))
				{
					//Four core's Pkt has been received completed yet
					if (!CheckSlotTable(restoreNotifyPkt.Slot_Index, &swIndex,&portNum)) {
						//normal programs cannot go here,just in case.
						printf("Cannot find the corresponding swIndex and portNum!\n\r");
						goto label2;
					}

					//xil_printf("Current swIndex=[%d] portNum=[%d]\n\r", swIndex,portNum);
					clearArr(restoreNotifyPkt.Slot_Index,restoreArr);
					enable_1848_port_rxtx(swIndex, portNum);

				}
			}
			//xil_printf("restoreArr:");
			//printStatusArr(restoreArr);//debug print
			sendReplyAckPkt(addr,port,RESTORE_NOTIFY_ACK);
label2:		break;
		default:
		{
			printf("Valid Prefix, invalid packet content!\n\r");
			break;
		}
	}
	label3:
	return;
}

void TokeIP(char *ipAddr,unsigned char array[])
{
	char *token;
	char * nouse;
	int i,number;
	char seps[] = ".";

	i = 0;
	token = strtok(ipAddr,seps);
	while( token != NULL )
	{
		number = strtol(token,&nouse,10);
		array[i] = number;
		i++;
		token = strtok(NULL,seps);
	}
}

/*机柜号(0-15)，机箱号(0-15)，槽位号(1-16)*/
int FindFromIPFile(unsigned char cabinet,unsigned char chassis,unsigned char slot,unsigned char ipArray[4])
{
	char _ipAddr[100];
	FILE *pFile = NULL;
	int _length;
	int _lineNum = 0;
	int _colNum = 0;
	char * nouse;
	int number;
	char seps[]   = "  ,\t\n";
	char *token;
	unsigned char _mcabinet,_mchassis,_mslot;

	if( (cabinet >= 16) || (chassis >= 16) || ( slot >=17 ) )
	{
		printf("参数不符合要求，请检查参数.\n");
		return -1;
	}

	pFile = fopen("/mnt/ipAddrFile.txt","r+t");
	if( pFile == NULL )
	{
		printf("打开ipAddrFile.txt 文件失败，将使用默认IP进行配置.\n");
	}
	else
	{
		_lineNum = 0;
		while( fgets(_ipAddr,50,pFile) != NULL )
		{
			_length = strlen(_ipAddr);
			_ipAddr[_length] = '\0';
			if( _lineNum == 0 )		//第一行为中外格式行
				_lineNum++;
			else
			{
				token = strtok(_ipAddr,seps);
				_colNum = 0;
				while( token != NULL )
				{
					_colNum++;
					if( _colNum == 4 )
					{
						TokeIP(token,ipArray);
						if( (_mcabinet == cabinet) && (_mchassis == chassis) && (_mslot == slot ) )
						{
							fclose( pFile );
							return 1;
						}
					}
					else
					{
						number = strtol(token,&nouse,10);
						if( _colNum == 1 )
							_mcabinet = number;
						else if( _colNum == 2 )
							_mchassis = number;
						else if( _colNum == 3 )
							_mslot = number;
					}
					token = strtok(NULL,seps);
				}
			}
		}

		fclose( pFile );
	}

	ipArray[0] = 192;
	ipArray[1] = 168;
	ipArray[2] = cabinet*16+chassis;
	ipArray[3] = 1+(slot-1)*16;
	printf("警告！使用默认规则的IP:%d-%d-%d-%d.\n",ipArray[0],ipArray[1],ipArray[2],ipArray[3]);
	return 0;
}



void NetInit()
{
	char _buf[100];
	unsigned char _ipAddr[4];

	if( FindFromIPFile(0,1,1,_ipAddr) == -1 )
	{
		printf("FindFromIPFile函数参数错误，网络配置失败.\n");
		return;
	}
	else
	{
		sprintf (_buf, "%d.%d.%d.%d",_ipAddr[0],_ipAddr[1],_ipAddr[2],_ipAddr[3]);
		printf("IP:%s\n",_buf);
	}
	nSocketID0 = udpCreateSocket(_buf,0,6000);
	if(nSocketID0 < 0)
	{
		printf("udp socket create fail\n");
		return;
	}
	udpAttachRecv(nSocketID0,(FUNCPTR)OnSocket);
}




