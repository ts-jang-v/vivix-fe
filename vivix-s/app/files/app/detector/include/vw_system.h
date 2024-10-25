/*
 * system.h
 *
 *  Created on: 2014. 03. 04.
 *      Author: myunghee
 */

 
#ifndef _VW_SYSTEM_H_
#define _VW_SYSTEM_H_

/*******************************************************************************
*	Include Files
*******************************************************************************/
#include "typedef.h"
#include "message.h"
#include "env.h"
#include "fpga.h"
#include "hw_monitor.h"
#include "vw_time.h"
#include "image_def.h"
#include "mutex.h"
#include "sync.h"
#include "vw_net.h"
#include "image_process.h"
#include "misc.h"
#include "aed.h"
#include "oled_image.h"
#include "micom.h"

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/

/*******************************************************************************
*	Type Definitions
*******************************************************************************/
/**
 * @brief   power management timer
 */
typedef enum
{
    ePM_TIMER_SLEEP = 0,
    ePM_TIMER_SHUTDOWN,
    
} PM_TIMER;

typedef enum
{
    eCHARGING = 0,
    eSLEEP_MODE_OFF,
    eKEEP_NORMAL,
    eIMAGE_GRAB,
    eFPGA_NOT_READY,
    eSLEEPING,
    
} GO_SLEEP_ERROR;


/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*	Variable Definitions
*******************************************************************************/


/*******************************************************************************
*	Function Prototypes
*******************************************************************************/
void *         	update_routine( void * arg );
void *         	pm_state_routine( void * arg );
void *         	monitor_routine( void * arg );
void *         	button_routine( void * arg );
void *         	osf_routine( void * arg );

class CVWSystem
{
private:
    s8                  m_p_log_id[15];
    int			        (* print_log)(int level,int print,const char * format, ...);
    
	// settings and controls
	bool                m_b_ready;
    CHWMonitor*         m_p_hw_mon_c;
    CVWOLEDimage*		m_p_oled_c;
    CImageProcess*		m_p_preview_image_process_c;
    bool                m_b_readout_started;
    void                set_readout_start(bool i_b_started);
    bool                m_b_image_grab;
    
    DEV_ERROR           m_dev_err_e;
    image_size_t        m_image_size_t;

    u32                 m_n_image_size_byte;
    bool                m_b_offset_cal_started;
    void                offset_cal_message( OFFSET_CAL_STATE i_state_e, u8* i_p_data=NULL, u16 i_w_data_len=0 );
    bool                offset_cal_request( u32 i_n_target_count, u32 i_n_skip_count=3 );

    CMutex*             m_p_charge_time_mutex_c;
    void                charge_time_lock(void){m_p_charge_time_mutex_c->lock();}
    void                charge_time_unlock(void){m_p_charge_time_mutex_c->unlock();}

    CMutex*             m_p_iw_command_mutex_c;

    CMutex*             m_p_grab_mutex_c;
    void                grab_lock(void){m_p_grab_mutex_c->lock();}
    void                grab_unlock(void){m_p_grab_mutex_c->unlock();}

    bool                m_b_reboot;
    NET_MODE            m_current_net_mode_e;   // 한 thread 내에서만 값이 변경되도록 해야 한다.
    u16					m_w_tether_speed;
    
    u16					m_w_normal_gate_state;
    
    // exp_req
    bool                m_b_sw_exp_req;
    u16                 m_w_saved_trigger_type;
    
    // notification
    bool                (* notify_handler)(vw_msg_t * msg);
    void                message_send( MSG i_msg_e, u8* i_p_data=NULL, u16 i_w_data_len=0 );
    void                message_ready_for_exposure(void);
    
    // update
    bool                m_b_update_is_running;
    pthread_t	        m_update_thread_t;
    u16                 m_w_update_progress;
    bool                m_b_update_result;
    VW_FILE_ID          m_file_id_e;
    s8                  m_p_update_file_name[256];
    void                update_proc(void);
    bool                update_proc_start( VW_FILE_ID i_id_e, const s8* i_p_file_name );
    void                update_proc_stop(void);
    
    bool                mmc_write( VW_FILE_ID i_id_e, const s8* i_p_file_name );
    void                mmc_write_bin_file(const s8* i_p_src_file, const s8* i_p_dst_file);
    bool                mmc_verify( VW_FILE_ID i_id_e, const s8* i_p_file_name );
    
    // power management
    bool                m_b_pm_is_running;
    pthread_t	        m_pm_thread_t;
    PM_MODE             m_pm_mode_e;
    bool                m_b_pm_keep_normal;
    pm_config_t         m_pm_config_t;
    CTime*              m_p_sleep_wait_c;
    bool                m_b_need_wake_up;
    bool                m_b_pm_test_sleep_start;
    bool                m_b_pm_wake_up;
    bool				m_b_go_sleep;
    
    CMutex*             m_p_pm_mutex_c;
    void                pm_lock(void){m_p_pm_mutex_c->lock();}
    void                pm_unlock(void){m_p_pm_mutex_c->unlock();}
    
