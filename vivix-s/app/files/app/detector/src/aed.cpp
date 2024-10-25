/*******************************************************************************
 모  듈 : aed.cpp
 작성자 : from FXMD-1008S aed.cpp
 버  전 : 0.0.1
 설  명 :
 참  고 :

 버  전:
         0.0.1  최초 작성
*******************************************************************************/



/*******************************************************************************
*	Include Files
*******************************************************************************/
#include <stdio.h>
#include <errno.h>          // errno
#include <string.h>            // memset(), memcpy()
#include <iostream>
#include <sys/ioctl.h>  	// ioctl()
#include <fcntl.h>          // open() O_NDELAY

#include "aed.h"
#include "vworks_ioctl.h"

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/
#define DEFAULT_DELAY 2
#define DEFAULT_COUNT 2

#define AED_STABLE_TIME		1000

#define AED_MAX_NUM		2

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
* \param        none
* \return       none
* \note
*******************************************************************************/
CAed::CAed(int (*log)(int,int,const char *,...))
{
	print_log = log;
	strcpy( m_p_log_id, "AED           " );
	
	m_n_spi_fd = -1;
	
	memset(&m_aed_t, 0, sizeof(aed_info_t));
	
	m_p_aed_stable_time_c = NULL;
	m_p_aed_resource_lock_c = new CMutex;

	m_aed_stable_on = false;
	
	m_b_aed_proc_is_running = false;
	m_b_aed_stable_proc_is_running = false;
	
	m_c_hw_ver = 0;

}

/******************************************************************************/
/**
* \brief        destructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CAed::~CAed()
{
	safe_delete(m_p_aed_stable_time_c);
}

/******************************************************************************/
/**
 * @brief
 * @param
 * @return
 * @note
*******************************************************************************/
void CAed::initialize(u8 i_c_hw_ver)
{
	m_c_hw_ver = i_c_hw_ver;
	
	m_p_env_c->aed_info_read(&m_aed_t);
	
	m_n_spi_fd = open("/dev/vivix_spi", O_RDWR);
	
	if( m_n_spi_fd < 0 )
	{
		print_log( ERROR, 1, "[%s] %s open error.\n",m_p_log_id, "/dev/vivix_spi" );
	}

}

