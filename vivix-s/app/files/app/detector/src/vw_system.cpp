/*******************************************************************************
 모  듈 : vw_system
 작성자 : 한명희
 버  전 : 0.0.1
 설  명 : 
 참  고 : 

 버  전:
         0.0.1  최초 작성
*******************************************************************************/



/*******************************************************************************
*	Include Files
*******************************************************************************/
#include <errno.h>          // errno
#include <string.h>			// memset() memset/memcpy
#include <stdio.h>			// fprintf(), fopen(), sprintf()
#include <stdlib.h>			// malloc(), free(),exit(), EXIT_SUCCESS rand/rand 
#include <iostream>
#include <arpa/inet.h>		// socklen_t, inet_ntoa() ,ntohs(),htons()
#include <unistd.h>
#include <sys/stat.h>  	 	// lstat
#include <math.h>
#include <linux/unistd.h>
#include <sys/syscall.h>
#include <limits>

#include "vw_system.h"
#include "vw_file.h"
#include "misc.h"
#include "web_def.h"
#include "vw_xml.h"
#include "vw_wrapper.h"

using namespace std;

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/
#define OSF_FIRST_TRIGGER_TIME_SEC      (3)
#define CAL_FILE_DEFECT_STRUCT	SYS_DATA_PATH "defect_structure.bin2"

//#define FLASH_WRITE_TIME_EVAL

/*******************************************************************************
*	Type Definitions
*******************************************************************************/

/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/
#define SET_ERR(err)    {if(err != eDEV_ERR_NO) m_dev_err_e = err;}
/*******************************************************************************
*	Variable Definitions
*******************************************************************************/
extern CVWSystem * 	g_p_system_c;

/*******************************************************************************
*	Function Prototypes
*******************************************************************************/

/******************************************************************************/
/**
* \brief        constructor
* \param        port        port number
* \param        log         log function
* \return       none
* \note
*******************************************************************************/
CVWSystem::CVWSystem(int (*log)(int,int,const char *,...))
{
    print_log = log;
    strcpy( m_p_log_id, "System        " );
    
    m_dev_err_e = eDEV_ERR_NO;
    
    m_p_charge_time_mutex_c = new CMutex;
    m_p_iw_command_mutex_c = new CMutex;
    m_b_reboot          = false;

    m_p_grab_mutex_c = new CMutex;
    
    m_b_ready   = false;
    
    m_p_env_c       = new CEnv( print_log );
    m_p_env_c->micom_eeprom_read = vw_micom_eeprom_read;
    m_p_env_c->micom_eeprom_write = vw_micom_eeprom_write;

    m_p_fpga_c      = new CFpga( print_log );
    m_p_micom_c		= new CMicom( print_log );
    m_p_hw_mon_c    = new CHWMonitor( print_log );
    m_p_hw_mon_c->m_p_micom_c = get_micom_instance();

    m_p_aed_c      				= new CAed( print_log );
   	m_p_aed_c->m_p_env_c		= m_p_env_c;
    m_p_aed_c->read_trig_volt	= vw_aed_trig_volt_read;

	m_p_preview_image_process_c = NULL;
    
    m_b_readout_started = false;
    m_b_image_grab      = false;
    
    m_b_update_is_running = false;
    m_b_offset_cal_started = false;
    m_b_sw_exp_req = false;
    m_b_update_result = false;
    m_b_pm_is_running = false;
    m_b_mon_print = false;
    m_b_auto_offset = false;
    m_b_osf_is_running = false;
    m_b_osf_saved_gain = false;
    m_b_osf_saved_defect = false;
    m_b_osf_vcom_started = false;
    m_b_test_image = false;
    m_b_web_ready = false;
    
    // power management
    m_pm_mode_e             = ePM_MODE_NORMAL;
    m_b_pm_keep_normal      = false;
    m_p_sleep_wait_c        = NULL;
    m_b_need_wake_up        = false;
    m_b_pm_wake_up          = false;
    m_b_go_sleep			= false;
    
    m_p_pm_mutex_c          = new CMutex;
    
    m_pm_config_t.b_sleep_enable    = false;
    m_pm_config_t.b_shutdown_enable = false;
    m_pm_config_t.w_sleep_time      = 30;
    m_pm_config_t.w_shutdown_time   = 60;
    
    m_b_pm_test_sleep_start = false;
    
    m_p_copy_image_t = NULL;
    
    m_n_file_size = 0;
    m_p_file_data = NULL;
    
    m_b_immediate_transfer = false;
    
    m_b_sw_connected = false;
    m_p_auto_offset_time_c = NULL;
    memset( &m_auto_offset_t, 0, sizeof(auto_offset_t) );
    
    // osf
    m_p_osf_data        = NULL;
    
    m_b_preview_enable  = false;
    m_b_tact_enable     = false;
    m_w_tact_id         = 0;
    
    m_b_trans_done_enable   = false;
    m_b_trans_done          = false;
    
    m_b_scu_connected       = false;

    memset(&m_patient_info_t, 0, sizeof(patient_info_t));    
      
    m_n_img_direction = 0;
    m_b_img_direction_user_input = false;

    m_n_image_size_byte = 0;
    m_w_tether_speed = 0;
    m_w_saved_trigger_type = 0;
    m_update_thread_t = (pthread_t)0;
    m_w_update_progress = 0;
    m_pm_thread_t = (pthread_t)0;
    m_w_mon_time_sec = 0;
    m_n_mon_image_count = 0;
    m_n_mon_offset_count = 0;

    m_w_grf_panel_bias = 0;
    m_w_grf_extra_flush = 0;
    m_osf_thread_t = (pthread_t)0;
    m_w_osf_trigger_total = 0;
    m_w_osf_trigger_count = 0;
    m_w_osf_trigger_time = 0;
    m_w_osf_saved_vcom = 0;
    m_w_osf_saved_trigger_type = 0;
    m_w_osf_saved_extra_flush = 0;
    m_w_osf_saved_digital_offset = 0;
    m_w_osf_saved_vcom_min = 0;
    m_n_osf_line_count = 0;
    m_n_tact_vcom_swing_time = 0;
    m_w_capture_init_count = 0;
    m_p_capture_init_c = NULL;
    m_p_cpature_ready_c = NULL;
    m_n_image_send_remain_time = 0;
    m_w_saved_image_count = 0;
    
    m_b_offset_enable = false;
    m_b_gain_enable = false;
    m_w_gain_average = 1024;
    
    m_n_offset_crc = 0;
    m_n_offset_crc_len = 0;
    
    m_n_gain_crc = 0;
    m_n_gain_crc_len = 0;
    
    m_b_offset_load = false;
    m_b_gain_load = false;
    m_b_cal_data_load_end = false;
    
    m_b_button_thread_is_running = false;
    m_button_thread_t = (pthread_t)0;
    m_b_is_ap_button_actived = false;
    
    m_p_charge_time_c = NULL;
    
    m_n_last_saved_image_id = 0;
    m_b_ap_switching_mode = false;
    m_w_normal_gate_state = 0x0fff;
    
    m_wifi_step_e = eWIFI_SIGNAL_STEP_5;
    
    m_p_wifi_low_noti_wait_time_c = NULL;

    print_log(DEBUG, 1, "[%s] CVWSystem\n", m_p_log_id);
    
}

/******************************************************************************/
/**
* \brief        constructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CVWSystem::CVWSystem(void)
{
}

/******************************************************************************/
/**
* \brief        destructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CVWSystem::~CVWSystem()
{
    if( m_b_update_is_running )
    {
        update_proc_stop();
    }
    
    monitor_proc_stop();
    
    if( m_b_pm_is_running )
    {
        m_b_pm_is_running = false;
    	
    	if( pthread_join( m_pm_thread_t, NULL ) != 0 )
    	{
    		print_log( ERROR, 1,"[%s] pthread_join: pm_state_proc:%s\n", \
    		                    m_p_log_id, strerror( errno ));
    	}
    }
    
    safe_delete( m_p_env_c );
    safe_delete( m_p_hw_mon_c );
    safe_delete( m_p_fpga_c );
    safe_delete( m_p_micom_c );
    safe_delete( m_p_preview_image_process_c);
    
    safe_delete( m_p_pm_mutex_c );
    safe_delete( m_p_charge_time_mutex_c );
    safe_delete( m_p_iw_command_mutex_c );
	safe_delete( m_p_grab_mutex_c );
	safe_delete( m_p_wifi_low_noti_wait_time_c );

    print_log(DEBUG, 1, "[%s] ~CVWSystem\n", m_p_log_id);
}


/******************************************************************************/
/**
* \brief        
* \param        
* \return       
* \note
*******************************************************************************/
void CVWSystem::initialize(void)
{
    print_log(DEBUG, 1, "[%s] initialize\n", m_p_log_id);
    
    DEV_ERROR err_e;
	u16 w_ver;
    s8 p_ver[128];
    
    if(is_vw_revision_board())
    {
        m_p_micom_c->initialize();

        memset( p_ver, 0, sizeof(p_ver) );

        w_ver = m_p_micom_c->get_version();
        sprintf( p_ver, "%d.%d.%d", (w_ver >> 8)& 0xF ,(w_ver >> 4)& 0xF , w_ver & 0xF );
        m_p_env_c->config_set( eCFG_VERSION_MICOM, p_ver );
    }

    err_e = m_p_env_c->initialize(m_p_hw_mon_c->hw_get_ctrl_version());
    SET_ERR(err_e)
    
    m_p_env_c->preset_load();
    
    preview_set_enable( m_p_env_c->preview_get_enable() );
    tact_set_enable( m_p_env_c->tact_get_enable() );
    
    image_direction_set();
    
    MODEL model_e = get_model();

    // init hw monitor
    err_e = m_p_hw_mon_c->initialize(model_e, get_battery_count(), 
                                    m_p_env_c->get_battery_charging_mode_in_eeprom(), m_p_env_c->get_battery_recharging_gauge_in_eeprom());
    SET_ERR(err_e)

    //load impact(acceleration) & gyro(angular) sensor cal data & load status
    hw_set_config();
    impact_cal_extra_detailed_offset_t impact_cal_extra_detailed_offset = m_p_env_c->get_impact_extra_detailed_offset();
	m_p_hw_mon_c->set_impact_extra_detailed_offset(&impact_cal_extra_detailed_offset);
	m_p_hw_mon_c->load_6plane_impact_sensor_cal_done(m_p_env_c->get_impact_6plane_cal_done());
    
    if(m_p_hw_mon_c->is_gyro_exist())
        m_p_hw_mon_c->set_gyro_offset(m_p_env_c->get_gyro_offset());
    else
        m_p_hw_mon_c->impact_stable_pos_proc_start();

    m_w_normal_gate_state = (model_e == eMODEL_2530VW)?0x03ff:0x0fff;

    m_p_oled_c = new CVWOLEDimage( print_log );
    m_p_oled_c->set_spi_fd(m_p_hw_mon_c->get_spi_fd());
    m_p_oled_c->set_gpio_fd(m_p_hw_mon_c->get_gpio_fd());
    m_p_oled_c->start();
    
    set_oled_display_event(eBOOTING);
    SetSamsungOnlyFunction(eFW_ADDR_WIFI_LEVEL_CHANGE, m_p_env_c->get_wifi_signal_step_num());
        
    err_e = m_p_fpga_c->hw_initialize(model_e, get_panel_type(), get_scintillator_type(), get_submodel_type(), m_p_hw_mon_c->hw_get_ctrl_version());
    SET_ERR(err_e)
    if(err_e == eDEV_ERR_FPGA_DONE_PIN)
    {
    	set_oled_error(eFPGA_CONFIG_ERROR, true);
    }

    //ROIC 동적 할당에의한 수정 및 분리 
    m_p_env_c->discover_roic_model_type();      //FPGA driver가 선행적으로 upload 되어야함

    err_e = m_p_fpga_c->register_roic_type(get_roic_model_type());  // roic model discover가 선행적으로 수행되어야함
    SET_ERR(err_e)

    err_e = m_p_fpga_c->reg_initialize();  //roic driver 가 선행적으로 open 되어야함
    SET_ERR(err_e)

    err_e = m_p_fpga_c->roic_initialize();  //register programming이 선행적으로 수행되어야함 
    SET_ERR(err_e)

    m_p_fpga_c->set_capture_count(m_p_env_c->oper_info_get_capture_count());
    
    m_p_aed_c->initialize(m_p_hw_mon_c->hw_get_ctrl_version());
    
    dev_load_trigger_type_config();
    
    memset( p_ver, 0, sizeof(p_ver) );
    m_p_fpga_c->get_version( p_ver );
    m_p_env_c->config_set( eCFG_VERSION_FPGA, p_ver );
    memset( &m_image_size_t, 0, sizeof(image_size_t) );
    m_p_hw_mon_c->get_image_size_info(&m_image_size_t);
    
    m_n_image_size_byte     = m_image_size_t.width * m_image_size_t.height * m_image_size_t.pixel_depth;
    
    m_p_fpga_c->write(eFPGA_REG_IMAGE_WIDTH, m_image_size_t.width);
    m_p_fpga_c->write(eFPGA_REG_IMAGE_HEIGHT, m_image_size_t.height);
    
    m_p_preview_image_process_c = new CImageProcess( print_log, m_image_size_t.width, m_image_size_t.height );
    
    set_immediate_transfer(true);
    
    print_log(DEBUG, 1, "[%s] image size: %d x %d (%d bytes)\n", \
                        m_p_log_id, m_image_size_t.width, m_image_size_t.height, m_n_image_size_byte);
    
    //err_e = offset_load();
    SET_ERR(err_e)
    //m_p_fpga_c->set_tg_time(m_offset_info_t.n_date);
    
    monitor_proc_start();
	button_proc_start();
	
    // start power management thread
    m_b_pm_is_running = true;
    // launch pm_state thread
	if( pthread_create(&m_pm_thread_t, NULL, pm_state_routine, ( void * ) this)!= 0 )
    {
    	print_log( ERROR, 1, "[%s] pm_state_routine pthread_create:%s\n", \
    	                    m_p_log_id, strerror( errno ));

    	m_b_pm_is_running = false;
    }
    
    image_check_remain_memory();

    //DEBUG
    //m_p_fpga_c->print_gain_type_table();

}

/******************************************************************************/
/**
* \brief        
* \param        
* \return       
* \note
*******************************************************************************/
bool CVWSystem::is_ready(void)
{
    return m_b_ready;
}

