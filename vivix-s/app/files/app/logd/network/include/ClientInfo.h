#ifndef __CLIENTINFO_H__
#define __CLIENTINFO_H__

#include <stdio.h>			// fprintf()
#include <stdlib.h>			// malloc(), free()
#include <unistd.h>			// getpid()
#include <stdbool.h>
#include <string.h>			// memset(), memcpy()
#include <fcntl.h>			// fcntl()
#include <sys/socket.h>		// socket(), setsocketopt(), connect()
#include <sys/poll.h>		// struct pollfd, poll()
#include <sys/ioctl.h>  	// ioctl()
#include <arpa/inet.h>		// inet_ntoa()
#include <errno.h>			// errno
#include <assert.h>			// assert()
#include <net/ethernet.h>	// ETHERMTU
#include <net/if.h>			// struct ifconf, struct ifreq
#include <netinet/tcp.h>	//TCP_NODELAY
#include <time.h>

#define PACKET_BUF_LENGTH 50*1024

class CClientInfo
{
	public:
		CClientInfo(void* a_pParent, int a_nIndex, struct sockaddr_in* a_stSockAddr, int a_nFd):m_nBufStartPos(0), m_nBufEndPos(0)
	{
		m_pParent = a_pParent;
		m_nIndex = a_nIndex;
		memcpy((void*)&m_stAddr, a_stSockAddr, sizeof(struct sockaddr));
		m_nFd = a_nFd;
		m_bIsRunning = true;
		m_nConnectedTime = time(NULL);
	};

		~CClientInfo(){ };

		void Init(int a_nIndex, struct sockaddr_in* a_stSockAddr, int a_nFd)
		{
			m_nIndex = a_nIndex;
			memcpy((void*)&m_stAddr, a_stSockAddr, sizeof(struct sockaddr));
			m_nFd = a_nFd;
			m_bIsRunning = true;
		}

	public:
		static void*	sServiceReceiver(void* a_pArg);
		bool			ServiceReceiver(void);


	public:
		void*						m_pParent;
		volatile bool				m_bIsRunning;
		volatile struct sockaddr_in m_stAddr;
		time_t						m_nConnectedTime;
		int							m_nIndex;
		int							m_nFd;
		pthread_t					m_nReceiverThread;
		unsigned char				m_cPacketBuf[PACKET_BUF_LENGTH];
		int							m_nBufStartPos;
		int							m_nBufEndPos;

};

#endif // __CLIENTINFO_H__

