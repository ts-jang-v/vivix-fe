/*
 * vw_system_def.h
 *
 *  Created on: 2014. 03. 12.
 *      Author: myunghee
 */

 
#ifndef VW_SYSTEM_DEF_H_
#define VW_SYSTEM_DEF_H_

/*******************************************************************************
*	Include Files
*******************************************************************************/
#include "typedef.h"

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/
//#define SYS_MOUNT_MMCBLK_NUM	        "3"
#define SYS_MOUNT_MMCBLK_NUM	        "2"
#define SYS_MOUNT_MMCBLK_P2		        "/dev/mmcblk" SYS_MOUNT_MMCBLK_NUM "p2"
#define SYS_MOUNT_MMCBLK_P3		        "/dev/mmcblk" SYS_MOUNT_MMCBLK_NUM "p3"

#define SYS_CONFIG_PATH                 "/vivix/"
#define SYS_RUN_PATH                    "/run/"
#define SYS_DATA_PATH                   "/mnt/mmc/p2/"
#define SYS_IMAGE_DATA_PATH             "/mnt/mmc/p2/image/"

#define SYS_LOG_PATH                    "/mnt/mmc/p3/"
#define SYS_MONITOR_LOG_PATH            "/mnt/mmc/p3/mlog/"
#define SYS_IMPACT_LOG_PATH             "/mnt/mmc/p3/impact/"
#define SYS_IMPACT_V2_LOG_PATH          "/mnt/mmc/p3/impactV2/"
#define SYS_EXPOSURE_LOG_PATH           "/mnt/mmc/p3/exp/"

#define CONFIG_XML_FILE                 "config_preset.xml"
#define PRESET_XML_FILE                 "preset.xml"

#define CONFIG_FPGA_FILE                "fpga.bin"
#define OFFSET_BIN_FILE                 "offset.bin"
#define DEFECT_MAP_FILE                 "defect.bin"
#define GAIN_BIN_FILE                   "gain.bin"
#define OSF_PROFILE_FILE                "osf_profile.bin"
#define OFFSET_BIN2_FILE                "offset.bin2"
#define DEFECT_MAP2_FILE                "defect.bin2"
#define DEFECT_GRID_MAP2_FILE           "defect_grid.bin2"
#define DEFECT_GATEOFF_FILE   	        "defectForGateOff.bin2"
#define GAIN_BIN2_FILE                  "gain.bin2"
#define BACKUP_IMAGE_LIST_FILE          "/tmp/backup.list"
#define BACKUP_IMAGE_XML_LIST_FILE      "/tmp/backup_xml_list.xml"

#define SELF_DIAG_LIST_FILE             "/tmp/diag_list.xml"
#define SELF_DIAG_RESULT_FILE           "/tmp/diag_result.xml"
#define NUMBER_OF_IMPACT_FILE	        "/mnt/mmc/p3/impact/number_of_impact_file.bin"
#define NUMBER_OF_IMPACT_FILE_V2        "/mnt/mmc/p3/impactV2/number_of_impact_file.bin"
#define IMPACT_CAL_LOG_FILE		        "impact_cal.log"

#define PREVIEW_GAIN_FILE		        "preview_gain.bin"
#define PREVIEW_DEFECT_FILE		        "preview_defect.bin"

#define DHCP_CONFIG_FILE		        "/run/udhcpd.conf"

#define DISPLAY_LOGO_FILE		        "/run/logo.bin"

#define OLED_TETHER_IS_GREEEN	        0
#define OLED_AP_IMG_1			        1

// EEPROM address map ( 0 ~ 0x1FFF : 8Kbytes )
#define EEPROM_MANUFACTURE_INFO_SIZE	0x400
#define EEPROM_OPER_INFO_SIZE			0x200
#define EEPROM_AED_BUF_SIZE				0x100
#define EEPROM_CUSTOM_INFO_SIZE			0x100
		
#define EEPROM_ADDR_ENV                 0x0000      
#define EEPROM_ADDR_OPER                (EEPROM_ADDR_ENV + EEPROM_MANUFACTURE_INFO_SIZE)
#define EEPROM_ADDR_AED_BUF		        (EEPROM_ADDR_OPER + EEPROM_OPER_INFO_SIZE)
#define EEPROM_ADDR_CUSTOM_INFO	        (EEPROM_ADDR_AED_BUF + EEPROM_AED_BUF_SIZE)

#define EEPROM_MAGIC_NUMBER             0x12345678
#define EEPROM_MAGIC_NUMBER_V2          0x12345679	// 제조정보에 Scintillator type 추가에 따른 magic number 변경
#define EEPROM_MAGIC_NUMBER_V3          0x12345680
#define EEPROM_MAGIC_NUMBER_V4          0x12345681  // 제조정보에 gyro offset cal 결과 값 저장에따른 magic number 변경
#define EEPROM_MAGIC_NUMBER_V5          0x12345682  // 제조정보에 battery charge mode 추가 및 magic number 추가

// backup image
#define BACKUP_IMAGE_MAX                (200)

