/*
 * system.h
 *
 *  Created on: 2015. 10. 02.
 *      Author: myunghee
 */

 
#ifndef _VW_NET_MANAGER_H_
#define _VW_NET_MANAGER_H_

/*******************************************************************************
*	Include Files
*******************************************************************************/
#include "typedef.h"
#include "message_def.h"
#include "message.h"
#include "vw_system.h"
#include "vw_time.h"

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/

/*******************************************************************************
*	Type Definitions
*******************************************************************************/
typedef struct 
{
    bool        b_sync;
	sync_ack_t  ack_t;
} sync_info_t;

typedef struct 
{
	bool		b_associated;
	s32			n_signal;
	char		p_ssid_str[100];
} __attribute__((packed)) ap_candidate_t;		
	
/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*	Variable Definitions
*******************************************************************************/


/*******************************************************************************
*	Function Prototypes
*******************************************************************************/
void*           ap_scan_routine( void* arg );
void*           net_state_routine( void* arg );

class CNetManager
{
private:
	s8					m_p_log_id[15];
	int					(* print_log)(int level,int print,const char * format, ...);
	
	bool				m_b_ap_scan_is_running;
	pthread_t			m_ap_scan_thread_t;
	
	CTime*				m_p_ap_sw_mode_timer_c;	
	ap_candidate_t		m_ap_candidate_t[MAX_CFG_COUNT];
	void				ap_scan_proc(void);
	
	u8					m_c_ap_scan_progress;
	bool				m_b_ap_scan_result;
	u32					m_n_ap_count;
    
	void				ap_scan_make_info(void);

	// notification
	bool                (* notify_handler)(vw_msg_t * msg);
	void				message_send( MSG i_msg_e, u8* i_p_data=NULL, u16 i_w_data_len=0 );

    // networks
    CMutex*             m_p_mutex_c;
    void                lock(void){m_p_mutex_c->lock();}
    void                unlock(void){m_p_mutex_c->unlock();}

    bool                m_b_is_running;
    pthread_t	        m_net_state_thread_t;
    NET_MODE            m_current_net_mode_e;   // 한 thread 내에서만 값이 변경되도록 해야 한다.
    bool                m_b_link_state;
    bool                m_b_ap_connected;       // station mode에서 AP가 연결되어 있는지 확인
    bool                m_b_wlan_enable;
    bool                m_b_ap_on;

    u16                 m_w_ap_channel;
    bool                m_b_ap_channel_result;
    
    // AP Sync
    sync_info_t         m_sync_info_t;
    
    bool				m_b_phy_reset_done;
    
    void                net_state_proc(void);
    
    NET_MODE            net_state_init(void);
    NET_MODE            net_state_tether( bool i_b_force=false );
    NET_MODE            net_state_station(void);
    NET_MODE            net_state_ap(void);
    void                net_state_print( NET_MODE i_mode_e );
    bool                net_state_check_tether(void);
    void 				net_speed_confirm( bool i_b_phy_reset = true);
    bool 				net_br_lan_state(void);
    bool 				net_br_lan_state_up_check(void);


    void 				net_phy_reset(void);
    void				net_phy_reset_clear(void) {m_b_phy_reset_done = false;}
    void				net_phy_reset_done(void) {m_b_phy_reset_done = true;}
    bool				net_can_do_phy_reset() {return m_b_phy_reset_done == false;}

    void                net_check_ip(NET_MODE i_net_mode_e);
    s32                 net_check_ip_collision(NET_MODE i_net_mode_e);
    
    u16                 net_get_tether_speed(void);
    void				net_dhcp_start( bool i_b_start=true);
    
    void				check_valid_speed(void);
    
        // wifi detection check
    bool				m_b_wifi_detection;
    void				wifi_detection_check(void);
    bool				wifi_detection_wait(void);
    
    bool				m_b_display_link_quality;
    CTime*				m_p_display_link_quality_time_c;
    
    bool				find_ap_candidates(void);
    bool				change_ap(void);
    
protected:
public:
    
	CNetManager(int (*log)(int,int,const char *,...));
	CNetManager(void);
	~CNetManager();
	
	bool				m_b_ap_switching_test_mode;
	
	friend void *		ap_scan_routine( void * arg );
	bool				ap_scan_proc_start(void);
	void				ap_scan_proc_stop(void);
    
	u8					ap_scan_get_progress(void){return m_c_ap_scan_progress;}
	bool				ap_scan_get_result(void){return m_b_ap_scan_result;}

    CVWSystem*          m_p_system_c;
    void				start(void);
	
	// notification
	void                set_notify_handler(bool (*noti)(vw_msg_t * msg));

	friend void *       net_state_routine( void * arg );
    void                net_state_proc_start(void);
    void                net_state_proc_stop(void);
    void 				net_state_proc_thread_join(void);


    void                net_set_mode( NET_MODE i_net_mode_e );
    void                net_set_interface(NET_MODE i_net_mode_e);
    
	
	bool                net_is_ready(void);
    
    bool                net_discovery(void);
    
    void                net_change_ip(NET_MODE i_net_mode_e);
    void				net_check_subnet( u32 i_n_src_addr );
    
    bool                net_ap_set_channel( u16 i_w_num );
    u16                 net_ap_get_channel(void){return m_w_ap_channel;}
    bool                net_ap_get_channel_result(void){return m_b_ap_channel_result;}
    void                net_get_mac( u8* o_p_mac, bool b_registered=false );
    
    bool				ap_switching(void);
};


#endif /* end of _VW_NET_MANAGER_H_*/

