/******************************************************************************/
/**
 * @file    oled_image.h
 * @brief   for oled image
 * @author  
*******************************************************************************/


#ifndef _OLED_IMAGE_
#define _OLED_IMAGE_

/*******************************************************************************
*   Include Files
*******************************************************************************/
#include "typedef.h"
#include "mutex.h"
#include "vw_system_def.h"
#include "vw_spi.h"
#include "vw_time.h"


/*******************************************************************************
*   Constant Definitions
*******************************************************************************/
#define OLED_IMG_BASE_Y					8

#define OLED_IMG_MODE_WIDTH				32
#define OLED_IMG_MODE_HEIGHT			32
#define OLED_IMG_MODE_SIZE				(OLED_IMG_MODE_WIDTH*OLED_IMG_MODE_HEIGHT/8)
#define OLED_IMG_MODE_SPOINT_X 			96
#define OLED_IMG_MODE_SPOINT_Y 			OLED_IMG_BASE_Y

#define OLED_IMG_BAT_WIDTH				48
#define OLED_IMG_BAT_HEIGHT				32
#define OLED_IMG_BAT_SIZE				(OLED_IMG_BAT_WIDTH*OLED_IMG_BAT_HEIGHT/8)
#define OLED_IMG_BAT_SPOINT_X 			0
#define OLED_IMG_BAT_SPOINT_Y 			OLED_IMG_BASE_Y

#define OLED_IMG_BAT_SOC_WIDTH			32
#define OLED_IMG_BAT_SOC_HEIGHT			16
#define OLED_IMG_BAT_SOC_SIZE			(OLED_IMG_BAT_SOC_WIDTH*OLED_IMG_BAT_SOC_HEIGHT/8)
#define OLED_IMG_BAT_SOC_SPOINT_X 		8
#define OLED_IMG_BAT_SOC_SPOINT_Y 		(OLED_IMG_BASE_Y + 8)

#define OLED_IMG_CHARGE_EN_WIDTH		16
#define OLED_IMG_CHARGE_EN_HEIGHT		32
#define OLED_IMG_CHARGE_EN_SIZE			(OLED_IMG_CHARGE_EN_WIDTH*OLED_IMG_CHARGE_EN_HEIGHT/8)
#define OLED_IMG_CHARGE_EN_SPOINT_X 	(OLED_IMG_BAT_SPOINT_X+OLED_IMG_BAT_WIDTH)
#define OLED_IMG_CHARGE_EN_SPOINT_Y 	(OLED_IMG_BASE_Y - 2)

#define OLED_IMG_PRESET_WIDTH			32
#define OLED_IMG_PRESET_HEIGHT			28
#define OLED_IMG_PRESET_SIZE			(OLED_IMG_PRESET_WIDTH*OLED_IMG_PRESET_HEIGHT/8)
#define OLED_IMG_PRESET_SPOINT_X 		(OLED_IMG_CHARGE_EN_SPOINT_X + OLED_IMG_CHARGE_EN_WIDTH)
#define OLED_IMG_PRESET_SPOINT_Y	 	(OLED_IMG_BASE_Y + 4)

#define OLED_IMG_INFO_WIDTH				128
#define OLED_IMG_INFO_HEIGHT			28
#define OLED_IMG_INFO_SIZE				(OLED_IMG_INFO_WIDTH*OLED_IMG_INFO_HEIGHT/8)
#define OLED_IMG_INFO_SPOINT_X 			0
#define OLED_IMG_INFO_SPOINT_Y	 		(OLED_IMG_BASE_Y + 2)

#define OLED_IMG_FULL_WIDTH				128
#define OLED_IMG_FULL_HEIGHT			32
#define OLED_IMG_FULL_SIZE_BYTE			(OLED_IMG_TEST_WIDTH*OLED_IMG_TEST_HEIGHT/8)
#define OLED_IMG_FULL_SPOINT_X 			0
#define OLED_IMG_FULL_SPOINT_Y 			OLED_IMG_BASE_Y

#define OLED_IMG_CHARACTER_WIDTH		16
#define OLED_IMG_CHARACTER_HEIGHT		28
#define OLED_IMG_CHARACTER_SIZE_BYTE	(OLED_IMG_CHARACTER_WIDTH*OLED_IMG_CHARACTER_HEIGHT/8)

#define OLED_IMG_NUM_OF_STATION_TYPE	6		
#define OLED_IMG_NUM_OF_TETHER_TYPE		2
#define OLED_IMG_NUM_OF_BAT_SOC_TYPE	33
#define OLED_IMG_NUM_OF_CHARACTERS		26
#define OLED_IMG_NUM_OF_NUMERIC			10