#define NFC_TAG_ID_LENGTH		        (32)		// 16의 배수가 되어야 한다.

// ADC 
#define ADC_VEF_VOLT			        (2.1f)
#define ADC_2X_VREF_VOLT		        (4.2f)
#define	ADC_DATA_BIT			        8
#define ADC_RESOLUTION			        (0x01 << ADC_DATA_BIT)

#define ADC_MICOM_VEF_VOLT			    (3.3f)
#define ADC_MICOM_DATA_BIT			    12
#define ADC_MICOM_RESOLUTION			(0x01 << ADC_MICOM_DATA_BIT)

#define ADC_VBATT_VOLTAGE_DIVIDER_NUMERATOR         (100.0f + 15.0f)    //회로도 참고 (Total 저항값)
#define ADC_VBATT_VOLTAGE_DIVIDER_DENOMINATOR       (15.0f)           

#define ADC_AED_TRIG_VOLTAGE_DIVIDER_NUMERATOR      (10.0f + 15.0f)
#define ADC_AED_TRIG_VOLTAGE_DIVIDER_DENOMINATOR    (15.0f)

#define ADC_VS_TRIG_VOLTAGE_DIVIDER_NUMERATOR      (100.0f + 62.0f + 9.0f) // 보정치: +9
#define ADC_VS_TRIG_VOLTAGE_DIVIDER_DENOMINATOR    (62.0f)

#define GAIN_TYPE_NUM			        5

//#define IMAGE_MAGIC_STRING		        "0x20140731"
#define TI_2256_FPGA_MAJOR_VERSION		"VIVIX-S-VW_1"
#define ADI_1255_FPGA_MAJOR_VERSION		"VIVIX-S-VW_2"

/*******************************************************************************
*	Type Definitions
*******************************************************************************/
typedef enum
{
    eDEV_ERR_NO = 0,
    
    eDEV_ERR_ENV_LOAD_FAILED = 0x10,
    eDEV_ERR_ENV_LOAD_DEV_ERROR,
    eDEV_ERR_ENV_LOAD_FILE_ERROR,
    eDEV_ERR_ENV_SAVE_FAILED,
    eDEV_ERR_ENV_SAVE_FILE_ERROR,
    eDEV_ERR_ENV_FILE_VERSION_ERROR,
    
    // h/w
    eDEV_ERR_HW_MON_DRIVER_OPEN_FAILED = 0x20,
    eDEV_ERR_HW_MON_MICOM_INIT_FAILED,
    eDEV_ERR_HW_MON_PROC_FAILED,
    
    
    // fpga
    eDEV_ERR_FPGA_MALLOC_FAILED = 0x30,
    eDEV_ERR_FPGA_INIT_FAILED,
    eDEV_ERR_FPGA_DONE_PIN,
    eDEV_ERR_FPGA_DRIVER_OPEN_FAILED,
    eDEV_ERR_ROIC_DRIVER_OPEN_FAILED,

    // offset
    eDEV_ERR_OFFSET_MALLOC_FAILED = 0x50,
    eDEV_ERR_OFFSET_FILE_OPEN_FAILED,
    eDEV_ERR_OFFSET_INVALID_CRC,
    eDEV_ERR_OFFSET_FILE_SAVE_FAILED,
    
} DEV_ERROR;

typedef enum
{
    eDEV_ID_FPGA    = 0,
    eDEV_ID_ROIC,	// reserved
    eDEV_ID_AED_0,
    eDEV_ID_AED_1,
    eDEV_ID_AED_2,
    eDEV_ID_FIRMWARE,
    eDEV_ID_ADC, 	// reserved
} DEV_ID;

typedef enum
{
    eFW_ADDR_LED_BLINKING    = 1,
    eFW_ADDR_WIFI_LEVEL_CHANGE = 2,
} FIRMWARE_ADDR;

typedef enum
{
    eSTAY_ON    = 0,
    eBLINK = 1,
} LED_CTRL;

typedef enum
{
    eWIFI_SIGNAL_STEP_5 = 0,
    eWIFI_SIGNAL_STEP_3 = 1,
} WIFI_STEP_NUM;

typedef enum
{
	eAED0_BIAS = 0,
	eAED0_OFFSET,
	eAED1_BIAS,
	eAED1_OFFSET,
	eAED_POTENTIOMETER_MAX_NUM,
} AED_POT_IDX;
	
	
typedef enum
{
    eOFFSET_CAL_ACQ_COUNT = 0,
    eOFFSET_CAL_ACQ_CANCELLED,
    eOFFSET_CAL_FAILED,
    eOFFSET_CAL_ACQ_COMPLETE,
    eOFFSET_CAL_ACQ_TIMEOUT,
    eOFFSET_CAL_ACQ_ERROR,
    eOFFSET_CAL_TRIGGER,
    eOFFSET_CAL_STOP_BY_SW,
    eOFFSET_CAL_REQUEST,
    
} OFFSET_CAL_STATE;

