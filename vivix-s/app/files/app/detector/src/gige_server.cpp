/*******************************************************************************
 모  듈 : gige_server
 작성자 : 한명희
 버  전 : 0.0.1
 설  명 : GigE pacekt 처리
 참  고 : 

 버  전:
         0.0.1  최초 작성
*******************************************************************************/



/*******************************************************************************
*	Include Files
*******************************************************************************/
#include <iostream>
#include "gige_server.h"
#include "misc.h"
#include "network.h"

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/

/*******************************************************************************
*	Type Definitions
*******************************************************************************/
typedef struct _DUMMY_REG_
{
	u32 addr;
	u32 data;
}gige_reg_t;


gige_reg_t gige_reg[]=
{
	{ 0x0004,0x0001},
	{ 0x0a00,0x0000},
	{ 0x0938,0x0bb8},
	{ 0x0900,0x0001},
	{ 0x0b10,0x0000},
	{ 0x0b00,0x0000},
	{ 0x0000,0x0000},
	{ 0x0904,0x0001},
	{ 0x0934,0x001f},
	{ 0x093c,0x0000},
	{ 0x0940,0x04f790d5},
	{ 0x0944,0x0000},
	{ 0x0d04,0x0000},
	{ 0x0204,0x0000},
	{ 0x88008,0x0000},
	{ 0x881b4,0x0000},
	{ 0x881b8,0x0000},
	{ 0x881bc,0x0000},
	{ 0x881c0,0x0000},
	{ 0x881c4,0x0000},
	{ 0x881c8,0x0000},
};

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
CGigEServer::CGigEServer(int port, int (*log)(int,int,const char *,...)):CUDPServer(port, log)
{
    print_log = log;
    m_p_system_c = NULL;
	m_p_net_manager_c = NULL;
	
    print_log(DEBUG, 1, "[GigE Server] CGigEServer\n");
}
/******************************************************************************/
/**
* \brief        constructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CGigEServer::CGigEServer(void)
{
}

/******************************************************************************/
/**
* \brief        destructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CGigEServer::~CGigEServer()
{
    print_log(DEBUG, 1, "[GigE Server] ~CGigEServer\n");
}

/******************************************************************************/
/**
* \brief        receive paket
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CGigEServer::receiver_todo( udp_handle_t* i_p_handle_t, u8* i_p_recv_buf, s32 i_n_len )
{
	gvcp_header_t 		gvcp;
	discovery_ack_t 	*ack;
	socklen_t			socklen = sizeof( struct sockaddr );
	s8* src_addr;
	u32 n_src_addr = ntohl(i_p_handle_t->input_addr_t.sin_addr.s_addr);
	
	src_addr = inet_ntoa( i_p_handle_t->input_addr_t.sin_addr );
	
	if( m_p_net_manager_c->net_is_ready() == false )
	{
	    print_log(DEBUG,1, "[GigE] GVCP_COMMAND_DISCOVERY_CMD(ip: %s), network is not ready\n", src_addr);
	    return true;
	}
	
    memcpy( &gvcp, i_p_recv_buf, sizeof(gvcp_header_t) );
	gvcp.packet_type = ntohs(gvcp.packet_type);
	gvcp.command = ntohs(gvcp.command);
	gvcp.size = ntohs(gvcp.size);
	gvcp.count = ntohs(gvcp.count);
	
    if( gvcp.command == GVCP_COMMAND_DISCOVERY_CMD )
	{
		print_log(DEBUG,1, "[GigE] GVCP_COMMAND_DISCOVERY_CMD(ip: %s(0x%08X))\n", src_addr, n_src_addr);
		
		//m_p_net_manager_c->net_check_subnet( n_src_addr );
		
		ack = (discovery_ack_t *)malloc(sizeof(discovery_ack_t));
		memset(ack,0,sizeof(discovery_ack_t));
		ack->status	 = 0x0000;
		ack->answer = htons(GVCP_COMMAND_DISCOVERY_ACK);
		ack->length = htons(0x00f8);
		ack->ack_id = htons(gvcp.count);
		ack->version_major = htons(0x0001);
		ack->device_mode = htonl(0x80000001);
		
		m_p_net_manager_c->net_get_mac(ack->mac);
		ack->current_ip = m_p_system_c->net_get_ip();
		
		ack->ip_config_option = htonl(0x00000007);
		ack->ip_config_current = htonl(0x00000005);
		
		ack->current_subnet = m_p_system_c->net_get_subnet();
		ack->default_gateway = htonl(0x0);
		
		strcpy((char *)ack->manufacture_name,		"VieWorks" );
		strcpy((char *)ack->model_name,				(const char*)m_p_system_c->get_model_name() );
		strcpy((char *)ack->device_version,			(const char*)m_p_system_c->get_device_version() );
		strcpy((char *)ack->serial,					(const char*)m_p_system_c->get_serial() );
		
		sendto( i_p_handle_t->nserver_socket,
	    				ack,
						sizeof(discovery_ack_t),0,
						( struct sockaddr * ) & i_p_handle_t->input_addr_t,
						socklen );
		free(ack);
	}
	else if(gvcp.command == GVCP_COMMAND_READ_REGISTER_CMD)
	{
		u32  address;
		size_t	 size;
		gvcp_packet_t * gvcp_t;
		u32 i;
		
		memcpy(&address,&i_p_recv_buf[sizeof(gvcp_header_t)],gvcp.size);
		
		address = ntohl(address);
		
		for(i = 0 ; i < sizeof(gige_reg)/8 ; i++)
		{
			if(gige_reg[i].addr == address)
			{
				if(gige_reg[i].addr >= 0x881b4 && gige_reg[i].addr <= 0x881c8)
				{
				    u8 p_mac[6];
		            m_p_system_c->net_get_mac(p_mac);
					gige_reg[i].data = p_mac[(gige_reg[i].addr - 0x881b4)/4];
				}
				break;
			}
		}

		if(i != sizeof(gige_reg)/8 )
		{
			gvcp_t = gvcp_packet_new_read_register_ack 	(gige_reg[i].data,gvcp.count, &size);
		}
		else
		{
			gvcp_t = gvcp_packet_new_read_register_ack 	(1,gvcp.count, &size);
		}
		
		sendto( i_p_handle_t->nserver_socket,
	    				gvcp_t,
						size,0,
						( struct sockaddr * ) & i_p_handle_t->input_addr_t,
						socklen );
		safe_free(gvcp_t);
	}
	else if(gvcp.command == GVCP_COMMAND_WRITE_REGISTER_CMD)
	{
		u32  address,reg_data;
		size_t	 size;
		gvcp_packet_t * gvcp_t;
		u32 i;
		
		memcpy(&address,&i_p_recv_buf[sizeof(gvcp_header_t)],4);
		memcpy(&reg_data,&i_p_recv_buf[sizeof(gvcp_header_t) + 4],4);
		address = ntohl(address);
		reg_data = ntohl(reg_data);
		
		for(i = 0 ; i < sizeof(gige_reg)/8 ; i++)
		{
			if(gige_reg[i].addr == address)
			{
				gige_reg[i].data = reg_data;
				if(gige_reg[i].addr >= 0x881b4 && gige_reg[i].addr <= 0x881c8)
				{
					m_p_mac_addr[(gige_reg[i].addr - 0x881b4)/4] = reg_data;
				}

				break;
			}
		}
		
		gvcp_t = gvcp_packet_new_write_register_ack 	(1 , gvcp.count, &size);
		
		sendto( i_p_handle_t->nserver_socket,
    				gvcp_t,
					size,0,
					( struct sockaddr * ) & i_p_handle_t->input_addr_t,
					socklen );
		safe_free(gvcp_t);
		if( address == 0x881c8)
		{
			manufacture_info_t p_info_t;
			m_p_system_c->net_set_mac(m_p_mac_addr);
			m_p_system_c->manufacture_info_get(&p_info_t);
			m_p_system_c->manufacture_info_set(&p_info_t);
		}
    }
    else if(gvcp.command == GVCP_COMMAND_READ_MEMORY_CMD)
	{
		u32  address;
		size_t	 size;
		gvcp_packet_t * gvcp_t;
		u32 i;
		
		read_mem_t	*cmd;
		
		cmd = (read_mem_t *)&i_p_recv_buf[sizeof(gvcp_header_t)] ;
		cmd->count = ntohs(cmd->count);
		
		memcpy(&address,&i_p_recv_buf[sizeof(gvcp_header_t)],gvcp.size);
		address = ntohl(address);
		for(i = 0 ; i < sizeof(gige_reg)/8 ; i++)
		{
			
			if(gige_reg[i].addr == address)
			{
				if(gige_reg[i].addr >= 0x881b4 && gige_reg[i].addr <= 0x881c8)
				{
				    u8 p_mac[6];
		            m_p_system_c->net_get_mac(p_mac);
					gige_reg[i].data = p_mac[(gige_reg[i].addr - 0x881b4)/4];
				}
				break;
			}
		}

		if(i != sizeof(gige_reg)/8 )
		{
			u32 n_gige_reg_data = ntohl(gige_reg[i].data);
			gvcp_t = gvcp_packet_new_read_memory_ack 	(address,cmd->count,gvcp.count, &size);
			memcpy (&((u8*)gvcp_t)[sizeof(gvcp_packet_t)], &n_gige_reg_data, sizeof (uint32_t));

		}
		else
		{
			gvcp_t = gvcp_packet_new_read_memory_ack 	(1,cmd->count,gvcp.count, &size);
		}
		sendto(	i_p_handle_t->nserver_socket,
						gvcp_t,
						size,0,
						( struct sockaddr * ) & i_p_handle_t->input_addr_t,
						socklen);
		safe_free(gvcp_t);
	}
	else if(gvcp.command == GVCP_COMMAND_WRITE_MEMORY_CMD)
	{
		u32  address,reg_data;
		size_t	 size;
		gvcp_packet_t * gvcp_t;
		u32 i;

		memcpy(&address,&i_p_recv_buf[sizeof(gvcp_header_t)],4);
		memcpy(&reg_data,&i_p_recv_buf[sizeof(gvcp_header_t) + 4],4);
		address = ntohl(address);
		reg_data = ntohl(reg_data);

		for(i = 0 ; i < sizeof(gige_reg)/8 ; i++)
		{
			if(gige_reg[i].addr == address)
			{
				gige_reg[i].data = reg_data;
				if(gige_reg[i].addr >= 0x881b4 && gige_reg[i].addr <= 0x881c8)
				{
					m_p_mac_addr[(gige_reg[i].addr - 0x881b4)/4] = reg_data;
				}

				break;
			}
		}
					//if(i != sizeof(gige_reg)/8 )
		gvcp_t = gvcp_packet_new_write_memory_ack 	(1 /*gige_reg[i].data*/, gvcp.count, &size);
		sendto( i_p_handle_t->nserver_socket,
				gvcp_t,
				size,0,
				( struct sockaddr * ) & i_p_handle_t->input_addr_t,
				socklen );
		safe_free(gvcp_t);
		if( address == 0x881c8)
		{
			manufacture_info_t p_info_t;
			m_p_system_c->net_set_mac(m_p_mac_addr);
			m_p_system_c->manufacture_info_get(&p_info_t);
			m_p_system_c->manufacture_info_set(&p_info_t);
		}
	}

    return true;
}
