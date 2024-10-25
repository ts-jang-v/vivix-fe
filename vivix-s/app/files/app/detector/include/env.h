/*
 * env.h
 *
 *  Created on: 2013. 12. 17.
 *      Author: myunghee
 */

 
#ifndef _DET_ENV_H_
#define _DET_ENV_H_

/*******************************************************************************
*	Include Files
*******************************************************************************/
#include "typedef.h"
#include "vw_system_def.h"
#include "vw_time.h"

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/
#define MAX_CFG_COUNT	(31)
#define STR_LEN         (64)
#define DEFAULT_DIGITAL_OFFSET 100 // 0x0048 Digital offset initial value -> m_p_reg[0x48] = 100;
#define	TEMPORARY_VER_PATH	"/tmp/version"

/*******************************************************************************
*	Type Definitions
*******************************************************************************/
enum
{
	eST_EEP_SUCCESS= 0,
	eST_EEP_IDLE,
	eST_EEP_BUSY,
	eST_EEP_TIMEOUT,
	eST_EEP_ADDR_INVALID,
	eST_EEP_RANGE_OVERFLOW,
	eST_EEP_PAGE_EXCEED,
	eST_EEP_IOCTL_FAIL,
	eST_EEP_NOT_SUPPORTED,
};

/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*	Variable Definitions
*******************************************************************************/
#define EEPROM_TOTAL_SIZE					(8 * 1024)
#define EEPROM_PAGE_SIZE					32

// ROIC_PWR_EN 신호 enable 직후부터 ROIC, GON, GOFF 전압 들어오기까지 실험 측정 치 - 약 250ms 
#define VW_ROIC_GATE_POWER_UP_TIME	        500

/*******************************************************************************
*	Function Prototypes
*******************************************************************************/

class CEnv
{
private:
    int					(* print_log)(int level,int print,const char * format, ...);
	s8					m_p_log_id[15];
    
    s8					m_p_config[MAX_CFG_COUNT][eCFG_MAX][STR_LEN];
    
    MODEL				m_model_e;
    ROIC_TYPE           m_roic_model_type_e;
    bool				m_b_preset;
    u8                  m_c_hw_ver;

    // 제조 정보
    s32                 m_n_eeprom_fd;
    manufacture_info_t  m_manufacture_info_t;
    void                manufacture_info_read(void);
    void                manufacture_info_write(void);
    void                manufacture_info_default(void);

    void 				effective_area_default(void);
    void				check_manufacture_battery_count(void);
    void				check_manufacture_effective_area(void);
    void                check_scintillator_type_and_update(void);
    
    // 운용 정보
    operation_info_t    m_oper_info_t;
    void                oper_info_write(void);
    
    // 고객사 customizing 정보
    customized_info_t   m_customized_info_t;
    void            	customized_info_read(void);
    void                customized_info_write(void);
    
    u32					eeprom_page_write(u16 i_w_addr, u8* i_p_data, u32 i_n_len);
    u32					eeprom_page_read(u16 i_w_addr, u8* o_p_data, u32 i_n_len);
    
    // version
    void                version_read(void);
    
    void				config_dhcp_update(void);
    
    void				print_xml(void);
    void				print_current(void);
    DEV_ERROR       	config_load_xml(const s8* i_p_file_name);
    
    void				config_default_save(void);
	
	bool				preset_load(const s8* i_p_file_name);
    bool                is_vw_revision_board(void);
protected:
public:
    
	CEnv(int (*log)(int,int,const char *,...));
	CEnv(void);
	~CEnv();
    
    DEV_ERROR       initialize(u8 i_c_hw_ver);

    // manufacture info
    void            manufacture_info_get( manufacture_info_t* o_p_info_t );
    void            manufacture_info_set( manufacture_info_t* i_p_info_t );
    void            manufacture_info_print(void);
    void            manufacture_info_reset(void);
    void            set_model(void);
    MODEL			get_model(void){return m_model_e;}
    ROIC_TYPE       discover_roic_model_type(void);
    ROIC_TYPE   	get_roic_model_type(void);
    
    // operation info
    void            oper_info_read(void);
    void            oper_info_reset(void);
    void            oper_info_print(void);
    u32             oper_info_get_capture_count(void){return m_oper_info_t.n_capture_count;}
    void            oper_info_capture(void);
    