typedef enum
{
    eOFFSET_DRIFT_ACQ_COUNT = 0,
    eOFFSET_DRIFT_ACQ_CANCELLED,
    eOFFSET_DRIFT_FAILED,
    eOFFSET_DRIFT_ACQ_COMPLETE,
    eOFFSET_DRIFT_ACQ_TIMEOUT,
    eOFFSET_DRIFT_ACQ_ERROR,
    eOFFSET_DRIFT_TRIGGER,
    eOFFSET_DRIFT_STOP_BY_SW,
    eOFFSET_DRIFT_REQUEST,
    
} OFFSET_DRIFT_STATE;

typedef enum
{
	eAP_STATION = 0,
	ePRESET_SWITCHING = 1,
} APBUTTON_TYPE;

/**
 * @brief   system settings
 */

typedef enum
{
    eCFG_MODEL              = 0x00,
    eCFG_SERIAL,
    
    // version
    eCFG_VERSION_PACKAGE,
    eCFG_VERSION_BOOTLOADER,
    eCFG_VERSION_KERNEL,
    eCFG_VERSION_RFS,
    eCFG_VERSION_FW,
    eCFG_VERSION_MICOM,
    eCFG_VERSION_FPGA,
    
    eCFG_TAG_ID         	= 0x10,
    eCFG_PRESET_NAME       	= 0x11,
    
    // LAN
    eCFG_IP_ADDRESS         = 0x20,
    eCFG_NETMASK,
    eCFG_GATEWAY,
    eCFG_POE_USE_ONLY_POWER,
    
    // WLAN
    eCFG_SSID,
    eCFG_KEY,
    eCFG_SCAN,
    eCFG_BSSID,
    
    // AP
    eCFG_AP_ENABLE,
    eCFG_AP_FREQ,
    eCFG_AP_COUNTRY,
    eCFG_AP_BAND,
    eCFG_AP_SSID,
    eCFG_AP_KEY,
    eCFG_AP_CHANNEL,
    eCFG_AP_GI,
    eCFG_AP_SECURITY,
    eCFG_AP_TX_POWER,
    eCFG_AP_COUNTRY_CODE,
    eCFG_AP_DHCP,
    
    // power mode
    eCFG_PM_SLEEP_ENABLE    = 0x40,
    eCFG_PM_SLEEP_ENTRANCE_TIME,
    eCFG_PM_DEEP_SLEEP_ENABLE,
    eCFG_PM_DEEP_SLEEP_ENTRANCE_TIME,
    eCFG_PM_SHUTDOWN_ENABLE,
    eCFG_PM_SHUTDOWN_ENTRANCE_TIME,
    eCFG_PM_POWER_SOURCE,   // Detector or SCU
    eCFG_PM_AWD,
    
    // image
    eCFG_IMAGE_TIMEOUT      = 0x50,
    eCFG_IMAGE_PREVIEW,
    eCFG_IMAGE_FAST_TACT,
    eCFG_IMAGE_DIRECTION,
    eCFG_IMAGE_DIRECTION_USER_INPUT,
    eCFG_IMAGE_BACKUP,
    
    // impact sensor
    eCFG_IMACT_PEAK,
    eCFG_DEFECT_LOWER_GRID,
    eCFG_AP_BUTTON_FUNC_TYPE,
    
    eCFG_OFFSET_ENABLE      = 0x60,
    eCFG_OFFSET_PERIOD,
    eCFG_OFFSET_COUNT,
    eCFG_OFFSET_TEMPERATURE,
    
    eCFG_EXP_TRIG_TYPE      = 0x70,
    
    eCFG_LAST,
    
    eCFG_MAX                = 0x100,
} CONFIG;

/**
 * @brief   model info
 */
typedef enum
{
    eMODEL_2530VW    = 0,
    eMODEL_3643VW,
    eMODEL_4343VW,
    
    eMODEL_MAX_NUM,
    
} MODEL;

/**
 * @brief   panel type
 */

/**
 * @brief   panel type
 */
typedef enum
{
    ePANEL_TYPE_BASIC    = 0,

    ePANEL_TYPE_MAX
} PANEL_TYPE;

/**
 * @brief   SUB_MODEL_TYPE
 */
typedef enum
{
    eSUBMODEL_TYPE_BASIC    = 0,
    eSUBMODEL_TYPE_PLUS     = 1,

    eSUBMODEL_TYPE_MAX
} SUBMODEL_TYPE;

/**
 * @brief   scintillator type
 */
