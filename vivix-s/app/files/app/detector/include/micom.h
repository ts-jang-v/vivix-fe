/******************************************************************************/
/**
 * @file    micom.h
 * @brief   micom communication
 * @author  
*******************************************************************************/


#ifndef _VW_MICOM_H_
#define _VW_MICOM_H_

/*******************************************************************************
*   Include Files
*******************************************************************************/
#include "typedef.h"
#include "mutex.h"
#include "vw_system_def.h"

/*******************************************************************************
*   Constant Definitions
*******************************************************************************/

//command
#define STX							0x02	/* Start of message identifier */
#define ETX							0x03	/* End of message ideitifier */

#define MAX_PAGE_COUNT				128

#define	ERROR_CRC					0x10
#define ERROR_WRONG_WRITE			0x20

#define COMMAND_HEADER_LEN          (2)
#define COMMAND_TAIL_LEN            (2)

#define COMMAND_DATA_LEN            (8)
#define COMMAND_BUFFER_LEN          (COMMAND_HEADER_LEN + COMMAND_DATA_LEN + COMMAND_TAIL_LEN)

#define COMMAND_BIG_DATA_LEN		(128)
#define COMMAND_BIG_BUFFER_LEN		(COMMAND_HEADER_LEN + COMMAND_BIG_DATA_LEN + COMMAND_TAIL_LEN)

//spi delay
#define MCU_RDELAY_SHORT			20000		//20ms
#define MCU_RDELAY_LONG				100000		//100ms
#define MCU_NVM_DELAY_LONG			100000		//100ms

#define IMPACT_GROUP_SIZE     		(2)
#define ADXL375B_NUMBER_OF_AXIS		(3)
#define ADXL375B_DATA_SIZE			(ADXL375B_NUMBER_OF_AXIS * 2)
#define ADXL375B_MICOM_FIFO_SIZE	(128 * IMPACT_GROUP_SIZE)
#define ADXL375B_MICOM_BUF_SIZE		(ADXL375B_MICOM_FIFO_SIZE * ADXL375B_DATA_SIZE)

#define PAGESIZE					64

/***************************************************************************************************
*	Type Definitions
***************************************************************************************************/

enum
{
	eMCU_ADC_ID_A = 0,
	eMCU_ADC_ID_B,
	eMCU_ADC_THM_A,
	eMCU_ADC_THM_B,
	eMCU_ADC_AED_A,
	eMCU_ADC_AED_B,
	eMCU_ADC_BATT,
	eMCU_ADC_VS,
};

/**
 * @brief   micom event
 */