    void            config_default(void);
    void            config_default_network(void);
    
    DEV_ERROR       config_load(const s8* i_p_file_name, bool is_reload = false);
    DEV_ERROR       config_save(const s8* i_p_file_name);
    DEV_ERROR       config_load_factory(void);
    s8*             config_get( CONFIG i_cfg_e );
    void            config_set( CONFIG i_cfg_e, const s8* i_p_value );
    bool            config_net_is_changed(const s8* i_p_file_name);
    
    // network settings
    bool            net_is_ap_enable(void);
    bool            net_is_dhcp_enable(void);
    bool            net_use_only_power(void);
    bool            net_is_valid_channel( u16 i_w_num );
    
    void            get_mac( u8* o_p_mac );
    void            set_mac( u8* i_p_mac );
    u32             get_ip(void);
    u32             get_subnet(void);
    
    const s8*       get_model_name(void);
    const s8*       get_device_version(void);
    const s8*       get_serial(void);
    
    SCINTILLATOR_TYPE   get_scintillator_type_default(void);
    
    bool            preview_get_enable(void);
    bool            tact_get_enable(void);
    
    void            print(void);
    
    bool            model_is_wireless(void);
    bool			model_is_csi(void);
    bool            load_auto_offset( auto_offset_t* o_p_offset_t );
    
    bool			change_preset_id( const s8* i_p_tag_id, bool* o_p_net_changed );
	s32				get_preset_index( const s8* i_p_ssid );
	s8*				get_preset_name( const s8* i_p_tag_id );
	
	void			preset_load(void);
	bool			preset_change( const s8* i_p_tag_id );
	
	bool			is_lower_grid_line_support(void);
	
	bool			is_backup_enable(void);
	bool			is_wired_detection_on(void);
	
	void			aed_info_read(aed_info_t* o_p_aed_t);
	void			aed_info_write(aed_info_t* i_p_aed_t);

	impact_cal_offset_t*	get_impact_offset(void);
	void					save_impact_offset(impact_cal_offset_t* i_p_impact_offset_t);

	gyro_cal_offset_t*	get_gyro_offset(void);
	void				save_gyro_offset(gyro_cal_offset_t* i_p_gyro_offset_t);

	u32                 eeprom_read( u16 i_w_addr, u8* o_p_data, u32 i_n_len );
    u32                 eeprom_write( u16 i_w_addr, u8* i_p_data, u32 i_n_len );
    
    u32                 (*micom_eeprom_read)( u16 i_w_addr, u8* o_p_data, u32 i_n_len, eeprom_blk_num_t i_n_eeprom_blk_num );
    u32                 (*micom_eeprom_write)( u16 i_w_addr, u8* i_p_data, u32 i_n_len, eeprom_blk_num_t i_n_eeprom_blk_num );
    //bool			    is_plus_model(void);
    
    SUBMODEL_TYPE       get_submodel_type(void);
    PANEL_TYPE		    get_panel_type(void);
    SCINTILLATOR_TYPE	get_scintillator_type(void);

    
    APBUTTON_TYPE	get_ap_button_type(void);
    bool			is_preset_loaded(void);
    
    u16				get_battery_count(void);
    u16				get_battery_slot_count(void);
    
    impact_cal_extra_detailed_offset_t  get_impact_extra_detailed_offset(void);
    void			save_impact_extra_detailed_offset(impact_cal_extra_detailed_offset_t* i_p_impact_cal_extra_detailed_offset);
    
    u8				get_impact_6plane_cal_done(void);
    void			save_impact_6plane_cal_done(u8 i_c_done);
	
    u8				get_wifi_signal_step_num(void);
    void			save_wifi_signal_step_num(u8 i_c_wifi_step_num);

    u32             verify_manufacture_info(manufacture_info_t* i_manufacture_info_t);

    bool			_check_exceed(s8* i_p_token, s32 i_n_compare_num);
    bool			_check_compatibility_package_version(s8* i_p_sdk_str);

    void            save_battery_charge_info(BAT_CHG_MODE* i_p_bat_chg_mode, u8* i_p_recharging_gauge);
    BAT_CHG_MODE    get_battery_charging_mode_in_eeprom(void);
    u8              get_battery_recharging_gauge_in_eeprom(void);
};


#endif /* end of _DET_ENV_H_*/

