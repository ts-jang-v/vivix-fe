#ifndef __NETWORKTCPCLIENT_H__
#define __NETWORKTCPCLIENT_H__



#include "NetworkConfig.h"
#include "Network.h"

#ifndef ETHERMTU
#define ETHERMTU    1500
#endif

class CNetworkTCPClient : public INetwork
{
	public:
		CNetworkTCPClient();
		virtual ~CNetworkTCPClient();


		bool	Init(void* a_pThis, param_t* a_pparam);				
		bool	Start();
		bool	Stop();
		bool	Restart();
		int 	Send(unsigned char* a_sData, int a_nLength);
		
		bool	WaitReceiver(bool a_bFlag);
		
		bool    is_running()
                {
                    return m_bIsRunning;
                };
					
		NT		get_network_type(){ return m_enetwork_type;};
		time_t	get_connected_time(){ return m_nconnected_time;};	
		
	private:
		bool	CheckStatus(eNETWORK_STATUS a_eStatus);
		bool	SetStatus(eNETWORK_STATUS a_eStatus);

	public:
		static	void*	sRoutine(void* a_pArg);
		bool	Routine(void);

	private:
		void*						m_pParent;
#ifdef WIN32
        HANDLE                      m_nThread;
#else
		pthread_t					m_nThread;
#endif
		volatile bool				m_bIsRunning;
		int 						m_nSocket;
		struct sockaddr_in			m_stServerAddr;
		volatile bool				m_bReceiverStarted;
		int							m_nBConnect;
		time_t						m_nConnectTime;
		eNETWORK_STATUS				m_eStatus;
	
	public:
		PFN_NETWORK_STATUS			m_pfnOnStatus;
		PFN_NETWORK_ERROR			m_pfnOnError;
		PFN_NETWORK_RECEIVE			m_pfnOnReceive;



};
#endif // __NETWORKTCPCLIENT_H__
