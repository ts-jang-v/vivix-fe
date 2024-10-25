/*
 * fpga.h
 *
 *  Created on: 2014. 2. 26.
 *      Author: myunghee
 */

 
#ifndef _FPGA_H_
#define _FPGA_H_

/*******************************************************************************
*	Include Files
*******************************************************************************/



#ifdef TARGET_COM
#include <sys/time.h>		// gettimeofday()
#include "typedef.h"
#include "mutex.h"
#include "vw_system_def.h"
#include "vw_time.h"        // sleep_ex
#include "fpga_prom.h"
#include "fpga_reg.h"
#else
#include "typedef.h"
#include "Mutex.h"
#include "Time.h"        // sleep_ex
#include "Fpga_prom.h"
#include "../app/detector/include/vw_system_def.h"
#include "../app/detector/include/fpga_reg.h"
#endif
/*******************************************************************************
*	Constant Definitions
*******************************************************************************/
#define REG_ADDRESS_MAX         (1024)

/*******************************************************************************
*	Type Definitions
*******************************************************************************/
typedef enum
{
    eUPDATE_STEP_NONE         = 0,
    eUPDATE_STEP_ERASE,
    eUPDATE_STEP_WRITE,
    eUPDATE_STEP_VERIFY,
    eUPDATE_STEP_END,
}UPDATE_STEP;


typedef struct _vw_regisger_t
{
    u16     addr;
    u16     data;
} vw_register_t;

typedef struct _reg_info_t
{
    u16             w_total;
    vw_register_t   reg_t[REG_ADDRESS_MAX];
} reg_info_t;

typedef union
{
	u32 n_data;
	float f_data;
	
}data_u;

/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*	Variable Definitions
*******************************************************************************/


/*******************************************************************************
*	Function Prototypes
*******************************************************************************/
void *         	config_routine( void * arg );

class CFpga
{
private:
    s8                  m_p_log_id[15];
    int			        (* print_log)(int level,int print,const char * format, ...);
	
    CMutex *		    io;
	void			    lock(void){io->lock();}
	void			    unlock(void){io->unlock();}
    
    void			    io_write( u16 i_w_addr, u16 i_w_data );
    u16				    io_read( u16 i_w_addr );
    
    // configuration
    bool                m_b_config_is_running;
    pthread_t	        m_thread_t;
    u16                 m_w_config_progress;
    bool                m_b_config_result;
    s8                  m_p_config_file_name[256];
    UPDATE_STEP         m_update_step_e;
    CPROM*              m_p_prom_c;
    void                config_proc(void);
    
    // register information
    MODEL               m_model_e;
    PANEL_TYPE			m_panel_type_e;
    SCINTILLATOR_TYPE	m_scintillator_type_e;
    SUBMODEL_TYPE	    m_submodel_type_e;
    ROIC_TYPE           m_roic_type_e;
    u8 mCtrlBoardVer;
    u16                 m_p_reg[REG_ADDRESS_MAX];
    void                reg_load_default(void);
    bool                reg_init_inner(void);
    u8 getCtrlVersion(void){return mCtrlBoardVer;}

    // driver file descriptor
    s32                 m_n_fd;
    
    // roic control
    s32                 m_n_fd_roic;
    void                roic_init_inner(void);
    
    // power state
    bool                m_b_power_on;
    bool                is_config_done(void);
    bool                is_register_ready(void);
    void                power_enable( bool i_b_on );
    
    // line interrupt
    CTime*              m_p_line_intr_c;
    CMutex *		    m_p_line_intr_lock_c;
	void			    line_intr_lock(void){m_p_line_intr_lock_c->lock();}
	void			    line_intr_unlock(void){m_p_line_intr_lock_c->unlock();}
    
    CTime*  			m_p_flush_wait_c;
        
    // roic power mode
    u8                  m_c_roic_idle;
    u8                  m_c_roic_drive;
    
    // aed select/deselect
    u16                 m_w_aed_select;
    
    bool                is_extra_flush_error(void);
    
    // exp req, exp ok interval time
    timeval				m_exp_req_timeval;
    struct timespec		m_exp_req_timespec;
    struct timespec		m_exp_ok_timespec;
    void				exposure_print_interval( s32 i_n_time1, s32 i_n_time2, s32 i_n_time3 );
    void				exposure_print_interval_to_file( s8* i_p_interval_time );
    
    bool				m_b_exp_ok;

    void				update_pseudo_flush_num(void);
    void				set_aed_error_time(void);
    
    u32					get_ddr_crc( u32 i_n_base_address, u32 i_n_crc_len );
    
    u32					get_roic_addr( u32 i_n_addr );
    void				set_roic_power_mode( u16 i_w_mode );
    
    // etc.
    void                change_reg_setting_for_MP(void);
protected:
public:
    
	CFpga(int (*log)(int,int,const char *,...));
	CFpga(void);
	~CFpga();
    
    // fpga configuration
    bool                config_start( const s8* i_p_file_name );
    void                config_stop(void);
    friend void *       config_routine( void * arg );
    u16                 config_get_progress(void);
    bool                config_get_result(void){ return m_b_config_result; }
    
    void                get_version( s8* o_p_ver );
    void                write( u16 i_w_addr, u16 i_w_data );
    u16                 read( u16 i_w_addr, bool i_b_saved=false );
    
    void                set_offset_calibration( bool i_b_set );
    u16                 trigger_get_type(void){return read(eFPGA_REG_TRIGGER_TYPE);}
    void                trigger_set_type(u16 i_w_type){return write(eFPGA_REG_TRIGGER_TYPE, i_w_type);}
    
