/******************************************************************************/
/**
 * @file    aed.h
 * @brief
 * @author  from FXMD-1008S aed.h
*******************************************************************************/

#ifndef _AED_H_
#define _AED_H_

/*******************************************************************************
*	Include Files
*******************************************************************************/
#include "typedef.h"
#include "vw_system_def.h"
#include "env.h"
#include "vw_time.h"
#include "mutex.h"

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
void*     aed_routine( void * arg );
void*     aed_stable_routine( void * arg );

class CAed
{
private:

    s8                  m_p_log_id[15];
    int			        (* print_log)(int level,int print,const char * format, ...);
    
    pthread_t			m_aed_thread_t;
    pthread_t			m_aed_stable_thread_t;
	
	aed_info_t			m_aed_t;
	CTime*              m_p_aed_stable_time_c;
	
	s32     			m_n_spi_fd;         // SPI   
	u8					m_c_hw_ver;
	
	bool				m_b_aed_proc_is_running;
	bool				m_b_aed_stable_proc_is_running;  

    CMutex*             m_p_aed_resource_lock_c;
    void                aed_resource_lock(void){m_p_aed_resource_lock_c->lock();}
    void                aed_resource_unlock(void){m_p_aed_resource_lock_c->unlock();}
	bool				m_aed_stable_on;
	
	void				aed_proc_stop(void);
	void				aed_stable_proc_stop(void);
		
	void				aed_proc(void);
	void				aed_stable_proc(void);
	
	int					aed_trig_get(AED_ID i_aed_id_e);
	int					aed_trig_measurement(AED_ID i_aed_id_e);
	bool				adjust_aed_offset(u8 i_c_aed_num);
	
	s32					potentiometer_write(u8 i_c_sel, u8 i_c_data);  
	bool				is_vw_revision_board(void);
public:
	CEnv*      			m_p_env_c;

	CAed(int (*log)(int,int,const char *,...));
	~CAed();

	void				initialize(u8 i_c_hw_ver);
	void				start(void);
	void				stop(void);
	bool				is_stable(void);
	bool				(*read_trig_volt)(AED_ID i_aed_id_e, u32* o_p_volt);
		
	u8					get_aed_bias_value(u8 i_aed_id);
	void				set_aed_bias_value(u8 i_aed_id, u8 i_c_data);
	u8					get_aed_offset_value(u8 i_aed_id);
	void				set_aed_offset_value(u8 i_aed_id, u8 i_c_data);
	void				set_aed_threshold_low_value(u8 i_aed_id, u16 i_w_data);
	u16					get_aed_threshold_low_value(u8 i_aed_id);
	void				set_aed_threshold_high_value(u8 i_aed_id, u16 i_w_data);
	u16					get_aed_threshold_high_value(u8 i_aed_id);
    
    u16					read(u8 i_c_aed_id, u32 i_n_addr);
    void				write(u8 i_c_aed_id, u32 i_n_addr, u16 i_w_value);

	friend void *		aed_routine( void * arg );
	friend void *		aed_stable_routine( void * arg );
};

#endif /* end of _AED_H_*/
