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

#include <sys/ioctl.h>
#include <sys/mman.h>


#include "calibration.h"
#include "vw_time.h"
#include "vw_file.h"
#include "misc.h"
#include "fpga_reg.h"
#include "image_process.h"

#else
#include "typedef.h"
#include "Vw_system.h"
#include "Common_def.h"
#include "Thread.h"
#include "Ioctl.h"
#include "Dev_io.h"
#include "../app/detector/include/calibration.h"
#include "../app/detector/include/fpga_reg.h"
#include "../app/detector/include/message_def.h"
#include <string.h>
#endif

using namespace std;

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/
#define CAL_FILE_OFFSET			SYS_DATA_PATH "offset.bin2"
#define CAL_FILE_GAIN			SYS_DATA_PATH "gain.bin2"
#define CAL_FILE_DEFECT			SYS_DATA_PATH "defect.bin2"
#define CAL_FILE_DEFECT_GRID	SYS_DATA_PATH "defect_grid.bin2"
#define CAL_FILE_DEFECT_GATEOFF	SYS_DATA_PATH "defectForGateOff.bin2"
#define CAL_FILE_DEFECT_STRUCT	SYS_DATA_PATH "defect_structure.bin2"
#define CAL_FILE_DEFECT_ORIGIN	SYS_DATA_PATH "defect.bin"

#define OFFSET_HEADER			"OFDT2..."
#define GAIN_HEADER				"GADT2..."
#define DEFECT_HEADER			"DFDT2..."

#define SYS_OFFSET_BACKUP_PATH  "/mnt/mmc/p2/offset/"


#define DEFECT_INTERPOLATION 4

#define OFFSET_CAL_WAIT_TIME        (5000)

#define CAL_CRC_LEN					(0x100000)	// 1Mbytes

/*******************************************************************************
*	Type Definitions
*******************************************************************************/
typedef struct DEFECT_MAP_FILE_INFO
{
      char fileHeader[12]; // tchar로 수정하지 말 것. 김대환
      int ver;
      char model[50];
      char serial[50];
      int mode;
      int numOfDefectPixel;
      int numOfDefectLine;

}defect_map_header;

typedef struct DEFECT_PIXEL_INFO
{
	int x;
	int y;
	char dummy[16];
}defect_pixel_by_sw;

typedef struct DEFECT_LINE_INFO
{
	int startPos;
	int endPos;
	int linePos;
	int direction;
	char dummy[20];
}defect_line_by_sw;

/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/

/*******************************************************************************
*	Variable Definitions
*******************************************************************************/
const static s8 s_p_path[eCAL_TYPE_MAX][128] \
					= {CAL_FILE_OFFSET, CAL_FILE_DEFECT, CAL_FILE_GAIN};
const static s8 s_p_file_header[eCAL_TYPE_MAX][CAL_HEADER_ID_LEN] \
					= {OFFSET_HEADER, DEFECT_HEADER, GAIN_HEADER};

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
CCalibration::CCalibration( int (*log)(int,int,const char *,...) )
{
    print_log = log;
    strcpy( m_p_log_id, "Calibration   " );
    
    m_p_mmap_t			= NULL;
    
    m_b_load_is_running = false;
    m_p_defect_info_t = NULL;
    m_n_defect_count = 0;
    
    m_p_cal_loaded[eCAL_TYPE_OFFSET]		= false;
    m_p_cal_loaded[eCAL_TYPE_GAIN]			= false;
    m_p_cal_loaded[eCAL_TYPE_DEFECT]		= false;
    
	m_b_load_result = false;
	m_b_offset_enable = false;
	m_b_offset_cal_started = false;
    //offset_init();
    m_n_tg_fd = -1;
	m_load_thread_t = (pthread_t)0;
	m_c_load_progress = 0;
	m_p_cal_mutex_c = NULL;
	m_p_offset = NULL;
	m_n_image_size_byte = 0;
	m_n_image_size_pixel = 0;
	m_n_offset_skip_count = 0;
	m_n_offset_target_count = 0;
	m_n_offset_interval = 0;
	m_n_offset_count = 0;
	m_n_offset_buffer_len = 0;
	m_p_offset_buffer = NULL;
	m_p_offset_wait_c = NULL;
	m_w_offset_cal_count = 0;
	m_p_system_c = NULL;
	
	m_p_gain_integer = NULL;
	m_p_gain_fraction = NULL;
	
	m_p_offset_corr_data = NULL;

	m_b_fw_correction_mode = false;
	m_b_gain_enable = false;
    m_w_gain_average = 1024;
    m_w_digital_offset = 100;
    m_b_max_saturation_enable = true;
    m_b_double_readout = true;
    
    m_b_load_all_running = false;
    m_load_all_thread_t = (pthread_t)0;
    
    print_log(DEBUG, 1, "[%s] CCalibration\n", m_p_log_id);
}

