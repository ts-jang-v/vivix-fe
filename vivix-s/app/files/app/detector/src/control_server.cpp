/******************************************************************************/
/**
 * @file    control_server.cpp
 * @brief   SDK/SCU 제어 명령 처리
 * @author  한명희
*******************************************************************************/

/*******************************************************************************
*   Include Files
*******************************************************************************/

#ifdef TARGET_COM
#include <iostream>
#include <sys/time.h>
#include "control_server.h"
#include "message_def.h"
#include "misc.h"
#include "vw_file.h"
#include "vw_system_def.h"
#include "vw_xml.h"
#else
#include "Thread.h"
#include "Dev_io.h"
#include "File.h"
#include "../app/detector/include/control_server.h"

#include "typedef.h"

#include "Common_def.h"

#include "Vw_system.h"

#include "File.h"

#include <string.h>
#endif
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
CControlServer::CControlServer(int port, int max, \
                int (*log)(int,int,const char *,...)):CTCPServer(port,max, log)
{
    
    print_log = log;
    m_b_debug = false;
    
    m_p_msgq_c = new CMessage;
    m_noti_thread_t = (pthread_t)0;
    m_b_noti_is_running = false;
	
    // alive check
    m_b_alive_is_running = false;
    m_alive_thread_t = (pthread_t)0;
    m_n_alive_interval_time = 0;
    m_p_alive_timer = NULL;
    m_b_alive_timer_update = false;

    // client
    m_b_client_valid    = false;
    m_n_client_seq_id   = 0;
    m_n_client_fd       = -1;

    // send syncronization
    m_p_mutex           = new CMutex;


    m_p_img_retransfer_c = NULL;    



    // image send
    m_p_stream_image_t = NULL;
    m_n_stream_payload = 0;
    m_n_stream_crc = 0;
    m_w_stream_slice_total = 0;
    m_w_stream_slice_current = 0;
    m_stream_thread_t = (pthread_t)0;
    m_b_stream_is_running = false;
    m_b_stream_sending_error = false;
    m_b_stream_send_enable  = false;
    m_n_stream_id           = 0xFFFFFFFF;

    // system update
    m_p_download_c      = NULL;
    //s8                  m_p_file_name[256];
    //VW_FILE_ID          m_file_id_e;
    //u32                 m_n_file_crc;            

    // throughput test
    //pthread_t			m_trans_thread_t;
    m_b_trans_is_running = false;
    //u16                 m_w_trans_slice_id;

    // offset cal
    m_b_offet_cal_stop  = false;

    // packet test
    m_w_packet_seq      = 0;

    m_p_system_c = 0;
    m_p_calibration_c = 0;
    m_p_net_manager_c = 0;
    m_p_diagnosis_c = 0;

    m_p_web_c = NULL;

#ifdef DELAY_PACKET_TEST
    m_p_resend_wait_c   = NULL;
#endif    

    // notification proc start
    m_b_noti_is_running = true;
	if( pthread_create( &m_noti_thread_t, NULL, noti_routine, (void*) this ) != 0 )
    {
    	print_log( ERROR, 1, "[Control Server] noti_routine pthread_create:%s\n",strerror( errno ));

    	m_b_noti_is_running = false;
    }
	print_log(DEBUG, 1, "[Control Server] CControlServer\n");

}
/******************************************************************************/
/**
* \brief        constructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CControlServer::CControlServer(void)
{
}

/******************************************************************************/
/**
* \brief        destructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CControlServer::~CControlServer()
{
    //safe_free( m_p_save_image_t->p_buffer );
    sender_stop();
    
    close_client( m_n_client_fd );
    
    
    m_b_noti_is_running = false;
	if( pthread_join( m_noti_thread_t, NULL ) != 0 )
	{
		print_log( ERROR, 1,"[Control Server] pthread_join: noti_proc:%s\n",strerror( errno ));
	}
	safe_delete( m_p_msgq_c );
	
    m_b_alive_is_running = false;
	if( pthread_join( m_alive_thread_t, NULL ) != 0 )
	{
		print_log( ERROR, 1,"[Control Server] pthread_join: alive_proc:%s\n",strerror( errno ));
	}

    safe_delete( m_p_alive_timer );
    safe_delete( m_p_mutex );
    
    print_log(DEBUG, 1, "[Control Server] ~CControlServer\n");
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CControlServer::initialize(void)
{
	// alive proc start
	m_n_alive_interval_time = 5;
	m_p_alive_timer = NULL;
	m_b_alive_timer_update = true;
	
    m_b_alive_is_running = true;
	
	if( pthread_create( &m_alive_thread_t, NULL, alive_routine, (void*) this ) != 0 )
    {
    	print_log( ERROR, 1, "[Control Server] alive_routine pthread_create:%s\n",strerror( errno ));

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
void * noti_routine( void * arg )
{
	CControlServer * svr = (CControlServer *)arg;
	svr->noti_proc();
	pthread_exit( NULL );
	return 0;
}

/******************************************************************************/
/**
 * @brief   디텍터의 상태가 변하여 발생한 메시지를 처리한다.
 * @param   
 * @return  
 * @note    send 오류 시에 client socket을 닫는다.
 *          이 외의 packet_send() 함수의 send 오류 시에는
 *          receiver_todo 함수의 packet_parse 함수에서 false를 반환하여
 *          client socket이 닫히므로 여기서는 닫지 않는다.
*******************************************************************************/
void CControlServer::noti_proc(void)
{
    print_log(DEBUG, 1, "[Control Server] start noti_proc...\n");
    
    vw_msg_t*   p_msg_t;
    bool        b_result = true;
    
    while( m_b_noti_is_running )
    {
        p_msg_t = m_p_msgq_c->get(1000);
        b_result = true;
        
        if( p_msg_t != NULL )
        {
            print_log(DEBUG, 1, "[Control Server] msg: 0x%04X\n", p_msg_t->type);
            
            if( m_p_system_c == NULL
                || m_p_system_c->is_ready() == false )
            {
                print_log(DEBUG, 1, "[Control Server] system is not ready to processing msg\n");
                safe_delete( p_msg_t );
                continue;
            }
            
            switch( p_msg_t->type )
            {
                case eMSG_OFFSET_CAL_STATE:
                {
                    b_result = noti_offset_cal( p_msg_t->contents, p_msg_t->size );
                    break;
                }
                case eMSG_IMAGE_STATE:
                {
                    b_result = noti_image( p_msg_t->contents, p_msg_t->size );
                    break;
                }
                case eMSG_POWER_MODE:
                {
                    noti_power_mode( p_msg_t->contents, p_msg_t->size );
                    break;
                }
                case eMSG_DISCONNECTION:
                {
                    noti_disconnect();
                    break;
                }
                case eMSG_OFFSET_DRIFT_STATE:
                {
                    noti_osf( p_msg_t->contents, p_msg_t->size );
                    break;
                }
                default:
                	break;
            }
            
            if( b_result == false )
            {
                // close client
                close_client( m_n_client_fd );
            }
            
            safe_delete( p_msg_t );
        }
    }
    
    print_log(DEBUG, 1, "[Control Server] end noti_proc...\n");
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::noti_image( u8* i_p_data, u16 i_w_lenth )
{
    u32     n_id;
    u32     n_state;
    u16     w_pre_exp = 0;
    u8      p_data[28];
    u32     n_exp_time;
    s32     p_pos[3];
    u8      p_correction[4];
    u16     w_direction = 0;
    bool    b_result = false;
    
    memset( p_data, 0, sizeof(p_data) );
    
    memcpy( &n_state,   &i_p_data[0], sizeof(u32) );
    memcpy( &n_id,      &i_p_data[4], sizeof(u32) );
    memcpy( &n_exp_time, &i_p_data[8], sizeof(u32) );
    
    memcpy( &p_pos[0], &i_p_data[12], sizeof(s32) );
    memcpy( &p_pos[1], &i_p_data[16], sizeof(s32) );
    memcpy( &p_pos[2], &i_p_data[20], sizeof(s32) );
    
    memcpy( p_correction, &i_p_data[24], sizeof(p_correction) );
    memcpy( &w_direction, &i_p_data[28], sizeof(w_direction) );
    
    print_log(DEBUG, 1, "[Control Server] image state: %d, id: 0x%08X\n", \
                                        n_state, n_id );
    
    if( n_state != eIMG_STATE_EXPOSURE
        && n_state != eIMG_STATE_EXPOSURE_TIMEOUT )
    {
        w_pre_exp = m_p_system_c->get_pre_exp();
    }
    
    if( n_state == eIMG_STATE_EXPOSURE )
    {
        m_b_stream_send_enable = false;
    }
    
    if( n_state != eIMG_STATE_SCU_IS_CONNECTED
        && n_state != eIMG_STATE_SCU_IS_DISCONNECTED )
    {
        m_p_system_c->set_image_state(n_state);
    }
    
    n_id        = htonl( n_id );
    w_pre_exp   = htons( w_pre_exp );
    n_exp_time  = htonl( n_exp_time );
    
    p_pos[0]    = htonl( p_pos[0] );
    p_pos[1]    = htonl( p_pos[1] );
    p_pos[2]    = htonl( p_pos[2] );
    
    w_direction	= htons( w_direction );
    
    memcpy( &p_data[0], &n_id, sizeof(n_id) );
    memcpy( &p_data[4], &w_pre_exp, sizeof(w_pre_exp) );
    memcpy( &p_data[6], &n_exp_time, sizeof(n_exp_time) );
    memcpy( &p_data[10], p_pos, sizeof(p_pos) );
    memcpy( &p_data[22], p_correction, sizeof(p_correction) );
    memcpy( &p_data[26], &w_direction, sizeof(w_direction) );
    
    b_result = packet_send( PAYLOAD_NOTI_IMAGE_ACQ_STATE, 0, \
                        p_data, sizeof(p_data), (u8)n_state );
    
    if( n_state == eIMG_STATE_ACQ_START )
    {
    	// sender_start가 호출되지 않은 상태에서 영상 재전송 시
    	// stream payload를 갱신하여
    	// 이전 stream payload(offset이나 dosf 영상)으로 전송하지 않도록 한다.
    	m_n_stream_payload = PAYLOAD_IMAGE;
    	
        m_b_stream_send_enable = true;
    }
                        
    return b_result;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::noti_power_mode( u8* i_p_data, u16 i_w_lenth )
{
    PM_MODE     mode_e;
    u8          c_result;
    
    memcpy( &mode_e, i_p_data, sizeof(mode_e) );
    
    print_log(DEBUG, 1, "[Control Server] power mode: 0x%02X\n", mode_e );
    
    c_result = (u8)mode_e;
    
    if( mode_e == ePM_MODE_SHUTDOWN
        || mode_e == ePM_MODE_SHUTDOWN_BY_SCU
        || mode_e == ePM_MODE_SHUTDOWN_BY_LOW_BATTERY
        || mode_e == ePM_MODE_SHUTDOWN_BY_PWR_BUTTON )
    {
        packet_send( PAYLOAD_NOTI_POWER_MODE, 0, NULL, 0, c_result );
        m_p_system_c->hw_power_off();
        return false;
    }
    
    if( mode_e == ePM_MODE_SLEEP )
    {
        m_p_calibration_c->offset_cal_stop();
        m_p_calibration_c->offset_cal_reset();
    }
        
    if( mode_e == ePM_MODE_TETHER_POWER_OFF )
    {
        m_p_system_c->hw_bat_charge(eBAT_CHARGE_0_OFF_1_OFF);
        return true;
    }

    if( mode_e == ePM_MODE_BATTERY_SWELLING_RECHG_BLK_START )
    {
        m_p_system_c->check_need_to_block_recharging();
        return true;
    }

    if( mode_e == ePM_MODE_BATTERY_SWELLING_RECHG_BLK_STOP )
    {
        m_p_system_c->force_to_stop_to_block_recharging();
        return true;
    }
    
    if( mode_e == ePM_MODE_BATTERY_0_ON_1_OFF)
    {
    	m_p_system_c->hw_bat_charge(eBAT_CHARGE_0_ON_1_OFF);
    	return true;
    }
    
    if( mode_e == ePM_MODE_BATTERY_0_OFF_1_ON)
    {
    	m_p_system_c->hw_bat_charge(eBAT_CHARGE_0_OFF_1_ON);
    	return true;
    }
   	
   	if( mode_e == ePM_MODE_BATTERY_0_ON_1_ON)
    {
    	m_p_system_c->hw_bat_charge(eBAT_CHARGE_0_ON_1_ON);
    	return true;
    }
        
    if( mode_e == ePM_MODE_BATTERY_0_OFF_1_OFF)
    {
    	m_p_system_c->hw_bat_charge(eBAT_CHARGE_0_OFF_1_OFF);
    	return true;
    }
    
    
    if( mode_e == ePM_MODE_WAKEUP_BY_PWR_BUTTON )
    {
        m_p_system_c->pm_wake_up();     // wake up by button
        
        // m_p_system_c->pm_wake_up() 함수를 호출하면
        // PM_MODE_WAKEUP을 보내므로 여기서는 보내지 않는다.
        
        return true;
    }
    
    return packet_send( PAYLOAD_NOTI_POWER_MODE, 0, NULL, 0, c_result );
    
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CControlServer::noti_disconnect(void)
{
    if( m_b_client_valid == true )
    {
        print_log(DEBUG, 1, "[Control Server] disconnection by system\n");
        close_client( m_n_client_fd );
    }
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::req_resend_image( u8* i_p_data )
{
#if 0
    packet_header_t*    p_hdr_t;
    
    u32                 n_req_id;
    u32                 n_cur_id;
    u32                 n_pair_number;
    u32                 i;
    
    u8*                 p_data;
    
    p_hdr_t = (packet_header_t*)i_p_data;
    
    if( p_hdr_t->c_operation == 0 )     // stop resending
    {
        m_b_is_resending = false;
        print_log(DEBUG, 1, "[Control Server] resend stop\n");
        
        return packet_send( PAYLOAD_RESEND_IMAGE, m_n_client_seq_id );
    }

    p_data = &i_p_data[sizeof(packet_header_t)];
    
    memcpy( &n_req_id,      &p_data[0], 4 );
    memcpy( &n_pair_number, &p_data[4], 4 );
    
    n_req_id        = ntohl( n_req_id );
    n_pair_number   = ntohl( n_pair_number );
    
    if( n_pair_number == 0 )
    {
        return packet_send( PAYLOAD_RESEND_IMAGE, m_n_client_seq_id  );
    }
    
    // resend 중이면 error를 반환한다.
    if( m_b_is_resending == true )
    {
        return packet_send( PAYLOAD_RESEND_IMAGE, m_n_client_seq_id, NULL, 0, 1 );
    }
    
    get_current_image(&n_cur_id);
    if( n_req_id != n_cur_id )
    {
        print_log(DEBUG, 1, "[Control Server] resend req id: 0x%08X, current id: 0x%08X\n", \
                        n_req_id, n_cur_id );
        
        return packet_send( PAYLOAD_RESEND_IMAGE, m_n_client_seq_id, NULL, 0, 2 );
    }
    
    if( p_hdr_t->c_operation == 2 )     // preview resending
    {
        m_b_resend_preview = true;
    }
    else
    {
        m_b_resend_preview = false;
    }
    
    m_n_resend_req_id = n_req_id;
    
    print_log(DEBUG, 1, "[Control Server] resend req(id: 0x%08X, preview: %d, pair num: %d)\n", \
                        m_n_resend_req_id, m_b_resend_preview, n_pair_number );
    
    // set resend index
    memcpy( m_p_resend_pair_t, &p_data[8], sizeof(m_p_resend_pair_t) );
    
    m_n_resend_pair_max = n_pair_number;
    
    for( i = 0; i < m_n_resend_pair_max; i++ )
    {
        m_p_resend_pair_t[i].w_start    = ntohs(m_p_resend_pair_t[i].w_start);
        m_p_resend_pair_t[i].w_end      = ntohs(m_p_resend_pair_t[i].w_end);
        
        print_log(DEBUG, 1, "[Control Server] resend req(start: %d, end: %d)\n", \
                        m_p_resend_pair_t[i].w_start, m_p_resend_pair_t[i].w_end );
    }
        
#endif
    return packet_send( PAYLOAD_RESEND_IMAGE, m_n_client_seq_id );
}

/******************************************************************************/
/**
* \brief        SW에서 영상 전송이 완료되었음을 알려준다.
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::req_image_trans_done( u8* i_p_data )
{
    u32 n_id;
    
    memcpy( &n_id, &i_p_data[sizeof(packet_header_t)], 4 );
    
    n_id = ntohl( n_id );
    
    if( m_n_stream_id == n_id )
    {
        m_p_system_c->trans_done( true );
        print_log(DEBUG, 1, "[Control Server] image trans done(id: 0x%08X)\n", n_id );
    }
    
    if( m_p_img_retransfer_c->is_valid_id( n_id ) == true )
    {
    	m_p_img_retransfer_c->clear_image( n_id );
    	
    	print_log(DEBUG, 1, "[Control Server] image resend trans done(id: 0x%08X)\n", n_id );
    }
	
	return packet_send( PAYLOAD_IMAGE_TRANS_DONE, m_n_client_seq_id );
}

/******************************************************************************/
/**
* \brief        요청된 backup 영상을 삭제한다.
* \param        u8* i_p_data : packet data
* \return       none
* \note
*******************************************************************************/
bool CControlServer::req_backup_image_delete( u8* i_p_data )
{
    u32 n_data[2];
    
    memcpy( n_data, &i_p_data[sizeof(packet_header_t)], sizeof(n_data) );
    
    n_data[0] = ntohl( n_data[0] );
    n_data[1] = ntohl( n_data[1] );
    
    m_p_system_c->backup_image_delete( n_data[0], n_data[1] );
    
    return packet_send( PAYLOAD_BACKUP_IMAGE_DELETE, m_n_client_seq_id );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CControlServer::req_self_diagnosis( u8* i_p_data )
{
    packet_header_t*    p_hdr_t;
    u8                  c_result = 0;       // success
    u8                  c_oper;
    
    p_hdr_t = (packet_header_t*)i_p_data;
    c_oper  = p_hdr_t->c_operation;

    // make list
    if( c_oper == 0 )
    {
        u8      c_scu;
        bool    b_scu_used;
        memcpy( &c_scu, &i_p_data[sizeof(packet_header_t)], sizeof(c_scu) );
        
        if( c_scu == 1 )
        {
            b_scu_used = true;
        }
        else
        {
            b_scu_used = false;
        }
        
        if( m_p_diagnosis_c->diag_make_list(b_scu_used) == true )
        {
            c_result = 0;
        }
        else
        {
            c_result = 1;
        }
        
        print_log(DEBUG, 1, "[Control Server] diag make list(%s)\n", c_result==0?"success":"failed");
        
        return packet_send( PAYLOAD_SELF_DIAGNOSIS, m_n_client_seq_id, \
                                NULL, 0, c_result);
    }
    
    // diag start
    if( c_oper == 1 )
    {
        u8      p_id[2];
        
        memcpy( &p_id, &i_p_data[sizeof(packet_header_t)], sizeof(p_id) );
        
        if( m_p_diagnosis_c->diag_start( (DIAG_CATEGORY)p_id[0], p_id[1] ) == true )
        {
            c_result = 0;
        }
        else
        {
            c_result = 1;
        }
        
        print_log(DEBUG, 1, "[Control Server] diag start(%d, %d)(%s)\n", p_id[0], p_id[1], c_result==0?"success":"failed");
        
        return packet_send( PAYLOAD_SELF_DIAGNOSIS, m_n_client_seq_id, \
                                NULL, 0, c_result);
    }
    
    // diag cancel
    if( c_oper == 2 )
    {
        m_p_diagnosis_c->diag_stop();
        
        c_result = (u8)m_p_diagnosis_c->diag_get_result();
        
        print_log(DEBUG, 1, "[Control Server] diag stop(%d)\n", c_result);
        
        return packet_send( PAYLOAD_SELF_DIAGNOSIS, m_n_client_seq_id, \
                                NULL, 0, c_result);
    }

    // diag progress
    if( c_oper == 3 )
    {
        u8 c_progress = (u8)m_p_diagnosis_c->diag_get_progress();
        
        c_result  = (u8)eDIAG_RESULT_FAIL;
        
        if( c_progress >= 100 )
        {
            c_result = (u8)m_p_diagnosis_c->diag_get_result();
            print_log(DEBUG, 1, "[Control Server] diag progress(result: %d)\n", c_result);
        }
        
        return packet_send( PAYLOAD_SELF_DIAGNOSIS, m_n_client_seq_id, \
                                &c_progress, sizeof(c_progress), c_result);
    }
    
    print_log(DEBUG, 1, "[Control Server] unknown diag operation(%d)\n", c_oper);
    return false;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CControlServer::req_acceleration_control( u8* i_p_data )
{
    packet_header_t*    p_hdr_t;
    u8                  c_result = 0;       // success
    u8                  c_oper;
    
    p_hdr_t = (packet_header_t*)i_p_data;
    c_oper  = p_hdr_t->c_operation;

    s32 p_acc[3];
    // get main angular sensor value 
    if( c_oper == 0 )
    {
        memset( p_acc, 0, sizeof(p_acc) );
        
        m_p_system_c->get_main_detector_angular_position( &p_acc[0], &p_acc[1], &p_acc[2]);
        
        p_acc[0] = htonl( p_acc[0] );
        p_acc[1] = htonl( p_acc[1] );
        p_acc[2] = htonl( p_acc[2] );
        
        return packet_send( PAYLOAD_ACCELERATION_CONTROL, m_n_client_seq_id, (u8*)p_acc, sizeof(p_acc) );
    }
    
    // start calibration
    if( c_oper == 1 )
    {
        m_p_system_c->impact_calibration_start();
        
        return packet_send( PAYLOAD_ACCELERATION_CONTROL, m_n_client_seq_id );
    }
    // stop calibration
    if( c_oper == 2 )
    {
        m_p_system_c->impact_calibration_stop();
        
        return packet_send( PAYLOAD_ACCELERATION_CONTROL, m_n_client_seq_id );
    }

    // get progress
    if( c_oper == 3 )
    {
        u8 c_progress;
        
        c_progress = m_p_system_c->impact_calibration_get_progress();
        
        return packet_send( PAYLOAD_ACCELERATION_CONTROL, m_n_client_seq_id, &c_progress, sizeof(c_progress) );
    }
    
    // get result
    if( c_oper == 4 )
    {
        //get main calibration result
        if(m_p_system_c->get_main_angular_offset_cal_process_result() == false)
        {
            c_result = 1;   //invalid calibration
        } 
        else
        {
            //If valid, Save calculated offset data & calibration result to non-volitile memory (ROM)
            m_p_system_c->angular_offset_cal_save();
        }       
        return packet_send( PAYLOAD_ACCELERATION_CONTROL, m_n_client_seq_id, NULL, 0, c_result );
    }
    
    // Acceleration value stable proc start
    if( c_oper == 5 )
    {
    	u8 c_enable = 0;
    	memcpy(&c_enable, &i_p_data[sizeof(packet_header_t)], sizeof(c_enable));
    	
    	if(c_enable == 1)
    		m_p_system_c->impact_stable_pos_proc_start();
    	else
    		m_p_system_c->impact_stable_pos_proc_stop();
    	
    	return packet_send( PAYLOAD_ACCELERATION_CONTROL, m_n_client_seq_id );
    }
    
    // Acceleration value stable proc is running
    if( c_oper == 6 )
    {
    	u8 c_is_running = (u8)m_p_system_c->impact_stable_proc_is_running();
    	
    	return packet_send( PAYLOAD_ACCELERATION_CONTROL, m_n_client_seq_id, &c_is_running, sizeof(c_is_running) );
    }   	
    
    // Set acceleration sensor calibration direction
    if( c_oper == 7 )
    {
    	u8 c_direction = 0;
    	memcpy(&c_direction, &i_p_data[sizeof(packet_header_t)], sizeof(c_direction));
    	
    	m_p_system_c->impact_set_cal_direction((ACC_DIRECTION)c_direction);
    	
    	return packet_send( PAYLOAD_ACCELERATION_CONTROL, m_n_client_seq_id );
    }  

    // get 6 plane calibration done, mis-direction
    if( c_oper == 8 )
    {
    	u8 pdata[2];
    	pdata[0] = m_p_system_c->get_6plane_impact_sensor_cal_done();
    	pdata[1] = m_p_system_c->get_6plane_impact_sensor_cal_misdir();
    	
    	return packet_send( PAYLOAD_ACCELERATION_CONTROL, m_n_client_seq_id, pdata, sizeof(pdata) );
    }  

    // get sub angular sensor value
    if( c_oper == 9 )
    {
        memset( p_acc, 0, sizeof(p_acc) );
        
        m_p_system_c->get_sub_detector_angular_position( &p_acc[0], &p_acc[1], &p_acc[2] );         //accelerometer 
        
        p_acc[0] = htonl( p_acc[0] );
        p_acc[1] = htonl( p_acc[1] );
        p_acc[2] = htonl( p_acc[2] );
        
        return packet_send( PAYLOAD_ACCELERATION_CONTROL, m_n_client_seq_id, (u8*)p_acc, sizeof(p_acc) );
    }
    
    // get result(accelerometer calibration result (within operaiton 1))
    if( c_oper == 10 )
    {
        //get main calibration result
        if(m_p_system_c->get_6plane_impact_sensor_cal_done() == 0)
        {
            c_result = 1;   //invalid calibration
        } 
        else
        {
            //do nothing. cal result already saved to non-volitile memory by operation 4
        }
        return packet_send( PAYLOAD_ACCELERATION_CONTROL, m_n_client_seq_id, NULL, 0, c_result );
    }
    
    return packet_send( PAYLOAD_ACCELERATION_CONTROL, m_n_client_seq_id );
    
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CControlServer::req_web_configuration( u8* i_p_data )
{
    packet_header_t*    p_hdr_t;
    u8                  c_oper;
    patient_info_t		info_t;
    
    p_hdr_t = (packet_header_t*)i_p_data;
    c_oper  = p_hdr_t->c_operation;
    
    if( m_p_web_c == NULL )
    {
    	return packet_send( PAYLOAD_WEB_CONFIGURATION, m_n_client_seq_id );
    }
    
    if( c_oper == 0 )
    {
    	memcpy( &info_t, &i_p_data[sizeof(packet_header_t)], sizeof(patient_info_t) );
    	
    	// add for backup image
    	m_p_system_c->patient_info_set( info_t );
		
		m_p_web_c->set_patient_info( info_t );
	}
	else if( c_oper == 1 )
	{
		m_p_web_c->reset_admin_password();
	}
	else if( c_oper == 2 )
	{
		m_p_web_c->init_patient_info();
	}
	
	return packet_send( PAYLOAD_WEB_CONFIGURATION, m_n_client_seq_id );
    
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CControlServer::req_packet_trigger( u8* i_p_data )
{
    packet_header_t*    p_hdr_t;
    u8                  c_oper;
    bool				b_result = false;
    bool				b_send_result = false;
    u8                  c_result = 1;
    u16					p_exposure[2] = {0,0};
    struct timespec		prev_time_t, cur_time_t;
    u32					n_elapsed_time_ms = 0;
    
    p_hdr_t = (packet_header_t*)i_p_data;
    c_oper  = p_hdr_t->c_operation;
    
    clock_gettime(CLOCK_MONOTONIC, &prev_time_t);
    
    if( c_oper == 0 )
    {
    	b_result = m_p_system_c->exposure_request(eTRIGGER_TYPE_PACKET);
    	
    	if( b_result == true )
	    {
	        c_result = 0;
	        
	        if( m_p_system_c->exposure_ok() == true )
	        {
	        	p_exposure[0] = htons(1);
	        }
	        else
	        {
	        	p_exposure[0] = 0;
	        }
	        p_exposure[1] = htons( m_p_system_c->exposure_ok_get_wait_time() );
	    }
	}
	
	b_send_result = packet_send( PAYLOAD_PACKET_TRIGGER, m_n_client_seq_id, (u8*)p_exposure, sizeof(p_exposure), c_result );
	clock_gettime(CLOCK_MONOTONIC, &cur_time_t);
	
	if( (cur_time_t.tv_sec - prev_time_t.tv_sec) != 0)
		n_elapsed_time_ms = cur_time_t.tv_nsec + 1000000000 - prev_time_t.tv_nsec;
	else
		n_elapsed_time_ms = cur_time_t.tv_nsec - prev_time_t.tv_nsec;

	print_log( DEBUG, 1, "[Control Server] Paket trigger response time: %d ms\n", n_elapsed_time_ms / 1000000 );
	return b_send_result;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CControlServer::req_image_retransmission( u8* i_p_data )
{
    packet_header_t*    p_hdr_t;
    u8                  c_oper;
    u8                  c_result = eIMAGE_RETRANS_REQUEST_RESULT_SUCCESS;
    u32					n_image_id;
    u16					w_data, w_type;
    image_retrans_req_t	req_t;
    u32					n_pos = 0;
    
    p_hdr_t = (packet_header_t*)i_p_data;
    c_oper  = p_hdr_t->c_operation;
    
    n_pos = sizeof(packet_header_t);
    memcpy( &n_image_id, &i_p_data[n_pos], sizeof(n_image_id) );
    
    n_pos += sizeof(n_image_id);
    memcpy( &w_type, &i_p_data[n_pos], sizeof(w_type) );
    
    n_pos += sizeof(w_type);
    memcpy( &w_data, &i_p_data[n_pos], sizeof(w_data) );
    
    memset( &req_t, 0, sizeof(image_retrans_req_t) );
    
    req_t.n_operation	= c_oper;
    req_t.n_image_id	= ntohl(n_image_id);
    req_t.w_image_type	= ntohs(w_type);
    req_t.w_data		= ntohs(w_data);
    
    print_log(DEBUG, 1, "[Control Server] retrans req: %d, id: 0x%08X, type: %d, data: %d\n", \
    						c_oper, req_t.n_image_id, req_t.w_image_type, req_t.w_data);
    
    if( c_oper >= eIMAGE_RETRANS_REQUEST_MAX )
    {
    	c_result = eIMAGE_RETRANS_REQUEST_RESULT_FAIL_UNKNOWN_REQUEST;
    	return packet_send( PAYLOAD_IMAGE_RETRANSMISSION, m_n_client_seq_id, NULL, 0, c_result );
    }
    
	if( c_oper == eIMAGE_RETRANS_REQUEST_CRC_PROGRESS )
    {
    	w_data = 0;
    	c_result = (u8)m_p_img_retransfer_c->request( &req_t, &w_data );
    	
    	w_data = htons(w_data);
    	
    	return packet_send( PAYLOAD_IMAGE_RETRANSMISSION, m_n_client_seq_id, (u8*)&w_data, sizeof(w_data), c_result );
	}
	
	if( c_oper == eIMAGE_RETRANS_REQUEST_CRC_RESULT )
    {
    	u32 n_data = 0;
    	c_result = (u8)m_p_img_retransfer_c->request( &req_t, &n_data );
    	
    	n_data = htonl(n_data);
    	
    	return packet_send( PAYLOAD_IMAGE_RETRANSMISSION, m_n_client_seq_id, (u8*)&n_data, sizeof(n_data), c_result );
	}
	
	if( c_oper == eIMAGE_RETRANS_REQUEST_ACCELERATION )
    {
    	s32 p_pos[3];
    	
    	memset( p_pos, 0, sizeof(p_pos) );
    	
    	c_result = (u8)m_p_img_retransfer_c->request( &req_t, p_pos );
    	
    	p_pos[0] = htonl(p_pos[0]);
    	p_pos[1] = htonl(p_pos[1]);
    	p_pos[2] = htonl(p_pos[2]);
    	
    	return packet_send( PAYLOAD_IMAGE_RETRANSMISSION, m_n_client_seq_id, (u8*)&p_pos, sizeof(p_pos), c_result );
	}
	
	if( c_oper == eIMAGE_RETRANS_REQUEST_FULL_IMAGE_CRC )
    {
    	u32 n_data = 0;
    	c_result = (u8)m_p_img_retransfer_c->request( &req_t, &n_data );
    	
    	n_data = htonl(n_data);
    	
    	return packet_send( PAYLOAD_IMAGE_RETRANSMISSION, m_n_client_seq_id, (u8*)&n_data, sizeof(n_data), c_result );
	}
	
	if( c_oper == eIMAGE_RETRANS_REQUEST_BACKUP_IMAGE_IS_SAVED )
    {
    	c_result = eIMAGE_RETRANS_REQUEST_RESULT_SUCCESS;
    	
    	if( m_p_img_retransfer_c->is_valid_id( req_t.n_image_id ) == false )
    	{
    		c_result = eIMAGE_RETRANS_REQUEST_RESULT_FAIL_INVALID_IMAGE_ID;
    		return packet_send( PAYLOAD_IMAGE_RETRANSMISSION, m_n_client_seq_id, NULL, 0, c_result );
    	}
    	
    	w_data = m_p_system_c->backup_image_is_saved( req_t.n_image_id );
    	w_data = htons(w_data);
    	
    	return packet_send( PAYLOAD_IMAGE_RETRANSMISSION, m_n_client_seq_id, (u8*)&w_data, sizeof(w_data), c_result );
	}
	
	if( c_oper == eIMAGE_RETRANS_REQUEST_STOP_SEND )
	{
		if( m_p_img_retransfer_c->is_sending() == true )
		{
			stop_send_image(m_n_client_fd);
		}
		
		c_result = m_p_img_retransfer_c->request( &req_t );
		
		return packet_send( PAYLOAD_IMAGE_RETRANSMISSION, m_n_client_seq_id, NULL, 0, c_result );
	}
	
	if( c_oper == eIMAGE_RETRANS_REQUEST_IMAGE_ID )
	{
		u32 n_data = 0;
    	
    	c_result = (u8)m_p_img_retransfer_c->get_id( &n_data );
    	
    	if( m_p_system_c->get_image_send_remain_time() == 0 )
    	{
    		n_data = 0;
    		c_result = (u8)eIMAGE_RETRANS_REQUEST_RESULT_FAIL_INVALID_IMAGE_ID;
    	}
    	
    	n_data = htonl(n_data);
    	
    	return packet_send( PAYLOAD_IMAGE_RETRANSMISSION, m_n_client_seq_id, (u8*)&n_data, sizeof(n_data), c_result );
	}
	
	c_result = (u8)m_p_img_retransfer_c->request( &req_t );
	
	return packet_send( PAYLOAD_IMAGE_RETRANSMISSION, m_n_client_seq_id, NULL, 0, c_result );
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CControlServer::req_sensitivity_calibration( u8* i_p_data )
{
    packet_header_t*    p_hdr_t;
    u8                  c_result = 0;       // success
    u8                  c_oper;
    //bool				b_result = false;
    
    p_hdr_t = (packet_header_t*)i_p_data;
    c_oper  = p_hdr_t->c_operation;

    // start sensitivity calibration
    if( c_oper == 0 )
    {
        //Not implement
        c_result = 1;
        return packet_send( PAYLOAD_SENSITIVITY_CALIBRATION, m_n_client_seq_id, NULL, 0, c_result );
    }

    // stop sensitivity calibration
    if( c_oper == 1 )
    {
        //Not implement
        c_result = 1;
        return packet_send( PAYLOAD_SENSITIVITY_CALIBRATION, m_n_client_seq_id, NULL, 0, c_result );
    }
    
    // get calibration progress
    if( c_oper == 2 )
    {
        //Not implement
        c_result = 1;
        return packet_send( PAYLOAD_SENSITIVITY_CALIBRATION, m_n_client_seq_id, NULL, 0, c_result );
    }

    // get calibration result
    if( c_oper == 3 )
    {
        //Not implement
        c_result = 1;
        return packet_send( PAYLOAD_SENSITIVITY_CALIBRATION, m_n_client_seq_id, NULL, 0, c_result );
    }

    // set product specific gain
    if( c_oper == 4 )
    {
        u32 n_data = 0;
        u32 n_pos = 0;

        n_pos = sizeof(packet_header_t);
        memcpy( &n_data, &i_p_data[n_pos], sizeof(n_data) );
        n_data = htonl(n_data);

        c_result = m_p_system_c->set_logical_digital_gain(n_data);

        return packet_send( PAYLOAD_SENSITIVITY_CALIBRATION, m_n_client_seq_id, NULL, 0, c_result );
    }

    // get product specific gain
    if( c_oper == 5 )
    {
        u32 n_data = 0;
        n_data = htonl(m_p_system_c->get_logical_digital_gain());

        return packet_send( PAYLOAD_SENSITIVITY_CALIBRATION, m_n_client_seq_id, (u8*)&n_data, sizeof(n_data), c_result );
    }

    // get product default sensitivity data
    if( c_oper == 6 )
    {
        u32 n_data[3] = { 0, 0, 0 };
        SCINTILLATOR_TYPE e_type = eSCINTILLATOR_TYPE_MAX;

        //get default sinctillator type index
        e_type = m_p_system_c->get_scintillator_type_default();
        n_data[0] = htonl(e_type); 
        
        //get default scintillator type ifs/input charge range
        n_data[1] = htonl( (u32) m_p_system_c->get_input_charge_by_scintillator_type(e_type) );

        //get default RFDG
        n_data[2] = htonl( (u32) m_p_system_c->get_roic_cmp_gain_by_scintillator_type(e_type) );

        return packet_send( PAYLOAD_SENSITIVITY_CALIBRATION, m_n_client_seq_id, (u8*)n_data, sizeof(n_data), c_result );
    }

    // get active sensitivity data
    if( c_oper == 7 )
    {
        //Not implement
        c_result = 1;
        return packet_send( PAYLOAD_SENSITIVITY_CALIBRATION, m_n_client_seq_id, NULL, 0, c_result );
    }

    return packet_send( PAYLOAD_SENSITIVITY_CALIBRATION, m_n_client_seq_id );
    
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CControlServer::req_gain_compensation_control( u8* i_p_data )
{
    packet_header_t*    p_hdr_t;
    u8                  c_result = 0;       // success
    u8                  c_oper;
    //bool				b_result = false;
    
    p_hdr_t = (packet_header_t*)i_p_data;
    c_oper  = p_hdr_t->c_operation;

    // Set Gain compensation control
    if( c_oper == 0 )
    {
        u32 n_data = 0;
        u32 n_pos = 0;

        n_pos = sizeof(packet_header_t);
        memcpy( &n_data, &i_p_data[n_pos], sizeof(n_data) );
        n_data = htonl(n_data);

        //set to m_p_reg // set to fpga.bin 
        c_result = m_p_system_c->set_gain_compensation(n_data);

        return packet_send( PAYLOAD_GAIN_COMPENSATION_CONTROL, m_n_client_seq_id, NULL, 0, c_result );
    }

    // Get Gain compensation control
    if( c_oper == 1 )
    {
        u32 n_data = 0;
        
        //read m_p_reg or fpga.bin 
        n_data = htonl(m_p_system_c->get_gain_compensation());

        return packet_send( PAYLOAD_GAIN_COMPENSATION_CONTROL, m_n_client_seq_id, (u8*)&n_data, sizeof(n_data), c_result );
    }
    
    return packet_send( PAYLOAD_GAIN_COMPENSATION_CONTROL, m_n_client_seq_id );
    
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CControlServer::test_resend( image_retrans_req_t* i_p_req_t )
{
	u16 w_data;
	
	u8 c_result = eIMAGE_RETRANS_REQUEST_RESULT_SUCCESS;
	
	printf("test_resend req: %d, id: 0x%08X, type: %d, data: %d\n", \
					i_p_req_t->n_operation, i_p_req_t->n_image_id, i_p_req_t->w_image_type, i_p_req_t->w_data);
    
    if( i_p_req_t->n_operation >= eIMAGE_RETRANS_REQUEST_MAX )
    {
    	c_result = eIMAGE_RETRANS_REQUEST_RESULT_FAIL_UNKNOWN_REQUEST;
    	printf("eIMAGE_RETRANS_REQUEST_RESULT_FAIL_UNKNOWN_REQUEST\n");
    }
	else if( i_p_req_t->n_operation == eIMAGE_RETRANS_REQUEST_CRC_PROGRESS )
    {
    	w_data = 0;
    	
    	c_result = (u8)m_p_img_retransfer_c->request( i_p_req_t, &w_data );
    	
    	printf("crc progress: %d, result: %d\n", w_data, c_result );
	}
	else if( i_p_req_t->n_operation == eIMAGE_RETRANS_REQUEST_CRC_RESULT )
    {
    	u32 n_data = 0;
    	c_result = (u8)m_p_img_retransfer_c->request( i_p_req_t, &n_data );
    	
    	printf("crc result: 0x%08X, result: %d\n", n_data, c_result );
	}
	else if( i_p_req_t->n_operation == eIMAGE_RETRANS_REQUEST_ACCELERATION )
    {
    	s32 p_pos[3];
    	
    	memset( p_pos, 0, sizeof(p_pos) );
    	
    	c_result = (u8)m_p_img_retransfer_c->request( i_p_req_t, p_pos );
    	
    	printf("x: %d, y: %d, z: %d, result: %d\n", p_pos[0], p_pos[1], p_pos[2], c_result );
	}
	else if( i_p_req_t->n_operation == eIMAGE_RETRANS_REQUEST_FULL_IMAGE_CRC )
    {
    	u32 n_data = 0;
    	c_result = (u8)m_p_img_retransfer_c->request( i_p_req_t, &n_data );
    	
    	printf("full image crc: 0x%08X, result: %d\n", n_data, c_result );
	}
	else if( i_p_req_t->n_operation == eIMAGE_RETRANS_REQUEST_BACKUP_IMAGE_IS_SAVED )
    {
    	c_result = eIMAGE_RETRANS_REQUEST_RESULT_SUCCESS;
    	
    	if( m_p_img_retransfer_c->is_valid_id( i_p_req_t->n_image_id ) == false )
    	{
    		c_result = eIMAGE_RETRANS_REQUEST_RESULT_FAIL_INVALID_IMAGE_ID;
    		printf("eIMAGE_RETRANS_REQUEST_RESULT_FAIL_INVALID_IMAGE_ID\n");
    		return;
    	}
    	
    	w_data = m_p_system_c->backup_image_is_saved( i_p_req_t->n_image_id );
    	
    	printf("backup image saved: %d, result: %d\n", w_data, c_result );
	}
	else if( i_p_req_t->n_operation == eIMAGE_RETRANS_REQUEST_STOP_SEND )
	{
		stop_send_image(m_n_client_fd);
		
		c_result = m_p_img_retransfer_c->request( i_p_req_t );
		
		printf("result: %d\n", c_result );
	}
	else
	{
		c_result = (u8)m_p_img_retransfer_c->request( i_p_req_t );
		
		printf("result: %d\n", c_result );
	}
	
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CControlServer::req_ap_information( u8* i_p_data )
{
    packet_header_t*    p_hdr_t;
    u8                  c_result = 0;       // success
    u8                  c_oper;
    bool				b_result = false;
    
    p_hdr_t = (packet_header_t*)i_p_data;
    c_oper  = p_hdr_t->c_operation;

    // start AP scan
    if( c_oper == 0 )
    {
      //  b_result = m_p_net_manager_c->ap_scan_proc_start();
      
     	 b_result = true;
        
        if( b_result == false )
        {
        	c_result = 1;
        }
        
        return packet_send( PAYLOAD_AP_INFORMATION, m_n_client_seq_id, NULL, 0, c_result );
    }
    
    // get progress
    if( c_oper == 1 )
    {
        c_result = m_p_net_manager_c->ap_scan_get_progress();
        
        return packet_send( PAYLOAD_AP_INFORMATION, m_n_client_seq_id, NULL, 0, c_result );
    }
    
    // get result
    if( c_oper == 2 )
    {
        b_result = m_p_net_manager_c->ap_scan_get_result();
        
        if( b_result == false )
        {
        	c_result = 1;
        }
        
        return packet_send( PAYLOAD_AP_INFORMATION, m_n_client_seq_id, NULL, 0, c_result );
    }

    return packet_send( PAYLOAD_AP_INFORMATION, m_n_client_seq_id );
    
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::noti_add( vw_msg_t * msg )
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
void CControlServer::receiver_started( s32 i_n_fd )
{
    m_n_client_fd = i_n_fd;
    
    print_log( ERROR, 1, "[Control Server] receiver_started(fd: %d)\n", m_n_client_fd );
    alive_reset();
}

/******************************************************************************/
/**
* \brief        receive paket
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::receiver_todo( client_info_t* i_p_client_t, u8* i_p_buf, s32 i_n_len )
{
	client_info_t* cur_client = (client_info_t *) i_p_client_t;
	packet_header_t * phdr;
	
	int i=0;
	int remain=0;
	int total_len;
	bool dump = false;

	if(cur_client->n_buf_end_pos + i_n_len >= CLIENT_BUFF_SIZE)
	{
		print_log( ERROR, 1, "[Control Server] Buffer overflow.(fd: %d)\n", i_p_client_t->sock_fd );
		cur_client->n_buf_end_pos = 0;
		return false;
	}
	
	memcpy( &cur_client->p_packet_buf[cur_client->n_buf_end_pos], i_p_buf, i_n_len);
	cur_client->n_buf_end_pos += i_n_len;
	
	total_len = cur_client->n_buf_end_pos;
	
	// find header
	for( i = 0; i < cur_client->n_buf_end_pos; i++ )
	{
		phdr = (packet_header_t *)&cur_client->p_packet_buf[i];
		if( memcmp( phdr->p_head, PACKET_HEAD, 4 ) == 0)
		{
		    if( (cur_client->n_buf_end_pos - (i)) >= PACKET_SIZE )
			{
				if( packet_parse( cur_client, &cur_client->p_packet_buf[i], PACKET_SIZE ) == 0 )
				{
					return false;
				}
				
				if( (cur_client->n_buf_end_pos - (i)) > PACKET_SIZE )
				{
				    remain = i + PACKET_SIZE;
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
			    memcpy( cur_client->p_packet_buf, &cur_client->p_packet_buf[remain], (cur_client->n_buf_end_pos - i) );
				cur_client->n_buf_end_pos = cur_client->n_buf_end_pos  - i;
				return true;
			}
		}
		else
		{
			if( dump == false )
			{
    			print_log(ERROR,1,"[Control Server] Header error (fd: %d, %d: 0x%04X)\n", i_p_client_t->sock_fd, i, cur_client->p_packet_buf[i]);
    			print_log(DEBUG, 1, "[Control Server] total_len: %d, buf_end_pos: %d, i: %d, remain: %d\n", total_len, cur_client->n_buf_end_pos, i, remain );
    		    
    		    memory_dump(cur_client->p_packet_buf, total_len);
				
				dump = true;
			}
		}
	}

	return false;
}

/******************************************************************************/
/**
* \brief        parsing packet
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::packet_parse( client_info_t* i_p_client_info_t, u8* i_p_data, u16 i_w_len )
{
	bool                ret = false;
	packet_header_t*    p_hdr_t;
	u8                  p_data[PACKET_SIZE];
	
	memcpy( p_data, i_p_data, PACKET_SIZE );
    
	p_hdr_t = (packet_header_t *)p_data;
	p_hdr_t->n_seq_id        = ntohl(p_hdr_t->n_seq_id);
	p_hdr_t->w_payload       = ntohs(p_hdr_t->w_payload);
	
	//print_log(DEBUG, m_b_debug, "[Control Server] packet_parse: payload(0x%04X), seq_id(0x%08X)\n", p_hdr_t->w_payload, p_hdr_t->n_seq_id);
    
    if( p_hdr_t->w_payload != PAYLOAD_LOG_IN
        && m_b_client_valid == false )
    {
        print_log(DEBUG, 1, "[Control Server] invalid user(fd: %d)\n", i_p_client_info_t->sock_fd);
    	return false;
    }
    
    m_n_client_fd       = i_p_client_info_t->sock_fd;
    m_n_client_seq_id   = p_hdr_t->n_seq_id;
    
    switch( p_hdr_t->w_payload )
	{
	    case PAYLOAD_LOG_IN:
	    {
	        ret = req_login( i_p_data );
	        break;
	    }
	    case PAYLOAD_LOG_OUT:
	    {
	        packet_send( PAYLOAD_LOG_OUT, m_n_client_seq_id );
	        return false;
	    }
	    case PAYLOAD_WIRELESS_CONFIG_RESET:
	    {
	        ret = req_config_reset( i_p_data );
	        break;
	    }
	    case PAYLOAD_REBOOT:
	    {
	        ret = req_reboot( i_p_data );
	        break;
	    }
	    case PAYLOAD_MANUFACTURE_INFO:
	    {
	        ret = req_manufacture_info( i_p_data );
	        break;
	    }
	    case PAYLOAD_PACKAGE_VERSION:
	    {
	        ret = req_package_version( i_p_data );
	        break;
	    }
	    case PAYLOAD_UPLOAD:
	    {
	        ret = req_upload( i_p_data );
	        break;
	    }
	    case PAYLOAD_DOWNLOAD:
	    {
	        ret = req_download( i_p_data );
	        break;
	    }
	    case PAYLOAD_MEMORY_CLEAR:
	    {
	        ret = req_memory_clear( i_p_data );
	        break;
	    }
	    
	    case PAYLOAD_FUNCTION_LIST:
	    {
	        ret = req_function_list( i_p_data );
	        break;
	    }
	    
	    // detector only
	    case PAYLOAD_STATE:
	    {
	        ret = req_state( i_p_data );
	        break;
	    }
	    case PAYLOAD_SLEEP_CONTROL:
	    {
	        ret = req_sleep_control( i_p_data );
	        break;
	    }
	    case PAYLOAD_DEVICE_REGISTER:
	    {
	        ret = req_device_register( i_p_data );
	        break;
	    }
	    case PAYLOAD_DARK_IMAGE:
	    {
	        ret = req_dark_image( i_p_data );
	        break;
	    }
	    case PAYLOAD_PREVIEW_ENABLE:
	    {
	        ret = req_preview_enable( i_p_data );
	        break;
	    }
	    case PAYLOAD_RESEND_IMAGE:
	    {
	        ret = req_resend_image( i_p_data );
	        break;
	    }
	    case PAYLOAD_IMAGE_TRANS_DONE:
	    {
	        ret = req_image_trans_done( i_p_data );
	        break;
	    }
	    case PAYLOAD_BACKUP_IMAGE_DELETE:
	    {
	        ret = req_backup_image_delete( i_p_data );
	        break;
	    }
	    case PAYLOAD_SELF_DIAGNOSIS:
	    {
	        ret = req_self_diagnosis( i_p_data );
	        break;
	    }
	    case PAYLOAD_ACCELERATION_CONTROL:
	    {
	        ret = req_acceleration_control( i_p_data );
	        break;
	    }
	    
	    case PAYLOAD_WEB_CONFIGURATION:
	    {
	        ret = req_web_configuration( i_p_data );
	        break;
	    }
	    case PAYLOAD_PACKET_TRIGGER:
	    {
	    	ret = req_packet_trigger( i_p_data );
	    	break;
	    }
	    case PAYLOAD_IMAGE_RETRANSMISSION:
	    {
	    	ret = req_image_retransmission( i_p_data );
	    	break;
	    }
	    case PAYLOAD_SENSITIVITY_CALIBRATION:
	    {
	    	ret = req_sensitivity_calibration( i_p_data );
	    	break;
	    }
	    case PAYLOAD_GAIN_COMPENSATION_CONTROL:
	    {
	    	ret = req_gain_compensation_control( i_p_data );
	    	break;
	    }
	    case PAYLOAD_AP_INFORMATION:
	    {
	        ret = req_ap_information( i_p_data );
	        break;
	    }
	    
	    case PAYLOAD_CALIBRATION:
	    {
	        ret = reqCalibration( i_p_data );
	        break;
	    }
	    case PAYLOAD_OFFSET_DRIFT:
	    {
	        ret = req_offset_drift( i_p_data );
	        break;
	    }
	    case PAYLOAD_OFFSET_CONTROL:
	    {
	        ret = req_offset_control( i_p_data );
	        break;
	    }
	    case PAYLOAD_CAL_DATA_CONTROL:
	    {
	        ret = req_cal_data_control( i_p_data );
	        break;
	    }
	    	
	    case PAYLOAD_NFC_CARD_CONTROL:
	    {
	        ret = req_nfc_card_control( i_p_data );
	        break;
	    }
		case PAYLOAD_SMART_W_CONTROL:
		{
			ret = req_smart_w_control( i_p_data );
			break;
		}
		case PAYLOAD_AED_OFF_DEBOUNCE_CONTROL:
		{
			ret = req_aed_off_debounce_control( i_p_data );
			break;
		}	        
        case PAYLOAD_BATTERY_CHARGING_MODE:
		{
			ret = req_battery_charging_mode( i_p_data );
			break;
		}	        
	    case PAYLOAD_TRANSMISSION_TEST:
	    {
	        ret = req_trans_test( i_p_data );
	        break;
	    }
	    case PAYLOAD_AP_CHANNEL_CHANGE:
	    {
	        ret = req_ap_channel_change( i_p_data );
	        break;
	    }
	    case PAYLOAD_AP_CHANNEL_CHANGE_RESULT:
	    {
	        ret = req_ap_channel_change_result( i_p_data );
	        break;
	    }

	    case SCINTILLATOR_TYPE_RANGE_ADVERTISEMENT:
	    {
	        ret = req_scintillator_type_range( i_p_data );
	        break;
        }
        case PAYLOAD_OFFSET_CORRECTION_CTRL:
        {
	        ret = req_offset_correction_ctrl( i_p_data );
	        break; 
        }
        case PAYLOAD_EXPOSURE_CONTROL:
        {
	        ret = reqExposureControl( i_p_data );
	        break;
        }

	    case PAYLOAD_TEST_DATA:
	    {
	        ret = true;
	        break;
	    }
	    default:
	    {
	        print_log(ERROR, 1, "[Control Server] invalid payload(0x%04X, 0x%08X, 0x%02X, 0x%02X, %c%c%c%c)\n", \
	                    p_hdr_t->w_payload, p_hdr_t->n_seq_id, p_hdr_t->c_operation, p_hdr_t->c_result, \
	                    p_hdr_t->p_head[0], p_hdr_t->p_head[1], p_hdr_t->p_head[2], p_hdr_t->p_head[3] );
	        //b_reset_alive = false;
	        return false;
	    }
	}
    
    alive_reset();
	return ret;
}

/******************************************************************************/
/**
* \brief        ending client
* \param        none
* \return       none
* \note
*******************************************************************************/
void CControlServer::client_notify_close( s32 i_n_fd )
{
    print_log(DEBUG, 1, "[Control Server] Disconnected(fd: %d)\n", i_n_fd);
    
    m_b_client_valid    = false;
    
    m_n_client_seq_id   = 0;
    m_n_client_fd       = -1;
    
    if( m_p_calibration_c->offset_cal_is_started() == true )
    {
        m_b_offet_cal_stop = true;
        print_log(DEBUG, 1, "[Control Server] offset cal stop by socket close\n");
    }

    m_p_system_c->set_sw_connection( false );
    
    // login request로 이동: trans_done side effct 방지
    //disable_function_list();
    
    m_b_stream_sending_error = false;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CControlServer::disable_function_list(void)
{
	// req_function_list에 의해 enable된 기능을 접속이 종료되면 disable 한다.
	// SDK를 혼용하여 사용하는 경우 버전에 따라 기능 동작이 다르므로
	// S/W 접속이 종료되면 이를 초기화하여 오동작을 막는다.
	m_p_system_c->trans_done_enable( false );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CControlServer::client_notify_recv_error( s32 i_n_fd )
{
    print_log(DEBUG, 1, "[Control Server] recv error(fd: %d)\n", i_n_fd);
    message_send(eMSG_DISCONNECTION);
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::packet_send( u32 i_n_payload, u32 i_n_seq_id, \
                                    u8* i_p_buff, u32 i_n_length, \
                                    u8 i_c_result )
{
    u8                  p_send_buf[PACKET_SIZE];
    packet_header_t*    p_hdr_t;
    bool                b_result;
    u32                 n_seq_id = m_w_packet_seq + i_n_seq_id;
    
    if( m_b_client_valid == false )
    {
        print_log(DEBUG, 1, "[Control Server] client is invalid\n" );
        return false;
    }
    
    memset( p_send_buf, 0, sizeof(p_send_buf) );
    p_hdr_t = (packet_header_t *)p_send_buf;
    
    memcpy( p_hdr_t->p_head, PACKET_HEAD, sizeof(p_hdr_t->p_head) );
    p_hdr_t->n_seq_id   = htonl( n_seq_id );
    p_hdr_t->w_payload  = htons( i_n_payload );
    p_hdr_t->c_result   = i_c_result;
    
    
    if( i_n_length > 0 )
    {
        memcpy( &p_send_buf[sizeof(packet_header_t)], i_p_buff, i_n_length );
    }
    
    lock_send();
    b_result = sender( m_n_client_fd, p_send_buf, PACKET_SIZE );
    unlock_send();
    
    print_log(DEBUG, m_b_debug, "[Control Server] seq: 0x%08X(0x%04X)\n", n_seq_id, i_n_payload);
    
    return b_result;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CControlServer::test_packet_send( u32 i_n_size )
{
    u8* p_buff = (u8*)malloc(i_n_size);
    
    if( p_buff != NULL )
    {
        print_log(DEBUG, 1, "[Control Server] test size: %d\n", i_n_size);
        
        memset( p_buff, 0, i_n_size );
        packet_send( 0xFFFF, 0, p_buff, i_n_size );
        
        safe_free( p_buff );
    }
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::req_login( u8* i_p_data )
{
    packet_header_t*    p_hdr_t;
    login_t             login_cfg_t;
    s8					p_sdk_version[32] = {0,};
    
    p_hdr_t = (packet_header_t*)i_p_data;
    
    memcpy( &login_cfg_t, &i_p_data[sizeof(packet_header_t)], sizeof(login_t) );
    
    login_cfg_t.n_time          = ntohl(login_cfg_t.n_time);
    login_cfg_t.w_interval_time = ntohs(login_cfg_t.w_interval_time);
    login_cfg_t.w_setup         = ntohs(login_cfg_t.w_setup);
    
    if( client_decode( &login_cfg_t ) == false )
    {
        return false;
    }
    
    memcpy(p_sdk_version, login_cfg_t.p_sw_version, 32 );
    
    if( m_p_system_c->_check_compatibility_sdk_version(p_sdk_version) == false )
    {
    	print_log(DEBUG, 1, "[Control Server] Not supported S/W version: %s\n", login_cfg_t.p_sw_version );
    	return false;
    }
    
    disable_function_list();
    
    m_n_client_seq_id       = p_hdr_t->n_seq_id;
    m_n_alive_interval_time = login_cfg_t.w_interval_time;
    m_b_client_valid        = true;
    
    // stop auto offset calibration
    if( m_p_calibration_c->offset_cal_is_started() == true )
    {
        m_b_offet_cal_stop      = true;
        print_log(DEBUG, 1, "[Control Server] offset cal stop by login\n" );
    }
    else
    {
    	m_b_offet_cal_stop	= false;
    }
    
    // system time syncronize
	s8          p_date[256];
	
	memset( p_date, 0, sizeof(p_date) );
	get_time_string( login_cfg_t.n_time, p_date );
	
	print_log(DEBUG, 1, "[Control Server] date: %s\n", p_date );
	sys_command("date --set %s", p_date);
	sys_command( "hwclock --systohc" );
    
    m_p_system_c->set_sw_connection( true );
    
    print_log( DEBUG, 1, "[Control Server] S/W Version: %s\n", login_cfg_t.p_sw_version );
    print_log( DEBUG, 1, "[Control Server] F/W version: %s\n", m_p_system_c->get_device_version() );
    
    return req_login_ack();
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::req_login_ack(void)
{
    manufacture_info_t  info_t;
    
    login_ack_t         ack_t;
    memset( &ack_t, 0, sizeof(login_ack_t) );

    sys_state_t state_t;
    memset( &state_t, 0, sizeof(sys_state_t) );
    m_p_system_c->get_state( &state_t );
    
    image_size_t image_t;
    memset( &image_t, 0, sizeof(image_t) );
    m_p_system_c->get_image_size_info( &image_t );
    
    ack_t.w_width               = htons( (u16)image_t.width );
    ack_t.w_height              = htons( (u16)image_t.height );
    ack_t.w_pixel_depth         = htons( (u16)image_t.pixel_depth );
    ack_t.w_backup_image_count  = htons( (u16)m_p_system_c->image_get_saved_count() );
    ack_t.w_offset_state        = htons( (u16)m_p_system_c->get_offset_state() );
    
    ack_t.w_hw_version          = htons( m_p_system_c->hw_get_ctrl_version() );
    ack_t.w_board_config        = htons( m_p_system_c->hw_get_sig_version() );

    ack_t.w_power_mode          = htons( state_t.w_power_mode );
    
    m_p_system_c->manufacture_info_get(&info_t);
    
    // model name
    strcpy( ack_t.p_model_name, info_t.p_model_name );
    
    // serial number
    strcpy( ack_t.p_serial_number, info_t.p_serial_number );
    
    // package version
    strcpy( ack_t.p_package_info, m_p_system_c->config_get(eCFG_VERSION_PACKAGE) );
    
    print_log(DEBUG, 1, "[Control Server] model name: %s, serial: %s\n", info_t.p_model_name, info_t.p_serial_number );
    print_log(DEBUG, 1, "[Control Server] package version: %s\n", ack_t.p_package_info );
    
    // company code
    ack_t.n_company_code = htonl( info_t.n_company_code );
    
    // mac address
    sprintf( ack_t.p_mac_address, "%02X:%02X:%02X:%02X:%02X:%02X", \
                            info_t.p_mac_address[0], info_t.p_mac_address[1], \
                            info_t.p_mac_address[2], info_t.p_mac_address[3], \
                            info_t.p_mac_address[4], info_t.p_mac_address[5] );
    
    // bootlaoder version
    strncpy( ack_t.p_bootloader_version, m_p_system_c->config_get(eCFG_VERSION_BOOTLOADER), 31 );
    
    // kernel version
    strncpy( ack_t.p_kernel_version, m_p_system_c->config_get(eCFG_VERSION_KERNEL), 31 );
    
    // ramdisk(root file system) version
    strncpy( ack_t.p_rfs_version, m_p_system_c->config_get(eCFG_VERSION_RFS), 31 );
    
    // f/w version
    strncpy( ack_t.p_fw_version, m_p_system_c->config_get(eCFG_VERSION_FW), 31 );
    
    // fpga version
    strncpy( ack_t.p_fpga_version, m_p_system_c->config_get(eCFG_VERSION_FPGA), 31 );
    
    // power control micom version
    if(m_p_system_c->is_vw_revision_board())
        sprintf( ack_t.p_micom_version, m_p_system_c->config_get(eCFG_VERSION_MICOM), 31);
    else
        sprintf( ack_t.p_micom_version, "N/A");

    // AED micom version
    sprintf( ack_t.p_aed_version, "N/A");

    // effective area
    ack_t.p_effective_area[eEFFECTIVE_AREA_TOP]     = htons( info_t.p_effective_area[eEFFECTIVE_AREA_TOP] );
    ack_t.p_effective_area[eEFFECTIVE_AREA_LEFT]    = htons( info_t.p_effective_area[eEFFECTIVE_AREA_LEFT] );
    ack_t.p_effective_area[eEFFECTIVE_AREA_RIGHT]   = htons( info_t.p_effective_area[eEFFECTIVE_AREA_RIGHT] );
    ack_t.p_effective_area[eEFFECTIVE_AREA_BOTTOM]  = htons( info_t.p_effective_area[eEFFECTIVE_AREA_BOTTOM] );
    
    u16data_u u16_u;
    u16 w_data = 0;

    ack_t.c_aed_count = m_p_system_c->get_aed_count();
    
     //20210903. youngjun han
     //사양: SDK에서 gain type --> IFS 변환 & ADI ROIC
     //      non-zero: Detector에서 gain type --> IFS 변환 & TI ROIC
     //1로 고정. VW에서는 ADI ROIC 여도 GAIN_TYPE_INDEX로 값을 전달받아야하기 때문. 실제 ROIC TYPE은 ROIC_TYPE_VER_1 사용 
    ack_t.w_roic_type_ver_0 = htons(eROIC_TYPE_TI_2256GR);            
    
    u16_u.BIT.SUPPORT = 1;
    u16_u.BIT.DATA = m_p_system_c->get_battery_slot_count();
    ack_t.w_bat_count = htons(u16_u.U16DATA);

	w_data = eCONFIG_PRESET;
    ack_t.w_config_file_type = htons(w_data);

	w_data = eSTATE_VW;
    ack_t.w_state_type = htons(w_data);  	 
   
   /* 0: Unknown(Default 16bit format)
      1: 16bit
      2: 32bit */
   	ack_t.c_exp_time_data_format = 2;
   
	// VW모델은 gain type 0~4까지 5개만 지원
    ack_t.c_gain_type_num = 5;

    //
    ack_t.c_roic_type_identification_ver = 0x01;
    ack_t.c_roic_type_ver_1 = m_p_system_c->get_roic_model_type();


    return packet_send( PAYLOAD_LOG_IN, m_n_client_seq_id, (u8*)&ack_t, sizeof(ack_t) );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::req_config_reset( u8* i_p_data )
{
    bool b_result = packet_send( PAYLOAD_WIRELESS_CONFIG_RESET, m_n_client_seq_id );
    
    m_p_net_manager_c->net_state_proc_stop();
    
    m_p_system_c->config_reset();
    
    m_p_net_manager_c->net_state_proc_start();
    
    return b_result;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::req_device_register( u8* i_p_data )
{
    packet_header_t*    p_hdr_t;
    OPERATION           oper_e;
    u8                  c_result = 0;
    
    u16                 w_device_id;
    u16                 w_count;
    u16                 i;
    u16                 w_data_pos;
    
    p_hdr_t = (packet_header_t*)i_p_data;
    
    oper_e    = (OPERATION)p_hdr_t->c_operation;
    
    memcpy( &w_device_id, &i_p_data[sizeof(packet_header_t)],       2 );
    memcpy( &w_count,     &i_p_data[sizeof(packet_header_t) + 2],   2 );
    
    w_device_id = ntohs( w_device_id );
    w_count     = ntohs( w_count );
    
    if( oper_e == eOPER_READ
        || oper_e == eOPER_READ_SAVED )
    {
        u8          p_ack_buf[PACKET_DATA_SIZE];
        dev_reg_t   reg_t;
        
        memset( p_ack_buf, 0, sizeof(p_ack_buf) );
        
        // copy device id, count
        memcpy( &p_ack_buf[0], &i_p_data[sizeof(packet_header_t)], 4 );
        
        w_data_pos = 4;
        
        for( i = 0; i < w_count; i++ )
        {
            memcpy( &reg_t, &i_p_data[sizeof(packet_header_t) + w_data_pos], sizeof(dev_reg_t) );
            
            reg_t.w_addr    = ntohs(reg_t.w_addr);
            
            if( w_device_id == 0 )
            {
                if( oper_e == eOPER_READ )
                {
                    reg_t.w_data = m_p_system_c->dev_reg_read(eDEV_ID_FPGA, (u32)reg_t.w_addr);
                }
                else
                {
                    reg_t.w_data = m_p_system_c->dev_reg_read_saved(eDEV_ID_FPGA, (u32)reg_t.w_addr);
                }
            }
            else if( w_device_id > 1
            		&& w_device_id <= 5 )
            {
            	if( oper_e == eOPER_READ )
                {
                    reg_t.w_data = m_p_system_c->dev_reg_read((DEV_ID)w_device_id, (u32)reg_t.w_addr);
                }
            }
            
            //printf("addr: 0x%04X, data: 0x%04X\n", reg_t.w_addr, reg_t.w_data);
            
            reg_t.w_addr    = htons(reg_t.w_addr);
            reg_t.w_data    = htons(reg_t.w_data);
            
            memcpy( &p_ack_buf[w_data_pos], &reg_t, sizeof(dev_reg_t) );
            w_data_pos += sizeof(dev_reg_t);
        }
        
        if( oper_e == eOPER_READ )
        {
            if( m_p_system_c->pm_is_normal_mode() == false )
            {
                c_result = 1;
            }
        }
        
        return packet_send( PAYLOAD_DEVICE_REGISTER, m_n_client_seq_id, \
                                p_ack_buf, w_data_pos, c_result );
    }
    
    if( oper_e == eOPER_SAVE )
    {
        dev_reg_addr_t      reg_addr_t;
        
        // skip device id and count
        w_data_pos      = 4;
        
        for( i = 0; i < w_count; i++ )
        {
            memcpy( &reg_addr_t, &i_p_data[sizeof(packet_header_t) + w_data_pos], sizeof(dev_reg_addr_t) );
            
            reg_addr_t.w_start   = ntohs(reg_addr_t.w_start);
            reg_addr_t.w_end     = ntohs(reg_addr_t.w_end);
            
            if( w_device_id == 0 )
            {
                m_p_system_c->dev_reg_save(eDEV_ID_FPGA, \
                                    reg_addr_t.w_start, reg_addr_t.w_end);
            }
            
            w_data_pos += sizeof(dev_reg_addr_t);
        }
        
        if( w_device_id == 0 )
        {
            m_p_system_c->dev_reg_save_file(eDEV_ID_FPGA);
        }
        
        if( m_p_system_c->pm_is_normal_mode() == false )
        {
            c_result = 1;
        }
        
        return packet_send( PAYLOAD_DEVICE_REGISTER, m_n_client_seq_id, NULL, 0, c_result );
    }
    
    if( oper_e == eOPER_WRITE )
    {
        dev_reg_t   reg_t;
        
        // skip device id and count
        w_data_pos      = 4;
        
        for( i = 0; i < w_count; i++ )
        {
            memcpy( &reg_t, &i_p_data[sizeof(packet_header_t) + w_data_pos], sizeof(dev_reg_t) );
            
            reg_t.w_addr    = ntohs(reg_t.w_addr);
            reg_t.w_data    = ntohs(reg_t.w_data);
            
            m_p_system_c->dev_reg_write((DEV_ID)w_device_id, reg_t.w_addr, reg_t.w_data);
            
            w_data_pos += sizeof(dev_reg_t);
        }

        if( m_p_system_c->pm_is_normal_mode() == false && (DEV_ID)w_device_id != eDEV_ID_FIRMWARE )
        {
            c_result = 1;
        }
        
        return packet_send( PAYLOAD_DEVICE_REGISTER, m_n_client_seq_id, NULL, 0, c_result );
    }
    
    print_log(ERROR, 1, "[Control Server] Unknown dev register operation: %d\n", oper_e );
    
    return false;    
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::req_reboot( u8* i_p_data )
{
	packet_header_t*    p_hdr_t;
    u8                  c_result = 1;
    
    p_hdr_t = (packet_header_t*)i_p_data;
    
    if( p_hdr_t->c_operation == 0 )
    {
    	packet_send( PAYLOAD_REBOOT, m_n_client_seq_id );
        m_p_system_c->hw_reboot();
        
        return true;
	}
	
	if( p_hdr_t->c_operation == 1 )
	{
		packet_send( PAYLOAD_REBOOT, m_n_client_seq_id );
		m_p_system_c->hw_power_off(true);
		
		return true;
	}
    
    return packet_send( PAYLOAD_REBOOT, m_n_client_seq_id, NULL, 0, c_result );;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::req_sleep_control( u8* i_p_data )
{
    packet_header_t*    p_hdr_t;
    
    p_hdr_t = (packet_header_t*)i_p_data;
    
    if( p_hdr_t->c_operation == 0 )
    {
    	u32 n_remain_sec_time =  m_p_system_c->get_wakeup_remain_time();
    	print_log(DEBUG, 1, "[Control Server] Expected wake up remaining time: %d\n", n_remain_sec_time );
    	
    	n_remain_sec_time = htonl(n_remain_sec_time);
        m_p_system_c->pm_wake_up(true);     // wake up by S/W
        return packet_send( PAYLOAD_SLEEP_CONTROL, m_n_client_seq_id, (u8*)&n_remain_sec_time, sizeof(n_remain_sec_time) );
    }
    else if( p_hdr_t->c_operation == 1 )
    {
        m_p_system_c->pm_set_keep_normal( false );
    }
    else if( p_hdr_t->c_operation == 2 )
    {
        m_p_system_c->pm_set_keep_normal( true );
    }
    else if( p_hdr_t->c_operation == 3 )
    {
    	u8 c_result = 0;
        if( m_p_system_c->pm_is_normal_mode() == false )
        {
        	c_result = 1;
        }
        return packet_send( PAYLOAD_SLEEP_CONTROL, m_n_client_seq_id, NULL, 0, c_result );
    }
    else if( p_hdr_t->c_operation == 4 )
    {
    	u32 n_remain_sec_time = m_p_system_c->pm_get_remain_sleep_time();
        
        n_remain_sec_time = htonl(n_remain_sec_time);
        
        return packet_send( PAYLOAD_SLEEP_CONTROL, m_n_client_seq_id, (u8*)&n_remain_sec_time, sizeof(n_remain_sec_time) );
    }
    else if( p_hdr_t->c_operation == 5 )
    {
    	u16 w_go_sleep_error = 0;
    	w_go_sleep_error = m_p_system_c->pm_go_sleep();
    	w_go_sleep_error = htons(w_go_sleep_error);
    	return packet_send( PAYLOAD_SLEEP_CONTROL, m_n_client_seq_id, (u8*)&w_go_sleep_error, sizeof(w_go_sleep_error) );
    }
    
    return packet_send( PAYLOAD_SLEEP_CONTROL, m_n_client_seq_id );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CControlServer::parse_mac( s8* i_p_mac, u8* o_p_mac )
{
	s8*         p_tok;
	const s8*   p_delim = ":";
	u16         i = 0;
	
	for (p_tok = strtok((s8 *)i_p_mac, p_delim); p_tok; p_tok = strtok(NULL, p_delim)) 
	{
		o_p_mac[i++] = strtoul( p_tok, NULL, 16 );
	}
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::req_manufacture_info( u8* i_p_data )
{
    packet_header_t*    p_hdr_t;
    OPERATION           oper_e;
    manufacture_info_t  info_t;
    manufacture_req_t   req_t;
    bool                b_ret = false;

    p_hdr_t = (packet_header_t*)i_p_data;
    oper_e = (OPERATION)p_hdr_t->c_operation;
    
    if( oper_e == eOPER_WRITE )
    {
        // write
        memset( &info_t, 0, sizeof(manufacture_info_t) );
        m_p_system_c->manufacture_info_get(&info_t);
        
        memset( &req_t, 0, sizeof(manufacture_req_t) );
        memcpy( &req_t, &i_p_data[sizeof(packet_header_t)], sizeof(manufacture_req_t) );
        
        // Parse data //
        strcpy( info_t.p_model_name,    req_t.p_model_name );
        strcpy( info_t.p_serial_number, req_t.p_serial_number );
        
        info_t.n_company_code = ntohl(req_t.n_company_code);
        info_t.n_release_date = time(NULL);
        
        parse_mac( req_t.p_mac_address, info_t.p_mac_address );
        
        print_log(DEBUG, 1, "[Control Server] mac address: %02X:%02X:%02X:%02X:%02X:%02X\n", \
        	                info_t.p_mac_address[0], info_t.p_mac_address[1], \
        	                info_t.p_mac_address[2], info_t.p_mac_address[3], \
        	                info_t.p_mac_address[4], info_t.p_mac_address[5] );
        // effective area
        info_t.p_effective_area[eEFFECTIVE_AREA_TOP]     = ntohs( req_t.p_effective_area[eEFFECTIVE_AREA_TOP] );
        info_t.p_effective_area[eEFFECTIVE_AREA_LEFT]    = ntohs( req_t.p_effective_area[eEFFECTIVE_AREA_LEFT] );
        info_t.p_effective_area[eEFFECTIVE_AREA_RIGHT]   = ntohs( req_t.p_effective_area[eEFFECTIVE_AREA_RIGHT] );
        info_t.p_effective_area[eEFFECTIVE_AREA_BOTTOM]  = ntohs( req_t.p_effective_area[eEFFECTIVE_AREA_BOTTOM] );
        
	    print_log( DEBUG, 1, "[Control Server] write effective area top: %d, bottom: %d, left: %d, right: %d\n", \
	                info_t.p_effective_area[eEFFECTIVE_AREA_TOP], \
	                info_t.p_effective_area[eEFFECTIVE_AREA_BOTTOM], \
	                info_t.p_effective_area[eEFFECTIVE_AREA_LEFT], \
	                info_t.p_effective_area[eEFFECTIVE_AREA_RIGHT] );

	    bool b_need_reboot = false;
	    
	   	if( info_t.n_scintillator_type != ntohl(req_t.n_scintillator_type) )
		{
			info_t.n_scintillator_type = ntohl(req_t.n_scintillator_type);
			b_need_reboot = true;
		}
		
	    if( info_t.w_battery_count_to_install != ntohs(req_t.w_battery_count_to_install) )
		{
			info_t.w_battery_count_to_install = ntohs(req_t.w_battery_count_to_install);
			b_need_reboot = true;
		}
		
        //Manufacturer Information Validation Check //
        //If not valid, do not save and send set fail error
        u32 result = m_p_system_c->verify_manufacture_info(&info_t); 
        if(result != 0)
        {
            print_log(DEBUG, 1, "[Control Server] invalid manufacture info. write request fail.\n");
            b_ret = packet_send( PAYLOAD_MANUFACTURE_INFO, m_n_client_seq_id, NULL, 0, result);
            b_need_reboot = false;
        }
        else    //Write manufactuer info data to EEPROM & reboot system if necessary //
        {
            m_p_system_c->manufacture_info_set(&info_t);
            b_ret = packet_send( PAYLOAD_MANUFACTURE_INFO, m_n_client_seq_id );
        }
        
        if(b_need_reboot)
        {
        	m_p_system_c->hw_reboot();
        }
    }
    else
    {
        memset( &info_t, 0, sizeof(manufacture_info_t) );
        m_p_system_c->manufacture_info_get(&info_t);
        
        memset( &req_t, 0, sizeof(manufacture_req_t) );
        
        strcpy( req_t.p_model_name,     info_t.p_model_name );
        strcpy( req_t.p_serial_number,  info_t.p_serial_number );
        
        req_t.n_company_code = htonl(info_t.n_company_code);
        req_t.n_release_date = htonl(info_t.n_release_date);
        
        sprintf( req_t.p_mac_address, "%02X:%02X:%02X:%02X:%02X:%02X", \
                                info_t.p_mac_address[0], info_t.p_mac_address[1], \
                                info_t.p_mac_address[2], info_t.p_mac_address[3], \
                                info_t.p_mac_address[4], info_t.p_mac_address[5] );
        
        // effective area
        req_t.p_effective_area[eEFFECTIVE_AREA_TOP]     = htons( info_t.p_effective_area[eEFFECTIVE_AREA_TOP] );
        req_t.p_effective_area[eEFFECTIVE_AREA_LEFT]    = htons( info_t.p_effective_area[eEFFECTIVE_AREA_LEFT] );
        req_t.p_effective_area[eEFFECTIVE_AREA_RIGHT]   = htons( info_t.p_effective_area[eEFFECTIVE_AREA_RIGHT] );
        req_t.p_effective_area[eEFFECTIVE_AREA_BOTTOM]  = htons( info_t.p_effective_area[eEFFECTIVE_AREA_BOTTOM] );
        
	    print_log( DEBUG, 1, "[Control Server] read effective area top: %d, bottom: %d, left: %d, right: %d\n", \
	                info_t.p_effective_area[eEFFECTIVE_AREA_TOP], \
	                info_t.p_effective_area[eEFFECTIVE_AREA_BOTTOM], \
	                info_t.p_effective_area[eEFFECTIVE_AREA_LEFT], \
	                info_t.p_effective_area[eEFFECTIVE_AREA_RIGHT] );
        
        req_t.n_scintillator_type = htonl(info_t.n_scintillator_type);
        req_t.w_battery_count_to_install = htons(info_t.w_battery_count_to_install);
        
        b_ret = packet_send( PAYLOAD_MANUFACTURE_INFO, m_n_client_seq_id, \
                                (u8*)&req_t, sizeof(manufacture_req_t) );
    }
    return b_ret;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::req_package_version( u8* i_p_data )
{
    packet_header_t*    p_hdr_t;
    OPERATION           oper_e;
    s8                  p_package_version[32];
    
    p_hdr_t = (packet_header_t*)i_p_data;    
    oper_e = (OPERATION)p_hdr_t->c_operation;
    
    if( oper_e == eOPER_READ )
    {
        strcpy( p_package_version, m_p_system_c->config_get(eCFG_VERSION_PACKAGE) );
        
        return packet_send( PAYLOAD_PACKAGE_VERSION, m_n_client_seq_id, \
                                (u8*)p_package_version, sizeof(p_package_version) );
    }
    else
    {
        // write
        memcpy( &p_package_version[0], &i_p_data[sizeof(packet_header_t)], sizeof(p_package_version) );
        
        m_p_system_c->config_set(eCFG_VERSION_PACKAGE, p_package_version);
        
        return packet_send( PAYLOAD_PACKAGE_VERSION, m_n_client_seq_id );
    }
    
}

/******************************************************************************/
/**
* \brief        
* \param        
* \return       
* \note         S/W -> Detector로 전송
*******************************************************************************/
bool CControlServer::req_upload( u8* i_p_data )
{
    packet_header_t*    p_hdr_t;
    MULTI_PACKET        oper_e;
    u8                  c_result = eMULTI_PACKET_ERR_NO;
    
    u32                 p_info[3];
    u32                 n_type;
    u32                 n_size;
    u32                 n_crc;
    
    p_hdr_t = (packet_header_t*)i_p_data;
    
    oper_e    = (MULTI_PACKET)p_hdr_t->c_operation;
    
    if( oper_e == eMULTI_PACKET_PREPARE )
    {
        memcpy( p_info, &i_p_data[sizeof(packet_header_t)], sizeof(p_info) );
        
        n_type = ntohl( p_info[0] );
        n_size = ntohl( p_info[1] );
        n_crc  = ntohl( p_info[2] );
        
        print_log(DEBUG, 1, "[Control Server] update type: %d, size: %d, crc: 0x%08X\n", \
                                n_type, n_size, n_crc );
        
        m_file_id_e = (VW_FILE_ID)n_type;
        
        if( download_start( m_file_id_e, n_crc, n_size ) == false )
        {
            c_result = (u8)eMULTI_PACKET_ERR_MEM_ALLOC;
        }
        
        return packet_send( PAYLOAD_UPLOAD, m_n_client_seq_id, NULL, 0, c_result );
    }
    
    if( oper_e == eMULTI_PACKET_SEND_DATA )
    {
        memcpy( p_info, &i_p_data[sizeof(packet_header_t)], sizeof(p_info) );
        
        download_set_data( &i_p_data[sizeof(packet_header_t) + sizeof(p_info)],(PACKET_DATA_SIZE - sizeof(p_info)) );
        
        return packet_send( PAYLOAD_UPLOAD, m_n_client_seq_id );
    }
    
    if( oper_e == eMULTI_PACKET_END_DATA )
    {
        // check crc
        if( download_verify(m_file_id_e) == false )
        {
            c_result = (u8)eMULTI_PACKET_ERR_CRC;
        }
        else if( download_compatibility_check(m_file_id_e) == false )
        {
            c_result = (u8)eMULTI_PACKET_ERR_WRITE;     //추후 다른 ENUM으로 변경. SDK 협의 필요
        }
        else if( download_save(m_file_id_e) == false )
        {
            c_result = (u8)eMULTI_PACKET_ERR_WRITE;
        }
        else if( m_p_system_c->update_start( m_file_id_e, m_p_file_name ) == false )
        {
            c_result = (u8)eMULTI_PACKET_ERR_WRITE;
        }
        
        return packet_send( PAYLOAD_UPLOAD, m_n_client_seq_id, NULL, 0, c_result );
    }
    
    if( oper_e == eMULTI_PACKET_UPDATE_PROGRESS )
    {
        u32 n_progress;
        
        n_progress = m_p_system_c->update_get_progress(m_file_id_e);
        
        n_progress = htonl(n_progress);
        
        return packet_send( PAYLOAD_UPLOAD, m_n_client_seq_id, \
                                (u8*)&n_progress, sizeof(n_progress));
    }
    
    if( oper_e == eMULTI_PACKET_UPDATE_RESULT )
    {
        bool b_result;
        
        if( m_p_system_c->update_get_result(m_file_id_e) == true )
        {
            c_result = eMULTI_PACKET_ERR_NO;
        }
        else
        {
            c_result = eMULTI_PACKET_ERR_WRITE;
        }
        
        print_log(DEBUG, 1, "[Control Server] update result: %d\n", c_result );
        
        b_result = packet_send( PAYLOAD_UPLOAD, m_n_client_seq_id, NULL, 0, c_result);
        
        m_p_system_c->update_stop(m_file_id_e);

        download_end();
        
        if( m_file_id_e == eVW_FILE_ID_CONFIG )
        {
        	bool b_changed = m_p_system_c->config_net_is_changed();
        	if( b_changed == true )
        	{
        		m_p_net_manager_c->net_state_proc_stop();
        	}

            m_p_system_c->config_reload();
            m_p_system_c->config_save();
            
        	if( b_changed == true )
        	{
        		m_p_net_manager_c->net_state_proc_start();
        	}
		}
        else if( m_file_id_e == eVW_FILE_ID_PRESET )
		{
			m_p_system_c->preset_load();
		}
		else if( m_file_id_e == eVW_FILE_ID_PATIENT_LIST_XML )
		{
		    if( m_p_web_c != NULL )
		    {
		    	m_p_web_c->update_patient_list();
		    }
		}
        
        return b_result;
    }
    
    print_log(ERROR, 1, "[Control Server] Unknown upload operation: %d\n", oper_e );
    
    return false;
}

/******************************************************************************/
/**
* \brief        
* \param        
* \return       
* \note         Detector -> S/W로 전송
*******************************************************************************/
bool CControlServer::req_download( u8* i_p_data )
{
    packet_header_t*    p_hdr_t;
    u8                  c_result = 0;       // success
    MULTI_PACKET        oper_e;
    
    u32                 p_info[3];
    
    p_hdr_t = (packet_header_t*)i_p_data;
    
    oper_e  = (MULTI_PACKET)p_hdr_t->c_operation;
    memcpy( p_info, &i_p_data[sizeof(packet_header_t)], sizeof(p_info) );
    
    if( oper_e == eMULTI_PACKET_PREPARE )
    {
        u32     n_type;
        u32     n_total_size = 0;
        u32     n_crc = 0;
        
        n_type = ntohl( p_info[0] );
        
        if( (VW_FILE_ID)n_type == eVW_FILE_ID_BACKUP_IMAGE_LIST_WITH_PATIENT_INFO
        	&& m_p_web_c != NULL )
        {
        	m_p_web_c->make_backup_image_list( BACKUP_IMAGE_XML_LIST_FILE );
        }
        
        if( m_p_system_c->file_open( (VW_FILE_ID)n_type, &n_total_size, &n_crc ) == false )
        {
            c_result = 1;       // open failed
        }
        else
        {
            memset( p_info, 0, sizeof(p_info) );
            
            p_info[0] = htonl( n_total_size );
            p_info[1] = htonl( n_crc );
        }
        print_log(DEBUG, 1, "[Control Server] download request start\n" );
        return packet_send( PAYLOAD_DOWNLOAD, m_n_client_seq_id, \
                                (u8*)p_info, sizeof(p_info), c_result);
    }
    
    if( oper_e == eMULTI_PACKET_SEND_DATA )
    {
        u32     n_slice_number;
        u32     n_data_size = PACKET_DATA_SIZE - sizeof(p_info);     // size of slice number
        u8      p_data[PACKET_DATA_SIZE];
        u32     n_pos;
        u32     n_read_size = 0;
        
        memset( p_data, 0, sizeof(p_data) );
        
        n_slice_number = ntohl( p_info[0] );
        
        n_pos = n_slice_number * n_data_size;
        
        memset( p_info, 0, sizeof(p_info) );
        if( m_p_system_c->file_read( n_pos, n_data_size, &p_data[sizeof(p_info)], &n_read_size ) == false )
        {
            c_result = 2;       // read failed
            p_info[0] = htonl( n_slice_number );
        }
        else
        {
            p_info[0] = htonl( n_slice_number );
            p_info[1] = htonl( n_read_size );
        }
        
        memcpy( &p_data[0], p_info, sizeof(p_info) );
        
        if( n_read_size != n_data_size )
        {
            print_log(DEBUG, 1, "[Control Server] last slice: %d(result: %d)\n", n_read_size, c_result );
            m_p_system_c->file_close();
        }
        
        return packet_send( PAYLOAD_DOWNLOAD, m_n_client_seq_id, \
                                p_data, sizeof(p_data), c_result);
    }
    
    if( oper_e == eMULTI_PACKET_END_DATA )
    {
         m_p_system_c->file_close();
         print_log(DEBUG, 1, "[Control Server] download end\n" );
         
         return packet_send( PAYLOAD_DOWNLOAD, m_n_client_seq_id );
    }
    
    print_log(ERROR, 1, "[Control Server] Unknown download operation: %d\n", oper_e );
    
    return false;
}

/******************************************************************************/
/**
* \brief        
* \param        
* \return       
* \note         
*******************************************************************************/
bool CControlServer::req_memory_clear( u8* i_p_data )
{
    packet_header_t*    p_hdr_t;
    VW_FILE_DELETE		file_del_id_e;
    
    p_hdr_t = (packet_header_t*)i_p_data;
    file_del_id_e = (VW_FILE_DELETE)p_hdr_t->c_operation;
    
    if( m_p_calibration_c->is_cal_file(file_del_id_e) == true )
    {
    	m_p_calibration_c->file_delete(file_del_id_e);
    }
    
    m_p_system_c->file_delete((VW_FILE_DELETE)p_hdr_t->c_operation);
    
    return packet_send( PAYLOAD_MEMORY_CLEAR, m_n_client_seq_id );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CControlServer::function_list_set_supported( function_list_ack_t* o_p_ack_t )
{
    o_p_ack_t->c_packet_trigger			= eSUPPORT;
    o_p_ack_t->c_smart_offset_support	= eSUPPORT;
    o_p_ack_t->c_internal_calibration	= eSUPPORT;
    o_p_ack_t->c_web_access_support		= eSUPPORT;
    o_p_ack_t->c_wake_up_time_support	= eSUPPORT;
    o_p_ack_t->c_go_sleep_support		= eSUPPORT;
	o_p_ack_t->c_sw_power_down_enable	= eSUPPORT;
	o_p_ack_t->c_aed_trig_volt_threshold= eSUPPORT;
    
    o_p_ack_t->c_scintillator_type_setting = eSUPPORT;
    o_p_ack_t->c_trigger_disable_support = eSUPPORT;
    o_p_ack_t->c_exp_index_info_support = eSUPPORT;
    o_p_ack_t->c_change_bat_count_support = eSUPPORT;
    o_p_ack_t->c_performance_option_support = eSUPPORT;
    o_p_ack_t->c_ftm_support			= eSUPPORT;
    o_p_ack_t->c_led_blink_control		= eSUPPORT;
    o_p_ack_t->c_wifi_signal_level		= eSUPPORT;

    o_p_ack_t->scintillator_type_range_advertisement_support = eSUPPORT;
    o_p_ack_t->gain_type_conversion_support = eSUPPORT;
    o_p_ack_t->sensitivity_calibration_support = eSUPPORT;                  
    o_p_ack_t->gain_compensation_control_support = eSUPPORT;
    o_p_ack_t->smart_w_support                   = eSUPPORT; //69
    o_p_ack_t->aed_off_debounce_support          = eSUPPORT; //70
    o_p_ack_t->offset_correction_mode_control_support = eSUPPORT;

    /* 0 : Not support
       1: AP 버튼 기능 1(0: None, 1: AP, 2: Station, 3: AP & Station)
       2: AP 버튼 기능 2(0: AP & Station, 1: Preset switching) */
    o_p_ack_t->c_ap_button_mobile_memory_cnt   = 2;		
	
	o_p_ack_t->c_tft_aed_support 	= eNOT_SUPPORT;
    o_p_ack_t->c_nfc_support		= eNOT_SUPPORT;
    o_p_ack_t->c_impact_count		= eNOT_SUPPORT;
    o_p_ack_t->c_exp_time			= eNOT_SUPPORT;
    o_p_ack_t->c_wired_only			= eNOT_SUPPORT;
    o_p_ack_t->c_led_control		= eNOT_SUPPORT;
    o_p_ack_t->c_offset_cal_skip	= eNOT_SUPPORT;
    o_p_ack_t->c_multi_frame		= eNOT_SUPPORT;
    o_p_ack_t->c_wpt_support		= eNOT_SUPPORT;
    o_p_ack_t->c_oper_info_support		= eNOT_SUPPORT;

    o_p_ack_t->c_nfc_onoff_support		= eNOT_SUPPORT;
    o_p_ack_t->c_vxtd_3543d_dynamic_support		= eNOT_SUPPORT;
    o_p_ack_t->c_fxrs_04a_uart_string_command_support		= eNOT_SUPPORT;
    o_p_ack_t->c_calibration_data_user_save_flag_support		= eNOT_SUPPORT;
    o_p_ack_t->c_wpt_aed_noise_identification_support		= eNOT_SUPPORT;
    o_p_ack_t->detector_overlap_support		= eNOT_SUPPORT;
    o_p_ack_t->skip_frame_count_support		= eNOT_SUPPORT;
    o_p_ack_t->wired_only_lock_support		= eNOT_SUPPORT;
    o_p_ack_t->modify_default_config_support		= eNOT_SUPPORT;   
    o_p_ack_t->mobile_memory_control_support        = eNOT_SUPPORT;   
    o_p_ack_t->promote_user_data_support            = eNOT_SUPPORT;   
    o_p_ack_t->circuit_breaker_support              = eNOT_SUPPORT;   

    //회로 구성에 따라, support/not support가 달라지는 기능
    if(m_p_system_c->hw_gyro_probe())
    {
        o_p_ack_t->c_acc_cal_6plane_support = eNOT_SUPPORT;
        o_p_ack_t->c_acc_gyro_combo_support	= eSUPPORT;   
    }
    else
    {
        o_p_ack_t->c_acc_cal_6plane_support = eSUPPORT;
        o_p_ack_t->c_acc_gyro_combo_support	= eNOT_SUPPORT;           
    }

    /* 0 : Unknown */
    o_p_ack_t->wireless_support = eUNKNOWN_OR_NOT_SUPPORT; 
    o_p_ack_t->c_battery_recharing_ctrl_mode = eSUPPORT; 

    if(m_p_system_c->is_vw_revision_board())
    {
        o_p_ack_t->c_battery_lifespan_extn_mode = eSUPPORT; 
        o_p_ack_t->c_battery_charge_complete = eSUPPORT; 
        o_p_ack_t->c_impact_log_v2 = eSUPPORT; 
    }

    if (m_p_system_c->IsReadoutSyncDefined())
    {
        print_log(DEBUG, 1, "[Control Server] readoutSync enable\n");
        o_p_ack_t->ReadOutSync = eSUPPORT; 
    }
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::req_function_list( u8* i_p_data )
{
    function_list_t     list_t;
    function_list_ack_t list_ack_t;
    
    memset( &list_ack_t, 0, sizeof(function_list_ack_t) );
    
    memcpy( &list_t, &i_p_data[sizeof(packet_header_t)], sizeof(function_list_t) );
    
    function_list_set_supported( &list_ack_t );

    if( list_t.c_transmission_done_enable == eENABLE )
    {
        m_p_system_c->trans_done_enable( true );
        
        list_ack_t.c_transmission_done_enable = eSUPPORT;
    }

    if( list_t.c_image_retransmission == eENABLE )
    {
    	m_p_img_retransfer_c->initialize( true, m_p_system_c->get_image_size_byte() );
    	list_ack_t.c_image_retransmission = eSUPPORT;
    }
    else
    {
    	m_p_img_retransfer_c->initialize( false, m_p_system_c->get_image_size_byte() );
    	list_ack_t.c_image_retransmission = 0;
    }
    
    //20210913. Youngjun Han
    //SDK에서는 login ack 데이터로 디텍터가 해당 기능을 지원하는지 안하는지 판단함. 따라서, login req 데이터 값에의해 디텍터가 다른 값을 return해서는 안됨
    m_p_system_c->gate_line_defect_enable(false);
    list_ack_t.c_gate_line_defect_support = eUNKNOWN_OR_NOT_SUPPORT;
    /*
    if( list_t.c_gate_line_defect_enable == eENABLE )
    {
    	m_p_system_c->gate_line_defect_enable(true);
    	list_ack_t.c_gate_line_defect_support = eSUPPORT;
    }
    else
    {
    	m_p_system_c->gate_line_defect_enable(false);
    	list_ack_t.c_gate_line_defect_support = 0;
    }    	
    */	
	    
    print_log(DEBUG, 1, "[Control Server] transmission done enable: %d\n", list_ack_t.c_transmission_done_enable);
    print_log(DEBUG, 1, "[Control Server] packet trigger enable: %d\n", list_ack_t.c_packet_trigger);
    print_log(DEBUG, 1, "[Control Server] smart offset enable: %d\n", list_ack_t.c_smart_offset_support);
    print_log(DEBUG, 1, "[Control Server] internal calibration enable: %d\n", list_ack_t.c_internal_calibration);

    return packet_send( PAYLOAD_FUNCTION_LIST, m_n_client_seq_id, (u8*)&list_ack_t, sizeof(function_list_ack_t) );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CControlServer::download_start( VW_FILE_ID i_id_e, u32 i_n_crc, u32 i_n_total_size )
{
    safe_delete( m_p_download_c );
    sys_command("rm /tmp/*.down");
    
    switch((u8)i_id_e)
    {
        case eVW_FILE_ID_CONFIG:
        {
            strcpy( m_p_file_name, "/tmp/config.down" );
            break;
        }

        case eVW_FILE_ID_PRESET:
        {
            strcpy( m_p_file_name, "/tmp/preset.down" );
            break;
        }
        
        case eVW_FILE_ID_OFFSET:
        {
            strcpy( m_p_file_name, "/tmp/offset.bin" );
            m_n_file_crc = i_n_crc;
            return true;
        }
        case eVW_FILE_ID_DEFECT_MAP:
        {
            strcpy( m_p_file_name, "/tmp/defect.bin" );
            m_n_file_crc = i_n_crc;
            return true;
        }
        case eVW_FILE_ID_GAIN:
        {
            strcpy( m_p_file_name, "/tmp/gain.bin" );
            m_n_file_crc = i_n_crc;
            return true;
        }
        case eVW_FILE_ID_OSF_PROFILE:
        {
            strcpy( m_p_file_name, "/tmp/osf_profile.bin" );
            m_n_file_crc = i_n_crc;
            return true;
        }
        
        case eVW_FILE_ID_BOOTLOADER:
        {
            strcpy( m_p_file_name, "/tmp/boot.down" );
            break;
        }
        case eVW_FILE_ID_KERNEL:
        {
            strcpy( m_p_file_name, "/tmp/kernel.down" );
            break;
        }
        case eVW_FILE_ID_RFS:
        {
            strcpy( m_p_file_name, "/tmp/rfs.down" );
            break;
        }
        case eVW_FILE_ID_APPLICATION:
        {
            strcpy( m_p_file_name, "/tmp/app.down" );
            break;
        }
        case eVW_FILE_ID_FPGA:
        {
            strcpy( m_p_file_name, "/tmp/fpga.down" );
            break;
        }
        case eVW_FILE_ID_PWR_MICOM:
        {
            strcpy( m_p_file_name, "/tmp/micom.down" );
            break;
        }
        case eVW_FILE_ID_DTB:
        {
            strcpy( m_p_file_name, "/tmp/dtb.down" );
            break;
        }
        case eVW_FILE_ID_PATIENT_LIST_XML:
        {
            strcpy( m_p_file_name, "/tmp/patient.down" );
            break;
        }
        case eVW_FILE_ID_STEP_XML:
        {
            strcpy( m_p_file_name, "/tmp/step.down" );
            break;
        }
        case eVW_FILE_ID_OFFSET_BIN2:
        {
            strcpy( m_p_file_name, "/tmp/offset.bin2" );
            m_n_file_crc = i_n_crc;
            return true;
        }
        case eVW_FILE_ID_DEFECT_BIN2:
        {
            strcpy( m_p_file_name, "/tmp/defect.bin2" );
            m_n_file_crc = i_n_crc;
            return true;
        }
        case eVW_FILE_ID_DEFECT_GRID_BIN2:
        {
            strcpy( m_p_file_name, "/tmp/defect_grid.bin2" );
            m_n_file_crc = i_n_crc;
            return true;
        }
        case eVW_FILE_ID_GAIN_BIN2:
        {
            strcpy( m_p_file_name, "/tmp/gain.bin2" );
            m_n_file_crc = i_n_crc;
            return true;
        }
        case eVW_FILE_ID_PREVIEW_GAIN:
        {
            strcpy( m_p_file_name, "/tmp/preview_gain.bin" );
            m_n_file_crc = i_n_crc;
            break;
        }
        case eVW_FILE_ID_PREVIEW_DEFECT:
        {
            strcpy( m_p_file_name, "/tmp/preview_defect.bin" );
            m_n_file_crc = i_n_crc;
            break;
        }
        case eVW_FILE_ID_DEFECT_FOR_GATEOFF_BIN2:
        {
            strcpy( m_p_file_name, "/tmp/defectForGateOff.bin2" );
            m_n_file_crc = i_n_crc;
            break;
        }
        default:
        {
            print_log(ERROR, 1, "[Control Server] Unknown id: %d\n", i_id_e);
            return false;
        }
    }
    
    m_p_download_c = new CVWDownload(m_p_file_name, print_log);
    
    if( m_p_download_c != NULL )
    {
        return m_p_download_c->start(i_n_crc, i_n_total_size);
    }
    
    safe_delete( m_p_download_c );
    
    return false;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CControlServer::download_set_data( u8* i_p_data, u16 i_w_len )
{
    if( m_p_download_c == NULL )
    {
        return false;
    }
    
    return m_p_download_c->set_data( i_p_data, i_w_len );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CControlServer::download_verify( VW_FILE_ID i_id_e )
{
    if( i_id_e == eVW_FILE_ID_OFFSET
        || i_id_e == eVW_FILE_ID_DEFECT_MAP
        || i_id_e == eVW_FILE_ID_GAIN
        || i_id_e == eVW_FILE_ID_OSF_PROFILE
        || i_id_e == eVW_FILE_ID_OFFSET_BIN2
        || i_id_e == eVW_FILE_ID_DEFECT_BIN2
        || i_id_e == eVW_FILE_ID_DEFECT_GRID_BIN2
        || i_id_e == eVW_FILE_ID_DEFECT_FOR_GATEOFF_BIN2
        || i_id_e == eVW_FILE_ID_GAIN_BIN2
        || i_id_e == eVW_FILE_ID_PREVIEW_GAIN
        || i_id_e == eVW_FILE_ID_PREVIEW_DEFECT )
    {
        return download_check_crc();
    }
    
    if( m_p_download_c == NULL )
    {
        return false;
    }
    
    return m_p_download_c->verify();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note     FW 버전 1.0.1.26 이상 기준: Bootloader version이 0.0.4 이하인 패키지는 업데이트 실패처리
*******************************************************************************/
bool CControlServer::download_compatibility_check( VW_FILE_ID i_id_e )
{   
    if( i_id_e == eVW_FILE_ID_OFFSET
        || i_id_e == eVW_FILE_ID_DEFECT_MAP
        || i_id_e == eVW_FILE_ID_GAIN
        || i_id_e == eVW_FILE_ID_OSF_PROFILE
        || i_id_e == eVW_FILE_ID_OFFSET_BIN2
        || i_id_e == eVW_FILE_ID_DEFECT_BIN2
        || i_id_e == eVW_FILE_ID_DEFECT_GRID_BIN2
        || i_id_e == eVW_FILE_ID_DEFECT_FOR_GATEOFF_BIN2
        || i_id_e == eVW_FILE_ID_GAIN_BIN2
        || i_id_e == eVW_FILE_ID_PREVIEW_GAIN
        || i_id_e == eVW_FILE_ID_PREVIEW_DEFECT )
    {
        return true;
    }
    
    if( m_p_download_c == NULL )
    {
        return false;
    }

    if( i_id_e == eVW_FILE_ID_BOOTLOADER )
    {
        char s_version[128] = "";
        if ( m_p_download_c->get_version(s_version) )     
            return m_p_system_c->check_compability_bootloader_version(s_version);
        return false;
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
bool CControlServer::download_check_crc(void)
{
    FILE*       p_file;
    u8*         p_data;
    u32         n_data_size;
    u32         n_crc;
    
    p_file = fopen(m_p_file_name, "rb");
    
    if( p_file == NULL )
    {
        print_log(ERROR, 1, "[Control Server] (%s)file open failed\n", m_p_file_name);
        return false;
    }
    
    n_data_size = file_get_size(p_file);
	
	p_data = (u8*)malloc(n_data_size);
	if( p_data == NULL )
	{
	    fclose( p_file );
	    
	    print_log(ERROR, 1, "[%s] (%s)malloc failed.\n", m_p_file_name);
	    return false;
	}
	
	fseek( p_file, 0, SEEK_SET );
	fread( p_data, 1, n_data_size, p_file );
	fclose( p_file );
	
	n_crc = make_crc32( p_data, n_data_size );
	if( m_n_file_crc != n_crc )
	{
	    print_log(DEBUG, 1, "[Control Server] (%s)CRC failed(0x%08X/0x%08X)\n", m_p_file_name, n_crc, m_n_file_crc);
	    safe_free(p_data);
	    return false;
	}
	
	safe_free(p_data);
	return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CControlServer::download_save( VW_FILE_ID i_id_e )
{
    if( i_id_e == eVW_FILE_ID_OFFSET
        || i_id_e == eVW_FILE_ID_DEFECT_MAP
        || i_id_e == eVW_FILE_ID_GAIN
        || i_id_e == eVW_FILE_ID_OSF_PROFILE
        || i_id_e == eVW_FILE_ID_OFFSET_BIN2
        || i_id_e == eVW_FILE_ID_DEFECT_BIN2
        || i_id_e == eVW_FILE_ID_DEFECT_GRID_BIN2
        || i_id_e == eVW_FILE_ID_DEFECT_FOR_GATEOFF_BIN2
        || i_id_e == eVW_FILE_ID_GAIN_BIN2
        || i_id_e == eVW_FILE_ID_PREVIEW_GAIN
        || i_id_e == eVW_FILE_ID_PREVIEW_DEFECT )
    {
        return true;
    }
    
    if( m_p_download_c == NULL )
    {
        return false;
    }
    
    //20210913. youngjun han. 
    //SDK로부터 전달받은 FPGA binary 안에 여러개의 binary가 존재하는지 확인 및 적절한 처리 수행
    //찾아진 vesion 정보 수 많큼 binary가 존재한다고 판단한다.
    //version정보는 1쌍의 magic string 사이에 있다.
    if( i_id_e == eVW_FILE_ID_FPGA )
    {
        //u32 image_num = 0x0000;
        u32 binary_offset = 0x0000;
        u32 binary_size = 0x0000;
        u32 ret = 0x0001;

        ROIC_TYPE roic_type_e = m_p_system_c->get_roic_model_type();
        print_log(ERROR, 1, "[%s] roic_type_e: %d\n", m_p_file_name, roic_type_e);
        
        switch(roic_type_e)
        {
        case eROIC_TYPE_ADI_1255:
            ret = m_p_download_c->search_fpga_major_version_loc(ADI_1255_FPGA_MAJOR_VERSION, &binary_offset, &binary_size);
            break;
        case eROIC_TYPE_TI_2256GR:
            ret = m_p_download_c->search_fpga_major_version_loc(TI_2256_FPGA_MAJOR_VERSION, &binary_offset, &binary_size);
            break;
        default:
            print_log(ERROR, 1, "[%s] err roic_type_e: %d\n", m_p_file_name, roic_type_e);
            break;
        }        
                
        if(ret == 0)    
        {
            return m_p_download_c->save(binary_offset, binary_size);
        }
        else
        {
            return m_p_download_c->save(0x0000, 0x0000);
        }
    }

    return m_p_download_c->save(0x0000, 0x0000);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CControlServer::download_end(void)
{
    sys_command("rm %s", m_p_file_name);
    safe_delete( m_p_download_c );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::reqCalibration(u8* pData)
{
    packet_header_t*    p_hdr_t;
    u32                 n_count;
    u8                  c_result = 0;
    
    p_hdr_t = (packet_header_t*)pData;
    
    if (p_hdr_t->c_operation == 0)
    {
        memcpy( &n_count, &pData[sizeof(packet_header_t)], sizeof(n_count) );
        n_count = ntohl( n_count );
        
        print_log(DEBUG, 1, "[Control Server] offset cal(count: %d)\n", n_count );
        
        if( m_p_calibration_c->offset_cal_request( n_count, 1 ) == false )
        {
            c_result = 1;
        }
        else
        {
            m_b_offet_cal_stop = false;
        }
        
        return packet_send(PAYLOAD_CALIBRATION, m_n_client_seq_id, NULL, 0, c_result);
    }
    else if (p_hdr_t->c_operation == 1)
    {
        m_b_offet_cal_stop = true;
        print_log(DEBUG, 1, "[Control Server] offset cal stop by SW\n" );
        
        return packet_send(PAYLOAD_CALIBRATION, m_n_client_seq_id);
    }
    
    return packet_send(PAYLOAD_CALIBRATION, m_n_client_seq_id);
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::req_offset_drift( u8* i_p_data )
{
    packet_header_t*    p_hdr_t;
    
    u16                 p_data[2];
    
    u8                  c_result = 0;
    
    p_hdr_t = (packet_header_t*)i_p_data;
    
    if( p_hdr_t->c_operation == 1 )
    {
        m_p_system_c->osf_stop();
        
        return packet_send( PAYLOAD_OFFSET_DRIFT, m_n_client_seq_id );
    }
    else if( p_hdr_t->c_operation == 2 )
    {
        u32 n_time = m_p_system_c->osf_get_vcom_swing_time();
        
        print_log(DEBUG, 1, "[Control Server] vcom swing time: %d ms\n", n_time);
        
        n_time = htonl(n_time);
        
        return packet_send( PAYLOAD_OFFSET_DRIFT, m_n_client_seq_id, (u8*)&n_time, sizeof(n_time) );
    }
        
    memcpy( p_data, &i_p_data[sizeof(packet_header_t)], sizeof(p_data) );
    
    p_data[0] = ntohs( p_data[0] );
    p_data[1] = ntohs( p_data[1] );
    
    if( m_p_system_c->osf_start( p_data[0], p_data[1] ) == false )
    {
        c_result = 1;
    }
    
    return packet_send( PAYLOAD_OFFSET_DRIFT, m_n_client_seq_id, NULL, 0, c_result );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::req_offset_control( u8* i_p_data )
{
    packet_header_t*    p_hdr_t;
    u8                  c_result;
    
    p_hdr_t = (packet_header_t*)i_p_data;
    
    if( p_hdr_t->c_operation == 0 )
    {
        m_p_calibration_c->offset_set_enable( false );
    }
    else if( p_hdr_t->c_operation == 1 )
    {
        m_p_calibration_c->offset_set_enable( true );
    }
    
    c_result = m_p_calibration_c->offset_get_enable();
    
    print_log(DEBUG, 1, "[Control Server] offset control: %d\n", c_result );
    
    return packet_send( PAYLOAD_OFFSET_CONTROL, m_n_client_seq_id, NULL, 0, c_result );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::req_cal_data_control( u8* i_p_data )
{
    packet_header_t*    p_hdr_t;
    u8 p_data[2];
    
    p_hdr_t = (packet_header_t*)i_p_data;
    
    memcpy( p_data, &i_p_data[sizeof(packet_header_t)], sizeof(p_data) );
    
    if( p_hdr_t->c_operation == 0 )
    {
    	p_data[1] = m_p_calibration_c->get_enable((CALIBRATION_TYPE)p_data[0]);
    	
    	print_log(DEBUG, 1, "[Control Server] %d is enable: %d\n", p_data[0], p_data[1]);
    	
    	return packet_send( PAYLOAD_CAL_DATA_CONTROL, m_n_client_seq_id, p_data, sizeof(p_data) );
    }
    
    if( p_hdr_t->c_operation == 1 )
    {
    	if( m_p_calibration_c->is_loaded((CALIBRATION_TYPE)p_data[0]) == false )
    	{
    		return packet_send( PAYLOAD_CAL_DATA_CONTROL, m_n_client_seq_id, p_data, sizeof(p_data), 1 );
    	}
    	
    	m_p_calibration_c->set_enable( (CALIBRATION_TYPE)p_data[0], p_data[1] );
    	
    	print_log(DEBUG, 1, "[Control Server] %d set enable: %d\n", p_data[0], p_data[1]);
    	
    	return packet_send( PAYLOAD_CAL_DATA_CONTROL, m_n_client_seq_id, p_data, sizeof(p_data), 0 );
    }
	
	if( p_hdr_t->c_operation == 2 )
    {
    	p_data[1] = m_p_calibration_c->is_loaded((CALIBRATION_TYPE)p_data[0]);
    	
    	print_log(DEBUG, 1, "[Control Server] %d is loaded: %d\n", p_data[0], p_data[1]);
    	
    	return packet_send( PAYLOAD_CAL_DATA_CONTROL, m_n_client_seq_id, p_data, sizeof(p_data) );
    }

	if( p_hdr_t->c_operation == 3 )
    {
    	print_log(DEBUG, 1, "[Control Server] %d load start: %d\n", p_data[0], p_data[1]);
    	
    	if( p_data[1] > 0 )
    	{
    		m_p_calibration_c->load_start( (CALIBRATION_TYPE)p_data[0] );
    	}
    	else
    	{
    		m_p_calibration_c->load_stop();
    	}

    	return packet_send( PAYLOAD_CAL_DATA_CONTROL, m_n_client_seq_id, p_data, sizeof(p_data) );
    }

	if( p_hdr_t->c_operation == 4 )
    {
    	u8 c_progress;
    	u8 c_result = 0;
    	
    	c_progress = m_p_calibration_c->load_get_progress();
    	
    	if( c_progress == 100 )
    	{
    		if( m_p_calibration_c->load_get_result() == false )
    		{
    			c_result = 1;
    		}
    	}
    	
    	return packet_send( PAYLOAD_CAL_DATA_CONTROL, m_n_client_seq_id, &c_progress, sizeof(c_progress), c_result );
    }
      
    return packet_send( PAYLOAD_CAL_DATA_CONTROL, m_n_client_seq_id );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::req_nfc_card_control( u8* i_p_data )
{
    return packet_send( PAYLOAD_NFC_CARD_CONTROL, m_n_client_seq_id );
}


/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::req_smart_w_control( u8* i_p_data )
{
    packet_header_t *p_hdr_t;
    u8 c_result = 0; // success
    u8 c_oper;
    // bool				b_result = false;

    p_hdr_t = (packet_header_t *)i_p_data;
    c_oper = p_hdr_t->c_operation;

    //Get Smart W enable
    if (c_oper == 0)
    {
        u8 c_data = m_p_system_c->get_smart_w_onoff();
        return packet_send(PAYLOAD_SMART_W_CONTROL, m_n_client_seq_id, (u8 *)&c_data, sizeof(c_data), c_result);
    }

    //Set Smart W enable
    else if (c_oper == 1)
    {
        u8 c_data = 0;
        u32 n_pos = 0;

        n_pos = sizeof(packet_header_t);
        memcpy(&c_data, &i_p_data[n_pos], sizeof(c_data));

        c_result = m_p_system_c->set_smart_w_onoff(c_data);
        if(c_result == 0)
            m_p_system_c->dev_reg_save_file(eDEV_ID_FPGA);

        return packet_send(PAYLOAD_SMART_W_CONTROL, m_n_client_seq_id, NULL, 0, c_result);
    }

    return packet_send(PAYLOAD_SMART_W_CONTROL, m_n_client_seq_id);
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::req_aed_off_debounce_control( u8* i_p_data )
{
    packet_header_t *p_hdr_t;
    u8 c_result = 0; // success
    u8 c_oper;
    // bool				b_result = false;

    p_hdr_t = (packet_header_t *)i_p_data;
    c_oper = p_hdr_t->c_operation;

    //Get aed off debounce time
    if (c_oper == 0)
    {
        u16 w_data = m_p_system_c->get_aed_off_debounce_time();
        w_data = htons(w_data);
        return packet_send(PAYLOAD_AED_OFF_DEBOUNCE_CONTROL, m_n_client_seq_id, (u8 *)&w_data, sizeof(w_data), c_result);
    }

    //Set aed off debounce time
    else if (c_oper == 1)
    {
        u16 w_data = 0;
        u32 n_pos = 0;

        n_pos = sizeof(packet_header_t);
        memcpy(&w_data, &i_p_data[n_pos], sizeof(w_data));
        w_data = ntohs(w_data);

        c_result = m_p_system_c->set_aed_off_debounce_time(w_data);
        if(c_result == 0)
            m_p_system_c->dev_reg_save_file(eDEV_ID_FPGA);

        return packet_send(PAYLOAD_AED_OFF_DEBOUNCE_CONTROL, m_n_client_seq_id, NULL, 0, c_result);
    }

    return packet_send(PAYLOAD_AED_OFF_DEBOUNCE_CONTROL, m_n_client_seq_id);
}

/******************************************************************************/
/**
* \brief        Control Battery Charging Mode
* \param        none
* \return       none
* \note         
*******************************************************************************/
bool CControlServer::req_battery_charging_mode( u8* i_p_data )
{
    packet_header_t *p_hdr_t;
    u8 c_result = 0; // 0: success, 1: failed
    u8 c_oper;
    u8 c_data;
    u32 n_data;
    u32 n_pos = 0;

    p_hdr_t = (packet_header_t *)i_p_data;
    c_oper = p_hdr_t->c_operation;

    n_pos = sizeof(packet_header_t);

    //get charging mode
    if (c_oper == 0)
    {
        n_data = m_p_system_c->get_battery_charging_mode();
        n_data = ntohl(n_data);

        return packet_send(PAYLOAD_BATTERY_CHARGING_MODE, m_n_client_seq_id, (u8 *)&n_data, sizeof(n_data), c_result);
    }
    //set charging mode
    else if (c_oper == 1)
    {
        /*set charging mode to vwsystem*/
        memcpy(&n_data, &i_p_data[n_pos], sizeof(n_data));
        n_data = ntohl(n_data);

        m_p_system_c->set_battery_charging_mode(n_data);
        return packet_send(PAYLOAD_BATTERY_CHARGING_MODE, m_n_client_seq_id, NULL, 0, c_result);
    }
    //get battery recharging gauge 
    else if (c_oper == 2)
    {
        c_data = m_p_system_c->get_battery_recharging_gauge();
        return packet_send(PAYLOAD_BATTERY_CHARGING_MODE, m_n_client_seq_id, (u8 *)&c_data, sizeof(c_data), c_result);
    }
    //set battery recharging gauge
    else if (c_oper == 3)
    {
        memcpy(&c_data, &i_p_data[n_pos], sizeof(c_data));
        /*if recharging gauge is out of range(50~100), failed to set*/
        if(m_p_system_c->is_recharging_gauge_out_of_range(c_data) == true) c_result = 1;
        /*set recharging gauge to vwsystem*/
        else m_p_system_c->set_battery_recharging_gauge(c_data);

        return packet_send(PAYLOAD_BATTERY_CHARGING_MODE, m_n_client_seq_id, NULL, 0, c_result);
    }

    return packet_send(PAYLOAD_BATTERY_CHARGING_MODE, m_n_client_seq_id);
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::req_trans_test( u8* i_p_data )
{
    packet_header_t*    p_hdr_t;
    bool                b_result = true;
    
    p_hdr_t = (packet_header_t*)i_p_data;
    
    if( p_hdr_t->c_operation == 0 )
    {
        return packet_send( PAYLOAD_TRANSMISSION_TEST, m_n_client_seq_id );
    }
    
    if( p_hdr_t->c_operation == 1 )
    {
        b_result = packet_send( PAYLOAD_TRANSMISSION_TEST, m_n_client_seq_id );
        trans_start();
    }
    else if( p_hdr_t->c_operation == 2 )
    {
        b_result = packet_send( PAYLOAD_TRANSMISSION_TEST, m_n_client_seq_id );
        trans_stop();
    }
    else if( p_hdr_t->c_operation == 3 )
    {
        b_result = packet_send( PAYLOAD_TRANSMISSION_TEST, m_n_client_seq_id );
        
        /*Smart W enable 상태의 경우를 고려해 image test 에는 X-ray delay num(0x00c3)를 잠시 0으로 설정*/
        m_p_system_c->test_image_change_xray_delay_num(true);
        m_p_system_c->test_image_enable( true );
        m_p_system_c->exposure_request( eTRIGGER_TYPE_SW );
        m_p_system_c->test_image_change_xray_delay_num(false);
    }
    
    return b_result;    
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::req_ap_channel_change( u8* i_p_data )
{
    u8      c_result = 0;
    u32     n_channel;
    
    memcpy( &n_channel, &i_p_data[sizeof(packet_header_t)], sizeof(n_channel) );
    n_channel   = ntohl( n_channel );
    
    if( m_p_net_manager_c->net_ap_get_channel() == n_channel )
    {
        c_result    = 0;
        n_channel   = htonl( n_channel );
        
        return packet_send( PAYLOAD_AP_CHANNEL_CHANGE_RESULT, m_n_client_seq_id, \
                            (u8*)&n_channel, sizeof(n_channel), c_result );
    }
    
    if( m_p_system_c->net_is_valid_channel((u16)n_channel) == false )
    {
        c_result    = 1;
        n_channel   = htonl( n_channel );
        
        return packet_send( PAYLOAD_AP_CHANNEL_CHANGE_RESULT, m_n_client_seq_id, \
                            (u8*)&n_channel, sizeof(n_channel), c_result );
    }
    
    c_result    = 0;
    n_channel   = htonl( n_channel );
    
    packet_send( PAYLOAD_AP_CHANNEL_CHANGE_RESULT, m_n_client_seq_id, \
                        (u8*)&n_channel, sizeof(n_channel), c_result );
    
    // 채널이 변경되면서 접속이 끊긴다.
    m_p_net_manager_c->net_ap_set_channel( (u16)n_channel );
    
    return true;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::req_ap_channel_change_result( u8* i_p_data )
{
    u8      c_result = 0;
    u32     n_channel = 0;
    
    c_result    = (u8)m_p_net_manager_c->net_ap_get_channel_result();
    n_channel   = m_p_net_manager_c->net_ap_get_channel();
    
    print_log(DEBUG, 1, "[Control Server] req_ap_channel_change_result(%d: %d)\n", c_result, n_channel);
    
    n_channel   = htonl( n_channel );
    
    return packet_send( PAYLOAD_AP_CHANNEL_CHANGE_RESULT, m_n_client_seq_id, \
                        (u8*)&n_channel, sizeof(n_channel), c_result );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::req_scintillator_type_range( u8* i_p_data )
{
    u8      c_result = 0;
    u32     n_range[2] = {0,0};
    
    //VW Scintillator Type Table
    /*
    0 - Lami (depreciated)
    1 - Decomp (depreciated)
    2 - A (YMIT_Std CsI, 3643/4343 VW default)
    3 - B (AbyzR 370um_CsI, none)
    4 - C (YMIT_PLUS CsI 증착, Innolux_500um 증착, none)
    5 - D (YMIT_Normal CsI, CETD_600um 증착, H사 Flexible 600u)
    6 - E (2530VAW/VAW PLUS 모델의 기본 Scintilator Type)
    7 - F (none, none)
    */
    n_range[0] = htonl(eSCINTILLATOR_TYPE_02);         //start
    n_range[1] = htonl(eSCINTILLATOR_TYPE_MAX - 1);    //end
            
    return packet_send( SCINTILLATOR_TYPE_RANGE_ADVERTISEMENT, m_n_client_seq_id, \
                        (u8*)&n_range, sizeof(n_range), c_result );
}

bool CControlServer::reqExposureControl(u8* pData)
{
    packet_header_t *pHeader;
    u8 result = 0; // 0: success, 1: failed
    u8 operation;

    pHeader = (packet_header_t *)pData;
    operation = pHeader->c_operation;

    if (operation == 0)
    {
        m_p_system_c->SetReadoutSync();
        return packet_send(PAYLOAD_EXPOSURE_CONTROL, m_n_client_seq_id, NULL, 0, result);
    }
            
    return packet_send(PAYLOAD_EXPOSURE_CONTROL, m_n_client_seq_id);
}

bool CControlServer::req_offset_correction_ctrl( u8* i_p_data )
{
    packet_header_t*    p_hdr_t;
    u8                  c_result = 0;       // success
    u8                  c_oper;
	u8 					c_buf[64]; 
	
    p_hdr_t = (packet_header_t*)i_p_data;
    c_oper  = p_hdr_t->c_operation;
    
	if (c_oper == eCORRECTION_PACKET_AVALIABLE)
	{
		c_buf[0] = true; 	// support flag for normal offset correction mode
		c_buf[1] = true; 	// support flag for Post offset correction mode
		print_log(DEBUG, 1, "[Control Server] offset correction mode[%d] : normal_support[%d], post_support[%d]\n", c_oper, c_buf[0], c_buf[1] );
		return packet_send( PAYLOAD_OFFSET_CORRECTION_CTRL, m_n_client_seq_id, (u8*)c_buf, 2, c_result );
	}
	else if (c_oper == eCORRECTION_PACKET_GET_ACTIVE)
	{
		c_buf[0] = m_p_system_c->dev_reg_read(eDEV_ID_FPGA, eFPGA_REG_DRO_IMG_CAPTURE_OPTION); 
		print_log(DEBUG, 1, "[Control Server] offset correction mode[%d] : post_offset_get_state[%d]\n", c_oper, c_buf[0]);
		return packet_send( PAYLOAD_OFFSET_CORRECTION_CTRL, m_n_client_seq_id, (u8*)c_buf, 1, c_result );
	}
	else if (c_oper == eCORRECTION_PACKET_SET_ACTIVE)
	{	
		c_buf[0] = i_p_data[sizeof(packet_header_t)]; 
		m_p_system_c->dev_reg_write( eDEV_ID_FPGA, eFPGA_REG_DRO_IMG_CAPTURE_OPTION, c_buf[0] );
        m_p_system_c->dev_reg_save(eDEV_ID_FPGA, eFPGA_REG_DRO_IMG_CAPTURE_OPTION, eFPGA_REG_DRO_IMG_CAPTURE_OPTION);
		m_p_system_c->dev_reg_save_file( eDEV_ID_FPGA );
		print_log(DEBUG, 1, "[Control Server] offset correction mode[%d] : post_offset_set_state[%d]\n", c_oper, c_buf[0]);
		return packet_send( PAYLOAD_OFFSET_CORRECTION_CTRL, m_n_client_seq_id, (u8*)c_buf, 1, c_result );
	}
	else if (c_oper == eCORRECTION_PACKET_APPLIED)
	{
		c_buf[0] = m_p_system_c->GetCurrentOffsetCorrectionMode(); 
		print_log(DEBUG, 1, "[Control Server] offset correction mode[%d] : post_offset_oper_mode[%d]\n", c_oper, c_buf[0]);
		return packet_send( PAYLOAD_OFFSET_CORRECTION_CTRL, m_n_client_seq_id, (u8*)c_buf, 1, c_result );
	}
	
	return packet_send( PAYLOAD_OFFSET_CORRECTION_CTRL, m_n_client_seq_id );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::noti_offset_cal( u8* i_p_data, u16 i_w_lenth )
{
    u32     n_state;
    u32     n_count  = 0;
    bool    b_send_state = true;
    u8      c_result = 0;
    
    memcpy( &n_state, i_p_data, sizeof(n_state) );
    
    print_log(DEBUG, 1, "[Control Server] noti_offset_cal state: %d\n", n_state);
    
    if( n_state == eOFFSET_CAL_ACQ_COUNT )
    {
        memcpy( &n_count, &i_p_data[4], sizeof(n_count) );
    }
    else if( n_state == eOFFSET_CAL_ACQ_COMPLETE )
    {
        m_p_calibration_c->offset_cal_stop();
        m_p_calibration_c->offset_cal_reset();
        memcpy( &n_count, &i_p_data[4], sizeof(n_count) );
        
        if( m_b_client_valid == true
            && m_b_offet_cal_stop == false )
        {
            send_stream_data(PAYLOAD_OFFSET_DATA);
        }
        
        m_b_offet_cal_stop = false;
    }
    else if( n_state == eOFFSET_CAL_ACQ_CANCELLED )
    {
        m_p_calibration_c->offset_cal_stop();
        m_p_calibration_c->offset_cal_reset();
        c_result = 1;
    }
    else if( n_state == eOFFSET_CAL_FAILED )
    {
        if( m_b_offet_cal_stop == true )
        {
            m_b_offet_cal_stop = false;
            
            m_p_calibration_c->offset_cal_stop();
            m_p_calibration_c->offset_cal_reset();
            b_send_state = false;
        }
        c_result = 2;
    }
    else if( n_state == eOFFSET_CAL_ACQ_TIMEOUT )
    {
        m_p_calibration_c->offset_cal_stop();
        m_p_calibration_c->offset_cal_reset();
        c_result = 2;
    }
    else if( n_state == eOFFSET_CAL_TRIGGER )
    {
        if( m_b_offet_cal_stop == true )
        {
            m_b_offet_cal_stop = false;
            
            m_p_calibration_c->offset_cal_stop();
            m_p_calibration_c->offset_cal_reset();
            b_send_state = false;
        }
        else
        {
            if( m_p_calibration_c->offset_cal_trigger() == false )
            {
                m_p_calibration_c->offset_cal_stop();
                m_p_calibration_c->offset_cal_reset();
                c_result = 2;
            }
            else
            {
                b_send_state = false;
            }
        }
        
    }
    else if( n_state == eOFFSET_CAL_ACQ_ERROR )
    {
        m_p_calibration_c->offset_cal_stop();
        c_result = 2;
    }
    else if( n_state == eOFFSET_CAL_REQUEST )
    {
        u32 p_count[2];
        
        memcpy( p_count, &i_p_data[sizeof(n_state)], sizeof(p_count) );
        
        m_p_calibration_c->offset_cal_start(p_count[0], p_count[1]);
        b_send_state = false;
    }
    else
    {
        c_result = 2;
    }
    
    n_count = htonl( n_count );
    
    if( b_send_state == true )
    {
	    return packet_send( PAYLOAD_NOTI_OFFSET_CAL_STATE, 0, \
	                        (u8*)&n_count, sizeof(n_count), c_result );
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
bool CControlServer::noti_osf( u8* i_p_data, u16 i_w_lenth )
{
    u32     n_state;
    u32     n_count  = 0;
    u8      c_result = 0;
    
    memcpy( &n_state, i_p_data, sizeof(n_state) );
    memcpy( &n_count, &i_p_data[4], sizeof(n_count) );
    
    print_log(DEBUG, 1, "[Control Server] noti_osf state: %d(count: %d)\n", n_state, n_count);
    
    if( n_state == eOFFSET_DRIFT_ACQ_COUNT )
    {
        c_result = 0;
    }
    else if( n_state == eOFFSET_DRIFT_ACQ_COMPLETE )
    {
        m_p_system_c->osf_stop();
        c_result = 0;
    }
    else if( n_state == eOFFSET_DRIFT_ACQ_CANCELLED )
    {
        m_p_system_c->osf_stop();
        c_result = 1;
    }
    else
    {
        m_p_system_c->osf_stop();
        c_result = 1;
    }
    
    n_count = htonl( n_count );
    
    return packet_send( PAYLOAD_NOTI_OFFSET_DRIFT, 0, (u8*)&n_count, sizeof(n_count), c_result );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void * alive_routine( void * arg )
{
	CControlServer * svr = (CControlServer *)arg;
	svr->alive_proc();
	pthread_exit( NULL );
	return 0;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CControlServer::alive_proc(void)
{
    print_log(DEBUG, 1, "[Control Server] start alive_proc...\n");
    
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
            else if( m_b_stream_is_running == true )
            {
            	if( m_p_alive_timer != NULL
                    && m_p_alive_timer->is_expired() == true )
				{
					print_log(DEBUG, 1, "[Control Server] alive check: wait image sending\n");
					safe_delete(m_p_alive_timer);
                	m_p_alive_timer = new CTime(m_n_alive_interval_time * 1000);
				}
            }
            else if( m_b_stream_sending_error == true )
            {
            	m_b_stream_sending_error = false;
            	print_log(DEBUG, 1, "[Control Server] alive check: image send error\n");
            	close_client( m_n_client_fd );
				safe_delete(m_p_alive_timer);
            }
            else if( m_p_img_retransfer_c->is_send_error() == true )
            {
				print_log(DEBUG, 1, "[Control Server] alive check: retransmission send error\n");
            	close_client( m_n_client_fd );
				safe_delete(m_p_alive_timer);
			}
            else
            {
                if( m_p_alive_timer != NULL
                    && m_p_alive_timer->is_expired() == true )
                {
                    print_log(DEBUG, 1, "[Control Server] alive check: control time out\n");
                    close_client( m_n_client_fd );
                    safe_delete(m_p_alive_timer);
                    
                    if( m_p_system_c != NULL
                        && m_p_system_c->is_ready() == true )
                    {
                        m_p_system_c->print_state();
                        m_p_system_c->dev_reg_print(eDEV_ID_ADC);
                    }
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
    
    print_log(DEBUG, 1, "[Control Server] end alive_proc...\n");
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CControlServer::alive_reset(void)
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
bool CControlServer::client_decode( login_t* i_p_login_t )
{
    u8 key[8];
    u8 p_time[4];
    
    memset( key, 0, sizeof(key) );
    memcpy( p_time, &i_p_login_t->n_time, sizeof(p_time) );
    
    key[0] = p_time[0];
    key[2] = p_time[1];
    key[4] = p_time[2];
    key[6] = p_time[3];
    key[1] = 'V';
    key[3] = 'W';
    key[5] = 'X';
    key[7] = 'R';
    
    m_cipher_c.Initialize( key, sizeof(key) );
    
    u8 p_out[8];
    m_cipher_c.Decode( i_p_login_t->p_encrypted_id, p_out, sizeof(p_out) );
    
    if( memcmp( p_out, (void*)"VWXRVWFX", sizeof(p_out) ) == 0 )
    {
        print_log(INFO, 1, "[Control Server] SW is connected.\n");
        return true;
    }
    
    print_log(INFO, 1, "[Control Server] client id is invalid\n");
    //print_log(DEBUG, 1, "[Control Server] time: 0x%04X\n", i_p_login_t->n_time);
    
#if 0
    u8 p_encoded_data[8];
    memset( p_encoded_data, 0, sizeof(p_encoded_data) );
    m_cipher_c.Encode( (u8*)"VWXRVWFX", p_encoded_data, 8 );
    
    print_log(DEBUG, 1, "[Control Server] key: %02X %02X %02X %02X %02X %02X %02X %02X\n", \
                    key[0], key[1], \
                    key[2], key[3], \
                    key[4], key[5], \
                    key[6], key[7] );
    
    print_log(DEBUG, 1, "[Control Server] enc: %02X %02X %02X %02X %02X %02X %02X %02X\n", \
                    i_p_login_t->p_encrypted_id[0], i_p_login_t->p_encrypted_id[1], \
                    i_p_login_t->p_encrypted_id[2], i_p_login_t->p_encrypted_id[3], \
                    i_p_login_t->p_encrypted_id[4], i_p_login_t->p_encrypted_id[5], \
                    i_p_login_t->p_encrypted_id[6], i_p_login_t->p_encrypted_id[7] );
    
    print_log(DEBUG, 1, "[Control Server] enc: %02X %02X %02X %02X %02X %02X %02X %02X\n", \
                    p_encoded_data[0], p_encoded_data[1], \
                    p_encoded_data[2], p_encoded_data[3], \
                    p_encoded_data[4], p_encoded_data[5], \
                    p_encoded_data[6], p_encoded_data[7] );
    
    print_log(DEBUG, 1, "[Control Server] dec: %02X %02X %02X %02X %02X %02X %02X %02X\n", \
                    p_out[0], p_out[1], \
                    p_out[2], p_out[3], \
                    p_out[4], p_out[5], \
                    p_out[6], p_out[7] );
#endif
    
    return false;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
#if 0
u8* CControlServer::get_current_image( u32* o_p_id )
{
    image_buffer_t* p_image_t;
    
    if( m_resend_state_e == eRESEND_STATE_ACQ_START )
    {
        // 전송하고 있는 영상 버퍼를 가져온다.
        p_image_t = m_p_stream_image_t;
    }
    else if( m_resend_state_e == eRESEND_STATE_ACQ_END )
    {
        // 메모리로 복사한 영상 버퍼를 가져온다.
        p_image_t = m_p_system_c->get_copy_image();
    }
    else
    {
        p_image_t = NULL;
    }
    
    if( p_image_t == NULL )
    {
        *o_p_id = 0xFFFFFFFF;
        return NULL;
    }
    
#if 0
    print_log(DEBUG, 1, "[Control Server] current image id: 0x%08X\n", \
                            p_image_t->header_t.n_id);
#endif
    
    *o_p_id = p_image_t->header_t.n_id;
    return p_image_t->p_buffer;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
u8* CControlServer::get_current_preview_image( u32* o_p_id )
{
    image_buffer_t* p_image_t;
    
    if( m_resend_state_e == eRESEND_STATE_ACQ_START )
    {
        // 전송하고 있는 영상 버퍼를 가져온다.
        p_image_t = m_p_stream_image_t;
    }
    else if( m_resend_state_e == eRESEND_STATE_ACQ_END )
    {
        // 메모리로 복사한 영상 버퍼를 가져온다.
        p_image_t = m_p_system_c->get_copy_image();
    }
    else
    {
        p_image_t = NULL;
    }
    
    if( p_image_t == NULL )
    {
        *o_p_id = 0xFFFFFFFF;
        return NULL;
    }
    
#if 0
    print_log(DEBUG, 1, "[Control Server] current image id: 0x%08X\n", \
                            p_image_t->header_t.n_id);
#endif
    
    *o_p_id = p_image_t->header_t.n_id;
    
    return p_image_t->preview_t.p_buffer;
}
#endif

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CControlServer::req_dark_image( u8* i_p_data )
{
    u8   c_result;
    bool b_result = m_p_system_c->exposure_request(eTRIGGER_TYPE_SW);
    
    if( b_result == true )
    {
        c_result = 0;
    }
    else
    {
        c_result = 1;
    }
    
    return packet_send( PAYLOAD_DARK_IMAGE, m_n_client_seq_id, NULL, 0, c_result );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CControlServer::req_preview_enable( u8* i_p_data )
{
    packet_header_t*    p_hdr_t;
    u16                 w_count = 0;
    
    p_hdr_t = (packet_header_t*)i_p_data;
    
    print_log(DEBUG, 1, "[Control Server] preview: %d\n", p_hdr_t->c_operation);

    if( p_hdr_t->c_operation == 0 )
    {
        m_p_system_c->preview_set_enable(false);
    }
    else if( p_hdr_t->c_operation == 1 )
    {
        m_p_system_c->preview_set_enable(true);
    }
    
    if( p_hdr_t->c_operation == 2 )
    {
        w_count = htons( m_p_system_c->tact_get_image_count() );
        return packet_send( PAYLOAD_PREVIEW_ENABLE, m_n_client_seq_id, (u8*)&w_count, sizeof(w_count) );
    }
    
    if( p_hdr_t->c_operation == 3 )
    {
        u32 n_data[2];
        
        memcpy( n_data, &i_p_data[sizeof(packet_header_t)], sizeof(n_data) );
        
        n_data[0] = ntohl( n_data[0] );
        n_data[1] = ntohl( n_data[1] );
    
        m_p_system_c->tact_delete_image( n_data[0], n_data[1] );
        
        return packet_send( PAYLOAD_PREVIEW_ENABLE, m_n_client_seq_id );
        
    }
    
    if( p_hdr_t->c_operation == 4 )
    {
        u8 c_enable = (u8)m_p_system_c->tact_get_enable();
        
        return packet_send( PAYLOAD_PREVIEW_ENABLE, m_n_client_seq_id, &c_enable, sizeof(c_enable) );
    }

    if( p_hdr_t->c_operation == 5 )
    {
        m_p_system_c->tact_flush_image();
        
        return packet_send( PAYLOAD_PREVIEW_ENABLE, m_n_client_seq_id );
    }
    
    return packet_send( PAYLOAD_PREVIEW_ENABLE, m_n_client_seq_id );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CControlServer::send_stream_data( u32 i_n_payload )
{
    u32 n_crc;
    image_buffer_t* p_img_buffer_t;
    
    n_crc   = m_p_calibration_c->offset_get_crc();
    
    print_log(DEBUG, 1, "[Control Server] malloc stream buffer(payload: 0x%04X)\n", i_n_payload);
    p_img_buffer_t = (image_buffer_t*)malloc( sizeof(image_buffer_t) );
    
    memset( p_img_buffer_t, 0, sizeof(image_buffer_t) );
    
    p_img_buffer_t->header_t.n_total_size   = m_p_system_c->get_image_size_byte();
    p_img_buffer_t->n_processed_length      = m_p_system_c->get_image_size_byte();
    
    p_img_buffer_t->p_buffer = (u8*)m_p_calibration_c->offset_get();
    print_log(DEBUG, 1, "[Control Server] buffer malloc: %X\n", p_img_buffer_t );
    p_img_buffer_t->preview_t.p_buffer = NULL;
    
    if( sender_start(p_img_buffer_t, i_n_payload, n_crc) == false )
    {
        memset( p_img_buffer_t, 0, sizeof(image_buffer_t) );
        safe_free(p_img_buffer_t);
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CControlServer::req_state( u8* i_p_data )
{
    sys_state_t state_t;
    u32 i;
    
    memset( &state_t, 0, sizeof(sys_state_t) );
    
    m_p_system_c->get_state( &state_t );
    
    state_t.w_network_mode          = htons( state_t.w_network_mode );
    state_t.w_power_source          = htons( state_t.w_power_source );
    state_t.w_link_quality_level    = htons( state_t.w_link_quality_level );
    state_t.w_link_quality          = htons( state_t.w_link_quality );
    state_t.w_battery_gauge         = htons( state_t.w_battery_gauge );
    state_t.w_battery_vcell         = htons( state_t.w_battery_vcell / 10 );
    state_t.w_temperature           = htons( state_t.w_temperature );
    state_t.w_humidity              = htons( state_t.w_humidity );
    
    state_t.w_tether_speed          = htons( state_t.w_tether_speed );
    state_t.w_power_mode            = htons( state_t.w_power_mode );
    
    state_t.w_battery_level         = htons( state_t.w_battery_level );
    state_t.w_battery_raw_gauge     = htons( state_t.w_battery_raw_gauge );
    state_t.w_battery_charge_complete_1     = htons( state_t.w_battery_charge_complete_1 );
    state_t.w_battery_charge_complete_2     = htons( state_t.w_battery_charge_complete_2 );
    
    for( i = 0; i < POWER_VOLT_MAX; i++ )
    {
        state_t.p_hw_power_volt[i] = htons( state_t.p_hw_power_volt[i] );
    }
   
    state_t.n_signal_level      = htonl( state_t.n_signal_level );
    state_t.n_bit_rate          = htonl( state_t.n_bit_rate );
    state_t.n_frequency         = htonl( state_t.n_frequency );

    for( i = 0; i < eAP_INFO_MAX; i++ )
    {
        state_t.p_ap_info[i] = htonl( state_t.p_ap_info[i] );
    }
    
    
    state_t.n_accum_of_captured_count = htonl( state_t.n_accum_of_captured_count );
    state_t.n_captured_count = htonl( state_t.n_captured_count );
    
	state_t.w_battery2_vcell	= htons( state_t.w_battery2_vcell / 10 );
    
    state_t.w_gate_state		= htons( state_t.w_gate_state );

	state_t.w_impact_count		= htons( state_t.w_impact_count );
    state_t.w_impact_count_V2	= htons( state_t.w_impact_count_V2 );
    
    return packet_send( PAYLOAD_STATE, m_n_client_seq_id, \
                        (u8*)&state_t, sizeof(sys_state_t) );
}

/******************************************************************************/
/**
 * @brief   유효한 client에 의해 login되고, 이전에 실행된 영상 전송 prcess가 
 			돌고 있지 않을 때 sender ready 상태가 된다.
 * @param   none
 * @return  true : sender is ready
 			false : sender is not ready
 * @note    none
*******************************************************************************/
bool CControlServer::sender_is_ready(void)
{
    if( m_b_client_valid == true
        && m_b_stream_is_running == false )
    {
        return true;
    }
    
    return false;
}

/******************************************************************************/
/**
 * @brief   전송하려는 영상 정보를 멤버변수에 저장하고 전송 proc 을 생성한다. 
 * @param   image_buffer_t* i_p_image_t : 영상 데이터
 			u32 i_n_payload : 영상 데이터 종류(preview, 일반영상, offset, test image), default -> 일반영상 
 			u32 i_n_crc : default -> 0
 * @return  true : 영상 전송 thread 생성 성공
 			false : 영상 전송 thread 생성 실패
 * @note    
*******************************************************************************/
bool CControlServer::sender_start(image_buffer_t* i_p_image_t, u32 i_n_payload, u32 i_n_crc)
{
    if( m_b_stream_is_running == true )
    {
        print_log(DEBUG, 1, "[Control Server] sender_start is already running" );
        return false;
    }
    
    m_b_stream_is_running = true;
    m_b_stream_sending_error = false;
    
    m_n_stream_payload = i_n_payload;
    m_n_stream_crc = i_n_crc;
    
    m_p_stream_image_t      = i_p_image_t;
    m_n_stream_id           = m_p_stream_image_t->header_t.n_id;
    m_p_system_c->trans_done( false );
    
    m_p_img_retransfer_c->prepare(m_n_stream_id, i_p_image_t->header_t.n_pos_x, i_p_image_t->header_t.n_pos_y, i_p_image_t->header_t.n_pos_z);
    
    print_log(DEBUG, 1, "[Control Server] sender_start image type: 0x%04X, id: 0x%08X\n", \
                            m_n_stream_payload, m_p_stream_image_t->header_t.n_id);
    
    if( pthread_create(&m_stream_thread_t, NULL, stream_routine, ( void * ) this)!= 0 )
    {
    	print_log( ERROR, 1, "[Control Server] stream pthread_create:%s\n",strerror( errno ));

    	m_b_stream_is_running = false;
    	
    	return false;
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
void CControlServer::sender_stop(void)
{
    if( m_b_stream_is_running == false )
    {
        print_log(DEBUG, 1, "[Control Server] sender_stop: sender thread is already terminated.\n");
        return;
    }
    
    print_log(DEBUG, 1, "[Control Server] sender_stop(image id: 0x%08X, sent lenght: %d/%d\n", \
            m_p_stream_image_t->header_t.n_id, m_p_stream_image_t->n_sent_length, \
            m_p_stream_image_t->header_t.n_total_size);
    
    // stop stream thread
    m_b_stream_is_running = false;
    m_b_stream_sending_error = false;
    
	if( pthread_join( m_stream_thread_t, NULL ) != 0)
    {
    	print_log( ERROR, 1,"[Control Server] pthread_join:(stream_proc):%s\n",strerror( errno ));
    }

    m_n_stream_id = 0xFFFFFFFF;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void * stream_routine( void * arg )
{
	CControlServer * svr = (CControlServer *)arg;
	svr->stream_proc();
	pthread_exit( NULL );
	return 0;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CControlServer::stream_proc(void)
{
    print_log(DEBUG, 1, "[Control Server] start sender_proc...\n");
    
    image_buffer_t* p_image_t = m_p_stream_image_t;
    u8*             p_buffer = p_image_t->p_buffer;
    
    u32             n_image_size;
    u32             n_processed_length;
    u32             n_sent_length;
    u32*            p_sent_length;
    u32             n_last_length;
    bool            b_send_result = true;
    bool            b_preview = false;
    
    u32             n_image_id = p_image_t->header_t.n_id;
    bool			b_resend_ready = false;
    
    print_log(DEBUG, 1, "[Control Server] image(id: 0x%08X): %p, buffer: %p\n", \
                            n_image_id, p_image_t, p_buffer);
    
    m_p_system_c->set_oled_display_event(eSEND_IMAGE_EVENT);
    
    // preview 기능이 enable되어 있으면 preview 영상부터 전송
    if( m_p_system_c->preview_get_enable() == true
        && p_image_t->preview_t.p_buffer != NULL
        && m_p_system_c->test_image_is_enabled() == false )
    {
        b_preview = true;
        n_image_size = p_image_t->preview_t.n_total_size;     // preview image size
        p_buffer = p_image_t->preview_t.p_buffer;
    }
    else
    {
        n_image_size = p_image_t->header_t.n_total_size;
        p_buffer = p_image_t->p_buffer;
    }
    
    m_w_stream_slice_total     = n_image_size/PACKET_IMAGE_SIZE;
    if( (n_image_size % PACKET_IMAGE_SIZE) != 0 )
    {
        m_w_stream_slice_total += 1;
    }
    m_w_stream_slice_current   = 0;
    
    if( b_preview == true )
    {
        print_log(DEBUG, 1, "[Control Server] preview image(id: 0x%08X), total slice: %d, image size: %d\n", \
                        n_image_id, m_w_stream_slice_total, n_image_size);
    }
    else
    {
        print_log(DEBUG, 1, "[Control Server] image(id: 0x%08X), total slice: %d, image size: %d\n", \
                        n_image_id, m_w_stream_slice_total, n_image_size);
    }
    
    if( m_n_stream_payload != PAYLOAD_OFFSET_DATA )
    { 
        stream_wait_start();
    }
    
    while( m_b_stream_is_running )
    {
    	if( b_resend_ready == false
    		&& p_image_t->n_processed_length == p_image_t->header_t.n_total_size )
    	{
    		m_p_img_retransfer_c->copy_image( b_preview, p_image_t->preview_t.p_buffer, p_image_t->p_buffer, p_image_t->n_crc );
    		b_resend_ready = true;
    	}
        
        if( b_preview )
        {
            n_processed_length = p_image_t->preview_t.n_processed_length;
            n_sent_length = p_image_t->preview_t.n_sent_length;
            p_sent_length = &p_image_t->preview_t.n_sent_length;
        }
        else
        {
            n_processed_length = p_image_t->n_processed_length;
            n_sent_length = p_image_t->n_sent_length;
            p_sent_length = &p_image_t->n_sent_length;
            
        }
        
        // 아직 전송되지 않았으나, 영상처리 완료되어 전송 가능한 영상 데이터가 packet size이상 될 경우 영상 packet 전송
        if( n_processed_length > (n_sent_length + PACKET_IMAGE_SIZE)  )
        {
            b_send_result = stream_send( b_preview, &p_buffer[n_sent_length], PACKET_IMAGE_SIZE );
            
            *p_sent_length += PACKET_IMAGE_SIZE;
        }
        // 전체 영상의 영상처리가 완료 되었을 때, packet size 이하의 남은 마지막 영상 데이터 전송
        else if( n_processed_length == n_image_size
            && n_processed_length  > n_sent_length )
        {
            n_last_length = n_processed_length - n_sent_length;
            b_send_result = stream_send( b_preview, &p_buffer[n_sent_length], n_last_length );
            
            print_log(DEBUG, 1, "[Control Server] last packet(len: %d)\n", n_last_length);
            
            *p_sent_length += n_last_length;
        }
        else if( n_sent_length == n_image_size )
        {
        	// preview 영상이 전송 완료된 경우 
            if( b_preview )
            {
                print_log(DEBUG, 1, "[Control Server] preview image(id: 0x%08X) sending is complete.\n", \
                                                        n_image_id);
                // FTM 일 경우, preview 영상만 전송하기 때문에 여기서 전송과정을 종료한다.
                if( m_p_system_c->tact_get_enable() == true )
                {
                    break;
                }
                
                // FTM 이 아닌경우, 전송 과정이 종료되지 않고 실제 영상 전송 시작
                b_preview   = false;
                p_buffer    = p_image_t->p_buffer;
                
                n_image_size            = p_image_t->header_t.n_total_size;
                m_w_stream_slice_total  = n_image_size/PACKET_IMAGE_SIZE;
                if( (n_image_size % PACKET_IMAGE_SIZE) != 0 )
                {
                    m_w_stream_slice_total += 1;
                }
                m_w_stream_slice_current   = 0;
                
                print_log(DEBUG, 1, "[Control Server] image(id: 0x%08X), total slice: %d, image size: %d\n", \
                                        n_image_id, m_w_stream_slice_total, n_image_size);
            }
            // 실제 영상 전송 완료된 경우 전송 과정 종료
            else
            {
                print_log(DEBUG, 1, "[Control Server] image(id: 0x%08X) sending is complete.\n", \
                                        n_image_id);
                break;
            }
        }
        else
        {
            sleep_ex(1000);
        }
        
        if( b_send_result == false )
        {
            print_log(DEBUG, 1, "[Control Server] image sending is failed.\n");
            //p_image_t->b_send_started = false;
            m_p_system_c->set_oled_display_event(eACTION_EVENT_END);
            break;
        }
    }


    if( b_resend_ready == false )
    {
    	u16 w_count = 0;
    	
    	while( w_count < 10 )
    	{
	    	if( p_image_t->n_processed_length == p_image_t->header_t.n_total_size )
			{
				m_p_img_retransfer_c->copy_image( true, p_image_t->preview_t.p_buffer, p_image_t->p_buffer, p_image_t->n_crc );
				break;
			}
			sleep_ex(500000);
			w_count++;
		}
		
		print_log(DEBUG, 1, "[Control Server] wait count: %d\n", w_count );
    }
 
    if( b_send_result == true )
    {
		m_p_system_c->set_oled_display_event(eACTION_EVENT_END);
    }
    else
    {
	p_image_t->b_send_started = false;
    	m_b_stream_sending_error = true;
    }

    
    if( m_n_stream_payload == PAYLOAD_OFFSET_DATA )
    {
        print_log(DEBUG, 1, "[Control Server] buffer free: %X\n", m_p_stream_image_t );
        safe_free(m_p_stream_image_t);
    }
    else
    {
        if( b_send_result == true
            && m_p_system_c->tact_get_enable() == false )
        {
        	// 영상 전송을 완료한 후, SW에서 영상 전송 완료 알림이 오기까지 기다린다.
            print_log(DEBUG, 1, "[Control Server] wait image trans done...\n" );
            
            while( m_b_stream_is_running == true )
            {
                if( m_b_client_valid == false )
                {
                    p_image_t->b_send_started = false;
                    break;
                }
                
                sleep_ex(10000);
                alive_reset();
            }
        }
    }
    
    print_log(DEBUG, 1, "[Control Server] stop sender_proc...\n");
    
    pthread_detach(m_stream_thread_t);
    m_b_stream_is_running = false;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CControlServer::stream_send( bool i_b_preview, u8* i_p_buff, u32 i_n_length )
{
    u8                  p_send_buf[PACKET_SIZE];
    packet_header_t*    p_hdr_t;
    bool                b_result;
    
    u32                 n_payload = m_n_stream_payload;
    u8                  p_info[8];
    u16                 w_slice_number;
    u16                 w_slice_total;
    u32                 n_image_id;
    u16                 w_tact_id;
    
    if( n_payload == PAYLOAD_IMAGE
        && i_b_preview == true )
    {
        n_payload = PAYLOAD_PREVIEW_IMAGE;
    }
    
    if( m_p_system_c->test_image_is_enabled() == true )
    {
        n_payload = PAYLOAD_TEST_IMAGE_DATA;
    }
    
    memset( p_send_buf, 0, PACKET_SIZE );
    
    // make header
    p_hdr_t = (packet_header_t *)p_send_buf;
    memcpy( p_hdr_t->p_head, PACKET_HEAD, sizeof(p_hdr_t->p_head) );
    p_hdr_t->w_payload      = htons( n_payload );
    
    if( n_payload == PAYLOAD_PREVIEW_IMAGE )
    {
        w_tact_id               = m_p_system_c->tact_get_id();
        p_hdr_t->c_operation    = (u8)((w_tact_id & 0xFF00)>> 8);
        p_hdr_t->c_result       = (u8)(w_tact_id & 0x00FF);
    }
    
    // make image info
    w_slice_number  = m_w_stream_slice_current;
    w_slice_total   = m_w_stream_slice_total;
    n_image_id      = m_p_stream_image_t->header_t.n_id;
    
    w_slice_number  = htons( w_slice_number );
    w_slice_total   = htons( w_slice_total );
    n_image_id      = htonl( n_image_id );
    
    memcpy( &p_info[0], &w_slice_number, 2 );
    memcpy( &p_info[2], &w_slice_total,  2 );
    memcpy( &p_info[4], &n_image_id,     4 );
    
    memcpy( &p_send_buf[sizeof(packet_header_t)], p_info, sizeof(p_info) );
    
    // copy image data
    memcpy( &p_send_buf[sizeof(packet_header_t) + sizeof(p_info)], i_p_buff, i_n_length );
    
    lock_send();
    b_result = send_image( m_n_client_fd, p_send_buf, PACKET_SIZE );
    unlock_send();
    
    if( b_result == true )
    {
        alive_reset();
    }
    
    if( m_w_stream_slice_total == (m_w_stream_slice_current + 1) )
    {
        print_log(DEBUG, 1, "[Control Server] last slice: %d\n", m_w_stream_slice_current );
    }
    
    m_w_stream_slice_current++;
    
    return b_result;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CControlServer::stream_wait_start(void)
{
    u32 n_wait = 0;
    while( m_b_stream_send_enable == false
            && m_b_stream_is_running == true )
    {
        sleep_ex(10000);
        n_wait++;
    }
    
    //if( m_b_stream_is_running == false )
    {
        print_log(ERROR, 1, "[Control Server] acq start notification wait time: %d ms\n", n_wait * 10 );
    }
    
    m_b_stream_send_enable = false;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void * trans_test_routine( void * arg )
{
	CControlServer * svr = (CControlServer *)arg;
	svr->trans_proc();
	pthread_exit( NULL );
	return 0;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CControlServer::trans_proc(void)
{
    u32 n_count = 0;
    
    print_log(DEBUG, 1, "[Control Server] start trans_proc...\n");
    
    m_w_trans_slice_id = 0;
    
    while( m_b_trans_is_running )
    {
        if( trans_send() == false )
        {
            break;
        }
        n_count++;
    }
    
    print_log(DEBUG, 1, "[Control Server] stop trans_proc...(count: %d)\n", n_count);
    
    pthread_detach(m_trans_thread_t);
    m_b_trans_is_running = false;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CControlServer::trans_start(void)
{
    m_b_trans_is_running = true;
    
    if( pthread_create(&m_trans_thread_t, NULL, trans_test_routine, ( void * ) this)!= 0 )
    {
    	print_log( ERROR, 1, "[Control Server] trans pthread_create:%s\n",strerror( errno ));

    	m_b_trans_is_running = false;
    	
    	return false;
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
void CControlServer::trans_stop(void)
{
    if( m_b_trans_is_running == false )
    {
        print_log(DEBUG, 1, "[Control Server] trans_stop: trans thread is already terminated.\n");
        return;
    }
    
    print_log(DEBUG, 1, "[Control Server] trans_stop.\n");
    
    // stop stream thread
    m_b_trans_is_running = false;
    
	if( pthread_join( m_trans_thread_t, NULL ) != 0)
    {
    	print_log( ERROR, 1,"[Control Server] pthread_join:(trans_proc):%s\n",strerror( errno ));
    }

}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CControlServer::trans_send(void)
{
    u8                  p_send_buf[PACKET_SIZE];
    packet_header_t*    p_hdr_t;
    bool                b_result;
    
    u8                  p_info[8];
    u16                 w_slice_number;

    memset( p_send_buf, 0, PACKET_SIZE );
    
    // make header
    p_hdr_t = (packet_header_t *)p_send_buf;
    memcpy( p_hdr_t->p_head, PACKET_HEAD, sizeof(p_hdr_t->p_head) );
    p_hdr_t->w_payload      = htons( PAYLOAD_TEST_DATA );
    
    // make image info
    w_slice_number  = m_w_trans_slice_id;
    
    w_slice_number  = htons( w_slice_number );
    
    memcpy( &p_info[0], &w_slice_number, 2 );
    
    memcpy( &p_send_buf[sizeof(packet_header_t)], p_info, sizeof(p_info) );
    
    lock_send();
    b_result = sender( m_n_client_fd, p_send_buf, PACKET_SIZE );
    unlock_send();
    
    if( b_result == true )
    {
        alive_reset();
    }
    
    m_w_trans_slice_id++;
    
    return b_result;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CControlServer::message_send( MSG i_msg_e, u8* i_p_data, u16 i_w_data_len )
{
    if( i_w_data_len > VW_PARAM_MAX_LEN )
    {
        print_log(ERROR, 1, "[Control Server] message data lenth is too long(%d/%d).\n", \
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
        print_log(ERROR, 1, "[Control Server] message_send(%d) failed\n", i_msg_e );
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CControlServer::retransmit( u32 i_n_image_id, bool i_b_preview, u8* i_p_buff, u32 i_n_length, u16 i_w_slice_number, u16 i_w_total_slice_number )
{
    u8                  p_send_buf[PACKET_SIZE];
    packet_header_t*    p_hdr_t;
    bool                b_result;
    
    u32                 n_payload = m_n_stream_payload;
    u8                  p_info[8];
    u16                 w_slice_number;
    u16                 w_slice_total;
    u32                 n_image_id;
    u16                 w_tact_id;
    
    if( n_payload == PAYLOAD_IMAGE
        && i_b_preview == true )
    {
        n_payload = PAYLOAD_PREVIEW_IMAGE;
    }
    
    if( m_p_system_c->test_image_is_enabled() == true )
    {
        n_payload = PAYLOAD_TEST_IMAGE_DATA;
    }
    
    memset( p_send_buf, 0, PACKET_SIZE );
    
    // make header
    p_hdr_t = (packet_header_t *)p_send_buf;
    memcpy( p_hdr_t->p_head, PACKET_HEAD, sizeof(p_hdr_t->p_head) );
    p_hdr_t->w_payload      = htons( n_payload );
    
    if( n_payload == PAYLOAD_PREVIEW_IMAGE )
    {
        w_tact_id               = m_p_system_c->tact_get_id();
        p_hdr_t->c_operation    = (u8)((w_tact_id & 0xFF00)>> 8);
        p_hdr_t->c_result       = (u8)(w_tact_id & 0x00FF);
    }
    
    // make image info
    w_slice_number  = i_w_slice_number;
    w_slice_total   = i_w_total_slice_number;
    n_image_id      = i_n_image_id;
    
    w_slice_number  = htons( w_slice_number );
    w_slice_total   = htons( w_slice_total );
    n_image_id      = htonl( n_image_id );
    
    memcpy( &p_info[0], &w_slice_number, 2 );
    memcpy( &p_info[2], &w_slice_total,  2 );
    memcpy( &p_info[4], &n_image_id,     4 );
    
    memcpy( &p_send_buf[sizeof(packet_header_t)], p_info, sizeof(p_info) );
    
    // copy image data
    memcpy( &p_send_buf[sizeof(packet_header_t) + sizeof(p_info)], i_p_buff, i_n_length );
    
    lock_send();
    b_result = send_image( m_n_client_fd, p_send_buf, PACKET_SIZE );
    unlock_send();
    
    if( b_result == true )
    {
        alive_reset();
    }
    
    if( i_w_total_slice_number == (i_w_slice_number + 1) )
    {
        print_log(DEBUG, 1, "[Control Server] retransmit last slice: %d\n", i_w_slice_number );
    }
    
    return b_result;
}
