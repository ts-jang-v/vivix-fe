/*
 * comm.h
 *
 *  Created on: 2013. 12. 17.
 *      Author: myunghee
 */

 
#ifndef _HWMONITOR_H_
#define _HWMONITOR_H_

/*******************************************************************************
*	Include Files
*******************************************************************************/
#include "typedef.h"
#include "message.h"
#include "env.h"
#include "mutex.h"
#include "message_def.h"
#include "vw_system_def.h"
#include "vw_time.h"
#include "vw_i2c.h"
#include "impact.h"
#include "micom.h"

using namespace std;

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/


/*******************************************************************************
*	Type Definitions
*******************************************************************************/
typedef struct _image_size
{
    u32 width;
    u32 height;
    u32 pixel_depth;
} image_size_t;

typedef struct _battery_charging_mode_info
{
    BAT_CHG_MODE chg_mode;
    u8 c_rechg_gauge;
    bool c_block_recharging;    // the flag that the detector blocks recharging now
} bat_chg_mode_info;

typedef struct _battery_t
{
    u16     w_vcell;
    u16     w_soc;
    u16     w_soc_raw;
    u16     w_level;
    bool    b_charge;
    bool    b_equip;
    bool    b_bid_equip;
    bool    b_power_warning;
    bool    b_power_off;

    u8		c_charger_state;
}__attribute__((packed))  battery_t;

/**
 * @brief   Battery level
 */
typedef enum
{
    eBAT_SOC_STATE_1   = 1,
    eBAT_SOC_STATE_2   = 2,
    eBAT_SOC_STATE_3   = 3,
    eBAT_SOC_STATE_4   = 4,
    eBAT_SOC_STATE_5   = 5,

    eBAT_SOC_STATE_WARN,
    eBAT_SOC_STATE_OFF,
    
} BAT_SOC_STATE;

typedef enum
{
    eBAT_CHARGE_IN_PROG = 1,
    eBAT_CHARGE_COMPLETE = 2,
    eBAT_CHARGE_SUSPEND = 3,
    
} BAT_CHARGE_STATE;

typedef enum
{
    eCHARGER_DISABLED = 0,
    eCHARGE_IN_PROGRESS = 1,
    eCHARGE_COMPLETE = 2,
    eCHARGE_SUSPEND = 3,
    
} CHARGER_STATE;

/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*	Variable Definitions
*******************************************************************************/
#define  MARVELL_PHY_OUI					(0x00005043)
#define  MARVELL_PHY_88E1510_MODEL_NUM 		(0x001D)
#define	 ADI_PHY_OUI						(0x0000A0EF)
#define	 ADI_PHY_1300_MODEL_NUM				(0x0003)

#define RECHARGING_GAUGE_MIN                (50)
#define RECHARGING_GAUGE_MAX                (95)

/*******************************************************************************
*	Function Prototypes
*******************************************************************************/
void*         	battery_routine( void* arg );
void *          blinking_charge_led_routine( void * arg ); 

class CHWMonitor
{
private:
    
    s32			        (* print_log)(s32 level, s32 print, const s8* format, ...);
    s8                  m_p_log_id[15];
    
    // thread
    bool                m_b_is_running;
    bool                m_b_is_blinking_running;
    
    //phy
    PHY_TYPE            m_phy_type;
    PHY_TYPE            phy_discovery(void);
    
    // battery
    s32                 m_p_bat_full_soc[eBATTERY_MAX];
    s32                 m_p_bat_empty_soc[eBATTERY_MAX];
    battery_t           m_battery_t[eBATTERY_MAX];
    s8					m_c_battery_num;
    CTime*              m_p_battery_low_time_c;
    CTime*              m_p_battery_off_time_c;
    CTime*				m_p_charging_start_timer_c;
    bool                m_b_battery_power_off;
       
    u8					m_c_aed_num;
    u32                 m_b_bat_equip_denoise_counter[eBATTERY_MAX];
    
    void                bat_check_charger_state(void);
    bool                bat_is_power_off(void);
    bool                bat_is_soc_empty(void);
    void                bat_check_low_volt(void);
	u16                 bat_get_soc( BATTERY_ID i_id_e, u16 i_w_raw_soc );
	
