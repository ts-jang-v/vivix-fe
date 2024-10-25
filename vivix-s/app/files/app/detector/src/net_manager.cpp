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

#include <linux/unistd.h>
#include <sys/syscall.h>

#include "net_manager.h"
#include "misc.h"
#include "vw_file.h"
#include "vw_net.h"

using namespace std;

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/
#define AP_SCAN_FILE		"/tmp/ap.scan"
//#define AP_SSID_LIST_FILE	"/tmp/ap.ssid"
//#define AP_SIGNAL_LIST_FILE	"/tmp/ap.signal"

#define MAX_AP_COUNT	100

#define MAX_5GHZ_TX_POWER_mW 50
#define MAX_2_4GHZ_TX_POWER_mW 30

/*******************************************************************************
*	Type Definitions
*******************************************************************************/
typedef struct _ap_info_t
{
    s8	p_ssid[256];
    s8	p_signal[20];
    s8	p_channel[20];
}ap_info_t;


/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/

/*******************************************************************************
*	Variable Definitions
*******************************************************************************/

#define AP_SCAN_RETRY_NUM_MAX       10

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
CNetManager::CNetManager(int (*log)(int,int,const char *,...))
{
	print_log = log;
	strcpy( m_p_log_id, "Net Manager   " );
	
    m_p_mutex_c = new CMutex;
    m_p_ap_sw_mode_timer_c = NULL;
    
    m_b_link_state = false;
    m_b_ap_connected = false;
    m_b_wlan_enable = false;
    m_b_ap_on = false;
    m_b_ap_channel_result = false;
    m_b_display_link_quality = false;

	m_b_ap_switching_test_mode = true;
	
    m_b_is_running = false;
    m_b_wifi_detection = false;
    memset( &m_sync_info_t, 0, sizeof(sync_info_t) );
    m_sync_info_t.b_sync = false;

    net_phy_reset_clear();
    
    m_n_ap_count = 0;
    m_net_state_thread_t = (pthread_t)0;
    m_w_ap_channel = 0;
    m_p_display_link_quality_time_c = NULL;
    m_p_system_c = NULL;

	print_log(DEBUG, 1, "[%s] CNetManager\n", m_p_log_id);
	
}