    DEV_ERROR           hw_initialize( MODEL i_model_e, PANEL_TYPE i_panel_type_e, SCINTILLATOR_TYPE i_scintillator_type_e, SUBMODEL_TYPE i_submodel_type_e, u8 ctrlBoardVer);
    DEV_ERROR           register_roic_type(ROIC_TYPE i_roic_type_e);
    DEV_ERROR           roic_initialize(void);
    DEV_ERROR           reg_initialize(void);

    u16                 reg_save( u16 i_w_start, u16 i_w_end );
    bool                reg_save_file(void);
    void                reg_print( bool i_b_local=true );
    s32                 get_fd(void){return m_n_fd;}
    void                set_test_pattern( u16 i_w_type );
    void                set_capture_count( u32 i_n_count );
    u32                 get_capture_count(void);

    void                offset_start( bool i_b_start=true, u32 i_n_target_count=0, u32 i_n_skip_count=0 );
    void                offset_set_recapture( bool i_b_recapture );
    
    // FPGA power up/down
    u32					m_n_expected_wakeup_time;
    void                power_down(void);
    bool                power_up(void);
    bool                is_ready(u16 i_w_timeout=100, bool i_b_wakeup=false);
    void 				set_expected_wakeup_time(void);
    u32					get_expected_wakeup_time(void);
	    
    // test
    u16                 line_intr_check(void);
    void                line_intr_end(void);
    // test
    void                aed_trigger(void);
    
    void                set_immediate_transfer( bool i_b_enable );
    
    // pre exp flush number
    u16                 get_pre_exp_flush_num(void);
    
    // AED sensor select/deselect
    void                aed_backup(void);
    void                aed_restore(void);
    
    // 
    bool                is_flush_enough(void);
    
	u32                 get_vcom_swing_time(void);
    u16                 get_vcom_remain(void);
    
    bool                is_xray_detected_in_extra_flush_section(void);
    
    // re-initialization
    bool                reinitialize(void);
    
    u16                 get_temperature( u8 i_c_id = 0 );
    
    void                set_tg_time( u32 i_n_time=0 );
    
    void                print_trigger_time(void);
    
    bool				exposure_request(TRIGGER_TYPE i_type_e=eTRIGGER_TYPE_LINE);
    void				exposure_aed(void);
    u16					exposure_ok_get_wait_time(void);
    bool				exposure_ok(void);
    void				exposure_request_stop(bool i_b_time_stamp=true);

    void				offset_set_enable( bool i_b_enable );
    bool				offset_get_enable(void);

    void				gain_set_enable( bool i_b_enable );
    bool				gain_get_enable(void);
    void				gain_set_pixel_average( u16 i_w_average );

    void                enable_interrupt( bool i_b_enable );
    
    void				set_power_on_tft( bool i_on );
    void                set_tg_line_count( u8 i_c_count );
    
    void 				tg_power_enable( bool i_b_on );
    void 				ro_power_enable( bool i_b_on );
    
    bool				is_offset_data_valid( u32 i_n_crc, u32 i_n_crc_len );
    bool				is_gain_data_valid( u32 i_n_crc, u32 i_n_crc_len );
    
    void				device_pgm(void);
    
    void				roic_dac_write_sel( u8 i_c_sel, u16 i_w_data );
	void				dac_setup(void);
    
    u16					roic_reg_read( u32 i_n_addr );
    void				roic_reg_write( u32 i_n_addr, u16 i_w_data );
    u16					roic_reg_read( u8 i_c_idx, u32 i_n_addr );
    void				roic_reg_write(  u8 i_c_idx, u32 i_n_addr, u16 i_w_data );
    
    void				roic_info(void);
    void				roic_reset(void);
    void				roic_sync(void);
    void				roic_delay_compensation(void);
    void				roic_bit_alignment(void);
    void				roic_power_down();
    void				roic_power_up();
    void				roic_bit_align_test(u16 w_retry_num);
    
    void				double_readout_enable(bool i_b_en);
    bool				is_double_readout_enabled(void);
    
    void				set_gain_type(void);
    void				print_gain_type_table(void);
    void				set_sensitivity();
    u16                 get_roic_cmp_gain_by_scintillator_type(SCINTILLATOR_TYPE i_e_scintillator_type);
    u16                 get_input_charge_by_scintillator_type(SCINTILLATOR_TYPE i_e_scintillator_type);
    void                set_scintillator_type(SCINTILLATOR_TYPE	i_e_scintillator_type){ m_scintillator_type_e = i_e_scintillator_type;};

    u32					get_exp_time(void);
    GEN_INTERFACE_TYPE  get_acq_mode(void);

    void				prom_access_enable(bool i_b_en){m_p_prom_c->set_flash_access_enable(i_b_en);}
    void				prom_qpi_enable(bool i_b_en){m_p_prom_c->set_qpi_mode(i_b_en);}
    
    void				print_line_error(void);
    void				disable_aed_trig_block(void);
    bool				is_disable_aed_trig_block(void);
    
    void				apply_gain_table_by_panel_type(PANEL_TYPE i_panel_type_e);
    
    void				flush_wait_end(void);
    void				tft_aed_control(void);
    bool				is_gate_line_defect_enabled(void);
    void				gate_line_defect_enable(bool i_b_enable);

    //smart w
    u8  				get_smart_w_onoff(void);
    u8  				set_smart_w_onoff(u8 i_enable);

    //aed off debounce time 
    u16  				get_aed_off_debounce_time(void);
    u8  				set_aed_off_debounce_time(u16 i_w_debounce_time);

    u16                 hw_get_sig_version(void);

    void SetReadoutSync(void);
    u16 GetReadoutSync(void);
};


#endif /* end of _FPGA_H_*/

