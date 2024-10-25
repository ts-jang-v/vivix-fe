#include "NetworkTCPServer.h"

CNetworkTCPServer::CNetworkTCPServer(): m_nThread(0), m_nSocket(-1),  m_nMaxClient(0), m_nCurIndex(0),	
	m_eStatus(eNETWORK_STATUS_NONE), m_pfnOnStatus(NULL), m_pfnOnError(NULL), m_pfnOnReceive(NULL), m_pfnOnConnect(NULL),
	m_pfnOnDisconnect(NULL), m_pParent(NULL),m_bReceiverStarted(false),m_bIsRunning(false),m_nClientCnt(0)
{

}


CNetworkTCPServer::~CNetworkTCPServer()
{
	for(int i =0; i < m_nMaxClient ; i++)
	{
		if(m_pClientInfo[i] == NULL) continue;
		
		m_pClientInfo[i]->m_bIsRunning = false;

		if(m_pClientInfo[i]->m_nReceiverThread && pthread_join(m_pClientInfo[i]->m_nReceiverThread, NULL) != 0)
		{
			m_pfnOnError(m_pParent, eNETWORK_ERROR_JOIN_THREAD_FAILED,errno);
		}

		if(m_pClientInfo[i]->m_nFd != -1)
		{
			if(shutdown(m_pClientInfo[i]->m_nFd, SHUT_RDWR) == -1)
			{
				continue;
			}

			m_pClientInfo[i]->m_nFd = -1;
		}

		delete m_pClientInfo[i];
		m_pClientInfo[i] = NULL;
	}




}


bool	CNetworkTCPServer::Init(void* a_pThis, int a_nPort, int a_nMaxClient, 
				PFN_NETWORK_STATUS a_pfnOnStatus, PFN_NETWORK_CONNECT a_pfnOnConnect, 
				PFN_NETWORK_DISCONNECT a_pfnOnDisconnect, 
				PFN_NETWORK_RECEIVE a_pfnOnReceive, PFN_NETWORK_ERROR a_pfnOnError)
{

	
	
	if(a_nPort < 0)
	{
		m_pfnOnError(a_pThis, eNETWORK_ERROR_ILLEGAL_PORT,0);
		return false;
	}

	if(a_nMaxClient > TCP_SERVER_MAX_CLIENTS)
	{
		m_pfnOnError(a_pThis, eNETWORK_ERROR_TOO_MUCH_CLIENT,0);
		return false;
	}

	if(CheckStatus(eNETWORK_STATUS_STOP))
			Stop();
			
	
	m_pfnOnStatus = a_pfnOnStatus;
	m_pfnOnReceive = a_pfnOnReceive;
	m_pfnOnError = a_pfnOnError;
	m_pfnOnConnect = a_pfnOnConnect;
	m_pfnOnDisconnect = a_pfnOnDisconnect;

	m_pParent = a_pThis;
	m_nMaxClient = a_nMaxClient;

	m_stServerAddr.sin_family = AF_INET;
	m_stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	m_stServerAddr.sin_port = htons(a_nPort);

	for(int i=0; i<m_nMaxClient; i++)
	{
		m_pClientInfo[i] = NULL;
		m_stPollfds[i].fd = -1;
	}
	
	SetStatus(eNETWORK_STATUS_INIT);

	return true;
}


bool	CNetworkTCPServer::Start()
{
	int sockopt = 1;
	struct ifconf ifconf;
	struct ifreq ifreq[MAX_NIC_NUM];

	if(CheckStatus(eNETWORK_STATUS_START) == false){
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_CHECK_STATUS,0);
		return false;
	}

	m_nSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(m_nSocket == -1)
	{
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_CREATE_SOCKET_FAILED,errno);
		return false;
	}


	if(fcntl(m_nSocket, F_SETFL, O_NONBLOCK) == -1)
	{
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_FCNTL_FAILED,errno);
		return false;
	}

	if( setsockopt(m_nSocket, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt))==-1)
	{
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_SETSOCKETOPT_FAILED,errno);
		return false;
	}

	if( setsockopt(m_nSocket, IPPROTO_TCP, TCP_NODELAY, &sockopt, sizeof(sockopt))==-1)
	{
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_SETSOCKETOPT_FAILED,errno);
		return false;
	}

	ifconf.ifc_len = sizeof(struct ifreq) * MAX_NIC_NUM;
	ifconf.ifc_ifcu.ifcu_req = ifreq;

	if(ioctl(m_nSocket, SIOCGIFCONF, & ifconf) == -1)
	{
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_IOCTL_FAILED,errno);
		return false;
	}
	else
	{
		m_stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	}



	if(bind(m_nSocket, (struct sockaddr*) &m_stServerAddr, sizeof(struct sockaddr)) == -1)
	{
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_BIND_FAILED,errno);
		return false;
	}

	if(listen(m_nSocket, 1) == -1)
	{
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_LISTEN_FAILED,errno);
		return false;
	}

	m_stPollfds[0].fd = m_nSocket;
	m_stPollfds[0].events = POLLIN;

	m_bIsRunning = true;

	USleep(10);

	if(pthread_create(&m_nThread, NULL, sRoutine, (void*)this) !=0)
	{
		m_pfnOnError(m_pParent, eNETWORK_ERROR_CREATE_THREAD_FAILED,0);
		m_bIsRunning = false;
		return false;
	}

	SetStatus(eNETWORK_STATUS_START);

	return true;
}