	void                bat_init(void);
	void                bat_check_initial_vcell(void);
	bool				bat_is_valid_vcell( u16 i_w_vcell );
    bool                bat_is_valid_soc_raw( u16 i_w_vcell, u16 i_soc_raw );
	void 				bat_check_charge_change(void);

    void                check_pwr_src_and_block_recharge(POWER_SOURCE cur_pwr_src, POWER_SOURCE old_pwr_src);
	
    // battery thread
    pthread_t	        m_bat_thread_t;
    void                battery_proc(void);
    
    // blinking_charge_led thread
    pthread_t	        blinking_charge_led_thread_t;
    void                blinking_charge_led_proc(void);
    
    void                check_shutdown_by_scu(void);
    
    // h/w information
    MODEL               m_model_e;
    image_size_t        m_image_size_t;
    hw_config_t         m_hw_config_t;
    
    // impact sensor
	CImpact*			m_p_impact_c;
    
    // monitoring
    s32                 m_n_fd;
    s32                 m_n_adc_fd;
    
        
    // true: 촬영 중(exp_req 처리 불가), false: 촬영 가능(exp_req 처리 가능)
    bool                m_b_exp_req;
    
    // notification
    bool                (* notify_sw_handler)(vw_msg_t * msg);
    void                message_send( MSG i_msg_e, u8* i_p_data=NULL, u16 i_w_data_len=0 );
    
    // power voltage
    power_volt_t        m_power_volt_t;
    
    // AP button state
    bool                m_b_ap_button_on;
    bool                m_b_ap_button_pressed;
    CTime*              m_p_ap_button_wait_c;
    void                ap_button_check(void);
    
    u16					m_w_impact_count;

    bool				m_b_phy_on;
    u8					m_c_led_blink_enable;
    
    bool				m_b_pwr_src_once_connected;
    
    bat_chg_mode_info   m_st_bat_chg_mode_info;
    
    bool				is_power_source_once_connected(void);
    void                get_adc_from_risc(u16* i_p_adc, power_volt_t* i_p_volt_t, float* i_p_raw_volt, float* i_p_result);
    void                get_adc_from_micom(u16* i_p_adc, power_volt_t* i_p_volt_t, float* i_p_raw_volt, float* i_p_result);

    void controlLedBlinkingWithMicom(u8 enable);
    void controlLedBlinkingWithGpio(u8 enable);
protected:
public:
    
	CHWMonitor(s32 (*log)(s32, s32, const s8* ,...));
	CHWMonitor(void);
	~CHWMonitor();
	
	friend void *       battery_routine( void * arg );
    friend void *       blinking_charge_led_routine( void * arg );    
  	CMicom*             m_p_micom_c;
  
    // settings
    CVWI2C*             m_p_i2c_c;
	CVWSPI*				m_p_spi_c;
    DEV_ERROR           initialize(MODEL i_model_e, u16 i_w_bat_count, BAT_CHG_MODE i_chg_mode, u8 c_rechg_gauge);
    void                set_config( hw_config_t i_hw_config_t );
    
    s32					get_gpio_fd(void){return m_n_fd;}
    s32					get_spi_fd(void){return m_n_adc_fd;}
    // h/w information
    MODEL               get_model(void){return m_model_e;}
    void                get_image_size_info( image_size_t* o_p_image_size_t );
    u8                  hw_get_ctrl_version(void);
    bool                is_vw_revision_board(void);
    u16                 get_temperature(void);
    void                temp_reset(void){m_p_i2c_c->temper_sensor_reset();}
    u16                 get_humidity(void){return m_p_i2c_c->get_humidity();}
    POWER_SOURCE        get_power_source(void);
    
	// battery
	u8					get_battery_count(void);
	void                bat_set_soc_factor( u32 i_n_idx, s32 i_n_full, s32 i_n_empty ){m_p_bat_full_soc[i_n_idx]=i_n_full;m_p_bat_empty_soc[i_n_idx]=i_n_empty;}
	void                bat_print(void);
    void                bat_rechg_print(void);
	void                get_battery( battery_t* o_p_battery_t, bool i_b_direct=false );
	bool                bat_diagnosis(u8 i_c_sel);
    void                bat_sw_quick_start(u8 i_c_sel){ m_p_i2c_c->bat_quick_start(i_c_sel); };
    bool                bat_is_soc_full(void);

	void                bat_charge( BAT_CHARGE i_e_charge_state );
	
