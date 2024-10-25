/*
 * image_def.h
 *
 *  Created on: 2014. 01. 14.
 *      Author: myunghee
 */

 
#ifndef IMAGE_DEF_H_
#define IMAGE_DEF_H_

/*******************************************************************************
*	Include Files
*******************************************************************************/
#ifdef TARGET_COM
#include "typedef.h"
#include "vw_time.h"
#else
#include "typedef.h"
#include "Time.h"
#endif




/*******************************************************************************
*	Constant Definitions
*******************************************************************************/
#define PREVIEW_SCALE       (16)

/*******************************************************************************
*	Type Definitions
*******************************************************************************/
typedef struct _image_header
{
    u32 n_id;				// 4
    u32 n_width_size;		// 8
    u32 n_height;			// 12
    u32 n_total_size;		// 16
    u32 n_time;				// 20
    
    s32 n_pos_x;			// 24
    s32 n_pos_y;			// 28
    s32 n_pos_z;			// 32
    
    u8	c_offset;			// 33
    u8	c_defect;			// 34
    u8	c_gain;				// 35
    u8	c_reserved_osf;		// 36
    
    u16	w_direction;		// 38
} image_header_t;

typedef struct _preview_buffer_t
{
    u8*     p_buffer;
    u32     n_processed_length;
    u32     n_sent_length;
    u32     n_total_size;
}preview_buffer_t;

typedef struct _image_buffer
{
    image_header_t  header_t;
    u8*             p_buffer;
    
    CTime*          p_capture_start_time_c;
    u32             n_processed_length;
    
    bool            b_send_started;
    u32             n_sent_length;
    
    CTime*          p_complete_time_c;
    bool            b_complete;
    
    // preview image buffer
    preview_buffer_t        preview_t;
    
    u16             w_error;
    u32				n_crc;
    
}__attribute__((packed)) image_buffer_t;

// image state
typedef enum
{
    eIMG_STATE_EXPOSURE     = 1,
    eIMG_STATE_ACQ_START,
    eIMG_STATE_ACQ_END,
    eIMG_STATE_ACQ_TIMEOUT,
    eIMG_STATE_SEND_END,
    eIMG_STATE_SEND_TIMEOUT,
    eIMG_STATE_SAVED,
    eIMG_STATE_SAVE_FAILED,
    eIMG_STATE_ACQ_START_WITH_ERROR,
    eIMG_STATE_ACQ_END_WITH_ERROR,
    eIMG_STATE_XRAY_DETECTED_ERROR,
    
    eIMG_STATE_RESERVED = 0x0F,
    
    eIMG_STATE_READY_FOR_EXPOSURE   = 0x10,
    eIMG_STATE_SCU_IS_CONNECTED     = 0x11,
    eIMG_STATE_SCU_IS_DISCONNECTED  = 0x12,
    
    eIMG_STATE_EXPOSURE_TIMEOUT = 0xFF,
    
} IMAGE_STATE;
    
/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*	Variable Definitions
*******************************************************************************/


/*******************************************************************************
*	Function Prototypes
*******************************************************************************/

#endif /* end of IMAGE_DEF_H_*/