    void                pm_init(void);
    void                pm_state_proc(void);
    PM_MODE             pm_check_normal(void);
    PM_MODE             pm_check_sleep(void);
    PM_MODE             pm_check_shutdown(void);
    PM_MODE             pm_check_wakeup(void);
    bool                pm_is_timer_expired( PM_MODE i_mode_e, PM_TIMER i_timer_e );
    void                pm_print_mode( PM_MODE i_mode_e );
    bool                pm_prepare_sleep(void);
    bool                pm_get_temperature( u16* o_p_temperature );
    
    // image
    image_buffer_t*     m_p_copy_image_t;
    
    // file(offset, defect, gain, config.xml, ...)
    u32                 m_n_file_size;
    u8*                 m_p_file_data;
    
    // H/W Settings
    void                hw_set_config(void);
    
    // line interrupt
    bool                m_b_immediate_transfer;     // readout and acquisition

    // monitoring logger
    bool                m_b_mon_is_running;
    pthread_t	        m_mon_thread_t;
    
    s8                  m_p_mon_file_name[256];
    u16                 m_w_mon_time_sec;
    bool                m_b_mon_print;
    u32                 m_n_mon_image_count;
    u32                 m_n_mon_offset_count;
    
    void                monitor_init(void);
    void                monitor_log( sys_state_t i_stat_t );
    void                monitor_get_time( s8* o_p_time_string );
    FILE*				monitor_get_file(void);
    
    void                monitor_proc(void);
    void                monitor_proc_start(void);
    void                monitor_proc_stop(void);
    
    void				monitor_xray_detection(void);
    
    // button routine   
    bool				m_b_is_ap_button_actived; 
    bool                m_b_button_thread_is_running;
    pthread_t	        m_button_thread_t;  
    bool				m_b_ap_switching_mode;
    
    void                button_proc_start(void);
    void                button_proc_stop(void);
    
     
    // state
    sys_state_t         m_sys_state_t;
    void                get_state_battery(sys_state_t* o_p_state_t);
    void                get_state_power(sys_state_t* o_p_state_t);
    void                get_state_wlan(sys_state_t* o_p_state_t);
    void                get_state_ap_stations(sys_state_t* o_p_state_t);
    void                reset_state_wireless(sys_state_t* o_p_state_t);
    WIFI_STEP_NUM		m_wifi_step_e;
    // sw connection
    bool                m_b_sw_connected;
    
    bool                m_b_auto_offset;
    auto_offset_t       m_auto_offset_t;
    u16                 m_w_old_temp;
    CTime*              m_p_auto_offset_time_c;
    void                auto_offset_set( bool i_b_set );
    void                auto_offset_check(void);
    
    // ghost removal function
    u16                 m_w_grf_panel_bias;
    u16                 m_w_grf_extra_flush;
    void                grf_reset(void);
    void                grf_restore(void);

    // offset stabilization function
    bool                m_b_osf_is_running;
    pthread_t	        m_osf_thread_t;
    u16                 m_w_osf_trigger_total;
    u16                 m_w_osf_trigger_count;
    u16                 m_w_osf_trigger_time;
    u16                 m_w_osf_saved_vcom;
    u16                 m_w_osf_saved_trigger_type;
    u16                 m_w_osf_saved_extra_flush;
    u16                 m_w_osf_saved_digital_offset;
    u16                 m_w_osf_saved_vcom_min;
    
    bool                m_b_osf_saved_gain;
    bool                m_b_osf_saved_defect;
    
    float*              m_p_osf_data;       // line average
    u32                 m_n_osf_line_count;
    bool                m_b_osf_vcom_started;
    
    void                osf_proc(void);
    void                osf_backup(void);
    void                osf_restore(void);
    bool                osf_trigger( bool i_b_vcom );
    bool                osf_check_vcom(void);
    void                osf_save(void);
    
    // detector position
    s32                 m_p_position[3];
    
    // preview
    bool                m_b_preview_enable;
    
    // fast tact-time mode
    bool                m_b_tact_enable;
    u16                 m_w_tact_id;
    u32                 m_n_tact_vcom_swing_time;
    u16                 tact_get_last_id(void);

    // capture state
    bool                m_b_cpature_ready;
    u16                 m_w_capture_init_count;
    CTime*              m_p_capture_init_c;
    CTime*              m_p_cpature_ready_c;
    void                capture_check_mode(void);
    bool                capture_is_ready( PM_MODE i_pm_mode_e );
    void                capture_init(void);
    void                capture_check_ready_next_exposure(void);
    bool				capture_is_dev_ready(void);
    
    // image transmission test
    bool                m_b_test_image;
    
    // image transmission done
    bool                m_b_trans_done_enable;
    bool                m_b_trans_done;
    
    bool                m_b_scu_connected;

    u32					m_n_image_send_remain_time;
    
    bool				m_b_web_ready;
    
