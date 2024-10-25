/******************************************************************************/
/**
 * @file    scu_server.cpp
 * @brief   SCU 요청을 처리한다. EXP_OK 신호가 발생하면 SCU에 알린다.
 * @author  
*******************************************************************************/

/*******************************************************************************
*   Include Files
*******************************************************************************/
#include <iostream>
#include "scu_server.h"
#include "message_def.h"
#include "misc.h"
#include "vw_file.h"

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
 * @brief   생성자
 * @param   port: port 번호
 * @param   log: log 함수
 * @return  없음
 * @note    
*******************************************************************************/
CSCUServer::CSCUServer(int port, int max, \
                int (*log)(int,int,const char *,...)):CTCPServer(port,max, log)
{
    
    print_log = log;
    m_b_debug = false;
    
    m_b_client_valid    = false;
    m_n_client_fd       = -1;

	m_p_system_c = NULL;
    
    m_p_mutex           = new CMutex;
    
    m_p_alive_timer     = NULL;
    m_n_alive_interval_time = 2;
    m_b_alive_timer_update = false;
    
    m_p_msgq_c = new CMessage;
    
    // notification proc start
    m_b_noti_is_running = true;
	if( pthread_create( &m_noti_thread_t, NULL, scu_noti_routine, (void*) this ) != 0 )
    {
    	print_log( ERROR, 1, "[SCU Server] scu_noti_routine pthread_create:%s\n",strerror( errno ));

    	m_b_noti_is_running = false;
    }
	
	// alive proc start
    m_b_alive_is_running = true;
	if( pthread_create( &m_alive_thread_t, NULL, scu_alive_routine, (void*) this ) != 0 )
    {
    	print_log( ERROR, 1, "[SCU Server] scu_alive_routine pthread_create:%s\n",strerror( errno ));

    	m_b_alive_is_running = false;
    }
	

}
/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
CSCUServer::CSCUServer(void)
{
}

