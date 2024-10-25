/******************************************************************************/
/**
 * @file    vw_i2c.cpp
 * @brief   i2c device
 * @author  
*******************************************************************************/

/*******************************************************************************
*   Include Files
*******************************************************************************/


#ifdef TARGET_COM
#include <stdio.h>
#include <errno.h>          // errno
#include <string.h>            // memset(), memcpy()
#include <iostream>
#include <sys/ioctl.h>  	// ioctl()
#include <fcntl.h>          // open() O_NDELAY
#include <stdlib.h>         // system()
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <unistd.h>

#include "vw_i2c.h"
#include "vw_time.h"
#else
#include "typedef.h"
#include "Mutex.h"
#include "File.h"
#include "Dev_io.h"
#include "Ioctl.h"
#include "Time.h"
#include "Test_double.h"
#include "../app/detector/include/vw_i2c.h"
#include <string.h>            // memset(), memcpy()
#endif



using namespace std;

/*******************************************************************************
*   Constant Definitions
*******************************************************************************/
#define I2C_ID_TEMPERATURE	    0x40
#define I2C_ID_PMIC			    0x08 // 0x0B ~ 0x0F
#define I2C_ID_BATTERY  	    0x36
#define I2C_ID_IMPACT			0x53
#define I2C_ID_IMPACT_R			0xA7
#define I2C_ID_IMPACT_W			0xA6

#define HDC_REG_CONFIG	 	   	0x02
#define HDC_REG_TEMPERATURE	   	0x00    // temperature
#define HDC_REG_HUMIDITY		0x01    // humidity
#define HDC_REG_DEVICE_ID		0xFF    // 0x1050

#define HDC_DEVICE_ID		    (0x1050)

#define MAX17041_VCELL_MSB      0x02
#define MAX17041_SOC_MSB        0x04
#define MAX17041_MODE_MSB       0x06
#define MAX17041_VER_MSB        0x08
#define MAX17041_RCOMP_MSB      0x0C
#define MAX17041_CMD_MSB        0xFE