    patient_info_t		m_patient_info_t;
    u16					m_w_saved_image_count;

    u32					m_n_img_direction;
    bool				m_b_img_direction_user_input;
    void				image_direction_set(void);
    
    
    bool                (* cal_data_load)(void);
    
    bool				m_b_offset_enable;
    bool				m_b_gain_enable;
    u16					m_w_gain_average;
    
    u32					m_n_offset_crc;
    u32					m_n_offset_crc_len;
    
    u32					m_n_gain_crc;
    u32					m_n_gain_crc_len;

    bool				m_b_offset_load;
    bool				m_b_gain_load;
    
    void				enable_image_correction(void);
    bool				is_cal_data_valid(void);
    void				wait_until_cal_data_load_end(void);
    
    void				aed_wait_end(void);
        	
    u32					m_n_last_saved_image_id;
    
    CTime*				m_p_charge_time_c;
    CTime*              m_p_wifi_low_noti_wait_time_c;
    //bool                m_b_is_charge_block;
    
protected:
public:
    
	CVWSystem(int (*log)(int,int,const char *,...));
	CVWSystem(void);
	~CVWSystem();
	
	CFpga*              m_p_fpga_c;
	CAed*               m_p_aed_c;
    CEnv*               m_p_env_c;
    CMicom*				m_p_micom_c;
	void                initialize(void);
	DEV_ERROR           err_get_code(void){return m_dev_err_e;}
	// system ready
	void                start(void);
	bool                is_ready(void);
	
	bool				m_b_cal_data_load_end;
	    
	// system information
	u16                 get_offset_state(void){return 0;}
    void                get_state( sys_state_t* o_p_state_t, bool i_b_direct=false );
    void                print_state(void);
    bool				get_state_wlan_lock( const s8* i_p_dev_name, vw_wlan_state_t* o_p_state );

    //iw command
    void                iw_command_lock(void){m_p_iw_command_mutex_c->lock();}
    void                iw_command_unlock(void){m_p_iw_command_mutex_c->unlock();}

    // backup image
    u16                 backup_image_get_count(void);
    void                backup_image_delete( u32 i_n_id, u32 i_n_timestamp=0 );
    void                backup_image_delete_oldest(void);
    bool                backup_image_save( image_header_t i_header_t, u8* i_p_data );
    void                backup_image_make_list(void);
    bool                backup_image_find( u32 i_n_image_id, s8* o_p_file_name );
    bool                backup_image_is_saved( u32 i_n_image_id );
    
    // h/w information
    void                get_image_size_info( image_size_t* o_p_image_size_t );
    u32                 get_image_size_byte(void){return m_n_image_size_byte;}
    u32                 get_image_id(void);
    ROIC_TYPE           get_roic_model_type(void);
    PHY_TYPE            get_phy_model_type(void);
    
    void                set_copy_image( image_buffer_t* i_p_image_t ){m_p_copy_image_t = i_p_image_t;}
    image_buffer_t*     get_copy_image(void){return m_p_copy_image_t;}
    
    u8                  hw_get_ctrl_version(void){return m_p_hw_mon_c->hw_get_ctrl_version();}
    u8                  hw_get_sig_version(void){return m_p_fpga_c->hw_get_sig_version();}
    bool                is_vw_revision_board(void){return m_p_hw_mon_c->is_vw_revision_board();}
    MODEL               get_model(void){return m_p_env_c->get_model();}
    void                hw_power_off( bool i_b_sw=false );
    void                hw_reboot(void);
    void                hw_get_power_volt( power_volt_t* o_p_pwr_volt_t );
    void                hw_bat_charge(BAT_CHARGE i_e_charge_state);
    POWER_SOURCE        hw_get_power_source(void){return m_p_hw_mon_c->get_power_source();}
    void 				hw_phy_reset(void){m_p_hw_mon_c->phy_reset();}
    void 				hw_phy_down(void){m_p_hw_mon_c->phy_down();}
    void 				hw_phy_up(void){m_p_hw_mon_c->phy_up();}
    bool				get_link_state(void);
    bool                hw_impact_diagnosis(void){return m_p_hw_mon_c->impact_diagnosis();}
    bool                hw_gyro_diagnosis(void){return m_p_hw_mon_c->acc_gyro_probe(); }
    bool                hw_bat_diagnosis(u8 i_c_sel){return m_p_hw_mon_c->bat_diagnosis(i_c_sel);}
    bool                hw_gyro_probe(void){return m_p_hw_mon_c->is_gyro_exist();}

    u8                	hw_bat_equip_start_idx(void){return m_p_hw_mon_c->get_bat_equip_start_idx();}
    void                sw_bat_qucik_start(u8 i_c_sel){return m_p_hw_mon_c->bat_sw_quick_start(i_c_sel);};
    
    bool				hw_get_tether_equip(void)	{return m_p_hw_mon_c->gpio_get_tether_equip();}
    void				power_control_for_phy(void);
    void                aed_power_control(GEN_INTERFACE_TYPE i_acq_mode, TRIGGER_SELECT i_e_trig, u8 i_c_smart_w_enable);