typedef enum
{
    eSCINTILLATOR_TYPE_00,                   //eSCINTILLATOR_TYPE_BASIC = 0,           //deprecated (Not use)
    eSCINTILLATOR_TYPE_01,                   //eSCINTILLATOR_TYPE_DEPO,                //deprecated (Not use)
    eSCINTILLATOR_TYPE_02,                   //eSCINTILLATOR_TYPE_A,                   //old ePANEL_BASIC (3643/4343VW)  
    eSCINTILLATOR_TYPE_03,                   //eSCINTILLATOR_TYPE_B,
    eSCINTILLATOR_TYPE_04,                   //eSCINTILLATOR_TYPE_C,                   //old ePANEL_DEPO (3643/4343VW PLUS)   
    eSCINTILLATOR_TYPE_05,                   //eSCINTILLATOR_TYPE_D,
    eSCINTILLATOR_TYPE_06,                   //eSCINTILLATOR_TYPE_E,                   //old ePANEL_BASIC (2530VW) & old ePANEL_DEPO (2530VW PLUS)   
    eSCINTILLATOR_TYPE_07,                   //eSCINTILLATOR_TYPE_F,                                                    
    eSCINTILLATOR_TYPE_08,                   //eSCINTILLATOR_TYPE_G,
    eSCINTILLATOR_TYPE_09,                   //eSCINTILLATOR_TYPE_H,
    eSCINTILLATOR_TYPE_10,                   //eSCINTILLATOR_TYPE_I,
    eSCINTILLATOR_TYPE_11,                   //eSCINTILLATOR_TYPE_J,
    eSCINTILLATOR_TYPE_12,                   //eSCINTILLATOR_TYPE_K,
    eSCINTILLATOR_TYPE_13,                   //eSCINTILLATOR_TYPE_L,
    eSCINTILLATOR_TYPE_14,                   //eSCINTILLATOR_TYPE_M,
    eSCINTILLATOR_TYPE_15,                   //eSCINTILLATOR_TYPE_N,

    eSCINTILLATOR_TYPE_MAX,    
} SCINTILLATOR_TYPE;


/**
 * @brief   roic model info
 */

typedef enum
{
    eROIC_TYPE_ADI_1255 = 0x00,
    eROIC_TYPE_TI_2256GR = 0x01,
    eROIC_TYPE_TI_2256TD = 0x02,
    
    eROIC_TYPE_MAX,
}ROIC_TYPE;

/**
 * @brief   phy model info
 */

typedef enum
{
    ePHY_MARVELL_88E1510 = 0x00,
    ePHY_ADI_ADIN1300 = 0x01,
    
    ePHY_TYPE_MAX,
} PHY_TYPE;

/**
 * @brief   network mode
 */
typedef enum
{
    eNET_MODE_TETHER    = 0,
    eNET_MODE_STATION,
    eNET_MODE_AP,
    eNET_MODE_INIT,
} NET_MODE;

/**
 * @brief   power source
 */
typedef enum
{
    ePWR_SRC_BATTERY    = 0,
    ePWR_SRC_TETHER,
    ePWR_SRC_USB,
    
    ePWR_SRC_UNKNOWN = 0xFF,
    
} POWER_SOURCE;
   
/**
 * @brief   power voltage
 */
typedef enum
{
    POWER_VOLT_B_ID_A = 0,
    POWER_VOLT_B_ID_B,
    POWER_VOLT_B_THM_A,
    POWER_VOLT_B_THM_B,
    POWER_VOLT_AED_TRIG_A,
    POWER_VOLT_AED_TRIG_B,
    POWER_VOLT_VBATT,
    POWER_VOLT_VS,
    POWER_VOLT_MAX,
} POWER_VOLT;


typedef enum
{
	eEVENT_OFF = 0,
	eBUTTON_EVENT = 1,
	eACTION_EVENT_END = 2,
	
	eACTION_EVENT_MIN = 0x10,
	eBOOTING,
	eWAKEUP_EVENT,
	eRESET_WLAN_EVENT,
	eSYNC_WLAN_EVENT,
	eACQ_IMAGE_EVENT,
	eSAVE_IMAGE_EVENT,
	eSEND_IMAGE_EVENT,
	eAP_MODE_ENABLE_EVENT,
	eSTA_MODE_ENABLE_EVENT,
	eSCAN_AP_EVENT,
	eCHANGE_AP_EVENT,
	eACTION_EVENT_MAX,
	
}OLED_EVENT;

typedef enum
{
	eFPGA_CONFIG_ERROR = 0,
	eROIC_BIT_ALIGN_ERROR,
	eGATE_STATE_ERROR,
	eAED_SENSOR_ERROR,
	eMAIN_VOLT_ERROR,
	eBAT_NUM_ERROR,
	eOLED_ERROR_MAX,
}OLED_ERROR;

/**
 * @brief   acceleration sensor calibration direction
 */
typedef enum
{
	eACC_DIR_Z_P_1G = 0,
	eACC_DIR_Z_N_1G,
	eACC_DIR_Y_P_1G,
	eACC_DIR_X_P_1G,
	eACC_DIR_Y_N_1G,
	eACC_DIR_X_N_1G,
	
	eACC_DIR_MAX,
} ACC_DIRECTION;

/**
 * @brief   battery charging mode
 */
typedef enum
{
	eCHG_MODE_NORMAL = 0,
	eCHG_MODE_RECHARGING_CONTROL,
	eCHG_MODE_LIFESPAN_EXTENSION,
	
	eCHG_MODE_MAX,
} BAT_CHG_MODE;

