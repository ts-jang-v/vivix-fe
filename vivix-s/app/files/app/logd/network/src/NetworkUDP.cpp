#include "NetworkUDP.h"

#include<pthread.h>

CNetworkUDP::CNetworkUDP()
	:m_pParent(NULL), m_nThread(0), m_bIsRunning(false), m_nSocket(-1), m_eStatus(eNETWORK_STATUS_NONE), m_eType(eTYPE_SERVER),
		 m_pfnOnStatus(NULL), m_pfnOnError(NULL), m_pfnOnReceive(NULL)
{

}


CNetworkUDP::~CNetworkUDP()
{

}



bool	CNetworkUDP::Init(void* a_pThis, const char* a_sIP,int a_nPort,  eTYPE a_eType,
		PFN_NETWORK_STATUS a_pfnOnStatus, PFN_NETWORK_RECEIVE a_pfnOnReceive, 
		PFN_NETWORK_ERROR a_pfnOnError)
{
	
	m_pfnOnStatus = a_pfnOnStatus;
	m_pfnOnReceive = a_pfnOnReceive;
	m_pfnOnError = a_pfnOnError;

	if(a_nPort < 0)
	{
		if(m_pfnOnError)
			m_pfnOnError(a_pThis, eNETWORK_ERROR_ILLEGAL_PORT,0);
		return false;
	}

	if(CheckStatus(eNETWORK_STATUS_STOP))
		Stop();

	m_pParent = a_pThis;
	
	m_eType = a_eType;

	m_stServerAddr.sin_family = AF_INET;
	
	if(m_eType == eTYPE_SERVER)
		m_stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	else if(m_eType == eTYPE_CLIENT)
		m_stServerAddr.sin_addr.s_addr = inet_addr(a_sIP);
		
	m_stServerAddr.sin_port = htons(a_nPort);
	
	SetStatus(eNETWORK_STATUS_INIT);

	return true;

}

bool	CNetworkUDP::Start()
{
	if(CheckStatus(eNETWORK_STATUS_START) == false){
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_CHECK_STATUS,0);
		return false;
	}

	m_nSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if(m_nSocket < 0)
	{
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_CREATE_SOCKET_FAILED,(int)errno);
		return false;
		
	}

	if(m_eType == eTYPE_SERVER)
	{
	struct ifconf	ifconf;
	struct ifreq	ifreq[MAX_NIC_NUM];

	ifconf.ifc_len = sizeof(struct ifreq) * MAX_NIC_NUM;
	ifconf.ifc_ifcu.ifcu_req = ifreq;

	if( ioctl( m_nSocket, SIOCGIFCONF, & ifconf ) == -1 )
	{
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_IOCTL_FAILED,(int)errno);
		return false;
	}
	else
	{
		m_stServerAddr.sin_addr.s_addr = htonl( INADDR_ANY);
	}
	if( bind(m_nSocket,( struct sockaddr * ) & m_stServerAddr,sizeof( struct sockaddr )) == -1	)
	{
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_BIND_FAILED,(int)errno);
		return false;
	}
	}	
	USleep(10);

	m_bIsRunning = true;

	if(pthread_create(&m_nThread, NULL, sRoutine, (void*)this) != false)
	{
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_CREATE_THREAD_FAILED,(int)errno);
		m_bIsRunning = false;
		return false;
	}


	SetStatus(eNETWORK_STATUS_START);

	return true;
}

bool	CNetworkUDP::Stop()
{
	if(CheckStatus(eNETWORK_STATUS_STOP) == false)
	{
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_CHECK_STATUS,0);
		return false;
	}

	m_bIsRunning = false;

	if(close(m_nSocket) == -1)
	{
		m_pfnOnError(m_pParent, eNETWORK_ERROR_CLOSE_SOCKET_FAILED,(int)errno);
		return false;
	}

	SetStatus(eNETWORK_STATUS_STOP);

	return true;
}

bool	CNetworkUDP::Restart()
{
	if(CheckStatus(eNETWORK_STATUS_RESTART) == false)
	{
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_CHECK_STATUS,0);
		return false;
	}

	if((Stop() && Start()) == false)
	{
		return false;
	}

	//if(m_pfnOnStatus)
	//	m_pfnOnStatus(m_pParent, eNETWORK_STATUS_RESTART);
	return true;
}


