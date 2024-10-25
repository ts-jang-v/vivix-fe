/*
 * image_retransfer.h
 *
 *  Created on: 2018. 12. 12.
 *      Author: hmh
 */

#ifndef _IMAGE_RETRANSFER_H
#define _IMAGE_RETRANSFER_H

/*******************************************************************************
*   Include Files
*******************************************************************************/
#include "mutex.h"

/*******************************************************************************
*   Constant Definitions
*******************************************************************************/
#define TCP_SEND_TIMEOUT_FOR_RETRANS			5

/*******************************************************************************
*   Type Definitions
*******************************************************************************/
typedef enum{
	eIMAGE_RETRANS_REQUEST_CRC_START = 0,
	eIMAGE_RETRANS_REQUEST_CRC_PROGRESS,
	eIMAGE_RETRANS_REQUEST_CRC_RESULT,
	eIMAGE_RETRANS_REQUEST_START_SEND,
	eIMAGE_RETRANS_REQUEST_STOP_SEND,
	eIMAGE_RETRANS_REQUEST_ACCELERATION,
	eIMAGE_RETRANS_REQUEST_FULL_IMAGE_CRC,
	eIMAGE_RETRANS_REQUEST_BACKUP_IMAGE_IS_SAVED,
	eIMAGE_RETRANS_REQUEST_IMAGE_ID,
	eIMAGE_RETRANS_REQUEST_MAX,
	
}IMAGE_RETRANS_REQUEST;

typedef enum{
	eIMAGE_RETRANS_REQUEST_RESULT_SUCCESS = 0,
	eIMAGE_RETRANS_REQUEST_RESULT_FAIL_FUNC_NOT_ENABLE,
	eIMAGE_RETRANS_REQUEST_RESULT_FAIL_UNKNOWN_REQUEST,
	eIMAGE_RETRANS_REQUEST_RESULT_FAIL_INVALID_SLICE_ID,
	eIMAGE_RETRANS_REQUEST_RESULT_FAIL_CREATE_THREAD,
	eIMAGE_RETRANS_REQUEST_RESULT_FAIL_THREAD_IS_ALREADY_CREATED,
	eIMAGE_RETRANS_REQUEST_RESULT_FAIL_DATA_SIZE_MISMATCH,
	eIMAGE_RETRANS_REQUEST_RESULT_FAIL_INVALID_IMAGE_ID,
	eIMAGE_RETRANS_REQUEST_RESULT_MAX,
	
}IMAGE_RETRANS_REQUEST_RESULT;

typedef enum{
	eIMAGE_RETRANS_FULL_IMAGE = 0,
	eIMAGE_RETRANS_PREVIEW_IMAGE,
	eIMAGE_RETRANS_PREVIEW_AND_FULL_IMAGE,
	
}IMAGE_RETRANS_TYPE;

typedef struct _image_retrans_req
{
	u32		n_operation;
	u32		n_image_id;
	u16		w_image_type;
	u16		w_data;
}image_retrans_req_t;

typedef struct _image_retrans_result
{
	u8 c_operation;
	u8 c_result;
	u16 w_data;
    
}image_retrans_result_t;

/*******************************************************************************
*   Macros (Inline Functions) Definitions
*******************************************************************************/

/*******************************************************************************
*   Variable Definitions
*******************************************************************************/

/*******************************************************************************
*   Function Prototypes
*******************************************************************************/
void*           crc_routine( void* argc );
void*           send_routine( void* argc );

#if 0
void image_retrans_init( bool i_b_enable );
void image_retrans_prepare( u32 i_n_image_id );
void image_retrans_copy_image(void);
bool image_retrans_is_send_error( void* i_p_client_handle );
bool image_retrans_is_valid_id( u32 i_n_id );

IMAGE_RETRANS_REQUEST_RESULT image_retrans_request( image_retrans_req_t* i_p_req_t, time_t timeout_t, void* i_p_client_handle, void* o_p_result );

void image_retrans_test_enable( bool i_b_enable );
void image_retrans_test( u16 i_w_number );
#endif


class CImageRetransfer
{
private:
	s8			m_p_log_id[15];
    int			(* print_log)(int level,int print,const char * format, ...);
    bool		(* send_packet)(u32 i_n_image_id, bool i_b_preview, u8* i_p_buff, u32 i_n_length, u16 i_w_slice_number, u16 i_w_total_slice_number);
    
    CMutex*		m_p_mutex;
	void		lock( void ){ m_p_mutex->lock(); }
	void		unlock( void ){ m_p_mutex->unlock(); }
    
    bool		m_b_enable;
    u32			m_n_image_size;
    
    u32			m_n_image_id;
    u8*			m_p_preview_image;
    u8*			m_p_full_image;
    bool		m_b_ready_send;
    bool		m_b_prepared;
    bool		m_b_preview_enable;		// accessable preview image
    
    void*		m_p_client_handle;
    bool		m_b_is_sending;
    pthread_t	m_sending_thread_t;
    
    bool		m_b_is_crc_calc;
    pthread_t	m_crc_thread_t;
    u16			m_w_crc_progress;
    u32			m_n_crc;
    u32			m_n_full_image_crc;
    
    bool		m_b_preview;
    IMAGE_RETRANS_TYPE	m_image_type_e;
    u16			m_w_slice_number;
    bool		m_b_send_error;
    
    s32			m_p_acceleration[3];		// X, Y, Z
    
    bool		is_valid_slice_number( bool i_b_preview, u16 i_w_number );
    
    IMAGE_RETRANS_REQUEST_RESULT start_crc( IMAGE_RETRANS_TYPE i_type_e, u16 i_w_slice_number );
    IMAGE_RETRANS_REQUEST_RESULT start_send( IMAGE_RETRANS_TYPE i_type_e, u16 i_w_slice_number );
    IMAGE_RETRANS_REQUEST_RESULT stop_send(void);
    
    void		crc_proc(void);
    void		send_proc(void);
    bool		send_image( bool i_b_preview, u8* i_p_image, u16 i_w_start, u16 i_w_end, u32 i_n_total_length );
    u32			get_total_image_length( bool i_b_preview=false );
    u16			get_max_slice_number( bool i_b_preview=false );
    
protected:
public:
	CImageRetransfer(int (*log)(int,int,const char *,...), bool (*sender)(u32 i_n_image_id, bool i_b_preview, u8* i_p_buff, u32 i_n_length, u16 i_w_slice_number, u16 i_w_total_slice_number));
	CImageRetransfer(void);
	~CImageRetransfer();
	
	friend void *       crc_routine( void* arg );
	friend void *       send_routine( void* arg );
	
	void initialize( bool i_b_enable, u32 i_n_image_size );
	void prepare( u32 i_n_image_id, s32 i_n_acc_x, s32 i_n_acc_y, s32 i_n_acc_z );
	void copy_image( bool i_b_preview, u8* i_p_preview, u8* i_p_full, u32 i_n_full_image_crc );
	bool is_send_error(void);
	bool is_valid_id( u32 i_n_id );
	void clear_image( u32 i_n_id );
	
	bool is_sending(void){return m_b_is_sending;}
	
	IMAGE_RETRANS_REQUEST_RESULT get_id( u32* o_p_id );
	
	IMAGE_RETRANS_REQUEST_RESULT request( image_retrans_req_t* i_p_req_t, void* o_p_result=NULL );
};


#endif /* end of _IMAGE_RETRANSFER_H*/
