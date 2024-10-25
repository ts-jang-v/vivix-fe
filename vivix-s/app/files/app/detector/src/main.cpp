/******************************************************************************/
/**
 * @file    main.cpp
 * @brief   main 모듈
 * @author  한명희
*******************************************************************************/

/******************************************************************************/
/**
 * @mainpage
 * 
 * @section     architecture
*******************************************************************************/


/*******************************************************************************
*	Include Files
*******************************************************************************/
#include <stdio.h>			// fprintf()
#include <stdlib.h>			// malloc(), free()
#include <unistd.h>			// getpid()
#include <stdbool.h>
#include <signal.h>
#include <string.h>			// memset(), memcpy()
#include <fcntl.h>			// fcntl()
#include <errno.h>			// errno
#include <assert.h>			// assert()
#include <sys/wait.h>		// waitpid(), WNOHANG
#include <syslog.h> 
#include <stdarg.h> 	// va_list

#include "message.h"
#include "misc.h"
#include "command.h"
#include "capture.h"
#include "image.h"
#include "gige_server.h"
#include "control_server.h"
#include "scu_server.h"
#include "ver.h"
#include "calibration.h"
#include "net_manager.h"
#include "diagnosis.h"
#include "image_retransfer.h"
#include "vw_wrapper.h"

using namespace std;

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/

/*******************************************************************************
*	Type Definitions
*******************************************************************************/

/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/

/*******************************************************************************
*	Variable Definitions
*******************************************************************************/
CCommand* 	    g_pcommand_c = NULL;
CImage*         g_p_image_c = NULL;
CCapture*       g_p_capture_c = NULL;
CControlServer* g_p_control_svr_c = NULL;
CVWSystem*      g_p_system_c = NULL;
CGigEServer*    g_p_gige_svr_c = NULL;
CSCUServer*     g_p_scu_svr_c = NULL;
CCalibration*   g_p_calibration_c = NULL;
CNetManager*	g_p_net_manager_c = NULL;
CDiagnosis*		g_p_diagnosis_c = NULL;
CWeb*			g_p_web_c = NULL;
CImageRetransfer*	g_p_retrans_c = NULL;

bool g_brun_main=false;

