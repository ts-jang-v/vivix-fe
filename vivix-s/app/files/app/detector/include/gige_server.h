/*
 * gige_server.h
 *
 *  Created on: 2013. 12. 17.
 *      Author: myunghee
 */

 
#ifndef _GIGESERVER_H_
#define _GIGESERVER_H_

/*******************************************************************************
*	Include Files
*******************************************************************************/
#include "typedef.h"
#include "udp_server.h"
#include "gvcp.h"
#include "vw_system.h"
#include "net_manager.h"

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

class CGigEServer : public CUDPServer
{
private:
        
    int			(* print_log)(int level,int print,const char * format, ...);
protected:
public:
    
	bool		receiver_todo( udp_handle_t* i_p_handle_t, u8* i_p_recv_buf, s32 i_n_len );
	
	CGigEServer(int port, int (*log)(int,int,const char *,...));
	CGigEServer(void);
	~CGigEServer();
	
	// system
	CVWSystem*     	m_p_system_c;
	u8				m_p_mac_addr[6];
	
	CNetManager*	m_p_net_manager_c;
};


#endif /* end of _GIGESERVER_H_*/

