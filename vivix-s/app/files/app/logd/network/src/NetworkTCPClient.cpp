#include "NetworkTCPClient.h"
#include "NetworkMISC.h"

CNetworkTCPClient::CNetworkTCPClient():m_pParent(NULL), m_nThread(0), m_bIsRunning(false),
	m_nSocket(-1), m_bReceiverStarted(false), m_nConnectTime(0), m_eStatus(eNETWORK_STATUS_NONE)
{
}

CNetworkTCPClient::~CNetworkTCPClient()
{
    
}

bool	CNetworkTCPClient::Init(void* a_pParent, param_t* a_pparam)
{

	
	if(CheckStatus(eNETWORK_STATUS_STOP))
		Stop();

	m_pfnOnStatus = a_pparam->pfnOnStatus;
	m_pfnOnReceive = a_pparam->pfnOnReceive;
	m_pfnOnError = a_pparam->pfnOnError;

	m_pParent = a_pParent;
	
	memcpy(&m_stServerAddr, &a_pparam->stsockaddr, sizeof(struct sockaddr_in));

	m_enetwork_type = a_pparam->enetwork_type;
	
	SetStatus(eNETWORK_STATUS_INIT);

	return true;
}

bool CNetworkTCPClient::Start()
{
	if(CheckStatus(eNETWORK_STATUS_START) == false){
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_CHECK_STATUS,0);
		return false;
	}

#ifdef WIN32
    unsigned long arg;
    WSADATA wsaData = {0};
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

	m_nSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

#ifdef WIN32    
    if(m_nSocket == INVALID_SOCKET)
#else
	if(m_nSocket == -1)
#endif
	{
		m_pfnOnError(m_pParent, eNETWORK_ERROR_CREATE_SOCKET_FAILED, ERRNO);
		return false;
	}

#ifdef WIN32
    arg = 1;
    //Nonblocking ½ÇÆÐ
//    if(ioctlsocket(m_nSocket, FIONBIO, &arg) != 0)
 //   {
 //      	m_pfnOnError(m_pParent, eNETWORK_ERROR_FCNTL_FAILED, ERRNO);
//		return false;
 //   }
    
   	if( setsockopt(m_nSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&sockopt, sizeof(sockopt))==-1)
	{
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_SETSOCKETOPT_FAILED,ERRNO);
		return false;
	}

	if( setsockopt(m_nSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&sockopt, sizeof(sockopt))==-1)
	{
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_SETSOCKETOPT_FAILED,ERRNO);
		return false;
	}

#elif LINUX
	if(fcntl(m_nSocket, F_SETFL, O_NONBLOCK) == -1)
	{
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_FCNTL_FAILED, ERRNO);
		return false;
	}
	
	if( setsockopt(m_nSocket, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt))==-1)
	{
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_SETSOCKETOPT_FAILED,ERRNO);
		return false;
	}

	if( setsockopt(m_nSocket, IPPROTO_TCP, TCP_NODELAY, &sockopt, sizeof(sockopt))==-1)
	{
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_SETSOCKETOPT_FAILED,ERRNO);
		return false;
	}

#endif

	m_bIsRunning = true;
	int bConectCnt=0;
	
	while(m_bIsRunning == true)
	{
#if defined(WIN32)
		if(connect(m_nSocket, (struct sockaddr*)&m_stServerAddr, sizeof(struct sockaddr)) == SOCKET_ERROR)
#else
        if(connect(m_nSocket, (struct sockaddr*)&m_stServerAddr, sizeof(struct sockaddr)) == -1)
#endif
		{
#ifdef WIN32                
			if(ERRNO == WSAEINPROGRESS)
			{
                SleepEx(10, false);
#else
			if(ERRNO == EINPROGRESS)
			{
    			USleep(10);
#endif
				continue;
			}
			else
			{
				if(bConectCnt++<3)
				{
#ifdef WIN32
                    SleepEx(10, false);
#else
					USleep(10);
#endif

					continue;
				}
				
				close(m_nSocket);
				m_nSocket = -1;

				m_bIsRunning = false;
				return false;
			}
		}
		break;
	}
#ifdef WIN32
    SleepEx(10, false);
#else
	USleep(10);
#endif


#ifdef WIN32
    DWORD dwThreadId = 0; 
    CreateThread(NULL,
                0,
                (LPTHREAD_START_ROUTINE)sRoutine,
                this,
                0,
                &dwThreadId );
#else
	if(pthread_create(&m_nThread, NULL, sRoutine, this) !=0)
	{
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_CREATE_THREAD_FAILED, ERRNO);
		return false;

	}