	// OLED
	void				oled_init(void) {m_p_oled_c->oled_init();}
    void				oled_sw_reset(void) {m_p_oled_c->oled_sw_reset();}
	void				oled_control(u8 i_c_addr, u8* i_p_data, u32 i_n_length){m_p_oled_c->oled_control(i_c_addr, i_p_data, i_n_length);}
	void				update_oled_state(sys_state_t* i_p_state_t);
	void				set_oled_display_event(OLED_EVENT i_event_e);
	void				set_ap_switching_mode(bool i_b_enable);
	bool				is_ap_switching_mode(void){return m_b_ap_switching_mode;}
	void				set_oled_error(OLED_ERROR i_error_e, bool i_b_err_flag){m_p_oled_c->update_error(i_error_e, i_b_err_flag);}
			
    // get impact reg
    u8					get_impact_reg(u8 i_c_addr) {return m_p_hw_mon_c->get_impact_reg(i_c_addr);}
    void				set_impact_reg(u8 i_c_addr, u8 i_c_data) {m_p_hw_mon_c->set_impact_reg(i_c_addr,i_c_data);}
    void                impact_print(void){m_p_hw_mon_c->impact_print();}
    u8                	impact_get_threshold(void){return m_p_hw_mon_c->impact_get_threshold();}
    void                impact_set_threshold(u8 i_c_threshold){m_p_hw_mon_c->impact_set_threshold(i_c_threshold);}
    void                impact_log_save(int i_n_cnt){m_p_hw_mon_c->impact_log_save(i_n_cnt);}
    void				impact_calibartion_offset_save(void);

    void				angular_offset_cal_save(void);

    void                read_detector_position(void);
    void                get_main_detector_angular_position( s32* o_p_x, s32* o_p_y, s32* o_p_z);
   	void                get_sub_detector_angular_position( s32* o_p_x, s32* o_p_y, s32* o_p_z );

    
    void                impact_calibration_start(void);
    void                impact_calibration_stop(void);
    u8                  impact_calibration_get_progress(void);
    bool                get_main_angular_offset_cal_process_result(void);
    bool                get_sub_angular_offset_cal_process_result(void);
	void				set_impact_extra_detailed_offset(s16* i_w_x_offset = NULL, s16* i_w_y_offset = NULL, s16* i_w_z_offset = NULL);
    void				get_impact_offset(s16* o_w_x_offset, s16* o_w_y_offset, s16* o_w_z_offset);
    void                impact_stable_pos_proc_start(void){m_p_hw_mon_c->impact_stable_pos_proc_start();}
    void                impact_stable_pos_proc_stop(void){m_p_hw_mon_c->impact_stable_pos_proc_stop();}
    bool				impact_stable_proc_is_running(void){return m_p_hw_mon_c->impact_stable_proc_is_running();}
    void				impact_stable_proc_hold(void){m_p_hw_mon_c->impact_stable_proc_hold();}
    void				impact_stable_proc_restart(void){m_p_hw_mon_c->impact_stable_proc_restart();}
    void				impact_set_cal_direction(ACC_DIRECTION i_dir_e){m_p_hw_mon_c->impact_set_cal_direction(i_dir_e);}
    u8					get_6plane_impact_sensor_cal_done(void){return m_p_hw_mon_c->get_6plane_impact_sensor_cal_done();}
    void				save_impact_6plane_cal_done(void);  
    u8					get_6plane_impact_sensor_cal_misdir(void){return m_p_hw_mon_c->get_6plane_impact_sensor_cal_misdir();}
    u8					get_combo_impact_cal_done(void){return m_p_hw_mon_c->get_6plane_impact_sensor_cal_done();}
     	
    bool                offset_cal_trigger(void);
    void                offset_cal_stop(void);
    bool                offset_cal_start( u32 i_n_target_count, u32 i_n_skip_count );
    void				offset_update_count(void){m_n_mon_offset_count++;}
	
    // configuration of system
    void                print(void){m_p_env_c->print();}
    void                config_reset(void);
    s8*                 config_get( CONFIG i_cfg_e ){ return m_p_env_c->config_get(i_cfg_e);}
    void                config_set( CONFIG i_cfg_e, const s8* i_p_value ){m_p_env_c->config_set(i_cfg_e, i_p_value);}
    void                config_save(void){m_p_env_c->config_save(CONFIG_XML_FILE);}
    void                config_reload(bool i_b_forced=false);
    bool				config_net_is_changed(void);
    bool				config_change_preset( const s8* i_p_ssid );
    void                preset_load(void){
    					sys_command("cp %s%s %s", SYS_CONFIG_PATH, PRESET_XML_FILE, SYS_RUN_PATH );
    					m_p_env_c->preset_load();}					
  	s32					config_get_preset_index(const s8* i_p_ssid){return m_p_env_c->get_preset_index(i_p_ssid);}
    
