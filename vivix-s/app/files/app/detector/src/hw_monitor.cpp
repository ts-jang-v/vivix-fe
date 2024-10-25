/*******************************************************************************
 모  듈 : hw_monitor
 작성자 : 한명희
 버  전 : 0.0.1
 설  명 : AP 설정 및 exposure request 처리
 참  고 : 

 버  전:
         0.0.1  최초 작성
*******************************************************************************/



/*******************************************************************************
*	Include Files
*******************************************************************************/
#include <errno.h>          // errno
#include <stdio.h>
#include <string.h>            // memset(), memcpy()
#include <iostream>
#include <sys/ioctl.h>  	// ioctl()
#include <fcntl.h>          // open() O_NDELAY
#include <stdlib.h>         // system()
#include <sys/poll.h>		// struct pollfd, poll()
#include <unistd.h>

#include <linux/unistd.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "hw_monitor.h"
#include "vw_file.h"
#include "vworks_ioctl.h"
#include "vw_net.h"
#include "misc.h"

using namespace std;

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/
#define	BATTERY_MAX						(2)
#define BLINKING_CNT_MAX				(8)
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
* \param        port        port number
* \param        log         log function
* \return       none
* \note
*******************************************************************************/
CHWMonitor::CHWMonitor(int (*log)(int,int,const char *,...))
{
    print_log = log;
    strcpy( m_p_log_id, "HWMON         " );
    
    m_b_is_running = false;
    

    m_b_exp_req     = false;
    
    m_n_fd = -1;
    memset( &m_hw_config_t, 0, sizeof(hw_config_t) );
    
    // initialize() 함수에서 env에서 읽은 모델명으로 갱신됨 
    // 2530, 3643, 4343 사이즈 CONTROL B/D 통합에 따라 BD_CFG 삭제하여 이 시점에서는 모델명 인식 불가능
    m_model_e = eMODEL_3643VW;
        
    memset( m_battery_t, 0, sizeof(battery_t) * BATTERY_MAX );
    m_p_battery_low_time_c  = NULL;
    m_p_battery_off_time_c  = NULL;
    m_b_battery_power_off   = false;
    
    // AP button
    m_b_ap_button_on        = false;
    m_b_ap_button_pressed   = false;
    m_p_ap_button_wait_c    = NULL;

    m_c_battery_num = BATTERY_MAX;
    m_bat_thread_t = (pthread_t)0;
    m_p_i2c_c = NULL;
    m_n_adc_fd = -1;
	
    m_p_impact_c = NULL;

    m_w_impact_count = 0;
    
    m_b_phy_on = true;
    m_c_led_blink_enable = 0;
    
    m_p_charging_start_timer_c = NULL;
    m_b_pwr_src_once_connected = false;
    
	m_phy_type = ePHY_TYPE_MAX;

	m_n_fd = open("/dev/vivix_gpio",O_RDWR|O_NDELAY);
	if( m_n_fd < 0 )
	{
		print_log(ERROR,1,"[%s] /dev/vivix_gpio device open error.\n", m_p_log_id);
	}

    print_log(DEBUG, 1, "[%s] CHWMonitor\n", m_p_log_id);
}
/******************************************************************************/
/**
* \brief        constructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CHWMonitor::CHWMonitor(void)
{
}

/******************************************************************************/
/**
* \brief        destructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CHWMonitor::~CHWMonitor()
{  
	battery_proc_stop();
	 
    if( m_n_fd != -1 )
    {
        close(m_n_fd);
    }
    
    if( m_n_adc_fd != -1 )
    {
        close(m_n_adc_fd);
    }
    
    safe_delete( m_p_i2c_c );   
    
    print_log(DEBUG, 1, "[%s] ~CHWMonitor\n", m_p_log_id);
}

/******************************************************************************/
/**
 * @brief	battery의 상태 및 관련 정보를 가지고 온다.
 * @param   o_p_battery_t: 값을 옮기거나 넣은 battery_t 구조체, i_b_direct: 측정해서 값을 넣을지 여부
 * @return	None
 * @note	battery_proc에서 i_b_direct = true를 통해 값을 측정해서 m_battery_t에 넣어놓으면, vw_system에서는 측정을 하지 않고 복사해서 가져감
 *******************************************************************************/