/**
 * @brief   impact sensor offset calibration result
 */
typedef struct _impact_cal_offset_t
{
    s8				c_x;
    s8				c_y;
    s8				c_z;	
}__attribute__((packed)) impact_cal_offset_t;

/**
 * @brief   impact sensor offset extra detailed calibration result
 */
typedef struct _impact_cal_extra_detailed_offset_t
{
    s16				s_x;
    s16				s_y;
    s16				s_z;	
}__attribute__((packed)) impact_cal_extra_detailed_offset_t;


/**
 * @brief   gyro sensor offset calibration result
 */
typedef struct _gyro_cal_offset_t
{
    s8				c_x;
    s8				c_y;
    s8				c_z;	
}__attribute__((packed)) gyro_cal_offset_t;

/**
 * @brief   h/w settings
 */
typedef struct _hw_config_t
{
    POWER_SOURCE    	power_source_e;
    u8              	c_impact_threshold;
	impact_cal_offset_t impact_offset_t;
} hw_config_t;

/**
 * @brief   power voltage
 */
typedef struct _power_volt_t
{
    POWER_SOURCE    power_source_e;
    u16             volt[POWER_VOLT_MAX];
} power_volt_t;


/**
 * @brief   effective area position
 */
typedef enum
{
    eEFFECTIVE_AREA_TOP    = 0,
    eEFFECTIVE_AREA_LEFT,
    eEFFECTIVE_AREA_RIGHT,
    eEFFECTIVE_AREA_BOTTOM,
    
    eEFFECTIVE_AREA_MAX,
} EFFECTIVE_AREA;

typedef enum
{
	eMICOM_EEPROM_BLK_FRAM = 0x00,
	eMICOM_EEPROM_BLK_CTL,
	eMICOM_EEPROM_BLK_IMPACT_INFO,
	eMICOM_EEPROM_BLK_IMPACT_PACKET,
} eeprom_blk_num_t;
/* Total memory 256K bits -> 32K Bytes, 0x0000 ~ 0x7FFF Address range
	 _______
	|		|	SYSTEM DATA
	|		|	IMPACT CTL DATA
	|		|	IMPACT INFO DATA
	|512____|
	|		|	IMPACT PACKET DATA(size 0x400, total 16ea packet)
	|16K____|
	|		|	FRAM DATA AREA
	|32K____|
*/

/**
 * @brief   제조 정보 목록
 */
typedef struct _manufacture_info_t
{
    u32     n_magic_number;         // 4	EEPROM 값이 유효한지 나타낸다.(0x12345678)
    
    u32     n_release_date;         // 8	제품 생산일
    s8      p_model_name[32];       // 40	모델명
    s8      p_serial_number[32];    // 72	제품 일련번호
    s8      p_package_info[32];		// 104
    u8      p_mac_address[6];       // 110	mac 주소
    u32     n_company_code;			// 114
    
    bool    b_wlan_factory;         // 115	출하 시부터 무선 모듈이 장착되었는지 표시
    bool    b_wlan_upgrade;         // 116	출하 후에 무선 모듈이 장착되었는지 표시
    
    u16     p_effective_area[eEFFECTIVE_AREA_MAX];	// 124
    
    u32     n_magic_number_v2;			// 128, EEPROM 값이 유효한지 나타낸다.(V2: 0x12345679)
    u32		n_scintillator_type;		// 132
    u16		w_battery_count_to_install;	// 134		
    
    // 신규 추가 시마다 magic number version 추가 필요
    
} __attribute__((packed)) manufacture_info_t;

/**
 * @brief   사용 정보
 */
typedef struct _operation_info_t
{
    u32     n_magic_number;                     // 4	EEPROM 값이 유효한지 나타낸다.(0x12345678)
    
    u32     n_capture_count;                    // 8	누적 촬영 횟수
    
	impact_cal_offset_t	impact_offset_t;	    // 11 충격 센서 offset calibration 결과 값
	
	u32     n_magic_number_v2;                  // 15
    s16		w_x_impact_extra_detailed_offset;   // 17
    s16		w_y_impact_extra_detailed_offset;   // 19
    s16		w_z_impact_extra_detailed_offset;   // 21
    
    u32     n_magic_number_v3;              // 25
    u8		c_acc_6plane_cal_done;          // 26

    u32     n_magic_number_v4;              // 27
    gyro_cal_offset_t	gyro_offset_t;	    // 30 gyro offset calibration 결과 값   
    u8		c_combo_impact_cal_done;        // 31

    u32     n_magic_number_v5;              // 35
    BAT_CHG_MODE bat_chg_mode;	            // 39 gyro offset calibration 결과 값   
    u8		c_rechg_gauge;                  // 40

} __attribute__((packed)) operation_info_t;


/**
 * @brief   AED 정보
 */
