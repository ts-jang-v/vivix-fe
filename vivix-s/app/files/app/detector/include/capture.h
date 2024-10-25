/******************************************************************************/
/**
 * @file    capture.h
 * @brief   
 * @author  한명희
*******************************************************************************/
 
#ifndef CAPTURE_H_
#define CAPTURE_H_

/*******************************************************************************
*	Include Files
*******************************************************************************/

#ifdef TARGET_COM
#include "typedef.h"
#include "message.h"
#include "message_def.h"
#include "image_def.h"
#include "vw_system.h"
#include "vw_time.h"
#include "vworks_ioctl.h"
#include "calibration.h"
#else
#include "typedef.h"
#include "Time.h"
#include "Common_def.h"
#include "Vw_system.h"
#include "File.h"
#include "Calibration.h"
#include "../app/detector/include/image_def.h"
//#include "Vw_system.h"
//#define _IOWR(a,b,c) b
//#include "../driver/include/vworks_ioctl.h"
#endif
/*******************************************************************************
*	Constant Definitions
*******************************************************************************/
#define CAPTURE_INTERNAL_TIME_OUT   (10000)      // ms
#define CAPTURE_TIME_OUT            (CAPTURE_INTERNAL_TIME_OUT + 200)      // ms

/*******************************************************************************
*	Type Definitions
*******************************************************************************/
typedef enum
{
    eTG_STATE_IDLE = 0,
    eTG_STATE_EXPOSURE,
    eTG_STATE_EXP_LINE_ACQ,
    eTG_STATE_OFFSET_LINE_ACQ,
    eTG_STATE_OFFSET_DRIFT_ACQ,
    
} TG_STATE;

/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*	Variable Definitions
*******************************************************************************/


/*******************************************************************************
*	Function Prototypes
*******************************************************************************/
void *         	capture_routine( void * arg );
int             test_wait( long usec );

class CCapture
{
private:
    
    int			        (* print_log)(int level,int print,const char * format, ...);
    s8                  m_p_log_id[15];
    
    // FPGA driver file descriptor
    s32                 m_n_fd;
    
    // capture thread
    bool	            m_b_is_running;
    pthread_t	        m_thread_t;
    void                capture_proc(void);
    void                state_idle_check(void);
    
    TG_STATE            state_idle(u8* i_p_line);
    TG_STATE            state_exposure(void);
    TG_STATE            state_acq(void);
    TG_STATE            state_acq_offset(u16* i_p_line);
    TG_STATE            state_acq_offset_drift(u16* i_p_line);
    
    CTime*              m_p_exp_time_c;
    CTime*              m_p_capture_time_c;
    
    // capture data
    u32                 m_n_image_width_size;   // width * byte per pixel
    u32                 m_n_image_height;
    u32                 m_n_image_pixel_depth;
    u32                 m_n_image_size_byte;         // m_n_image_width_size * m_n_image_height
    // m_n_image_width_size * m_n_image_height/byte per pixel
    u32                 m_n_image_size_pixel;
    
    u32                 m_n_line_count;
    
    image_buffer_t*     m_p_image_t;
    
    void                image_buffer_alloc(void);
    
    void                process_image( image_buffer_t* i_p_image_t );
    void                (* add_image_handler)( image_buffer_t* i_p_image_t );
    void                state_print(TG_STATE i_state_e);
    
    // notification
    bool                (* notify_handler)(vw_msg_t * msg);
    void                message_send( MSG i_msg_e, u8* i_p_data=NULL, u16 i_w_data_len=0 );
    void                image_state_message( IMAGE_STATE i_state_e, u32 i_n_image_id=0 );
    
    void                make_info( image_buffer_t* o_p_image_buffer_t );
    void                check_offset_calibration(void);
    
    void				make_preview( image_buffer_t* i_p_image_t, u32 i_n_pos );
    
    //timer
    void                create_timer_for_exposure_timeout(void);
protected:
public:
    
	CCapture(int (*log)(int,int,const char *,...));
	CCapture(void);
	~CCapture();
    
    friend void *       capture_routine( void * arg );
    
    // image
    void                start(void);
    void                set_add_image_handler(void (*add)(image_buffer_t* i_p_image_t));
    
	// system
	CVWSystem*          m_p_system_c;
	bool                m_b_offset_cal;
	
	CCalibration*		m_p_calibration_c;
	
    // notification
    void                set_notify_handler(bool (*noti)(vw_msg_t * msg));
    
};


#endif /* end of CAPTURE_H_*/