/******************************************************************************/
/**
* \brief        constructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CCalibration::CCalibration(void)
{
}

/******************************************************************************/
/**
* \brief        destructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CCalibration::~CCalibration()
{
    safe_free( m_p_offset );
    safe_free( m_p_offset_buffer );
    safe_delete( m_p_cal_mutex_c );
    safe_free( m_p_defect_info_t);
	
	print_log(DEBUG, 1, "[%s] ~CCalibration\n", m_p_log_id);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCalibration::get_size(CALIBRATION_TYPE i_cal_type_e)
{
	if( i_cal_type_e == eCAL_TYPE_OFFSET
		|| i_cal_type_e == eCAL_TYPE_GAIN )
	{
		return (sizeof(cal_header_t) + m_p_system_c->get_image_size_byte());
	}
	
	print_log(ERROR, 1, "[%s] Not supported cal_type_e(%d)\n", m_p_log_id, i_cal_type_e);
	
	return 0;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CCalibration::is_valid_header( CALIBRATION_TYPE i_type_e, cal_header_t* i_p_header_t )
{
	if( strcmp(s_p_file_header[i_type_e], i_p_header_t->header.p_file_header) == 0 )
	{
		print_header( i_p_header_t );
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
void CCalibration::print_header( cal_header_t* i_p_header_t )
{
	s8      p_date[256];
	
	memset( p_date, 0, sizeof(p_date) );
	get_time_string( i_p_header_t->header.n_date, p_date );

	print_log(INFO, 1, "[%s] header: %s \n",			m_p_log_id, i_p_header_t->header.p_file_header );
	print_log(INFO, 1, "[%s] model: %s \n",				m_p_log_id, i_p_header_t->header.p_model_name );
	print_log(INFO, 1, "[%s] serial: %s \n",			m_p_log_id, i_p_header_t->header.p_serial );
	print_log(INFO, 1, "[%s] verison: %d \n",			m_p_log_id, i_p_header_t->header.w_verison );
	print_log(INFO, 1, "[%s] date: %s \n",				m_p_log_id, p_date );
	print_log(INFO, 1, "[%s] data size: %d \n",			m_p_log_id, i_p_header_t->header.n_data_size );
	print_log(INFO, 1, "[%s] width: %d \n",				m_p_log_id, i_p_header_t->header.n_width );
	print_log(INFO, 1, "[%s] height: %d \n",			m_p_log_id, i_p_header_t->header.n_height );
	print_log(INFO, 1, "[%s] byte per pixel: %d \n",	m_p_log_id, i_p_header_t->header.n_byte_per_pixel );
	
	print_log(INFO, 1, "[%s] temperature: %d \n",		m_p_log_id, i_p_header_t->header.w_temperature );
	print_log(INFO, 1, "[%s] exp. time: %d \n",			m_p_log_id, (i_p_header_t->header.w_exposure_time_high << 16) | i_p_header_t->header.w_exposure_time_low );
	print_log(INFO, 1, "[%s] digital gain: %d \n",		m_p_log_id, i_p_header_t->header.w_digital_gain );
	print_log(INFO, 1, "[%s] pixel average: %d \n",		m_p_log_id, i_p_header_t->header.w_pixel_average );
	print_log(INFO, 1, "[%s] defect count: %d \n",		m_p_log_id, i_p_header_t->header.n_defect_total_count );
	
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CCalibration::set_correction_enable( CALIBRATION_TYPE i_type_e, bool i_b_enable )
{
	if( i_type_e == eCAL_TYPE_OFFSET )
	{
		offset_set_enable( i_b_enable );
	}
	else if( i_type_e == eCAL_TYPE_GAIN )
	{
		gain_set_enable( i_b_enable );
	}
	else if( i_type_e == eCAL_TYPE_DEFECT )
	{
		m_p_system_c->defect_set_enable( i_b_enable );
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CCalibration::get_correction_enable( CALIBRATION_TYPE i_type_e )
{
	if( i_type_e == eCAL_TYPE_OFFSET )
	{
		return offset_get_enable();
	}
	
	if( i_type_e == eCAL_TYPE_GAIN )
	{
		return gain_get_enable();
	}
	
	if( i_type_e == eCAL_TYPE_DEFECT )
	{
		return m_p_system_c->defect_get_enable();
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
void CCalibration::get_shared_memory(void)
{
	void*	p_shared_memory;
	u32		n_length = (sizeof(mmap_t));
	
	p_shared_memory = mmap(NULL, n_length, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, m_n_tg_fd, 0);
    
	if (p_shared_memory == (void *)MAP_FAILED)
	{
		print_log(ERROR, 1, "[%s] Couldn't allocate shared memory.  Error %d\n", m_p_log_id, errno);
		return;
	}
    
	print_log(DEBUG, 1, "[%s] Got shared memory - address %p(len: %d)\n", m_p_log_id, p_shared_memory, n_length);
	
	m_p_mmap_t = (mmap_t*)p_shared_memory;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CCalibration::start(void)
{
	m_n_tg_fd = m_p_system_c->dev_get_fd(eDEV_ID_FPGA);
	
	get_shared_memory();
	
	offset_init();
	offset_load();
	
	load_all_start();
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void* load_routine( void * arg )
{
	CCalibration* cal = (CCalibration *)arg;
	cal->load_proc();
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
void* load_all_routine( void * arg )
{
	CCalibration* cal = (CCalibration *)arg;
	cal->load_all_proc();
	cal->load_all_end();
	pthread_exit( NULL );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CCalibration::load_start(CALIBRATION_TYPE i_type_e)
{
	if( i_type_e >= eCAL_TYPE_MAX )
	{
		print_log(INFO, 1, "[%s] invalid data type: %d\n", m_p_log_id, i_type_e);
		return false;
	}
	
	if( m_b_load_is_running == true )
	{
		print_log(INFO, 1, "[%s] load(%d) proc already started: %d\n", m_p_log_id, i_type_e);
		return false;
	}
	
	m_c_load_progress	= 0;
	m_b_load_result		= false;
	
	set_load(i_type_e, false);
	
	strcpy( m_p_file_path, s_p_path[i_type_e]);
	m_cal_type_e = i_type_e;
	
	prepare_correction();
	
    // launch capture thread
    m_b_load_is_running = true;
    if( pthread_create(&m_load_thread_t, NULL, load_routine, ( void * ) this)!= 0 )
    {
    	print_log( ERROR, 1, "[%s] cal load pthread_create:%s\n", m_p_log_id, strerror( errno ));
    	m_b_load_is_running = false;
    	
    	return false;
    }
    
    pthread_detach(m_load_thread_t);
	
	return true;	
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CCalibration::load_all_start(void)
{
	if( m_b_load_all_running == true )
	{
		print_log(INFO, 1, "[%s] load all proc already started\n", m_p_log_id);
		return false;
	}
	
	if( pthread_attr_init(&m_load_all_thread_attr_t) != 0 ) 
	{
		print_log(INFO, 1, "[%s] load all attribute init failed\n", m_p_log_id);
		return false;
	}
	
	if( pthread_attr_setdetachstate(&m_load_all_thread_attr_t, PTHREAD_CREATE_DETACHED) != 0 )
	{
		print_log(INFO, 1, "[%s] set attribute PTHREAD_CREATE_DETACHED failed\n", m_p_log_id);
		return false;
	}
	
    // launch capture thread
    m_p_system_c->m_b_cal_data_load_end = false;
    m_b_load_all_running = true;
    if( pthread_create(&m_load_all_thread_t, &m_load_all_thread_attr_t, load_all_routine, ( void * ) this)!= 0 )
    {
    	print_log( ERROR, 1, "[%s] cal load all pthread_create:%s\n", m_p_log_id, strerror( errno ));
    	m_b_load_all_running = false;
    	
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
void CCalibration::load_stop(void)
{
    if( m_b_load_is_running == false )
    {
        print_log( ERROR, 1, "[%s] load_proc is already stopped.\n", m_p_log_id);
        return;
    }
    
    m_b_load_is_running = false;
	if( pthread_join( m_load_thread_t, NULL ) != 0 )
	{
		print_log( ERROR, 1,"[%s] cal load  pthread_join: update_proc:%s\n", \
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
void CCalibration::load_proc(void)
{
	u8 c_progress = 0;
	
	print_log(DEBUG, 1, "[%s] start load_proc...(%d)\n", m_p_log_id, syscall( __NR_gettid ) );
	
	if( m_cal_type_e == eCAL_TYPE_DEFECT )
	{
		m_b_load_result = read_correction_data(m_cal_type_e);
	}	
	else if( m_cal_type_e == eCAL_TYPE_OFFSET && m_b_double_readout == true )
	{
		// Double readout offset cal 모드인 경우 무조건 load 성공 처리
		m_b_load_result = true;
	}
	else
	{
		// 저장된 cal data가 없거나, cal data가 invalid한 경우 load 실패
		if( load_data_prepare(m_cal_type_e) == false )
		{
			m_b_load_result = false;
		}
		else
		{
			if(m_b_fw_correction_mode == true )
			{
				m_b_load_result = read_correction_data(m_cal_type_e);
			}
			else
			{				
				if( load_data_write_start(m_cal_type_e) == true )
				{
					while( m_b_load_is_running == true )
					{
						c_progress = load_data_get_progress();
						
						if( c_progress == 100 )
						{					
							m_b_load_result = true;
							print_log(DEBUG, 1, "[%s] cal data write success\n", m_p_log_id );					
							break;
						}
						else
						{
							m_c_load_progress = c_progress;
						}
						
						sleep_ex(300000);		// 300ms
					}
					
					load_data_write_stop();
				}
			}
		}
	}

	m_c_load_progress	= 100;	
	m_b_load_is_running = false;	
	set_load(m_cal_type_e, m_b_load_result);	
	set_correction_enable( m_cal_type_e, m_b_load_result );
	print_log(DEBUG, 1, "[%s] stop load_proc...(result: %d)\n", m_p_log_id, m_p_cal_loaded[m_cal_type_e] );   

}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CCalibration::load_all_proc(void)
{
	load_all();
	m_b_load_all_running = false;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CCalibration::load_data_write_start( CALIBRATION_TYPE i_type_e )
{
	ioctl_data_t	ctl_t;
	u32				n_address = 0;
	
	if( i_type_e == eCAL_TYPE_OFFSET )
	{
		n_address = 0x04000000;
	}
	else if( i_type_e == eCAL_TYPE_GAIN )
	{
		n_address = 0x08000000;
	}
	else
	{
		print_log(DEBUG, 1, "[%s] invalid data type: %d\n", m_p_log_id, i_type_e );
		return false;
	}
	
	memset( &ctl_t, 0, sizeof(ioctl_data_t) );
	
	ctl_t.data[0] = 1;
	memcpy( &ctl_t.data[1], &n_address, 4 );
	
	ioctl( m_n_tg_fd, VW_MSG_IOCTL_TG_LOAD_CAL_DATA, &ctl_t );
	
	return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CCalibration::load_data_write_stop(void)
{
	ioctl_data_t	ctl_t;
	
	memset( &ctl_t, 0, sizeof(ioctl_data_t) );
	
	ioctl( m_n_tg_fd, VW_MSG_IOCTL_TG_LOAD_CAL_DATA, &ctl_t ); 
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u8 CCalibration::load_data_get_progress(void)
{
	u32 n_write = m_p_mmap_t->n_write_offset;
	u32 n_read	= m_p_mmap_t->n_read_offset;
	
	return (u8)((n_write * 100)/n_read);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CCalibration::read_defect_correction_data(void)
{
	FILE*			p_file;
	cal_header_t	file_header_t;
	char			p_file_name[128];
	CImageProcess*	p_image_process;
	bool			b_result = false;
	
	if( m_p_system_c->is_lower_grid_line_support() == true )
	{
		sprintf( p_file_name, "%s", CAL_FILE_DEFECT_GRID );
	}
	else if( m_p_system_c->is_gate_line_defect_enabled() == true )
	{
		sprintf( p_file_name, "%s", CAL_FILE_DEFECT_GATEOFF );
	}
	else
	{
		sprintf( p_file_name, "%s", CAL_FILE_DEFECT );
	}
	
	print_log(ERROR, 1, "[%s] load defect(%s).\n", m_p_log_id, p_file_name);
	
	p_file = fopen(p_file_name, "rb");
	
	if( p_file == NULL )
	{
		print_log(ERROR, 1, "[%s] file open failed.(%s)\n", m_p_log_id, p_file_name);
		return false;
	}
	
	fread( &file_header_t, 1, sizeof(cal_header_t), p_file );
	
	if( is_valid_header( eCAL_TYPE_DEFECT, &file_header_t ) == false )
	{
		print_log(ERROR, 1, "[%s] invalid file header(%s).\n", m_p_log_id, file_header_t.header.p_file_header);
		
		fclose( p_file );
		
		return false;
	}
	
	p_image_process = m_p_system_c->get_image_process();
	
	if( p_image_process != NULL
		&& file_header_t.header.n_defect_total_count >= 0 )
	{
		b_result = p_image_process->defect_load( file_header_t.header.n_defect_total_count, p_file );
	}
	
	fclose( p_file );
	
	return b_result;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CCalibration::load_all(void)
{
	CTime*	p_wait_time;
	int		i;
	bool	b_result = true;
	
	for( i = (int)eCAL_TYPE_OFFSET; i < eCAL_TYPE_MAX; i++ )
	{
		if( load_start( (CALIBRATION_TYPE)i ) == false )
		{
			b_result = false;
			continue;
		}
		
		p_wait_time = new CTime(5000);
		while( m_b_load_is_running == true )
		{
			if( p_wait_time->is_expired() == true )
			{
				load_stop();
				print_log(DEBUG, 1, "[%s] cal data(%d) load timeout\n", m_p_log_id, i );
				break;
			}
			sleep_ex(300000);		// 300ms
		}

		if( m_b_load_is_running == false )
		{
			print_log(DEBUG, 1, "[%s] cal data(%d) load end\n", m_p_log_id, i );
		}
		
		safe_delete( p_wait_time );
		
		if( m_b_load_result == false )
		{
			b_result = false;
		}
	}
	
	return b_result;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CCalibration::load_data_prepare( CALIBRATION_TYPE i_type_e )
{
	FILE*			p_file;
	cal_header_t	file_header_t;
	u32				n_valid_file_size, n_file_size;
	

	
	p_file = fopen(s_p_path[i_type_e], "rb");
	
	if( p_file == NULL )
	{
		print_log(ERROR, 1, "[%s] file open failed.(%s)\n", m_p_log_id, s_p_path[i_type_e]);
		return false;
	}
	
	// check size
	n_valid_file_size	= get_size( i_type_e );
	n_file_size			= file_get_size( p_file );
	n_file_size			= n_file_size - 4;	// crc 4 byte
	
	if( n_valid_file_size == 0
		|| n_valid_file_size != n_file_size )
	{
		print_log(ERROR, 1, "[%s] invalid file size(%d/%d)\n", m_p_log_id, n_file_size, n_valid_file_size);
		fclose( p_file );
		return false;
	}
	
	// load data for checking crc
	u8* p_data = (u8*)malloc(n_valid_file_size);
	if( p_data == NULL )
	{
		print_log(ERROR, 1, "[%s] data malloc(size: %d) failed\n", m_p_log_id, n_valid_file_size );
		fclose( p_file );
		return false;
	}
	
	// check crc
	FILE_ERR err;

	err = file_check_crc( p_file, p_data, n_valid_file_size );
	if( err != eFILE_ERR_NO )
	{
		print_log(ERROR, 1, "[%s] invalid file crc(err: %d)\n", m_p_log_id, err);
		fclose( p_file );
		
		safe_free( p_data );
		return false;
	}
	
	fclose( p_file );
	
	// check header validation
	memcpy( &file_header_t, p_data, sizeof(cal_header_t) );
	
	if( is_valid_header( i_type_e, &file_header_t ) == false )
	{
		print_log(ERROR, 1, "[%s] invalid file header(%s).\n", m_p_log_id, file_header_t.header.p_file_header);
		fclose( p_file );
		safe_free( p_data );
		return false;
	}
	
	u32 n_crc;
	n_crc = make_crc32( &p_data[sizeof(cal_header_t)], CAL_CRC_LEN );
	
	if( i_type_e == eCAL_TYPE_GAIN )
	{
		gain_set_pixel_average(file_header_t.header.w_pixel_average);
		m_p_system_c->gain_set_crc(n_crc, CAL_CRC_LEN);
	}
	else if( i_type_e == eCAL_TYPE_OFFSET )
	{
		m_p_system_c->offset_set_crc(n_crc, CAL_CRC_LEN);
	}
	
	// load data to ram
	print_log(DEBUG, 1, "[%s] read data size: %d\n", m_p_log_id, file_header_t.header.n_data_size);
	
	memcpy( m_p_mmap_t->p_data, &p_data[sizeof(cal_header_t)], file_header_t.header.n_data_size );
	
	m_p_mmap_t->n_read_offset	= file_header_t.header.n_data_size;
	m_p_mmap_t->n_write_offset	= 0;
	
	safe_free( p_data );
	return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CCalibration::save(CALIBRATION_TYPE i_type_e)
{
	cal_header_t	header_t;
	FILE*			p_file;
	u8*				p_data;
	u32				n_file_size;
	
	if( i_type_e != eCAL_TYPE_GAIN
		&& i_type_e != eCAL_TYPE_DEFECT )
	{
		print_log(ERROR, 1, "[%s] Not supported type(%d)\n", m_p_log_id, i_type_e);
		return false;
	}
	
	n_file_size = get_size(i_type_e);
	
	memset( &header_t, 0, sizeof(cal_header_t) );
	
	strcpy( header_t.header.p_file_header,	s_p_file_header[i_type_e] );
	strcpy( header_t.header.p_model_name,	m_p_system_c->get_model_name() );
	strcpy( header_t.header.p_serial,		m_p_system_c->get_serial() );
	
	header_t.header.n_date		= time(NULL);
	header_t.header.n_data_size = n_file_size - sizeof(cal_header_t);
	
	header_t.header.n_width				= m_image_size_t.width;
	header_t.header.n_height			= m_image_size_t.height;
	header_t.header.n_byte_per_pixel	= m_image_size_t.pixel_depth;
	
	p_file = fopen("/tmp/tmp.cal", "wb");
	
	if( p_file == NULL )
	{
		print_log(ERROR, 1, "[%s] file open failed.(%s)\n", m_p_log_id, s_p_path[i_type_e]);
		return false;
	}
	
	p_data = (u8*)malloc(n_file_size);
	memset( p_data, 0, n_file_size );
	
	memcpy( p_data, &header_t, sizeof(cal_header_t) );
	
	file_write_bin( p_file, p_data, n_file_size );
	
	fclose( p_file );
	
	safe_free(p_data);
	
	file_copy_to_flash( "/tmp/tmp.cal", s_p_path[i_type_e] );
    
    sys_command("rm /tmp/tmp.cal");
	
	print_log(DEBUG, 1, "[%s] file is saved.(%s)\n", m_p_log_id, s_p_path[i_type_e]);
	
	return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CCalibration::offset_init(void)
{
	m_p_cal_mutex_c = new CMutex;
	sys_command("mkdir %s", SYS_OFFSET_BACKUP_PATH);

	m_p_offset = NULL;
	m_b_offset_enable = false;
	m_n_image_size_byte = 0;
	m_b_offset_cal_started = false;
	m_p_offset_wait_c = NULL;
	
	m_p_system_c->get_image_size_info(&m_image_size_t);
	
	m_n_image_size_pixel	= m_image_size_t.width * m_image_size_t.height;
	m_n_image_size_byte		= m_n_image_size_pixel * m_image_size_t.pixel_depth;
	
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CCalibration::offset_make_header( cal_header_t* o_p_header_t )
{
	cal_header_t header_t;
	memset( &header_t, 0, sizeof(cal_header_t) );
	
	strcpy( header_t.header.p_file_header, OFFSET_HEADER );
	
	strcpy( header_t.header.p_model_name, m_p_system_c->get_model_name() );
	strcpy( header_t.header.p_serial, m_p_system_c->get_serial() );
	
	if( strcmp( m_offset_header_t.header.p_file_header, OFFSET_HEADER ) != 0 )
	{
		// make new header
		header_t.header.w_verison = 1;
	}
	else
	{
		header_t.header.w_verison = m_offset_header_t.header.w_verison;
	}

	header_t.header.n_date			= time(NULL);
	header_t.header.n_data_size		= m_n_image_size_byte;
	header_t.header.n_width			= m_image_size_t.width;
	header_t.header.n_height		= m_image_size_t.height;
	header_t.header.n_byte_per_pixel = 2;
	
    sys_state_t state_t;
    m_p_system_c->get_state( &state_t );
	header_t.header.w_temperature	= state_t.w_temperature;
	
	header_t.header.w_exposure_time_low	= m_p_system_c->dev_reg_read(eDEV_ID_FPGA, eFPGA_REG_EXP_TIME_LOW);
	header_t.header.w_exposure_time_high = m_p_system_c->dev_reg_read(eDEV_ID_FPGA, eFPGA_REG_EXP_TIME_HIGH);
	header_t.header.w_digital_gain	= m_p_system_c->dev_reg_read(eDEV_ID_FPGA, eFPGA_REG_DIGITAL_GAIN);
	
	memcpy( &m_offset_header_t, &header_t, sizeof(cal_header_t) );
	memcpy( o_p_header_t, &header_t, sizeof(cal_header_t) );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CCalibration::offset_get_size(void)
{
    return (m_n_image_size_byte + sizeof(offset_info_t));
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CCalibration::offset_cal_message( OFFSET_CAL_STATE i_state_e, u8* i_p_data, u16 i_w_data_len )
{
    u32 n_state;
    vw_msg_t msg;
    bool b_result;
    
    if( i_w_data_len + sizeof( n_state ) > VW_PARAM_MAX_LEN )
    {
        print_log(ERROR, 1, "[%s] message data lenth is too long(%d/%d).\n", \
                            m_p_log_id, i_w_data_len, VW_PARAM_MAX_LEN );
        return;
    }
    
    n_state = i_state_e;
    
    memset( &msg, 0, sizeof(vw_msg_t) );
    msg.type = (u16)eMSG_OFFSET_CAL_STATE;
    msg.size = sizeof(n_state) + i_w_data_len;
    
    memcpy( &msg.contents[0], &n_state, sizeof(n_state) );
    if( i_w_data_len > 0 )
    {
        memcpy( &msg.contents[sizeof(n_state)], i_p_data, i_w_data_len );
    }
    
    b_result = notify_handler( &msg );
    
    if( b_result == false )
    {
        print_log(ERROR, 1, "[%s] offset_cal_message(%d) failed\n", \
                            m_p_log_id, i_state_e );
    }
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CCalibration::offset_load(void)
{
    print_log(DEBUG, 1, "[%s] load offset data\n", m_p_log_id);
    
	FILE*			p_file;
	cal_header_t	file_header_t;
	
	memset( &m_offset_header_t, 0, sizeof(cal_header_t) );
	
	m_p_offset = (u16*)malloc(m_p_system_c->get_image_size_byte());
	if( m_p_offset == NULL )
	{
		print_log(ERROR, 1, "[%s] data malloc(size: %d) failed\n", m_p_log_id, m_p_system_c->get_image_size_byte() );
		return false;
	}
	
	memset( m_p_offset, 0, m_p_system_c->get_image_size_byte() );

	p_file = fopen(s_p_path[eCAL_TYPE_OFFSET], "rb");
	
	if( p_file == NULL )
	{
		print_log(ERROR, 1, "[%s] file open failed.(%s)\n", m_p_log_id, s_p_path[eCAL_TYPE_OFFSET]);
		return false;
	}
	
	fread( &file_header_t, 1, sizeof(cal_header_t), p_file );
	
	if( m_p_system_c->get_image_size_byte() != file_header_t.header.n_data_size )
	{
		print_log(ERROR, 1, "[%s] file data size error(%d/%d)\n", m_p_log_id, m_p_system_c->get_image_size_byte(), file_header_t.header.n_data_size);
		fclose( p_file );
		return false;
	}
	
	fread( m_p_offset, 1, file_header_t.header.n_data_size, p_file );
	fclose( p_file );
	
	memcpy( &m_offset_header_t, &file_header_t, sizeof(cal_header_t) );
	
	return true;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CCalibration::offset_save(void)
{
    print_log(DEBUG, 1, "[%s] save offset data\n", m_p_log_id);
    
	FILE*   p_file;
	
	p_file = fopen("/tmp/offset.bin2", "wb");

	if( p_file == NULL )
	{
		print_log(ERROR, 1, "[%s] offset.bin open failed\n", m_p_log_id);
		return false;
	}
	
	cal_header_t header_t;
	memset( &header_t, 0, sizeof(cal_header_t) );
	
	offset_make_header( &header_t );

    print_log(DEBUG, 1, "[%s] offset calibration: offset data is created(digital gain: %d, exp time: %d, temp: %2.1f).\n", \
                        m_p_log_id, header_t.header.w_digital_gain, (header_t.header.w_exposure_time_high << 16) | header_t.header.w_exposure_time_low, header_t.header.w_temperature/10.0f	);
	
	u32 n_file_size = get_size( eCAL_TYPE_OFFSET );
	u8* p_data = (u8*)malloc(n_file_size);
	if( p_data == NULL )
	{
		print_log(ERROR, 1, "[%s] data malloc(size: %d) failed\n", m_p_log_id, n_file_size );
		fclose( p_file );
		return false;
	}
	
	memset( p_data, 0, sizeof(n_file_size) );
	memcpy( p_data, &header_t, sizeof(cal_header_t) );
	
	if( m_p_offset != NULL )
	{
		memcpy( &p_data[sizeof(cal_header_t)], m_p_offset, header_t.header.n_data_size );
	}
	
	file_write_bin( p_file, p_data, n_file_size );
	
    fclose( p_file );
	safe_free(p_data);
	
	// save to flash
	FILE_ERR err;
	
	err = file_copy_to_flash( "/tmp/offset.bin2", SYS_DATA_PATH );
    
    sys_command("rm /tmp/offset.bin2");
	
	if( err != eFILE_ERR_NO )
	{
	    print_log(INFO, 1, "[%s] offset.bin save failed\n", m_p_log_id);
	    return false;
    }
	
	// backup offset file
	s8  p_date[256];
	s8  p_offset_path[256];
	s8  p_offset_backup_path[256];
	
	memset( p_date, 0, sizeof(p_date) );
	get_time_string(header_t.header.n_date, p_date);
	
	sprintf( p_offset_path, "%s%s", SYS_DATA_PATH, OFFSET_BIN2_FILE );
	sprintf( p_offset_backup_path, "%soffset_0x%04X.bin2", SYS_OFFSET_BACKUP_PATH, header_t.header.n_date );
	
	if( file_get_count(SYS_OFFSET_BACKUP_PATH, "*.bin2") >= 10 )
	{
	    file_delete_oldest(SYS_OFFSET_BACKUP_PATH);
	}
	
	file_copy_to_flash(p_offset_path, p_offset_backup_path);
	
	print_log(INFO, 1, "[%s] %s write success(backup(%s): %s)\n", m_p_log_id, OFFSET_BIN2_FILE, p_date, p_offset_backup_path);
	
	return true;
	
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
u32 CCalibration::offset_get_crc(void)
{
    return make_crc32( (u8*)m_p_offset, offset_get_size() );
}


/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CCalibration::offset_cal_accumulate( u16* i_p_data, u32 i_n_line_byte_len )
{
    u32 i;
    u32 n_line_pixel_len        = i_n_line_byte_len/(m_image_size_t.pixel_depth);
    
    if( m_b_offset_cal_started == false )
    {
        return true;
    }
    
    // skip offset data
    if( m_n_offset_count < m_n_offset_skip_count )
    {
        m_n_offset_buffer_len += n_line_pixel_len;
        
        if( m_n_offset_buffer_len == m_n_image_size_pixel )
        {
            m_n_offset_count++;
            m_n_offset_buffer_len = 0;
            print_log(INFO, 1, "[%s] offset acqusition skip (acq: %d)\n", \
                                m_p_log_id, m_n_offset_count);
            
            m_p_system_c->capture_end();
            
            offset_cal_next_trigger();

	        cal_lock();
	        safe_delete(m_p_offset_wait_c);
	        m_p_offset_wait_c = new CTime(m_p_system_c->get_exp_wait_time() + OFFSET_CAL_WAIT_TIME);
	        cal_unlock();
        }
        return false;
    }
    
    // accumulate offset data
    cal_lock();
    if( m_p_offset_buffer != NULL )
    {
        for( i = 0; i < n_line_pixel_len; i++ )
        {
            m_p_offset_buffer[m_n_offset_buffer_len + i] += i_p_data[i];
        }
        cal_unlock();
    }
    else
    {
        cal_unlock();
        return true;
    }
    
    m_n_offset_buffer_len += n_line_pixel_len;
    
    if( m_n_offset_buffer_len == m_n_image_size_pixel )
    {
        u32 n_count;
        
        m_n_offset_count++;
        n_count = m_n_offset_count - m_n_offset_skip_count;
        
        print_log(INFO, 1, "[%s] offset acquisition (acq: %d(size: %d))\n", \
                            m_p_log_id, n_count, m_n_offset_buffer_len);
        
        m_n_offset_buffer_len = 0;
        
        cal_lock();
        safe_delete(m_p_offset_wait_c);
        cal_unlock();
        
        m_p_system_c->capture_end();
        
        if( m_n_offset_count >= m_n_offset_target_count )
        {
            print_log(INFO, 1, "[%s] offset acquisition end(%d)\n", \
                                m_p_log_id, n_count);
            
            offset_cal_make();
            offset_cal_upload();
            
            offset_cal_stop();
            offset_cal_message( eOFFSET_CAL_ACQ_COMPLETE, (u8*)&n_count, sizeof(n_count) );
            
            return true;
        }
        else
        {
            m_n_offset_buffer_len = 0;
            offset_cal_message( eOFFSET_CAL_ACQ_COUNT, (u8*)&n_count, sizeof(n_count) );
            
            offset_cal_next_trigger();
        }
        
        cal_lock();
        m_p_offset_wait_c = new CTime(m_p_system_c->get_exp_wait_time() + OFFSET_CAL_WAIT_TIME);
        cal_unlock();
    }
    
    return false;
}

/******************************************************************************/
/**
* \brief        
* \param        
* \return       none
* \note
*******************************************************************************/
bool CCalibration::offset_cal_start( u32 i_n_target_count, u32 i_n_skip_count, u32 i_n_interval )
{
    if( m_b_offset_cal_started == true )
    {
        print_log(ERROR, 1, "[%s] offset calibration is already started.\n", \
                            m_p_log_id);
        offset_cal_message( eOFFSET_CAL_FAILED );
        return false;
    }
    
    if( offset_cal_set( i_n_target_count, i_n_skip_count ) == false )
    {
        offset_cal_message( eOFFSET_CAL_FAILED );
        return false;
    }
    
    if( m_p_system_c->offset_cal_start( i_n_target_count, i_n_skip_count ) == false )
    {
        offset_cal_message( eOFFSET_CAL_FAILED );
        return false;
    }
    
    m_n_offset_interval = i_n_interval;
    
    if( m_n_offset_interval < 3000 )
    {
    	m_n_offset_interval = 3000;
    }
    
    print_log(INFO, 1, "[%s] offset calibration (target: %d, skip: %d, interval: %dms)\n", \
                        m_p_log_id, m_n_offset_target_count, m_n_offset_skip_count, m_n_offset_interval);

    m_b_offset_cal_started = true;
    
    offset_cal_message( eOFFSET_CAL_TRIGGER );

    cal_lock();
    m_p_offset_wait_c = new CTime(m_p_system_c->get_exp_wait_time() + OFFSET_CAL_WAIT_TIME);
    cal_unlock();
    
    return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CCalibration::offset_cal_request( u32 i_n_target_count, u32 i_n_skip_count )
{
    if( m_b_offset_cal_started == true )
    {
        print_log(ERROR, 1, "[%s] offset calibration is already started.\n", \
                            m_p_log_id);
        offset_cal_message( eOFFSET_CAL_FAILED );
        return false;
    }
    
    if( m_p_system_c->pm_reset_sleep_timer() == false )
    {
        print_log(ERROR, 1, "[%s] starting offset calibration is failed(power mode is not normal).\n", \
                            m_p_log_id);
        return false;
    }
    
    u32 p_count[2];
    
    p_count[0] = i_n_target_count;
    p_count[1] = i_n_skip_count;
    
    offset_cal_message(eOFFSET_CAL_REQUEST, (u8*)p_count, sizeof(p_count) );
    
    return true;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CCalibration::offset_cal_make(void)
{
    u32 total_count = m_n_offset_count - m_n_offset_skip_count;
    //u32 total_count = m_n_offset_count;
    
    print_log(INFO, 1, "[%s] offset calibration: make offset data(%d)\n", \
                        m_p_log_id, total_count);
    
    if( (m_n_offset_count <= m_n_offset_skip_count)
        || (m_n_offset_count < m_n_offset_target_count) )
    {
        print_log(INFO, 1, "[%s] invalid offset count\n", m_p_log_id);
        offset_cal_message( eOFFSET_CAL_FAILED );
        return false;
    }
    
    if( m_p_offset_buffer == NULL )
    {
        print_log(ERROR, 1, "[%s] offset buffer is null\n", m_p_log_id);
        offset_cal_message( eOFFSET_CAL_FAILED );
        return false;
    }
    
    u32 i;
    
    // make offset data
    cal_lock();
    if( m_p_offset_buffer != NULL
    	&& m_p_offset != NULL )
    {
        for( i = 0; i < m_n_image_size_pixel; i++ )
        {
            m_p_offset[i] = m_p_offset_buffer[i]/total_count;
        }
    }
    safe_free( m_p_offset_buffer );
    cal_unlock();
    
    offset_save();
    
    m_p_system_c->offset_update_count();
    
    return true;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CCalibration::offset_cal_stop(void)
{
    print_log(INFO, 1, "[%s] offset calibration stop(error: 0x%04X).\n", \
    					m_p_log_id, m_p_system_c->dev_reg_read(eDEV_ID_FPGA, eFPGA_REG_OFFSET_CAP_ERR));

	if( m_b_offset_cal_started == false )
    {
        return;
    }
    
    m_b_offset_cal_started  = false;
    
	m_p_system_c->capture_end();
	m_p_system_c->offset_cal_stop();
    
    print_log(INFO, 1, "[%s] offset calibration stop(%d/%d) (target: %d, skip: %d)\n", \
                        m_p_log_id, m_n_offset_count, m_n_offset_buffer_len, \
                        m_n_offset_target_count, m_n_offset_skip_count);

    //m_p_fpga_c->set_capture_count(m_p_env_c->oper_info_get_capture_count());
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CCalibration::offset_cal_is_timeout(void)
{
    bool b_result = false;
    
    cal_lock();
    if( m_p_offset_wait_c != NULL
        && m_p_offset_wait_c->is_expired() == true )
    {
        b_result = true;
    }
    cal_unlock();
    
    return b_result;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CCalibration::offset_cal_set( u32 i_n_target_count, u32 i_n_skip_count )
{
    offset_cal_reset();
    
    cal_lock();
    m_p_offset_buffer = (u32*) malloc( m_n_image_size_byte * 2 );
    if( m_p_offset_buffer == NULL )
    {
        cal_unlock();
        print_log(ERROR, 1, "[%s] offset buffer malloc failed.\n", m_p_log_id);
        return false;
    }
    
    memset( m_p_offset_buffer, 0, (m_n_image_size_byte * 2) );
    cal_unlock();
    
    m_n_offset_skip_count   = i_n_skip_count;
    m_n_offset_target_count = i_n_skip_count + i_n_target_count;
	//m_n_offset_target_count = i_n_target_count;
    
    print_log(INFO, 1, "[%s] offset calibration (target: %d, skip: %d)\n", \
                        m_p_log_id, m_n_offset_target_count, \
                        m_n_offset_skip_count);
    return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CCalibration::offset_cal_next_trigger(void)
{
    CTime* p_wait_c = NULL;
    
    print_log(DEBUG, 1, "[%s] wait next exp_req for offset cal\n", m_p_log_id);
    
    p_wait_c = new CTime(m_n_offset_interval);
    while( p_wait_c->is_expired() == false )
    {
        sleep_ex(1000000);       // 1s
    }
    safe_delete( p_wait_c );
    
    offset_cal_message( eOFFSET_CAL_TRIGGER );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CCalibration::offset_cal_reset(void)
{
    m_n_offset_buffer_len   = 0;
    m_n_offset_count        = 0;
    m_n_offset_target_count = 0;
    m_n_offset_skip_count   = 0;
    
    cal_lock();
    safe_free( m_p_offset_buffer );
    safe_delete( m_p_offset_wait_c );
    cal_unlock();
    
    print_log(INFO, 1, "[%s] offset calibration reset\n", m_p_log_id);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CCalibration::offset_cal_trigger(void)
{
    if( offset_cal_is_started() == false )
    {
        return false;
    }
    
    return m_p_system_c->offset_cal_trigger();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CCalibration::offset_cal_upload(void)
{
	CTime* p_wait_time;
	u8 c_progress;
	bool b_load_result = true;
	
	print_log(INFO, 1, "[%s] start offset data upload to FPGA\n", m_p_log_id);
	
	if( load_data_prepare(eCAL_TYPE_OFFSET) == false )
	{
		set_load(eCAL_TYPE_OFFSET, b_load_result);
    	set_correction_enable(eCAL_TYPE_OFFSET, b_load_result);
    	return;
	}
	
	if(m_p_system_c->is_battery_saving_mode() == true )
	{
		b_load_result = read_correction_data(eCAL_TYPE_OFFSET);
	}
	else
	{
		if( load_data_write_start(eCAL_TYPE_OFFSET) == false )
		{
			b_load_result = false;
		}
		else
		{
			p_wait_time = new CTime(2000);
			c_progress = 0;
			
			while( c_progress < 100 )
			{
				c_progress = load_data_get_progress();
				
				if( p_wait_time->is_expired() == true )
				{
					b_load_result = false;
					print_log(DEBUG, 1, "[%s] offset data upload timeout!!!\n", m_p_log_id);
					break;
				}
				
				print_log(DEBUG, 1, "[%s] offset data upload to FPGA(progress: %d)\n", m_p_log_id, c_progress);
				sleep_ex(100000);		// 100ms
			}
			safe_delete(p_wait_time);
			
			load_data_write_stop();
			print_log(INFO, 1, "[%s] end offset data upload to FPGA\n", m_p_log_id);
		}
	}
	    
    set_load(eCAL_TYPE_OFFSET, b_load_result);
    set_correction_enable(eCAL_TYPE_OFFSET, b_load_result);
    
}

/******************************************************************************/
/**
 * @brief   
 * @param   noti: 함수 포인터(notify_sw)
 * @return  
 * @note    main.cpp에서 등록한다.
*******************************************************************************/
void CCalibration::set_notify_handler(bool (*noti)(vw_msg_t * msg))
{
    notify_handler = noti;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CCalibration::is_cal_file( VW_FILE_DELETE i_file_delete_e )
{
	if( i_file_delete_e == eVW_FILE_DELETE_ALL
		|| i_file_delete_e == eVW_FILE_DELETE_ALL_CAL_DATA
		|| i_file_delete_e == eVW_FILE_DELETE_OFFSET
		|| i_file_delete_e == eVW_FILE_DELETE_GAIN
		|| i_file_delete_e == eVW_FILE_DELETE_DEFECT_MAP
		|| i_file_delete_e == eVW_FILE_DELETE_OSF_PROFILE
		|| i_file_delete_e == eVW_FILE_DELETE_ALL_NEW_CAL_DATA
		|| i_file_delete_e == eVW_FILE_DELETE_OFFSET_BIN2
		|| i_file_delete_e == eVW_FILE_DELETE_GAIN_BIN2
		|| i_file_delete_e == eVW_FILE_DELETE_DEFECT_MAP_BIN2
		|| i_file_delete_e == eVW_FILE_DELETE_DEFECT_GRID_MAP_BIN2
		|| i_file_delete_e == eVW_FILE_DELETE_DEFECT_GATEOFF_BIN2 )
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
void CCalibration::file_delete( VW_FILE_DELETE i_file_delete_e )
{
    s8  p_cmd[256];
    
    if( i_file_delete_e == eVW_FILE_DELETE_ALL
    	|| i_file_delete_e == eVW_FILE_DELETE_ALL_CAL_DATA )
    {
        m_b_offset_enable = false;
        
        sprintf( p_cmd, "rm %s%s", SYS_DATA_PATH, OFFSET_BIN_FILE );
        sys_command( p_cmd );
        sprintf( p_cmd, "rm %s%s", SYS_DATA_PATH, GAIN_BIN_FILE );
        sys_command( p_cmd );
        sprintf( p_cmd, "rm %s%s", SYS_DATA_PATH, DEFECT_MAP_FILE );
        sys_command( p_cmd );
        sprintf( p_cmd, "rm %s%s", SYS_DATA_PATH, OSF_PROFILE_FILE );
        sys_command( p_cmd );
        
        sprintf( p_cmd, "rm %s%s", SYS_DATA_PATH, OFFSET_BIN2_FILE );
        sys_command( p_cmd );
        sprintf( p_cmd, "rm %s%s", SYS_DATA_PATH, GAIN_BIN2_FILE );
        sys_command( p_cmd );
        sprintf( p_cmd, "rm %s%s", SYS_DATA_PATH, DEFECT_MAP2_FILE );
        sys_command( p_cmd );
        sprintf( p_cmd, "rm %s%s", SYS_DATA_PATH, DEFECT_GRID_MAP2_FILE );
        sys_command( p_cmd );
        sprintf( p_cmd, "rm %s%s", SYS_DATA_PATH, DEFECT_GATEOFF_FILE );
        sys_command( p_cmd );
        
    }
    else if( i_file_delete_e == eVW_FILE_DELETE_ALL_NEW_CAL_DATA )
    {
        sprintf( p_cmd, "rm %s%s", SYS_DATA_PATH, OFFSET_BIN2_FILE );
        sys_command( p_cmd );
        sprintf( p_cmd, "rm %s%s", SYS_DATA_PATH, GAIN_BIN2_FILE );
        sys_command( p_cmd );
        sprintf( p_cmd, "rm %s%s", SYS_DATA_PATH, DEFECT_MAP2_FILE );
        sys_command( p_cmd );
        sprintf( p_cmd, "rm %s%s", SYS_DATA_PATH, DEFECT_GRID_MAP2_FILE );
        sys_command( p_cmd );
        sprintf( p_cmd, "rm %s%s", SYS_DATA_PATH, DEFECT_GATEOFF_FILE );
        sys_command( p_cmd );        
    }
    else if( i_file_delete_e == eVW_FILE_DELETE_OFFSET )
    {
        m_b_offset_enable = false;
        sprintf( p_cmd, "rm %s%s", SYS_DATA_PATH, OFFSET_BIN_FILE );
        sys_command( p_cmd );
    }
    else if( i_file_delete_e == eVW_FILE_DELETE_GAIN )
    {
        sprintf( p_cmd, "rm %s%s", SYS_DATA_PATH, GAIN_BIN_FILE );
        sys_command( p_cmd );
    }
    else if( i_file_delete_e == eVW_FILE_DELETE_DEFECT_MAP )
    {
        sprintf( p_cmd, "rm %s%s", SYS_DATA_PATH, DEFECT_MAP_FILE );
        sys_command( p_cmd );
    }
     else if( i_file_delete_e == eVW_FILE_DELETE_OSF_PROFILE )
    {
        sprintf( p_cmd, "rm %s%s", SYS_DATA_PATH, OSF_PROFILE_FILE );
        sys_command( p_cmd );    	
    }
    else if( i_file_delete_e == eVW_FILE_DELETE_OFFSET_BIN2 )
    {
        m_b_offset_enable = false;
        sprintf( p_cmd, "rm %s%s", SYS_DATA_PATH, OFFSET_BIN2_FILE );
        sys_command( p_cmd );
    }
    else if( i_file_delete_e == eVW_FILE_DELETE_GAIN_BIN2 )
    {
        sprintf( p_cmd, "rm %s%s", SYS_DATA_PATH, GAIN_BIN2_FILE );
        sys_command( p_cmd );
    }
    else if( i_file_delete_e == eVW_FILE_DELETE_DEFECT_MAP_BIN2 )
    {
        sprintf( p_cmd, "rm %s%s", SYS_DATA_PATH, DEFECT_MAP2_FILE );
        sys_command( p_cmd );
    }
    else if( i_file_delete_e == eVW_FILE_DELETE_DEFECT_GRID_MAP_BIN2 )
    {
        sprintf( p_cmd, "rm %s%s", SYS_DATA_PATH, DEFECT_GRID_MAP2_FILE );
        sys_command( p_cmd );
    }
    else if( i_file_delete_e == eVW_FILE_DELETE_DEFECT_GATEOFF_BIN2 )
    {
        sprintf( p_cmd, "rm %s%s", SYS_DATA_PATH, DEFECT_GATEOFF_FILE );
        sys_command( p_cmd );
    }
    else
    {
        print_log(ERROR, 1, "[%s] unknown file type(0x%02X)\n", m_p_log_id, i_file_delete_e);
        return;
    }
    
    sys_command("sync");
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CCalibration::get_enable( CALIBRATION_TYPE i_type_e )
{
	if( i_type_e >= eCAL_TYPE_MAX )
	{
		print_log(INFO, 1, "[%s] invalid data type: %d\n", m_p_log_id, i_type_e);
		return false;
	}
	
	if( is_loaded( i_type_e ) == false )
	{
		print_log(INFO, 1, "[%s] data(%d) is not loaded\n", m_p_log_id, i_type_e);
		return false;
	}
	
	return get_correction_enable(i_type_e);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CCalibration::set_enable( CALIBRATION_TYPE i_type_e, bool i_b_enable )
{
	if( i_type_e >= eCAL_TYPE_MAX )
	{
		print_log(INFO, 1, "[%s] invalid data type: %d\n", m_p_log_id, i_type_e);
		return;
	}
	
	if( is_loaded(i_type_e) == true )
	{
		set_correction_enable( i_type_e, i_b_enable );
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CCalibration::is_loaded( CALIBRATION_TYPE i_type_e )
{
	if( i_type_e >= eCAL_TYPE_MAX )
	{
		print_log(INFO, 1, "[%s] invalid data type: %d\n", m_p_log_id, i_type_e);
		return false;
	}
	
	if(i_type_e == eCAL_TYPE_OFFSET)
	{
		if(m_p_system_c->is_double_readout_enabled() == true)
		{
			return true;
		}
	}
	
	return m_p_cal_loaded[i_type_e];
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CCalibration::set_load( CALIBRATION_TYPE i_type_e, bool i_b_load )
{
	m_p_cal_loaded[i_type_e] = i_b_load;
	
	if( i_type_e == eCAL_TYPE_OFFSET )
	{
		m_p_system_c->offset_set_load( i_b_load );
	}
	else if( i_type_e == eCAL_TYPE_GAIN )
	{
		m_p_system_c->gain_set_load(i_b_load);
	}
}

#if 0
/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CCalibration::dfc_pixel_pre_processing(s32 i_n_x, s32 i_n_y, u8* i_p_dfc_type, defect_pixel_info_t* o_p_defect_info)
{
	s32 n_width = m_image_size_t.width;
	s32 n_height = m_image_size_t.height;
	signed char p_pos_info[48][2] = { 
				{0,-1},{0,1},{-1,0},{1,0},{-1,-1},{1,-1},{-1,1},{1,1},
				{0,-2},{0,2},{-2,0},{2,0},{-2,-1},{2,-1},{-2,1},{2,1},
				{-1,-2},{1,-2},{-1,2},{1,2},{-2,-2},{2,-2},{-2,2},{2,2},
				{0,-3},{0,3},{-3,0},{3,0},{-3,-1},{3,-1},{-3,1},{3,1},
				{-1,-3},{1,-3},{-1,3},{1,3},{-3,-2},{3,-2},{-3,2},{3,2},
				{-2,-3},{2,-3},{-2,3},{2,3},{-3,-3},{3,-3},{-3,3},{3,3}
			};
	
	s32 i;
	//Comment - (2012/09/07, Yang) Ignore over ranged defect
	if(i_n_x < 0 || i_n_x >= n_width || i_n_y < 0 || i_n_y >= n_height)
	{
		printf("i_n_x : %d, i_n_y : %d\n",i_n_x,i_n_y);
		return;
	}
	
	
	u32 n_pos_one_array;
	u32 n_cnt = 0;
	s32 n_max_line = i_n_y;
	
	o_p_defect_info->n_position_x = i_n_x;
	o_p_defect_info->n_position_y = i_n_y;
	o_p_defect_info->b_defect_type = eDEFECT_PIXEL;

	u8 *src_index = (u8*)&o_p_defect_info->n_src_info;
	memset(src_index,48,4);
	
	for( i = 0 ; i < 48 ; i++)
	{
		n_pos_one_array = (i_n_y + p_pos_info[i][1])*n_width + i_n_x + p_pos_info[i][0];
		if( (i_n_y+p_pos_info[i][1]) >= 0 && (i_n_y+p_pos_info[i][1]) < n_height &&\
			(i_n_x+ p_pos_info[i][0]) >= 0 && (i_n_x+ p_pos_info[i][0]) < n_width && i_p_dfc_type[n_pos_one_array] == eDEFECT_NONE)
		{
			src_index[n_cnt++] = i;
			
			if(n_max_line < (i_n_y+p_pos_info[i][1]))
			{
				n_max_line = (i_n_y+p_pos_info[i][1]);
			}
			
			
			if(n_cnt >= DEFECT_INTERPOLATION)
			{
				break;
			}
		}
	}		
	o_p_defect_info->w_available_line = n_max_line;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CCalibration::dfc_vline_pre_processing(s32 i_n_x, s32 i_n_y, u8* i_p_dfc_type, defect_pixel_info_t* o_p_defect_info)
{
	s32 n_width = m_image_size_t.width;
	s32 n_height = m_image_size_t.height;
	if(i_n_x < 0 || i_n_y < 0 || i_n_y >= n_height || i_n_x >= n_width)
	{
		printf("i_n_x : %d, i_n_y : %d\n",i_n_y,i_n_y);
		return true;
	}
	bool b_ret = false;
	
	u32 n_pos_one_array;
	n_pos_one_array = i_n_y * n_width + i_n_x;

	s32 n_offset=0;
	s32 n_chk_pos;
	u16 *p_src_index = (u16*)&o_p_defect_info->n_src_info;
	o_p_defect_info->n_src_info = 0;
	
	while(true)
	{
		n_offset++;
		n_chk_pos = n_pos_one_array - n_offset;
		// goes over limit?
		if((i_n_x - n_offset) <= 0)
			break;

		if(i_p_dfc_type[n_chk_pos] == eDEFECT_NONE)
		{
			p_src_index[0] = n_offset;
			break;
		}
	}

	n_offset=0;
	while(true)
	{
		n_offset++;
		n_chk_pos = n_pos_one_array + n_offset;
		if((i_n_x + n_offset) >= n_width-1)
			break;

		if(i_p_dfc_type[n_chk_pos] == eDEFECT_NONE)
		{
			p_src_index[1] = n_offset;
			break;
		}
	}
	
	if(p_src_index[0] != 0 && p_src_index[1] != 0)
	{
		o_p_defect_info->n_position_x = i_n_x;
		o_p_defect_info->n_position_y = i_n_y;
		o_p_defect_info->b_defect_type = eDEFECT_LINE_V;			
		o_p_defect_info->w_available_line = i_n_y;
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
bool CCalibration::dfc_hline_pre_processing(s32 i_n_x, s32 i_n_y, u8* i_p_dfc_type, defect_pixel_info_t* o_p_defect_info)
{
	s32 n_width = m_image_size_t.width;
	s32 n_height = m_image_size_t.height;
	
	if(i_n_x < 0 || i_n_y < 0 || i_n_x >= n_width || i_n_y >= n_height)
	{
		printf("i_n_x : %d, i_n_y : %d\n",i_n_x,i_n_y);
		return true;
	}
	bool b_ret = false;
	
	u32 n_pos_one_array;
	n_pos_one_array = i_n_y * n_width + i_n_x;
	s32 n_offset=0;
	s32 n_chk_pos;
	
	u16 *p_src_index = (u16*)&o_p_defect_info->n_src_info;
	o_p_defect_info->n_src_info = 0;
	while(true)
	{
		n_offset++;
		n_chk_pos = n_pos_one_array - n_offset*n_width;
		if((i_n_y - n_offset) <= 0)
			break;

		if (i_p_dfc_type[n_chk_pos] == eDEFECT_NONE)
		{
			p_src_index[0] = n_offset;
			break;
		}
	}

	n_offset=0;
	while(true)
	{
		n_offset++;
		n_chk_pos = n_pos_one_array + n_offset*n_width;
		if((i_n_y + n_offset) >= n_height-1)
			break;
		
		if (i_p_dfc_type[n_chk_pos] == eDEFECT_NONE)
		{
			p_src_index[1] = n_offset;
			break;
		}
	}
	if(p_src_index[0] != 0 && p_src_index[1] != 0)
	{
		o_p_defect_info->n_position_x = i_n_x;
		o_p_defect_info->n_position_y = i_n_y;
		o_p_defect_info->b_defect_type = eDEFECT_LINE_H;
		o_p_defect_info->w_available_line = i_n_y + p_src_index[1];
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
void CCalibration::dfc_correction_pre_processing(u32 i_n_pixel_defect_cnt, u16 i_p_defect_info[][2]\
														,u32 i_n_hline_defect_cnt, u16 i_p_hline_defect_info[][3] \
														,u32 i_n_vline_defect_cnt, u16 i_p_vline_defect_info[][3]\
														,u32* o_n_total_defect_cnt, defect_pixel_info_t* o_p_defect_info)
{

	s32 n_width = m_image_size_t.width;
	s32 n_height = m_image_size_t.height;
	u8* p_dfc_type = new u8[n_width*n_height];
	
	printf("=============n_width : %d, n_height : %d, m_image_size_t.height : %d\n",n_width,n_height,m_image_size_t.height);
	
	s32 i;
	u32 n_defect_cnt = 0;
	memset(p_dfc_type,eDEFECT_NONE,n_width*m_image_size_t.height*sizeof(u8));
	*o_n_total_defect_cnt = 0;
	
	for( i = 0 ; i < (s32)i_n_pixel_defect_cnt; i++)
	{
		if(i_p_defect_info[i][0] < 0 || i_p_defect_info[i][0]>= n_width || i_p_defect_info[i][1] < 0 || i_p_defect_info[i][1] >= n_height)
			continue;
		p_dfc_type[i_p_defect_info[i][1]*n_width + i_p_defect_info[i][0]] = eDEFECT_PIXEL;
	}
	for( i = 0 ; i < (s32)i_n_hline_defect_cnt ; i++)
	{
		s32 n_start = i_p_hline_defect_info[i][1];
		s32 n_end = i_p_hline_defect_info[i][2];
		s32 y = i_p_hline_defect_info[i][0];
		
		if ( n_end < n_start )
		{
			s32 n_temp = n_end;
			n_end = n_start;
			n_start = n_temp;
		}
		
		printf("------------n_start : %d, n_end: %d\n",n_start,n_end);
		
		if(n_start < 0 || n_end < 0 || y < 0 || n_start >= n_width || n_end >= n_width || y >= n_height)
				continue;
		
		for(s32 x = n_start; x<=n_end; x++)
		{
			if(	p_dfc_type[y*n_width + x] != eDEFECT_NONE)
			{
				continue;
			}	
			p_dfc_type[y*n_width + x] = eDEFECT_LINE_H;
		}		
	}

	for( i = 0 ; i < (s32)i_n_vline_defect_cnt ; i++)
	{
		s32 n_start = i_p_vline_defect_info[i][1];
		s32 n_end = i_p_vline_defect_info[i][2];
		s32 x = i_p_vline_defect_info[i][0];	
		
		if ( n_end < n_start )
		{
			s32 n_temp = n_end;
			n_end = n_start;
			n_start = n_temp;
		}	
		printf("------------n_start : %d, n_end: %d\n",n_start,n_end);
		if(x < 0 || n_start < 0 || n_end < 0 || x >= n_width || n_start >= n_height || n_end >= n_height)
			continue;
				
		for(s32 y = n_start; y<=n_end; y++)
		{
			
			if(	p_dfc_type[y*n_width + x] == eDEFECT_LINE_H)
			{
				// case : crossed pixel by defect-lines
				p_dfc_type[y*n_width + x] = eDEFECT_PIXEL;
			}
			else if( p_dfc_type[y*n_width + x] != eDEFECT_NONE) 
			{
				continue;
			}
			p_dfc_type[y*n_width + x] = eDEFECT_LINE_V;
		}
	}

	for( i = 0 ; i < n_height ; i++)
	{
		s32 j;
		for ( j = 0; j < n_width ; j++)
		{
			u8 c_type = p_dfc_type[i*n_width + j];
			if(c_type == eDEFECT_NONE)
			{
				continue;
			}
			else if(c_type == eDEFECT_PIXEL)
			{
				dfc_pixel_pre_processing(j,i,p_dfc_type,&o_p_defect_info[n_defect_cnt]);
				n_defect_cnt++;
			}
			else if(c_type == eDEFECT_LINE_H)
			{
				bool n_ret;
				n_ret = dfc_hline_pre_processing(j,i,p_dfc_type,&o_p_defect_info[n_defect_cnt]);
				if( n_ret == true)
				{
					n_defect_cnt++;
				}
			}
			else if(c_type == eDEFECT_LINE_V)
			{
				bool n_ret;
				n_ret = dfc_vline_pre_processing(j,i,p_dfc_type,&o_p_defect_info[n_defect_cnt]);
				if( n_ret == true)
				{
					n_defect_cnt++;
				}
				
			}
			else
			{
				print_log(ERROR, 1, "[%s] type(%d) is unknown.\n", m_p_log_id, c_type);
			}
		}
	}
	*o_n_total_defect_cnt = n_defect_cnt;
	
	delete [] p_dfc_type;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CCalibration::dfc_correction_conversion()
{
	FILE *fp;
	char* p_file_buf;
	s32 n_file_size;
	s32 n_idx;
	defect_map_header *p_file_header;
	defect_pixel_by_sw *p_pixel_defect_array;
	defect_line_by_sw *p_line_defect_array;
	u16 (*i_p_defect_info)[2];
	u16 (*i_p_hline_defect_info)[3];
	u16 (*i_p_vline_defect_info)[3];
	s32 n_hline_defect_cnt = 0;
	s32 n_vline_defect_cnt = 0;
	u32 n_total_defect_cnt = 0;
	defect_pixel_info_t *p_defect_info_t;
	
	
	fp = fopen(CAL_FILE_DEFECT_ORIGIN,"rb");
	if( fp == NULL)
	{
		printf("------------dfc_correction_conversion failed\n");
		return;
	}
	n_file_size = file_get_size(fp);
	p_file_buf = (char*)malloc(n_file_size);
	fread(p_file_buf,n_file_size,1,fp);
	fclose(fp);
	
	p_file_header = (defect_map_header *)p_file_buf;
	p_pixel_defect_array = (defect_pixel_by_sw *)&p_file_buf[sizeof(defect_map_header)];
	p_line_defect_array = (defect_line_by_sw *)&p_file_buf[sizeof(defect_map_header) + p_file_header->numOfDefectPixel*sizeof(defect_pixel_by_sw)];
	
	printf("------------pixel defect count : %d, line defect count: %d\n",p_file_header->numOfDefectPixel,p_file_header->numOfDefectLine);
	
	
	i_p_defect_info= (u16 (*)[2])malloc(p_file_header->numOfDefectPixel *  2 * sizeof(u16));
	i_p_hline_defect_info = (u16 (*)[3])malloc(p_file_header->numOfDefectLine *  3 * sizeof(u16));
	i_p_vline_defect_info = (u16 (*)[3])malloc(p_file_header->numOfDefectLine *  3 * sizeof(u16));
	
	for ( n_idx = 0 ; n_idx < p_file_header->numOfDefectPixel ; n_idx++)
	{
		i_p_defect_info[n_idx][0] = p_pixel_defect_array[n_idx].x;
		i_p_defect_info[n_idx][1] = p_pixel_defect_array[n_idx].y;
	}
	
	n_total_defect_cnt = p_file_header->numOfDefectPixel;
	
	for ( n_idx = 0 ; n_idx < p_file_header->numOfDefectLine ; n_idx++)
	{
		n_total_defect_cnt+= abs(p_line_defect_array[n_idx].endPos - p_line_defect_array[n_idx].startPos + 1);
		if( p_line_defect_array[n_idx].direction == 0)
		{
			i_p_vline_defect_info[n_vline_defect_cnt][1] = p_line_defect_array[n_idx].startPos;
			i_p_vline_defect_info[n_vline_defect_cnt][2] = p_line_defect_array[n_idx].endPos;
			i_p_vline_defect_info[n_vline_defect_cnt][0] = p_line_defect_array[n_idx].linePos;
			
			n_vline_defect_cnt++;
			
		}
		else
		{
			i_p_hline_defect_info[n_hline_defect_cnt][1] = p_line_defect_array[n_idx].startPos;
			i_p_hline_defect_info[n_hline_defect_cnt][2] = p_line_defect_array[n_idx].endPos;
			i_p_hline_defect_info[n_hline_defect_cnt][0] = p_line_defect_array[n_idx].linePos;
			
			n_hline_defect_cnt++;
		}
	}
	p_defect_info_t = (defect_pixel_info_t*)malloc(n_total_defect_cnt* sizeof(defect_pixel_info_t));
	printf("------------n_vline_defect_cnt : %d, n_vline_defect_cnt: %d\n",n_hline_defect_cnt,n_vline_defect_cnt);
	dfc_correction_pre_processing(p_file_header->numOfDefectPixel,i_p_defect_info,\
								n_hline_defect_cnt,i_p_hline_defect_info,\
								n_vline_defect_cnt,i_p_vline_defect_info,\
								&n_total_defect_cnt,p_defect_info_t);
	
		
	fp = fopen(CAL_FILE_DEFECT_STRUCT,"wb");
	if( fp == NULL)
	{
		printf("------------dfc_correction_conversion failed2\n");
		return;
	}
	calibration_file_header_t header_t;
	memset((char*)&header_t,0,sizeof(calibration_file_header_t));	
	strcpy( header_t.information.p_file_header, "DFDT2...");
	header_t.information.n_defect_total_count = n_total_defect_cnt;
	
	
	
	fwrite((char*)&header_t,sizeof(calibration_file_header_t),1,fp);
	fwrite((char*)p_defect_info_t,n_total_defect_cnt*sizeof(defect_pixel_info_t),1,fp);
	fclose(fp);
	
	free(p_file_buf);
	free(i_p_defect_info);
	free(i_p_hline_defect_info);
	free(i_p_vline_defect_info);
}

#endif


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CCalibration::read_gain_correction_data(u16* i_p_data, u32 i_n_size)
{	
	safe_delete(m_p_gain_integer);
	safe_delete(m_p_gain_fraction);

	// load data for checking crc
	m_p_gain_integer = (u16*)malloc(i_n_size);
	if( m_p_gain_integer == NULL )
	{
		print_log(ERROR, 1, "[%s] gain data(integer) malloc(size: %d) failed\n", m_p_log_id, i_n_size );
		return false;
	}

	// load data for checking crc
	m_p_gain_fraction = (u16*)malloc(i_n_size);
	if( m_p_gain_fraction == NULL )
	{
		print_log(ERROR, 1, "[%s] gain data(fraction) malloc(size: %d) failed\n", m_p_log_id, i_n_size );
		return false;
	}

	memset(m_p_gain_integer, 0, i_n_size);
	memset(m_p_gain_fraction, 0, i_n_size);
	
	u32 n_idx;
	for(n_idx = 0; n_idx < i_n_size>>1; n_idx++)
	{
		if(i_p_data[n_idx] == 0)
		{
			m_p_gain_integer[n_idx] = 1;
			m_p_gain_fraction[n_idx] = 0;
		}	
		else	
		{
			m_p_gain_integer[n_idx] = m_w_gain_average / i_p_data[n_idx];
			m_p_gain_fraction[n_idx] = ((m_w_gain_average << 10) / i_p_data[n_idx]) - (m_p_gain_integer[n_idx] << 10);
			m_p_gain_fraction[n_idx] = m_p_gain_fraction[n_idx] & 0x3ff;
		}		
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
void CCalibration::apply_gain(u16* i_p_img_data, u32 i_n_pixel_offset, u32 i_n_pixel_num)
{	
	if( m_b_fw_correction_mode == false
		|| m_p_cal_loaded[eCAL_TYPE_GAIN] == false 
		|| m_b_gain_enable == false
		|| m_p_gain_integer == NULL
		|| m_p_gain_fraction == NULL)
	{
		return;
	}
	
	u32 n_corrected_img_data = 0;
	u32 n_idx;
	u32 A = 0, B = 0;
	for(n_idx = i_n_pixel_offset; n_idx < i_n_pixel_offset+i_n_pixel_num; n_idx++ )
	{		
		n_corrected_img_data = *(i_p_img_data + n_idx);
		
		if(m_b_max_saturation_enable == true && n_corrected_img_data >= 0xffff)
			continue;
		
		A = m_p_gain_integer[n_idx] * n_corrected_img_data + ((m_p_gain_fraction[n_idx] * n_corrected_img_data) >> 10) + m_w_digital_offset;
		B = m_p_gain_integer[n_idx] * m_w_digital_offset + ((m_p_gain_fraction[n_idx] * m_w_digital_offset) >> 10);
		
		if(A >= B)
			n_corrected_img_data = A - B;
		else
			n_corrected_img_data = 0;
			
		if(n_corrected_img_data > 0xffff)
			*(i_p_img_data + n_idx) = 0xffff;
		else
			*(i_p_img_data + n_idx) = (u16)n_corrected_img_data;
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CCalibration::read_offset_correction_data(u16* i_p_data, u32 i_n_size)
{	
	safe_delete(m_p_offset_corr_data);

	// load data for checking crc
	m_p_offset_corr_data = (u16*)malloc(i_n_size);
	if( m_p_offset_corr_data == NULL )
	{
		print_log(ERROR, 1, "[%s] offset data malloc(size: %d) failed\n", m_p_log_id, i_n_size );
		return false;
	}

	memset(m_p_offset_corr_data, 0, i_n_size);
	memcpy(m_p_offset_corr_data, i_p_data, i_n_size);

	return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CCalibration::apply_offset(u16* i_p_img_data, u32 i_n_pixel_offset, u32 i_n_pixel_num)
{	
	if( m_b_fw_correction_mode == false
		|| m_b_double_readout == true
		|| m_p_cal_loaded[eCAL_TYPE_OFFSET] == false 
		|| m_b_offset_enable == false
		|| m_p_offset_corr_data == NULL)
	{
		return;
	}
	
	s32 n_corrected_img_data = 0;
	u32 n_idx;
	for(n_idx = i_n_pixel_offset; n_idx < i_n_pixel_offset+i_n_pixel_num; n_idx++ )
	{		
		n_corrected_img_data = *(i_p_img_data + n_idx);
		
		if(m_b_max_saturation_enable == true && n_corrected_img_data >= 0xffff)
			continue;

		n_corrected_img_data = n_corrected_img_data - m_p_offset_corr_data[n_idx];
		
		if(n_corrected_img_data < 0)
			*(i_p_img_data + n_idx) = 0;
		else
			*(i_p_img_data + n_idx) = (u16)n_corrected_img_data;
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CCalibration::gain_set_enable(bool i_b_enable)
{
	m_b_gain_enable = i_b_enable;
	m_p_system_c->gain_set_enable(i_b_enable);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CCalibration::gain_get_enable(void)
{
	bool b_gain_enable = m_p_system_c->gain_get_enable();
	
	if(m_b_gain_enable != b_gain_enable)
	{
		print_log(ERROR, 1, "[%s] gain enable : %d is invalid.\n", m_p_log_id, b_gain_enable );
		m_p_system_c->gain_set_enable(m_b_gain_enable);
	}
	
	return m_b_gain_enable;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CCalibration::offset_set_enable(bool i_b_enable)
{
	m_b_offset_enable = i_b_enable;
	m_p_system_c->offset_set_enable(i_b_enable);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CCalibration::offset_get_enable(void)
{
	bool b_offset_enable = m_p_system_c->offset_get_enable();
	
	if(m_b_offset_enable != b_offset_enable)
	{
		print_log(ERROR, 1, "[%s] offset enable : %d is invalid.\n", m_p_log_id, b_offset_enable );
		m_p_system_c->offset_set_enable(m_b_offset_enable);
	}
	
	return m_b_offset_enable;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CCalibration::gain_set_pixel_average( u16 i_w_average )
{
	double lf_gain_compensation = m_p_system_c->dev_reg_read(eDEV_ID_FPGA, eFPGA_REG_GAIN_COMPENSATION) / 10000.0;
	double lf_final_gain_average = ((double)i_w_average) * (1.0 + lf_gain_compensation);
	m_w_gain_average = (u16)(lf_final_gain_average + 0.5);					//소수점 첫째자리에서 반올림 수행 
	m_p_system_c->gain_set_pixel_average(m_w_gain_average);

	//추가 로직. gain compensation 값 보정
	print_log(DEBUG, 1, "[%s] ori_target_level: %d gain_comp: %lf final_target_level: %d \n", m_p_log_id, i_w_average, lf_gain_compensation, m_w_gain_average);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CCalibration::prepare_correction(void)
{
	m_b_fw_correction_mode = m_p_system_c->is_battery_saving_mode();
	m_b_max_saturation_enable = m_p_system_c->is_max_saturation_enabled();
	m_b_double_readout = m_p_system_c->is_double_readout_enabled();
	
	if(m_b_fw_correction_mode == true)
	{
		m_w_digital_offset = m_p_system_c->get_digital_offset();
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CCalibration::read_correction_data(CALIBRATION_TYPE i_cal_type_e)
{
	bool b_read_result = false;
	if(i_cal_type_e == eCAL_TYPE_DEFECT)
	{
		b_read_result = read_defect_correction_data();
	}
	else if(i_cal_type_e == eCAL_TYPE_GAIN)
	{
		b_read_result = read_gain_correction_data((u16*)m_p_mmap_t->p_data, m_p_mmap_t->n_read_offset);
	}
	else if(i_cal_type_e == eCAL_TYPE_OFFSET)
	{
		b_read_result = read_offset_correction_data((u16*)m_p_mmap_t->p_data, m_p_mmap_t->n_read_offset);
	}
	
	return b_read_result;
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CCalibration::load_all_end(void)
{
	m_p_system_c->m_b_cal_data_load_end = true;
	print_log(DEBUG, 1, "[%s] cal data load all end.\n", m_p_log_id );
}
	