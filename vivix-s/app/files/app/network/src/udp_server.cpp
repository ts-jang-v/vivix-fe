/*******************************************************************************
 모  듈 : udp_server
 작성자 : 한명희
 버  전 : 0.0.1
 설  명 : udp server
 참  고 : 

 버  전:
         0.0.1  최초 작성
*******************************************************************************/



/*******************************************************************************
*	Include Files
*******************************************************************************/
#include <iostream>
#include <stdio.h>
#include <linux/unistd.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/poll.h>		// struct pollfd, poll()

#include "udp_server.h"


using namespace std;

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/

/*******************************************************************************
*	Type Definitions
*******************************************************************************/

/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/

/*******************************************************************************
*	Variable Definitions
*******************************************************************************/

/*******************************************************************************
*	Function Prototypes
*******************************************************************************/



/******************************************************************************/
/**
* \brief        constructor
* \param        port        port number
* \param        log         log function
* \return       none
* \note
*******************************************************************************/
CUDPServer::CUDPServer(int port , int (*log)(int,int,const char *,...))
{

	print_log= log;
	m_bis_running = false;
	m_nserver_socket = -1;
	
	m_server_addr_t.sin_family = AF_INET;
	m_server_addr_t.sin_addr.s_addr = INADDR_ANY;
	m_server_addr_t.sin_port = htons( port );

}

/******************************************************************************/
/**
* \brief        constructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CUDPServer::CUDPServer(void)
{
}

/******************************************************************************/
/**
* \brief        destructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CUDPServer::~CUDPServer(void)
{
	stop();
}

/******************************************************************************/
/**
* \brief        star server
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CUDPServer::start(void )
{
    int sockopt = 1;
	struct ifconf	ifconf;
	struct ifreq	ifreq[ MAX_NIC_NUM ];


	if(m_bis_running == true || m_nserver_socket != -1 )
    {
    	print_log( ERROR, 1, "[UDP Server] server already started.\n",DEBUG_INFO);
    	return false;
    }
    
    m_nserver_socket =	socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	
	if(m_nserver_socket == -1 )
    {
    	print_log( ERROR, 1, "[UDP Server] socket:%s\n",strerror( errno ));
    	return false;
    }

    // set itself's ip address.
	ifconf.ifc_len = sizeof( struct ifreq ) * MAX_NIC_NUM;
	ifconf.ifc_ifcu.ifcu_req = ifreq;

	if( ioctl(m_nserver_socket, SIOCGIFCONF, & ifconf ) == -1 )
    {
    	print_log( ERROR, 1, "[UDP Server] ioctl:%s\n",strerror( errno ));
    }
	else
    {
    	m_server_addr_t.sin_addr.s_addr = htonl( INADDR_ANY);
    }

    // enable reuseaddr
	if( setsockopt(m_nserver_socket, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof( sockopt )) == -1)
    {
    	print_log( ERROR, 1, "[UDP Server] setsockopt(SO_REUSEADDR):%s\n",strerror( errno ));
    }

    // bind
	if( bind(m_nserver_socket,( struct sockaddr * ) &m_server_addr_t,sizeof( struct sockaddr )) == -1)
    {
    	print_log( ERROR, 1, "[UDP Server] bind:%s\n",strerror( errno ));

    	return false;
    }

    // server loop
	m_bis_running = true;
    
    network_usleep( 10 );
    
    // launch server
	if( pthread_create(&m_server_t,NULL,udp_server_routine,( void * ) this)!= 0 )
    {
    	print_log( ERROR, 1, "[UDP Server] pthread_create:%s\n",strerror( errno ));

    	m_bis_running = false;
    	return false;
    }

	return true;
}

/******************************************************************************/
/**
* \brief        stop server
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CUDPServer::stop( void )
{
	print_log( INFO, 1, "[UDP Server] port %u\n", ntohs( m_server_addr_t.sin_port ));

	if(m_bis_running == false || m_nserver_socket == -1 )
    {
    	print_log( DEBUG, 1, "[UDP Server] already stoped.\n",DEBUG_INFO);
    	return false;
    }

	m_bis_running = false;

	if( pthread_join(m_server_t,NULL) != 0)
    {
    	print_log( ERROR, 1,"[UDP Server] pthread_join:(server_proc):%s\n",strerror( errno ));
    }
    
    // close server socket
	if( shutdown( m_nserver_socket, SHUT_RDWR ) == -1 )
    {
    	print_log( ERROR, 1, "[UDP Server] shutdown:%s\n",strerror( errno ));

    	return false;
    }

	print_log( OPERATE, 1, "[UDP Server] closed.\n",DEBUG_INFO);

	m_nserver_socket = -1;
	
	return true;
}

/******************************************************************************/
/**
* \brief        create server thread
* \param        none
* \return       none
* \note
*******************************************************************************/
void * udp_server_routine( void * arg )
{
	CUDPServer * server = (CUDPServer *)arg;
	server->server_proc();
	pthread_exit( NULL );
}

/******************************************************************************/
/**
* \brief        create server thread
* \param        none
* \return       none
* \note
*******************************************************************************/
void CUDPServer::server_proc()
{
	int					n_recv;
	socklen_t	        	socklen = sizeof( struct sockaddr );
    udp_handle_t        handle;
    struct pollfd		events[1];
    
    memset(events,0,sizeof(events));
    
	print_log( OPERATE, 1,"[UDP Server] Server is started(%d).\n",syscall( __NR_gettid ));
	
	handle.nserver_socket   = m_nserver_socket;
    
    events[0].fd = m_nserver_socket;
    events[0].events = POLLIN;
	
	while( m_bis_running )
    {
        if( poll( (struct pollfd *)&events, 1, 100 ) <= 0 )
        {
            continue;
        }
		
		if( events[0].revents & POLLIN )
		{
		    n_recv =recvfrom(
    					m_nserver_socket,
    					( void * ) m_pbuf_recv,
    					ETHERMTU,
    					MSG_DONTWAIT,	// non-blocking
    					( struct sockaddr * ) & m_input_addr_t,
    					& socklen
    					);
    		
    		if( n_recv == -1 )
    		{
    			// MSG_DONTWAIT
    			if( errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK)
    			{
    				network_usleep( 10 );
    				continue;
    			}
#if defined( _DEBUG_UDP_SERVER )
    			print_log(ERROR,1,"%s:%d %s:recvfrom:%s\n",DEBUG_INFO,strerror( errno ));
#endif
    			break;
    		}
    		else if( n_recv > 0 )
    		{
#if defined( _DEBUG_UDP_SERVER )
    			print_log(ERROR,1,"%s:%d %s:%d bytes received from client %s\n",
    					DEBUG_INFO,n_recv,inet_ntoa( m_input_addr_t.sin_addr ));
#endif
    			
    			
    			handle.input_addr_t     = m_input_addr_t;
    			receiver_todo(&handle, m_pbuf_recv,n_recv);
    			
    		}
    	}
	}

#if defined( _DEBUG_UDP_SERVER )
	print_log(ERROR,1,"%s:%d %s:exit.\n",DEBUG_INFO);
#endif

	pthread_exit( NULL );


}

bool CUDPServer::sender(udp_handle_t* a_phandle_t, u8* a_pbuf, s32 a_nlen)
{
    sendto( a_phandle_t->nserver_socket, \
            (void*) a_pbuf, a_nlen, 0, \
            (struct sockaddr *)&(a_phandle_t->input_addr_t), sizeof(a_phandle_t->input_addr_t) );
    
    return true;
}
