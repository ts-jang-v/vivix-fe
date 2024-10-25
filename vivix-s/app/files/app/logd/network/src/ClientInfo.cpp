#include "ClientInfo.h"
#include "NetworkTCPServer.h"

void* CClientInfo::sServiceReceiver(void* a_pArg)
{
	CClientInfo* pThis = (CClientInfo*)a_pArg;
	pThis->ServiceReceiver();
	pthread_exit(NULL);
}


bool CClientInfo::ServiceReceiver()
{	
	bool b_ret = true;
	uint8_t	BufRecv[ETHERMTU];
	CNetworkTCPServer* pServer = (CNetworkTCPServer*)m_pParent;
	int nRecv;
    struct pollfd		events[1];
    
    memset(events,0,sizeof(events));
    events[0].fd = m_nFd;
    events[0].events = POLLIN;

	pServer->m_bReceiverStarted = true;

	if(pServer->WaitReceiver(false) == false)
	{
		return false;
	}
	if(pServer->m_pfnOnConnect)
		pServer->m_pfnOnConnect((void*)(pServer->m_pParent), m_nIndex,++pServer->m_nClientCnt);
		
	while(pServer->m_bIsRunning == true && m_bIsRunning == true)
	{
        if( poll( (struct pollfd *)&events, 1, 10 ) <= 0 )
        {
            continue;
        }
		
		nRecv = recv(m_nFd, BufRecv, sizeof(BufRecv), MSG_DONTWAIT);

		if(nRecv == -1)
		{
			if(errno != EAGAIN)
				break;

			USleep(10);
			continue;
		}
		else if(nRecv == 0)
			break;

		m_nConnectedTime = time(NULL);
		if(pServer->m_pfnOnReceive)
			b_ret = pServer->m_pfnOnReceive((void*)(pServer->m_pParent),m_nIndex, BufRecv, nRecv);
		
		if(b_ret == false)
			pServer->DelClient(m_nFd);

	}

	if(pServer->m_pfnOnDisconnect)
		pServer->m_pfnOnDisconnect((void*)(pServer->m_pParent), m_nIndex,--pServer->m_nClientCnt);
	m_nFd = -1;
	return true;
}
