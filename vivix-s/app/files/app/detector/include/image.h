/*
 * image.h
 *
 *  Created on: 2014. 01. 14.
 *      Author: myunghee
 */

 
#ifndef IMAGE_H_
#define IMAGE_H_

/*******************************************************************************
*	Include Files
*******************************************************************************/
#include "typedef.h"
#include "message.h"
#include "mutex.h"
#include "image_def.h"
#include "vw_system.h"
#include "web.h"
#include "image_retransfer.h"

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/
#define IMAGE_BUFFER_COUNT  (5)

#define COMPLETE_TIME_OUT_MIN       (5000)

/*******************************************************************************
*	Type Definitions
*******************************************************************************/
typedef enum
{
    eACQ_STATE_START = 0,
    eACQ_STATE_SEND,
    eACQ_STATE_SEND_WAIT_END,
    eACQ_STATE_SAVE,
    eACQ_STATE_END,
    eACQ_STATE_IMAGE_ERROR,
} ACQUSITION_STATE;
/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*	Variable Definitions
*******************************************************************************/


/*******************************************************************************
*	Function Prototypes
*******************************************************************************/
void *         	image_routine( void * arg );

class CImage
{
private:
    int			        (* print_log)(int level,int print,const char * format, ...);
    
    image_buffer_t*     m_p_buffer_t[IMAGE_BUFFER_COUNT];
    
    u32                 m_n_front;
    u32                 m_n_rear;
    bool                is_empty(void)
    {
        if( m_n_rear == m_n_front )
        {
            return true;
        }
        return false;
    }
    u32                 get_count(void)
    {
        if( m_n_rear < m_n_front )
        {
            return (IMAGE_BUFFER_COUNT - m_n_front) + m_n_rear;
        }
        return ( m_n_rear - m_n_front );
    }
    
    bool                m_b_save_half;
    
    bool                m_b_is_running;
    pthread_t	        m_thread_t;
    void                image_proc(void);
	
    // image send
    bool                (* is_ready_to_send)(void);
    bool                (* start_send_thread)(image_buffer_t* i_p_image_t);
    void                (* stop_send_thread)(void);
    
    // image save
    image_buffer_t*     m_p_save_image_t;
    void                copy_to_memory( image_buffer_t* i_p_image_t );
    void                save_to_flash( image_buffer_t* i_p_image_t );
    
    void                delete_image( image_buffer_t* i_p_image_t );
    void                check_queue_state(void);
    
    void                print_state( ACQUSITION_STATE i_state_e );
    ACQUSITION_STATE    check_backup_image( image_buffer_t* i_p_image_t );
    ACQUSITION_STATE    check_send_complete( image_buffer_t* i_p_image_t );
    ACQUSITION_STATE    check_send_wait_end( image_buffer_t* i_p_image_t );
    u32                 get_wait_time(void);
    void				update_remain_time( CTime* i_p_time_c = NULL );
    
    ACQUSITION_STATE    get_initial_acq_state( image_buffer_t* i_p_image_t );
    ACQUSITION_STATE    check_error_image( image_buffer_t* i_p_image_t );
    bool                is_valid_image( image_buffer_t* i_p_image_t );
    
    // notification
    bool                (* notify_handler)(vw_msg_t * msg);
    void                state_message(IMAGE_STATE i_state_e, image_buffer_t* i_p_image_t );
    
    bool				m_b_preview_started;
    void				make_preview( image_buffer_t* i_p_image_t );
    
protected:
public:
    
	CImage(int (*log)(int,int,const char *,...));
	CImage(void);
	~CImage();
    
    void                add_image( image_buffer_t* i_p_image_t );
    
    friend void *       image_routine( void * arg );
    
    // notification
    void                set_notify_handler(bool (*noti)(vw_msg_t * msg));
    
    void                set_send_handler( bool (*is_ready)(void), bool (*start_send)(image_buffer_t* i_p_image_t), void (*stop_send)(void) );
    
    // system
    CVWSystem*          m_p_system_c;
	
	// web
	CWeb*				m_p_web_c;

	CImageRetransfer*	m_p_img_retransfer_c;
};


#endif /* end of IMAGE_H_*/

