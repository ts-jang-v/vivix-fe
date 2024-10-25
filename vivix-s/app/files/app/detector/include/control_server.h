/*
 * comm.h
 *
 *  Created on: 2013. 12. 17.
 *      Author: myunghee
 */

 
#ifndef _CONTROL_SERVER_H_
#define _CONTROL_SERVER_H_

/*******************************************************************************
*	Include Files
*******************************************************************************/


#ifdef TARGET_COM
#include "typedef.h"
#include "mutex.h"
#include "message.h"
//#include "stream_server.h"
#include "tcp_server.h"
#include "payload_def.h"
#include "image_def.h"
#include "vw_system.h"
#include "vw_time.h"
#include "blowfish.h"
#include "vw_download.h"
#include "calibration.h"
#include "net_manager.h"
#include "diagnosis.h"
#include "web.h"
#include "image_retransfer.h"
#else
#include "typedef.h"
#include "Tcp_server.h"
#include "Message.h"
#include "Time.h"
#include "../app/detector/include/payload_def.h"
#include "Common_def.h"
#include "Vw_download.h"
#include "Vw_system.h"
#include "Calibration.h"
#include "Net_manager.h"
#include "Diagnosis.h"
#include "Web.h"

#include "../app/utils/include/blowfish.h"
#include "../app/detector/include/message_def.h"
#include "../app/detector/include/image_def.h"
//#include "Time.h"
//#include "Common_def.h"
//#include "Vw_system.h"
//#include "File.h"
//#include "Calibration.h"

//#include "Vw_system.h"
//#define _IOWR(a,b,c) b
//#include "../driver/include/vworks_ioctl.h"
#endif
/*******************************************************************************
*	Constant Definitions
*******************************************************************************/
#define CONTROL_SVR_PORT            (5005)

//#define DELAY_PACKET_TEST


/*******************************************************************************
*	Type Definitions
*******************************************************************************/
// resend state
typedef enum
{
    eRESEND_STATE_IDLE          = 0,
    eRESEND_STATE_ACQ_START,
    eRESEND_STATE_ACQ_END,
} RESEND_STATE;

/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*	Variable Definitions
*******************************************************************************/


/*******************************************************************************
*	Function Prototypes
*******************************************************************************/
void*           alive_routine( void* argc );
void*           noti_routine( void * arg );
void*           stream_routine( void * arg );
void*           resend_routine( void * arg );

void*           trans_test_routine( void * arg );

class CControlServer : public CTCPServer
{
private:
    int			        (* print_log)(int level,int print,const char * format, ...);
    bool                m_b_debug;
    
    // notification
    CMessage*           m_p_msgq_c;
    pthread_t			m_noti_thread_t;
    bool				m_b_noti_is_running;
	void				noti_proc(void);
	void                message_send( MSG i_msg_e, u8* i_p_data=NULL, u16 i_w_data_len=0 );
	
    // alive check
    bool                m_b_alive_is_running;
    pthread_t	        m_alive_thread_t;
    u32                 m_n_alive_interval_time;
    CTime*              m_p_alive_timer;
    bool                m_b_alive_timer_update;
    
    void                alive_proc(void);
    void                alive_reset(void);
    
    // client management
    bool                m_b_client_valid;
    u32                 m_n_client_seq_id;
    s32                 m_n_client_fd;

    // send syncronization
    CMutex*             m_p_mutex;
	void                lock_send( void ){ m_p_mutex->lock(); }
	void                unlock_send( void ){ m_p_mutex->unlock(); }

    
    CBlowFish           m_cipher_c;
    bool                client_decode( login_t* i_p_login_t );
    
    // image send
    image_buffer_t*     m_p_stream_image_t;
    u32                 m_n_stream_payload;
    u32                 m_n_stream_crc;
    
    u16                 m_w_stream_slice_total;
    u16                 m_w_stream_slice_current;
    
    pthread_t			m_stream_thread_t;
    bool				m_b_stream_is_running;
    bool				m_b_stream_sending_error;
    void				stream_proc(void);
	bool                stream_send( bool i_b_preview, u8* i_p_buff, u32 i_n_length );
	
	bool                m_b_stream_send_enable;
	void                stream_wait_start(void);
	
	u32                 m_n_stream_id;

    // system update
    CVWDownload*        m_p_download_c;
    s8                  m_p_file_name[256];
    VW_FILE_ID          m_file_id_e;
    u32                 m_n_file_crc;
    
    bool                download_start( VW_FILE_ID i_id_e, u32 i_n_crc, u32 i_n_total_size );
    bool                download_set_data( u8* i_p_data, u16 i_w_len );
    bool                download_verify( VW_FILE_ID i_id_e );
    bool                download_compatibility_check( VW_FILE_ID i_id_e );
    bool                download_check_crc(void);
    bool                download_save( VW_FILE_ID i_id_e );
    void                download_end(void);
    
    // throughput test
    pthread_t			m_trans_thread_t;
    bool				m_b_trans_is_running;
    u16                 m_w_trans_slice_id;
	void				trans_proc(void);
	bool                trans_start(void);
	void                trans_stop(void);
	bool                trans_send(void);
	    
