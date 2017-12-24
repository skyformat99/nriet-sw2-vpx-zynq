/*
 * Copyright (c) 2009 Xilinx, Inc.  All rights reserved.
 *
 * Xilinx, Inc.
 * XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A
 * COURTESY TO YOU.  BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS
 * ONE POSSIBLE   IMPLEMENTATION OF THIS FEATURE, APPLICATION OR
 * STANDARD, XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION
 * IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE
 * FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.
 * XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO
 * THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO
 * ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE
 * FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include "udpapp.h"


fault_Notify_Pkt faultNotifyPkt;
Notify_Pkt NotifyPkt;
state_Notify_Pkt stateNofityPkt;
update_Notify_Pkt updateNotifyPkt;
reply_Ack_Pkt ackPkt;
stop_Notify_Pkt stopNotifyPkt;
restore_Notify_Pkt restoreNotifyPkt;
extern int hrReadynumbers;
extern int hrBackupIndex;
extern unsigned short statusArr[12];


void createUpdateNotifyPkt(update_Notify_Pkt *pkt,unsigned char arr[])
{
	unsigned int i;
	for(i=0;i<4;i++)
	{
		arr[i]=(pkt->packet.PktPrefix[i])&0xff;
	}
	arr[i++]=(pkt->packet.Pkt_Type)&0xff;
	arr[i++]=((pkt->Hr_State)>>8)&0xff;
	arr[i++]=(pkt->Hr_State)&0xff;
	arr[i++]=(pkt->Back_Index)&0xff;
	for(i=20;i<24;i++)
	{
		arr[i]=(pkt->packet.PktSuffix[i])&0xff;
	}
	return;
}
void createReplyAckPkt(char arr[])
{
	unsigned int i;
	for(i=0;i<4;i++)
	{
		arr[i]=(ackPkt.packet.PktPrefix[i])&0xff;
	}
	arr[i++]=(ackPkt.packet.Pkt_Type)&0xff;
	arr[i++]=(ackPkt.Recv_Flag)&0xff;
	for(i=8;i<12;i++)
	{
		arr[i]=(ackPkt.packet.PktSuffix[i])&0xff;
	}
	return;
}
void parseStateNotifyPkt(state_Notify_Pkt *pkt,unsigned char arr[])
{
	int i=5;
	pkt->packet=NotifyPkt;
	pkt->Slot_Index=arr[i++]&0xff;
	pkt->slice_ID=arr[i++]&0xff;
	if(!isRepeatPkt(pkt->Slot_Index,pkt->slice_ID,statusArr))
	{
		printf("This packet has shown up before!\n\r");
		return;
	}
	hrReadynumbers++;
	printStatusArr(statusArr);
	pkt->Back_Flag=arr[i++]&0xff;
	if(pkt->Back_Flag==1)
	{
		//Ignore Back_Flag from the same Slot_Index
		if(hrBackupIndex!=pkt->Slot_Index)
		{
			printf("Warning: Another Backup Slot prev[%d]!\n\r",hrBackupIndex);
		}
		hrBackupIndex=pkt->Slot_Index;
		printf("Current Backup Slot[%d]\n\r",hrBackupIndex);
	}
	//xil_printf("hrReadynumbers=%d\n\r",hrReadynumbers);
}
void parseFaultNotifyPkt(fault_Notify_Pkt *pkt,unsigned char arr[])
{
	pkt->packet=NotifyPkt;
	//to be realized
	return;
}

int parseNotifyPktStruct(Notify_Pkt *pkt,unsigned char arr[],unsigned short length)
{
	int i;
	/*
	for(i=0;i<length;i++)
	{
		printf("%02x ",(arr[i]&0xff));
	}
	printf("\n\r");
	*/
	for(i=0;i<4;i++)
	{
		pkt->PktPrefix[i]=arr[i]&0xff;
		if(pkt->PktPrefix[i]!=0xAA)
		{
			return 0;
		}
	}
	pkt->Pkt_Type=arr[i++]&0xff;

	for(i=length-4;i<length;i++)
	{
		pkt->PktPrefix[i]=arr[i]&0xff;
		if(pkt->PktPrefix[i]!=0xFF)
		{
			return 0;
		}
	}
	return 1;
}

void parseStopNotifyPkt(stop_Notify_Pkt *pkt,unsigned char arr[])
{
	int i=5;
	pkt->packet=NotifyPkt;
	pkt->Slot_Index=arr[i++]&0xff;
	pkt->Pkt_count=arr[i++]&0xff;
	return;
}
void parseRestoreNotifyPkt(restore_Notify_Pkt *pkt,unsigned char arr[])
{

	int i=5;
	pkt->packet=NotifyPkt;
	pkt->Slot_Index=arr[i++]&0xff;
	pkt->Slice_ID=arr[i++]&0xff;
	return;
}