#define MAX17041_CMD_RESET		    0x5400
#define MAX17041_CMD_QUICK_START    0x4000

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

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
CVWI2C::CVWI2C(s32 (*log)(s32, s32, const s8* ,...))
{
    print_log = log;
    
    io = new CMutex;
 
    m_n_i2c0_fd = -1;
	m_n_i2c1_fd = -1;
	m_n_i2c2_fd = -1;
	m_n_i2c3_fd = -1;

    print_log(DEBUG, 1, "[I2C] CVWI2C()\n");
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
CVWI2C::CVWI2C(void)
{
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
CVWI2C::~CVWI2C()
{
    close(m_n_i2c0_fd);
    close(m_n_i2c1_fd);
    close(m_n_i2c2_fd);
    close(m_n_i2c3_fd);

    safe_delete(io);
    
    print_log(DEBUG, 1, "[I2C] ~CVWI2C()\n");
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWI2C::initialize(MODEL i_model_e)
{
    m_model_e = i_model_e;
    
	m_n_i2c0_fd = open( "/dev/i2c-0", O_RDWR|O_NDELAY );
	if( m_n_i2c0_fd < 0 )
	{
		print_log( ERROR, 1, "[I2C] /dev/i2c-0 open error.\n" );

		return false;
	}
	
	m_n_i2c1_fd = open( "/dev/i2c-1", O_RDWR|O_NDELAY );
	if( m_n_i2c1_fd < 0 )
	{
		print_log( ERROR, 1, "[I2C] /dev/i2c-1 open error.\n" );

		return false;
	}
	
	//bat_init();
	
	m_n_i2c2_fd = open( "/dev/i2c-2", O_RDWR|O_NDELAY );
	if( m_n_i2c2_fd < 0 )
	{
		print_log(ERROR, 1, "[I2C] /dev/i2c-2 open error.\n");

		return false;
	}
	
	m_n_i2c3_fd = open( "/dev/i2c-3", O_RDWR|O_NDELAY );
	if( m_n_i2c2_fd < 0 )
	{
		print_log(ERROR, 1, "[I2C] /dev/i2c-3 open error.\n");

		return false;
	}
	
	temper_sensor_reset();
	
	return true;
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CVWI2C::temper_sensor_read( u8 i_c_addr )
{
    u8  p_data[3];
    u32	n_time_out = 10;
    s32 n_ret;
    s32 n_fd;
    
    n_fd = m_n_i2c3_fd;
    
    lock();
	
	if( ioctl( n_fd, I2C_SLAVE_FORCE, I2C_ID_TEMPERATURE ) < 0 )
	{
		print_log(ERROR, 1, "[I2C] temperature sensor address error.\n");
	}
	
	n_ret = write( n_fd, &i_c_addr, 1 );
	
	if( n_ret < 1 )
	{
	    print_log(ERROR, 1, "[I2C] temperature sensor read error(0x%02X: %d).\n", n_ret, i_c_addr);
	}
	
	sleep_ex(20000);
	
	do{
	    n_ret = read( n_fd, p_data, 3 );
	    sleep_ex(1000);
	}while(n_ret < 0 && n_time_out--);
	
    unlock();
    
    if( n_time_out < 10 )
	{
	    print_log(ERROR, 1, "[I2C] temperature sensor read retry : %d.\n", 10-n_time_out);
	}
	
    return (p_data[0] << 8) | ( p_data[1] & 0xFC );
}



/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CVWI2C::get_temperature(void)
{
    u16     w_data;
    float   f_result;
    
	w_data = temper_sensor_read(HDC_REG_TEMPERATURE);
	f_result = -40 + (float)165/(float)65536 *(float)w_data; //T= -46.85 + 175.72 * ST/2^16
    
    if( f_result < 0 )
    {
        return 0;
    }
    
    return (u16)(f_result * 10);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CVWI2C::get_humidity(void)
{
    u16     w_data;
    float   f_result;
    
	w_data = temper_sensor_read(HDC_REG_HUMIDITY);
	f_result = (w_data * 100)/(float)65536;
    
    if( f_result < 0 )
    {
        return 0;
    }
    
    return (u16)(f_result * 10);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWI2C::temper_sensor_reset(void)
{
    u8  c_addr = HDC_REG_CONFIG;
    s32 n_ret;
    s32 n_fd;
    u8	p_data[3];
    u32	n_time_out = 10;
    u16 w_data = 0;
    
	p_data[0] = HDC_REG_CONFIG;
	p_data[1] = 0x90;		// software reset
	p_data[2] = 0x00;
    
    n_fd = m_n_i2c3_fd;
    
    lock();
	
	if( ioctl( n_fd, I2C_SLAVE_FORCE, I2C_ID_TEMPERATURE ) < 0 )
    {
    	print_log(ERROR, 1, "[I2C] temperature sensor address error.\n");
    }
    
    n_ret = write( n_fd, p_data, 3 );
    
    if( n_ret < 1 )
    {
        print_log(ERROR, 1, "[I2C] temperature sensor reset error(%d).\n", n_ret);
    }
    
    sleep_ex(20000);    // 20ms
    
    unlock();
    
	
	memset( p_data, 0, sizeof(p_data) );
	
	lock();

	if( ioctl( n_fd, I2C_SLAVE_FORCE, I2C_ID_TEMPERATURE ) < 0 )
    {
    	print_log(ERROR, 1, "[I2C] temperature sensor address error.\n");
    }
    
    n_ret = write( n_fd, &c_addr, 1 );
    
    if( n_ret < 1 )
    {
        print_log(ERROR, 1, "[I2C] temperature sensor reset error(%d).\n", n_ret);
    }
    
    do{
   		n_ret = read( n_fd, p_data, 3 );
    	sleep_ex(1000);
	}while(n_ret < 0 && n_time_out--);
 
    unlock();   
    
    if( n_time_out < 10 )
	{
	    print_log(ERROR, 1, "[I2C] temperature sensor reset read retry : %d.\n", 10-n_time_out);
	}
    
    w_data = p_data[0] << 8;
	w_data |= p_data[1];
	
	print_log(DEBUG, 1, "[I2C] hdc config: 0x%04X.\n", w_data);
	print_log(DEBUG, 1, "[I2C] hdc temp: %d, humidity: %d\n", get_temperature(), get_humidity());
}



/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CVWI2C::bat_reg_read( u8 i_c_addr, u8 i_c_sel )
{
    u8 p_data[2];
    u16 w_data;
    s32 n_fd;
    n_fd = bat_get_fd(i_c_sel);
    
    lock();
	if( ioctl( n_fd, I2C_SLAVE_FORCE, I2C_ID_BATTERY ) < 0 )
	{
		print_log(ERROR, 1, "[I2C] I2C_ID_BATTERY address error.\n");
	}
	
	write( n_fd, &i_c_addr, 1 );
	
	if(read( n_fd, p_data, sizeof(p_data) ) < 0 )
	{
		print_log( ERROR, 1, "[I2C] battery register(addr: 0x%04X) read failed.\n", i_c_addr );
	}
	unlock();
	
	w_data = p_data[0];
	w_data = w_data << 8;
	w_data = w_data | p_data[1];
	
	return w_data;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  unit: 0.01V
 * @note    
*******************************************************************************/
u16 CVWI2C::bat_get_volt(u8 i_c_sel)
{
	u16     w_vcell;
	float   f_volt;
	u16     w_volt;
	
	w_vcell = bat_reg_read(MAX17041_VCELL_MSB, i_c_sel);
	
	f_volt = (w_vcell >> 4) * 2.5f/1000.0f;
	
	f_volt = f_volt*13.2f/8.7f;
	
	w_volt = (u16)((f_volt + 0.005) * 100);
	
	return w_volt;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CVWI2C::bat_get_soc(u8 i_c_sel)
{
    u16     w_soc;
    float   f_soc;
    
    w_soc = bat_reg_read(MAX17041_SOC_MSB, i_c_sel);
    
    /*
    Bits=18
	SOCValue = ((SOC1 * 256) + SOC2) * 0.00390625
	Bits=19
	SOCValue = ((SOC1 * 256) + SOC2) * 0.001953125
	*/
	
    f_soc = w_soc /512.0f;
    w_soc = (u16)(f_soc * 10);
    
    if(w_soc > 1000)
    {
        return 1000;
    }
    
    return w_soc;	
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWI2C::bat_init_one(int i_n_fd)
{
	u8 p_data[3];
    
    p_data[0] = MAX17041_CMD_MSB;
    p_data[1] = (u8)((MAX17041_CMD_RESET & 0xFF00) >> 8);
    p_data[2] = (u8)(MAX17041_CMD_RESET & 0x00FF);
    
    lock();
	if( ioctl( i_n_fd, I2C_SLAVE_FORCE, I2C_ID_BATTERY ) < 0 )
	{
		print_log(ERROR, 1, "[I2C] I2C_ID address error.\n");
		unlock();
		return;
	}
	
	write( i_n_fd, p_data, sizeof(p_data) );
	
	sleep_ex(200000);   // 100 ms

    p_data[0] = MAX17041_RCOMP_MSB;
    p_data[1] = 0x6F;
    p_data[2] = 0x00;
    
    write( i_n_fd, p_data, sizeof(p_data) );
	
	unlock();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWI2C::bat_init(void)
{
	bat_init_one(m_n_i2c1_fd);
	bat_init_one(m_n_i2c2_fd);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    해당 함수가 호출 될 떄에는 배터리의 충전 / 방전 동작을 최소화 해야함.
 * 			(충전 circuit off, 촬영 X 환경에서만 하기 함수 호출 요망)
*******************************************************************************/
void CVWI2C::bat_quick_start( u8 i_c_sel)
{
    u8 p_data[3];
    s32 n_fd;
    
	print_log(ERROR, 1, "[I2C] bat_sw_quick_start idx: %d\n", i_c_sel);

    p_data[0] = MAX17041_MODE_MSB;
    p_data[1] = (u8)((MAX17041_CMD_QUICK_START & 0xFF00) >> 8);
    p_data[2] = (u8)(MAX17041_CMD_QUICK_START & 0x00FF);
    
    n_fd = bat_get_fd(i_c_sel);
	
    lock();
	if( ioctl( n_fd, I2C_SLAVE_FORCE, I2C_ID_BATTERY ) < 0 )
	{
		print_log(ERROR, 1, "[I2C] I2C_ID_BATTERY address error.\n");
		unlock();
		return;
	}
	
	write( n_fd, p_data, sizeof(p_data) );
	
	//sleep_ex(200000);   // 100 ms
	
	unlock();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWI2C::bat_set_rcomp(u16 i_w_comp, u8 i_c_sel)
{
    u8 p_data[3];
    s32 n_fd;
    
    p_data[0] = MAX17041_RCOMP_MSB;
    p_data[1] = (u8)(i_w_comp & 0xFF00) >> 8;
    p_data[2] = (u8)(i_w_comp & 0x00FF);
    
	n_fd = bat_get_fd(i_c_sel);
    
    lock();
	if( ioctl( n_fd, I2C_SLAVE_FORCE, I2C_ID_BATTERY ) < 0 )
	{
		print_log(ERROR, 1, "[I2C] I2C_ID_BATTERY address error.\n");
		unlock();
		return;
	}
	
	write( n_fd, p_data, sizeof(p_data) );
	
	unlock();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
s32	CVWI2C::bat_get_fd(u8 i_c_sel)
{
	if( i_c_sel == 0)
    {    	
    	return m_n_i2c1_fd;
    }
    else if( i_c_sel == 1)
    {
    	return m_n_i2c2_fd;
	}
	else
	{
		return m_n_i2c1_fd;
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CVWI2C::bat_get_rcomp(u8 i_c_sel)
{
    return bat_reg_read(MAX17041_RCOMP_MSB,i_c_sel);	
}

/***********************************************************************************************//**
* \brief		i2c_pmic_read_reg
* \param
* \return
* \note
***************************************************************************************************/
void CVWI2C::pmic_read_reg(u8 a_b_addr, u8 *a_p_reg)
{
	struct i2c_rdwr_ioctl_data msgset_t;
	struct i2c_msg				p_msg_t[2];
	p_msg_t[0].addr = I2C_ID_PMIC;
	p_msg_t[0].len = 1;
	p_msg_t[0].flags = 0;
	p_msg_t[0].buf = &a_b_addr;
	
	p_msg_t[1].addr = I2C_ID_PMIC;
	p_msg_t[1].len = 1;
	p_msg_t[1].flags |= 1;
	p_msg_t[1].buf = a_p_reg;
	
	msgset_t.nmsgs = 2;
	msgset_t.msgs = p_msg_t;
	lock();
	if( ioctl(m_n_i2c0_fd, I2C_SLAVE_FORCE, I2C_ID_PMIC) < 0 )
	{
		print_log(ERROR,1,"%s:%d %s  I2C_ID_PMIC address error.\n",DEBUG_INFO);
	}	
	
	if( ioctl(m_n_i2c0_fd, I2C_RDWR, &msgset_t) < 0 )
	{
		print_log(ERROR,1,"%s:%d %s  I2C_ID_PMIC read error.\n",DEBUG_INFO);
	}
	unlock();
}

/***********************************************************************************************//**
* \brief		i2c_pmic_write_reg
* \param
* \return
* \note
***************************************************************************************************/
void CVWI2C::pmic_write_reg(u8 a_b_addr, u8 a_b_reg)
{
	u8 p_wdata[2];
	
	lock();
	if( ioctl(m_n_i2c0_fd, I2C_SLAVE_FORCE, I2C_ID_PMIC) < 0 )
	{
		print_log(ERROR,1,"%s:%d %s  I2C_ID_PMIC address error.\n",DEBUG_INFO);
	}	
	
	p_wdata[0] = a_b_addr;
	p_wdata[1] = a_b_reg;
	//i2c_write(m_n_i2c_fd,p_wdata,2);
	write(m_n_i2c0_fd,p_wdata,2);
	
	unlock();
}

/***********************************************************************************************//**
* \brief		i2c_pmic_write_reg
* \param
* \return
* \note
***************************************************************************************************/
u8 CVWI2C::impact_read_reg(u8 i_c_addr)
{
	u32 n_ret = 0;
	u8	c_rdata = 0;
    u32 n_fd = m_n_i2c2_fd;
    
    lock();
	
	if( ioctl( n_fd, I2C_SLAVE_FORCE, I2C_ID_IMPACT ) < 0 )
	{
		print_log(ERROR, 1, "[I2C] impact sensor address error.\n");
	}
	
	n_ret = write( n_fd, &i_c_addr, 1 );
	
	if( n_ret < 1 )
	{
	    print_log(ERROR, 1, "[I2C] impact sensor write addr error(0x%02X: %d).\n", n_ret, i_c_addr);
	}

	n_ret = read( n_fd, &c_rdata, 1 );
	
	if( n_ret < 1 )
	{
	    print_log(ERROR, 1, "[I2C] impact read error %d.\n", n_ret);
	    c_rdata = 0;
	}	
	
	unlock();
	
	return c_rdata;

}

/***********************************************************************************************//**
* \brief		i2c_pmic_write_reg
* \param
* \return
* \note
***************************************************************************************************/
void CVWI2C::impact_write_reg(u8 i_c_addr, u8 i_c_data)
{
	u32 n_ret = 0;
	u8 p_wdata[2] = {0,};
    u32 n_fd = m_n_i2c2_fd;
    
    lock();
	
	if( ioctl( n_fd, I2C_SLAVE_FORCE, I2C_ID_IMPACT ) < 0 )
	{
		print_log(ERROR, 1, "[I2C] impact sensor address error.\n");
	}
	p_wdata[0] = i_c_addr;
	p_wdata[1] = i_c_data;
	
	n_ret = write( n_fd, p_wdata, 2 );
	
	if( n_ret < 1 )
	{
	    print_log(ERROR, 1, "[I2C] impact sensor write error : %d(adr : 0x%02X data : 0x%02X).\n", n_ret, i_c_addr, i_c_data);
	}

	unlock();

}


/***********************************************************************************************//**
* \brief		i2c_pmic_write_reg
* \param
* \return
* \note
***************************************************************************************************/
void CVWI2C::impact_write_read(u8* i_p_wdata, u32 i_n_wlen, u8* o_p_rdata, u32 i_n_rlen)
{
	u32 n_ret = 0;
    u32 n_fd = m_n_i2c2_fd;
    
    lock();
	
	if( ioctl( n_fd, I2C_SLAVE_FORCE, I2C_ID_IMPACT ) < 0 )
	{
		print_log(ERROR, 1, "[I2C] impact sensor address error.\n");
	}
	
	n_ret = write( n_fd, i_p_wdata, i_n_wlen );
	
	if( n_ret < 1 )
	{
	    print_log(ERROR, 1, "[I2C] impact sensor write addr error %d.\n", n_ret);
	}

	n_ret = read( n_fd, o_p_rdata, i_n_rlen );
	
	if( n_ret < 1 )
	{
	    print_log(ERROR, 1, "[I2C] impact read error %d.\n", n_ret);
	    o_p_rdata = NULL;
	}	
	
	unlock();
	
}