bool CNetworkTCPServer::Stop()
{
	if(CheckStatus(eNETWORK_STATUS_STOP) == false){
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_CHECK_STATUS,0);
		return false;
	}

	m_bIsRunning = false;
	
	//#error need lock

	for(int i =0; i < m_nMaxClient ; i++)
	{
		if(m_pClientInfo[i] == NULL) continue;

		m_pClientInfo[i]->m_bIsRunning = false;

		if(m_pClientInfo[i]->m_nReceiverThread && pthread_join(m_pClientInfo[i]->m_nReceiverThread, NULL) != 0)
		{
			if(m_pfnOnError)
				m_pfnOnError(m_pParent, eNETWORK_ERROR_JOIN_THREAD_FAILED,errno);
			return false;
		}

		if(m_pClientInfo[i]->m_nFd != -1)
		{
			if(shutdown(m_pClientInfo[i]->m_nFd, SHUT_RDWR) == -1)
			{
				continue;
			}

			m_pClientInfo[i]->m_nFd = -1;
		}

		delete m_pClientInfo[i];
		m_pClientInfo[i] = NULL;

	}

	if(shutdown(m_nSocket, SHUT_RDWR) == -1)
	{
		return false;
	}

	m_nSocket = -1;
	m_nClientCnt = 0;

	SetStatus(eNETWORK_STATUS_STOP);

	return true;
}


bool CNetworkTCPServer::Restart()
{
	if(CheckStatus(eNETWORK_STATUS_RESTART) == false){
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_CHECK_STATUS,0);
		return false;
	}

	if( (Stop() && Start() ) == false)
		return false;

	//if(m_pfnOnStatus)
	//	m_pfnOnStatus(m_pParent, eNETWORK_STATUS_RESTART);

	return true;

}