typedef struct _aed_info_t
{
    u32     n_magic_number;         // 4	EEPROM 값이 유효한지 나타낸다.(0x12345678)
    
    int		n_aed0_trig_low_thr;
   	int		n_aed0_trig_high_thr;
	u8		p_aed_pot[eAED_POTENTIOMETER_MAX_NUM]; 
	
	u32     n_magic_number_v2;
    int		n_aed1_trig_low_thr;
   	int		n_aed1_trig_high_thr;
} __attribute__((packed)) aed_info_t;


/**
 * @brief   Customizing 정보
 */
typedef struct _custom_info_t
{
    u32     n_magic_number;         // 4	EEPROM 값이 유효한지 나타낸다.(0x12345678)
	u8		c_wifi_signal_step;
	
} __attribute__((packed)) customized_info_t;

/**
 * @brief   system file ID
 */
typedef enum
{
    eVW_FILE_ID_CONFIG       = 0x00,
    eVW_FILE_ID_PRESET       = 0x01,
    
    eVW_FILE_ID_OFFSET                  = 0x10,
    eVW_FILE_ID_DEFECT_MAP              = 0x11,
    eVW_FILE_ID_GAIN                    = 0x12,
    eVW_FILE_ID_OSF_PROFILE             = 0x13,
    eVW_FILE_ID_BACKUP_IMAGE_LIST       = 0x14,
    eVW_FILE_ID_BACKUP_IMAGE            = 0x15,
    eVW_FILE_ID_BACKUP_IMAGE_THUMBNAIL  = 0x16,
    eVW_FILE_ID_SELF_DIAGNOSIS_LIST     = 0x17,
    eVW_FILE_ID_SELF_DIAGNOSIS_RESULT   = 0x18,
    
    eVW_FILE_ID_PATIENT_LIST_XML		= 0x1A,
    eVW_FILE_ID_STEP_XML				= 0x1B,
    
    eVW_FILE_ID_BACKUP_IMAGE_LIST_WITH_PATIENT_INFO	= 0x1C,
    eVW_FILE_ID_BACKUP_IMAGE_WITH_PATIENT_INFO		= 0x1D,
    
    eVW_FILE_ID_BOOTLOADER   = 0x20,
    eVW_FILE_ID_KERNEL,
    eVW_FILE_ID_RFS,
    eVW_FILE_ID_DTB,
    eVW_FILE_ID_APPLICATION,
    eVW_FILE_ID_PWR_MICOM,
    eVW_FILE_ID_FPGA,
    eVW_FILE_ID_AED,
    
    eVW_FILE_ID_OFFSET_BIN2				= 0x30,
    eVW_FILE_ID_DEFECT_BIN2				= 0x31,
    eVW_FILE_ID_GAIN_BIN2				= 0x32,
    eVW_FILE_ID_PREVIEW_GAIN			= 0x33,
    eVW_FILE_ID_PREVIEW_DEFECT			= 0x34,
    eVW_FILE_ID_DEFECT_GRID_BIN2		= 0x35,
    eVW_FILE_ID_DOSF_BIN				= 0x36,
    eVW_FILE_ID_DEFECT_FOR_GATEOFF_BIN2	= 0x37,
    
    eVW_FILE_ID_MAX,
    
} VW_FILE_ID;

/**
 * @brief   system file delete
 */
typedef enum
{
    eVW_FILE_DELETE_ALL             = 0xFF,
    
    eVW_FILE_DELETE_BACKUP_IMAGE    = 0x01,
    eVW_FILE_DELETE_LOG_FILE        = 0x02,
    eVW_FILE_DELETE_OPER_INFO       = 0x03,
    eVW_FILE_DELETE_PRESET   	    = 0x04,
    
    eVW_FILE_DELETE_ALL_CAL_DATA    = 0x10,
    eVW_FILE_DELETE_OFFSET          = 0x11,
    eVW_FILE_DELETE_GAIN            = 0x12,
    eVW_FILE_DELETE_DEFECT_MAP      = 0x13,
    eVW_FILE_DELETE_OSF_PROFILE     = 0x14,
    eVW_FILE_DELETE_PREVIEW_GAIN	= 0x15,
    eVW_FILE_DELETE_PREVIEW_DEFECT	= 0x16,

    eVW_FILE_DELETE_PATIENT_XML     = 0x1A,
    eVW_FILE_DELETE_STEP_XML		= 0x1B,

    eVW_FILE_DELETE_ALL_NEW_CAL_DATA	= 0x20,
    eVW_FILE_DELETE_OFFSET_BIN2			= 0x21,
    eVW_FILE_DELETE_GAIN_BIN2			= 0x22,
    eVW_FILE_DELETE_DEFECT_MAP_BIN2		= 0x23,
    eVW_FILE_DELETE_DEFECT_GRID_MAP_BIN2 = 0x24,
    eVW_FILE_DELETE_DEFECT_GATEOFF_BIN2 = 0x25,
    
    eVW_FILE_DELETE_FPGA_BIN = 0x30,
    
} VW_FILE_DELETE;

/**
 * @brief   power management mode
 */
