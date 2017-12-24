#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/ip.h>
#include <linux/socket.h>
//#include <linux/route.h>
//#include <linux/udp.h>
//#include <linux/major.h>
//#include <linux/nfs_fs.h>
#include <net/route.h>
#include <pthread.h>
#include "udpSocketLib.h"


#define MAX_SOCKET_COUNT 64
#define MAX_ADDR_LEN 24

#ifndef ClearStructByZero
#define ClearStructByZero(st) memset(&(st),0,sizeof((st)))
#endif

extern 	unsigned long 	inet_addr (const char *);

static int s_nUDPSocketLibInited = 0;



struct UDPSocketDes
{
	int nPeerBinded;
	int nSocketFD;
	int nRecvThreadID;
	int nUseable;

	struct sockaddr_in localaddr;
	struct sockaddr_in groupaddr;
	struct sockaddr_in peeraddr_send;
	struct sockaddr_in peeraddr_recv;

	char * pRecvBuffer;
	void * pParentData;
	void * pLocalData;
	FUNCPTR callbackRecv;
} ;

static struct UDPSocketDes s_udpSocketDes[MAX_SOCKET_COUNT];
static char s_udpSocketAddress[MAX_ADDR_LEN];
#if 1
static struct UDPSocketDes * udpGetDes(int nID)
{
	struct UDPSocketDes * pDes;
	if((nID < 0)||(nID >= MAX_SOCKET_COUNT))
	{
		printf("udpGetDes: error socket index\n");
		return NULL;
	}
	pDes = &(s_udpSocketDes[nID]);
	if(pDes->nUseable == 0)
	{
		printf("udpGetDes: socket not create\n");
		return NULL;
	}
	return pDes;
}

/*****************************************************************
 * udpParseAddress: change ipaddress like("192.168.110.10")to struct sockaddr_in 
 * 
 *  */
static  int udpParseAddress(char * addr, struct sockaddr_in * addr_in)
{
	addr_in->sin_family = AF_INET;
	if(addr == NULL)
	{
		addr_in->sin_addr.s_addr = htonl(INADDR_ANY);
	}
	else
	{
		if(((addr_in->sin_addr).s_addr=(in_addr_t)inet_addr(addr)) == ((in_addr_t)ERROR)) return -1;
	}
	return 0;
}

/*************************************************************************8
 * LOCAL int udpRecvThread(int pClientData), daemon thread for udpsocket recv
 */
static void* RecvThread(void * arg)
{
		int peerlen,bRun;
		struct UDPSocketDes * pDes;
		int n,nID;

		nID = (int)arg;
		pDes = udpGetDes(nID);


		if(pDes == NULL)
		{
			printf("Error Descriptor, fail to run recv thread\n");
			return 0;
		}

		peerlen = sizeof(struct sockaddr_in);
		bRun = 1;
		while(bRun)
		{
			n = recvfrom(pDes->nSocketFD,pDes->pRecvBuffer,65536,0,(struct sockaddr *)&(pDes->peeraddr_recv), (socklen_t *__restrict)&peerlen);
			if(n < 0)
			{
				sleep(1);
				continue;
			}

			if(pDes->callbackRecv != NULL)
			{
				pDes->callbackRecv(pDes->pParentData,nID,pDes->pRecvBuffer,n);
			}
		}
		return 0;
}

#if 0
static  void udpRecvThread(int nID)
{
	int peerlen;
	int n;
	int bRun;
	struct UDPSocketDes * pDes;
	/*printf("run udpRecvThread @ id %d\n",nID);*/
	pDes = udpGetDes(nID);
	if(pDes == NULL)
	{
		printf("Error Descriptor, fail to run recv thread\n");
		return ;
	}
	peerlen = sizeof(struct sockaddr_in);
	bRun = 1;
	while(bRun)
	{
		n = recvfrom(pDes->nSocketFD,pDes->pRecvBuffer,65536,0,(struct sockaddr *)&(pDes->peeraddr_recv), (socklen_t *__restrict)&peerlen);
		if(n < 0)
		{
			sleep(1);
			continue;
		}
		if(pDes->callbackRecv != NULL)
		{
			pDes->callbackRecv(pDes->pParentData,nID,pDes->pRecvBuffer,n);
		}
	}
	return;
}
#endif

/************************************************************************
 * udpCreateSocket:
 * 	strAddr : NULL, use a default network card to send and recv,otherwise,use specified;
 *  strGroup: NULL, do not join group, otherwise ,join grou;
 *  int nPort: 0,use a default port, otherwise, use specified.
 * */

