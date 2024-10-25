/*******************************************************************************
 모  듈 : command
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
#include <iostream>
#include <vector>
#include <fstream>

#include <stdio.h>			// fprintf()
#include <string.h>			// memset() memset/memcpy
#include <stdlib.h>			// malloc(), free(),exit(), EXIT_SUCCESS rand/rand 
#include <stdbool.h>		// bool, true, false
//#include <ctype.h>			// toupper()
#include <math.h>
#include <unistd.h>			// read close
#include <fcntl.h>			// fcntl()
#include <termio.h>
#include <linux/types.h>
#include <sys/statvfs.h>	// statvfs()
#include <errno.h>			// errno
#include <time.h>

#include "command.h"
#include "blowfish.h"
#include "vw_file.h"
#include "vw_time.h"
#include "vw_download.h"
#include "misc.h"
#include "vw_net.h"
#include "fpga_prom.h"

#include "rapidxml.hpp"
#include "rapidxml_print.hpp"

using namespace std;

using namespace rapidxml;

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/
#define		BS		0x08 
#define		CR		0x0d 
#define		LF		0x0a 

/*******************************************************************************
*	Type Definitions
*******************************************************************************/

/**
 * @brief   
 */
typedef enum
{
    eCMD_STATE_SUCCESS   = 0,
    eCMD_STATE_OK,
    eCMD_STATE_INVALID_ARG,
    eCMD_STATE_INVALID_COMMAND,
    
} CMD_STATE;

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
* \param        none
* \return       none
* \note
*******************************************************************************/
CCommand::CCommand(void)
{
    m_bis_running = true;
    m_p_web_c = NULL;

    m_p_capture_c = NULL;
	m_p_system_c = NULL;
	m_p_control_svr_c = NULL;
	m_p_scu_svr_c = NULL;
	m_p_calibration_c = NULL;
	m_p_net_manager_c = NULL;
	m_p_diagnosis_c = NULL;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
CCommand::~CCommand(void)
{
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
s32 CCommand::get_line(s8 *s, s32 i_n_max)
{
	s32 cnt = 0;
	s8  c;
	
	printf(">");
	
	while( m_bis_running == true )
	{
		c = getchar();
		//printf("[%c,%x]",c,c );
		if( c == CR || c == LF || (cnt + 1) >= i_n_max)
		{
			*s = 0;
			break;
		} 

		switch( c )
		{
		    case BS:
		    {
		        if( cnt > 0 )
				{ 
					cnt--; *s-- = ' '; 
					printf("\b \b");  
				} 
				break;
			}
    		default:
    		{
    		    if( c >= 32 && c <= 126 )
    		    {
    		        cnt++;
    			    *s++ = c;
    			    //		printf("[%c,%x]",c,c );
    			}
    			break;
    		}
		}
	}
	
	return(cnt);
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
s32 CCommand::parse_args(s8 *line, s8 *argv[])
{
	s8 		*tok;
	s32 		argc;
	const s8 	*delim = " \f\n\r\t\v";
	
	argc = 0;
	argv[argc] = NULL;
					
	for (tok = strtok((s8 *)line, delim); tok; tok = strtok(NULL, delim)) 
	{
		if(tok[0] == '/' && tok[1] == '/')
			return argc;
		argv[argc++] = (s8 *)tok;
	}
	return argc;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CCommand::print_error( u32 i_n_err )
{
    switch( i_n_err )
    {
        case eCMD_STATE_INVALID_ARG:
        {
            printf("invalid argument\n");
            break;
        }
        case eCMD_STATE_INVALID_COMMAND:
        {
            printf("invalid command\n");
            break;
        }
        default:
            break;
    }
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
u32 CCommand::strtoul(const s8 *cp, s8 **endp, u32 base)
{
	u64 result = 0,value;

	if (*cp == '0') {
		cp++;
		if ((*cp == 'x') && isxdigit(cp[1])) {
			base = 16;
			cp++;
		}
		if (!base) {
			base = 8;
		}
	}
	if (!base) {
		base = 10;
	}
	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
	    ? toupper(*cp) : *cp)-'A'+10) < base) {
		result = result*base + value;
		cp++;
	}
	if (endp)
		*endp = (s8 *)cp;
	return result;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
s64 CCommand::strtol(const char *cp, s8 **endp, u32 base)
{
	if(*cp=='-')
		return -strtoul(cp+1,endp,base);
	return strtoul(cp,endp,base);
}

/******************************************************************************/
/**
* \brief        parser
* \param        none
* \return       none
* \note
*******************************************************************************/
u32 CCommand::cmd_parse(s8 *pStrCmd)
{
	s8		*argv[20];
	s32 		argc, i;
	u32			status;
	
	status 	= eCMD_STATE_INVALID_COMMAND;
	
	for(i = 0; i < 20; i++)
	{
		argv[i] = NULL;
	}
	
	argc = parse_args(pStrCmd, argv);
	if(argc == 0)
	{
		status = 1;
		return status;
	}
	
	upper_str(argv[0]);

	if(strcmp((s8 *)argv[0], "HELP") == 0) 
	{
		printf("Not supported command.\n");
		status = 0x01;
	}
	else if( strcmp((s8 *)argv[0], "EXIT") == 0) 
	{
		exit();
	}
	else if( strcmp((s8 *)argv[0], "TCP") == 0) 
	{
		status = cmd_test_tcp(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "UDP") == 0) 
	{
		status = cmd_test_udp(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "ENV") == 0) 
	{
		status = cmd_env(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "CAL") == 0) 
	{
		status = cmd_cal(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "LDG") == 0)     //local digital gain 
	{
		status = cmd_logical_digital_gain(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "GC") == 0)     //local digital gain 
	{
		status = cmd_gain_compensation(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "OFFSET") == 0) 
	{
		status = cmd_offset(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "EXP") == 0) 
	{
		status = cmd_exp_req(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "CIPHER") == 0) 
	{
		status = cmd_cipher(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "SVR") == 0) 
	{
		status = cmd_server_test(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "RESEND") == 0) 
	{
		status = cmd_resend(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "FPGA") == 0) 
	{
		status = cmd_fpga(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "ROIC") == 0) 
	{
		status = cmd_roic(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "TP") == 0) 
	{
		status = cmd_test_pattern(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "SYS") == 0) 
	{
		status = cmd_system(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "VER") == 0) 
	{
		status = cmd_version(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "NET") == 0) 
	{
		status = cmd_network(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "DISCOVERY") == 0) 
	{
		status = cmd_discovery(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "FACTORY") == 0) 
	{
		status = cmd_factory(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "DEBUG") == 0) 
	{
		status = cmd_debug(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "AED") == 0) 
	{
		status = cmd_aed(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "STATE") == 0) 
	{
		status = cmd_state(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "TEST") == 0) 
	{
		status = cmd_test(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "UPDATE") == 0) 
	{
		status = cmd_update(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "PM") == 0) 
	{
		status = cmd_power_manage(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "GAIN") == 0) 
	{
		status = cmd_gain(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "BAT") == 0) 
	{
		status = cmd_battery(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "OPER") == 0) 
	{
		status = cmd_oper_info(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "IMG") == 0) 
	{
		status = cmd_image(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "VWSUDO") == 0) 
	{
		status = cmd_uart_display(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "INT") == 0) 
	{
		status = cmd_interrupt(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "MLOG") == 0) 
	{
		status = cmd_monitor(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "CRC") == 0) 
	{
		status = cmd_crc(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "PMIC") == 0) 
	{
		status = cmd_pmic(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "DIAG") == 0) 
	{
		status = cmd_diag(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "PHY") == 0) 
	{
		status = cmd_phy(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "TACT") == 0) 
	{
		status = cmd_tact(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "PACKET") == 0) 
	{
		status = cmd_packet(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "AP") == 0) 
	{
		status = cmd_ap(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "WEB") == 0) 
	{
		status = cmd_web(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "OLED") == 0) 
	{
		status = cmd_oled(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "XML") == 0) 
	{
		status = cmd_xml(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "GDEFECT") == 0) 
	{
		status = cmd_generate_defect(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "POW") == 0) 
	{
		status = cmd_power_ctrl(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "DRO") == 0) 
	{
		status = cmd_double_readout(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "IMP") == 0) 
	{
		status = cmd_impact(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "MICOM") == 0) 
	{
		status = cmd_micom(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "EEP") == 0) 
	{
		status = cmd_eeprom(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "PROM") == 0) 
	{
		status = cmd_prom(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "CSET") == 0) 
	{
		status = cmd_custom_set(argc, argv);
	}
	else if( strcmp((s8 *)argv[0], "PANEL") == 0) 
	{
		status = cmd_panel(argc, argv);
	}

	else
	{
		status = cmd_system(argc, argv);
	}
	
	
	switch(status)
	{
		case eCMD_STATE_SUCCESS:
		    break;
		case eCMD_STATE_OK:
			printf("OK\r\n");
			break;
		default:
			//printf("Error : 0x%08X\r\n", status);
			print_error( status );
		break;
	}
	
	return status;
}

/******************************************************************************/
/**
* \brief        constructor
* \param        none
* \return       none
* \note
*******************************************************************************/
void CCommand::exit(void)
{
	m_bis_running = false;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
u32	CCommand::proc(void)
{
	s8 	cmd_str[128];
	u32		ret=1;

	memset( cmd_str, 0, sizeof(cmd_str) );
	s32 cnt = get_line(cmd_str, sizeof(cmd_str));

	if(cnt > 0)
	{
		ret = cmd_parse(cmd_str);
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
u32 CCommand::cmd_test_tcp(int argc,char *argv[])
{
#if 0
    int count = (2560 * 2 * 3072) / 1400 + 1;
    int i;
    
    printf("send count: %d\n", count );
    m_pcomm_c->send_test_packet( 1 );
    for( i = 0; i < count; i++ )
    {
        m_pcomm_c->send_test_packet( 3 );
    }
    m_pcomm_c->send_test_packet( 2 );
#endif
    
    return 0;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_test_udp(int argc,char *argv[])
{
    int count = (2560 * 2 * 3072) / 1400 + 1;
    
    printf("send count: %d\n", count );
#if 0
    int i;
    m_pudp_c->send_test_packet( 1 );
    for( i = 0; i < count; i++ )
    {
        m_pudp_c->send_test_packet( 3 );
    }
    m_pudp_c->send_test_packet( 2 );
#endif    
    return 0;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_env(int argc,char *argv[])
{
	if( argc == 1 )
	{
		m_p_system_c->print();
	}
    else if( argc == 2 )
    {
        if( strcmp( argv[1], "preview" ) == 0 )
        {
            printf("preview: %d\n", m_p_system_c->preview_get_enable());
        }
        else if( strcmp( argv[1], "reset" ) == 0 )
        {
            m_p_system_c->config_reset();
        }
        else if( strcmp( argv[1], "reload" ) == 0 )
        {
        	m_p_net_manager_c->net_state_proc_stop();
            m_p_system_c->config_reload(true);
            m_p_net_manager_c->net_state_proc_start();
        }
    }
    else if( argc == 3 )
    {
        if( strcmp( argv[1], "preview" ) == 0 )
        {
            u16 w_enable = atoi( argv[2] );
            
            if( w_enable > 0 )
            {
                m_p_system_c->preview_set_enable(true);
                printf("preview enable\n");
            }
            else
            {
                m_p_system_c->preview_set_enable(false);
                printf("preview disable\n");
            }
        }
        else if( strcmp( argv[1], "del" ) == 0 )
        {
            VW_FILE_DELETE del_e = (VW_FILE_DELETE)strtoul( argv[2], NULL, 16 );
            m_p_system_c->file_delete(del_e);
        }
    }
    return 0;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_cal(int argc,char *argv[])
{
    if( argc == 1 )
    {
        printf("cal load file_type\n");
        printf("\tfile_type: 1 - gain, 2 - defect\n");
        printf("cal make test_file_name start_value\n");
        printf("cal save file_type\n");
        printf("\tfile_type: 1 - gain, 2 - defect\n");
    }
    else if( argc == 3 )
	{
		u32 n_type;
		n_type = atoi( argv[2] );
	
		m_p_calibration_c->load_start( (CALIBRATION_TYPE)n_type );
	}
    else if( argc == 4 )
    {
    	if( strcmp( argv[1], "make" ) == 0 )
    	{
	    	FILE* fp = fopen( argv[2], "wb");
	    	u32 i;
	    	u16 w_tp = atoi( argv[3] );

			image_size_t image_info;
		
			memset( &image_info, 0, sizeof(image_size_t) );
			m_p_system_c->get_image_size_info( &image_info );
	    	
	    	for( i = 0; i < image_info.width * image_info.height; i++ )
	    	{
	    		w_tp++;
	    		
	    		fwrite( &w_tp, 1, sizeof(u16), fp );
	    		
	    		if( w_tp == 0xFFFF )
	    		{
	    			w_tp = 0;
	    		}
	    	}
	    	fclose( fp );
    	}
    }
    
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_offset(int argc,char *argv[])
{
    if( argc == 1 )
    {
        if( m_p_system_c->offset_get_enable() == true )
        {
            printf("offset is enable.\n");
        }
        else
        {
            printf("offset is disable.\n");
        }
    }
    else if( argc == 2 )
    {
    	bool set;
	    set = (bool)atoi( argv[1] );
	    
	    m_p_system_c->offset_set_enable( set );
    }
    
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_logical_digital_gain(int argc,char *argv[])
{   
    if( argc == 2 )
    {
        if( strcmp( argv[1], "get" ) == 0 )
        {
            u32 n_data = 0;
            n_data = m_p_system_c->get_logical_digital_gain();
            printf( "Get logical_digital_gain: %lf\n", (double)(n_data/1024.0) );
        }
        else
        {
            return eCMD_STATE_INVALID_ARG;
        }        
    }
    else if( argc == 3 )
    {
        if( strcmp( argv[1], "set" ) == 0 )
        {
            double lf_data = strtod(argv[2], NULL);
            printf( "input: %lf\n", lf_data );
            m_p_system_c->set_logical_digital_gain( (u32)(lf_data * 1024.0) );
        }
        else
        {
            return eCMD_STATE_INVALID_ARG;
        }
    }
    else
    {
        return eCMD_STATE_INVALID_ARG;
    }
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_gain_compensation(int argc,char *argv[])
{   
    if( argc == 2 )
    {
        if( strcmp( argv[1], "get" ) == 0 )
        {
            u32 n_data = 0;
            n_data = m_p_system_c->get_gain_compensation();
            printf( "Get logical_digital_gain: %d (%f %%)\n", (n_data), n_data/100.0 );
        }
        else
        {
            return eCMD_STATE_INVALID_ARG;
        }        
    }
    else if( argc == 3 )
    {
        if( strcmp( argv[1], "set" ) == 0 )
        {
            u16 w_data = atoi(argv[2]);
            m_p_system_c->set_gain_compensation( w_data );
            m_p_calibration_c->load_start( eCAL_TYPE_GAIN );
        }
        else
        {
            return eCMD_STATE_INVALID_ARG;
        }
    }
    else
    {
        return eCMD_STATE_INVALID_ARG;
    }
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_exp_req(int argc,char *argv[])
{
    if( argc == 2 )
    {
        if( strcmp( argv[1], "a" ) == 0 )
        {
            m_p_system_c->aed_trigger();
        }
        else
        {
            u16 w_exp_type = (u16) atoi(argv[1]);
            // test for sw trigger
            if( w_exp_type == 0 )
            {
                m_p_system_c->exposure_request_enable();
            }
            else
            {
                m_p_system_c->exposure_request(eTRIGGER_TYPE_SW);
            }
        }
    }
    else
    {
        m_p_system_c->exposure_request();
    }
    return 0;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_cipher(int argc,char *argv[])
{
    if( argc == 1 )
    {
        printf("cipher input_file_name\n");
    }
    else if( argc == 2 )
    {
        FILE* fp;
        int org_size, enc_size, dec_size;
        u8* org;
        u8* enc;
        u8* dec;
        u8 key[8];
        
        CBlowFish cipher;
        
        memcpy( key, "1234qwer", sizeof(key) );
        
        cipher.Initialize(key, sizeof(key));
        
        fp = fopen( argv[1], "r" );
        
        if( fp != NULL )
        {
            org_size = file_get_size( fp );
            org = (u8*) malloc(org_size);
            fread( org, 1, org_size, fp ); 
            fclose( fp );
            
            enc_size = cipher.GetOutputLength(org_size);
            
            enc = (u8*)malloc(enc_size);
            memset( enc, 0, enc_size );
            
            cipher.Encode( org, enc, org_size );
            
            dec_size = cipher.GetOutputLength(enc_size);
            dec = (u8*)malloc(dec_size);
            memset( dec, 0, dec_size );
            cipher.Decode( enc, dec, enc_size );
            
            fp = fopen( "cipher.result", "w" );
            fwrite( dec, 1, dec_size, fp );
            fclose( fp );
            
            printf("org size: %d, enc size: %d, dec size: %d\n", 
                        org_size, enc_size, dec_size);
            
            safe_free(org);
            safe_free(enc);
            safe_free(dec);
        }
    }
    
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_server_test(int argc,char *argv[])
{
    if( argc == 1 )
    {
        printf("send size_of_data\n");
        m_p_control_svr_c->test_packet_send( 1000 );
        m_p_control_svr_c->test_packet_send( 1400 );
        m_p_control_svr_c->test_packet_send( 1500 );
        m_p_control_svr_c->test_packet_send( 3000 );
    }
    else if( argc == 2 )
    {
        u32 size;
        
        size = atoi(argv[1]);
        
        m_p_control_svr_c->test_packet_send( size );
    }

    return 0;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_resend(int argc,char *argv[])
{
	if( argc == 1 )
	{
		printf("resend enable\n");
		printf("resend disable\n");
		
		printf("resend oper id type data\n");
		printf("\t oper 0 = CRC_START\n");
		printf("\t oper 1 = CRC_PROGRESS\n");
		printf("\t oper 2 = CRC_RESULT\n");
		printf("\t oper 3 = START_SEND\n");
		printf("\t oper 4 = STOP_SEND\n");
		printf("\t oper 5 = ACCELERATION\n");
		printf("\t oper 6 = FULL_IMAGE_CRC\n");
		printf("\t oper 7 = BACKUP_IMAGE_IS_SAVED\n");
	}
	else if( argc == 2 )
    {
        if( strcmp(argv[1], "enable") == 0 )
        {
            m_p_control_svr_c->m_p_img_retransfer_c->initialize( true, m_p_system_c->get_image_size_byte() );
        }
        else if( strcmp(argv[1], "disable") == 0 )
        {
            m_p_control_svr_c->m_p_img_retransfer_c->initialize( false, m_p_system_c->get_image_size_byte() );
        }
    }
    else if( argc == 5 )
    {
    	image_retrans_req_t	req_t;
    	
    	req_t.n_operation	= strtoul( argv[1], NULL, 10 );
    	req_t.n_image_id	= strtoul( argv[2], NULL, 10 );
    	req_t.w_image_type	= strtoul( argv[3], NULL, 10 );
    	req_t.w_data		= strtoul( argv[4], NULL, 10 );
    	
    	m_p_control_svr_c->test_resend( &req_t );
    }
    
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_fpga(int argc,char *argv[])
{
    u32 n_state = eCMD_STATE_SUCCESS;
    
    if( argc == 2 )
    {
        if( strcmp(argv[1], "print") == 0 )
        {
            m_p_system_c->dev_reg_print( eDEV_ID_FPGA );
        }
        else if( strcmp(argv[1], "save") == 0 )
        {
            m_p_system_c->dev_reg_save_file( eDEV_ID_FPGA );
        }
        else if( strcmp(argv[1], "init") == 0 )
        {
            m_p_system_c->dev_reg_init(eDEV_ID_FPGA);
        }
        else if( strcmp(argv[1], "pgm") == 0 )
        {
            m_p_system_c->device_pgm();
        }
    }
    else if( argc == 3 )
    {
        if( strcmp(argv[1], "r") == 0 )
        {
            u16 w_addr = strtoul( argv[2], NULL, 10 );
            printf("0x%04X\n", m_p_system_c->dev_reg_read( eDEV_ID_FPGA, w_addr ));
        }
        else if( strcmp(argv[1], "verify") == 0 )
        {
            CPROM* p_prom_c = new CPROM(lprintf);
            
            p_prom_c->verify(argv[2]);
        }
        else if( strcmp(argv[1], "line") == 0 )
        {
        	u8 c_count = (u8)atoi(argv[2]);
            m_p_system_c->set_tg_line_count( c_count );
        }
        else
        {
            n_state = eCMD_STATE_INVALID_ARG;
        }
    }
    else if( argc == 4 )
    {
        if( strcmp(argv[1], "w") == 0 )
        {
            u16 w_addr = strtoul( argv[2], NULL, 10 );
            u16 w_data = strtoul( argv[3], NULL, 10 );
            
            m_p_system_c->dev_reg_write( eDEV_ID_FPGA, w_addr, w_data );
        }
        else if( strcmp(argv[1], "save") == 0 )
        {
            u16 w_addr_start = strtoul( argv[2], NULL, 10 );
            u16 w_addr_end = strtoul( argv[3], NULL, 10 );
            
            m_p_system_c->dev_reg_save( eDEV_ID_FPGA, w_addr_start, w_addr_end );
        }
        else
        {
            n_state = eCMD_STATE_INVALID_ARG;
        }
    }
    return n_state;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_panel(int argc,char *argv[])
{
    u32 n_state = eCMD_STATE_SUCCESS;
    
    if( argc == 2 )
    {
        if( strcmp(argv[1], "print") == 0 )
        {
            m_p_system_c->print_gain_type_table();
        }
        else
        {
            n_state = eCMD_STATE_INVALID_ARG;
        }
    }
    else
    {
        n_state = eCMD_STATE_INVALID_ARG;
    }
    return n_state;
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_prom(int argc,char *argv[])
{
    u32 n_state = eCMD_STATE_SUCCESS;
    
    if( argc == 3 )
    {
		if( strcmp(argv[1], "en") == 0 )
		{
		    bool b_en = (bool)strtoul( argv[2], NULL, 10 );
		    m_p_system_c->prom_access_enable(b_en);
		}
		else if( strcmp(argv[1], "qpi") == 0 )
		{
		    bool b_en = (bool)strtoul( argv[2], NULL, 10 );
		    m_p_system_c->prom_qpi_enable(b_en);
		}
		else
		{
		    n_state = eCMD_STATE_INVALID_ARG;
		}
	}
    else
	{
	    n_state = eCMD_STATE_INVALID_ARG;
	}
	
    return n_state;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_roic(int argc,char *argv[])
{
		u32 n_state = eCMD_STATE_SUCCESS;
		
    if( argc == 2 )
    {
		if( strcmp(argv[1], "info") == 0 )
		{
		    m_p_system_c->dev_info(eDEV_ID_ROIC);
		}
		else if( strcmp(argv[1], "sync") == 0 )
		{
		    m_p_system_c->roic_sync();
		}
		else if( strcmp(argv[1], "cmp") == 0 )
		{
		    m_p_system_c->roic_delay_compensation();
		} 
		else if( strcmp(argv[1], "bit") == 0 )
		{
		    m_p_system_c->roic_bit_alignment();
		}
		else if( strcmp(argv[1], "down") == 0 )
		{
		    m_p_system_c->roic_power_down();
		} 
		else if( strcmp(argv[1], "up") == 0 )
		{
		    m_p_system_c->roic_power_up();
		}
		else
		{
		    n_state = eCMD_STATE_INVALID_ARG;
		}
    }
    else if( argc == 3 )
    {
		if( strcmp(argv[1], "r") == 0 )
		{
		    u16 w_addr = strtoul( argv[2], NULL, 10 );
		    printf("0x%04X\n", m_p_system_c->dev_reg_read( eDEV_ID_ROIC, 0x20000 | w_addr ));
		}
		else if( strcmp(argv[1], "align") == 0 )
		{
			u16 w_retry_num = strtoul( argv[2], NULL, 10 );
		    m_p_system_c->roic_bit_align_test(w_retry_num);
		} 
		else
		{
    		n_state = eCMD_STATE_INVALID_ARG;
		}
    }
    else if( argc == 4 )
    {
		if( strcmp(argv[1], "w") == 0 )
		{
		    u16 w_addr = strtoul( argv[2], NULL, 10 );
		    u16 w_data = strtoul( argv[3], NULL, 10 );
	
		    m_p_system_c->dev_reg_write( eDEV_ID_ROIC, 0x20000 | w_addr, w_data );
		}       
		else if( strcmp(argv[1], "r") == 0 )
		{
		    u8 c_idx = strtoul( argv[2], NULL, 10 );
		    u16 w_addr = strtoul( argv[3], NULL, 10 );
		    printf("0x%04X\n", m_p_system_c->roic_reg_read( c_idx, 0x20000 | w_addr ));
		}
		else
		{
   			n_state = eCMD_STATE_INVALID_ARG;
		}
    }
    else if( argc == 5 )
    {
		if( strcmp(argv[1], "tga") == 0 && strcmp(argv[2], "w") == 0)
		{
			    u16 w_addr = strtoul( argv[3], NULL, 10 );
			    u16 w_data = strtoul( argv[4], NULL, 10 );
		
			    m_p_system_c->dev_reg_write( eDEV_ID_ROIC, w_addr, w_data );    				
		}
		else if( strcmp(argv[1], "tgb") == 0 && strcmp(argv[2], "w") == 0)
		{
			    u16 w_addr = strtoul( argv[3], NULL, 10 );
			    u16 w_data = strtoul( argv[4], NULL, 10 );
		
			    m_p_system_c->dev_reg_write( eDEV_ID_ROIC, w_addr | 0x10000, w_data );    				
		}
		else if( strcmp(argv[1], "w") == 0 )
		{
			u8 c_idx = strtoul( argv[2], NULL, 10 );
		    u16 w_addr = strtoul( argv[3], NULL, 10 );
		    u16 w_data = strtoul( argv[4], NULL, 10 );
	
		    m_p_system_c->roic_reg_write( c_idx, 0x20000 | w_addr, w_data );
		}
		else if( strcmp(argv[1], "test") == 0 )
		{
		    u32 n_test_count = strtoul( argv[2], NULL, 10 );
		    u16 w_addr = strtoul( argv[3], NULL, 10 );
		    u16 w_data = strtoul( argv[4], NULL, 10 );
		    u16 w_read_data = 0;
		    
		    for(u32 i = 0; i < n_test_count; i++)
		   	{
		   		 m_p_system_c->dev_reg_write( eDEV_ID_ROIC, 0x20000 | w_addr, w_data );
		   		 w_read_data = m_p_system_c->dev_reg_read( eDEV_ID_ROIC, 0x20000 | w_addr );
		   		 
		   		 if( w_read_data != w_data)
		   		 {
		   		 	printf("roic write - read test failed, data : 0x%x - 0x%x, count : %d\n",w_data, w_read_data, n_test_count);
		   		 	break;
		   		 }
		   	}
		    
		    printf("roic write - read test complete\n");
	
		}     
    }
    else
	{
	    n_state = eCMD_STATE_INVALID_ARG;
	}
	
    return n_state;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_test_pattern(int argc,char *argv[])
{
    if( argc == 2 )
    {
        u16 w_tp = strtoul( argv[1], NULL, 10 );
        
        if( w_tp >= 0 && w_tp < 10 )
        {
            m_p_system_c->dev_set_test_pattern( w_tp );
        }
        else
        {
            printf("Invalid test pattern(tp type: 1~8, tp off: 0)\n");
            return eCMD_STATE_INVALID_ARG;
        }
    }
    
    return eCMD_STATE_OK;
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_system(int argc,char *argv[])
{
    s8 cmd[256];
    int n_count = argc;
    int i = 1;
    
    memset( cmd, 0, sizeof(cmd) );
    
    if( strcmp( argv[0], "SYS" ) != 0 )
    {
    	lower_str(argv[0]);
    	n_count = argc + 1;
    	i = 0;
    }
    else
    {
    	n_count = argc;
    	i = 1;
    }
    
    while( n_count > 1 )
    {
        strcat( cmd, argv[i] );
        strcat( cmd, " " );
        n_count--;
        i++;
    }
    
    printf("%s\n", cmd);
    system(cmd);
    
    return eCMD_STATE_SUCCESS;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_version(int argc,char *argv[])
{
    manufacture_info_t info_t;
    
    memset( &info_t, 0, sizeof(manufacture_info_t) );
    m_p_system_c->manufacture_info_get( &info_t );
    
    printf("H/W Ver: 0x%02X\n", m_p_system_c->hw_get_ctrl_version());
    printf("Board config: 0x%02X\n", m_p_system_c->get_model());
    printf("Package Version: %s\n", m_p_system_c->config_get(eCFG_VERSION_PACKAGE) );
    printf("Bootloader: %s\n", m_p_system_c->config_get(eCFG_VERSION_BOOTLOADER));
    printf("Kernel: %s\n", m_p_system_c->config_get(eCFG_VERSION_KERNEL));
    printf("Ramdisk: %s\n", m_p_system_c->config_get(eCFG_VERSION_RFS));
    printf("F/W: %s\n", m_p_system_c->config_get(eCFG_VERSION_FW));
    printf("FPGA: %s\n", m_p_system_c->config_get(eCFG_VERSION_FPGA));
    
    return eCMD_STATE_SUCCESS;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_network(int argc,char *argv[])
{
    if( argc == 1 )
    {
        printf("ip: %s\n", m_p_system_c->config_get(eCFG_IP_ADDRESS));
        printf("netmask: %s\n", m_p_system_c->config_get(eCFG_NETMASK));
        printf("gateway: %s\n", m_p_system_c->config_get(eCFG_GATEWAY));
        printf("SSID: %s\n", m_p_system_c->config_get(eCFG_SSID));
        printf("AP Channel: %s\n", m_p_system_c->config_get(eCFG_AP_CHANNEL));
    }
    else if( argc == 2 )
    {
        if( strcmp(argv[1], "mac") == 0 )
        {
            u8 p_mac[6];
            memset( p_mac, 0, sizeof(p_mac) );
            
            m_p_net_manager_c->net_get_mac( p_mac );
            
            printf("mac: %02X:%02X:%02X:%02X:%02X:%02X\n", \
                    p_mac[0], p_mac[1], p_mac[2], \
                    p_mac[3], p_mac[4], p_mac[5]);
        }
        else if( strcmp( argv[1], "save" ) == 0 )
        {
            m_p_system_c->config_save();
        }
        else if( strcmp( argv[1], "help" ) == 0 )
        {
            printf("net save\n");
            printf("net restart\n");
            printf("net ip ip_address\n");
            printf("net netmask net_mask\n");
            printf("net gateway gateway_address\n");
            printf("net ssid ssid_string\n");
            printf("net key key_value\n");
        }
        else if( strcmp( argv[1], "test" ) == 0 )
        {
            m_p_net_manager_c->net_change_ip(eNET_MODE_TETHER);
        }
    }
    else if( argc == 3 )
    {
        if( strcmp( argv[1], "ip" ) == 0 )
        {
            m_p_system_c->config_set(eCFG_IP_ADDRESS, argv[2]);
        }
        else if( strcmp( argv[1], "netmask" ) == 0 )
        {
            m_p_system_c->config_set(eCFG_NETMASK, argv[2]);
        }
        else if( strcmp( argv[1], "gateway" ) == 0 )
        {
            m_p_system_c->config_set(eCFG_GATEWAY, argv[2]);
        }
        else if( strcmp( argv[1], "ssid" ) == 0 )
        {
            m_p_system_c->config_set(eCFG_SSID, argv[2]);
        }
        else if( strcmp( argv[1], "key" ) == 0 )
        {
            m_p_system_c->config_set(eCFG_KEY, argv[2]);
        }
        else if( strcmp( argv[1], "ch" ) == 0 )
        {
            m_p_net_manager_c->net_ap_set_channel((u16)atoi(argv[2]));
        }
    }
        
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_discovery(int argc,char *argv[])
{
    m_p_net_manager_c->net_discovery();
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CCommand::parse_mac( s8* i_p_mac, u8* o_p_mac )
{
	s8*         p_tok;
	const s8*   p_delim = ":";
	u16         i = 0;
	
	for (p_tok = strtok((s8 *)i_p_mac, p_delim); p_tok; p_tok = strtok(NULL, p_delim)) 
	{
		o_p_mac[i++] = strtoul( p_tok, NULL, 16 );
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CCommand::factory_print(void)
{
    u32 n_time = time(NULL);
    struct tm*  p_gmt_tm_t;
	s8 p_mac[32];
    
    p_gmt_tm_t = localtime( (time_t*)&n_time );
	
	m_factory_info_t.n_release_date = n_time;
    p_gmt_tm_t = localtime( (time_t*)&m_factory_info_t.n_release_date );
    
    printf( "date: %04d/%02d/%02d-%02d:%02d:%02d\n", \
            p_gmt_tm_t->tm_year+1900, p_gmt_tm_t->tm_mon+1, p_gmt_tm_t->tm_mday,	\
            p_gmt_tm_t->tm_hour, p_gmt_tm_t->tm_min, p_gmt_tm_t->tm_sec );
    
    printf( "model name: %s\n", m_factory_info_t.p_model_name );
    
    printf( "serial number: %s\n", m_factory_info_t.p_serial_number );
    
    sprintf( p_mac, "%02X:%02X:%02X:%02X:%02X:%02X", \
                m_factory_info_t.p_mac_address[0], m_factory_info_t.p_mac_address[1], \
                m_factory_info_t.p_mac_address[2], m_factory_info_t.p_mac_address[3], \
                m_factory_info_t.p_mac_address[4], m_factory_info_t.p_mac_address[5] );
    printf( "mac address: %s\n", p_mac );
    printf( "effective area(top: %d, left: %d, right: %d, bottom: %d)\n", \
                m_factory_info_t.p_effective_area[eEFFECTIVE_AREA_TOP], \
                m_factory_info_t.p_effective_area[eEFFECTIVE_AREA_LEFT], \
                m_factory_info_t.p_effective_area[eEFFECTIVE_AREA_RIGHT], \
                m_factory_info_t.p_effective_area[eEFFECTIVE_AREA_BOTTOM] );
	
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_factory(int argc,char *argv[])
{
    if( argc == 1 )
    {
        printf("factory load  : loading from EEPROM\n");
        printf("factory save  : save to EEPROM\n");
        printf("factory       : display current settings\n\n");
        printf("factory reset : clear EEPROM\n");
        
        printf("factory model model_name\n");
        printf("factory serial serial_number\n");
        printf("factory mac mac_address\n");
        printf("factory effect top bottom left right\n\n");
        
        factory_print();
    }
    else if( argc == 2 )
    {
        if( strcmp(argv[1], "load") == 0 )
        {
            m_p_system_c->manufacture_info_get( &m_factory_info_t );
            m_p_system_c->manufacture_info_print();
        }
        else if( strcmp(argv[1], "save") == 0 )
        {
            u32 n_time = time(NULL);
	        struct tm*  p_gmt_tm_t;
	        
	        p_gmt_tm_t = localtime( (time_t*)&n_time );
	        
	        if( p_gmt_tm_t->tm_year+1900 < 2014 )
	        {
	            printf("Current system year(%d) is invalid. Check system time.\n", p_gmt_tm_t->tm_year+1900);
	        }
	        else
	        {
	            s8 p_mac[32];
	            
            	m_factory_info_t.n_release_date = n_time;
	            
	            m_p_system_c->manufacture_info_set( &m_factory_info_t );
	            
                sprintf( p_mac, "%02X:%02X:%02X:%02X:%02X:%02X", \
                        m_factory_info_t.p_mac_address[0], m_factory_info_t.p_mac_address[1], \
                        m_factory_info_t.p_mac_address[2], m_factory_info_t.p_mac_address[3], \
                        m_factory_info_t.p_mac_address[4], m_factory_info_t.p_mac_address[5] );
	            
	            sys_command("fw_setenv ethaddr %s", p_mac);
	            sys_command("sync");
	            
	            m_p_system_c->manufacture_info_print();
	        }
	    }
	    else if( strcmp(argv[1], "reset") == 0 )
	    {
	        m_p_system_c->manufacture_info_reset();
	    }
    }
    else if( argc == 3 )
    {
        if( strcmp(argv[1], "mac") == 0 )
        {
            u8 p_mac[6];
            
            parse_mac( argv[2], p_mac );
            
            memcpy( m_factory_info_t.p_mac_address, p_mac, sizeof(p_mac) );
        }
        else if( strcmp(argv[1], "model") == 0 )
        {
            strcpy( m_factory_info_t.p_model_name, argv[2] );
        }
        else if( strcmp(argv[1], "serial") == 0 )
        {
            strcpy( m_factory_info_t.p_serial_number, argv[2] );
        }
    }
    else if( argc == 4 )
    {
        if( strcmp(argv[1], "model") == 0 )
        {
            strcpy( m_factory_info_t.p_model_name, argv[2] );
            strcat( m_factory_info_t.p_model_name, " " );
            strcat( m_factory_info_t.p_model_name, argv[3] );
        }
    }
    else if( argc == 6 )
    {
        if( strcmp(argv[1], "effect") == 0 )
        {
            m_factory_info_t.p_effective_area[eEFFECTIVE_AREA_TOP]      = atoi( argv[2] );
            m_factory_info_t.p_effective_area[eEFFECTIVE_AREA_BOTTOM]   = atoi( argv[3] );
            m_factory_info_t.p_effective_area[eEFFECTIVE_AREA_LEFT]     = atoi( argv[4] );
            m_factory_info_t.p_effective_area[eEFFECTIVE_AREA_RIGHT]    = atoi( argv[5] );
        }
    }

    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_debug(int argc,char *argv[])
{
    if( argc == 1 )
    {
        m_p_control_svr_c->set_debug(true);
        m_p_scu_svr_c->set_debug(true);
    }
    else if( argc == 2 )
    {
        bool set;
        set = (bool)atoi( argv[1] );
        
        m_p_control_svr_c->set_debug( set );
        m_p_scu_svr_c->set_debug( set );
    }
    
    return eCMD_STATE_OK;
}



/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_aed(int argc,char *argv[])
{
	u32 status = eCMD_STATE_OK;
	char* type = NULL;

	int value[4]={0,};
	u8  buf[5]={0,}; 
	u16 w_data, w_addr;

	type = (char*)argv[1];

	if( argc == 1 )
	{
		printf("aed (POT num) (value)\n");
		printf("APD1_BIAS_POT : 0\n");
		printf("APD1_OFFSET_POT : 1\n");
		printf("APD2_BIAS_POT : 2\n");
		printf("APD2_OFFSET_POT : 3\n");

		printf("aed th l (low thresholed value - mV)\n");
		printf("aed th h (high thresholed value - mV)\n");
	}	
	else if(argc == 2)
	{
		if( strcmp( argv[1], "r" ) == 0 )
		{
			u32 n_low_threshold = 0, n_high_threshold = 0;
			value[eAED0_BIAS] = m_p_system_c->m_p_aed_c->read(0, eBIAS);
			value[eAED0_OFFSET] = m_p_system_c->m_p_aed_c->read(0, eOFFSET);
			value[eAED1_BIAS] = m_p_system_c->m_p_aed_c->read(1, eBIAS);
			value[eAED1_OFFSET] = m_p_system_c->m_p_aed_c->read(1, eOFFSET);
			
			n_low_threshold = m_p_system_c->m_p_aed_c->read(0, eAED_VOLT_THR_L);
			n_high_threshold = m_p_system_c->m_p_aed_c->read(0, eAED_VOLT_THR_H);
			printf("AED threshold low :%d, AED threshold high :%d\n", n_low_threshold, n_high_threshold);
			printf("Bias1:%d,Bias2:%d,Offset1:%d,Offset2:%d\n", value[eAED0_BIAS], value[eAED1_BIAS], value[eAED0_OFFSET], value[eAED1_OFFSET]);

		}
		else if( strcmp( argv[1], "on" ) == 0 )
        {
			m_p_system_c->aed_power_control(eGEN_INTERFACE_RAD, eTRIG_SELECT_NON_LINE, 0);
		}
		else if( strcmp( argv[1], "off" ) == 0 )
        {
			m_p_system_c->aed_power_control(eGEN_INTERFACE_RAD, eTRIG_SELECT_ACTIVE_LINE, 0);
		}
		else if( strcmp( argv[1], "start" ) == 0 )
        {
        	m_p_system_c->m_p_aed_c->start();
        }
		else if( strcmp( argv[1], "stop" ) == 0 )
        {
        	m_p_system_c->m_p_aed_c->stop();
        }        	
		else
		{
			status = eCMD_STATE_INVALID_COMMAND;
		}

	}
	else if(argc == 3)
	{
		buf[1] = strtoul((char *)argv[2], NULL, 0);

		if( type[0] == 0x30 || type[0] == 0x31 || type[0] == 0x32 || type[0] == 0x33 )
		{
			buf[0] = type[0] - 0x30;
			status = eCMD_STATE_OK;

			w_addr = buf[0];
			w_data = buf[1];

			printf("aed w_addr : %d, w_data : %d\n", w_addr, w_data);
			
			if(w_addr == 1 || w_addr == 3)
			{
				m_p_system_c->m_p_aed_c->set_aed_offset_value(w_addr>>1, w_data);
			}
			else if(w_addr == 0 || w_addr == 2)
			{
				m_p_system_c->m_p_aed_c->set_aed_bias_value(w_addr>>1, w_data);
			}
		}
		else
		{
			status = eCMD_STATE_INVALID_COMMAND;
		}
		
	}
	else if(argc == 4)
	{
		if( strcmp( argv[1], "th" ) == 0 )
		{
			u32 n_threshold = strtoul( argv[3], NULL, 10 );
			if( strcmp( argv[2], "l" ) == 0 )
			{
				m_p_system_c->m_p_aed_c->write( 0, eAED_VOLT_THR_L, n_threshold );
			}
			else if( strcmp( argv[2], "h" ) == 0 )
			{
				m_p_system_c->m_p_aed_c->write( 0, eAED_VOLT_THR_H, n_threshold );
			}
		}
		else
		{
			status = eCMD_STATE_INVALID_COMMAND;
		}
	}
	else
	{
		status = eCMD_STATE_INVALID_COMMAND;
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
u32 CCommand::cmd_eeprom(int argc,char *argv[])
{
	u16 w_addr = 0;
	u32 n_len = 0;
	u8 p_wdata[EEPROM_TOTAL_SIZE];
	u8 p_rdata[EEPROM_TOTAL_SIZE];
	
    if( argc == 4 )
    {	
        if( strcmp( argv[1], "w" ) == 0 )
        {	
        	w_addr = (u16)strtoul( argv[2], NULL, 10 );
        	p_wdata[0] = (u8)strtoul( argv[3], NULL, 10 );
        	
        	m_p_system_c->eeprom_write(w_addr, p_wdata, 1);
        }
        else if( strcmp( argv[1], "d" ) == 0 )
        {
        	w_addr = (u16)strtoul( argv[2], NULL, 10 );
        	n_len = strtoul( argv[3], NULL, 10 );
        	
        	m_p_system_c->eeprom_read(w_addr, p_rdata, n_len); 
        	
        	u32 n_idx;
        	for(n_idx = 0; n_idx < n_len; n_idx++)
        	{
        		printf("%02X ", p_rdata[n_idx]);
        		if( (n_idx!=0) && ((n_idx % 0x1f)==0) )
   					printf("\n");
   			}
   			printf("\n");
   			
   		}
        else if( strcmp( argv[1], "e" ) == 0 )
        {
        	u32 n_idx;
        	for(n_idx = 0; n_idx < EEPROM_TOTAL_SIZE; n_idx++)
        		p_wdata[n_idx] = 0xff;
        		
        	w_addr = (u16)strtoul( argv[2], NULL, 10 );
        	n_len = strtoul( argv[3], NULL, 10 );
        	
        	m_p_system_c->eeprom_write(w_addr, p_wdata, n_len);
        }
   	}
   	
   	return eCMD_STATE_OK;
}
        
        	

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_state(int argc,char *argv[])
{
    if( argc == 1 )
    {
        m_p_system_c->print_state();
        m_p_system_c->dev_reg_print(eDEV_ID_ADC);
    }
    else if( argc == 3)
    {
        if( strcmp( argv[1], "adc" ) == 0 )
        {
    		u8 c_ch = (u8)strtoul( argv[2], NULL, 10 );
    		 m_p_system_c->adc_read_test(c_ch);
    	}
    }
    
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_test(int argc,char *argv[])
{
	if( argc == 2 )
    {
        CVWDownload download_c("/tmp/test.bin", lprintf);
        
        download_c.self_test(argv[1]);
    }
    
    
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_update(int argc,char *argv[])
{
    if( argc == 4 )
    {
        if( strcmp( argv[1], "start" ) == 0 )
        {
            if( strcmp( argv[2], "fpga" ) == 0 )
            {
                m_p_system_c->update_start( eVW_FILE_ID_FPGA, argv[3] );
            }
            else if( strcmp( argv[2], "boot" ) == 0 )
            {
                m_p_system_c->update_start( eVW_FILE_ID_BOOTLOADER, argv[3] );
            }
            else if( strcmp( argv[2], "kernel" ) == 0 )
            {
                m_p_system_c->update_start( eVW_FILE_ID_KERNEL, argv[3] );
            }
            else if( strcmp( argv[2], "rfs" ) == 0 )
            {
                m_p_system_c->update_start( eVW_FILE_ID_RFS, argv[3] );
            }
            else if( strcmp( argv[2], "app" ) == 0 )
            {
                m_p_system_c->update_start( eVW_FILE_ID_APPLICATION, argv[3] );
            }
            else if( strcmp( argv[2], "dtb" ) == 0 )
            {
                m_p_system_c->update_start( eVW_FILE_ID_DTB, argv[3] );
            }
            else if( strcmp( argv[2], "offset" ) == 0 )
            {
                m_p_system_c->update_start( eVW_FILE_ID_OFFSET, argv[3] );
            }
            else if( strcmp( argv[2], "defect" ) == 0 )
            {
                m_p_system_c->update_start( eVW_FILE_ID_DEFECT_MAP, argv[3] );
            }
            else
            {
                return eCMD_STATE_INVALID_COMMAND;
            }
        }
        else
        {
            return eCMD_STATE_INVALID_COMMAND;
        }
    }
    else if( argc == 3 )
    {
        if( strcmp( argv[1], "stop" ) == 0 )
        {
            if( strcmp( argv[2], "fpga" ) == 0 )
            {
                m_p_system_c->update_stop( eVW_FILE_ID_FPGA );
            }
            else if( strcmp( argv[2], "boot" ) == 0 )
            {
                m_p_system_c->update_stop( eVW_FILE_ID_BOOTLOADER );
            }
            else if( strcmp( argv[2], "kernel" ) == 0 )
            {
                m_p_system_c->update_stop( eVW_FILE_ID_KERNEL );
            }
            else if( strcmp( argv[2], "rfs" ) == 0 )
            {
                m_p_system_c->update_stop( eVW_FILE_ID_RFS );
            }
            else if( strcmp( argv[2], "app" ) == 0 )
            {
                m_p_system_c->update_stop( eVW_FILE_ID_APPLICATION );
            }
            else if( strcmp( argv[2], "dtb" ) == 0 )
            {
                m_p_system_c->update_stop( eVW_FILE_ID_DTB );
            }
            else if( strcmp( argv[2], "offset" ) == 0 )
            {
                m_p_system_c->update_stop( eVW_FILE_ID_OFFSET );
            }
            else if( strcmp( argv[2], "defect" ) == 0 )
            {
                m_p_system_c->update_stop( eVW_FILE_ID_DEFECT_MAP );
            }
            
            
        }
        else if( strcmp( argv[1], "res" ) == 0 )
        {
            bool b_result = false;
            if( strcmp( argv[2], "fpga" ) == 0 )
            {
                b_result = m_p_system_c->update_get_result( eVW_FILE_ID_FPGA );
            }
            else if( strcmp( argv[2], "boot" ) == 0 )
            {
                b_result = m_p_system_c->update_get_result( eVW_FILE_ID_BOOTLOADER );
            }
            else if( strcmp( argv[2], "kernel" ) == 0 )
            {
                b_result = m_p_system_c->update_get_result( eVW_FILE_ID_KERNEL );
            }
            else if( strcmp( argv[2], "rfs" ) == 0 )
            {
                b_result = m_p_system_c->update_get_result( eVW_FILE_ID_RFS );
            }
            else if( strcmp( argv[2], "app" ) == 0 )
            {
                b_result = m_p_system_c->update_get_result( eVW_FILE_ID_APPLICATION );
            }
            else if( strcmp( argv[2], "dtb" ) == 0 )
            {
                b_result = m_p_system_c->update_get_result( eVW_FILE_ID_DTB );
            }
            else if( strcmp( argv[2], "offset" ) == 0 )
            {
                b_result = m_p_system_c->update_get_result( eVW_FILE_ID_OFFSET );
            }
            else if( strcmp( argv[2], "defect" ) == 0 )
            {
                b_result = m_p_system_c->update_get_result( eVW_FILE_ID_DEFECT_MAP );
            }
            
            
            printf("result: %d\n", b_result );
        }
    }
    
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_power_manage(int argc,char *argv[])
{
    if( argc == 3 )
    {
        if( strcmp( argv[1], "sleep" ) == 0 )
        {
            u16 w_start = strtoul( argv[2], NULL, 10 );
            
            if( w_start > 0 )
            {
                //m_p_system_c->dev_power_down();
                m_p_system_c->pm_test_sleep(true);
            }
            else
            {
                //m_p_system_c->dev_power_up();
                m_p_system_c->pm_test_sleep(false);
            }
        }
       	else if( strcmp( argv[1], "led" ) == 0 )
        {
        	u16 w_onoff = strtoul( argv[2], NULL, 10 );
        	m_p_system_c->pm_power_led_ctrl(w_onoff);
        }
    }
    else if( argc == 2 )
    {
        if( strcmp( argv[1], "reboot" ) == 0 )
        {
            m_p_system_c->hw_reboot();
        }
        else if( strcmp( argv[1], "shutdown" ) == 0 )
        {
            m_p_system_c->hw_power_off();
        }
        else if( strcmp( argv[1], "reset" ) == 0 )
        {
            m_p_system_c->pm_reset_sleep_timer();
        }
        else if( strcmp( argv[1], "wake" ) == 0 )
        {
            m_p_system_c->pm_wake_up(true);
        }
        else if( strcmp( argv[1], "show" ) == 0 )
        {
            printf("remain time(sec): %d\n", m_p_system_c->pm_get_remain_sleep_time() );
        }
    }
    else
    {
        printf("pm sleep 0/1\n");
        printf("pm show\n");
        printf("pm reboot\n");
        printf("pm shutdown\n");
        printf("pm wake\n");
        printf("clear pm timer: pm reset\n");
        printf("show pm timer: pm show\n");
    }
    
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_gain(int argc,char *argv[])
{
    float   f_gain;
    u16     w_temp;
    u16     w_gain;
    u16     w_sw_offset;
    u16     w_digital_offset;
    
    if( argc == 4 )
    {
        f_gain              = atof( argv[1] );
        
        w_gain = (u16)f_gain;           // 정수부
        w_temp = (f_gain * 1000) - (w_gain * 1000); // 소수부
        
        w_gain = (w_gain << 10) & 0xFC00;   // 상위  6bit
        w_gain = w_gain | (w_temp & 0x03FF);  // 하위 10bit
        
        w_sw_offset         = atoi( argv[2] );
        w_digital_offset    = atoi( argv[3] );
        
        m_p_system_c->dev_reg_write( eDEV_ID_FPGA, eFPGA_REG_DIGITAL_GAIN, w_gain );
        m_p_system_c->dev_reg_write( eDEV_ID_FPGA, eFPGA_REG_SW_OFFSET, w_sw_offset );
        m_p_system_c->dev_reg_write( eDEV_ID_FPGA, eFPGA_REG_DIGITAL_OFFSET, w_digital_offset );
    }
    else if( argc == 3 )
    {
        f_gain              = atof( argv[1] );
        w_gain = (u16)f_gain;           // 정수부
        w_temp = (f_gain * 1000) - (w_gain * 1000); // 소수부
        
        w_gain = (w_gain << 10) & 0xFC00;   // 상위  6bit
        w_gain = w_gain | (w_temp & 0x03FF);  // 하위 10bit
        
        w_sw_offset         = atoi( argv[2] );
        
        m_p_system_c->dev_reg_write( eDEV_ID_FPGA, eFPGA_REG_DIGITAL_GAIN, w_gain );
        m_p_system_c->dev_reg_write( eDEV_ID_FPGA, eFPGA_REG_SW_OFFSET, w_sw_offset );
    }
    else if( argc == 2 )
    {
        f_gain              = atof( argv[1] );
        
        w_gain = (u16)f_gain;           // 정수부
        w_temp = (f_gain * 1000) - (w_gain * 1000); // 소수부
        
        w_gain = (w_gain << 10) & 0xFC00;   // 상위  6bit
        w_gain = w_gain | (w_temp & 0x03FF);  // 하위 10bit
        
        m_p_system_c->dev_reg_write( eDEV_ID_FPGA, eFPGA_REG_DIGITAL_GAIN, w_gain );
    }
    else
    {
        w_gain = m_p_system_c->dev_reg_read( eDEV_ID_FPGA, eFPGA_REG_DIGITAL_GAIN );
        f_gain = (w_gain >> 10) & 0x003F;      // 정수부
        f_gain = f_gain + ((w_gain & 0x03FF)/1000.0f);  // 소수부
        
        w_sw_offset = m_p_system_c->dev_reg_read( eDEV_ID_FPGA, eFPGA_REG_SW_OFFSET );
        w_digital_offset = m_p_system_c->dev_reg_read( eDEV_ID_FPGA, eFPGA_REG_DIGITAL_OFFSET );
        
        printf("gain gain hw_offset digital_offset\n");
        printf("gain(0x%04X): %2.3f\n", eFPGA_REG_DIGITAL_GAIN, f_gain);
        printf("sw offset(0x%04X): %d\n", eFPGA_REG_SW_OFFSET, w_sw_offset);
        printf("digital offset(0x%04X): %d\n", eFPGA_REG_DIGITAL_OFFSET, w_digital_offset);
    }
    
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_battery(int argc,char *argv[])
{
    s32     n_full;
    s32     n_empty;
    u32		n_idx;

    if( argc == 2 )
    {
        if( strcmp( argv[1], "qs" ) == 0 )
        {
            m_p_system_c->sw_bat_qucik_start(0);
            m_p_system_c->sw_bat_qucik_start(1);
        }
        else
        {
            return eCMD_STATE_INVALID_ARG;
        }
    }
    else if( argc == 3 )
    {
        if( strcmp( argv[1], "qs" ) == 0 )
        {
            u16 idx = strtoul( argv[2], NULL, 10 );
            
            if( idx >= 0 )
            {
                //m_p_system_c->dev_power_down();
                m_p_system_c->sw_bat_qucik_start(idx);
            }
            else
            {
                return eCMD_STATE_INVALID_ARG;
            }
        }
        else
        {
            return eCMD_STATE_INVALID_ARG;
        }
    }
    else if( argc == 4 )
    {
    	n_idx	= atoi( argv[1] );
        n_full  = atoi( argv[2] );
        n_empty = atoi( argv[3] );
        
        m_p_system_c->bat_set_soc_factor( n_idx, n_full, n_empty );
    }
    else
    {
        m_p_system_c->bat_print();
        m_p_system_c->bat_rechg_print();
    }
    
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_oper_info(int argc,char *argv[])
{
    if( argc == 2 )
    {
        if( strcmp( argv[1], "reset" ) == 0 )
        {
            m_p_system_c->oper_info_reset();
        }
        else if( strcmp( argv[1], "load" ) == 0 )
        {
            m_p_system_c->oper_info_read();
            m_p_system_c->oper_info_print();
        }
    }
    else
    {
        m_p_system_c->oper_info_print();
    }
    
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_image(int argc,char *argv[])
{
    if( argc == 2 )
    {
        if( strcmp( argv[1], "l" ) == 0 )
        {
            m_p_system_c->backup_image_make_list();
            printf("\n\nbackup image count: %d\n\n", m_p_system_c->image_get_saved_count());
            sys_command("cat %s", BACKUP_IMAGE_LIST_FILE);
        }
        else if( strcmp( argv[1], "x" ) == 0 )
        {
            if( m_p_web_c != NULL )
            {
            	m_p_web_c->make_backup_image_list( BACKUP_IMAGE_XML_LIST_FILE );
            	sys_command("cat %s", BACKUP_IMAGE_XML_LIST_FILE);
            }
            else
            {
            	printf("Web is not available.");
            }
        }
        else if( strcmp( argv[1], "d" ) == 0 )
        {
            m_p_system_c->backup_image_delete_oldest();
        }
    }
    else if( argc == 3 )
    {
        u32 n_id;
        
        if( strcmp( argv[1], "d" ) == 0 )
        {
            n_id = strtoul( argv[2], NULL, 10 );
            m_p_system_c->backup_image_delete(n_id);
        }
        else if( strcmp( argv[1], "f" ) == 0 )
        {
            s8 p_file_name[256];
            
            memset( p_file_name, 0, sizeof(p_file_name) );
            n_id = strtoul( argv[2], NULL, 10 );
            m_p_system_c->backup_image_find(n_id, p_file_name);
            
            if( strlen( p_file_name ) > 0 )
            {
                printf("0x%08X: %s\n", n_id, p_file_name );
            }
            else
            {
                printf("0x%08X: fail to find\n", n_id );
            }
        }
    }
    else
    {
        printf("img l       ==> print backup image list\n");
        printf("img x       ==> make backup image list with patient info.\n");
        printf("img d id    ==> delete backup image by id\n");
        printf("img f id    ==> find backup image by id\n");
    }
    
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_uart_display(int argc,char *argv[])
{
    if( argc == 2 )
    {
        if(strcmp(argv[1],"icewine") == 0)
        {
        	m_p_system_c->uart_display(true);
        }
    }
    if( argc == 1 )
    {
       	m_p_system_c->uart_display(false);
    }
    
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_interrupt(int argc,char *argv[])
{
    u16 w_enable;
    
    if( argc == 2 )
    {
        w_enable  = atoi( argv[1] );
        if( w_enable > 0 )
        {
            m_p_system_c->set_immediate_transfer( true );
        }
        else
        {
            m_p_system_c->set_immediate_transfer( false );
        }
    }
    else
    {
        if( m_p_system_c->get_immediate_transfer() == false )
        {
            printf("interrupt disabled\n");
        }
        else
        {
            printf("interrupt enabled\n");
        }
    }
    
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_monitor(int argc,char *argv[])
{  
    if( argc == 1 )
    {
        printf("mlog x      : mlog print off\n");
        printf("mlog d      : mlog print on\n");
        printf("mlog d sec  : mlog cycle(unit: second)\n");
    }
    else if( argc == 2 )
    {
    	if( strcmp(argv[1], "x") == 0 )
        {
            m_p_system_c->monitor_print( false );
        }
        else if( strcmp(argv[1], "d") == 0 )
        {
            m_p_system_c->monitor_print( true );
        }
    }
    else if( argc == 3 )
	{
	    u32     n_time = strtoul( argv[2], NULL, 10 );
	    
	    if( strcmp(argv[1], "d") == 0 )
	    {
	        m_p_system_c->monitor_print( true, n_time );
	    }
    }
    
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_crc(int argc,char *argv[])
{
    u8  p_data[1024];
    u16 i, w_end;
    
    memset( p_data, 0, sizeof(p_data) );
    
    if( argc == 2 )
    {
    	w_end = (u16)atoi( argv[1] );
    	
        for( i = 0; i < w_end; i++ )
        {
            p_data[i*2] = i;
        }
        
        printf("Data\n");
        for( i = 0; i < w_end * 2; i+=2 )
        {
            printf("%02X %02X\n", p_data[i], p_data[i+1] );
        }
        printf("crc: 0x%04X\n", make_crc32( p_data, (w_end*2) ) );
    }
    
    
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_pmic(int argc,char *argv[])
{
    u8 b_addr;
    u8 b_reg;

    if( argc == 2 )
    {
        if( strcmp( argv[1], "rst" ) == 0 )
        {  
       		m_p_system_c->pmic_reset();
    	}	
	}	    
    else if( argc == 3 )
    {
        if( strcmp( argv[1], "r" ) == 0 )
        {
        	b_addr = strtoul( argv[2], NULL, 10 );
        	m_p_system_c->get_pmic_reg(b_addr,&b_reg);
        	printf("0x%02x\n",b_reg);
        }
    }
    else if( argc == 4 )
    {
    	if( strcmp( argv[1], "w" ) == 0 )
        {
        	b_addr = strtoul( argv[2], NULL, 10 );
        	b_reg = strtoul( argv[3], NULL, 10 );
        	m_p_system_c->set_pmic_reg(b_addr,b_reg);
        }

    }
    
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_diag(int argc,char *argv[])
{
    if( argc == 3 )
    {
        if( strcmp( argv[1], "make" ) == 0 )
        {
            if( atoi(argv[2]) > 0 )
            {
                m_p_diagnosis_c->diag_make_list( true );
            }
            else
            {
                m_p_diagnosis_c->diag_make_list( false );
            }
            
            sys_command("cat %s", SELF_DIAG_LIST_FILE);
        }
        else
        {
            u8 c_category   = (u8)atoi(argv[1]);
            u8 c_item_id    = (u8)atoi(argv[2]);
            
            m_p_diagnosis_c->diag_start( (DIAG_CATEGORY)c_category, c_item_id );
            
            sys_command("cat %s", SELF_DIAG_RESULT_FILE);
        }
        
    }
    
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_phy(int argc,char *argv[])
{
    if( argc == 1 )
    {
        printf("phy read ifr_name reg_addr\n");
        printf("phy write ifr_name reg_addr reg_value\n");
        printf("Ex.) read ID of phy #0\n");
        printf("==> phy read eth0 0x02\n");
    }
    else if( argc == 4 )
    {
        if( strcmp( argv[1], "read" ) == 0 )
        {
            u16 w_addr, w_value = 0;
            
            w_addr = strtoul( argv[3], NULL, 10 );
            
            if( vw_net_phy_read( argv[2], w_addr, &w_value ) == true )
            {
                printf("read value: 0x%x\n", w_value );
            }
        }
    }
    else if( argc == 5 )
    {
        if( strcmp( argv[1], "write" ) == 0 )
        {
            u16 w_addr, w_value;
            
            w_addr = strtoul( argv[3], NULL, 10 );
            w_value = strtoul( argv[4], NULL, 10 );
            
            vw_net_phy_write( argv[2], w_addr, w_value );
        }
    }
    
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_tact(int argc,char *argv[])
{
    if( argc == 1 )
    {
        printf("tact: %d\n", m_p_system_c->tact_get_enable());
    }
    else if( argc == 2 )
    {
        bool b_enable;
        
        b_enable   = (bool)atoi(argv[1]);
        m_p_system_c->tact_set_enable( b_enable );
    }
    
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_packet(int argc,char *argv[])
{
    if( argc == 2 )
    {
        u16 w_sequence_number;
        
        w_sequence_number   = (u16)atoi(argv[1]);
        m_p_control_svr_c->packet_test( w_sequence_number );
    }
    
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_ap(int argc,char *argv[])
{
	if( argc == 2 )
    {
        if( strcmp( argv[1], "sw" ) == 0 )
        {  
            m_p_net_manager_c->ap_switching(); 
            return eCMD_STATE_OK;
        }
    }
    else if( argc == 3 )
    {
    	if( strcmp( argv[1], "sw" ) == 0 )
        {
        	bool b_en = (bool)strtoul( argv[2], NULL, 10 );
        	m_p_net_manager_c->m_b_ap_switching_test_mode = b_en;
        	return eCMD_STATE_OK;
        }
    }
    
    return eCMD_STATE_INVALID_ARG;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_web(int argc,char *argv[])
{
    if( argc == 1 )
    {
        printf("web reset\n");
        printf("web defect 0|1\n");
        printf("web defect print\n");
        printf("web gain 0|1\n");
        printf("web patient id name\n");
    }
    else if( argc == 2 )
    {
        if( strcmp( argv[1], "reset" ) == 0 )
        {
            if( m_p_web_c != NULL )
            {
            	m_p_web_c->reset_admin_password();
            }
        }
    }
    else if( argc == 3 )
    {
        if( strcmp( argv[1], "defect" ) == 0 )
        {
            if( strcmp( argv[2], "print" ) == 0 )
            {
            	if( m_p_web_c != NULL )
	            {
	            	m_p_web_c->defect_print();
	            	printf("check /tmp/defect.list\n");
	            }
	        }
	        else
	        {
            	if( m_p_web_c != NULL )
	            {
	            	m_p_web_c->defect_correction_on((bool)atoi(argv[2]));
	            }
	        }
        }
        else if(strcmp( argv[1], "gain" ) == 0 )
        {
            if( m_p_web_c != NULL )
            {
            	m_p_web_c->gain_on((bool)atoi(argv[2]));
            }
        }
    }
    else if( argc == 4 )
    {
        if( strcmp( argv[1], "patient" ) == 0 )
        {
        	patient_info_t info_t;
        	
        	memset( &info_t, 0, sizeof(patient_info_t) );
        	
        	strcpy( info_t.id, argv[2] );
        	strcpy( info_t.name, argv[3] );
        	strcpy( info_t.studyDate, "d" );
        	strcpy( info_t.studyTime, "t" );
        	
        	m_p_web_c->set_patient_info( info_t );
        }
    }
    
    return eCMD_STATE_OK;
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_oled(int argc,char *argv[])
{
	if( argc == 1 )
	{
		printf("oled on\n");
		printf("oled off\n");
		printf("oled test\n");
	}
	else if( argc == 2 )
	{
    	if( strcmp( argv[1], "init" ) == 0 )
    	{
    		m_p_system_c->oled_init();
    		printf("oled display init\n");
    	}
    	if( strcmp( argv[1], "reset" ) == 0 )
    	{
    		m_p_system_c->oled_sw_reset();
    		printf("oled sw reset\n");
    	}
	}
	else if( argc == 4 )
	{
		if( strcmp( argv[1], "w" ) == 0 )
      	{
    		u8 c_addr = 0;
    		u8 c_data = 0;
    		c_addr = (u8)strtoul( argv[2], NULL, 10 );
    		c_data = (u8)strtoul( argv[3], NULL, 10 );
    		m_p_system_c->oled_control(c_addr, &c_data, 1 );
	    }
	    else if( strcmp( argv[1], "err" ) == 0 )
	    {
    		u16 w_err_num = (u16)strtoul( argv[2], NULL, 10 );
    		bool b_err = (bool)strtoul( argv[3], NULL, 10 );
    		m_p_system_c->set_oled_error((OLED_ERROR)w_err_num, b_err);
    	}
	}
	
    return eCMD_STATE_OK;
}	

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_xml(int argc,char *argv[])
{
    if( argc == 1 )
    {
    	printf("xml file_name\n");
    }
    else if( argc == 2 )
    {
    	char test_value[10];
    	
    	sprintf( test_value, "test_value" );
    	
		cout << "Parsing my beer journal..." << endl;
		xml_document<> doc;
		xml_node<> * root_node;
		// Read the xml file into a vector
		ifstream theFile (argv[1]);
		vector<char> buffer((istreambuf_iterator<char>(theFile)), istreambuf_iterator<char>());
		buffer.push_back('\0');
		
		theFile.close();
		
		// Parse the buffer using the xml file parsing library into doc 
		doc.parse<0>(&buffer[0]);
		// Find our root node
		root_node = doc.first_node("MyBeerJournal");
		// Iterate over the brewerys
		for (xml_node<> * brewery_node = root_node->first_node("Brewery"); brewery_node; brewery_node = brewery_node->next_sibling())
		{
		    printf("I have visited %s in %s. ", 
		    	brewery_node->first_attribute("name")->value(),
		    	brewery_node->first_attribute("location")->value());
	            // Interate over the beers
		    for(xml_node<> * beer_node = brewery_node->first_node("Beer"); beer_node; beer_node = beer_node->next_sibling())
		    {
		    	printf("On %s, I tried their %s which is a %s. ", 
		    		beer_node->first_attribute("dateSampled")->value(),
		    		beer_node->first_attribute("name")->value(), 
		    		beer_node->first_attribute("description")->value());
		    	printf("I gave it the following review: %s", beer_node->value());
		    	beer_node->first_attribute("description")->value(test_value, strlen(test_value));
		    }
		    cout << endl;
		}
		
		std::ofstream file;
		file.open("/tmp/test.xml");
		file << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
		file << doc;
		file.close();
    }
    return eCMD_STATE_OK;
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_generate_defect(int argc,char *argv[])
{
#if 1
#define CAL_FILE_DEFECT_STRUCT	SYS_DATA_PATH "defect.bin2"	
	//m_p_calibration_c->dfc_correction_conversion();
	//m_p_system_c->defect_load(CAL_FILE_DEFECT_STRUCT);
	m_p_system_c->defect_print();
#undef CAL_FILE_DEFECT_STRUCT
#endif
    return eCMD_STATE_OK;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_power_ctrl(int argc,char *argv[])
{
	bool b_en;
  	if( argc == 3 )
    {
    	if( strcmp( argv[1], "tg" ) == 0 )
    	{
    		b_en = strtoul( argv[2], NULL, 10 );
    		m_p_system_c->hw_tg_power_enable(b_en);
    	}
    	else if( strcmp( argv[1], "ro" ) == 0 )
    	{
    		b_en = strtoul( argv[2], NULL, 10 );
    		m_p_system_c->hw_ro_power_enable(b_en);
    	}
    }
    return eCMD_STATE_OK;
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_double_readout(int argc,char *argv[])
{
	bool b_en;
	
  	if( argc == 2 )
    {
	    if( strcmp( argv[1], "get" ) == 0 )
    	{
    		printf("Double readout %s\n", (m_p_system_c->is_double_readout_enabled())?"enabled":"disabled");
    	}
    }
  	else if( argc == 3 )
    {
    	if( strcmp( argv[1], "en" ) == 0 )
    	{
    		b_en = strtoul( argv[2], NULL, 10 );
    		m_p_system_c->double_readout_enable(b_en);
    	}
    }
    return eCMD_STATE_OK;
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_impact(int argc,char *argv[])
{
    u32 n_state = eCMD_STATE_SUCCESS;

    
    if( argc == 1 )
    {
        m_p_system_c->impact_print();
    }
    else if( argc == 2 )
    {
        if( strcmp(argv[1], "save") == 0 )
        {
            m_p_system_c->impact_calibartion_offset_save();
        }
        else if( strcmp(argv[1], "cal") == 0 )
        {
            u8 c_progress;
            m_p_system_c->impact_calibration_start();
            
            c_progress = 0;
            while( c_progress < 100 )
            {
                c_progress = m_p_system_c->impact_calibration_get_progress();
                printf("progress: %d%%\n", c_progress);
                sleep_ex(1000000);
            }
            m_p_system_c->impact_calibration_stop();
            
            printf("result: %d\n", m_p_system_c->get_main_angular_offset_cal_process_result());
        }
        else if( strcmp(argv[1], "log") == 0 )
        {
            m_p_system_c->impact_log_save(0);
        }
        else if( strcmp(argv[1], "pos") == 0 )
        {
            s32 p_pos[3];
            memset( p_pos, 0, sizeof(p_pos) );
            printf("pos\n");
            m_p_system_c->get_main_detector_angular_position( &p_pos[0], &p_pos[1], &p_pos[2]);
        }
        else if( strcmp(argv[1], "timestamp") == 0)        
        {
            u32     n_time = time(NULL);
		    struct tm*  p_gmt_tm_t;
    
    		p_gmt_tm_t = localtime( (time_t*)&n_time );
    		
    		printf("time(NULL): %d\n",n_time);
        	printf( "%04d_%02d_%02d_%02d_%02d_%02d_ImpactLog.csv", \
		                            p_gmt_tm_t->tm_year+1900, p_gmt_tm_t->tm_mon+1, p_gmt_tm_t->tm_mday, \
		                            p_gmt_tm_t->tm_hour, p_gmt_tm_t->tm_min, p_gmt_tm_t->tm_sec );
        }
        else if( strcmp(argv[1], "offset") == 0 )
        {
        	s16 w_x, w_y, w_z;
        	m_p_system_c->get_impact_offset(&w_x, &w_y, &w_z);
        	printf("offset x: %d, y: %d, z: %d\n", w_x, w_y, w_z);
        }
    }
    else if( argc == 3 )
    {
        if( strcmp(argv[1], "thr") == 0 )
        {
            u8 c_threshold = (u8)atoi( argv[2] );
            
            m_p_system_c->impact_set_threshold(c_threshold);
            printf("impact threshold data : 0x%02X.\n", m_p_system_c->impact_get_threshold());
        }
        else if( strcmp(argv[1], "r") == 0 )
        {
        	u8 c_addr;
        	u8 c_val;
           	c_addr  = strtoul( argv[2], NULL, 10 );
        	c_val = m_p_system_c->get_impact_reg(c_addr);
        	printf("impact reg read addr : 0x%02X, data : 0x%02X.\n", c_addr, c_val);
        }
        else if( strcmp(argv[1], "x") == 0 )
        {
        	s16 w_data = (s16)strtoul( argv[2], NULL, 10 );
        	m_p_system_c->set_impact_extra_detailed_offset(&w_data, NULL, NULL);
        }
        else if( strcmp(argv[1], "y") == 0 )
        {
        	s16 w_data = (s16)strtoul( argv[2], NULL, 10 );
        	m_p_system_c->set_impact_extra_detailed_offset(NULL, &w_data, NULL);
        }
        else if( strcmp(argv[1], "z") == 0 )
        {
        	s16 w_data = (s16)strtoul( argv[2], NULL, 10 );
        	m_p_system_c->set_impact_extra_detailed_offset(NULL, NULL, &w_data);
        }
        else if( strcmp(argv[1], "lpf") == 0 )
        {
        	u8 c_enable = (u8)strtoul( argv[2], NULL, 10 );
	    	if(c_enable == 1)
	    		m_p_system_c->impact_stable_pos_proc_start();
	    	else
	    		m_p_system_c->impact_stable_pos_proc_stop();
	    }
        else if( strcmp(argv[1], "cal") == 0 )
        {
        	ACC_DIRECTION dir_e = (ACC_DIRECTION)strtoul( argv[2], NULL, 10 );
	    	m_p_system_c->impact_set_cal_direction(dir_e);
	    }	
        else
        {
            n_state = eCMD_STATE_INVALID_ARG;
        }
    }
    else if( argc == 4 )
    {
        if( strcmp(argv[1], "w") == 0 )
        {
            u8 c_addr = strtoul( argv[2], NULL, 10 );
            u8 c_data = strtoul( argv[3], NULL, 10 );           
            
            m_p_system_c->set_impact_reg(c_addr,c_data);
        }
        if( strcmp(argv[1], "filter") == 0 )
        {    
            printf("DEPRECATED\n");
        }       	
        else
        {
            n_state = eCMD_STATE_INVALID_ARG;
        }
    }

    return (n_state == eCMD_STATE_SUCCESS) ? eCMD_STATE_OK:eCMD_STATE_INVALID_ARG;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_micom(int argc,char *argv[])
{
	u32 n_state = eCMD_STATE_INVALID_ARG;

	if( argc == 1 )
	{
		printf("micom adc\n");
        printf("micom state\n");
        printf("micom ver\n");
        printf("micom dump {fram|ctl|info|pkt} #addr --> show 64Bytes dump\n");
        printf("micom eeprom r {fram|ctl|info|pkt} #addr\n");
        printf("micom eeprom w {fram|ctl|info|pkt} #addr #1byte_data\n");
		
        return eCMD_STATE_SUCCESS;
    }			
	else if( argc == 2 )
	{	
		if( strcmp(argv[1], "adc") == 0 )
        {
        	m_p_micom_c->print_adc();
        	
        	return eCMD_STATE_SUCCESS;
        }
		else if( strcmp(argv[1], "state") == 0 )
        {
        	m_p_micom_c->print_state();
        	
        	return eCMD_STATE_SUCCESS;
        }
        else if( strcmp(argv[1], "ver") == 0 )
        {
        	m_p_micom_c->get_version();
        	
        	return eCMD_STATE_SUCCESS;
        }
    }
    else if( argc == 4 )
	{	
        if (strcmp(argv[1], "dump") == 0)
        {
            u16 w_addr = 0;
            int i, j;
            u8 p_data[4][16];
            memset(p_data, 0, 64);
            u8 cnt = 64;

            w_addr = (u16)strtoul(argv[3], NULL, 10);
            w_addr = (w_addr >> 4) << 4; /* 16byte align */

            if (strcmp(argv[2], "fram") == 0)
                m_p_micom_c->eeprom_read(w_addr, (u8 *)&p_data, (u32)cnt, eMICOM_EEPROM_BLK_FRAM);
            else if (strcmp(argv[2], "ctl") == 0)
                m_p_micom_c->eeprom_read(w_addr, (u8 *)&p_data, (u32)cnt, eMICOM_EEPROM_BLK_CTL);
            else if (strcmp(argv[2], "info") == 0)
                m_p_micom_c->eeprom_read(w_addr, (u8 *)&p_data, (u32)cnt, eMICOM_EEPROM_BLK_IMPACT_INFO);
            else if (strcmp(argv[2], "pkt") == 0)
                m_p_micom_c->eeprom_read(w_addr, (u8 *)&p_data, (u32)cnt, eMICOM_EEPROM_BLK_IMPACT_PACKET);

            printf("\teeprom dump[%s]\n", argv[2]);
            printf("\n");
            printf("         00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F        ASCII\n");
            printf("         -------------------------------------------------------------------\n");

            for (i = 0; i < (64 / 16); i++)
            {
                printf("0x%04X>", w_addr);
                for (j = 0; j < 16; j++) /* Display data with Hexa format */
                {
                    printf("%s%02X", (j & 0x07) ? " " : "  ", p_data[i][j]);
                }
                printf(" ");
                for (j = 0; j < 16; j++) /* Display ascii value of data */
                {
                    printf("%s%c", (j & 0x07) ? "" : " ", isgraph(p_data[i][j]) ? p_data[i][j] : '.');
                }
                printf("\n");
                w_addr += 16;
            }

            return eCMD_STATE_SUCCESS;
        }
    }

    else if( argc == 5 )
    {
		if( strcmp(argv[1], "eeprom") == 0
			&& strcmp(argv[2], "r") == 0 )
        {
        	u16 w_addr = 0;
        	u8 c_data = 0;
        	
        	w_addr  = (u16)strtoul( argv[4], NULL, 10 );

            if (strcmp(argv[3], "fram") == 0)
                m_p_micom_c->eeprom_read(w_addr, (u8 *)&c_data, sizeof(c_data), eMICOM_EEPROM_BLK_FRAM);
            else if (strcmp(argv[3], "ctl") == 0)
                m_p_micom_c->eeprom_read(w_addr, (u8 *)&c_data, sizeof(c_data), eMICOM_EEPROM_BLK_CTL);
            else if (strcmp(argv[3], "info") == 0)
                m_p_micom_c->eeprom_read(w_addr, (u8 *)&c_data, sizeof(c_data), eMICOM_EEPROM_BLK_IMPACT_INFO);
            else if (strcmp(argv[3], "pkt") == 0)
                m_p_micom_c->eeprom_read(w_addr, (u8 *)&c_data, sizeof(c_data), eMICOM_EEPROM_BLK_IMPACT_PACKET);

            printf("eeprom read[%s] addr : 0x%04X, data : 0x%02X.\n", argv[3], w_addr, c_data);

            return eCMD_STATE_SUCCESS;
        }
    }
    else if( argc == 6 )
    {
		if( strcmp(argv[1], "eeprom") == 0
			&& strcmp(argv[2], "w") == 0 )
        {
        	u16 w_addr = 0;
        	u8 c_data = 0;
        	
           	w_addr  = (u16)strtoul( argv[4], NULL, 10 );
            c_data = (u8)strtoul(argv[5], NULL, 10);

            if (strcmp(argv[3], "fram") == 0)
                m_p_micom_c->eeprom_write(w_addr, (u8 *)&c_data, sizeof(c_data), eMICOM_EEPROM_BLK_FRAM);
            else if (strcmp(argv[3], "ctl") == 0)
                m_p_micom_c->eeprom_write(w_addr, (u8 *)&c_data, sizeof(c_data), eMICOM_EEPROM_BLK_CTL);
            else if (strcmp(argv[3], "info") == 0)
                m_p_micom_c->eeprom_write(w_addr, (u8 *)&c_data, sizeof(c_data), eMICOM_EEPROM_BLK_IMPACT_INFO);
            else if (strcmp(argv[3], "pkt") == 0)
                m_p_micom_c->eeprom_write(w_addr, (u8 *)&c_data, sizeof(c_data), eMICOM_EEPROM_BLK_IMPACT_PACKET);

            printf("eeprom write[%s] addr : 0x%04X, data : 0x%02X.\n", argv[3], w_addr, c_data);

            return eCMD_STATE_SUCCESS;
        }

    }

	return n_state;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCommand::cmd_custom_set(int argc,char *argv[])
{
	if( argc == 1 )
	{
    	printf("cset 1 data : led ctrl(0: stay on, 1: blink)\n");
    	printf("cset 2 data : station mode signal level change(0: 5 level, 1: 3 level)\n");
    }
  	else if( argc == 3 )
    {
    	u32 n_addr = strtoul( argv[1], NULL, 10 );
    	u16 w_data = strtoul( argv[2], NULL, 10 );
    	m_p_system_c->SetSamsungOnlyFunction(n_addr, w_data);
    }
    
    return eCMD_STATE_OK;
}
