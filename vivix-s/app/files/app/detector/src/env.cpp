/*******************************************************************************
 모  듈 : env
 작성자 : 한명희
 버  전 : 0.0.1
 설  명 : 설정 값 load/save/set/get
 참  고 : 

 버  전:
         0.0.1  최초 작성
*******************************************************************************/



/*******************************************************************************
*	Include Files
*******************************************************************************/

#ifdef TARGET_COM
#include <iostream>
#include <string.h>			// memset(), memcpy()
#include <arpa/inet.h>		// inet_ntoa()
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>			// memset() memset/memcpy
#include <stdlib.h>			// malloc(), free(),exit(), EXIT_SUCCESS rand/rand 
#include <stdio.h>			// fprintf()
#include <sys/ioctl.h>  	// ioctl()
#include <fcntl.h>          // open() O_NDELAY
#include <unistd.h>

#include <vector>
#include <fstream>

#include "env.h"
#include "ver.h"
#include "vw_file.h"
#include "vw_net.h"
#include "vw_xml.h"
#include "vworks_ioctl.h"
#include "misc.h"
#include "rapidxml.hpp"
#include "rapidxml_print.hpp"


#else
#include "typedef.h"
#include "Dev_io.h"
#include "../app/detector/include/env.h"

#include "../app/xml/include/rapidxml.hpp"
#define RAPIDXML_NO_STREAMS
#include "../app/xml/include/rapidxml_print.hpp"
#include <string.h>

#endif
using namespace std;
using namespace rapidxml;


/*******************************************************************************
*	Constant Definitions
*******************************************************************************/

#define PRESET_ID_SYSTEM			0
#define AED_LOW_THRESHOLD_VOLTAGE (900)
#define AED_HIGH_THRESHOLD_VOLTAGE (950)

/*******************************************************************************
*	Type Definitions
*******************************************************************************/

/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/

/*******************************************************************************
*	Variable Definitions
*******************************************************************************/

/*******************************************************************************
*	Function Prototypes
*******************************************************************************/



/******************************************************************************/
/**
* \brief        constructor
* \return       none
* \note
*******************************************************************************/
CEnv::CEnv(int (*log)(int,int,const char *,...))
{
    print_log = log;

    m_b_preset = false;
	m_roic_model_type_e = eROIC_TYPE_MAX;
	m_c_hw_ver = 0;

    strcpy( m_p_log_id, "Env           " );
    
    memset( m_p_config, 0, sizeof(m_p_config) );
    
	m_n_eeprom_fd = open( "/dev/vivix_spi", O_RDWR|O_NDELAY );
	
	if( m_n_eeprom_fd < 0 )
	{
		print_log(ERROR,1,"[%s] /dev/vivix_spi device open error.\n", m_p_log_id);
		
		m_n_eeprom_fd = -1;
	} 
}
/******************************************************************************/
/**
* \brief        constructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CEnv::CEnv(void)
{
}

/******************************************************************************/
/**
* \brief        destructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CEnv::~CEnv()
{
    if( m_n_eeprom_fd != -1 )
    {
        close( m_n_eeprom_fd );
    }
    
    print_log(DEBUG, 1, "[%s] ~CEnv\n", m_p_log_id);
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
DEV_ERROR CEnv::initialize(u8 i_c_hw_ver)
{
    print_log(DEBUG, 1, "[%s] CEnv\n", m_p_log_id);

    m_model_e = eMODEL_3643VW;
	strcpy( m_p_config[0][eCFG_MODEL], "FXRD-3643VAW");
	strcpy( m_p_config[0][eCFG_SERIAL], "None");
	m_c_hw_ver = i_c_hw_ver;

    manufacture_info_default();
    
	config_load(CONFIG_XML_FILE);

	return eDEV_ERR_NO;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note			should be called once in operation lifespan
*******************************************************************************/
ROIC_TYPE CEnv::discover_roic_model_type(void)
{
	m_roic_model_type_e = eROIC_TYPE_MAX;
	s32	fd_roic = 0;
	ioctl_data_t iod_t;
	memset(&iod_t, 0, sizeof(ioctl_data_t));
    CTime* p_roic_power_wait = NULL;

	//ADI DISCOVERY
	//0. INSTALL & Load ADI 1255 ROIC driver 
	if(insmod("/run/vivix_adi_adas1256.ko") < 0)
	{
		print_log(ERROR,1,"[%s] install adi_adas1256 modules failed.\n", m_p_log_id);
		return eROIC_TYPE_MAX;
	}
	
	fd_roic = open("/dev/vivix_adi_adas1256",O_RDWR|O_NDELAY);		
	if(fd_roic < 0)
	{
		print_log(ERROR,1,"[%s] vivix_adi_adas1256 driver open failed.\n", m_p_log_id);
		return eROIC_TYPE_MAX;
	}   	

	//1. Prepare discovery ioctl
	iod_t.data[0] = 1;
    ioctl( fd_roic, VW_MSG_IOCTL_RO_PWR_EN, &iod_t ); 

	iod_t.data[0] = m_model_e;
	ioctl( fd_roic, VW_MSG_IOCTL_ROIC_SET_PREREQUISITE, &iod_t );


    p_roic_power_wait = new CTime(VW_ROIC_GATE_POWER_UP_TIME);
    while( p_roic_power_wait->is_expired() == false )
    {
    	sleep_ex(50000);       // 50 ms
    }
    safe_delete(p_roic_power_wait);


	//2.Run ADI ROIC discovery and otehrs  
	ioctl( fd_roic, VW_MSG_IOCTL_ROIC_DISCOVERY, &iod_t );

	if(iod_t.data[0])
	{
		print_log(ERROR,1,"[%s] eROIC_TYPE_ADI_1255 discovery success\n", m_p_log_id);
		m_roic_model_type_e = eROIC_TYPE_ADI_1255;
		goto END;
	}
	else	//if not discovered, clear resource 
	{
		close(fd_roic);
		if(rmmod("vivix_adi_adas1256") < 0)
		{
			print_log(ERROR,1,"[%s] remove vivix_adi_adas1256 module failed.\n", m_p_log_id);
			return eROIC_TYPE_MAX;
		}
	}

	//TI DISCOVERY
	//0. INSTALL & Load ADI 1255 ROIC driver 
	if(insmod("/run/vivix_ti_afe2256.ko") < 0)
	{
		print_log(ERROR,1,"[%s] install vivix_ti_afe2256 modules failed.\n", m_p_log_id);
		return eROIC_TYPE_MAX;
	}
	
	fd_roic = open("/dev/vivix_ti_afe2256",O_RDWR|O_NDELAY);		
	if(fd_roic < 0)
	{
		print_log(ERROR,1,"[%s] vivix_ti_afe2256 driver open failed.\n", m_p_log_id);
		return eROIC_TYPE_MAX;
	}   	

	//1. Prepare discovery ioctl
	iod_t.data[0] = 1;
    ioctl( fd_roic, VW_MSG_IOCTL_RO_PWR_EN, &iod_t ); 

	iod_t.data[0] = m_model_e;
	ioctl( fd_roic, VW_MSG_IOCTL_ROIC_SET_PREREQUISITE, &iod_t );

    p_roic_power_wait = new CTime(VW_ROIC_GATE_POWER_UP_TIME);
    while( p_roic_power_wait->is_expired() == false )
    {
    	sleep_ex(50000);       // 50 ms
    }
    safe_delete(p_roic_power_wait);

	//2.Run ADI ROIC discovery and otehrs  
	ioctl( fd_roic, VW_MSG_IOCTL_ROIC_DISCOVERY, &iod_t );
	if(iod_t.data[0])
	{
		print_log(ERROR,1,"[%s] eROIC_TYPE_TI_2256GR discovery success\n", m_p_log_id);
		m_roic_model_type_e = eROIC_TYPE_TI_2256GR;
		goto END;
	}
	else	//if not discovered, clear resource 
	{
		close(fd_roic);
		if(rmmod("vivix_ti_afe2256") < 0)
		{
			print_log(ERROR,1,"[%s] remove vivix_ti_afe2256 module failed.\n", m_p_log_id);
			return eROIC_TYPE_MAX;
		}
	}

// ERROR:
// 	print_log(ERROR,1,"[%s] discover_roic_model_type failed. No ROIC is discovered.\n", m_p_log_id);

END: 
	close(fd_roic);	
	return	m_roic_model_type_e;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
ROIC_TYPE CEnv::get_roic_model_type(void)
{
	return	m_roic_model_type_e;
}

/******************************************************************************/
/**
* \brief        make default env file
* \param        none
* \return       none
* \note
*******************************************************************************/
void CEnv::config_default(void)
{
	// LAN
    strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_IP_ADDRESS],    "169.254.1.10" );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_NETMASK],       "255.255.0.0" );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_GATEWAY],       "169.254.2.100" );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_POE_USE_ONLY_POWER],  "off" );
	
	// WLAN
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_SSID],          "vivix" );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_KEY],           "1234567890" );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_SCAN],          "0" );
	
	// AP
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_ENABLE],     "off" );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_FREQ], "5");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_COUNTRY], "KR");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_COUNTRY_CODE], "39");

	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_BAND], "HT40+");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_SSID], "vivix_ap");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_KEY], "1234567890");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_CHANNEL], "36");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_GI], "400");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_SECURITY], "psk2");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_TX_POWER], "17");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_DHCP], "on");
	
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_TAG_ID], "0");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_PRESET_NAME], "System");
	
	// power mode
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_PM_SLEEP_ENABLE], "off");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_PM_SLEEP_ENTRANCE_TIME], "10");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_PM_DEEP_SLEEP_ENABLE], "off");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_PM_DEEP_SLEEP_ENTRANCE_TIME], "30");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_PM_SHUTDOWN_ENABLE], "off");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_PM_SHUTDOWN_ENTRANCE_TIME], "30");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_PM_POWER_SOURCE], "Detector");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_PM_AWD], "off");
	
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_TIMEOUT], "20");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_PREVIEW], "off");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_FAST_TACT], "off");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_BACKUP], "enable");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_DIRECTION], "0");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_DIRECTION_USER_INPUT], "0");
	
	// 초기화 항목에서 제외
	//strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_IMACT_PEAK], "100");
	
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_DEFECT_LOWER_GRID], "off");
	
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_OFFSET_ENABLE], "off");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_OFFSET_PERIOD], "30");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_OFFSET_COUNT], "5");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_OFFSET_TEMPERATURE], "3");
	
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_EXP_TRIG_TYPE], "2");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_BUTTON_FUNC_TYPE], "0");
}