    void                manufacture_info_get( manufacture_info_t* o_p_info_t ){m_p_env_c->manufacture_info_get(o_p_info_t);}
    void                manufacture_info_set( manufacture_info_t* i_p_info_t );
    void                manufacture_info_print(void){m_p_env_c->manufacture_info_print();}
    void                manufacture_info_reset(void){m_p_env_c->manufacture_info_reset();}

    void                oper_info_reset(void);
    void                oper_info_read(void){m_p_env_c->oper_info_read();}
    void                oper_info_print(void){m_p_env_c->oper_info_print();}
    u32                 eeprom_read( u16 i_w_addr, u8* o_p_data, u32 i_n_len ){return m_p_env_c->eeprom_read(  i_w_addr,  o_p_data,  i_n_len );}
    u32                 eeprom_write( u16 i_w_addr, u8* i_p_data, u32 i_n_len ){return m_p_env_c->eeprom_write(  i_w_addr,  i_p_data,  i_n_len );}
    
    // dev
    void				dev_info( DEV_ID i_id_e );
    u16                 dev_reg_save( DEV_ID i_id_e, u16 i_w_start, u16 i_w_end );
    bool                dev_reg_save_file( DEV_ID i_id_e );
    u16                 dev_reg_read( DEV_ID i_id_e, u32 i_w_addr );
    u16                 dev_reg_read_saved( DEV_ID i_id_e, u16 i_w_addr );
    bool                dev_reg_write( DEV_ID i_id_e, u32 i_w_addr, u16 i_w_data );
    s32                 dev_get_fd( DEV_ID i_id_e );
    void                dev_set_test_pattern( u16 i_w_type );
    void				dev_load_trigger_type_config(void);
    void                dev_reg_print( DEV_ID i_id_e );
    void                dev_reg_init( DEV_ID i_id_e );
    void                dev_power_down(void){m_p_fpga_c->power_down();}
    bool                dev_power_up(void){return m_p_fpga_c->power_up();}
     bool				dev_is_ready(void){return m_p_fpga_c->is_ready(0, false);}
    bool                fpga_reinit(void);
    void                print_gain_type_table(void){m_p_fpga_c->print_gain_type_table();}
    
    // network
	void				net_set_mode( NET_MODE i_net_mode_e ){m_current_net_mode_e = i_net_mode_e;}
	NET_MODE			net_get_mode(void){return m_current_net_mode_e;}
	void				net_set_tether_speed( u16 i_w_speed );
	bool				net_is_need_reboot(void){return m_b_reboot;}
	bool				net_is_valid_speed(void);
	
    void                net_get_mac( u8* o_p_mac ){m_p_env_c->get_mac( o_p_mac );}
    void                net_set_mac( u8* i_p_mac ){m_p_env_c->set_mac( i_p_mac );}
    
    u32                 net_get_ip(void){return m_p_env_c->get_ip();}
    u32                 net_get_subnet(void){return m_p_env_c->get_subnet();}
    
    bool            	model_is_wireless(void){return m_p_env_c->model_is_wireless();}
    SUBMODEL_TYPE		get_submodel_type(void){return m_p_env_c->get_submodel_type();}
    PANEL_TYPE			get_panel_type(void){return m_p_env_c->get_panel_type();}
    SCINTILLATOR_TYPE	get_scintillator_type(void){return m_p_env_c->get_scintillator_type();}
    SCINTILLATOR_TYPE   get_scintillator_type_default(void){ return m_p_env_c->get_scintillator_type_default();}
    u32                 verify_manufacture_info(manufacture_info_t* i_manufacture_info_t){return m_p_env_c->verify_manufacture_info(i_manufacture_info_t);};

    u32                 set_logical_digital_gain(u32 i_logical_digital_gain);
    u32                 get_logical_digital_gain(void);
    u32                 set_gain_compensation(u32 i_gain_compensation);
    u32                 get_gain_compensation(void);

    // wireless
    u16                 wireless_get_level( u16 i_w_qaulity );
    
	void                button_proc(void);
	void				activate_ap_button_function(void);
	void				deactivate_ap_button_function(void);
	bool				is_ap_button_function_activated(void);  
	
    bool            	net_use_only_power(void){return m_p_env_c->net_use_only_power();}
    bool            	net_is_ap_on(void){return m_p_env_c->net_is_ap_enable();}
    bool            	net_is_dhcp_enable(void){return m_p_env_c->net_is_dhcp_enable();}
    bool                net_is_valid_channel( u16 i_w_num ){return m_p_env_c->net_is_valid_channel(i_w_num);}
    
    const s8*           get_model_name(void){return m_p_env_c->get_model_name();}
    const s8*           get_device_version(void){return m_p_env_c->get_device_version();}
    const s8*           get_serial(void){return m_p_env_c->get_serial();}
    
