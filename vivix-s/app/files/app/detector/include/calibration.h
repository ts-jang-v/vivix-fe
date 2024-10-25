/*
 * calibration.h
 *
 *  Created on: 2014. 03. 04.
 *      Author: myunghee
 */

 
#ifndef _CALIBRATION_H_
#define _CALIBRATION_H_

/*******************************************************************************
*	Include Files
*******************************************************************************/

#ifdef TARGET_COM
#include "typedef.h"
#include "vw_system.h"
#include "vworks_ioctl.h"
#else
#include "typedef.h"
#include "Vw_system.h"
#include "Image_process.h"
#define _IOWR(a,b,c) b
#include "../driver/include/vworks_ioctl.h"
#endif


/*******************************************************************************
*	Constant Definitions
*******************************************************************************/
#define CAL_HEADER_SIZE			(1024)
#define CAL_HEADER_ID_LEN		(9)

/*******************************************************************************
*	Type Definitions
*******************************************************************************/
/**
 * @brief   calibration type
 */
typedef enum
{
    eCAL_TYPE_OFFSET    = 0,
	eCAL_TYPE_DEFECT,
	eCAL_TYPE_GAIN,
	//eCAL_TYPE_OSF,
    eCAL_TYPE_MAX,
    
} CALIBRATION_TYPE;


/**
 * @brief   calibration file header
 */
typedef union _cal_header_t
{
    u8			p_data[1024];
    struct
    {
    	s8		p_file_header[CAL_HEADER_ID_LEN];
    	s8		p_model_name[50];
    	s8		p_serial[50];
    	u16		w_verison;
    	u32		n_date;
    	u32		n_data_size;
    	u32		n_width;
    	u32		n_height;
    	u32		n_byte_per_pixel;
    	u16		w_temperature;
    	u16		w_exposure_time_low;
    	u16		w_digital_gain;
    	u16		w_pixel_average;
    	u32		n_defect_total_count;
    	u16		w_exposure_time_high;
    	
    } __attribute__((packed))header;
} cal_header_t;

typedef struct _offset_info_t
{
    u32             n_date;         // time stamp
    u16             w_sw_offset_not_used;
    u16             w_temperature;
    u16             w_exposure_time;
    u16             w_digital_gain;
    u8              p_reserved[12];
}offset_info_t;



/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*	Variable Definitions
*******************************************************************************/


/*******************************************************************************
*	Function Prototypes
*******************************************************************************/
void *         	load_routine( void * arg );
void *         	load_all_routine( void * arg );

class CCalibration
{
private:
    s8                  m_p_log_id[15];
    int			        (* print_log)(int level,int print,const char * format, ...);
    
	bool				m_p_cal_loaded[eCAL_TYPE_MAX];
    
    s32					m_n_tg_fd;				// tg file descriptor
    defect_pixel_info_t *m_p_defect_info_t;
    s32					m_n_defect_count;
	
	// gain correction
	bool				m_b_fw_correction_mode;
    u16*				m_p_gain_integer;
    u16*				m_p_gain_fraction;
    bool				m_b_gain_enable;
    u16					m_w_gain_average;  
    u16					m_w_digital_offset;  
    bool				m_b_max_saturation_enable;
    
    u16*				m_p_offset_corr_data;
    bool				m_b_double_readout;
    
    u32					get_size(CALIBRATION_TYPE i_cal_type_e);
    bool				is_valid_header( CALIBRATION_TYPE i_type_e, cal_header_t* i_p_header_t );
    void				print_header( cal_header_t* i_p_header_t );
    void				set_correction_enable( CALIBRATION_TYPE i_type_e, bool i_b_enable );
    bool				get_correction_enable( CALIBRATION_TYPE i_type_e );
    
    bool                m_b_load_is_running;
    bool                m_b_load_all_running;
    pthread_t	        m_load_thread_t;
    pthread_t	        m_load_all_thread_t;
    pthread_attr_t 		m_load_all_thread_attr_t;
    
    CALIBRATION_TYPE	m_cal_type_e;
    u8					m_c_load_progress;
    bool				m_b_load_result;
    s8					m_p_file_path[256];
    void                load_proc(void);
    void                load_all_proc(void);
    void				set_load( CALIBRATION_TYPE i_type_e, bool i_b_load );
    
    mmap_t*				m_p_mmap_t;
    void				get_shared_memory(void);
    
	bool				write_start(void);
	void				write_stop(void);
	bool				write_update_progress(void);
	
	bool				load_data_prepare( CALIBRATION_TYPE i_type_e );
	bool				load_data_write_start( CALIBRATION_TYPE i_type_e );
	void				load_data_write_stop(void);
	u8					load_data_get_progress(void);
	bool				read_defect_correction_data(void);

	// offset data
	CMutex*				m_p_cal_mutex_c;
	void				cal_lock(void){m_p_cal_mutex_c->lock();}
	void				cal_unlock(void){m_p_cal_mutex_c->unlock();}
	
	image_size_t        m_image_size_t;
	
