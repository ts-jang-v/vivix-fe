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

#ifdef TARGET_COM
#include <iostream>
#include <stdio.h>			// fprintf()
#include <string.h>			// memset() memset/memcpy
#include <stdlib.h>			// malloc(), free(),exit(), EXIT_SUCCESS rand/rand 
#include <stdbool.h>		// bool, true, false
#include <assert.h>
#include <errno.h>            // errno
#include <sys/poll.h>		// struct pollfd, poll()

#include <linux/unistd.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "capture.h"
#include "message_def.h"
#include "vw_file.h"
#else
#include "typedef.h"
#include "Time.h"
#include "File.h"
#include "Common_def.h"
#include "Thread.h"
#include "Poll.h"
#include "Dev_io.h"
#include "Vw_system.h"
#include "Calibration.h"
#include "../app/detector/include/capture.h"
//#include "Vw_system.h"
//#define _IOWR(a,b,c) b
//#include "../driver/include/vworks_ioctl.h"
#include <string.h>
#endif
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
* \brief        constructor
* \param        log         log function
* \return       none
* \note
*******************************************************************************/
CCapture::CCapture(int (*log)(int,int,const char *,...))
{
    print_log = log;
    strcpy( m_p_log_id, "CAPTURE       " );
    
    m_p_image_t = NULL;
    m_n_line_count = 0;
    
    m_p_system_c = NULL;
    m_n_fd = -1;
    
    m_b_offset_cal = false;
    
    m_p_exp_time_c = NULL;
    m_p_capture_time_c = NULL;

    m_b_is_running = false;

    m_n_fd = -1;
    m_thread_t = (pthread_t)0;
    m_n_image_width_size = 0;
    m_n_image_height = 0;
    m_n_image_pixel_depth = 0;
    m_n_image_size_byte = 0;
    m_n_image_size_pixel = 0;
    m_p_calibration_c = NULL;

}
/******************************************************************************/
/**
* \brief        constructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CCapture::CCapture(void)
{
}

/******************************************************************************/
/**
* \brief        destructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CCapture::~CCapture()
{
    m_b_is_running = false;
	
	if( pthread_join( m_thread_t, NULL ) != 0)
    {
    	print_log( ERROR, 1,"[%s] pthread_join:(capture_proc):%s\n", m_p_log_id, strerror( errno ));
    }
    
    if( m_p_image_t != NULL )
    {
        safe_free( m_p_image_t->preview_t.p_buffer );
        safe_free( m_p_image_t->p_buffer );
        
        safe_delete( m_p_image_t->p_capture_start_time_c );
        safe_delete( m_p_image_t->p_complete_time_c );
        
        safe_free( m_p_image_t );
    }
    
    print_log( DEBUG, 1, "[%s] ~CCapture\n", m_p_log_id );
}

/******************************************************************************/
/**
* \brief        capture thread 를 생성한다.
* \param        none
* \return       none
* \note					 
*******************************************************************************/
void CCapture::start(void)
{
    image_size_t size;
    memset( &size, 0, sizeof(image_size_t) );
    
    m_p_system_c->get_image_size_info( &size );
    
    m_n_image_width_size    = size.width * size.pixel_depth;
    m_n_image_height        = size.height;
    m_n_image_size_byte     = m_n_image_width_size * m_n_image_height;
    
    print_log(DEBUG, 1, "[%s] image width: %d, height: %d, pixel depth: %d, size: %dbytes\n",
                        m_p_log_id, size.width, size.height, size.pixel_depth, m_n_image_size_byte );
    
    m_n_fd = m_p_system_c->dev_get_fd(eDEV_ID_FPGA);
    
    // launch capture thread
    m_b_is_running = true;
	if( pthread_create(&m_thread_t, NULL, capture_routine, ( void * ) this)!= 0 )
    {
    	print_log( ERROR, 1, "[%s] pthread_create:%s\n", m_p_log_id, strerror( errno ));

    	m_b_is_running = false;
    }

}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CCapture::image_buffer_alloc(void)
{
    m_p_image_t = (image_buffer_t*)malloc( sizeof(image_buffer_t) );
    memset( m_p_image_t, 0, sizeof(image_buffer_t) );
    
    m_p_image_t->header_t.n_width_size  = m_n_image_width_size;
    m_p_image_t->header_t.n_height      = m_n_image_height;
    m_p_image_t->header_t.n_total_size  = m_n_image_size_byte;
    
    m_p_image_t->p_buffer = (u8*)malloc( m_n_image_size_byte );
    memset( m_p_image_t->p_buffer, 0, m_n_image_size_byte );
    
    u32 n_preview_size = m_n_image_size_byte/PREVIEW_SCALE;
    
    m_p_image_t->preview_t.n_total_size = n_preview_size;
    m_p_image_t->preview_t.p_buffer     = (u8*)malloc( n_preview_size );
    memset( m_p_image_t->preview_t.p_buffer, 0, n_preview_size );

    print_log(DEBUG, 1, "[%s] alloc image buffer(addr: %p)\n", m_p_log_id, m_p_image_t);
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void * capture_routine( void * arg )
{
	CCapture * capture = (CCapture *)arg;
	capture->capture_proc();
	pthread_exit( NULL );
	return 0;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
TG_STATE CCapture::state_idle(u8* i_p_line)
{
    s32                 ret;
    struct pollfd		events[1];
    s32					n_errno;
    
    memset(events,0,sizeof(events));
    events[0].fd = m_n_fd;
    events[0].events = POLLIN | POLLPRI;

    state_idle_check();
    
    ret = poll( (struct pollfd *)&events, 1, 1 );
    
    if(ret < 0)
    {
    	n_errno = errno;
    	if( n_errno != EAGAIN && n_errno != EINTR )
    	{
    		print_log(ERROR, 1, "[%s] poll error(%s)\n", m_p_log_id, strerror( n_errno ));
    	}
    	
        return eTG_STATE_IDLE;
    }
    
    if(events[0].revents & POLLPRI)
    {   	
        check_offset_calibration();
        image_state_message(eIMG_STATE_EXPOSURE);
        m_p_system_c->set_oled_display_event(eACQ_IMAGE_EVENT);
        m_p_system_c->read_detector_position();
        m_p_system_c->impact_stable_proc_hold();
        return eTG_STATE_EXPOSURE;
    }
    
    if( m_p_calibration_c->offset_cal_is_started() == true )
    {
        return eTG_STATE_OFFSET_LINE_ACQ;
    }
    
    if( m_p_system_c->osf_is_started() == true )
    {
        return eTG_STATE_OFFSET_DRIFT_ACQ;
    }
    
    
    return eTG_STATE_IDLE;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
TG_STATE CCapture::state_exposure(void)
{
    s32                 ret;
    struct pollfd		events[1];
    s32					n_errno;
    
    memset(events,0,sizeof(events));
    events[0].fd = m_n_fd;
    events[0].events = POLLIN ;
    
    ret = poll( (struct pollfd *)&events, 1, 1 );
    
    if(ret < 0)
    {
    	n_errno = errno;
        if( n_errno != EAGAIN && n_errno != EINTR )
        {
        	print_log(ERROR, 1, "[%s] state_exposure poll error(%s)\n", m_p_log_id, strerror( n_errno )); 
        }
        
        return eTG_STATE_EXPOSURE;
    }
    
    if(events[0].revents & POLLIN)
    {
        safe_delete( m_p_exp_time_c );
        
        // EXPOSURE 상태부터 실제 영상 데이터가 들어오기까지의 timeout
        safe_delete(m_p_capture_time_c);
        m_p_capture_time_c = new CTime( 5000 );
        
        return eTG_STATE_EXP_LINE_ACQ;
    }
    
    if( m_p_exp_time_c == NULL )
    {
        create_timer_for_exposure_timeout();
    }
    else 
    {
        if (m_p_exp_time_c->is_expired() == true)
        {
            safe_delete(m_p_exp_time_c);

            if( m_p_system_c->dev_is_ready() == false )
            {
                return eTG_STATE_EXPOSURE;
            }
            
            print_log(ERROR, 1, "[%s] exposure timeout\n", m_p_log_id);
            image_state_message(eIMG_STATE_EXPOSURE_TIMEOUT);
            return eTG_STATE_IDLE;
        }
    }
    
    return eTG_STATE_EXPOSURE;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
TG_STATE CCapture::state_acq(void)
{
    s32                 ret;
    
    struct pollfd		events[1];
    s32					n_errno;
    
    memset(events,0,sizeof(events));
    events[0].fd = m_n_fd;
    events[0].events = POLLIN | POLLPRI;

    if( m_p_capture_time_c->is_expired() == true )
	{
		print_log(DEBUG, 1, "[%s] image capture timeout.\n", m_p_log_id);
		safe_delete(m_p_capture_time_c);
		
		m_p_system_c->restart_battery_saving_mode();
		m_p_system_c->print_line_error();
		
		image_state_message(eIMG_STATE_ACQ_TIMEOUT);
		
		return eTG_STATE_IDLE;
	}
    
    ret = poll( (struct pollfd *)&events, 1, 1 );
    
    if(ret < 0)
    {
    	n_errno = errno;
    	if( n_errno != EAGAIN && n_errno != EINTR )
        {
        	print_log(ERROR, 1, "[%s] state_acq poll error(%s)\n", m_p_log_id, strerror( n_errno )); 
        }
        
        return eTG_STATE_EXP_LINE_ACQ;
    }
    
    if(events[0].revents & POLLIN)
    {
        u8* p_buffer;
        
        p_buffer = (u8*)&m_p_image_t->p_buffer[m_n_line_count * m_n_image_width_size];
        
        ret = read( m_n_fd, p_buffer, m_n_image_width_size);
        if( ret != (s32)m_n_image_width_size )
        {
            print_log(ERROR, 1, "[%s] A line is not complete(%d/%d).\n", m_p_log_id, ret, m_n_image_width_size);
            return eTG_STATE_EXP_LINE_ACQ;
        }
        
        if( m_n_line_count == 0 )
        {
            make_info( m_p_image_t );

            m_n_line_count = 0;
            
            print_log(DEBUG, 1, "[%s] acq started(id: 0x%08X)\n", m_p_log_id, m_p_image_t->header_t.n_id);
            
            // 영상 들어오기 시작할 때부터 영상이 모두  들어오기까지 timeout
            m_p_image_t->p_capture_start_time_c = new CTime(CAPTURE_TIME_OUT);
            
            // CImage의 멤버 큐에 영상 정보를 넣음
            add_image_handler(m_p_image_t);
            m_p_system_c->defect_correction_idx_reset();
            m_p_calibration_c->prepare_correction();
        }
        
        // line 단위로 영상 처리 (preview image 생성, defect correction )
        process_image( m_p_image_t );
        
        if( m_p_image_t->n_processed_length == m_n_image_size_byte )
        {

	        print_log(DEBUG, 1, "[%s] image capture is complete(id: 0x%08X, exp time: %d ms).\n", \
	                            m_p_log_id, m_p_image_t->header_t.n_id, m_p_system_c->get_measrued_exposure_time() );

            print_log(DEBUG, 1, "[%s] exp time: XRAY_FRAME(%dms), OFFSET_FRAME(%dms)\n", m_p_log_id, m_p_system_c->get_frame_exposure_time(1), m_p_system_c->get_frame_exposure_time(2));
            
            m_n_line_count = 0;
            
            print_log(DEBUG, 1, "[%s] image crc: 0x%08X\n", m_p_log_id, m_p_image_t->n_crc);
            
            // alloc memory for the next image acquisition
            image_buffer_alloc();
            return eTG_STATE_IDLE;
        }
    }
    
    return eTG_STATE_EXP_LINE_ACQ;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
TG_STATE CCapture::state_acq_offset(u16* i_p_line)
{
    s32                 ret;
    struct pollfd		events[1];
    
    memset(events,0,sizeof(events));
    events[0].fd = m_n_fd;
    events[0].events = POLLIN | POLLPRI;
    
    ret = poll( (struct pollfd *)&events, 1, 1 );
    
    if(ret < 0)
    {
        print_log(ERROR, 1, "[%s] state_acq_offset poll error(%s)\n", m_p_log_id, strerror( errno ));
        if( errno != EAGAIN && errno != EINTR )
        {
            m_p_calibration_c->offset_cal_message(eOFFSET_CAL_ACQ_CANCELLED);
            return eTG_STATE_IDLE;
        }
        else
        {
            return eTG_STATE_OFFSET_LINE_ACQ;
        }
    }
    
    if(events[0].revents & POLLPRI)
    {
        print_log(ERROR, 1, "[%s] X-ray exposure in offset calibration mode.\n", m_p_log_id);
        
        m_p_calibration_c->offset_cal_message(eOFFSET_CAL_ACQ_CANCELLED);
        
        return eTG_STATE_EXPOSURE;
    }
    
    if(events[0].revents & POLLIN)
    {
        ret = read( m_n_fd, (u8*)i_p_line, m_n_image_width_size);
        if( ret != (s32)m_n_image_width_size )
        {
            print_log(ERROR, 1, "[%s] Offset line is not complete(%d/%d).\n", m_p_log_id, ret, m_n_image_width_size);
            
            m_p_calibration_c->offset_cal_message(eOFFSET_CAL_ACQ_CANCELLED);
            return eTG_STATE_IDLE;
        }
        
        if( m_p_calibration_c->offset_cal_accumulate(i_p_line, m_n_image_width_size) == true )
        {
            return eTG_STATE_IDLE;
        }
    }

    if( m_p_calibration_c->offset_cal_is_started() == false )
    {
        return eTG_STATE_IDLE;
    }
    else
    {
    	if( m_p_calibration_c->offset_cal_is_timeout() == true )
        {
            print_log(INFO, 1, "[%s] offset calibration is cancelled by timeout.\n", m_p_log_id);
            //m_p_system_c->offset_cal_stop(true);
            m_p_calibration_c->offset_cal_message(eOFFSET_CAL_ACQ_TIMEOUT);
        }
    }
    
    return eTG_STATE_OFFSET_LINE_ACQ;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
TG_STATE CCapture::state_acq_offset_drift(u16* i_p_line)
{
    if( m_p_system_c->osf_is_started() == false )
    {
        return eTG_STATE_IDLE;
    }

    s32                 ret;
    struct pollfd		events[1];
    
    memset(events,0,sizeof(events));
    events[0].fd = m_n_fd;
    events[0].events = POLLIN | POLLPRI;
    
    ret = poll( (struct pollfd *)&events, 1, 1 );
    
    if(ret < 0)
    {
        print_log(ERROR, 1, "[%s] state_acq_offset_drift poll error(%s)\n", m_p_log_id, strerror( errno ));
        if( errno != EAGAIN && errno != EINTR )
        {
            //m_p_system_c->offset_cal_message(eOFFSET_CAL_ACQ_CANCELLED);
            m_p_system_c->osf_message(eOFFSET_DRIFT_ACQ_CANCELLED);
            return eTG_STATE_IDLE;
        }
        else
        {
            return eTG_STATE_OFFSET_DRIFT_ACQ;
        }
    }
    
    if(events[0].revents & POLLIN)
    {
        ret = read( m_n_fd, (u8*)i_p_line, m_n_image_width_size);
        if( ret != (s32)m_n_image_width_size )
        {
            print_log(ERROR, 1, "[%s] Offset drift line is not complete(%d/%d).\n", m_p_log_id, ret, m_n_image_width_size);
            return eTG_STATE_IDLE;
        }
        
        if( m_p_system_c->osf_accumulate(i_p_line, m_n_image_width_size) == true )
        {
            return eTG_STATE_IDLE;
        }
    }
    
    return eTG_STATE_OFFSET_DRIFT_ACQ;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CCapture::capture_proc(void)
{
    print_log(DEBUG, 1, "[%s] start proc...(%d)\n", m_p_log_id, syscall( __NR_gettid ));
    u8*    p_line_buffer;
    
    // offset, invalid line
    p_line_buffer = (u8*)malloc( m_n_image_width_size );
    memset( p_line_buffer, 0, m_n_image_width_size );
    
    TG_STATE        tg_state_e = eTG_STATE_IDLE;
    TG_STATE        tg_state_old_e = eTG_STATE_IDLE;
    
    image_buffer_alloc();
    
    m_n_line_count = 0;
    state_print(tg_state_e);
    while( m_b_is_running )
    {
        if( tg_state_e != tg_state_old_e )
        {
            state_print(tg_state_e);
            tg_state_old_e = tg_state_e;
        }
        
        switch( (u8)tg_state_e )
        {
            case eTG_STATE_IDLE:
            {
                tg_state_e = state_idle(p_line_buffer);
                break;
            }
            case eTG_STATE_EXPOSURE:
            {
                tg_state_e = state_exposure();
                break;
            }
            case eTG_STATE_OFFSET_LINE_ACQ:
            {
                tg_state_e = state_acq_offset((u16*)p_line_buffer);
                break;
            }
            case eTG_STATE_EXP_LINE_ACQ:
            {
                tg_state_e = state_acq();
                break;
            }
            case eTG_STATE_OFFSET_DRIFT_ACQ:
            {
                tg_state_e = state_acq_offset_drift((u16*)p_line_buffer);
                break;
            }
            default:
            {
                break;
            }
        }
    } /* end of while( m_b_is_running ) */
    safe_free( p_line_buffer );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CCapture::state_idle_check(void)
{
    if( m_n_line_count > 0
        && m_n_line_count < m_n_image_height )
    {
        print_log(DEBUG, 1, "[%s] incomplete m_n_line_count(%d/%d)\n", m_p_log_id, m_n_line_count, m_n_image_height);
        m_n_line_count = 0;
        
        image_buffer_alloc();
    }
    
    if( m_p_calibration_c->offset_cal_is_started() == true )
    {
        if( m_p_calibration_c->offset_cal_is_timeout() == true )
        {
            print_log(INFO, 1, "[%s] offset calibration is cancelled by timeout.\n", m_p_log_id);
            //m_p_system_c->offset_cal_stop(true);
            m_p_calibration_c->offset_cal_message(eOFFSET_CAL_ACQ_TIMEOUT);
        }
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    Preview 이미지는 1/16배율로, 4x4 구역의 첫번째 pixel 값만을 사용한다.
*******************************************************************************/
void CCapture::make_preview( image_buffer_t* i_p_image_t, u32 i_n_pos )
{
    u32 i;
	u16* src;
	u16* dst;
	
	src = (u16 *)&i_p_image_t->p_buffer[i_n_pos];
	dst = (u16 *)&i_p_image_t->preview_t.p_buffer[i_p_image_t->preview_t.n_processed_length];
	
	for( i = 0; i < m_n_image_width_size/2; i+=4 )
	{
		dst[i/4] = src[i];
	}
	
	i_p_image_t->preview_t.n_processed_length += m_n_image_width_size/4;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CCapture::process_image( image_buffer_t* i_p_image_t )
{
	m_p_calibration_c->apply_offset((u16 *)i_p_image_t->p_buffer, m_n_line_count*m_n_image_width_size>>1, m_n_image_width_size>>1);
	m_p_calibration_c->apply_gain((u16 *)i_p_image_t->p_buffer, m_n_line_count*m_n_image_width_size>>1, m_n_image_width_size>>1);
	
	if( m_p_system_c->defect_get_enable())
	{
		m_p_system_c->defect_correction((u16 *)i_p_image_t->p_buffer,m_n_line_count);
	}
	
	if( (m_n_line_count + 1) <= 8 )
	{
		if( (m_n_line_count + 1) == 8 )
		{
			make_preview( i_p_image_t, i_p_image_t->n_processed_length );
			
			i_p_image_t->n_crc = make_crc32_with_init_crc( &i_p_image_t->p_buffer[i_p_image_t->n_processed_length], (m_n_image_width_size * 4), i_p_image_t->n_crc );
			i_p_image_t->n_processed_length += (m_n_image_width_size * 4);
		}
	}
	else
	{
		if( (m_n_line_count + 1) == m_n_image_height )
		{
			make_preview( i_p_image_t, i_p_image_t->n_processed_length );
			make_preview( i_p_image_t, i_p_image_t->n_processed_length + (m_n_image_width_size * 4) );
			
			i_p_image_t->n_crc = make_crc32_with_init_crc( &i_p_image_t->p_buffer[i_p_image_t->n_processed_length], (m_n_image_width_size * 8), i_p_image_t->n_crc );
			i_p_image_t->n_processed_length += (m_n_image_width_size * 8);
		}
		else if( ((m_n_line_count + 1)  % 4) == 0 )
		{
			make_preview( i_p_image_t, i_p_image_t->n_processed_length );
			
			i_p_image_t->n_crc = make_crc32_with_init_crc( &i_p_image_t->p_buffer[i_p_image_t->n_processed_length], (m_n_image_width_size * 4), i_p_image_t->n_crc );
			i_p_image_t->n_processed_length += (m_n_image_width_size * 4);
		}
	}
	
    m_n_line_count++;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CCapture::set_notify_handler(bool (*noti)(vw_msg_t * msg))
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
void CCapture::set_add_image_handler(void (*add)(image_buffer_t* i_p_image_t))
{
    add_image_handler = add;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CCapture::message_send( MSG i_msg_e, u8* i_p_data, u16 i_w_data_len )
{
    if( i_w_data_len > VW_PARAM_MAX_LEN )
    {
        print_log(ERROR, 1, "[%s] message data lenth is too long(%d/%d).\n", 
                                m_p_log_id, i_w_data_len, VW_PARAM_MAX_LEN );
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
    
    b_result = notify_handler( &msg );
    
    if( b_result == false )
    {
        print_log(ERROR, 1, "[%s] message_send(%d) failed\n", m_p_log_id, i_msg_e );
    }
    
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CCapture::image_state_message( IMAGE_STATE i_state_e, u32 i_n_image_id )
{
    vw_msg_t msg;
    bool b_result;
    
    u32 n_state;
    u32 n_exp_time = m_p_system_c->get_exp_wait_time();
    
    memset( &msg, 0, sizeof(vw_msg_t) );
    msg.type = (u16)eMSG_IMAGE_STATE;
    msg.size = sizeof( n_state ) + sizeof( i_n_image_id );
    
    n_state = i_state_e;
    memcpy( &msg.contents[0], &n_state, sizeof( n_state ) );
    memcpy( &msg.contents[4], &i_n_image_id, sizeof( i_n_image_id ) );
    memcpy( &msg.contents[8], &(n_exp_time), sizeof( n_exp_time ) );
    
    b_result = notify_handler( &msg );
    
    if( b_result == false )
    {
        print_log(ERROR, 1, "[%s] image_state_message(%d) failed\n", m_p_log_id, i_state_e );
    }
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CCapture::state_print(TG_STATE i_state_e)
{
    switch( i_state_e )
    {
        case eTG_STATE_EXPOSURE:
        {
            print_log(INFO, 1, "[%s] eTG_STATE_EXPOSURE\n", m_p_log_id);
            break;
        }
        case eTG_STATE_EXP_LINE_ACQ:
        {
            print_log(INFO, 1, "[%s] eTG_STATE_EXP_LINE_ACQ\n", m_p_log_id);
            break;
        }
        case eTG_STATE_OFFSET_LINE_ACQ:
        {
            print_log(INFO, 1, "[%s] eTG_STATE_OFFSET_LINE_ACQ\n", m_p_log_id);
            break;
        }
        case eTG_STATE_IDLE:
        {
            print_log(INFO, 1, "[%s] eTG_STATE_IDLE\n", m_p_log_id);
            break;
        }
        case eTG_STATE_OFFSET_DRIFT_ACQ:
        {
            print_log(INFO, 1, "[%s] eTG_STATE_OFFSET_DRIFT_ACQ\n", m_p_log_id);
            break;
        }
        default:
        {
            print_log(INFO, 1, "[%s] state(%d) is unknown.\n", m_p_log_id, i_state_e);
        }
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CCapture::make_info( image_buffer_t* o_p_image_buffer_t )
{
    s32 p_pos[3];
    
    memset( p_pos, 0, sizeof(p_pos) );
    
    m_p_system_c->get_main_detector_angular_position( &p_pos[0], &p_pos[1], &p_pos[2] );
    
    o_p_image_buffer_t->header_t.n_id       = time(NULL);	//m_p_system_c->get_image_id(); //이게 결국 image packet의 image ID 결국 iamge id는 영상 수집 시작 시간 
    o_p_image_buffer_t->header_t.n_time     = o_p_image_buffer_t->header_t.n_id;
    
    o_p_image_buffer_t->header_t.n_pos_x    = p_pos[0];
    o_p_image_buffer_t->header_t.n_pos_y    = p_pos[1];
    o_p_image_buffer_t->header_t.n_pos_z    = p_pos[2];
    
    o_p_image_buffer_t->header_t.c_offset	= m_p_system_c->offset_get_enable();
    o_p_image_buffer_t->header_t.c_defect	= m_p_system_c->defect_get_enable();
    o_p_image_buffer_t->header_t.c_gain		= m_p_calibration_c->get_enable(eCAL_TYPE_GAIN);
    o_p_image_buffer_t->header_t.c_reserved_osf		= 0;
    
    o_p_image_buffer_t->header_t.w_direction	= m_p_system_c->get_direction();
    
    o_p_image_buffer_t->w_error             = m_p_system_c->get_image_error();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CCapture::check_offset_calibration(void)
{
    if( m_p_calibration_c->offset_cal_is_started() == true )
    {
        m_p_calibration_c->offset_cal_message(eOFFSET_CAL_ACQ_CANCELLED);
    }
}

/******************************************************************************/
/**
 * @brief   Exposure request 발생 후 아무런 신호도 안 들어왔을 때를 고려한 timeout(exp_time + exp_ok_delay + 5s)
 * @param   None
 * @return  None
 * @note    20220915. smart W enable + DR일 때를 고려해 해당 시간을 수정함 (3s plus --> 5s plus)
 *******************************************************************************/
void CCapture::create_timer_for_exposure_timeout(void)
{
    u32 n_time;

    if (m_p_exp_time_c != NULL)
    {
        safe_delete(m_p_exp_time_c);
        print_log(DEBUG, 1, "[%s] safe_delete the exp_timeout timer: %d\n", m_p_log_id, __LINE__);
    }

    n_time = m_p_system_c->get_exp_wait_time() + 5000;
    m_p_exp_time_c = new CTime(n_time);

    print_log(DEBUG, 1, "[%s] exp_timeout timer start... %dms\n", m_p_log_id, n_time);
}