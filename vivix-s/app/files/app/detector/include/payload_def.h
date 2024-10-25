/*
 * gige.h
 *
 *  Created on: 2013. 12. 17.
 *      Author: myunghee
 */

 
#ifndef PAYLOAD_DEF_H_
#define PAYLOAD_DEF_H_

/*******************************************************************************
*	Include Files
*******************************************************************************/
#include "typedef.h"

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/

// request from s/w to detector     
#define PAYLOAD_LOG_IN                          0x0100
#define PAYLOAD_LOG_OUT                         0x0101
#define PAYLOAD_WIRELESS_CONFIG_RESET           0x0102
#define PAYLOAD_REBOOT                          0x0103
#define PAYLOAD_MANUFACTURE_INFO                0x0104
#define PAYLOAD_PACKAGE_VERSION                 0x0105
// multi-packet
#define PAYLOAD_UPLOAD                          0x01A0  // S/W -> Detector/SCU
#define PAYLOAD_DOWNLOAD                        0x01A1  // S/W <- Detector/SCU

#define PAYLOAD_MEMORY_CLEAR                    0x01A2

// function list
#define PAYLOAD_FUNCTION_LIST                   0x01F0

// Detector only
#define PAYLOAD_STATE                           0x0200
#define PAYLOAD_SLEEP_CONTROL                   0x0201
#define PAYLOAD_DEVICE_REGISTER                 0x0202

#define PAYLOAD_DARK_IMAGE                      0x0210
#define PAYLOAD_PREVIEW_ENABLE                  0x0211
#define PAYLOAD_RESEND_IMAGE                    0x0212
#define PAYLOAD_IMAGE_TRANS_DONE                0x0213
#define PAYLOAD_BACKUP_IMAGE_DELETE             0x0214
#define PAYLOAD_SELF_DIAGNOSIS                  0x0215
#define PAYLOAD_ACCELERATION_CONTROL            0x0216
#define PAYLOAD_AP_INFORMATION		            0x0217
#define PAYLOAD_WEB_CONFIGURATION			    0x0218
#define PAYLOAD_PACKET_TRIGGER				    0x0219
#define PAYLOAD_IMAGE_RETRANSMISSION		    0x021A
#define PAYLOAD_SENSITIVITY_CALIBRATION		    0x021B
#define PAYLOAD_GAIN_COMPENSATION_CONTROL	    0x021C

#define PAYLOAD_CALIBRATION                     0x0220
#define PAYLOAD_OFFSET_DRIFT                    0x0221
#define PAYLOAD_OFFSET_CONTROL                  0x0222
#define PAYLOAD_CAL_DATA_CONTROL                0x0223
#define PAYLOAD_NFC_CARD_CONTROL                0x0224
#define PAYLOAD_WIRELESS_CHARGE_CONTROL		    0x0225
#define PAYLOAD_PROMOTE_USER_DATA               0x0226      //NOT USING
#define PAYLOAD_SMART_W_CONTROL		            0x0227
#define PAYLOAD_AED_OFF_DEBOUNCE_CONTROL	    0x0228
#define PAYLOAD_BATTERY_CHARGING_MODE           0x0229

#define PAYLOAD_TRANSMISSION_TEST               0x0230
#define PAYLOAD_AP_CHANNEL_CHANGE               0x0231
#define PAYLOAD_AP_CHANNEL_CHANGE_RESULT        0x0232

#define SCINTILLATOR_TYPE_RANGE_ADVERTISEMENT   0x0263
#define PAYLOAD_OFFSET_CORRECTION_CTRL          0x0265
#define PAYLOAD_EXPOSURE_CONTROL                0x0269

// notification detector -> s/w
#define PAYLOAD_NOTI_POWER_MODE                 0x0A01
#define PAYLOAD_NOTI_OFFSET_CAL_STATE           0x0A02
#define PAYLOAD_NOTI_IMAGE_ACQ_STATE            0x0A03
#define PAYLOAD_NOTI_OFFSET_DRIFT               0x0A04

// stream
#define PAYLOAD_PREVIEW_IMAGE                   0xAA00
#define PAYLOAD_IMAGE                           0xAA01
#define PAYLOAD_OFFSET_DATA                     0xAA02
#define PAYLOAD_TEST_IMAGE_DATA                 0xAA03
#define PAYLOAD_TEST_DATA                       0xAA04


/*******************************************************************************
*	Type Definitions
*******************************************************************************/
typedef struct
{
	u8     	p_head[4];              // VWP1
	u32		n_seq_id;
	u16     w_payload;
	u8      c_operation;            // read/write/save
	u8      c_result;
}__attribute__((packed)) packet_header_t;   //VWP1 packet header


