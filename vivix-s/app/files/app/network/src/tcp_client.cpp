/******************************************************************************/
/**
 * @file    tcp_client.cpp
 * @brief   TCP client 贸府
 * @author  
*******************************************************************************/

/*******************************************************************************
*   Include Files
*******************************************************************************/
#include <iostream>
#include <sys/poll.h>		// struct pollfd, poll()

#include "tcp_client.h"


using namespace std;



/*******************************************************************************
*   Constant Definitions
*******************************************************************************/


/*******************************************************************************
*   Type Definitions
*******************************************************************************/


/*******************************************************************************
*   Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*   Variable Definitions
*******************************************************************************/


/*******************************************************************************
*   Function Prototypes
*******************************************************************************/
/******************************************************************************/
/**
 * @brief   积己磊
 * @param   port: 器飘 锅龋
 * @param   ip: IP 林家
 * @return  
 * @note    
*******************************************************************************/
CTCPClient::CTCPClient(s32 (*log)(s32,s32,const s8*,...))
{
    print_log = log;
    
    m_p_mutex = new CMutex;
	
	m_b_is_running = false;
	m_n_socket = -1;
	
	m_b_connected = false;
	
	m_recv_pthread_t = (u32)NULL;
	m_b_receiver_started    = false;
	
	memset( &m_server_sockaddr_in, 0, sizeof(m_server_sockaddr_in) );
	
	print_log( DEBUG, 1, "[TCP Client] CTCPClient\n");
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
CTCPClient::CTCPClient(void)
{
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
CTCPClient::~CTCPClient(void)
{
	stop();
	safe_delete( m_p_mutex );
}

/******************************************************************************/
/**
 * @brief   socket 积己, 立加, receiver  thread 积己
 * @param   
 * @return  
 * @note    client_service_receiver甫 捞侩窍咯 thread 积己
*******************************************************************************/
bool CTCPClient::start(void)
{
	int sockopt = 1;
	
	print_log( DEBUG, 1, "[TCP Client] client to %s\n", inet_ntoa( m_server_sockaddr_in.sin_addr ));


	if( m_n_socket != -1 )
    	close(m_n_socket);

	if(m_recv_pthread_t != (u32)NULL)
    	pthread_join(m_recv_pthread_t,NULL);

	m_n_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

	if(m_n_socket == -1 )
    {
    	print_log( ERROR, 1,"[TCP Client] client to %s: sockek:%s\n",
                                	inet_ntoa( m_server_sockaddr_in.sin_addr ), strerror( errno ));
    	return false;
    }

    // non-blocking socket
	if( fcntl(m_n_socket, F_SETFL, O_NONBLOCK ) == -1 )
    {
    	print_log( ERROR, 1,"[TCP Client] client to %s, fcntl:%s\n",
                                	inet_ntoa( m_server_sockaddr_in.sin_addr ), strerror( errno ));
    	return false;
    }

    // enable reuseaddr
	if( setsockopt(m_n_socket, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof( sockopt )) == -1)
    {
    	print_log( ERROR, 1,"[TCP Client] client to %s, setsockopt:%s\n",
                                	inet_ntoa( m_server_sockaddr_in.sin_addr ), strerror( errno ));
    	return false;
    }

	if( setsockopt(m_n_socket, SOL_SOCKET, TCP_NODELAY, &sockopt, sizeof( sockopt )) == -1)
    {
    	print_log( ERROR, 1,"[TCP Client] client to %s, setsockopt:%s\n",
                                	inet_ntoa( m_server_sockaddr_in.sin_addr ), strerror( errno ));
    	return false;
    }


    // client loop
	m_b_is_running = true;
    s32 n_wait = 0;
	while( m_b_is_running == true )
    {
        // connect
    	if( connect(m_n_socket, ( struct sockaddr * )&m_server_sockaddr_in, sizeof( struct sockaddr )) == -1)
        {
            // O_NONBLOCK, wait 100 msec
        	if( n_wait >= 10000 )
            {
            	close(m_n_socket );
            	m_n_socket = -1;

            	print_log( ERROR, 1,"[TCP Client] client to %s, connect failed:%s\n",
                                	inet_ntoa( m_server_sockaddr_in.sin_addr ), strerror( errno ));
                
            	m_b_is_running = false;
            	return false;
            }
      	    
      	    network_usleep( 10 );
       	    n_wait++;
            continue;
        }
    	break;
    }

    
	print_log( INFO, 1, "[TCP Client] client to %s:%u(fd: %d), launch receiver.\n",
            	inet_ntoa( m_server_sockaddr_in.sin_addr ), \
            	ntohs( m_server_sockaddr_in.sin_port ), m_n_socket);

	network_usleep( 10 );

	//if( 0 )
    {
        // launch receiver for client
    	if( pthread_create( &m_recv_pthread_t, NULL, client_service_receiver, this ) != 0 )
        {
        	print_log( ERROR, 1,"[TCP Client] client to %s:%u, pthread_create:receiver:%s\n",
                        	inet_ntoa( m_server_sockaddr_in.sin_addr ), \
                        	ntohs( m_server_sockaddr_in.sin_port ), strerror( errno ));
            
        	m_b_is_running = false;
        	return false;
        }
    
        // wait until receiver started
    	if( wait_receiver( true ) == false )
        {
        	m_b_is_running = false;
        	return false;
        }
    }


    // notify receiver/sender that creation completed.
	m_b_receiver_started = false;
	//m_b_connected = true;

	print_log( INFO, 1, "[TCP Client] client to %s:%u\nnotify receiver that creation completed.\n",
        	inet_ntoa( m_server_sockaddr_in.sin_addr ), \
        	ntohs( m_server_sockaddr_in.sin_port ));

	return true;
}

/******************************************************************************/
/**
 * @brief   socket 积己, 立加, receiver  thread 积己
 * @param   
 * @return  
 * @note    client_service_receiver甫 捞侩窍咯 thread 积己
*******************************************************************************/
bool CTCPClient::start(s8* ip, s32 port)
{
    memset( &m_server_sockaddr_in, 0, sizeof(struct sockaddr_in) );
    
	m_server_sockaddr_in.sin_family = AF_INET;
	m_server_sockaddr_in.sin_port = htons(port);
	m_server_sockaddr_in.sin_addr.s_addr = inet_addr(ip);
	
	print_log(DEBUG, 1, "[TCP Client] ip: %s(0x%08X)\n", ip, m_server_sockaddr_in.sin_addr.s_addr);
	
	return start();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CTCPClient::stop(void)
{
	if( m_b_is_running == false || m_n_socket == -1 )
    {
    	print_log( ERROR, 1,"[TCP Client] client to %s already stoped.\n", \
    	                    inet_ntoa( m_server_sockaddr_in.sin_addr ));
    	return false;
    }

    // exit loop
	m_b_is_running = false;

    // join receiver
	if( m_recv_pthread_t !=  (u32)NULL )
    {
    	if( pthread_join(m_recv_pthread_t,NULL) != 0)
        {
        	print_log( ERROR, 1,"[TCP Client] client to %s, pthread_join:receiver:%s\n", \
                        inet_ntoa( m_server_sockaddr_in.sin_addr ), strerror( errno ));
        }
    }

	m_recv_pthread_t = (u32)NULL;

    // close client socket
	if( close( m_n_socket ) == -1 )
    {
    	print_log( ERROR, 1,"[TCP Client] client to %s, close:%s\n", \
                    inet_ntoa( m_server_sockaddr_in.sin_addr ), strerror( errno ));
    	return false;
    }

	print_log( OPERATE, 1, "[TCP Client] client to %s closed.\n", \
	                inet_ntoa( m_server_sockaddr_in.sin_addr ));
    
	m_n_socket = -1;
	return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CTCPClient::restart( void )
{

	return(stop(  ) && start(  ));
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CTCPClient::wait_receiver( bool flag )
{

#if 0
	print_log( INFO, 1, "[TCP Client] client to %s, flag = %s\n", \
                    inet_ntoa( m_server_sockaddr_in.sin_addr ), \
                    ( flag == true ? "true" : "false" ));
#endif

	while( m_b_receiver_started != flag )
    {
    	if( m_b_is_running == false )
        {
        	return false;
        }

    	network_usleep( 10 );
    }

	return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void* client_service_receiver( void * arg )
{
	CTCPClient * client = (CTCPClient *)arg;
	client->recv_proc();
	pthread_exit( NULL );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CTCPClient::recv_proc(void)
{
    s32	            	n_recv;
	unsigned char		p_recv_buf[ ETHERMTU ];
    struct pollfd		events[1];
    
    memset(events,0,sizeof(events));

	print_log( INFO, 1, "[TCP Client] client to %s, pid %u\n",
                    inet_ntoa( m_server_sockaddr_in.sin_addr ), getpid());
    
    // notify tcp_client that receiver started
	m_b_receiver_started = true;

    // wait until creation completed.
	if( wait_receiver(  false ) == false )
    {
    	print_log( ERROR, 1, "[TCP Client] client to %s receiver exit.\n",
                    inet_ntoa( m_server_sockaddr_in.sin_addr ));
    	m_b_connected = false;
    	pthread_exit( NULL );
    }
    
    m_b_connected = true;
    
    events[0].fd = m_n_socket;
    events[0].events = POLLIN;
    
    while( m_b_is_running == true )
    {
        if( poll( (struct pollfd *)&events, 1, 10 ) <= 0 )
        {
            continue;
        }
        
        if( events[0].revents & POLLIN )
    	{
            memset( p_recv_buf, 0, sizeof( p_recv_buf ) );
        	n_recv = recv( m_n_socket, p_recv_buf, sizeof( p_recv_buf ), MSG_DONTWAIT );
    
        	if( n_recv == -1 )
            {
                // MSG_DONTWAIT
            	if( errno != EAGAIN )
                {
                	print_log( ERROR, 1, "[TCP Client] client to %s: recv:%s\n",
                            inet_ntoa( m_server_sockaddr_in.sin_addr ), strerror( errno ));
                	break;    // exit
                }
            	
            	network_usleep( 10 );
            	continue;
            }
        	else if( n_recv == 0 )    // close
            {
            	print_log( OPERATE, 1, "[TCP Client] client to %s:closed.\n",
                                inet_ntoa( m_server_sockaddr_in.sin_addr ));
            	break;    // exit loop.
            }
    #if defined( _DEBUG_TCP_CLIENT )
        	else
            {
            	print_log( DEBUG, 1, "[TCP Client] %s:%d %s:client to %s:%d\n %d bytes received from %s:%u\n", \
            	        DEBUG_INFO, inet_ntoa( m_server_sockaddr_in.sin_addr ), \
            	        ntohs( m_server_sockaddr_in.sin_port ), n_recv, \
                    	inet_ntoa( m_server_sockaddr_in.sin_addr ), \
                    	ntohs( m_server_sockaddr_in.sin_port ));
                
        #if defined( _DEBUG_TCP_CLIENT_PACKET )
            	network_print_packet( p_recv_buf, n_recv );
        #endif
            }
    #else
        	else
            {
            	print_log( DEBUG, 0, "[TCP Client] client to %s:%d\n %d bytes received from %s:%u\n", \
                    	inet_ntoa( m_server_sockaddr_in.sin_addr ), \
                    	ntohs( m_server_sockaddr_in.sin_port ), n_recv, \
                    	inet_ntoa( m_server_sockaddr_in.sin_addr ), \
                    	ntohs( m_server_sockaddr_in.sin_port ));
            }
    #endif
        	if( receiver_todo( p_recv_buf, n_recv ) == false )
            {
            	break;
            }
        }
    }

	m_b_connected = false;
	m_b_is_running =false;
	
	notify_close();
    
	print_log( OPERATE, 1, "[TCP Client] client(fd: %d) to %s, receiver exit.\n", \
	                    m_n_socket, inet_ntoa( m_server_sockaddr_in.sin_addr ) );

}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CTCPClient::sender( u8* i_p_buf, s32 i_n_len )
{
    int ret = -1;
    int total_len = 0;
    int nwait = 0;
    
    lock();
    
    if( m_b_connected == false )
    {
        unlock();
        print_log( ERROR, 1, "[TCP Client] client is not connected.\n");
        return false;
    }
    
    while(1)
    {
        ret = send( m_n_socket, &i_p_buf[total_len], (i_n_len - total_len), 0 );
        
        if( ret == -1 )
        {
            if( errno != EAGAIN && errno != EINTR )
			{
				print_log( ERROR, 1, "[TCP Client] send: %d - %s\n", errno, strerror( errno ));
				m_b_connected = false;
				break;	// exit
			}
			network_usleep( 10 );
			nwait++;
		}
		else
		{
		    total_len += ret;
	    }
	    
		if( total_len == i_n_len )
		{
			break;
		}
	    
	    if( nwait > 50000 ) // 5s
	    {
	        ret = -1;
	        m_b_connected = false;
	        break;
	    }  
    }
    
    unlock();
    
    if( ret < 0 )
    {
        return false;
    }
    
    return true;
}