    // exposure request
    bool                exposure_request( TRIGGER_TYPE i_type_e=eTRIGGER_TYPE_LINE );
    bool				exposure_ok(void){return m_p_fpga_c->exposure_ok();}
    u16					exposure_ok_get_wait_time(void){return m_p_fpga_c->exposure_ok_get_wait_time();}
    void                exposure_request_enable(void);
    u32                 get_exp_wait_time(void);
    u16                 get_pre_exp(void);
    void                aed_trigger(void){ m_p_fpga_c->aed_trigger();}
    u16                 get_image_error(void);
    
    // preview
    bool                preview_get_enable(void);
    void                preview_set_enable(bool i_b_enable){m_b_preview_enable = i_b_enable;}
    
    // notification
    void                set_notify_handler(bool (*noti)(vw_msg_t * msg));
    
	// system update
	bool                update_get_info( u8* o_p_data, u32* o_p_len );
	bool                update_set_info( u8* i_p_data, u32 i_n_len );
	bool                update_start( VW_FILE_ID i_id_e, const s8* i_p_file_name );
	void                update_stop( VW_FILE_ID i_id_e );
	u16                 update_get_progress( VW_FILE_ID i_id_e );
	bool                update_get_result( VW_FILE_ID i_id_e );
	friend void *       update_routine( void * arg );
	
	// power management
	friend void *       pm_state_routine( void * arg );
    bool                pm_is_normal_mode(void);
    void                pm_set_keep_normal(bool i_b_set){m_b_pm_keep_normal = i_b_set;}
    void                pm_wake_up( bool i_b_by_sw=false );
    void                pm_test_sleep( bool i_b_start ){m_b_pm_test_sleep_start=i_b_start;}
    bool                pm_reset_sleep_timer(void);
    u32					pm_get_remain_sleep_time(void);
    u16					pm_go_sleep(void);
    
    // roic
	void				roic_sync(){m_p_fpga_c->roic_sync();};	
	void				roic_delay_compensation(){m_p_fpga_c->roic_delay_compensation();};
	void				roic_bit_align_test(u16 w_retry_num){m_p_fpga_c->roic_bit_align_test(w_retry_num);};
	void				roic_bit_alignment(){m_p_fpga_c->roic_bit_alignment();};
	void				roic_power_down(){m_p_fpga_c->roic_power_down();};
	void				roic_power_up(){m_p_fpga_c->roic_power_up();};
	
	u16					roic_reg_read( u8 i_c_idx, u32 i_n_addr ){ return m_p_fpga_c->roic_reg_read(i_c_idx, i_n_addr); };
	void				roic_reg_write(  u8 i_c_idx, u32 i_n_addr, u16 i_w_data ){ return m_p_fpga_c->roic_reg_write(i_c_idx, i_n_addr, i_w_data);};


    // battery
    void                bat_print(void){m_p_hw_mon_c->bat_print();}
    void                bat_set_soc_factor( u32 i_n_idx, s32 i_n_full, s32 i_n_empty ){m_p_hw_mon_c->bat_set_soc_factor(i_n_idx, i_n_full, i_n_empty);}
    u8					get_battery_count(void){return m_p_env_c->get_battery_count();}
    u8					get_battery_slot_count(void){return m_p_env_c->get_battery_slot_count();}
    void                check_need_to_block_recharging(void);
    void                force_to_stop_to_block_recharging(void);
    u32                 get_battery_charging_mode(void);
    void                set_battery_charging_mode(u32 i_bat_chg_mode);
    u8                  get_battery_recharging_gauge(void);
    void                set_battery_recharging_gauge(u8 i_rechg_gauge);
    bool                is_recharging_gauge_out_of_range(u8 i_rechg_gauge);
    void                bat_rechg_print(void){m_p_hw_mon_c->bat_rechg_print();}
    
    // uart display
    void				uart_display(bool a_b_en) { m_p_hw_mon_c->set_uart_display(a_b_en);}
    
    // capture state
    void                capture_end(void);
    
    void                set_immediate_transfer( bool i_b_enable );
    bool                get_immediate_transfer(void){return m_b_immediate_transfer;}
        
    // file(offset, defect, gain, config.xml, ...)
    bool                file_open( VW_FILE_ID i_id_e, u32* o_p_size, u32* o_p_crc, u32 i_n_image_id=0 );
    bool                file_read( u32 i_n_pos, u32 i_n_size, u8* o_p_data, u32* o_p_size );
    void                file_close(void);
    void                file_delete(VW_FILE_DELETE i_file_delete_e);

	// monitoring
	friend void *       monitor_routine( void * arg );
	void                monitor_print( bool i_b_start=true, u32 i_n_time=0 );
	
	// sw connection
	void                set_sw_connection( bool i_b_connected );
	void                set_image_state( u32 i_n_state );
	
	 // pmic
    void				get_pmic_reg(u8 a_b_addr,u8 *a_p_reg) {m_p_hw_mon_c->get_pmic_reg(a_b_addr,a_p_reg);}
    void				set_pmic_reg(u8 a_b_addr,u8 a_b_reg) {m_p_hw_mon_c->set_pmic_reg(a_b_addr,a_b_reg);}
    void				pmic_reset(void){ m_p_hw_mon_c->pmic_reset(); }
    
