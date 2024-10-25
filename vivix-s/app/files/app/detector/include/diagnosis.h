/*
 * diagnosis.h
 *
 *  Created on: 2015. 11. 03.
 *      Author: myunghee
 */

 
#ifndef _VW_DIAGNOSIS_H_
#define _VW_DIAGNOSIS_H_

/*******************************************************************************
*	Include Files
*******************************************************************************/


#ifdef TARGET_COM
#include "typedef.h"
#include "vw_system.h"
#include "net_manager.h"
#else
#include "typedef.h"
#include "File.h"
#include "Vw_system.h"
#include "Net_manager.h"
#endif
/*******************************************************************************
*	Constant Definitions
*******************************************************************************/
#define DIAG_VALUE_LENGTH           (256)

/*******************************************************************************
*	Type Definitions
*******************************************************************************/
 /**
 * @brief   diag catetory
 */
typedef enum
{
    eDIAG_CATEGORY_VOLTAGE = 1,
    eDIAG_CATEGORY_BATTERY,
    eDIAG_CATEGORY_WIRELESS,
    eDIAG_CATEGORY_SENSOR,
    eDIAG_CATEGORY_MEMORY,
    eDIAG_CATEGORY_IC,
    
    eDIAG_CATEGORY_NONE = 0xFF,
    
} DIAG_CATEGORY;

/**
 * @brief   diag result
 */
typedef enum
{
    eDIAG_RESULT_SUCCESS = 0,
    eDIAG_RESULT_FAIL,
    eDIAG_RESULT_TIMEOUT,
    
} DIAG_RESULT;

typedef enum
{
	eDIAG_TYPE_DIAG=0,
    eDIAG_TYPE_INFO,
    
    eDIAG_TYPE_NONE = 0xff
}DIAG_TYPE;

typedef struct _diag_info_
{
	DIAG_CATEGORY	category_e;
	u8				c_item_id;
	const s8*		p_name;
	DIAG_TYPE       type_e;
	s32				n_timeout; //msec
	bool			b_scu_used;
	const s8*		p_criteria;
	const s8*		p_description;	
}diag_info_t;

/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*	Variable Definitions
*******************************************************************************/


/*******************************************************************************
*	Function Prototypes
*******************************************************************************/
void *         	diag_routine( void * arg );

class CDiagnosis
{
private:
    s8                  m_p_log_id[15];
    int			        (* print_log)(int level,int print,const char * format, ...);
    
    bool                m_b_diag_is_running;
    pthread_t	        m_diag_thread_t;
    diag_info_t*        m_p_diag_info_t;
    DIAG_RESULT         m_diag_result_e;
    u8                  m_c_diag_progress;
    
    void                diag_proc(void);
    bool                diag_find_item( DIAG_CATEGORY i_category_e, u8 i_c_id );
    void                diag_reset(void){m_p_diag_info_t=NULL;}
    bool                diag_make_result(void);
    bool                diag_print( s8* i_p_result, s8 i_p_value[][DIAG_VALUE_LENGTH], u16 i_w_value_count );
    
    DIAG_RESULT         diag_voltage( u8 i_c_id );
    DIAG_RESULT         diag_battery( u8 i_c_id );
    DIAG_RESULT         diag_wireless( u8 i_c_id );
    bool                diag_wireless_module(void);
    bool                diag_wireless_connection(void);
    
    DIAG_RESULT         diag_sensor( u8 i_c_id );
    bool                diag_sensor_aed(void);
    bool                diag_sensor_impact(void);
    
    DIAG_RESULT         diag_memory( u8 i_c_id );
    bool                diag_memory_detection(void);
    bool                diag_memory_filesystem(void);
    bool                diag_memory_use( u16* o_p_use );
    
    DIAG_RESULT         diag_ic( u8 i_c_id );
    bool                diag_ic_rtc(void);
    void                diag_ic_rtc_start_coin_cell(void);
    void                diag_ic_rtc_check_coin_cell(void);
    
    bool                diag_ic_fpga(void);
    u16                 diag_ic_fpga_test( u16 i_w_data );
    
protected:
public:
    
	CDiagnosis(int (*log)(int,int,const char *,...));
	CDiagnosis(void);
	~CDiagnosis();
	
    // self diagnosis
    friend void *       diag_routine( void * arg );
    bool                diag_make_list( bool i_b_scu );
    bool                diag_start( DIAG_CATEGORY i_category_e, u8 i_c_id );
    void                diag_stop(void);
    u8                  diag_get_progress(void){return m_c_diag_progress;}
    DIAG_RESULT         diag_get_result(void){return m_diag_result_e;}
    
    CVWSystem*          m_p_system_c;
    CNetManager*		m_p_net_manager_c;
};


#endif /* end of _VW_DIAGNOSIS_H_*/