#define PACKET_DATA_SIZE    (1434)
#define PACKET_IMAGE_SIZE   (PACKET_DATA_SIZE - 8)
#define PACKET_HEAD         ("VWP1")
#define PACKET_SIZE         (PACKET_DATA_SIZE + (signed)sizeof(packet_header_t))
#define RESEND_PAIR_MAX     ((PACKET_DATA_SIZE - 8)/4)

typedef struct _login_t
{
    u32     n_time;
    u16     w_interval_time;
    u8      p_encrypted_id[8];
    u16     w_setup;           // 1: setup, 0: viewer
    s8      p_sw_version[32];
    
}__attribute__((packed)) login_t;

typedef union _u8data_u
{
	u8 U8DATA;
	struct
	{
		u8	SUPPORT : 1;
		u8	DATA	: 7;
	}BIT;
}u8data_u;

typedef union _u16data_u
{
	u16 U16DATA;
	struct
	{	
		u16	SUPPORT : 1;
		u16	DATA	: 15;
	}BIT;
}u16data_u;  	

typedef struct _login_ack_t
{
    u16     w_width;
    u16     w_height;
    u16     w_pixel_depth;
    u16     w_backup_image_count;
    u16     w_offset_state;             // 0: ready, 1: not ready
    u16     w_hw_version;
    u16     w_board_config;
    s8      p_rfs_version[32];
    s8      p_model_name[32];
    s8      p_serial_number[32];
    s8      p_bootloader_version[32];
    s8      p_kernel_version[32];
    s8      p_fpga_version[32];
    s8      p_fw_version[32];
    u32     n_company_code;
    s8      p_package_info[32];
    s8      p_mac_address[32];
    s8      p_micom_version[32];
    u16     p_effective_area[4];
    s8      p_aed_version[32];
    u16     w_power_mode;
    s8      p_cur_mac_addr[32];
    u8 		c_aed_count;
    u16 	w_roic_type_ver_0;                    //0: ADI ROIC & SDK에서 gain type --> IFS 변환  1: TI 2256 ROIC & Detector에서 gain type --> IFS 변환 
    u16		w_bat_count;
    u16		w_config_file_type;                   //eCONFIG = 1       eCONFIG_PRESET = 2
    u16		w_state_type;                         //eSTATE_N = 1      eSTATE_VW = 2,
    u8 		c_exp_time_data_format;               /* 0: Unknown(Default 16bit format 1: 16bit 2: 32bit */
    u8		c_gain_type_num;
    s8      p_fpga2_version[32];
    u8      c_gate_state_type;
    u8      c_roic_type_identification_ver;       //0: w_roic_type_old 참조 1: w_roic_type_new 참조
    u8      c_roic_type_ver_1;                    //0: ADI 1255 1: TI 2256GR 2:TI 2256TD
}__attribute__((packed)) login_ack_t;


typedef struct _dev_reg_addr_t
{
    u16     w_start;
    u16     w_end;
}__attribute__((packed)) dev_reg_addr_t;

typedef struct _dev_reg_t
{
    u16     w_addr;
    u16     w_data;
}__attribute__((packed)) dev_reg_t;

typedef struct _resend_pair_t
{
    u16     w_start;
    u16     w_end;
} resend_pair_t;

typedef struct _manufacture_req_t
{
    s8      p_model_name[32];
    s8      p_serial_number[32];
    u32     n_company_code;
    s8      p_mac_address[32];
    u32     n_release_date;
    u16     p_effective_area[4];
    u32		n_scintillator_type;
    u16		w_battery_count_to_install;   
}__attribute__((packed)) manufacture_req_t;

typedef struct _function_list_t
{
    u8		c_transmission_done_enable;
   	u8		c_packet_trigger;
   	u8		c_dosf_enable;
   	u8		c_integration_type;
   	u8		c_ap_button_mobile_memory_cnt;
   	u8		c_backup_image_disable;
   	u8		c_image_retransmission;
   	u8		c_dhcp_enable;
   	u8		c_sw_power_down_enable;
   	u8		c_always_wired_detection;
   	u8		c_smart_offset_support;
   	u8		c_internal_calibration;
   	u8		c_nfc_support;
   	u8		c_ftm_support;
   	u8		c_web_access_support;
   	u8		c_wake_up_time_support;
   	u8		c_go_sleep_support;
    u8		c_offset_cal_skip;			// offset calibration 시 skip counter 사용 여부
	u8		c_gate_line_defect_enable;
} function_list_t;