    // offset stabilization function
    bool                osf_is_started(void){return m_b_osf_is_running;}
    bool                osf_accumulate( u16* i_p_data, u32 i_n_line_byte_len );
    void                osf_message( OFFSET_DRIFT_STATE i_state_e, u8* i_p_data=NULL, u16 i_w_data_len=0 );
    bool                osf_start( u16 i_w_count, u16 i_w_time );
    void                osf_stop(void);
    friend void *       osf_routine( void * arg );
    u32                 osf_get_vcom_swing_time(void){return m_p_fpga_c->get_vcom_swing_time();}

    // Fast Tact-time Mode
    bool                tact_get_enable(void);
    void                tact_set_enable(bool i_b_enable);
    u16                 tact_get_image_count(void);
    void                tact_delete_image( u32 i_n_id, u32 i_n_timestamp );
    bool                tact_save_image( image_header_t i_header_t, u8* i_p_data, bool i_b_last_half );
    bool                tact_save_image( image_header_t i_header_t, u8* i_p_data );
    u16                 tact_get_id(void);
    void                tact_flush_image(void);
    void                tact_update_id(void);
    
    void                image_check_remain_memory(void);
    u16					image_get_saved_count(void){return m_w_saved_image_count;}
    bool				image_is_capture(void){return m_b_image_grab;}
    
    // image transmission test
    void                test_image_enable( bool i_b_enable ){m_b_test_image = i_b_enable;}
    bool                test_image_is_enabled(void){return m_b_test_image;}
    void                test_image_change_xray_delay_num( bool i_b_on );
    
    // image transmission done
    void                trans_done_enable( bool i_b_enable ){m_b_trans_done_enable = i_b_enable;}
    void                trans_done( bool i_b_done ){m_b_trans_done = i_b_done;}
    bool                trans_is_done(void);
    
    void                scu_set_connection( bool i_b_connected );
    
    void				set_image_send_remain_time( u32 i_n_time ){ m_n_image_send_remain_time = i_n_time;}
    u32					get_image_send_remain_time(void){ return m_n_image_send_remain_time;}
    
    // web
    bool				is_web_enable(void);
    bool				is_web_ready(void){return m_b_web_ready;}
    void				web_set_ready( bool i_b_ready ){m_b_web_ready = i_b_ready;}
    
    void				patient_info_set( patient_info_t i_info_t );
 	
	void				offset_set_enable( bool i_b_enable );
    bool				offset_get_enable(void);
    void				offset_set_crc( u32 i_n_crc, u32 i_n_len ){m_n_offset_crc = i_n_crc;m_n_offset_crc_len=i_n_len;}
    void				offset_set_load( bool i_b_load ){m_b_offset_load = i_b_load;}

    void				defect_set_enable( bool i_b_enable ){m_p_preview_image_process_c->defect_set_enable(i_b_enable);}
    bool				defect_get_enable(void){return m_p_preview_image_process_c->defect_get_enable();}

    void				gain_set_enable( bool i_b_enable );
    bool				gain_get_enable(void);
    void				gain_set_pixel_average( u16 i_w_average );
    u16					gain_get_pixel_average(void){return m_w_gain_average;}
    void				gain_set_crc( u32 i_n_crc, u32 i_n_len ){m_n_gain_crc = i_n_crc;m_n_gain_crc_len=i_n_len;}
    void				gain_set_load( bool i_b_load ){m_b_gain_load = i_b_load;}
    
    u16					get_digital_offset(void);
    bool				is_battery_saving_mode(void);
    void				restart_battery_saving_mode(void);
    bool				is_max_saturation_enabled(void){return (bool)m_p_fpga_c->read(eFPGA_REG_MSC_EN);}
    	
    void				capture_ready(void);
    void                set_cal_data_load_handler(bool (*load)(void)){cal_data_load = load;}
    
    void				display_update(void){}
    
    u16					get_direction(void);
    
    void				set_power_on_tft( bool i_on ){m_p_fpga_c->set_power_on_tft(i_on);}
    void                set_tg_line_count( u8 i_c_count ){m_p_fpga_c->set_tg_line_count(i_c_count);}

    CImageProcess*		get_image_process(void)	{ return m_p_preview_image_process_c;}
    void 				defect_correction( u16* i_p_buffer, u32 i_n_cur_line_index )\
    														{m_p_preview_image_process_c->defect_correction(i_p_buffer,i_n_cur_line_index);}
    void 				defect_correction_idx_reset(void)	{ m_p_preview_image_process_c->defect_correction_idx_reset(); }
    void 				defect_print(void)	{ m_p_preview_image_process_c->defect_print();}
    
    bool				is_lower_grid_line_support(void){return m_p_env_c->is_lower_grid_line_support();}
    