int udpCreateSocket(char * strAddr,char * strGroup,int nPort)
{
	int i;
	int socklen;
	int nID,tid;
	struct ip_mreq     mcast;
	struct UDPSocketDes * pDes = NULL;
	struct sockaddr_in anyaddr;
	unsigned char loopback  = 0;

	tid = 0;
/* if udpSocketLib not inited ,then init it*/
	if(0 == s_nUDPSocketLibInited)
	{
		for(i = 0;i<MAX_SOCKET_COUNT;i++)
		{
			ClearStructByZero(s_udpSocketDes[i]);
		}
		s_nUDPSocketLibInited = 1;
	}
/* Search a empty descriptor*/
	for(i = 0;i<MAX_SOCKET_COUNT;i++)
	{
		pDes = &(s_udpSocketDes[i]);
		if(pDes->nUseable != 0) continue;
		break;
	}
	if(i >= MAX_SOCKET_COUNT) /*no empty descript*/
	{
		printf("udpCreateSocket: fail to create udp socket, no empty entry\n");
		return -1;
	}

	nID = i;
/* create a udp socket*/
	socklen = sizeof(struct sockaddr_in);
	ClearStructByZero(pDes->localaddr);
/*parse any addr*/
	udpParseAddress(NULL,&anyaddr);
	anyaddr.sin_port = htons(nPort);
	//anyaddr.sin_len = (u_char)socklen;  zz zz
/* parse input strAddr*/
	if(udpParseAddress(strAddr,&(pDes->localaddr)) < 0)
	{
		printf("udpCreateSocket: fail to parse input address(");
		if(strAddr == NULL) printf("empty address");
		else printf(strAddr);
		printf(")\n");
		return -1;
	}
/*	pDes->localaddr.sin_port = htons(nPort);
	pDes->localaddr.sin_len = (u_char)socklen;*/
/* craete a udp socket*/	
	pDes->nSocketFD = socket(AF_INET,SOCK_DGRAM,0);
	if(pDes->nSocketFD < 0)
	{
		printf("udpCreateSocket: fail to create a udp socket\n");
		return -1;
	}
/*bind a soceaddr, in vxWorks or linux like operator system ,bind address must be zero
 * otherwise socket can not be received
 * */
	if(bind(pDes->nSocketFD,(struct sockaddr *)&anyaddr,socklen)<0)
	{
		printf("udpCreateSocket: fail to bind to localaddr\n");
		close(pDes->nSocketFD);
		return -1;
	}

/*set local data , for application use*/
	pDes->pLocalData = (void *)pDes;
/*alloc recv buffer for receive*/
	pDes->pRecvBuffer = (char *)malloc(0x10000);
	if(pDes->pRecvBuffer == NULL)
	{
		printf("udpCreateSocket: fail to alloc recv buffer\n");
		close(pDes->nSocketFD);
		return -1;
	}
/*set useable flag, at least ,this socket can send and recv p2p*/
	pDes->nUseable = 1;
/*craete recv thread*/
	//插口创建完成，下面必须创建一个阻塞线程用于接收数据
	pthread_create((pthread_t *__restrict)&tid,NULL,RecvThread, (void *__restrict)nID);
	if(pDes->nRecvThreadID == ERROR)
	{
		printf("udpCreateSocket: fail to create recv thread\n");
		close(pDes->nSocketFD);
		return -1;
	}

/*join group now*/
	if(strGroup != NULL)
	{
		ClearStructByZero(mcast);
		ClearStructByZero(pDes->groupaddr);
		if(udpParseAddress(strGroup,&(pDes->groupaddr)) < 0) /*join group fail, do not return*/
		{
			printf("udpCreateSocket: fail to parse input address(");
			printf(strGroup);
			printf(")\n");
		}
		mcast.imr_multiaddr.s_addr = pDes->groupaddr.sin_addr.s_addr;
		mcast.imr_interface.s_addr = pDes->localaddr.sin_addr.s_addr;
		if(setsockopt(pDes->nSocketFD,IPPROTO_IP,IP_ADD_MEMBERSHIP,(char *)&mcast,sizeof(mcast))<0)
		{
			printf("udpCreateSocket: fail to join group(");
			printf(strGroup);
			printf(")\n");
		}
		if(setsockopt(pDes->nSocketFD,IPPROTO_IP,IP_MULTICAST_LOOP,(char *)&loopback,sizeof(unsigned char)) < 0)
		{
			printf("warning: disable multicast loop back failed\n");
		}
	}
	return nID;
}


