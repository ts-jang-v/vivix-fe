/*
 * tcp_server.h
 *
 *  Created on: 2014. 01. 14.
 *      Author: 
 */

 
#ifndef _TCP_SERVER_H_
#define _TCP_SERVER_H_

/*******************************************************************************
*	Include Files
*******************************************************************************/
#include <stdio.h>            // fprintf()
#include <stdlib.h>            // malloc(), free()
#include <unistd.h>            // getpid()
#include <stdbool.h>
#include <string.h>            // memset(), memcpy()
#include <fcntl.h>            // fcntl()
#include <sys/socket.h>        // socket(), setsocketopt(), connect()
#include <sys/poll.h>        // struct pollfd, poll()
#include <sys/ioctl.h>      // ioctl()
#include <arpa/inet.h>        // inet_ntoa()
#include <errno.h>            // errno
#include <assert.h>            // assert()
#include <net/ethernet.h>    // ETHERMTU
#include <net/if.h>            // struct ifconf, struct ifreq
#include <netinet/tcp.h>    //TCP_NODELAY

#include "network.h"

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/
#define		TCP_SERVER_MAX_CLIENTS	10
#define     CLIENT_BUFF_SIZE    (50 * 1024)
/*******************************************************************************
*	Type Definitions
*******************************************************************************/
typedef struct 
{
	bool	                	b_is_running;
	struct sockaddr_in	    	sock_addr_t;
	s32	                    	sock_fd;
	pthread_t	            	recv_thread_t;
	u8	                     	p_packet_buf[CLIENT_BUFF_SIZE];
	s32	                    	n_buf_end_pos;
	void*                       p_user_data;
}client_info_t;


/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*	Variable Definitions
*******************************************************************************/


/*******************************************************************************
*	Function Prototypes
*******************************************************************************/
void *         	tcp_server_routine( void * arg );
void *         	tcp_service_receiver( void * arg );

class CTCPServer : public CNetwork
{

private:
	int	                    (* print_log)(int level,int print,const char * format, ...);
	
	// server thread
	bool	            	m_b_thread_running;
	pthread_t	        	m_thread_t;
	
	// server socket information
	s32	                	m_socket_fd;
	struct sockaddr_in		m_server_addr;
	
	// client information
	u32	                	m_n_client_cnt;
	u32	                	m_n_client_max;
	u32	                	m_n_client_cur_index;
	client_info_t *        	m_client_t[ TCP_SERVER_MAX_CLIENTS ];
	bool	            	m_b_receiver_started;
	s32						m_n_client_stop_fd;
    
    // client management
	struct pollfd	    	m_p_poll_fd[ TCP_SERVER_MAX_CLIENTS + 1 ];
	void                 	server_proc(void);
	void                 	service_proc(void);
	bool                 	add_client( struct sockaddr_in* i_p_client_addr_t, s32 i_n_fd );
	bool                 	del_client( s32 i_n_fd );
	bool                 	wait_receiver( bool i_b_flag );
	void                 	print_clients(void);
    
    // client resource syncronization
    CMutex*                 m_p_mutex;
    
    struct timespec 		m_wait_t;
    void 					timer_set(u32 i_n_msec);
   	void 					timer_clear();
   	bool 					timer_is_expired(void);
   	
   	void					print_send_time( struct timespec i_start_timespec, struct timespec i_end_timespec );

protected :


public:
	bool                 	restart( void );
	bool                 	stop(void);
	bool	            	start(void);
    
	void	            	close_client( s32 i_n_fd );
	bool                    sender( s32 i_n_fd, u8* i_p_buf, s32 i_n_len, u32 i_n_wait_time=5000 );
	bool					send_image( s32 i_n_fd, u8* i_p_buf, s32 i_n_len, u32 i_n_wait_time=5000 );
	void                    stop_send_image( s32 i_n_fd );
	
	// client resource syncronization
	void                 	lock_client( void ){ m_p_mutex->lock(); }
	void                 	unlock_client( void ){ m_p_mutex->unlock(); }
    
	virtual void		    client_notify_close( s32 i_n_fd )=0;
	virtual bool	    	receiver_todo( client_info_t* i_p_client_t, u8* i_p_buf, s32 i_n_len )=0;
	virtual void	    	receiver_started( s32 i_n_fd )=0;
	virtual void            client_notify_recv_error( s32 i_n_fd )=0;
	
	friend void *         	tcp_server_routine( void * arg );
	friend void *         	tcp_service_receiver( void * arg );
	
	CTCPServer(int port , int max,int (*log)(int,int,const char *,...));
	CTCPServer(void);
    ~CTCPServer(void);
};

#endif  /* end of _TCP_SERVER_H_ */