	bool                bat_is_equipped(u8 i_c_sel);
	u8					bat_get_equip_num(void);
	BAT_SOC_STATE       bat_get_soc_state(u16 i_w_soc);
	void                bat_notify_low_volt(void);
	u8					get_bat_equip_start_idx(void);
    void                bat_set_remain_SOC_range(s32 i_n_empty_soc, s32 i_n_full_soc);
    u16                 get_battery_gauge(void);
	
	BAT_SOC_STATE 		bat_get_state(void);
    void				restore_charging_state(void);
    void                set_charging_state_full(bool i_b_on);
    
    void				battery_proc_start(void);
    void				battery_proc_stop(void);
   
    bool				power_source_can_charge(POWER_SOURCE i_pwr_src_e);
        
    // ADC
    void                adc_print(void);
    void                get_power_volt(power_volt_t* o_p_pwr_volt_t, bool i_b_direct=false, bool i_b_print=false);

    // GYRO SENSOR 
    void                get_gyro_position( s32* o_p_x, s32* o_p_y, s32* o_p_z ){m_p_impact_c->get_gyro_position(o_p_x, o_p_y, o_p_z);}
    void                get_gyro_offset(gyro_cal_offset_t* o_p_gyro_offset_t){m_p_impact_c->get_gyro_offset(o_p_gyro_offset_t);}
    void                set_gyro_offset(gyro_cal_offset_t* i_p_gyro_cal_offset_t){m_p_impact_c->set_gyro_offset(i_p_gyro_cal_offset_t);}
    bool				acc_gyro_probe(void){return m_p_spi_c->acc_gyro_probe();}

	// IMPACT SENSOR (ACCELEROMETER)
	u8					get_impact_reg(u8 i_c_addr){return m_p_impact_c->reg_read(i_c_addr);}
    void				set_impact_reg(u8 i_c_addr , u8 i_c_data) {m_p_impact_c->reg_write(i_c_addr,i_c_data);}
    void                impact_print(void){m_p_impact_c->impact_print();}
    u8                	impact_get_threshold(void){return m_hw_config_t.c_impact_threshold;}
    void                impact_set_threshold(u8 i_c_threshold){m_p_impact_c->impact_set_threshold(i_c_threshold);}
    void                impact_log_save(int i_n_cnt){m_p_impact_c->impact_log_save(i_n_cnt);}
    void                read_detector_position(void);
    
    void                impact_get_pos( s32* o_p_x, s32* o_p_y, s32* o_p_z ){m_p_impact_c->impact_read_pos(o_p_x, o_p_y, o_p_z);}
    void                impact_get_offset(impact_cal_offset_t* o_p_imapct_cal_offset){return m_p_impact_c->get_impact_offset(o_p_imapct_cal_offset);}
    void                impact_calibration_start(void){m_p_impact_c->impact_calibration_start();}
    void                impact_calibration_stop(void){m_p_impact_c->impact_calibration_stop();}
    u8                  impact_calibration_get_progress(void){return m_p_impact_c->impact_calibration_get_progress();}
    bool                impact_calibration_get_result(void){return m_p_impact_c->impact_calibration_get_result();}
    bool                gyro_calibration_get_result(void){return m_p_impact_c->gyro_calibration_get_result();}
    bool				impact_diagnosis(void){return m_p_impact_c->impact_diagnosis();}
  
    u16					get_impact_count(void){return m_p_impact_c->get_impact_count();}
    void				clear_impact_count(void){m_p_impact_c->clear_impact_count();}

