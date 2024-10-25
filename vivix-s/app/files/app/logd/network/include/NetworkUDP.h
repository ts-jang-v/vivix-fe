#ifndef __NETWORKUDP_H__
#define __NETWORKUDP_H__

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

#include "NetworkConfig.h"
#include "NetworkMISC.h"

typedef enum
{
	eTYPE_SERVER = 0x01,
	eTYPE_CLIENT	
}eTYPE;

class CNetworkUDP
{
	public:
		CNetworkUDP();
		~CNetworkUDP();

		bool	Init(void* a_pThis, const char* a_sIP,int a_nPort, eTYPE a_eType, PFN_NETWORK_STATUS a_pfnOnStatus, PFN_NETWORK_RECEIVE a_pfnOnReceive, PFN_NETWORK_ERROR a_pfnOnError);
		bool	Start();
		bool	Stop();
		bool	Restart();
		int 	Send(unsigned char* a_sData, int a_nLength);

	private:
		bool	CheckStatus(eNETWORK_STATUS a_eStatus);
		bool	SetStatus(eNETWORK_STATUS a_eStatus);

	public:
		static	void*	sRoutine(void* a_pArg);
		bool	Routine(void);
		
	private:
		void*						m_pParent;
		pthread_t					m_nThread;
		volatile bool				m_bIsRunning;
		int 						m_nSocket;
		struct sockaddr_in			m_stServerAddr;
		volatile struct sockaddr_in	m_stInputAddr;
		volatile uint8_t 		m_nBufRecv[ETHERMTU];
		eNETWORK_STATUS				m_eStatus;
		eTYPE						m_eType;
		
	public:
		PFN_NETWORK_STATUS			m_pfnOnStatus;
		PFN_NETWORK_ERROR			m_pfnOnError;
		PFN_NETWORK_RECEIVE			m_pfnOnReceive;

};

#endif // __NETWORKUDP_H__