int udpJoinGroup(int nID,char * strAddr,char * strGroup)
{
	struct UDPSocketDes * pDes;
	struct ip_mreq     mcast;
	//int socketlen = sizeof(struct sockaddr_in);
	unsigned char loopback  = 0;
	
	pDes = udpGetDes(nID);
	if(pDes == NULL) 
		return -1;
	
	if( strAddr == NULL || strGroup == NULL )
		return -1;
	
	/*join group now*/		
	ClearStructByZero(mcast);
	ClearStructByZero(pDes->groupaddr);
	if(udpParseAddress(strGroup,&(pDes->groupaddr)) < 0) /*join group fail, do not return*/
	{
		printf("udpCreateSocket: fail to parse input address(");
		printf(strGroup);
		printf(")\n");
	}
	mcast.imr_multiaddr.s_addr = pDes->groupaddr.sin_addr.s_addr;
	mcast.imr_interface.s_addr = pDes->localaddr.sin_addr.s_addr;
	if(setsockopt(pDes->nSocketFD,IPPROTO_IP,IP_ADD_MEMBERSHIP,(char *)&mcast,sizeof(mcast))<0)
	{
		printf("udpCreateSocket: fail to join group(");
		printf(strGroup);
		printf(")\n");
	}
	if(setsockopt(pDes->nSocketFD,IPPROTO_IP,IP_MULTICAST_LOOP,(char *)&loopback,sizeof(unsigned char)) < 0)
	{
		printf("warning: disable multicast loop back failed\n");
	}
	return 1;
}

/*************************************************************************************
 * int udpBindPeer(int nID,char * strPeerAddr, int nPeerPort),
 * bind defaultsend to peer address,this is not necessary
 * 
 */
int udpBindPeer(int nID,char * strPeerAddr, int nPeerPort)
{
	struct UDPSocketDes * pDes;
	//int socketlen = sizeof(struct sockaddr_in);
	pDes = udpGetDes(nID);
	if(pDes == NULL) return -1;

	if(strPeerAddr == NULL)
	{
		pDes->nPeerBinded = 0;
		printf("udpBindPeer: unBind peer address\n");
		return 1;
	}
	if((nPeerPort >= 65536)||(nPeerPort <= 0))
	{
		printf("udpBindPeer: Invalid input of peer udp port\n");
		pDes->nPeerBinded = 0;
		return -1;
	}
	if(udpParseAddress(strPeerAddr,&(pDes->peeraddr_send)) < 0)
	{
		printf("udpBindPeer: Invalid input of peer ip address\n");
		pDes->nPeerBinded = 0;
		return -1;
	}
	pDes->peeraddr_send.sin_port = htons(nPeerPort);
	//pDes->peeraddr_send.sin_len = (u_char)socketlen;  zz zz
	pDes->nPeerBinded = 1;
	return 0;
}

/************************************************************************************************
 * int udpSendPeer(int nID,void * pSendBuf,int nLen); OK return 0, Fail -1
 * Send data to binded peer address and port
 */
int udpSendPeer(int nID,void * pSendBuf,int nLen)/*OK return 0, Fail -1*/
{
	struct UDPSocketDes * pDes;
	int socklen;
	pDes = udpGetDes(nID);
	if(pDes == NULL) return -1;

	if(pDes->nPeerBinded <= 0)
	{
		printf("udpSendPeer: peer address and port not binded\n");
		return -1;
	}
	socklen = sizeof(struct sockaddr_in);
	return sendto(pDes->nSocketFD,(char *)pSendBuf,nLen,0,(struct sockaddr *)&(pDes->peeraddr_send),socklen);
}
/************************************************************************************
 * udpSendPeerEx: Send Data to specified ipaddress and portnum, not use binded peer address
 */
int udpSendPeerEx(int nID,char * strPeerAddr, int nPeerPort,void * pSendBuf,int nLen)
{
	struct UDPSocketDes * pDes;
	struct sockaddr_in peeraddr;
	int socklen;
	
	pDes = udpGetDes(nID);
	if(pDes == NULL) return -1;

	if(strPeerAddr == NULL)
	{
		printf("udpSendPeer: invalid input peer address\n");
		return -1;
	}
	if(udpParseAddress(strPeerAddr,&peeraddr) <0)
	{
		printf("udpSendPeerEx: parse peer address error\n");
		return -1;
	}
	if((nPeerPort <= 0)||(nPeerPort >= 65536))
	{
		printf("udpSendPeerEx: invalid peer port : %d \n", nPeerPort);
		return -1;
	}
	socklen = sizeof(struct sockaddr_in);
	//peeraddr.sin_len = (u_char)socklen;		zz zz
	peeraddr.sin_port = htons(nPeerPort);
	return sendto(pDes->nSocketFD,(char *)pSendBuf,nLen,0,(struct sockaddr *)&(peeraddr),socklen);
}
/*****************************************************************************
 * udpAttachRecv(int nID,FUNCPTR callbackRecv)
 * addach recvcallback function , define it as static int OnSocket(void * pParentData,int nID,char * pRecvBuf,int nRecvLen);
 * pParentData is used for c++, to trans local function to member function
 */
