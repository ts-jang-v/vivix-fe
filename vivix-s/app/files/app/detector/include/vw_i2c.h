/******************************************************************************/
/**
 * @file    vw_i2c.h
 * @brief   I2C device
 * @author  
*******************************************************************************/


#ifndef _VW_I2C_
#define _VW_I2C_

/*******************************************************************************
*   Include Files
*******************************************************************************/

#ifdef TARGET_COM
#include "typedef.h"
#include "mutex.h"
#include "vw_system_def.h"
#else
#include "typedef.h"
#include "Mutex.h"
#include "File.h"
#include "Dev_io.h"
#include "Ioctl.h"
#include "Time.h"
#include "Test_double.h"
#include "../app/detector/include/vw_system_def.h"
#endif


/*******************************************************************************
*   Constant Definitions
*******************************************************************************/

/*******************************************************************************
*   Type Definitions
*******************************************************************************/

/*******************************************************************************
*   Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*   Variable Definitions
*******************************************************************************/


/*******************************************************************************
*   Function Prototypes
*******************************************************************************/

/**
 * @class   
 * @brief   
 */
class CVWI2C
{
private:
    s32     (* print_log)(s32 level, s32 print, const s8* format, ...);
    
    CMutex *		    io;
	void			    lock(void){io->lock();}
	void			    unlock(void){io->unlock();}
	
	MODEL               m_model_e;
    
    s32     m_n_i2c0_fd;        // I2C: pmic
    s32     m_n_i2c1_fd;     	// I2C: battery A guage, temperature/humidity
    s32     m_n_i2c2_fd;     	// I2C: battery B guage, ACC sensor 
    s32     m_n_i2c3_fd;     	// I2C: TEMP, NFC 

	// battery
    u16     bat_reg_read( u8 i_c_addr , u8 i_c_sel);
    void    bat_init(void);
    s32		bat_get_fd(u8 i_c_sel);
    void 	bat_init_one(int i_n_fd);

	// Temp & Humidity
	
    
protected:
    
public:
	CVWI2C(s32 (*log)(s32, s32, const s8* ,...));
	CVWI2C(void);
	~CVWI2C();
	
	s32 bat_get_fd_test(u8 i_c_sel) {return this->bat_get_fd(i_c_sel);}
	void bat_init_test(void) { this->bat_init();}
	u16 bat_reg_read_test( u8 i_c_addr , u8 i_c_sel){return this->bat_reg_read(i_c_addr,i_c_sel);}
	void set_model_e(MODEL i_model_e) {m_model_e = i_model_e;}

	bool    initialize(MODEL i_model_e);
	
	// battery
	u16     bat_get_volt(u8 i_c_sel);
	u16     bat_get_soc(u8 i_c_sel);
	void    bat_quick_start(u8 i_c_sel);
	void    bat_set_rcomp(u16 i_w_comp,u8 i_c_sel);
	u16     bat_get_rcomp(u8 i_c_sel);
	
	
    void	temper_sensor_reset(void);
    u16     get_temperature(void);
    u16     get_humidity(void);
    u16		temper_sensor_read( u8 i_c_addr );
    
    void	pmic_read_reg(u8 a_b_addr, u8 *a_p_reg);
    void 	pmic_write_reg(u8 a_b_addr, u8 a_b_reg);
    
    u8		impact_read_reg(u8 i_c_addr);
    void	impact_write_reg(u8 i_c_addr, u8 i_c_data);
    void	impact_write_read(u8* i_p_wdata, u32 i_n_wlen, u8* o_p_rdata, u32 i_n_rlen);
    
};



#endif /* _VW_I2C_ */