    void				set_impact_extra_detailed_offset(impact_cal_extra_detailed_offset_t* i_p_impact_cal_extra_detailed_offset){m_p_impact_c->set_impact_extra_detailed_offset(i_p_impact_cal_extra_detailed_offset);}
    void				get_impact_extra_detailed_offset(impact_cal_extra_detailed_offset_t* o_p_impact_cal_extra_detailed_offset){m_p_impact_c->get_impact_extra_detailed_offset(o_p_impact_cal_extra_detailed_offset);}
    void                impact_stable_pos_proc_start(void){m_p_impact_c->stable_pos_proc_start();}
    void                impact_stable_pos_proc_stop(void){m_p_impact_c->stable_pos_proc_stop();}
    bool				impact_stable_proc_is_running(void){return m_p_impact_c->is_stable_proc_is_running();}
    void				impact_stable_proc_hold(void){m_p_impact_c->m_b_stable_proc_hold = true;}
    void				impact_stable_proc_restart(void){m_p_impact_c->m_b_stable_proc_hold = false;}
    void				impact_set_cal_direction(ACC_DIRECTION i_dir_e){m_p_impact_c->set_cal_direction(i_dir_e);}
    void				load_6plane_impact_sensor_cal_done(u8 i_c_done){m_p_impact_c->load_6plane_sensor_cal_done(i_c_done);}
    u8					get_6plane_impact_sensor_cal_done(void){return m_p_impact_c->get_6plane_sensor_cal_done();}
    u8					get_6plane_impact_sensor_cal_misdir(void){return m_p_impact_c->get_6plane_sensor_cal_misdir();}
    bool                is_gyro_exist(void){return m_p_impact_c->is_gyro_exist();};

    // notification
    void                set_notify_sw_handler(bool (*noti)(vw_msg_t * msg));
    
    // AP button
    void                ap_button_set( bool i_b_on ){m_b_ap_button_on = i_b_on;}
    bool                ap_button_get(void){return m_b_ap_button_on;}
    bool                ap_button_is_pressed(void){return m_b_ap_button_pressed;}
    void                ap_button_clear(void);
    bool                ap_button_get_raw(void);
	
	// POWER button
	bool				pwr_button_get_raw(void);
	    
    // UART display
    void 				set_uart_display(bool i_b_en);
     
    // pmic
    void				get_pmic_reg(u8 a_b_addr,u8 *a_p_reg){m_p_i2c_c->pmic_read_reg(a_b_addr,a_p_reg);}
    void				set_pmic_reg(u8 a_b_addr,u8 a_b_reg){m_p_i2c_c->pmic_write_reg(a_b_addr,a_b_reg);}
    
    void 				phy_reset(void);
    void				phy_down(void);
    void				phy_up(void);
    bool				phy_is_up(void){return m_b_phy_on;}
    void                phy_init(void);

    bool				is_no_battery(void) {return m_battery_t[eBATTERY_0].b_equip == false && m_battery_t[eBATTERY_1].b_equip == false;}
    
    void 				gpio_pin_ss(u8 i_c_ctrl);
	void 				gpio_pin_rs(u8 i_c_ctrl);
	void 				gpio_pin_reset(u8 i_c_ctrl);
	void 				gpio_pin_pd(u8 i_c_ctrl);
	
	bool 				gpio_get_tether_equip(void);
    bool 				gpio_get_adaptor_equip(void);
	void 				aed_power_enable( bool i_b_on );
    bool                aed_power_en_status(void);
    
	void				vdd_core_enable( bool i_b_on );
    //aed
	void				boot_led_on(void);
    void                micom_boot_led_on(void);
	void				boot_end(void);
	u8					get_aed_count(void);
    void				set_aed_count(MODEL i_model_e);
    
	void				adc_read_test(u8 i_c_ch);
	void				pmic_reset(void);
	void				hw_power_off(void);
	
	void				power_led_ctrl(u8 i_c_enable);
	u8					get_power_led_ctrl(void){return m_c_led_blink_enable;}
	void				boot_led_ctrl(u8 i_c_enable);
	
    //phy model 
    PHY_TYPE            get_phy_model_type(void){ return m_phy_type;}

    //battery swelling 단기 대책
    void                blinking_charge_led_proc_start(void);
    void                blinking_charge_led_proc_stop(void);

    BAT_CHG_MODE        get_battery_charging_mode(void);
    void                set_battery_charging_mode(BAT_CHG_MODE i_bat_chg_mode);
    u8                  get_battery_recharging_gauge(void);
    void                set_battery_recharging_gauge(u8 i_rechg_gauge);

    void                check_pwr_src_and_block_recharging(POWER_SOURCE cur_pwr_src, POWER_SOURCE old_pwr_src);
    void                check_pwr_src_is_battery_and_charge_stop(POWER_SOURCE cur_pwr_src, POWER_SOURCE old_pwr_src);
    void                check_battery_gauge_to_recharge(void);
    void                check_soc_is_full(void);
    void                print_block_charge_change(void);
};

#endif /* end of _HWMONITOR_H_*/