int CNetworkTCPServer::Send(int a_nIndex,unsigned char* a_sData, int a_nLength)
{
	int nret = 0;
	int npos = 0;
	int nlen = a_nLength;
	
	if(CheckStatus(eNETWORK_STATUS_SEND) == false){
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

	if(m_pClientInfo[a_nIndex] == NULL || m_pClientInfo[a_nIndex]->m_nFd == -1)
	{
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_UNKNOWN_CLIENT, 0);
		return false;
	}
	while(1)
	{
		send(m_pClientInfo[a_nIndex]->m_nFd, &a_sData[npos], a_nLength,0);
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

CClientInfo*	CNetworkTCPServer::GetClientInfo(int a_nIndex)
{
	if(m_pClientInfo[a_nIndex])
		return m_pClientInfo[a_nIndex];

	return NULL;
}


void	CNetworkTCPServer::PrintClient()
{

}


bool	CNetworkTCPServer::AddClient(struct sockaddr_in* a_stClientAddr, int a_nFd)
{	
	bool bFound = false;
	int	 nIndex=0;

	for(int i=0; i< m_nMaxClient ; i++)
	{
		if(m_pClientInfo[i] == NULL) continue;

		if( (m_pClientInfo[i]->m_stAddr.sin_addr.s_addr == a_stClientAddr->sin_addr.s_addr)
				&& (m_pClientInfo[i]->m_stAddr.sin_port == a_stClientAddr->sin_port) )
		{
			bFound = true;
			break;
		}

	}

	if(m_nClientCnt > TCP_SERVER_MAX_CLIENTS)
	{
		return false;
	}


	if(bFound == false)
	{
		
		for(int i=0; i< m_nMaxClient ; i++)
		{
			if(m_pClientInfo[i] && m_pClientInfo[i]->m_nFd == -1)
			{
				m_pClientInfo[i]->Init(i, a_stClientAddr, a_nFd);
				nIndex = i;
				break;
			}
			else if(m_pClientInfo[i] == NULL)
			{
				m_pClientInfo[i] = new CClientInfo((void*)this, i, a_stClientAddr, a_nFd);

				nIndex = i;
				break;
			}

		}


		USleep(10);

		if(pthread_create( &m_pClientInfo[nIndex]->m_nReceiverThread, NULL, m_pClientInfo[nIndex]->sServiceReceiver, (void*) m_pClientInfo[nIndex]) !=0)
		{
			if(m_pfnOnError)
				m_pfnOnError(m_pParent, eNETWORK_ERROR_CREATE_THREAD_FAILED, errno);
			m_pClientInfo[nIndex]->m_bIsRunning = false;
			return false;
		}


		if(WaitReceiver(true) == false)
		{
			m_pClientInfo[nIndex]->m_bIsRunning = false;
			return false;
		}

	}

	m_bReceiverStarted = false;


	return true;
}


bool	CNetworkTCPServer::DelClient(int a_nFd)
{
	for(int i = 0; i < m_nMaxClient; i++)
	{
		if(m_pClientInfo[i] == NULL) continue;

		if(m_pClientInfo[i]->m_nFd == a_nFd)
		{
			m_pClientInfo[i]->m_bIsRunning = false;

			if(shutdown(m_pClientInfo[i]->m_nFd, SHUT_RDWR) == -1)
			{

			}

			m_pClientInfo[i]->m_nFd = -1;


			return true;

		}
	}

	return false;
}


bool	CNetworkTCPServer::CheckStatus(eNETWORK_STATUS a_eStatus)
{
	bool ret = false;
	switch((int)m_eStatus){
	case eNETWORK_STATUS_INIT:
		ret = (bool)(eNETWORK_STATUS_START & a_eStatus);
		break;
	case eNETWORK_STATUS_START:
		ret = (bool)((eNETWORK_STATUS_INIT | eNETWORK_STATUS_STOP | eNETWORK_STATUS_RESTART | eNETWORK_STATUS_SEND ) & a_eStatus);
		break;
	case eNETWORK_STATUS_STOP:
		ret = (bool)((eNETWORK_STATUS_INIT | eNETWORK_STATUS_START) & a_eStatus);
		break;
	default:
		break;

	}

	return ret;
}

bool	CNetworkTCPServer::SetStatus(eNETWORK_STATUS a_eStatus)
{
	m_eStatus = a_eStatus;

	if(m_pfnOnStatus)
		m_pfnOnStatus(m_pParent, m_eStatus);

	return true;
}


bool	CNetworkTCPServer::WaitReceiver(bool a_bFlag)
{
	while(m_bReceiverStarted != a_bFlag)
	{
		if(m_bIsRunning == false)
			return false;

		USleep(10);
	}

	return true;

}

void*	CNetworkTCPServer::sRoutine(void* a_pArg)
{
	CNetworkTCPServer* pThis = (CNetworkTCPServer*)a_pArg;
	pThis->Routine();
	pthread_exit(NULL);
}

bool	CNetworkTCPServer::Routine(void)
{
	int nFd = -1;
	struct sockaddr_in stClientAddr;
	socklen_t	socklen = sizeof(struct sockaddr);

	while(m_bIsRunning)
	{
		if(poll(m_stPollfds, 1, -1) == -1)
			continue;

		if(m_stPollfds[0].revents & POLLIN)
		{
			while(m_bIsRunning)
			{

				nFd = accept(m_nSocket, (struct sockaddr*) & stClientAddr, &socklen);

				if(nFd == -1)
				{	
					if(errno != EAGAIN)
					{
						return false;
					}
					USleep(10);
					break;
				}

				if(m_nClientCnt >= m_nMaxClient)
				{
					if(shutdown(nFd, SHUT_RDWR) == -1)
					{

					}
					close(nFd);
					continue;
				}

				if(AddClient(&stClientAddr, nFd) == false)
					break;

			}

		}
		else
		{
			USleep(10);
		}
	}
	return true;
}