	u16*				m_p_offset;
	u32 				m_n_image_size_byte;
	u32 				m_n_image_size_pixel;
	bool				m_b_offset_enable;
	cal_header_t		m_offset_header_t;
	u32					offset_get_size(void);
	void				offset_init(void);
	void				offset_make_header( cal_header_t* o_p_header_t );
	
	// offset calibration
	u32					m_n_offset_skip_count;
	u32					m_n_offset_target_count;
	u32					m_n_offset_interval;
	u32					m_n_offset_count;
	u32					m_n_offset_buffer_len;
	u32*				m_p_offset_buffer;
	bool				m_b_offset_cal_started;
	CTime*				m_p_offset_wait_c;
	bool				offset_cal_set( u32 i_n_target_count, u32 i_n_skip_count );
	void				offset_cal_next_trigger(void);
	void				offset_cal_upload(void);
	
	u16					m_w_offset_cal_count;
	
	bool				read_offset_correction_data(u16* i_p_data, u32 i_n_size);
		
	// gain correction
	bool				read_gain_correction_data(u16* i_p_data, u32 i_n_size);
	void				gain_set_enable(bool i_b_enable);
	bool				gain_get_enable(void);
	void				gain_set_pixel_average( u16 i_w_average );
	
	bool				read_correction_data(CALIBRATION_TYPE i_cal_type_e);
	
	// notification
	bool                (* notify_handler)(vw_msg_t * msg);
protected:
public:
    
	CCalibration(int (*log)(int,int,const char *,...));
	CCalibration(void);
	~CCalibration();
	
	bool				save(CALIBRATION_TYPE i_type_e);
	
	bool				load_all(void);
    bool				load_all_start(void);
    void				load_all_end(void);
    bool                load_start(CALIBRATION_TYPE i_type_e);
    void                load_stop(void);
    friend void *       load_routine( void * arg );
    friend void *       load_all_routine( void * arg );
    bool                load_get_result(void){return m_b_load_result;}
    u8	                load_get_progress(void){return m_c_load_progress;}
    
    // offset correction
    void				apply_offset(u16* i_p_img_data, u32 i_n_pixel_offset, u32 i_n_pixel_num);
    	
    // gain correction
    void				apply_gain(u16* i_p_img_data, u32 i_n_pixel_offset, u32 i_n_pixel_num);    
    void				prepare_correction(void);
    	
    CVWSystem*          m_p_system_c;
    void				start(void);

    // offset data
	bool				offset_load(void);
	bool				offset_save(void);
	u16*				offset_get(void){return m_p_offset;}
	u32 				offset_get_crc(void);
	void				offset_set_enable( bool i_b_enable );
	bool				offset_get_enable(void);
	bool				offset_cal_start( u32 i_n_target_count, u32 i_n_skip_count, u32 i_n_interval=3000 );
	bool				offset_cal_request( u32 i_n_target_count, u32 i_n_skip_count=3 );
	u32 				offset_cal_get_target_count(void){return m_n_offset_target_count;}
	bool				offset_cal_accumulate( u16* i_p_data, u32 i_n_line_byte_len );
	bool				offset_cal_make(void);
	void				offset_cal_stop(void);
	bool				offset_cal_is_started(void){return m_b_offset_cal_started;}
	bool				offset_cal_is_timeout(void);
	void				offset_cal_message( OFFSET_CAL_STATE i_state_e, u8* i_p_data=NULL, u16 i_w_data_len=0 );
	void				offset_cal_reset(void);
	bool				offset_cal_trigger(void);
	
	// notification
	void                set_notify_handler(bool (*noti)(vw_msg_t * msg));
	
	bool				is_cal_file( VW_FILE_DELETE i_file_delete_e );
	void				file_delete( VW_FILE_DELETE i_file_delete_e );
	
	bool				get_enable( CALIBRATION_TYPE i_type_e );
	void				set_enable( CALIBRATION_TYPE i_type_e, bool i_b_enable );
	bool				is_loaded( CALIBRATION_TYPE i_type_e );
	
#if 0
	void 				dfc_pixel_pre_processing(s32 i_n_x, s32 i_n_y, u8* i_p_dfc_type, defect_pixel_info_t* o_p_defect_info);
	bool 				dfc_vline_pre_processing(s32 i_n_x, s32 i_n_y, u8* i_p_dfc_type, defect_pixel_info_t* o_p_defect_info);
	bool 				dfc_hline_pre_processing(s32 i_n_x, s32 i_n_y, u8* i_p_dfc_type, defect_pixel_info_t* o_p_defect_info);
	void 				dfc_correction_pre_processing(u32 i_n_pixel_defect_cnt, u16 i_p_defect_info[][2]\
														,u32 i_n_hline_defect_cnt, u16 i_p_hline_defect_info[][3] \
														,u32 i_n_vline_defect_cnt, u16 i_p_vline_defect_info[][3]\
														,u32* o_n_total_defect_cnt, defect_pixel_info_t* o_p_defect_info);
	void 				dfc_correction_conversion();
#endif	
};


#endif /* end of _CALIBRATION_H_*/