/******************************************************************************/
/**
* \brief        
* \param        
* \return       
* \note
*******************************************************************************/
void CVWSystem::config_reset(void)
{
    m_p_env_c->config_default();
    config_save();
    config_reload(true);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::config_reload(bool i_b_forced)
{
    m_p_env_c->config_load(CONFIG_XML_FILE, true);
        
    pm_init();
    hw_set_config();

	image_direction_set();
    preview_set_enable( m_p_env_c->preview_get_enable() );
    tact_set_enable( m_p_env_c->tact_get_enable() );
    
    dev_load_trigger_type_config();
    
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::config_net_is_changed(void)
{
    sys_command("cp %s%s %s", SYS_CONFIG_PATH, CONFIG_XML_FILE, SYS_RUN_PATH );
	
	return m_p_env_c->config_net_is_changed(CONFIG_XML_FILE);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::config_change_preset( const s8* i_p_ssid )
{
	return m_p_env_c->preset_change(i_p_ssid);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::hw_set_config(void)
{
    hw_config_t config_t;
    s8*         p_config = NULL;
    s32         n_threshold;
    
    memset( &config_t, 0, sizeof(hw_config_t) );
    p_config = config_get(eCFG_PM_POWER_SOURCE);
    
    upper_str(p_config);
    
    if( strcmp( p_config, "DETECTOR" ) == 0 )
    {
        config_t.power_source_e = ePWR_SRC_BATTERY;
    }
    else
    {
        config_t.power_source_e = ePWR_SRC_TETHER;
    }
    
    p_config = config_get(eCFG_IMACT_PEAK);
    n_threshold = atoi(config_get(eCFG_IMACT_PEAK));
    
    if( n_threshold > 0
        && n_threshold <= 200 )
    {
        config_t.c_impact_threshold = (u8)n_threshold;
    }
    else
    {
        config_t.c_impact_threshold = 100;
    }
    
    memcpy(&(config_t.impact_offset_t), m_p_env_c->get_impact_offset(), sizeof(impact_cal_offset_t));
    
    m_p_hw_mon_c->set_config(config_t);
    
}
/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::impact_calibartion_offset_save(void)
{
    impact_cal_offset_t impact_cal_offset;
    m_p_hw_mon_c->impact_get_offset(&impact_cal_offset);
	m_p_env_c->save_impact_offset(&impact_cal_offset);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::angular_offset_cal_save(void)
{
    //impact sensor offset read and save to non volitile
    impact_cal_offset_t impact_cal_offset;
    m_p_hw_mon_c->impact_get_offset(&impact_cal_offset);
	m_p_env_c->save_impact_offset(&impact_cal_offset);

    //impact sensor extra detailed offset read and save to non volitile
    impact_cal_extra_detailed_offset_t impact_cal_extra_detailed_offset;
    m_p_hw_mon_c->get_impact_extra_detailed_offset(&impact_cal_extra_detailed_offset);
    m_p_env_c->save_impact_extra_detailed_offset(&impact_cal_extra_detailed_offset);
	m_p_env_c->save_impact_6plane_cal_done(m_p_hw_mon_c->get_6plane_impact_sensor_cal_done());

    //gyro sensor offset read and save to non volitile
    if(m_p_hw_mon_c->is_gyro_exist())
    {
        gyro_cal_offset_t gyro_offset_t;
        m_p_hw_mon_c->get_gyro_offset(&gyro_offset_t);
        m_p_env_c->save_gyro_offset(&gyro_offset_t);  
    }

}

/******************************************************************************/
/**
 * @brief   디텍터 상태가 변하면 S/W에 알린다.
 * @param   noti: 함수 포인터(notify_sw)
 * @return  
 * @note    main.cpp에서 등록한다.
*******************************************************************************/
void CVWSystem::set_notify_handler(bool (*noti)(vw_msg_t * msg))
{
    notify_handler = noti;
    m_p_hw_mon_c->set_notify_sw_handler( noti );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CVWSystem::exposure_request( TRIGGER_TYPE i_type_e )
{
    bool b_ret = false;
    
    u16 w_trigger_type;
    
    if( pm_reset_sleep_timer() == false )
    {
        print_log(DEBUG, 1, "[%s] PM mode is not PM_MODE_NORMAL\n", m_p_log_id);
        return false;
    }
    
    if( m_p_fpga_c->is_ready() == false )
    {
        print_log(DEBUG, 1, "[%s] FPGA is not ready.\n", m_p_log_id);
        
        return false;
    }
    
    if( m_b_offset_cal_started == true )
    {
        print_log(INFO, 1, "[%s] offset calibration is started.\n", \
                            m_p_log_id);
        return false;
    }
    
    w_trigger_type = m_p_fpga_c->trigger_get_type();
    
    if( i_type_e == eTRIGGER_TYPE_SW )
    {
        print_log(DEBUG, 1, "[%s] exp_req by sw\n", m_p_log_id);
        
        if( w_trigger_type == eTRIG_SELECT_NON_LINE
            && m_p_fpga_c->is_flush_enough() == false )
        {
            return false;
        }
        
        if( m_b_sw_exp_req == true )
        {
        	print_log(DEBUG, 1, "[%s] next exp_req is not enabled\n", m_p_log_id);
        	return false;
        }
        
        m_p_fpga_c->trigger_set_type(eTRIG_SELECT_ACTIVE_LINE);
        
        grf_reset();
        
        m_w_saved_trigger_type = w_trigger_type;
        
        b_ret = m_p_fpga_c->exposure_request(eTRIGGER_TYPE_SW);
        
        if( b_ret == true )
        {
            m_b_sw_exp_req = true;
        }
        else
        {
            m_b_sw_exp_req = false;
            m_p_fpga_c->trigger_set_type(m_w_saved_trigger_type);
            grf_restore();
        }
        
        return b_ret;
    }
    
    if( w_trigger_type == eTRIG_SELECT_NON_LINE
    	|| m_p_fpga_c->is_disable_aed_trig_block() == true )
    {
        print_log(INFO, 1, "[%s] invalid request(%d) (trigger type: %d, aed block: %d)\n", \
                            m_p_log_id, i_type_e, w_trigger_type, m_p_fpga_c->is_disable_aed_trig_block());
        return false;
    }
    
    print_log(DEBUG, 1, "[%s] exp_req by SCU\n", m_p_log_id);
    
    m_b_sw_exp_req = false;
    
    return m_p_fpga_c->exposure_request(i_type_e);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    촬영이 끝나면 호출된다.
*******************************************************************************/
void CVWSystem::exposure_request_enable(void)
{
    if( m_b_sw_exp_req == true )
    {
        grf_restore();
        
        m_p_fpga_c->set_capture_count(m_p_env_c->oper_info_get_capture_count());
        m_p_fpga_c->trigger_set_type(m_w_saved_trigger_type);
        
        m_b_sw_exp_req = false;
    }
    else
    {
        m_n_mon_image_count++;
        m_p_env_c->oper_info_capture();
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    setup에서 설정하는 exposure time + EXP_OK_DELAY
*******************************************************************************/
u32 CVWSystem::get_exp_wait_time(void)
{
    u32 n_wait_time = 0;
    
    n_wait_time = m_p_fpga_c->get_exp_time() \
                    + m_p_fpga_c->read(eFPGA_REG_EXP_OK_DELAY_TIME);
    
    return n_wait_time;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CVWSystem::get_pre_exp(void)
{
    u16 w_trigger_type = m_p_fpga_c->trigger_get_type();
    
    if( w_trigger_type == eTRIG_SELECT_NON_LINE )
    {
        return 0;
    }
    
    return m_p_fpga_c->get_pre_exp_flush_num();
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
u16 CVWSystem::dev_reg_save( DEV_ID i_id_e, u16 i_w_start, u16 i_w_end )
{
    if( i_id_e == eDEV_ID_FPGA )
    {
        return m_p_fpga_c->reg_save(i_w_start, i_w_end);
    }
    else if( i_id_e == eDEV_ID_ROIC )
    {
        return 0;
    }
    
    return 0;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CVWSystem::dev_reg_save_file( DEV_ID i_id_e )
{
    if( i_id_e == eDEV_ID_FPGA )
    {
        return m_p_fpga_c->reg_save_file();
    }
    else if( i_id_e == eDEV_ID_ROIC )
    {
        return true;
    }
    
    return false;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
u16 CVWSystem::dev_reg_read( DEV_ID i_id_e, u32 i_n_addr )
{
    if( i_id_e == eDEV_ID_FPGA )
    {
        return m_p_fpga_c->read((u16)i_n_addr);
    }
    else if( i_id_e == eDEV_ID_ROIC )
    {
        return m_p_fpga_c->roic_reg_read(i_n_addr);
    }
    else if( i_id_e == eDEV_ID_AED_0 || i_id_e == eDEV_ID_AED_1 )
    {
    	u8 c_aed_id = i_id_e - eDEV_ID_AED_0;
        return m_p_aed_c->read( c_aed_id, i_n_addr );
	}
    else if( i_id_e == eDEV_ID_FIRMWARE )
    {
    	return GetSamsungOnlyFunction(i_n_addr);
    }
    
    return 0;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
u16 CVWSystem::dev_reg_read_saved( DEV_ID i_id_e, u16 i_w_addr )
{
    if( i_id_e == eDEV_ID_FPGA )
    {
        return m_p_fpga_c->read(i_w_addr, true);
    }
    
    return 0;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CVWSystem::dev_reg_write( DEV_ID i_id_e, u32 i_n_addr, u16 i_w_data )
{
    if( i_id_e == eDEV_ID_FPGA )
    {
        if( i_n_addr == eFPGA_REG_FW_TEST )
        {
            //print_log( ERROR, 1, "[%s] FW TEST\n", m_p_log_id);
            //device_pgm();   //TEST
        }    

    	if( i_n_addr == eFPGA_REG_TRIGGER_TYPE)
        {
        	aed_power_control(get_acq_mode(), (TRIGGER_SELECT)i_w_data, get_smart_w_onoff());
	    }
	    
        m_p_fpga_c->write( (u16)i_n_addr, i_w_data );
        
        if( i_n_addr == eFPGA_REG_DRO_IMG_CAPTURE_OPTION )
        {
        	m_p_fpga_c->offset_set_enable(m_b_offset_enable);
            aed_power_control(get_acq_mode(), (TRIGGER_SELECT)trigger_get_type(), get_smart_w_onoff());
        }
       	else if( i_n_addr == eFPGA_REG_DDR_IDLE_STATE )
        {
        	cal_data_load();
        }
        
        return true;
    }
    else if( i_id_e == eDEV_ID_ROIC )
    {
        m_p_fpga_c->roic_reg_write( i_n_addr, i_w_data );
        return true;
	}
    else if( i_id_e == eDEV_ID_AED_0 || i_id_e == eDEV_ID_AED_1 )
    {
    	u8 c_aed_id = i_id_e - eDEV_ID_AED_0;
        m_p_aed_c->write( c_aed_id, i_n_addr, i_w_data );
        return true;
	}
    else if( i_id_e == eDEV_ID_FIRMWARE )
    {
    	SetSamsungOnlyFunction(i_n_addr, i_w_data);
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
s32 CVWSystem::dev_get_fd( DEV_ID i_id_e )
{
    if( i_id_e == eDEV_ID_FPGA )
    {
        return m_p_fpga_c->get_fd();
    }
    
    print_log(ERROR, 1, "[%s] device(ID: %d) is not supported.\n", m_p_log_id, i_id_e );
    return -1;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::dev_set_test_pattern( u16 i_w_type )
{
    m_p_fpga_c->set_test_pattern(i_w_type);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::dev_load_trigger_type_config(void)
{
	u16 w_value;
	
	w_value = atoi(config_get(eCFG_EXP_TRIG_TYPE));
    TRIGGER_SELECT e_trigger_type = (w_value == 0 ? eTRIG_SELECT_ACTIVE_LINE : eTRIG_SELECT_NON_LINE);

    m_p_fpga_c->trigger_set_type(eTRIG_SELECT_ACTIVE_LINE);
	aed_power_control(get_acq_mode(), (TRIGGER_SELECT)e_trigger_type, get_smart_w_onoff());
    m_p_fpga_c->trigger_set_type(e_trigger_type);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::aed_wait_end(void)
{
	CTime* p_wait_time = new CTime(3000);
	
	while( p_wait_time && p_wait_time->is_expired() == false )
	{
		if(m_p_aed_c->is_stable())
		{
			break;
		}
		sleep_ex(100000);		//100msec
	}

    if(m_p_aed_c->is_stable())
        print_log(DEBUG, 1, "[%s] AED sensor is stable\n", m_p_log_id);
    else
        print_log(DEBUG, 1, "[%s] AED sensor is not stable (timeout)\n", m_p_log_id);
	
	safe_delete(p_wait_time);
}
	
/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::dev_reg_print( DEV_ID i_id_e )
{
    if( i_id_e == eDEV_ID_FPGA )
    {
        m_p_fpga_c->reg_print();
    }
    else if( i_id_e == eDEV_ID_ADC )
    {
        m_p_hw_mon_c->adc_print();
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::dev_reg_init( DEV_ID i_id_e )
{
    if( i_id_e == eDEV_ID_FPGA )
    {
        m_p_fpga_c->reinitialize();
        m_p_fpga_c->write( eFPGA_REG_XRAY_READY, 1 );
    }
}

/******************************************************************************/
/**
 * @brief
 * @param
 * @return
 * @note
 *******************************************************************************/
bool CVWSystem::fpga_reinit(void)
{
    CTime *p_wait_c;
    u16 w_read;
    bool b_ready = false;

    // wait until FPGA configuration is done
    p_wait_c = new CTime(2000);
    while (p_wait_c->is_expired() == false)
    {
        w_read = m_p_fpga_c->read(eFPGA_REG_XRAY_READY);
        if (w_read == 0)
        {
            print_log(ERROR, 1, "[%s] FPGA is ready.\n", m_p_log_id);
            b_ready = true;
            break;
        }
        else
        {
            sleep_ex(10000); // 10 ms
        }
    }
    safe_delete(p_wait_c);

    if (b_ready == false)
    {
        return false;
    }

    if (m_p_fpga_c->reinitialize() == false)
    {
        print_log(ERROR, 1, "[%s] fpga re-init(%d) failed\n", m_p_log_id, m_w_capture_init_count);
        return false;
    }

    dev_load_trigger_type_config();
    m_p_fpga_c->set_capture_count(m_p_env_c->oper_info_get_capture_count());

    if (is_cal_data_valid() == false)
    {
        print_log(ERROR, 1, "[%s] fpga cal. data is not valid, reload cal data.\n", m_p_log_id);
        cal_data_load();
        wait_until_cal_data_load_end();
    }
    else
    {
        enable_image_correction();
    }

    // FPGA enable
    m_p_fpga_c->write(eFPGA_REG_XRAY_READY, 1);

    print_log(ERROR, 1, "[%s] fpga re-init(%d) success\n", m_p_log_id, m_w_capture_init_count);

    return true;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CVWSystem::message_send( MSG i_msg_e, u8* i_p_data, u16 i_w_data_len )
{
    if( i_w_data_len > VW_PARAM_MAX_LEN )
    {
        print_log(ERROR, 1, "[%s] message data lenth is too long(%d/%d).\n", \
                        m_p_log_id, i_w_data_len, VW_PARAM_MAX_LEN );
        return;
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
        print_log(ERROR, 1, "[%s] message_send(%d) failed\n", \
                            m_p_log_id, i_msg_e );
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::message_ready_for_exposure(void)
{
    u32 n_state = eIMG_STATE_READY_FOR_EXPOSURE;
    
    message_send( eMSG_IMAGE_STATE, (u8*)&n_state, sizeof(n_state) );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::offset_cal_trigger(void)
{
    if( pm_reset_sleep_timer() == false )
    {
        print_log(ERROR, 1, "[%s] offset_cal_trigger is failed(power mode is not normal).\n", \
                            m_p_log_id);
        return false;
    }
    
    print_log(DEBUG, 1, "[%s] exp_req by offset\n", m_p_log_id);
    
    return m_p_fpga_c->exposure_request();
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CVWSystem::offset_cal_stop(void)
{
	m_b_offset_cal_started = false;
	
    m_p_fpga_c->offset_start( false );
}

/******************************************************************************/
/**
* \brief        
* \param        
* \return       none
* \note
*******************************************************************************/
bool CVWSystem::offset_cal_start( u32 i_n_target_count, u32 i_n_skip_count )
{
    if( m_p_fpga_c->trigger_get_type() == eTRIG_SELECT_NON_LINE
        && m_p_fpga_c->is_flush_enough() == false )
    {
        return false;
    }
    
    m_b_offset_cal_started = true;
    
	m_p_fpga_c->offset_start( true, i_n_target_count, i_n_skip_count );
    
    set_readout_start(true);
    
    return true;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CVWSystem::offset_cal_message( OFFSET_CAL_STATE i_state_e, u8* i_p_data, u16 i_w_data_len )
{
    u32 n_state;
    vw_msg_t msg;
    bool b_result;
    
    if( i_w_data_len + sizeof( n_state ) > VW_PARAM_MAX_LEN )
    {
        print_log(ERROR, 1, "[%s] message data lenth is too long(%d/%d).\n", \
                            m_p_log_id, i_w_data_len, VW_PARAM_MAX_LEN );
        return;
    }
    
    n_state = i_state_e;
    
    memset( &msg, 0, sizeof(vw_msg_t) );
    msg.type = (u16)eMSG_OFFSET_CAL_STATE;
    msg.size = sizeof(n_state) + i_w_data_len;
    
    memcpy( &msg.contents[0], &n_state, sizeof(n_state) );
    if( i_w_data_len > 0 )
    {
        memcpy( &msg.contents[sizeof(n_state)], i_p_data, i_w_data_len );
    }
    
    b_result = notify_handler( &msg );
    
    if( b_result == false )
    {
        print_log(ERROR, 1, "[%s] offset_cal_message(%d) failed\n", \
                            m_p_log_id, i_state_e );
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::offset_cal_request( u32 i_n_target_count, u32 i_n_skip_count )
{
    if( m_b_offset_cal_started == true )
    {
        print_log(ERROR, 1, "[%s] offset calibration is already started.\n", \
                            m_p_log_id);
        return false;
    }
    
    if( pm_reset_sleep_timer() == false )
    {
        print_log(ERROR, 1, "[%s] starting offset calibration is failed(power mode is not normal).\n", \
                            m_p_log_id);
        return false;
    }
    
    u32 p_count[2];
    
    p_count[0] = i_n_target_count;
    p_count[1] = i_n_skip_count;
    
    offset_cal_message(eOFFSET_CAL_REQUEST, (u8*)p_count, sizeof(p_count) );
    
    return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::get_image_size_info( image_size_t* o_p_image_size_t )
{
    memcpy( o_p_image_size_t, &m_image_size_t, sizeof(image_size_t) );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CVWSystem::get_image_id(void)
{
    if( m_b_sw_exp_req == true )
    {
        return 0;
    }
    
    return m_p_fpga_c->get_capture_count();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::get_state(sys_state_t* o_p_state_t, bool i_b_direct)
{
    if( i_b_direct == false )
    {
        memcpy( o_p_state_t, &m_sys_state_t, sizeof(sys_state_t) );
        return;
    }
    
    o_p_state_t->w_network_mode     = m_current_net_mode_e;
    o_p_state_t->w_tether_speed     = m_w_tether_speed;
    
    if( o_p_state_t->w_network_mode == eNET_MODE_STATION )
    {
        get_state_wlan( o_p_state_t );
    }
    else if( o_p_state_t->w_network_mode == eNET_MODE_AP )
    {
        get_state_ap_stations( o_p_state_t );
    }
    else
    {
        reset_state_wireless(o_p_state_t);
    }
    
    get_state_battery( o_p_state_t );
    get_state_power( o_p_state_t );
    
    o_p_state_t->b_fpga_ready = m_p_fpga_c->is_ready();
    o_p_state_t->w_gate_state = m_p_fpga_c->read(eFPGA_REQ_GATE_STATE);
    
    o_p_state_t->w_temperature      = m_p_hw_mon_c->get_temperature();
    o_p_state_t->w_humidity         = m_p_hw_mon_c->get_humidity();
    
    o_p_state_t->n_accum_of_captured_count = m_p_env_c->oper_info_get_capture_count();
    o_p_state_t->n_captured_count = m_n_mon_image_count;
    if(is_vw_revision_board()) 
	    o_p_state_t->w_impact_count_V2 = m_p_hw_mon_c->get_impact_count();
    else
        o_p_state_t->w_impact_count = m_p_hw_mon_c->get_impact_count();

}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::reset_state_wireless(sys_state_t* o_p_state_t)
{
    o_p_state_t->w_link_quality_level   = 0;
    o_p_state_t->w_link_quality         = 0;
    o_p_state_t->n_signal_level         = 0;
    o_p_state_t->n_bit_rate             = 0;
    o_p_state_t->n_frequency            = 0;
    
    sprintf( o_p_state_t->p_ap_mac, "None" );
    
    strcpy( o_p_state_t->p_ap_station_mac, "None" );
    memset( o_p_state_t->p_ap_info, 0, sizeof(o_p_state_t->p_ap_info) );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::get_state_battery(sys_state_t* o_p_state_t)
{
    battery_t           bat_t[eBATTERY_MAX];
    bool                b_warning = false;
    u16					w_battery_level;
    u8					c_equip_idx = m_p_hw_mon_c->get_bat_equip_start_idx();

    m_p_hw_mon_c->get_battery(bat_t);
    
    if( m_p_hw_mon_c->get_battery_count() == 2)
    {
	    o_p_state_t->w_battery_gauge        = m_p_hw_mon_c->get_battery_gauge();
	    o_p_state_t->w_battery_raw_gauge    = (bat_t[eBATTERY_0].w_soc_raw + bat_t[eBATTERY_1].w_soc_raw + 1) / 2;
	    
	    w_battery_level						= m_p_hw_mon_c->bat_get_soc_state(o_p_state_t->w_battery_gauge);
	    
	    if( w_battery_level >= eBAT_SOC_STATE_1
	            && w_battery_level <= eBAT_SOC_STATE_5 )
	    {
	    	o_p_state_t->w_battery_level = w_battery_level;
	    }
	    else
	    {
	    	o_p_state_t->w_battery_level = eBAT_SOC_STATE_1;
	    }
	    
	    if( m_p_hw_mon_c->get_battery_count() == 2 )
	    {
	        if( bat_t[eBATTERY_0].b_power_warning == true
    	        && bat_t[eBATTERY_1].b_power_warning == true )
    	    {
    	        b_warning = true;
    	    }
    	}
    	else
    	{
	        if( bat_t[eBATTERY_0].b_power_warning == true
    	        || bat_t[eBATTERY_1].b_power_warning == true )
    	    {
    	        b_warning = true;
    	    }
    	}
    	
    	o_p_state_t->w_battery_vcell        = bat_t[eBATTERY_0].w_vcell;
	    o_p_state_t->b_battery_equipped     = bat_t[eBATTERY_0].b_equip;
	    o_p_state_t->b_battery_charge       = bat_t[eBATTERY_0].b_charge;
	    
	    o_p_state_t->w_battery2_vcell        = bat_t[eBATTERY_1].w_vcell;
	    o_p_state_t->b_battery2_equipped     = bat_t[eBATTERY_1].b_equip;
	    o_p_state_t->b_battery2_charge       = bat_t[eBATTERY_1].b_charge;
	}
	else// if( m_p_hw_mon_c->get_battery_count() == 1 )
	{		
		o_p_state_t->w_battery_gauge        = m_p_hw_mon_c->get_battery_gauge();
	    o_p_state_t->w_battery_raw_gauge    = bat_t[c_equip_idx].w_soc_raw;
	    o_p_state_t->w_battery_level        = bat_t[c_equip_idx].w_level;
	    
	    b_warning = bat_t[c_equip_idx].b_power_warning;
	    
	    o_p_state_t->w_battery_vcell        = bat_t[c_equip_idx].w_vcell;
	    o_p_state_t->b_battery_equipped     = bat_t[c_equip_idx].b_equip;
	    o_p_state_t->b_battery_charge       = bat_t[c_equip_idx].b_charge;

	    w_battery_level						= m_p_hw_mon_c->bat_get_soc_state(o_p_state_t->w_battery_gauge);
	    
	    if( w_battery_level >= eBAT_SOC_STATE_1
	            && w_battery_level <= eBAT_SOC_STATE_5 )
	    {
	    	o_p_state_t->w_battery_level = w_battery_level;
	    }
	    else
	    {
	    	o_p_state_t->w_battery_level = eBAT_SOC_STATE_1;
	    }
	    
	    o_p_state_t->w_battery2_vcell = 0;
	    o_p_state_t->b_battery2_equipped = false;
	    o_p_state_t->b_battery2_charge = false;
	}
    
    if( o_p_state_t->b_battery_warning != b_warning
        && b_warning == true )
    {
        m_p_hw_mon_c->bat_notify_low_volt();
    }
    
    if(is_vw_revision_board())
    {
        if( (bat_t[eBATTERY_0].c_charger_state >> 1) == 1)
        {
            
            o_p_state_t->w_battery_charge_complete_1 = 1;
        }
        else
        {
            o_p_state_t->w_battery_charge_complete_1 = 2;
        }

        if( (bat_t[eBATTERY_1].c_charger_state >> 1) == 1)
        {
            o_p_state_t->w_battery_charge_complete_2 = 1;
        }
        else
        {
            o_p_state_t->w_battery_charge_complete_2 = 2;
        }
    }

    o_p_state_t->b_battery_warning = b_warning;
    
    o_p_state_t->b_scu_connected = m_b_scu_connected;
    
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::get_state_power(sys_state_t* o_p_state_t)
{
    power_volt_t        pwr_volt_t;
    
    memset( &pwr_volt_t, 0, sizeof(power_volt_t) );
    
    m_p_hw_mon_c->get_power_volt( &pwr_volt_t );
    
    memcpy( o_p_state_t->p_hw_power_volt, pwr_volt_t.volt, sizeof(o_p_state_t->p_hw_power_volt) );
    
    o_p_state_t->w_power_source     = (u16)m_p_hw_mon_c->get_power_source();
    o_p_state_t->w_power_mode       = m_pm_mode_e;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::get_state_wlan(sys_state_t* o_p_state_t)
{
    vw_wlan_state_t     wlan_state_t;
    bool                b_result;
    u16 w_link_quality_level = 0;
    
    memset( &wlan_state_t, 0, sizeof(vw_wlan_state_t) );
    
    b_result = get_state_wlan_lock( "wlan0", &wlan_state_t );
    
    w_link_quality_level = wireless_get_level(wlan_state_t.c_quality);
    
    // WIFI siganl level -> Bad 이하
    if( w_link_quality_level <= 2 )
	{
		//print_log(DEBUG, 1, "[%s] WLAN signal is bad : %d (lower than 40)\n", m_p_log_id, w_link_quality_level);
    		
    	if( m_p_wifi_low_noti_wait_time_c == NULL )
    	{
			print_log(DEBUG, 1, "[%s] WLAN signal is bad(%d : lower than 40), timer start.\n", m_p_log_id, w_link_quality_level);
    
			m_p_wifi_low_noti_wait_time_c = new CTime(5000);
		}
		else if( m_p_wifi_low_noti_wait_time_c->is_expired() == true )
		{
			//safe_delete(m_p_wifi_low_noti_wait_time_c);
			o_p_state_t->w_link_quality_level = w_link_quality_level;
			o_p_state_t->w_link_quality       = wlan_state_t.c_quality;
			o_p_state_t->n_signal_level       = wlan_state_t.n_level;
		    o_p_state_t->n_bit_rate           = wlan_state_t.n_bitrate;
		    o_p_state_t->n_frequency          = wlan_state_t.n_frequency;
		}
	}
	else
	{
		if(m_p_wifi_low_noti_wait_time_c != NULL)
		{
			print_log(DEBUG, 1, "[%s] WLAN signal is recovered(%d).\n", m_p_log_id, w_link_quality_level);
			safe_delete(m_p_wifi_low_noti_wait_time_c);
		}
			
		o_p_state_t->w_link_quality_level = w_link_quality_level;
		o_p_state_t->w_link_quality       = wlan_state_t.c_quality;
	    o_p_state_t->n_signal_level       = wlan_state_t.n_level;
	    o_p_state_t->n_bit_rate           = wlan_state_t.n_bitrate;
	    o_p_state_t->n_frequency          = wlan_state_t.n_frequency;
	}
    
    //memcpy( o_p_state_t->p_ap_mac, wlan_state_t.p_ap_mac, sizeof(wlan_state_t.p_ap_mac) );
    if( b_result == true )
    {
        sprintf( o_p_state_t->p_ap_mac, "%02X:%02X:%02X:%02X:%02X:%02X", \
                            wlan_state_t.p_ap_mac[0], wlan_state_t.p_ap_mac[1], \
                            wlan_state_t.p_ap_mac[2], wlan_state_t.p_ap_mac[3], \
                            wlan_state_t.p_ap_mac[4], wlan_state_t.p_ap_mac[5] );
    }
    else
    {
        sprintf( o_p_state_t->p_ap_mac, "None" );
    }
    
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::get_state_ap_stations(sys_state_t* o_p_state_t)
{
    s32 n_result;
    s8  p_buffer[1024];
    
    
    sys_command("iw wlan0 station dump > /tmp/station.dump");
    
    memset( p_buffer, 0, sizeof(p_buffer) );
    n_result = process_get_result("cat /tmp/station.dump | awk '/Station/ {print $2}'", p_buffer);
    if( n_result > 0 )
    {
        strncpy( o_p_state_t->p_ap_station_mac, p_buffer, (sizeof(o_p_state_t->p_ap_station_mac) - 1) );
    }
    else
    {
        strcpy( o_p_state_t->p_ap_station_mac, "None" );
        memset( o_p_state_t->p_ap_info, 0, sizeof(o_p_state_t->p_ap_info) );
        
        //print_log(DEBUG, 1, "[%s] No stations\n", m_p_log_id);
        return;
    }
    
	// inactive_time
	memset( p_buffer, 0, sizeof(p_buffer) );
	n_result = process_get_result("cat /tmp/station.dump | awk '/inactive/ {print $3}'", p_buffer);
	if( n_result > 0 )
	{
	    o_p_state_t->p_ap_info[eAP_INFO_INACTIVE_TIME] = atoi(p_buffer);
	}
    
	// rx bytes
	memset( p_buffer, 0, sizeof(p_buffer) );
	n_result = process_get_result("cat /tmp/station.dump | awk '/rx bytes/ {print $3}'", p_buffer);
	if( n_result > 0 )
	{
	    o_p_state_t->p_ap_info[eAP_INFO_RX_BYTES] = atoi(p_buffer);
	}

	// rx packets
	memset( p_buffer, 0, sizeof(p_buffer) );
	n_result = process_get_result("cat /tmp/station.dump | awk '/rx packets/ {print $3}'", p_buffer);
	if( n_result > 0 )
	{
	    o_p_state_t->p_ap_info[eAP_INFO_RX_PACKET] = atoi(p_buffer);
	}

	 //rx bitrate
	memset( p_buffer, 0, sizeof(p_buffer) );
	n_result = process_get_result("cat /tmp/station.dump | awk '/rx bitrate/ {print $3}'", p_buffer);
	if( n_result > 0 )
	{
	    o_p_state_t->p_ap_info[eAP_INFO_RX_BITRATE] = atoi(p_buffer);
	}

	// rx mcs
	memset( p_buffer, 0, sizeof(p_buffer) );
	n_result = process_get_result("cat /tmp/station.dump | awk '/rx bitrate/ {print $6}'", p_buffer);
	if( n_result > 0 )
	{
	    o_p_state_t->p_ap_info[eAP_INFO_RX_MCS] = atoi(p_buffer);
	}
	
	// tx bytes
	memset( p_buffer, 0, sizeof(p_buffer) );
	n_result = process_get_result("cat /tmp/station.dump | awk '/tx bytes/ {print $3}'", p_buffer);
	if( n_result > 0 )
	{
	    o_p_state_t->p_ap_info[eAP_INFO_TX_BYTES] = atoi(p_buffer);
	}

	// tx packets
	memset( p_buffer, 0, sizeof(p_buffer) );
	n_result = process_get_result("cat /tmp/station.dump | awk '/tx packets/ {print $3}'", p_buffer);
	if( n_result > 0 )
	{
	    o_p_state_t->p_ap_info[eAP_INFO_TX_PACKET] = atoi(p_buffer);
	}

	// tx retries
	memset( p_buffer, 0, sizeof(p_buffer) );
	n_result = process_get_result("cat /tmp/station.dump | awk '/tx retries/ {print $3}'", p_buffer);
	if( n_result > 0 )
	{
	    o_p_state_t->p_ap_info[eAP_INFO_TX_RETRIES] = atoi(p_buffer);
	}

	 //tx bitrate
	memset( p_buffer, 0, sizeof(p_buffer) );
	n_result = process_get_result("cat /tmp/station.dump | awk '/tx bitrate/ {print $3}'", p_buffer);
	if( n_result > 0 )
	{
	    o_p_state_t->p_ap_info[eAP_INFO_TX_BITRATE] = atoi(p_buffer);
	}

	// tx mcs
	memset( p_buffer, 0, sizeof(p_buffer) );
	n_result = process_get_result("cat /tmp/station.dump | awk '/tx bitrate/ {print $6}'", p_buffer);
	if( n_result > 0 )
	{
	    o_p_state_t->p_ap_info[eAP_INFO_TX_MCS] = atoi(p_buffer);
	}

	// signal
	memset( p_buffer, 0, sizeof(p_buffer) );
	n_result = process_get_result("cat /tmp/station.dump | awk '/signal:/ {print $2}'", p_buffer);
	if( n_result > 0 )
	{
	    o_p_state_t->p_ap_info[eAP_INFO_SIGNAL] = atoi(p_buffer);
	}

	// signal avg
	memset( p_buffer, 0, sizeof(p_buffer) );
	n_result = process_get_result("cat /tmp/station.dump | awk '/signal avg:/ {print $3}' ", p_buffer);
	if( n_result > 0 )
	{
	    o_p_state_t->p_ap_info[eAP_INFO_SIGNAL_AVERAGE] = atoi(p_buffer);
	}
    
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CVWSystem::wireless_get_level( u16 i_w_quality )
{
	if( m_wifi_step_e == eWIFI_SIGNAL_STEP_3 )
    {
    	if( i_w_quality > 55 )    // 56 ~ 65
    	{
    		return 4;
    	}
    	else if( i_w_quality > 35 )
    	{
    		return 3;
    	}
    	else if( i_w_quality > 10 )
    	{
    		return 2;
    	}
    	
    	return 0;
    }
   	else
   	{
	    if( i_w_quality > 65 )         // 66 ~ 70
	    {
	        return 5;
	    }
	    else if( i_w_quality > 55 )    // 56 ~ 65
	    {
	        return 4;
	    }
	    else if( i_w_quality > 40 )    // 41 ~ 55
	    {
	        return 3;
	    }
	    else if( i_w_quality > 30 )    // 31 ~ 40
	    {
	        return 2;
	    }
	    else if( i_w_quality > 10 )    // ~ 30
	    {
	        return 1;
	    }
    
    	return 0;
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::print_state(void)
{
    sys_state_t state_t;
    vw_wlan_state_t wlan_state_t;
    bool            b_wlan_state_result;
    u32				n_idx;
    battery_t       bat_t[eBATTERY_MAX];
    u8				c_start_idx = m_p_hw_mon_c->get_bat_equip_start_idx();
    
    memset( &state_t, 0, sizeof(battery_t)*eBATTERY_MAX );    
    
    memset( &state_t, 0, sizeof(sys_state_t) );
    
    get_state( &state_t );
    m_p_hw_mon_c->get_battery(bat_t);

    print_log(DEBUG,1,"[%s] net mode      : %d(0: tether, 1: station , 2: AP)\n", m_p_log_id, state_t.w_network_mode );
    print_log(DEBUG,1,"[%s] power source  : %d(0: battery, 1: tether)\n", m_p_log_id, state_t.w_power_source );
    print_log(DEBUG,1,"[%s] Tether speed  : %d\n", m_p_log_id, state_t.w_tether_speed );
    
    for( n_idx = c_start_idx; n_idx < c_start_idx + m_p_hw_mon_c->get_battery_count(); n_idx++ )
   	{
   		print_log(DEBUG,1,"[%s] Battery equip(%d) : %d\n", m_p_log_id, n_idx, bat_t[n_idx].b_equip );
	    print_log(DEBUG,1,"[%s] Battery charge(%d): %d\n", m_p_log_id, n_idx, bat_t[n_idx].b_charge );
	    print_log(DEBUG,1,"[%s] Battery level(%d) : %d(5 ~ 1)\n", m_p_log_id, n_idx, bat_t[n_idx].w_level );
	    print_log(DEBUG,1,"[%s] Battery SOC(%d)   : %2.1f(raw: %2.1f)\n", m_p_log_id, n_idx, bat_t[n_idx].w_soc/10.0f, bat_t[n_idx].w_soc_raw/10.0f );
	    print_log(DEBUG,1,"[%s] Battery VCell(%d) : %2.2f\n", m_p_log_id, n_idx, bat_t[n_idx].w_vcell/100.0f );   		
   	}
   	print_log(DEBUG,1,"[%s] Battery Warning : %d\n", m_p_log_id, state_t.b_battery_warning );
    
    print_log(DEBUG,1,"[%s] Temperature   : %2.1f(%2.1f, %2.1f)\n", m_p_log_id, state_t.w_temperature/10.0f, \
                            m_p_fpga_c->get_temperature(0)/10.0f, m_p_fpga_c->get_temperature(1)/10.0f );
    print_log(DEBUG,1,"[%s] Humidity      : %2.1f\n", m_p_log_id, state_t.w_humidity/10.0f );
    
    // wireless state
    print_log(DEBUG,1,"[%s] wireless link quality level: %d\n", m_p_log_id, state_t.w_link_quality_level );
    print_log(DEBUG,1,"[%s] wireless link quality      : %d\n", m_p_log_id, state_t.w_link_quality );
    
    memset( &wlan_state_t, 0, sizeof(vw_wlan_state_t) );
    
    b_wlan_state_result = get_state_wlan_lock( "wlan0", &wlan_state_t );
    
    if( b_wlan_state_result == false )
    {
        print_log( ERROR, 1, "[%s] vw_net_wlan_state failed\n", m_p_log_id );
    }
    else
    {
        print_log(DEBUG,1,"[%s] ESSID    : %s\n", m_p_log_id, wlan_state_t.p_essid);
        print_log(DEBUG,1,"[%s] AP MAC   : %02X:%02X:%02X:%02X:%02X:%02X\n", m_p_log_id, \
                wlan_state_t.p_ap_mac[0], wlan_state_t.p_ap_mac[1], \
                wlan_state_t.p_ap_mac[2], wlan_state_t.p_ap_mac[3], \
                wlan_state_t.p_ap_mac[4], wlan_state_t.p_ap_mac[5] );
        
        print_log(DEBUG,1,"[%s] frequency: %1.3f GHz\n", m_p_log_id, wlan_state_t.n_frequency/1000.0f);
        print_log(DEBUG,1,"[%s] level    : %d dBm\n",    m_p_log_id, wlan_state_t.n_level);
        print_log(DEBUG,1,"[%s] bit rate : %d Mbps\n",   m_p_log_id, wlan_state_t.n_bitrate);
        print_log(DEBUG,1,"[%s] tx power : %d dBm\n",    m_p_log_id, wlan_state_t.n_txpower);
    }
    print_log(DEBUG,1,"[%s] power mode     : %d(0: normal, 1: wakeup, 2: sleep)\n", m_p_log_id, state_t.w_power_mode );
    print_log(DEBUG,1,"[%s] fpga ready     : %d(0: not ready, 1: ready)\n", m_p_log_id, state_t.b_fpga_ready );
    print_log(DEBUG,1,"[%s] Gate state     : 0x%04X(Normal: 0x0FFF for 4343VW, 3643VW or 0x03FF for 2530VW)\n", m_p_log_id, state_t.w_gate_state );
    print_log(DEBUG,1,"[%s] SCU connection : %d(0: not connected, 1: connected)\n", m_p_log_id, state_t.b_scu_connected );
    
    if( state_t.w_network_mode == eNET_MODE_AP )
    {
        print_log(DEBUG,1,"[%s] Information of a connected station\n", m_p_log_id);
        print_log(DEBUG,1,"[%s] Station MAC: %s\n", m_p_log_id, state_t.p_ap_station_mac);
        print_log(DEBUG,1,"[%s] \tinactive time: %d ms\n", m_p_log_id, state_t.p_ap_info[eAP_INFO_INACTIVE_TIME]);
        print_log(DEBUG,1,"[%s] \tsignal:        %d dBm\n", m_p_log_id, state_t.p_ap_info[eAP_INFO_SIGNAL]);
        print_log(DEBUG,1,"[%s] \tsignal avg:    %d dBm\n", m_p_log_id, state_t.p_ap_info[eAP_INFO_SIGNAL_AVERAGE]);
        
        print_log(DEBUG,1,"[%s] \trx bytes:      %d\n", m_p_log_id, state_t.p_ap_info[eAP_INFO_RX_BYTES]);
        print_log(DEBUG,1,"[%s] \trx bitrate:    %d MBit/s\n", m_p_log_id, state_t.p_ap_info[eAP_INFO_RX_BITRATE]);
        print_log(DEBUG,1,"[%s] \trx MCS:        %d\n", m_p_log_id, state_t.p_ap_info[eAP_INFO_RX_MCS]);
        print_log(DEBUG,1,"[%s] \trx packets:    %d\n", m_p_log_id, state_t.p_ap_info[eAP_INFO_RX_PACKET]);
        print_log(DEBUG,1,"[%s] \trx retries:    %d\n", m_p_log_id, state_t.p_ap_info[eAP_INFO_RX_RETRIES]);
        
        print_log(DEBUG,1,"[%s] \ttx bytes:      %d\n", m_p_log_id, state_t.p_ap_info[eAP_INFO_TX_BYTES]);
        print_log(DEBUG,1,"[%s] \ttx bitrate:    %d MBit/s\n", m_p_log_id, state_t.p_ap_info[eAP_INFO_TX_BITRATE]);
        print_log(DEBUG,1,"[%s] \ttx MCS:        %d\n", m_p_log_id, state_t.p_ap_info[eAP_INFO_TX_MCS]);
        print_log(DEBUG,1,"[%s] \ttx packets:    %d\n", m_p_log_id, state_t.p_ap_info[eAP_INFO_TX_PACKET]);
        print_log(DEBUG,1,"[%s] \ttx retries:    %d\n", m_p_log_id, state_t.p_ap_info[eAP_INFO_TX_RETRIES]);
        
    }

}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::get_state_wlan_lock( const s8* i_p_dev_name, vw_wlan_state_t* o_p_state )
{
	bool b_result;
	
	iw_command_lock();
    b_result = vw_net_wlan_state( "wlan0", o_p_state );
    iw_command_unlock();
    
    return b_result;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::update_start( VW_FILE_ID i_id_e, const s8* i_p_file_name )
{
    if( i_id_e == eVW_FILE_ID_MAX )
    {
        return false;
    }
    
    if( i_id_e == eVW_FILE_ID_FPGA )
    {
        return m_p_fpga_c->config_start( i_p_file_name );
    }
    else if( i_id_e == eVW_FILE_ID_PWR_MICOM )
    {
        if(is_vw_revision_board())
            return m_p_micom_c->update_start( i_p_file_name );
        else   
            return true;
    }
    
	return update_proc_start(i_id_e, i_p_file_name);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::update_stop( VW_FILE_ID i_id_e )
{
    if( i_id_e == eVW_FILE_ID_MAX )
    {
        return;
    }
    
    if( i_id_e == eVW_FILE_ID_FPGA )
    {
        m_p_fpga_c->config_stop();
        return;
    }
    else if( i_id_e == eVW_FILE_ID_PWR_MICOM )
    {
        if(is_vw_revision_board())
            return m_p_micom_c->update_stop();
        else   
            return;
    }
    
    update_proc_stop();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CVWSystem::update_get_progress( VW_FILE_ID i_id_e )
{
    if( i_id_e == eVW_FILE_ID_MAX )
    {
        return 100;
    }
    
    if( i_id_e == eVW_FILE_ID_FPGA )
    {
        return m_p_fpga_c->config_get_progress();
    }
    else if( i_id_e == eVW_FILE_ID_PWR_MICOM )
    {
        if(is_vw_revision_board())
            return m_p_micom_c->get_update_progress();
        else   
            return 100;
    }

    
    return m_w_update_progress;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::update_get_result( VW_FILE_ID i_id_e )
{
    if( i_id_e == eVW_FILE_ID_MAX )
    {
        return false;
    }
    
    if( i_id_e == eVW_FILE_ID_FPGA )
    {
        return m_p_fpga_c->config_get_result();
    }
    else if( i_id_e == eVW_FILE_ID_PWR_MICOM )
    {
        if(is_vw_revision_board())
            return m_p_micom_c->get_update_result();
        else   
            return true;
    }
    
    return m_b_update_result;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::update_proc_start( VW_FILE_ID i_id_e, const s8* i_p_file_name )
{
    if( m_b_update_is_running == true )
    {
        print_log( ERROR, 1, "[%s] update is already started.\n", m_p_log_id);
        update_proc_stop();
        
        return false;
    }
    
    m_file_id_e = i_id_e;
    strcpy( m_p_update_file_name, i_p_file_name );
    m_w_update_progress = 0;
    m_b_update_result   = false;
    
    // launch capture thread
    m_b_update_is_running = true;
    if( pthread_create(&m_update_thread_t, NULL, update_routine, ( void * ) this)!= 0 )
    {
    	print_log( ERROR, 1, "[%s] pthread_create:%s\n", m_p_log_id, strerror( errno ));
    	m_b_update_is_running = false;
    	
    	return false;
    }
    pthread_detach(m_update_thread_t);

    return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::update_proc_stop(void)
{
    if( m_b_update_is_running == false )
    {
        print_log( ERROR, 1, "[%s] update_proc is already stopped.\n", m_p_log_id);
        return;
    }
    
    m_b_update_is_running = false;
	
    //20210913. youngjun han
    //pthread_create 직후 pthread_detach 방식으로 변경
    /*
    if( pthread_join( m_update_thread_t, NULL ) != 0 )
	{
		print_log( ERROR, 1,"[%s] pthread_join: update_proc:%s\n", \
		                    m_p_log_id, strerror( errno ));
	}
    */
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::mmc_write( VW_FILE_ID i_id_e, const s8* i_p_file_name )
{
	bool b_result = true;
	
    if( i_id_e == eVW_FILE_ID_BOOTLOADER )
    {
        sys_command("echo 0 > /sys/block/mmcblk%sboot0/force_ro",SYS_MOUNT_MMCBLK_NUM);
        sys_command("dd if=%s of=/dev/mmcblk%sboot0 seek=66 count=4000", i_p_file_name,SYS_MOUNT_MMCBLK_NUM);
        sys_command("sync");
        sys_command("echo 1 > /sys/block/mmcblk%sboot0/force_ro",SYS_MOUNT_MMCBLK_NUM);
    }
    else if( i_id_e == eVW_FILE_ID_KERNEL )
    {
        sys_command("dd if=%s of=/dev/mmcblk%s seek=2048 count=80000", i_p_file_name,SYS_MOUNT_MMCBLK_NUM);
        sys_command("sync");
    }
    else if( i_id_e == eVW_FILE_ID_RFS )
    {
        sys_command("dd if=%s of=/dev/mmcblk%s seek=42050 count=95000", i_p_file_name,SYS_MOUNT_MMCBLK_NUM);
        sys_command("sync");
    }
    else if( i_id_e == eVW_FILE_ID_DTB )
    {
        sys_command("dd if=%s of=/dev/mmcblk%s seek=87052 count=1024", i_p_file_name,SYS_MOUNT_MMCBLK_NUM);
        sys_command("sync");
    }
    else if( i_id_e == eVW_FILE_ID_APPLICATION )
    {
    	sys_command("rm -rf %s", WEB_SAVED_DIR );
        sys_command("mv %s  /tmp/app_image.gz", i_p_file_name);        // /tmp/app_image.gz
        sys_command("gunzip /tmp/app_image.gz");        // /tmp/app_image.gz
        sys_command("mkdir /tmp/app_mount");
        sys_command("mount -t ext4 -o loop /tmp/app_image /tmp/app_mount/");
        
        sleep_ex(3000000);

        sys_command("cp -ar /tmp/app_mount/* %s", SYS_CONFIG_PATH);
        sys_command("sync");
        
        // without verification
        sys_command("umount /tmp/app_mount");
        sys_command("rm /tmp/app_image");
        sys_command("losetup -d /dev/loop0");
    }
    else if( i_id_e == eVW_FILE_ID_OFFSET )
    {
        mmc_write_bin_file(i_p_file_name, OFFSET_BIN_FILE);
    }
    else if( i_id_e == eVW_FILE_ID_DEFECT_MAP )
    {
        mmc_write_bin_file(i_p_file_name, DEFECT_MAP_FILE);
    }
    else if( i_id_e == eVW_FILE_ID_GAIN )
    {
        mmc_write_bin_file(i_p_file_name, GAIN_BIN_FILE);
    }
    else if( i_id_e == eVW_FILE_ID_OSF_PROFILE )
    {
        mmc_write_bin_file(i_p_file_name, OSF_PROFILE_FILE);
    }
    else if( i_id_e == eVW_FILE_ID_OFFSET_BIN2 )
    {
        mmc_write_bin_file(i_p_file_name, OFFSET_BIN2_FILE);
    }
    else if( i_id_e == eVW_FILE_ID_DEFECT_BIN2 )
    {
        mmc_write_bin_file(i_p_file_name, DEFECT_MAP2_FILE);
    }
    else if( i_id_e == eVW_FILE_ID_DEFECT_GRID_BIN2 )
    {
        mmc_write_bin_file(i_p_file_name, DEFECT_GRID_MAP2_FILE);
    }
    else if( i_id_e == eVW_FILE_ID_DEFECT_FOR_GATEOFF_BIN2 )
    {
        mmc_write_bin_file(i_p_file_name, DEFECT_GATEOFF_FILE);
    }
    else if( i_id_e == eVW_FILE_ID_GAIN_BIN2 )
    {
        mmc_write_bin_file(i_p_file_name, GAIN_BIN2_FILE);
    }
    else if( i_id_e == eVW_FILE_ID_PREVIEW_GAIN )
    {
        mmc_write_bin_file(i_p_file_name, PREVIEW_GAIN_FILE);
    }
    else if( i_id_e == eVW_FILE_ID_PREVIEW_DEFECT )
    {
        mmc_write_bin_file(i_p_file_name, PREVIEW_DEFECT_FILE);
    }
    else if( i_id_e == eVW_FILE_ID_CONFIG )
    {
        s8      p_save_path[256];
        
        sprintf( p_save_path, "%s%s", SYS_CONFIG_PATH, CONFIG_XML_FILE );
        
        file_copy_to_flash( i_p_file_name, p_save_path );
    }
    else if( i_id_e == eVW_FILE_ID_PRESET )
    {
        s8      p_save_path[256];
        
        sprintf( p_save_path, "%s%s", SYS_CONFIG_PATH, PRESET_XML_FILE );
        
        file_copy_to_flash( i_p_file_name, p_save_path );
    }
    else if( i_id_e == eVW_FILE_ID_PATIENT_LIST_XML )
    {
        sys_command("cp %s %s", i_p_file_name, WEB_SAVED_XML_FILE_PATIENT );
        sys_command("sync");
    }
    else if( i_id_e == eVW_FILE_ID_STEP_XML )
    {
        sys_command("cp %s %s", i_p_file_name, WEB_SAVED_XML_FILE_STEP );
        sys_command("sync");
        
        sys_command("cp %s %s", WEB_SAVED_XML_FILE_STEP, WEB_XML_FILE_DIR );
    }
    
    return b_result;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::mmc_write_bin_file(const s8* i_p_src_file, const s8* i_p_dst_file)
{
    FILE*   p_src_file;
    FILE*   p_dst_file;
    u8*     p_data;
    u32     n_size;
    s8      p_save_path_tmp[256];
    s8      p_save_path[256];
    
    p_src_file = fopen(i_p_src_file, "rb");
    
    if( p_src_file == NULL )
    {
        print_log(ERROR, 1, "[%s] mmc_write_bin_file(%s) open failed.\n", \
                                m_p_log_id, i_p_src_file);
        return;
    }
    
    n_size = file_get_size( p_src_file );
    p_data = (u8*)malloc( n_size );
    
    if( p_data == NULL )
    {
        print_log(ERROR, 1, "[%s] mmc_write_bin_file(%s) malloc failed.\n", \
                                m_p_log_id, i_p_src_file);
        fclose( p_src_file );
        return;
    }
    
    fread( p_data, 1, n_size, p_src_file );
    fclose( p_src_file );
    
    sprintf( p_save_path_tmp, "/tmp/tmp_%s", i_p_dst_file );
    
    p_dst_file = fopen( p_save_path_tmp, "wb" );
    
    if( p_dst_file == NULL )
    {
        print_log(ERROR, 1, "[%s] mmc_write_bin_file(%s) open failed.\n", \
                                m_p_log_id, p_save_path_tmp);
        safe_free(p_data);
        return;
    }
    
    // append CRC
    file_write_bin( p_dst_file, p_data, n_size );
    
    fclose( p_dst_file );
    safe_free(p_data);
    
    // copy to flash
    sprintf( p_save_path, "%s%s", SYS_DATA_PATH, i_p_dst_file );
    
    file_copy_to_flash( p_save_path_tmp, p_save_path );
    
    sys_command("rm %s", p_save_path_tmp);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::mmc_verify( VW_FILE_ID i_id_e, const s8* i_p_file_name )
{
    FILE*   p_file;
    u32     n_file_size;
    u32     n_block_count;
    bool    b_result = false;
    s8      p_file_path1[512];
    s8      p_file_path2[512];
    
    if( i_id_e != eVW_FILE_ID_APPLICATION )
    {
        p_file = fopen( i_p_file_name, "rb" );
    
        if( p_file == NULL )
        {
            print_log(ERROR, 1, "[%s] file(%s) open failed.\n", \
                                m_p_log_id, i_p_file_name);
            return false;
        }
        
        n_file_size = file_get_size( p_file );
        fclose( p_file );
        
        n_block_count = n_file_size/512;
        if( n_file_size % 512 > 0 )
        {
            n_block_count += 1;
        }
    }
    
    if( i_id_e == eVW_FILE_ID_BOOTLOADER )
    {
        sys_command("dd if=/dev/mmcblk%sboot0 of=/tmp/boot.dump skip=2 count=%d", SYS_MOUNT_MMCBLK_NUM, n_block_count);
        file_compare( i_p_file_name, "/tmp/boot.dump", &b_result );
        sys_command("rm /tmp/boot.dump");
    }
    else if( i_id_e == eVW_FILE_ID_KERNEL )
    {
        sys_command("dd if=/dev/mmcblk%s of=/tmp/kernel.dump skip=2048 count=%d", SYS_MOUNT_MMCBLK_NUM, n_block_count);
        file_compare( i_p_file_name, "/tmp/kernel.dump", &b_result );
        sys_command("rm /tmp/kernel.dump");
    }
    else if( i_id_e == eVW_FILE_ID_RFS )
    {
        sys_command("dd if=/dev/mmcblk%s of=/tmp/rfs.dump skip=32768 count=%d", SYS_MOUNT_MMCBLK_NUM, n_block_count);
        file_compare( i_p_file_name, "/tmp/rfs.dump", &b_result );
        sys_command("rm /tmp/rfs.dump");
    }
    else if( i_id_e == eVW_FILE_ID_DTB )
    {
        sys_command("dd if=/dev/mmcblk%s of=/tmp/dtb.dump skip=30720 count=%d", SYS_MOUNT_MMCBLK_NUM, n_block_count);
        file_compare( i_p_file_name, "/tmp/dtb.dump", &b_result );
        sys_command("rm /tmp/dtb.dump");
    }
    else if( i_id_e == eVW_FILE_ID_APPLICATION )
    {
        // app_mount 디렉토리 내의 각 파일을 비교한다.
        sys_command("ls -1 /tmp/app_mount > list");
        p_file = fopen( "list", "r" );
        
        if( p_file == NULL )
        {
            print_log(ERROR, 1, "[%s] file(list) open failed.\n", \
                                m_p_log_id);
            sys_command("umount /tmp/app_mount");
            sys_command("rm /tmp/app_image");
            return false;
        }
        
        s8 p_file_name[256];
        while(1)
        {
            memset( p_file_name, 0, sizeof(p_file_name) );
            if( fscanf( p_file, "%s", p_file_name ) <= 0 )
            {
                break;
            }
            
            print_log(DEBUG, 1, "[%s] file name: %s\n", m_p_log_id, p_file_name );
            
            if( strcmp( p_file_name, "lost+found" ) == 0 )
            {
                continue;
            }
            
            sprintf( p_file_path1, "/tmp/app_mount/%s", p_file_name );
            sprintf( p_file_path2, "%s%s", SYS_CONFIG_PATH, p_file_name );
            
            file_compare( p_file_path1, p_file_path2, &b_result );
            
            if( b_result == false )
            {
                print_log(ERROR, 1, "[%s] verfication failed(file: %s)...\n", \
                                    m_p_log_id, p_file_path2 );
                break;
            }
        }
        fclose( p_file );
        sys_command("umount /tmp/app_mount");
        sys_command("rm /tmp/app_image");
    }
    
    return b_result;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void* update_routine( void * arg )
{
	CVWSystem* sys = (CVWSystem *)arg;
	sys->update_proc();
	pthread_exit( NULL );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CVWSystem::update_proc(void)
{
    print_log(DEBUG, 1, "[%s] start update_proc(id: %d, file: %s)...\n", \
                        m_p_log_id, m_file_id_e, m_p_update_file_name );
                        
                        
	bool b_valid_chk = true;
	VALID_ERR e_valid_chk = eVALID_NO_ERR;
	
	m_b_update_result = false;
	
	// check valid file
	if( m_file_id_e == eVW_FILE_ID_KERNEL  || m_file_id_e == eVW_FILE_ID_RFS)
	{
		
		e_valid_chk = image_file_valid_confirm(m_p_update_file_name);
	}
	else if( m_file_id_e == eVW_FILE_ID_DTB)
	{
		e_valid_chk = fdt_file_valid_confirm(m_p_update_file_name);
	}	
	else if( m_file_id_e == eVW_FILE_ID_BOOTLOADER )
	{
		e_valid_chk = boot_file_valid_confirm(m_p_update_file_name);
	}
	
	if( e_valid_chk != eVALID_NO_ERR)
	{
		b_valid_chk = false;
		
		if( e_valid_chk == eVALID_FILE_ERR)
		{
			print_log(ERROR, 1, "[%s] verfication eVALID_FILE_ERR(file: %s)...\n", \
                                    m_p_log_id, m_p_update_file_name );
		}
		else if( e_valid_chk == eVALID_MAGIC_KEY_ERR)
		{
			print_log(ERROR, 1, "[%s] verfication eVALID_MAGIC_KEY_ERR(file: %s)...\n", \
                                    m_p_log_id, m_p_update_file_name );
		}
		else if( e_valid_chk == eVALID_HEADER_CRC_ERR)
		{
			print_log(ERROR, 1, "[%s] verfication eVALID_HEADER_CRC_ERR(file: %s)...\n", \
                                    m_p_log_id, m_p_update_file_name );
		}
		else if( e_valid_chk == eVALID_DATA_CRC_ERR)
		{
			print_log(ERROR, 1, "[%s] verfication eVALID_DATA_CRC_ERR(file: %s)...\n", \
                                    m_p_log_id, m_p_update_file_name );
		}
		else if( e_valid_chk == eVALID_IVT_TAG_ERR)
		{
			print_log(ERROR, 1, "[%s] verfication eVALID_IVT_TAG_ERR(file: %s)...\n", \
                                    m_p_log_id, m_p_update_file_name );
		}
	}

	
    if( b_valid_chk)
    {
    	m_b_update_result = mmc_write(m_file_id_e, m_p_update_file_name);
    	//m_b_update_result = true;
    }
    m_w_update_progress = 100;
    
    print_log(DEBUG, 1, "[%s] stop update_proc(id: %d, file: %s)...\n", \
                        m_p_log_id, m_file_id_e, m_p_update_file_name );
    
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void* pm_state_routine( void * arg )
{
	CVWSystem* sys = (CVWSystem *)arg;
	sys->pm_state_proc();
	pthread_exit( NULL );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CVWSystem::pm_state_proc(void)
{
    print_log(DEBUG, 1, "[%s] start pm_state_proc...(%d)\n", \
                        m_p_log_id, syscall( __NR_gettid ) );
    
    PM_MODE     old_pm_mode_e, cur_pm_mode_e;
    bool        b_pm_test_started = false;
    bool        b_sw_connected = false;
    
    pm_init();
    
    m_pm_mode_e = ePM_MODE_NORMAL;
    old_pm_mode_e = m_pm_mode_e;
    cur_pm_mode_e = m_pm_mode_e;

    pm_print_mode( cur_pm_mode_e );
    
    auto_offset_set(true);
    capture_init();
    
    while( m_b_pm_is_running == true )
    {
        if( capture_is_ready(cur_pm_mode_e) == true )
        {
            capture_check_mode();
        }
        
        if( cur_pm_mode_e != old_pm_mode_e )
        {
            pm_lock();
            m_pm_mode_e = cur_pm_mode_e;
            pm_unlock();
            
            pm_print_mode( cur_pm_mode_e );
            old_pm_mode_e = cur_pm_mode_e;
            
            pm_reset_sleep_timer();
            
            if( cur_pm_mode_e == ePM_MODE_NORMAL )
            {
                auto_offset_set( true );
            }
            
            message_send( eMSG_POWER_MODE, (u8*)&cur_pm_mode_e, sizeof(PM_MODE) );
        }
        else
        {
            //sleep_ex(500000);       // 500 ms
            sleep_ex(100000);       // 100 ms
        }
        
        if( b_pm_test_started != m_b_pm_test_sleep_start )
        {
            b_pm_test_started = m_b_pm_test_sleep_start;
            
            if( m_b_pm_test_sleep_start == true )
            {
                cur_pm_mode_e = ePM_MODE_SLEEP;
                pm_prepare_sleep();
                continue;
            }
            else
            {
            	if(cur_pm_mode_e!= ePM_MODE_NORMAL)
               		cur_pm_mode_e = ePM_MODE_WAKEUP;
            }
        }
        
        if( cur_pm_mode_e == ePM_MODE_NORMAL )
        {
            if( b_sw_connected != m_b_sw_connected )
            {
                b_sw_connected = m_b_sw_connected;
                
                if( b_sw_connected == true )
                {
                    auto_offset_set( false );
                }
                else
                {
                    auto_offset_set( true );
                }
            }
        }
        
        switch( cur_pm_mode_e )
        {
            case ePM_MODE_NORMAL:
            {
                cur_pm_mode_e = pm_check_normal();
                break;
            }
            case ePM_MODE_SLEEP:
            {
                cur_pm_mode_e = pm_check_sleep();
                break;
            }
            case ePM_MODE_DEEPSLEEP:
            case ePM_MODE_SHUTDOWN:
            {
                cur_pm_mode_e = pm_check_shutdown();
                break;
            }
            case ePM_MODE_WAKEUP:
            {
                cur_pm_mode_e = pm_check_wakeup();
                break;
            }
            default:
            {
                break;
            }
        }
        
        if( m_b_pm_wake_up == true )
        {
            cur_pm_mode_e = ePM_MODE_WAKEUP;
        }

        
    }
    
    print_log(DEBUG, 1, "[%s] stop pm_state_proc...\n", m_p_log_id );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::pm_init(void)
{
    s8* p_config = NULL;
    
    p_config = config_get(eCFG_PM_SLEEP_ENABLE);
    if( strcmp( p_config, "on" ) == 0 )
    {
        m_pm_config_t.b_sleep_enable = true;
    }
    else
    {
        m_pm_config_t.b_sleep_enable = false;
    }
    
    p_config = config_get(eCFG_PM_SHUTDOWN_ENABLE);
    if( strcmp( p_config, "on" ) == 0 )
    {
        m_pm_config_t.b_shutdown_enable = true;
    }
    else
    {
        m_pm_config_t.b_shutdown_enable = false;
    }
    
    p_config = config_get(eCFG_PM_SLEEP_ENTRANCE_TIME);
    m_pm_config_t.w_sleep_time = atoi(p_config);
    
    p_config = config_get(eCFG_PM_SHUTDOWN_ENTRANCE_TIME);
    m_pm_config_t.w_shutdown_time = atoi(p_config);
    
    pm_reset_sleep_timer();
    
    print_log(DEBUG, 1, "[%s] sleep: %d, time: %d\n", m_p_log_id, \
                        m_pm_config_t.b_sleep_enable, m_pm_config_t.w_sleep_time );
    print_log(DEBUG, 1, "[%s] shutdown: %d, time: %d\n", m_p_log_id, \
                        m_pm_config_t.b_shutdown_enable, m_pm_config_t.w_shutdown_time );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::pm_print_mode( PM_MODE i_mode_e )
{
    switch( i_mode_e )
    {
        case ePM_MODE_NORMAL:
        {
            print_log(DEBUG, 1, "[%s] PM_MODE_NORMAL\n", m_p_log_id);
            break;
        }
        case ePM_MODE_SLEEP:
        {
            print_log(DEBUG, 1, "[%s] PM_MODE_SLEEP\n", m_p_log_id);
            break;
        }
        case ePM_MODE_DEEPSLEEP:
        {
            print_log(DEBUG, 1, "[%s] PM_MODE_DEEPSLEEP\n", m_p_log_id);
            break;
        }
        case ePM_MODE_SHUTDOWN:
        {
            print_log(DEBUG, 1, "[%s] PM_MODE_SHUTDOWN\n", m_p_log_id);
            break;
        }
        case ePM_MODE_WAKEUP:
        {
            print_log(DEBUG, 1, "[%s] PM_MODE_WAKEUP\n", m_p_log_id);
            break;
        }
        default:
        {
            break;
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
PM_MODE CVWSystem::pm_check_normal(void)
{
    auto_offset_check();
    
    if( m_b_pm_keep_normal == true
        ||  m_p_hw_mon_c->power_source_can_charge(m_p_hw_mon_c->get_power_source()) == true )
    {
    	m_b_go_sleep = false;
        pm_reset_sleep_timer();
        return ePM_MODE_NORMAL;
    }
    
    if( m_pm_config_t.b_sleep_enable == true )
    {
        if( m_b_go_sleep == true )
        {
        	m_b_go_sleep = false;
        	pm_reset_sleep_timer();
        	return ePM_MODE_SLEEP;
        }
        	
        if( pm_is_timer_expired(ePM_MODE_NORMAL, ePM_TIMER_SLEEP) == true )
        {
            return ePM_MODE_SLEEP;
        }
    }
    
    m_b_go_sleep = false;
    
    if( m_pm_config_t.b_shutdown_enable == true )
    {
        if( pm_is_timer_expired(ePM_MODE_NORMAL, ePM_TIMER_SHUTDOWN) == true )
        {
            return ePM_MODE_SHUTDOWN;
        }
    }
    
    return ePM_MODE_NORMAL;
}
	
/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
PM_MODE CVWSystem::pm_check_sleep(void)
{
    if( m_b_pm_test_sleep_start == true )
    {
        return ePM_MODE_SLEEP;
    }
    
    if( m_b_pm_keep_normal == true
        || m_p_hw_mon_c->power_source_can_charge(m_p_hw_mon_c->get_power_source()) == true
        || m_pm_config_t.b_sleep_enable == false )
    {
        if( m_b_need_wake_up == true )
        {
            return ePM_MODE_WAKEUP;
        }
        
        return ePM_MODE_NORMAL;
    }
    
    if( pm_prepare_sleep() == false )
    {
    	return ePM_MODE_NORMAL;
    }
    
    if( m_pm_config_t.b_shutdown_enable == true )
    {
        if( pm_is_timer_expired(ePM_MODE_SLEEP, ePM_TIMER_SHUTDOWN) == true )
        {
            return ePM_MODE_SHUTDOWN;
        }
    }
    
    return ePM_MODE_SLEEP;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CVWSystem::pm_go_sleep(void)
{
	u16 w_ret = 0;
	if(m_p_hw_mon_c->power_source_can_charge(m_p_hw_mon_c->get_power_source()) == true)
	{
		w_ret |= ((0x01) << eCHARGING);
	}
	
	if(m_pm_config_t.b_sleep_enable == false)
	{
		w_ret |= ((0x01) << eSLEEP_MODE_OFF);
	}
	
	if(m_b_pm_keep_normal == true)
	{
		w_ret |= ((0x01) << eKEEP_NORMAL);
	}

	if(m_b_image_grab == true)
	{
		w_ret |= ((0x01) << eIMAGE_GRAB);
	}
	
	if(m_p_fpga_c->is_ready() == false)
	{
		w_ret |= ((0x01) << eFPGA_NOT_READY);
	}
	
	if(m_b_need_wake_up == true)
	{
		w_ret |= ((0x01) << eSLEEPING);
	}
		
	if(w_ret == 0)
	{
		m_b_go_sleep = true;
	}
	else
	{
		print_log(DEBUG, 1, "[%s] PM Go sleep error : %d\n", m_p_log_id, w_ret);
	}
	
	return w_ret;
}
	
/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::pm_prepare_sleep(void)
{
    if( m_b_need_wake_up == false )
    {
    	if( m_b_image_grab == true )
    	{
    		return false;
    	}
        
        print_log(DEBUG, 1, "[%s] PM_MODE_SLEEP: prepare sleep mode\n", m_p_log_id);
	    
        m_b_need_wake_up = true;

        //AED Power off 조건 강제 발생
        m_p_fpga_c->trigger_set_type(eTRIG_SELECT_ACTIVE_LINE);
		aed_power_control(eGEN_INTERFACE_RAD, eTRIG_SELECT_ACTIVE_LINE, (u8)0 );

        m_p_fpga_c->power_down();
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
bool CVWSystem::pm_get_temperature( u16* o_p_temperature )
{
    bool b_result = false;
    
    pm_lock();
    if( m_pm_mode_e == ePM_MODE_NORMAL )
    {
        *o_p_temperature = m_p_fpga_c->get_temperature(0);
        b_result = true;
    }
    pm_unlock();
    
    return b_result;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
PM_MODE CVWSystem::pm_check_shutdown(void)
{
    //sleep_ex(3000000);      // 3초
    
    
    return ePM_MODE_SHUTDOWN;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
PM_MODE CVWSystem::pm_check_wakeup(void)
{
	m_b_pm_wake_up = false;
    set_oled_display_event(eWAKEUP_EVENT);
    
    if( m_p_fpga_c->power_up() == true )
    {   
    	dev_load_trigger_type_config();		// AED 모드인 경우 AED proc start
        m_b_need_wake_up = false;

        m_p_fpga_c->set_capture_count(m_p_env_c->oper_info_get_capture_count());

		if(is_cal_data_valid() == false)
		{
			cal_data_load();
			wait_until_cal_data_load_end();
		}
		
        m_p_fpga_c->flush_wait_end();
        
        capture_ready();
        
        set_oled_display_event(eACTION_EVENT_END);
        pm_test_sleep(false);
        
        return ePM_MODE_NORMAL;
    }
    
    set_oled_display_event(eACTION_EVENT_END);
    
    return ePM_MODE_WAKEUP;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::pm_wake_up( bool i_b_by_sw )
{
    pm_lock();
    if( m_pm_mode_e == ePM_MODE_NORMAL
    	|| m_pm_mode_e == ePM_MODE_WAKEUP )
    {
    	safe_delete( m_p_sleep_wait_c );
        pm_unlock();
        return;
    }
    pm_unlock();
    
    if( m_b_need_wake_up == true &&
    	m_b_pm_wake_up == false )
    {
        print_log(DEBUG, 1, "[%s] wake up by %s\n", m_p_log_id, (i_b_by_sw == true)?"S/W":"Micom or NFC");
        
        m_b_pm_wake_up = true;
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::pm_is_normal_mode(void)
{
    if( m_pm_mode_e == ePM_MODE_NORMAL )
    {
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
bool CVWSystem::pm_is_timer_expired( PM_MODE i_mode_e, PM_TIMER i_timer_e )
{
    bool b_result = false;
    
    pm_lock();
    
    if( i_mode_e == ePM_MODE_NORMAL )
    {
        if( m_p_sleep_wait_c == NULL )
        {
            if( i_timer_e == ePM_TIMER_SLEEP )
            {
                print_log(DEBUG, 1, "[%s] PM_MODE_NORMAL: sleep wait started\n", m_p_log_id);
                m_p_sleep_wait_c = new CTime(m_pm_config_t.w_sleep_time * (60 * 1000));
            }
            else
            {
                print_log(DEBUG, 1, "[%s] PM_MODE_NORMAL: shutdown wait started\n", m_p_log_id);
                m_p_sleep_wait_c = new CTime(m_pm_config_t.w_shutdown_time * (60 * 1000));
            }
        }
        else if( m_p_sleep_wait_c->is_expired() == true )
        {
            safe_delete( m_p_sleep_wait_c );
            
			if( i_timer_e == ePM_TIMER_SLEEP )
	        {
	            print_log(DEBUG, 1, "[%s] PM_MODE_NORMAL: sleep by timer\n", m_p_log_id);
	        }
	        else
	        {
	            print_log(DEBUG, 1, "[%s] PM_MODE_NORMAL: shutdown by timer\n", m_p_log_id);
	        }
	        b_result = true;
        }
    }
    else if( i_mode_e == ePM_MODE_SLEEP )
    {
        if( m_p_sleep_wait_c == NULL )
        {
            m_p_sleep_wait_c = new CTime(m_pm_config_t.w_shutdown_time * (60 * 1000));
            print_log(DEBUG, 1, "[%s] PM_MODE_SLEEP: shutdown wait started(%d minutes)\n", \
                                m_p_log_id, m_pm_config_t.w_shutdown_time);
        }
        else if( m_p_sleep_wait_c->is_expired() == true )
        {
            safe_delete( m_p_sleep_wait_c );
            print_log(DEBUG, 1, "[%s] PM_MODE_SLEEP: shutdown by timer\n", m_p_log_id);
            
            b_result = true;
        }
    }
    
    if( b_result == true
    	&& m_b_image_grab == true )
    {
    	b_result = false;
    }
    
    pm_unlock();
    
    return b_result;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::pm_reset_sleep_timer(void)
{
    bool b_result = false;
    
    pm_lock();
    if( m_pm_mode_e == ePM_MODE_NORMAL )
    {
        safe_delete( m_p_sleep_wait_c );
        
        b_result = true;
    }
    pm_unlock();
    
    return b_result;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CVWSystem::pm_get_remain_sleep_time(void)
{
	u32 n_remain_sec_time = 0;
	
    pm_lock();
    if( m_pm_mode_e == ePM_MODE_NORMAL )
    {
        if( m_p_sleep_wait_c != NULL )
        {
        	n_remain_sec_time = m_p_sleep_wait_c->get_remain_time()/1000;
        }
        else
        {
        	if( m_pm_config_t.b_sleep_enable == true )
        	{
        		n_remain_sec_time = m_pm_config_t.w_sleep_time * 60;
        	}
        	else if( m_pm_config_t.b_shutdown_enable == true )
        	{
        		n_remain_sec_time = m_pm_config_t.w_shutdown_time * 60;
        	}
        	else
        	{
        		n_remain_sec_time = 3600;	// default shutdown time * 60
        	}
        }
    }
    pm_unlock();
    
    return n_remain_sec_time;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::hw_get_power_volt( power_volt_t* o_p_pwr_volt_t )
{
    memset( o_p_pwr_volt_t, 0, sizeof(power_volt_t) );
    
    m_p_hw_mon_c->get_power_volt( o_p_pwr_volt_t );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::hw_power_off( bool i_b_sw )
{
    CTime* p_wait_c = NULL;
    
    if( i_b_sw == true )
    {
    	set_oled_display_event(eEVENT_OFF);
    	print_log(DEBUG, 1, "[%s] hw_power_off: by sw\n", m_p_log_id);
    	
    	print_state();
    	dev_reg_print(eDEV_ID_ADC);
    	
    	sys_command("sync");
    	
    	sleep_ex(500000);       // 500 ms

        m_p_hw_mon_c->hw_power_off();
        return;
    }
    
    // 전원 버튼이 3초 이상 눌렸을 때, AP 버튼이 눌린 상태이면
    // 네트워크 정보 초기화 후 재부팅 진행
    if( m_p_hw_mon_c->ap_button_get_raw() == true )
    {
    	set_oled_display_event(eRESET_WLAN_EVENT);
    	
        p_wait_c = new CTime(2000);
        
        m_b_reboot = true;
        
        m_p_env_c->config_default_network();
        config_save();
        
        while( p_wait_c->is_expired() == false )
        {
            sleep_ex(500000);       // 500 ms
        }
        
        safe_delete( p_wait_c );
    
        set_oled_display_event(eEVENT_OFF);
        sleep_ex(500000);

        hw_reboot();

        return;
    }
    
    print_state();
    dev_reg_print(eDEV_ID_ADC);
    
    if( m_b_image_grab == true )
    {
        print_log(DEBUG, 1, "[%s] hw_power_off: waiting image acq.\n", m_p_log_id);
        
        p_wait_c = new CTime( 5000 + get_exp_wait_time() );
        
        while( p_wait_c->is_expired() == false )
        {
            sleep_ex(500000);       // 500 ms
            if( m_b_image_grab == false )
            {
                break;
            }
        }
        safe_delete( p_wait_c );
    }
    
    set_oled_display_event(eEVENT_OFF);
    
    sys_command("sync");
    
    sleep_ex(500000);       // 500 ms

    m_p_hw_mon_c->hw_power_off();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::hw_reboot(void)
{
	set_oled_display_event(eEVENT_OFF);
    sys_command("sync");
    sleep_ex(500000);       // 500 ms

    if (is_vw_revision_board())     m_p_micom_c->reboot();

    sys_command("reboot");
    //m_p_hw_mon_c->pmic_reset();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::file_open( VW_FILE_ID i_id_e, u32* o_p_size, u32* o_p_crc, u32 i_n_image_id )
{
    s8          p_file_path[256];
    FILE*       p_file;
    s8          p_file_option[5];
    u32         n_size;
    u32         n_crc_size = 4;
    
    m_n_file_size = 0;
    safe_free(m_p_file_data);
    
    strcpy( p_file_option, "rb" );
    
    switch( i_id_e )
    {
        case eVW_FILE_ID_CONFIG:
        {
            sprintf( p_file_path, "%s%s", SYS_CONFIG_PATH, CONFIG_XML_FILE );
            
            strcpy( p_file_option, "r" );
            n_crc_size = 0;
            break;
        }
        case eVW_FILE_ID_PRESET:
        {
            sprintf( p_file_path, "%s%s", SYS_CONFIG_PATH, PRESET_XML_FILE );
            
            strcpy( p_file_option, "r" );
            n_crc_size = 0;
            break;
        }
        case eVW_FILE_ID_OFFSET:
        {
            sprintf( p_file_path, "%s%s", SYS_DATA_PATH, OFFSET_BIN_FILE );
            break;
        }
        case eVW_FILE_ID_DEFECT_MAP:
        {
            sprintf( p_file_path, "%s%s", SYS_DATA_PATH, DEFECT_MAP_FILE );
            break;
        }
        case eVW_FILE_ID_GAIN:
        {
            sprintf( p_file_path, "%s%s", SYS_DATA_PATH, GAIN_BIN_FILE );
            break;
        }
        case eVW_FILE_ID_OSF_PROFILE:
        {
            sprintf( p_file_path, "%s%s", SYS_DATA_PATH, OSF_PROFILE_FILE );
            break;
        }
        case eVW_FILE_ID_OFFSET_BIN2:
        {
            sprintf( p_file_path, "%s%s", SYS_DATA_PATH, OFFSET_BIN2_FILE );
            break;
        }
        case eVW_FILE_ID_DEFECT_BIN2:
        {
            sprintf( p_file_path, "%s%s", SYS_DATA_PATH, DEFECT_MAP2_FILE );
            break;
        }
        case eVW_FILE_ID_DEFECT_GRID_BIN2:
        {
            sprintf( p_file_path, "%s%s", SYS_DATA_PATH, DEFECT_GRID_MAP2_FILE );
            break;
        }
        case eVW_FILE_ID_DEFECT_FOR_GATEOFF_BIN2:
        {
            sprintf( p_file_path, "%s%s", SYS_DATA_PATH, DEFECT_GATEOFF_FILE );
            break;
        }
        case eVW_FILE_ID_GAIN_BIN2:
        {
            sprintf( p_file_path, "%s%s", SYS_DATA_PATH, GAIN_BIN2_FILE );
            break;
        }
        case eVW_FILE_ID_BACKUP_IMAGE_LIST:
        {
            sprintf( p_file_path, "%s", BACKUP_IMAGE_LIST_FILE );
            backup_image_make_list();
            
            strcpy( p_file_option, "r" );
            n_crc_size = 0;
            break;
        }
        case eVW_FILE_ID_BACKUP_IMAGE:
        {
            s8 p_file_name[256];
            
            memset( p_file_name, 0, sizeof(p_file_name) );
            backup_image_find( i_n_image_id, p_file_name );
            
            if( strlen( p_file_name ) <= 0 )
            {
                print_log(ERROR, 1, "[%s] Fail to find image file(id: 0x%08X)\n", m_p_log_id, i_n_image_id);
                return false;
            }
            
            sprintf( p_file_path, "%s", p_file_name );
            
            break;
        }
        case eVW_FILE_ID_SELF_DIAGNOSIS_LIST:
        {
            sprintf( p_file_path, "%s", SELF_DIAG_LIST_FILE );
            strcpy( p_file_option, "r" );
            n_crc_size = 0;
            
            break;
        }
        case eVW_FILE_ID_SELF_DIAGNOSIS_RESULT:
        {
            sprintf( p_file_path, "%s", SELF_DIAG_RESULT_FILE );
            strcpy( p_file_option, "r" );
            n_crc_size = 0;
            
            break;
        }
        case eVW_FILE_ID_BACKUP_IMAGE_LIST_WITH_PATIENT_INFO:
        {
            sprintf( p_file_path, "%s", BACKUP_IMAGE_XML_LIST_FILE );
            strcpy( p_file_option, "r" );
            n_crc_size = 0;
            
            break;
        }
        case eVW_FILE_ID_PREVIEW_GAIN:
        {
            sprintf( p_file_path, "%s%s", SYS_DATA_PATH, PREVIEW_GAIN_FILE );
            break;
        }
        case eVW_FILE_ID_PREVIEW_DEFECT:
        {
            sprintf( p_file_path, "%s%s", SYS_DATA_PATH, PREVIEW_DEFECT_FILE );
            break;
        }
        default:
        {
            print_log(ERROR, 1, "[%s] Not supported file id(%d)\n", m_p_log_id, i_id_e);
            return false;
        }
    }

    // file read
    p_file = fopen( p_file_path, p_file_option );
    
    if( p_file == NULL )
    {
        print_log(ERROR, 1, "[%s] file(%s) open error\n", m_p_log_id, p_file_path);
        return false;
    }
    
    n_size = file_get_size( p_file );
    
    if( n_size == 0 )
    {
        print_log(ERROR, 1, "[%s] file(%s) size: 0\n", m_p_log_id, p_file_path);
        return false;
    }
    
    n_size = n_size - n_crc_size;
    
    m_p_file_data = (u8*)malloc(n_size);
    
    if( m_p_file_data == NULL )
    {
        print_log(ERROR, 1, "[%s] file(%s) malloc failed\n", m_p_log_id, p_file_path);
        return false;
    }
    
    m_n_file_size   = n_size;
    *o_p_size       = n_size;
    
    if( n_crc_size > 0 )
    {
        if( file_check_crc( p_file, m_p_file_data, m_n_file_size ) != eFILE_ERR_NO )
        {
            fclose( p_file );
            safe_free(m_p_file_data);
            print_log(ERROR, 1, "[%s] file(%s) crc failed\n", \
                                m_p_log_id, p_file_path);
            return false;
        }
    }
    else
    {
        n_size = fread( m_p_file_data, 1, n_size, p_file );
        if( n_size != m_n_file_size )
        {
            fclose( p_file );
            safe_free(m_p_file_data);
            print_log(ERROR, 1, "[%s] file(%s) read failed(%d/%d)\n", \
                                m_p_log_id, p_file_path, n_size, m_n_file_size);
            return false;
        }
    }
    
    fclose( p_file );
    
    *o_p_crc = make_crc32( m_p_file_data, m_n_file_size );
    
    print_log(DEBUG, 1, "[%s] file(%s) size: %d, crc: 0x%08X\n", m_p_log_id, \
                            p_file_path, *o_p_size, *o_p_crc);
    
    return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::file_read( u32 i_n_pos, u32 i_n_size, u8* o_p_data, u32* o_p_size )
{
    if( m_n_file_size == 0
        || m_p_file_data == NULL
        || i_n_pos >= m_n_file_size )
    {
        print_log(ERROR, 1, "[%s] file read failed(%d/%d)\n", m_p_log_id, i_n_pos, m_n_file_size);
        return false;
    }
    
    u32 n_copy_size;
    
    if( i_n_pos + i_n_size > m_n_file_size )
    {
        n_copy_size = m_n_file_size - i_n_pos;
    }
    else
    {
        n_copy_size = i_n_size;
    }
    
    memcpy( o_p_data, &m_p_file_data[i_n_pos], n_copy_size );
    
    *o_p_size = n_copy_size;
    
    return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::file_close(void)
{
    m_n_file_size = 0;
    safe_free(m_p_file_data);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::file_delete(VW_FILE_DELETE i_file_delete_e)
{
    s8  p_cmd[256];
    
    if( i_file_delete_e == eVW_FILE_DELETE_ALL )
    {
        sprintf( p_cmd, "rm -rf %s*", SYS_IMAGE_DATA_PATH );
        sys_command( p_cmd );
        
        sprintf( p_cmd, "rm -rf %slog/*", SYS_LOG_PATH );
        sys_command( p_cmd );
        
        oper_info_reset();
        
        sprintf( p_cmd, "rm %s%s", SYS_CONFIG_PATH, PRESET_XML_FILE );
        sys_command( p_cmd );

    }
    else if( i_file_delete_e == eVW_FILE_DELETE_BACKUP_IMAGE )
    {
        sprintf( p_cmd, "rm -rf %s*", SYS_IMAGE_DATA_PATH );
        sys_command( p_cmd );
    }
    else if( i_file_delete_e == eVW_FILE_DELETE_LOG_FILE )
    {
        sprintf( p_cmd, "rm -rf %slog/*", SYS_LOG_PATH );
        sys_command( p_cmd );
    }
    else if( i_file_delete_e == eVW_FILE_DELETE_OPER_INFO )
    {
        oper_info_reset();
    }
    else if( i_file_delete_e == eVW_FILE_DELETE_PRESET )
    {
        sprintf( p_cmd, "rm %s%s", SYS_CONFIG_PATH, PRESET_XML_FILE );
        sys_command( p_cmd );
    }
    else if( i_file_delete_e == eVW_FILE_DELETE_FPGA_BIN )
    {
        sprintf( p_cmd, "rm %s%s", SYS_CONFIG_PATH, CONFIG_FPGA_FILE );
        sys_command( p_cmd );
    }
    else
    {
        print_log(ERROR, 1, "[%s] unknown file type(0x%02X)\n", m_p_log_id, i_file_delete_e);
        return;
    }
    
    sys_command("sync");
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CVWSystem::backup_image_get_count(void)
{
    return file_get_count(SYS_IMAGE_DATA_PATH, "*.bin*");
}

/******************************************************************************/
/**
 * @brief   특정 영상 파일을 삭제한다.
 * @param   u32 i_n_id : 지우려는 영상의 ID
 			u32 i_n_timestamp : 지우려는 영상의 timestamp
 * @return  none
 * @note    
*******************************************************************************/
void CVWSystem::backup_image_delete( u32 i_n_id, u32 i_n_timestamp )
{
    s8  p_cmd[512];
    
    if( i_n_timestamp == 0 )
    {
        sprintf( p_cmd, "rm %s0x%08X*.bin*", SYS_IMAGE_DATA_PATH, i_n_id );
    }
    else
    {
        sprintf( p_cmd, "rm %s0x%08X_0x%08X.bin*", SYS_IMAGE_DATA_PATH, i_n_id, i_n_timestamp );
    }
    
    sys_command( p_cmd );
    sys_command("sync");
    
    print_log(DEBUG, 1, "[%s] delete backup image(%s)\n", m_p_log_id, p_cmd);
    
    image_check_remain_memory();
    
}

/******************************************************************************/
/**
 * @brief   저장된 영상 중 가장 오래된 파일 1개를 삭제한다.
 * @param   none
 * @return  none
 * @note    
*******************************************************************************/
void CVWSystem::backup_image_delete_oldest(void)
{
    file_delete_oldest(SYS_IMAGE_DATA_PATH);
}

/******************************************************************************/
/**
 * @brief   backup 영상을 디텍터 내부에 저장한다.
 * @param   image_header_t i_header_t : 영상 정보 header
 			u8* i_p_data : 영상 데이터
 * @return  true : backup 영상 저장 성공
 			false : backup 영상 저장 실패
 * @note	none    
*******************************************************************************/
bool CVWSystem::backup_image_save( image_header_t i_header_t, u8* i_p_data )
{
    FILE*   p_file;
    s8      p_file_name[256];
	s8      p_date[256];
	u32		n_crc;
	
	backup_image_header_t header_t;
	
	if( m_p_env_c->is_backup_enable() == false )
	{
		print_log(INFO, 1, "[%s] image(id: 0x%08X) is not saved(backup disable).\n", m_p_log_id, i_header_t.n_id);
		return false;
	}
	
	memset( p_date, 0, sizeof(p_date) );
	get_time_string( i_header_t.n_time, p_date );
    
    sprintf( p_file_name, "/tmp/0x%08X_0x%08X.bin", i_header_t.n_id, i_header_t.n_time );
    
    print_log(INFO, 1, "[%s] open a file to save image(file name: %s(date: %s), image id: 0x%08X, size: %d).\n", \
                            m_p_log_id, p_file_name, p_date, i_header_t.n_id, i_header_t.n_total_size);
    
    // make image file with information.
    memset( &header_t, 0, sizeof(backup_image_header_t) );
    
	memcpy( &header_t.information.n_id, &i_header_t, sizeof(image_header_t) );
	
	header_t.information.n_id				= i_header_t.n_id;
	header_t.information.n_width_size		= i_header_t.n_width_size;
	header_t.information.n_height			= i_header_t.n_height;
	header_t.information.n_total_size		= i_header_t.n_total_size;
	header_t.information.n_pos_x			= i_header_t.n_pos_x;
	header_t.information.n_pos_y			= i_header_t.n_pos_y;
	header_t.information.n_pos_z			= i_header_t.n_pos_z;
	header_t.information.c_offset			= i_header_t.c_offset;
	header_t.information.c_defect			= i_header_t.c_defect;
	header_t.information.c_gain				= i_header_t.c_gain;
	header_t.information.c_reserved_osf		= i_header_t.c_reserved_osf;
	header_t.information.w_direction		= i_header_t.w_direction;
		
	header_t.information.n_vcom_swing_time	= m_p_fpga_c->get_vcom_swing_time();
	header_t.information.w_exposure_time_low = m_p_fpga_c->read(eFPGA_REG_EXP_TIME_LOW);
	header_t.information.w_exposure_time_high = m_p_fpga_c->read(eFPGA_REG_EXP_TIME_HIGH);
	
	memcpy( &header_t.information.patient_t, &m_patient_info_t, sizeof(patient_info_t) );
    // write file
    p_file = fopen( p_file_name, "wb" );
    
    if( p_file != NULL )
    {
	    n_crc = make_crc32( i_p_data, i_header_t.n_total_size );
	    n_crc = make_crc32_with_init_crc( header_t.p_data, sizeof(backup_image_header_t), n_crc );
    	
    	fwrite( i_p_data, 1, i_header_t.n_total_size, p_file );
    	fwrite( header_t.p_data, 1, sizeof(backup_image_header_t), p_file );
    	fwrite( &n_crc, 1, sizeof(n_crc), p_file );
        
	    fclose(p_file);
	    
	    file_copy_to_flash(p_file_name, SYS_IMAGE_DATA_PATH);
        print_log(INFO, 1, "[%s] image(%s) is saved.\n", m_p_log_id, p_file_name);
        
        image_check_remain_memory();
        
        sys_command("rm %s", p_file_name);
        
        m_n_last_saved_image_id = i_header_t.n_id;
        
        return true;
    }
    else
    {
    	print_log(ERROR, 1, "[%s] file open is failed.\n", m_p_log_id);
        return false;
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::backup_image_make_list(void)
{
    s8  p_cmd[256];
    
    sprintf( p_cmd, "ls -1 %s0x*.bin* > %s", SYS_IMAGE_DATA_PATH, BACKUP_IMAGE_LIST_FILE );
    sys_command( p_cmd );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::backup_image_find( u32 i_n_image_id, s8* o_p_file_name )
{
    s8  p_cmd[256];
    s8  p_result[256];
    
    memset( p_result, 0, sizeof(p_result) );
    
    sprintf( p_cmd, "ls -1 %s0x%08X_*.bin*", SYS_IMAGE_DATA_PATH, i_n_image_id );
    process_get_result( p_cmd, p_result );
    
    if( strlen( p_result ) > 0 )
    {
        strcpy( o_p_file_name, p_result );
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
bool CVWSystem::backup_image_is_saved( u32 i_n_image_id )
{
	if( i_n_image_id == m_n_last_saved_image_id )
	{
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
void CVWSystem::capture_end(void)
{
    m_p_fpga_c->line_intr_end();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::capture_check_ready_next_exposure(void)
{
	bool b_fpga_ready = m_p_fpga_c->is_ready();
	
    if( b_fpga_ready  == true
        && m_b_readout_started == true
    	&& m_b_offset_cal_started == false
    	&& osf_is_started() == false )
    {
    	set_readout_start(false);
    }
    
    grab_lock();
    if( m_b_image_grab == false )
    {
		if( m_p_cpature_ready_c != NULL )
        {
            if( b_fpga_ready == true
                || m_p_cpature_ready_c->is_expired() == true )
            {
                message_ready_for_exposure();
                safe_delete(m_p_cpature_ready_c);
            }
        }
    }
    grab_unlock();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::capture_check_mode(void)
{
    u16 w_offset_err;
    
    w_offset_err = m_p_fpga_c->line_intr_check();
    
    if( m_b_offset_cal_started == true
        && w_offset_err > 0 )
    {
        m_p_fpga_c->offset_start( false );
    
        //sleep_ex(10000);       // 10 ms
        
        offset_cal_message(eOFFSET_CAL_ACQ_ERROR);
        
        print_log(DEBUG, 1, "[%s] offset calibration is stopped.(0x%04X)\n", m_p_log_id, w_offset_err);
        
        //capture_end();
    }
    
    if( m_p_fpga_c->is_xray_detected_in_extra_flush_section() == true )
    {
        u32 n_state = eIMG_STATE_XRAY_DETECTED_ERROR;
        message_send(eMSG_IMAGE_STATE, (u8*)&n_state, sizeof(n_state));
    }
    
    capture_check_ready_next_exposure();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::capture_is_ready( PM_MODE i_pm_mode_e )
{
    if( m_p_capture_init_c != NULL )
    {
        if( m_p_capture_init_c->is_expired() == false )     //5초 동안 대기
        {
            return false;
        }
        else                                                //5초가 지나야 ready 체크 및 
        {
        	if(is_ready())
            {
            	safe_delete( m_p_capture_init_c );
            
            	capture_ready();
                if(!is_vw_revision_board())
		    	    boot_led_on();
            	m_w_capture_init_count = 0;
            }
            else
            {
            	return false;
            }
        }
    }
    
    if( i_pm_mode_e != ePM_MODE_NORMAL )
    {
        return false;
    }
    
    return capture_is_dev_ready();

}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::capture_is_dev_ready(void)
{
    u16 w_read;
    bool b_result = false;
 
    w_read = m_p_fpga_c->read(eFPGA_REG_XRAY_READY);

    if( w_read == 1 )
    {
    	return true;
    }
    else
    {
        m_w_capture_init_count++;
        print_log(ERROR, 1, "[%s] fpga re-init(%d) start - 0x22: 0x%04X\n", m_p_log_id, m_w_capture_init_count, w_read);

        b_result = fpga_reinit();
        return b_result;
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::capture_init(void)
{
    m_p_capture_init_c = new CTime(5000);
    m_p_cpature_ready_c = NULL;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::set_immediate_transfer( bool i_b_enable )
{
    m_b_immediate_transfer = i_b_enable;
    m_p_fpga_c->set_immediate_transfer(i_b_enable);
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void* button_routine( void * arg )
{
	CVWSystem* sys = (CVWSystem *)arg;
	sys->button_proc();
	pthread_exit( NULL );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CVWSystem::button_proc(void)
{
    CTime*      p_ap_button_timer_c = NULL;
    CTime*      p_pwr_button_timer_c = NULL;
    bool		b_ap_button_event_handled = false;
    
    print_log(DEBUG, 1, "[%s] start button_proc...(%d)\n", \
                        m_p_log_id, syscall( __NR_gettid ) );
     
     
	while( m_b_button_thread_is_running == true )
    {	
    	// 1. POWER button check
        if( m_p_hw_mon_c->pwr_button_get_raw() == true )
        {
        	if(p_pwr_button_timer_c == NULL)
            {
            	p_pwr_button_timer_c = new CTime( 3000 );
            }
            else
            {
           		if( p_pwr_button_timer_c->is_expired() == true )
            	{ 
       				hw_power_off();
       			}
       		}     	
        }
        else
        {
        	if(p_pwr_button_timer_c != NULL)
        	{
        		if( p_pwr_button_timer_c->is_expired() == false )
            	{
            		if( false == pm_is_normal_mode() && true == m_p_oled_c->is_oled_on() )
	            	{
	            		PM_MODE pm_e = ePM_MODE_WAKEUP_BY_PWR_BUTTON;
	            		message_send( eMSG_POWER_MODE, (u8*)&pm_e, sizeof(PM_MODE) );
	            	}
	            	else
	            	{
	            		set_oled_display_event(eBUTTON_EVENT);
	            	}
	            }
	            
	            safe_delete( p_pwr_button_timer_c );
            }
        }    	
    	
    	// 2. AP button check
        if( m_p_hw_mon_c->ap_button_get_raw() == true )
        {
        	if(p_ap_button_timer_c == NULL)
            {
            	p_ap_button_timer_c = new CTime( 3000 );
            }
            else
            {
            	if(p_ap_button_timer_c->is_expired() == true && b_ap_button_event_handled == false)
            	{
	       			activate_ap_button_function();
	       			/* AP button이 6초 이상 계속해서 눌린 경우 Station -> AP -> Station 되돌아 가는 현상 방지
	       			   AP button event가 한번 처리되면, 이후에 계속 AP button이 눌려있는 경우에는 처리하지 않도록 함
	       			   AP button을 떼었을 때 b_ap_button_event_handled flag 초기화 */
	       			b_ap_button_event_handled = true;
	       		}
	       	}
	    }
	    else          		
		{
			if(p_ap_button_timer_c != NULL)
			{
				if(p_ap_button_timer_c->is_expired() == false)	
				{	
	            	if( false == pm_is_normal_mode() && true == m_p_oled_c->is_oled_on() )
	            	{
	            		PM_MODE pm_e = ePM_MODE_WAKEUP_BY_PWR_BUTTON;
	            		message_send( eMSG_POWER_MODE, (u8*)&pm_e, sizeof(PM_MODE) );
	            	}
	            	else
	            	{
	            		set_oled_display_event(eBUTTON_EVENT);
	            	}
	            }
	            else
	            {
	            	b_ap_button_event_handled = false;
	            }
	            
	            safe_delete( p_ap_button_timer_c );
	        }
        }
        
        sleep_ex(100000);		// 100ms
        
	}
	
    safe_delete( p_ap_button_timer_c );
    safe_delete( p_pwr_button_timer_c );
    
    print_log(DEBUG, 1, "[%s] stop button_proc...\n", m_p_log_id );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void* monitor_routine( void * arg )
{
	CVWSystem* sys = (CVWSystem *)arg;
	sys->monitor_proc();
	pthread_exit( NULL );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::monitor_print( bool i_b_start, u32 i_n_time_sec )
{
    m_b_mon_print = i_b_start;
    
    if( i_n_time_sec > 0 )
    {
        m_w_mon_time_sec = i_n_time_sec;
    }
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CVWSystem::monitor_proc(void)
{
    CTime*      p_mon_time_c = NULL;
    CTime*      p_state_time_c = NULL;
    CTime*      p_xray_time_c = NULL;
    u16         w_mon_time_sec = m_w_mon_time_sec;
    
    print_log(DEBUG, 1, "[%s] start monitor_proc...(%d)\n", \
                        m_p_log_id, syscall( __NR_gettid ) );
    
    memset( &m_sys_state_t, 0, sizeof(sys_state_t) );
    get_state( &m_sys_state_t, true );
    update_oled_state(&m_sys_state_t);
    
    //monitor_init();
    
    p_state_time_c	= new CTime( 1000 );
    p_xray_time_c	= new CTime( 5000 );
    
    while( m_b_mon_is_running == true )
    {
        if( p_state_time_c->is_expired() == true )
        {
            safe_delete( p_state_time_c );
            p_state_time_c = new CTime( 1000 );
            
            get_state( &m_sys_state_t, true );
            update_oled_state(&m_sys_state_t);
        }

        if( p_xray_time_c->is_expired() == true )
        {
            safe_delete( p_xray_time_c );
            p_xray_time_c = new CTime( 5000 );
            
            monitor_xray_detection();
        }
        
        if( p_mon_time_c == NULL
            || p_mon_time_c->is_expired() == true )
        {
            safe_delete( p_mon_time_c );
            p_mon_time_c = new CTime( w_mon_time_sec * 1000 );
            
            monitor_log(m_sys_state_t);
        }
        
        if( w_mon_time_sec != m_w_mon_time_sec )
        {
            w_mon_time_sec = m_w_mon_time_sec;
            safe_delete( p_mon_time_c );
        }
        
        sleep_ex(500000);       // 500ms
    }
    
    safe_delete( p_state_time_c );
    safe_delete( p_xray_time_c );
    safe_delete( p_mon_time_c );
    
    print_log(DEBUG, 1, "[%s] stop monitor_proc...\n", m_p_log_id );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CVWSystem::monitor_init(void)
{
    sys_command("mkdir %s", SYS_MONITOR_LOG_PATH);
    
    if( file_get_count( SYS_MONITOR_LOG_PATH, "*.csv" ) > 30 )
    {
        file_delete_oldest( SYS_MONITOR_LOG_PATH );
    }
    
    m_w_mon_time_sec = 60;
    m_b_mon_print = false;
    
    m_n_mon_image_count     = 0;
    m_n_mon_offset_count    = 0;
    
    FILE*       p_log_file;
        
    p_log_file = monitor_get_file();
    
    if( p_log_file == NULL )
    {
        print_log(DEBUG, 1, "[%s] mlog file open error(%s)\n", m_p_log_id, m_p_mon_file_name );
    }
    
    fprintf( p_log_file, "\n" );
    fclose( p_log_file );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::monitor_log( sys_state_t i_stat_t )
{
    s8              p_log[1024];
    s8              p_time_string[128];
    
    FILE*           p_log_file;
    u32             n_link;
    battery_t       bat_t[eBATTERY_MAX];
    u16				w_sleep;
    
    memset( bat_t, 0, sizeof(battery_t) * eBATTERY_MAX);
    
    memset( p_time_string, 0, sizeof(p_time_string) );
    
    p_log_file = monitor_get_file();
    
    if( p_log_file == NULL )
    {
        print_log(DEBUG, 1, "[%s] mlog file open error(%s)\n", m_p_log_id, m_p_mon_file_name );
        return;
    }
    
    monitor_get_time( p_time_string );
    
    n_link = 0;
    if( m_current_net_mode_e == eNET_MODE_STATION )
    {
        n_link = i_stat_t.w_link_quality;
        
    }
    m_p_hw_mon_c->get_battery(bat_t);
    
    if( i_stat_t.w_power_mode == ePM_MODE_NORMAL )
    {
    	w_sleep = 0;
    }
    else
    {
    	w_sleep = 1;
    }
    
	sprintf( p_log, "%s, B_ID, %2.2f, %2.2f, B_THM, %2.2f, %2.2f, AED_TRIG, %2.2f, %2.2f, VBATT, %2.2f, VS, %2.2f, VCELL, %2.2f, %2.2f, Remain, %2.1f, %2.1f, realSOC, %2.1f, %2.1f, PW, %d, %d, PO, %d, %d, image, %d, temp, %2.1f, %2.1f, Sleep, %d, WIFI, %d, MAC, %s, EQUIP, %d, %d, PWR SRC, %d", \
                    p_time_string, \
                    i_stat_t.p_hw_power_volt[POWER_VOLT_B_ID_A]/100.0f, \
                    i_stat_t.p_hw_power_volt[POWER_VOLT_B_ID_B]/100.0f, \
                    i_stat_t.p_hw_power_volt[POWER_VOLT_B_THM_A]/100.0f, \
                    i_stat_t.p_hw_power_volt[POWER_VOLT_B_THM_B]/100.0f, \
                    i_stat_t.p_hw_power_volt[POWER_VOLT_AED_TRIG_A]/100.0f, \
                    i_stat_t.p_hw_power_volt[POWER_VOLT_AED_TRIG_B]/100.0f, \
                    i_stat_t.p_hw_power_volt[POWER_VOLT_VBATT]/100.0f, \
                    i_stat_t.p_hw_power_volt[POWER_VOLT_VS]/100.0f, \
                    bat_t[eBATTERY_0].w_vcell/100.0f, \
                    bat_t[eBATTERY_1].w_vcell/100.0f, \
                    bat_t[eBATTERY_0].w_soc/10.0f, \
                    bat_t[eBATTERY_1].w_soc/10.0f, \
                    bat_t[eBATTERY_0].w_soc_raw/10.0f, \
                    bat_t[eBATTERY_1].w_soc_raw/10.0f, \
                    bat_t[eBATTERY_0].b_power_warning, bat_t[eBATTERY_1].b_power_warning, \
                    bat_t[eBATTERY_0].b_power_off, bat_t[eBATTERY_1].b_power_off, \
                    m_n_mon_image_count, i_stat_t.w_temperature/10.0f, \
                    m_p_hw_mon_c->get_temperature()/10.0f, w_sleep, \
                    n_link, i_stat_t.p_ap_mac, bat_t[eBATTERY_0].b_equip, bat_t[eBATTERY_1].b_equip ,i_stat_t.w_power_source );

    if( m_b_mon_print == true )
    {
        printf( "%s\n", p_log );
    }
    
    fprintf( p_log_file, "%s\n", p_log );
    fflush( p_log_file );
    fclose( p_log_file );
    
    sys_command("sync");
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::monitor_get_time( s8* o_p_time_string )
{
    u32 n_time = time(NULL);
    struct tm*  p_gmt_tm_t;
        
    p_gmt_tm_t = localtime( (time_t*)&n_time );
    
    sprintf( o_p_time_string, "%04d/%02d/%02d-%02d:%02d:%02d", \
                        p_gmt_tm_t->tm_year+1900, p_gmt_tm_t->tm_mon+1, p_gmt_tm_t->tm_mday, \
                        p_gmt_tm_t->tm_hour, p_gmt_tm_t->tm_min, p_gmt_tm_t->tm_sec );    
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
FILE* CVWSystem::monitor_get_file(void)
{
    u32 n_time = time(NULL);
    struct tm*  p_gmt_tm_t;
        
    p_gmt_tm_t = localtime( (time_t*)&n_time );
    
    sprintf( m_p_mon_file_name, "%s%04d_%02d_%02d.csv", SYS_MONITOR_LOG_PATH, \
                        p_gmt_tm_t->tm_year+1900, p_gmt_tm_t->tm_mon+1, p_gmt_tm_t->tm_mday );

	return fopen( m_p_mon_file_name, "a+" );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CVWSystem::monitor_proc_start(void)
{
    monitor_init();
    
    m_b_mon_is_running = true;
    
	if( pthread_create(&m_mon_thread_t, NULL, monitor_routine, ( void * ) this)!= 0 )
    {
    	print_log( ERROR, 1, "[%s] monitor_routine pthread_create:%s\n", \
    	                    m_p_log_id, strerror( errno ));

    	m_b_mon_is_running = false;
    }
    
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CVWSystem::monitor_proc_stop(void)
{
    if( m_b_mon_is_running )
    {
        m_b_mon_is_running = false;
    	
    	if( pthread_join( m_mon_thread_t, NULL ) != 0 )
    	{
    		print_log( ERROR, 1,"[%s] pthread_join: monitor_proc:%s\n", \
    		                    m_p_log_id, strerror( errno ));
    	}
    }
}


/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CVWSystem::button_proc_start(void)
{  
    m_b_button_thread_is_running = true;
    
	if( pthread_create(&m_button_thread_t, NULL, button_routine, ( void * ) this)!= 0 )
    {
    	print_log( ERROR, 1, "[%s] button_routine pthread_create:%s\n", \
    	                    m_p_log_id, strerror( errno ));

    	m_b_button_thread_is_running = false;
    }  
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CVWSystem::button_proc_stop(void)
{
    if( m_b_button_thread_is_running )
    {
        m_b_button_thread_is_running = false;
    	
    	if( pthread_join( m_button_thread_t, NULL ) != 0 )
    	{
    		print_log( ERROR, 1,"[%s] pthread_join: button_proc:%s\n", \
    		                    m_p_log_id, strerror( errno ));
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
void CVWSystem::monitor_xray_detection(void)
{
	if( m_p_fpga_c->trigger_get_type() == eTRIG_SELECT_ACTIVE_LINE )
	{
		u16 w_stb_time	= m_p_fpga_c->read(eFPGA_REG_SET_DEB_AED_STB_TIME);

		if( w_stb_time > 0 )
		{
			float f_threshold_value = w_stb_time * 0.9f;
			u16 w_stb_count = m_p_fpga_c->read(eFPGA_REG_DEB_AED_STB_CNT);
			
			if( w_stb_count > (u16)f_threshold_value )
			{
    			print_log( INFO, 1,"[%s] x-ray is detected: %d\n", \
    		                    m_p_log_id, w_stb_count);
			}
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
void CVWSystem::set_sw_connection( bool i_b_connected )
{
    m_b_sw_connected = i_b_connected;
    
    if( i_b_connected == true )
    {
    	pm_reset_sleep_timer();
        m_p_fpga_c->set_tg_time();
        
        m_p_hw_mon_c->clear_impact_count();
    }
    else
    {
        pm_set_keep_normal( false );
        /* Multiple detector open 상태에서 선택되지 않은 detector는 AED trigger에 의한 
           촬영을 block 하도록 수정됨 - 2020.01.06 SDK 1.0.18.2 이상부터 적용
           이에 따라, SW 연결 끊어지면 AED trigger block 했던 것을 해제하여 stand-alone 모드로 동작 시 문제가 없도록 해야 함 */
        m_p_fpga_c->disable_aed_trig_block();
        /* SW 접속 시 LED blinking 하도록 하는 설정을 사용 중인 경우,
           SW연결이 끊어지면 LED가 on 상태로 유지하도록 함, 해당 설정을 사용하지 않는다면
           기존에도 stay on 상태이기 때문에 아래와 같이 설정하는 것이 영향을 주지 않음 */
        pm_power_led_ctrl(eSTAY_ON);
        if( m_b_osf_is_running == true )
        {
            osf_message(eOFFSET_DRIFT_FAILED);
        }
    }

    if( test_image_is_enabled() == true )
    {
    	print_log( DEBUG, 1, "[%s] test image is enable(%d) --> disable\n", m_p_log_id, m_b_test_image );
    	test_image_enable(false);
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::auto_offset_set( bool i_b_set )
{
    m_b_auto_offset = i_b_set;
    m_w_old_temp    = 0;
    
    if( m_b_auto_offset == false )
    {
        safe_delete( m_p_auto_offset_time_c );
        print_log(DEBUG, 1, "[%s] auto offset disable\n", m_p_log_id);
        return;
    }
    
    m_b_auto_offset = m_p_env_c->load_auto_offset( &m_auto_offset_t );
    
    if( m_b_auto_offset == true )
    {
        safe_delete( m_p_auto_offset_time_c );
        m_p_auto_offset_time_c = new CTime(m_auto_offset_t.w_period_time * 1000 * 60);

        m_w_old_temp = m_p_hw_mon_c->get_temperature();
        
        m_p_fpga_c->offset_set_recapture(m_auto_offset_t.b_aed_capture);
        
        print_log(DEBUG, 1, "[%s] auto offset current temp: %2.1f, wait time: %d min\n", \
                        m_p_log_id, m_w_old_temp/10.0f, m_auto_offset_t.w_period_time);
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::auto_offset_check(void)
{
    u16         w_temp_difference;
    u16         w_current_temp;
    
    if( m_p_auto_offset_time_c == NULL
        || m_p_auto_offset_time_c->is_expired() == false
        || m_b_offset_cal_started == true )
    {
        return;
    }
    
    // check exposure time
    if( m_p_fpga_c->read(eFPGA_REG_DEB_AED_STB_CNT) > 0 )
    {
        return;
    }
    
    // check temperature difference
    w_current_temp = m_p_hw_mon_c->get_temperature();
    
    if( w_current_temp >= m_w_old_temp )
    {
        w_temp_difference = w_current_temp - m_w_old_temp;
    }
    else
    {
        w_temp_difference = m_w_old_temp - w_current_temp;
    }
    
    w_temp_difference = w_temp_difference/10;
    
    if( w_temp_difference >= m_auto_offset_t.w_temp_difference )
    {
        m_w_old_temp = w_current_temp;
        
        print_log(DEBUG, 1, "[%s] auto offset updated temp: %d\n", m_p_log_id, m_w_old_temp);
        
        offset_cal_request(m_auto_offset_t.w_count, 1);
    }
    
    safe_delete(m_p_auto_offset_time_c);
    m_p_auto_offset_time_c = new CTime(m_auto_offset_t.w_period_time * 1000 * 60);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::start(void)
{
    m_b_ready = true;
    
    print_state();
    dev_reg_print(eDEV_ID_ADC);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::grf_reset(void)
{
    m_w_grf_panel_bias     = m_p_fpga_c->read(eFPGA_REG_PANEL_BIAS);
    m_w_grf_extra_flush    = m_p_fpga_c->read(eFPGA_REG_EXTRA_FLUSH); 
    print_log(DEBUG, 1, "[%s] GRF reset: panel bias %d -> 0, extra flush %d -> 0\n", \
                        m_p_log_id, m_w_grf_panel_bias, m_w_grf_extra_flush);
    
    m_p_fpga_c->write(eFPGA_REG_PANEL_BIAS, 0);
    m_p_fpga_c->write(eFPGA_REG_EXTRA_FLUSH, 0);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::grf_restore(void)
{
    m_p_fpga_c->write(eFPGA_REG_PANEL_BIAS, m_w_grf_panel_bias);
    m_p_fpga_c->write(eFPGA_REG_EXTRA_FLUSH, m_w_grf_extra_flush);

    print_log(DEBUG, 1, "[%s] GRF restore: panel bias %d, extra flush %d\n", \
                            m_p_log_id, m_w_grf_panel_bias, m_w_grf_extra_flush);
}

/******************************************************************************/
/**
 * @brief	preview기능이 활성화 되어 있는지 확인한다.
 * @param  	none
 * @return  true : preview 기능이 활성화 되어 있다.
 			false : preview 기능이 비활성화 되어 있다.
 * @note    
*******************************************************************************/
bool CVWSystem::preview_get_enable(void)
{
	// 유선 연결 모드이면 preview 기능을 무조건 비활성화 한다.
    if( m_current_net_mode_e == eNET_MODE_TETHER )
    {
        return false;
    }
    
    // FTM 을 사용하면 preview기능을 무조건 활성화 한다.
    if( m_b_tact_enable == true )
    {
        return true;
    }
    
    // 유선 연결이 아닌경우, 사용자가 preview 기능을 사용한다고 설정했으면 preview 기능을 활성화 한다.
    if( m_b_preview_enable == true )
    {
        
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
void CVWSystem::set_image_state( u32 i_n_state )
{
    if( i_n_state == eIMG_STATE_EXPOSURE )
    {
		grab_lock();
        
		m_b_image_grab = true;
         
        if( m_p_cpature_ready_c != NULL )
        {
        	message_ready_for_exposure();
			safe_delete(m_p_cpature_ready_c);
		}
		m_p_cpature_ready_c = new CTime( 10000 + get_exp_wait_time() );
        
        grab_unlock();
        
        set_readout_start( true );
        
        m_p_fpga_c->exposure_aed();
        
        m_p_fpga_c->exposure_request_stop();
    }
    else if( i_n_state == eIMG_STATE_EXPOSURE_TIMEOUT
            || i_n_state == eIMG_STATE_ACQ_TIMEOUT
            || i_n_state == eIMG_STATE_SEND_END
            || i_n_state == eIMG_STATE_SEND_TIMEOUT
            || i_n_state == eIMG_STATE_SAVED
            || i_n_state == eIMG_STATE_SAVE_FAILED
            || i_n_state == eIMG_STATE_ACQ_END_WITH_ERROR )
    {
    	grab_lock();
        m_b_image_grab = false;
        grab_unlock();
        
        /*soc가 100%가 아니라면()*/
        restore_power_config();

		impact_stable_proc_restart();
    }
    else if( i_n_state == eIMG_STATE_READY_FOR_EXPOSURE )
    {      
        exposure_request_enable();
        
        print_log(DEBUG, 1, "[%s] next exp_req is enable\n", m_p_log_id);
    }
    
    if( i_n_state == eIMG_STATE_ACQ_END
        || i_n_state == eIMG_STATE_ACQ_END_WITH_ERROR )
    {
        m_p_fpga_c->print_trigger_time();
    }
    
    pm_reset_sleep_timer();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::hw_bat_charge(BAT_CHARGE i_e_charge_state)
{	
    if( m_b_readout_started == true )
    {
        return;
    }
    
    if( i_e_charge_state != eBAT_CHARGE_0_OFF_1_OFF
    	&& net_get_mode() == eNET_MODE_TETHER )
    {
    	if( net_is_valid_speed() == false )
	    {
	    	return;
	    }
	    
	    charge_time_lock();
	    if( m_p_charge_time_c != NULL )
	    {
	    	if( m_p_charge_time_c->is_expired() == false )
		   	{
		   		charge_time_unlock();
		    	return;
			}
			else
			{
				safe_delete(m_p_charge_time_c);
				print_log(DEBUG, 1, "[%s] bat charge timer end\n", m_p_log_id);
			}
		}
		charge_time_unlock();
	}
    
    m_p_hw_mon_c->bat_charge(i_e_charge_state);
    
    //display_update();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::set_readout_start(bool i_b_started)
{
    m_b_readout_started = i_b_started;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::oper_info_reset(void)
{
    m_p_env_c->oper_info_reset();
    m_p_fpga_c->set_capture_count(m_p_env_c->oper_info_get_capture_count());
}

/******************************************************************************/
/**
 * @brief   AED 모드에서 에러 영상인지 여부를 확인한다.
 * @param   none
 * @return  u16 w_error
 			  1 : Extra flush와 flush 구간에 걸쳐 들어온 AED trigger에 의해 촬영된 영상
 			  0 : 위의 경우가 아닌 경우
 * @note    AED 모드에서 Extra flush 구간 중에 들어온 X-ray 신호는 무시되지만
 			X-ray가 Extra flush 구간과 flush 구간에 걸친 경우 촬영이 진행된다.
 			이 경우 gate 단위로 artifact 가 있는 영상을 획득할 수 있는 문제가 있다.
*******************************************************************************/
u16 CVWSystem::get_image_error(void)
{
	u16 w_error = m_p_fpga_c->read(eFPGA_REG_IMAGE_ERROR);
    
    if( w_error > 0 )
    {
        print_log(DEBUG, 1, "[%s] image error: 0x%04X\n", m_p_log_id, w_error);
        
        m_p_fpga_c->write(eFPGA_REG_IMAGE_ERROR, 0);
        m_p_fpga_c->write(eFPGA_REG_IMAGE_ERROR_EXTRA_FLUSH, 0);
    }
    
    return w_error;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void* osf_routine( void * arg )
{
	CVWSystem* sys = (CVWSystem *)arg;
	sys->osf_proc();
	pthread_exit( NULL );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CVWSystem::osf_proc(void)
{
    print_log(DEBUG, 1, "[%s] start osf_proc...(%d)\n", m_p_log_id, syscall( __NR_gettid ) );
    
    CTime*  p_wait_c    = NULL;
    bool    b_result    = true;
    
    osf_backup();
    
    while( m_b_osf_is_running == true )
    {
        if( m_w_osf_trigger_count >= m_w_osf_trigger_total )
        {
            break;
        }
        
        if( m_w_osf_trigger_count == 0 )
        {
            // trigger with vcom
            m_w_osf_trigger_count++;
            
            m_p_fpga_c->write( eFPGA_REG_VCOM_MIN_CNT, OSF_FIRST_TRIGGER_TIME_SEC );
            
            b_result = osf_trigger( true );
            print_log(DEBUG, 1, "[%s] vcom swing time: %d\n", m_p_log_id, m_p_fpga_c->get_vcom_swing_time());
        }
        else if( m_w_osf_trigger_count == 1 )
        {
            if( osf_check_vcom() == true )
            {
                m_w_osf_trigger_count++;
                
                safe_delete( p_wait_c );
                p_wait_c = new CTime(m_w_osf_trigger_time * 1000);
                
                b_result = osf_trigger( false );
            }
        }
        else
        {
            if( p_wait_c != NULL
                && p_wait_c->is_expired() == true )
            {
                safe_delete( p_wait_c );
                p_wait_c = new CTime(m_w_osf_trigger_time * 1000);
                
                m_w_osf_trigger_count++;
                // trigger without vcom
                b_result = osf_trigger( false );
            }
        }
        
        if( b_result == false )
        {
            print_log(DEBUG, 1, "[%s] failed to osf trigger(%d)\n", m_p_log_id, m_w_osf_trigger_count );
            osf_message(eOFFSET_DRIFT_FAILED);
            break;
        }
        
        sleep_ex(1000);
    }
    
    safe_delete( p_wait_c );
    
	if( b_result == true )
	{
		p_wait_c = new CTime(get_exp_wait_time() + 3000);
    
	    // osf data가 생성될 때까지 기다린다.
	    // m_b_osf_is_running가 false여도 
	    //m_p_osf_data 메모리 사용을 osf_accumulate에서 다 할 때까지 기다려야 한다.
	    while( p_wait_c->is_expired() == false )
	    {
	        sleep_ex(100000);
	    }
		
		safe_delete( p_wait_c );
	}
    
    osf_restore();
    
    print_log(DEBUG, 1, "[%s] stop osf_proc...(%d)\n", m_p_log_id, m_w_osf_trigger_count );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CVWSystem::osf_start( u16 i_w_count, u16 i_w_time )
{
    if( i_w_count == 0
        || i_w_time < 2 )
    {
        print_log(DEBUG, 1, "[%s] invalid osf settings(%d, %d)\n", m_p_log_id, i_w_count, i_w_time );
        return false;
    }
    
    if( m_b_osf_is_running == true )
    {
        print_log( ERROR, 1, "[%s] osf is already started.\n", m_p_log_id);
        osf_stop();
        
        return false;
    }
    
    m_w_osf_trigger_total   = i_w_count + 1;
    m_w_osf_trigger_time    = i_w_time;
    m_w_osf_trigger_count   = 0;
    m_n_osf_line_count      = 0;
    m_b_osf_vcom_started    = false;
    
    u32 n_data_size         = sizeof(float) * i_w_count * m_image_size_t.height;
    
    printf("osf data size: %d\n", n_data_size );
    m_p_osf_data            = (float*)malloc( n_data_size );
    
    memset( m_p_osf_data, 0, n_data_size );
    
    print_log(DEBUG, 1, "[%s] osf settings(count: %d, %ds)\n", m_p_log_id, i_w_count, i_w_time );
    
    // launch capture thread
    m_b_osf_is_running = true;
    if( pthread_create(&m_osf_thread_t, NULL, osf_routine, ( void * ) this)!= 0 )
    {
    	print_log( ERROR, 1, "[%s] osf pthread_create:%s\n", m_p_log_id, strerror( errno ));
    	m_b_osf_is_running = false;
    	
    	return false;
    }

    return true;    
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CVWSystem::osf_stop(void)
{
    if( m_b_osf_is_running == false )
    {
        print_log( ERROR, 1, "[%s] osf_proc is already stopped.\n", m_p_log_id);
        return;
    }
    
    m_b_osf_is_running = false;
	if( pthread_join( m_osf_thread_t, NULL ) != 0 )
	{
		print_log( ERROR, 1,"[%s] osf pthread_join: update_proc:%s\n", \
		                    m_p_log_id, strerror( errno ));
	}

    safe_free(m_p_osf_data);
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CVWSystem::osf_message( OFFSET_DRIFT_STATE i_state_e, u8* i_p_data, u16 i_w_data_len )
{
    u32 n_state;
    vw_msg_t msg;
    bool b_result;
    
    if( i_w_data_len + sizeof( n_state ) > VW_PARAM_MAX_LEN )
    {
        print_log(ERROR, 1, "[%s] osf_message data lenth is too long(%d/%d).\n", \
                            m_p_log_id, i_w_data_len, VW_PARAM_MAX_LEN );
        return;
    }
    
    n_state = i_state_e;
    
    memset( &msg, 0, sizeof(vw_msg_t) );
    msg.type = (u16)eMSG_OFFSET_DRIFT_STATE;
    msg.size = sizeof(n_state) + i_w_data_len;
    
    memcpy( &msg.contents[0], &n_state, sizeof(n_state) );
    if( i_w_data_len > 0 )
    {
        memcpy( &msg.contents[sizeof(n_state)], i_p_data, i_w_data_len );
    }
    
    b_result = notify_handler( &msg );
    
    if( b_result == false )
    {
        print_log(ERROR, 1, "[%s] osf_message(%d) failed\n", m_p_log_id, i_state_e );
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::osf_backup(void)
{
    m_w_osf_saved_vcom              = m_p_fpga_c->read(eFPGA_REG_PANEL_BIAS);
    m_w_osf_saved_extra_flush       = m_p_fpga_c->read(eFPGA_REG_EXTRA_FLUSH);
    m_w_osf_saved_trigger_type      = m_p_fpga_c->trigger_get_type();
    m_w_osf_saved_digital_offset    = m_p_fpga_c->read(eFPGA_REG_DIGITAL_OFFSET);
    m_w_osf_saved_vcom_min          = m_p_fpga_c->read( eFPGA_REG_VCOM_MIN_CNT );
    
    m_b_osf_saved_gain				= m_p_fpga_c->gain_get_enable();
    m_b_osf_saved_defect			= defect_get_enable();
    m_p_fpga_c->gain_set_enable(false);
    defect_set_enable(false);
    
    //AED Power off 조건 강제 발생
    m_p_fpga_c->trigger_set_type(eTRIG_SELECT_ACTIVE_LINE);
    aed_power_control(eGEN_INTERFACE_RAD, eTRIG_SELECT_ACTIVE_LINE, get_smart_w_onoff());

    m_p_fpga_c->write(eFPGA_REG_DIGITAL_OFFSET, 0);
    m_p_fpga_c->set_capture_count(0);
    
    set_readout_start( true );
    
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CVWSystem::get_digital_offset(void)
{
	return m_p_fpga_c->read(eFPGA_REG_DIGITAL_OFFSET);
}
	
/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::osf_restore(void)
{
    m_p_fpga_c->write(eFPGA_REG_PANEL_BIAS, m_w_osf_saved_vcom);
    m_p_fpga_c->write(eFPGA_REG_EXTRA_FLUSH, m_w_osf_saved_extra_flush);
    m_p_fpga_c->write(eFPGA_REG_DIGITAL_OFFSET, m_w_osf_saved_digital_offset);
    m_p_fpga_c->write( eFPGA_REG_VCOM_MIN_CNT, m_w_osf_saved_vcom_min );

    m_p_fpga_c->trigger_set_type(eTRIG_SELECT_ACTIVE_LINE);
    aed_power_control(get_acq_mode(), (TRIGGER_SELECT)m_w_osf_saved_trigger_type, get_smart_w_onoff());
    m_p_fpga_c->trigger_set_type(m_w_osf_saved_trigger_type);
    
    m_p_fpga_c->gain_set_enable(m_b_osf_saved_gain);
    defect_set_enable(m_b_osf_saved_defect);
    
    //m_p_fpga_c->get_capture_count(0);
    m_p_fpga_c->set_capture_count(m_p_env_c->oper_info_get_capture_count());
    
    set_readout_start( false );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::osf_trigger( bool i_b_vcom )
{
    if( i_b_vcom == false )
    {
        m_p_fpga_c->write(eFPGA_REG_PANEL_BIAS, 0);
        m_p_fpga_c->write(eFPGA_REG_EXTRA_FLUSH, 0);
    }
    else
    {
        m_p_fpga_c->write(eFPGA_REG_PANEL_BIAS, m_w_osf_saved_vcom);
        m_p_fpga_c->write(eFPGA_REG_EXTRA_FLUSH, m_w_osf_saved_vcom);
    }
    
    return m_p_fpga_c->exposure_request();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::osf_check_vcom(void)
{
    if( m_b_osf_vcom_started == true
        && m_p_fpga_c->get_vcom_remain() == 0 )
    {
        return true;
    }
    
    return false;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CVWSystem::osf_accumulate( u16* i_p_data, u32 i_n_line_byte_len )
{
    u32 i;
    u32 n_line_pixel_len        = i_n_line_byte_len/(m_image_size_t.pixel_depth);
    u32 n_line_sum;
    u32 n_index;
    
    if( m_b_osf_is_running == false )
    {
        return true;
    }
    
    if( m_w_osf_trigger_count == 1 )
    {
        m_n_osf_line_count++;
        if( get_immediate_transfer() == false && m_b_osf_vcom_started == false )
        {
            m_b_osf_vcom_started = true;
        }
        if( m_n_osf_line_count == m_image_size_t.height )
        {
            print_log(INFO, 1, "[%s] osf acqusition skip\n", m_p_log_id);
            
            if( get_immediate_transfer() == true && m_b_osf_vcom_started == false )
        	{
            	m_b_osf_vcom_started = true;
	        }
            m_n_osf_line_count = 0;
            
            capture_end();
            
        }
        return false;
    }
    
    // accumulate offset data
    n_line_sum = 0;
    n_index = m_image_size_t.width * m_n_osf_line_count;
    for( i = 0; i < n_line_pixel_len; i++ )
    {
		n_line_sum += i_p_data[i];
    }
    
    n_index = (m_w_osf_trigger_count - 2) * m_image_size_t.height + m_n_osf_line_count;
    
    m_p_osf_data[n_index] = (float)n_line_sum/(float)n_line_pixel_len;
    
    m_n_osf_line_count++;
    
    if( m_n_osf_line_count == m_image_size_t.height )
    {
        u32 n_count = m_w_osf_trigger_count - 1;
        
        m_n_osf_line_count = 0;

        capture_end();
        
        if( m_w_osf_trigger_count >= m_w_osf_trigger_total )
        {
            print_log(INFO, 1, "[%s] osf acquisition end(%d)\n", m_p_log_id, n_count);
            
            osf_save();
            
            osf_message( eOFFSET_DRIFT_ACQ_COMPLETE, (u8*)&n_count, sizeof(n_count) );
            
            return true;
        }
        else
        {
            print_log(INFO, 1, "[%s] osf acquisition (acq: %d)\n", m_p_log_id, n_count);
            osf_message( eOFFSET_DRIFT_ACQ_COUNT, (u8*)&n_count, sizeof(n_count) );
        }
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
void CVWSystem::osf_save(void)
{
    u16     i;
    u32     n_line;
    FILE*   p_file;
    s8      p_file_name[256];
    
    sprintf( p_file_name, "/tmp/osf_profile.csv" );
    
    p_file = fopen(p_file_name, "w");
    
    // make first line(time info)
    fprintf( p_file, ", %d", OSF_FIRST_TRIGGER_TIME_SEC );
    for( i = 1; i <= (m_w_osf_trigger_total - 1); i++ )
    {
        fprintf( p_file, ", %d", OSF_FIRST_TRIGGER_TIME_SEC + (i * m_w_osf_trigger_time) );
    }
    fprintf( p_file, "\n" );
    
    for( n_line = 0; n_line < m_image_size_t.height; n_line++ )
    {
        fprintf( p_file, "%d, ", n_line);
        for( i = 0; i < (m_w_osf_trigger_total - 1); i++ )
        {
            fprintf( p_file, "%.02f, ", m_p_osf_data[(i * m_image_size_t.height) + n_line] );
        }
        fprintf( p_file, "\n");
    }
    
    fclose( p_file );
    
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::tact_get_enable(void)
{
    if( m_current_net_mode_e == eNET_MODE_TETHER )
    {
        return false;
    }
    
    return m_b_tact_enable;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::tact_set_enable(bool i_b_enable)
{
    m_b_tact_enable = i_b_enable;
    
    if( m_b_tact_enable == true )
    {
        m_w_tact_id = tact_get_last_id();
        print_log(DEBUG, 1, "[%s] current FTM id: %d\n", m_p_log_id, m_w_tact_id );
    }
}
/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::image_direction_set(void)
{
	m_n_img_direction = atoi(config_get(eCFG_IMAGE_DIRECTION));
	if( atoi(config_get(eCFG_IMAGE_DIRECTION_USER_INPUT)) > 0 )
	{
		m_b_img_direction_user_input = true;
	}
	else
	{
		m_b_img_direction_user_input = false;
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CVWSystem::tact_get_last_id(void)
{
    s32 n_result;
    s8  p_buffer[32];
    s8  p_command[256];
    
    memset( p_buffer, 0, sizeof(p_buffer) );
    sprintf( p_command, "ls %s*.ftm | awk 'END{print}' | awk 'BEGIN{FS=\"_\"}{print $1}' | awk 'BEGIN{FS=\"/\"}{print $NF}'", SYS_IMAGE_DATA_PATH);
    n_result = process_get_result(p_command, p_buffer);
    
    if( n_result > 0 )
    {
        return (u16)strtoul(p_buffer, NULL, 16);
    }
    
    return 0;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CVWSystem::tact_get_image_count(void)
{
    return file_get_count(SYS_IMAGE_DATA_PATH, "*.ftm");
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::tact_delete_image( u32 i_n_id, u32 i_n_timestamp )
{
    s8  p_cmd[512];
    
    if( i_n_timestamp == 0 )
    {
        sprintf( p_cmd, "rm %s0x*_0x%08X_*.ftm", SYS_IMAGE_DATA_PATH, i_n_id );
    }
    else
    {
        sprintf( p_cmd, "rm %s0x*_0x%08X_0x%08X.ftm", SYS_IMAGE_DATA_PATH, i_n_id, i_n_timestamp );
    }
    
    sys_command( p_cmd );
    sys_command("sync");
    
    print_log(DEBUG, 1, "[%s] delete ftm image(%s)\n", m_p_log_id, p_cmd);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::tact_save_image( image_header_t i_header_t, u8* i_p_data, bool i_b_last_half )
{
    FILE*   p_file;
    s8      p_file_name[256];
	s8      p_date[256];

	u8*     p_image_info;
	u32     n_image_info_len;
	tact_image_header_t		tact_header_t;
	
    if( i_p_data == NULL )
    {
        print_log(INFO, 1, "[%s] invalid image data(size: %d).\n", \
                            m_p_log_id, i_header_t.n_total_size);
        return false;
    }
    
    if( i_b_last_half == true )
    {
    	// 영상 사이즈 + 영상 이외 부가정보 저장을 위한 여분 1KB (image header + vcom time + exp time )
        n_image_info_len = i_header_t.n_total_size + 1024;
    }
    else
    {
        n_image_info_len = i_header_t.n_total_size/2;
    }
    
	p_image_info = (u8*)malloc( n_image_info_len );
	if( p_image_info == NULL )
	{
        print_log(INFO, 1, "[%s] tact_save_image malloc failed(size: %d).\n", \
                            m_p_log_id, n_image_info_len );
	    return false;
	}
	
	memset( p_date, 0, sizeof(p_date) );
	get_time_string( i_header_t.n_time, p_date );
    
    
    // make image file with information.
    memset( p_image_info, 0, n_image_info_len );
    memset( &tact_header_t, 0, sizeof(tact_image_header_t) );
    
  	tact_header_t.n_id				= i_header_t.n_id;
	tact_header_t.n_width_size		= i_header_t.n_width_size;
	tact_header_t.n_height			= i_header_t.n_height;
	tact_header_t.n_total_size		= i_header_t.n_total_size;
	tact_header_t.n_pos_x			= i_header_t.n_pos_x;
	tact_header_t.n_pos_y			= i_header_t.n_pos_y;
	tact_header_t.n_pos_z			= i_header_t.n_pos_z;
	tact_header_t.n_vcom_swing_time	= m_p_fpga_c->get_vcom_swing_time();
	tact_header_t.w_exposure_time_low = m_p_fpga_c->read(eFPGA_REG_EXP_TIME_LOW);
	tact_header_t.c_offset			= i_header_t.c_offset;
	tact_header_t.c_defect			= i_header_t.c_defect;
	tact_header_t.c_gain			= i_header_t.c_gain;
	tact_header_t.c_reserved_osf	= i_header_t.c_reserved_osf;
	tact_header_t.w_direction		= i_header_t.w_direction;
	tact_header_t.w_exposure_time_high = m_p_fpga_c->read(eFPGA_REG_EXP_TIME_HIGH);
	
    if( i_b_last_half == true )
    {
        memcpy( p_image_info, i_p_data, i_header_t.n_total_size );
        
        memcpy( &p_image_info[i_header_t.n_total_size], &tact_header_t, sizeof(tact_image_header_t) );
         
        sprintf( p_file_name, "%s0x%04X_0x%08X_0x%08X.ftm", SYS_IMAGE_DATA_PATH, m_w_tact_id, i_header_t.n_id, i_header_t.n_time );
        
#ifdef FLASH_WRITE_TIME_EVAL
        print_log(INFO, 1, "[%s] open a file to save image(file name: %s(date: %s), image id: 0x%08X(0x%04X), size: %d).\n", \
                                m_p_log_id, p_file_name, p_date, i_header_t.n_id, m_w_tact_id, i_header_t.n_total_size);
#endif
        
        p_file = fopen( p_file_name, "ab" );
    }
    else
    {
        memcpy( p_image_info, i_p_data, n_image_info_len );
        
        sprintf( p_file_name, "/tmp/0x%04X_0x%08X_0x%08X.ftm", m_w_tact_id, i_header_t.n_id, i_header_t.n_time );
        
#ifdef FLASH_WRITE_TIME_EVAL
        print_log(INFO, 1, "[%s] open a file to save image(file name: %s(date: %s), image id: 0x%08X(0x%04X), size: %d).\n", \
                                m_p_log_id, p_file_name, p_date, i_header_t.n_id, m_w_tact_id, i_header_t.n_total_size);
#endif        
        // write file
        p_file = fopen( p_file_name, "wb" );
    }
	
#ifdef FLASH_WRITE_TIME_EVAL
	struct timespec  flash_write_start_timespec;
	struct timespec  flash_write_end_timespec;
	
	if( clock_gettime(CLOCK_MONOTONIC, &flash_write_start_timespec) < 0 )
	{
	    print_log(ERROR,1,"[%s] flash_write_start_timespec time check error\n", m_p_log_id);
	}
#endif
    
    if( p_file != NULL )
    {
        if( i_b_last_half == true )
        {
            u32 n_crc;
            // flash 메모리에 이미 저장되어 있는 영상의 앞 절반 사이즈
            u32 n_flash_image_size = i_header_t.n_total_size/2;

	        n_crc =  make_crc32( p_image_info, n_image_info_len );
            
            fseek( p_file, 0, SEEK_END);
            //fwrite( &p_image_info[i_header_t.n_total_size/2], 1, (n_image_info_len - i_header_t.n_total_size/2), p_file ); 
            fwrite( &p_image_info[n_flash_image_size], 1, (n_image_info_len - n_flash_image_size), p_file ); 
            fwrite( &n_crc, 1, sizeof(n_crc), p_file ); 
    	    fclose(p_file);
			
#ifdef FLASH_WRITE_TIME_EVAL
			if( clock_gettime(CLOCK_MONOTONIC, &flash_write_end_timespec) < 0 )
			{
			    print_log(ERROR,1,"[%s] flash_write_end_timespec time check error\n", m_p_log_id);
			}
#endif
    	    
    	    safe_free(p_image_info);
    	    
    	    sys_command("sync");
        }
        else
        {        
            fwrite( p_image_info, 1, n_image_info_len, p_file ); 
	        fclose(p_file);
	        
	        safe_free(p_image_info);
	        
	        // tmp에 저장되어 있는 앞쪽 절반 영상을 실제 저장영역으로 copy
	        file_copy_to_flash(p_file_name, SYS_IMAGE_DATA_PATH);
#ifdef FLASH_WRITE_TIME_EVAL
			if( clock_gettime(CLOCK_MONOTONIC, &flash_write_end_timespec) < 0 )
			{
			    print_log(ERROR,1,"[%s] flash_write_end_timespec time check error\n", m_p_log_id);
			}
#endif
	        
	        sys_command("rm %s", p_file_name);
	    }
	    
        print_log(INFO, 1, "[%s] image(%s) is saved.\n", m_p_log_id, p_file_name);

#ifdef FLASH_WRITE_TIME_EVAL
 	struct timespec calc_t;  
 	u32             n_time = 0;
    
    calc_t.tv_sec = flash_write_end_timespec.tv_sec - flash_write_start_timespec.tv_sec;
    calc_t.tv_nsec = flash_write_end_timespec.tv_nsec - flash_write_start_timespec.tv_nsec;	
    
    if(calc_t.tv_nsec < 0)
    {
    	calc_t.tv_sec--;
    	calc_t.tv_nsec = calc_t.tv_nsec + 1000000000;
    }	
    
    n_time = calc_t.tv_sec * 1000 + calc_t.tv_nsec / 1000000;

    print_log( INFO, 1, "[%s] flash write time: %d ms\n", m_p_log_id, n_time );
#endif
        
        // 영상의 나머지 절반의 내부저장을 마친 경우, 
        // backup영상 수를 확인하여 한계 값을 넘는 경우 가장 오래된 파일 순으로 삭제한다.
        if( i_b_last_half == true )
        {
            image_check_remain_memory();
        }
        return true;
    }
    else
    {
        safe_free(p_image_info);
        print_log(ERROR, 1, "[%s] file open is failed.\n", m_p_log_id);
        return false;
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::tact_save_image( image_header_t i_header_t, u8* i_p_data )
{
    FILE*   p_file;
    s8      p_file_name[256];
	s8      p_date[256];
	
	u8*     p_image_info;
	u32     n_image_info_len = i_header_t.n_total_size + 1024;
	tact_image_header_t		tact_header_t;
	
    if( i_p_data == NULL )
    {
        print_log(INFO, 1, "[%s] invalid image data(size: %d).\n", \
                            m_p_log_id, i_header_t.n_total_size);
        return false;
    }

	p_image_info = (u8*)malloc( n_image_info_len );
	if( p_image_info == NULL )
	{
        print_log(INFO, 1, "[%s] tact_save_image malloc failed(size: %d).\n", \
                            m_p_log_id, n_image_info_len );
	    return false;
	}
	
	memset( p_date, 0, sizeof(p_date) );
	get_time_string( i_header_t.n_time, p_date );
    
    sprintf( p_file_name, "/tmp/0x%04X_0x%08X_0x%08X.ftm", m_w_tact_id, i_header_t.n_id, i_header_t.n_time );
    
    print_log(INFO, 1, "[%s] open a file to save image(file name: %s(date: %s), image id: 0x%08X(0x%04X), size: %d).\n", \
                            m_p_log_id, p_file_name, p_date, i_header_t.n_id, m_w_tact_id, i_header_t.n_total_size);
    
    // make image file with information.
    memset( p_image_info, 0, n_image_info_len );
    
    memset( &tact_header_t, 0, sizeof(tact_image_header_t) );
    
  	tact_header_t.n_id				= i_header_t.n_id;
	tact_header_t.n_width_size		= i_header_t.n_width_size;
	tact_header_t.n_height			= i_header_t.n_height;
	tact_header_t.n_total_size		= i_header_t.n_total_size;
	tact_header_t.n_pos_x			= i_header_t.n_pos_x;
	tact_header_t.n_pos_y			= i_header_t.n_pos_y;
	tact_header_t.n_pos_z			= i_header_t.n_pos_z;
	tact_header_t.n_vcom_swing_time	= m_p_fpga_c->get_vcom_swing_time();
	tact_header_t.w_exposure_time_low = m_p_fpga_c->read(eFPGA_REG_EXP_TIME_LOW);
	tact_header_t.c_offset			= i_header_t.c_offset;
	tact_header_t.c_defect			= i_header_t.c_defect;
	tact_header_t.c_gain			= i_header_t.c_gain;
	tact_header_t.c_reserved_osf	= i_header_t.c_reserved_osf;
	tact_header_t.w_direction		= i_header_t.w_direction;
	tact_header_t.w_exposure_time_high = m_p_fpga_c->read(eFPGA_REG_EXP_TIME_HIGH);
	
    memcpy( p_image_info, i_p_data, i_header_t.n_total_size );
    
    memcpy( &p_image_info[i_header_t.n_total_size], &tact_header_t, sizeof(tact_image_header_t) );
    
    // write file
    p_file = fopen( p_file_name, "wb" );
    
    if( p_file != NULL )
    {
        file_write_bin( p_file, p_image_info, n_image_info_len );
	    fclose(p_file);
        
        file_copy_to_flash(p_file_name, SYS_IMAGE_DATA_PATH);
        print_log(INFO, 1, "[%s] image(%s) is saved.\n", m_p_log_id, p_file_name);
        
   	    safe_free(p_image_info);

        image_check_remain_memory();
        
        sys_command("rm %s", p_file_name);
        
        return true;
    }
    else
    {
        safe_free(p_image_info);
        print_log(ERROR, 1, "[%s] file open is failed.\n", m_p_log_id);
        return false;
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CVWSystem::tact_get_id(void)
{
    if( tact_get_enable() == true )
    {
        return m_w_tact_id;
    }
    
    return 0;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::tact_flush_image(void)
{
    if( tact_get_enable() == false )
    {
        return;
    }
    
    s8  p_cmd[512];
    
    m_w_tact_id = 0;
    
    sprintf( p_cmd, "rm %s*.ftm", SYS_IMAGE_DATA_PATH );
    sys_command( p_cmd );
    sys_command("sync");
    
    print_log(DEBUG, 1, "[%s] flush ftm images\n", m_p_log_id);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::tact_update_id(void)
{
    if( tact_get_enable() == true )
    {
        m_w_tact_id++;
        
        if( m_w_tact_id == 0 )
        {
            m_w_tact_id = 1;
        }
    }
}

/******************************************************************************/
/**
 * @brief   디텍터 내부 저장된 영상 수가 한계치를 넘는 경우, 가장 오래된 영상 파일을 삭제한다.
 * @param   none
 * @return  none
 * @note    내부 저장된 영상 수 = backup 영상 수 + FTM 영상 수
*******************************************************************************/
void CVWSystem::image_check_remain_memory(void)
{
	m_w_saved_image_count = backup_image_get_count() + tact_get_image_count();
    if( m_w_saved_image_count > BACKUP_IMAGE_MAX )
    {
        // delete oldest backup image
        backup_image_delete_oldest();
    }
    m_w_saved_image_count = backup_image_get_count() + tact_get_image_count();
}

/******************************************************************************/
/**
 * @brief   image transmission test를 위해 x_ray delay num 값을 변경하는 함수
 * @param   i_b_on: true(eFPGA_XRAY_DELAY_NUM = 0), false(eFPGA_XRAY_DELAY_NUM = 기존 저장 값)
 * @return  None
 * @note    일시 변경을 위해 register 값만 변경하고 따로 저장하지 않고 복구 시 저장되어 있는 값으로 복구
 *******************************************************************************/
void CVWSystem::test_image_change_xray_delay_num(bool i_b_on)
{
    u16 w_data;
    if(i_b_on == true)
    {
        w_data = m_p_fpga_c->read(eFPGA_XRAY_DELAY_NUM);
        
        if(w_data != 0)
        {
            m_p_fpga_c->write(eFPGA_XRAY_DELAY_NUM, 0);
            //print_log(ERROR, 1, "[%s] [TP] eFPGA_XRAY_DELAY_NUM : %d\n", m_p_log_id, m_p_fpga_c->read(eFPGA_XRAY_DELAY_NUM));
        }
    }
    else
    {
        w_data = m_p_fpga_c->read(eFPGA_XRAY_DELAY_NUM, true); //fpga.bin에 저장되어 있는 값
        m_p_fpga_c->write(eFPGA_XRAY_DELAY_NUM, w_data);
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::trans_is_done(void)
{
    if( m_b_trans_done_enable == false )
    {
        return true;
    }
    
    return m_b_trans_done;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::scu_set_connection( bool i_b_connected )
{
    u32 n_state;
    
    if( m_b_scu_connected != i_b_connected )
    {
        m_b_scu_connected = i_b_connected;
        
        if( m_b_scu_connected == true )
        {
            n_state = eIMG_STATE_SCU_IS_CONNECTED;
        }
        else
        {
            n_state = eIMG_STATE_SCU_IS_DISCONNECTED;
        }
    
        message_send( eMSG_IMAGE_STATE, (u8*)&n_state, sizeof(n_state) );
       
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::is_web_enable(void)
{
	m_b_web_ready = false;
	
	return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::patient_info_set( patient_info_t i_info_t )
{
	memcpy( &m_patient_info_t, &i_info_t, sizeof(patient_info_t) );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::capture_ready(void)
{
	u16 w_ready = m_p_fpga_c->read( eFPGA_REG_XRAY_READY );
	
	if( w_ready != 0 )
	{
		print_log(DEBUG, 1, "[%s] capture is not ready(0x%04X)!!!\n", m_p_log_id, w_ready);
		return;
	}
	
	m_p_fpga_c->write( eFPGA_REG_XRAY_READY, 1 );
	m_p_fpga_c->enable_interrupt(true);
	
	m_p_fpga_c->tft_aed_control();
	
	print_log(DEBUG, 1, "[%s] capture is ready.\n", m_p_log_id);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CVWSystem::get_direction(void)
{
	return (u16)m_n_img_direction;
}
                                                                              
/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::power_control_for_phy(void)
{
	if( m_p_env_c->is_wired_detection_on() == true )
	{
		if( m_p_hw_mon_c->phy_is_up() == false )
		{
			m_p_hw_mon_c->phy_up();
		}
		return;
	}
	
	if( m_current_net_mode_e == eNET_MODE_STATION || m_current_net_mode_e == eNET_MODE_AP)
	{
		m_p_hw_mon_c->phy_down();
	}
	else if( m_current_net_mode_e == eNET_MODE_TETHER)// && ( m_current_net_mode_e == eNET_MODE_AP)
	{
		m_p_hw_mon_c->phy_up();
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::get_link_state(void)
{
	if( m_p_env_c->is_wired_detection_on() == true )
	{
		return vw_net_link_state("eth0");
	}
	
	return hw_get_tether_equip();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::offset_set_enable( bool i_b_enable )
{
	m_b_offset_enable = i_b_enable;
	m_p_fpga_c->offset_set_enable(i_b_enable);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::offset_get_enable(void)
{
	if( is_double_readout_enabled() == true || is_battery_saving_mode() == false )
	{
		bool b_enable = m_p_fpga_c->offset_get_enable();
	
		if( m_b_offset_enable != b_enable )
		{
			print_log(ERROR, 1, "[%s] offset enable is not valid(%d/%d)\n", m_p_log_id, m_b_offset_enable, b_enable);
			
			m_p_fpga_c->offset_set_enable(m_b_offset_enable);
		}
	}
	
	return m_b_offset_enable;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::gain_set_enable( bool i_b_enable )
{
	m_b_gain_enable = i_b_enable;
	m_p_fpga_c->gain_set_enable(i_b_enable);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::gain_get_enable(void)
{
	if(is_battery_saving_mode() == false)
	{
		bool b_enable = m_p_fpga_c->gain_get_enable();
	
		if( m_b_gain_enable != b_enable )
		{
			print_log(ERROR, 1, "[%s] gain enable is not valid(%d/%d)\n", m_p_log_id, m_b_gain_enable, b_enable);
		
			m_b_gain_enable = b_enable;
		}
	}
	
	return m_b_gain_enable;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::gain_set_pixel_average( u16 i_w_average )
{
	m_w_gain_average = i_w_average;
	m_p_fpga_c->gain_set_pixel_average(i_w_average);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::enable_image_correction(void)
{
	m_p_fpga_c->offset_set_enable(m_b_offset_enable);
	m_p_fpga_c->gain_set_enable(m_b_gain_enable);
	m_p_fpga_c->gain_set_pixel_average(m_w_gain_average);
}

/******************************************************************************/
/**
 * @brief   FPGA ddr에 upload 되어 있는 cal data 가 정상인지 crc체크
 * @param   
 * @return  false : FPGA ddr에 upload 되어 있는 cal data 비정상, upload 필요한 경우
 			true : FPGA ddr에 upload 되어 있는 cal data 정상, 혹은 FPGA ddr에 cal data가 애초에 없어 
 			       추가적인 cal data upload 불필요한 경우
 * @note    FPGA ddr에 upload 되어 있는 cal data 가 정상인지 체크해야 하는 경우는
 			fpga power down 후 wake up 시, 정전기 등에 의한 fpga re-init직후 등이 있음
*******************************************************************************/
bool CVWSystem::is_cal_data_valid(void)
{
	if( is_battery_saving_mode() == false )
	{
		if( is_double_readout_enabled() == false && m_b_offset_load == true )
		{
			if( m_p_fpga_c->is_offset_data_valid(m_n_offset_crc, m_n_offset_crc_len) == false )
			{
				return false;
			}
		}
		
		if( m_b_gain_load == true )
		{
			if( m_p_fpga_c->is_gain_data_valid(m_n_gain_crc, m_n_gain_crc_len) == false )
			{
				return false;
			}
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
void CVWSystem::wait_until_cal_data_load_end(void)
{
	CTime* p_wait_time = new CTime(5000);
	while(p_wait_time && p_wait_time->is_expired() == false )
	{
		if(m_b_cal_data_load_end == true)
			break;
			
		sleep_ex(100000);	// sleep 100ms
	}
	
	safe_delete(p_wait_time);
}
	
/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
 *******************************************************************************/
void CVWSystem::dev_info( DEV_ID i_id_e )
{
    if( i_id_e == eDEV_ID_ROIC )
    {
		m_p_fpga_c->roic_info();
    }  
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
 *******************************************************************************/
u8 CVWSystem::get_aed_count()
{
	return m_p_hw_mon_c->get_aed_count();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::read_detector_position(void)
{
    memset( m_p_position, 0, sizeof(m_p_position) );
    m_p_hw_mon_c->impact_get_pos( &m_p_position[0], &m_p_position[1], &m_p_position[2] );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    Gyro sensor 존재 시, gyro sensor val return
 *          gyro sensor 미존재 시, acceleration sensor val return
*******************************************************************************/
void CVWSystem::get_main_detector_angular_position( s32* o_p_x, s32* o_p_y, s32* o_p_z)
{
    if(m_p_hw_mon_c->is_gyro_exist())
    {
        m_p_hw_mon_c->get_gyro_position( o_p_x, o_p_y, o_p_z );
        //print_log(DEBUG, 1, "[%s] main gyro pos x: %6d, y: %6d, z: %6d\n", m_p_log_id, *o_p_x, *o_p_y, *o_p_z);
    }
    else
    {
        m_p_hw_mon_c->impact_get_pos( o_p_x, o_p_y, o_p_z );
        //print_log(DEBUG, 1, "[%s] main acc pos x: %6d, y: %6d, z: %6d\n", m_p_log_id, *o_p_x, *o_p_y, *o_p_z);       
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    Gyro sensor 존재 시, acceleration sensor val return
 *          gyro sensor 미존재 시, return 0
*******************************************************************************/
void CVWSystem::get_sub_detector_angular_position( s32* o_p_x, s32* o_p_y, s32* o_p_z)
{
    if(m_p_hw_mon_c->is_gyro_exist())
    {
        m_p_hw_mon_c->impact_get_pos( o_p_x, o_p_y, o_p_z );
        //print_log(DEBUG, 1, "[%s] sub acc pos x: %6d, y: %6d, z: %6d\n", m_p_log_id, *o_p_x, *o_p_y, *o_p_z);  
    }
    else
    {
        *o_p_x = 0;
        *o_p_y = 0; 
        *o_p_z = 0;
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::activate_ap_button_function(void)
{
	print_log(DEBUG, 1, "[%s] activate_ap_button_function\n", m_p_log_id);
	m_b_is_ap_button_actived = true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::deactivate_ap_button_function(void)
{
	print_log(DEBUG, 1, "[%s] deactivate_ap_button_function\n", m_p_log_id);
	m_b_is_ap_button_actived = false;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::is_ap_button_function_activated(void)
{
	return m_b_is_ap_button_actived;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::restore_power_config(void)
{
	print_log(DEBUG, 1, "[%s] restore power config\n", m_p_log_id);
                            
	m_p_hw_mon_c->restore_charging_state();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::boot_end(void)
{
    if(is_vw_revision_board())
        micom_boot_led_on();
	m_p_hw_mon_c->boot_end();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::net_set_tether_speed( u16 i_w_speed )
{
	if( m_w_tether_speed != i_w_speed
		&& i_w_speed == 1000
		&& hw_get_power_source() == ePWR_SRC_TETHER )
	{
		charge_time_lock();
		
		safe_delete(m_p_charge_time_c);
		m_p_charge_time_c = new CTime(1000);
		
		charge_time_unlock();
		
		print_log(DEBUG, 1, "[%s] bat charge timer start\n", m_p_log_id);
	}
	
	m_w_tether_speed = i_w_speed;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::net_is_valid_speed(void)
{
	if( m_w_tether_speed == 1000 )
	{
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
void CVWSystem::update_oled_state(sys_state_t* i_p_state_t)
{
	m_p_oled_c->update_state(i_p_state_t);
	
	if( m_current_net_mode_e == eNET_MODE_AP)
	{
		
		m_p_oled_c->update_ssid((u8*)config_get(eCFG_AP_SSID));
	}
	else 
	{
		m_p_oled_c->update_ssid((u8*)config_get(eCFG_SSID));
	}	
	
	m_p_oled_c->update_ip((u8*)config_get(eCFG_IP_ADDRESS));
	m_p_oled_c->update_preset((u8*)config_get(eCFG_PRESET_NAME));
	
    pm_lock();
    if( m_pm_mode_e == ePM_MODE_NORMAL && i_p_state_t->b_fpga_ready == true )
    {  
		// GATE 에러 체크, 에러 발생 시 해당 GATE 단위 영상 이상
		if(i_p_state_t->w_gate_state !=  m_w_normal_gate_state)
		{
			set_oled_error(eGATE_STATE_ERROR, true);
		}
		else
		{
			set_oled_error(eGATE_STATE_ERROR, false);
		}
		
		// ROIC bit alignment 에러 체크, 에러 발생 시 해당 ROIC 영상 이상
		if(m_p_fpga_c->read(eFPGA_REG_TI_ROIC_BIT_ALIGN_ERR) != 0)
		{
			set_oled_error(eROIC_BIT_ALIGN_ERROR, true);
		}
		else
		{
			set_oled_error(eROIC_BIT_ALIGN_ERROR, false);
		}
		
		// AED 모드일 때에만 AED 에러 체크
		if( m_p_fpga_c->trigger_get_type() == eTRIG_SELECT_NON_LINE )
		{
			u8 c_aed_idx = 0;
			u8 c_aed_count = m_p_hw_mon_c->get_aed_count();
			u16 w_select_all = 0;
	
			for(c_aed_idx = 0; c_aed_idx < c_aed_count; c_aed_idx++)
			{
				w_select_all |= (0x01 << c_aed_idx);
			}
			
			u16 w_aed_sel = m_p_fpga_c->read(eFPGA_REG_AED_SELECT);
			u16 w_aed_err = m_p_fpga_c->read(eFPGA_REG_AED_ERROR);
			
			// AED 가 모두 선택되어 있지 않거나, 선택된 AED 센서 모두 에러가 발생한 경우 OLED에 에러 표시		
			if( ( (w_aed_sel & w_select_all) == 0 ) || \
				( (w_aed_err & w_select_all) == (w_aed_sel & w_select_all)) )
			{
				set_oled_error(eAED_SENSOR_ERROR, true);
			}
			else
			{
				set_oled_error(eAED_SENSOR_ERROR, false);
			}
		}
		else
		{
			// DR 모드일 때에는 AED 에러 해제
			set_oled_error(eAED_SENSOR_ERROR, false);
		}
	}
	pm_unlock();
	
	// MAIN 전압 8V 체크, 7V미만, 9V초과이면 에러 표시
	if( i_p_state_t->p_hw_power_volt[POWER_VOLT_VS] < 700
		&&  i_p_state_t->p_hw_power_volt[POWER_VOLT_VS] > 900 )
	{
		set_oled_error(eMAIN_VOLT_ERROR, true);
	}
	else
	{
		set_oled_error(eMAIN_VOLT_ERROR, false);
	}
	
	// 장착된 Battery 개수 체크, 제조정보에 설정한 배터리 수와 불일치 하는 경우 에러 표시 
	if(m_p_hw_mon_c->get_battery_count() != m_p_hw_mon_c->bat_get_equip_num())
	{
		set_oled_error(eBAT_NUM_ERROR, true);
	}
	else
	{
		set_oled_error(eBAT_NUM_ERROR, false);
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::set_oled_display_event(OLED_EVENT i_event_e)
{
	m_p_oled_c->set_oled_display_event(i_event_e);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::set_ap_switching_mode(bool i_b_enable)
{
	m_b_ap_switching_mode = i_b_enable;
	m_p_oled_c->m_b_preset_blink = i_b_enable;
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSystem::apply_gain_table(void)
{
	m_p_fpga_c->apply_gain_table_by_panel_type(get_panel_type());
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::is_battery_saving_mode(void)
{
	bool b_battery_save = false;
	u16 w_data = m_p_fpga_c->read(eFPGA_REG_DDR_IDLE_STATE);
	switch(w_data)
	{
		case 0:		// fpga ddr idle state : power off, fw에서 correction 필요
			b_battery_save = true;
			break;
		case 1:		// fpga ddr idle state : power on, fpga에서 correction 수행
			b_battery_save = false;
			break;
		default:
			b_battery_save = false;
			break;
	}
	
	return b_battery_save;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note     IDLE 상태에서 DDR power off 설정 시 FPGA에서는 Exposure 구간에 DDR power ON 하고 
 			FW에서 마지막 라인 영상 가져가면 DDR Power OFF 하도록 설계함
			Image timeout 에러와 같이 영상을 다 못 가져가는 에러 발생 시에도 DDR Power OFF 하는 방어 코드 필요함
*******************************************************************************/
void CVWSystem::restart_battery_saving_mode(void)
{
	if(is_battery_saving_mode() == true)
	{
		m_p_fpga_c->write(eFPGA_REG_DDR_IDLE_STATE, 0);
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note     
*******************************************************************************/
u32 CVWSystem::get_wakeup_remain_time(void)
{
	return m_p_fpga_c->get_expected_wakeup_time();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note     
*******************************************************************************/
void CVWSystem::gate_line_defect_enable(bool i_b_enable)
{
	if( i_b_enable == true && is_lower_grid_line_support() == true )
	{
		m_p_fpga_c->gate_line_defect_enable(false);
		print_log(DEBUG, 1, "[%s] Gate line defect enable failed. Lower grid line is already enabled\n", m_p_log_id);
	}
	else
	{
		m_p_fpga_c->gate_line_defect_enable(i_b_enable);
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note     
*******************************************************************************/
void CVWSystem::set_impact_extra_detailed_offset(s16* i_w_x_offset, s16* i_w_y_offset, s16* i_w_z_offset)
{
    impact_cal_extra_detailed_offset_t impact_extra_detailed_offset;
    memset(&impact_extra_detailed_offset, 0x00, sizeof(impact_cal_extra_detailed_offset_t));
    if(i_w_x_offset!=NULL)
        impact_extra_detailed_offset.s_x = *i_w_x_offset;
    if(i_w_y_offset!=NULL)
        impact_extra_detailed_offset.s_y = *i_w_y_offset;
    if(i_w_z_offset!=NULL)
        impact_extra_detailed_offset.s_z = *i_w_z_offset;

    m_p_hw_mon_c->set_impact_extra_detailed_offset(&impact_extra_detailed_offset);
    m_p_env_c->save_impact_extra_detailed_offset(&impact_extra_detailed_offset);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note     
*******************************************************************************/
void CVWSystem::get_impact_offset(s16* o_w_x_offset, s16* o_w_y_offset, s16* o_w_z_offset)
{
    impact_cal_offset_t imapct_cal_offset;

	m_p_hw_mon_c->impact_get_offset(&imapct_cal_offset);
    *o_w_x_offset = imapct_cal_offset.c_x;
    *o_w_y_offset = imapct_cal_offset.c_y;
    *o_w_z_offset = imapct_cal_offset.c_z;
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note     
*******************************************************************************/
void CVWSystem::save_impact_6plane_cal_done(void)
{
	u8 c_done = m_p_hw_mon_c->get_6plane_impact_sensor_cal_done();
	m_p_env_c->save_impact_6plane_cal_done(c_done);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note     
*******************************************************************************/
void CVWSystem::SetSamsungOnlyFunction( u32 i_n_addr, u16 i_w_data )
{
	switch(i_n_addr)
	{
		case eFW_ADDR_LED_BLINKING:
			pm_power_led_ctrl(i_w_data);
			break;
		case eFW_ADDR_WIFI_LEVEL_CHANGE:
			m_wifi_step_e = (WIFI_STEP_NUM)i_w_data;
			m_p_oled_c->set_wifi_signal_step_num(m_wifi_step_e);
			m_p_env_c->save_wifi_signal_step_num((u8)i_w_data);
			break;
		default:
			break;
	}
		
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note     
*******************************************************************************/
u16 CVWSystem::GetSamsungOnlyFunction( u32 i_n_addr )
{
	u16 w_data = 0;
	switch(i_n_addr)
	{
		case eFW_ADDR_LED_BLINKING:
			w_data = pm_get_power_led_ctrl();
			break;
		case eFW_ADDR_WIFI_LEVEL_CHANGE:
			w_data = (u16)m_wifi_step_e;
			break;
		default:
			break;
	}
	
	return w_data;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note     
*******************************************************************************/
ROIC_TYPE CVWSystem::get_roic_model_type(void)
{
	return m_p_env_c->get_roic_model_type();
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note     
*******************************************************************************/
PHY_TYPE CVWSystem::get_phy_model_type(void)
{
	return m_p_hw_mon_c->get_phy_model_type();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note     일부 SDK에 Package revision 정보와 BD revsion 정보를 매칭해서 업데이트 가능/불가 판단하는 로직이 누락 되어 있음.
 *           이를 대체하기위해 하기 로직 구현. 추후, 모든 SDK에서 처리될 수 있을 때, 하기 로직은 불필요.
*******************************************************************************/
bool CVWSystem::check_compability_bootloader_version(char* i_s_version)
{	
	s8* p_token;
	s8 p_token_string[] = ".\n ";
    bool ret = true;

    char s_version[128] = "";

    // VW udpate 불가 조건
    // 1. ADI ROIC이고, bootloader 버전이 0.0.4 이하인 경우
    // 2. ADI PHY이고, bootloader 버전이 0.0.6 이하인 경우 업데이트 실패 처리
    if( (m_p_env_c->get_model() == eMODEL_2530VW) ||
        (m_p_env_c->get_model() == eMODEL_3643VW) ||
        (m_p_env_c->get_model() == eMODEL_4343VW) )
    {
        if(get_roic_model_type() == eROIC_TYPE_ADI_1255)
        {
            memcpy(s_version, i_s_version, 128);
            p_token = strtok(s_version,p_token_string);
            if( _check_exceed(p_token,0) == false )
            {
                p_token = strtok(NULL,p_token_string);
                if( _check_exceed(p_token,0) == false )
                {
                    p_token = strtok(NULL,p_token_string);
                    if( _check_exceed(p_token,4) == false)
                    {
                        print_log(DEBUG, 1, "[%s] Not compatible bootloader version(%s). Update stop.\n", m_p_log_id, i_s_version);
                        return false;
                    }
                }
            }
        }

        if(get_phy_model_type() == ePHY_ADI_ADIN1300)
        {
            memcpy(s_version, i_s_version, 128);
            p_token = strtok(s_version,p_token_string);
            if( _check_exceed(p_token,0) == false )
            {
                p_token = strtok(NULL,p_token_string);
                if( _check_exceed(p_token,0) == false )
                {
                    p_token = strtok(NULL,p_token_string);
                    if( _check_exceed(p_token,6) == false)
                    {
                        print_log(DEBUG, 1, "[%s] Not compatible bootloader version(%s). Update stop.\n", m_p_log_id, i_s_version);
                        return false;
                    }
                }
            }
        }

    }
    
    return ret;
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::_check_exceed(s8* i_p_token, s32 i_n_compare_num)
{
	return m_p_env_c->_check_exceed(i_p_token, i_n_compare_num);
}

/******************************************************************************/
/**
 * @brief   SDK version 1.0.4.16이상에서만 로그인 허용
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSystem::_check_compatibility_sdk_version(s8* i_p_sdk_str)
{
	s8* p_token;
	s8 p_token_string[] = ".TP\n ";
	
	p_token = strtok(i_p_sdk_str,p_token_string);
	
	if ( _check_exceed(p_token,1) == true)
	{
		return true; 
	}

	p_token = strtok(NULL,p_token_string);
	
	if ( _check_exceed(p_token,0) == true)
	{
		return true; 
	}
	
	p_token = strtok(NULL,p_token_string);
	
	if ( _check_exceed(p_token,4) == true)
	{
		return true; 
	}
	
	p_token = strtok(NULL,p_token_string);
	
	if ( _check_exceed(p_token,15) == true)
	{
		return true; 
	}
	
	return false;	
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  0: success
 *          1: fail
 * @note     
*******************************************************************************/
u32 CVWSystem::set_logical_digital_gain(u32 i_logical_digital_gain)
{
    u32 ret  = 0;

    SCINTILLATOR_TYPE e_default_scintillator_type = get_scintillator_type_default();

    double lf_default_rfdg_gain = get_roic_cmp_gain_by_scintillator_type(e_default_scintillator_type) / 1024.0;     //unit: ratio
    double lf_logical_digital_gain = i_logical_digital_gain / 1024.0;  //unit: ratio 

    //calc target gain
    //target gain = (default analog gain / default analog gain) * default RFDG * default digital gain * logical digital gain
    //            =  (1) * default RFDG * 1 * local digital gain 
    double lf_target_final_gain = lf_default_rfdg_gain * lf_logical_digital_gain;
    print_log(DEBUG, 1, "[%s] Target scint type: %d IFS r: %lf RFDG r: %lf DG r: %lf LDG r: %lf final r: %lf\n", m_p_log_id, e_default_scintillator_type   \
                                                                                                              , 1.0, lf_default_rfdg_gain \
                                                                                                              , 1.0, lf_logical_digital_gain, lf_target_final_gain );
    //dfeault value validation
    if(lf_target_final_gain <= 0)
    {
        ret = 1;
        print_log(DEBUG, 1, "[%s] invalid lf_target_final_gain: %lf. Quit.\n", m_p_log_id, lf_target_final_gain);    
        return ret;
    }     
    
    u32 n_default_input_charge_range = get_input_charge_by_scintillator_type(e_default_scintillator_type);   //unit: bit

    //SCINTILLATOR_TYPE e_candidate_scintillator_type = eSCINTILLATOR_TYPE_00;
    u32 n_candidate_input_charge_range = 0;     //unit: bit
    double lf_candidate_analog_gain_ratio = 0.0;
    double lf_candidate_rfdg_gain = 0.0;         //unit: ratio 
    double lf_candidate_digital_gain = 0.0;
    double lf_candidate_final_gain = 0.0;

    SCINTILLATOR_TYPE e_bestfit_scintillator_type = eSCINTILLATOR_TYPE_MAX;
    double lf_bestfit_rfdg_gain = std::numeric_limits<double>::max( );                     
    double lf_bestfit_digital_gain = 1.0;

 
    //for all list, find RFDG * Digital gain and find mimum diff & but larger than 1
    for(int i = eSCINTILLATOR_TYPE_02; i < eSCINTILLATOR_TYPE_MAX; i++)
    {
        n_candidate_input_charge_range = get_input_charge_by_scintillator_type((SCINTILLATOR_TYPE)i);   //unit: bit
        switch(get_roic_model_type())
        {
            case eROIC_TYPE_ADI_1255:
                lf_candidate_analog_gain_ratio = (n_default_input_charge_range + 1.0) / (n_candidate_input_charge_range + 1.0);
                //lf_candidate_analog_gain_ratio = (0.125 * (n_default_input_charge_range + 1) / (0.125 * (n_candidate_input_charge_range + 1));

                break;

            case eROIC_TYPE_TI_2256GR:
                lf_candidate_analog_gain_ratio = (n_default_input_charge_range - 15.0) / (n_candidate_input_charge_range - 15.0);
                //lf_candidate_analog_gain_ratio = 0.6 * (n_default_input_charge_range - 15) / (0.6 * (n_candidate_input_charge_range - 15));
                break;

            default:        //use TI 
                lf_candidate_analog_gain_ratio = (n_default_input_charge_range - 15.0) / (n_candidate_input_charge_range - 15.0);
                //lf_candidate_analog_gain_ratio = 0.6 * (n_default_input_charge_range - 15) / (0.6 * (n_candidate_input_charge_range - 15));
                break;
        }

        lf_candidate_rfdg_gain = get_roic_cmp_gain_by_scintillator_type((SCINTILLATOR_TYPE)i) / 1024.0;     //unit: ratio 

        if( ( lf_candidate_analog_gain_ratio <= 0 ) || ( lf_candidate_rfdg_gain <= 0 ) )
        {
            ret = 1;
            print_log(DEBUG, 1, "[%s] invalid Candidate scint type: %d IFS rr: %lf or RFDG r: %lf. skip.\n", m_p_log_id, (SCINTILLATOR_TYPE)i \
                                                                                                                   , lf_candidate_analog_gain_ratio
                                                                                                                   , lf_candidate_rfdg_gain);    
            continue;           
        }

        lf_candidate_digital_gain = lf_target_final_gain / (lf_candidate_analog_gain_ratio * lf_candidate_rfdg_gain); 
        lf_candidate_final_gain = lf_candidate_analog_gain_ratio * lf_candidate_rfdg_gain * lf_candidate_digital_gain;
        
        print_log(DEBUG, 1, "[%s] Candidate scint type: %d IFS rr: %lf RFDG r: %lf DG r: %lf final r: %lf\n", m_p_log_id, (SCINTILLATOR_TYPE)i \
                                                                                                          , lf_candidate_analog_gain_ratio, lf_candidate_rfdg_gain \
                                                                                                          , lf_candidate_digital_gain, lf_candidate_final_gain);

        //find best candidate
        if( ((lf_candidate_rfdg_gain * lf_candidate_digital_gain) >= 1.0) && \
            ((lf_candidate_rfdg_gain * lf_candidate_digital_gain) < (lf_bestfit_rfdg_gain * lf_bestfit_digital_gain)) ) 
        {
            e_bestfit_scintillator_type = (SCINTILLATOR_TYPE)i;
            lf_bestfit_rfdg_gain = lf_candidate_rfdg_gain;
            lf_bestfit_digital_gain = lf_candidate_digital_gain;
        }
    }

    //validation
    if( e_bestfit_scintillator_type == eSCINTILLATOR_TYPE_MAX )
    {
        //set fail & not save
        print_log(DEBUG, 1, "[%s] bestfit not found. quit.\n", m_p_log_id);   
        ret = 1;
        return ret;
    }    
        
    print_log(DEBUG, 1, "[%s] Final scint type: %d DG r: %lf\n", m_p_log_id, e_bestfit_scintillator_type, lf_bestfit_digital_gain);

    //apply & save 
    //specific gain to fpga reg (FW only) & save
    m_p_fpga_c->write(eFPGA_REG_LOGICAL_DIGITAL_GAIN, i_logical_digital_gain);
    m_p_fpga_c->reg_save(eFPGA_REG_LOGICAL_DIGITAL_GAIN, eFPGA_REG_LOGICAL_DIGITAL_GAIN);

    //bestfit scintilltator type to manufacture info & apply to roic 
    manufacture_info_t	new_manufacture_info_t;
    manufacture_info_get(&new_manufacture_info_t);
    new_manufacture_info_t.n_scintillator_type = e_bestfit_scintillator_type;
    manufacture_info_set(&new_manufacture_info_t);  
    m_p_fpga_c->set_gain_type();

    //bestfit digital gain  to fpga reg & save 
    m_p_fpga_c->write(eFPGA_REG_DIGITAL_GAIN, (u16)(lf_bestfit_digital_gain * 1024.0 + 0.5) );
    m_p_fpga_c->reg_save(eFPGA_REG_DIGITAL_GAIN, eFPGA_REG_DIGITAL_GAIN);    

    //lf_default_rfdg_gain * lf_logical_digital_gain;
    double lf_analog_gain_from_reg = m_p_fpga_c->read(eFPGA_REG_DIGITAL_GAIN) / 1024.0;
    print_log(DEBUG, 1, "[%s] real final analog gain r: %lf\n", m_p_log_id, lf_analog_gain_from_reg);
    
    //TEST
    //print_log(DEBUG, 1, "[%s] real final analog gain r: %.15lf\n", m_p_log_id, (1.0/1024.0));

    //save reg to fpga.bin file
    dev_reg_save_file(eDEV_ID_FPGA);

    return ret;    
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note     
*******************************************************************************/
u32 CVWSystem::get_logical_digital_gain(void)
{
	return m_p_fpga_c->read(eFPGA_REG_LOGICAL_DIGITAL_GAIN);
}


/******************************************************************************/
/**
 * @brief   
 * @param   i_gain_compensation: ratio (unit: 0.01%)
 * @return  0: success
 *          1: fail
 * @note     
*******************************************************************************/
u32 CVWSystem::set_gain_compensation(u32 i_gain_compensation)
{
    u32 ret = 0;

    //validation (i_gain_compensation ratio range: 0% ~ 50%)
    if( (i_gain_compensation < 0) || (i_gain_compensation > 5000)  )
    {
        //set fail & not save
        print_log(DEBUG, 1, "[%s] invalid i_gain_compensation: %d.\n", m_p_log_id, i_gain_compensation);   
        ret = 1;
        return ret;
    }    
    else
    {
        //TEMP
        print_log(DEBUG, 1, "[%s] i_gain_compensation: %d (unit: 0.01%)\n", m_p_log_id, i_gain_compensation);

        //apply & save 
        //specific gain to fpga reg (FW only) & save
        m_p_fpga_c->write(eFPGA_REG_GAIN_COMPENSATION, i_gain_compensation);
        m_p_fpga_c->reg_save(eFPGA_REG_GAIN_COMPENSATION, eFPGA_REG_GAIN_COMPENSATION);
        //save reg to fpga.bin file
        dev_reg_save_file(eDEV_ID_FPGA);
    }

    return ret;    
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note     
*******************************************************************************/
u32 CVWSystem::get_gain_compensation(void)
{
	return m_p_fpga_c->read(eFPGA_REG_GAIN_COMPENSATION);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note     
*******************************************************************************/
void CVWSystem::manufacture_info_set( manufacture_info_t* i_p_info_t )
{
    m_p_env_c->manufacture_info_set(i_p_info_t);
    
    //reload manufacture info... //현재 동작 중인 값도 업데이트
    m_p_fpga_c->set_scintillator_type( (SCINTILLATOR_TYPE) i_p_info_t->n_scintillator_type );     
}

/******************************************************************************/
/**
 * @brief   
 * @param	
 * @return  
 * @note	
 *******************************************************************************/
u8 CVWSystem::get_smart_w_onoff(void)
{
    u8 c_ret = 0;
    c_ret = m_p_fpga_c->get_smart_w_onoff();
    return c_ret;
}

/******************************************************************************/
/**
 * @brief   
 * @param	
 * @return  
 * @note	
 *******************************************************************************/
u8 CVWSystem::set_smart_w_onoff(u8 i_enable)
{
    u8 c_ret = 0;
    c_ret = m_p_fpga_c->set_smart_w_onoff(i_enable);
    aed_power_control(get_acq_mode(), (TRIGGER_SELECT)trigger_get_type(), get_smart_w_onoff());

    return c_ret;
}

/******************************************************************************/
/**
 * @brief   
 * @param	
 * @return  
 * @note	
 *******************************************************************************/
u32 CVWSystem::get_measrued_exposure_time(void)
{
    u32 n_ret = 0;
    n_ret = (m_p_fpga_c->read(eFPGA_REG_TCON_XRAY_EXPOSURE_TIME_HIGH) << 16);
    n_ret |= m_p_fpga_c->read(eFPGA_REG_TCON_XRAY_EXPOSURE_TIME_LOW);
    return n_ret;
}

/******************************************************************************/
/**
 * @brief
 * @param
 * @return
 * @note
 *******************************************************************************/
u32 CVWSystem::get_frame_exposure_time(u8 i_panel_num)
{
    u32 n_ret = 0;

    switch (i_panel_num)
    {
    case 1:
        n_ret = (m_p_fpga_c->read(eFPGA_XRAY_FRAME_EXPOSURE_TIME_HIGH) << 16);
        n_ret |= m_p_fpga_c->read(eFPGA_XRAY_FRAME_EXPOSURE_TIME_LOW);
        break;
    case 2:
        n_ret = (m_p_fpga_c->read(eFPGA_OFFSET_FRAME_EXPOSURE_TIME_HIGH) << 16);
        n_ret |= m_p_fpga_c->read(eFPGA_OFFSET_FRAME_EXPOSURE_TIME_LOW);
        break;
    default:
        break;
    }
    return n_ret;
}

/******************************************************************************/
/**
 * @brief   
 * @param	
 * @return  
 * @note	
 *******************************************************************************/
u16 CVWSystem::get_aed_off_debounce_time(void)
{
    u16 w_ret = 0;
    w_ret = m_p_fpga_c->get_aed_off_debounce_time();
    return w_ret;
}

/******************************************************************************/
/**
 * @brief   
 * @param	
 * @return  
 * @note	
 *******************************************************************************/
u8 CVWSystem::set_aed_off_debounce_time(u16 i_w_debounce_time)
{
    u8 c_ret = 0;
    c_ret = m_p_fpga_c->set_aed_off_debounce_time(i_w_debounce_time);
    return c_ret;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    Trigger Type과 Smart W ON/OFF 상태를 확인하여 AED Sensor를 ON/OFF 및 관련 register 설정함. 
 *          (smart W 사양서 참고)
*******************************************************************************/
void CVWSystem::aed_power_control(GEN_INTERFACE_TYPE i_acq_mode, TRIGGER_SELECT i_e_trig, u8 i_c_smart_w_enable)
{

	//Control logic (smart W 사양서 참고)
    bool aed_power_onoff = ( (i_acq_mode != eGEN_INTERFACE_RAD) && (i_e_trig != eTRIG_SELECT_NON_LINE) ) || 
                      ( (i_acq_mode == eGEN_INTERFACE_RAD) && (i_e_trig != eTRIG_SELECT_NON_LINE) && (i_c_smart_w_enable == 0) ) ? false : true;

    //AED power control 도중에 FPGA가 noise 성 신호를 인식하지 않도록 AED sensor OFF 처리
    //DR trigger 상태에서 패키지 업데이트할 경우, eFPGA_REG_TCON_AED_SENSOR_OFF 값이 0 으로 설정되는 것을 방지하기위해 하기 코드 추가
	if(aed_power_onoff == false)
        m_p_fpga_c->write(eFPGA_REG_TCON_AED_SENSOR_OFF, 1);    

    //Compare with current status and Apply if required
    print_log(INFO, 1, "[%s] AED power %s (drive_mode: %d, trigger: %d, smart W: %s)\n", m_p_log_id,
              aed_power_onoff ? "ON" : "OFF", i_acq_mode, i_e_trig, i_c_smart_w_enable ? "Enable" : "Disable");
    
    //AED POWER OFF --> ON Case
    if(aed_power_onoff)
    {
        m_p_hw_mon_c->aed_power_enable(true);
        m_p_aed_c->start();             //해당 함수에서 thread 생성 및 AED sensor 안정화 대기 및 FPGA가 AED 신호 설정하도록 설정
        aed_wait_end();                 //현재 thread의 aed sensor 안정화 까지 대기 로직
        m_p_fpga_c->write(eFPGA_REG_TCON_AED_SENSOR_OFF, 0);
    }
    else    //AED POWER ON --> OFF Case
    {
        m_p_aed_c->stop();
        m_p_hw_mon_c->aed_power_enable(false);
        
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param	
 * @return  
 * @note	
 *******************************************************************************/
GEN_INTERFACE_TYPE CVWSystem::get_acq_mode(void)
{
    return m_p_fpga_c->get_acq_mode();
}

/******************************************************************************/
/**
 * @brief   GYRO 실장 HW: Gyro calibration + impact 1면 calibration start
 * 		    GYRO 미실장 HW: impact 6면 calibration start
 * @param	
 * @return  
 * @note	
 *******************************************************************************/
void CVWSystem::impact_calibration_start(void)
{
    m_p_hw_mon_c->impact_calibration_start();      
}

/******************************************************************************/
/**
 * @brief   GYRO 실장 HW: Gyro calibration + impact 1면 calibration start
 * 		    GYRO 미실장 HW: impact 6면 calibration start
 * @param	
 * @return  
 * @note	
 *******************************************************************************/
void CVWSystem::impact_calibration_stop(void)
{
    m_p_hw_mon_c->impact_calibration_stop();
}

/******************************************************************************/
/**
 * @brief   GYRO 실장 HW, GYRO 미실장 HW 공용
 * @param	
 * @return  
 * @note	
 *******************************************************************************/
u8 CVWSystem::impact_calibration_get_progress(void)
{
    return m_p_hw_mon_c->impact_calibration_get_progress();
}

/******************************************************************************/
/**
 * @brief   부팅 이후, 가장 최근에 수행된 main sensor offset calibration 대한 결과를 반환하는 함수
 * @param	
 * @return  
 * @note	
 *******************************************************************************/
bool CVWSystem::get_main_angular_offset_cal_process_result(void)
{
    if(m_p_hw_mon_c->is_gyro_exist())
        return m_p_hw_mon_c->gyro_calibration_get_result();
    else
        return m_p_hw_mon_c->impact_calibration_get_result();
}

/******************************************************************************/
/**
 * @brief   부팅 이후, 가장 최근에 수행된 sub sensor offset calibration 대한 결과를 반환하는 함수
 * @param	
 * @return  
 * @note	
 *******************************************************************************/
bool CVWSystem::get_sub_angular_offset_cal_process_result(void)
{
    if(m_p_hw_mon_c->is_gyro_exist())
        return m_p_hw_mon_c->impact_calibration_get_result();
    else
        return true;
}

/******************************************************************************/
/**
 * @brief   check if it needs to block recharging
 * @param	
 * @return  
 * @note	Battery Swelling 단기 대책, if message is received, not to accept
 *******************************************************************************/
void CVWSystem::check_need_to_block_recharging(void)
{
    // if(m_b_is_charge_block == true)
    // {
    //     /*msg 처리 플래그가 1이라면 지금 수행 중이라고 하고 return*/
    //     return;
    // }
    
    // /*msg 처리 플래그가 0이라면 hw_monitor에게 block_to_recharge() 수행하라고 시킴*/
    // m_b_is_charge_block = true;        
    //print_log(INFO, 1, "##########%s\n", __func__ );
    m_p_hw_mon_c->blinking_charge_led_proc_start();
    //m_p_hw_mon_c->block_to_recharge();
}

/******************************************************************************/
/**
 * @brief   recharge 행동(blinking LED) 강제 중지
 * @param	
 * @return  
 * @note	Battery Swelling 단기 대책
 *******************************************************************************/
void CVWSystem::force_to_stop_to_block_recharging(void)
{
    /*강제 종료 cmd를 hw_montior애 날리기*/
    // if(m_b_is_charge_block == false)
    //     return;
    
    //m_b_is_charge_block = false;
    m_p_hw_mon_c->blinking_charge_led_proc_stop();
}


/******************************************************************************/
/**
 * @brief   
 * @param	
 * @return  
 * @note	
 *******************************************************************************/
u32 CVWSystem::get_battery_charging_mode(void)
{
    BAT_CHG_MODE chg_mode;
    chg_mode = m_p_hw_mon_c->get_battery_charging_mode();
    printf("get battery charging mode: %d\n", chg_mode);

    return (u32)chg_mode;
}

/******************************************************************************/
/**
 * @brief   
 * @param	
 * @return  
 * @note	
 *******************************************************************************/
void CVWSystem::set_battery_charging_mode(u32 i_bat_chg_mode)
{
    BAT_CHG_MODE chg_mode;
    chg_mode = (BAT_CHG_MODE)i_bat_chg_mode;
    m_p_hw_mon_c->set_battery_charging_mode(chg_mode);
    m_p_env_c->save_battery_charge_info(&chg_mode, NULL);

    printf("set recharging mode: %d\n", chg_mode);
}

/******************************************************************************/
/**
 * @brief   
 * @param	
 * @return  
 * @note	
 *******************************************************************************/
u8 CVWSystem::get_battery_recharging_gauge(void)
{
    u8 c_rechg_gauge = m_p_hw_mon_c->get_battery_recharging_gauge();
    printf("get recharging gauge: %d%%\n", c_rechg_gauge);
    return c_rechg_gauge;
}

/******************************************************************************/
/**
 * @brief   
 * @param	
 * @return  
 * @note	
 *******************************************************************************/
void CVWSystem::set_battery_recharging_gauge(u8 i_rechg_gauge)
{
    u8 c_rechg_gauge = i_rechg_gauge;

    m_p_hw_mon_c->set_battery_recharging_gauge(c_rechg_gauge);
    m_p_env_c->save_battery_charge_info(NULL, &c_rechg_gauge);

    printf("set recharging gauge: %d%%\n", c_rechg_gauge);
}

/******************************************************************************/
/**
 * @brief   check rechaging gauge is out of range
 * @param	
 * @return  
 * @note	
 *******************************************************************************/
bool CVWSystem::is_recharging_gauge_out_of_range(u8 i_rechg_gauge)
{
    if(i_rechg_gauge < RECHARGING_GAUGE_MIN || i_rechg_gauge > RECHARGING_GAUGE_MAX)
    {
        return true;
    }

    return false;
}

bool CVWSystem::IsReadoutSyncDefined(void)
{
    if (m_p_fpga_c->GetReadoutSync() == 0xDEAD)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool CVWSystem::GetCurrentOffsetCorrectionMode(void)
{
    if (m_b_offset_cal_started == false && m_b_osf_is_running == false)
    {
        if (m_p_fpga_c->read(eFPGA_REG_DRO_IMG_CAPTURE_OPTION, true) == 1) 
        {
            return true; 
        }
    }	

	return false; 
}