/******************************************************************************/
/**
* \brief        make default network settings
* \param        none
* \return       none
* \note
*******************************************************************************/
void CEnv::config_default_network(void)
{
    print_log(DEBUG,1,"[%s] reset network settings\n", m_p_log_id);
    
    // LAN
    strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_IP_ADDRESS],			"169.254.1.10" );
    strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_NETMASK],				"255.255.0.0" );
    strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_GATEWAY],				"169.254.2.100" );
    strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_POE_USE_ONLY_POWER],	"off" );
    
    // WLAN
    strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_SSID],	"vivix" );
    strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_KEY],		"1234567890" );
    strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_SCAN],	"0" );
    
    // AP
    strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_ENABLE],	"off" );
    strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_FREQ],		"5");
    strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_COUNTRY],	"KR");
    strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_COUNTRY_CODE], "39");
    strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_BAND],		"HT40+");
    strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_SSID],		"vivix_ap");
    strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_KEY],		"1234567890");
    strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_CHANNEL],	"36");
    strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_GI],		"400");
    strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_SECURITY],	"psk2");
    strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_TX_POWER],	"17");
    
    strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_DHCP], "on");
    
    // Preset 이름 초기화 되도록 수정
    strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_PRESET_NAME], "System");
    
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CEnv::config_net_is_changed(const s8* i_p_file_name)
{
	xml_document<> doc;
	xml_node<> * root_node;
	char* p_config;
	
	// Read the xml file into a vector
	ifstream theFile(i_p_file_name);
	
	if( !theFile.is_open() )
	{
		print_log(ERROR,1,"[%s] config_net_is_changed: %s open failed\n", m_p_log_id, i_p_file_name);
		return false;
	}
	
	vector<char> buffer((istreambuf_iterator<char>(theFile)), istreambuf_iterator<char>());
	buffer.push_back('\0');
	
	theFile.close();
	
	// Parse the buffer using the xml file parsing library into doc 
	doc.parse<0>(&buffer[0]);
	
	root_node = doc.first_node("Device");
	
	if( root_node->first_node("APBUTTON") != NULL )
	{
		xml_node<> * node = root_node->first_node("APBUTTON");
		
		p_config = node->first_attribute("type")->value();
		if( strcmp( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_BUTTON_FUNC_TYPE], p_config ) != 0 )
		{
			return true;
		}
	}
	
	xml_node<> * preset_level_1_node;
	xml_node<> * preset_level_2_node;
	
	xml_node<> * preset_node = root_node->first_node("Preset");
	
	preset_level_1_node = preset_node->first_node("Network");
	
	// LAN
	preset_level_2_node = preset_level_1_node->first_node("Lan");
	
	p_config = preset_level_2_node->first_attribute("ip")->value();
	if( strcmp( m_p_config[PRESET_ID_SYSTEM][eCFG_IP_ADDRESS], p_config ) != 0 )
	{
		print_log(DEBUG,1,"[%s] IP address %s -> %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_IP_ADDRESS], p_config);
		return true;
	}
	
	p_config = preset_level_2_node->first_attribute("netmask")->value();
	if( strcmp( m_p_config[PRESET_ID_SYSTEM][eCFG_NETMASK], p_config ) != 0 )
	{
		print_log(DEBUG,1,"[%s] netmask %s -> %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_NETMASK], p_config);
		return true;
	}
	
	p_config = preset_level_2_node->first_attribute("gateway")->value();
	if( strcmp( m_p_config[PRESET_ID_SYSTEM][eCFG_GATEWAY], p_config ) != 0 )
	{
		print_log(DEBUG,1,"[%s] gateway %s -> %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_GATEWAY], p_config);
		return true;
	}
	
	p_config = preset_level_2_node->first_attribute("use_only_power")->value();
	if( strcmp( m_p_config[PRESET_ID_SYSTEM][eCFG_POE_USE_ONLY_POWER], p_config ) != 0 )
	{
		print_log(DEBUG,1,"[%s] wireless only %s -> %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_POE_USE_ONLY_POWER], p_config);
		return true;
	}
	
	// WLAN
	preset_level_2_node = preset_level_1_node->first_node("WLan");
	
	p_config = preset_level_2_node->first_attribute("ssid")->value();
	if( strcmp( m_p_config[PRESET_ID_SYSTEM][eCFG_SSID], p_config ) != 0 )
	{
		print_log(DEBUG,1,"[%s] ssid %s -> %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_SSID], p_config);
		return true;
	}
	
	p_config = preset_level_2_node->first_attribute("key")->value();
	if( strcmp( m_p_config[PRESET_ID_SYSTEM][eCFG_KEY], p_config ) != 0 )
	{
		print_log(DEBUG,1,"[%s] key %s -> %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_KEY], p_config);
		return true;
	}
	
	p_config = preset_level_2_node->first_attribute("scan")->value();
	if( strcmp( m_p_config[PRESET_ID_SYSTEM][eCFG_SCAN], p_config ) != 0 )
	{
		print_log(DEBUG,1,"[%s] scan %s -> %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_SCAN], p_config);
		return true;
	}

	// AWD
	preset_level_1_node = preset_node->first_node("Powermode");
	preset_level_2_node = preset_level_1_node->first_node("AWD");
	
	if( preset_level_2_node != NULL )
	{
		p_config = preset_level_2_node->first_attribute("onoff")->value();
		if( strcmp( m_p_config[PRESET_ID_SYSTEM][eCFG_PM_AWD], p_config ) != 0 )
		{
			print_log(DEBUG,1,"[%s] AWD onoff %s -> %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_PM_AWD], p_config);
			return true;
		}
	}
	
	// AP
	preset_level_1_node = preset_node->first_node("Network");
	preset_level_2_node = preset_level_1_node->first_node("AP");
	
	p_config = preset_level_2_node->first_attribute("enable")->value();
	if( strcmp( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_ENABLE], p_config ) != 0 )
	{
		print_log(DEBUG,1,"[%s] AP enable %s -> %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_AP_ENABLE], p_config);
		return true;
	}
	
	if( strcmp( p_config, "on" ) == 0 )
	{
		p_config = preset_level_2_node->first_attribute("freq")->value();
	    if( strcmp( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_FREQ], p_config ) != 0 )
	    {
	    	print_log(DEBUG,1,"[%s] Frequency %s -> %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_AP_FREQ], p_config);
	    	return true;
	    }
	
	    p_config = preset_level_2_node->first_attribute("country")->value();
	    if( strcmp( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_COUNTRY], p_config ) != 0 )
	    {
	    	print_log(DEBUG,1,"[%s] Country %s -> %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_AP_COUNTRY], p_config);
	    	return true;
	    }

	    p_config = preset_level_2_node->first_attribute("country_code")->value();
	    if( strcmp( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_COUNTRY_CODE], p_config ) != 0 )
	    {
	    	print_log(DEBUG,1,"[%s] Country code %s -> %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_AP_COUNTRY_CODE], p_config);
	    	return true;
	    }
	    
	    p_config = preset_level_2_node->first_attribute("band")->value();
	    if( strcmp( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_BAND], p_config ) != 0 )
	    {
	    	print_log(DEBUG,1,"[%s] Band %s -> %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_AP_BAND], p_config);
	    	return true;
	    }
	    
	    p_config = preset_level_2_node->first_attribute("ssid")->value();
	    if( strcmp( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_SSID], p_config ) != 0 )
	    {
	    	print_log(DEBUG,1,"[%s] AP ssid %s -> %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_AP_SSID], p_config);
	    	return true;
	    }
		
		p_config = preset_level_2_node->first_attribute("key")->value();
	    if( strcmp( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_KEY], p_config ) != 0 )
	    {
	    	print_log(DEBUG,1,"[%s] AP key %s -> %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_AP_KEY], p_config);
	    	return true;
	    }
	
		p_config = preset_level_2_node->first_attribute("channel")->value();
	    if( strcmp( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_CHANNEL], p_config ) != 0 )
	    {
	    	print_log(DEBUG,1,"[%s] AP channel %s -> %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_AP_CHANNEL], p_config);
	    	return true;
	    }
		
		if( preset_level_2_node->first_attribute("dhcp") != NULL )
		{
			p_config = preset_level_2_node->first_attribute("dhcp")->value();
		    
		    if( strcmp( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_DHCP], p_config ) != 0 )
		    {
		    	print_log(DEBUG,1,"[%s] AP dhcp %s -> %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_AP_DHCP], p_config);
		    	return true;
		    }
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
DEV_ERROR CEnv::config_load_xml(const s8* i_p_file_name)
{
	xml_document<> doc;
	xml_node<> * root_node;
	bool b_init_preset_name = false;
	
	// Read the xml file into a vector
	ifstream theFile(i_p_file_name);
	
	if( !theFile.is_open() )
	{
		print_log(ERROR,1,"[%s] %s open failed\n", m_p_log_id, i_p_file_name);
		return eDEV_ERR_ENV_LOAD_FAILED;
	}
	
	vector<char> buffer((istreambuf_iterator<char>(theFile)), istreambuf_iterator<char>());
	buffer.push_back('\0');
	
	theFile.close();
	
	// Parse the buffer using the xml file parsing library into doc 
	doc.parse<0>(&buffer[0]);
	
	root_node = doc.first_node("Device");
	
	xml_node<> * node = root_node->first_node("Impact");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_IMACT_PEAK], node->first_attribute("peak")->value() );
	
	if( root_node->first_node("APBUTTON") == NULL )
	{
		strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_BUTTON_FUNC_TYPE], "0" );
	}
	else
	{	
		xml_node<> * node = root_node->first_node("APBUTTON");
		if( strncmp( node->first_attribute("type")->value(), "0", 1 ) == 0)
		{
			// Preset 이름 초기화 되도록 수정
			b_init_preset_name = true;
		}
		
		strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_BUTTON_FUNC_TYPE], node->first_attribute("type")->value() );
	}	
	
	node = root_node->first_node("ImageProcesses");
	if( node == NULL )
	{
		print_log(ERROR,1,"[%s] %s invalid format\n", m_p_log_id, i_p_file_name);
		return eDEV_ERR_ENV_LOAD_FAILED;
	}
	
	xml_node<> * level_1_node = node->first_node("LowerGridLine");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_DEFECT_LOWER_GRID], level_1_node->first_attribute("use")->value() );
	
	xml_node<> * preset_level_1_node;
	xml_node<> * preset_level_2_node;
	xml_node<> * preset_node = root_node->first_node("Preset");
		
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_TAG_ID], preset_node->first_attribute("tag")->value() );
	
	if( b_init_preset_name == true )
	{
		// Preset 이름 초기화 되도록 수정
		strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_PRESET_NAME], "System");
		print_log(DEBUG,1,"[%s] Preset name reset %s -> System\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_PRESET_NAME]);
	}
	else
	{
		strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_PRESET_NAME], preset_node->first_attribute("name")->value() );
	}
	
	preset_level_1_node = preset_node->first_node("Network");
	
	// LAN
	preset_level_2_node = preset_level_1_node->first_node("Lan");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_IP_ADDRESS], preset_level_2_node->first_attribute("ip")->value() );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_NETMASK], preset_level_2_node->first_attribute("netmask")->value() );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_GATEWAY], preset_level_2_node->first_attribute("gateway")->value() );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_POE_USE_ONLY_POWER], preset_level_2_node->first_attribute("use_only_power")->value() );
	
	// WLAN
	preset_level_2_node = preset_level_1_node->first_node("WLan");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_SSID], preset_level_2_node->first_attribute("ssid")->value() );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_KEY], preset_level_2_node->first_attribute("key")->value() );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_SCAN], preset_level_2_node->first_attribute("scan")->value() );
	
	// AP
	preset_level_2_node = preset_level_1_node->first_node("AP");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_ENABLE], preset_level_2_node->first_attribute("enable")->value() );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_FREQ], preset_level_2_node->first_attribute("freq")->value() );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_COUNTRY], preset_level_2_node->first_attribute("country")->value() );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_COUNTRY_CODE], preset_level_2_node->first_attribute("country_code")->value() );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_BAND], preset_level_2_node->first_attribute("band")->value() );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_SSID], preset_level_2_node->first_attribute("ssid")->value() );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_KEY], preset_level_2_node->first_attribute("key")->value() );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_CHANNEL], preset_level_2_node->first_attribute("channel")->value() );
	
	if( preset_level_2_node->first_attribute("dhcp") == NULL )
	{
		strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_DHCP], "on" );
	}
	else
	{
		strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_DHCP], preset_level_2_node->first_attribute("dhcp")->value() );
	}
	
	// power mode
	preset_level_1_node = preset_node->first_node("Powermode");
	
	preset_level_2_node = preset_level_1_node->first_node("Sleep");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_PM_SLEEP_ENABLE], preset_level_2_node->first_attribute("enable")->value() );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_PM_SLEEP_ENTRANCE_TIME], preset_level_2_node->first_attribute("time")->value() );
	
	preset_level_2_node = preset_level_1_node->first_node("Shutdown");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_PM_SHUTDOWN_ENABLE], preset_level_2_node->first_attribute("enable")->value() );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_PM_SHUTDOWN_ENTRANCE_TIME], preset_level_2_node->first_attribute("time")->value() );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_PM_POWER_SOURCE], preset_level_2_node->first_attribute("power_source")->value() );
	
	if( preset_level_1_node->first_node("AWD") == NULL )
	{
		strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_PM_AWD], "off" );
	}
	else
	{
		preset_level_2_node = preset_level_1_node->first_node("AWD");
		strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_PM_AWD], preset_level_2_node->first_attribute("onoff")->value() );
	}
	
	// image
	preset_level_1_node = preset_node->first_node("ImageOptions");
	preset_level_2_node = preset_level_1_node->first_node("Parameter");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_TIMEOUT], preset_level_2_node->first_attribute("timeout")->value() );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_PREVIEW], preset_level_2_node->first_attribute("preview")->value() );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_FAST_TACT], preset_level_2_node->first_attribute("fast_tact")->value() );
	
	if( preset_level_2_node->first_attribute("backup") == NULL )
	{
		strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_BACKUP], "enable" );
	}
	else
	{
		strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_BACKUP], preset_level_2_node->first_attribute("backup")->value() );
	}        

	preset_level_2_node = preset_level_1_node->first_node("PostProcess");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_DIRECTION], preset_level_2_node->first_attribute("direction")->value() );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_DIRECTION_USER_INPUT], preset_level_2_node->first_attribute("direction_user_input")->value() );
	
	
	preset_level_1_node = preset_node->first_node("AutoOffset");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_OFFSET_ENABLE], preset_level_1_node->first_attribute("enable")->value() );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_OFFSET_PERIOD], preset_level_1_node->first_attribute("interval")->value() );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_OFFSET_COUNT], preset_level_1_node->first_attribute("shot")->value() );
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_OFFSET_TEMPERATURE], preset_level_1_node->first_attribute("temp")->value() );
	
	preset_level_1_node = preset_node->first_node("Exposure");
	strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_EXP_TRIG_TYPE], preset_level_1_node->first_attribute("trigger")->value() );
	
	return eDEV_ERR_NO;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CEnv::preset_load(const s8* i_p_file_name)
{
	xml_document<> doc;
	xml_node<> * root_node;
	
	bool b_loaded = false;
	
	// Read the xml file into a vector
	ifstream theFile(i_p_file_name);
	
	if( !theFile.is_open() )
	{
		print_log(ERROR,1,"[%s] %s open failed\n", m_p_log_id, i_p_file_name);
		return b_loaded;
	}
	
	vector<char> buffer((istreambuf_iterator<char>(theFile)), istreambuf_iterator<char>());
	buffer.push_back('\0');
	
	theFile.close();
	
	// Parse the buffer using the xml file parsing library into doc 
	doc.parse<0>(&buffer[0]);
	
	root_node = doc.first_node("VPreset");
	
	int version = atoi(root_node->first_attribute("version")->value());
	if( version != 1 )
	{
		print_log(ERROR,1,"[%s] XML file version mismatch(1 -> %d)\n", m_p_log_id, version);
		return b_loaded;
	}
	
	u16 w_preset_id = 0;
	xml_node<> * preset_level_1_node;
	xml_node<> * preset_level_2_node;
	
	for (xml_node<> * preset_node = root_node->first_node("Preset"); preset_node; preset_node = preset_node->next_sibling())
	{
	    w_preset_id = atoi(preset_node->first_attribute("id")->value());
	    if( w_preset_id == PRESET_ID_SYSTEM
	    	|| w_preset_id >= MAX_CFG_COUNT )
	    {
			print_log(ERROR,1,"[%s] Preset ID(%d) is invalid.\n", m_p_log_id, w_preset_id);
			continue;
	    }

	    strcpy( m_p_config[w_preset_id][eCFG_TAG_ID], preset_node->first_attribute("tag")->value() );
	    strcpy( m_p_config[w_preset_id][eCFG_PRESET_NAME], preset_node->first_attribute("name")->value() );
	    
	    preset_level_1_node = preset_node->first_node("Network");
	    
	    // LAN
    	preset_level_2_node = preset_level_1_node->first_node("Lan");
	    strcpy( m_p_config[w_preset_id][eCFG_IP_ADDRESS], preset_level_2_node->first_attribute("ip")->value() );
	    strcpy( m_p_config[w_preset_id][eCFG_NETMASK], preset_level_2_node->first_attribute("netmask")->value() );
	    strcpy( m_p_config[w_preset_id][eCFG_GATEWAY], preset_level_2_node->first_attribute("gateway")->value() );
	    strcpy( m_p_config[w_preset_id][eCFG_POE_USE_ONLY_POWER], preset_level_2_node->first_attribute("use_only_power")->value() );
	    
	    // WLAN
	    preset_level_2_node = preset_level_1_node->first_node("WLan");
	    strcpy( m_p_config[w_preset_id][eCFG_SSID], preset_level_2_node->first_attribute("ssid")->value() );
	    strcpy( m_p_config[w_preset_id][eCFG_KEY], preset_level_2_node->first_attribute("key")->value() );
	    strcpy( m_p_config[w_preset_id][eCFG_SCAN], preset_level_2_node->first_attribute("scan")->value() );
	    
	    // power mode
	    preset_level_1_node = preset_node->first_node("Powermode");
	    
	    preset_level_2_node = preset_level_1_node->first_node("Sleep");
	    strcpy( m_p_config[w_preset_id][eCFG_PM_SLEEP_ENABLE], preset_level_2_node->first_attribute("enable")->value() );
	    strcpy( m_p_config[w_preset_id][eCFG_PM_SLEEP_ENTRANCE_TIME], preset_level_2_node->first_attribute("time")->value() );
	    
	    preset_level_2_node = preset_level_1_node->first_node("Shutdown");
	    strcpy( m_p_config[w_preset_id][eCFG_PM_SHUTDOWN_ENABLE], preset_level_2_node->first_attribute("enable")->value() );
	    strcpy( m_p_config[w_preset_id][eCFG_PM_SHUTDOWN_ENTRANCE_TIME], preset_level_2_node->first_attribute("time")->value() );
	    strcpy( m_p_config[w_preset_id][eCFG_PM_POWER_SOURCE], preset_level_2_node->first_attribute("power_source")->value() );
	    
	    // image
	    preset_level_1_node = preset_node->first_node("ImageOptions");
	    preset_level_2_node = preset_level_1_node->first_node("Parameter");
	    strcpy( m_p_config[w_preset_id][eCFG_IMAGE_TIMEOUT], preset_level_2_node->first_attribute("timeout")->value() );
	    strcpy( m_p_config[w_preset_id][eCFG_IMAGE_PREVIEW], preset_level_2_node->first_attribute("preview")->value() );
	    strcpy( m_p_config[w_preset_id][eCFG_IMAGE_FAST_TACT], preset_level_2_node->first_attribute("fast_tact")->value() );
	    
	    preset_level_2_node = preset_level_1_node->first_node("PostProcess");
	    strcpy( m_p_config[w_preset_id][eCFG_IMAGE_DIRECTION], preset_level_2_node->first_attribute("direction")->value() );
	    strcpy( m_p_config[w_preset_id][eCFG_IMAGE_DIRECTION_USER_INPUT], preset_level_2_node->first_attribute("direction_user_input")->value() );
	    
	    preset_level_1_node = preset_node->first_node("AutoOffset");
	    strcpy( m_p_config[w_preset_id][eCFG_OFFSET_ENABLE], preset_level_1_node->first_attribute("enable")->value() );
	    strcpy( m_p_config[w_preset_id][eCFG_OFFSET_PERIOD], preset_level_1_node->first_attribute("interval")->value() );
	    strcpy( m_p_config[w_preset_id][eCFG_OFFSET_COUNT], preset_level_1_node->first_attribute("shot")->value() );
	    strcpy( m_p_config[w_preset_id][eCFG_OFFSET_TEMPERATURE], preset_level_1_node->first_attribute("temp")->value() );
	    
	    preset_level_1_node = preset_node->first_node("Exposure");
	    strcpy( m_p_config[w_preset_id][eCFG_EXP_TRIG_TYPE], preset_level_1_node->first_attribute("trigger")->value() );
	    
	    b_loaded = true;
	}
	
	u16 w_id = 0;
	for(w_id = ++w_preset_id; w_id < MAX_CFG_COUNT; w_id++)
	{
		memset( m_p_config[w_id], 0, eCFG_MAX*STR_LEN);
	}
	
	return b_loaded;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
DEV_ERROR CEnv::config_load(const s8* i_p_file_name, bool is_reload)
{
	DEV_ERROR result = eDEV_ERR_NO;
	
	if(is_reload == false)
	{
		manufacture_info_read();
		oper_info_read();
		customized_info_read();
	}
	set_model();
	
	// set_model() 호출에 의해 model enum 값 설정되어야 effective area 및 battery count의 올바른 초기화 가능
	check_manufacture_effective_area();
	check_manufacture_battery_count();
	check_scintillator_type_and_update();
	
	print_log(DEBUG,1,"[%s] capture count: %d\n", m_p_log_id, m_oper_info_t.n_capture_count);
	version_read();
	
	config_default();
	
	result = config_load_xml(i_p_file_name);
	
	if( result != eDEV_ERR_NO )
	{
	    config_default();
	    config_default_save();
	}
	
	print_current();
	config_dhcp_update();
	
    return result;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
DEV_ERROR CEnv::config_load_factory(void)
{
    return eDEV_ERR_NO;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CEnv::config_default_save(void)
{
	print_log(INFO,1,"[%s] config_default_save\n", m_p_log_id);
	
    FILE* p_file;
    
    p_file = fopen( CONFIG_XML_FILE, "w" );
    if( p_file == NULL )
    {
        print_log(ERROR, 1, "[Env] %s file open error\n", CONFIG_XML_FILE);
        return;
    }
    
    fprintf( p_file, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
    fprintf( p_file, "<Device>\n");
    fprintf( p_file, "\t<Impact peak=\"100\"/>\n");
    fprintf( p_file, "\t<APBUTTON type=\"0\"/>\n");
    fprintf( p_file, "\t<ImageProcesses>\n");
    fprintf( p_file, "\t\t<ADC use=\"on\"/>\n");
    fprintf( p_file, "\t\t<ALDC use=\"on\"/>\n");
    fprintf( p_file, "\t\t<OSF use=\"off\"/>\n");
    fprintf( p_file, "\t\t<LLC use=\"off\"/>\n");
    fprintf( p_file, "\t\t<LowerGridLine use=\"%s\"/>\n", m_p_config[PRESET_ID_SYSTEM][eCFG_DEFECT_LOWER_GRID]);
	fprintf(p_file, "\t\t<EdgeMasking use=\"on\" value=\"%d\"/>\n", DEFAULT_DIGITAL_OFFSET);

	if(m_model_e == eMODEL_2530VW)
    {
    	fprintf( p_file, "\t\t<UserEffectiveArea top=\"0\" bottom=\"2559\" left=\"0\" right=\"2047\"/>\n");
    }
    else if(m_model_e == eMODEL_4343VW)
    {
    	fprintf( p_file, "\t\t<UserEffectiveArea top=\"0\" bottom=\"3071\" left=\"0\" right=\"3071\"/>\n");
    }
    else
    {
    	fprintf( p_file, "\t\t<UserEffectiveArea top=\"0\" bottom=\"3071\" left=\"0\" right=\"2559\"/>\n");
    }
    
    fprintf( p_file, "\t</ImageProcesses>\n");
    
    fprintf( p_file, "\t<Preset id=\"0\" name=\"System\" tag=\"0\">\n");
	fprintf( p_file, "\t\t<Network>\n");
	fprintf( p_file, "\t\t\t<Lan ip=\"%s\" netmask=\"%s\" gateway=\"%s\" use_only_power=\"%s\"/>\n", \
								m_p_config[PRESET_ID_SYSTEM][eCFG_IP_ADDRESS], m_p_config[PRESET_ID_SYSTEM][eCFG_NETMASK],
								m_p_config[PRESET_ID_SYSTEM][eCFG_GATEWAY], m_p_config[PRESET_ID_SYSTEM][eCFG_POE_USE_ONLY_POWER] );
	
	fprintf( p_file, "\t\t\t<WLan ssid=\"%s\" key=\"%s\" scan=\"%s\"/>\n", \
								m_p_config[PRESET_ID_SYSTEM][eCFG_SSID], m_p_config[PRESET_ID_SYSTEM][eCFG_KEY],
								m_p_config[PRESET_ID_SYSTEM][eCFG_SCAN] );
								
	fprintf( p_file, "\t\t\t<AP enable=\"%s\" freq=\"%s\" country=\"%s\" country_code=\"%s\" band=\"%s\" ssid=\"%s\" key=\"%s\" channel=\"%s\" dhcp=\"%s\"/>\n", \
                        		m_p_config[PRESET_ID_SYSTEM][eCFG_AP_ENABLE], m_p_config[PRESET_ID_SYSTEM][eCFG_AP_FREQ], \
                        		m_p_config[PRESET_ID_SYSTEM][eCFG_AP_COUNTRY], m_p_config[PRESET_ID_SYSTEM][eCFG_AP_COUNTRY_CODE], \
                        		m_p_config[PRESET_ID_SYSTEM][eCFG_AP_BAND], m_p_config[PRESET_ID_SYSTEM][eCFG_AP_SSID], \
                        		m_p_config[PRESET_ID_SYSTEM][eCFG_AP_KEY], m_p_config[PRESET_ID_SYSTEM][eCFG_AP_CHANNEL], \
                        		m_p_config[PRESET_ID_SYSTEM][eCFG_AP_DHCP]);
	fprintf( p_file, "\t\t</Network>\n");

	fprintf( p_file, "\t\t<Powermode>\n");
	fprintf( p_file, "\t\t\t<Sleep enable=\"%s\" time=\"%s\"/>\n", \
								m_p_config[PRESET_ID_SYSTEM][eCFG_PM_SLEEP_ENABLE], m_p_config[PRESET_ID_SYSTEM][eCFG_PM_SLEEP_ENTRANCE_TIME] );
	fprintf( p_file, "\t\t\t<Shutdown enable=\"%s\" time=\"%s\" power_source=\"%s\"/>\n", \
								m_p_config[PRESET_ID_SYSTEM][eCFG_PM_SHUTDOWN_ENABLE], m_p_config[PRESET_ID_SYSTEM][eCFG_PM_SHUTDOWN_ENTRANCE_TIME], \
								m_p_config[PRESET_ID_SYSTEM][eCFG_PM_POWER_SOURCE]);
	fprintf( p_file, "\t\t\t\t\t<AWD	onoff=\"%s\"/>\n", \
								m_p_config[PRESET_ID_SYSTEM][eCFG_PM_AWD]);
	fprintf( p_file, "\t\t</Powermode>\n");
	
	fprintf( p_file, "\t\t<ImageOptions>\n");
	fprintf( p_file, "\t\t\t<Parameter timeout=\"%s\" preview=\"%s\" fast_tact=\"%s\" backup=\"%s\"/>\n", \
								m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_TIMEOUT],
								m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_PREVIEW], m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_FAST_TACT], \
								m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_BACKUP]);
								
	fprintf( p_file, "\t\t\t<PostProcess direction=\"%s\" direction_user_input=\"%s\"/>\n", \
								m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_DIRECTION], m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_DIRECTION_USER_INPUT] );
	fprintf( p_file, "\t\t</ImageOptions>\n");
	
	fprintf( p_file, "\t\t<AutoOffset enable=\"%s\" interval=\"%s\" temp=\"%s\" shot=\"%s\"/>\n", \
                        		m_p_config[PRESET_ID_SYSTEM][eCFG_OFFSET_ENABLE], 
                        		m_p_config[PRESET_ID_SYSTEM][eCFG_OFFSET_PERIOD], m_p_config[PRESET_ID_SYSTEM][eCFG_OFFSET_TEMPERATURE],
                        		m_p_config[PRESET_ID_SYSTEM][eCFG_OFFSET_COUNT] );

	fprintf( p_file, "\t\t<Exposure trigger=\"%s\"/>\n", m_p_config[PRESET_ID_SYSTEM][eCFG_EXP_TRIG_TYPE] );
	
	fprintf( p_file, "\t</Preset>\n");
	fprintf( p_file, "</Device>\n");    
    fclose( p_file );
    
    file_copy_to_flash( CONFIG_XML_FILE, SYS_CONFIG_PATH );
    
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
DEV_ERROR CEnv::config_save(const s8* i_p_file_name)
{
	print_log(INFO,1,"[%s] config_save\n", m_p_log_id);
	
	xml_document<> doc;
	xml_node<> * root_node;
	
	// Read the xml file into a vector
	ifstream theFile(i_p_file_name);
	
	if( !theFile.is_open() )
	{
		print_log(ERROR,1,"[%s] %s open failed\n", m_p_log_id, i_p_file_name);
		return eDEV_ERR_ENV_SAVE_FILE_ERROR;
	}
	
	vector<char> buffer((istreambuf_iterator<char>(theFile)), istreambuf_iterator<char>());
	buffer.push_back('\0');
	
	theFile.close();

	// Parse the buffer using the xml file parsing library into doc 
	doc.parse<0>(&buffer[0]);
	
	root_node = doc.first_node("Device");

	xml_node<> * node = root_node->first_node("Impact");
	node->first_attribute("peak")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_IMACT_PEAK], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_IMACT_PEAK]) );

	if( root_node->first_node("APBUTTON") == NULL )
	{	
		char *node_name = doc.allocate_string("APBUTTON");
		xml_node<> *node = doc.allocate_node(node_element, node_name);
		
		root_node->append_node(node);
		node = root_node->first_node("APBUTTON");
		xml_attribute<> *attr = doc.allocate_attribute("type", m_p_config[PRESET_ID_SYSTEM][eCFG_AP_BUTTON_FUNC_TYPE]);
		node->append_attribute(attr);
	}
	else
	{
		node = root_node->first_node("APBUTTON");
		node->first_attribute("type")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_BUTTON_FUNC_TYPE], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_AP_BUTTON_FUNC_TYPE]) );			
	}				

	node = root_node->first_node("ImageProcesses");
	xml_node<> * level_1_node;
	
	level_1_node = node->first_node("LowerGridLine");
	level_1_node->first_attribute("use")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_DEFECT_LOWER_GRID], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_DEFECT_LOWER_GRID]) );
	
	xml_node<> * preset_level_1_node;
	xml_node<> * preset_level_2_node;
	xml_node<> * preset_node = root_node->first_node("Preset");
	
	// tag
	preset_node->first_attribute("tag")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_TAG_ID], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_TAG_ID]) );
	preset_node->first_attribute("name")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_PRESET_NAME], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_PRESET_NAME]) );
	
	// network
	preset_level_1_node = preset_node->first_node("Network");
	
	// LAN
	preset_level_2_node = preset_level_1_node->first_node("Lan");
	preset_level_2_node->first_attribute("ip")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_IP_ADDRESS], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_IP_ADDRESS]) );
	preset_level_2_node->first_attribute("netmask")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_NETMASK], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_NETMASK]) );
	preset_level_2_node->first_attribute("gateway")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_GATEWAY], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_GATEWAY]) );
	preset_level_2_node->first_attribute("use_only_power")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_POE_USE_ONLY_POWER], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_POE_USE_ONLY_POWER]) );
	
	// WLAN
	preset_level_2_node = preset_level_1_node->first_node("WLan");
	preset_level_2_node->first_attribute("ssid")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_SSID], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_SSID]) );
	preset_level_2_node->first_attribute("key")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_KEY], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_KEY]) );
	preset_level_2_node->first_attribute("scan")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_SCAN], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_SCAN]) );
	
	// AP
	preset_level_2_node = preset_level_1_node->first_node("AP");
	preset_level_2_node->first_attribute("enable")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_ENABLE], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_AP_ENABLE]) );
	preset_level_2_node->first_attribute("freq")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_FREQ], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_AP_FREQ]) );
	preset_level_2_node->first_attribute("country")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_COUNTRY], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_AP_COUNTRY]) );
	preset_level_2_node->first_attribute("country_code")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_COUNTRY_CODE], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_AP_COUNTRY_CODE]) );
	preset_level_2_node->first_attribute("band")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_BAND], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_AP_BAND]) );
	preset_level_2_node->first_attribute("ssid")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_SSID], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_AP_SSID]) );
	preset_level_2_node->first_attribute("key")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_KEY], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_AP_KEY]) );
	preset_level_2_node->first_attribute("channel")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_CHANNEL], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_AP_CHANNEL]) );
	
	if( preset_level_2_node->first_attribute("dhcp") == NULL )
	{
		xml_attribute<> *attr = doc.allocate_attribute("dhcp", m_p_config[PRESET_ID_SYSTEM][eCFG_AP_DHCP]);
		preset_level_2_node->append_attribute(attr);
	}
	else
	{
		preset_level_2_node->first_attribute("dhcp")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_AP_DHCP], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_AP_DHCP]) );
	}
		
	// power mode
	preset_level_1_node = preset_node->first_node("Powermode");
	
	preset_level_2_node = preset_level_1_node->first_node("Sleep");
	preset_level_2_node->first_attribute("enable")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_PM_SLEEP_ENABLE], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_PM_SLEEP_ENABLE]) );
	preset_level_2_node->first_attribute("time")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_PM_SLEEP_ENTRANCE_TIME], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_PM_SLEEP_ENTRANCE_TIME]) );
	
	preset_level_2_node = preset_level_1_node->first_node("Shutdown");
	preset_level_2_node->first_attribute("enable")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_PM_SHUTDOWN_ENABLE], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_PM_SHUTDOWN_ENABLE]) );
	preset_level_2_node->first_attribute("time")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_PM_SHUTDOWN_ENTRANCE_TIME], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_PM_SHUTDOWN_ENTRANCE_TIME]) );
	preset_level_2_node->first_attribute("power_source")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_PM_POWER_SOURCE], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_PM_POWER_SOURCE]) );
	
	if( preset_level_1_node->first_node("AWD") == NULL )
	{
		char *node_name = doc.allocate_string("AWD");
		xml_node<> *node = doc.allocate_node(node_element, node_name);
		
		preset_level_1_node->append_node(node);
		preset_level_2_node = preset_level_1_node->first_node("AWD");
		xml_attribute<> *attr = doc.allocate_attribute("onoff", m_p_config[PRESET_ID_SYSTEM][eCFG_PM_AWD]);
		preset_level_2_node->append_attribute(attr);
	}
	else
	{
		preset_level_2_node = preset_level_1_node->first_node("AWD");
		preset_level_2_node->first_attribute("onoff")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_PM_AWD], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_PM_AWD]) );
	}
	
	// image
	preset_level_1_node = preset_node->first_node("ImageOptions");
	preset_level_2_node = preset_level_1_node->first_node("Parameter");
	preset_level_2_node->first_attribute("timeout")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_TIMEOUT], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_TIMEOUT]) );
	preset_level_2_node->first_attribute("preview")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_PREVIEW], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_PREVIEW]) );
	preset_level_2_node->first_attribute("fast_tact")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_FAST_TACT], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_FAST_TACT]) );
	
	if( preset_level_2_node->first_attribute("backup") == NULL )
	{
		xml_attribute<> *attr = doc.allocate_attribute("backup", m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_BACKUP]);
		preset_level_2_node->append_attribute(attr);
	}
	else
	{
		preset_level_2_node->first_attribute("backup")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_BACKUP], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_BACKUP]) );
	}
	
	preset_level_2_node = preset_level_1_node->first_node("PostProcess");
	preset_level_2_node->first_attribute("direction")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_DIRECTION], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_DIRECTION]) );
	preset_level_2_node->first_attribute("direction_user_input")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_DIRECTION_USER_INPUT], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_DIRECTION_USER_INPUT]) );
	
	preset_level_1_node = preset_node->first_node("AutoOffset");
	preset_level_1_node->first_attribute("enable")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_OFFSET_ENABLE], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_OFFSET_ENABLE]) );
	preset_level_1_node->first_attribute("interval")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_OFFSET_PERIOD], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_OFFSET_PERIOD]) );
	preset_level_1_node->first_attribute("shot")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_OFFSET_COUNT], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_OFFSET_COUNT]) );
	preset_level_1_node->first_attribute("temp")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_OFFSET_TEMPERATURE], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_OFFSET_TEMPERATURE]) );
	
	preset_level_1_node = preset_node->first_node("Exposure");
	preset_level_1_node->first_attribute("trigger")->value( m_p_config[PRESET_ID_SYSTEM][eCFG_EXP_TRIG_TYPE], strlen(m_p_config[PRESET_ID_SYSTEM][eCFG_EXP_TRIG_TYPE]) );
	
	ofstream save_file(i_p_file_name);
	save_file << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
	save_file << doc;
	save_file.close();
	
    file_copy_to_flash( i_p_file_name, SYS_CONFIG_PATH );
    
    return eDEV_ERR_NO;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
