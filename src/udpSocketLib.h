#ifndef _udpSocketLib_h_20100810
#define _udpSocketLib_h_20100810
/**********************************************************************************************************
*updsocketLib 采用UDP协议进行通讯的函数封装,适用并测试于vxWorks6.8,
*
*函数说明:
*int udpCreateSocket(char * strAddr, char * strGroup, int nPort);
* 功能:初始化一个UDP通讯插口
* 参数:
*  1: char * strAddr  需要建立插口的ip地址,这个参数仅在加入组播时有效,表示选用哪一块网卡加入组播,如果不用加入组播,设为NULL即可
*  2: char * strGroup 需要加入的组播地址,如果不加入组播,设为NULL即可
*  3: int nPort       需要建立插口的端口地址
*  返回:
*  ID 号 大于或等于0 表示成功,-1表示失败,这个ID号是以后函数必须使用的.
*  说明: 最多创建64个插口
*
*int udpBindPeer(int nID,char * strPeerAddr, int nPeerPort);
* 功能: 绑定发送地址,便于以后使用udpSendPeer函数进行发送,而不需要每次指定
* 参数: 
*  1: int nID  插口索引号
*  2: char * strPeerAddr  对方ip地址
*  3: int nPeerPort 对方端口
* 返回:
*  0 表示成功,-1表示失败
*
*
*例子:
*

#include "stdio.h"
#include "udpSocketLib.h"

static int nSocketID0 = -1;

static void OnSocket(void * pParent,int nID,char * buf,int len)
{
	char * peeraddress;
	int peerport;
	peeraddress = udpGetRecvAddress(nID);
	peerport = udpGetRecvPort(nID);
	if(len < 0) return;
	printf("Recv from addr :");
	printf(peeraddress);
	printf("\n");
	printf("Recv from port : %d\n",peerport);
	udpSendPeerEx(nID,peeraddress,peerport,(void *)buf,len);
}
void NetInit()
{
	nSocketID0 = udpCreateSocket("168.168.168.9","230.168.168.168",5190);
	if(nSocketID0 < 0)
	{
		printf("udp socket create fail\n");
		return;
	}
	udpAttachRecv(nSocketID0,(FUNCPTR)OnSocket);
	udpBindPeer(nSocketID0,"230.168.168.168",6003);
}
void SendEx(char * peeraddr, int peerport)
{
	int i;
	char buf[256];
	for(i = 0;i<256;i++)
	{
		buf[i] = (i+0x30)&0xff;
	}
	udpSendPeerEx(nSocketID0,peeraddr,peerport,buf,256);
}
void Send()
{
	int i;
	char buf[256];
	for(i = 0;i<256;i++)
	{
		buf[i] = (i+0x30)&0xff;
	}
	udpSendPeer(nSocketID0,buf,256);
}

*
*
************************************************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif


typedef int 		(*FUNCPTR) ();
#define OK		0
#define ERROR		(-1)

int udpCreateSocket(char * strAddr, char * strGroup, int nPort);/*return socket id,Fail -1*/
int udpBindPeer(int nID,char * strPeerAddr, int nPeerPort);/*OK return 0, Fail -1*/
int udpSendPeer(int nID,void * pSendBuf,int nLen);/*OK return 0, Fail -1*/
int udpSendPeerEx(int nID,char * strPeerAddr, int nPeerPort,void * pSendBuf,int nLen);/*OK return 0, Fail -1*/
int udpAttachRecv(int nID,FUNCPTR callbackRecv);	/*OK return 0, Fail -1*/
int udpEnableBroadCast(int nID);
int udpDisableBroadCast(int nID);
int udpDestroySocket(int nID);
int udpSetParentData(int nID,void * pParentData);
void * udpGetParentData(int nID);
char * udpGetRecvAddress(int nID);
int udpGetRecvPort(int nID);
int udpGetSocketFD(int nID);
int udpJoinGroup(int nID,char * strAddr,char * strGroup);

#ifdef __cplusplus
}
#endif

#endif/*_udpSocketLib_h_20100810*/