typedef enum
{
    eMICOM_EVENT_IDLE       				= 0,
    eMICOM_EVENT_POWER_OFF  				= 0x01,
    eMICOM_EVENT_WAKE_UP    				= 0x02,
    eMICOM_EVENT_VERSION    				= 0x03,
    eMICOM_EVENT_STATE      				= 0x04,
    eMICOM_EVENT_ERROR      				= 0x05,
    eMICOM_EVENT_REBOOT     				= 0x06,
    eMICOM_EVENT_SLEEP_ON   				= 0x07,
    eMICOM_EVENT_SLEEP_OFF  				= 0x08,
    eMICOM_EVENT_UPDATE_REQ,
    eMICOM_EVENT_UPDATE_CMD 				= 0x0A,
	eMICOM_EVENT_UPDATE_BOOT_ACK,
	eMICOM_EVENT_UPDATE_DATA,
	eMICOM_EVENT_GET_BOOTLOADER_VERSION,
	eMICOM_EVENT_AED_ADC,
	eMICOM_EVENT_BAT_ADC,
	
	eMICOM_EVENT_IMPACT_NO_DATA 			= 0x10,
	eMICOM_EVENT_IMPACT_RESEND 				= 0x11,
	eMICOM_EVENT_IMPACT_BUF_CNT 			= 0x12,	//0x12
	eMICOM_EVENT_IMPACT_BUF_ONE_CLEAR,				//0x13	
	eMICOM_EVENT_IMPACT_REG_READ,					//0x14	
	eMICOM_EVENT_IMPACT_REG_WRITE,					//0x15	
	eMICOM_EVENT_IMPACT_REG_SAVE,					//0x16	
	eMICOM_EVENT_IMPACT_BUF_RESET,					//0x17
	eMICOM_EVENT_ACC_GYRO_CAL_DATA_SAVE,			//0x18
	eMICOM_EVENT_ACC_GYRO_CAL_DATA_REQ,				//0x19
	
	eMICOM_EVENT_TIMESTAMP_GET 				= 0x1A,	//0x1A	
	eMICOM_EVENT_TIMESTAMP_SET,						//0x1B
	eMICOM_EVENT_CHK_REBOOT,
	eMICOM_EVENT_AUTO_POWER_ON_OFF_CONFIG,			//0x1D
	eMICOM_EVENT_WPT_PD,

	eMICOM_EVENT_EEPROM_REQUEST = 0x20,
	eMICOM_EVENT_EEPROM_DATA,
	eMICOM_EVENT_LIFESPAN_EXTENSION,				//0x22
	eMICOM_EVENT_NEW_IMPACT_LOGIC,					//0x23
	eMICOM_EVENT_LED_CONTROL,						//0x24
	
	eMICOM_EVENT_IMPACT_0 = 0x40,
	eMICOM_EVENT_IMPACT_1,
	eMICOM_EVENT_IMPACT_2,
	eMICOM_EVENT_IMPACT_3,
	eMICOM_EVENT_IMPACT_4,
	eMICOM_EVENT_IMPACT_5,   
	eMICOM_EVENT_IMPACT_6,   
	eMICOM_EVENT_IMPACT_7,   
	eMICOM_EVENT_IMPACT_8,   
	eMICOM_EVENT_IMPACT_9,   
	eMICOM_EVENT_IMPACT_10,
	eMICOM_EVENT_IMPACT_11,
	eMICOM_EVENT_IMPACT_12,
	eMICOM_EVENT_IMPACT_13,
	eMICOM_EVENT_IMPACT_14,
	eMICOM_EVENT_IMPACT_15,
	eMICOM_EVENT_IMPACT_16,
	eMICOM_EVENT_IMPACT_17,
	eMICOM_EVENT_IMPACT_18,
	eMICOM_EVENT_IMPACT_19,
	eMICOM_EVENT_IMPACT_20,
	eMICOM_EVENT_IMPACT_21,
	eMICOM_EVENT_IMPACT_22,
	eMICOM_EVENT_IMPACT_23,
	eMICOM_EVENT_IMPACT_24,
	eMICOM_EVENT_IMPACT_25,
	eMICOM_EVENT_IMPACT_26,
	eMICOM_EVENT_IMPACT_27,
	eMICOM_EVENT_IMPACT_28,
	eMICOM_EVENT_IMPACT_29,
	eMICOM_EVENT_IMPACT_30,
	eMICOM_EVENT_IMPACT_31,
	eMICOM_EVENT_IMPACT_32,
	eMICOM_EVENT_IMPACT_33,
	eMICOM_EVENT_IMPACT_34,
	eMICOM_EVENT_IMPACT_35,
	eMICOM_EVENT_IMPACT_36,
	eMICOM_EVENT_IMPACT_37,
	eMICOM_EVENT_IMPACT_38,
	eMICOM_EVENT_IMPACT_39, 
	eMICOM_EVENT_IMPACT_40,
	eMICOM_EVENT_IMPACT_41,
	eMICOM_EVENT_IMPACT_42,
	eMICOM_EVENT_IMPACT_43,
	eMICOM_EVENT_IMPACT_44,
	eMICOM_EVENT_IMPACT_45,
	eMICOM_EVENT_IMPACT_46,
	eMICOM_EVENT_IMPACT_47,
	eMICOM_EVENT_IMPACT_48,
} MICOM_EVENT;

typedef enum
{
	eUPDATE_CMD_NONE = 0x00,
	eUPDATE_CMD_START,
}UPDATE_CMD_TYPE;