#define OLED_INFO_DISPLAY_LEN			8
#define	OLED_SSID_INFO_MAX_LENGTH		(72) /* SSID max 64 characters + "SSID:" 5 characters + extra 3 characters(for 8 characters align) */
#define	OLED_IP_INFO_MAX_LENGTH			(24) /* xxx.xxx.xxx.xxx max 15 characters + "IP:" 3 characters + extra 6 characters(for 8 characters align) */
#define OLED_PRESET_MAX_LENGTH			2
#define OLED_ERROR_INFO_LENGTH			8
#define OLED_OPER_INFO_MAX_LENGTH		32

#define SCROLL_UNIT_PIXEL_NUM			8
#define	SCROLL_START_COLUMN_IDX			(OLED_IMG_INFO_WIDTH/SCROLL_UNIT_PIXEL_NUM) // 전체 화면 Width 이후부터 scroll

/*******************************************************************************
*   Type Definitions
*******************************************************************************/
enum
{
	eOLED_FULL = 0, 
	eOLED_BAT, 
	eOLED_BAT_SOC,
	eOLED_CHARGE_EN,
	eOLED_PRESET,
	eOLED_MODE,
	eOLED_INFO,
	eOLED_NUM_OF_AREA,
};

typedef enum
{
	eOLED_OFF= 0,
	eOLED_ERROR,
	eOLED_MAIN,
	eOLED_IP,
	eOLED_SSID,
	eOLED_LAST,
}OLED_DISP_BASIC_SEQ;

typedef enum
{
    eOLED_IDLE_STATE = 0,
    eOLED_BASIC_DISP_STATE,
    eOLED_ACTION_DISP_STATE,
    
} OLED_STATE;

/**
 * @brief  기본 main 화면에 표시되는 정보
 */
typedef struct _main_t
{
	u16 	w_battery_gauge;
	u16		b_charge;
	u8  	p_preset_name[2];
	u16     w_network_mode;
	u8    	c_1G_speed;         
	u16     w_link_quality_level;	
	u16     w_power_mode;           // 0: normal, 1: wakeup, 2: sleep
}__attribute__((packed)) main_info_t;


/**
 * @brief  oled 에 표시할 정보 구조체
 */
typedef struct _oled_info_t
{
	main_info_t		main_t;
	u8				p_ssid[64];
	u8				p_ip[64];
	u32				n_error_num;
	bool			b_error_update_flag;

}__attribute__((packed)) oled_info_t;


/*******************************************************************************
*   Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*   Variable Definitions
*******************************************************************************/


/*******************************************************************************
*   Function Prototypes
*******************************************************************************/
void*         	oled_routine( void* arg );

/**
 * @class   
 * @brief   
 */
class CVWOLEDimage
{
private:
    
    OLED_DISP_BASIC_SEQ m_basic_disp_seq_e;

    s8                  m_p_log_id[15];
    s32					m_n_spi_fd;
    s32					m_n_gpio_fd;
    
    CMutex *		    event;
	void			    lock(void){event->lock();}
	void			    unlock(void){event->unlock();}
	
	CTime*              m_p_oled_on_timer_c;
	    
	bool				m_b_button_event_req;
	bool				m_b_action_event_req;
	OLED_EVENT			m_cur_oled_event;
	OLED_STATE			m_oled_state_e;
	bool				m_b_state_updated;
	bool				m_b_ssid_updated;
	bool				m_b_ip_updated;
	bool				m_b_is_oled_on;
	bool				m_b_error_occured;
	bool				m_b_return_to_basic_disp;
	
	WIFI_STEP_NUM		m_wifi_step_num_e;
	
    CVWSPI*				m_p_spi_c;
    bool				m_b_oled_proc_is_running;
    pthread_t			m_oled_thread_t;
    
    main_info_t			m_updated_main_t;
    oled_info_t			m_oled_info_t;   
    
    u32					m_n_scroll_end_column; 
	
	u32 				m_p_pos_info[eOLED_NUM_OF_AREA][4];
		
    // MODE Area
    static const u8 	m_p_mode_ap_img[OLED_IMG_MODE_SIZE];
    static const u8 	m_p_mode_none_img[OLED_IMG_MODE_SIZE];
    static const u8 	m_p_mode_station_img[OLED_IMG_NUM_OF_STATION_TYPE][OLED_IMG_MODE_SIZE];
    static const u8 	m_p_mode_wifi_img[OLED_IMG_NUM_OF_STATION_TYPE][OLED_IMG_MODE_SIZE];
    static const u8 	m_p_mode_tether_img[OLED_IMG_NUM_OF_TETHER_TYPE][OLED_IMG_MODE_SIZE];
    static const u8 	m_p_mode_sleep_img[OLED_IMG_MODE_SIZE];
    
    // Battery Area
    static const u8 	m_p_bat_img[OLED_IMG_BAT_SIZE];
    static const u8 	m_p_bat_soc_img[OLED_IMG_NUM_OF_BAT_SOC_TYPE][OLED_IMG_BAT_SOC_SIZE];
    static const u8 	m_p_charge_img[2][OLED_IMG_CHARGE_EN_SIZE];
    