/*******************************************************************************
*	Function Prototypes
*******************************************************************************/

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void set_signal_handler( void ( * func )( int ), int sig, int flags )
{
    struct sigaction act;

	assert( func );

    act.sa_handler = func;
    sigemptyset( & act.sa_mask );
    sigaddset( & act.sa_mask, sig );
    act.sa_flags = flags;
    sigaction( sig, & act, NULL );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void signal_handler( int signo )
{
	if(signo != SIGCHLD)
	{
	    fprintf( stderr, "ViVIX: signal %d received.\n", signo );
	    lprintf( DEBUG, 1, "[Main] signal %d received.\n", signo );
	}

	if( signo == SIGCHLD )
	{
		// wait until child process has exited.
		while( waitpid( ( pid_t ) -1, NULL, WNOHANG ) > ( pid_t ) 0 )
		{
			sleep_ex( 10 );
		}
	}
	else if( signo == SIGUSR1 )
	{
		g_brun_main = false;
		if(g_pcommand_c != NULL) g_pcommand_c->exit();
	}
	else
	{
		sleep_ex(1000000);
		g_brun_main = false;
		if(g_pcommand_c != NULL) g_pcommand_c->exit();
	}
}


/******************************************************************************/
/**
 * @brief   S/W에 디텍터 상태를 알리기 위한 메시지 전달 함수
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool notify_sw( vw_msg_t* i_p_msg_t )
{
    return g_p_control_svr_c->noti_add( i_p_msg_t );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void add_image( image_buffer_t* i_p_image_t )
{
    return g_p_image_c->add_image( i_p_image_t );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool sender_is_ready(void)
{
    return g_p_control_svr_c->sender_is_ready();
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool sender_start( image_buffer_t* i_p_image_t )
{
    return g_p_control_svr_c->sender_start(i_p_image_t);
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void sender_stop(void)
{
    g_p_control_svr_c->sender_stop();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool cal_data_load(void)
{
	return g_p_calibration_c->load_all_start();
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool sender_retransmit(u32 i_n_image_id, bool i_b_preview, u8* i_p_buff, u32 i_n_length, u16 i_w_slice_number, u16 i_w_total_slice_number)
{
    return g_p_control_svr_c->retransmit(i_n_image_id, i_b_preview, i_p_buff, i_n_length, i_w_slice_number, i_w_total_slice_number );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void format_ext4(char * i_p_dev)
{
    char        p_cmd[100];
    sprintf(p_cmd,"mkfs.ext4 %s -E nodiscard",i_p_dev);
    system(p_cmd);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool mount_ext4(char * i_p_dev,char * i_p_dest)
{
    char        p_cmd[100];

    memset(p_cmd,0,100);
    sprintf(p_cmd,"mount -t ext4 %s %s", i_p_dev, i_p_dest);

    if(system(p_cmd) == 0)
    {
        return true;
    }
    else
    {
        return false;
    }

    return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
#include <sys/resource.h>

void set_core_dump_enabled(void)
{
	struct rlimit rlim;

	rlim.rlim_cur = RLIM_INFINITY;
	rlim.rlim_max = RLIM_INFINITY;

	setrlimit(RLIMIT_CORE,&rlim);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void print_uptime(void)
{
    s8  p_command[128];
    s8  p_uptime[128];
    s32 n_len;
    
    memset( p_command, 0, sizeof(p_command) );
    memset( p_uptime, 0, sizeof(p_uptime) );
    
    sprintf( p_command, "cat /proc/uptime | awk '{print $1}'");
    
    n_len = process_get_result( p_command, p_uptime );
    if( n_len > 0 )
    {
        p_uptime[n_len-1] = '\0';
        
        lprintf( DEBUG, 1, "[Main] uptime: %s\n", p_uptime );
    }
}

/******************************************************************************/
/**
* \brief        main
* \param        none
* \return       none
* \note
*******************************************************************************/
int main( int argc, char ** argv )
{
    set_core_dump_enabled();
    
    if( mount_ext4( (char*)SYS_MOUNT_MMCBLK_P2, (char*)SYS_DATA_PATH ) == false )
    {
        lprintf( DEBUG, 1, "[Main] format: %s\n",SYS_MOUNT_MMCBLK_P2 );
        format_ext4( (char*)SYS_MOUNT_MMCBLK_P2 );
        mount_ext4( (char*)SYS_MOUNT_MMCBLK_P2, (char*)SYS_DATA_PATH );
    }
    
	set_signal_handler( signal_handler, SIGINT, 0 );
	set_signal_handler( signal_handler, SIGTERM, 0 );
	set_signal_handler( signal_handler, SIGQUIT, 0 );
	set_signal_handler( signal_handler, SIGCHLD, 0 );
	set_signal_handler( signal_handler, SIGUSR1, 0 );
	set_signal_handler( SIG_IGN, SIGPIPE, 0 );
	
	lprintf( DEBUG, 1, "[Main] version info: %s\n", VERSION_INFO );
    
    g_p_control_svr_c   = new CControlServer( CONTROL_SVR_PORT, 1, lprintf );
    g_p_scu_svr_c       = new CSCUServer( SCU_SVR_PORT, 1, lprintf );
    
    g_p_system_c        = new CVWSystem( lprintf );    
    g_p_system_c->set_notify_handler( notify_sw );
    g_p_system_c->initialize();

    g_p_calibration_c = new CCalibration( lprintf );
    g_p_calibration_c->m_p_system_c = g_p_system_c;
    g_p_calibration_c->set_notify_handler( notify_sw );
    g_p_calibration_c->start();

    g_p_net_manager_c = new CNetManager( lprintf );
    g_p_net_manager_c->m_p_system_c = g_p_system_c;
    g_p_net_manager_c->set_notify_handler( notify_sw );
    g_p_net_manager_c->start();
    
    g_p_diagnosis_c	= new CDiagnosis( lprintf );
    g_p_diagnosis_c->m_p_system_c		= g_p_system_c;
    g_p_diagnosis_c->m_p_net_manager_c	= g_p_net_manager_c;
    
    g_p_image_c = new CImage( lprintf );
    g_p_image_c->set_send_handler(sender_is_ready, sender_start, sender_stop);
    g_p_image_c->set_notify_handler( notify_sw );
    g_p_image_c->m_p_system_c = g_p_system_c;
    
    g_p_capture_c = new CCapture( lprintf );
    g_p_capture_c->set_add_image_handler( add_image );
    g_p_capture_c->set_notify_handler( notify_sw );
    g_p_capture_c->m_p_system_c			= g_p_system_c;
    g_p_capture_c->m_p_calibration_c	= g_p_calibration_c;
    g_p_capture_c->start();
    
    // UDP server start
    g_p_gige_svr_c      = new CGigEServer( GVCP_PORT, lprintf );
    g_p_gige_svr_c->m_p_system_c		= g_p_system_c;
    g_p_gige_svr_c->m_p_net_manager_c	= g_p_net_manager_c;
    
    g_p_retrans_c = new CImageRetransfer(lprintf, sender_retransmit);
    
    // start server for S/W
    g_p_control_svr_c->m_p_system_c			= g_p_system_c;
    g_p_control_svr_c->m_p_calibration_c	= g_p_calibration_c;
    g_p_control_svr_c->m_p_net_manager_c	= g_p_net_manager_c;
	g_p_control_svr_c->m_p_diagnosis_c		= g_p_diagnosis_c;
    g_p_control_svr_c->m_p_img_retransfer_c = g_p_retrans_c;
    
    g_p_image_c->m_p_img_retransfer_c = g_p_retrans_c;
    
    g_p_control_svr_c->initialize();
    
// start server for SCU
    g_p_scu_svr_c->m_p_system_c			= g_p_system_c;
    
    g_p_system_c->set_cal_data_load_handler(cal_data_load);
        
    // complete init
    g_p_system_c->start();

    print_uptime();
    sys_command("mkdir %skernel", SYS_LOG_PATH);
    sys_command("chmod +x kernel_log.sh");
    sys_command("./kernel_log.sh &");
    
    sys_command("sysctl -w vm.min_free_kbytes=16384");
    
	if( g_p_system_c->is_web_enable() == true )
	{
		g_p_web_c = new CWeb( lprintf );
		
		g_p_web_c->m_p_system_c			= g_p_system_c;
		g_p_web_c->m_p_net_manager_c	= g_p_net_manager_c;
		g_p_web_c->initialize();
		
		g_p_control_svr_c->m_p_web_c	= g_p_web_c;
		g_p_image_c->m_p_web_c			= g_p_web_c;
	}
    
    g_p_control_svr_c->start();
    g_p_scu_svr_c->start();
    g_p_gige_svr_c->start();
    
    g_pcommand_c = new CCommand;
    g_pcommand_c->m_p_capture_c     = g_p_capture_c;
    g_pcommand_c->m_p_system_c      = g_p_system_c;
    g_pcommand_c->m_p_control_svr_c = g_p_control_svr_c;
    g_pcommand_c->m_p_scu_svr_c     = g_p_scu_svr_c;
    g_pcommand_c->m_p_calibration_c	= g_p_calibration_c;
    g_pcommand_c->m_p_net_manager_c	= g_p_net_manager_c;
    g_pcommand_c->m_p_diagnosis_c	= g_p_diagnosis_c;

    if( g_p_system_c->is_web_enable() == true )
    {
    	g_pcommand_c->m_p_web_c			= g_p_web_c;
    }
    sys_command("pkill watchdog");
	g_brun_main = true;
	
	// Boot LED set
	g_p_system_c->boot_end();
	
    g_pcommand_c->m_p_micom_c	= g_p_system_c->get_micom_instance();

    while(g_brun_main == true)
	{
		g_pcommand_c->proc();
		//usleep(100000);
	}
	
	safe_delete( g_pcommand_c );
	
	safe_delete( g_p_capture_c );
	safe_delete( g_p_image_c );
	safe_delete( g_p_web_c );

	safe_delete( g_p_retrans_c );
	safe_delete( g_p_control_svr_c );
	safe_delete( g_p_scu_svr_c );
	safe_delete( g_p_gige_svr_c );
	safe_delete( g_p_calibration_c );
	safe_delete( g_p_net_manager_c );
	safe_delete( g_p_diagnosis_c );
	
	safe_delete( g_p_system_c );
    
    exit( EXIT_SUCCESS );
}

/******************************************************************************/
/**
 * @brief   func for aed_read_trig_volt callback
 * @param   i_aed_id_e: AED id
 * @param   o_p_volt: AED trigger voltage(mV)
 * @return  true: successfully get ADC value from micom
 * @return  false: failed to get ADC value from micom
 * @note    AED class needs to call this function
*******************************************************************************/
bool vw_aed_trig_volt_read( AED_ID i_aed_id_e, u32* o_p_volt )
{
	return g_p_system_c->aed_read_trig_volt( i_aed_id_e, o_p_volt );
}

/******************************************************************************/
/**
 * @brief   func for eeprom_read from micom callback
 * @param   i_w_addr: address
 * @param   o_p_data: pointer of data read from eeprom
 * @param   i_n_len: length of data
 * @param   i_n_eeprom_blk_num: data block of eeprom
 * @return  data length read from eeprom
 * @note    ENV class needs to call this function
*******************************************************************************/
u32 vw_micom_eeprom_read( u16 i_w_addr, u8* o_p_data, u32 i_n_len, eeprom_blk_num_t i_n_eeprom_blk_num )
{
	return g_p_system_c->micom_eeprom_read( i_w_addr, o_p_data, i_n_len, i_n_eeprom_blk_num );
}

/******************************************************************************/
/**
 * @brief   func for eeprom_write from micom callback
 * @param   i_w_addr: address
 * @param   o_p_data: pointer of data to write to eeprom
 * @param   i_n_len: length of data
 * @param   i_n_eeprom_blk_num: data block of eeprom
 * @return  data length written to eeprom
 * @note    ENV class needs to call this function
*******************************************************************************/
u32 vw_micom_eeprom_write( u16 i_w_addr, u8* o_p_data, u32 i_n_len, eeprom_blk_num_t i_n_eeprom_blk_num )
{
	return g_p_system_c->micom_eeprom_write( i_w_addr, o_p_data, i_n_len, i_n_eeprom_blk_num );
}