typedef enum
{
    ePM_MODE_NORMAL = 0,
    ePM_MODE_WAKEUP,
    ePM_MODE_SLEEP,
    ePM_MODE_DEEPSLEEP,
    ePM_MODE_SHUTDOWN,
    
    ePM_MODE_SHUTDOWN_BY_SCU = 0x10,
    ePM_MODE_SHUTDOWN_BY_LOW_BATTERY,
    ePM_MODE_SHUTDOWN_BY_PWR_BUTTON,
    
    ePM_MODE_LOW_BATTERY = 0x20,
    
    ePM_MODE_WAKEUP_BY_PWR_BUTTON = 0x30,
    
    ePM_MODE_TETHER_POWER_ON        = 0x40,
    ePM_MODE_TETHER_POWER_OFF       = 0x41,
    ePM_MODE_BATTERY_CONNECTED      = 0x42,
    ePM_MODE_BATTERY_DISCONNECTED   = 0x43,
    ePM_MODE_BATTERY_0_ON_1_OFF		= 0x44,
    ePM_MODE_BATTERY_0_OFF_1_ON		= 0x45,
    ePM_MODE_BATTERY_0_ON_1_ON		= 0x46,
    ePM_MODE_BATTERY_0_OFF_1_OFF	= 0x47,
    ePM_MODE_BATTERY_SWELLING_RECHG_BLK_START = 0x48, 
    ePM_MODE_BATTERY_SWELLING_RECHG_BLK_STOP = 0x49,
    
} PM_MODE;


/**
 * @brief   power management settings
 */
typedef struct _pm_config_t
{
    bool    b_sleep_enable;
    bool    b_shutdown_enable;
    
    u16     w_sleep_time;
    u16     w_shutdown_time;
    
} pm_config_t;

/**
 * @brief   AP info
 */
typedef enum
{
    eAP_INFO_INACTIVE_TIME = 0,
    eAP_INFO_SIGNAL,
    eAP_INFO_SIGNAL_AVERAGE,
    
    eAP_INFO_RX_BYTES,
    eAP_INFO_RX_BITRATE,
    eAP_INFO_RX_MCS,
    eAP_INFO_RX_PACKET,
    eAP_INFO_RX_RETRIES,
    
    eAP_INFO_TX_BYTES,
    eAP_INFO_TX_BITRATE,
    eAP_INFO_TX_MCS,
    eAP_INFO_TX_PACKET,
    eAP_INFO_TX_RETRIES,
        
    eAP_INFO_MAX,
    
} AP_INFO;

typedef enum
{
    eBIAS = 0x1,
    eOFFSET = 0x2,
    eSTATUS = 0x10,
    eVERSION = 0x11,
    eAED_VOLT_THR_L = 0x30,
    eAED_VOLT_THR_H = 0x31,
    
} AED_DEV_ADDR;

typedef enum
{
	eAED_ID_0 = 0,
	eAED_ID_1,
	eAED_ID_MAX,	
} AED_ID;
	
	
typedef enum
{
	eBATTERY_0 = 0,
	eBATTERY_1,
	eBATTERY_MAX,
} BATTERY_ID;

typedef enum
{
	eBAT_CHARGE_0_ON_1_OFF = 0,
	eBAT_CHARGE_0_OFF_1_ON = 1,
	eBAT_CHARGE_0_ON_1_ON = 2,
	eBAT_CHARGE_0_OFF_1_OFF = 3,
} BAT_CHARGE;

typedef enum
{
	eBATTERY_CHARGER_RESET_NORMAL = 0,
    eBATTERY_CHARGER_RESET_WORKED,
    eBATTERY_CHARGER_RESET_COMM_FAIL,
} BATTERY_CHARGER_RESET_STATE;

/**
 * @brief   system state
 */
typedef struct _sys_state_t
{
    u16     w_network_mode;         // NET_MODE: Tether - 0, Station - 1, AP - 2
    u16     w_power_source;         // tether - 1, battery - 0
    u16     w_link_quality_level;
    u16     w_link_quality;
    u16     w_battery_gauge;
    u16     w_battery_vcell;
    u16     w_temperature;
    u16     w_humidity;
    
    u16     w_tether_speed;         // 유선 link 속도: Tether인 경우에만 값이 반영됨
    bool    b_battery_equipped;     // 0: 없음 1: 있음
    bool    b_battery_charge;       // 0: 충전 중이 아님 1: 충전 중
    
    u16     w_power_mode;           // 0: normal, 1: wakeup, 2: sleep
    bool    b_fpga_ready;
    
    u16     w_battery_level;
    u16     w_battery_raw_gauge;
    
    u16     p_hw_power_volt[POWER_VOLT_MAX];
    
    s32     n_signal_level;
    u32     n_bit_rate;
    u32     n_frequency;
    
    s32     p_ap_info[eAP_INFO_MAX];
    
    //u8      p_ap_mac[6];
    s8      p_ap_mac[32];
    s8      p_ap_station_mac[32];
    
    u32		n_accum_of_captured_count;
    u32		n_captured_count;
    
    u16     p_hw_power_volt2[POWER_VOLT_MAX];
        
    u16     w_battery2_vcell;
    bool    b_battery2_equipped;     // 0: 없음 1: 있음
    bool    b_battery2_charge;       // 0: 충전 중이 아님 1: 충전 중
    
    bool    b_battery_warning;      // battery low voltage
    
    bool    b_scu_connected;
    u16		w_gate_state;			// success: 0x0FFF for 1717N and 1417N, 0x03FF for 1012N
	
	u16		w_impact_count;
	u32 	n_total_exp_time;
	u32		n_gate_state_32bit;

    BATTERY_CHARGER_RESET_STATE    b_charger_reset;
    bool    b_bind_with_sensor;
    u16		w_impact_count_V2;
    u16     w_battery_charge_complete_1; // 0: not support 1: not complete 2: complete
    u16     w_battery_charge_complete_2; // 0: not support 1: not complete 2: complete
} __attribute__((packed)) sys_state_t;