void CHWMonitor::get_battery( battery_t* o_p_battery_t, bool i_b_direct )
{
    if( i_b_direct == false )
    {
    	battery_t           bat_t[BATTERY_MAX];
    	
    	memcpy( bat_t, m_battery_t, sizeof(battery_t) * eBATTERY_MAX );               
        memset( o_p_battery_t, 0, sizeof(battery_t) * eBATTERY_MAX );
        memcpy( o_p_battery_t, bat_t, sizeof(battery_t) * eBATTERY_MAX );
        
        return;
    }
    
	bool        b_equip[eBATTERY_MAX];
    u32			n_idx;
	bool		b_pm_mode_changed = false;

    for( n_idx = 0; n_idx < eBATTERY_MAX; n_idx++)
    {  
		b_equip[n_idx] = bat_is_equipped(n_idx);

		//배터리 착탈 상태 변경 확인
	    if( b_equip[n_idx] != m_battery_t[n_idx].b_equip )
	    {
	    	b_pm_mode_changed = true;
        	
	    	m_battery_t[n_idx].w_vcell = 0;
	    	m_battery_t[n_idx].w_soc = 0;
	    	m_battery_t[n_idx].w_soc_raw = 0;
	    	m_battery_t[n_idx].w_level = 0;
	    	m_battery_t[n_idx].b_power_warning = false;
	    	m_battery_t[n_idx].b_power_off = false;
			//m_battery_t[n_idx].b_is_stabilized = false;
	        
	        if( b_equip[n_idx] == false )
	        {
	        	print_log(DEBUG, 1, "[%s] battery %d was removed\n", m_p_log_id, n_idx);
	        }
	        else
	        {
				//bat_stabilization_init(n_idx);
				print_log(DEBUG, 1, "[%s] battery %d was equipped\n", m_p_log_id, n_idx);
	        }
	        m_battery_t[n_idx].b_equip = b_equip[n_idx];
	    }
	    
		//m_battery_t[n_idx].b_is_stabilized 업데이트
		//bat_stabilization_check(n_idx);

	    if( m_battery_t[n_idx].b_equip == true )
	    {
	        BAT_SOC_STATE       soc_state_e;
	        
	        m_battery_t[n_idx].w_vcell		= m_p_i2c_c->bat_get_volt(n_idx);	        
	        
	        if( bat_is_valid_vcell(m_battery_t[n_idx].w_vcell) )
	        	m_battery_t[n_idx].w_soc_raw	= m_p_i2c_c->bat_get_soc(n_idx);
	        else
	        	m_battery_t[n_idx].w_soc_raw	= 0;

			//soc_raw 값 validation 
			//Vcell range 변환 식을 이용해 % 변환 후 soc raw 값과 45프로 이상 차이날 경우, guage system chip restart & 값 재확인
			/*
			if( bat_is_valid_soc_raw(m_battery_t[n_idx].w_vcell, m_battery_t[n_idx].w_soc_raw) == false )				
			{
				m_p_i2c_c->bat_quick_start(n_idx);
				m_battery_t[n_idx].w_vcell		= m_p_i2c_c->bat_get_volt(n_idx);	        
				m_battery_t[n_idx].w_soc_raw	= m_p_i2c_c->bat_get_soc(n_idx);
			}	
			*/

			m_battery_t[n_idx].w_soc		= bat_get_soc((BATTERY_ID)n_idx, m_battery_t[n_idx].w_soc_raw);
	        soc_state_e						= bat_get_soc_state(m_battery_t[n_idx].w_soc);
	        
	        if( soc_state_e >= eBAT_SOC_STATE_1
	            && soc_state_e <= eBAT_SOC_STATE_5 )
	        {
	            m_battery_t[n_idx].w_level = (u16)soc_state_e;
	        }
	        else
	        {
	            m_battery_t[n_idx].w_level = eBAT_SOC_STATE_1;
	        }
	    }
	}

	if( b_pm_mode_changed)
	{		
		//b_pm_mode_changed = false;
		bat_check_charge_change();		
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CHWMonitor::bat_diagnosis(u8 i_c_sel)
{
    u16 w_old_value = m_p_i2c_c->bat_get_rcomp(i_c_sel);
    u16 w_value;
    
    m_p_i2c_c->bat_set_rcomp(0x0C,i_c_sel);
    
    w_value = m_p_i2c_c->bat_get_rcomp(i_c_sel);
    
    m_p_i2c_c->bat_set_rcomp(w_old_value,i_c_sel);
    
    if( w_value != 0x0C )
    {
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
void * battery_routine( void * arg )
{
	CHWMonitor * mon = (CHWMonitor *)arg;
	mon->battery_proc();
	pthread_exit( NULL );
}

/******************************************************************************/
/**
* \brief        battery의 soc 상태를 확인한다.
* \param        none
* \return       none
* \note
*******************************************************************************/
BAT_SOC_STATE CHWMonitor::bat_get_state(void)
{
	u16 w_soc = 0;
	u32 n_idx;
            	
	for( n_idx = 0; n_idx <  get_battery_count(); n_idx++)      	
	{
		w_soc += m_battery_t[n_idx].w_soc;
	}
	
	w_soc = (w_soc + (get_battery_count()/2))/get_battery_count();
	
    // update battery LED
    return bat_get_soc_state(w_soc);
}
	
/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CHWMonitor::battery_proc(void)
{
    print_log(DEBUG, 1, "[%s] start battery_proc...(%d)\n", m_p_log_id, syscall( __NR_gettid ));
    
    BAT_SOC_STATE   soc_state_e;
    BAT_SOC_STATE   cur_soc_state_e;
    POWER_SOURCE    power_source_e = ePWR_SRC_UNKNOWN;

    cur_soc_state_e = eBAT_SOC_STATE_OFF;
    
    soc_state_e = cur_soc_state_e;
	
	// ADC 로 모델마다 읽을 전원 값 읽어들임
    get_power_volt( &m_power_volt_t, true );
    
    bat_check_initial_vcell();
	
    while( m_b_is_running == true )
    {
        get_power_volt( &m_power_volt_t, true );
        
		if( is_power_source_once_connected() == true )
        {
        	check_shutdown_by_scu();
        }
        	
		get_battery( m_battery_t, true );
		
		if( bat_get_equip_num() > 0 )
		{
			soc_state_e = bat_get_state();
		    
			/*soc state 체크*/
		    if( cur_soc_state_e != soc_state_e )
		    {
		        cur_soc_state_e = soc_state_e;
		        
		    }
		    
			/*외부전원 변경 여부 체크*/
		    if( power_source_e != m_power_volt_t.power_source_e )
		    {
				check_pwr_src_is_battery_and_charge_stop(m_power_volt_t.power_source_e, power_source_e);
				
				if(m_st_bat_chg_mode_info.chg_mode == eCHG_MODE_RECHARGING_CONTROL)
					check_pwr_src_and_block_recharging(m_power_volt_t.power_source_e, power_source_e);
									
		        power_source_e = m_power_volt_t.power_source_e;
		    }
		    
			if(m_st_bat_chg_mode_info.chg_mode == eCHG_MODE_RECHARGING_CONTROL)
				check_battery_gauge_to_recharge();

			/*외부전원일 경우 충전 상태 변경*/
		    if( power_source_can_charge(power_source_e) == true 
				&& m_b_is_blinking_running == false)
		    {
		    	bat_check_charger_state();
				if( m_st_bat_chg_mode_info.c_block_recharging == false)
		        	bat_check_charge_change();
		    }
		    
			/*low valtage warning 체크*/
		    bat_check_low_volt();
			
			/*soc full 여부 체크*/
			check_soc_is_full();
		}
		else
		{
		    if( power_source_e != m_power_volt_t.power_source_e )
		    {
		        power_source_e = m_power_volt_t.power_source_e;
		        
		        if( power_source_e != ePWR_SRC_BATTERY )
		        {
		       		bat_charge( eBAT_CHARGE_0_OFF_1_OFF);	//배터리가 장착이 안 되어 있으면 강제적으로 bat_state 갱신
		        }
		    }
		        
		    // update battery LED
		    if( cur_soc_state_e != eBAT_SOC_STATE_OFF )
		    {
		        cur_soc_state_e = eBAT_SOC_STATE_OFF;
		    }
		}
        
        sleep_ex(500000);      // 500 ms
    }
    print_log(DEBUG, 1, "[%s] stop battery_proc...\n", m_p_log_id);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CHWMonitor::check_shutdown_by_scu(void)
{
    PM_MODE         pm_mode_e;
    
    if( m_hw_config_t.power_source_e == ePWR_SRC_TETHER
        && m_power_volt_t.power_source_e == ePWR_SRC_BATTERY )
    {
        print_log(DEBUG, 1, "[%s] Power off by SCU\n", m_p_log_id);
        
        // notify S/W
        pm_mode_e = ePM_MODE_SHUTDOWN_BY_SCU;
        message_send(eMSG_POWER_MODE, (u8*)&pm_mode_e, sizeof(pm_mode_e));
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CHWMonitor::bat_get_soc( BATTERY_ID i_id_e, u16 i_w_raw_soc )
{
    float f_raw_soc;
    float f_soc;
    
    f_raw_soc = i_w_raw_soc/10.0f;
    
    f_soc =((float)(f_raw_soc - m_p_bat_empty_soc[i_id_e] ) / (float)(m_p_bat_full_soc[i_id_e] - m_p_bat_empty_soc[i_id_e]) * (float)100);
    
    f_soc = f_soc * 10;
    
    if( f_soc > 1000 )
    {
        return 1000;
    }
    
    if( f_soc < 10 )
    {
        return 10;
    }
        
    return (u16)f_soc;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
BAT_SOC_STATE CHWMonitor::bat_get_soc_state( u16 i_w_soc )
{
    u16 w_soc;
    
    w_soc = i_w_soc/10;
    
    if( w_soc > 80 )
    {
        return eBAT_SOC_STATE_5;
    }

    if( w_soc > 60 )
    {
        return eBAT_SOC_STATE_4;
    }

    if( w_soc > 50 )
    {
        return eBAT_SOC_STATE_3;
    }

    if( w_soc > 30 )
    {
        return eBAT_SOC_STATE_2;
    }
    
    if( w_soc > 10 )
    {
        return eBAT_SOC_STATE_1;
    }
    
    return eBAT_SOC_STATE_WARN;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CHWMonitor::bat_print(void)
{
	u32 n_idx;
	u8 c_start_idx = get_bat_equip_start_idx();
	
	for( n_idx = c_start_idx; n_idx < c_start_idx + get_battery_count(); n_idx++)
	{
		print_log(DEBUG, 1, "[%s] Battery Level(%d)        : %d\n", m_p_log_id, n_idx, m_battery_t[n_idx].w_level );
    	print_log(DEBUG, 1, "[%s] SOC(%d)          : %2.1f(raw: %2.1f)\n", m_p_log_id, n_idx, m_battery_t[n_idx].w_soc/10.0f, m_battery_t[n_idx].w_soc_raw/10.0f );
    	print_log(DEBUG, 1, "[%s] VCell(%d)        : %2.2f\n", m_p_log_id, n_idx, m_battery_t[n_idx].w_vcell/100.0f );
    	print_log(DEBUG, 1, "[%s] Full SOC(%d)     : %d\n", m_p_log_id, n_idx, m_p_bat_full_soc[n_idx] );
	    print_log(DEBUG, 1, "[%s] Empty SOC(%d)    : %d\n", m_p_log_id, n_idx, m_p_bat_empty_soc[n_idx]);
	    print_log(DEBUG, 1, "[%s] Charge Enable(%d) : %d\n", m_p_log_id, n_idx, m_battery_t[n_idx].b_charge);
	    print_log(DEBUG, 1, "[%s] Charge State(%d) : %d\n", m_p_log_id, n_idx, m_battery_t[n_idx].c_charger_state);
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CHWMonitor::bat_rechg_print(void)
{
	print_log(DEBUG, 1, "[%s] battery charging mode:	%d\n", m_p_log_id, m_st_bat_chg_mode_info.chg_mode );
	print_log(DEBUG, 1, "[%s] battery recharging gauge: %d\n", m_p_log_id, m_st_bat_chg_mode_info.c_rechg_gauge );
	print_log(DEBUG, 1, "[%s] battery recharging block: %d\n", m_p_log_id, m_st_bat_chg_mode_info.c_block_recharging );
	print_log(DEBUG, 1, "[%s] blinking LED running:		%d\n", m_p_log_id, m_b_is_blinking_running);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CHWMonitor::restore_charging_state(void)
{
	u32	n_idx;
	ioctl_data_t 	iod_t;
	
	ioctl( m_n_fd, VW_MSG_IOCTL_GPIO_B_CHG_RESTORE, &iod_t );

	if (iod_t.data[0] == true) // if battery is full charged
	{
		print_log(ERROR, 1, "[%s] battery is full charged. it doesn't have to restore\n", m_p_log_id);
		return;
	}
	else // if battery is charged
	{
		for (n_idx = 0; n_idx < eBATTERY_MAX; n_idx++)
		{
			m_battery_t[n_idx].b_charge = false;
		}

		safe_delete(m_p_charging_start_timer_c);
		m_p_charging_start_timer_c = new CTime(1000);
	}
}		
	
/******************************************************************************/
/**
 * @brief
 * @param
 * @return
 * @note
 *******************************************************************************/
void CHWMonitor::set_charging_state_full(bool i_b_on)
{
	ioctl_data_t iod_t;

	memset(&iod_t, 0, sizeof(ioctl_data_t));
	iod_t.data[0] = i_b_on;

	ioctl(m_n_fd, VW_MSG_IOCTL_GPIO_B_CHG_FULL, &iod_t);
}
	
/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
DEV_ERROR CHWMonitor::initialize(MODEL i_model_e, u16 i_w_bat_count, BAT_CHG_MODE i_chg_mode, u8 c_rechg_gauge )
{
    m_n_adc_fd = open( "/dev/vivix_spi", O_RDWR|O_NDELAY );
	if( m_n_adc_fd < 0 )
	{
		print_log(ERROR,1,"[%s] /dev/vivix_spi device open error.\n", m_p_log_id);
		
		safe_delete( m_p_i2c_c );
		close(m_n_fd);
		return eDEV_ERR_HW_MON_DRIVER_OPEN_FAILED;
	}
	
	m_model_e = i_model_e;
	m_c_battery_num = i_w_bat_count;
	print_log(DEBUG,1,"[%s] Model type: %d\n", m_p_log_id, m_model_e);
	print_log(DEBUG,1,"[%s] Board version: %d\n", m_p_log_id, hw_get_ctrl_version());
	print_log(DEBUG,1,"[%s] Battery count: %d\n", m_p_log_id, m_c_battery_num);
	
	m_p_i2c_c = new CVWI2C(print_log);
	m_p_i2c_c->initialize(m_model_e);

	m_p_spi_c = new CVWSPI(m_n_fd, m_n_adc_fd, print_log);
	
	if (is_vw_revision_board())	m_p_impact_c = new CMicomImpact(m_p_spi_c, m_p_micom_c, print_log);
	else						m_p_impact_c = new CRiscImpact(m_n_fd, m_p_i2c_c, m_p_spi_c, print_log);

	m_p_impact_c->initialize();

	bat_set_remain_SOC_range(3, 98); 

	set_aed_count(m_model_e);

	memset(m_b_bat_equip_denoise_counter, 0x00, sizeof(u32) * eBATTERY_MAX);
    
    memset( &m_power_volt_t, 0, sizeof(power_volt_t) );
    get_power_volt( &m_power_volt_t, true );
	
	//BID CHECK + exceptional error case handling just for VW model 
	bat_init();
	
	//get phy model
	m_phy_type = phy_discovery();

	m_st_bat_chg_mode_info.chg_mode = i_chg_mode;
	m_st_bat_chg_mode_info.c_rechg_gauge = c_rechg_gauge;
	m_st_bat_chg_mode_info.c_block_recharging = false;
	m_b_is_blinking_running = false;

    return eDEV_ERR_NO;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CHWMonitor::battery_proc_start(void)
{
	battery_proc_stop();
	
    // launch battery thread
    m_b_is_running = true;
	if( pthread_create(&m_bat_thread_t, NULL, battery_routine, ( void * ) this)!= 0 )
    {
    	print_log( ERROR, 1, "[%s] battery_routine create failed:%s\n", m_p_log_id, strerror( errno ));

    	m_b_is_running = false;
    }
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CHWMonitor::battery_proc_stop(void)
{
    if( m_b_is_running )
    {
        m_b_is_running = false;
    	
    	if( pthread_join( m_bat_thread_t, NULL ) != 0 )
    	{
    		print_log( ERROR, 1,"[%s] pthread_join: battery_proc:%s\n", m_p_log_id, strerror( errno ));
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
void CHWMonitor::set_config( hw_config_t i_hw_config_t )
{
	// preset 변경 등에 의해 power control 방식이 수정된 경우, 
	// m_b_pwr_src_once_connected flag에 의해 power off처리되지 않도록 한번 false 로 초기화 해줌 
	if(i_hw_config_t.power_source_e != m_hw_config_t.power_source_e)
	{
		m_b_pwr_src_once_connected = false;
	}
	
	memcpy( &m_hw_config_t, &i_hw_config_t, sizeof(hw_config_t) );
    
    m_p_impact_c->impact_set_threshold(m_hw_config_t.c_impact_threshold);
    m_p_impact_c->set_impact_offset(&m_hw_config_t.impact_offset_t);
}

/******************************************************************************/
/**
 * @brief   디텍터 상태가 변하면 S/W에 메시지를 전달하는 함수 등록
 * @param   
 * @return  
 * @note    CVWSystem::set_notify_handler에서 호출한다.
*******************************************************************************/
void CHWMonitor::set_notify_sw_handler(bool (*noti)(vw_msg_t * msg))
{
    notify_sw_handler = noti;
}

/******************************************************************************/
/**
* \brief        모델에 따른 영상의 full size 크기를 out param에 입력
* \param        out : o_p_image_size_t
* \return       none
* \note
*******************************************************************************/
void CHWMonitor::get_image_size_info( image_size_t* o_p_image_size_t )
{
    if( m_model_e == eMODEL_2530VW )
    {
        o_p_image_size_t->width         = 2048;
        o_p_image_size_t->height        = 2560;
        o_p_image_size_t->pixel_depth   = 2;
    }
    else if( m_model_e == eMODEL_4343VW )
    {
        o_p_image_size_t->width         = 3072;
        o_p_image_size_t->height        = 3072;
        o_p_image_size_t->pixel_depth   = 2;
    }
    else if( m_model_e == eMODEL_3643VW )
    {
        o_p_image_size_t->width         = 2560;
        o_p_image_size_t->height        = 3072;
        o_p_image_size_t->pixel_depth   = 2;
    }
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CHWMonitor::message_send( MSG i_msg_e, u8* i_p_data, u16 i_w_data_len )
{
    if( i_w_data_len > VW_PARAM_MAX_LEN )
    {
        print_log(ERROR, 1, "[%s] message data lenth is too long(%d/%d).\n", 
                                m_p_log_id, i_w_data_len, VW_PARAM_MAX_LEN );
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
    
    b_result = notify_sw_handler( &msg );
    
    if( b_result == false )
    {
        print_log(ERROR, 1, "[%s] message_send(%d) failed\n", m_p_log_id, i_msg_e );
    }
    
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  hw version pin value
 * @note    
*******************************************************************************/
u8 CHWMonitor::hw_get_ctrl_version(void)
{
	ioctl_data_t 	iod_t;

	ioctl( m_n_fd, VW_MSG_IOCTL_GPIO_HW_VER ,&iod_t ); 
	
	return iod_t.data[0];
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
POWER_SOURCE CHWMonitor::get_power_source(void)
{
    return m_power_volt_t.power_source_e;
    //return ePWR_SRC_BATTERY;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CHWMonitor::get_temperature(void)
{
    return m_p_i2c_c->get_temperature();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CHWMonitor::adc_print(void)
{
    power_volt_t volt_t;
    
    memset( &volt_t, 0, sizeof(power_volt_t) );
    
    get_power_volt( &volt_t, true, true );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  POWER_VOLT_B_ID_A unit: 0.01V
 *			POWER_VOLT_B_ID_B unit: 0.01V

 * @note   	개선가능 포인트: 특정 channel 값만 획득할 수 있도록 변경
*******************************************************************************/
void CHWMonitor::get_power_volt(power_volt_t* o_p_pwr_volt_t, bool i_b_direct, bool i_b_print)
{
    if( i_b_direct == true )
    {
		u16 p_adc[POWER_VOLT_MAX] = {0,};
		power_volt_t volt_t;
		float raw_volt[POWER_VOLT_MAX], result[POWER_VOLT_MAX];

		if (is_vw_revision_board())	get_adc_from_micom(p_adc, &volt_t, raw_volt, result);
        else						get_adc_from_risc(p_adc, &volt_t, raw_volt, result);

    	// VBATT - Tether : 24V, USB : 18V
    	// 15V 이상이면 tether 혹은 USB
		/**
		 * [20240306] [giwook] 
		 * USB를 power source로 인식하기 위해서는 
		 * USB 인식조건이 반드시 'result[POWER_VOLT_VBATT] > 14.0f'여야만 함
		 * 그렇지 않으면 기존 보드와의 호환성이 무너짐
		*/

   		if(gpio_get_tether_equip())
		{
			volt_t.power_source_e = ePWR_SRC_TETHER;
		}
		else if( result[POWER_VOLT_VBATT] > 14.0f )
		{
			volt_t.power_source_e = ePWR_SRC_USB;
		}
    	else if( result[POWER_VOLT_B_ID_A] > 3.3f * 0.70f
    			&& result[POWER_VOLT_B_ID_B] > 3.3f * 0.70f )
		{
			volt_t.power_source_e = ePWR_SRC_UNKNOWN;
		}
		else
		{
			volt_t.power_source_e = ePWR_SRC_BATTERY;
		}
    	
    	memcpy( o_p_pwr_volt_t, &volt_t, sizeof(power_volt_t) );
    	
    	if( i_b_print == true )
    	{
    	    print_log(DEBUG, 1, "[%s] =============== ADC ===============\n", m_p_log_id);
        	print_log(DEBUG, 1, "[%s] B_ID_A		:  %2.2fV(raw: %2.2fV, %d)\n", m_p_log_id, result[0], raw_volt[0], p_adc[0] );
        	print_log(DEBUG, 1, "[%s] B_ID_B		:  %2.2fV(raw: %2.2fV, %d)\n", m_p_log_id, result[1], raw_volt[1], p_adc[1] );
        	print_log(DEBUG, 1, "[%s] B_THM_A	:  %2.2fV(raw: %2.2fV, %d)\n", m_p_log_id, result[2], raw_volt[2], p_adc[2] );
        	print_log(DEBUG, 1, "[%s] B_THM_B	:  %2.2fV(raw: %2.2fV, %d)\n", m_p_log_id, result[3], raw_volt[3], p_adc[3] );
        	print_log(DEBUG, 1, "[%s] AED_TRIG_A	:  %2.2fV(raw: %2.2fV, %d)\n", m_p_log_id, result[4], raw_volt[4], p_adc[4] );
        	print_log(DEBUG, 1, "[%s] AED_TRIG_B	:  %2.2fV(raw: %2.2fV, %d)\n", m_p_log_id, result[5], raw_volt[5], p_adc[5] );
        	print_log(DEBUG, 1, "[%s] VBATT		:  %2.2fV(raw: %2.2fV, %d)\n", m_p_log_id, result[6], raw_volt[6], p_adc[6] );
        	print_log(DEBUG, 1, "[%s] VS		:  %2.2fV(raw: %2.2fV, %d)\n", m_p_log_id, result[7], raw_volt[7], p_adc[7] );
        	print_log(DEBUG, 1, "[%s] Tether equip	:  %s\n", m_p_log_id, (gpio_get_tether_equip())?"equipped":"Not eqquiped" );
        	print_log(DEBUG, 1, "[%s] Cur PW SRC	:  %d (0:BAT, 1:Tether, 2:USB, 255:Unknown)\n", m_p_log_id, volt_t.power_source_e );	
        }
    }
    else
    {
        memcpy( o_p_pwr_volt_t, &m_power_volt_t, sizeof(power_volt_t) );
    }
    
    if(get_battery_count() == 1 && get_bat_equip_start_idx() != 0 )
    {
    	o_p_pwr_volt_t->volt[POWER_VOLT_B_ID_A] = o_p_pwr_volt_t->volt[POWER_VOLT_B_ID_B];
    	o_p_pwr_volt_t->volt[POWER_VOLT_B_THM_A] = o_p_pwr_volt_t->volt[POWER_VOLT_B_THM_B];
    }
}

/******************************************************************************/
/**
 * @brief   battery가 장착되었는지 확인한다.
 * @param  	u8 i_c_sel : 배터리 cell 번호
 * @return  true : 장착된 상태
 			false : 장착되어 있지 않은 상태
 * @note    B_ID 값과 VCELL 값을 확인하여 battery 착탈 여부를 인식한다. 
*******************************************************************************/
bool CHWMonitor::bat_is_equipped(u8 i_c_sel)
{
	power_volt_t        volt_t;
	bool				b_equip = false;
	u16					w_vcell = 0;
	
	memset(&volt_t, 0, sizeof(power_volt_t));

	get_power_volt(&volt_t, true, false);
	w_vcell = m_p_i2c_c->bat_get_volt(i_c_sel);

	/*VW 전용 B_ID debounce logic */
	//현재값: 장착 
	if( volt_t.volt[POWER_VOLT_B_ID_A + i_c_sel] <= (3.3f * 30.0f) )
	{
		m_b_bat_equip_denoise_counter[i_c_sel] = 0;
		m_battery_t[i_c_sel].b_bid_equip = true;

	}
	else	//현재값: 탈착
	{
		m_b_bat_equip_denoise_counter[i_c_sel]++;

		if( m_b_bat_equip_denoise_counter[i_c_sel] >= 5 )	//기존값 탈착 || Exceed Denoise threshold
		{
			m_battery_t[i_c_sel].b_bid_equip = false;
		}
	}


	switch(i_c_sel)
	{
		case 0:
			// BID 가 3.3V 보다 작거나, VCELL 1V 이상일 경우 장착으로 인식
			if( m_battery_t[i_c_sel].b_bid_equip || (w_vcell >= 100) )
				b_equip = true;
			break;

		case 1:
			switch(m_model_e)
			{
				case eMODEL_2530VW:
					//무조건 false (사양) 
					b_equip = false;
					break;
				
				case eMODEL_3643VW:
				case eMODEL_4343VW:
					// BID 가 3.3V 보다 작거나, VCELL 1V 이상일 경우 장착으로 인식
					if( m_battery_t[i_c_sel].b_bid_equip || (w_vcell >= 100) )
						b_equip = true;
					break;

				default:
					print_log(DEBUG, 1, "[%s] Invalid model info : %d\n", m_p_log_id, m_model_e);
					break;
			}
			break;

		default: 
			print_log(DEBUG, 1, "[%s] Invalid battery cell number : %d\n", m_p_log_id, i_c_sel);
			break;
	}

   	//print_log(DEBUG, 1, "[%s] Battery i_c_sel: %d w_vcell: %d volt[POWER_VOLT_B_ID_A]: %d volt[POWER_VOLT_B_ID_B]: %d \n", m_p_log_id, i_c_sel, w_vcell, volt_t.volt[POWER_VOLT_B_ID_A], volt_t.volt[POWER_VOLT_B_ID_B] );
    
    return b_equip;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CHWMonitor::bat_init(void)
{
	u32 n_idx;
	ioctl_data_t 	iod_t;
	u16 vcell = 0;
	u16 raw_soc = 0;
	
	iod_t.data[0] = eBAT_CHARGE_0_OFF_1_OFF;
	ioctl( m_n_fd, VW_MSG_IOCTL_GPIO_B_CHG_EN, &iod_t );
	sleep_ex(10000);      // 10 ms
	
	for( n_idx = 0; n_idx < eBATTERY_MAX; n_idx++)
	{
		m_battery_t[n_idx].c_charger_state = eCHARGER_DISABLED;
		if( bat_is_equipped(n_idx) == true )
   		{
	        m_battery_t[n_idx].b_equip = true;
			
			//20211214. youngjun han. VW 모델의 간헐적 배터리 탈착 이슈에 대한 예외 방어 코드 
			//HW quick start 시점에서 배터리 탈착 발생 시, soc 값 비정상적 출력될 수 있음. 해당 경우만 확인하여 sw quickstart 실행될 수 있도록 하기 코드 추가
			vcell = m_p_i2c_c->bat_get_volt(n_idx);
			raw_soc = m_p_i2c_c->bat_get_soc(n_idx);

			if( bat_is_valid_soc_raw(vcell, raw_soc) == false )
			{
				//run sw quick start
				m_p_i2c_c->bat_quick_start(n_idx);
			}
    	}
    	else
    	{
    		m_battery_t[n_idx].b_equip = false;    		
    	}

    	print_log(DEBUG, 1, "[%s] battery(%d) equip: %d vcell: %d soc_raw: %d rcomp: %d\n", m_p_log_id, n_idx, m_battery_t[n_idx].b_equip, \
																								vcell, raw_soc, m_p_i2c_c->bat_get_rcomp(n_idx) );
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CHWMonitor::bat_check_initial_vcell(void)
{
	u16 w_soc;
	u16 w_vbat;
	u32 n_idx;
	
	POWER_SOURCE    power_source_e; 
	 
	power_source_e = m_power_volt_t.power_source_e;
	
	for( n_idx = 0; n_idx < eBATTERY_MAX; n_idx++)
	{
		if( m_battery_t[n_idx].b_equip == true )
		{
			w_soc	= m_p_i2c_c->bat_get_soc(n_idx);
			w_vbat	= m_p_i2c_c->bat_get_volt(n_idx);

			print_log(DEBUG, 1, "[%s] battery(%d) init vcell and soc (soc: %2.2f, vbatt: %2.2f, full: %d, empty: %d, pwr: %d)\n", \
							m_p_log_id, n_idx, w_soc/10.0f, w_vbat/100.0f, m_p_bat_full_soc[n_idx], m_p_bat_empty_soc[n_idx], power_source_e );
		}
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   i_w_vcell: (unit: 10ms)
 * @return  
 * @note    
*******************************************************************************/
bool CHWMonitor::bat_is_valid_vcell( u16 i_w_vcell )
{
	if( i_w_vcell <= 900 )
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
 * @note    충전이 한 thread에서만 되게 한다.
*******************************************************************************/
void CHWMonitor::bat_charge( BAT_CHARGE i_e_charge_state )
{
	ioctl_data_t 	iod_t;
	BAT_CHARGE		charge_state_e = i_e_charge_state;
	//u32				n_idx;
	
	if( m_battery_t[eBATTERY_0].b_equip == false && m_battery_t[eBATTERY_1].b_equip == false\
		&& charge_state_e != eBAT_CHARGE_0_OFF_1_OFF)
	{
	    print_log(DEBUG, 1, "[%s] battery charge failed(no battery)\n", m_p_log_id);
	    return;
	}
	
	if(m_p_charging_start_timer_c != NULL)
	{
		if(m_p_charging_start_timer_c->is_expired() == false)
		{
			print_log(DEBUG, 1, "[%s] battery charge ignored. 1sec waiting\n", m_p_log_id);
			return;
		}
		else
		{
			safe_delete(m_p_charging_start_timer_c);
		}
	}
	
	// 배터리 충전이 모두 diable 된 상태에서 1개 이상의 배터리 충전을 enable 하고자 하는 경우
	// 특히 영상 촬영을 위해 배터리 충전을 disable 했다가 배터리 충전을 재개하는 경우
	// 충전 전류 inrush에 의해 PHY가 down될 수 있음. 따라서 1초 후에 충전 재개 하도록 수정
	if(m_battery_t[eBATTERY_0].b_charge == false && m_battery_t[eBATTERY_1].b_charge == false)
	{
 		if( charge_state_e == eBAT_CHARGE_0_ON_1_ON )
 		{
 			charge_state_e = eBAT_CHARGE_0_ON_1_OFF;
 		}
	}

	if( charge_state_e != eBAT_CHARGE_0_OFF_1_OFF
	    && power_source_can_charge(m_power_volt_t.power_source_e) == false )
	{
	    print_log(DEBUG, 1, "[%s] battery charge failed.(There is no power source that can charge batteries)\n", m_p_log_id);
	    return;
	}
	
	iod_t.data[0] = charge_state_e;
	
    ioctl( m_n_fd, VW_MSG_IOCTL_GPIO_B_CHG_EN, &iod_t ); 
    if(iod_t.data[0] != 0)
 	{
	    print_log(DEBUG, 1, "[%s] Battery charge request is ignored during image acquisition.\n", m_p_log_id);
	    return;
	}
	   
    if( charge_state_e != eBAT_CHARGE_0_OFF_1_OFF )
    {
        sleep_ex(100000);      // 100 ms
    }
	
	if(eBAT_CHARGE_0_ON_1_OFF == charge_state_e)
	{
		m_battery_t[eBATTERY_0].b_charge = true;
		m_battery_t[eBATTERY_1].b_charge = false;
	}
	else if(eBAT_CHARGE_0_OFF_1_ON == charge_state_e)
	{
		m_battery_t[eBATTERY_0].b_charge = false;
		m_battery_t[eBATTERY_1].b_charge = true;
	}
	else if(eBAT_CHARGE_0_ON_1_ON == charge_state_e)
	{
		m_battery_t[eBATTERY_0].b_charge = true;
		m_battery_t[eBATTERY_1].b_charge = true;
	}
	else
	{
		m_battery_t[eBATTERY_0].b_charge = false;
		m_battery_t[eBATTERY_1].b_charge = false;
	}
	
	print_log(DEBUG, 1, "[%s] battery charge_%d %s\n", m_p_log_id, eBATTERY_0, m_battery_t[eBATTERY_0].b_charge?"start":"stop");
	print_log(DEBUG, 1, "[%s] battery charge_%d %s\n", m_p_log_id, eBATTERY_1, m_battery_t[eBATTERY_1].b_charge?"start":"stop");
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CHWMonitor::bat_check_charger_state(void)
{
    u32				n_idx;
    ioctl_data_t 	iod_t;
    int				n_ret = 0;
	
	if( m_battery_t[eBATTERY_0].b_charge == false && m_battery_t[eBATTERY_1].b_charge == false )
    {
        return;
    }
     
    for( n_idx = 0; n_idx < eBATTERY_MAX; n_idx++)
    {
    	if( m_battery_t[n_idx].b_equip == true )
    	{
    		if( m_battery_t[n_idx].b_charge == true )
    		{
    			memset(&iod_t, 0, sizeof(ioctl_data_t));
    			iod_t.data[0] = n_idx;
				n_ret = ioctl( m_n_fd, VW_MSG_IOCTL_GPIO_B_CHARGING_STAT ,&iod_t );
				if( n_ret < 0 )
				{
					print_log(ERROR, 1, "[%s] charger state ioctl failed.\n", m_p_log_id);
				}
				else
				{
					m_battery_t[n_idx].c_charger_state = iod_t.data[0];
				}
    		}
    	}
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    VW의 경우, b_equip 이 아닌 bid를 가지고 충전 여부를 판단.
 * 			사유: 간헐적 단기간 bid 떨어짐 이슈 해결을 위해 b_equip 신호를 bid와 vcell을 연동하여 판단함.
 * 				  이 때, 장기간 bid 떨어짐 이슈에서 Error 05를 정상적으로 발생시키기 위해서는 충전이 종료되어야하며,
 * 				  이를 위해 bid가 끊어질 경우, 충전을 중지함. 
 * 					단기간 bid 떨어짐 이슈일 경우: bid 복구되어 충전 재개될 것이며
 * 					장기간 bid 떨어짐 이슈일 경우: bid 복구되지 않아 충전 종료 및 wcell로 drop 되어 equip 값 false 처리 및 error 05 발생
*******************************************************************************/
void CHWMonitor::bat_check_charge_change(void)
{
	PM_MODE 	charge_state_e = ePM_MODE_NORMAL;

/*
    if( m_battery_t[eBATTERY_0].b_equip == true && m_battery_t[eBATTERY_1].b_equip == true)
    {      	
	   	charge_state_e = ePM_MODE_BATTERY_0_ON_1_ON;
    }
    else if(m_battery_t[eBATTERY_0].b_equip == false && m_battery_t[eBATTERY_1].b_equip == true)
    {
   		charge_state_e = ePM_MODE_BATTERY_0_OFF_1_ON;
    }
    else if(m_battery_t[eBATTERY_0].b_equip == true && m_battery_t[eBATTERY_1].b_equip == false)
    {
		charge_state_e = ePM_MODE_BATTERY_0_ON_1_OFF;
    }
    else
    {
    	charge_state_e = ePM_MODE_BATTERY_0_OFF_1_OFF;
    }    
*/

    if( ( m_power_volt_t.volt[POWER_VOLT_B_ID_A] <= (3.3f * 30.0f) ) && ( m_power_volt_t.volt[POWER_VOLT_B_ID_B] <= (3.3f * 30.0f) ) )
    {      	
	   	charge_state_e = ePM_MODE_BATTERY_0_ON_1_ON;
    }
    else if( ( m_power_volt_t.volt[POWER_VOLT_B_ID_A] > (3.3f * 30.0f) ) && ( m_power_volt_t.volt[POWER_VOLT_B_ID_B] <= (3.3f * 30.0f) ) )
    {
   		charge_state_e = ePM_MODE_BATTERY_0_OFF_1_ON;
    }
    else if( ( m_power_volt_t.volt[POWER_VOLT_B_ID_A] <= (3.3f * 30.0f) ) && ( m_power_volt_t.volt[POWER_VOLT_B_ID_B] > (3.3f * 30.0f) ) )
    {
		charge_state_e = ePM_MODE_BATTERY_0_ON_1_OFF;
    }
    else
    {
    	charge_state_e = ePM_MODE_BATTERY_0_OFF_1_OFF;
    }    

    if( charge_state_e >= ePM_MODE_BATTERY_0_ON_1_OFF && charge_state_e <= ePM_MODE_BATTERY_0_OFF_1_OFF  )
    {
    	if(!(( m_battery_t[eBATTERY_0].b_charge == false && m_battery_t[eBATTERY_1].b_charge == false\
    		&& charge_state_e == ePM_MODE_BATTERY_0_OFF_1_OFF)\
    		|| (m_battery_t[eBATTERY_0].b_charge == false && m_battery_t[eBATTERY_1].b_charge == true\
    		&& charge_state_e == ePM_MODE_BATTERY_0_OFF_1_ON)\
    		||(m_battery_t[eBATTERY_0].b_charge == true && m_battery_t[eBATTERY_1].b_charge == false\
    		&& charge_state_e == ePM_MODE_BATTERY_0_ON_1_OFF)\
    		||(m_battery_t[eBATTERY_0].b_charge == true && m_battery_t[eBATTERY_1].b_charge == true\
    		&& charge_state_e == ePM_MODE_BATTERY_0_ON_1_ON)))
    	{
    		// message로 처리하는 이유: 충전이 한 thread에서만 되게 한다.
    		message_send(eMSG_POWER_MODE, (u8*)&charge_state_e, sizeof(charge_state_e));
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
u8 CHWMonitor::bat_get_equip_num(void)
{
    s8  c_count = 0;
    u32 n_idx;
    
    for( n_idx = 0; n_idx < eBATTERY_MAX; n_idx++ )
   	{
   		if( m_battery_t[n_idx].b_equip == true)
   		{
   			c_count++;
   		}
   	}
   	
   	return c_count;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CHWMonitor::bat_is_soc_full(void)
{
    u32 n_idx;
    u16 w_soc_total = 0;
    u8	c_bat_num;
    
	//bat_sw_quick_start(0);
	//bat_sw_quick_start(1);
	
    for( n_idx = 0; n_idx < eBATTERY_MAX; n_idx++ )
    {
    	if( m_battery_t[n_idx].b_equip == true )
        {
        	w_soc_total += m_battery_t[n_idx].w_soc;
    		//print_log(DEBUG, 1, "[%s] [###########] w_soc idx(%d) : %d\n", m_p_log_id, n_idx, m_battery_t[n_idx].w_soc);
        }
    }
    
    if( m_model_e == eMODEL_4343VW
    	|| m_model_e == eMODEL_3643VW )
    {
    	c_bat_num = bat_get_equip_num();
    	if( c_bat_num > 0 )
    	{
    		w_soc_total = w_soc_total/c_bat_num;
    	}
    }
    //print_log(DEBUG, 1, "[%s] [###########] w_soc_total: %d\n", m_p_log_id, w_soc_total);

    if( w_soc_total >= 1000 )
    {
    	//print_log(DEBUG, 1, "[%s] [###########] full charged!%d\n", m_p_log_id, w_soc_total);
        return true;
    }
   	else
	{
   		return false;
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CHWMonitor::bat_is_soc_empty(void)
{
    u32 n_idx;
    const s32	k_soc_empty   = 10;
    u16 w_soc_total = 0;
    u8	c_bat_num;
    
    for( n_idx = 0; n_idx < eBATTERY_MAX; n_idx++ )
    {
    	if( m_battery_t[n_idx].b_equip == true )
        {
        	 w_soc_total += m_battery_t[n_idx].w_soc_raw;
        }
    }
    
    if( m_model_e == eMODEL_4343VW
    	|| m_model_e == eMODEL_3643VW )
    {
    	c_bat_num = bat_get_equip_num();
    	if( c_bat_num > 0 )
    	{
    		w_soc_total = w_soc_total/c_bat_num;
    	}
    }
    
    if( w_soc_total < k_soc_empty )
    {
    	print_log(DEBUG, 1, "[%s] empty battery: %f\n", m_p_log_id, w_soc_total/10.0f);
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
bool CHWMonitor::bat_is_power_off(void)
{
    u32 		n_idx;
    const s32	k_off_vcell = 960;
    u32			n_equip_cnt = 0;
    u32	 		n_bat_off_cnt = 0;
    
    PM_MODE     pm_mode_e;
    
    if( power_source_can_charge(m_power_volt_t.power_source_e) == true )
    {
    	// 배터리 충전 가능한 전원이 꽂힌 경우, low battery로 power off되지 않도록 한다.
    	return false;
    }
    
	if( m_b_battery_power_off == true )
    {
    	if( m_p_battery_off_time_c == NULL )
    	{
    		m_p_battery_off_time_c = new CTime(5000);
    	}
    	else if( m_p_battery_off_time_c->is_expired() == true )
    	{
    		safe_delete( m_p_battery_off_time_c );
    		m_b_battery_power_off = false;
    	}
        else
        {
        	return true;
        }
    }

    n_equip_cnt = bat_get_equip_num();
    
    if( bat_is_soc_empty() == true )
    {
        m_b_battery_power_off = true;
        bat_print();

      	pm_mode_e = ePM_MODE_SHUTDOWN_BY_LOW_BATTERY;
        message_send(eMSG_POWER_MODE, (u8*)&pm_mode_e, sizeof(pm_mode_e));
        
        return true;
    }

    for( n_idx = 0; n_idx < eBATTERY_MAX; n_idx++ )
    {
    	if( m_battery_t[n_idx].b_equip == true
    	    && m_battery_t[n_idx].w_vcell < k_off_vcell )
        {
        	 m_battery_t[n_idx].b_power_off = true;
        	 n_bat_off_cnt++;
        }
    }
    
    if( n_bat_off_cnt == n_equip_cnt )
    {
        if( m_p_battery_off_time_c == NULL )
        {
            m_p_battery_off_time_c = new CTime(5000);
            return false;
        }
        else if( m_p_battery_off_time_c->is_expired() == true )
        {
            m_b_battery_power_off = true;
            bat_print();
        	
        	pm_mode_e = ePM_MODE_SHUTDOWN_BY_LOW_BATTERY;
            message_send(eMSG_POWER_MODE, (u8*)&pm_mode_e, sizeof(pm_mode_e));
            
            safe_delete( m_p_battery_off_time_c );
            
            return true;
        }
    }
    else
    {
        safe_delete( m_p_battery_off_time_c );
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
void CHWMonitor::bat_check_low_volt(void)
{
    u32 		n_idx;
    const s32	k_warn_vcell    = 1020;
    const s32	k_warn_soc      = 20;	//평균 2퍼
    u32			n_equip_cnt = 0;
    u32	 		n_bat_warn_cnt = 0;
    u32         n_bat_normal_cnt = 0;
    u16         w_soc_total = 0;

    if( bat_is_power_off() == true )
    {
        return;
    }
    
    n_equip_cnt = bat_get_equip_num();
   
    for( n_idx = 0; n_idx < eBATTERY_MAX; n_idx++)
    {
    	if( m_battery_t[n_idx].b_equip == true )
    	{
    	    w_soc_total += m_battery_t[n_idx].w_soc;
    	    if( m_battery_t[n_idx].w_vcell < k_warn_vcell )
    	    {
    	        if( m_battery_t[n_idx].b_power_warning == false )
        		{
        		    n_bat_normal_cnt++;
        		}
        		n_bat_warn_cnt++;
        	}
        	else
        	{
        	    m_battery_t[n_idx].b_power_warning = false;
        	}
    	}
    }
    
	//2022.7.11 장착된 배터리의 soc 값 평균 2% 이하면 power warning true . 즉시 warn true 조건 
    if ( w_soc_total <= k_warn_soc * n_equip_cnt )
    {
        for( n_idx = 0; n_idx < eBATTERY_MAX; n_idx++)
        {
        	if( m_battery_t[n_idx].b_equip == true )
        	{
        	    m_battery_t[n_idx].b_power_warning = true;
        	}
        }
        return;
    }
    
    if( n_bat_warn_cnt == n_equip_cnt )
    {
        if( n_bat_warn_cnt == n_bat_normal_cnt )
        {
            if( m_p_battery_low_time_c == NULL )
            {
                m_p_battery_low_time_c = new CTime(5000);
            }
            else if( m_p_battery_low_time_c->is_expired() == true )
            {
                safe_delete(m_p_battery_low_time_c);
                
                for( n_idx = 0; n_idx < eBATTERY_MAX; n_idx++)
                {
                	if( m_battery_t[n_idx].b_equip == true )
                	{
                	    if( m_battery_t[n_idx].w_vcell < k_warn_vcell
                	        || m_battery_t[n_idx].w_soc <= k_warn_soc )
                    	{
                    		 m_battery_t[n_idx].b_power_warning = true;
                    	}
                    }
                }
            }
        }
        else
        {
            safe_delete(m_p_battery_low_time_c);
        }
    }
    else
    {
        safe_delete(m_p_battery_low_time_c);
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CHWMonitor::bat_notify_low_volt(void)
{
    PM_MODE     pm_mode_e;
    
    pm_mode_e = ePM_MODE_LOW_BATTERY;
    message_send(eMSG_POWER_MODE, (u8*)&pm_mode_e, sizeof(pm_mode_e));
}

/******************************************************************************/
/**
 * @brief	set empty and full SOC for REMAIN range
 * @param
 * @return
 * @note
 *******************************************************************************/
void CHWMonitor::bat_set_remain_SOC_range(s32 i_n_empty_soc, s32 i_n_full_soc)
{
	m_p_bat_full_soc[eBATTERY_0] = i_n_full_soc;
	m_p_bat_full_soc[eBATTERY_1] = i_n_full_soc;
	m_p_bat_empty_soc[eBATTERY_0] = i_n_empty_soc; //수정: 요구사항 empty SOC는 0으로 하는 것
	m_p_bat_empty_soc[eBATTERY_1] = i_n_empty_soc; //수정: 요구사항 empty SOC는 0으로 하는 것
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CHWMonitor::ap_button_get_raw(void)
{
	ioctl_data_t 	iod_t;
	
	ioctl( m_n_fd, VW_MSG_IOCTL_GPIO_AP_BUTTON ,&iod_t );
	
	if( iod_t.data[0] == 0 )
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
bool CHWMonitor::pwr_button_get_raw(void)
{
	bool ret = false;
	if (is_vw_revision_board())
	{
		ret = m_p_micom_c->is_power_button_on();
	}
	else
	{
		ioctl_data_t 	iod_t;
		ioctl( m_n_fd, VW_MSG_IOCTL_GPIO_PWR_BUTTON ,&iod_t );
	
		if( iod_t.data[0] == 0 )	ret = true;
		else						ret = false;
	}

	return ret;
}

/******************************************************************************/
/**
 * @brief   
 * @param   i_b_en: display(on/off)
 * @return  
 * @note    
*******************************************************************************/
void CHWMonitor::set_uart_display(bool i_b_en)
{
    ioctl_data_t 	iod_t;
    
    if( i_b_en == true )
	{
	    iod_t.data[0] = 1;
	}
	else
	{
	    iod_t.data[0] = 0;
	}
    
    ioctl( m_n_fd, VW_MSG_IOCTL_GPIO_UART1_TX_EN, &iod_t );
    
    sys_command("fw_setenv uart_display %s", (i_b_en == true) ? "1":"0");
    sys_command("sync");
    
    print_log(DEBUG, 1, "[%s] uart display %s\n", \
                        m_p_log_id, (i_b_en == true) ? "on":"off" );
}


/********************************************************************************
 * \brief		phy reset
 * \param
 * \return
 * \note
 ********************************************************************************/
static void phy_eth0_down()
{
	sys_command("ifconfig eth0 down");
}

/********************************************************************************
 * \brief		phy reset
 * \param
 * \return
 * \note
 ********************************************************************************/
static void phy_eth0_up()
{
	sys_command("ifconfig eth0 up");
}

/********************************************************************************
 * \brief		phy reset
 * \param
 * \return
 * \note
 ********************************************************************************/
void CHWMonitor::phy_reset(void) 
{

	print_log(DEBUG, 1, "[%s] phy reset\n", m_p_log_id );

	phy_down();

	sleep_ex(100000);	 // 100 ms

	phy_up();
}

/********************************************************************************
 * \brief		phy reset
 * \param
 * \return
 * \note
 ********************************************************************************/
void CHWMonitor::phy_down(void) 
{
	ioctl_data_t 	iod_t;
	phy_eth0_down();
	
	iod_t.data[0] = 0;
	ioctl( m_n_fd, VW_MSG_IOCTL_GPIO_PHY_RESET, &iod_t );
	
	m_b_phy_on = false;
	
	print_log(DEBUG, 1, "[%s] phy power down\n", m_p_log_id );
}

/********************************************************************************
 * \brief		phy reset
 * \param
 * \return
 * \note
 ********************************************************************************/
void CHWMonitor::phy_up(void) 
{
	ioctl_data_t 	iod_t;
	 
	iod_t.data[0] = 1;    
	ioctl( m_n_fd, VW_MSG_IOCTL_GPIO_PHY_RESET, &iod_t );
	sleep_ex(10);	 // 10 ms
	phy_init();
	phy_eth0_up();
	m_b_phy_on = true;
	
	print_log(DEBUG, 1, "[%s] phy power up\n", m_p_log_id );
}

/********************************************************************************
 * \brief		phy discovery
 * \param
 * \return
 * \note		MIDO통신하여 OUI 및 model 값을 확인하여  processor MAC에 연결된 PHY 모델을 구분
 *              CHIP 			: PHY_ID_1 : PHY_ID_2
 * 				Marvell 88E1510 : 0x0141   : 0xDD0[x]
 *              ADIN 1300 		: 0x0283   : 0xBC3[X]  ([X]는 don't care)
 ********************************************************************************/
PHY_TYPE CHWMonitor::phy_discovery(void) 
{
	PHY_TYPE ret;
	u16 reg_val = 0x0000;
	u32 phy_oui = 0x0000;
	u16 phy_model_num = 0x000;
	
	//read reg 2, 3	 & parse OUI, Model number 
	vw_net_phy_read( "eth0", 0x02, &reg_val );
	phy_oui = reg_val;
	phy_oui = (phy_oui << 6);
	vw_net_phy_read( "eth0", 0x03, &reg_val );
	phy_oui = (phy_oui | (reg_val >> 10));
	phy_model_num = ( (reg_val >> 4) & (0x3F) );

	//analyze OUI, model number
	switch (phy_oui)
	{
		case MARVELL_PHY_OUI:
			switch(phy_model_num)
			{
				case MARVELL_PHY_88E1510_MODEL_NUM:
					print_log(DEBUG, 1, "[%s] phy discovery success. Type: %d\n", m_p_log_id, ePHY_MARVELL_88E1510 );
					ret = ePHY_MARVELL_88E1510;
					break;
				default:
					ret = ePHY_MARVELL_88E1510;
					print_log(DEBUG, 1, "[%s] phy discovery fail. Set Default Type: %d\n", m_p_log_id, ePHY_MARVELL_88E1510);
					break;
			}
			break;

		case ADI_PHY_OUI:
			switch(phy_model_num)
			{
				case ADI_PHY_1300_MODEL_NUM:
					print_log(DEBUG, 1, "[%s] phy discovery success. Type: %d\n", m_p_log_id, ePHY_ADI_ADIN1300 );
					ret = ePHY_ADI_ADIN1300;
					break;
				default:
					ret = ePHY_ADI_ADIN1300;
					print_log(DEBUG, 1, "[%s] phy discovery fail. Set Default Type: %d\n", m_p_log_id, ePHY_ADI_ADIN1300);
					break;				
			}
			break;

		default:
			ret = ePHY_ADI_ADIN1300;
			print_log(DEBUG, 1, "[%s] phy discovery fail. Set Default Type: %d\n", m_p_log_id, ePHY_ADI_ADIN1300);
			break;

	}

	//DEBUG
	//print_log(DEBUG, 1, "[%s] PHY OUI: 0x%04X MODEL NUM: 0x%04X\n", m_p_log_id, phy_oui, phy_model_num);
	
	return ret;
}

/********************************************************************************
 * \brief		phy init
 * \param
 * \return
 * \note		PHY 칩 모델을 구분하여 동작
 *              Marvell 88E1510
 *              ADIN 1300
 ********************************************************************************/
void CHWMonitor::phy_init(void) 
{
	print_log(DEBUG, 1, "[%s] phy init\n", m_p_log_id );

	switch(m_phy_type)
	{
		case ePHY_MARVELL_88E1510:
			//do nothing
			break;
		case ePHY_ADI_ADIN1300:
			//ADIN 1300. 125Mhz ref clk gen. GP_CLK.
			vw_net_phy_write( "eth0", 0x10, 0xff1f );
			vw_net_phy_write( "eth0", 0x11, 0x0020 );
			break;
		default:
			print_log(DEBUG, 1, "[%s] Unknown phy type: %d\n", m_p_log_id, m_phy_type);
	}
	
}

/********************************************************************************
 * \brief		VDD core control
 * \param
 * \return
 * \note
 ********************************************************************************/
void CHWMonitor::vdd_core_enable( bool i_b_on )
{
    ioctl_data_t 	iod_t;
    
    if( i_b_on == false )
    {
        iod_t.data[0] = 1;
    }
    else
    {
        iod_t.data[0] = 0;
    }
    
	print_log(DEBUG, 1, "[%s] VDD core enable: %d\n", m_p_log_id, i_b_on);
    
	ioctl( m_n_fd, VW_MSG_IOCTL_GPIO_VDD_CORE_ENABLE, &iod_t );
}

/********************************************************************************
 * \brief		
 * \param
 * \return
 * \note
 ********************************************************************************/
bool CHWMonitor::gpio_get_tether_equip(void)
{
	ioctl_data_t 	iod_t;
	
	ioctl( m_n_fd, VW_MSG_IOCTL_GPIO_TETHER_EQUIP, &iod_t );
	
	if(iod_t.data[0] == 0)
		return true;
	else
		return false;
}

/********************************************************************************
 * \brief		
 * \param
 * \return
 * \note
 ********************************************************************************/
bool CHWMonitor::gpio_get_adaptor_equip(void)
{
	ioctl_data_t 	iod_t;
	
	ioctl( m_n_fd, VW_MSG_IOCTL_GPIO_ADAPTOR_EQUIP, &iod_t );
	
	if(iod_t.data[0] == 0)
		return true;
	else
		return false;
}

/********************************************************************************
 * \brief		
 * \param
 * \return
 * \note
 ********************************************************************************/
void CHWMonitor::boot_led_on(void)
{
	ioctl_data_t 	iod_t;
	ioctl( m_n_fd, VW_MSG_IOCTL_GPIO_BOOT_END, &iod_t );
}

/********************************************************************************
 * \brief		
 * \param
 * \return
 * \note
 ********************************************************************************/
void CHWMonitor::micom_boot_led_on(void)
{
	m_p_micom_c->power_led_ready();
}

/********************************************************************************
 * \brief		
 * \param
 * \return
 * \note
 ********************************************************************************/
void CHWMonitor::power_led_ctrl(u8 i_c_enable)
{
	if (is_vw_revision_board())
	{
		controlLedBlinkingWithMicom(i_c_enable);
	}
	else
	{
		controlLedBlinkingWithGpio(i_c_enable);
	}
}

void CHWMonitor::controlLedBlinkingWithMicom(u8 enable)
{
	m_c_led_blink_enable = enable;
	m_p_micom_c->ControlLedBlinking(enable);
}

void CHWMonitor::controlLedBlinkingWithGpio(u8 enable)
{
	ioctl_data_t 	iod_t;
	memset(&iod_t, 0, sizeof(ioctl_data_t));

	m_c_led_blink_enable = enable;
	
	// Power LED blinking mode on
	if( enable == 1 )
    {
        iod_t.data[0] = 0;
    }
    else	// Power LED on 유지
    {
        iod_t.data[0] = 1;
    }
	
	ioctl( m_n_fd, VW_MSG_IOCTL_GPIO_PWR_LED_CTRL, &iod_t );
}

/********************************************************************************
 * \brief		
 * \param
 * \return
 * \note
 ********************************************************************************/
void CHWMonitor::boot_led_ctrl(u8 i_c_enable)
{
	if (is_vw_revision_board())
		return;
	ioctl_data_t 	iod_t;
	memset(&iod_t, 0, sizeof(ioctl_data_t));
	iod_t.data[0] = i_c_enable;
	ioctl( m_n_fd, VW_MSG_IOCTL_GPIO_BOOT_LED_RST_CTRL, &iod_t );
}

/********************************************************************************
 * \brief		
 * \param
 * \return
 * \note
 ********************************************************************************/
void CHWMonitor::boot_end(void)
{	
	// 초기 부팅 동작에 의한 inrush에 의해 메인 전원이 다운될 가능성이 있어,
	// 전류가 많이 소모되는 배터리 충전 동작은 부팅 완료 후에 진행되도록 함 
	battery_proc_start();
}

/********************************************************************************
 * \brief		
 * \param
 * \return
 * \note
 ********************************************************************************/
u8 CHWMonitor::get_aed_count(void)
{
	return m_c_aed_num;
}

/********************************************************************************
 * \brief	set the number of AED depending on the model
 * \param
 * \return
 * \note
 ********************************************************************************/
void CHWMonitor::set_aed_count(MODEL i_model_e)
{
	switch (i_model_e)
	{
	case eMODEL_2530VW:
	case eMODEL_3643VW:
	case eMODEL_4343VW:
		m_c_aed_num = 2;
		break;

	default:
		m_c_aed_num = 0;
		break;
	}
}


/********************************************************************************
 * \brief		
 * \param
 * \return
 * \note
 ********************************************************************************/
u8 CHWMonitor::get_battery_count(void)
{
	return m_c_battery_num;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u8 CHWMonitor::get_bat_equip_start_idx(void)
{
    u8  c_equip_idx = 0;
    u32 n_idx;
    
    for( n_idx = 0; n_idx < eBATTERY_MAX; n_idx++ )
   	{
   		if( m_battery_t[n_idx].b_equip == true)
   		{
   			c_equip_idx = n_idx;
   			break;
   		}
   	}
   	
   	return c_equip_idx;
}

/********************************************************************************
 * \brief		
 * \param
 * \return
 * \note	ONLY FOR TEST
 ********************************************************************************/
void CHWMonitor::adc_read_test(u8 i_c_ch)
{
	if (is_vw_revision_board())
	{
		print_log(DEBUG, 1, "[%s] NOT SUPPORTED(%s)\n", m_p_log_id, __func__);
		return;
	}

	ioctl_data_t 	iod_t;
	iod_t.data[0] = i_c_ch;
	ioctl( m_n_adc_fd , VW_MSG_IOCTL_SPI_ADC_READ_TEST, &iod_t );
	print_log(DEBUG, 1, "[%s] ADC read ch : %d, data : 0x%02x\n", m_p_log_id, i_c_ch, iod_t.data[0] );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CHWMonitor::aed_power_enable( bool i_b_on )
{
    ioctl_data_t 	iod_t;
    iod_t.data[0] = (u8)i_b_on;
    ioctl( m_n_fd, VW_MSG_IOCTL_GPIO_AED_PWR_EN_CNTL, &iod_t ); 
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CHWMonitor::aed_power_en_status(void)
{
    ioctl_data_t 	iod_t;
    memset(&iod_t, 0x00, sizeof(ioctl_data_t));
    ioctl( m_n_fd, VW_MSG_IOCTL_GPIO_AED_PWR_EN_STATUS, &iod_t ); 
	return (bool)iod_t.data[0];
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CHWMonitor::pmic_reset(void)
{
    ioctl_data_t 	iod_t;
    
    ioctl( m_n_fd, VW_MSG_IOCTL_GPIO_PMIC_RST, &iod_t ); 
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CHWMonitor::hw_power_off(void)
{
	if (is_vw_revision_board())
	{
		m_p_micom_c->power_off();
	}
	else
	{
		ioctl_data_t 	iod_t;
		ioctl( m_n_fd, VW_MSG_IOCTL_GPIO_PWR_OFF, &iod_t );
	}
} 	

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CHWMonitor::power_source_can_charge(POWER_SOURCE i_pwr_src_e)
{
	if(ePWR_SRC_TETHER == i_pwr_src_e || ePWR_SRC_USB == i_pwr_src_e)
		return true;
	else
		return false;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CHWMonitor::is_power_source_once_connected(void)
{
    if( m_b_pwr_src_once_connected == false
        && power_source_can_charge(m_power_volt_t.power_source_e) == true )
    {
        m_b_pwr_src_once_connected = true;
    }
    
    return m_b_pwr_src_once_connected;
}

/******************************************************************************/
/**
 * @brief   
 * @param   i_w_vcell: unit 0.01V
 			i_soc_raw: unit 0.1%
 * @return  
 * @note    Vcell의 범위를 9V ~ 13.1V 로 가정한다. 
            9보다 낮은 경우나, 13.1 보다 큰 경우는 예외 처리 
			(true 반환. Vcell의 validation은 bat_is_valid_vcell 에서 수행하기 때문) 


			validation invalid logic 변경
			변경1: 기존 table 값 참조한 invalid 로직 삭제
			변경2: 12V 이하의 vcell 전압이고 & 100% soc 값일 경우, invalid 처리
*******************************************************************************/
bool CHWMonitor::bat_is_valid_soc_raw( u16 i_w_vcell, u16 i_soc_raw )
{
	//u16 w_vcell_conv_idx = 0x0000;
	//u16 w_vcell_lut_soc_raw = 0x0000;

	//print_log(DEBUG, 1, "[%s] temp (%d %d)\n",  m_p_log_id, i_w_vcell, i_soc_raw);

	//디텍터가 IDLE 상태일 때에만 유효한 validation 임. 촬영 / 충전 / 배터리 탈착 등의 동작 시에는 하기 조건은 정상 상태에서도 발생할 수 있음.
	//10.5V 이상의 vcell 전압이고 & 1% soc 미만 값일 경우, invalid 처리
	if( (i_w_vcell) >= 1050 && (i_soc_raw < 10) )
	{
		print_log(DEBUG, 1, "[%s] initial soc_raw_validation fail (%d %d)\n",  m_p_log_id, i_w_vcell, i_soc_raw);
		return false;
	}

	//12.5V 이하의 vcell 전압이고 & 100% soc 값일 경우, invalid 처리
	if( (i_w_vcell) <= 1250 && (i_soc_raw == 1000) )
	{
		print_log(DEBUG, 1, "[%s] initial soc_raw_validation fail (%d %d)\n",  m_p_log_id, i_w_vcell, i_soc_raw);
		return false;
	}

	return true;
}

/******************************************************************************/
/**
* \brief        start blinking charge led procedure
* \param        none
* \return       none
* \note			called when ePM_MODE_BATTERY_SWELLING_RECHARGING is received 
*******************************************************************************/
void CHWMonitor::blinking_charge_led_proc_start(void)
{
	// launch battery thread
    m_b_is_blinking_running = true;
	m_st_bat_chg_mode_info.c_block_recharging = true;

	if (pthread_create(&blinking_charge_led_thread_t, NULL, blinking_charge_led_routine, (void *)this) != 0)
	{
    	print_log( ERROR, 1, "[%s] battery_routine create failed:%s\n", m_p_log_id, strerror( errno ));

    	m_b_is_blinking_running = false;
    }
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CHWMonitor::blinking_charge_led_proc_stop(void)
{
	//ioctl_data_t 	iod_t;
	if( m_b_is_blinking_running && blinking_charge_led_thread_t != NULL )
    {
        m_b_is_blinking_running = false;
    	
    	if( pthread_join( blinking_charge_led_thread_t, NULL ) != 0 )
    	{
    		print_log( ERROR, 1,"[%s] pthread_join: battery_proc:%s\n", m_p_log_id, strerror( errno ));
    	}
    }	

	// m_battery_t[eBATTERY_0].b_charge = false;
	// m_battery_t[eBATTERY_1].b_charge = false;
	// bat_charge( eBAT_CHARGE_0_OFF_1_OFF);	// 강제적으로 bat_state 갱신
	// iod_t.data[0] = eBAT_CHARGE_0_OFF_1_OFF;
	// ioctl( m_n_fd, VW_MSG_IOCTL_GPIO_B_CHG_EN, &iod_t );

}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void * blinking_charge_led_routine( void * arg )
{
	CHWMonitor * mon = (CHWMonitor *)arg;
	mon->blinking_charge_led_proc();
	pthread_exit( NULL );
}

/******************************************************************************/
/**
* \brief        blinking charge led to show that the detector isn't recharging battery
* \param        none
* \return       none
* \note			period: 2.5s
				<reason>
				it takes around 1.5s to turn on charge led after B_CHG_EN is set
				so, LED turned on for 1s, turned off for 1.5s
				because of the reason there is no delay after B_CHG_EN is reset
*******************************************************************************/
void CHWMonitor::blinking_charge_led_proc(void)
{
    print_log(DEBUG, 1, "[%s] start blinking_charge_led_proc...(%d)\n", m_p_log_id, syscall( __NR_gettid ));
    
	u8 blinking_cnt = 0;
	ioctl_data_t 	iod_t;

    while( m_b_is_blinking_running == true )
    {
		iod_t.data[0] = eBAT_CHARGE_0_ON_1_ON;
		ioctl( m_n_fd, VW_MSG_IOCTL_GPIO_B_CHG_EN, &iod_t );
        sleep_ex(2400000);      // 2.5s
		//print_log(DEBUG, 1, "[%s] ON\n", m_p_log_id);
		iod_t.data[0] = eBAT_CHARGE_0_OFF_1_OFF;
		ioctl( m_n_fd, VW_MSG_IOCTL_GPIO_B_CHG_EN, &iod_t );
        sleep_ex(100000);      // 2.5s
		//print_log(DEBUG, 1, "[%s] OFF\n", m_p_log_id);
		blinking_cnt++;

		if(blinking_cnt >= BLINKING_CNT_MAX)
		{
			break;
		}
    }
	
	m_b_is_blinking_running = false;

	/*thread 탈출할 때는 항상 끄기*/
	//m_battery_t[eBATTERY_0].b_charge = false;
	//m_battery_t[eBATTERY_1].b_charge = false;
	bat_charge( eBAT_CHARGE_0_OFF_1_OFF);	// 강제적으로 bat_state 갱신

    print_log(DEBUG, 1, "[%s] stop blinking_charge_led_proc...\n", m_p_log_id);
}

/******************************************************************************/
/**
 * @brief   get battery charging mode in member val of HWMonitor
 * @param	None
 * @return  battery charging mode
 * @note	battery charging mode comes from eeprom oper info
 *******************************************************************************/
BAT_CHG_MODE CHWMonitor::get_battery_charging_mode(void)
{
	print_block_charge_change();
    return m_st_bat_chg_mode_info.chg_mode;
}

/******************************************************************************/
/**
 * @brief   set battery charging mode in member val of HWMonitor
 * @param	battery charging mode
 * @return  None
 * @note	battery charging mode comes from eeprom oper info
 *******************************************************************************/
void CHWMonitor::set_battery_charging_mode(BAT_CHG_MODE i_bat_chg_mode)
{
    m_st_bat_chg_mode_info.chg_mode = i_bat_chg_mode;

	if(m_st_bat_chg_mode_info.chg_mode == eCHG_MODE_LIFESPAN_EXTENSION)
	{
		m_p_micom_c->set_lifespan_extension(true);
	}
	else
	{
		m_p_micom_c->set_lifespan_extension(false);
	}
	
	print_block_charge_change();
}

/******************************************************************************/
/**
 * @brief   get battery recharging gauge in member val of HWMonitor
 * @param	None
 * @return  recharging gauge
 * @note	recharging gauge comes from eeprom oper info
 *******************************************************************************/
u8 CHWMonitor::get_battery_recharging_gauge(void)
{
	print_block_charge_change();
    return m_st_bat_chg_mode_info.c_rechg_gauge;
}

/******************************************************************************/
/**
 * @brief   set battery recharging gauge in member val of HWMonitor
 * @param	recharging gauge
 * @return  None
 * @note	recharging gauge comes from eeprom oper info
 *******************************************************************************/
void CHWMonitor::set_battery_recharging_gauge(u8 i_rechg_gauge)
{
	m_st_bat_chg_mode_info.c_rechg_gauge = i_rechg_gauge;
	print_block_charge_change();
}

/******************************************************************************/
/**
 * @brief   get battery gauge
 * @param	None
 * @return  battery gauge
 * @note	the calculation is equal to the gauge that sends to SDK
 *******************************************************************************/
u16 CHWMonitor::get_battery_gauge(void)
{
	u16 w_battery_gauge;
	u8 c_equip_idx = get_bat_equip_start_idx();

	if (get_battery_count() == 2)
	{
		w_battery_gauge = (m_battery_t[eBATTERY_0].w_soc + m_battery_t[eBATTERY_1].w_soc + 1) / 2;
	}
	else // if( get_battery_count() == 1 )
	{
		w_battery_gauge = m_battery_t[c_equip_idx].w_soc;
	}

	if(m_st_bat_chg_mode_info.chg_mode == eCHG_MODE_LIFESPAN_EXTENSION && w_battery_gauge >= 900)
	{
		return 900;
	}
	
	return w_battery_gauge;
}

/******************************************************************************/
/**
 * @brief   check power source and proceed block recharging
 * @param	cur_pwr_src: current power source
 * @param	old_pwr_src: previous power source
 * @return  None
 * @note	
 *******************************************************************************/
void CHWMonitor::check_pwr_src_and_block_recharging(POWER_SOURCE cur_pwr_src, POWER_SOURCE old_pwr_src)
{
	PM_MODE         pm_mode_e;

	if( cur_pwr_src == ePWR_SRC_BATTERY )
	{
		pm_mode_e = ePM_MODE_BATTERY_SWELLING_RECHG_BLK_STOP;
		message_send(eMSG_POWER_MODE, (u8*)&pm_mode_e, sizeof(pm_mode_e));
		
		return;
	}

	if( power_source_can_charge(cur_pwr_src) == true
		&& old_pwr_src != ePWR_SRC_UNKNOWN/*부팅으로 충전 되는 순간은 제외하기 위함*/ )
	{
		if (get_battery_gauge() >= m_st_bat_chg_mode_info.c_rechg_gauge * 10)
		{
			pm_mode_e = ePM_MODE_BATTERY_SWELLING_RECHG_BLK_START;
			message_send(eMSG_POWER_MODE, (u8*)&pm_mode_e, sizeof(pm_mode_e));
		}
	}
}

/******************************************************************************/
/**
 * @brief   check power source and proceed block recharging
 * @param	cur_pwr_src: current power source
 * @param	old_pwr_src: previous power source
 * @return  None
 * @note	
 *******************************************************************************/
void CHWMonitor::check_pwr_src_is_battery_and_charge_stop(POWER_SOURCE cur_pwr_src, POWER_SOURCE old_pwr_src)
{
	PM_MODE         pm_mode_e;

	if( cur_pwr_src == ePWR_SRC_BATTERY )
	{
        pm_mode_e = ePM_MODE_TETHER_POWER_OFF;
		message_send(eMSG_POWER_MODE, (u8*)&pm_mode_e, sizeof(pm_mode_e));
		m_st_bat_chg_mode_info.c_block_recharging = false;
		print_block_charge_change();
	}
}

/******************************************************************************/
/**
 * @brief   check battery gauge to recharge
 * @param	None
 * @return  None
 * @note	if current battery gauge is lower than recharging gauge because of natural discharge or changing batteries,
 * 			it needs to recharge again.
 *******************************************************************************/
void CHWMonitor::check_battery_gauge_to_recharge(void)
{
	if (get_battery_gauge() < m_st_bat_chg_mode_info.c_rechg_gauge * 10)
	{
		m_st_bat_chg_mode_info.c_block_recharging = false;
	}
}

/******************************************************************************/
/**
 * @brief   check batteries are fully charged
 * @param	None
 * @return  None
 * @note	if it is fully charged, set charging state full to block charging in driver code
 * 			this function needs because of the method of Battery Swelling that not to set B_CHG_EN disable -> enable 
 * 			in image acqusition when batteries arefully charged.
 *******************************************************************************/
void CHWMonitor::check_soc_is_full(void)
{
	bool b_is_fully_charged = bat_is_soc_full();
	if (b_is_fully_charged == true)	set_charging_state_full(true);
	else 							set_charging_state_full(false);
}

/******************************************************************************/
/**
 * @brief   print block
 * @param	None
 * @return  None
 * @note	
 *******************************************************************************/
void CHWMonitor::print_block_charge_change(void)
{
	print_log(DEBUG, 1, "[%s] blk full: %d, blk rechg: %d, blk blink: %d\n", m_p_log_id, bat_is_soc_full(), m_st_bat_chg_mode_info.c_block_recharging, m_b_is_blinking_running);
	print_log(DEBUG, 1, "[%s] current gauge: %d, recharging gauge(x10): %d\n", m_p_log_id, get_battery_gauge(), m_st_bat_chg_mode_info.c_rechg_gauge*10);
}

/***********************************************************************************************//**
* \brief		check if it is the board made after vw revision 
* \param		None
* \return		true: vw revision board
* \return		false: NOT vw revision board
* \note			vw revision board has the board number greater than 2
***************************************************************************************************/
bool CHWMonitor::is_vw_revision_board(void)
{
	if( hw_get_ctrl_version() <= 2) return false;
	else return true;
}

/***********************************************************************************************//**
* \brief		get adc value from risc peripheral
* \param		i_p_adc: adc value buffer
* \param		i_p_volt_t: constructure with power source and x100 voltage value from i_p_result
* \param		i_p_raw_volt: voltage value from adc based on 3.3V
* \param		i_p_result: real voltage value from adc 
* \return		None
* \note			this should be operated if it is not vw revision board
***************************************************************************************************/
void CHWMonitor::get_adc_from_risc(u16* i_p_adc, power_volt_t* i_p_volt_t, float* i_p_raw_volt, float* i_p_result)
{
	u16 i;
	u8 p_adc[POWER_VOLT_MAX];
	ioctl_data_t        iod_t;
	ioctl( m_n_adc_fd , VW_MSG_IOCTL_SPI_ADC_READ_ALL, &iod_t );        
	memcpy( p_adc, iod_t.data, sizeof(p_adc));
	// update voltage
	memset( i_p_volt_t, 0, sizeof(power_volt_t) );
	for( i = 0; i < POWER_VOLT_MAX; i++ )
	{
		i_p_adc[i] = (u16)p_adc[i];
		i_p_raw_volt[i] = p_adc[i] * ADC_2X_VREF_VOLT / (float)(ADC_RESOLUTION);
		if( i == POWER_VOLT_VBATT )
			i_p_result[i]   = i_p_raw_volt[i] * (115.0f/15.0f);
		else if( i == POWER_VOLT_VS )
			i_p_result[i]   = i_p_raw_volt[i] * (162.0f/62.0f);
		else if( hw_get_ctrl_version() >= 1 && (i == POWER_VOLT_AED_TRIG_A || i == POWER_VOLT_AED_TRIG_B) )
			i_p_result[i]   = i_p_raw_volt[i] * (5.0f/3.0f);
		else
			i_p_result[i] = i_p_raw_volt[i];
		
		i_p_volt_t->volt[i]   = (u16)(i_p_result[i] * 100);
	}
}

/***********************************************************************************************//**
* \brief		get adc value from micom peripheral
* \param		i_p_adc: adc value buffer
* \param		i_p_volt_t: constructure with power source and x100 voltage value from i_p_result
* \param		i_p_raw_volt: voltage value from adc based on 3.3V
* \param		i_p_result: real voltage value from adc 
* \return		None
* \note			this should be operated if it is vw revision board
***************************************************************************************************/
void CHWMonitor::get_adc_from_micom(u16* i_p_adc, power_volt_t* i_p_volt_t, float* i_p_raw_volt, float* i_p_result)
{
	u16 i;
	u8 p_recv[COMMAND_DATA_LEN * 2] = {0,};	//2개의 command 응답을 저장함. 
	
	m_p_micom_c->get(eMICOM_EVENT_BAT_ADC, NULL, p_recv, 0);
	m_p_micom_c->get(eMICOM_EVENT_AED_ADC, NULL, &p_recv[COMMAND_DATA_LEN], 0);
	
	for(i = 0; i < COMMAND_DATA_LEN; i++)
	{
		i_p_adc[i] = (u16)(p_recv[i * 2] << 8 & 0xFF00) | (u16)(p_recv[i * 2 + 1] & 0x00FF);
	}
	
	// update voltage
	memset( i_p_volt_t, 0, sizeof(power_volt_t) );
    for( i = 0; i < POWER_VOLT_MAX; i++ )
	{
	    i_p_raw_volt[i] = i_p_adc[i] * ADC_MICOM_VEF_VOLT / (float)(ADC_MICOM_RESOLUTION);
	    if( i == POWER_VOLT_VBATT )
	    {
			i_p_result[i]   = i_p_raw_volt[i] * (ADC_VBATT_VOLTAGE_DIVIDER_NUMERATOR / ADC_VBATT_VOLTAGE_DIVIDER_DENOMINATOR);
	    }
		else if( i == POWER_VOLT_AED_TRIG_A || i == POWER_VOLT_AED_TRIG_B )
		{
			i_p_result[i]   = i_p_raw_volt[i] * (ADC_AED_TRIG_VOLTAGE_DIVIDER_NUMERATOR / ADC_AED_TRIG_VOLTAGE_DIVIDER_DENOMINATOR);
		}
		else if( i == POWER_VOLT_VS )
		{
			i_p_result[i]   = i_p_raw_volt[i] * (ADC_VS_TRIG_VOLTAGE_DIVIDER_NUMERATOR / ADC_VS_TRIG_VOLTAGE_DIVIDER_DENOMINATOR);
		}
	    else
	    {
	    	i_p_result[i] = i_p_raw_volt[i];
	    }
	    
	    i_p_volt_t->volt[i]   = (u16)(i_p_result[i] * 100);
	}
}