/******************************************************************************/
/**
* \brief        constructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CNetManager::CNetManager(void)
{
}

/******************************************************************************/
/**
* \brief        destructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CNetManager::~CNetManager()
{    
    //net_state_proc_stop();
    net_state_proc_thread_join();
    
    safe_delete( m_p_mutex_c );
    
    print_log(DEBUG, 1, "[%s] ~CNetManager\n", m_p_log_id);
}


/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CNetManager::ap_switching(void)
{	
	bool b_ret = false;
    s32 command_result = 0x00;
	u8 c_retry_num = 0;
    s8  p_command[256];
    s8  p_result[128];
	
	if(m_p_system_c->is_ap_switching_mode() == false)
	{
        memset(p_command, 0x00, sizeof(p_command));
        memset(p_result, 0x00, sizeof(p_result));

		m_p_system_c->set_oled_display_event(eSCAN_AP_EVENT);

        //sys_command("ifconfig wlan0 down");     //TEMP
        //sys_command("ifconfig wlan0 up");       //TEMP

		// Delete existing AP scan files
        sys_command("rm -f %s", AP_SCAN_FILE);
        
        //AP Switching:AP Scan  
		for(c_retry_num = 0; c_retry_num < AP_SCAN_RETRY_NUM_MAX; c_retry_num++)
		{
            print_log(DEBUG, 1, "[%s] AP Switching:AP Scan [Run: %d]\n", m_p_log_id, c_retry_num );

    		// AP Switching:AP Scan:[0] - RUN AP SCAN COMMAND
            m_p_system_c->iw_command_lock();
            
            //sys_command("iw dev wlan0 scan flush | awk '$1 ~ /^signal/ || $1 ~ /^SSID/ {print $2}' > %s;", AP_SCAN_FILE);
    	    sprintf( p_command, "iw dev wlan0 scan flush | awk '$1 ~ /^signal/ || $1 ~ /^SSID/ {print $2}' > %s;", AP_SCAN_FILE);
            command_result = process_get_result(p_command, p_result);
            m_p_system_c->iw_command_unlock();

            if( command_result < 0 ) //실패 
            {
                print_log(ERROR, 1, "[%s] AP Switching:AP Scan:[0] fail\n", m_p_log_id);
                sleep_ex(1000000);  //1sec
                continue;
            }

            // AP_switching[1] - AP scanning result analysis 

            //sys_command("cat %s;", AP_SCAN_FILE);   //DEBUG

            memset(p_command, 0x00, sizeof(p_command));
            memset(p_result, 0x00, sizeof(p_result));
            
            sprintf( p_command, "wc -l %s | awk '{print $1}';", AP_SCAN_FILE);
            command_result = process_get_result(p_command, p_result);

            if( command_result < 0 ) //실패 
            {
                print_log(ERROR, 1, "[%s] AP Switching:AP Scan:[1] fail\n", m_p_log_id);
                sleep_ex(1000000);  //1sec
                continue;
            }

            m_n_ap_count = atoi(p_result);
            print_log(DEBUG, 1, "[%s] AP count: %d\n", m_p_log_id, m_n_ap_count);

            // AP_switching[2] - AP scanning result (no data) 
            if( m_n_ap_count <= 0 ) //실패 (no ap found) 
            {
                print_log(ERROR, 1, "[%s] AP Switching:AP Scan:[2] fail\n", m_p_log_id);
                sleep_ex(1000000);  //1sec
                continue;
            }
            
            // AP_switching[3] - Matcing AP scanning result & preset data 
            if( find_ap_candidates() == false )
            {
                print_log(DEBUG, 1, "[%s] AP Switching:AP Scan:[3] fail\n", m_p_log_id);
                sleep_ex(1000000);  //1sec
                continue;
            }

            //Found matcing candidate
            break;
            
        }
		
		if(c_retry_num == AP_SCAN_RETRY_NUM_MAX)
		{
			print_log(DEBUG, 1, "[%s] AP Switching:AP Scan Fail [Run: AP_SCAN_RETRY_NUM_MAX]\n", m_p_log_id);
			m_p_system_c->set_oled_display_event(eACTION_EVENT_END);
			return false;
		}
	    
	    m_p_system_c->set_ap_switching_mode(true);
	}
	
	if(m_p_ap_sw_mode_timer_c == NULL)
	{
		m_p_ap_sw_mode_timer_c = new CTime(20000);
	}
	
	m_p_system_c->set_oled_display_event(eCHANGE_AP_EVENT);
		
	b_ret = change_ap();
   	print_log(DEBUG, 1, "[%s] AP Switching: %s\n", m_p_log_id, b_ret==true?"switched":"not switched"); 
   	sleep_ex(500000);
   	
   	if(b_ret == false)
	{
		m_p_system_c->set_ap_switching_mode(false);
		safe_delete(m_p_ap_sw_mode_timer_c);
	}
   	
   	m_p_system_c->set_oled_display_event(eACTION_EVENT_END);
   	
   	return b_ret;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    scan된 AP list와 preset list를 비교하여 두곳에서 모두 찾을 수 있는 AP list만 
 *          m_ap_candidate_t에 기록 
*******************************************************************************/
bool CNetManager::find_ap_candidates(void)
{	
	char		p_ssid_string[100];
	char		p_sig_string[100];
	s32			n_preset_index = -1;	
	FILE*		p_ap_scan_file = NULL;
	bool		b_result = false;
	u32			i = 0;
	
    p_ap_scan_file = fopen(AP_SCAN_FILE, "r");

	
	if(p_ap_scan_file == NULL)
	{
		fclose(p_ap_scan_file);
		return b_result;
	}
	
	// Reset list of ap candidates
	for(i = 0; i < MAX_CFG_COUNT; i++)
	{
		m_ap_candidate_t[i].b_associated = false;
		m_ap_candidate_t[i].n_signal = -100;
		memset(m_ap_candidate_t[i].p_ssid_str, 0, 100);
	}
		
	memset( p_ssid_string, 0, sizeof(p_ssid_string) );
	
    //TRAVERSE THROUGH ..modification required 
	while( (fgets( p_sig_string, (sizeof(p_sig_string) - 1), p_ap_scan_file ) != NULL) &&
            (fgets( p_ssid_string, sizeof(p_ssid_string) - 1, p_ap_scan_file ) != NULL) )
	{
		p_ssid_string[strlen(p_ssid_string)-1] = 0;
		n_preset_index = m_p_system_c->config_get_preset_index(p_ssid_string);

        s32 n_signal = (s32)atof( p_sig_string );

        //print_log(DEBUG, 1, "[%s] [TP] signal : %d, SSID : %s, SSID length : %d.\n", m_p_log_id, n_signal, p_ssid_string, strlen(p_ssid_string));

		if(n_preset_index > 0)
		{	
            if(n_signal > m_ap_candidate_t[n_preset_index].n_signal)
            {
                b_result = true;
                m_ap_candidate_t[n_preset_index].n_signal = n_signal;
                memset(m_ap_candidate_t[n_preset_index].p_ssid_str, 0, 100);
                memcpy(m_ap_candidate_t[n_preset_index].p_ssid_str, p_ssid_string, strlen(p_ssid_string)+1);
                print_log(DEBUG, 1, "[%s] AP candidate signal : %d, SSID : %s, SSID length : %d.\n", m_p_log_id, n_signal, p_ssid_string, strlen(p_ssid_string));
            }     
		}

		memset( p_ssid_string, 0, sizeof(p_ssid_string) );
        memset( p_sig_string, 0, sizeof(p_sig_string) );
	}
	
	fclose(p_ap_scan_file);
	return b_result;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CNetManager::change_ap(void)
{
	u32		n_preset_idx = 0, n_cur_idx = 0;
	s32		n_max_signal = -100;
	s32		n_best_ap_idx = -1;
	
	n_cur_idx = m_p_system_c->config_get_preset_index(m_p_system_c->config_get(eCFG_SSID));
	m_ap_candidate_t[n_cur_idx].b_associated = true;
	
	for(n_preset_idx = 1; n_preset_idx < MAX_CFG_COUNT; n_preset_idx++ )
	{
		if( m_ap_candidate_t[n_preset_idx].b_associated == false && \
			m_ap_candidate_t[n_preset_idx].n_signal > n_max_signal && \
			n_cur_idx != n_preset_idx )
		{
			n_max_signal = m_ap_candidate_t[n_preset_idx].n_signal;
			n_best_ap_idx = n_preset_idx;
		}
	}
	
	if(n_best_ap_idx > 0)
	{
		m_p_system_c->config_change_preset(m_ap_candidate_t[n_best_ap_idx].p_ssid_str);
	  
		m_p_system_c->config_save();
		m_p_system_c->config_reload();	
		
		m_ap_candidate_t[n_best_ap_idx].b_associated = true;
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
void CNetManager::start(void)
{
	net_state_proc_start();
}

/******************************************************************************/
/**
 * @brief   
 * @param   noti: 함수 포인터(notify_sw)
 * @return  
 * @note    main.cpp에서 등록한다.
*******************************************************************************/
void CNetManager::set_notify_handler(bool (*noti)(vw_msg_t * msg))
{
    notify_handler = noti;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CNetManager::message_send( MSG i_msg_e, u8* i_p_data, u16 i_w_data_len )
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
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void* net_state_routine( void* arg )
{
	CNetManager * net = (CNetManager *)arg;
	net->net_state_proc();
	pthread_exit( NULL );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CNetManager::net_state_proc(void)
{
    print_log(DEBUG, 1, "[%s] start net_state_proc...(%d)\n", m_p_log_id, syscall( __NR_gettid ));
    
    NET_MODE    old_net_mode_e;
    CTime*      p_tether_check_c = NULL;
    
    m_b_display_link_quality = false;
    m_p_display_link_quality_time_c = NULL;
    
    m_w_ap_channel = atoi(m_p_system_c->config_get(eCFG_AP_CHANNEL));
    
    m_current_net_mode_e = eNET_MODE_INIT;
    m_p_system_c->net_set_mode(m_current_net_mode_e);
    
    net_state_print( m_current_net_mode_e );
    
    old_net_mode_e = m_current_net_mode_e;
    
    m_current_net_mode_e = net_state_init();
    
    m_p_system_c->net_set_mode(m_current_net_mode_e);
    
    p_tether_check_c = new CTime(5000);
    
    // Start 화면 scroll 종료 후 main 화면 표시 시점
    m_p_system_c->set_oled_display_event(eACTION_EVENT_END);
    	
    while( m_b_is_running == true )
    {
        if( m_current_net_mode_e != old_net_mode_e )
        {
            net_state_print( m_current_net_mode_e );
            old_net_mode_e = m_current_net_mode_e;
            
            m_p_system_c->net_set_mode(m_current_net_mode_e);
            
            if( m_current_net_mode_e == eNET_MODE_STATION )
            {
            	m_b_display_link_quality = true;
            	m_p_display_link_quality_time_c = new CTime(10000);
            }
            
            m_p_system_c->display_update();
            
            m_p_system_c->power_control_for_phy();
        }
        
        if( p_tether_check_c != NULL
            && p_tether_check_c->is_expired() == true )
        {
            safe_delete(p_tether_check_c);
            
            if(m_current_net_mode_e == eNET_MODE_TETHER)
            {
                NET_MODE net_mode_e;
                
                /* init 후에 tether cable을 제거하는 경우 tether link 상태를 강제로 확인하여
                   무선 모드를 인식할 수 있도록 한다.
                */
                net_mode_e = net_state_tether(true);
                
                if( net_mode_e != m_current_net_mode_e )
                {
                    m_current_net_mode_e = net_mode_e;
                    continue;
                }
            }
        }
        
        if(m_p_ap_sw_mode_timer_c && m_p_ap_sw_mode_timer_c->is_expired())
        {
        	m_p_system_c->set_ap_switching_mode(false);
        	safe_delete(m_p_ap_sw_mode_timer_c);
        }
        
        switch( m_current_net_mode_e )
        {
            case eNET_MODE_TETHER:
            {
                m_current_net_mode_e = net_state_tether();
                break;
            }
            case eNET_MODE_STATION:
            {
                m_current_net_mode_e = net_state_station();
                break;
            }
            case eNET_MODE_AP:
            {
                m_current_net_mode_e = net_state_ap();
                break;
            }
            default:
            {
                break;
            }
        }
        sleep_ex(1000000); // check every 1000 ms
    }
    
    print_log(DEBUG, 1, "[%s] stop net_state_proc...\n", m_p_log_id);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CNetManager::net_is_ready(void)
{
    if( m_current_net_mode_e != eNET_MODE_INIT )
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
bool CNetManager::net_discovery(void)
{
    s32 n_sock;
    s32 n_sockopt = 1;
    u32 n_send_len;

    struct sockaddr_in server_addr;
    struct sockaddr_in addr_in_t;
    u32     n_ip;
    u32     n_subnet;
    
    bool    b_result = false;

    memset( &server_addr, 0, sizeof( server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons( SYNC_PORT );
    
    inet_aton( m_p_system_c->config_get(eCFG_IP_ADDRESS), &addr_in_t.sin_addr);
    n_ip = addr_in_t.sin_addr.s_addr;

    inet_aton( m_p_system_c->config_get(eCFG_NETMASK), &addr_in_t.sin_addr);
    n_subnet = addr_in_t.sin_addr.s_addr;

    n_ip = n_ip & n_subnet;
    n_ip = n_ip | (~n_subnet);
    
    server_addr.sin_addr.s_addr = inet_addr("169.254.255.255");
    
    printf("broadcast ip: 0x%08X(0x%08X)\n", n_ip, server_addr.sin_addr.s_addr);
    
    server_addr.sin_addr.s_addr = n_ip;
    
    n_sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    
    setsockopt( n_sock, SOL_SOCKET, SO_BROADCAST, &n_sockopt, sizeof( n_sockopt ) );
    
    sync_header_t header_t;
    
    memset( &header_t, 0, sizeof(sync_header_t) );
    
    header_t.command        = htons(SYNC_COMMAND_DISCOVERY_CMD);
    header_t.size           = htons(1);
    header_t.count          = 0;

    n_send_len = sendto( n_sock, &header_t, sizeof(header_t), 0, \
                    ( struct sockaddr*)&server_addr, sizeof( server_addr) );
    
    if( n_send_len < sizeof(header_t) )
    {
        print_log(ERROR, 1, "[%s] discovery packet send failed\n", m_p_log_id );
        close( n_sock );
        
        return false;
    }
    
    CTime*  p_wait_c = new CTime(1000);
    int     server_addr_size;
    s32     n_recv_len;
    
    sync_ack_t ack_t;
    memset( &ack_t, 0, sizeof(sync_ack_t) );
    
    server_addr_size= sizeof(server_addr);
    while(1)
    {
        n_recv_len = recvfrom( n_sock, &ack_t, sizeof(sync_ack_t), MSG_DONTWAIT, 
                        ( struct sockaddr*)&server_addr, (socklen_t*)&server_addr_size);
        
        if( n_recv_len == sizeof(sync_ack_t) )
        {
            print_log(DEBUG, 1, "[%s] sync discovery ack\n", m_p_log_id);
            printf("AP ssid: %s\n", (s8*)ack_t.ssid);
            printf("AP key: %s\n", (s8*)ack_t.key);
            printf("AP MAC: %s\n", (s8*)ack_t.mac);
            
            memcpy( &m_sync_info_t.ack_t, &ack_t, sizeof(sync_ack_t) );
            m_sync_info_t.b_sync = true;
            
            b_result = true;
            break;
        }
        else if( p_wait_c->is_expired() == true )
        {
            print_log(ERROR, 1, "[%s] discovery time out(recv: %d)\n", \
                                m_p_log_id, n_recv_len);
            break;
        }
    	else if( n_recv_len == -1 )
    	{
    		// MSG_DONTWAIT
    		if( errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK)
    		{
    			sleep_ex( 10 );
    			continue;
    		}
    		print_log(ERROR,1,"[%s] discovery recv error: %s\n", m_p_log_id, strerror( errno ));
    		break;
    	}
        else
        {
            sleep_ex(200000);
        }
    }
    
    close( n_sock );
    safe_delete( p_wait_c );
    
    return b_result;
    
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CNetManager::net_set_mode( NET_MODE i_net_mode_e )
{
    sys_command("uci set network.lan.ipaddr=%s", m_p_system_c->config_get(eCFG_IP_ADDRESS));
    sys_command("uci set network.lan.netmask=%s", m_p_system_c->config_get(eCFG_NETMASK));
    sys_command("uci set network.lan.gateway=%s", m_p_system_c->config_get(eCFG_GATEWAY));
    
    sys_command( "uci commit network" );
    
    // 무선 전용 모델인 경우
    if( m_b_wlan_enable == true
        && m_p_system_c->net_use_only_power() == true )
    {
    	// ethernet controller 비활성화
        m_p_system_c->hw_phy_down();
    }
    else	// 무선 전용 모델이 아닌 경우 ethernet controller 활성화 
    {
        m_p_system_c->hw_phy_up();
    }
      
    // uci로 관리할 수 있는 wifi configuration file detection
   	wifi_detection_check();
       
    if( i_net_mode_e == eNET_MODE_STATION
        || i_net_mode_e == eNET_MODE_TETHER )
    {
    	// 디텍터 AP 모드로 사용하는 경우가 아니면 dhcp server stop
        net_dhcp_start(false);
        
        sys_command( "uci del wireless.@wifi-device[0].channel" );
        sys_command( "uci del wireless.@wifi-device[0].encryption" );
        sys_command( "uci del wireless.@wifi-device[0].key" );
        sys_command( "uci del wireless.@wifi-device[0].htmode" );
        sys_command( "uci del wireless.@wifi-device[0].hwmode" );
        sys_command( "uci del wireless.@wifi-device[0].ht_capab" );

        sys_command( "uci del wireless.@wifi-device[0].disabled" );
        sys_command( "uci del wireless.@wifi-iface[0].network" );
        sys_command( "uci del wireless.@wifi-iface[0].network=lan" );
        sys_command( "uci del wireless.@wifi-iface[0].bssid" );
        
        // wireless 설정을 station 모드로 설정
        sys_command( "uci set wireless.@wifi-iface[0].mode=sta" );
        sys_command( "uci set wireless.@wifi-iface[0].client_bridge=1" );
        
        // 연결 가능한 AP 를 찾았으면 해당 AP가 응답한 MAC, SSID, KEY 저장 
        if( m_sync_info_t.b_sync == true )
        {
            sys_command( "uci set wireless.@wifi-iface[0].bssid=%s", (s8*) m_sync_info_t.ack_t.mac );
            sys_command( "uci set wireless.@wifi-iface[0].ssid=%s", (s8*) m_sync_info_t.ack_t.ssid );
            sys_command( "uci set wireless.@wifi-iface[0].key=%s", (s8*) m_sync_info_t.ack_t.key );
            
            m_sync_info_t.b_sync = false;
        }
        else	// 연결 가능한 AP 가 없으면 기존 저장된 SSID, KEY 저장
        {
            sys_command( "uci set wireless.@wifi-iface[0].ssid=%s", m_p_system_c->config_get(eCFG_SSID) );
            sys_command( "uci set wireless.@wifi-iface[0].key=%s", m_p_system_c->config_get(eCFG_KEY) );
        }
        
        sys_command( "uci set wireless.@wifi-iface[0].channel=auto" );
        sys_command( "uci set wireless.@wifi-iface[0].encryption=psk2" );
        
        // 변경된 wireless 설정 commit
        sys_command( "uci commit wireless" );
        
        // 변경된 wireless 설정 적용
        sys_command("/etc/init.d/network restart");
        
        if( i_net_mode_e == eNET_MODE_TETHER )
        {
        	// 유선모드인 경우, 브릿지에 IP, netmask 설정
            sys_command( "ifconfig br-lan %s netmask %s", m_p_system_c->config_get(eCFG_IP_ADDRESS), m_p_system_c->config_get(eCFG_NETMASK) );
        }
        else
        {
        	// 디텍터 station모드인 경우 무선랜 interface에 IP, netmask 설정
            sys_command( "ifconfig wlan0 %s netmask %s", m_p_system_c->config_get(eCFG_IP_ADDRESS), m_p_system_c->config_get(eCFG_NETMASK) );
        }
        
    }
    else if( i_net_mode_e == eNET_MODE_AP )
    {
        sys_command( "uci  del wireless.@wifi-device[0].ht_capab" );
        sys_command( "uci  del wireless.@wifi-device[0].vht_capab" );
        sys_command( "uci  del wireless.@wifi-iface[0].client_bridge" );
        sys_command( "uci  del wireless.@wifi-iface[0].channel" );
        sys_command( "uci  del wireless.@wifi-device[0].disabled" );        
        sys_command( "uci  del wireless.@wifi-device[0].hwmode_11ac");
                      
        sys_command("uci set wireless.@wifi-device[0].channel=%s", m_p_system_c->config_get(eCFG_AP_CHANNEL));
        sys_command("uci set wireless.@wifi-device[0].encryption=%s", m_p_system_c->config_get(eCFG_AP_SECURITY));
        sys_command("uci set wireless.@wifi-device[0].key=%s", m_p_system_c->config_get(eCFG_AP_KEY));
        
        // bandwidth
        if( strcmp( m_p_system_c->config_get(eCFG_AP_BAND), "HT80") == 0 )
        {
        	sys_command("uci set wireless.@wifi-device[0].htmode=%s", "HT40+");
        }
        else
        {
        	sys_command("uci set wireless.@wifi-device[0].htmode=%s", m_p_system_c->config_get(eCFG_AP_BAND));
        }
        
        if( strcmp( m_p_system_c->config_get(eCFG_AP_BAND), "HT80") == 0 )
        {
        	sys_command("uci set wireless.@wifi-device[0].hwmode_11ac=a");
        }

    	if( strcmp( m_p_system_c->config_get(eCFG_AP_FREQ), "2" ) == 0 )
        {
            // 2.4GHz
            sys_command("uci set wireless.@wifi-device[0].hwmode=11ng");
        }
        else
    	{
    	    // 5GHz
    	    sys_command("uci set wireless.@wifi-device[0].hwmode=11na");
    	}
    	
    	// gaurd interval
        if( strcmp( m_p_system_c->config_get(eCFG_AP_GI), "400" ) == 0 )
        {
            if( strcmp( m_p_system_c->config_get(eCFG_AP_BAND), "HT20" ) == 0 )
            {
                // HT20
                sys_command("uci add_list wireless.@wifi-device[0].ht_capab=SHORT-GI-20");
            }
            else if( strcmp( m_p_system_c->config_get(eCFG_AP_BAND), "HT80" ) == 0 )
            {
                // VHT80
                sys_command("uci add_list wireless.@wifi-device[0].vht_capab=SHORT-GI-80");
            }
            else
            {
            	// HT40, HT40+
                sys_command("uci add_list wireless.@wifi-device[0].ht_capab=SHORT-GI-40");
            }
        }
        sys_command("uci add_list wireless.@wifi-device[0].ht_capab=TX-STBC");
        sys_command("uci add_list wireless.@wifi-device[0].ht_capab=RX-STBC1");
        
        if( strcmp( m_p_system_c->config_get(eCFG_AP_BAND), "HT20" ) != 0 )
        {
            // HT40, HT40+
            sys_command("uci add_list wireless.@wifi-device[0].ht_capab=DSSS_CCK-40");
        }
        
        // wireless 설정을 AP 모드로 설정
        // 디텍터 AP 모드일때의 SSID, KEY, encryption 값 입력
        sys_command("uci set wireless.@wifi-iface[0].encryption=%s", m_p_system_c->config_get(eCFG_AP_SECURITY));
        sys_command("uci set wireless.@wifi-iface[0].key=%s", m_p_system_c->config_get(eCFG_AP_KEY));
        sys_command("uci set wireless.@wifi-iface[0].ssid=%s", m_p_system_c->config_get(eCFG_AP_SSID));
        sys_command("uci set wireless.@wifi-iface[0].mode=ap");
        sys_command("uci set wireless.@wifi-iface[0].network=lan");

        
        // 변경된 wireless 설정 commit
        sys_command("uci commit wireless");
         // 변경된 wireless 설정 적용
        sys_command("/etc/init.d/network restart");
        
        // 브릿지에 IP, netmask 설정
        sys_command( "ifconfig br-lan %s netmask %s", m_p_system_c->config_get(eCFG_IP_ADDRESS), m_p_system_c->config_get(eCFG_NETMASK) );
        
        // DHCP server start
        net_dhcp_start();
        
        // tx power
#if 0
        float tx_power = atoi( m_p_env_c->config_get(eCFG_AP_TX_POWER) );
        if( strcmp( m_p_env_c->config_get(eCFG_AP_FREQ), "2" ) == 0 )
        {
            tx_power =(float)( MAX_2_4GHZ_TX_POWER_mW * tx_power) / (float)100;
        }
        else
        {
            tx_power =(float)( MAX_5GHZ_TX_POWER_mW * tx_power) / (float)100;
        }
        
        sys_command("iwconfig wlan0 txpower %dmW", (int)tx_power);
#endif
    }
    
    net_set_interface( i_net_mode_e );
}

/******************************************************************************/
/**
 * @brief   network 설정에 따라 network interface 사용 여부를 결정한다.
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CNetManager::net_set_interface(NET_MODE i_net_mode_e)
{
	// 디텍터 AP 모드인 경우, 브릿지/무선랜 interface 사용
    if( i_net_mode_e == eNET_MODE_AP )
    {
        sys_command("ifconfig br-lan up");
        sys_command("ifconfig wlan0 up");
        print_log(DEBUG, 1, "[%s] AP mode\n", m_p_log_id);
    }
    else
    {
        // 디텍터 station 모드인 경우, 무선랜 interface만 사용
        if( i_net_mode_e == eNET_MODE_STATION )
        {
            sys_command("ifconfig br-lan down");
            sys_command("ifconfig wlan0 %s netmask %s", \
                            m_p_system_c->config_get(eCFG_IP_ADDRESS), \
                            m_p_system_c->config_get(eCFG_NETMASK));
            sys_command("ifconfig wlan0 up");
            print_log(DEBUG, 1, "[%s] Station mode\n", m_p_log_id);
        }
        else	// 유선모드인 경우, 브릿지 및 유선랜 interface 사용,  
        {
            sys_command("ifconfig wlan0 down");
            //m_p_system_c->hw_phy_up();
            sys_command("ifconfig br-lan %s netmask %s", \
                            m_p_system_c->config_get(eCFG_IP_ADDRESS), \
                            m_p_system_c->config_get(eCFG_NETMASK));
            sys_command("ifconfig br-lan up");
            print_log(DEBUG, 1, "[%s] Tether mode\n", m_p_log_id);
        }
    }
}

/******************************************************************************/
/**
 * @brief   연결된 네트워크 상에 동일한 IP를 가지고 있는 장치가 있는지 확인
 * @param   
 * @return  
 * @note    
*******************************************************************************/
s32 CNetManager::net_check_ip_collision(NET_MODE i_net_mode_e)
{
    s8      p_result[256];
    FILE*   p_file;
    
    print_log(DEBUG, 1, "[%s] net_check_ip_collision(%d)\n", m_p_log_id, i_net_mode_e);
    
    if( i_net_mode_e == eNET_MODE_STATION )
    {
        sys_command( "arping -q -D -I wlan0 -c 1 %s; echo $? > /tmp/ip_exist", \
                        m_p_system_c->config_get(eCFG_IP_ADDRESS) );
    }
    else
    {
        sys_command( "arping -q -D -I br-lan -c 1 %s; echo $? > /tmp/ip_exist", \
                        m_p_system_c->config_get(eCFG_IP_ADDRESS) );
    }
    
    memset( p_result, 0, sizeof(p_result) );
    p_file = fopen("/tmp/ip_exist", "r");
    
    if( p_file == NULL )
    {
        print_log(ERROR, 1, "[%s] reading file(/tmp/ip_exist) failed.\n", m_p_log_id);
        return 0;
    }
    
    fscanf( p_file, "%s", p_result );
    fclose( p_file );
    
    return atoi( p_result );
}

/******************************************************************************/
/**
 * @brief   연결된 네트워크 상에 동일한 IP 주소를 가지고 있는 장치가 있으면 IP를 변경한다.
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CNetManager::net_check_ip(NET_MODE i_net_mode_e)
{
    if( net_check_ip_collision(i_net_mode_e) == 1 )
    {
        net_change_ip(i_net_mode_e);
        
        if( net_check_ip_collision(i_net_mode_e) == 1 )
        {
            net_change_ip(i_net_mode_e);
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
void CNetManager::net_change_ip(NET_MODE i_net_mode_e)
{
    s8  p_ip_address[STR_LEN];
    u32 n_current_ip = m_p_system_c->net_get_ip();
    u32 n_new_ip;
    
    srand(time(NULL));
    
    n_current_ip = ntohl(n_current_ip);
    
    n_new_ip = n_current_ip;
    while( n_new_ip == n_current_ip )
    {
        n_new_ip = (n_current_ip & 0xFFFFFF00) | (rand() % 255);
    }
    
    sprintf( p_ip_address, "%d.%d.%d.%d", \
                ((n_new_ip & 0xFF000000) >> 24), ((n_new_ip & 0x00FF0000) >> 16), \
                ((n_new_ip & 0x0000FF00) >> 8), (n_new_ip & 0x000000FF) );
    
    print_log(DEBUG, 1, "[%s] IP changed( %s -> %s )\n", m_p_log_id, \
                    m_p_system_c->config_get(eCFG_IP_ADDRESS), p_ip_address);
    
    m_p_system_c->config_set( eCFG_IP_ADDRESS, p_ip_address );
    m_p_system_c->config_save();
    
    if( i_net_mode_e == eNET_MODE_STATION )
    {
        sys_command("ifconfig wlan0 %s", m_p_system_c->config_get(eCFG_IP_ADDRESS));
    }
    else
    {
        sys_command("ifconfig br-lan %s", m_p_system_c->config_get(eCFG_IP_ADDRESS));
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CNetManager::net_check_subnet( u32 i_n_src_addr )
{
    struct in_addr	addr;
    u32     		n_ip_mask;
    u32				n_ip;
    u32				n_gateway;
    
    inet_aton( m_p_system_c->config_get(eCFG_IP_ADDRESS), &addr);
    n_ip = (u32)addr.s_addr;
    n_ip = ntohl( n_ip );
	
	if( (i_n_src_addr & 0xFFFF0000) == (n_ip & 0xFFFF0000) )
	{
		return;
	}
	
	print_log(DEBUG, 1, "[%s] netmask is different (src: 0x%08X, cur: 0x%08X)\n", \
							m_p_log_id, i_n_src_addr, n_ip);
	
	n_ip_mask = i_n_src_addr & 0xFFFFFF00;
	
	// update ip address
	inet_aton( m_p_system_c->config_get(eCFG_IP_ADDRESS), &addr);
	n_ip = (u32)addr.s_addr;
	n_ip = ntohl( n_ip );
	
	n_ip = n_ip & 0x000000FF;
	n_ip = n_ip_mask | n_ip;
	
	// update gateway ip address
	inet_aton( m_p_system_c->config_get(eCFG_GATEWAY), &addr);
	n_gateway = (u32)addr.s_addr;
	n_gateway = ntohl( n_gateway );
	
	n_gateway = n_gateway & 0x000000FF;
	n_gateway = n_ip_mask | n_gateway;

	print_log(DEBUG, 1, "[%s] new ip: 0x%08X, gateway: 0x%08X\n", \
							m_p_log_id, n_ip, n_gateway );

	addr.s_addr	= htonl( n_ip );
	m_p_system_c->config_set( eCFG_IP_ADDRESS, inet_ntoa( addr ) );
	
	addr.s_addr	= htonl( n_gateway );
	m_p_system_c->config_set( eCFG_GATEWAY, inet_ntoa( addr ) );
	
	m_p_system_c->config_save();
	
	net_check_ip(m_current_net_mode_e);
	
	if( m_current_net_mode_e == eNET_MODE_STATION )
	{
	    sys_command( "ifconfig wlan0 %s", m_p_system_c->config_get(eCFG_IP_ADDRESS) );
	}
	else
	{
	    sys_command( "ifconfig br-lan %s", m_p_system_c->config_get(eCFG_IP_ADDRESS) );
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CNetManager::net_get_tether_speed(void)
{
    s8      p_speed[256];
    s8      p_duplex[256];
    FILE*   p_file;
    
    net_br_lan_state_up_check();
    
    lock();
    sys_command( "cat /sys/class/net/eth0/speed > /tmp/speed" );
    
    memset( p_speed, 0, sizeof(p_speed) );
    p_file = fopen("/tmp/speed", "r");
    
    if( p_file == NULL )
    {
        print_log(ERROR, 1, "[%s] reading file(/tmp/speed) failed.\n", m_p_log_id);
        unlock();
        return 0;
    }
    
    fscanf( p_file, "%s", p_speed );
    fclose( p_file );
    
    sys_command( "cat /sys/class/net/eth0/duplex > /tmp/duplex" );
    memset( p_duplex, 0, sizeof(p_duplex) );
    p_file = fopen("/tmp/duplex", "r");

    if( p_file == NULL )
    {
        print_log(ERROR, 1, "[%s] reading file(/tmp/duplex) failed.\n", m_p_log_id);
    }
    else
    {
        fscanf( p_file, "%s", p_duplex );
        fclose( p_file );
    }
    
    unlock();
    
    print_log(DEBUG, 1, "[%s] link speed: %s, duplex: %s\n", m_p_log_id, p_speed, p_duplex);
    
    return (u16)atoi( p_speed );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CNetManager::net_dhcp_start( bool i_b_start )
{
	if( m_p_system_c->net_is_dhcp_enable() == false )
	{
		print_log(DEBUG, 1, "[%s] DHCP server is disable(start: %d).\n", m_p_log_id, i_b_start);
		return;
	}
	
	if( i_b_start == false )
	{
		sys_command("pkill udhcpd");
		print_log(DEBUG, 1, "[%s] DHCP server stop\n", m_p_log_id);
	}
	
	if( m_p_system_c->is_web_enable() == true
		&& m_p_system_c->net_is_ap_on() == true )
	{
		sys_command("udhcpd %s", DHCP_CONFIG_FILE);
		print_log(DEBUG, 1, "[%s] DHCP server start\n", m_p_log_id);
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CNetManager::net_get_mac( u8* o_p_mac, bool b_registered )
{
    if( b_registered == false )
    {
        u8 p_mac[6];
        
        memset( p_mac, 0, sizeof(p_mac) );
        
        if( m_current_net_mode_e == eNET_MODE_STATION
            || m_current_net_mode_e == eNET_MODE_AP )
        {
            vw_net_if_get_mac("wlan0", p_mac);
            memcpy( o_p_mac, p_mac, sizeof(p_mac) );
            return;
        }
    }
    
    m_p_system_c->net_get_mac( o_p_mac );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
NET_MODE CNetManager::net_state_init(void)
{
    m_b_ap_on           = false;
    m_b_wlan_enable     = m_p_system_c->model_is_wireless();
    m_b_link_state      = false;
    
    m_b_ap_connected    = false;
    
    m_p_system_c->net_set_tether_speed(0);
    
    if( m_b_wlan_enable == false )
    {
        net_set_mode( eNET_MODE_TETHER );
        
        print_log(DEBUG, 1, "[%s] !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", m_p_log_id);
        print_log(DEBUG, 1, "[%s] !!! This model is not for wireless.!!!\n", m_p_log_id);
        print_log(DEBUG, 1, "[%s] !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", m_p_log_id);
        
        return eNET_MODE_TETHER;
    }

    if( m_p_system_c->net_use_only_power() == false
        && m_p_system_c->hw_get_power_source() == ePWR_SRC_TETHER )
    {
        net_set_mode(eNET_MODE_TETHER);
        return eNET_MODE_TETHER;
    }
    else
    {
        if( m_p_system_c->net_is_ap_on() == true && m_p_system_c->get_ap_button_type() != ePRESET_SWITCHING )
        {
            m_b_ap_on = true;
            
            net_set_mode(eNET_MODE_AP);
            
            return eNET_MODE_AP;
        }
        else
        {
            net_set_mode(eNET_MODE_STATION);
            return eNET_MODE_STATION;
        }
    }
}

bool CNetManager::net_br_lan_state(void)
{
	bool b_ret = false;
	s8	p_output[10];
	
	process_get_result("cat /sys/class/net/br-lan/operstate", p_output);
	if(strcmp(p_output,"up\n") == 0)
	{
		b_ret = true;
	}
	return b_ret;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CNetManager::net_br_lan_state_up_check(void)
{
	CTime wait_c(5000);
	bool b_ret = false;
	    
	while( wait_c.is_expired() == false && net_br_lan_state() == false)
	{
		sleep_ex(500000);
	}
	b_ret = net_br_lan_state();
	print_log(INFO, 1, "[%s] br-lan state(%s)\n",m_p_log_id, b_ret ? "up":"down");
		
	return b_ret;
}
/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CNetManager::net_phy_reset(void)
{
    if(net_can_do_phy_reset())
	{
		print_log(INFO, 1, "[%s] phy reset\n", m_p_log_id);
		m_p_system_c->hw_phy_reset();		
		net_phy_reset_done();
	}
	
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CNetManager::net_speed_confirm( bool i_b_phy_reset )
{
	u16 w_speed;
    
    w_speed = net_get_tether_speed();
    
    if( w_speed <= 100
    	&& i_b_phy_reset == true )
    {
    	print_log(INFO, 1, "[%s] because of tether speed(%d), the phy will be reset.\n",m_p_log_id, w_speed);
		net_phy_reset();
		w_speed = net_get_tether_speed();
    }
    
    m_p_system_c->net_set_tether_speed(w_speed);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
NET_MODE CNetManager::net_state_tether( bool i_b_force )
{
    bool b_link_state;
    
    //b_link_state = vw_net_link_state("eth0");
    
    b_link_state = m_p_system_c->get_link_state();
    //printf("=========================== %d %d\n",m_p_system_c->hw_get_tether_equip(),m_p_system_c->net_use_only_power());
    
    // Tether cable 탈착 상태가 변경 되었을 경우
    if( b_link_state != m_b_link_state
        || i_b_force == true )
    {
        m_b_link_state = b_link_state;
        
        // Tether cable을 분리한 경우
        if( m_b_link_state == false )
        {
            m_p_system_c->net_set_tether_speed(0);
            
            // 무선 연결 지원 모델인 경우
            if( m_b_wlan_enable == true )
            {
            	// SW 연결 종료
            	message_send(eMSG_DISCONNECTION);
            	
            	// AP on 상태이거나, AP 버튼을 눌러 AP on 한 경우
                if( m_b_ap_on == true
                    || m_p_system_c->net_is_ap_on() == true )
                {
                    net_set_mode( eNET_MODE_AP );
                    
                    m_b_ap_on = true;
                    
                    // 디텍터 AP 모드로 전환
                    return eNET_MODE_AP;
                }
                
                // AP on 상태도 아니고, AP 버튼을 눌러 AP on 한 상태도 아닌경우 디텍터 station 모드로 전환
                net_set_interface( eNET_MODE_STATION );
                
                return eNET_MODE_STATION;
            }
            
            // 유선 모델인 경우, tether mode유지
        }
        else
        {
            net_speed_confirm();
            
            if( i_b_force == false )
            {
                net_check_ip(eNET_MODE_TETHER);
            }
        }
    }
    
    if( m_b_link_state == true )
    {
    	check_valid_speed();
    }
    
    // Tether cable이 연결된 상태로 유지되고 있고, 무선 모드 지원 모델인 경우
    // AP 버튼을 누르면 tether cable로 연결된 AP와 무선 동기화 
    if( m_b_wlan_enable == true
        && m_p_system_c->is_ap_button_function_activated() == true )
    {        
    	m_p_system_c->set_oled_display_event(eSYNC_WLAN_EVENT);
    	
        u16 w_count = 0;
        while( w_count < 3 )
        {
            if( net_discovery() == true )
            {
                message_send(eMSG_DISCONNECTION);
                
                m_p_system_c->net_set_tether_speed(0);
                m_b_ap_connected = false;
                
                m_p_system_c->config_set(eCFG_SSID, (s8*)m_sync_info_t.ack_t.ssid);
                m_p_system_c->config_set(eCFG_KEY, (s8*)m_sync_info_t.ack_t.key);
                
                m_p_system_c->config_save();
                
                net_set_mode( eNET_MODE_STATION );
                
                m_p_system_c->deactivate_ap_button_function();
                
                m_p_system_c->set_oled_display_event(eACTION_EVENT_END);
                
                return eNET_MODE_STATION;
            }
            w_count++;
            sleep_ex(200000);
        }
        
        m_p_system_c->set_oled_display_event(eACTION_EVENT_END);
        m_p_system_c->deactivate_ap_button_function();
        
    }
    
    return eNET_MODE_TETHER;
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CNetManager::net_state_check_tether(void)
{
    bool b_link_state;
    
    // POE를 power 공급 용도로만 사용하는 무선 전용 모델인 경우 유선 전환할 수 없도록 함
    if( m_p_system_c->net_use_only_power() == true )
    {
        return false;
    }
    
    b_link_state = m_p_system_c->get_link_state();
    
    //printf("======net_state_check_tether== %d %d\n",m_p_system_c->hw_get_tether_equip(),m_p_system_c->net_use_only_power());
    
    if( b_link_state != m_b_link_state )
    {
        m_b_link_state = b_link_state;
        
        // tether cable이 연결된 경우 유선 모드로 전환
        if( m_b_link_state == true )
        {
        	net_phy_reset_clear();
            message_send(eMSG_DISCONNECTION);
            
            //sleep_ex(2000000);
            m_p_system_c->hw_phy_reset();
            
            sleep_ex(100000);
            net_set_interface( eNET_MODE_TETHER );
            net_speed_confirm();
            
            net_check_ip(eNET_MODE_TETHER);
            
            return true;
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
NET_MODE CNetManager::net_state_station(void)
{
	bool b_ap_switched = false;
    if( net_state_check_tether() == true )
    {
        return eNET_MODE_TETHER;
    }
    
    // change to AP mode
    if( m_p_system_c->is_ap_button_function_activated() == true )
    {
    	// Tether 케이블이 연결되어 있는 경우, AP 버튼을 유효한 시간만큼 눌러도 디텍터 AP 모드로 전환하지 않음
    	// 네트워크 설정 초기화를 위한 AP 버튼 동작인 경우에도 디텍터 AP 모드로 전환하지 않고 상태 유지
        if( m_b_link_state == true
            || m_p_system_c->net_is_need_reboot() == true )
        {
            m_p_system_c->deactivate_ap_button_function();
            
            return eNET_MODE_STATION;
        }
        
        if( m_p_system_c->get_ap_button_type() == ePRESET_SWITCHING )
        {
        	if( m_p_system_c->is_preset_loaded() == true )
        	{
        		b_ap_switched = ap_switching();
	        	if(b_ap_switched == true)
	        	{
	        		net_set_mode(eNET_MODE_STATION);
	        	}
	        }
	        
	        m_p_system_c->deactivate_ap_button_function();
	        
	        return eNET_MODE_STATION;
        }
        else
        {
        	m_p_system_c->set_oled_display_event(eAP_MODE_ENABLE_EVENT);
        
	        // 위의 경우가 아니면 AP 모드로 변경
	        message_send(eMSG_DISCONNECTION);
	        
	        m_b_ap_on = true;
	        m_b_ap_connected = false;
	        m_p_system_c->net_set_tether_speed(0);
	        
	        m_p_system_c->config_set(eCFG_AP_ENABLE, "on");
	        m_p_system_c->config_save();
	
	        net_set_mode( eNET_MODE_AP );
	        
	        m_p_system_c->deactivate_ap_button_function();
	        
	        m_p_system_c->set_oled_display_event(eACTION_EVENT_END);
	        
	        m_p_system_c->deactivate_ap_button_function();
	        return eNET_MODE_AP;
	    }
    }
    
    sys_state_t state_t;
    
    m_p_system_c->get_state( &state_t );
    
    if( state_t.w_link_quality_level > 2 )
    {	
		// 신규 성립된 외부 AP와의 연결인 경우 IP 충돌되는지 확인
       if( m_b_ap_connected == false )
        {
            m_b_ap_connected = true;
            net_check_ip(eNET_MODE_STATION);
        }
    }
    else
    {
        if( state_t.w_link_quality_level == 0 )
        {
            m_b_ap_connected = false;
        }
        else if( m_b_ap_connected == false )
        {
            m_b_ap_connected = true;
            net_check_ip(eNET_MODE_STATION);
        }
    }
    
    if( m_p_display_link_quality_time_c != NULL
    	&& m_p_display_link_quality_time_c->is_expired() == false )
    {
    	if( m_b_display_link_quality == true
    		&& state_t.w_link_quality_level > 0 )
    	{
    		m_p_system_c->display_update();
    		
    		m_b_display_link_quality = false;
    		safe_delete(m_p_display_link_quality_time_c);
    	}
    }
    else
    {
    	m_b_display_link_quality = false;
    	safe_delete(m_p_display_link_quality_time_c);
    }
    
    return eNET_MODE_STATION;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
NET_MODE CNetManager::net_state_ap(void)
{
    if( net_state_check_tether() == true )
    {
        return eNET_MODE_TETHER;
    }

    // change to station mode
    if( m_p_system_c->is_ap_button_function_activated() == true )
    {    	
    	// 네트워크 설정 초기화를 위한 AP 버튼 동작인 경우에는 디텍터 AP 모드로 전환하지 않고 상태 유지
        if( m_p_system_c->net_is_need_reboot() == true )
        {
            m_p_system_c->deactivate_ap_button_function();
            return eNET_MODE_AP;
        }
        
        m_p_system_c->set_oled_display_event(eSTA_MODE_ENABLE_EVENT);
        
        message_send(eMSG_DISCONNECTION);
        
        m_b_ap_on = false;
        
        m_p_system_c->config_set(eCFG_AP_ENABLE, "off");
        m_p_system_c->config_save();
        
        net_set_mode( eNET_MODE_STATION );
        
        m_p_system_c->deactivate_ap_button_function();
        
        m_p_system_c->set_oled_display_event(eACTION_EVENT_END);
        
        return eNET_MODE_STATION;
    }
    
    return eNET_MODE_AP;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CNetManager::net_state_print( NET_MODE i_mode_e )
{
    switch( i_mode_e )
    {
        case eNET_MODE_INIT:
        {
            print_log(DEBUG, 1, "[%s] NET_MODE_INIT\n", m_p_log_id);
            break;
        }
        case eNET_MODE_TETHER:
        {
            print_log(DEBUG, 1, "[%s] NET_MODE_TETHER\n", m_p_log_id);
            break;
        }
        case eNET_MODE_STATION:
        {
            print_log(DEBUG, 1, "[%s] NET_MODE_STATION\n", m_p_log_id);
            break;
        }
        case eNET_MODE_AP:
        {
            print_log(DEBUG, 1, "[%s] NET_MODE_AP\n", m_p_log_id);
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
void CNetManager::net_state_proc_start(void)
{
    m_b_is_running = true;
    // launch net_state thread
	if( pthread_create(&m_net_state_thread_t, NULL, net_state_routine, ( void * ) this)!= 0 )
    {
    	print_log( ERROR, 1, "[%s] net_state_routine pthread_create:%s\n", \
    	                    m_p_log_id, strerror( errno ));

    	m_b_is_running = false;
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CNetManager::net_state_proc_thread_join(void)
{
	m_b_is_running = false;
    
    sys_command("ifconfig br-lan down");
    sys_command("ifconfig wlan0 down");
    
	if( pthread_join( m_net_state_thread_t, NULL ) != 0 )
	{
		print_log( ERROR, 1,"[%s] pthread_join: net_state_proc:%s\n", \
		                    m_p_log_id, strerror( errno ));
	}
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CNetManager::net_state_proc_stop(void)
{
	message_send(eMSG_DISCONNECTION);
	
    if( m_b_is_running )
    {
        net_state_proc_thread_join();
    }

    m_current_net_mode_e = eNET_MODE_INIT;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CNetManager::net_ap_set_channel( u16 i_w_num )
{
    if( m_w_ap_channel == i_w_num )
    {
        print_log(DEBUG, 1, "[%s] AP channel (current: %d)\n", m_p_log_id, m_w_ap_channel );
        return true;
    }
    
    if( m_p_system_c->net_is_valid_channel( i_w_num ) == false )
    {
        print_log(DEBUG, 1, "[%s] Invalid AP channel (%d)\n", m_p_log_id, i_w_num );
        return false;
    }
    
    print_log(DEBUG, 1, "[%s] AP channel change (%d -> %d)\n", m_p_log_id, m_w_ap_channel, i_w_num );
    
    sys_command("uci set wireless.@wifi-device[0].channel=%d", i_w_num);
    sys_command("uci commit wireless");
    sys_command("/etc/init.d/network restart");
    
    CTime wait_c(3000);
    
    while( wait_c.is_expired() == false )
    {
        sleep_ex(500000); // check every 500 ms
    }
    
    m_b_ap_channel_result = false;
    if( vw_net_link_state("wlan0") == false )
    {
        print_log(ERROR, 1, "[%s] AP channel change failed(current: %d)\n", m_p_log_id, m_w_ap_channel );
        
        sys_command("uci set wireless.@wifi-device[0].channel=%d", m_w_ap_channel);
        sys_command("uci commit wireless");
        sys_command("/etc/init.d/network restart");
        
        return false;
    }
    
    m_w_ap_channel = i_w_num;
    m_b_ap_channel_result = true;
    
    return true;
}

/******************************************************************************/
/**
 * @brief   wifi configuration 파일 생성에 실패하면 무선 모듈을 다시 적재하여 재시도
 * @param   
 * @return  
 * @note   	계속해서 실패하는 경우, 최대 12초까지 이 함수에서 머무를 수 있음
*******************************************************************************/
void CNetManager::wifi_detection_check( void )
{
#if 1
	if( wifi_detection_wait() == false )
	{
		int i;
		
		for( i = 0; i < 5; i++ )
		{
			sys_command("mkdir /lib/modules/4.1.15");
			sys_command("rmmod ath11k_pci.ko");
			sys_command("rmmod ath11k.ko");
			sys_command("rmmod ath.ko");
			
			sys_command("rm /etc/config/wireless");
			
			sleep_ex(200000);
			
			sys_command("insmod /run/ath.ko");
			sys_command("insmod /run/ath11k.ko");
			sys_command("insmod /run/ath11k_pci.ko");
			
			sleep_ex(200000);
			
			if( wifi_detection_wait() == true )
			{
				break;
			}
			print_log(ERROR, 1, "[%s] wifi module reload failed(%d)\n", m_p_log_id, i );
		}
	}

#else	
	const u32 k_timeout_cnt = 20;
	u32 n_timeout = k_timeout_cnt;
    while( m_b_wifi_detection == false && n_timeout-- > 0 )
    {
    	s8 c_result[512];
    	u32 n_length;
    	n_length = process_get_result("wc -c  /etc/config/wireless| awk '{print $1}'",c_result);
    	
    	
    	if( n_length != 0)
    	{
    		if( atoi(c_result) == 0)
    		{
	    		sys_command("/sbin/wifi detect > /tmp/wireless.tmp");
	    		sys_command("cp /tmp/wireless.tmp  /etc/config/wireless");
	    	}
	    	else
	    	{
	    		m_b_wifi_detection =  true;
	    	}
    	}
    	sleep_ex(200000); // check every 200 ms
    }
    
    print_log(DEBUG, 1, "[%s] wifi_detection_check - tried count(%d) \n", m_p_log_id,k_timeout_cnt - n_timeout);
#endif
}

/******************************************************************************/
/**
 * @brief   생선된 wifi configuration file이 있는지 확인하고 
 			없거나 비어있으면 wifi configuration file을 rebuild 한다.
 * @param   
 * @return  true : wifi configuration file 찾기 or 생성 성공
 			false : wifi configuration file 찾기 or 생성 실패
 * @note    200ms 마다 1회씩 총 10회 retry 하여 2초까지 기다린다.
*******************************************************************************/
bool CNetManager::wifi_detection_wait(void)
{
	const u32 k_timeout_cnt = 10;
	u32 n_timeout = k_timeout_cnt;
	s8 c_result[512];
	
    while( n_timeout-- > 0 )
    {
    	u32 n_length;
    	n_length = process_get_result("wc -c  /etc/config/wireless| awk '{print $1}'",c_result);
    	
    	if( n_length != 0)
    	{
    		if( atoi(c_result) == 0)
    		{
	    		sys_command("/sbin/wifi detect > /tmp/wireless.tmp");
	    		sys_command("cp /tmp/wireless.tmp  /etc/config/wireless");
	    	}
	    	else
	    	{
	    		print_log(DEBUG, 1, "[%s] wifi_detection_wait success - tried count(%d) \n", m_p_log_id,k_timeout_cnt - n_timeout);
	    		return true;
	    	}
    	}
    	sleep_ex(200000); // check every 200 ms
    }
    
    print_log(DEBUG, 1, "[%s] wifi_detection_wait failed - tried count(%d) \n", m_p_log_id,k_timeout_cnt - n_timeout);
    
    return false;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CNetManager::check_valid_speed(void)
{
	if( m_p_system_c->net_is_valid_speed() == false
		|| net_br_lan_state() == false )
    {
    	net_speed_confirm(false);
   		
   		if( m_p_system_c->net_is_valid_speed() == false )
   		{
   			net_phy_reset_clear();
   			net_speed_confirm(true);
   		}
    }
}
