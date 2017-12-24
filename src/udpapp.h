/*
 * udpapp.h
 *
 *  Created on: 2013-9-22
 *      Author: Administrator
 */

#ifndef UDPAPP_H_
#define UDPAPP_H_

#include <stdio.h>
#include <string.h>


#define SLOTINDEXMAX 12
#define SLICEIDMAX 4
#define REBOOTBOARDMAX 12

extern struct ip_addr peeraddr;
extern unsigned short peerport;

char ppIsBigEndian();
void ppEndian16(unsigned short * pData,int len16);
void ppEndian32(unsigned int * pData,int len32);

typedef enum{
	STATE_NOTIFY_ACK=1,
	STOP_NOTIFY_ACK,
	RESTORE_NOTIFY_ACK
}TypeAck;

typedef enum{
	STATE_NOTIFY_PKT=1,
	FAULT_NOTIFY_PKT,
	UPDATE_NOTIFY_PKT,
	REPLY_ACK_PKT,
	STOP_NOTIFY_PKT,
	RESTORE_NOTIFY_PKT
}TypePkt;

typedef struct{
	unsigned char PktPrefix[4];
	TypePkt Pkt_Type;
	unsigned char PktSuffix[4];
} Notify_Pkt;

typedef struct{
	Notify_Pkt packet;
	unsigned char Slot_Index;
	unsigned char slice_ID;
	unsigned char Back_Flag;
	unsigned char Reserved[12];
} state_Notify_Pkt;

typedef struct{
	Notify_Pkt packet;
	//to be realization

} fault_Notify_Pkt;

typedef struct{
	Notify_Pkt packet;
	unsigned short Hr_State;
	unsigned char Back_Index;
	unsigned char Reserved[18];
} update_Notify_Pkt;

typedef struct{
	Notify_Pkt packet;
	TypeAck Recv_Flag;
	unsigned char Reserved[2];
} reply_Ack_Pkt;

typedef struct{
	Notify_Pkt packet;
	unsigned char Slot_Index;
	unsigned char Pkt_count;
	unsigned char Reserved[13];
} stop_Notify_Pkt;

typedef struct{
	Notify_Pkt packet;
	unsigned char Slot_Index;
	unsigned char Slice_ID;
	unsigned char Reserved[13];
} restore_Notify_Pkt;

void createNotifyPktStruct(Notify_Pkt *pkt,unsigned short typeId);
void createUpdateNotifyPkt(update_Notify_Pkt *pkt,unsigned char arr[]);
void createReplyAckPkt(char arr[]);

int parseNotifyPktStruct(Notify_Pkt *pkt,unsigned char arr[],unsigned short length);
void parseStateNotifyPkt(state_Notify_Pkt *pkt,unsigned char arr[]);
void parseFaultNotifyPkt(fault_Notify_Pkt *pkt,unsigned char arr[]);
void parseStopNotifyPkt(stop_Notify_Pkt *pkt,unsigned char arr[]);
void parseRestoreNotifyPkt(restore_Notify_Pkt *pkt,unsigned char arr[]);
void sendReplyAckPkt(char *addr,int port,TypeAck type);
int CheckSlotTable(unsigned char slot,unsigned char *swIndex,unsigned char *portID);
int isRepeatPkt(unsigned short Slot_Index,unsigned short Slice_ID,unsigned short arr[]);
int setArr(unsigned short Slot_Index,unsigned short Slice_ID,unsigned short arr[]);
int checkArr(unsigned short Slot_Index,unsigned short arr[]);
void clearArr(unsigned short Slot_Index,unsigned short arr[]);
void printStatusArr(unsigned short arr[]);
void printRebootArr(int arr[]);

int udpSendPeerEx(int nID,char * strPeerAddr, int nPeerPort,void * pSendBuf,int nLen);

extern int nSocketID0;
extern void rioSendDB(unsigned short dbInfo,unsigned short dstID);
extern fault_Notify_Pkt faultNotifyPkt;
extern Notify_Pkt NotifyPkt;
extern state_Notify_Pkt stateNofityPkt;
extern update_Notify_Pkt updateNotifyPkt;
extern reply_Ack_Pkt ackPkt;
extern stop_Notify_Pkt stopNotifyPkt;
extern restore_Notify_Pkt restoreNotifyPkt;

#endif /* UDPAPP_H_ */