typedef struct _function_list_ack_t
{
    u8		c_transmission_done_enable;
   	u8		c_packet_trigger;
   	u8		c_dosf_enable;
   	u8		c_integration_type;
   	u8		c_ap_button_mobile_memory_cnt;
   	u8		c_backup_image_disable;
   	u8		c_image_retransmission;
   	u8		c_dhcp_enable;				//프로토콜 문서는 disable support 임
   	u8		c_sw_power_down_enable;
   	u8		c_always_wired_detection;	//10
/***********************************************************************************************************/
   	u8		c_smart_offset_support;
   	u8		c_internal_calibration;
   	u8		c_nfc_support;
   	u8		c_ftm_support;
   	u8		c_web_access_support;
   	u8		c_wake_up_time_support;
   	u8		c_go_sleep_support;
    u8		c_impact_count;
    u8		c_exp_time;                 // impact count 및 X-ray 노출 시간 기록 기능 지원 여부
    u8		c_wired_only;				// 20 무선 비활성화 기능 지원 여부 
/***********************************************************************************************************/
    u8		c_led_control;
	u8		c_offset_cal_skip;			// offset calibration 시 skip counter 지원 여부
	u8		c_aed_trig_volt_threshold;
	u8		c_scintillator_type_setting;
	u8		c_trigger_disable_support;
	u8		c_exp_index_info_support;
	u8		c_change_bat_count_support;
	u8		c_sftp_support;
	u8		c_scu_trig_disable_support;
	u8		c_performance_option_support;	//30
/***********************************************************************************************************/
	u8		c_gate_line_defect_support;
	u8		c_tft_aed_support;
	u8		c_led_blink_control;
	u8		c_wifi_signal_level;
	u8		c_multi_frame;
	u8		c_acc_cal_6plane_support;
	u8 		b_WPA2;								// 0 - Not Supported 	
    u8 		b_SendAveragedImageForGain;			// Receive gain count
    u8 		b_Align;							// 0 - Not Supported 	 
	u8		c_wpt_support;					// 40 무선충전 지원여부
/***********************************************************************************************************/
	u8		c_oper_info_support;
	u8		c_acc_gyro_combo_support;
	u8		c_nfc_onoff_support;
    u8      c_vxtd_3543d_dynamic_support;
    u8      c_fxrs_04a_uart_string_command_support;
    u8      c_calibration_data_user_save_flag_support;
    u8      c_wpt_aed_noise_identification_support;         //지원할 경우, Multi Frame Image Info (0x0251)에서 WPT AED Noise existence 값을 참조. 값을 통해 Noise depress filter(SDK) 적용
    u8      detector_overlap_support;                   
    u8      skip_frame_count_support;                       //NDT function
    u8      wired_only_lock_support;                        // 50 NDT function
/***********************************************************************************************************/
    u8      modify_default_config_support;                  //NDT function
    u8      scintillator_type_range_advertisement_support;  //0: unknown 1: support 2:not suuport
    u8      gain_type_conversion_support;                   //0: unknown 1: support 2:not suuport (conversion in SDK)
    u8      mobile_memory_control_support;                  //0: unknown 1: support 2:not suuport
    u8      promote_user_data_support;                      //0: unknown 1: support 2:not suuport
    u8      wireless_support;                               //0: unknown 1: support 2:not suuport
    u8      circuit_breaker_support;                        //0: unknown 1: support 2:not suuport
    u8      calibration_file_ftp_upload_location_support;   //0: unknown 1: support 2:not suuport
    u8      passing_calibration_file_upload_crc_check_support;  //0: unknown 1: support 2:not suuport
    u8      sensitivity_calibration_support;                    // 60 0: unknown 1: support 2:not suuport
/***********************************************************************************************************/
    u8      gain_compensation_control_support;
    u8      c_auto_power_control_support;                       //지원할 경우, by SCU가 아닌 auto power on enable / disable, auto power off enable / disable로 출력
    u8      offset_correction_mode_control_support;
    u8      fluoroscopy_frame_control_support;
    u8      acqusition_mode_control_support;
    u8      binning_mode_control_support;
    u8      aoi_control_support;
    u8      xray_alarm_support;
    u8      smart_w_support;                                    //exposure time을 짧게 잡은 상태에서 AED 센서를 이용하여 필요할 때만 exposure time을 적절하게 연장하는 기능
    u8      aed_off_debounce_support;							//70
/***********************************************************************************************************/
	u8 		c_dosf_line_count_lookup;		// 0 : unknown, 1 : support							// Index 71 (DOSF Ling Count 제어 0: line 4, 1 : line 변경 가능 )
	u8 		c_max_skip_frame_exp;			// 0 : not support, 1 : support						// Index 72
	u8 		c_cutoff_standby_power;			// 0 : not support, 1 : support						// Index 73
	u8 		c_battery_recharing_ctrl_mode;	// 0 : not support, 1 : support	(payload 0x0229)	// Index 74
	u8 		c_immediate_transfer;			// 0 : not support, 1 : support						// Index 75
	u8 		c_using_short_xray_pulse;		// 0 : not support, 1 : support						// Index 76
	u8 		c_applied_smart_w;				// 0 : not support, 1 : support						// Index 77
	u8 		c_use_binding_with_sensor_part;	// 0 : not support, 1 : support						// Index 78
	u8 		c_make_defect_bin2_by_fw;		// 0 : sdk makes bin2 file, 1: fw makes bin2 file	// Index 79
    u8 		c_battery_lifespan_extn_mode;	// 0 : not support, 1 : support (payload 0x0229)	// Index 80
/***********************************************************************************************************/
    u8 		c_preview_mode;		            // 0: unknown 1: support 2:not support   			// Index 81
    u8 		c_battery_charge_complete;	    // 0 : not support, 1 : support (payload 0x0200)	// Index 82
    u8 		c_impact_log_v2;	            // 0 : not support, 1 : support (payload 0x0200)	// Index 83
    u8 		c_relay_trigger_mode;			// 0 : not support, 1 : support						// Index 84
    u8 		c_serial_command_bypass;		// 0 : not support, 1 : support						// Index 85
    u8 		c_multi_acc_sensor_control;		// 0 : not support, 1 : support						// Index 86
    u8 		ReadOutSync;                    // 0 : not support, 1 : support						// Index 87

} function_list_ack_t;