s8* CEnv::config_get( CONFIG i_cfg_e )
{
	return m_p_config[PRESET_ID_SYSTEM][i_cfg_e];
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CEnv::config_set( CONFIG i_cfg_e, const s8* i_p_value )
{
    if( i_p_value == NULL )
    {
        print_log(ERROR, 1, "[%s] cfg(0x%02X) value is invalid(null).\n", m_p_log_id, i_cfg_e);
        return;
    }
    else if( i_cfg_e >= eCFG_MAX )
    {
        print_log(ERROR, 1, "[%s] cfg index(0x%02X) is invalid.\n", m_p_log_id, i_cfg_e);
        return;
    }
    else if( strlen(i_p_value) >= STR_LEN )
    {
        print_log(ERROR, 1, "[%s] cfg(0x%02X) value is too long(<%d).\n", m_p_log_id, i_cfg_e, STR_LEN);
        return;
    }
    
	strcpy( m_p_config[PRESET_ID_SYSTEM][i_cfg_e], i_p_value );
    
    if(i_cfg_e == eCFG_VERSION_PACKAGE)
    {
    	s8	p_cmd[256]={0,};
    	/* 1. Make version file */
    	sprintf(p_cmd,"echo \"%s\" > %s",i_p_value,TEMPORARY_VER_PATH);    	
    	system(p_cmd);
    	
    	/* 2. mmc clear */
    	sprintf(p_cmd,"dd if=/dev/zero of=/dev/mmcblk%s seek=128 count=1;sync",SYS_MOUNT_MMCBLK_NUM);
    	system(p_cmd);
    	
    	/* 3. write version to mmc */
    	sprintf(p_cmd,"dd if=%s of=/dev/mmcblk%s seek=65536 bs=1 count=%d;sync",TEMPORARY_VER_PATH,SYS_MOUNT_MMCBLK_NUM,strlen(i_p_value));    	
    	system(p_cmd);
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CEnv::config_dhcp_update(void)
{
	FILE*	p_dhcp_config_file = fopen(DHCP_CONFIG_FILE, "w");
	struct sockaddr_in addr_in_t;
	u32     n_ip, n_start_ip, n_end_ip;
	struct in_addr addr_t;
	
	inet_aton( config_get(eCFG_IP_ADDRESS), &addr_in_t.sin_addr);
    n_ip = addr_in_t.sin_addr.s_addr;
    n_ip = ntohl(n_ip);
    
    if( (n_ip & 0x000000FF) > 0xF0 ) // 0xF0 = 240
    {
    	n_start_ip	= (n_ip & 0xFFFFFF00) | 0xE6;	// 0xE6 = 230
    	n_end_ip	= (n_ip & 0xFFFFFF00) | 0xEF;	// 0xEF = 239
    }
    else
    {
     	n_start_ip	= (n_ip & 0xFFFFFF00) | 0xF1;	// 0xF1 = 241
    	n_end_ip	= (n_ip & 0xFFFFFF00) | 0xFA;	// 0xFA = 250
    }
    
    n_start_ip	= htonl(n_start_ip);
    n_end_ip	= htonl(n_end_ip);
    
    addr_t.s_addr = n_start_ip;
	fprintf( p_dhcp_config_file, "start %s\n", inet_ntoa( addr_t ) );
	
	addr_t.s_addr = n_end_ip;
	fprintf( p_dhcp_config_file, "end %s\n", inet_ntoa( addr_t ) );
	
	fprintf( p_dhcp_config_file, "interface br-lan\n" );
	fprintf( p_dhcp_config_file, "max_leases 10\n" );
	fprintf( p_dhcp_config_file, "option  subnet  255.255.255.0\n");
	
	fclose( p_dhcp_config_file );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CEnv::get_mac( u8* o_p_mac )
{
    memcpy( o_p_mac, m_manufacture_info_t.p_mac_address, sizeof(m_manufacture_info_t.p_mac_address) );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CEnv::set_mac( u8* i_p_mac )
{
    print_log(DEBUG, 1, "[%s] set mac addr: %02X:%02X:%02X:%02X:%02X:%02X\n", \
                                m_p_log_id, i_p_mac[0], i_p_mac[1], i_p_mac[2], \
                                i_p_mac[3], i_p_mac[4], i_p_mac[5] );
    
    memcpy( m_manufacture_info_t.p_mac_address, i_p_mac, sizeof(m_manufacture_info_t.p_mac_address) );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       4byte ip, network byte order
* \note
*******************************************************************************/
u32 CEnv::get_ip(void)
{
    struct in_addr addr;
    
    inet_aton(m_p_config[PRESET_ID_SYSTEM][eCFG_IP_ADDRESS], &addr);
    
    return (u32)addr.s_addr;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       4byte subnet, network byte order
* \note
*******************************************************************************/
u32 CEnv::get_subnet(void)
{
    struct in_addr addr;
    
    inet_aton(m_p_config[PRESET_ID_SYSTEM][eCFG_NETMASK], &addr);
    
    return (u32)addr.s_addr;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
const s8* CEnv::get_model_name(void)
{
    return m_p_config[0][eCFG_MODEL];
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
const s8* CEnv::get_device_version(void)
{
    return m_p_config[0][eCFG_VERSION_FW];
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
const s8* CEnv::get_serial(void)
{
    return m_p_config[0][eCFG_SERIAL];
}

/******************************************************************************/
/**
* \brief        실제 설치될 배터리 수
* \param        none
* \return       none
* \note
*******************************************************************************/
u16 CEnv::get_battery_count(void)
{
	return m_manufacture_info_t.w_battery_count_to_install;
}

/******************************************************************************/
/**
* \brief        모델 별 장착 가능한 최대 배터리 수
* \param        none
* \return       none
* \note
*******************************************************************************/
u16 CEnv::get_battery_slot_count(void)
{
	if( m_model_e == eMODEL_2530VW )
    {
        return 1;
    }
    else
    {
    	return 2;
    }
}
	
/******************************************************************************/
/**
* \brief        모델 별 장착 가능한 최대 배터리 수
* \param        none
* \return       none
* \note
*******************************************************************************/
void CEnv::check_manufacture_battery_count(void)
{		
	// Battery 장착 개수가 slot 개수보다 크거나 0인 경우 비정상적으로 저장된 데이터로 간주하고 slot 개수로 초기화
	if(m_manufacture_info_t.w_battery_count_to_install > get_battery_slot_count() 
		|| m_manufacture_info_t.w_battery_count_to_install == 0)
	{
		print_log(DEBUG, 1, "[%s] Invalid battery count : %d (init battery count : %d)\n", m_p_log_id, m_manufacture_info_t.w_battery_count_to_install, get_battery_slot_count());
		m_manufacture_info_t.w_battery_count_to_install = get_battery_slot_count();
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CEnv::print_xml(void)
{
	xml_document<> doc;
	xml_node<> * root_node;
	
	// Read the xml file into a vector
	ifstream theFile(PRESET_XML_FILE);
	vector<char> buffer((istreambuf_iterator<char>(theFile)), istreambuf_iterator<char>());
	buffer.push_back('\0');
	// Parse the buffer using the xml file parsing library into doc 
	doc.parse<0>(&buffer[0]);
	
	root_node = doc.first_node("VPreset");
	
	printf("version: %s\n", root_node->first_attribute("version")->value());
	
	xml_node<> * current_preset_node = root_node->first_node("CurrentPreset");
	printf("CurrentPreset id: %s\n", current_preset_node->first_attribute("id")->value());
	
	xml_node<> * preset_level_1_node;
	xml_node<> * preset_level_2_node;
	for (xml_node<> * preset_node = root_node->first_node("Preset"); preset_node; preset_node = preset_node->next_sibling())
	{
		printf("Preset ID: %s, Name: %s, Tag: %s\n", 
	    	preset_node->first_attribute("id")->value(),
	    	preset_node->first_attribute("name")->value(),
	    	preset_node->first_attribute("tag")->value());
	    
	    
	    printf("Network\n");
	    preset_level_1_node = preset_node->first_node("Network");
	    preset_level_2_node = preset_level_1_node->first_node("Lan");
	    printf("\tLAN IP: %s, Netmask: %s, Gateway: %s, Use only power: %s\n", 
		   		preset_level_2_node->first_attribute("ip")->value(),
		   		preset_level_2_node->first_attribute("netmask")->value(),
		   		preset_level_2_node->first_attribute("gateway")->value(),
		   		preset_level_2_node->first_attribute("use_only_power")->value());

	    preset_level_2_node = preset_level_1_node->first_node("WLan");
	    printf("\tWLAN SSID: %s, Key: %s, Scan: %s\n", 
		   		preset_level_2_node->first_attribute("ssid")->value(),
		   		preset_level_2_node->first_attribute("key")->value(),
		   		preset_level_2_node->first_attribute("scan")->value());

	    preset_level_2_node = preset_level_1_node->first_node("AP");
	    printf("\tAP enable: %s, freq: %s, country: %s, band: %s, ssid: %s, key: %s, channel: %s\n", 
		   		preset_level_2_node->first_attribute("enable")->value(),
		   		preset_level_2_node->first_attribute("freq")->value(),
		   		preset_level_2_node->first_attribute("country")->value(),
		   		preset_level_2_node->first_attribute("band")->value(),
		   		preset_level_2_node->first_attribute("ssid")->value(),
		   		preset_level_2_node->first_attribute("key")->value(),
		   		preset_level_2_node->first_attribute("channel")->value());
		
		
	    printf("Powermode\n");
	    preset_level_1_node = preset_node->first_node("Powermode");
	    preset_level_2_node = preset_level_1_node->first_node("Sleep");
	    printf("\tSleep enable: %s, time: %s\n", 
		   		preset_level_2_node->first_attribute("enable")->value(),
		   		preset_level_2_node->first_attribute("time")->value());

	    preset_level_2_node = preset_level_1_node->first_node("Shutdown");
	    printf("\tShutdown enable: %s, time: %s, power_source: %s\n", 
		   		preset_level_2_node->first_attribute("enable")->value(),
		   		preset_level_2_node->first_attribute("time")->value(),
		   		preset_level_2_node->first_attribute("power_source")->value());
		   		
		
		printf("ImageOptions\n");
	    preset_level_1_node = preset_node->first_node("ImageOptions");
	    preset_level_2_node = preset_level_1_node->first_node("Parameter");
	    printf("\tParameter digitalOffset: %s, gainType: %s, timeout: %s, testPattern: %s, preview: %s\n", 
		   		preset_level_2_node->first_attribute("digitalOffset")->value(),
		   		preset_level_2_node->first_attribute("gainType")->value(),
		   		preset_level_2_node->first_attribute("timeout")->value(),
		   		preset_level_2_node->first_attribute("testPattern")->value(),
		   		preset_level_2_node->first_attribute("preview")->value());

	    preset_level_2_node = preset_level_1_node->first_node("PostProcess");
	    printf("\tPostProcess direction: %s\n", 
		   		preset_level_2_node->first_attribute("direction")->value());

		preset_level_1_node = preset_node->first_node("AutoOffset");
	    printf("AutoOffset enable: %s, interval: %s, temp: %s, shot: %s, aed_capture: %s\n", 
		   		preset_level_1_node->first_attribute("enable")->value(),
		   		preset_level_1_node->first_attribute("interval")->value(),
		   		preset_level_1_node->first_attribute("temp")->value(),
		   		preset_level_1_node->first_attribute("shot")->value(),
		   		preset_level_1_node->first_attribute("aed_capture")->value());

		
		printf("Device\n");
	    preset_level_1_node = preset_node->first_node("Device");
	    preset_level_2_node = preset_level_1_node->first_node("Impact");
	    printf("\tImpact peak: %s\n", 
		   		preset_level_2_node->first_attribute("peak")->value());

	    preset_level_2_node = preset_level_1_node->first_node("Exposure");
	    printf("\tExposure trigger: %s, exposureTime: %s, preExposure: %s, EXP_OK_Delay: %s, debounceAED: %s, debounceDR: %s\n", 
		   		preset_level_2_node->first_attribute("trigger")->value(),
		   		preset_level_2_node->first_attribute("exposureTime")->value(),
		   		preset_level_2_node->first_attribute("preExposure")->value(),
		   		preset_level_2_node->first_attribute("EXP_OK_Delay")->value(),
		   		preset_level_2_node->first_attribute("debounceAED")->value(),
		   		preset_level_2_node->first_attribute("debounceDR")->value());
	}
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CEnv::print_current(void)
{
	print_log(DEBUG,1,"[%s] Preset name: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_PRESET_NAME] );
    
	print_log(DEBUG,1,"[%s] ip: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_IP_ADDRESS] );
	print_log(DEBUG,1,"[%s] netmask: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_NETMASK] );
	print_log(DEBUG,1,"[%s] gateway: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_GATEWAY] );
	print_log(DEBUG,1,"[%s] use_only_power: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_POE_USE_ONLY_POWER] );
	
	print_log(DEBUG,1,"[%s] ssid: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_SSID] );
	print_log(DEBUG,1,"[%s] key: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_KEY] );
	print_log(DEBUG,1,"[%s] scan: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_SCAN] );
	
	print_log(DEBUG,1,"[%s] ap_onoff: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_AP_ENABLE] );
	print_log(DEBUG,1,"[%s] freq: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_AP_FREQ] );
	print_log(DEBUG,1,"[%s] country: %s\n",m_p_log_id,  m_p_config[PRESET_ID_SYSTEM][eCFG_AP_COUNTRY] );
	print_log(DEBUG,1,"[%s] country code: %s\n",m_p_log_id,  m_p_config[PRESET_ID_SYSTEM][eCFG_AP_COUNTRY_CODE] );
	print_log(DEBUG,1,"[%s] band: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_AP_BAND] );
	print_log(DEBUG,1,"[%s] ssid: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_AP_SSID] );
	print_log(DEBUG,1,"[%s] key: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_AP_KEY] );
	print_log(DEBUG,1,"[%s] channel: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_AP_CHANNEL] );
	print_log(DEBUG,1,"[%s] dhcp: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_AP_DHCP] );
	
	print_log(DEBUG,1,"[%s] image timeout: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_TIMEOUT] );
	print_log(DEBUG,1,"[%s] preview: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_PREVIEW] );
	print_log(DEBUG,1,"[%s] fast_tact: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_FAST_TACT] );
	print_log(DEBUG,1,"[%s] backup: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_BACKUP] );
	print_log(DEBUG,1,"[%s] direction: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_DIRECTION] );
	print_log(DEBUG,1,"[%s] direction user input: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_DIRECTION_USER_INPUT] );

	print_log(DEBUG,1,"[%s] sleep onoff: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_PM_SLEEP_ENABLE] );
	print_log(DEBUG,1,"[%s] sleep time: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_PM_SLEEP_ENTRANCE_TIME] );
	print_log(DEBUG,1,"[%s] shutdown onoff: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_PM_SHUTDOWN_ENABLE] );
	print_log(DEBUG,1,"[%s] shutdown time: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_PM_SHUTDOWN_ENTRANCE_TIME] );
	print_log(DEBUG,1,"[%s] power_source: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_PM_POWER_SOURCE] );
	print_log(DEBUG,1,"[%s] always wired detection: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_PM_AWD] );
	
	print_log(DEBUG,1,"[%s] peak: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_IMACT_PEAK] );
	print_log(DEBUG,1,"[%s] defect lower grid line support: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_DEFECT_LOWER_GRID] );
	
	print_log(DEBUG,1,"[%s] auto offset onoff: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_OFFSET_ENABLE] );
	print_log(DEBUG,1,"[%s] auto offset period: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_OFFSET_PERIOD] );
	print_log(DEBUG,1,"[%s] auto offset count: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_OFFSET_COUNT] );
	print_log(DEBUG,1,"[%s] auto offset temperature: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_OFFSET_TEMPERATURE] );

	print_log(DEBUG,1,"[%s] exp trigger type: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_EXP_TRIG_TYPE] );
	print_log(DEBUG,1,"[%s] AP button type: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_AP_BUTTON_FUNC_TYPE] );
	
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CEnv::print(void)
{
	print_current();
}

/******************************************************************************/
/**
 * @brief   어느 network interface를 사용하여야 하는지 알려준다.
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CEnv::net_is_ap_enable(void)
{
    if( strcmp(m_p_config[PRESET_ID_SYSTEM][eCFG_AP_ENABLE], "on") == 0 )
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
bool CEnv::net_is_dhcp_enable(void)
{
    if( strcmp(m_p_config[PRESET_ID_SYSTEM][eCFG_AP_DHCP], "on") == 0 )
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
bool CEnv::net_use_only_power(void)
{
    if( strcmp(m_p_config[PRESET_ID_SYSTEM][eCFG_POE_USE_ONLY_POWER], "on") == 0 )
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
void CEnv::manufacture_info_read(void)
{
	if (is_vw_revision_board())	micom_eeprom_read( EEPROM_ADDR_ENV, (u8*)&m_manufacture_info_t, sizeof(manufacture_info_t), eMICOM_EEPROM_BLK_FRAM );
    else						eeprom_read( EEPROM_ADDR_ENV, (u8*)&m_manufacture_info_t, sizeof(manufacture_info_t) );
		
	if( m_manufacture_info_t.n_magic_number == EEPROM_MAGIC_NUMBER )
	{
		strcpy( m_p_config[0][eCFG_MODEL], m_manufacture_info_t.p_model_name );
		strcpy( m_p_config[0][eCFG_SERIAL], m_manufacture_info_t.p_serial_number );
		
		print_log(DEBUG, 1, "[%s] name: %s\n", m_p_log_id, m_p_config[0][eCFG_MODEL]);
		print_log(DEBUG, 1, "[%s] serial: %s\n", m_p_log_id,  m_p_config[0][eCFG_SERIAL]);
		
		if ( m_manufacture_info_t.n_magic_number_v2 == EEPROM_MAGIC_NUMBER_V2 )
		{
			print_log(DEBUG, 1, "[%s] Scintillator type: %d\n", m_p_log_id,  m_manufacture_info_t.n_scintillator_type);
			print_log(DEBUG, 1, "[%s] Battery count to install: %d\n", m_p_log_id, m_manufacture_info_t.w_battery_count_to_install);
		}
		else
		{
			m_manufacture_info_t.n_scintillator_type = 0;
			m_manufacture_info_t.w_battery_count_to_install = get_battery_slot_count();
			print_log(DEBUG, 1, "[%s] Scintillator type and battery count is intialized.\n", m_p_log_id );
			
			//manufacture_info_write();
		}
	}
	else
	{
		print_log(ERROR, 1, "[%s] n_magic_number: %d\n", m_p_log_id, m_manufacture_info_t.n_magic_number);
		print_log(ERROR, 1, "[%s] name: %s\n", m_p_log_id, m_manufacture_info_t.p_model_name);
		print_log(ERROR, 1, "[%s] serial: %s\n", m_p_log_id, m_manufacture_info_t.p_serial_number);
		
	    strcpy( m_p_config[0][eCFG_MODEL], "FXRD-3643VAW");
	    strcpy( m_p_config[0][eCFG_SERIAL], "None");
	    
	    manufacture_info_default();
	    
	    print_log(ERROR, 1, "[%s] default manufacture info\n", m_p_log_id);
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CEnv::manufacture_info_write(void)
{
    s8 p_mac[32];
	
	strcpy( m_p_config[0][eCFG_MODEL], m_manufacture_info_t.p_model_name );
	strcpy( m_p_config[0][eCFG_SERIAL], m_manufacture_info_t.p_serial_number );
    
    //print_log(DEBUG, 1, "[%s] Scintillator type set : %d\n", m_p_log_id, m_manufacture_info_t.n_scintillator_type);
    
    m_manufacture_info_t.n_magic_number = EEPROM_MAGIC_NUMBER;
    m_manufacture_info_t.n_magic_number_v2 = EEPROM_MAGIC_NUMBER_V2;
    
	if (is_vw_revision_board())	micom_eeprom_write( EEPROM_ADDR_ENV, (u8*)&m_manufacture_info_t, sizeof(manufacture_info_t), eMICOM_EEPROM_BLK_FRAM );
	else						eeprom_write( EEPROM_ADDR_ENV, (u8*)&m_manufacture_info_t, sizeof(manufacture_info_t) );

    sprintf( p_mac, "%02X:%02X:%02X:%02X:%02X:%02X", \
                m_manufacture_info_t.p_mac_address[0], m_manufacture_info_t.p_mac_address[1], \
                m_manufacture_info_t.p_mac_address[2], m_manufacture_info_t.p_mac_address[3], \
                m_manufacture_info_t.p_mac_address[4], m_manufacture_info_t.p_mac_address[5] );
    
    sys_command("fw_setenv ethaddr %s", p_mac);
    sys_command("sync");
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CEnv::manufacture_info_default(void)
{
    memset( &m_manufacture_info_t, 0, sizeof(manufacture_info_t) );
	
	strcpy( m_manufacture_info_t.p_model_name, m_p_config[0][eCFG_MODEL] );
	strcpy( m_manufacture_info_t.p_serial_number, m_p_config[0][eCFG_SERIAL] );
	strcpy( m_manufacture_info_t.p_package_info, "None" );
	
	m_model_e = eMODEL_3643VW;  
	
	effective_area_default();
	
	m_manufacture_info_t.p_mac_address[0] = 0x84;
	m_manufacture_info_t.p_mac_address[1] = 0xEA;
	m_manufacture_info_t.p_mac_address[2] = 0x99;   
	
	m_manufacture_info_t.n_scintillator_type = 0;
	m_manufacture_info_t.w_battery_count_to_install = get_battery_slot_count();
  
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CEnv::effective_area_default(void)
{
	if( m_model_e == eMODEL_2530VW )
    {        
        m_manufacture_info_t.p_effective_area[eEFFECTIVE_AREA_TOP]      = 12;
        m_manufacture_info_t.p_effective_area[eEFFECTIVE_AREA_LEFT]     = 12;
        m_manufacture_info_t.p_effective_area[eEFFECTIVE_AREA_RIGHT]    = 2035;
        m_manufacture_info_t.p_effective_area[eEFFECTIVE_AREA_BOTTOM]   = 2547;
    }
    else if( m_model_e == eMODEL_4343VW)
    {      
        m_manufacture_info_t.p_effective_area[eEFFECTIVE_AREA_TOP]      = 12;
        m_manufacture_info_t.p_effective_area[eEFFECTIVE_AREA_LEFT]     = 12;
        m_manufacture_info_t.p_effective_area[eEFFECTIVE_AREA_RIGHT]    = 3059;
        m_manufacture_info_t.p_effective_area[eEFFECTIVE_AREA_BOTTOM]   = 3059;
    }
    else /* m_model_e == eMODEL_3643VW */
    { 
        m_manufacture_info_t.p_effective_area[eEFFECTIVE_AREA_TOP]      = 12;
        m_manufacture_info_t.p_effective_area[eEFFECTIVE_AREA_LEFT]     = 12;
        m_manufacture_info_t.p_effective_area[eEFFECTIVE_AREA_RIGHT]    = 2547;
        m_manufacture_info_t.p_effective_area[eEFFECTIVE_AREA_BOTTOM]   = 3059;
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CEnv::check_manufacture_effective_area(void)
{
	bool b_invalid_effective_area = false;
	
	if( m_manufacture_info_t.n_magic_number != EEPROM_MAGIC_NUMBER )
	{    
		b_invalid_effective_area = true;
	}
	else
	{
		if( m_manufacture_info_t.p_effective_area[eEFFECTIVE_AREA_RIGHT] == 0 
			|| m_manufacture_info_t.p_effective_area[eEFFECTIVE_AREA_BOTTOM] == 0 )
		{
			b_invalid_effective_area = true;
		}
		
		if( m_model_e == eMODEL_2530VW )
		{
			if( m_manufacture_info_t.p_effective_area[eEFFECTIVE_AREA_RIGHT] > 2047 
				|| m_manufacture_info_t.p_effective_area[eEFFECTIVE_AREA_BOTTOM] > 2559 )
			{
				b_invalid_effective_area = true;
			}
		}
		else if( m_model_e == eMODEL_3643VW )
		{
			if( m_manufacture_info_t.p_effective_area[eEFFECTIVE_AREA_RIGHT] > 2559  )
			{
				b_invalid_effective_area = true;
			}
		}
	}
	
	if(b_invalid_effective_area == true)
	{	
		effective_area_default();
		print_log(ERROR, 1, "[%s] Invalid effective area, init default.\n", m_p_log_id);
	}
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    scintillator type 0, 1 is deprecated.
 *          if stored value is deprecated value, convert it to updated type.
*******************************************************************************/
SCINTILLATOR_TYPE CEnv::get_scintillator_type_default(void)
{
	SCINTILLATOR_TYPE e_type = eSCINTILLATOR_TYPE_MAX;

	//2530 VW default: decomp --> E로 변환
	//2530 VW plus: decomp --> E로 변환
	if( m_model_e == eMODEL_2530VW )	
	{
		e_type = eSCINTILLATOR_TYPE_06;
		print_log(ERROR, 1, "[%s] get_scintillator_type_default: %d (aka eSCINTILLATOR_TYPE_E)\n", m_p_log_id, e_type);
	}
	else if( m_model_e == eMODEL_3643VW )	
	{
		if( get_submodel_type() == eSUBMODEL_TYPE_PLUS )	//3643/4343 VW Plus: decomp --> C로 변환
		{
			e_type = eSCINTILLATOR_TYPE_04;
			print_log(ERROR, 1, "[%s] get_scintillator_type_default: %d (aka eSCINTILLATOR_TYPE_C)\n", m_p_log_id, e_type);
		}
		else	//3643/4343 VW default: decomp --> A 로 변환
		{
			e_type = eSCINTILLATOR_TYPE_02;
			print_log(ERROR, 1, "[%s] get_scintillator_type_default: %d (aka eSCINTILLATOR_TYPE_A)\n", m_p_log_id, e_type);
		}
	}	
	else if( m_model_e == eMODEL_4343VW	)			
	{
		if( get_submodel_type() == eSUBMODEL_TYPE_PLUS )	//3643/4343 VW Plus: decomp --> C로 변환
		{
			e_type = eSCINTILLATOR_TYPE_04;
			print_log(ERROR, 1, "[%s] get_scintillator_type_default: %d (aka eSCINTILLATOR_TYPE_C)\n", m_p_log_id, e_type);
		}
		else	//3643/4343 VW default: decomp --> A 로 변환
		{
			e_type = eSCINTILLATOR_TYPE_02;
			print_log(ERROR, 1, "[%s] get_scintillator_type_default: %d (aka eSCINTILLATOR_TYPE_A)\n", m_p_log_id, e_type);
		}
	}			
	else
	{
		print_log(ERROR, 1, "[%s] get_scintillator_type_default Failed. return Default: %d\n", m_p_log_id, e_type);	
	}	

	return e_type;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    scintillator type 0, 1 is deprecated.
 *          if stored value is deprecated value, convert it to updated type.
*******************************************************************************/
void CEnv::check_scintillator_type_and_update(void)
{
	//If deprecated value
	if( ( m_manufacture_info_t.n_scintillator_type == eSCINTILLATOR_TYPE_00 ) ||
		( m_manufacture_info_t.n_scintillator_type == eSCINTILLATOR_TYPE_01 ) )
	{
		print_log(ERROR, 1, "[%s] scintillator type %d is deprecated.\n", m_p_log_id, m_manufacture_info_t.n_scintillator_type);
		
		m_manufacture_info_t.n_scintillator_type = get_scintillator_type_default();
	}
}

/******************************************************************************/
/**
 * @brief
 * @param
 * @return
 * @note
*******************************************************************************/
void CEnv::aed_info_read(aed_info_t* o_p_aed_t)
{
	bool b_invalid_data = false;
	if (is_vw_revision_board())	micom_eeprom_read( EEPROM_ADDR_AED_BUF, (u8*)o_p_aed_t, sizeof(aed_info_t), eMICOM_EEPROM_BLK_FRAM );
    else						eeprom_read( EEPROM_ADDR_AED_BUF, (u8*)o_p_aed_t, sizeof(aed_info_t) );
	
	if( o_p_aed_t->n_magic_number != EEPROM_MAGIC_NUMBER )
	{   
		b_invalid_data = true;
	}
	
	if( o_p_aed_t->n_aed0_trig_high_thr == 0 || (o_p_aed_t->n_aed0_trig_low_thr > o_p_aed_t->n_aed0_trig_high_thr) )
	{
		b_invalid_data = true;
	}
	
	if( o_p_aed_t->n_magic_number_v2 != EEPROM_MAGIC_NUMBER_V2 )
	{
	    o_p_aed_t->n_aed1_trig_low_thr = AED_LOW_THRESHOLD_VOLTAGE;
	    o_p_aed_t->n_aed1_trig_high_thr = AED_HIGH_THRESHOLD_VOLTAGE;
		print_log(ERROR, 1, "[%s] Initialize AED1 info. low : %d, high : %d\n", m_p_log_id, o_p_aed_t->n_aed1_trig_low_thr, o_p_aed_t->n_aed1_trig_high_thr);
	}	
		
	if( b_invalid_data )
	{	
	    o_p_aed_t->n_aed0_trig_low_thr = AED_LOW_THRESHOLD_VOLTAGE;
	    o_p_aed_t->n_aed0_trig_high_thr = AED_HIGH_THRESHOLD_VOLTAGE;
	    o_p_aed_t->p_aed_pot[eAED0_BIAS] = 190;
	    o_p_aed_t->p_aed_pot[eAED1_BIAS] = 190;
	    o_p_aed_t->p_aed_pot[eAED0_OFFSET] = 50;
	    o_p_aed_t->p_aed_pot[eAED1_OFFSET] = 50;
	    o_p_aed_t->n_aed1_trig_low_thr = AED_LOW_THRESHOLD_VOLTAGE;
	    o_p_aed_t->n_aed1_trig_high_thr = AED_HIGH_THRESHOLD_VOLTAGE;
		
		print_log(ERROR, 1, "[%s] Load default AED info\n", m_p_log_id);
	}

}

/******************************************************************************/
/**
 * @brief
 * @param
 * @return
 * @note
*******************************************************************************/
void CEnv::aed_info_write(aed_info_t* i_p_aed_t)
{	
	i_p_aed_t->n_magic_number = EEPROM_MAGIC_NUMBER;
	i_p_aed_t->n_magic_number_v2 = EEPROM_MAGIC_NUMBER_V2;

	if (is_vw_revision_board())	micom_eeprom_write( EEPROM_ADDR_AED_BUF, (u8*)i_p_aed_t, sizeof(aed_info_t), eMICOM_EEPROM_BLK_FRAM );
	else						eeprom_write( EEPROM_ADDR_AED_BUF, (u8*)i_p_aed_t, sizeof(aed_info_t) );
}


/***********************************************************************************************//**
* \brief
* \param
* \return
* \note
***************************************************************************************************/
u32 CEnv::eeprom_read( u16 i_w_addr, u8* o_p_data, u32 i_n_len )
{	
	u32		status = eST_EEP_SUCCESS;
	u16		address;
	u32		margin;
	u32		size;
	
	if(i_n_len == 0)
	{
		print_log( DEBUG, 1, "[%s] EEPROM read, length 0.\n",m_p_log_id);
		return eST_EEP_SUCCESS;
	}
	
	if(i_w_addr & (~(EEPROM_TOTAL_SIZE - 1)))
	{
		print_log( DEBUG, 1, "[%s] EEPROM read error, invalid address.\n",m_p_log_id);
		status = eST_EEP_ADDR_INVALID;
		return status;
	}

	if((i_w_addr + i_n_len - 1) & (~(EEPROM_TOTAL_SIZE - 1)))
	{
		print_log( DEBUG, 1, "[%s] EEPROM read error, the length exceeds the total eeprom size.\n",m_p_log_id);
		status = eST_EEP_RANGE_OVERFLOW;
		return status;
	}

	address = i_w_addr;
	margin = EEPROM_PAGE_SIZE - (i_w_addr & (EEPROM_PAGE_SIZE - 1));
	
	if(i_n_len > margin)
	{
		status = eeprom_page_read(address, o_p_data, margin);
		if(status != eST_EEP_SUCCESS)
		{
			return status;
		}
		address 	+= margin;
		o_p_data 	+= margin;
		i_n_len		-= margin;
		
		while(i_n_len)
		{
			size = (i_n_len > EEPROM_PAGE_SIZE) ? EEPROM_PAGE_SIZE : i_n_len;
			status = eeprom_page_read(address, o_p_data, size);
			if(status != eST_EEP_SUCCESS)
			{
				return status;
			}
			address	+= size;
			o_p_data	+= size;
			i_n_len		-= size;
		}
	}
	else
	{
		status = eeprom_page_read(address, o_p_data, i_n_len);
	}
	
	return status;
	

}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CEnv::eeprom_write( u16 i_w_addr, u8* i_p_data, u32 i_n_len )
{
	u32		status;
	u16		address;
	u32		margin;
	u32		size;
	
	if(i_n_len == 0)
	{
		print_log( DEBUG, 1, "[%s] EEPROM write, length 0.\n",m_p_log_id);
		return eST_EEP_SUCCESS;
	}
	
	if(i_w_addr & (~(EEPROM_TOTAL_SIZE - 1)))
	{
		print_log( DEBUG, 1, "[%s] EEPROM write error, invalid address.\n",m_p_log_id);
		status = eST_EEP_ADDR_INVALID;
		return status;
	}

	if((i_w_addr + i_n_len - 1) & (~(EEPROM_TOTAL_SIZE - 1)))
	{
		print_log( DEBUG, 1, "[%s] EEPROM write error, the length exceeds the total eeprom size.\n",m_p_log_id);
		status = eST_EEP_RANGE_OVERFLOW;
		return status;
	}

	address = i_w_addr;
	margin = EEPROM_PAGE_SIZE - (i_w_addr & (EEPROM_PAGE_SIZE - 1));
	
	if(i_n_len > margin)
	{
		status = eeprom_page_write(address, i_p_data, margin);
		if(status != eST_EEP_SUCCESS)
		{
			return status;
		}
		address 	+= margin;
		i_p_data 	+= margin;
		i_n_len		-= margin;
		
		while(i_n_len)
		{
			size = (i_n_len > EEPROM_PAGE_SIZE) ? EEPROM_PAGE_SIZE : i_n_len;
			status = eeprom_page_write(address, i_p_data, size);
			if(status != eST_EEP_SUCCESS)
			{
				return status;
			}
			address	+= size;
			i_p_data	+= size;
			i_n_len		-= size;
		}
	}
	else
	{
		status = eeprom_page_write(address, i_p_data, i_n_len);
	}
	
	return status;
}

/***********************************************************************************************//**
* \brief
* \param
* \return
* \note
***************************************************************************************************/
u32 CEnv::eeprom_page_write(u16 i_w_addr, u8* i_p_data, u32 i_n_len)
{
	u32				status = eST_EEP_SUCCESS;
	ioctl_data_t 	iod_t;
	memset(&iod_t, 0, sizeof(ioctl_data_t));
	
	if(i_n_len == 0)
	{
		print_log( DEBUG, 1, "[%s] EEPROM page write, length 0.\n",m_p_log_id);
		return eST_EEP_SUCCESS;
	}	

	/* check if given range exceeds specific page */
	if(((i_w_addr & (EEPROM_PAGE_SIZE - 1)) + i_n_len - 1) & (~(EEPROM_PAGE_SIZE - 1)))
	{
		print_log( DEBUG, 1, "[%s] EEPROM page write failed, attempts to write accross a page boundary.\n",m_p_log_id);
		status = eST_EEP_PAGE_EXCEED;
		return status;
	}

	iod_t.size = i_n_len;
	iod_t.data[0] = (i_w_addr >> 8) & 0x00FF;
	iod_t.data[1] =  i_w_addr & 0x00FF;
	memcpy(&iod_t.data[2], i_p_data, i_n_len);

	if (ioctl( m_n_eeprom_fd, VW_MSG_IOCTL_SPI_EEPROM_WRITE ,(char*)&iod_t ) < 0)
	{
		print_log( DEBUG, 1, "[%s] EEPROM page write failed, ioctl error.\n",m_p_log_id);
  		return eST_EEP_IOCTL_FAIL;
  	}

  	if(iod_t.ret < 0)
	{
  		print_log( ERROR, 1, "[%s] EEPROM page write failed, EEPROM Write in process timeout.\n",m_p_log_id);
  		return eST_EEP_TIMEOUT;
  	}  		
	
	return status;
}


/***********************************************************************************************//**
* \brief
* \param
* \return
* \note
***************************************************************************************************/
u32 CEnv::eeprom_page_read(u16 i_w_addr, u8* o_p_data, u32 i_n_len)
{	
	u32				status = eST_EEP_SUCCESS;
	ioctl_data_t 	iod_t;
	memset(&iod_t, 0, sizeof(ioctl_data_t));
	
	if(i_n_len == 0)
	{
		print_log( DEBUG, 1, "[%s] EEPROM page read, length 0.\n",m_p_log_id);
		return eST_EEP_SUCCESS;
	}	

	/* check if given range exceeds specific page */
	if(((i_w_addr & (EEPROM_PAGE_SIZE - 1)) + i_n_len - 1) & (~(EEPROM_PAGE_SIZE - 1)))
	{
		print_log( DEBUG, 1, "[%s] EEPROM page read failed, attempts to read accross a page boundary.\n",m_p_log_id);
		status = eST_EEP_PAGE_EXCEED;
		return status;
	}

	iod_t.size = i_n_len;
	iod_t.data[0] = (i_w_addr >> 8) & 0x00FF;
	iod_t.data[1] =  i_w_addr & 0x00FF;

	if (ioctl( m_n_eeprom_fd, VW_MSG_IOCTL_SPI_EEPROM_READ ,(char*)&iod_t ) < 0)
	{
		print_log( DEBUG, 1, "[%s] EEPROM page write failed, ioctl error.\n",m_p_log_id);
  		return eST_EEP_IOCTL_FAIL;
  	}
	
	memcpy(o_p_data, &iod_t.data[2], i_n_len); 		
	
	return status;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CEnv::manufacture_info_print(void)
{
	struct tm*  p_gmt_tm_t;
	
	if( m_manufacture_info_t.n_magic_number != EEPROM_MAGIC_NUMBER )
	{
	    print_log( ERROR, 1, "[%s] manufacture info. is invalid\n", m_p_log_id );
	    return;
	}
	
	if( strlen(m_manufacture_info_t.p_package_info) > 0 )
	{
	    print_log( DEBUG, 1, "[%s] production package version: %s\n", m_p_log_id, m_manufacture_info_t.p_package_info );
	}
	
	if( m_manufacture_info_t.n_release_date > 0 )
	{
	    p_gmt_tm_t = localtime( (time_t*)&m_manufacture_info_t.n_release_date );
	    print_log( DEBUG, 1, "[%s] production date: %04d/%02d/%02d-%02d:%02d:%02d\n", \
	            m_p_log_id, p_gmt_tm_t->tm_year+1900, p_gmt_tm_t->tm_mon+1, p_gmt_tm_t->tm_mday,	\
                p_gmt_tm_t->tm_hour, p_gmt_tm_t->tm_min, p_gmt_tm_t->tm_sec );
	}
	
	if( strlen(m_manufacture_info_t.p_model_name) > 0 )
	{
	    print_log( DEBUG, 1, "[%s] model name: %s\n", m_p_log_id, m_manufacture_info_t.p_model_name );
	}

	if( strlen(m_manufacture_info_t.p_serial_number) > 0 )
	{
	    print_log( DEBUG, 1, "[%s] serial number: %s\n", m_p_log_id, m_manufacture_info_t.p_serial_number );
	}

	if( m_manufacture_info_t.p_mac_address[0] > 0 )
	{
	    print_log( DEBUG, 1, "[%s] registered mac address: %02X:%02X:%02X:%02X:%02X:%02X\n", \
	                m_p_log_id, \
	                m_manufacture_info_t.p_mac_address[0], m_manufacture_info_t.p_mac_address[1], \
	                m_manufacture_info_t.p_mac_address[2], m_manufacture_info_t.p_mac_address[3], \
	                m_manufacture_info_t.p_mac_address[4], m_manufacture_info_t.p_mac_address[5] );
	}

	print_log( DEBUG, 1, "[%s] effective area top: %d, bottom: %d, left: %d, right: %d\n", \
	                m_p_log_id, \
	                m_manufacture_info_t.p_effective_area[eEFFECTIVE_AREA_TOP], \
	                m_manufacture_info_t.p_effective_area[eEFFECTIVE_AREA_BOTTOM], \
	                m_manufacture_info_t.p_effective_area[eEFFECTIVE_AREA_LEFT], \
	                m_manufacture_info_t.p_effective_area[eEFFECTIVE_AREA_RIGHT] );
	
	if( m_manufacture_info_t.n_magic_number_v2 == EEPROM_MAGIC_NUMBER_V2 )              
	{
		if( get_submodel_type() == eSUBMODEL_TYPE_PLUS )
		{
			print_log( DEBUG, 1, "[%s] Scintillator type : %d\n", m_p_log_id, m_manufacture_info_t.n_scintillator_type );
		}
		print_log( DEBUG, 1, "[%s] Battery count : %d\n", m_p_log_id, m_manufacture_info_t.w_battery_count_to_install );
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CEnv::manufacture_info_reset(void)
{
    manufacture_info_print();
    
    manufacture_info_default();
    
    if (is_vw_revision_board())	micom_eeprom_write( EEPROM_ADDR_ENV, (u8*)&m_manufacture_info_t, sizeof(manufacture_info_t), eMICOM_EEPROM_BLK_FRAM );
    else						eeprom_write( EEPROM_ADDR_ENV, (u8*)&m_manufacture_info_t, sizeof(manufacture_info_t) );
    print_log(DEBUG, 1, "[%s] manufacture info reset\n", m_p_log_id);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CEnv::manufacture_info_get( manufacture_info_t* o_p_info_t )
{
    memcpy( o_p_info_t, &m_manufacture_info_t, sizeof(manufacture_info_t) );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CEnv::manufacture_info_set( manufacture_info_t* i_p_info_t )
{
    bool b_update_date = false;
    manufacture_info_t	ori_info_t;
    MODEL ori_e = get_model();
    
    memcpy( &ori_info_t, &m_manufacture_info_t, sizeof(manufacture_info_t) );
    memcpy( &m_manufacture_info_t, i_p_info_t, sizeof(manufacture_info_t) );    
    
    print_log(DEBUG, 1, "[%s] Scintillator type set: %d\n", m_p_log_id, m_manufacture_info_t.n_scintillator_type);
    
    if( strcmp(ori_info_t.p_model_name, i_p_info_t->p_model_name) != 0 )
    {
    	b_update_date = true;
    	
	    set_model();
	    if(ori_e != get_model())
		{
			print_log(DEBUG, 1, "[%s] Model type is changed %d -> %d, set default effective area\n", m_p_log_id, ori_e, get_model());
			effective_area_default();
			
		}   
		//SUBMODEL이 NORMAL --> PLUS 혹은 그 반대의 경우에도 scintillator type은 업데이트 되어야함
		m_manufacture_info_t.n_scintillator_type = get_scintillator_type_default();
    }
    
    if( strcmp(ori_info_t.p_serial_number, i_p_info_t->p_serial_number) != 0 )
    {
        b_update_date = true;
    }

    if( memcmp(ori_info_t.p_mac_address, i_p_info_t->p_mac_address, 6) != 0 )
    {
        b_update_date = true;
    }
    
    if( b_update_date == true )
    {   
        strcpy(m_manufacture_info_t.p_package_info, m_p_config[PRESET_ID_SYSTEM][eCFG_VERSION_PACKAGE]);    
    }
    else
    {
        m_manufacture_info_t.n_release_date = ori_info_t.n_release_date;
    }
	
    manufacture_info_write();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CEnv::version_read(void)
{
    strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_VERSION_FW], (s8*)FW_VERSION );
    
    s8  p_command[128];
    s8  p_version[128];
    s32 n_len;
    
    memset( p_command, 0, sizeof(p_command) );
    memset( p_version, 0, sizeof(p_version) );
    
    sprintf( p_command, "fw_printenv %s | sed ""s/%s=//""", "b_ver", "b_ver");
    
    n_len = process_get_result( p_command, p_version );
    if( n_len > 0 )
    {
        p_version[n_len-1] = '\0';
        
        strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_VERSION_BOOTLOADER], p_version );
    }

    memset( p_version, 0, sizeof(p_version) );
    sprintf( p_command, "fw_printenv %s | sed ""s/%s=//""", "k_ver", "k_ver");
    
    n_len = process_get_result( p_command, p_version );
    if( n_len > 0 )
    {
        p_version[n_len-1] = '\0';
        
        strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_VERSION_KERNEL], p_version );
    }
    
    memset( p_version, 0, sizeof(p_version) );
    sprintf( p_command, "fw_printenv %s | sed ""s/%s=//""", "r_ver", "r_ver");
    
    n_len = process_get_result( p_command, p_version );
    if( n_len > 0 )
    {
        p_version[n_len-1] = '\0';
        
        strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_VERSION_RFS], p_version );
    }
    
    memset( p_version, 0, sizeof(p_version) );
    sprintf( p_command, "fw_printenv %s | sed ""s/%s=//""", "p_ver", "p_ver");
    
    n_len = process_get_result( p_command, p_version );
    if( n_len > 0 )
    {
        p_version[n_len-1] = '\0';
        
        strcpy( m_p_config[PRESET_ID_SYSTEM][eCFG_VERSION_PACKAGE], p_version );
    }
    
	//이슈 :"SDK 1.0.26 이하 & 실행 패키지 버전 FW 1.0.0.8 이하 & 업데이트 패키지 버전 FW 1.0.0.9 이상"의 조건에서 
	//      실제 디텍터 내부 binary는 정상적으로 업데이트되지만, 패키지 버전은 업데이트되지 않고, SDK에서 실패 처리 및 디텍터 재부팅 수행하지 않는 이슈 
	//1.0.0.8보다 작거나 같으면 패키지에서 1.0.1.3 보다 큰 패키지로 업데이트 할 경우, SDK에서 Package 버전 전달에 실패하므로, 
	//FW에서 스스로 FW 버전과 매칭되는 package version 으로 업데이트 필요 
	if( _check_compatibility_package_version(m_p_config[PRESET_ID_SYSTEM][eCFG_VERSION_PACKAGE]) == false ) 
	{ 	
		if(strcmp(m_p_config[PRESET_ID_SYSTEM][eCFG_VERSION_FW], "1.0.2.0") == 0)
			strcpy(m_p_config[PRESET_ID_SYSTEM][eCFG_VERSION_PACKAGE], "1.0.2.0"); 	
		if(strcmp(m_p_config[PRESET_ID_SYSTEM][eCFG_VERSION_FW], "1.0.3.15") == 0)
			strcpy(m_p_config[PRESET_ID_SYSTEM][eCFG_VERSION_PACKAGE], "1.0.6.0"); 	
		if(strcmp(m_p_config[PRESET_ID_SYSTEM][eCFG_VERSION_FW], "1.0.4.0") == 0)
			strcpy(m_p_config[PRESET_ID_SYSTEM][eCFG_VERSION_PACKAGE], "1.0.7.0");
	}

    print_log(DEBUG, 1, "[%s] Package: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_VERSION_PACKAGE] );
    print_log(DEBUG, 1, "[%s] Bootloader: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_VERSION_BOOTLOADER] );
    print_log(DEBUG, 1, "[%s] Kernel: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_VERSION_KERNEL] );
    print_log(DEBUG, 1, "[%s] Ramdisk: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_VERSION_RFS] );
    print_log(DEBUG, 1, "[%s] Firmware: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_VERSION_FW] );
    print_log(DEBUG, 1, "[%s] FPGA: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_VERSION_FPGA] );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CEnv::preview_get_enable(void)
{
    if( strcmp( m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_PREVIEW], "on") == 0 )
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
bool CEnv::tact_get_enable(void)
{
    if( strcmp( m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_FAST_TACT], "on") == 0 )
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
void CEnv::set_model(void)
{	
    if( !strncmp(m_manufacture_info_t.p_model_name, "FXRD-2530", strlen("FXRD-2530")) )
    {
    	m_model_e = eMODEL_2530VW;
        strcpy( m_p_config[0][eCFG_MODEL], m_manufacture_info_t.p_model_name);
    }
    else if( !strncmp(m_manufacture_info_t.p_model_name, "FXRD-3643", strlen("FXRD-3643")) )
    {
    	m_model_e = eMODEL_3643VW;	
        strcpy( m_p_config[0][eCFG_MODEL], m_manufacture_info_t.p_model_name);
    }
	else if( !strncmp(m_manufacture_info_t.p_model_name, "VXTD-3643", strlen("VXTD-3643")) )
    {
    	m_model_e = eMODEL_3643VW;	
        strcpy( m_p_config[0][eCFG_MODEL], m_manufacture_info_t.p_model_name);
    }
    else if( !strncmp(m_manufacture_info_t.p_model_name, "FXRD-4343", strlen("FXRD-4343")) )
    {
    	m_model_e = eMODEL_4343VW;
        strcpy( m_p_config[0][eCFG_MODEL], m_manufacture_info_t.p_model_name);
    }
    else
    {
    	m_model_e = eMODEL_3643VW;
    	print_log(ERROR, 1, "[%s] Invalid model name : %s, Set model type to FXRD-3643VW as a default setting.\n", m_p_log_id, m_manufacture_info_t.p_model_name);

        strcpy( m_p_config[0][eCFG_MODEL], "FXRD-3643VAW");
        strcpy( m_manufacture_info_t.p_model_name, "FXRD-3643VAW" );		
    }
}

/******************************************************************************/
/**
 * @brief   운용정보를 초기화
 * @param   
 * @return  
 * @note    operation info에는 capture count 와 같이 운용과정에서 계속 갱신되는
 			데이터들이 포함되며, 생산공정에서 출하전 이 정보들을 삭제하는 과정이 있음
 			VW모델의 경우, 가속도 센서 cal 값을 함께 포함시켜 영역 전체를 초기화할 경우
 			가속도 센서 cal data가 소실될 수 있음
*******************************************************************************/
void CEnv::oper_info_reset(void)
{
	/* [2020.06.19 MJI] 생산공정에서 수행되는 total shot count clear 기능이 capture count 정보에만 
	   한정되어 가속도 센서 cal 값이 소실되지 않도록 함 */
    m_oper_info_t.n_capture_count = 0;
    oper_info_write();
    
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CEnv::oper_info_print(void)
{
	if( m_oper_info_t.n_magic_number != EEPROM_MAGIC_NUMBER )
	{
	    print_log( ERROR, 1, "[%s] operation info. is invalid\n", m_p_log_id );
	    return;
	}
	
    print_log(DEBUG, 1, "[%s] catpure count: %d\n", m_p_log_id, m_oper_info_t.n_capture_count );
    print_log(DEBUG, 1, "[%s] impact offset x : %d, y : %d, z : %d\n", m_p_log_id, m_oper_info_t.impact_offset_t.c_x, m_oper_info_t.impact_offset_t.c_y, m_oper_info_t.impact_offset_t.c_z );
	print_log(DEBUG, 1, "[%s] gyro offset x : %d, y : %d, z : %d\n", m_p_log_id, m_oper_info_t.gyro_offset_t.c_x, m_oper_info_t.gyro_offset_t.c_y, m_oper_info_t.gyro_offset_t.c_z );
	print_log(DEBUG, 1, "[%s] battery charging mode : %d, recharging gauge : %d\n", m_p_log_id, m_oper_info_t.bat_chg_mode, m_oper_info_t.c_rechg_gauge);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CEnv::oper_info_capture(void)
{
    m_oper_info_t.n_capture_count++;
    oper_info_write();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
impact_cal_offset_t* CEnv::get_impact_offset(void)
{
	return &(m_oper_info_t.impact_offset_t);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
impact_cal_extra_detailed_offset_t CEnv::get_impact_extra_detailed_offset(void)
{
	impact_cal_extra_detailed_offset_t impact_cal_extra_detailed_offset;
	impact_cal_extra_detailed_offset.s_x = m_oper_info_t.w_x_impact_extra_detailed_offset;
	impact_cal_extra_detailed_offset.s_y = m_oper_info_t.w_y_impact_extra_detailed_offset;
	impact_cal_extra_detailed_offset.s_z = m_oper_info_t.w_z_impact_extra_detailed_offset;
	return impact_cal_extra_detailed_offset;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CEnv::save_impact_extra_detailed_offset(impact_cal_extra_detailed_offset_t* i_p_impact_cal_extra_detailed_offset)
{
	if(i_p_impact_cal_extra_detailed_offset != NULL)
	{
		print_log( ERROR, 1, "[%s] Save impact extra detailed offset x: %d y: %d z: %d\n", m_p_log_id, i_p_impact_cal_extra_detailed_offset->s_x, i_p_impact_cal_extra_detailed_offset->s_y, i_p_impact_cal_extra_detailed_offset->s_z );
		m_oper_info_t.w_x_impact_extra_detailed_offset = i_p_impact_cal_extra_detailed_offset->s_x;
		m_oper_info_t.w_y_impact_extra_detailed_offset = i_p_impact_cal_extra_detailed_offset->s_y;
		m_oper_info_t.w_z_impact_extra_detailed_offset = i_p_impact_cal_extra_detailed_offset->s_z;
		oper_info_write();
	}	
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u8 CEnv::get_impact_6plane_cal_done(void)
{
	print_log( ERROR, 1, "[%s] Acceleration sensor 6-plane cal done, 0x%02X\n", m_p_log_id, m_oper_info_t.c_acc_6plane_cal_done );
	return m_oper_info_t.c_acc_6plane_cal_done;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CEnv::save_impact_6plane_cal_done(u8 i_c_done)
{
	print_log( ERROR, 1, "[%s] Save Acceleration sensor 6-plane cal done, 0x%02X\n", m_p_log_id, i_c_done );
	m_oper_info_t.c_acc_6plane_cal_done = i_c_done;
	oper_info_write();
}



/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CEnv::customized_info_read(void)
{
	if (is_vw_revision_board())	micom_eeprom_read( EEPROM_ADDR_CUSTOM_INFO, (u8*)&m_customized_info_t, sizeof(customized_info_t), eMICOM_EEPROM_BLK_FRAM );
    else						eeprom_read( EEPROM_ADDR_CUSTOM_INFO, (u8*)&m_customized_info_t, sizeof(customized_info_t) );
	
	print_log(DEBUG, 1, "[%s] Customized info read, wifi step: %d\n", m_p_log_id, m_customized_info_t.c_wifi_signal_step );
	
	if( m_customized_info_t.n_magic_number != EEPROM_MAGIC_NUMBER )
	{
	    memset( &m_customized_info_t, 0, sizeof(customized_info_t) );
	}

	if( m_customized_info_t.c_wifi_signal_step != 0 && m_customized_info_t.c_wifi_signal_step != 1 )
	{
		m_customized_info_t.c_wifi_signal_step = 0;
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CEnv::customized_info_write(void)
{
    m_customized_info_t.n_magic_number = EEPROM_MAGIC_NUMBER;
	if (is_vw_revision_board())	micom_eeprom_write( EEPROM_ADDR_CUSTOM_INFO, (u8*)&m_customized_info_t, sizeof(customized_info_t), eMICOM_EEPROM_BLK_FRAM );
	else						eeprom_write( EEPROM_ADDR_CUSTOM_INFO, (u8*)&m_customized_info_t, sizeof(customized_info_t) );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u8 CEnv::get_wifi_signal_step_num(void)
{
	u8 u_wifi_signal_step_num = m_customized_info_t.c_wifi_signal_step;
	if(u_wifi_signal_step_num != 0 && u_wifi_signal_step_num != 1)
		return 0; // 값이 비정상인 경우 default 0 (5 step) 
	else
		return u_wifi_signal_step_num;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CEnv::save_wifi_signal_step_num(u8 i_c_wifi_step_num)
{
	if(i_c_wifi_step_num != 0 && i_c_wifi_step_num != 1)
		m_customized_info_t.c_wifi_signal_step = 0; // 값이 비정상인 경우 default 0 (5 step) 
	else
		m_customized_info_t.c_wifi_signal_step = i_c_wifi_step_num;
		
	customized_info_write();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CEnv::save_impact_offset(impact_cal_offset_t* i_p_impact_offset_t)
{
	memcpy(&(m_oper_info_t.impact_offset_t), i_p_impact_offset_t, sizeof(impact_cal_offset_t));
	print_log(DEBUG, 1, "[%s] Save Impact offset x : %d, y : %d, z : %d\n", m_p_log_id, m_oper_info_t.impact_offset_t.c_x, m_oper_info_t.impact_offset_t.c_y, m_oper_info_t.impact_offset_t.c_z );
	oper_info_write();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CEnv::oper_info_read(void)
{
	if (is_vw_revision_board())	micom_eeprom_read( EEPROM_ADDR_OPER, (u8*)&m_oper_info_t, sizeof(m_oper_info_t), eMICOM_EEPROM_BLK_FRAM );
    else						eeprom_read( EEPROM_ADDR_OPER, (u8*)&m_oper_info_t, sizeof(operation_info_t) );
	
	if( m_oper_info_t.n_magic_number != EEPROM_MAGIC_NUMBER )
	{
	    memset( &m_oper_info_t, 0, sizeof(operation_info_t) );
	}
	
	if( m_oper_info_t.n_magic_number_v2 != EEPROM_MAGIC_NUMBER_V2 )
    {
    	m_oper_info_t.w_x_impact_extra_detailed_offset = 0;
   		m_oper_info_t.w_y_impact_extra_detailed_offset = 0;
    	m_oper_info_t.w_z_impact_extra_detailed_offset = 0;
    }	
    
	if( m_oper_info_t.n_magic_number_v3 != EEPROM_MAGIC_NUMBER_V3 || (m_oper_info_t.c_acc_6plane_cal_done & ~0x3f) != 0 )
    {
    	m_oper_info_t.c_acc_6plane_cal_done = 0;
    }	
	
	if( m_oper_info_t.n_magic_number_v4 != EEPROM_MAGIC_NUMBER_V4 )
    {
    	m_oper_info_t.gyro_offset_t.c_x = 0;
		m_oper_info_t.gyro_offset_t.c_y = 0;
		m_oper_info_t.gyro_offset_t.c_z = 0;
		m_oper_info_t.c_combo_impact_cal_done = 0;
    }	

	if( m_oper_info_t.n_magic_number_v5 != EEPROM_MAGIC_NUMBER_V5 )
    {
    	m_oper_info_t.bat_chg_mode = eCHG_MODE_NORMAL;	//init
		m_oper_info_t.c_rechg_gauge = 80;				//init
    }	
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CEnv::oper_info_write(void)
{
    m_oper_info_t.n_magic_number = EEPROM_MAGIC_NUMBER;
    m_oper_info_t.n_magic_number_v2 = EEPROM_MAGIC_NUMBER_V2;
    m_oper_info_t.n_magic_number_v3 = EEPROM_MAGIC_NUMBER_V3;
	m_oper_info_t.n_magic_number_v4 = EEPROM_MAGIC_NUMBER_V4;
	m_oper_info_t.n_magic_number_v5 = EEPROM_MAGIC_NUMBER_V5;
	if (is_vw_revision_board())	micom_eeprom_write( EEPROM_ADDR_OPER, (u8*)&m_oper_info_t, sizeof(operation_info_t), eMICOM_EEPROM_BLK_FRAM );
	else						eeprom_write( EEPROM_ADDR_OPER, (u8*)&m_oper_info_t, sizeof(operation_info_t) );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CEnv::net_is_valid_channel( u16 i_w_num )
{
    
    if( strcmp( config_get(eCFG_AP_FREQ), "2" ) == 0 )
    {
        if( strcmp( config_get(eCFG_AP_BAND), "HT20" ) == 0 )
        {
            if( strcmp( config_get(eCFG_AP_COUNTRY), "US" ) == 0 )
            {
                if( i_w_num >= 1 && i_w_num <= 11 )
                {
                    return true;
                }
            }
            else
            {
                if( i_w_num >= 1 && i_w_num <= 13 )
                {
                    return true;
                }
            }
        }
        else if( strcmp( config_get(eCFG_AP_BAND), "HT40+" ) == 0 )
        {
            if( i_w_num >= 1 && i_w_num <= 9 )
            {
                return true;
            }
        }
        else
        {
            if( strcmp( config_get(eCFG_AP_COUNTRY), "US" ) == 0 )
            {
                if( i_w_num >= 5 && i_w_num <= 11 )
                {
                    return true;
                }
            }
            else
            {
                if( i_w_num >= 5 && i_w_num <= 13 )
                {
                    return true;
                }
            }
        }
    }
    else
    {
        if( strcmp( config_get(eCFG_AP_BAND), "HT20" ) == 0 )
        {
            if( strcmp( config_get(eCFG_AP_COUNTRY), "KR" ) == 0
                || strcmp( config_get(eCFG_AP_COUNTRY), "US" ) == 0 )
            {
                if( i_w_num == 36 || i_w_num == 40 || i_w_num == 44
                    || i_w_num == 48 || i_w_num == 149 || i_w_num == 153
                    || i_w_num == 157 || i_w_num == 161 )
                {
                    return true;
                }
            }
            else if( strcmp( config_get(eCFG_AP_COUNTRY), "CN" ) == 0 )
            {
                if( i_w_num == 149 || i_w_num == 153
                    || i_w_num == 157 || i_w_num == 161 )
                {
                    return true;
                }
            }
            else
            {
                if( i_w_num == 36 || i_w_num == 40
                    || i_w_num == 44 || i_w_num == 48 )
                {
                    return true;
                }
            }
        }
        else if( strcmp( config_get(eCFG_AP_BAND), "HT40+" ) == 0 )
        {
            if( strcmp( config_get(eCFG_AP_COUNTRY), "KR" ) == 0
                || strcmp( config_get(eCFG_AP_COUNTRY), "US" ) == 0 )
            {
                if( i_w_num == 36 || i_w_num == 44
                    || i_w_num == 149 || i_w_num == 157 )
                {
                    return true;
                }
            }
            else if( strcmp( config_get(eCFG_AP_COUNTRY), "CN" ) == 0 )
            {
                if( i_w_num == 149 || i_w_num == 157 )
                {
                    return true;
                }
            }
            else
            {
                if( i_w_num == 36 || i_w_num == 44 )
                {
                    return true;
                }
            }
        }
        else
        {
            if( strcmp( config_get(eCFG_AP_COUNTRY), "KR" ) == 0
                || strcmp( config_get(eCFG_AP_COUNTRY), "US" ) == 0 )
            {
                if( i_w_num == 40 || i_w_num == 48
                    || i_w_num == 153 || i_w_num == 161 )
                {
                    return true;
                }
            }
            else if( strcmp( config_get(eCFG_AP_COUNTRY), "CN" ) == 0 )
            {
                if( i_w_num == 153 || i_w_num == 161 )
                {
                    return true;
                }
            }
            else
            {
                if( i_w_num == 40 || i_w_num == 48 )
                {
                    return true;
                }
            }
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
bool CEnv::model_is_wireless(void)
{
	return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CEnv::model_is_csi(void)
{
	if( m_p_config[0][eCFG_MODEL][10] == 'A' )
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
bool CEnv::load_auto_offset( auto_offset_t* o_p_offset_t )
{
    auto_offset_t   offset_t;
    bool            b_enable = false;
    
    memset( &offset_t, 0, sizeof(auto_offset_t) );
    
    if( strcmp( config_get(eCFG_OFFSET_ENABLE), "on" ) == 0 )
    {
        b_enable = true;
        
        offset_t.w_period_time      = (u16)atoi(config_get(eCFG_OFFSET_PERIOD));
        offset_t.w_count            = (u16)atoi(config_get(eCFG_OFFSET_COUNT));
        offset_t.w_temp_difference  = (u16)atoi(config_get(eCFG_OFFSET_TEMPERATURE));
        
        offset_t.b_aed_capture = false;
    }
    
    memcpy( o_p_offset_t, &offset_t, sizeof(auto_offset_t) );
    
    return b_enable;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CEnv::preset_load(void)
{
	m_b_preset = preset_load(PRESET_XML_FILE);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CEnv::is_preset_loaded(void)
{
	return m_b_preset;
}
	
/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CEnv::preset_change( const s8* i_p_ssid )
{
	u16 i;
	u16 w_preset_id = PRESET_ID_SYSTEM;
	
	if( is_preset_loaded() == false )
	{
		print_log(DEBUG, 1, "[%s] Preset is not loaded\n", m_p_log_id );
		return false;
	}
	
	for( i = 1; i < MAX_CFG_COUNT; i++ )
	{
		if( strlen(i_p_ssid) != 0 && strlen(m_p_config[i][eCFG_SSID]) != 0 \
			&& strlen(i_p_ssid) == strlen(m_p_config[i][eCFG_SSID]) \
			&& strncmp( m_p_config[i][eCFG_SSID], i_p_ssid, strlen(i_p_ssid) ) == 0 )
		{	
			w_preset_id = i;
			break;
		}
	}
	
	if( w_preset_id != PRESET_ID_SYSTEM )
	{		
		for( i = eCFG_TAG_ID; i < eCFG_LAST; i++ )
		{
			if( i == eCFG_IMACT_PEAK
				|| i == eCFG_DEFECT_LOWER_GRID )
			{
				continue;
			}
			
			if( strlen( m_p_config[w_preset_id][i] ) > 0 )
			{
				strcpy( m_p_config[PRESET_ID_SYSTEM][i], m_p_config[w_preset_id][i] );
			}
		}
		
		print_log(DEBUG, 1, "[%s] Preset is changed: %s\n", m_p_log_id, m_p_config[PRESET_ID_SYSTEM][eCFG_PRESET_NAME] );
	}
	else
	{
		print_log(DEBUG, 1, "[%s] set preset ssid failed...(not registerd ssid: %s)\n", m_p_log_id, i_p_ssid);
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
s32 CEnv::get_preset_index( const s8* i_p_ssid )
{
	u16 i;

	if( is_preset_loaded() == false )
	{
		print_log(DEBUG, 1, "[%s] Preset is not loaded\n", m_p_log_id );
		return -1;
	}
		
	for( i = 1; i < MAX_CFG_COUNT; i++ )
	{
		if( strlen(i_p_ssid) != 0 && strlen(m_p_config[i][eCFG_SSID]) != 0 \
			&& strlen(i_p_ssid) == strlen(m_p_config[i][eCFG_SSID]) \
			&& strncmp( m_p_config[i][eCFG_SSID], i_p_ssid, strlen(i_p_ssid) ) == 0 )
		{	
			return i;
		}
	}

	return -1;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
s8* CEnv::get_preset_name( const s8* i_p_tag_id )
{
	u16 i;

	if( is_preset_loaded() == false )
	{
		print_log(DEBUG, 1, "[%s] Preset is not loaded\n", m_p_log_id );
		return NULL;
	}
	
	for( i = 0; i < MAX_CFG_COUNT; i++ )
	{
		if( strcmp( m_p_config[i][eCFG_TAG_ID], i_p_tag_id ) == 0 )
		{
			return m_p_config[i][eCFG_PRESET_NAME];
		}
	}
	
	return NULL;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CEnv::is_lower_grid_line_support(void)
{
	if( strcmp(m_p_config[PRESET_ID_SYSTEM][eCFG_DEFECT_LOWER_GRID], "off" ) == 0 )
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
bool CEnv::is_backup_enable(void)
{
	if( strcmp(m_p_config[PRESET_ID_SYSTEM][eCFG_IMAGE_BACKUP], "enable") == 0 )
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
bool CEnv::is_wired_detection_on(void)
{
	if( strcmp(m_p_config[PRESET_ID_SYSTEM][eCFG_PM_AWD], "on") == 0 )
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
SUBMODEL_TYPE CEnv::get_submodel_type(void)
{
	s32 n_len = strlen( m_p_config[0][eCFG_MODEL] );
    
    if( n_len > 0 )
    {
        if( strstr(m_p_config[0][eCFG_MODEL], "PLUS") != NULL )
        {
            return eSUBMODEL_TYPE_PLUS;
        }
    }

    return eSUBMODEL_TYPE_BASIC;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
PANEL_TYPE CEnv::get_panel_type(void)
{
	return ePANEL_TYPE_BASIC;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
 *          
 *  scintillator type 2: TYPE A
    scintillator type 3: TYPE B
    scintillator type 4: TYPE C
    scintillator type 5: TYPE D
    scintillator type 6: TYPE E
    scintillator type 7: TYPE F
*******************************************************************************/
SCINTILLATOR_TYPE CEnv::get_scintillator_type(void)
{
    SCINTILLATOR_TYPE scintillator_type_e = (SCINTILLATOR_TYPE)m_manufacture_info_t.n_scintillator_type;

    switch(scintillator_type_e)
    {
        case eSCINTILLATOR_TYPE_02:
            print_log(ERROR, 1, "[%s] scintillator type: %d (aka TYPE A)\n", m_p_log_id, scintillator_type_e);
            break;
        case eSCINTILLATOR_TYPE_03:
            print_log(ERROR, 1, "[%s] scintillator type: %d (aka TYPE B)\n", m_p_log_id, scintillator_type_e);
            break;
        case eSCINTILLATOR_TYPE_04:
            print_log(ERROR, 1, "[%s] scintillator type: %d (aka TYPE C)\n", m_p_log_id, scintillator_type_e);
            break;
        case eSCINTILLATOR_TYPE_05:
            print_log(ERROR, 1, "[%s] scintillator type: %d (aka TYPE D)\n", m_p_log_id, scintillator_type_e);
            break;
        case eSCINTILLATOR_TYPE_06:
            print_log(ERROR, 1, "[%s] scintillator type: %d (aka TYPE E)\n", m_p_log_id, scintillator_type_e);
            break;
        case eSCINTILLATOR_TYPE_07:
            print_log(ERROR, 1, "[%s] scintillator type: %d (aka TYPE F)\n", m_p_log_id, scintillator_type_e);
            break;
        case eSCINTILLATOR_TYPE_08:
            print_log(ERROR, 1, "[%s] scintillator type: %d (aka TYPE G)\n", m_p_log_id, scintillator_type_e);
            break;
        case eSCINTILLATOR_TYPE_09:
            print_log(ERROR, 1, "[%s] scintillator type: %d (aka TYPE H)\n", m_p_log_id, scintillator_type_e);
            break;
        case eSCINTILLATOR_TYPE_10:
            print_log(ERROR, 1, "[%s] scintillator type: %d (aka TYPE I)\n", m_p_log_id, scintillator_type_e);
            break;
        case eSCINTILLATOR_TYPE_11:
            print_log(ERROR, 1, "[%s] scintillator type: %d (aka TYPE J)\n", m_p_log_id, scintillator_type_e);
            break;
        case eSCINTILLATOR_TYPE_12:
            print_log(ERROR, 1, "[%s] scintillator type: %d (aka TYPE K)\n", m_p_log_id, scintillator_type_e);
            break;
        case eSCINTILLATOR_TYPE_13:
            print_log(ERROR, 1, "[%s] scintillator type: %d (aka TYPE L)\n", m_p_log_id, scintillator_type_e);
            break;
        case eSCINTILLATOR_TYPE_14:
            print_log(ERROR, 1, "[%s] scintillator type: %d (aka TYPE M)\n", m_p_log_id, scintillator_type_e);
            break;
        case eSCINTILLATOR_TYPE_15:
            print_log(ERROR, 1, "[%s] scintillator type: %d (aka TYPE N)\n", m_p_log_id, scintillator_type_e);
            break;
        default:
            scintillator_type_e = eSCINTILLATOR_TYPE_02;
            print_log(ERROR, 1, "[%s] invalid scintillator type: %d\n", m_p_log_id, m_manufacture_info_t.n_scintillator_type);
            print_log(ERROR, 1, "[%s] set default scintillator type: %d (aka eSCINTILLATOR_TYPE_A)\n", m_p_log_id, scintillator_type_e);
            break;
    }

	return scintillator_type_e;
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
APBUTTON_TYPE CEnv::get_ap_button_type(void)
{
	if( strcmp(m_p_config[PRESET_ID_SYSTEM][eCFG_AP_BUTTON_FUNC_TYPE], "0") == 0 )
    {
        return eAP_STATION;
    }
    
    if( strcmp(m_p_config[PRESET_ID_SYSTEM][eCFG_AP_BUTTON_FUNC_TYPE], "1") == 0 )
    {
    	return ePRESET_SWITCHING;
    }

    return eAP_STATION;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return   0: verification success
 *           1: verification fail
 * @note     Scintillator Type validation check만 수행중
*******************************************************************************/
u32 CEnv::verify_manufacture_info(manufacture_info_t* i_manufacture_info_t)
{
    u32 scintillator_type = i_manufacture_info_t->n_scintillator_type;
    u32 ret = 0x00;

    //scintillator verification check 
    //FW Model specification: 2 <= scintillator type <= 7 
    //scintillator type 2: TYPE A
    //scintillator type 3: TYPE B
    //scintillator type 4: TYPE C
    //scintillator type 5: TYPE D
    //scintillator type 6: TYPE E
    //scintillator type 7: TYPE F
    if( (eSCINTILLATOR_TYPE_02 > scintillator_type) || (eSCINTILLATOR_TYPE_MAX <= scintillator_type)  )
    {
        print_log(DEBUG, 1, "[%s] Invalid scintillator type\n", m_p_log_id);
        ret |= 1;
    }
    return ret;
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CEnv::_check_exceed(s8* i_p_token, s32 i_n_compare_num)
{
	if ( i_p_token == NULL )
	{
		return false; 
	}
	else
	{
		if( atoi(i_p_token) > i_n_compare_num)
		{
			return true;
		}
	}
	return false;
}

/******************************************************************************/
/**
 * @brief   1.0.0.8 이하이면 false, 초과이면 true
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CEnv::_check_compatibility_package_version(s8* i_p_sdk_str)
{
	s8* p_token;
	s8 p_token_string[] = ".\n ";
	char s_package_ver[128];

	strcpy(s_package_ver, i_p_sdk_str);
	
	p_token = strtok(s_package_ver,p_token_string);
	
	if ( _check_exceed(p_token,1) == true)
	{
		return true; 
	}

	p_token = strtok(NULL,p_token_string);
	
	if ( _check_exceed(p_token,0) == true)
	{
		return true; 
	}
	
	p_token = strtok(NULL,p_token_string);
	
	if ( _check_exceed(p_token,0) == true)
	{
		return true; 
	}
	
	p_token = strtok(NULL,p_token_string);
	
	if ( _check_exceed(p_token,8) == true)
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
void CEnv::save_gyro_offset(gyro_cal_offset_t* i_p_gyro_offset_t)
{
	memcpy(&(m_oper_info_t.gyro_offset_t), i_p_gyro_offset_t, sizeof(gyro_cal_offset_t));
	m_oper_info_t.c_combo_impact_cal_done = 1;
	print_log(DEBUG, 1, "[%s] Save gyro offset x : %d, y : %d, z : %d\n", m_p_log_id, m_oper_info_t.gyro_offset_t.c_x, m_oper_info_t.gyro_offset_t.c_y, m_oper_info_t.gyro_offset_t.c_z );
	oper_info_write();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    from non volitile memory
*******************************************************************************/
gyro_cal_offset_t* CEnv::get_gyro_offset(void)
{
	return &(m_oper_info_t.gyro_offset_t);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CEnv::save_battery_charge_info(BAT_CHG_MODE* i_p_bat_chg_mode, u8* i_p_recharging_gauge)
{
	if(i_p_bat_chg_mode != NULL)
		memcpy(&(m_oper_info_t.bat_chg_mode), i_p_bat_chg_mode, sizeof(BAT_CHG_MODE));

	if(i_p_recharging_gauge != NULL)	
		memcpy(&(m_oper_info_t.c_rechg_gauge), i_p_recharging_gauge, sizeof(i_p_recharging_gauge));

	oper_info_write();
	print_log(DEBUG, 1, "[%s] battery charging mode : %d, recharging gauge : %d\n", m_p_log_id, m_oper_info_t.bat_chg_mode, m_oper_info_t.c_rechg_gauge );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
BAT_CHG_MODE CEnv::get_battery_charging_mode_in_eeprom(void)
{
	return m_oper_info_t.bat_chg_mode;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u8 CEnv::get_battery_recharging_gauge_in_eeprom(void)
{
	return m_oper_info_t.c_rechg_gauge;
}

/***********************************************************************************************//**
* \brief		check if it is the board made after vw revision 
* \param		None
* \return		true: vw revision board
* \return		false: NOT vw revision board
* \note			vw revision board has the board number greater than 2
***************************************************************************************************/
bool CEnv::is_vw_revision_board(void)
{
	if( m_c_hw_ver <= 2) return false;
	else return true;
}
