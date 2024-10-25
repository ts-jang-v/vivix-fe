/*******************************************************************************
 모  듈 : impact.cpp
 작성자 : jeongil min
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
#include <stdlib.h>			// malloc(), free(), atoi()
#include <errno.h>          	// errno
#include <string.h>            // memset(), memcpy()
#include <iostream>
#include <fcntl.h>          	// open() O_NDELAY
#include <time.h>
#include <sys/poll.h>			// struct pollfd, poll()
#include <sys/syscall.h>
#include <unistd.h>

#include "impact.h"
#include "misc.h"           	// sys_command
#include "vw_time.h"			// sleep_ex
#include <math.h>				// pow

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/

#define ADX_CAL_SAMPLE_NUM				20
#define LEVEL1_HEIGHT					50
#define LEVEL2_HEIGHT					100
#define CORRECTION_FACTOR_50HZ			100
#define CORRECTION_FACTOR_400HZ			108
#define FILTERING_COUNT_50HZ			1
#define FILTERING_COUNT_400HZ			15

#define swap_32(A) ((((u32)(A) & 0xff000000) >> 24) | \
					 (((u32)(A) & 0x00ff0000) >> 8) | \
					 (((u32)(A) & 0x0000ff00) << 8) | \
					 (((u32)(A) & 0x000000ff) << 24))
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


CImpact::CImpact(CVWSPI* i_p_spi_c, int (*log)(int,int,const char *,...))
{
	print_log = log;
	strcpy( m_p_log_id, "IMPACT        " );
	m_p_spi_c = i_p_spi_c;

	m_b_stable_proc_hold = false;
	m_b_gyro_exist = m_p_spi_c->acc_gyro_probe();
}

CImpact::~CImpact()
{
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CImpact::get_gyro_offset(gyro_cal_offset_t* o_p_gyro_offset_t)
{
	m_p_spi_c->acc_gyro_reg_read(ACC_GYRO_REG_X_OFS_USR, (u8*)&(o_p_gyro_offset_t->c_x), 1);
	m_p_spi_c->acc_gyro_reg_read(ACC_GYRO_REG_Y_OFS_USR, (u8*)&(o_p_gyro_offset_t->c_y), 1);
	m_p_spi_c->acc_gyro_reg_read(ACC_GYRO_REG_Z_OFS_USR, (u8*)&(o_p_gyro_offset_t->c_z), 1);

	//print_log(DEBUG, 1, "[%s] gyro offset x: %d y: %d z: %d \n",m_p_log_id, o_p_gyro_offset_t->c_x, o_p_gyro_offset_t->c_y, o_p_gyro_offset_t->c_z);

}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CImpact::set_gyro_offset(gyro_cal_offset_t* i_p_gyro_offset_t)
{ 
	print_log(DEBUG, 1, "[%s]set Gyro Offset Data (cal data x: 0x%x y: 0x%x z: 0x%x)\n", m_p_log_id, i_p_gyro_offset_t->c_x, \
																					i_p_gyro_offset_t->c_y, i_p_gyro_offset_t->c_z);	
	m_p_spi_c->acc_gyro_reg_write(ACC_GYRO_REG_X_OFS_USR, (u8*)&(i_p_gyro_offset_t->c_x), 1);
	m_p_spi_c->acc_gyro_reg_write(ACC_GYRO_REG_Y_OFS_USR, (u8*)&(i_p_gyro_offset_t->c_y), 1);
	m_p_spi_c->acc_gyro_reg_write(ACC_GYRO_REG_Z_OFS_USR, (u8*)&(i_p_gyro_offset_t->c_z), 1);
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void * impact_cal_routine( void * arg )
{
	CImpact * impact = (CImpact *)arg;
	impact->impact_cal_proc();
	pthread_exit( NULL );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void * impact_event_routine( void * arg )
{
	CImpact * impact = (CImpact *)arg;
	impact->impact_event_proc();
	pthread_exit( NULL );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void * impact_logging_routine( void * arg )
{
	CImpact * impact = (CImpact *) arg;
	impact->impact_logging_proc();
	pthread_exit( NULL );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void * impact_get_pos_routine( void * arg )
{
	CImpact * impact = (CImpact *)arg;
	impact->impact_get_pos_proc();
	pthread_exit( NULL );
}

/******************************************************************************/
/**
* \brief        constructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CRiscImpact::CRiscImpact(int i_n_fd, CVWI2C* i_p_i2c_c, CVWSPI* i_p_spi_c, int (*log)(int,int,const char *,...)):CImpact(i_p_spi_c, log)
{
	print_log = log;
	strcpy( m_p_log_id, "RISCIMPACT    " );
	
	m_n_fd = i_n_fd;
	
	m_p_mutex_c = NULL;
	m_p_i2c_c = i_p_i2c_c;

	m_p_spi_c = i_p_spi_c;

	m_b_impact_cal_is_running = false;

	//cal ���� 
    m_c_cal_progress = 0;
    m_b_is_gyro_cal_result_valid = false;
    m_b_is_impact_cal_result_valid = false;

    m_impact_cal_thread_t = (pthread_t)0;
    
    m_w_impact_count = 0;
    
    m_n_queue_rear = 0;
	m_n_queue_front = 0;
	m_n_queue_num_of_events = 0;

	m_n_x = 0;
	m_n_y = 0;
	m_n_z = 1000;

	memset(&m_impact_cal_extra_detailed_offset_t, 0x00, sizeof(impact_cal_extra_detailed_offset_t));
	
	m_p_motion_timer_c = NULL;
	m_f_cur_val_weight = 0.05f;
	
	reset_6plane_cal_variables();
    
	m_n_sample_num = SENSOR_VALUE_FILTER_SAMPLE_SIZE;
	m_n_cut_num = SENSOR_VALUE_FILTER_CUTOFF_SIZE;
		
	m_b_stable_pos_proc_is_running = false;
	m_b_stable_proc_hold = false;
	
	m_cal_direction_e = eACC_DIR_Z_P_1G;
	m_c_6plane_cal_done = 0;
	m_c_6plane_cal_misdir = 0;

	
	memset(m_p_impact_queue, 0, sizeof(m_p_impact_queue));
}

/******************************************************************************/
/**
* \brief        destructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CRiscImpact::~CRiscImpact()
{
	safe_delete(m_p_mutex_c);
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::initialize(void)
{
	m_p_mutex_c = new CMutex();

	acc_impact_sensor_init();	
	impact_log_init();
	
	if(is_gyro_exist())
		acc_gyro_combo_sensor_init();

    // launch impact event thread
	if( pthread_create(&m_impact_event_thread_t, NULL, impact_event_routine, ( void * ) this)!= 0 )
    {
    	print_log( ERROR, 1, "[%s] impact_event_routine pthread_create:%s\n", \
    	                    m_p_log_id, strerror( errno ));
    }

    // launch impact logging thread
	if( pthread_create(&m_impact_logging_thread_t, NULL, impact_logging_routine, ( void * ) this)!= 0 )
    {
    	print_log( ERROR, 1, "[%s] impact_logging_routine pthread_create:%s\n", \
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
u8 CRiscImpact::reg_read( u8 i_c_addr )
{
	return m_p_i2c_c->impact_read_reg(i_c_addr);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::reg_write( u8 i_c_addr, u8 i_c_data )
{
	m_p_i2c_c->impact_write_reg(i_c_addr, i_c_data);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::acc_impact_sensor_init(void)
{
	adxl_reg_u reg_u;
	
	reg_write(ADXL375B_DUR,255);

	reg_u.SHOCK_AXES.DATA = 0;
	reg_u.SHOCK_AXES.BIT.SZ_EN = eON;
	reg_u.SHOCK_AXES.BIT.SY_EN = eON;
	reg_u.SHOCK_AXES.BIT.SX_EN = eON;
	reg_u.SHOCK_AXES.BIT.SUPPRESS = 0;
	reg_write(ADXL375B_SHOCK_AXES,reg_u.SHOCK_AXES.DATA );

	reg_u.INT_EN.DATA = 0;
	reg_u.INT_EN.BIT.SSHOCK = eON;
	reg_write(ADXL375B_INT_ENABLE,reg_u.INT_EN.DATA );

	reg_u.INT_MAP.DATA = 0;
	reg_u.INT_MAP.BIT.SSHOCK = eINT1;
	reg_write(ADXL375B_INT_MAP,reg_u.INT_MAP.DATA );
	
	
	reg_u.FIFO_CTL.DATA = 0;
	reg_u.FIFO_CTL.BIT.FIFO_MODE = eSTREAM;
	reg_u.FIFO_CTL.BIT.SAMPLE = ADXL375B_PREBUF_SIZE;	
	reg_write(ADXL375B_FIFO_CTL,reg_u.FIFO_CTL.DATA );
	
	
	reg_u.BW_RATE.DATA = 0;
	reg_u.BW_RATE.BIT.RATE = eADXL375B_OUTPUT_RATE_50HZ;
	reg_write(ADXL375B_BW_RATE,reg_u.BW_RATE.DATA );
	
	reg_u.DATA_FORMAT.DATA = 0;
	reg_u.DATA_FORMAT.BIT.FULL_RES = eON;	
	reg_u.DATA_FORMAT.BIT.RANGE = 3;	
	reg_write(ADXL375B_DATA_FORMAT,reg_u.DATA_FORMAT.DATA );
	
	reg_u.POW_CTL.DATA = 0;
	reg_u.POW_CTL.BIT.MEASURE = eON;
	reg_write(ADXL375B_POWER_CTL,reg_u.POW_CTL.DATA);

}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::impact_cal_log_save(impact_cal_offset_t* i_p_impact_cal_offset, gyro_cal_offset_t* i_p_gyro_cal_offset)
{
	FILE *p_fp;
	s8      p_file_name[256];
	struct tm*  p_gmt_tm_t;
	u32 n_time = time(NULL);
	
	sprintf(p_file_name,"/tmp/%s",IMPACT_CAL_LOG_FILE);
	
	p_fp = fopen(p_file_name,"w");
	
	p_gmt_tm_t = localtime( (time_t*)&n_time );
    fprintf( p_fp, "%04d-%02d-%02d %02d:%02d:%02d\n", \
                        p_gmt_tm_t->tm_year+1900, p_gmt_tm_t->tm_mon+1, p_gmt_tm_t->tm_mday, \
                        p_gmt_tm_t->tm_hour, p_gmt_tm_t->tm_min, p_gmt_tm_t->tm_sec);

    fprintf( p_fp, "6plane cal done bitflag:0x%04X 6plane cal misdir bitflag:0x%04X\n", \
                        m_c_6plane_cal_done, m_c_6plane_cal_misdir);

    fprintf( p_fp, "impact x offset:%4d y offset:%4d z offset:%4d\n", \
                        i_p_impact_cal_offset->c_x,i_p_impact_cal_offset->c_y,i_p_impact_cal_offset->c_z);

    fprintf( p_fp, "impact x detailed offset:%4d y detailed offset:%4d z detailed offset:%4d\n", \
						m_impact_cal_extra_detailed_offset_t.s_x, m_impact_cal_extra_detailed_offset_t.s_y, m_impact_cal_extra_detailed_offset_t.s_z);

    fprintf( p_fp, "gyro x offset:%4d y offset:%4d z offset:%4d\n", \
                        i_p_gyro_cal_offset->c_x, i_p_gyro_cal_offset->c_y, i_p_gyro_cal_offset->c_z);

	fclose(p_fp);
	
	sys_command("cp %s %s", p_file_name, SYS_LOG_PATH);
    sys_command("sync");
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CRiscImpact::impact_cal_proc(void)
{
    print_log(DEBUG, 1, "[%s] start %s impact_cal_proc...\n", m_p_log_id, m_b_gyro_exist ? "gyro, accel" : "6plane");
    
	if(is_gyro_exist())
    	gyro_impact_calibration();
	else
		sixplane_impact_calibration();

    m_c_cal_progress = 100;
    
    print_log(DEBUG, 1, "[%s] stop %s impact_cal_proc...\n", m_p_log_id, m_b_gyro_exist ? "gyro, accel" : "6plane");
    
    pthread_detach(m_impact_cal_thread_t);
    m_b_impact_cal_is_running = false;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CRiscImpact::impact_event_proc(void)
{
    s32                 ret;
    struct pollfd		events[1];
    s32					n_error = 0;

    memset(events,0,sizeof(events));
    events[0].fd = m_n_fd;
    events[0].events = POLLIN;

    while(1)   
    {
	    ret = poll( (struct pollfd *)&events, 1, 100 );
	    
	    if(ret < 0)
	    {
	    	n_error = errno;
	    	if(n_error == EAGAIN || n_error == EINTR)
	    	{
	    		continue;
	    	}	
	    
	        print_log(ERROR, 1, "[%s] poll error(%s)\n", m_p_log_id, strerror( n_error ));
	    }
	    
	    if(events[0].revents & POLLIN)
	    {   
	    	interrupt_disable();
	    	m_b_stable_proc_hold = true;	
	    	m_p_mutex_c->lock();
	    		
			read_axis_data((u8*)m_p_impact_queue[m_n_queue_rear]);
			queue_rear_inc();
			m_n_queue_num_of_events++; 
			 
			m_p_mutex_c->unlock(); 
			m_b_stable_proc_hold = false;	
			interrupt_enable();	
	    }
	}
    
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CRiscImpact::impact_logging_proc(void)
{
	u32 n_events_num = 0;
	u32 n_idx = 0;
	
	while(1)
	{
		n_events_num = get_num_of_events();
		if(n_events_num > 0)
		{
			for(n_idx = 0; n_idx < n_events_num; n_idx++)
			{
				impact_log_save(n_idx);
			}
		}
		
		sleep_ex(500000);
	}
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note   GYRO 실장 HW: Gyro calibration + impact 1면 calibration start
*******************************************************************************/
void CRiscImpact::impact_calibration_start(void)
{
    m_c_cal_progress = 0;
    
    if( m_b_impact_cal_is_running == true )
    {
        impact_calibration_stop();
    }
    
    m_b_impact_cal_is_running = true;
	if( pthread_create(&m_impact_cal_thread_t, NULL, impact_cal_routine, ( void * ) this)!= 0 )
    {
    	print_log( ERROR, 1, "[%s] impact_cal_routine pthread_create:%s\n", m_p_log_id, strerror( errno ));

    	m_b_impact_cal_is_running = false;
    	m_c_cal_progress = 100;
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::impact_calibration_stop(void)
{
    if( m_b_impact_cal_is_running )
    {
        m_b_impact_cal_is_running = false;
    	
    	if( pthread_join( m_impact_cal_thread_t, NULL ) != 0 )
    	{
    		print_log( ERROR, 1,"[%s] pthread_join: impact_cal_proc:%s\n", m_p_log_id, strerror( errno ));
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
void CRiscImpact::stable_pos_proc_start(void)
{ 
    if( m_b_stable_pos_proc_is_running == true )
    {
    	print_log( DEBUG, 1, "[%s] stable_pos is running!!!!\n", m_p_log_id);
        return;
    }
    
    print_log( DEBUG, 1, "[%s] stable_pos_proc_start!!!!\n", m_p_log_id);
    
	if( pthread_create(&m_impact_get_pos_thread_t, NULL, impact_get_pos_routine, ( void * ) this)!= 0 )
    {
    	print_log( ERROR, 1, "[%s] impact_get_pos_routine pthread_create:%s\n", m_p_log_id, strerror( errno ));

    	m_b_stable_pos_proc_is_running = false;
    }
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::stable_pos_proc_stop(void)
{
    if( m_b_stable_pos_proc_is_running )
    {
    	print_log( DEBUG, 1, "[%s] stable_pos_proc_stop!!!!\n", m_p_log_id);
        m_b_stable_pos_proc_is_running = false;
    	
    	if( pthread_join( m_impact_get_pos_thread_t, NULL ) != 0 )
    	{
    		print_log( ERROR, 1,"[%s] pthread_join: stable_pos_proc:%s\n", m_p_log_id, strerror( errno ));
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
void CRiscImpact::impact_set_threshold(u8 i_c_threshold)
{
 	float f_value;
    
    if( i_c_threshold > 200 )
    {
        f_value = 255;
    }
    else
    {
        f_value = (255 * i_c_threshold) / 200.0f;
        f_value += 0.5;
    }
    
   print_log(DEBUG, 1, "[%s] threshold %dG -> %d * 780mG\n", m_p_log_id, i_c_threshold, (u8)f_value );
   reg_write(ADXL375B_THRESH_SHOCK, (u8)f_value);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CRiscImpact::impact_get_maximum_value( u8* i_p_log)
{
	u16 			w_maximum_val = 0;;
	u32				n_idx;
	adxl_data_t* 	p_adxl_data_t = (adxl_data_t*)i_p_log;
	u16				w_x;
	u16				w_y;
	u16				w_z;
	
	for( n_idx = 0; n_idx < ADXL375B_FIFO_SIZE ; n_idx++)
	{
		u16 w_sub_maximum;
		w_x = abs(p_adxl_data_t[n_idx].w_x);
		w_y = abs(p_adxl_data_t[n_idx].w_y);
		w_z = abs(p_adxl_data_t[n_idx].w_z);
		
		if( w_x < w_y)
		{
			w_sub_maximum = w_y;
		}
		else
		{
			w_sub_maximum = w_x;
		}
		if( w_sub_maximum < w_z)
		{
			w_sub_maximum = w_z;
		}
		if( w_maximum_val < w_sub_maximum)
		{
			w_maximum_val = w_sub_maximum;
		}
	}
	return w_maximum_val;
}

/********************************************************************************
 * \brief		
 * \param
 * \return
 * \note
 ********************************************************************************/
void CRiscImpact::get_impact_offset(impact_cal_offset_t* o_p_impact_offset_t)
{
	o_p_impact_offset_t->c_x = reg_read(ADXL375B_OFSX);
	o_p_impact_offset_t->c_y = reg_read(ADXL375B_OFSY);
	o_p_impact_offset_t->c_z = reg_read(ADXL375B_OFSZ);
}	

/********************************************************************************
 * \brief		
 * \param
 * \return
 * \note
 ********************************************************************************/
void CRiscImpact::set_impact_offset(impact_cal_offset_t* i_p_impact_offset_t)
{
	print_log(DEBUG, 1, "[%s]set impact Offset Data (cal data x: 0x%x y: 0x%x z: 0x%x)\n", m_p_log_id, i_p_impact_offset_t->c_x, \
																					i_p_impact_offset_t->c_y, i_p_impact_offset_t->c_z);

	reg_write(ADXL375B_OFSX,i_p_impact_offset_t->c_x);
	reg_write(ADXL375B_OFSY,i_p_impact_offset_t->c_y);
	reg_write(ADXL375B_OFSZ,i_p_impact_offset_t->c_z);
}	
	
/********************************************************************************
 * \brief		self programing
 * \param
 * \return
 * \note		impact sensor + gyro sensor 
 ********************************************************************************/
void CRiscImpact::gyro_impact_calibration()
{
	adxl_reg_u 		reg_u;
	adxl_reg_u 		reg_save_u;
	
	impact_cal_offset_t impact_cal_offset;
	memset(&impact_cal_offset, 0x00, sizeof(impact_cal_offset_t));
	
	s32 			w_x = 0;
    s32 			w_y = 0;
    s32 			w_z = 0;
    s32				n_sum_x = 0;
    s32				n_sum_y = 0;
    s32				n_sum_z = 0;
    
    u16 			w_count = 0; 
    s16 			w_diff1 = 0, w_diff2 = 0;
    
	interrupt_disable();
	
	reset_6plane_cal_variables();

	memset(&m_impact_cal_extra_detailed_offset_t, 0x00, sizeof(impact_cal_extra_detailed_offset_t));
	
	reg_save_u.FIFO_CTL.DATA = reg_read(ADXL375B_FIFO_CTL);
	
	reg_u.FIFO_CTL.DATA = 0;
	reg_u.FIFO_CTL.BIT.FIFO_MODE = eSTREAM;
	reg_u.FIFO_CTL.BIT.SAMPLE = ADXL375B_PREBUF_SIZE;	
	reg_write(ADXL375B_FIFO_CTL,reg_u.FIFO_CTL.DATA );
	
	reg_write(ADXL375B_OFSX,0);
	reg_write(ADXL375B_OFSY,0);
	reg_write(ADXL375B_OFSZ,0);
	
	reg_u.POW_CTL.DATA = 0;
	reg_u.POW_CTL.BIT.MEASURE = eON;
	reg_write(ADXL375B_POWER_CTL,reg_u.POW_CTL.DATA);

	while(w_count < ADX_CAL_SAMPLE_NUM)
    {	
		impact_read_pos(&w_x, &w_y, &w_z);		

		n_sum_x += w_x;
		n_sum_y += w_y;
		n_sum_z += w_z;
		
		w_count++;
		m_c_cal_progress += 2;
		if( m_c_cal_progress >= 40 )
			m_c_cal_progress = 40;
			
		sleep_ex(100000);      // 100 ms
	}
	
	m_c_cal_progress = 40;
	
	n_sum_x /= ADX_CAL_SAMPLE_NUM;
	n_sum_y /= ADX_CAL_SAMPLE_NUM;
	n_sum_z = (n_sum_z / ADX_CAL_SAMPLE_NUM) - 1000; // z axi is 1G
	
	n_sum_x = (n_sum_x + (n_sum_x > 0 ? 98:-98))/196;
	n_sum_y = (n_sum_y + (n_sum_y > 0 ? 98:-98))/196;
	n_sum_z = (n_sum_z + (n_sum_z > 0 ? 98:-98))/196;
	    
	impact_cal_offset.c_x = -1*clip_s16(n_sum_x);
	impact_cal_offset.c_y = -1*clip_s16(n_sum_y);
	impact_cal_offset.c_z = -1*clip_s16(n_sum_z);
	
	// 가속도 센서 내부에 설정할 offset 값
	// 196mg 단위로만 조정 가능
	reg_write(ADXL375B_OFSX, impact_cal_offset.c_x);
	reg_write(ADXL375B_OFSY, impact_cal_offset.c_y);
	reg_write(ADXL375B_OFSZ, impact_cal_offset.c_z);

	reg_write(ADXL375B_FIFO_CTL,reg_save_u.FIFO_CTL.DATA);

	reg_u.POW_CTL.DATA = 0;
	reg_u.POW_CTL.BIT.MEASURE = eON;
	reg_write(ADXL375B_POWER_CTL,reg_u.POW_CTL.DATA);
	
	interrupt_enable();
	
	impact_read_pos(&w_x, &w_y, &w_z);

	w_diff1 =  0 - w_x;
	w_diff2 =  0 - w_y;
		
	if( ( w_diff1 > 300 ||  w_diff1 < -300 ) || ( w_diff2 > 300 ||  w_diff2 < -300 ) || w_z < 0 )
	{
		m_b_is_impact_cal_result_valid = false;
	}
	else
	{
		m_b_is_impact_cal_result_valid = true;
	}

	set_6plane_sensor_cal_done(m_cal_direction_e);
	set_6plane_sensor_cal_misdir(m_cal_direction_e, 0);	

	print_log(DEBUG, 1, "[%s] accel cal result: %d %d %d\n", m_p_log_id, impact_cal_offset.c_x, impact_cal_offset.c_y, impact_cal_offset.c_z);

	m_c_cal_progress = 45;

	//ISM330DLC(Combo sensor) calibration Start

	gyro_cal_offset_t gyro_cal_offset;
	memset(&gyro_cal_offset, 0x00, sizeof(gyro_cal_offset_t));

	s16 w_gyro_x = 0;
	s16 w_gyro_y = 0;
	s16 w_gyro_z = 0;
	s32 n_gyro_sum_x = 0;
	s32 n_gyro_sum_y = 0;
	s32 n_gyro_sum_z = 0;
	u16 w_gyro_count = 0; 
	float f_data[3] = {0.0f, };

	//user offset reset
	m_p_spi_c->acc_gyro_reg_write(ACC_GYRO_REG_X_OFS_USR, (u8*)&(gyro_cal_offset.c_x), 1);
	m_p_spi_c->acc_gyro_reg_write(ACC_GYRO_REG_Y_OFS_USR, (u8*)&(gyro_cal_offset.c_y), 1);
	m_p_spi_c->acc_gyro_reg_write(ACC_GYRO_REG_Z_OFS_USR, (u8*)&(gyro_cal_offset.c_z), 1);
	
	//Offset 측정 
	while(w_gyro_count < ADX_CAL_SAMPLE_NUM)
    {	
		m_p_spi_c->acc_gyro_get_pos( &w_gyro_x, &w_gyro_y, &w_gyro_z );
		f_data[0] += ( (float)w_gyro_x * 0.061f );			//FS +-2G 일 때, SAMPLE UNIT: 0.061mg/LSB
		f_data[1] += ( (float)w_gyro_y * 0.061f );
		f_data[2] += ( (float)w_gyro_z * 0.061f );
		
		//print_log(DEBUG, 1, "[%s][TP] read offset %d: %d %d %d\n", m_p_log_id, w_gyro_count, w_gyro_x, w_gyro_y, w_gyro_z);

		w_gyro_count++;
		m_c_cal_progress += 2;
		if( m_c_cal_progress >= 90 )
			m_c_cal_progress = 90;
			
		sleep_ex(100000);      // 100 ms
	}
	
	m_c_cal_progress = 90;
	
	n_gyro_sum_x = (s32)f_data[0];
	n_gyro_sum_y = (s32)f_data[1];
	n_gyro_sum_z = (s32)f_data[2];
	
	//offset 측정 값 평균 계산
	n_gyro_sum_x /= ADX_CAL_SAMPLE_NUM;
	n_gyro_sum_y /= ADX_CAL_SAMPLE_NUM;
	n_gyro_sum_z /= ADX_CAL_SAMPLE_NUM;

	//기본 값 제외 offset 값 추출 (기본 값 x: 0g y:0g z:1g)
	n_gyro_sum_z = n_gyro_sum_z - 1000;
						
	//f_data UNIT --> OFFSET UNIT
	//f_data UNIT: 1mg/LSB
	//OFFSET UNIT: 0.9765625mg/LSB
	n_gyro_sum_x *= 1.0309;
	n_gyro_sum_y *= 1.0309;
	n_gyro_sum_z *= 1.0309;

	gyro_cal_offset.c_x = -1*clip_s16(n_gyro_sum_x);			
	gyro_cal_offset.c_y = -1*clip_s16(n_gyro_sum_y);
	gyro_cal_offset.c_z = clip_s16(n_gyro_sum_z);
	
	m_p_spi_c->acc_gyro_reg_write(ACC_GYRO_REG_X_OFS_USR, (u8*)&(gyro_cal_offset.c_x), 1);
	m_p_spi_c->acc_gyro_reg_write(ACC_GYRO_REG_Y_OFS_USR, (u8*)&(gyro_cal_offset.c_y), 1);
	m_p_spi_c->acc_gyro_reg_write(ACC_GYRO_REG_Z_OFS_USR, (u8*)&(gyro_cal_offset.c_z), 1);
	
	print_log(DEBUG, 1, "[%s] combo cal result: %d %d %d\n", m_p_log_id, gyro_cal_offset.c_x, gyro_cal_offset.c_y, gyro_cal_offset.c_z);

	//save_config(c_offset_x, c_offset_y, c_offset_z);
	
	m_b_is_gyro_cal_result_valid = true;

	m_c_cal_progress = 95;

	impact_cal_log_save(&impact_cal_offset, &gyro_cal_offset);
}


/********************************************************************************
 * \brief		
 * \param
 * \return
 * \note		impact sensor 6 plane cal 
 ********************************************************************************/
void CRiscImpact::sixplane_impact_calibration()
{
	adxl_reg_u 		reg_u;
	adxl_reg_u 		reg_save_u;
	float			n_avr_x = 0.0f;
	float			n_avr_y = 0.0f;
	float			n_avr_z = 0.0f;
	u16 			n_idx;
	u8 				c_mis_dir_res = 0;
	

	impact_cal_offset_t impact_cal_offset;
	memset(&impact_cal_offset, 0x00, sizeof(impact_cal_offset_t));

	adxl_data_t 	p_data_buf_t[ADXL375B_FIFO_SIZE];

	interrupt_disable();

	if( m_cal_direction_e == eACC_DIR_Z_P_1G )
	{   
		// sensor 정방향 calibration 시 모든 6면 cal 관련 변수 초기화
		reset_6plane_cal_variables();
		
		reg_save_u.FIFO_CTL.DATA = reg_read(ADXL375B_FIFO_CTL);
		
		reg_u.FIFO_CTL.DATA = 0;
		reg_u.FIFO_CTL.BIT.FIFO_MODE = eSTREAM;
		reg_u.FIFO_CTL.BIT.SAMPLE = ADXL375B_PREBUF_SIZE;	
		reg_write(ADXL375B_FIFO_CTL,reg_u.FIFO_CTL.DATA );
		
		reg_write(ADXL375B_OFSX,0);
		reg_write(ADXL375B_OFSY,0);
		reg_write(ADXL375B_OFSZ,0);
		
		reg_u.POW_CTL.DATA = 0;
		reg_u.POW_CTL.BIT.MEASURE = eON;
		reg_write(ADXL375B_POWER_CTL,reg_u.POW_CTL.DATA);
		
		n_idx = 0;
		while(n_idx != ADXL375B_FIFO_SIZE)
		{
			u32 n_eidx = 0;
			reg_u.FIFO_STATUS.DATA = reg_read(ADXL375B_FIFO_STATUS);
			
			for(n_eidx = 0; n_eidx < reg_u.FIFO_STATUS.BIT.ENTRIES; n_eidx++ )
			{			
				get_axis((u8*)&p_data_buf_t[n_idx]);		
				n_idx++;
	
				if( n_idx >= ADXL375B_FIFO_SIZE )
				{
					print_log(DEBUG, 1, "[%s] axis read overflow\n", m_p_log_id);
					break;
				}
	
				if( (n_idx % 4) == 0 )
				{
				    m_c_cal_progress++;
				    if( m_c_cal_progress >= 40 )
				    {
				        m_c_cal_progress = 40;
				    }
				}
				
				sleep_ex(100000);      // 100 ms
			}
		}
		
		m_c_cal_progress = 40;
		
		for(n_idx = 0 ; n_idx < ADXL375B_FIFO_SIZE; n_idx++)
		{
			n_avr_x += p_data_buf_t[n_idx].w_x;
			n_avr_y += p_data_buf_t[n_idx].w_y;
			n_avr_z += p_data_buf_t[n_idx].w_z;
		}
		
		if(n_avr_z < 0)
		{
			print_log(DEBUG, 1, "[%s] invalid n_avr_z: %f\n", m_p_log_id, n_avr_z);
			c_mis_dir_res = 1;
		}

		n_avr_z -= 1/ 0.049 * ADXL375B_FIFO_SIZE; // z axi is 1G
		
		n_avr_x /= ADXL375B_FIFO_SIZE;
		n_avr_y /= ADXL375B_FIFO_SIZE;
		n_avr_z /= ADXL375B_FIFO_SIZE;
		
		n_avr_x = (n_avr_x + (n_avr_x > 0 ? 2:-2) )/4;
		n_avr_y = (n_avr_y + (n_avr_y > 0 ? 2:-2) )/4;
		n_avr_z = (n_avr_z + (n_avr_z > 0 ? 2:-2) )/4;
		    
	
		impact_cal_offset.c_x = -1*clip_s16(n_avr_x);
		impact_cal_offset.c_y = -1*clip_s16(n_avr_y);
		impact_cal_offset.c_z = -1*clip_s16(n_avr_z);
	
		// 가속도 센서 내부에 설정할 offset 값
		// 196mg 단위로만 조정 가능
		reg_write(ADXL375B_OFSX, impact_cal_offset.c_x);
		reg_write(ADXL375B_OFSY, impact_cal_offset.c_y);
		reg_write(ADXL375B_OFSZ, impact_cal_offset.c_z);
		
		sleep_ex(1000000);
			
		
		reg_write(ADXL375B_FIFO_CTL,reg_save_u.FIFO_CTL.DATA);
	
		reg_u.POW_CTL.DATA = 0;
		reg_u.POW_CTL.BIT.MEASURE = eON;
		reg_write(ADXL375B_POWER_CTL,reg_u.POW_CTL.DATA);
		
		print_log(DEBUG, 1, "[%s] accel cal result: %d %d %d\n", m_p_log_id, impact_cal_offset.c_x, impact_cal_offset.c_y, impact_cal_offset.c_z);

		m_c_cal_progress = 45;
	}
	else
	{
		m_c_cal_progress = 0;
	}
	
	/* 가속도 센서 내부에 설정할 수 있는 offset으로는 
		196mg 단위로만 조정 가능하기 때문에 196mg 이내의 offset 값을 더 세밀하게 조정하기 위해
		아래 작업을 수행하여 안정적으로 수렴된 x, y, z값이 0, 0, 1000이 되도록 세부 offset값 구하여 EEPROM에 저장 */
	
	adxl_data_t	adxl_t;
    float      p_pos[3];
    u16 w_count = 0;
    u16 n_total_count = 60;
    u16 w_idx = 0;
    s16 w_x = 0;
    s16 w_y = 0;
    s16 w_z = 0;
    s16 w_last_x[10]={0,};
    s16 w_last_y[10]={0,};
    s16 w_last_z[10]={0,};
    
	while(w_count < n_total_count)
    {
    	memset( p_pos, 0, sizeof(p_pos) );
		
		/* 10회 average	start */
		for(w_idx = 0; w_idx < 10; w_idx++)
    	{
    		get_axis((u8*)&adxl_t);
    		p_pos[0] += adxl_t.w_x;
		    p_pos[1] += adxl_t.w_y;
		    p_pos[2] += adxl_t.w_z;
		    sleep_ex(20000);	// 50Hz measurment
		}

		p_pos[0] = (p_pos[0] * 49)/10.0f;
		p_pos[1] = (p_pos[1] * 49)/10.0f;
		p_pos[2] = (p_pos[2] * 49)/10.0f;
		/* 10회 average end */
		
		if(w_count == 0)
		{
		    w_x = (s32)(p_pos[0]);
		    w_y = (s32)(p_pos[1]);
		    w_z = (s32)(p_pos[2]);
		    w_count++;
		    continue;
		}
		
		// 이전 값에 가중치 95%, 현재 값에 가중치 5% 두어 값이 안정화 되도록 함
		p_pos[0] = p_pos[0]*(0.05f) + w_x*(0.95f);
	    p_pos[1] = p_pos[1]*(0.05f) + w_y*(0.95f);
	    p_pos[2] = p_pos[2]*(0.05f) + w_z*(0.95f);  
	    
	    w_x = (s32)(p_pos[0]);
	    w_y = (s32)(p_pos[1]);
	    w_z = (s32)(p_pos[2]);
	    
	    //print_log(DEBUG, 1, "[%s] x: %d, y: %d, z: %d\n", m_p_log_id, w_x, w_y, w_z);
	    
	    if( w_count > 0 && (w_count % (n_total_count/50)) == 0 )
		{
		    m_c_cal_progress++;
		    if(m_c_cal_progress >= 95)
		    	m_c_cal_progress = 95;
	    }
	    
	    if(n_total_count - w_count <= 10)
	   	{
	   		w_last_x[10 + w_count - n_total_count] = w_x;
	   		w_last_y[10 + w_count - n_total_count] = w_y;
	   		w_last_z[10 + w_count - n_total_count] = w_z;
	   	}
	   	
	   	w_count++;
	}
	
	u8 i_c_idx = 0;
	w_x = 0;
	w_y = 0;
	w_z = 0;
	for(i_c_idx = 0; i_c_idx < 10; i_c_idx++)
	{
		w_x += w_last_x[i_c_idx];
		w_y += w_last_y[i_c_idx];
		w_z += w_last_z[i_c_idx];
	}
	
	w_x = w_x / 10;
	w_y = w_y / 10;
	w_z = w_z / 10;
	
	//print_log(DEBUG, 1, "[%s] x: %d, y: %d, z: %d\n", m_p_log_id, w_x, w_y, w_z);
	
	if( m_cal_direction_e == eACC_DIR_Z_P_1G )
	{	
		s16 w_diff1 = 0, w_diff2 = 0;
		w_diff1 =  0 - w_x;
		w_diff2 =  0 - w_y;
		
		if( ( w_diff1 > 300 ||  w_diff1 < -300 ) || ( w_diff2 > 300 ||  w_diff2 < -300 ) || w_z < 0 )
		{
			c_mis_dir_res = 1;
		}
		else
		{			
			m_w_ax_p_zaxis = w_diff1;
			m_w_ay_p_zaxis = w_diff2;
		}
	}
	else if( m_cal_direction_e == eACC_DIR_Z_N_1G )
	{
		s16 w_diff1 = 0, w_diff2 = 0;
		w_diff1 =  0 - w_x;
		w_diff2 =  0 - w_y;
		
		if( ( w_diff1 > 300 ||  w_diff1 < -300 ) || ( w_diff2 > 300 ||  w_diff2 < -300 ) || w_z > 0 )
		{
			c_mis_dir_res = 1;
		}
		else
		{			
			m_w_ax_n_zaxis = w_diff1;
			m_w_ay_n_zaxis = w_diff2;
		}
	}
	else if( m_cal_direction_e == eACC_DIR_X_P_1G )
	{
		s16 w_diff1 = 0, w_diff2 = 0;
		w_diff1 =  0 - w_y;
		w_diff2 =  0 - w_z;
		
		if( ( w_diff1 > 300 ||  w_diff1 < -300 ) || ( w_diff2 > 300 ||  w_diff2 < -300 ) || w_x < 0 )
		{
			c_mis_dir_res = 1;
		}
		else
		{			
			m_w_ay_p_xaxis = w_diff1;
			m_w_az_p_xaxis = w_diff2;
		}
	}
	else if( m_cal_direction_e == eACC_DIR_X_N_1G )
	{
		s16 w_diff1 = 0, w_diff2 = 0;
		w_diff1 =  0 - w_y;
		w_diff2 =  0 - w_z;
		
		if( ( w_diff1 > 300 ||  w_diff1 < -300 ) || ( w_diff2 > 300 ||  w_diff2 < -300 ) || w_x > 0 )
		{
			c_mis_dir_res = 1;
		}
		else
		{			
			m_w_ay_n_xaxis = w_diff1;
			m_w_az_n_xaxis = w_diff2;
		}
	}
	else if( m_cal_direction_e == eACC_DIR_Y_P_1G )
	{
		s16 w_diff1 = 0, w_diff2 = 0;
		w_diff1 =  0 - w_x;
		w_diff2 =  0 - w_z;
		
		if( ( w_diff1 > 300 ||  w_diff1 < -300 ) || ( w_diff2 > 300 ||  w_diff2 < -300 ) || w_y < 0  )
		{
			c_mis_dir_res = 1;
		}
		else
		{			
			m_w_ax_p_yaxis = w_diff1;
			m_w_az_p_yaxis = w_diff2;
		}
	}
	else if( m_cal_direction_e == eACC_DIR_Y_N_1G )
	{
		s16 w_diff1 = 0, w_diff2 = 0;
		w_diff1 =  0 - w_x;
		w_diff2 =  0 - w_z;
		
		if( ( w_diff1 > 300 ||  w_diff1 < -300 ) || ( w_diff2 > 300 ||  w_diff2 < -300 ) || w_y > 0 )
		{
			c_mis_dir_res = 1;
		}
		else
		{			
			m_w_ax_n_yaxis = w_diff1;
			m_w_az_n_yaxis = w_diff2;
		}
	}
	else
	{
		print_log(DEBUG, 1, "[%s] Invalid direction enum : %d.\n", m_p_log_id, m_cal_direction_e);
	}
	
	if(c_mis_dir_res == 0)
	{
		m_b_is_impact_cal_result_valid = true;
		set_6plane_sensor_cal_done(m_cal_direction_e);
	}
	else
	{
		m_b_is_impact_cal_result_valid = false;
	}
	
	set_6plane_sensor_cal_misdir(m_cal_direction_e, c_mis_dir_res);	

	//각 면의 calibration 수행 결과를 기반으로 프로세서에서 추가로 보정하는 extra detailed offset 값을 업데이트 함.
	m_impact_cal_extra_detailed_offset_t.s_x = (m_w_ax_p_zaxis + m_w_ax_n_zaxis + m_w_ax_p_yaxis + m_w_ax_n_yaxis)/4;
	m_impact_cal_extra_detailed_offset_t.s_y = (m_w_ay_p_zaxis + m_w_ay_n_zaxis + m_w_ay_p_xaxis + m_w_ay_n_xaxis)/4;
	m_impact_cal_extra_detailed_offset_t.s_z = (m_w_az_p_xaxis + m_w_az_n_xaxis + m_w_az_p_yaxis + m_w_az_n_yaxis)/4;
	
	interrupt_enable();

	//결과 로그 출력
	print_log(DEBUG, 1, "[%s] %d plane accel extra detailed cal result: %d %d %d\n", m_p_log_id, \
						m_cal_direction_e, m_impact_cal_extra_detailed_offset_t.s_x, m_impact_cal_extra_detailed_offset_t.s_y, m_impact_cal_extra_detailed_offset_t.s_z);
	
	gyro_cal_offset_t gyro_cal_offset;
	memset(&gyro_cal_offset, 0x00, sizeof(gyro_cal_offset_t));
	impact_cal_log_save(&impact_cal_offset, &gyro_cal_offset);

	m_c_cal_progress = 95;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u8 CRiscImpact::impact_get_threshold(void)
{ 
    return reg_read(ADXL375B_THRESH_SHOCK);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::set_cal_direction(ACC_DIRECTION i_dir_e)
{
	if(i_dir_e >= eACC_DIR_MAX)
	{
		print_log(DEBUG, 1, "[%s] Invalid acc sensor calibration direction : %d.\n", m_p_log_id, m_cal_direction_e);
		return;
	}
	
	m_cal_direction_e = i_dir_e;
	print_log(DEBUG, 1, "[%s] Set acc sensor calibration direction : %d.\n", m_p_log_id, m_cal_direction_e);
}

	
/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CRiscImpact::impact_diagnosis(void)
{
    u8      c_old    = reg_read(ADXL375B_THRESH_SHOCK);
    bool    b_result = true;
    
    reg_write(ADXL375B_THRESH_SHOCK, 26);
    
    if( reg_read(ADXL375B_THRESH_SHOCK) != 26 )
    {
        b_result = false;
    }
    
    reg_write(ADXL375B_THRESH_SHOCK, c_old);
    
    return b_result;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::get_gyro_position( s32* o_p_x, s32* o_p_y, s32* o_p_z )
{
	float f_sensitivity = 0.061f;
	float f_data[3];
	s16   w_data[3];
	
	m_p_spi_c->acc_gyro_get_pos( &w_data[0], &w_data[1], &w_data[2] );

	f_data[0] = (float)w_data[0] * f_sensitivity;
	f_data[1] = (float)w_data[1] * f_sensitivity;
	f_data[2] = (float)w_data[2] * f_sensitivity;
	
	//print_log(DEBUG, 1, "[%s][TP] x: %d y: %d z: %d \n",m_p_log_id, w_data[0], w_data[1], w_data[2]);

	*o_p_x = (s32)f_data[0];
	*o_p_y = (s32)f_data[1];
	*o_p_z = (s32)f_data[2];
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::impact_read_pos( s32* o_p_x, s32* o_p_y, s32* o_p_z, bool i_b_direct )
{
    adxl_data_t	adxl_t;
    float      p_pos[3];
    u16 w_count = 10;
    u16 w_idx = 0;
    
	//GYRO 센서 존재 시, 
    if(m_b_stable_pos_proc_is_running == false)
    {
    	memset( p_pos, 0, sizeof(p_pos) );
    
	    for(w_idx = 0; w_idx < w_count; w_idx++)
	    {
	    	get_axis((u8*)&adxl_t);
	
		    p_pos[0] += (adxl_t.w_x * 49);
		    p_pos[1] += (adxl_t.w_y * 49);
		    p_pos[2] += (adxl_t.w_z * 49);   
		    
		    sleep_ex(20000);	// 20ms
		}
    
	    *o_p_x = (s32)(p_pos[0]/(float)w_count) + m_impact_cal_extra_detailed_offset_t.s_x;
	    *o_p_y = (s32)(p_pos[1]/(float)w_count) + m_impact_cal_extra_detailed_offset_t.s_y;
	    *o_p_z = (s32)(p_pos[2]/(float)w_count) + m_impact_cal_extra_detailed_offset_t.s_z;
	}
    else		//GYRO sensor 미 존재 시,
    {

		// 이전 값에 가중치를 많이두는 누적방식, 좀 더 안정적인 대신 변화에 대한 반응속도가 느림
    	if(i_b_direct == true)		//stable thread에서만 호출하며 실제 sensor read & 기존 획득 값을 참조하여 누적 평균치 계산 및 update
	    {
	    	s32 n_x_diff = 0;
	    	s32 n_y_diff = 0;
	    	s32 n_z_diff = 0;
	    	memset( p_pos, 0, sizeof(p_pos) );
			
			for(w_idx = 0; w_idx < w_count; w_idx++)
	    	{
	    		get_axis((u8*)&adxl_t);
	    		p_pos[0] += adxl_t.w_x;
			    p_pos[1] += adxl_t.w_y;
			    p_pos[2] += adxl_t.w_z;
			    sleep_ex(20000);
			}
			
			p_pos[0] = (p_pos[0] * 49)/10.0f + m_impact_cal_extra_detailed_offset_t.s_x;
			p_pos[1] = (p_pos[1] * 49)/10.0f + m_impact_cal_extra_detailed_offset_t.s_y;
			p_pos[2] = (p_pos[2] * 49)/10.0f + m_impact_cal_extra_detailed_offset_t.s_z;
			
			n_x_diff = ((m_n_x - (s32)p_pos[0]) > 0)?(m_n_x - (s32)p_pos[0]):(~(m_n_x - (s32)p_pos[0])+1);
			n_y_diff = ((m_n_y - (s32)p_pos[1]) > 0)?(m_n_y - (s32)p_pos[1]):(~(m_n_y - (s32)p_pos[1])+1);
			n_z_diff = ((m_n_z - (s32)p_pos[2]) > 0)?(m_n_z - (s32)p_pos[2]):(~(m_n_z - (s32)p_pos[2])+1);
			
			// print_log(DEBUG, 1, "[%s] diff x: %d, diff y: %d, diff z: %d\n",m_p_log_id, n_x_diff, n_y_diff, n_z_diff);
			
			if(m_p_motion_timer_c == NULL)
			{
				// 변화폭이 큰 경우, 반대로 현재 값에 95% 가중치를 두어 변화속도를 빠르게 함
				if(n_x_diff > 200 || n_y_diff > 200 || n_z_diff > 200)
				{
					m_f_cur_val_weight = 0.95f;
					m_p_motion_timer_c = new CTime(2000);
				}
				else
			    {
			    	m_f_cur_val_weight = 0.05f;
			    }
			}
			else 
		    {
		    	if( m_p_motion_timer_c->is_expired() == true )
		    	{
		    		m_f_cur_val_weight = 0.05f;
		    		safe_delete(m_p_motion_timer_c);
		    	}
		    }

	    	p_pos[0] = p_pos[0]*m_f_cur_val_weight + m_n_x*(1.0f - m_f_cur_val_weight);
	    	p_pos[1] = p_pos[1]*m_f_cur_val_weight + m_n_y*(1.0f - m_f_cur_val_weight);
	    	p_pos[2] = p_pos[2]*m_f_cur_val_weight + m_n_z*(1.0f - m_f_cur_val_weight);
		    	
		    m_n_x = (s32)(p_pos[0]);
		    m_n_y = (s32)(p_pos[1]);
		    m_n_z = (s32)(p_pos[2]);
		}
	    else				//계산된 값 획득
	    {
	    	*o_p_x = m_n_x;
		    *o_p_y = m_n_y;
		    *o_p_z = m_n_z;
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
u32 CRiscImpact::impact_file_num(void)
{
	// get the number of impact files    
    s8	p_buf[20];
    s8  p_cmd_buf[256];
    
    sprintf(p_cmd_buf,"ls -l %s*.csv | grep ^- | wc -l",SYS_IMPACT_LOG_PATH);
    if( get_shell_cmd_output(p_cmd_buf,20,p_buf) < 0)
    {
    	print_log(DEBUG, 1, "[%s] popen read error\n",m_p_log_id);
    	return 0;
    }
	return atoi(p_buf);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::impact_log_init(void)
{
    // make imapct log directory
    sys_command( "mkdir %s", SYS_IMPACT_LOG_PATH );
    
    u32 n_impact_file_num;
    
   
    n_impact_file_num = impact_file_num();
    print_log(INFO, 1, "[%s] impact file num : %d\n",m_p_log_id, n_impact_file_num );
    
    if( n_impact_file_num > 100)
    {
	    // 90일 지난 파일은 삭제
	    sys_command( "find %s*.csv -mtime +91 -print | head -n %d | xargs rm", SYS_IMPACT_LOG_PATH,n_impact_file_num - 100 );
	}
    
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::impact_log_save(int i_n_cnt)
{ 
	FILE*   p_file = NULL;
	s8      p_file_name[256];
    struct tm*  p_gmt_tm_t;
    u32		n_time;
	
	n_time = time(NULL);
	p_gmt_tm_t = localtime( (time_t*)&n_time );
    
    sprintf( p_file_name, "/tmp/%04d_%02d_%02d_%02d_%02d_%02d_%05d_ImpactLog.csv", \
                        p_gmt_tm_t->tm_year+1900, p_gmt_tm_t->tm_mon+1, p_gmt_tm_t->tm_mday, \
                        p_gmt_tm_t->tm_hour, p_gmt_tm_t->tm_min, p_gmt_tm_t->tm_sec, \
                        i_n_cnt );
                        
    print_log(DEBUG, 1, "[%s] impact log file %s\n", m_p_log_id, p_file_name);
    
    p_file = fopen( p_file_name, "w" );
    
    if( p_file == NULL )
    {
    	fclose(p_file);
        print_log(ERROR, 1, "[%s] impact log file create failed\n", m_p_log_id);
        return;
    }		
                                
	memset(m_p_impact_buf, 0, ADXL375B_BUF_SIZE);
	
	m_p_mutex_c->lock();
	impact_log_read(m_p_impact_buf);            
	buffer_front_clear();     
	m_p_mutex_c->unlock();  
	  
    impact_log_write( p_file, m_p_impact_buf);  
	
    fclose(p_file);
	p_file = NULL;
	sys_command("cp %s %s", p_file_name, SYS_IMPACT_LOG_PATH);        		
	sys_command("rm %s", p_file_name );
	sys_command("sync");
     
    print_log(DEBUG, 1, "[%s] storing impact log(%s)\n", m_p_log_id, p_file_name );
     	  
    m_w_impact_count++;
    
    print_log(DEBUG, 1, "[%s] impact count: %d\n", m_p_log_id, m_w_impact_count );

}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::impact_log_read( u8* o_p_log  )
{
	memcpy(o_p_log, (u8*)(get_buffer_point()), ADXL375B_BUF_SIZE);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::impact_log_write( FILE* i_p_file, u8* i_p_log )
{
    adxl_data_t* 	p_adxl_data_t = (adxl_data_t*)i_p_log;
    u16 i;
    
    for( i = 0 ; i < ADXL375B_FIFO_SIZE; i++)
    {
    	if( i_p_file == NULL )
    	{
    	    printf("idx(%3d),%6d,%6d,%6d\n", \
        	        i, \
        	        (p_adxl_data_t[i].w_x) * 49 + m_impact_cal_extra_detailed_offset_t.s_x, \
        	        (p_adxl_data_t[i].w_y) * 49 + m_impact_cal_extra_detailed_offset_t.s_y, \
                    (p_adxl_data_t[i].w_z) * 49 + m_impact_cal_extra_detailed_offset_t.s_z );
        }
        else
        {
    	    fprintf(i_p_file, "idx(%3d),%6d,%6d,%6d\n", \
        	        i, \
        	        (p_adxl_data_t[i].w_x) * 49 + m_impact_cal_extra_detailed_offset_t.s_x, \
        	        (p_adxl_data_t[i].w_y) * 49 + m_impact_cal_extra_detailed_offset_t.s_y, \
                    (p_adxl_data_t[i].w_z) * 49 + m_impact_cal_extra_detailed_offset_t.s_z );
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
void CRiscImpact::impact_print(void)
{
    u8 c_threshold;
    signed char c_ofs_x, c_ofs_y, c_ofs_z;
    float x, y, z;
    
    adxl_data_t	adxl_t;
    memset(&adxl_t, 0, sizeof(adxl_data_t));
    
    c_threshold = reg_read(ADXL375B_THRESH_SHOCK);
    
    c_ofs_x = (s8)reg_read(ADXL375B_OFSX);
    c_ofs_y = (s8)reg_read(ADXL375B_OFSY);
    c_ofs_z = (s8)reg_read(ADXL375B_OFSZ);
    
    x = c_ofs_x * 1.56f;
    y = c_ofs_y * 1.56f;
    z = c_ofs_z * 1.56f;
    
    printf("threshold: %fG\n", (c_threshold * 780)/1000.f);
    printf("offset X: %4.3fG, Y: %4.3fG, Z: %4.3fG\n", x, y, z);
    
    get_axis((u8*)&adxl_t);
    x = adxl_t.w_x * 0.049f;
    y = adxl_t.w_y * 0.049f;
    z = adxl_t.w_z * 0.049f;
    
    printf("acc X: %4.3fG, Y: %4.3fG, Z: %4.3fG\n", x, y, z);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::read_axis_data(u8* o_p_axis_buf)
{
	adxl_reg_u reg_u;
	adxl_data_t* p_data_buf_t = (adxl_data_t*)o_p_axis_buf;
	
	axis_data_pack_read(p_data_buf_t);

	reg_u.FIFO_CTL.DATA = 0;
	reg_u.FIFO_CTL.BIT.FIFO_MODE = eBYPASS;
	reg_u.FIFO_CTL.BIT.SAMPLE = 0;	
	reg_write(ADXL375B_FIFO_CTL,reg_u.FIFO_CTL.DATA );
	reg_u.FIFO_STATUS.DATA = reg_read(ADXL375B_INT_SOURCE);
		
	reg_u.FIFO_CTL.DATA = 0;
	reg_u.FIFO_CTL.BIT.FIFO_MODE = eSTREAM;
	reg_u.FIFO_CTL.BIT.SAMPLE = ADXL375B_PREBUF_SIZE;	
	reg_write(ADXL375B_FIFO_CTL,reg_u.FIFO_CTL.DATA );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::axis_data_pack_read(adxl_data_t* o_p_data_buf_t)
{
	u16 n_idx = 0;
	adxl_reg_u reg_u;
	while(n_idx != ADXL375B_FIFO_SIZE)
	{
		reg_u.FIFO_STATUS.DATA = reg_read(ADXL375B_FIFO_STATUS);
		if(reg_u.FIFO_STATUS.BIT.ENTRIES != 0)
		{
			u32 n_entri_idx = reg_u.FIFO_STATUS.BIT.ENTRIES;
			for(; n_idx < ADXL375B_FIFO_SIZE && n_entri_idx > 0; n_entri_idx--)
			{
				get_axis((u8*)&o_p_data_buf_t[n_idx]);
				n_idx++;
			}
		}
		else
		{
			sleep_ex(100000);
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
void CRiscImpact::get_axis(u8* o_p_buf)
{
	u8 p_addr[1];
	p_addr[0] = ADXL375B_DATAX0;

	m_p_i2c_c->impact_write_read(p_addr,1,o_p_buf,ADXL375B_DATA_SIZE);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::interrupt_disable(void)
{
	adxl_reg_u reg_u;
	reg_u.INT_EN.DATA = 0;
	reg_u.INT_EN.BIT.SSHOCK = eOFF;
	reg_write(ADXL375B_INT_ENABLE,reg_u.INT_EN.DATA );	
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::interrupt_enable(void)
{
	adxl_reg_u reg_u;
	reg_u.INT_EN.DATA = 0;
	reg_u.INT_EN.BIT.SSHOCK = eON;
	reg_write(ADXL375B_INT_ENABLE,reg_u.INT_EN.DATA );
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::queue_rear_inc(void)
{
	m_n_queue_rear++;
	if ( m_n_queue_rear == ADXL375B_QUEUE_SIZE)
	{
		m_n_queue_rear = 0;
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::queue_front_inc(void)
{
	m_n_queue_front++;
	if ( m_n_queue_front == ADXL375B_QUEUE_SIZE)
	{
		m_n_queue_front = 0;
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16* CRiscImpact::get_buffer_point(void)
{
	return (u16*)m_p_impact_queue[m_n_queue_front];
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::buffer_front_clear(void)
{
	queue_front_inc();
	
	m_n_queue_num_of_events--;
	if( m_n_queue_num_of_events < 0)
		m_n_queue_num_of_events = 0;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32	CRiscImpact::get_num_of_events(void)
{
	return m_n_queue_num_of_events;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::impact_get_pos_proc(void)
{
	m_n_cur_idx = 0;
	
	m_b_stable_pos_proc_is_running = true;
	    
	while(m_b_stable_pos_proc_is_running)
	{
		if(m_b_stable_proc_hold == false)
		{	
			impact_read_pos(NULL, NULL, NULL, true);
			sleep_ex(20000);	// 50Hz measurement mode
		}
		else
		{
			sleep_ex(500000);
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
bool CRiscImpact::is_stable_proc_is_running(void)
{
	return m_b_stable_pos_proc_is_running;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::set_impact_extra_detailed_offset(impact_cal_extra_detailed_offset_t* i_p_impact_cal_extra_detailed_offset)
{
	memcpy(&m_impact_cal_extra_detailed_offset_t, i_p_impact_cal_extra_detailed_offset, sizeof(impact_cal_extra_detailed_offset_t));
	print_log(DEBUG, 1, "[%s] Set detailed offset x: %d, y: %d, z: %d\n", m_p_log_id, m_impact_cal_extra_detailed_offset_t.s_x, m_impact_cal_extra_detailed_offset_t.s_y, m_impact_cal_extra_detailed_offset_t.s_z);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::get_impact_extra_detailed_offset(impact_cal_extra_detailed_offset_t* o_p_impact_cal_extra_detailed_offset)
{
	memcpy(o_p_impact_cal_extra_detailed_offset, &m_impact_cal_extra_detailed_offset_t, sizeof(impact_cal_extra_detailed_offset_t));
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::load_6plane_sensor_cal_done(u8 i_c_done)
{
	m_c_6plane_cal_done = i_c_done;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u8 CRiscImpact::get_6plane_sensor_cal_done(void)
{
	return m_c_6plane_cal_done;
}

/******************************************************************************/
/**
 * @brief   해당 면의 cal done bit flag set
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::set_6plane_sensor_cal_done(ACC_DIRECTION i_dir_e)
{
	m_c_6plane_cal_done |= (0x01 << i_dir_e);
}

/******************************************************************************/
/**
 * @brief   i_dir_e 면 calibration 수행 결과가 invalid 할 때, i_c_misdirected 이 1로 설정된다.
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::set_6plane_sensor_cal_misdir(ACC_DIRECTION i_dir_e, u8 i_c_misdirected)
{
	if(i_c_misdirected)
		m_c_6plane_cal_misdir |= (0x01 << i_dir_e);
	else
		m_c_6plane_cal_misdir &= ~(0x01 << i_dir_e);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u8 CRiscImpact::get_6plane_sensor_cal_misdir(void)
{
	return m_c_6plane_cal_misdir;
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::reset_6plane_cal_variables(void)
{
	m_c_6plane_cal_done = 0;
	m_c_6plane_cal_misdir = 0;
	
	m_w_ax_p_zaxis = 0;
	m_w_ay_p_zaxis = 0;
	
	m_w_ax_n_zaxis = 0;
	m_w_ay_n_zaxis = 0;
	
	m_w_az_p_xaxis = 0;
	m_w_ay_p_xaxis = 0;
	
	m_w_az_n_xaxis = 0;
	m_w_ay_n_xaxis = 0;
	
	m_w_ax_p_yaxis = 0;
	m_w_az_p_yaxis = 0;
	
	m_w_ax_n_yaxis = 0;
	m_w_az_n_yaxis = 0;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CRiscImpact::acc_gyro_combo_sensor_init(void)
{
	m_p_spi_c->acc_gyro_combo_sensor_init();
	print_log(DEBUG, 1, "[%s] gyro sensor initialized\n", m_p_log_id);
}

/******************************************************************************/
/**
* \brief        constructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CMicomImpact::CMicomImpact(CVWSPI* i_p_spi_c, CMicom* i_p_micom_c, int (*log)(int,int,const char *,...)):CImpact(i_p_spi_c, log)
{
	print_log = log;
	strcpy( m_p_log_id, "MICOMIMPACT   " );
	
	m_p_spi_c = i_p_spi_c;
	m_p_micom_c = i_p_micom_c;

	m_b_log_is_running = false;
	
    // impact sensor calibration
    m_b_cal_is_running = false;
    m_c_cal_progress = 0;
    m_b_cal_result = false;
    m_b_impact_cal_result = false;
    
    m_cal_thread_t = (pthread_t)0;
    m_w_impact_count = 0;

    m_b_stable_proc_hold = false;
}

/******************************************************************************/
/**
* \brief        destructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CMicomImpact::~CMicomImpact()
{
	if( m_b_log_is_running )
    {
        m_b_log_is_running = false;
    	
    	if( pthread_join( m_log_thread_t, NULL ) != 0 )
    	{
    		print_log( ERROR, 1,"[%s] pthread_join: log_proc:%s\n", m_p_log_id, strerror( errno ));
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
void CMicomImpact::initialize(void)
{
	//update impact count
    FILE*	p_file = NULL;		// for number of impact file
    
    //read number of impact file
	p_file = fopen(NUMBER_OF_IMPACT_FILE_V2,"rb");
    if(p_file != NULL)
    {
    	fread((u8*)&m_w_impact_count,1,sizeof(u32),p_file);
    	fclose(p_file);
    }
	else
	{
		print_log(ERROR,1,"[%s] %s open failed\n", m_p_log_id, NUMBER_OF_IMPACT_FILE_V2 );
	}

	impact_reg_init();
	impact_log_init();

	if (is_gyro_exist())
		acc_gyro_combo_sensor_init();

    // launch impact logging thread
    m_b_log_is_running = true;
	
	if( pthread_create(&m_log_thread_t, NULL, impact_logging_routine, ( void * ) this)!= 0 )
    {
    	print_log( ERROR, 1, "[%s] micom_impact_logging_routine pthread_create:%s\n", \
    	                    m_p_log_id, strerror( errno ));
		
		m_b_log_is_running = false;
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicomImpact::impact_reg_init(void)
{
	adxl_reg_u reg_u;
	
	reg_write(ADXL375B_THRESH_SHOCK,200);
	reg_write(ADXL375B_DUR,255);

	reg_u.SHOCK_AXES.DATA = 0;
	reg_u.SHOCK_AXES.BIT.SZ_EN = eON;
	reg_u.SHOCK_AXES.BIT.SY_EN = eON;
	reg_u.SHOCK_AXES.BIT.SX_EN = eON;
	reg_u.SHOCK_AXES.BIT.SUPPRESS = 0;
	reg_write(ADXL375B_SHOCK_AXES,reg_u.SHOCK_AXES.DATA );

	reg_u.BW_RATE.DATA = 0;
	reg_u.BW_RATE.BIT.RATE = eADXL375B_OUTPUT_RATE_400HZ;
	reg_write(ADXL375B_BW_RATE,reg_u.BW_RATE.DATA );

	reg_u.DATA_FORMAT.DATA = 0;
	reg_u.DATA_FORMAT.BIT.FULL_RES = eON;	
	reg_u.DATA_FORMAT.BIT.RANGE = 3;	
	reg_write(ADXL375B_DATA_FORMAT,reg_u.DATA_FORMAT.DATA );

	reg_u.INT_MAP.DATA = 0;
	reg_u.INT_MAP.BIT.SSHOCK = eINT2;
	reg_u.INT_MAP.BIT.WATERMARK = eINT1;
	reg_write(ADXL375B_INT_MAP,reg_u.INT_MAP.DATA );

	reg_u.FIFO_CTL.DATA = 0;
	reg_u.FIFO_CTL.BIT.FIFO_MODE = eFIFO;
	reg_u.FIFO_CTL.BIT.SAMPLE = ADXL375B_PREBUF_SIZE;	
	reg_write(ADXL375B_FIFO_CTL,reg_u.FIFO_CTL.DATA );

	reg_u.INT_EN.DATA = 0;
	reg_u.INT_EN.BIT.SSHOCK = eON;
	reg_u.INT_EN.BIT.WATERMARK = eON;
	reg_write(ADXL375B_INT_ENABLE,reg_u.INT_EN.DATA );
	
	reg_u.POW_CTL.DATA = 0;
	reg_u.POW_CTL.BIT.MEASURE = eON;
	reg_write(ADXL375B_POWER_CTL,reg_u.POW_CTL.DATA);

}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicomImpact::cal_log_save(void)
{
	FILE *p_fp;
	s8      p_file_name[256];
	struct tm*  p_gmt_tm_t;
	u32 n_time = time(NULL);
	
    signed char c_ofs_x, c_ofs_y, c_ofs_z;
    float x, y, z;
    
    c_ofs_x = (s8)reg_read(ADXL375B_OFSX);
    c_ofs_y = (s8)reg_read(ADXL375B_OFSY);
    c_ofs_z = (s8)reg_read(ADXL375B_OFSZ);
    
    x = c_ofs_x * 1.56f;
    y = c_ofs_y * 1.56f;
    z = c_ofs_z * 1.56f;
	
	sprintf(p_file_name,"/tmp/%s",IMPACT_CAL_LOG_FILE);
	
	p_fp = fopen(p_file_name,"w");
	
	p_gmt_tm_t = localtime( (time_t*)&n_time );
    fprintf( p_fp, "%04d-%02d-%02d %02d:%02d:%02d\noffset X: %4.3fG, Y: %4.3fG, Z: %4.3fG\n", \
                        p_gmt_tm_t->tm_year+1900, p_gmt_tm_t->tm_mon+1, p_gmt_tm_t->tm_mday, \
                        p_gmt_tm_t->tm_hour, p_gmt_tm_t->tm_min, p_gmt_tm_t->tm_sec,\
                        x, y, z);

	fclose(p_fp);
	
	sys_command("cp %s %s", p_file_name, SYS_LOG_PATH);
    sys_command("sync");
	
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CMicomImpact::impact_cal_proc(void)
{
    print_log(DEBUG, 1, "[%s] start cal_proc...\n", m_p_log_id);
    accel_sensors_calibration();

    m_c_cal_progress = 100;
    
    print_log(DEBUG, 1, "[%s] stop cal_proc...\n", m_p_log_id);
    pthread_detach(m_cal_thread_t);
    m_b_cal_is_running = false;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CMicomImpact::impact_logging_proc(void)
{
	print_log(DEBUG, 1, "[%s] start log_proc...(%d)\n", m_p_log_id, syscall( __NR_gettid ));
	
	while( m_b_log_is_running == true )
	{
		/*
		if( is_log_exist(&bit_map) == true )
		{
			log_save();
		}
		*/

		impact_log_collection_routine();
		sleep_ex(500000);
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicomImpact::impact_calibration_start(void)
{
    m_c_cal_progress = 0;
    m_b_cal_result = false;
    m_b_impact_cal_result = false;
    
    if( m_b_cal_is_running == true )
    {
        impact_calibration_stop();
    }
    
    m_b_cal_is_running = true;
	if( pthread_create(&m_cal_thread_t, NULL, impact_cal_routine, ( void * ) this)!= 0 )
    {
    	print_log( ERROR, 1, "[%s] impact_cal_routine pthread_create:%s\n", m_p_log_id, strerror( errno ));

    	m_b_cal_is_running = false;
    	m_c_cal_progress = 100;
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicomImpact::impact_calibration_stop(void)
{
    if( m_b_cal_is_running )
    {
        m_b_cal_is_running = false;
    	
    	if( pthread_join( m_cal_thread_t, NULL ) != 0 )
    	{
    		print_log( ERROR, 1,"[%s] pthread_join: cal_proc:%s\n", m_p_log_id, strerror( errno ));
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
void CMicomImpact::impact_read_pos( s32* o_p_x, s32* o_p_y, s32* o_p_z, bool i_b_direct )
{
	s16 p_temp_pos[3] = {0,};
	m_p_micom_c->impact_read_pos(&p_temp_pos[0], &p_temp_pos[1], &p_temp_pos[2]);

	*o_p_x = (s32)p_temp_pos[0];
	*o_p_y = (s32)p_temp_pos[1];
	*o_p_z = (s32)p_temp_pos[2];
}

/******************************************************************************/
/**
 * @brief	check if impact calibration is valid
 * @param	None
 * @return	T/F
 * @note	for impact sensor calibration, it should ideally be (x,y,z) = (0,0,1000)
 * 			False if x and y have a +-300 difference or z has a negative value
 * 			20220510 If you write the values of OFF_X, OFF_Y, and OFF_Z and proceed right away, it doesn't print 'Done', so it is corrected to proceed after combo sensor cal
 *******************************************************************************/
bool CMicomImpact::check_impact_cal(void)
{
	acc_data_t temp_acc_data_t;
	s16 w_diff1 = 0,	w_diff2 = 0;

	impact_read_pos(&temp_acc_data_t.w_x, &temp_acc_data_t.w_y, &temp_acc_data_t.w_z);

	w_diff1 = 0 - temp_acc_data_t.w_x;
	w_diff2 = 0 - temp_acc_data_t.w_y;

	if ((w_diff1 > 300 || w_diff1 < -300) || (w_diff2 > 300 || w_diff2 < -300) || temp_acc_data_t.w_z < 0)
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
void CMicomImpact::impact_set_threshold(u8 i_c_threshold)
{
 	float f_value;
 	u8 c_threshold = i_c_threshold;
    
    if(c_threshold == 0)
    {
    	c_threshold = 100;
    	print_log(DEBUG, 1, "[%s] Invalid threshold, change to default val %dG->%dG.\n", m_p_log_id, i_c_threshold, c_threshold );
    }
    
    if( c_threshold > 200 )
    {
        f_value = 255;
    }
    else
    {
        f_value = (255 * c_threshold) / 200.0f;
        f_value += 0.5;
    }
    
   print_log(DEBUG, 1, "[%s] threshold %dG -> %d * 780mG\n", m_p_log_id, c_threshold, (u8)f_value );
   reg_write(ADXL375B_THRESH_SHOCK, (u8)f_value);
   
   m_p_micom_c->get( eMICOM_EVENT_IMPACT_REG_SAVE );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u8 CMicomImpact::impact_get_threshold(void)
{ 
    return reg_read(ADXL375B_THRESH_SHOCK);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CMicomImpact::get_max_value( u8* i_p_log)
{
	u16 			w_maximum_val = 0;;
	u32				n_idx;
	adxl_data_t* 	p_adxl_data_t = (adxl_data_t*)i_p_log;
	u16				w_x;
	u16				w_y;
	u16				w_z;
	
	for( n_idx = 0; n_idx < ADXL375B_MICOM_FIFO_SIZE ; n_idx++)
	{
		u16 w_sub_maximum;
		w_x = abs(p_adxl_data_t[n_idx].w_x);
		w_y = abs(p_adxl_data_t[n_idx].w_y);
		w_z = abs(p_adxl_data_t[n_idx].w_z);
		
		if( w_x < w_y)
		{
			w_sub_maximum = w_y;
		}
		else
		{
			w_sub_maximum = w_x;
		}
		if( w_sub_maximum < w_z)
		{
			w_sub_maximum = w_z;
		}
		if( w_maximum_val < w_sub_maximum)
		{
			w_maximum_val = w_sub_maximum;
		}
	}
	return w_maximum_val;
}
	
/********************************************************************************
 * \brief		self programing
 * \param
 * \return
 * \note		ADXL375(accelerometer) 1 plane calibration & ISM330DLC(gyro combo) calibration
 ********************************************************************************/
void CMicomImpact::accel_sensors_calibration()
{
	impact_sensor_calibration(eADXL375B_OUTPUT_RATE_50HZ, m_p_offset_50Hz);

	impact_sensor_calibration(eADXL375B_OUTPUT_RATE_400HZ, NULL);

	m_c_cal_progress = 45;

	//ISM330DLC(Combo sensor) calibration Start
	gyro_sensor_calibration();
	
	m_b_cal_result = true;

	// check impact sensor calibration
	m_b_impact_cal_result = check_impact_cal();

	m_c_cal_progress = 95;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CMicomImpact::impact_diagnosis(void)
{
    u8      c_old    = reg_read(ADXL375B_THRESH_SHOCK);
    bool    b_result = true;
    
    reg_write(ADXL375B_THRESH_SHOCK, 26);
    
    if( reg_read(ADXL375B_THRESH_SHOCK) != 26 )
    {
        b_result = false;
    }
    
    reg_write(ADXL375B_THRESH_SHOCK, c_old);
    
    return b_result;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicomImpact::get_gyro_position( s32* o_p_x, s32* o_p_y, s32* o_p_z  )
{
	float f_sensitivity = 0.061f;
	float f_data[3];
	s16   w_data[3];
	
	m_p_spi_c->acc_gyro_get_pos( &w_data[0], &w_data[1], &w_data[2] );

	f_data[0] = (float)w_data[0] * f_sensitivity;
	f_data[1] = (float)w_data[1] * f_sensitivity;
	f_data[2] = (float)w_data[2] * f_sensitivity;
	
	// [20240514] [giwook] ECR23-I006 이전의 모델과 호환성을 위해 별도의 x, y, z값 변경없이 SDK에 전달
	*o_p_x = (s32)f_data[0];
	*o_p_y = (s32)f_data[1];
	*o_p_z = (s32)f_data[2];
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CMicomImpact::get_log_num(void)
{
	// get the number of impact files    
    s8	p_buf[20];
    s8  p_cmd_buf[256];
    
    sprintf(p_cmd_buf,"ls -l %s*.csv | grep ^- | wc -l",SYS_IMPACT_V2_LOG_PATH);
    if( get_shell_cmd_output(p_cmd_buf,20,p_buf) < 0)
    {
    	print_log(DEBUG, 1, "[%s] popen read error\n",m_p_log_id);
    	return 0;
    }
	return atoi(p_buf);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicomImpact::impact_log_init(void)
{
    // make imapct log directory
	sys_command( "mkdir %s", SYS_IMPACT_LOG_PATH );
    sys_command( "mkdir %s", SYS_IMPACT_V2_LOG_PATH );
    
    u32 n_impact_file_num;
    
   
    n_impact_file_num = get_log_num();
    print_log(INFO, 1, "[%s] impact file num : %d\n",m_p_log_id, n_impact_file_num );
    
    if( n_impact_file_num > 100)
    {
	    // Delete files older than 90 days
	    sys_command( "find %s*.csv -mtime +91 -print | head -n %d | xargs rm", SYS_IMPACT_V2_LOG_PATH,n_impact_file_num - 100 );
	}
    
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CMicomImpact::impact_log_collection_routine(void)
{ 
    u16 w_bit_map = get_bit_map();
    
	if( w_bit_map == 0x00 )	//empty 
		return false;
	
	//when impact is detected, delay 1s for securing the time for writing impact packet to eeprom
	print_log(DEBUG, 1, "[%s] delay writing impact log for 1s...\n", m_p_log_id);
	sleep_ex(1000000);

	//bit map exist and need to be collected
	print_log(DEBUG, 1, "[%s] micom buffer bit map : 0x%x\n", m_p_log_id, w_bit_map);

	// read impact buf infomation from micom
	FILE*   p_file = NULL;
	s8      p_file_name[256];
	u32 	p_timestamp[IMPACT_NUMBER_OF_BUF];
    u16 	p_maximum_val[IMPACT_NUMBER_OF_BUF];
    bool	p_keep_flag[IMPACT_NUMBER_OF_BUF];
    u16		i;
    struct tm*  p_gmt_tm_t;
    FILE*	p_file_for_cnt = NULL;		// for number of impact file
    u32		temp_impact_count = m_w_impact_count;
    u32		n_time;
	u16		w_bw_rate;
    	    
    memset(p_timestamp,0,sizeof(p_timestamp));
    memset(p_maximum_val,0,sizeof(p_maximum_val));
    memset(p_keep_flag,0,sizeof(p_keep_flag));
    
    // for writing file and finding buffer including best maximum value
	for( i = 0; i < IMPACT_NUMBER_OF_BUF; i++ )
	{
		if( ( w_bit_map & (1 << i) ) != 0 )
    	{
    		//read timestamp, maximum value, keeping flag
    		get_timestamp(i,&p_timestamp[i],&w_bw_rate,&p_keep_flag[i]);   		

			//print_log(DEBUG, 1, "[%s] get_buffer. idx: %d keep: %d\n", m_p_log_id, i, p_keep_flag[i]);

    		//make file name by timestamp and impact file idx
		    n_time = time(NULL);
			p_gmt_tm_t = localtime( (time_t*)&n_time );
			
		    sprintf( p_file_name, "/tmp/%04d_%02d_%02d_%02d_%02d_%02d_%05d_ImpactLog.csv", \
		                        p_gmt_tm_t->tm_year+1900, p_gmt_tm_t->tm_mon+1, p_gmt_tm_t->tm_mday, \
		                        p_gmt_tm_t->tm_hour, p_gmt_tm_t->tm_min, p_gmt_tm_t->tm_sec,temp_impact_count );
		    
		    print_log(DEBUG, 1, "[%s] impact log file %s(0x%08X)\n", m_p_log_id, p_file_name, p_timestamp[i]);
		    
		    p_file = fopen( p_file_name, "w" );
		    
		    if( p_file == NULL )
		    {
		        print_log(ERROR, 1, "[%s] impact_log file create failed\n", m_p_log_id);
		        continue;
		    }
			
	   		//read impact infomation from micom and write to file
			if( p_keep_flag[i] == false )
    		{
    		    memset( m_p_impact_buf, 0, ADXL375B_MICOM_BUF_SIZE );    		    
		    
			    if( log_read( i, m_p_impact_buf, w_bw_rate ) == true )
			    {
			        log_write( p_file, m_p_impact_buf, w_bw_rate );
			        p_maximum_val[i] = get_max_value( m_p_impact_buf);
			        			        
			        fclose(p_file);
			    	p_file = NULL;
			    	sys_command("cp %s %s", p_file_name, SYS_IMPACT_V2_LOG_PATH);        		
        			sys_command("rm %s", p_file_name );
        			temp_impact_count++;
        			print_log(ERROR, 1, "[%s] storing impact log(%s)\n", m_p_log_id, p_file_name );
        			
			    }
			    else
			    {
			        print_log(ERROR, 1, "[%s] reading impact log failed(%d)\n", m_p_log_id, i );
	   			    fclose(p_file);
				    p_file = NULL;
					continue;		//do not clear buffer
			    }
	   		}
			
			//clearing buffer in micom eeprom and keeping a buffer in micom eeprom.
			clear_buffer( p_maximum_val[i], i );

       		//printf("%d: timestamp : %u, maximum val :  %d, p_keep_flag : %d\n",i,p_timestamp[i],p_maximum_val[i] * 49,p_keep_flag[i]);
		}
	}
	
	//writing number of impact file
	if( (p_file_for_cnt = fopen(NUMBER_OF_IMPACT_FILE_V2,"wb")) != NULL)
    {
    	fwrite((u8*)&temp_impact_count,1,sizeof(u32),p_file_for_cnt);
    	fclose(p_file_for_cnt);
    }
    sys_command("sync");
	m_w_impact_count = temp_impact_count;
	print_log(DEBUG, 1, "[%s] impact count: %d\n", m_p_log_id, m_w_impact_count );

	return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CMicomImpact::log_read( u8 i_c_idx, u8* o_p_log , u16 i_w_bw_rate )
{
	u8  p_send[COMMAND_DATA_LEN];
	u8  p_recv[COMMAND_BIG_DATA_LEN];
	u16 i;
	print_log(ERROR, 1, "[%s] log_read start\n", m_p_log_id);
	for( i = 0; i < (IMPACT_GROUP_SIZE * ADXL375B_DATA_SIZE); i++ )
	{
	    memset( p_send, 0, sizeof(p_send) );
	    memset( p_recv, 0, sizeof(p_recv) );
	    
	    p_send[0] = i_c_idx;
    	
    	if( m_p_micom_c->get( (MICOM_EVENT)(eMICOM_EVENT_IMPACT_0 + i), p_send, p_recv, 1 ) == false )
    	{
    		print_log(DEBUG, 1, "[%s] log read error: %d\n", m_p_log_id );
			return false;
    	}
    	
    	memcpy( &o_p_log[i * COMMAND_BIG_DATA_LEN], p_recv, COMMAND_BIG_DATA_LEN );
    }
	print_log(ERROR, 1, "[%s] log_read end\n", m_p_log_id);
	return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicomImpact::log_write( FILE* i_p_file, u8* i_p_log, u16 i_w_bw_rate )
{
    //V2 헤더 작성 (V2, DATA_RATE, LEVEL1_HEIGHT, LEVEL2_HEIGHT, correction factor)
	write_impactV2_header(i_p_file, i_w_bw_rate);

	if(i_w_bw_rate == eADXL375B_OUTPUT_RATE_50HZ) 	    
	{
		write_impactV2_data(i_p_file, i_p_log, 128);
	}
	else if(i_w_bw_rate == eADXL375B_OUTPUT_RATE_400HZ)
	{
		write_impactV2_data(i_p_file, i_p_log, ADXL375B_MICOM_FIFO_SIZE);
	}
	else //default 50Hz
	{
		write_impactV2_data(i_p_file, i_p_log, 128);
	}
    
}

/******************************************************************************/
/**
 * @brief   V2 헤더 작성 (V2, DATA_RATE, LEVEL1_HEIGHT, LEVEL2_HEIGHT, correction factor, filtering count)
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicomImpact::write_impactV2_header(FILE* i_p_file, u16 i_w_bw_rate)
{
	if(i_w_bw_rate == eADXL375B_OUTPUT_RATE_50HZ)
	{
		fprintf(i_p_file, "V2, %d, %d, %d, %d, %d\n",
			(int)(25*pow(2, i_w_bw_rate - eADXL375B_OUTPUT_RATE_25HZ)), LEVEL1_HEIGHT, LEVEL2_HEIGHT, CORRECTION_FACTOR_50HZ, FILTERING_COUNT_50HZ );
	}
	else if(i_w_bw_rate == eADXL375B_OUTPUT_RATE_400HZ)
	{
		fprintf(i_p_file, "V2, %d, %d, %d, %d, %d\n",
			(int)(25*pow(2, i_w_bw_rate - eADXL375B_OUTPUT_RATE_25HZ)), LEVEL1_HEIGHT, LEVEL2_HEIGHT, CORRECTION_FACTOR_400HZ, FILTERING_COUNT_400HZ );
	}
	else //default 50Hz
	{
		fprintf(i_p_file, "V2, %d, %d, %d, %d, %d\n",
			(int)(25*pow(2, i_w_bw_rate - eADXL375B_OUTPUT_RATE_25HZ)), LEVEL1_HEIGHT, LEVEL2_HEIGHT, CORRECTION_FACTOR_50HZ, FILTERING_COUNT_50HZ );
	}
}

/******************************************************************************/
/**
 * @brief   V2 내용 작성 
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicomImpact::write_impactV2_data(FILE* i_p_file, u8* i_p_log, u16 i_w_idx)
{
	adxl_data_t* 	p_adxl_data_t = (adxl_data_t*)i_p_log;
	u16 i;

	for( i = 0 ; i < i_w_idx; i++)
	{
		if( i_p_file == NULL )
		{
			printf("idx(%3d),%6d,%6d,%6d\n", \
					i, \
					(p_adxl_data_t[i].w_x) * 49, \
					(p_adxl_data_t[i].w_y) * 49, \
					(p_adxl_data_t[i].w_z) * 49 );
		}
		else
		{
			fprintf(i_p_file, "idx(%3d),%6d,%6d,%6d\n", \
					i, \
					(p_adxl_data_t[i].w_x) * 49, \
					(p_adxl_data_t[i].w_y) * 49, \
					(p_adxl_data_t[i].w_z) * 49 );
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
u8 CMicomImpact::reg_read( u8 i_c_addr )
{
	u8 p_send[COMMAND_DATA_LEN] = {0,};
	u8 p_recv[COMMAND_DATA_LEN] = {0,};
	
	p_send[0] = i_c_addr;
	
    if( m_p_micom_c->get( eMICOM_EVENT_IMPACT_REG_READ, p_send, p_recv ) == false )
    {
        return 0;
    }
	
	return p_recv[0];
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicomImpact::reg_write( u8 i_c_addr, u8 i_c_data )
{
	u8 p_send[COMMAND_DATA_LEN] = {0,};
	u8 p_recv[COMMAND_DATA_LEN] = {0,};
	
	p_send[0] = i_c_addr;
	p_send[1] = i_c_data;
	
	m_p_micom_c->get( eMICOM_EVENT_IMPACT_REG_WRITE, p_send, p_recv );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    NOT USED
*******************************************************************************/
bool CMicomImpact::is_log_exist(void)
{
    u8 p_data[COMMAND_DATA_LEN];
    micom_state_t	state_t;
    
    memset( p_data, 0, sizeof(p_data) );
    
    if( m_p_micom_c->get(eMICOM_EVENT_STATE, NULL, p_data ) == false )
    {
        return false;
    }
    
    memcpy( &state_t, p_data, sizeof(micom_state_t) );
    
    if( state_t.IO_STATUS.BIT.ADXL_FULL == 1 )
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
u16 CMicomImpact::get_bit_map(void)
{
	u8 	p_send[COMMAND_DATA_LEN] = {0,};
	u8 	p_recv[COMMAND_DATA_LEN] = {0,};
	u16 w_bit_map = 0;;

	if( m_p_micom_c->get( eMICOM_EVENT_IMPACT_BUF_CNT, p_send, p_recv ) ==  true )
	{
		w_bit_map = *(u16*)&p_recv[0];
	}
	
	return w_bit_map;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicomImpact::get_timestamp(u32 i_n_idx, u32* o_p_timestamp, u16* o_p_bw_rate, bool* o_p_keep_flag)
{
	u8 p_send[COMMAND_DATA_LEN] = {0,};
	u8 p_recv[COMMAND_DATA_LEN] = {0,};
	
	p_send[0] = i_n_idx;
	
	m_p_micom_c->get( eMICOM_EVENT_TIMESTAMP_GET, p_send, p_recv );
	
	*o_p_timestamp = swap_32(*(u32*)p_recv);
	*o_p_bw_rate = *(u16*)&p_recv[4]; //RISC 전원 off, on을 구별하기 위함
	*o_p_keep_flag = p_recv[6] == 1;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicomImpact::clear_buffer(u16 i_w_max_val, u32 i_n_idx, bool i_b_keep_flag)
{
	u8 p_send[COMMAND_DATA_LEN] = {0,};
	u8 p_recv[COMMAND_DATA_LEN] = {0,};
	
	p_send[0] = i_n_idx;
	p_send[1] = (i_b_keep_flag == true);
	*(u16*)&p_send[2] = i_w_max_val;

	//print_log(DEBUG, 1, "[%s] clear_buffer. idx: %d keep: %d\n", m_p_log_id, i_n_idx, i_b_keep_flag);

	m_p_micom_c->get( eMICOM_EVENT_IMPACT_BUF_ONE_CLEAR, p_send, p_recv );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicomImpact::impact_print(void)
{
    u8 c_threshold;
    signed char c_ofs_x, c_ofs_y, c_ofs_z;
    float x, y, z;
    
    adxl_data_t	adxl_t;
    memset(&adxl_t, 0, sizeof(adxl_data_t));
    
    c_threshold = reg_read(ADXL375B_THRESH_SHOCK);
    
    c_ofs_x = (s8)reg_read(ADXL375B_OFSX);
    c_ofs_y = (s8)reg_read(ADXL375B_OFSY);
    c_ofs_z = (s8)reg_read(ADXL375B_OFSZ);
    
    x = c_ofs_x * 196 / 1000;
    y = c_ofs_y * 196 / 1000;
    z = c_ofs_z * 196 / 1000;
    
    printf("threshold: %fG\n", (c_threshold * 780)/1000.0f);
    printf("offset X: %4.3fG, Y: %4.3fG, Z: %4.3fG\n", x, y, z);
    
    impact_read_pos(&adxl_t.w_x, &adxl_t.w_y, &adxl_t.w_z);
    x = adxl_t.w_x/1000.0f;
    y = adxl_t.w_y/1000.0f;
    z = adxl_t.w_z/1000.0f;
    
    printf("acc X: %4.3fG, Y: %4.3fG, Z: %4.3fG\n", x, y, z);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicomImpact::save_config(s8 i_c_offs_x, s8 i_c_offs_y, s8 i_c_offs_z)
{
	u8 p_send[COMMAND_DATA_LEN] = {0,};
	
	m_p_micom_c->get( eMICOM_EVENT_IMPACT_REG_SAVE );
	
	p_send[0] = i_c_offs_x;
	p_send[1] = i_c_offs_y;
	p_send[2] = i_c_offs_z;
	p_send[3] = m_p_offset_50Hz[0];
	p_send[4] = m_p_offset_50Hz[1];
	p_send[5] = m_p_offset_50Hz[2];
	
	m_p_micom_c->get( eMICOM_EVENT_ACC_GYRO_CAL_DATA_SAVE, p_send, NULL );
	
	cal_log_save();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicomImpact::get_axis(adxl_data_t* o_p_axis_data_t)
{
	u8 p_send[COMMAND_DATA_LEN] = {0,};
	u8 p_recv[COMMAND_DATA_LEN] = {0,};
	
	m_p_micom_c->get(eMICOM_EVENT_STATE,p_send,p_recv);
	
	memcpy(o_p_axis_data_t,&p_recv[2],sizeof(adxl_data_t));
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicomImpact::interrupt_disable(void)
{
	adxl_reg_u reg_u;
	reg_u.INT_EN.DATA = 0;
	reg_u.INT_EN.BIT.SSHOCK = eOFF;
	reg_write(ADXL375B_INT_ENABLE,reg_u.INT_EN.DATA );	
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicomImpact::interrupt_enable(void)
{
	adxl_reg_u reg_u;
	reg_u.INT_EN.DATA = 0;
	reg_u.INT_EN.BIT.SSHOCK = eON;
	reg_write(ADXL375B_INT_ENABLE,reg_u.INT_EN.DATA );
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicomImpact::acc_gyro_combo_sensor_init(void)
{
	u8 p_recv[COMMAND_DATA_LEN] = {0,};
	
	m_p_spi_c->acc_gyro_combo_sensor_init();
	
	m_p_micom_c->get(eMICOM_EVENT_ACC_GYRO_CAL_DATA_REQ, NULL, p_recv);

	m_p_spi_c->acc_gyro_reg_write(ACC_GYRO_REG_X_OFS_USR, &p_recv[0], 1);
	m_p_spi_c->acc_gyro_reg_write(ACC_GYRO_REG_Y_OFS_USR, &p_recv[1], 1);
	m_p_spi_c->acc_gyro_reg_write(ACC_GYRO_REG_Z_OFS_USR, &p_recv[2], 1);	

	print_log(DEBUG, 1, "[%s] gyro sensor initialized\n", m_p_log_id);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CMicomImpact::print_invalid_compatibility(const char* func_name)
{
	print_log( DEBUG, 1, "[%s] INVALID IN THIS DETECTOR: %s\n", m_p_log_id, func_name);
}

/********************************************************************************
 * \brief		
 * \param
 * \return
 * \note
 ********************************************************************************/
void CMicomImpact::get_impact_offset(impact_cal_offset_t* o_p_impact_offset_t)
{
	o_p_impact_offset_t->c_x = reg_read(ADXL375B_OFSX);
	o_p_impact_offset_t->c_y = reg_read(ADXL375B_OFSY);
	o_p_impact_offset_t->c_z = reg_read(ADXL375B_OFSZ);
}	

/********************************************************************************
 * \brief		
 * \param
 * \return
 * \note
 ********************************************************************************/
void CMicomImpact::set_impact_offset(impact_cal_offset_t* i_p_impact_offset_t)
{
	print_log(DEBUG, 1, "[%s]set impact Offset Data (cal data x: 0x%x y: 0x%x z: 0x%x)\n", m_p_log_id, i_p_impact_offset_t->c_x, \
																					i_p_impact_offset_t->c_y, i_p_impact_offset_t->c_z);

	reg_write(ADXL375B_OFSX,i_p_impact_offset_t->c_x);
	reg_write(ADXL375B_OFSY,i_p_impact_offset_t->c_y);
	reg_write(ADXL375B_OFSZ,i_p_impact_offset_t->c_z);
}	

/********************************************************************************
 * \brief		
 * \param
 * \return
 * \note
 ********************************************************************************/
void CMicomImpact::gyro_sensor_calibration(void)
{
	s8				c_offset_x = 0;
	s8				c_offset_y = 0;
	s8				c_offset_z = 0;

	s16 			w_x = 0;
    s16 			w_y = 0;
    s16 			w_z = 0;

    s32				n_sum_x = 0;
    s32				n_sum_y = 0;
    s32				n_sum_z = 0;
	
    u16 			w_count = 0; 
	float f_data[3] = {0.0f, };

	//user offset reset
	m_p_spi_c->acc_gyro_reg_write(ACC_GYRO_REG_X_OFS_USR, (u8*)&c_offset_x, 1);
	m_p_spi_c->acc_gyro_reg_write(ACC_GYRO_REG_Y_OFS_USR, (u8*)&c_offset_y, 1);
	m_p_spi_c->acc_gyro_reg_write(ACC_GYRO_REG_Z_OFS_USR, (u8*)&c_offset_z, 1);
	
	//Offset measurement 
	while(w_count < ADX_CAL_SAMPLE_NUM)
    {	
		m_p_spi_c->acc_gyro_get_pos( &w_x, &w_y, &w_z );
		f_data[0] += ( (float)w_x * 0.061f );			//FS +-2G, SAMPLE UNIT: 0.061mg/LSB
		f_data[1] += ( (float)w_y * 0.061f );
		f_data[2] += ( (float)w_z * 0.061f );
		
		w_count++;
		m_c_cal_progress += 2;
		if( m_c_cal_progress >= 90 )
			m_c_cal_progress = 90;
			
		sleep_ex(100000);      // 100 ms
	}
	
	m_c_cal_progress = 90;
	
	n_sum_x = (s32)f_data[0];
	n_sum_y = (s32)f_data[1];
	n_sum_z = (s32)f_data[2];
	
	//offset measurement average calculation
	n_sum_x /= ADX_CAL_SAMPLE_NUM;
	n_sum_y /= ADX_CAL_SAMPLE_NUM;
	n_sum_z /= ADX_CAL_SAMPLE_NUM;

	// Extract the offset value (default x: 0g, y: 0g, z:1g)
	n_sum_z = n_sum_z - 1000;
						
	//f_data UNIT --> OFFSET UNIT
	//f_data UNIT: 1mg/LSB
	//OFFSET UNIT: 0.9765625mg/LSB
	n_sum_x *= 1.0309;
	n_sum_y *= 1.0309;
	n_sum_z *= 1.0309;

	c_offset_x = -1*clip_s16(n_sum_x);			
	c_offset_y = -1*clip_s16(n_sum_y);
	c_offset_z = clip_s16(n_sum_z);
	
	m_p_spi_c->acc_gyro_reg_write(ACC_GYRO_REG_X_OFS_USR, (u8*)&c_offset_x, 1);
	m_p_spi_c->acc_gyro_reg_write(ACC_GYRO_REG_Y_OFS_USR, (u8*)&c_offset_y, 1);
	m_p_spi_c->acc_gyro_reg_write(ACC_GYRO_REG_Z_OFS_USR, (u8*)&c_offset_z, 1);
		
	save_config(c_offset_x, c_offset_y, c_offset_z);
}
	
/********************************************************************************
 * \brief		
 * \param
 * \return
 * \note
 ********************************************************************************/
void CMicomImpact::impact_sensor_calibration(u16 i_w_bw_rate, s8* i_p_offset)
{
	adxl_reg_u 		reg_u;
	adxl_reg_u 		reg_save_u;
	
	s8				c_offset_x = 0;
	s8				c_offset_y = 0;
	s8				c_offset_z = 0;
	
	s16 			w_x = 0;
    s16 			w_y = 0;
    s16 			w_z = 0;
    s32				n_sum_x = 0;
    s32				n_sum_y = 0;
    s32				n_sum_z = 0;
    
    u16 			w_count = 0; 
	u8 				p_send[COMMAND_DATA_LEN] = {0,};
    
	interrupt_disable();

	if(i_w_bw_rate == eADXL375B_OUTPUT_RATE_50HZ)
	{
		p_send[0] = 0;
		m_p_micom_c->get( eMICOM_EVENT_NEW_IMPACT_LOGIC, p_send, NULL );
	}
	else
	{
		p_send[0] = 1;
		m_p_micom_c->get( eMICOM_EVENT_NEW_IMPACT_LOGIC, p_send, NULL );
	}


	reg_save_u.FIFO_CTL.DATA = reg_read(ADXL375B_FIFO_CTL);
	
	reg_u.FIFO_CTL.DATA = 0;
	reg_u.FIFO_CTL.BIT.FIFO_MODE = eSTREAM;
	reg_u.FIFO_CTL.BIT.SAMPLE = ADXL375B_PREBUF_SIZE;	
	reg_write(ADXL375B_FIFO_CTL,reg_u.FIFO_CTL.DATA );
	
	reg_write(ADXL375B_OFSX,0);
	reg_write(ADXL375B_OFSY,0);
	reg_write(ADXL375B_OFSZ,0);
	
	reg_u.POW_CTL.DATA = 0;
	reg_u.POW_CTL.BIT.MEASURE = eON;
	reg_write(ADXL375B_POWER_CTL,reg_u.POW_CTL.DATA);

	sleep_ex(500000);
	while(w_count < ADX_CAL_SAMPLE_NUM)
    {	
		impact_read_pos(&w_x, &w_y, &w_z);		
		n_sum_x += w_x;
		n_sum_y += w_y;
		n_sum_z += w_z;
		
		w_count++;
		m_c_cal_progress += 1;
		if( m_c_cal_progress >= 40 ) // 50Hz: 20 400Hz: 20
			m_c_cal_progress = 40;
			
		sleep_ex(100000);      // 100 ms
	}
	
	n_sum_x /= ADX_CAL_SAMPLE_NUM;
	n_sum_y /= ADX_CAL_SAMPLE_NUM;
	n_sum_z = (n_sum_z / ADX_CAL_SAMPLE_NUM) - 1000; // z axi is 1G
	
	n_sum_x = (n_sum_x + (n_sum_x > 0 ? 98:-98))/196;
	n_sum_y = (n_sum_y + (n_sum_y > 0 ? 98:-98))/196;
	n_sum_z = (n_sum_z + (n_sum_z > 0 ? 98:-98))/196;
	    
								
	c_offset_x = -1*clip_s16(n_sum_x);
	c_offset_y = -1*clip_s16(n_sum_y);
	c_offset_z = -1*clip_s16(n_sum_z);
	
	if(i_p_offset != NULL)
	{
		i_p_offset[0] = c_offset_x;
		i_p_offset[1] = c_offset_y;
		i_p_offset[2] = c_offset_z;							
	}
	
	// Offset value to be set in the acceleration sensor
	// Adjustable in 196mg units only
	reg_write(ADXL375B_OFSX, c_offset_x);
	reg_write(ADXL375B_OFSY, c_offset_y);
	reg_write(ADXL375B_OFSZ, c_offset_z);

	reg_write(ADXL375B_FIFO_CTL,reg_save_u.FIFO_CTL.DATA);

	reg_u.POW_CTL.DATA = 0;
	reg_u.POW_CTL.BIT.MEASURE = eON;
	reg_write(ADXL375B_POWER_CTL,reg_u.POW_CTL.DATA);
	
	if(i_w_bw_rate == eADXL375B_OUTPUT_RATE_50HZ)
	{
		p_send[0] = 1;
		m_p_micom_c->get( eMICOM_EVENT_NEW_IMPACT_LOGIC, p_send, NULL );
	}

	interrupt_enable();
}	
