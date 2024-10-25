/*******************************************************************************
 모  듈 : capture
 작성자 : 한명희
 버  전 : 0.0.1
 설  명 : 촬영 모드에 따라 영상을 수집한다.
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

#include "tcp_server.h"

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
* \brief        
* \param        port: port number
* \param        max: maximum number of clients
* \return       none
* \note
*******************************************************************************/
CTCPServer::CTCPServer(int port , int max,int (*log)(int,int,const char *,...))
{
	print_log= log;
	
	timer_clear();
	
	m_p_mutex = new CMutex;
	
	m_b_thread_running = false;
	
	m_n_client_cnt = 0;
	
	if( max > TCP_SERVER_MAX_CLIENTS )
	{
	    m_n_client_max = TCP_SERVER_MAX_CLIENTS;
	    print_log(ERROR, 1, "[TCP Server] client number is overflow(%d/max:%d).\n", max, TCP_SERVER_MAX_CLIENTS);
	}
	else
	{
	    m_n_client_max = max;
	}

	m_socket_fd = -1;
	m_server_addr.sin_family = AF_INET;
	m_server_addr.sin_addr.s_addr = INADDR_ANY;
	m_server_addr.sin_port = htons( port );

	for(u32 i = 0; i < m_n_client_max ; ++i )
    {
    	m_client_t[i] = NULL;
    	m_p_poll_fd[ i ].fd = -1;
    }

	m_b_receiver_started = false;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
CTCPServer::CTCPServer(void)
{
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
CTCPServer::~CTCPServer(void)
{
	stop();
	safe_delete(m_p_mutex);
}

/******************************************************************************/
/**
* \brief        server start
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CTCPServer::start(void)
{
	int sockopt = 1;
	struct ifconf	ifconf;
	struct ifreq	ifreq[ MAX_NIC_NUM ];

	if(m_b_thread_running == true || m_socket_fd != -1 )
    {
    	print_log( ERROR, 1, "[TCP Server] server already started.\n",DEBUG_INFO);
    	return false;
    }

	m_socket_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

	if(m_socket_fd == -1 )
    {
    	print_log( ERROR, 1, "[TCP Server] socket:%s\n",strerror( errno ));
    	return false;
    }

    // enable reuseaddr
	if( setsockopt(m_socket_fd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof( sockopt )) == -1)
    {
    	print_log( ERROR, 1, "[TCP Server] setsockopt(SO_REUSEADDR):%s\n",strerror( errno ));
    }

    // non-blocking socket
	if( fcntl(m_socket_fd, F_SETFL, O_NONBLOCK ) == -1 )
    {
    	print_log( ERROR, 1, "[TCP Server] fcntl:%s\n",strerror( errno ));
    	return false;
    }

    // set itself's ip address.
	ifconf.ifc_len = sizeof( struct ifreq ) * MAX_NIC_NUM;
	ifconf.ifc_ifcu.ifcu_req = ifreq;

	if( ioctl(m_socket_fd, SIOCGIFCONF, &ifconf ) == -1 )
    {
    	print_log( ERROR, 1, "[TCP Server] ioctl:%s\n",strerror( errno ));
    }
	else
    {
    	m_server_addr.sin_addr.s_addr = htonl( INADDR_ANY );
    	//m_server_addr.sin_addr.s_addr = htonl( inet_aton(i_p_ip_addr) );
    }
    
    // bind
	if( bind(m_socket_fd,( struct sockaddr * ) &m_server_addr,sizeof( struct sockaddr )) == -1)
    {
    	print_log( ERROR, 1, "[TCP Server] bind:%s\n",strerror( errno ));

    	return false;
    }

    // listen
	if( listen(m_socket_fd,1) == -1)
    {
    	print_log( ERROR, 1, "[TCP Server] listen:%s\n",strerror( errno ));

    	return false;
    }

	m_p_poll_fd[ 0 ].fd = m_socket_fd;
	m_p_poll_fd[ 0 ].events = POLLIN;

	network_usleep( 10 );

    // server loop
	m_b_thread_running = true;
    // launch server
	if( pthread_create(&m_thread_t,NULL,tcp_server_routine,( void * ) this)!= 0 )
    {
    	print_log( ERROR, 1, "[TCP Server] pthread_create:%s\n",strerror( errno ));

    	m_b_thread_running = false;
    	return false;
    }

	return true;
}

/******************************************************************************/
/**
* \brief        server and client stop
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CTCPServer::stop( void )
{
	u32 i;

	print_log( INFO, 1, "[TCP Server] port %u\n",ntohs(m_server_addr.sin_port ));

	if( m_b_thread_running == false || m_socket_fd == -1 )
    {
    	print_log( DEBUG, 1, "[TCP Server] already stoped.\n",DEBUG_INFO);
    	return false;
    }
    
    m_b_thread_running = false;
    
    // exit loop
	//lock_client();
	
	for( i = 0; i < m_n_client_max; ++i )
    {
    	if(m_client_t[ i ] == NULL) continue;

    	m_client_t[ i ]->b_is_running = false;

        // join receiver
    	if( pthread_join( m_client_t[ i ]->recv_thread_t, NULL ) != 0 )
        {
        	print_log( ERROR, 1, "[TCP Server] pthread_join:%s\n",
                	strerror( errno ));

        	//continue;
        }

#if 0
    	if(m_client_t[ i ]->sock_fd != -1 )
        {
        	if( shutdown(m_client_t[ i ]->sock_fd, SHUT_RDWR ) == -1 )
            {
            	print_log( ERROR, 1, "[TCP Server] shutdown:%s\n",
                	strerror( errno ));
            	//continue;
            }
            
            close( m_client_t[ i ]->sock_fd );

        	m_client_t[ i ]->sock_fd = -1;
            
        	print_log( INFO, 1, "[TCP Server] client %s:%d closed.\n",
            	inet_ntoa(m_client_t[ i ]->sock_addr_t.sin_addr ), ntohs(m_client_t[ i ]->sock_addr_t.sin_port ));
        }
    	safe_free(m_client_t[ i ] );
#endif
    }

	if( pthread_join(m_thread_t,NULL) != 0)
    {
    	print_log( ERROR, 1,"[TCP Server] pthread_join:(server_proc):%s\n",strerror( errno ));
    }
    
    //unlock_client();
    
    // close server socket
	if( shutdown(m_socket_fd, SHUT_RDWR ) == -1 )
    {
    	print_log( ERROR, 1, "[TCP Server] shutdown:%s\n",strerror( errno ));

    	return false;
    }

	print_log( OPERATE, 1, "[TCP Server] closed.\n");

	m_socket_fd = -1;
	m_n_client_cnt = 0;

	return true;
}

/******************************************************************************/
/**
* \brief        server restart
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CTCPServer::restart( void)
{
	return(stop( ) && start());
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void * tcp_server_routine( void * arg )
{
	CTCPServer * server = (CTCPServer *)arg;
	server->server_proc();
	pthread_exit( NULL );
}

/******************************************************************************/
/**
* \brief        client accept 처리
* \param        none
* \return       none
* \note
*******************************************************************************/
void CTCPServer::server_proc(void)
{
	int	                	fd = -1;
	struct sockaddr_in     	client_addr;
	socklen_t	        	socklen = sizeof( struct sockaddr );
	bool	            	rtn;


	print_log( OPERATE, 1,"[TCP Server] Server is started(%d).\n", syscall( __NR_gettid ));

	while(m_b_thread_running )
    {
    	if( poll( m_p_poll_fd, 1, 100 ) <= 0 )
        {
        	continue;
        }

    	if( m_p_poll_fd[ 0 ].revents & POLLIN )
        {
        	while( m_b_thread_running )
            {
            	fd = accept( m_socket_fd, (struct sockaddr*)&client_addr, &socklen );

            	if( fd == -1 )
                {
                    // MSG_DONTWAIT
                	if( errno != EAGAIN )
                    {
                    	print_log( ERROR, 1,"[TCP Server] accept:%s\n",strerror( errno ));
                    	pthread_exit( NULL );
                    }

                	network_usleep( 10 );
                	break;
                }

            	print_log( INFO, 1, "[TCP Server] client fd %d connected from %s:%d\n",
                        	fd, inet_ntoa( client_addr.sin_addr ), ntohs( client_addr.sin_port ));

            	if( m_n_client_cnt >= m_n_client_max )
                {
                	print_log( ERROR, 1, "[TCP Server] connection is over CLIENTS(%d)\n", m_n_client_max);

                	if( shutdown( fd, SHUT_RDWR ) == -1 )
                    {
                    	print_log( ERROR, 1, "[TCP Server] fd %d shutdown:%s\n", fd, strerror( errno ) );
                    }
                	
                	close(fd);
                    continue;
                }

            	lock_client();
                rtn = add_client( &client_addr, fd );
                unlock_client();
                if( rtn == false )
                {
                    print_log(ERROR, 1, "[TCP Server] add client is failed.\n");
                	break;
                }
            }    // while(m_b_thread_running )
        }    // if( m_p_poll_fd[ 0 ].revents & POLLIN )
    	else
        {
        	network_usleep( 10 );
        }
    }

	print_log( INFO, 1, "[TCP Server] exit.\n");


}

/******************************************************************************/
/**
* \brief        add client to client list
* \param        i_p_client_addr_t: socket address
* \param        i_n_fd: socket file descriptor
* \return       none
* \note
*******************************************************************************/
bool CTCPServer::add_client( struct sockaddr_in* i_p_client_addr_t, s32 i_n_fd )
{
	bool	found = false;
	u32		i;
	u32     index = 0;


	print_log( DEBUG, 0, "[TCP Server] addr = %s:%d, fd = %d\n",
                	inet_ntoa( i_p_client_addr_t->sin_addr ), ntohs( i_p_client_addr_t->sin_port ), i_n_fd );

	for( i = 0; i < m_n_client_max; i++ )
    {
    	if(m_client_t[i] == NULL) continue;

        // find client addr.
    	if( m_client_t[ i ]->sock_addr_t.sin_addr.s_addr == i_p_client_addr_t->sin_addr.s_addr )
        {
        	print_log( ERROR, 1, "[TCP Server] client(ip: %s) is already exist.\n",
            	inet_ntoa(m_client_t[ i ]->sock_addr_t.sin_addr ));

        	found = true;
        	break;
        }
    }

	if( found == false )
    {
        // allocate client info
    	for( i = 0 ; i < m_n_client_max; i++ )
        {
        	if(m_client_t[i] == NULL)
            {
            	m_client_t[i] =( client_info_t * )malloc( sizeof( client_info_t ) );
            	assert(m_client_t[i] );
            	
            	memset( m_client_t[i], 0, sizeof( client_info_t ) );
            	index = i;
            	break;
            }
        }

        //printf("@@@@@@@@@@@@@ client %x->%d @@@@@@@@@@@@@\n",client[ index ],index);
        // non-blocking socket
    	if( fcntl( i_n_fd, F_SETFL, O_NONBLOCK ) == -1 )
        {
        	print_log( ERROR, 1, "[TCP Server] add client fcntl:%s\n",strerror( errno ));
        	return false;
        }
    	
    	m_client_t[ index ]->b_is_running = true;

    	memcpy( (void*) &m_client_t[ index ]->sock_addr_t, i_p_client_addr_t, sizeof( struct sockaddr ) );

    	m_client_t[index ]->sock_fd = i_n_fd;
        
        m_n_client_stop_fd = -1;
        
    	print_log( OPERATE, 1, "[TCP Server] add client addr = %s:%d\n", inet_ntoa( i_p_client_addr_t->sin_addr ),ntohs( i_p_client_addr_t->sin_port ));
        
    	m_n_client_cur_index = index;

    	print_log( INFO, 1, "[TCP Server] launch receiver. client_cnt = %d,client[%d ]->fd: %d\n", m_n_client_cnt, index, i_n_fd);
        
    	network_usleep( 10 );
        
        // launch receiver for client
        receiver_started( m_client_t[index ]->sock_fd );
        //pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        if( pthread_create( &m_client_t[ index ]->recv_thread_t, NULL, tcp_service_receiver, this ) != 0 )
        {
        	print_log( ERROR, 1, "[TCP Server] pthread_create:%s\n",strerror( errno ));
            
        	m_client_t[ index ]->b_is_running = false;
        	return false;
        }
        
        //pthread_detach( m_client_t[ index ]->recv_thread_t );

        // wait until receiver started
    	if( wait_receiver( true ) == false )
        {
        	m_client_t[ index ]->b_is_running = false;
        	return false;
        }

#if defined( _DEBUG_TCP_SERVER )
    	print_clients( void);
#endif

        ++m_n_client_cnt;

    	m_b_receiver_started = false;

    	print_log( INFO, 1, "[TCP Server] notify receiver/sender that creation completed.\n");

    }

	return true;
}

/******************************************************************************/
/**
* \brief        delete client from client list
* \param        i_n_fd: socket file descriptor
* \return       none
* \note
*******************************************************************************/
bool CTCPServer::del_client( s32 i_n_fd )
{
	u32	          	i;
	bool	      	deleted = false;

    lock_client();
	for( i = 0; i < m_n_client_max; ++i )
    {
        //printf("############ client %x->%d##############\n",client[ i ],i);
    	if(m_client_t[ i ] == NULL) continue;

    	if(m_client_t[ i ]->sock_fd == i_n_fd )
        {
        	print_log( OPERATE, 1, "[TCP Server] delete last fd %d\n", m_client_t[ i ]->sock_fd );
       	    
            m_client_t[ i ]->b_is_running = false;
        	
            // join receiver
        	if( pthread_join( m_client_t[ i ]->recv_thread_t, NULL ) != 0 )
            {
            	print_log( ERROR, 1, "[TCP Server] pthread_join:%s\n", strerror( errno ));
            }
            
            client_notify_close( i_n_fd );
            
         	if( shutdown( m_client_t[ i ]->sock_fd, SHUT_RDWR ) == -1 )
            {
            	print_log( ERROR, 1, "[TCP Server]shutdown(): %s\n",strerror( errno ));
            }
            
            close( m_client_t[ i ]->sock_fd );
        	m_client_t[ i ]->sock_fd = -1;
        	
        	m_n_client_stop_fd = -1;

        	safe_free( m_client_t[ i ] );
        	m_client_t[ i ] = NULL;

            --m_n_client_cnt;
        	deleted = true;
            
        	break;
        }
    }
    
    unlock_client();

	return deleted;
}

/******************************************************************************/
/**
* \brief        wait for receiver start
* \param        i_b_flag: receiver is started or stopped
* \return       none
* \note
*******************************************************************************/
bool CTCPServer::wait_receiver( bool i_b_flag )
{

	while( m_b_receiver_started != i_b_flag )
    {
    	if(m_b_thread_running == false )
        {
        	return false;
        }

    	network_usleep( 10 );
    }

	return true;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void  CTCPServer::print_clients(void)
{
	u32 i;

	for( i = 0; i < m_n_client_max; i++ )
    {
    	if(m_client_t[i] != NULL)
        {
        	print_log( DEBUG, 1, "\t%2d: fd = %d, addr = %s:%d\n", 
        	                        i, m_client_t[i]->sock_fd,
                                    inet_ntoa(m_client_t[ i ]->sock_addr_t.sin_addr ),
                                    ntohs(m_client_t[ i ]->sock_addr_t.sin_port ));
        }
    }

	if( i == 0 )
    {
    	print_log( DEBUG, 1, "%s:%d %s : client empty.\n",DEBUG_INFO );
    }
}

/******************************************************************************/
/**
* \brief        
* \param        arg: CTCPServer instance
* \return       none
* \note
*******************************************************************************/
void * tcp_service_receiver(void * arg)
{
	CTCPServer *server = (CTCPServer *)arg;
	server->service_proc();
	pthread_exit( NULL );
}

/******************************************************************************/
/**
* \brief        receiver of client
* \param        none
* \return       none
* \note
*******************************************************************************/
void CTCPServer::service_proc(void)
{
	client_info_t *        	cur_client;
	bool *                	is_crunning;
	int	                	n_recv, fd;
	uint8_t	            	buf_recv[ ETHERMTU ];
    struct pollfd           events[1];
    
    memset(events,0,sizeof(events));
    
	cur_client = m_client_t[m_n_client_cur_index];
	assert( cur_client );
	is_crunning = &cur_client->b_is_running;

	fd = cur_client->sock_fd;

	m_b_receiver_started = true;

    events[0].fd = fd;
    events[0].events = POLLIN;

    // wait until creation completed.
	if( wait_receiver(false ) == false )
    {
    	print_log( ERROR, 1, "[TCP Receiver] exit(wait_receiver is false).\n");
        
    	pthread_exit( NULL );
    }

	print_log( INFO, 1, "[TCP Receiver] notify tcp_server that receiver(id: %d, fd: %d) started.\n", syscall( __NR_gettid ), fd);
	
	while( m_b_thread_running == true && *is_crunning == true)
    {
        if( poll( (struct pollfd *)&events, 1, 100 ) < 0 )
        {
            print_log( ERROR, 1, "[TCP Receiver] poll: (%d) - %s\n", errno, strerror( errno ));
            continue;
        }
        
        if( events[0].revents & POLLIN )
        {
        	n_recv = recv( fd, buf_recv, sizeof( buf_recv ), MSG_DONTWAIT );

        	if( n_recv == -1 )
            {
            	if( errno != EAGAIN && errno != EINTR )
                {
                	print_log( ERROR, 1, "[TCP Receiver] recv: (%d) - %s\n", errno, strerror( errno ));
                    break;    // exit
                }
                
            	network_usleep( 10 );
            	continue;
            }
        	else if( n_recv == 0 )    // close
            {
            	print_log( ERROR, 1, "[TCP Receiver] n_recv == 0\n");
            	break;    // exit
            }
#if defined( _DEBUG_TCP_SERVER )
        	else
            {
    #if defined( _DEBUG_TCP_SERVER_PACKET )
            	network_print_packet( buf_recv, n_recv );
    #endif
            }
#endif
            if( receiver_todo(cur_client,buf_recv,n_recv) == false)
            {
                print_log( ERROR, 1, "[TCP Receiver] receiver_todo(fd: %d) false\n", fd);
            	break;    // exit
            }
        }
    }

    //print_log( OPERATE, 1, "[TCP Receiver] delete client(fd: %d)\n", fd);
    //close_client( cur_client );
    client_notify_recv_error(fd);
    
	print_log( OPERATE, 1, "[TCP Receiver] exit(fd: %d)\n", fd);

}

/******************************************************************************/
/**
* \brief        close client
* \param        i_n_fd: socket descriptor
* \return       none
* \note
*******************************************************************************/
void CTCPServer::close_client( s32 i_n_fd )
{
	del_client( i_n_fd );
}

/******************************************************************************/
/**
* \brief        sender of client
* \param        i_p_client_t: client information
* \param        i_p_buf: data buffer for sending
* \param        i_n_len: length of buffer
* \return       none
* \note
*******************************************************************************/
bool CTCPServer::sender( s32 i_n_fd, u8* i_p_buf, s32 i_n_len, u32 i_n_wait_time )
{
    int ret = -1;
    int total_len = 0;
    struct timespec start_timespec;
    struct timespec end_timespec;
    
    int	n_errno = 0;
    struct pollfd		events[1];
    
    memset(events,0,sizeof(events));

    events[0].fd = i_n_fd;
    events[0].events = POLLOUT;
    
	if( clock_gettime(CLOCK_MONOTONIC, &start_timespec) < 0 )
	{
	    print_log(ERROR,1,"[TCP Server] start_timespec check error\n");
	}
    
    if( i_n_wait_time < 5000 )
    {
    	timer_set(5000);
    }
    else
    {
    	timer_set(i_n_wait_time);
    }
    
    while(1)
    {
	    if( timer_is_expired() == true )
	    {
	        ret = -1;
	        print_log( ERROR, 1, "[TCP Server] send timeout\n" );
	        break;
	    }  

        if( poll( (struct pollfd *)&events, 1, 100 ) <= 0 )
        {
            continue;
        }
        
        if( events[0].revents & POLLOUT )
        {
	        ret = send( i_n_fd, &i_p_buf[total_len], (i_n_len - total_len), 0 );
	        n_errno = errno;
	        if( ret == -1 )
	        {
	        	print_log( ERROR, 1, "[TCP Server] send: %d - %s\n", n_errno, strerror( n_errno ));
	        	
	            if( n_errno != ENOTCONN && n_errno != EDESTADDRREQ  && n_errno != EBADF 
	            	&& n_errno != ENOTSOCK && n_errno != EFAULT && n_errno != EPIPE )
				{
					network_usleep( 10 );
					continue;
				}
				else
				{
					break;
				}
			}
			else
			{
			    total_len += ret;
		    }
		    
			if( total_len == i_n_len )
			{
				break;
			}
		}
    }

	if( clock_gettime(CLOCK_MONOTONIC, &end_timespec) < 0 )
	{
	    print_log(ERROR,1,"[TCP Server] end_timespec check error\n");
	}
	else
	{
	 	print_send_time(start_timespec, end_timespec);
	}

    timer_clear();
    
    if( ret < 0 )
    {
        return false;
    }
    
    return true;
}

/******************************************************************************/
/**
* \brief        sender of client
* \param        i_p_client_t: client information
* \param        i_p_buf: data buffer for sending
* \param        i_n_len: length of buffer
* \return       none
* \note
*******************************************************************************/
bool CTCPServer::send_image( s32 i_n_fd, u8* i_p_buf, s32 i_n_len, u32 i_n_wait_time )
{
    int ret = -1;
    int total_len = 0;
    struct timespec start_timespec;
    struct timespec end_timespec;
    
    int	n_errno = 0;
    struct pollfd		events[1];
    
    memset(events,0,sizeof(events));

    events[0].fd = i_n_fd;
    events[0].events = POLLOUT;
    
	if( clock_gettime(CLOCK_MONOTONIC, &start_timespec) < 0 )
	{
	    print_log(ERROR,1,"[TCP Server] start_timespec check error\n");
	}
    
    if( i_n_wait_time < 5000 )
    {
    	timer_set(5000);
    }
    else
    {
    	timer_set(i_n_wait_time);
    }
    
    while(1)
    {
	    if( timer_is_expired() == true )
	    {
	        ret = -1;
	        print_log( ERROR, 1, "[TCP Server] image send timeout\n" );
	        break;
	    }

	    if( m_n_client_stop_fd == i_n_fd )
	    {
	        ret = -1;
	        m_n_client_stop_fd = -1;
	        print_log( ERROR, 1, "[TCP Server] stop image send\n" );
	        break;
	    }
	    
        if( poll( (struct pollfd *)&events, 1, 100 ) <= 0 )
        {
            continue;
        }
        
        if( events[0].revents & POLLOUT )
        {
	        ret = send( i_n_fd, &i_p_buf[total_len], (i_n_len - total_len), 0 );
	        n_errno = errno;
	        if( ret == -1 )
	        {
	        	print_log( ERROR, 1, "[TCP Server] image send: %d - %s\n", n_errno, strerror( n_errno ));
	        	
	            if( n_errno != ENOTCONN && n_errno != EDESTADDRREQ  && n_errno != EBADF
	            	&& n_errno != ENOTSOCK && n_errno != EFAULT && n_errno != EPIPE )
				{
					network_usleep( 10 );
					continue;
				}
				else
				{
					break;
				}
			}
			else
			{
			    total_len += ret;
		    }
		    
			if( total_len == i_n_len )
			{
				break;
			}
		}
    }

	if( clock_gettime(CLOCK_MONOTONIC, &end_timespec) < 0 )
	{
	    print_log(ERROR,1,"[TCP Server] end_timespec check error\n");
	}
	else
	{
	 	print_send_time(start_timespec, end_timespec);
	}

    timer_clear();
    
    if( ret < 0 )
    {
        return false;
    }
    
    return true;
}

/******************************************************************************/
/**
* \brief        
* \param        i_p_client_t: client information
* \return       none
* \note
*******************************************************************************/
void CTCPServer::stop_send_image( s32 i_n_fd )
{
	m_n_client_stop_fd = i_n_fd;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CTCPServer::timer_set(u32 i_n_msec)
{
    //struct timespec now;
    
    if( clock_gettime(CLOCK_MONOTONIC, &m_wait_t) < 0 )
	{
	    cerr << "[CTCPServer] clock_gettime error\n";
	}
	else
	{
	    m_wait_t.tv_sec = m_wait_t.tv_sec + i_n_msec/1000;
    	m_wait_t.tv_nsec = m_wait_t.tv_nsec + ((i_n_msec % 1000) * 1000000);
    	
    	if( m_wait_t.tv_nsec >= 1000000000 )
    	{
    	    m_wait_t.tv_sec++;
    	    m_wait_t.tv_nsec -= 1000000000;
    	}
    	//printf("[CTime] target sec: %ld, nsec: %ld\n", m_wait_t.tv_sec, m_wait_t.tv_nsec);
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CTCPServer::timer_clear()
{
	m_wait_t.tv_sec = 0;
    m_wait_t.tv_nsec = 0;
	
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CTCPServer::timer_is_expired(void)
{
    struct timespec now;
    struct timespec timeout;
    
 	if( clock_gettime(CLOCK_MONOTONIC, &now) < 0 )
	{
	    cerr << "[CTCPServer] is_expired: clock_gettime error\n";
	    return true;
	}
	else
	{
	    
    	timeout.tv_sec = m_wait_t.tv_sec - now.tv_sec;
    	timeout.tv_nsec = m_wait_t.tv_nsec - now.tv_nsec;
    	
    	if( timeout.tv_nsec < 0 )
    	{
    	    timeout.tv_nsec += 1000000000;
    	    --timeout.tv_sec;
    	}
    	
    	if( timeout.tv_sec < 0 )
    	{
    	    //printf("[CTime] expired sec: %ld, nsec: %ld\n", now.tv_sec, now.tv_nsec);
    	    return true;
    	}
	}
   
	return false;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CTCPServer::print_send_time( struct timespec i_start_timespec, struct timespec i_end_timespec )
{
 	struct timespec calc_t;  
 	u32             n_time = 0;
    
    calc_t.tv_sec = i_end_timespec.tv_sec - i_start_timespec.tv_sec;
    calc_t.tv_nsec = i_end_timespec.tv_nsec - i_start_timespec.tv_nsec;	
    
    if(calc_t.tv_nsec < 0)
    {
    	calc_t.tv_sec--;
    	calc_t.tv_nsec = calc_t.tv_nsec + 1000000000;
    }	
    
    n_time = calc_t.tv_sec * 1000 + calc_t.tv_nsec / 1000000;
    
    if( n_time > 1000 )
    {
    	print_log( INFO, 1, "[TCP Server] send time: %d ms\n", n_time );
    }
}
