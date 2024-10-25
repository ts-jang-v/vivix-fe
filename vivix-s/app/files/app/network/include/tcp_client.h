/******************************************************************************/
/**
 * @file    
 * @brief   
 * @author  
*******************************************************************************/


#ifndef _TCP_CLIENT_
#define _TCP_CLIENT_

/*******************************************************************************
*   Include Files
*******************************************************************************/

#include <stdio.h>            // fprintf()
#include <stdlib.h>            // malloc(), free()
#include <stdbool.h>
#include <unistd.h>            // getpid()
#include <string.h>            // memset(), memcpy()
#include <fcntl.h>            // fcntl()
#include <sys/socket.h>        // socket(), setsocketopt(), connect()
#include <arpa/inet.h>        // inet_ntoa()
#include <errno.h>            // errno
#include <assert.h>            // assert()
#include <net/ethernet.h>    // ETHERMTU
#include <netinet/tcp.h>    //TCP_NODELAY

#include "network.h"



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

void *             client_service_receiver( void * arg );


/**
 * @class   TCP client
 * @brief   
 */

class CTCPClient : public CNetwork
{
private:
	s32	                    (* print_log)(s32 level, s32 print, const s8* format, ...);
    
    pthread_t               m_recv_pthread_t;
    bool                    m_b_is_running;
    bool                    m_b_connected;
    void                    recv_proc(void);

    // send syncronization
    CMutex*                 m_p_mutex;
	void                 	lock( void ){ m_p_mutex->lock(); }
	void                 	unlock( void ){ m_p_mutex->unlock(); }
    
    // socket info
    s32                     m_n_socket;
    struct sockaddr_in      m_server_sockaddr_in;
    bool                    start(void);
    
    // recv related resource
    bool                    m_b_receiver_started;

protected :

public:
	CTCPClient(s32 (*log)(s32, s32 ,const s8*,...));
	CTCPClient(void);
    ~CTCPClient(void);
    
    s32                     get_socket(void){return m_n_socket;}
    s8*                     get_server_address(void){return inet_ntoa(m_server_sockaddr_in.sin_addr);}
    s32                     get_server_port(void){return ntohs( m_server_sockaddr_in.sin_port);}
    bool                    is_connected(void){return m_b_connected;}
    
    bool                    wait_receiver( bool flag );
    bool                    restart( void );
    bool                    stop(void);
    bool                    start(s8* ip, s32 port);
    
    bool                    sender( u8* i_p_buf, s32 i_n_len );

    virtual bool            receiver_todo( u8* data, s32 length ) = 0;
    virtual void            notify_close(void) = 0;

    friend void *           client_service_receiver( void * arg );
};

#endif /*  _TCP_CLIENT_ */
