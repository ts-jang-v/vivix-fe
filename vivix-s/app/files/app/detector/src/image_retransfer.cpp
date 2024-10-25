/*******************************************************************************
 모  듈 : img_retrans
 작성자 : 한명희
 버  전 : 0.0.1
 설  명 : 영상 재전송 처리
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
#include <errno.h>            // errno

#include "misc.h"
#include "payload_def.h"

#include "typedef.h"
#include "image_retransfer.h"
#include "vw_file.h"

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
* \param        none
* \return       none
* \note
*******************************************************************************/
CImageRetransfer::CImageRetransfer(int (*log)(int,int,const char *,...), bool (*sender)(u32 i_n_image_id, bool i_b_preview, u8* i_p_buff, u32 i_n_length, u16 i_w_slice_number, u16 i_w_total_slice_number))
{
	strcpy( m_p_log_id, "ImageRetrans  " );
	
	print_log = log;
	send_packet = sender;
	
	m_p_preview_image = NULL;
	m_p_full_image = NULL;
	
	m_b_preview_enable = false;
	
	m_b_enable = false;
	
	m_p_mutex = new CMutex;
	
	print_log(DEBUG, 1, "[%s] CImageRetransfer\n", m_p_log_id);
}

/******************************************************************************/
/**
* \brief        constructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CImageRetransfer::CImageRetransfer(void)
{
}

/******************************************************************************/
/**
* \brief        destructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CImageRetransfer::~CImageRetransfer()
{
	safe_free(m_p_preview_image);
	safe_free(m_p_full_image);
	
	safe_delete( m_p_mutex );
	
	print_log(DEBUG, 1, "[%s] ~CImageRetransfer\n", m_p_log_id);
}

/******************************************************************************/
/**
 * @brief   요청한 slice 번호가 영상의 최대 slice 번호보다 작은지 확인
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CImageRetransfer::is_valid_slice_number( bool i_b_preview, u16 i_w_number )
{
	if( m_b_ready_send == false )
	{
		print_log(DEBUG, 1, "[%s] sending is not ready\n", m_p_log_id);
		return false;
	}
	
	if( i_w_number >= get_max_slice_number(i_b_preview) )
	{
		print_log(DEBUG, 1, "[%s] invalid slice number: %d -> %d\n", m_p_log_id, i_w_number, get_max_slice_number(i_b_preview));
		return false;
	}
	
	return true;
}

/******************************************************************************/
/**
 * @brief   영상 처음부터 slice number까지의 CRC 계산
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CImageRetransfer::crc_proc(void)
{
	print_log(DEBUG, 1, "[%s] start crc thread \n", m_p_log_id);
	
	u32 n_max_length = get_total_image_length(false);
	u32 n_length = (m_w_slice_number * PACKET_IMAGE_SIZE) + PACKET_IMAGE_SIZE;
	u32 n_crc;
	
	if( m_image_type_e == eIMAGE_RETRANS_FULL_IMAGE )
	{
		if( n_length > n_max_length )
		{
			n_length = n_max_length;
		}
	}
	else
	{
		if( n_length > n_max_length/16 )
		{
			n_length = n_max_length/16;
		}
	}
	
	m_w_crc_progress = 50;
	
	if( m_image_type_e == eIMAGE_RETRANS_FULL_IMAGE )
	{
		n_crc = make_crc32( m_p_full_image, n_length );
	}
	else
	{
		n_crc = make_crc32( m_p_preview_image, n_length );
	}
	
	print_log(DEBUG, 1, "[%s] crc: 0x%08X, length: %d\n", m_p_log_id, n_crc, n_length);
	
	m_n_crc = n_crc;
	m_w_crc_progress = 100;
	m_b_is_crc_calc = false;
	
	print_log(DEBUG, 1, "[%s] stop crc thread \n", m_p_log_id);
	pthread_exit(NULL);
}

/******************************************************************************/
/**
 * @brief   영상 처음부터 slice number까지의 CRC 계산
 * @param   
 * @return  
 * @note    
*******************************************************************************/
IMAGE_RETRANS_REQUEST_RESULT CImageRetransfer::start_crc( IMAGE_RETRANS_TYPE i_type_e, u16 i_w_slice_number )
{
	bool b_preview = true;
	
	if( i_type_e == eIMAGE_RETRANS_FULL_IMAGE )
	{
		b_preview = false;
	}
	
	print_log(DEBUG, 1, "[%s] type: %d, slice number: %d, ready: %d\n", m_p_log_id, i_type_e, i_w_slice_number, m_b_ready_send);

	if( is_valid_slice_number(b_preview, i_w_slice_number) == false
		|| m_b_ready_send == false )
	{
		return eIMAGE_RETRANS_REQUEST_RESULT_FAIL_INVALID_SLICE_ID;
	}
	
	if( m_b_is_crc_calc == true )
	{
		return eIMAGE_RETRANS_REQUEST_RESULT_FAIL_THREAD_IS_ALREADY_CREATED;
	}
	
	m_w_slice_number	= i_w_slice_number;
	m_image_type_e		= i_type_e;

	m_b_is_crc_calc		= true;
	m_w_crc_progress	= 0;
	m_n_crc				= 0;
	
	if( pthread_create(&m_crc_thread_t, NULL, crc_routine, (void*)this) != 0 )
	{
		print_log(DEBUG, 1, "[%s] pthread_create:%s\n", m_p_log_id, strerror( errno ));
		m_b_is_crc_calc = false;
		return eIMAGE_RETRANS_REQUEST_RESULT_FAIL_CREATE_THREAD;
	}

	pthread_detach(m_crc_thread_t);
	return eIMAGE_RETRANS_REQUEST_RESULT_SUCCESS;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CImageRetransfer::send_image( bool i_b_preview, u8* i_p_image, u16 i_w_start, u16 i_w_end, u32 i_n_total_length )
{
	u16	i;
	u32 n_send_length	= 0;
	u32 n_send_pos		= 0;
	
	for( i = i_w_start ; i < i_w_end ; i++ )
	{
		if( m_b_is_sending == false )
		{
			print_log(DEBUG, 1, "[%s] stop sending(slice num : %d)\n", m_p_log_id, i - 1);
			return true;
		}
		
		n_send_pos		= i * PACKET_IMAGE_SIZE;
		n_send_length	= PACKET_IMAGE_SIZE;
		
		if( (n_send_pos + PACKET_IMAGE_SIZE)  > i_n_total_length )
		{
			n_send_length = i_n_total_length - n_send_pos;
			print_log(DEBUG, 1, "[%s] last slice len : %d\n", m_p_log_id, n_send_length);
		}
		
		if( send_packet( m_n_image_id, i_b_preview, &i_p_image[n_send_pos], n_send_length, i, i_w_end ) == false )
		{
			print_log(DEBUG, 1, "[%s] sending error (slice num : %d)\n", m_p_log_id, i);
			
			return false;
		}
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
void CImageRetransfer::send_proc(void)
{
	u16	w_max_slice_number;
	u16	w_start_slice_number;
	u32 n_total_image_length;
	
	w_start_slice_number = m_w_slice_number;
	
	print_log(DEBUG, 1, "[%s] start sending thread(start slice: %d)\n", m_p_log_id, w_start_slice_number);
	
	if( m_image_type_e == eIMAGE_RETRANS_PREVIEW_IMAGE
		|| m_image_type_e == eIMAGE_RETRANS_PREVIEW_AND_FULL_IMAGE )
	{
		w_max_slice_number		= get_max_slice_number( true );
		n_total_image_length	= get_total_image_length( true );
		
		if( send_image( true, m_p_preview_image, w_start_slice_number, w_max_slice_number, n_total_image_length ) == false )
		{
			m_b_send_error = true;
		}
	}
	
	if( m_b_send_error == true )
	{
		m_b_is_sending = false;
	
		print_log(DEBUG, 1, "[%s] stop sending thread \n", m_p_log_id);
		pthread_exit(NULL);
		
		return;
	}
	
	if( m_image_type_e == eIMAGE_RETRANS_FULL_IMAGE
		|| m_image_type_e == eIMAGE_RETRANS_PREVIEW_AND_FULL_IMAGE )
	{
		w_max_slice_number		= get_max_slice_number( false );
		n_total_image_length	= get_total_image_length( false );
		
		if( m_image_type_e == eIMAGE_RETRANS_PREVIEW_AND_FULL_IMAGE )
		{
			w_start_slice_number = 0;
		}
		
		if( send_image( false, m_p_full_image, w_start_slice_number, w_max_slice_number, n_total_image_length ) == false )
		{
			m_b_send_error = true;
		}
	}
	
	m_b_is_sending = false;
	
	print_log(DEBUG, 1, "[%s] stop sending thread \n", m_p_log_id);
	pthread_exit(NULL);
}

/******************************************************************************/
/**
 * @brief   slice number부터 영상 전송 시작
 * @param   
 * @return  
 * @note    
*******************************************************************************/
IMAGE_RETRANS_REQUEST_RESULT CImageRetransfer::start_send( IMAGE_RETRANS_TYPE i_type_e, u16 i_w_slice_number )
{
	bool b_preview = true;
	
	if( i_type_e == eIMAGE_RETRANS_FULL_IMAGE )
	{
		b_preview = false;
	}
	
	print_log(DEBUG, 1, "[%s] start slice number: %d\n", m_p_log_id, i_w_slice_number);
	
	if( is_valid_slice_number(b_preview, i_w_slice_number) == false )
	{
		return eIMAGE_RETRANS_REQUEST_RESULT_FAIL_INVALID_SLICE_ID;
	}
	
	if( m_b_is_sending == true )
	{
		return eIMAGE_RETRANS_REQUEST_RESULT_FAIL_THREAD_IS_ALREADY_CREATED;
	}
	
	m_b_is_sending		= true;
	m_w_slice_number	= i_w_slice_number;
	m_image_type_e		= i_type_e;
	
	if( pthread_create(&m_sending_thread_t, NULL, send_routine, (void*)this) != 0 )
	{
		print_log(DEBUG, 1, "[%s] pthread_create:%s\n", m_p_log_id, strerror( errno ));
		m_b_is_sending = false;
		return eIMAGE_RETRANS_REQUEST_RESULT_FAIL_CREATE_THREAD;
	}

	pthread_detach(m_sending_thread_t);
	
	return eIMAGE_RETRANS_REQUEST_RESULT_SUCCESS;
}

/******************************************************************************/
/**
 * @brief   영상 전송 중단
 * @param   
 * @return  
 * @note    
*******************************************************************************/
IMAGE_RETRANS_REQUEST_RESULT CImageRetransfer::stop_send(void)
{
	print_log(DEBUG, 1, "[%s] stop sending\n", m_p_log_id);
	
	m_b_is_sending = false;
	
	return eIMAGE_RETRANS_REQUEST_RESULT_SUCCESS;
}

/******************************************************************************/
/**
 * @brief   영상 재전송 기능 초기화
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CImageRetransfer::initialize( bool i_b_enable, u32 i_n_image_size )
{
	print_log(DEBUG, 1, "[%s] enable: %d\n", m_p_log_id, i_b_enable );
	
	if( i_b_enable == false )
	{
		lock();
		
		safe_free(m_p_preview_image);
		safe_free(m_p_full_image);
		
		m_b_enable = false;
		
		unlock();
		
		return;
	}
	
	if( m_b_enable == true )
	{
		print_log(DEBUG, 1, "[%s] already initialized.\n", m_p_log_id );
		return;
	}
	
	m_n_image_size = i_n_image_size;
	
	print_log(DEBUG, 1, "[%s] image size: %d\n", m_p_log_id, m_n_image_size );
	
	lock();
	
	m_p_preview_image = (u8*)malloc(get_total_image_length(true));
	m_p_full_image = (u8*)malloc(get_total_image_length());
	
	memset(m_p_preview_image, 0, get_total_image_length(true));
	memset(m_p_full_image, 0, get_total_image_length());
	
	m_n_image_id = 0;
	m_b_ready_send = false;
	m_b_prepared = false;
	
	m_n_full_image_crc = 0;
	m_b_send_error = false;
	m_b_enable = true;
	
	m_b_preview_enable = false;
	
	m_b_is_sending = false;
	
	unlock();
}

/******************************************************************************/
/**
 * @brief   촬영한 영상의 id를 저장하고 관련 변수 초기화
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CImageRetransfer::prepare( u32 i_n_image_id, s32 i_n_acc_x, s32 i_n_acc_y, s32 i_n_acc_z )
{
	if( m_b_enable == false )
	{
		return;
	}
	
	lock();
	
	m_b_prepared = false;
	m_b_ready_send = false;
	
	m_n_image_id = i_n_image_id;
	
	m_p_acceleration[0] = i_n_acc_x;
	m_p_acceleration[1] = i_n_acc_y;
	m_p_acceleration[2] = i_n_acc_z;
	
	m_b_send_error = false;
	m_b_is_crc_calc = false;
	m_b_preview_enable = false;
	
	m_b_prepared = true;
	
	m_b_is_sending = false;
	
	unlock();
	
	print_log(DEBUG, 1, "[%s] image id: 0x%08X, X: %d, Y: %d, Z: %d\n", m_p_log_id, i_n_image_id, \
			m_p_acceleration[0], m_p_acceleration[1], m_p_acceleration[2]);
}

/******************************************************************************/
/**
 * @brief   촬영한 영상을 복사하여 재전송 요청 수행할 수 있도록 함
 * @param   
 * @return  
 * @note    영상을 다 수집한 뒤에 수행해야 함
*******************************************************************************/
void CImageRetransfer::copy_image( bool i_b_preview, u8* i_p_preview, u8* i_p_full, u32 i_n_full_image_crc )
{
	if( m_b_enable == false )
	{
		return;
	}
	
	lock();
	
	if( i_b_preview == true )
	{
		print_log(DEBUG, 1, "[%s] preview image len: %d\n", m_p_log_id, get_total_image_length(true));
		
		memcpy(m_p_preview_image, i_p_preview, get_total_image_length(true));
		m_b_preview_enable = true;
	}
	else
	{
		m_b_preview_enable = false;
	}
	
	memcpy(m_p_full_image, i_p_full, get_total_image_length());
	
	m_n_full_image_crc = i_n_full_image_crc;
	
	m_b_ready_send = true;
	
	unlock();
	
	print_log(DEBUG, 1, "[%s] image copy(crc: 0x%08X)\n", m_p_log_id, m_n_full_image_crc);
}

/******************************************************************************/
/**
 * @brief   전송 오류가 발생했는지 확인
 * @param   
 * @return  
 * @note    image retrans 기능을 지원할 경우에만 send error 확인
 *			지원하지 않는 경우는 기존과 동일하게 아무 동작도 하지 않도록 함
*******************************************************************************/
bool CImageRetransfer::is_send_error(void)
{
	if( m_b_enable == false )
	{
		return false;
	}
	
	if( m_b_send_error == true )
	{
		m_b_send_error = false;
		
		return true;
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
bool CImageRetransfer::is_valid_id( u32 i_n_id )
{
	if( m_b_enable == false )
	{
		return false;
	}
	
	if( m_b_prepared == false )
	{
		return false;
	}
	
	if( i_n_id == m_n_image_id )
	{
		return true;
	}
	
	print_log(DEBUG, 1, "[%s] invalid id(0x%08X <-> 0x%08X)\n", m_p_log_id, i_n_id, m_n_image_id );
	
	return false;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CImageRetransfer::clear_image( u32 i_n_id )
{
	if( m_n_image_id == 0 )
	{
		print_log(DEBUG, 1, "[%s] invalid clear request(0x%08X)\n", m_p_log_id, i_n_id );
		return;
	}
	
	lock();
	
	if( i_n_id == m_n_image_id )
	{
		m_n_image_id = 0;
		
		m_b_prepared = false;
		m_b_ready_send = false;
		
		m_b_send_error = false;
		m_b_is_crc_calc = false;
		m_b_preview_enable = false;
		
		m_b_is_sending = false;
	}
	
	unlock();
	
	if( m_n_image_id == 0 )
	{
		print_log(DEBUG, 1, "[%s] clear resend image(0x%08X)\n", m_p_log_id, i_n_id );
	}
	else
	{
		print_log(DEBUG, 1, "[%s] invalid id(0x%08X <-> 0x%08X)\n", m_p_log_id, i_n_id, m_n_image_id );
	}
}

/******************************************************************************/
/**
 * @brief   영상 재전송 동작 요청 처리
 * @param   
 * @return  
 * @note    
*******************************************************************************/
IMAGE_RETRANS_REQUEST_RESULT CImageRetransfer::request( image_retrans_req_t* i_p_req_t, void* o_p_result )
{
	if( m_b_enable == false )
	{
		return eIMAGE_RETRANS_REQUEST_RESULT_FAIL_FUNC_NOT_ENABLE;
	}
	
	if( is_valid_id(i_p_req_t->n_image_id) == false )
	{
		return eIMAGE_RETRANS_REQUEST_RESULT_FAIL_INVALID_IMAGE_ID;
	}
	
	if( m_b_preview_enable == false
		&& (i_p_req_t->w_image_type == eIMAGE_RETRANS_PREVIEW_IMAGE || i_p_req_t->w_image_type == eIMAGE_RETRANS_PREVIEW_AND_FULL_IMAGE) )
	{
		return eIMAGE_RETRANS_REQUEST_RESULT_FAIL_UNKNOWN_REQUEST;
	}
	
	if( i_p_req_t->n_operation == eIMAGE_RETRANS_REQUEST_CRC_START )
	{
		return start_crc((IMAGE_RETRANS_TYPE)i_p_req_t->w_image_type, i_p_req_t->w_data);
	}
	else if( i_p_req_t->n_operation == eIMAGE_RETRANS_REQUEST_CRC_PROGRESS )
	{
		if( o_p_result != NULL )
		{
			*(u16*)o_p_result = m_w_crc_progress;
		}
		
		print_log(DEBUG, 1, "[%s] crc progress: %d\n", m_p_log_id, m_w_crc_progress);
		
		return eIMAGE_RETRANS_REQUEST_RESULT_SUCCESS;
	}
	else if( i_p_req_t->n_operation == eIMAGE_RETRANS_REQUEST_CRC_RESULT )
	{
		if( o_p_result != NULL )
		{
			*(u32*)o_p_result = m_n_crc;
		}
		
		print_log(DEBUG, 1, "[%s] crc result: 0x%08X\n", m_p_log_id, m_n_crc);
		
		return eIMAGE_RETRANS_REQUEST_RESULT_SUCCESS;
	}
	else if( i_p_req_t->n_operation == eIMAGE_RETRANS_REQUEST_FULL_IMAGE_CRC )
	{
		if( o_p_result != NULL )
		{
			*(u32*)o_p_result = m_n_full_image_crc;
		}
		
		print_log(DEBUG, 1, "[%s] full image crc: 0x%08X\n", m_p_log_id, m_n_full_image_crc);
		return eIMAGE_RETRANS_REQUEST_RESULT_SUCCESS;
	}
	else if( i_p_req_t->n_operation == eIMAGE_RETRANS_REQUEST_START_SEND )
	{
		return start_send((IMAGE_RETRANS_TYPE)i_p_req_t->w_image_type, i_p_req_t->w_data);
	}
	else if( i_p_req_t->n_operation == eIMAGE_RETRANS_REQUEST_STOP_SEND )
	{
		return stop_send();
	}
	else if( i_p_req_t->n_operation == eIMAGE_RETRANS_REQUEST_ACCELERATION )
	{
		if( o_p_result != NULL )
		{
			memcpy((u8*)o_p_result, (u8*)m_p_acceleration, sizeof(m_p_acceleration));
		}
		
		return eIMAGE_RETRANS_REQUEST_RESULT_SUCCESS;
	}
	
	return eIMAGE_RETRANS_REQUEST_RESULT_FAIL_UNKNOWN_REQUEST;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void * crc_routine( void * arg )
{
	CImageRetransfer* p_retransfer_c = (CImageRetransfer *)arg;
	p_retransfer_c->crc_proc();
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
void * send_routine( void * arg )
{
	CImageRetransfer* p_retransfer_c = (CImageRetransfer *)arg;
	p_retransfer_c->send_proc();
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
u32 CImageRetransfer::get_total_image_length( bool i_b_preview )
{
	if( i_b_preview == true )
	{
		return (m_n_image_size/16);
	}
	
	return m_n_image_size;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CImageRetransfer::get_max_slice_number( bool i_b_preview )
{
	return (get_total_image_length(i_b_preview)/PACKET_IMAGE_SIZE + 1);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
IMAGE_RETRANS_REQUEST_RESULT CImageRetransfer::get_id( u32* o_p_id )
{
	if( m_b_enable == false
		|| m_b_prepared == false )
	{
		return eIMAGE_RETRANS_REQUEST_RESULT_FAIL_INVALID_IMAGE_ID;
	}
	
	memcpy( o_p_id, &m_n_image_id, sizeof(u32) );
	
	return eIMAGE_RETRANS_REQUEST_RESULT_SUCCESS;
}
