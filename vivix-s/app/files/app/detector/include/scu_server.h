/******************************************************************************/
/**
 * @file    scu_server.h
 * @brief   기존 SCU와 신규 SCU 간의 통신 호환성을 유지하기 위한 모듈이다.
 *          기존에 사용했던 프로토콜을 이용하여 5001번 포트로 통신을 한다.
 * @author  
*******************************************************************************/

 
#ifndef _SCU_SERVER_H_
#define _SCU_SERVER_H_

/*******************************************************************************
*	Include Files
*******************************************************************************/
#include "typedef.h"
#include "mutex.h"
#include "message.h"
#include "vw_system.h"
#include "vw_time.h"
#include "blowfish.h"
#include "tcp_server.h"

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/
#define SCU_SVR_PORT            (5001)

#define     OPT_EXT     0x80
#define     OPT_MUL     0x40
#define		OPT_ACK		0x10
#define     OPT_STA     0x01
#define     OPT_END     0x02

/*******************************************************************************
*	Type Definitions
*******************************************************************************/
typedef struct
{
    u8      head[4];
    u8      payload;
    u8      opt;
    u16     payload_length;
    u16     slice_number;
    u16     slice_size;
}__attribute__((packed)) mp_header_t;


/**
 * @brief   기존 SCU의 프로토콜 payload
 */
typedef enum
{
	VWSP_PAYLOAD_LOGIN      = 0x01,
	VWSP_PAYLOAD_CAPTURE    = 0x07,
	VWSP_PAYLOAD_GOODBYE    = 0x09,
	VWSP_PAYLOAD_EXP_OK     = 0x0B,
	VWSP_PAYLOAD_KEEP_ALIVE = 0x0C,
	
}VWSP_PAYLOAD;

/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*	Variable Definitions
*******************************************************************************/


/*******************************************************************************
*	Function Prototypes
*******************************************************************************/
void*           scu_alive_routine( void* argc );
void*           scu_noti_routine( void * arg );

class CSCUServer : public CTCPServer
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
	void                noti_disconnect(void);
	
    // alive check
    bool                m_b_alive_is_running;
    pthread_t	        m_alive_thread_t;
    u32                 m_n_alive_interval_time;
    CTime*              m_p_alive_timer;
    bool                m_b_alive_timer_update;
    
    void                alive_proc(void);
    void                alive_reset(void);
    bool                alive_req(void);
    
    // client management
    bool                m_b_client_valid;
    s32                 m_n_client_fd;
    bool                check_scu( u32 i_n_time, u8* i_p_encode );
    
    // send syncronization
    CMutex*             m_p_mutex;
	void                lock_send( void ){ m_p_mutex->lock(); }
	void                unlock_send( void ){ m_p_mutex->unlock(); }
        
    CBlowFish           m_cipher_c;
    
    // packet processing
    bool		        packet_parse( client_info_t* i_p_client_info_t, u8* i_p_data, u16 i_w_len );
    bool                packet_send( u8 i_c_payload, u8* i_p_buff=NULL, u32 i_n_length=0);
    
    bool                login_req( u8* i_p_data );
    bool                logout_req(void);
    bool                exp_req(void);
    bool                exp_ack(void);
    
protected:
public:
	CSCUServer(int port, int max, int (*log)(int,int,const char *,...));
	CSCUServer(void);
	~CSCUServer();
    
	// notification
	bool                noti_add( vw_msg_t * msg );
	friend void * 		scu_noti_routine( void* arg );
	
	// alive check
    friend void *       scu_alive_routine( void* arg );
	
	// client management
	void		        client_notify_close( s32 i_n_fd );
	void                client_notify_recv_error( s32 i_n_fd );
	
	// packet processing
	bool		        receiver_todo( client_info_t* i_p_client_t, u8* i_p_buf, s32 i_n_len );
	void		        receiver_started( s32 i_n_fd );
	
	//
	CVWSystem*          m_p_system_c;
	
	void                set_debug(bool i_b_debug){m_b_debug = i_b_debug;}

};


#endif /* end of _SCU_SERVER_H_*/