typedef enum
{
    eIMPACT_REG_THRESH_SHOCK       = 0x1D,
    
    eIMPACT_REG_OFS_X       = 0x1E,
    eIMPACT_REG_OFS_Y       = 0x1F,
    eIMPACT_REG_OFS_Z       = 0x20,
    
    eIMPACT_REG_DATA_X0     = 0x32,
    eIMPACT_REG_DATA_X1     = 0x33,
    eIMPACT_REG_DATA_Y0     = 0x34,
    eIMPACT_REG_DATA_Y1     = 0x35,
    eIMPACT_REG_DATA_Z0     = 0x36,
    eIMPACT_REG_DATA_Z1     = 0x37,
    
} IMPACT_REG;

typedef struct _micom_state
{
    u8          c_state;
    
    union
    {
        u8  DATA;
        struct
        {
            u8  EXT_WPT_REQ     		: 1;
        	u8 	WPT_HALL_SENSOR_ACTIVE	: 1; //is Hall sensor active 
            u8  WPT_FINAL_STATE 		: 1; //WPT final(current) state
			u8  WAKEUP_REQ     			: 1;
			u8  POWER_OFF       		: 1;
            u8  ADXL_FULL      			: 1;
            u8  POE             		: 1;
            u8  M_BTN          			: 1;
        }BIT;
    }IO_STATUS;
    
	adxl_data_t adxl_t;
}micom_state_t;

typedef union _power_ctrl
{
	u8  c_by_scu_by_detector;
	u8	c_auto_power_on; // Enable: 1, Disable: 0
	u8	c_auto_power_off;	 // Enable: 1, Disable: 0
}power_ctrl_t;


typedef struct _page_list_
{
	u8			data[PAGESIZE];
	u8			is_used;
}page_t;


/*******************************************************************************
*   FOR X/YMODEM
*******************************************************************************/
#define USE_MICOM_XYMODEM			1

#define X_STX 0x02
#define X_ACK 0x06
#define X_NAK 0x15
#define X_EOF 0x04

#define CRC_POLY 0x1021

#define MIN(a, b)					((a) < (b) ? (a) : (b))

enum {
		PROTOCOL_XMODEM,
		PROTOCOL_YMODEM,
};

struct xmodem_chunk {
		u8 start;
		u8 block;
		u8 block_neg;
		u8 payload[1024];
		u16 crc;
} __attribute__((packed));

#define UCMD_SIZE					2
#define MASTER_HEADER				0xAA
#define SLAVE_HEADER				0x55

#define DOWNLOAD_CMD				1
#define UPLOAD_CMD					2
#define REBOOT_CMD					3
#define PROTECT_W_CMD				4
#define PROTECT_R_CMD				5
#define SLAVE_RX_READY				0xFF

#define COMPLETE_CMD				0x80

/* protection type */  
enum{
  FLASHIF_PROTECTION_NONE         = 0x40,
  FLASHIF_PROTECTION_PCROPENABLED = 0x41,
  FLASHIF_PROTECTION_WRPENABLED   = 0x42,
  FLASHIF_PROTECTION_RDPENABLED   = 0x44,
};


/*******************************************************************************
*   Variable Definitions
*******************************************************************************/


/*******************************************************************************
*   Function Prototypes
*******************************************************************************/


/**
 * @class   
 * @brief   
 */
class CMicom
{
private:
    s8          m_p_log_id[15];
    s32         (* print_log)(s32 level,s32 print,const s8 * format, ...);
    u8      	m_resend_buf[COMMAND_BIG_BUFFER_LEN];
	u8      	m_resend_size;

    // spi port
    s32         m_n_fd;
	// uart port, for self-programming
    s32			m_n_fd_self;
    
    
    bool        m_b_debug;
    
    
    // micom upgrade
    u32 		m_n_index;
    u8*			m_p_data;
    u8			m_c_crc;
    
    CMutex*		m_p_mutex_c;
    void		lock(void){m_p_mutex_c->lock();}
    void		unlock(void){m_p_mutex_c->unlock();}

