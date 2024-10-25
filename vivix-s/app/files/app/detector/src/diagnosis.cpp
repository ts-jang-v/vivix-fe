/*******************************************************************************
 모  듈 : diagnosis
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

#ifdef TARGET_COM
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

#include "diagnosis.h"
#include "vw_system_def.h"
#include "misc.h"
#else
#include "typedef.h"
#include "Thread.h"
#include "Dev_io.h"
#include "Stat.h"
#include "../app/detector/include/fpga_reg.h"
#include "../app/detector/include/diagnosis.h"
#include <string.h>
#endif
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
static s8 s_p_diag_category[][20] =
{
	"NONE",
	"VOLTAGE", // category id : 1
	"BATTERY",
	"WIRELESS",
	"SENSOR",
	"MEMORY",
	"IC"
};

static s8 s_p_diag_type[][20]=
{
	"DIAG",
	"INFO"
};


static diag_info_t s_p_diag_info_t[] =
{ 
	/* Category ID			Item ID		Name				Type				Timeout		SCU			Criteria(pass)						Description*/
#if 0
	{eDIAG_CATEGORY_VOLTAGE,	1,		"DDC",				eDIAG_TYPE_DIAG,	5000,		true,		"9.6V < voltage < 14.4V",			"DDC output voltage"},
	{eDIAG_CATEGORY_VOLTAGE,	2,		"MAIN",				eDIAG_TYPE_DIAG,	5000,		false,		"3.36V < voltage < 5.04V",			"Main voltage"},
	{eDIAG_CATEGORY_VOLTAGE,	3,		"SIGNAL",			eDIAG_TYPE_DIAG,	5000,		false,		"2.04V < voltage < 3.96V",			"Core voltage"},
#else
	{eDIAG_CATEGORY_VOLTAGE,	1,		"MAIN",				eDIAG_TYPE_DIAG,	5000,		false,		"7.0V < voltage < 9.0V",			"Main voltage"},
#endif
    
    {eDIAG_CATEGORY_BATTERY,	1,		"Detection",		eDIAG_TYPE_DIAG,	5000,		false,		"Battery is detected.",				"Battery detection"},
	{eDIAG_CATEGORY_BATTERY,	2,		"Voltage",			eDIAG_TYPE_INFO,	5000,		false,		"N/A",								"Battery voltage"},
	{eDIAG_CATEGORY_BATTERY,	3,		"Remain",			eDIAG_TYPE_INFO,	5000,		false,		"N/A",								"Battery remain"},
	
	{eDIAG_CATEGORY_WIRELESS,	1,		"Detection",		eDIAG_TYPE_DIAG,	5000,		false,		"Wireless module is detected.",		"Wireless module detection"},
	{eDIAG_CATEGORY_WIRELESS,	2,		"Connection",		eDIAG_TYPE_DIAG,	60000,		true,		"Detector is connected with SCU.",	"Wireless module detection"},
	
	{eDIAG_CATEGORY_SENSOR,		1,		"Impact Sensor",	eDIAG_TYPE_DIAG,	5000,		false,		"Detector is communicated with imact sensor.",	"Impact sensor"},
	{eDIAG_CATEGORY_SENSOR,		2,		"Temperature",		eDIAG_TYPE_DIAG,	5000,		false,		"0 < temperature < 60",			"Temperature"},
	{eDIAG_CATEGORY_SENSOR,		3,		"AED",				eDIAG_TYPE_DIAG,	5000,		false,		"",									"AED"},
	{eDIAG_CATEGORY_SENSOR,		4,		"Combo Sensor",	    eDIAG_TYPE_DIAG,	5000,		false,		"Detector is communicated with combo sensor.",	"Combo sensor"},

	{eDIAG_CATEGORY_MEMORY,		1,		"Detection",		eDIAG_TYPE_DIAG,	5000,		false,		"MMC is detected.",				    "MMC memory"},
	{eDIAG_CATEGORY_MEMORY,		2,		"Filesystem",		eDIAG_TYPE_DIAG,	120000,		false,		"Filesystem is normal.",			"MMC filesystem"},
	{eDIAG_CATEGORY_MEMORY,		3,		"Status",			eDIAG_TYPE_INFO,	10000,		false,		"N/A",								"MMC status"},
	
	{eDIAG_CATEGORY_IC,			1,		"FPGA",				eDIAG_TYPE_DIAG,	5000,		false,		"Detector is communicated with IC.","FPGA"},
	{eDIAG_CATEGORY_IC,			2,		"Fuel Gauge",		eDIAG_TYPE_DIAG,	5000,		false,		"Detector is communicated with IC.","Fuel gauge"},