/******************************************************************************/
/**
* \brief        destructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CSCUServer::~CSCUServer()
{
    m_b_noti_is_running = false;
	
	if( pthread_join( m_noti_thread_t, NULL ) != 0 )
	{
		print_log( ERROR, 1,"[SCU Server] pthread_join: noti_proc:%s\n",strerror( errno ));
	}
    safe_delete( m_p_msgq_c );
   
    close_client( m_n_client_fd );
    
    m_b_alive_is_running = false;
	if( pthread_join( m_alive_thread_t, NULL ) != 0 )
	{
		print_log( ERROR, 1,"[SCU Server] pthread_join: alive_proc:%s\n",strerror( errno ));
	}
    
    safe_delete( m_p_alive_timer );
    safe_delete( m_p_mutex );
    
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void * scu_noti_routine( void * arg )
{
	CSCUServer * svr = (CSCUServer *)arg;
	svr->noti_proc();
	pthread_exit( NULL );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CSCUServer::noti_proc(void)
{
    print_log(DEBUG, 1, "[SCU Server] start noti_proc...\n");
    
    vw_msg_t*   p_msg_t;
    
    while( m_b_noti_is_running )
    {
        p_msg_t = m_p_msgq_c->get(1000);
        
        if( p_msg_t != NULL )
        {
            print_log(INFO, 1, "[SCU Server] msg: %04d\n", p_msg_t->type);
            
            switch( p_msg_t->type )
            {
                case eMSG_DISCONNECTION:
                {
                    noti_disconnect();
                    break;
                }
                default:
                {
                    break;
                }
            }
            
            safe_delete( p_msg_t );
        }
    }
    
    print_log(DEBUG, 1, "[SCU Server] end noti_proc...\n");
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CSCUServer::noti_add( vw_msg_t * msg )
{
    return m_p_msgq_c->add( msg );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CSCUServer::noti_disconnect(void)
{
    if( m_b_client_valid == true )
    {
        print_log(DEBUG, 1, "[SCU Server] disconnection by system\n");
        close_client( m_n_client_fd );
    }
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CSCUServer::receiver_started( s32 i_n_fd )
{
    m_n_client_fd = i_n_fd;
    
    print_log( ERROR, 1, "[SCU Server] receiver_started(fd: %d)\n", m_n_client_fd );
    alive_reset();
}

/******************************************************************************/
/**
* \brief        receive paket
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CSCUServer::receiver_todo( client_info_t* i_p_client_t, u8* i_p_buf, s32 i_n_len )
{
	client_info_t* cur_client = (client_info_t *) i_p_client_t;
	mp_header_t * phdr;
	
	int i=0;
	int remain=0;
	int total_len;
	int n_len;
	bool dump = false;

	if(cur_client->n_buf_end_pos + i_n_len >= 50 * 1024)
	{
		print_log( ERROR, 1, "[SCU Server] Buffer overflow.(fd: %d)\n", i_p_client_t->sock_fd );
		cur_client->n_buf_end_pos = 0;
		return false;
	}

	memcpy( &cur_client->p_packet_buf[cur_client->n_buf_end_pos], i_p_buf, i_n_len);
	cur_client->n_buf_end_pos += i_n_len;
	
	total_len = cur_client->n_buf_end_pos;
	
	// find header
	for( i = 0; i < cur_client->n_buf_end_pos; i++ )
	{
		phdr = (mp_header_t *)&cur_client->p_packet_buf[i];
		if( memcmp( phdr->head, "VWSP", 4 ) == 0)
		{
		    n_len = ntohs(phdr->payload_length) + sizeof(mp_header_t);
		    if( n_len  <= (cur_client->n_buf_end_pos - (i)))
		    {
		        if( packet_parse( cur_client, &cur_client->p_packet_buf[i], n_len ) == 0 )
				{
					return false;
				}
				if( n_len < (cur_client->n_buf_end_pos - (i)) )
				{
					remain = i+n_len;
					i = remain-1;
					continue;
				}
				else
				{
					cur_client->n_buf_end_pos = 0;
					return true;
				}
		    }
			else
			{
				memcpy( cur_client->p_packet_buf , &cur_client->p_packet_buf[remain], (cur_client->n_buf_end_pos - i) );
				cur_client->n_buf_end_pos = cur_client->n_buf_end_pos  - i;
				return true;
			}
		}
		else
		{
			if( dump == false )
			{
    			print_log(ERROR,1,"[SCU Server] Header error (fd: %d, %d: 0x%04X)\n", i_p_client_t->sock_fd, i, cur_client->p_packet_buf[i]);
    			print_log(DEBUG, 1, "[SCU Server] total_len: %d, buf_end_pos: %d, i: %d, remain: %d\n", total_len, cur_client->n_buf_end_pos, i, remain );
				memory_dump(cur_client->p_packet_buf, total_len);
				dump = true;
			}
		}
	}

	return true;
}

/******************************************************************************/
/**
* \brief        parsing packet
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CSCUServer::packet_parse( client_info_t* i_p_client_info_t, u8* i_p_data, u16 i_w_len )
{
	bool ret = false;
	mp_header_t* p_hdr_t;
    
	p_hdr_t = (mp_header_t *)i_p_data;
	p_hdr_t->payload_length = ntohs(p_hdr_t->payload_length);
	p_hdr_t->slice_number   = ntohs(p_hdr_t->slice_number);
	p_hdr_t->slice_size     = ntohs(p_hdr_t->slice_size);
	
    if( p_hdr_t->payload != VWSP_PAYLOAD_LOGIN )
    {
	    if( m_b_client_valid == false )
    	{
    	    print_log(INFO, 1, "[SCU Server] invalid user(fd: %d)\n", i_p_client_info_t->sock_fd);
    	    return false;
    	}
    }
    
    m_n_client_fd = i_p_client_info_t->sock_fd;
    
	print_log(DEBUG, m_b_debug, "[SCU Server] rx_do: payload(0x%02X)\n", p_hdr_t->payload);
	switch( p_hdr_t->payload )
	{
	    case VWSP_PAYLOAD_LOGIN:
	    {
	        ret = login_req( i_p_data );
	        break;
	    }
	    case VWSP_PAYLOAD_GOODBYE:
	    {
	        ret = logout_req();
	        return 0;
	    }
	    case VWSP_PAYLOAD_KEEP_ALIVE:
	    {
	        ret = alive_req();
	        break;
	    }
	    case VWSP_PAYLOAD_CAPTURE:
	    {
	        ret = exp_req();
	        break;
	    }
	    default:
	    {
	        print_log(INFO, 1, "[SCU Server] invalid payload(0x%02X)\n", p_hdr_t->payload);
	    }
	}

	return ret;
}

/******************************************************************************/
/**
* \brief        ending client
* \param        none
* \return       none
* \note
*******************************************************************************/
void CSCUServer::client_notify_close( s32 i_n_fd )
{
    print_log(INFO, 1, "[SCU Server] Disconnected(fd: %d/%d)\n", i_n_fd, m_n_client_fd);

    m_n_client_fd       = -1;
    m_b_client_valid    = false;
    
    m_p_system_c->scu_set_connection( false );
}

