#ifndef __NETWORKTCPSERVER_H__
#define __NETWORKTCPSERVER_H__

#include <stdio.h>			// fprintf()
#include <stdlib.h>			// malloc(), free()
#include <unistd.h>			// getpid()
#include <stdbool.h>
#include <string.h>			// memset(), memcpy()
#include <fcntl.h>			// fcntl()
#include <sys/socket.h>		// socket(), setsocketopt(), connect()
#include <sys/types.h>		// setsocketopt()
#include <sys/poll.h>		// struct pollfd, poll()
#include <sys/ioctl.h>  	// ioctl()
#include <arpa/inet.h>		// inet_ntoa()
#include <errno.h>			// errno
#include <assert.h>			// assert()
#include <net/ethernet.h>	// ETHERMTU
#include <net/if.h>			// struct ifconf, struct ifreq
#include <netinet/tcp.h>	//TCP_NODELAY
#include <pthread.h>

#include "NetworkConfig.h"
#include "NetworkMISC.h"
#include "ClientInfo.h"

class CClientInfo;

class CNetworkTCPServer
{
	public:
		CNetworkTCPServer();
		~CNetworkTCPServer();

		bool	Init(void* a_pParent, int a_nPort, int a_nMaxClient, PFN_NETWORK_STATUS a_pfnOnStatus, PFN_NETWORK_CONNECT a_pfnOnConnect, PFN_NETWORK_DISCONNECT a_pfnOnDisconnect, 
				PFN_NETWORK_RECEIVE a_pfnOnReceive, PFN_NETWORK_ERROR a_pfnOnError);
		bool	Start();
		bool	Stop();
		bool	Restart();
		int 	Send(int a_nIndex, unsigned char* a_sData, int a_nLength);
		CClientInfo* GetClientInfo(int a_nIndex);
		void	PrintClient(void);
		bool	WaitReceiver(bool a_bFlag);
		bool	DelClient(int a_nFd);
		
	private:
		bool	AddClient(struct sockaddr_in* a_stClientAddr, int a_nFd);		
		bool	CheckStatus(eNETWORK_STATUS a_eStatus);
		bool	SetStatus(eNETWORK_STATUS a_eStatus);


	public:
		static	void*	sRoutine(void* a_pArg);
		bool	Routine(void);

	private:
		pthread_t					m_nThread;
		int 						m_nSocket;
		struct sockaddr_in			m_stServerAddr;
		int 						m_nMaxClient;
		CClientInfo*				m_pClientInfo[TCP_SERVER_MAX_CLIENTS];
		int							m_nCurIndex;
		struct	pollfd				m_stPollfds[TCP_SERVER_MAX_CLIENTS+1];
		eNETWORK_STATUS				m_eStatus;

	public:
		PFN_NETWORK_STATUS			m_pfnOnStatus;
		PFN_NETWORK_ERROR			m_pfnOnError;
		PFN_NETWORK_RECEIVE			m_pfnOnReceive;
		PFN_NETWORK_CONNECT			m_pfnOnConnect;
		PFN_NETWORK_DISCONNECT		m_pfnOnDisconnect;

		void*						m_pParent;
		volatile bool				m_bReceiverStarted;
		volatile bool				m_bIsRunning;
		int							m_nClientCnt;

};

#endif // __NETWORKTCPSERVER_H__