int 	CNetworkUDP::Send(unsigned char* a_sData, int a_nLength)
{
	struct sockaddr stAddr;
	int nret = 0;
	int npos = 0;
	int nlen = a_nLength;
	
	if(CheckStatus(eNETWORK_STATUS_SEND) == false)
	{
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_CHECK_STATUS,0);
		return false;
	}

//	if(strlen((const char*)a_sData) != a_nLength)
//	{
//		if(m_pfnOnError)
//			m_pfnOnError(m_pParent, eNETWORK_ERROR_UNMATCH_MSG_LENGTH, 0);
//		return false;
//	}
	
	if(m_eType == eTYPE_SERVER)
		memcpy(&stAddr, (const void*)&m_stInputAddr, sizeof(struct sockaddr));
	else if(m_eType == eTYPE_CLIENT)
		memcpy(&stAddr, (const void*)&m_stServerAddr, sizeof(struct sockaddr));
		
	while(1)
	{
		nret = sendto(m_nSocket, (void*)a_sData, a_nLength, 0 , (struct sockaddr*) &stAddr, sizeof(stAddr)/* sizeof(struct sockaddr) */ );
	
		if( nret < 0)
		{
			if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK )				
				continue;
				
			if(m_pfnOnError)
				m_pfnOnError(m_pParent, eNETWORK_ERROR_SEND_FAILED,ERRNO);
				
			return false;
		}
		else
		{
			if( (nret > 0) && (nret < nlen))
			{
				npos += nret;
				nlen = a_nLength - npos;
				continue;
			}

			break;
		}		
	}
		
	//if(m_pfnOnStatus)
	//	m_pfnOnStatus(m_pParent, eNETWORK_STATUS_SEND);
	
	return true;
}


bool	CNetworkUDP::CheckStatus(eNETWORK_STATUS a_eStatus)
{
	bool ret = false;
	switch((int)m_eStatus){
	case eNETWORK_STATUS_INIT:
		ret = (bool)(eNETWORK_STATUS_START & a_eStatus);
		break;
	case eNETWORK_STATUS_START:
		ret = (bool)((eNETWORK_STATUS_INIT | eNETWORK_STATUS_STOP  | eNETWORK_STATUS_RESTART | eNETWORK_STATUS_SEND ) & a_eStatus);
		break;
	case eNETWORK_STATUS_STOP:
		ret = (bool)((eNETWORK_STATUS_INIT | eNETWORK_STATUS_START) & a_eStatus);
		break;
	default:
		break;

	}

	return ret;
}

bool	CNetworkUDP::SetStatus(eNETWORK_STATUS a_eStatus)
{
	m_eStatus = a_eStatus;

	if(m_pfnOnStatus)
		m_pfnOnStatus(m_pParent, m_eStatus);

	return true;
}


void*	CNetworkUDP::sRoutine(void* a_pArg)
{
	CNetworkUDP* pThis = (CNetworkUDP*)a_pArg;
	pThis->Routine();
	pthread_exit(NULL);
}

bool	CNetworkUDP::Routine()
{
	int nRecv = 0;
	int nSockLen = sizeof(struct sockaddr);	
    struct pollfd		events[1];
    
    memset(events,0,sizeof(events));
    events[0].fd = m_nSocket;
    events[0].events = POLLIN;

	while(m_bIsRunning)
	{
        if( poll( (struct pollfd *)&events, 1, 10 ) <= 0 )
        {
            continue;
        }
        
        if( events[0].revents & POLLIN )
		{
		    if(m_eType == eTYPE_SERVER)
    			nRecv = recvfrom(m_nSocket, (void*)m_nBufRecv, ETHERMTU, MSG_DONTWAIT, (struct sockaddr*)&m_stInputAddr,(socklen_t*) &nSockLen);			
    		else if(m_eType == eTYPE_CLIENT)
    			nRecv = recvfrom(m_nSocket, (void*)m_nBufRecv, ETHERMTU, MSG_DONTWAIT, NULL, NULL);
    
    		if(nRecv == -1)
    		{
    			if(errno == EAGAIN)
    			{
    				USleep(10);
    				continue;
    			}
    			break;
    		}
    		else
    		{
    			if(m_pfnOnReceive)
    				m_pfnOnReceive(m_pParent, 0, m_nBufRecv, nRecv);
    		}
    	}
	}

	return true;

}