/******************************************************************************/
/**
* \brief        ending client
* \param        none
* \return       none
* \note
*******************************************************************************/
void CSCUServer::client_notify_recv_error( s32 i_n_fd )
{
    print_log(INFO, 1, "[SCU Server] recv error(fd: %d)\n", i_n_fd);
    message_send(eMSG_DISCONNECTION);
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CSCUServer::packet_send( u8 i_c_payload, u8* i_p_buff, u32 i_n_length )
{
    u8                  p_send_buf[2048];
    mp_header_t *		p_header_t;
    bool                b_result;
    
    print_log(DEBUG, m_b_debug, "[SCU Server] packet_send: payload(0x%02X), data size(%d)\n", \
                                        i_c_payload, i_n_length);
    p_header_t = (mp_header_t *)p_send_buf;
	p_header_t->head[0] = 'V';
	p_header_t->head[1] = 'W';
	p_header_t->head[2] = 'S';
	p_header_t->head[3] = 'P';
	p_header_t->payload = i_c_payload;
	p_header_t->slice_number = 0;
	p_header_t->slice_size= htons(i_n_length);
	p_header_t->opt = OPT_STA | OPT_END;
	p_header_t->payload_length = htons(i_n_length);;
    
	if( i_n_length != 0 && i_p_buff != NULL )
	{
		memcpy( &p_send_buf[sizeof(mp_header_t)], i_p_buff, i_n_length );
	}
	
	lock_send();
	b_result = sender( m_n_client_fd, p_send_buf, (sizeof(mp_header_t) + i_n_length) );
	unlock_send();
	
	return b_result;
}

/******************************************************************************/
/**
 * @brief   SCU 접속인지 확인한다.
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CSCUServer::check_scu( u32 i_n_time, u8* i_p_encode )
{
    u8 key[8];
    u8 p_time[4];
    
    memset( key, 0, sizeof(key) );
    memcpy( p_time, &i_n_time, sizeof(p_time) );
    
    key[0] = p_time[0];
    key[2] = p_time[1];
    key[4] = p_time[2];
    key[6] = p_time[3];
    key[1] = 'V';
    key[3] = 'F';
    key[5] = 'X';
    key[7] = 'R';
    
    m_cipher_c.Initialize( key, sizeof(key) );
    
    u8 p_out[8];
    m_cipher_c.Decode( i_p_encode, p_out, sizeof(p_out) );
    
    if( memcmp( p_out, (void*)"SCU", 3 ) == 0 )
    {
        print_log(INFO, 1, "[SCU Server] SCU is connected.\n");
        return true;
    }
    
    print_log(INFO, 1, "[SCU Server] client id is invalid.\n");
    return false;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CSCUServer::login_req( u8* i_p_data )
{
    time_t time;
    u8 encode[8];
    
	memcpy( &time, &i_p_data[sizeof(mp_header_t)], 4 );
	time = ntohl(time);
	
	memcpy( encode, &i_p_data[sizeof(mp_header_t) + 4], 8 );
	
    if( check_scu( time, encode ) == true )
    {
        m_b_client_valid = true;
        m_n_alive_interval_time = 10;
        alive_reset();
        
        m_p_system_c->scu_set_connection( true );
        
        return packet_send( VWSP_PAYLOAD_LOGIN );
    }
    
    return false;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CSCUServer::logout_req(void)
{
    m_b_client_valid = false;
    
    return false;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CSCUServer::alive_req(void)
{
    bool        ret;
    
    // stop alive check timer
    alive_reset();
    
    
    ret = packet_send( VWSP_PAYLOAD_KEEP_ALIVE );
    
    
    return ret;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CSCUServer::exp_req( void )
{
    if( m_p_system_c->exposure_request() == true )
    {
        return packet_send(VWSP_PAYLOAD_EXP_OK);
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
bool CSCUServer::exp_ack(void)
{
    return packet_send(VWSP_PAYLOAD_EXP_OK);

}


/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void * scu_alive_routine( void * arg )
{
	CSCUServer * svr = (CSCUServer *)arg;
	svr->alive_proc();
	pthread_exit( NULL );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CSCUServer::alive_proc(void)
{
    print_log(DEBUG, 1, "[SCU Server] start alive_proc...\n");
    
    CTime* p_timer = new CTime(1000);
    
    while( m_b_alive_is_running )
    {
        if( p_timer->is_expired() == true )
        {
            if( m_b_alive_timer_update == true )
            {
                m_b_alive_timer_update = false;
                safe_delete(m_p_alive_timer);
                m_p_alive_timer = new CTime(m_n_alive_interval_time * 1000);
            }
            else
            {
                if( m_p_alive_timer != NULL
                    && m_p_alive_timer->is_expired() == true )
                {
                   close_client( m_n_client_fd );
                   print_log(DEBUG, 1, "[SCU Server] alive check: SCU time out\n");
                   safe_delete(m_p_alive_timer);
                }
            }
            
            safe_delete( p_timer );
            p_timer = new CTime(1000);
        }
        else
        {
            sleep_ex(500000);
        }
    }
    
    safe_delete( p_timer );
    
    print_log(DEBUG, 1, "[SCU Server] end alive_proc...\n");
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CSCUServer::alive_reset(void)
{
    m_b_alive_timer_update = true;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CSCUServer::message_send( MSG i_msg_e, u8* i_p_data, u16 i_w_data_len )
{
    if( i_w_data_len > VW_PARAM_MAX_LEN )
    {
        print_log(ERROR, 1, "[SCU Server] message data lenth is too long(%d/%d).\n", \
                        i_w_data_len, VW_PARAM_MAX_LEN );
        return;
    }
    
    vw_msg_t msg;
    bool b_result;
    
    memset( &msg, 0, sizeof(vw_msg_t) );
    msg.type = (u16)i_msg_e;
    msg.size = i_w_data_len;
    
    if( i_w_data_len > 0 )
    {
        memcpy( msg.contents, i_p_data, i_w_data_len );
    }
    
    b_result = noti_add( &msg );
    
    if( b_result == false )
    {
        print_log(ERROR, 1, "[SCU Server] message_send(%d) failed\n", i_msg_e );
    }
}