/******************************************************************************/
/**
 * @brief
 * @param
 * @return
 * @note
*******************************************************************************/
void CAed::start(void)
{	
	if( m_b_aed_proc_is_running == true )
	{
		print_log( DEBUG, 1, "[%s] AED proc is already running, ignore aed start command.\n", m_p_log_id );	
		return;
	}
	
	sleep_ex(100000);		// aed power enable이후 전원 안정화를 위한 100msec delay
	
    // launch aed thread

    aed_proc_stop();
        
    m_b_aed_proc_is_running = true; 
	if( pthread_create(&m_aed_thread_t, NULL, aed_routine, ( void * ) this)!= 0 )
    {
    	m_b_aed_proc_is_running = false;
    	print_log( ERROR, 1, "[%s] aed_routine pthread_create:%s\n", \
    	                    m_p_log_id, strerror( errno ));
    }

    aed_stable_proc_stop();

	m_p_aed_stable_time_c = new CTime(AED_STABLE_TIME);
	print_log( DEBUG, 1, "[%s] AED stable timer start.\r\n", m_p_log_id );
    // launch aed thread
    m_b_aed_stable_proc_is_running = true;
	if( pthread_create(&m_aed_stable_thread_t, NULL, aed_stable_routine, ( void * ) this)!= 0 )
    {
    	m_b_aed_stable_proc_is_running = false;
    	print_log( ERROR, 1, "[%s] aed_stable_routine pthread_create:%s\n", \
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
void CAed::stop(void)
{
	aed_resource_lock();
	m_aed_stable_on = false;
	aed_resource_unlock();

	aed_stable_proc_stop();
	aed_proc_stop();
	
	
	print_log( DEBUG, 1, "[%s] AED proc stop.\r\n", m_p_log_id );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CAed::aed_proc_stop(void)
{
    if( m_b_aed_proc_is_running )
    {
        m_b_aed_proc_is_running = false;
    	
    	if( pthread_join( m_aed_thread_t, NULL ) != 0 )
    	{
    		print_log( ERROR, 1,"[%s] pthread_join: aed_proc:%s\n", m_p_log_id, strerror( errno ));
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
void CAed::aed_stable_proc_stop(void)
{
	safe_delete(m_p_aed_stable_time_c);
	
    if( m_b_aed_stable_proc_is_running )
    {
        m_b_aed_stable_proc_is_running = false;
    	
    	if( pthread_join( m_aed_stable_thread_t, NULL ) != 0 )
    	{
    		print_log( ERROR, 1,"[%s] pthread_join: aed_stable_proc:%s\n", m_p_log_id, strerror( errno ));
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
void* aed_routine( void * arg )
{
	CAed *aed = (CAed *)arg;
	aed->aed_proc();
	pthread_exit( NULL );
}

/******************************************************************************/
/**
 * @brief
 * @param
 * @return
 * @note
*******************************************************************************/
void* aed_stable_routine( void * arg )
{
	CAed *aed = (CAed *)arg;
	aed->aed_stable_proc();
	pthread_exit( NULL );
}


/******************************************************************************/
/**
 * @brief
 * @param
 * @return
 * @note
*******************************************************************************/
void CAed::aed_proc(void)
{
	bool b_aed0_range_in = false;
	bool b_aed1_range_in = false;
	
	potentiometer_write(eAED0_BIAS, m_aed_t.p_aed_pot[eAED0_BIAS]);
	potentiometer_write(eAED1_BIAS, m_aed_t.p_aed_pot[eAED1_BIAS]);
	
	while(m_b_aed_proc_is_running)
	{
		b_aed0_range_in = adjust_aed_offset(0);
		b_aed1_range_in = adjust_aed_offset(1);
			
		if( b_aed0_range_in == true && b_aed1_range_in == true )
		{
			sleep_ex(100000);			// 100msec delay
		}
	}
}


/******************************************************************************/
/**
 * @brief	AED 가 안정화 될 때까지 대기
 * @param
 * @return
 * @note
*******************************************************************************/
void CAed::aed_stable_proc(void)
{
	while(m_b_aed_stable_proc_is_running)
	{
		aed_resource_lock();
		if(m_aed_stable_on == false && m_p_aed_stable_time_c != NULL )
		{
			if(m_p_aed_stable_time_c->is_expired())
			{
				m_aed_stable_on = true;
				safe_delete(m_p_aed_stable_time_c);
				print_log( DEBUG, 1, "[%s] AED is stable (%dms).\r\n", m_p_log_id, AED_STABLE_TIME);
				m_b_aed_stable_proc_is_running = false;
				aed_resource_unlock();
				return;
			}
		}
		aed_resource_unlock();

		sleep_ex(10000);
	}
}

/******************************************************************************/
/**
 * @brief
 * @param
 * @return
 * @note
*******************************************************************************/
bool CAed::adjust_aed_offset(u8 i_c_aed_num)
{
	u32 aed_trig_volt = 0;
	u8 c_aed_id = i_c_aed_num;
	s32	n_offset = 0;
	u32 aed_low_thr = 0;
	u32 aed_high_thr = 0;

	n_offset = m_aed_t.p_aed_pot[eAED0_OFFSET+2*c_aed_id];
	aed_low_thr = (u32)get_aed_threshold_low_value(c_aed_id);
	aed_high_thr = (u32)get_aed_threshold_high_value(c_aed_id);
	
	potentiometer_write(eAED0_OFFSET+2*c_aed_id, (u8)n_offset);
	
	if (is_vw_revision_board()) 
	{
		if( read_trig_volt((AED_ID)c_aed_id, &aed_trig_volt) == false )
		{
			return false;
		}
	}
	else
	{
		aed_trig_volt = (u32)aed_trig_measurement((AED_ID)c_aed_id);
	}

	if( aed_low_thr <= aed_trig_volt && aed_trig_volt <= aed_high_thr )
	{
		return true;
	}
	else if( aed_trig_volt <= aed_low_thr )
	{
		n_offset--;
	}
	else if( aed_trig_volt >= aed_high_thr )
	{
		n_offset++;
	}
	
	if(n_offset < 0)
		n_offset = 0;
	else if(n_offset > 255)
		n_offset = 255;	
		
	m_aed_t.p_aed_pot[eAED0_OFFSET+2*c_aed_id] = (u8)n_offset;
	return false;
}

/******************************************************************************/
/**
 * @brief
 * @param
 * @return
 * @note
*******************************************************************************/
int CAed::aed_trig_measurement(AED_ID i_aed_id_e)
{
	int n_aed_trig_volt = 0;
	u32 n_count = 0;
	u32 n_measurment_num = DEFAULT_COUNT;
	for(n_count = 0; n_count < n_measurment_num; n_count++)
	{
		n_aed_trig_volt +=  aed_trig_get(i_aed_id_e); // AED_TRIG value
		sleep_ex(2000);
	}

	return (n_aed_trig_volt + (n_measurment_num>>1))/n_measurment_num;	
}

/******************************************************************************/
/**
 * @brief
 * @param
 * @return
 * @note
*******************************************************************************/
int CAed::aed_trig_get(AED_ID i_aed_id_e)
{
	int n_aed_trig_volt = 0;

	if(i_aed_id_e != eAED_ID_0 && i_aed_id_e != eAED_ID_1)
	{
		print_log( ERROR, 1, "[%s] Invalid AED trigger channel : %d!\n",m_p_log_id, i_aed_id_e);
		return 0;
	}
	else
	{	
		ioctl_data_t 	iod_t;
		memset(&iod_t, 0, sizeof(ioctl_data_t));
		
		if (ioctl( m_n_spi_fd, VW_MSG_IOCTL_SPI_ADC_READ_ALL ,(char*)&iod_t ) < 0)
		{
	  		print_log( ERROR, 1, "[%s] AED read ioctl error!!!\n",m_p_log_id);
	  		return 0;
	  	}

	  	n_aed_trig_volt = iod_t.data[4+i_aed_id_e];
	  	n_aed_trig_volt = 1000*n_aed_trig_volt*ADC_2X_VREF_VOLT/ADC_RESOLUTION; // mV
	  	if(m_c_hw_ver >= 1)
	  		n_aed_trig_volt = n_aed_trig_volt*5/3;
	}

	return n_aed_trig_volt;
}

/******************************************************************************/
/**
 * @brief
 * @param
 * @return
 * @note
*******************************************************************************/
void CAed::set_aed_offset_value(u8 i_aed_id, u8 i_c_data)
{
	m_aed_t.p_aed_pot[eAED0_OFFSET+2*i_aed_id] = i_c_data;
	potentiometer_write(eAED0_OFFSET+2*i_aed_id, i_c_data);
	m_p_env_c->aed_info_write(&m_aed_t);
	
	print_log( DEBUG, 1, "[%s] AED(%d) Set - Offset:%d \n", m_p_log_id, i_aed_id, i_c_data);
}

/******************************************************************************/
/**
 * @brief
 * @param
 * @return
 * @note
*******************************************************************************/
u8 CAed::get_aed_offset_value(u8 i_aed_id)
{
	print_log( DEBUG, 1, "[%s] AED(%d) Get - Offset:%d \n", m_p_log_id, i_aed_id, m_aed_t.p_aed_pot[eAED0_OFFSET+2*i_aed_id]);

	return m_aed_t.p_aed_pot[eAED0_OFFSET+2*i_aed_id];
}

/******************************************************************************/
/**
 * @brief
 * @param
 * @return
 * @note
*******************************************************************************/
void CAed::set_aed_bias_value(u8 i_aed_id, u8 i_c_data)
{
	m_aed_t.p_aed_pot[eAED0_BIAS+2*i_aed_id] = i_c_data;
	potentiometer_write(eAED0_BIAS+2*i_aed_id, i_c_data);
	m_p_env_c->aed_info_write(&m_aed_t);
	
	print_log( DEBUG, 1, "[%s] AED(%d) Set - Bias:%d \n", m_p_log_id, i_aed_id, m_aed_t.p_aed_pot[eAED0_BIAS+2*i_aed_id]);
}

/******************************************************************************/
/**
 * @brief
 * @param
 * @return
 * @note
*******************************************************************************/
u8 CAed::get_aed_bias_value(u8 i_aed_id)
{
	print_log( DEBUG, 1, "[%s] AED(%d) Get - Bias:%d \n", m_p_log_id, i_aed_id, m_aed_t.p_aed_pot[eAED0_BIAS+2*i_aed_id]);

	return m_aed_t.p_aed_pot[eAED0_BIAS+2*i_aed_id];
}

/******************************************************************************/
/**
 * @brief
 * @param
 * @return
 * @note
*******************************************************************************/
void CAed::set_aed_threshold_low_value(u8 i_aed_id, u16 i_w_data)
{
	if(i_aed_id == eAED_ID_1)
	{
		m_aed_t.n_aed1_trig_low_thr = i_w_data;
	}
	else
	{
		m_aed_t.n_aed0_trig_low_thr = i_w_data;
	}
	
	m_p_env_c->aed_info_write(&m_aed_t);
	
	print_log( DEBUG, 1, "[%s] AED(%d) Set - AED trigger voltage low threshold:%d \n", m_p_log_id, i_aed_id, i_w_data);
}

/******************************************************************************/
/**
 * @brief
 * @param
 * @return
 * @note
*******************************************************************************/
u16 CAed::get_aed_threshold_low_value(u8 i_aed_id)
{
	u16 w_thr_low = 0;
	if(i_aed_id == eAED_ID_1)
	{
		w_thr_low = (u16)m_aed_t.n_aed1_trig_low_thr;
	}
	else
	{
		w_thr_low = (u16)m_aed_t.n_aed0_trig_low_thr;
	}
	
	//print_log( DEBUG, 1, "[%s] AED(%d) Get - AED trigger voltage low threshold:%d \n", m_p_log_id, i_aed_id, w_thr_low);
	
	return w_thr_low;
	
}


/******************************************************************************/
/**
 * @brief
 * @param
 * @return
 * @note
*******************************************************************************/
void CAed::set_aed_threshold_high_value(u8 i_aed_id, u16 i_w_data)
{
	if(i_aed_id == eAED_ID_1)
	{
		m_aed_t.n_aed1_trig_high_thr = i_w_data;
	}
	else
	{
		m_aed_t.n_aed0_trig_high_thr = i_w_data;
	}
	
	m_p_env_c->aed_info_write(&m_aed_t);
	
	print_log( DEBUG, 1, "[%s] AED(%d) Set - AED trigger voltage high threshold:%d \n", m_p_log_id, i_aed_id, i_w_data);
}

/******************************************************************************/
/**
 * @brief
 * @param
 * @return
 * @note
*******************************************************************************/
u16 CAed::get_aed_threshold_high_value(u8 i_aed_id)
{
	u16 w_thr_high = 0;
	if(i_aed_id == eAED_ID_1)
	{
		w_thr_high = (u16)m_aed_t.n_aed1_trig_high_thr;
	}
	else
	{
		w_thr_high = (u16)m_aed_t.n_aed0_trig_high_thr;
	}
	
	//print_log( DEBUG, 1, "[%s] AED(%d) Get - AED trigger voltage high threshold:%d \n", m_p_log_id, i_aed_id, w_thr_high);
	
	return w_thr_high;
}



/******************************************************************************/
/**
 * @brief
 * @param
 * @return
 * @note
*******************************************************************************/
s32 CAed::potentiometer_write(u8 i_c_sel, u8 i_c_data)
{
	ioctl_data_t 	iod_t;
	memset(&iod_t, 0, sizeof(ioctl_data_t));
	
	iod_t.data[0] = i_c_sel;
	iod_t.data[1] = i_c_data;
	
	if (ioctl( m_n_spi_fd, VW_MSG_IOCTL_SPI_POTENTIOMETER_WRITE ,(char*)&iod_t ) < 0)
	{
  		print_log( ERROR, 1, "[%s] Potentiometer write error!!! sel : %d, data : 0x%02X\n",m_p_log_id, i_c_sel, i_c_data);
  		return -1;
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
void CAed::write(u8 i_c_aed_id, u32 i_n_addr, u16 i_w_value)
{
	if(eBIAS == i_n_addr)
	{
		set_aed_bias_value(i_c_aed_id, (u8)i_w_value);
	}
	else if(eOFFSET == i_n_addr)
	{
		set_aed_offset_value(i_c_aed_id, (u8)i_w_value);
	}
	else if(eAED_VOLT_THR_L == i_n_addr)
	{
		set_aed_threshold_low_value(i_c_aed_id, i_w_value);
	}
	else if(eAED_VOLT_THR_H == i_n_addr)
	{
		set_aed_threshold_high_value(i_c_aed_id, i_w_value);
	}
}
	

/******************************************************************************/
/**
 * @brief
 * @param
 * @return
 * @note
*******************************************************************************/
u16 CAed::read(u8 i_c_aed_id, u32 i_n_addr)
{
	if(eBIAS == i_n_addr)
	{
		return get_aed_bias_value(i_c_aed_id);
	}
	else if(eOFFSET == i_n_addr)
	{
		return get_aed_offset_value(i_c_aed_id);
	}	
	else if(eAED_VOLT_THR_L == i_n_addr)
	{
		return get_aed_threshold_low_value(i_c_aed_id);
	}
	else if(eAED_VOLT_THR_H == i_n_addr)
	{
		return get_aed_threshold_high_value(i_c_aed_id);
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
bool CAed::is_stable(void)
{
	bool b_ret;
	aed_resource_lock();
	b_ret = m_aed_stable_on;	
	aed_resource_unlock();
	return b_ret;
}

/***********************************************************************************************//**
* \brief		check if it is the board made after vw revision 
* \param		None
* \return		true: vw revision board
* \return		false: NOT vw revision board
* \note			vw revision board has the board number greater than 2
***************************************************************************************************/
bool CAed::is_vw_revision_board(void)
{
	if( m_c_hw_ver <= 2) return false;
	else return true;
}