    // Characters, numerics, point, hyphen, underbar, colon, comma
    static const u8 	m_p_upper_char[OLED_IMG_NUM_OF_CHARACTERS][OLED_IMG_CHARACTER_SIZE_BYTE];
	static const u8 	m_p_lower_char[OLED_IMG_NUM_OF_CHARACTERS][OLED_IMG_CHARACTER_SIZE_BYTE];
	static const u8 	m_p_numeric[OLED_IMG_NUM_OF_NUMERIC][OLED_IMG_CHARACTER_SIZE_BYTE];
    static const u8 	m_p_point[OLED_IMG_CHARACTER_SIZE_BYTE];
    static const u8 	m_p_hyphen[OLED_IMG_CHARACTER_SIZE_BYTE];
    static const u8 	m_p_underbar[OLED_IMG_CHARACTER_SIZE_BYTE];
    static const u8 	m_p_colon[OLED_IMG_CHARACTER_SIZE_BYTE];
    static const u8 	m_p_comma[OLED_IMG_CHARACTER_SIZE_BYTE];
  	static const u8 	m_p_blank[OLED_IMG_CHARACTER_SIZE_BYTE];
    
	OLED_STATE			get_cur_state(void);
	OLED_DISP_BASIC_SEQ	get_cur_basic_display_seq(void){return m_basic_disp_seq_e;}
	void				set_basic_display_seq(OLED_DISP_BASIC_SEQ i_seq_e){m_basic_disp_seq_e = i_seq_e;}
	void				set_next_basic_display_seq(void);

	bool				is_oled_off_condition(void);
	
	u32					oled_get_size_of_area(u32 i_n_area);
	void				oled_set_region_by_area(u32 i_n_area);
		
    u8* 				get_mode_img(u32 i_n_mode, u32 i_n_status);
	u8* 				get_battery_img(void);
	u8* 				get_bat_soc_img(u32 i_n_soc_level);
	u8* 				get_charge_bat_img(bool i_b_charge);
	u8* 				get_sleep_img(void);
		
	void 				get_preset_img(u8* o_p_img, u32* o_p_len);
	void 				get_ip_img(u8* o_p_img, u32* o_p_len);
	void 				get_ssid_img(u8* o_p_img, u32* o_p_len);
	void 				get_error_img(u8* o_p_img, u32* o_p_len);
	void 				get_operating_img(u8* o_p_img, u32* o_p_len);
	void				get_scroll_img(u8* o_p_img, u32* o_p_total_colum_num);
	
	u32* 				get_pos_info(u32 i_n_area_type);
	
    void				make_string_img_data(u8* o_p_dest_buf, u8* i_p_src_str, u32 i_n_len);
    
    void				draw_main_display(bool i_b_draw_all);
    void				oled_display_onoff(u8 i_c_onoff);
    
    bool				prepare_scroll_img(void);
    void				scroll_img(u32* i_p_idx);
    void				blink_preset_name(void);
    	
    OLED_STATE			state_oled_idle(void);
    OLED_STATE			state_oled_basic_disp(void);
    OLED_STATE			state_oled_action_disp(void);
    void				state_print(OLED_STATE i_state_e);

    
protected:
    
public:

	CVWOLEDimage(int (*log)(int,int,const char *,...));
	~CVWOLEDimage();	
	
	bool				m_b_preset_blink;
	
	void				set_spi_fd(s32 i_n_fd){m_n_spi_fd = i_n_fd;}
	void				set_gpio_fd(s32 i_n_fd){m_n_gpio_fd = i_n_fd;}
	
    int			        (* print_log)(int level,int print,const char * format, ...);
    
	friend void *       oled_routine( void * arg );
	
	void                oled_proc(void);
	   
	void				start(void);
	void				oled_init(void){m_p_spi_c->oled_init();}
	void				oled_sw_reset(void){m_p_spi_c->oled_sw_reset();}
	
	void 				oled_control(u8 i_n_idx, u8* i_p_val = NULL, u32 i_n_length = 0){m_p_spi_c->oled_sp9020_write(i_n_idx, i_p_val, i_n_length);}
	
	bool				is_oled_on(void){return m_b_is_oled_on;}
		
	void				set_oled_display_event(OLED_EVENT i_event_e);
		
	void				update_ssid(u8* i_p_ssid);
	void				update_ip(u8* i_p_ip);
	void 				update_state(sys_state_t* i_p_state_t);
	void				update_error(OLED_ERROR i_error_e, bool i_b_err_flag);
	void				update_preset(u8* i_p_preset_name);
	
	void 				set_wifi_signal_step_num(WIFI_STEP_NUM i_step_num_e);
	
};



#endif /* _OLED_IMAGE_ */
