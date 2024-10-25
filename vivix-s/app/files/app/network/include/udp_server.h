/*
 * udp_server.h
 *
 *  Created on: 2013. 12. 17.
 *      Author: myunghee
 */

#ifndef UDP_SERVER_
#define UDP_SERVER_


/*******************************************************************************
*	Include Files
*******************************************************************************/
#include "network.h"

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/


/*******************************************************************************
*	Type Definitions
*******************************************************************************/
typedef struct _udp_handle
{
    s32                     nserver_socket;
    struct sockaddr_in      input_addr_t;
}udp_handle_t;

/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*	Variable Definitions
*******************************************************************************/


/*******************************************************************************
*	Function Prototypes
*******************************************************************************/
void*   udp_server_routine( void * arg );

class CUDPServer : public CNetwork
{
private:
    pthread_t               m_server_t;
    bool                    m_bis_running;
    struct sockaddr_in      m_server_addr_t;
    
    s32                     m_nserver_socket;
    struct sockaddr_in      m_input_addr_t;
    
    u8                      m_pbuf_recv[ ETHERMTU ];
    
    void                    server_proc(void);
    int                     (* print_log)(int level,int print,const char * format, ...);
	
protected :

public:
	bool                 	stop(void);
	bool	            	start(void);
	
	bool                    sender(udp_handle_t* a_phandle_t, u8* a_pbuf, s32 a_nlen);
	virtual bool	    	receiver_todo(udp_handle_t* a_phandle_t, u8* a_pbuf_recv, s32 a_nlen)=0;
	
	friend void *         	udp_server_routine( void * arg );
	
	CUDPServer(int port, int (*log)(int,int,const char *,...));
	CUDPServer(void);
    ~CUDPServer(void);
};

#endif /* end of UDP_SERVER_*/

