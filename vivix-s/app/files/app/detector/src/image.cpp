/*******************************************************************************
 모  듈 : image
 작성자 : 한명희
 버  전 : 0.0.1
 설  명 : image buffer
 참  고 : 

 버  전:
         0.0.1  최초 작성
*******************************************************************************/



/*******************************************************************************
*	Include Files
*******************************************************************************/
#include <iostream>
#include <stdio.h>			// fprintf()
#include <string.h>			// memset() memset/memcpy
#include <stdlib.h>			// malloc(), free(),exit(), EXIT_SUCCESS rand/rand 
#include <stdbool.h>		// bool, true, false
#include <assert.h>
#include <errno.h>			// errno

#include <linux/unistd.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "image.h"
#include "vw_file.h"
#include "misc.h"

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
* \param        log         log function
* \return       none
* \note
*******************************************************************************/
CImage::CImage(int (*log)(int,int,const char *,...))
{
    print_log = log;
    
    sys_command("mkdir %s", SYS_IMAGE_DATA_PATH);
    
    memset( m_p_buffer_t, 0, sizeof(m_p_buffer_t) );
    
    m_n_front = m_n_rear = 0;
    
    m_b_save_half = false;
    
    m_p_web_c = NULL;

    m_b_preview_started = false;

    m_p_system_c = NULL;
    
    m_p_save_image_t = (image_buffer_t* ) malloc( sizeof(image_buffer_t) );
    memset( m_p_save_image_t, 0, sizeof(image_buffer_t) );
        
    // launch capture thread
    m_b_is_running = true;
	if( pthread_create(&m_thread_t, NULL, image_routine, ( void * ) this)!= 0 )
    {
    	print_log( ERROR, 1, "[Image] pthread_create:%s\n",strerror( errno ));

    	m_b_is_running = false;
    }
}
/******************************************************************************/
/**
* \brief        constructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CImage::CImage(void)
{
}

/******************************************************************************/
/**
* \brief        destructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CImage::~CImage()
{
    m_b_is_running = false;
	if( pthread_join( m_thread_t, NULL ) != 0 )
	{
		print_log( ERROR, 1,"[Image] pthread_join: image_proc:%s\n",strerror( errno ));
	}
    
    safe_free( m_p_save_image_t->p_buffer );
    safe_free( m_p_save_image_t );
    
    print_log(DEBUG, 1, "[Image] ~CImage\n");
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void * image_routine( void * arg )
{
	CImage * image = (CImage *)arg;
	image->image_proc();
	pthread_exit( NULL );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CImage::image_proc(void)
{
    print_log(DEBUG, 1, "[Image] start of image proc...(%d)\n", syscall( __NR_gettid ));
    image_buffer_t* p_image_t;
    
    ACQUSITION_STATE state_e, state_old_e;
    
    while(m_b_is_running)
    {
    	// 큐에 영상데이터가 하나 이상 있으면 전송 혹은 저장 시작
        if( is_empty() == false )
        {
            p_image_t = m_p_buffer_t[m_n_front];
            
            if( p_image_t->n_processed_length > 0 )
            {
                state_e     = eACQ_STATE_START;
                state_old_e = eACQ_STATE_START;
                print_state( state_e );
                
                while(1)
                {
                    if( state_e != state_old_e )
                    {
                        print_state( state_e );
                        state_old_e = state_e;
                        
                        if( state_e == eACQ_STATE_END )
                        {
                            break;
                        }
                    }
                    else
                    {
                        sleep_ex(1);
                    }
                    
                    switch( state_e )
                    {
                        case eACQ_STATE_START:
                        {
                            state_e = get_initial_acq_state(p_image_t);
                            break;
                        }
                        case eACQ_STATE_SEND:
                        {
                            state_e = check_send_complete(p_image_t);
                            break;
                        }
                        case eACQ_STATE_SEND_WAIT_END:
                        {
                            state_e = check_send_wait_end(p_image_t);
                            break;
                        }
                        case eACQ_STATE_SAVE:
                        {
                            state_e = check_backup_image(p_image_t);
                            break;
                        }
                        case eACQ_STATE_IMAGE_ERROR:
                        {
                            state_e = check_error_image(p_image_t);
                            break;
                        }
                        case eACQ_STATE_END:
                        {
                            break;
                        }
                    } /* end of switch( state_e ) */
                } /* end of while(1) */
            }
            else
            {
                sleep_ex(1000);
            }
        }
        else
        {
            sleep_ex(10000);
        }
    }
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CImage::copy_to_memory( image_buffer_t* i_p_image_t )
{
    if( m_p_save_image_t->p_buffer == NULL )
    {
        m_p_save_image_t->p_buffer = (u8*) malloc( i_p_image_t->header_t.n_total_size );
        m_p_save_image_t->preview_t.p_buffer = (u8*) malloc( i_p_image_t->preview_t.n_total_size );
        print_log(DEBUG, 1, "[Image] image save buffer is allocated.\n");
    }
    
    memcpy( &m_p_save_image_t->header_t, &i_p_image_t->header_t, sizeof(image_header_t) );

    memcpy( m_p_save_image_t->p_buffer, i_p_image_t->p_buffer, i_p_image_t->header_t.n_total_size );
    
    if( m_p_system_c->preview_get_enable() == true )
    {
        memcpy( m_p_save_image_t->preview_t.p_buffer, i_p_image_t->preview_t.p_buffer, i_p_image_t->preview_t.n_total_size );
    }
    
    m_p_system_c->set_copy_image(m_p_save_image_t);
    
    print_log(ERROR, 1, "[Image(id: 0x%08X)] copy to memory(addr: %p, %p)\n", \
                    m_p_save_image_t->header_t.n_id, i_p_image_t, i_p_image_t->p_buffer);
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CImage::save_to_flash( image_buffer_t* i_p_image_t )
{
    bool    b_result;
    
    m_p_system_c->set_oled_display_event(eSAVE_IMAGE_EVENT);
    
    copy_to_memory(i_p_image_t);
    
    if( m_p_system_c->tact_get_enable() == true
        && m_p_system_c->test_image_is_enabled() == false )
    {
		if( m_b_save_half == false )
        {
        	b_result = m_p_system_c->tact_save_image( m_p_save_image_t->header_t, \
            	                        m_p_save_image_t->p_buffer );
		}
		else
        {
        	b_result = m_p_system_c->tact_save_image( m_p_save_image_t->header_t, \
                                        m_p_save_image_t->p_buffer, true );
		}
    }
    else
    {
        if( m_p_system_c->test_image_is_enabled() == true )
        {
            m_p_system_c->test_image_enable( false );
			m_p_system_c->set_oled_display_event(eACTION_EVENT_END);
            return;
        }
        
        b_result = m_p_system_c->backup_image_save( m_p_save_image_t->header_t, \
                                    m_p_save_image_t->p_buffer );
        
        m_p_system_c->test_image_enable( false );
    }
    
    if( b_result == true )
    {
        print_log(INFO, 1, "[Image(id: 0x%08X)] image is saved.\n", m_p_save_image_t->header_t.n_id);
        
        state_message(eIMG_STATE_SAVED, m_p_save_image_t);
        m_p_system_c->set_oled_display_event(eACTION_EVENT_END);
    }
    else
    {
        print_log(INFO, 1, "[Image(id: 0x%08X)] image save failed.\n", m_p_save_image_t->header_t.n_id);
        state_message(eIMG_STATE_SAVE_FAILED, m_p_save_image_t);
        m_p_system_c->set_oled_display_event(eACTION_EVENT_END);
    }
}

/******************************************************************************/
/**
* \brief        image queue 에서 처리(전송 또는 저장) 완료한 image 를 삭제, 자원 정리
* \param        none
* \return       none
* \note
*******************************************************************************/
void CImage::delete_image( image_buffer_t* i_p_image_t )
{
    print_log(DEBUG, 1, "[Image(id: 0x%08X)] delete image from Queue(%d)(addr: %p, %p)\n", \
                    i_p_image_t->header_t.n_id, m_n_front, i_p_image_t, i_p_image_t->p_buffer);
    
    print_log(DEBUG, 1, "[Image(id: 0x%08X)] free memory of image (addr: %p, %p)\n", \
                        i_p_image_t->header_t.n_id, i_p_image_t, i_p_image_t->p_buffer);
    
    safe_delete( i_p_image_t->p_capture_start_time_c );
    safe_delete( i_p_image_t->p_complete_time_c );
    
    safe_free( i_p_image_t->preview_t.p_buffer );
    safe_free( i_p_image_t->p_buffer );
    safe_free( i_p_image_t );
    
    m_n_front = (m_n_front + 1) % IMAGE_BUFFER_COUNT;
    
}

/******************************************************************************/
/**
* \brief        멤버 큐에 영상 정보를 저장한다.
* \param        image_buffer_t* i_p_image_t : 영상 정보
* \return       none
* \note
*******************************************************************************/
void CImage::add_image( image_buffer_t* i_p_image_t )
{
    m_p_buffer_t[m_n_rear] = i_p_image_t;
    m_n_rear = (m_n_rear + 1) % IMAGE_BUFFER_COUNT;
    
    if( m_n_front == m_n_rear )
    {
        print_log(ERROR, 1, "[Image] buffer is full.\n");
    }
    else
    {
        print_log(DEBUG, 1, "[Image(id: 0x%08X)] add image(addr: %p, %p)\n", \
                    i_p_image_t->header_t.n_id, i_p_image_t, i_p_image_t->p_buffer);
    }
}

/******************************************************************************/
/**
* \brief        
* \param        i_state_e: 출력할 상태 값
* \return       none
* \note         
*******************************************************************************/
void CImage::print_state( ACQUSITION_STATE i_state_e )
{
    switch( i_state_e )
    {
        case eACQ_STATE_START:
        {
            print_log(INFO, 1, "[Image] eACQ_STATE_START\n");
            break;
        }
        case eACQ_STATE_SEND:
        {
            print_log(INFO, 1, "[Image] eACQ_STATE_SEND\n");
            break;
        }
        case eACQ_STATE_SEND_WAIT_END:
        {
            print_log(INFO, 1, "[Image] eACQ_STATE_SEND_WAIT_END\n");
            break;
        }
        case eACQ_STATE_SAVE:
        {
            print_log(INFO, 1, "[Image] eACQ_STATE_SAVE\n");
            break;
        }
        case eACQ_STATE_END:
        {
            print_log(INFO, 1, "[Image] eACQ_STATE_END\n");
            break;
        }
        case eACQ_STATE_IMAGE_ERROR:
        {
            print_log(INFO, 1, "[Image] eACQ_STATE_IMAGE_ERROR\n");
            break;
        }
        default:
        {
            print_log(INFO, 1, "[Image] state(%d) is unknown.\n", i_state_e);
        }
    }
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
ACQUSITION_STATE CImage::check_backup_image( image_buffer_t* i_p_image_t )
{
    if( i_p_image_t->n_processed_length == i_p_image_t->header_t.n_total_size )
    {
    	m_p_img_retransfer_c->prepare( i_p_image_t->header_t.n_id, i_p_image_t->header_t.n_pos_x, i_p_image_t->header_t.n_pos_y, i_p_image_t->header_t.n_pos_z);
    	m_p_img_retransfer_c->copy_image( true, i_p_image_t->preview_t.p_buffer, i_p_image_t->p_buffer, i_p_image_t->n_crc );
    	
    	make_preview(i_p_image_t);
    	
        save_to_flash(i_p_image_t);
        
        delete_image(i_p_image_t);
        
        m_p_system_c->capture_end();
        
        state_message(eIMG_STATE_ACQ_END, i_p_image_t);
        
        return eACQ_STATE_END;
    }
    
    // check capture timeout
    if( i_p_image_t->p_capture_start_time_c != NULL
        && i_p_image_t->p_capture_start_time_c->is_expired() == true )
    {
        print_log(ERROR, 1, "[Image(id: 0x%08X)] image capture is not complete(timed out: %d/%d(%p)).\n", \
            i_p_image_t->header_t.n_id, i_p_image_t->n_processed_length, i_p_image_t->header_t.n_total_size, i_p_image_t);
        
        delete_image(i_p_image_t);
        
        m_p_system_c->capture_end();
        
        return eACQ_STATE_END;
    }
    
    return eACQ_STATE_SAVE;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
ACQUSITION_STATE CImage::check_send_complete( image_buffer_t* i_p_image_t )
{
    if( i_p_image_t->n_processed_length < i_p_image_t->header_t.n_total_size )
    {
        if( i_p_image_t->p_capture_start_time_c != NULL
        && i_p_image_t->p_capture_start_time_c->is_expired() == true )
        {
            print_log(ERROR, 1, "[Image(id: 0x%08X)] image capture is not complete(timed out: %d/%d(%p)).\n", \
                i_p_image_t->header_t.n_id, i_p_image_t->n_processed_length, i_p_image_t->header_t.n_total_size, i_p_image_t);
            
            state_message(eIMG_STATE_ACQ_TIMEOUT, i_p_image_t);
            
            stop_send_thread();
            
            delete_image(i_p_image_t);
            
            m_p_system_c->capture_end();
            
            return eACQ_STATE_END;
        }
        
        // 영상의 반 이상 획득했을 때 Fast tact-time mode이면 영상 저장 시작
        if( i_p_image_t->n_processed_length >= ((i_p_image_t->header_t.n_total_size/2))
            && m_b_save_half == false )
        {
            if( m_p_system_c->tact_get_enable() == true
                && m_p_system_c->test_image_is_enabled() == false )
            {
            	// Fast tact-time mode 영상의 앞쪽 절반을 디텍터 내부에 저장
                m_p_system_c->tact_save_image( i_p_image_t->header_t, i_p_image_t->p_buffer, false );
            }
            
            m_b_save_half = true;
        }
        return eACQ_STATE_SEND;
    }
    
    // capture is complete
    if( i_p_image_t->p_complete_time_c == NULL )
    {
        u32 n_wait = get_wait_time();

        make_preview(i_p_image_t);
        
        if( m_p_system_c->tact_get_enable() == true
            && m_p_system_c->test_image_is_enabled() == false )
        {
            print_log(DEBUG, 1, "[Image(id: 0x%08X)] preview complete timer(%d sec/sent: %d) is started.\n", \
                                i_p_image_t->header_t.n_id, n_wait/1000, i_p_image_t->preview_t.n_sent_length);
            
            save_to_flash(i_p_image_t);
        }
        else
        {
            print_log(DEBUG, 1, "[Image(id: 0x%08X)] complete timer(%d sec/sent: %d) is started.\n", \
                                i_p_image_t->header_t.n_id, n_wait/1000, i_p_image_t->n_sent_length);
            
            copy_to_memory(i_p_image_t);
        }
        
        i_p_image_t->p_complete_time_c = new CTime(n_wait);

        m_p_system_c->capture_end();
        
        state_message(eIMG_STATE_ACQ_END, i_p_image_t);
        
        return eACQ_STATE_SEND;
    }
    
    if( m_p_system_c->tact_get_enable() == true
        && m_p_system_c->test_image_is_enabled() == false )
    {
        if( i_p_image_t->preview_t.n_sent_length == (i_p_image_t->preview_t.n_total_size) )
        {
            state_message(eIMG_STATE_SEND_END, i_p_image_t);
            
            stop_send_thread();
            
            delete_image(i_p_image_t);
            
            return eACQ_STATE_END;
        }
    }
    else
    {
        if( i_p_image_t->n_sent_length == i_p_image_t->header_t.n_total_size )
        {
            return eACQ_STATE_SEND_WAIT_END;
        }
    }
    
    if( i_p_image_t->p_complete_time_c->is_expired() == true
        || i_p_image_t->b_send_started == false )
    {
        print_log(ERROR, 1, "[Image(id: 0x%08X)] complete timer is expired or send failed(%d: %d/%d).\n", \
                                i_p_image_t->header_t.n_id, i_p_image_t->b_send_started, \
                                i_p_image_t->n_sent_length, i_p_image_t->header_t.n_total_size);
        
        if( m_p_system_c->tact_get_enable() == true
            && m_p_system_c->test_image_is_enabled() == false )
        {
            print_log(ERROR, 1, "[Image(id: 0x%08X)] complete timer is expired(preview sending is not complete).\n", \
                                i_p_image_t->header_t.n_id);

            state_message(eIMG_STATE_SEND_TIMEOUT, i_p_image_t);
        }
        else
        {
            print_log(ERROR, 1, "[Image(id: 0x%08X)] complete timer is expired(sending is not complete).\n", \
                                    i_p_image_t->header_t.n_id);

            save_to_flash(i_p_image_t);
            state_message(eIMG_STATE_SEND_TIMEOUT, i_p_image_t);
        }
        
        stop_send_thread();

        delete_image(i_p_image_t);
        
        check_queue_state();        
        
        return eACQ_STATE_END;
    }
    
    update_remain_time( i_p_image_t->p_complete_time_c );
    
    return eACQ_STATE_SEND;
}


/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
ACQUSITION_STATE CImage::check_send_wait_end( image_buffer_t* i_p_image_t )
{
    if( m_p_system_c->trans_is_done() == true )
    {
        print_log(DEBUG, 1, "[Image(id: 0x%08X)] image trans done...\n", i_p_image_t->header_t.n_id );
        
        m_p_system_c->test_image_enable( false );
        
        state_message(eIMG_STATE_SEND_END, i_p_image_t);
        stop_send_thread();
        
        delete_image(i_p_image_t);
        
        check_queue_state();
        
        return eACQ_STATE_END;
    }
    
    if( i_p_image_t->p_complete_time_c->is_expired() == true
        || i_p_image_t->b_send_started == false )
    {
        print_log(ERROR, 1, "[Image(id: 0x%08X)] complete timer is expired or send failed(%d).\n", \
                                i_p_image_t->header_t.n_id, i_p_image_t->b_send_started);
        
        save_to_flash(i_p_image_t);
        state_message(eIMG_STATE_SEND_TIMEOUT, i_p_image_t);
        
        stop_send_thread();
        
        delete_image(i_p_image_t);
        
        check_queue_state();
        
        return eACQ_STATE_END;
    }
    
    update_remain_time( i_p_image_t->p_complete_time_c );
    
    return eACQ_STATE_SEND_WAIT_END;
}

/******************************************************************************/
/**
* \brief        Queue buffer 크기의 반 이상이 영상으로 채워지면 flash에 저장한다.
* \param        none
* \return       none
* \note         영상 전송 시, timeout이 발생한 경우에 호출한다.
*******************************************************************************/
void CImage::check_queue_state(void)
{
    image_buffer_t* p_image_t;
    u32 n_image_count = get_count();
    
    if( n_image_count > IMAGE_BUFFER_COUNT/2 )
    {
        print_log(DEBUG, 1, "[Image] start image backup(Q size: %d)\n", n_image_count);
        while( n_image_count > 1 )      // 마지막 버퍼는 사용 중일 수 있으므로 남겨둔다.
        {
            p_image_t = m_p_buffer_t[m_n_front];
            if( p_image_t->n_processed_length == p_image_t->header_t.n_total_size )
            {
                save_to_flash(p_image_t);
                delete_image(p_image_t);
            }
            else if( p_image_t->p_capture_start_time_c != NULL
                    && p_image_t->p_capture_start_time_c->is_expired() == true )
            {
                print_log(ERROR, 1, "[Image(id: 0x%08X)] image capture is not complete(timed out: %d/%d(%p)).\n", \
                    p_image_t->header_t.n_id, p_image_t->n_processed_length, p_image_t->header_t.n_total_size, p_image_t);
                
                state_message(eIMG_STATE_ACQ_TIMEOUT, p_image_t);
                delete_image(p_image_t);
            }
            n_image_count--;
        }
    }
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CImage::set_notify_handler(bool (*noti)(vw_msg_t * msg))
{
    notify_handler = noti;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CImage::state_message( IMAGE_STATE i_state_e, image_buffer_t* i_p_image_t )
{
    vw_msg_t msg;
    bool b_result;
    
    u32 n_state;
    u32 n_image_id = i_p_image_t->header_t.n_id;
    s32 p_pos[3];
    u8	p_correction[4];
    u16 w_direction;
    
    p_pos[0] = i_p_image_t->header_t.n_pos_x;
    p_pos[1] = i_p_image_t->header_t.n_pos_y;
    p_pos[2] = i_p_image_t->header_t.n_pos_z;
    
    p_correction[0] = i_p_image_t->header_t.c_offset;
    p_correction[1] = i_p_image_t->header_t.c_defect;
    p_correction[2] = i_p_image_t->header_t.c_gain;
    p_correction[3] = i_p_image_t->header_t.c_reserved_osf;
    
    w_direction		= i_p_image_t->header_t.w_direction;
    
    memset( &msg, 0, sizeof(vw_msg_t) );
    msg.type = (u16)eMSG_IMAGE_STATE;
    msg.size = sizeof( n_state ) + sizeof( n_image_id );
    
    n_state = i_state_e;
    memcpy( &msg.contents[0], &n_state, sizeof( n_state ) );
    memcpy( &msg.contents[4], &n_image_id, sizeof( n_image_id ) );
    memcpy( &msg.contents[12], p_pos, sizeof( p_pos ) );
    memcpy( &msg.contents[24], p_correction, sizeof( p_correction ) );
    memcpy( &msg.contents[28], &w_direction, sizeof( w_direction ) );
    
    b_result = notify_handler( &msg );
    
    if( b_result == false )
    {
        print_log(ERROR, 1, "[Image] state_message(%d) failed\n", i_state_e );
    }
}


/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CImage::set_send_handler( bool (*is_ready)(void), bool (*start_send)(image_buffer_t* i_p_image_t), void (*stop_send)(void) )
{
    is_ready_to_send    = is_ready;
    start_send_thread   = start_send;
    stop_send_thread    = stop_send;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CImage::get_wait_time(void)
{
    u32 n_time_out;
    
    if( m_p_system_c->tact_get_enable() == true
        && m_p_system_c->test_image_is_enabled() == false )
    {
        return 3000;
    }
    
    n_time_out = atoi( m_p_system_c->config_get(eCFG_IMAGE_TIMEOUT) );
    
    if( n_time_out < 5 )
    {
        return COMPLETE_TIME_OUT_MIN;
    }
    
    return (n_time_out * 1000);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CImage::update_remain_time( CTime* i_p_time_c )
{
	if( i_p_time_c == NULL )
	{
		m_p_system_c->set_image_send_remain_time(get_wait_time());
		return;
	}
	
	m_p_system_c->set_image_send_remain_time(i_p_time_c->get_remain_time());
}

/******************************************************************************/
/**
 * @brief	AED 모드에서 에러영상인지 확인한다.   
 * @param   none
 * @return  true  : 유효한 영상
 			false : Extra flush 구간과 flush 구간에 걸쳐 들어온 AED trigger에 의해 촬영된 영상
 * @note    none
*******************************************************************************/
bool CImage::is_valid_image( image_buffer_t* i_p_image_t )
{
    if( i_p_image_t->w_error == 0 )
    {
        return true;
    }
    
    return false;
}

/******************************************************************************/
/**
 * @brief   획득한 영상의 오류 여부에 따라 영상 전송/저장 상태로 진행하거나, 
 			영상획득 오류상태로 변경한다.			
 * @param   image_buffer_t* i_p_image_t : 획득 영상 데이터 
 * @return  ACQUSITION_STATE : 다음에 진행될 영상 데이터 처리 상태
 * @note    
*******************************************************************************/
ACQUSITION_STATE CImage::get_initial_acq_state( image_buffer_t* i_p_image_t )
{
    m_b_save_half = false;
    m_b_preview_started = false;
    
    // 획득한 영상에 문제가 없으면 영상 전송 또는 저장 상태로 진행한다.
    if( is_valid_image(i_p_image_t) == true )
    {
        state_message(eIMG_STATE_ACQ_START, i_p_image_t);
        
        // Fast tact-time mode일 경우, 현재 영상에 대한 ID를 직전 영상 ID에서 1만큼 더해 부여한다.
        m_p_system_c->tact_update_id();
        
        update_remain_time();
        
        // 영상 전송이 가능한 상태이면 영상 전송 시작
        if( is_ready_to_send() == true )
        {
            m_p_system_c->set_copy_image(NULL);
            
            i_p_image_t->b_send_started = start_send_thread(i_p_image_t);
            
            print_log(DEBUG, 1, "[Image(id: 0x%08X)] sending thread is started.\n", \
                                        i_p_image_t->header_t.n_id);
            
            return eACQ_STATE_SEND;
        }
        else	// 영상 전송이 불가능한 상태이면 디텍터 내부에 영상 저장 시작
        {
            return eACQ_STATE_SAVE;
        }
    }
    
    m_p_system_c->set_oled_display_event(eACTION_EVENT_END);
    // 획득한 영상에 문제가 있으면 SW에 error 상태를 전송한다.
    state_message(eIMG_STATE_ACQ_START_WITH_ERROR, i_p_image_t);
    
    return eACQ_STATE_IMAGE_ERROR;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
ACQUSITION_STATE CImage::check_error_image( image_buffer_t* i_p_image_t )
{
    if( i_p_image_t->n_processed_length == i_p_image_t->header_t.n_total_size )
    {
        state_message(eIMG_STATE_ACQ_END_WITH_ERROR, i_p_image_t);
        
        delete_image(i_p_image_t);
        
        m_p_system_c->capture_end();
        
        return eACQ_STATE_END;
    }
    
    // check capture timeout
    if( i_p_image_t->p_capture_start_time_c != NULL
        && i_p_image_t->p_capture_start_time_c->is_expired() == true )
    {
        print_log(ERROR, 1, "[Image(id: 0x%08X)] image capture is not complete(timed out: %d/%d(%p)).\n", \
            i_p_image_t->header_t.n_id, i_p_image_t->n_processed_length, i_p_image_t->header_t.n_total_size, i_p_image_t);
        
        state_message(eIMG_STATE_ACQ_TIMEOUT, i_p_image_t);
        
        delete_image(i_p_image_t);
        
        m_p_system_c->capture_end();
        
        return eACQ_STATE_END;
    }
    
    return eACQ_STATE_IMAGE_ERROR;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CImage::make_preview( image_buffer_t* i_p_image_t )
{
    if( m_b_preview_started == true )
    {
    	return;
    }
    
    if( (i_p_image_t->preview_t.n_processed_length == i_p_image_t->preview_t.n_total_size)
    	&& m_p_system_c->is_web_ready() == true )
    {
    	m_b_preview_started = true;
    	m_p_web_c->preview_save( i_p_image_t->preview_t.p_buffer, i_p_image_t->preview_t.n_total_size, i_p_image_t->header_t.n_time );
    }
}