#if 0
	{eDIAG_CATEGORY_IC,			3,		"RTC",				eDIAG_TYPE_DIAG,	60000,		false,		"Detector is communicated with IC.","RTC"},
#endif

	// end list
	{eDIAG_CATEGORY_NONE,		0,			NULL,			eDIAG_TYPE_NONE,	0,			false,      NULL,		NULL}
};

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
CDiagnosis::CDiagnosis(int (*log)(int,int,const char *,...))
{
    print_log = log;
    strcpy( m_p_log_id, "Diagnosis     " );
    
    // diag
    m_b_diag_is_running = false;
    m_p_diag_info_t     = NULL;

    m_diag_thread_t = (pthread_t)0;
    m_c_diag_progress = 0;
    m_p_system_c = NULL;
    m_p_net_manager_c = NULL;
    
    print_log(DEBUG, 1, "[%s] CDiagnosis\n", m_p_log_id);
    
}

/******************************************************************************/
/**
* \brief        constructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CDiagnosis::CDiagnosis(void)
{
}

/******************************************************************************/
/**
* \brief        destructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CDiagnosis::~CDiagnosis()
{
    print_log(DEBUG, 1, "[%s] ~CDiagnosis\n", m_p_log_id);
}


/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void* diag_routine( void * arg )
{
	CDiagnosis* diag = (CDiagnosis *)arg;
	diag->diag_proc();
	pthread_exit( NULL );
	return 0;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CDiagnosis::diag_proc(void)
{
    print_log(DEBUG, 1, "[%s] start diag_proc...(category: %d, id: %d)\n", \
                        m_p_log_id, m_p_diag_info_t->category_e, m_p_diag_info_t->c_item_id );
    
    //while( m_b_diag_is_running == true )
    {
        switch( (u8)m_p_diag_info_t->category_e )
        {
            case eDIAG_CATEGORY_VOLTAGE:
            {
                m_diag_result_e = diag_voltage( m_p_diag_info_t->c_item_id );
                break;
            }
            case eDIAG_CATEGORY_BATTERY:
            {
                m_diag_result_e = diag_battery( m_p_diag_info_t->c_item_id );
                break;
            }
            case eDIAG_CATEGORY_WIRELESS:
            {
                m_diag_result_e = diag_wireless( m_p_diag_info_t->c_item_id );
                break;
            }
            case eDIAG_CATEGORY_SENSOR:
            {
                m_diag_result_e = diag_sensor( m_p_diag_info_t->c_item_id );
                break;
            }
            case eDIAG_CATEGORY_MEMORY:
            {
                m_diag_result_e = diag_memory( m_p_diag_info_t->c_item_id );
                break;
            }
            case eDIAG_CATEGORY_IC:
            {
                m_diag_result_e = diag_ic( m_p_diag_info_t->c_item_id );
                break;
            }
            default:
            {
                m_diag_result_e = eDIAG_RESULT_FAIL;
                break;
            }
        }
        m_c_diag_progress = 100;
    }
    
    print_log(DEBUG, 1, "[%s] stop diag_proc...\n", m_p_log_id );
    
    pthread_detach(m_diag_thread_t);
    m_b_diag_is_running = false;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CDiagnosis::diag_start( DIAG_CATEGORY i_category_e, u8 i_c_id )
{
    m_diag_result_e     = eDIAG_RESULT_FAIL;
    m_c_diag_progress   = 0;
    
    if( diag_find_item(i_category_e, i_c_id) == false )
    {
        return false;
    }
    
    m_b_diag_is_running = true;
    
	if( pthread_create(&m_diag_thread_t, NULL, diag_routine, ( void * ) this)!= 0 )
    {
    	print_log( ERROR, 1, "[%s] diag_routine pthread_create:%s\n", \
    	                    m_p_log_id, strerror( errno ));

    	m_b_diag_is_running = false;
    	
    	diag_reset();
    	
    	return false;
    }
    
    return true;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CDiagnosis::diag_stop(void)
{
    if( m_b_diag_is_running )
    {
        m_b_diag_is_running = false;
    	
    	if( pthread_join( m_diag_thread_t, NULL ) != 0 )
    	{
    		print_log( ERROR, 1,"[%s] pthread_join: diag_proc:%s\n", \
    		                    m_p_log_id, strerror( errno ));
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
bool CDiagnosis::diag_make_list( bool i_b_scu )
{
    FILE*           p_file;
	diag_info_t*    p_info_t;
    u16             i = 0;
    
    
	p_file = fopen(SELF_DIAG_LIST_FILE, "w");
	
	if( p_file == NULL )
	{
		print_log( ERROR, 1, "[%s] %s open failed\n", m_p_log_id, SELF_DIAG_LIST_FILE );
		return false;
	}
	
    fprintf( p_file, "<?xml version=\"1.0\"?>\n");
    fprintf( p_file, "<DIAGNOSIS_LIST>\n");
    
    p_info_t = &s_p_diag_info_t[i];
    while( p_info_t->category_e != eDIAG_CATEGORY_NONE )
    {
        if( p_info_t->b_scu_used == true
            && i_b_scu == false )
        {
            i++;
            p_info_t = &s_p_diag_info_t[i];
            continue;
        }
        
        if( (p_info_t->category_e == eDIAG_CATEGORY_SENSOR) &&
            (p_info_t->c_item_id == 4) &&
            (m_p_system_c->hw_gyro_probe() == false) )
        {
            i++;
            p_info_t = &s_p_diag_info_t[i];
            continue;            
        }

        if( m_p_system_c->model_is_wireless() == false )
        {
            if( p_info_t->category_e == eDIAG_CATEGORY_WIRELESS
                || p_info_t->category_e == eDIAG_CATEGORY_BATTERY )
            {
                i++;
                p_info_t = &s_p_diag_info_t[i];
                continue;
            }
        }
        
        fprintf( p_file, "\t<ITEM id=\"D%d%d%d\" category=\"%s\" name=\"%s\" type=\"%s\" timeout=\"%d\"/>\n", \
                        p_info_t->category_e, p_info_t->c_item_id/10, p_info_t->c_item_id%10, \
                        s_p_diag_category[p_info_t->category_e], p_info_t->p_name, \
                        s_p_diag_type[p_info_t->type_e], p_info_t->n_timeout );
        
        i++;
        p_info_t = &s_p_diag_info_t[i];
    }
    fprintf( p_file, "</DIAGNOSIS_LIST>\n");
		
	fclose(p_file);
	return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CDiagnosis::diag_find_item( DIAG_CATEGORY i_category_e, u8 i_c_id )
{
    diag_info_t*    p_info_t;
    u16             i = 0;
    
    p_info_t = &s_p_diag_info_t[i];
    
    while( p_info_t->category_e != eDIAG_CATEGORY_NONE )
    {
        if( p_info_t->category_e == i_category_e
            && p_info_t->c_item_id == i_c_id )
        {
            m_p_diag_info_t = p_info_t;
            return true;
        }
        
        i++;
        p_info_t = &s_p_diag_info_t[i];
    }
    
    print_log( ERROR, 1,"[%s] unknown category: %d or item: %d\n", \
    		                    m_p_log_id, i_category_e, i_c_id );
    m_p_diag_info_t = NULL;
    
    return false;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
DIAG_RESULT CDiagnosis::diag_voltage( u8 i_c_id )
{
    s8          p_result[256];
    s8          p_value[1][256];
    u16         p_hw_power_volt[POWER_VOLT_MAX];
    POWER_VOLT  power_volt_e;
    u16         w_min, w_max;
    sys_state_t	state_t;
    
    memset( &state_t, 0, sizeof(sys_state_t) );
    m_p_system_c->get_state( &state_t );
    
    memcpy( p_hw_power_volt, state_t.p_hw_power_volt, sizeof(p_hw_power_volt) );
    
    if( i_c_id == 1 )
    {
        power_volt_e    = POWER_VOLT_VS;
        w_min           = 700;
        w_max           = 900;
    }
    else
    {
        return eDIAG_RESULT_FAIL;
    }
    
    if( p_hw_power_volt[power_volt_e] > w_min
        && p_hw_power_volt[power_volt_e] < w_max )
    {
        sprintf( p_result, "%s", "PASS" );
    }
    else
    {
        sprintf( p_result, "%s", "NG" );
    }
    
    sprintf( p_value[0], "%.2f V", p_hw_power_volt[power_volt_e]/100.0f );
    
    if( diag_print(p_result, p_value, 1) == false )
    {
        return eDIAG_RESULT_FAIL;
    }
    
    return eDIAG_RESULT_SUCCESS;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
DIAG_RESULT CDiagnosis::diag_battery( u8 i_c_id )
{
    s8          p_result[256];
    s8          p_value[1][256];
    sys_state_t state_t;
    u8			c_battery_num;
    
    memset( &state_t, 0, sizeof(sys_state_t) );
    m_p_system_c->get_state( &state_t );
    
    c_battery_num = m_p_system_c->get_battery_count();
    
    if( i_c_id == 1 )
    {
        //if( state_t.b_battery_equipped == true && ( state_t.b_battery2_equipped == true ||  m_p_hw_mon_c->bat_get_battery_num() == 1))
        if( c_battery_num == 1 )
        {
	        if( state_t.b_battery_equipped == true)
	        {
	            sprintf( p_result, "%s", "PASS" );
	            sprintf( p_value[0], "%s", "YES" );
	        }
	        else
	        {
	            sprintf( p_result, "%s", "NG" );
	            sprintf( p_value[0], "%s", "NO" );
	        }
	    }
	    else if( c_battery_num == 2 )
	    {
	    	if( state_t.b_battery_equipped == true &&  state_t.b_battery2_equipped == true)
	        {
	            sprintf( p_result, "%s", "PASS" );
	            sprintf( p_value[0], "%s %s", "1-YES","2-YES" );
	        }
	        else if ( state_t.b_battery_equipped == false &&  state_t.b_battery2_equipped == true)
	        {
	        	sprintf( p_result, "%s", "NG" );
	            sprintf( p_value[0], "%s %s", "1-YES","2-NO" );
	        }
	        else if ( state_t.b_battery_equipped == true &&  state_t.b_battery2_equipped == false)
	        {
	        	sprintf( p_result, "%s", "NO" );
	            sprintf( p_value[0], "%s %s","1-NO" , "2-YES" );
	        }	        
	        else
	        {
	            sprintf( p_result, "%s", "NG" );
	            sprintf( p_value[0], "%s %s","1-NO", "2-NO" );
	        }
	    }
    }
    else if( i_c_id == 2 )
    {
        sprintf( p_result, "%s", "N/A" );
        if( c_battery_num == 1 )
        {
        	sprintf( p_value[0], "%2.2f", state_t.w_battery_vcell/100.0f );
        }
        else if( c_battery_num == 2 )
    	{
        	sprintf( p_value[0], "1-%2.2f 2-%2.2f", state_t.w_battery2_vcell/100.0f, state_t.w_battery_vcell/100.0f );
        }
    }
    else if( i_c_id == 3 )
    {
        sprintf( p_result, "%s", "N/A" );
        sprintf( p_value[0], "%2.1f", state_t.w_battery_gauge/10.0f );
    }
    else
    {
        return eDIAG_RESULT_FAIL;
    }
    
    if( diag_print(p_result, p_value, 1) == false )
    {
        return eDIAG_RESULT_FAIL;
    }
    
    return eDIAG_RESULT_SUCCESS;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
DIAG_RESULT CDiagnosis::diag_wireless( u8 i_c_id )
{
    s8          p_result[256];
    s8          p_value[1][256];
    
    if( i_c_id == 1 )
    {
        if( diag_wireless_module() == true )
        {
            sprintf( p_result, "%s", "PASS" );
            sprintf( p_value[0], "%s", "YES" );
        }
        else
        {
            sprintf( p_result, "%s", "NG" );
            sprintf( p_value[0], "%s", "NO" );
        }
    }
    else if( i_c_id == 2 )
    {
        if( m_p_system_c->net_get_mode() != eNET_MODE_TETHER )
        {
            sprintf( p_result, "%s", "N/A" );
            sprintf( p_value[0], "%s", "N/A" );
        }
        else if( diag_wireless_connection() == true )
        {
            sprintf( p_result, "%s", "PASS" );
            sprintf( p_value[0], "%s", "YES" );
        }
        else
        {
            sprintf( p_result, "%s", "NG" );
            sprintf( p_value[0], "%s", "NO" );
        }
    }
    else
    {
        return eDIAG_RESULT_FAIL;
    }
    
    if( diag_print(p_result, p_value, 1) == false )
    {
        return eDIAG_RESULT_FAIL;
    }
    
    return eDIAG_RESULT_SUCCESS;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CDiagnosis::diag_wireless_module(void)
{
    s32 n_result;
    s8  p_buffer[1024];
    
    memset( p_buffer, 0, sizeof(p_buffer) );
    
    n_result = process_get_result("lspci -k | awk '/ath/ {print $4}'", p_buffer);
    if( n_result <= 0 )
    {
        print_log( ERROR, 1, "[%s] Finding PCI device is failed.\n", m_p_log_id );
        return false;
    }
    
    //168c:0030 - ath9k, 168c:003C - ath10k
    if( strcmp(p_buffer, "168c:0030\n") != 0 && strcmp(p_buffer, "168c:003c\n") != 0)
    {
        print_log( ERROR, 1, "[%s] PCI device id(%s/168c:0030) error\n", m_p_log_id, p_buffer );
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
bool CDiagnosis::diag_wireless_connection(void)
{
    bool                b_result = true;
    vw_wlan_state_t     wlan_state_t;
    CTime*              p_wait_time_c = NULL;
    u16                 w_count = 0;
    
    p_wait_time_c = new CTime(m_p_diag_info_t->n_timeout);
    
    while( w_count < 3 )
    {
        if( m_p_net_manager_c->net_discovery() == true )
        {
            break;
        }
        
        sleep_ex(1000000);
        
        w_count++;
    }
    
    m_p_net_manager_c->net_set_mode( eNET_MODE_STATION );
    
    while( m_b_diag_is_running == true )
    {
        
        sleep_ex(100000);
        
        memset( &wlan_state_t, 0, sizeof(vw_wlan_state_t) );
        
        b_result = m_p_system_c->get_state_wlan_lock( "wlan0", &wlan_state_t );
        
        if( b_result == true )
        {
            if( m_p_system_c->wireless_get_level(wlan_state_t.c_quality) > 1 )
            {
                break;
            }
        }
        
        if( p_wait_time_c->is_expired() == true )
        {
            safe_delete(p_wait_time_c);
            b_result = false;
            break;
        }
    }
    
    m_p_net_manager_c->net_set_interface( eNET_MODE_TETHER );
    
    return b_result;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
DIAG_RESULT CDiagnosis::diag_sensor( u8 i_c_id )
{
    s8          p_result[256];
    s8          p_value[1][256];
    sys_state_t state_t;
    
    if( i_c_id == 1 )
    {
        if( m_p_system_c->hw_impact_diagnosis() == true )
        {
            sprintf( p_result, "%s", "PASS" );
            sprintf( p_value[0], "%s", "YES" );
        }
        else
        {
            sprintf( p_result, "%s", "NG" );
            sprintf( p_value[0], "%s", "NO" );
        }
    }
    else if( i_c_id == 2 )
    {
    	memset( &state_t, 0, sizeof(sys_state_t) );
    	m_p_system_c->get_state(&state_t);
        
        if( state_t.w_temperature > 0
            && state_t.w_temperature < 600 )
        {
            sprintf( p_result, "%s", "PASS" );
        }
        else
        {
            sprintf( p_result, "%s", "NG" );
        }
        
        sprintf( p_value[0], "%2.1f", state_t.w_temperature/10.0f );
    }
    else if( i_c_id == 3 )
    {
        if( diag_sensor_aed() == true )
        {
            sprintf( p_result, "%s", "PASS" );
            sprintf( p_value[0], "%s", "YES" );
        }
        else
        {
            sprintf( p_result, "%s", "NG" );
            sprintf( p_value[0], "%s", "NO" );
        }
    }
    else if( i_c_id == 4 )
    {
        if( m_p_system_c->hw_gyro_diagnosis() == true )
        {
            sprintf( p_result, "%s", "PASS" );
            sprintf( p_value[0], "%s", "YES" );
        }
        else
        {
            sprintf( p_result, "%s", "NG" );
            sprintf( p_value[0], "%s", "NO" );
        }
    }
    else
    {
        return eDIAG_RESULT_FAIL;
    }
    
    if( diag_print(p_result, p_value, 1) == false )
    {
        return eDIAG_RESULT_FAIL;
    }
    return eDIAG_RESULT_SUCCESS;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CDiagnosis::diag_sensor_aed(void)
{
    CTime*  p_wait_time_c;
    u16     w_wait_time;
    u16     w_progress_unit;
    
    w_wait_time     = m_p_system_c->dev_reg_read(eDEV_ID_FPGA, (u32)eFPGA_REG_AED_HIGH_CNT);
    
    if( w_wait_time == 0 )
    {
        print_log( ERROR, 1, "[%s] invalid value(eFPGA_REG_AED_HIGH_CNT: 0)\n", m_p_log_id );
        return false;
    }
    
    w_progress_unit = w_wait_time/500;
    
    if( w_progress_unit == 0 )
    {
        w_progress_unit = 80;
    }
    else
    {
        w_progress_unit = 80/w_progress_unit;
    }
    
   m_p_system_c->dev_reg_write(eDEV_ID_FPGA, (u32)eFPGA_REG_AED_ERROR, 0);
    
    m_c_diag_progress = 10;
    
    p_wait_time_c = new CTime(w_wait_time);
    
    
    while( m_b_diag_is_running == true )
    {
        sleep_ex(500000);
        m_c_diag_progress += w_progress_unit;
        
        if( p_wait_time_c->is_expired() == true )
        {
            safe_delete(p_wait_time_c);
            break;
        }
    }
    
    if( m_b_diag_is_running == false )
    {
        return false;
    }
    
    if( m_p_system_c->dev_reg_read(eDEV_ID_FPGA, (u32)eFPGA_REG_AED_ERROR) > 0 )
    {
        print_log( ERROR, 1, "[%s] AED sensor error is detected.\n", m_p_log_id );
        
        m_p_system_c->dev_reg_write(eDEV_ID_FPGA, (u32)eFPGA_REG_AED_ERROR, 0);
        
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
DIAG_RESULT CDiagnosis::diag_memory( u8 i_c_id )
{
    s8          p_result[256];
    s8          p_value[1][256];
    
    if( i_c_id == 1 )
    {
        if( diag_memory_detection() == true )
        {
            sprintf( p_result, "%s", "PASS" );
            sprintf( p_value[0], "%s", "YES" );
        }
        else
        {
            sprintf( p_result, "%s", "NG" );
            sprintf( p_value[0], "%s", "NO" );
        }
    }
    else if( i_c_id == 2 )
    {
        if( diag_memory_filesystem() == true )
        {
            sprintf( p_result, "%s", "PASS" );
            sprintf( p_value[0], "%s", "YES" );
        }
        else
        {
            sprintf( p_result, "%s", "NG" );
            sprintf( p_value[0], "%s", "NO" );
        }
    }
    else if( i_c_id == 3 )
    {
        u16 w_use = 0;
        if( diag_memory_use(&w_use) == true )
        {
            
            sprintf( p_result, "%s", "PASS" );
            sprintf( p_value[0], "%d %%", w_use );
        }
        else
        {
            sprintf( p_result, "%s", "NG" );
            sprintf( p_value[0], "0" );
        }
    }
    else
    {
        return eDIAG_RESULT_FAIL;
    }
    
    if( diag_print(p_result, p_value, 1) == false )
    {
        return eDIAG_RESULT_FAIL;
    }
    
    return eDIAG_RESULT_SUCCESS;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CDiagnosis::diag_memory_detection(void)
{
    struct	stat	stat_t;
    
    if( lstat( SYS_MOUNT_MMCBLK_P2, &stat_t ) != 0 )
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
bool CDiagnosis::diag_memory_filesystem(void)
{
    if( diag_memory_detection() == false )
    {
        return false;
    }
    
    sys_command("umount %s",SYS_MOUNT_MMCBLK_P2);
    sys_command("e2fsck  -cvy -j ext4 %s",SYS_MOUNT_MMCBLK_P2);
    sys_command("mount -t ext4 %s /mnt/mmc/p2/",SYS_MOUNT_MMCBLK_P2);
    
    return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CDiagnosis::diag_memory_use( u16* o_p_use )
{
    s32             n_result;
    s8              p_buffer[1024];
    s8              p_cmd[256];
    struct	stat	stat_t;
    
    if( lstat(SYS_MOUNT_MMCBLK_P3, &stat_t ) != 0 )
    {
        return false;
    }
    
    memset( p_buffer, 0, sizeof(p_buffer) );
    sprintf(p_cmd,"df -m | awk '/mmcblk%sp2/ {print $5}'",SYS_MOUNT_MMCBLK_NUM);
    n_result = process_get_result(p_cmd, p_buffer);
    if( n_result <= 0 )
    {
        print_log( ERROR, 1, "[%s] df command failed\n", m_p_log_id );
        
        *o_p_use = 0;
        
        return false;
    }
    
    *o_p_use = (u16)atoi(p_buffer);
        
    return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
DIAG_RESULT CDiagnosis::diag_ic( u8 i_c_id )
{
    s8          p_result[256];
    s8          p_value[1][256];
    
    if( i_c_id == 1 )
    {
        if( diag_ic_fpga() == true )
        {
            sprintf( p_result, "%s", "PASS" );
            sprintf( p_value[0], "%s", "YES" );
        }
        else
        {
            sprintf( p_result, "%s", "NG" );
            sprintf( p_value[0], "%s", "NO" );
        }
    }
    else if( i_c_id == 2 )
    {
    	u8		c_idx = 0;
    	bool	b_diagnosis_all_clean = true;
    	u8		c_bat_num = m_p_system_c->get_battery_count();
    	u8		c_start_idx = m_p_system_c->hw_bat_equip_start_idx();
    	
    	for( c_idx = c_start_idx; c_idx < c_start_idx + c_bat_num; c_idx++ )
    	{
    		if( m_p_system_c->hw_bat_diagnosis(c_idx) == false)
    		{
    			b_diagnosis_all_clean = false;
    			break;
    		}
    	}
        if( b_diagnosis_all_clean )
        {
            sprintf( p_result, "%s", "PASS" );
            sprintf( p_value[0], "%s", "YES" );
        }
        else
        {
            sprintf( p_result, "%s", "NG" );
            sprintf( p_value[0], "%s", "NO" );
        }
    }
    else if( i_c_id == 3 )
    {
        if( diag_ic_rtc() == true )
        {
            sprintf( p_result, "%s", "PASS" );
            sprintf( p_value[0], "%s", "YES" );
        }
        else
        {
            sprintf( p_result, "%s", "NG" );
            sprintf( p_value[0], "%s", "NO" );
        }
    }
    else
    {
        return eDIAG_RESULT_FAIL;
    }
    
    if( diag_print(p_result, p_value, 1) == false )
    {
        return eDIAG_RESULT_FAIL;
    }
    
    return eDIAG_RESULT_SUCCESS;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CDiagnosis::diag_ic_rtc(void)
{
    s8      p_cur_time[256];
    s8      p_new_time[256];
    s32     n_len;
    CTime*  p_wait_time_c;
    
    n_len = process_get_result("cat /sys/class/rtc/rtc0/time", p_cur_time);
    
    if( n_len <= 0 )
    {
        print_log( ERROR, 1, "[%s] getting rtc time is failed\n", m_p_log_id );
        return false;
    }
    
    m_c_diag_progress = 10;
    
    p_wait_time_c = new CTime(3000);
    
    while( m_b_diag_is_running == true )
    {
        sleep_ex(500000);
        m_c_diag_progress += 5;
        if( p_wait_time_c->is_expired() == true )
        {
            safe_delete(p_wait_time_c);
            break;
        }
    }
    
    if( m_b_diag_is_running == false )
    {
        return false;
    }
    
    n_len = process_get_result("cat /sys/class/rtc/rtc0/time", p_new_time);
    
    if( n_len <= 0 )
    {
        print_log( ERROR, 1, "[%s] getting new rtc time is failed\n", m_p_log_id );
        return false;
    }
    
    if( strcmp( p_new_time, p_cur_time ) == 0 )
    {
        return false;
    }
    
    diag_ic_rtc_start_coin_cell();
    
    return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CDiagnosis::diag_ic_rtc_start_coin_cell(void)
{
    FILE*   p_file;
    s8      p_path[256];
    u32     n_time;
    
    sprintf( p_path, "%s%s", SYS_LOG_PATH, "rtc.time" );
    
    p_file = fopen( p_path, "w" );
    
    n_time = time(NULL);
    
    fwrite( &n_time, 1, sizeof(n_time), p_file );
    
    fclose( p_file );
    
    print_log(DEBUG, 1, "[%s] rtc diag: saved(%d)\n", m_p_log_id, n_time );
    
    sys_command("sync");
    
    m_p_system_c->hw_reboot();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CDiagnosis::diag_ic_rtc_check_coin_cell(void)
{
    FILE*   p_file;
    s8      p_path[256];
    u32     n_saved_time, n_cur_time;
    
    s8      p_result[256];
    s8      p_value[1][256];
    
    m_c_diag_progress = 50;
    
    sprintf( p_path, "%s%s", SYS_LOG_PATH, "rtc.time" );
    
    p_file = fopen( p_path, "r" );
    
    if( p_file == NULL )
    {
        return;
    }
    
    fread( &n_saved_time, 1, sizeof(n_saved_time), p_file );
    
    fclose( p_file );
    
    sys_command("rm %s", p_path);
    sys_command("sync");
    
    n_cur_time = time(NULL);
    
    print_log(DEBUG, 1, "[%s] rtc diag: cur(%d), saved(%d)\n", m_p_log_id, n_cur_time, n_saved_time );
    
    m_diag_result_e = eDIAG_RESULT_SUCCESS;
    
    if( n_cur_time > n_saved_time )
    {
        sprintf( p_result, "%s", "PASS" );
        sprintf( p_value[0], "%s", "YES" );
        
    }
    else
    {
        sprintf( p_result, "%s", "NG" );
        sprintf( p_value[0], "%s", "NO" );
        
        m_diag_result_e = eDIAG_RESULT_FAIL;
    }
    
    // make diag result
    if( diag_find_item( eDIAG_CATEGORY_IC, 3 ) == true )
    {
        if( diag_print(p_result, p_value, 1) == false )
        {
            m_diag_result_e = eDIAG_RESULT_FAIL;
        }
    }
    else
    {
        m_diag_result_e = eDIAG_RESULT_FAIL;
    }
    
    m_c_diag_progress = 100;
    
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CDiagnosis::diag_ic_fpga(void)
{
    u16 i, w_data;
    
    if( diag_ic_fpga_test(0xFFFF) != 0xFFFF )
    {
        return false;
    }
    if( diag_ic_fpga_test(0xAAAA) != 0xAAAA )
    {
        return false;
    }
    if( diag_ic_fpga_test(0x5555) != 0x5555 )
    {
        return false;
    }
    if( diag_ic_fpga_test(0) != 0 )
    {
        return false;
    }
    
    w_data = 1;
    for( i = 0; i < 16; i++ )
    {
        if( diag_ic_fpga_test(w_data) != w_data )
        {
            return false;
        }
        w_data = w_data << 1;
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
u16 CDiagnosis::diag_ic_fpga_test( u16 i_w_data )
{
    m_p_system_c->dev_reg_write(eDEV_ID_FPGA, eFPGA_REG_CONNECTION_TEST, i_w_data);
    
    return m_p_system_c->dev_reg_read(eDEV_ID_FPGA, eFPGA_REG_CONNECTION_TEST);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CDiagnosis::diag_print( s8* i_p_result, s8 i_p_value[][DIAG_VALUE_LENGTH], u16 i_w_value_count )
{
    FILE*       p_file;
    u16         i;
    
    m_c_diag_progress = 95;
	
	p_file = fopen(SELF_DIAG_RESULT_FILE, "w");
	
	if( p_file == NULL )
	{
		print_log( ERROR, 1, "[%s] %s open failed\n", m_p_log_id, SELF_DIAG_RESULT_FILE );
		return false;
	}
	
    fprintf( p_file, "<?xml version=\"1.0\"?>\n");
    fprintf( p_file, "<DIAGNOSIS>\n");
    fprintf( p_file, "\t<ITEM id=\"D%d%d%d\" category=\"%s\" name=\"%s\" type=\"%s\"/>\n", \
                        m_p_diag_info_t->category_e, m_p_diag_info_t->c_item_id/10, m_p_diag_info_t->c_item_id%10, \
                        s_p_diag_category[m_p_diag_info_t->category_e], m_p_diag_info_t->p_name, \
                        s_p_diag_type[m_p_diag_info_t->type_e] );
    fprintf( p_file, "\t<RESULT judgment=\"%s\">\n", i_p_result );
    for( i = 0; i < i_w_value_count; i++ )
    {
        fprintf( p_file, "\t\t<VALUE>%s</VALUE>\n", i_p_value[i] );
    }
    fprintf( p_file, "\t</RESULT>\n");
    fprintf( p_file, "</DIAGNOSIS>\n");
	
	
	fclose(p_file);
	return true;
}