	bool		m_b_msg_stop;
	bool		m_b_update_started;

	int			open_serial(const char *path);
	int			xymodem_send(int serial_fd, const char *filename, int protocol, int wait, u16 *progress);
	void 		dump_serial(void);
	bool		send_file(const s8 *filename, u16 *progress);
	bool		uart_tx(u8 *tx_addr, u8 len);
	void		uart_rx(u8 *rx_addr, u8 len);
	bool		uart_update_stm32f(const s8 *filename, u16 *progress);
	void		uart_update_stm32f_finish(void);

	u32			eeprom_unit_read( u16 i_w_addr, u8* o_p_data, u32 i_n_len, eeprom_blk_num_t i_n_eeprom_blk_num = eMICOM_EEPROM_BLK_FRAM );
	u32			eeprom_unit_write( u16 i_w_addr, u8* i_p_data, u32 i_n_len, eeprom_blk_num_t i_n_eeprom_blk_num = eMICOM_EEPROM_BLK_FRAM );
	
	void		print_reboot_status( u8 i_c_code );
    // micom update
    bool		m_b_update_is_running;
    pthread_t	m_update_thread_t;
    bool		m_b_update_result;
    s8			m_p_update_file_name[256];
    u16			m_w_update_progress;
    bool		m_b_updated;
    bool		update_using_uart(void);
    
    bool		_get( MICOM_EVENT i_event_e, u8* a_p_send=NULL, u8* a_p_recv=NULL, bool big_size=false );

protected:
    
public:
	CMicom(s32 (*log)(s32,s32,const s8 *,...));
	CMicom(void);
	~CMicom();
	
	bool        initialize(void);
	bool        recv( MICOM_EVENT* o_p_evnet_e, u8* o_p_event_data, u16 i_w_wait_time=200, bool big_cmd=0, u16 save_len=COMMAND_BIG_DATA_LEN);
 	//bool        send( u8 i_c_payload );
 	bool 		send( u8 i_c_payload , u8* i_b_data=NULL, bool big_cmd=0);
 	
 	void        set_debug( bool i_b_set ){m_b_debug = i_b_set;}
 	u8			get_byte(void);
 	u8			get_character(void) {return m_p_data[m_n_index++];}
 	u8          get_check_sum( u8* i_p_buffer, u16 i_w_len );

	u32			eeprom_read( u16 i_w_addr, u8* o_p_data, u32 i_n_len, eeprom_blk_num_t i_n_eeprom_blk_num);
    u32			eeprom_write( u16 i_w_addr, u8* i_p_data, u32 i_n_len, eeprom_blk_num_t i_n_eeprom_blk_num);

	bool		aed_read_trig_volt( AED_ID i_aed_id_e, u32* o_p_volt );

	void		impact_read_pos( s16* o_p_x, s16* o_p_y, s16* o_p_z );

	bool		get( MICOM_EVENT i_event_e, u8* a_p_send=NULL, u8* a_p_recv=NULL, bool big_size=false, bool i_b_update=false );
	
	void		print_adc(void);
	void		print_state(void);
	
	void		send_payload(u8 *a_b_payload, bool i_b_ack=true);
	
	bool		is_power_button_on(void);
	
	void		power_led_ready(void);
	
	bool        update_start( const s8* i_p_file_name);
	void 		update_stop(void);
    void 		update_proc(void);
	u16         get_update_progress(void) { return m_w_update_progress; }	
    bool		get_update_result(void) {return m_b_update_result;}
    
    void		reboot(void);
    void		power_off(void);
    
    void		set_sleep_led( bool i_b_on );
    
    u16			get_version(void);
    void		set_timestamp(void);
	void		set_power_ctrl_method(hw_config_t* i_p_hw_config_t);
	void		get_power_ctrl_method( u8* i_p_recv );
	void		set_lifespan_extension( bool i_b_en );
	void ControlLedBlinking(u8 enable);
};



#endif /* _VW_MICOM_H_ */