/**
 * @brief   
 */
typedef enum
{
    eOPER_READ = 0,
    eOPER_WRITE,
    eOPER_SAVE,
    eOPER_READ_SAVED,
} OPERATION;

/**
 * @brief   power mode notification
 */
typedef enum
{
    ePM_NOTI_SLEEP_MODE = 0x00,
    ePM_NOTI_DEEP_SLEEP_MODE,
    ePM_NOTI_SHUTDOWN_MODE,
    
    ePM_NOTI_WAKE_UP_BY_PWR_BUTTON,
    
    ePM_NOTI_SHUTDOWN_BY_SCU = 0x10,
    ePM_NOTI_SHUTDOWN_BY_LOW_BATTERY,
    ePM_NOTI_SHUTDOWN_BY_PWR_BUTTON,
    
} PM_NOTI;

/**
 * @brief   
 */
typedef enum
{
    eMULTI_PACKET_PREPARE = 0,
    eMULTI_PACKET_SEND_DATA,
    eMULTI_PACKET_END_DATA,
    eMULTI_PACKET_UPDATE_PROGRESS,
    eMULTI_PACKET_UPDATE_RESULT,
} MULTI_PACKET;

/**
 * @brief   
 */
typedef enum
{
    eMULTI_PACKET_ERR_NO = 0,
    eMULTI_PACKET_ERR_CRC,
    eMULTI_PACKET_ERR_WRITE,
    eMULTI_PACKET_ERR_ERASE,
    eMULTI_PACKET_ERR_MEM_ALLOC,
} MULTI_PACKET_ERROR;

/**
 * @brief   config.xml 종류
 */
typedef enum
{
    eCONFIG = 1,
    eCONFIG_PRESET = 2,
} CONFIG_FILE_TYPE;

/**
 * @brief   detector state 종류
 */
typedef enum
{
    eSTATE_N = 1,
    eSTATE_VW = 2,
} STATE_TYPE;

/**
 * @brief   function list support enum
 */
typedef enum
{
    eUNKNOWN_OR_NOT_SUPPORT = 0,
    eSUPPORT = 1,
    eNOT_SUPPORT = 2,
} FUNC_SUPPORT;


/**
 * @brief   function list enable enum
 */
typedef enum
{
    eDISABLE = 0,
    eENABLE = 1,

} FUNC_ENABLE;

typedef enum
{
	eCORRECTION_PACKET_AVALIABLE = 0, // Get available offset correction mode
	eCORRECTION_PACKET_GET_ACTIVE, // Get offset correction mode
	eCORRECTION_PACKET_SET_ACTIVE, // Set offset correction mode
	eCORRECTION_PACKET_APPLIED // Get Applied offset correction mode
} CORRECTION_DEF;
/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*	Variable Definitions
*******************************************************************************/

/*******************************************************************************
*	Function Prototypes
*******************************************************************************/

#endif /* end of PAYLOAD_DEF_H_ */