/**
 * @brief   LED ID
 */
typedef enum
{
    eLED_ID_DATA,
    eLED_ID_AP,
    eLED_ID_BATTERY,
    eLED_ID_ACTIVE,
    
    eLED_ID_MAX,
} LED_ID;

/**
 * @brief   LED 동작 상태
 */
typedef enum
{
    eLED_STATE_OFF,
    
    eLED_STATE_ON,
    eLED_STATE_ON_GREEN,
    eLED_STATE_ON_BLUE,
    eLED_STATE_ON_ORANGE,
    
    eLED_STATE_BLINK = 0x10,
    eLED_STATE_BLINK_GREEN,
    eLED_STATE_BLINK_BLUE,
    eLED_STATE_BLINK_ORANGE,
    
} LED_STATE;

/**
 * @brief   LED 색
 */
typedef enum
{
    eLED_COLOR_NONE,
    eLED_COLOR_GREEN,
    eLED_COLOR_BLUE,
    eLED_COLOR_ORANGE,
    
} LED_COLOR;

/**
 * @brief   AP state by button
 */
typedef enum
{
    eAP_STATE_OFF,
    eAP_STATE_ON,
    eAP_STATE_CHANGE,
    
} AP_STATE;

/**
 * @brief   TRIGGER TYPE
 */
typedef enum
{
    eTRIGGER_TYPE_LINE = 0,
    eTRIGGER_TYPE_SW,
    eTRIGGER_TYPE_OFFSET,
    eTRIGGER_TYPE_PACKET,
    
} TRIGGER_TYPE;

/**
 * @brief   auto offset settings
 */
typedef struct _auto_offset_t
{
    u16     w_period_time;
    u16     w_count;
    u16     w_temp_difference;
    bool    b_aed_capture;
    
}auto_offset_t;

/**
 * @brief   patient information
 */
typedef struct _PATIENT_INFO_
{
    char      	name[130];
	char      	id[130];
	char		accNo[34];
	char		studyDate[18];
	char		studyTime[14];

    time_t  	time;
    char    	serialNum[20];
	int			type;
	u8			reserve[24];
	char		birth[18];
    char    	viewerSerial[18];
    long    	stepKey;
    
    int			neutered;
    char      	species_description[130];
    char      	breed_description[130];
    char      	responsible_person[130];
    
}__attribute__((packed)) patient_info_t;

/**
 * @brief   patient information
 */
typedef union _BACKUP_IMAGE_HEADER_
{
	u8			p_data[1024];
	struct
	{
		u32		n_id;
		u32		n_width_size;
		u32		n_height;
		u32		n_total_size;
		u32		n_time;
		
		s32		n_pos_x;
		s32		n_pos_y;
		s32		n_pos_z;
		
		u32		n_vcom_swing_time;	// ms
		u16		w_exposure_time_low;	// ms
		
		patient_info_t	patient_t;

	    u8		c_offset;
	    u8		c_defect;
	    u8		c_gain;
	    u8		c_reserved_osf;
	    
	    u16		w_direction;
	    u16		w_exposure_time_high;
		
	}information;
    
}__attribute__((packed)) backup_image_header_t;


typedef struct _TACT_IMAGE_HEADER_
{
	u32		n_id;
	u32		n_width_size;
	u32		n_height;
	u32		n_total_size;
	u32		n_time;
	
	s32		n_pos_x;
	s32		n_pos_y;
	s32		n_pos_z;
	
	u32		n_vcom_swing_time;	// ms
	u16		w_exposure_time_low;	// ms

    u8		c_offset;
    u8		c_defect;
    u8		c_gain;
    u8		c_reserved_osf;
    
    u16		w_direction;
    u16		w_exposure_time_high;

}__attribute__((packed)) tact_image_header_t;

typedef struct _adxl_data
{
	s16 w_x;
	s16 w_y;
	s16 w_z;
}adxl_data_t;

/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*	Variable Definitions
*******************************************************************************/

/*******************************************************************************
*	Function Prototypes
*******************************************************************************/

#endif /* end of VW_SYSTEM_DEF_H_ */