void sendReplyAckPkt(char *addr,int port,TypeAck type)
{
	int i;
	char str[12];

	memset(&ackPkt,0,sizeof(reply_Ack_Pkt));
	for(i=0;i<4;i++)
	{
		ackPkt.packet.PktPrefix[i]=0XAA;
	}
	for(i=8;i<12;i++)
	{
		ackPkt.packet.PktSuffix[i]=0XFF;
	}
	ackPkt.packet.Pkt_Type=REPLY_ACK_PKT;
	ackPkt.Recv_Flag=type;

	memset(str,1,sizeof(str));
	createReplyAckPkt(str);

	udpSendPeerEx(nSocketID0,addr,port,str,sizeof(str));
	//udp_transfer_data(addr,port,str,sizeof(str));
	return;
}
int CheckSlotTable(unsigned char slot,unsigned char *swIndex,unsigned char *portID)
{
	switch(slot)
	{
		case 1:
			*swIndex=1;
			*portID=1;
			break;
		case 2:
				*swIndex=0;
				*portID=2;
				break;
		case 3:
				*swIndex=0;
				*portID=5;
				break;
		case 4:
				*swIndex=0;
				*portID=9;
				break;
		case 5:
				*swIndex=0;
				*portID=4;
				break;
		case 8:
				*swIndex=0;
				*portID=8;
				break;
		case 9:
				*swIndex=1;
				*portID=2;
				break;
		case 10:
				*swIndex=1;
				*portID=5;
				break;
		case 11:
				*swIndex=1;
				*portID=9;
				break;
		case 12:
				*swIndex=1;
				*portID=4;
				break;
		default:
			return 0;
	}
	return 1;
}

int isRepeatPkt(unsigned short Slot_Index,unsigned short Slice_ID,unsigned short arr[])
{
	unsigned short temp;
	if((0>=Slot_Index)||(Slot_Index>SLOTINDEXMAX)||(0>=Slice_ID)||(Slice_ID>SLICEIDMAX))
	{
		printf("ERROR:The value of Slot Index OR Slice ID from the packet is invalid!\n\r");
		return 0;
	}
	temp=(arr[Slot_Index-1]>>(Slice_ID-1))&0x01;
	if(!temp)
	{
		arr[Slot_Index-1]=(0x01<<(Slice_ID-1))|arr[Slot_Index-1];
		return 1;
	}else
	{
		return 0;
	}
}

void clearArr(unsigned short Slot_Index,unsigned short arr[])
{
	if((0>=Slot_Index)||(Slot_Index>SLOTINDEXMAX))
	{
		printf("ERROR:The value of Slot Index OR Slice ID from the packet is invalid!\n\r");
		return;
	}
	arr[Slot_Index-1]=0x00;
}

void printStatusArr(unsigned short arr[])
{
	int i,j;
	unsigned short temp;
	for(i=0;i<SLOTINDEXMAX;i++)
	{
		printf("| ");
		/*
		 * PRINT FORMAT:D C B A
		for(j=SLICEIDMAX-1;j>=0;j--)
		{
			temp=(arr[i]>>j)&0x01;
			printf("%d ",temp);
		}
		*/
		//PRINT FORMAT:A B C D
		for(j=0;j<SLICEIDMAX;j++)
		{
			temp=(arr[i]>>j)&0x01;
			printf("%d ",temp);
		}
	}
	printf("|\n\r");

}

int setArr(unsigned short Slot_Index,unsigned short Slice_ID,unsigned short arr[])
{
	unsigned short temp;
	if((0>=Slot_Index)||(Slot_Index>SLOTINDEXMAX)||(0>=Slice_ID)||(Slice_ID>SLICEIDMAX))
	{
		printf("ERROR:The value of Slot Index OR Slice ID from the packet is invalid[%d][%d]!\n\r",Slot_Index,Slice_ID);
		return 0;
	}
	temp=(arr[Slot_Index-1]>>(Slice_ID-1))&0x01;
	if(!temp)
	{
		arr[Slot_Index-1]=(0x01<<(Slice_ID-1))|arr[Slot_Index-1];
		return 1;
	}else
	{
		printf("SetStatusArr corresponding bit has been set already, possible the bit was not clean before!\n\r");
		return 0;
	}
}
int checkArr(unsigned short Slot_Index,unsigned short arr[])
{
	unsigned short temp;
	if((0>=Slot_Index)||(Slot_Index>SLOTINDEXMAX))
	{
		printf("ERROR:The value of Slot Index from the packet is invalid![%d]\n\r",Slot_Index);
		return 0;
	}
	temp=arr[Slot_Index-1];

	if(temp==0x0F)
	{
		return 1;
	}else
	{
		return 0;
	}
}


void printRebootArr(int arr[])
{
	unsigned short i;
	printf("Reboot Array:");
	for(i=0;i<REBOOTBOARDMAX;i++)
	{
		printf("| %d ",arr[i]);
	}
	printf("|\n\r");
}