    // offset cal
    bool                m_b_offet_cal_stop;
    // packet processing
    bool		        packet_parse( client_info_t* i_p_client_info_t, u8* i_p_data, u16 i_w_len );
    bool                packet_send( u32 i_n_payload, u32 i_n_seq_id, \
                                        u8* i_p_buff=NULL, u32 i_n_length=0, \
                                        u8 i_c_result=0 );
    
    bool                req_login( u8* i_p_data );
    bool                req_login_ack(void);
    
    bool                req_config_reset( u8* i_p_data );
    bool                req_reboot( u8* i_p_data );
    bool                req_manufacture_info( u8* i_p_data );
    bool                req_package_version( u8* i_p_data );
    bool                req_upload( u8* i_p_data );
    bool                req_download( u8* i_p_data );
    bool                req_memory_clear( u8* i_p_data );
    bool                req_function_list( u8* i_p_data );
    void				function_list_set_supported( function_list_ack_t* o_p_ack_t );
    
    // detector only
    bool                req_state( u8* i_p_data );
    bool                req_sleep_control( u8* i_p_data );
    bool                req_device_register( u8* i_p_data );
    
    bool                req_dark_image( u8* i_p_data );
    bool                req_preview_enable( u8* i_p_data );
    bool                req_resend_image( u8* i_p_data );
    bool                req_image_trans_done( u8* i_p_data );
    bool                req_backup_image_delete( u8* i_p_data );
    bool                req_self_diagnosis( u8* i_p_data );
    bool                req_acceleration_control( u8* i_p_data );
    bool                req_ap_information( u8* i_p_data );
	bool                req_web_configuration( u8* i_p_data );
	bool				req_packet_trigger( u8* i_p_data );
    bool		        req_image_retransmission( u8* i_p_data );
    bool                req_sensitivity_calibration( u8* i_p_data );
    bool                req_gain_compensation_control( u8* i_p_data );

    bool                reqCalibration(u8* pData);
    bool                req_offset_drift( u8* i_p_data );
    bool                req_offset_control( u8* i_p_data );
    bool                req_cal_data_control( u8* i_p_data );
	
	bool				req_nfc_card_control( u8* i_p_data );

	bool				req_smart_w_control( u8* i_p_data );
	bool				req_aed_off_debounce_control( u8* i_p_data );
    bool                req_battery_charging_mode( u8* i_p_data );

    bool                req_trans_test( u8* i_p_data );
    bool                req_ap_channel_change( u8* i_p_data );
    bool                req_ap_channel_change_result( u8* i_p_data );

    bool                req_scintillator_type_range( u8* i_p_data );
    bool                req_offset_correction_ctrl( u8* i_p_data );
    bool                reqExposureControl(u8* pData);
    
    // noti processing
    bool                noti_offset_cal( u8* i_p_data=NULL, u16 i_w_lenth=0 );
    bool                noti_image( u8* i_p_data=NULL, u16 i_w_lenth=0 );
    bool                noti_micom( u8* i_p_data=NULL, u16 i_w_lenth=0 );
    bool                noti_power_mode( u8* i_p_data=NULL, u16 i_w_lenth=0 );
    void                noti_disconnect(void);
    bool                noti_osf( u8* i_p_data=NULL, u16 i_w_lenth=0 );
    
    void                parse_mac( s8* i_p_mac, u8* o_p_mac );
    
    // packet test
    u16                 m_w_packet_seq;
    
    void				disable_function_list(void);
    
protected:
public:
	CControlServer(int port, int max, int (*log)(int,int,const char *,...));
	CControlServer(void);
	~CControlServer();
	
	// server start
	
	//debug
	void                set_debug(bool i_b_debug){m_b_debug = i_b_debug;}
    
	// notification
	bool                noti_add( vw_msg_t * msg );
	friend void * 		noti_routine( void* arg );
	
	// image data transfer
	bool                sender_is_ready(void);
	
	bool                sender_start(image_buffer_t* i_p_image_t, u32 i_n_payload=PAYLOAD_IMAGE, u32 i_n_crc = 0);
	
	void                sender_stop(void);
	
	// alive check
    friend void *       alive_routine( void* arg );
	
	// client management
	void		        client_notify_close( s32 i_n_fd );
	void                client_notify_recv_error( s32 i_n_fd );
	
	// packet processing
	bool		        receiver_todo( client_info_t* i_p_client_t, u8* i_p_buf, s32 i_n_len );
	void		        receiver_started( s32 i_n_fd );
	
	//
	CVWSystem*          m_p_system_c;
	CCalibration*       m_p_calibration_c;
    CNetManager*		m_p_net_manager_c;
    CDiagnosis*			m_p_diagnosis_c;
	CWeb*				m_p_web_c;
	
	// test
	void                test_packet_send( u32 i_n_size );
    void                send_stream_data( u32 i_n_payload );

    friend void * 		stream_routine( void* arg );
	
	void                packet_test( u16 i_w_sequence_number ){m_w_packet_seq = i_w_sequence_number;}
    
    // throughput test
    friend void*        trans_test_routine( void* arg );
    
    // image retransfer
    CImageRetransfer*	m_p_img_retransfer_c;
    bool				retransmit( u32 i_n_image_id, bool i_b_preview, u8* i_p_buff, u32 i_n_length, u16 i_w_slice_number, u16 i_w_total_slice_number );
    void				test_resend( image_retrans_req_t* i_p_req_t );
    
    void				initialize(void);
};


#endif /* end of _CONTROL_SERVER_H_*/