    void 				hw_tg_power_enable( bool i_b_on ) { m_p_fpga_c->tg_power_enable(i_b_on);}
    void 				hw_ro_power_enable( bool i_b_on ) { m_p_fpga_c->ro_power_enable(i_b_on);}
    
    void				device_pgm(void){m_p_fpga_c->device_pgm();}
    
    void				vdd_core_enable( bool i_b_on ){m_p_hw_mon_c->vdd_core_enable( i_b_on );}
    void				boot_led_on(void){m_p_hw_mon_c->boot_led_on();}
    void				micom_boot_led_on(void){m_p_hw_mon_c->micom_boot_led_on();}
    void				boot_end(void);
    
    void				double_readout_enable(bool i_b_en){m_p_fpga_c->double_readout_enable(i_b_en);}
    bool				is_double_readout_enabled(void){return m_p_fpga_c->is_double_readout_enabled();}
    
    u8					get_aed_count();
    u16					get_selected_aed(void){return m_p_fpga_c->read(eFPGA_REG_AED_SELECT);}
    					  
    void				adc_read_test(u8 i_c_ch){m_p_hw_mon_c->adc_read_test(i_c_ch);}
    
	void				restore_power_config(void);
	
    void				prom_access_enable(bool i_b_en){m_p_fpga_c->prom_access_enable(i_b_en);}
    void				prom_qpi_enable(bool i_b_en){m_p_fpga_c->prom_qpi_enable(i_b_en);}
    
    void				print_line_error(void){m_p_fpga_c->print_line_error();}
    
    APBUTTON_TYPE		get_ap_button_type(void){return m_p_env_c->get_ap_button_type();}
    bool				is_preset_loaded(void){return m_p_env_c->is_preset_loaded();}
    
    void				apply_gain_table(void);
    u16                 get_roic_cmp_gain_by_scintillator_type(SCINTILLATOR_TYPE i_e_scintillator_type){ return m_p_fpga_c->get_roic_cmp_gain_by_scintillator_type(i_e_scintillator_type); };
    u16                 get_input_charge_by_scintillator_type(SCINTILLATOR_TYPE i_e_scintillator_type){ return m_p_fpga_c->get_input_charge_by_scintillator_type(i_e_scintillator_type); };
    
    u32					get_wakeup_remain_time(void);
    
    bool				is_gate_line_defect_enabled(void){return m_p_fpga_c->is_gate_line_defect_enabled();}
	void				gate_line_defect_enable(bool i_b_enable);
	
	void				pm_power_led_ctrl(u8 i_c_enable){m_p_hw_mon_c->power_led_ctrl(i_c_enable);}
	u8					pm_get_power_led_ctrl(void){return m_p_hw_mon_c->get_power_led_ctrl();}
	void				pm_boot_led_ctrl(u8 i_c_enable){m_p_hw_mon_c->boot_led_ctrl(i_c_enable);}
	
	void				SetSamsungOnlyFunction( u32 i_n_addr, u16 i_w_data );
	u16					GetSamsungOnlyFunction( u32 i_n_addr );
	
    bool                check_compability_bootloader_version(char* s_version);
    bool				_check_exceed(s8* i_p_token, s32 i_n_compare_num);
    bool				_check_compatibility_sdk_version(s8* i_p_sdk_str);

    u16                 trigger_get_type(void){return m_p_fpga_c->trigger_get_type();}
    GEN_INTERFACE_TYPE  get_acq_mode(void);

    //smart w
    u8  				get_smart_w_onoff(void);
    u8  				set_smart_w_onoff(u8 i_enable);
    u32                 get_measrued_exposure_time(void);
    u32                 get_frame_exposure_time(u8 i_panel_num);

    //aed off debounce time 
    u16  				get_aed_off_debounce_time(void);
    u8  				set_aed_off_debounce_time(u16 i_w_debounce_time);

    CMicom*				get_micom_instance(void){return m_p_micom_c;}
    bool		    	aed_read_trig_volt( AED_ID i_aed_id_e, u32* o_p_volt ){ return m_p_micom_c->aed_read_trig_volt(i_aed_id_e, o_p_volt);}
    u32                 micom_eeprom_read( u16 i_w_addr, u8* o_p_data, u32 i_n_len, eeprom_blk_num_t i_n_eeprom_blk_num ){return m_p_micom_c->eeprom_read(i_w_addr, o_p_data, i_n_len, i_n_eeprom_blk_num);}
    u32                 micom_eeprom_write( u16 i_w_addr, u8* i_p_data, u32 i_n_len, eeprom_blk_num_t i_n_eeprom_blk_num ){return m_p_micom_c->eeprom_write(i_w_addr, i_p_data, i_n_len, i_n_eeprom_blk_num);}

    void SetReadoutSync(void){m_p_fpga_c->SetReadoutSync();}
    bool IsReadoutSyncDefined(void);
    bool GetCurrentOffsetCorrectionMode(void);
};


#endif /* end of _VW_SYSTEM_H_*/