int udpAttachRecv(int nID,FUNCPTR callbackRecv)
{
	struct UDPSocketDes * pDes;
	pDes = udpGetDes(nID);
	if(pDes == NULL) return -1;
	pDes->callbackRecv = callbackRecv;
	return 0;
}
/*********************************************************************************
 * udpEnableBroadCast()
 * 
 */
int udpEnableBroadCast(int nID)
{
	int bBroadcast = 1;
	int n;
	struct UDPSocketDes * pDes;
	pDes = udpGetDes(nID);
	if(pDes == NULL) return -1;
	n = setsockopt(pDes->nSocketFD,SOL_SOCKET,SO_BROADCAST,(char *)&bBroadcast,sizeof(int));
	/*注意返回－1表示成功*/
	return n+1;
}
/*********************************************************************************
 * udpDisableBroadCast()
 * 
 */
int udpDisableBroadCast(int nID)
{
	int bBroadcast = 0;
	int n;
	struct UDPSocketDes * pDes;
	pDes = udpGetDes(nID);
	if(pDes == NULL) return -1;
	n = setsockopt(pDes->nSocketFD,SOL_SOCKET,SO_BROADCAST,(char *)&bBroadcast,sizeof(int));
	/*注意返回－1表示*/
	return n+1;
}

/***********************************************************************************
 * udpSetParentData, just set for c++ interface, as a param for function pointer callbackRecv
 */
int udpSetParentData(int nID,void * pParentData)
{
	struct UDPSocketDes * pDes;
	pDes = udpGetDes(nID);
	if(pDes == NULL) return -1;
	pDes->pParentData = pParentData;
	return 0;
}
/***********************************************************************************
 * udpGetParentData, just set for c++ interface, as a param for function pointer callbackRecv
 */
void * udpGetParentData(int nID)
{
	struct UDPSocketDes * pDes;
	pDes = udpGetDes(nID);
	if(pDes == NULL) return NULL;
	return pDes->pParentData;
}
char * udpGetRecvAddress(int nID)
{
	struct UDPSocketDes * pDes;
	int m;
	unsigned char n1,n2,n3,n4;
	pDes = udpGetDes(nID);
	if(pDes == NULL) return NULL;
	m = pDes->peeraddr_recv.sin_addr.s_addr;
	n1 = m & 0xff;m >>= 8;
	n2 = m & 0xff;m >>= 8;
	n3 = m & 0xff;m >>= 8;
	n4 = m & 0xff;
	sprintf(s_udpSocketAddress,"%d.%d.%d.%d",n1,n2,n3,n4);
	return s_udpSocketAddress;
}
int udpGetRecvPort(int nID)
{
	struct UDPSocketDes * pDes;
	pDes = udpGetDes(nID);
	if(pDes == NULL) return 0;
	return ntohs(pDes->peeraddr_recv.sin_port);
}
/*******************************************************************************
 * int udpGetSocketFD(int nID); take the udpSocket fd for advanced user
 * 
 */
int udpGetSocketFD(int nID)
{
	struct UDPSocketDes * pDes;
	pDes = udpGetDes(nID);
	if(pDes == NULL) return 0;
	return pDes->nSocketFD;
}

#if 0
static int nSocketID0 = -1;

static void udpTest_OnSocket(void * pParent,int nID,char * buf,int len)
{
	//void * parent = pParent;
	char * peeraddress;
	int peerport;
	if(pParent != NULL)
	{
		return;
	}
	peeraddress = udpGetRecvAddress(nID);
	peerport = udpGetRecvPort(nID);
	if(len < 0) return;
	printf("Recv from addr :");
	printf(peeraddress);
	printf("\n");
	printf("Recv from port : %d\n",peerport);
	udpSendPeerEx(nID,peeraddress,peerport,(void *)buf,len);
}
void udpTest()
{
	nSocketID0 = udpCreateSocket("168.168.168.9","230.168.168.168",5190);
	if(nSocketID0 < 0)
	{
		printf("udp socket create fail\n");
		return;
	}
	udpAttachRecv(nSocketID0,(FUNCPTR)udpTest_OnSocket);
	udpBindPeer(nSocketID0,"230.168.168.168",6003);
}
void udpTestSendEx(char * peeraddr, int peerport)
{
	int i;
	char buf[256];
	for(i = 0;i<256;i++)
	{
		buf[i] = (i+0x30)&0xff;
	}
	udpSendPeerEx(nSocketID0,peeraddr,peerport,buf,256);
}
void udpTestSend()
{
	int i;
	char buf[256];
	for(i = 0;i<256;i++)
	{
		buf[i] = (i+0x30)&0xff;
	}
	udpSendPeer(nSocketID0,buf,256);
}
#endif

#endif