#endif

	if(WaitReceiver(true) == false)
		return false;

	m_bReceiverStarted = false;

	SetStatus(eNETWORK_STATUS_START);
	return true;
}


bool CNetworkTCPClient::Stop(){

	if(CheckStatus(eNETWORK_STATUS_STOP) == false){
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_CHECK_STATUS,0);
		return false;
	}

	m_bIsRunning = false;

#if defined(WIN32)
    if(closesocket(m_nSocket) == SOCKET_ERROR)
#else
	if(close(m_nSocket) == -1)
#endif
	{
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_CLOSE_SOCKET_FAILED, ERRNO);
		return false;
	}

	m_nSocket = -1;

	SetStatus(eNETWORK_STATUS_STOP);
	return true;
}


bool CNetworkTCPClient::Restart()
{
	if(CheckStatus(eNETWORK_STATUS_RESTART) == false){
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_CHECK_STATUS,0);
		return false;
	}

	if( (Stop() && Start()) == false)
		return false;

	//if(m_pfnOnStatus)
	//	m_pfnOnStatus(m_pParent, eNETWORK_STATUS_RESTART);

	return true;
	

}


int CNetworkTCPClient::Send(unsigned char* a_sData, int a_nLength)
{
	int nret = 0;
	int npos = 0;
	int nlen = a_nLength;
	
	if(CheckStatus(eNETWORK_STATUS_SEND) == false){
		if(m_pfnOnError)
			m_pfnOnError(m_pParent, eNETWORK_ERROR_CHECK_STATUS,ERRNO);

		return false;
	}

	if((int)strlen((const char*)a_sData) != a_nLength)
	{
		m_pfnOnError(m_pParent, eNETWORK_ERROR_UNMATCH_MSG_LENGTH, ERRNO);
		return false;
	}


	while(1)
	{
#ifdef WIN32
		nret = send(m_nSocket, (const char*)a_sData[npos], a_nLength, ERRNO);
#else
		nret = send(m_nSocket, &a_sData[npos], a_nLength, 0);
#endif
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

bool	CNetworkTCPClient::CheckStatus(eNETWORK_STATUS a_eStatus)
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

bool	CNetworkTCPClient::SetStatus(eNETWORK_STATUS a_eStatus)
{
	m_eStatus = a_eStatus;

	if(m_pfnOnStatus)
		m_pfnOnStatus(m_pParent, m_eStatus);

	return true;
}


bool	CNetworkTCPClient::WaitReceiver(bool a_bFlag)
{
	while(m_bReceiverStarted != a_bFlag)
	{
		if(m_bIsRunning == false)
			return false;
#ifdef WIN32
        SleepEx(10, false);
#else
		USleep(10);
#endif

	}

	return true;

}

void*	CNetworkTCPClient::sRoutine(void* a_pArg)
{
	CNetworkTCPClient* pThis = (CNetworkTCPClient*)a_pArg;
	pThis->Routine();
}

bool CNetworkTCPClient::Routine()
{
	unsigned char	BufRecv[ETHERMTU]={0,};
	//unsigned char	BufRecv[1400]={0,};
	int nRecv;
	m_bReceiverStarted = true;

	if(WaitReceiver(false) == false){
#ifdef WIN32
        ExitThread(0);
#else
		pthread_exit(NULL);
#endif
		return false;
	}

	while(m_bIsRunning == true)
	{
		memset(BufRecv, 0x00, sizeof(BufRecv));
#ifdef WIN32
		nRecv = recv(m_nSocket, (char*)BufRecv, sizeof(BufRecv), 0);
#else
    	nRecv = recv(m_nSocket, BufRecv, sizeof(BufRecv), MSG_DONTWAIT);
#endif

		if(nRecv == -1)
		{

#ifdef WIN32
			if(ERRNO != WSAEWOULDBLOCK)
#else
     		if(ERRNO != EAGAIN && ERRNO != EWOULDBLOCK)
#endif
				break;
#ifdef WIN32
            SleepEx(10, false);
#else
			USleep(10);
#endif

			continue;
		}
		else if(nRecv ==0)
			break;
		
//		on_receive((void*)(m_pParent),0, BufRecv, nRecv);
		if(m_pfnOnReceive)
			m_pfnOnReceive(m_pParent, 0, BufRecv, nRecv);

	}

	m_bIsRunning = false;
#ifdef WIN32
    ExitThread(0);
#else
	pthread_exit(NULL);
#endif
}


