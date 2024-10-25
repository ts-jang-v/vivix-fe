/******************************************************************************/
/**
 * @file    vw_spi.cpp
 * @brief   spi device
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
#include <linux/spi/spidev.h>
#include "vw_spi.h"
#include "vw_time.h"
#include "vworks_ioctl.h"
#else
#include "../app/detector/include/vw_spi.h"
#include "../driver/include/vworks_ioctl.h"
#include <string.h>            // memset(), memcpy()
//#include "vworks_ioctl.h"
#endif

using namespace std;

/*******************************************************************************
*   Constant Definitions
*******************************************************************************/
#define HIGH		1
#define LOW			0
#define DEV_SPI    "/dev/vivix_spi"

#define delay(n) sleep_ex((n)*1000)

#define SP9020_REG_SOFT_RST				0x01
#define SP9020_REG_DDISP_ONOFF			0x02
#define SP9020_REG_DATA_RW				0x08
#define SP9020_REG_DISP_DIRECTION		0x09
#define SP9020_REG_DISP_PEAK_WIDTH		0x10
#define SP9020_REG_PREC_DOT_CURRENT		0x12
#define SP9020_REG_DSTBY_ONOFF			0x14
#define SP9020_REG_PEAK_DELAY			0x16
#define SP9020_REG_ROW_SCAN				0x17
#define SP9020_REG_PRECHARGE_WIDTH		0x18
#define SP9020_REG_DFRAME				0x1A
#define SP9020_REG_DATA_REVERSE			0x1C
#define SP9020_REG_WRITE_DIRECTION		0x1D
#define SP9020_REG_COUT_SEQ				0x1E

#define SP9020_REG_READ_REG				0x20

#define SP9020_REG_DISP_SIZE_X			0x30
#define SP9020_REG_DISP_SIZE_Y			0x32
#define SP9020_REG_DATA_BOX_ADDR_XS		0x34
#define SP9020_REG_DATA_BOX_ADDR_XE		0x35
#define SP9020_REG_DATA_BOX_ADDR_YS		0x36
#define SP9020_REG_DATA_BOX_ADDR_YE		0x37
#define SP9020_REG_DISPLAY_ADDR_XS		0x38
#define SP9020_REG_DISPLAY_ADDR_YS		0x39
#define SP9020_REG_AGINC_EN				0x3C
#define SP9020_REG_EXT_IREF				0x3D
#define SP9020_REG_VCC_R_SEL			0x3F
#define SP9020_REG_MD_TEST				0x40
#define SP9020_REG_LOW_SCAN				0x41
#define SP9020_REG_DC_DC_SET			0x42
#define SP9020_REG_ROW_OVERLAP			0x48

#define SP9020_REG_S_SLEEP_TIMER		0xC0
#define SP9020_REG_S_SLEEP_START		0xC2
#define SP9020_REG_S_STEP_TIMER			0xC3
#define SP9020_REG_S_STEP_UNIT			0xC4
#define SP9020_REG_S_SCROLL_AREA		0xC6
#define SP9020_REG_S_CONDITION			0xCC
#define SP9020_REG_S_START_STOP			0xCD

#define OLED_PANEL_DOT_MATRIX_WIDTH		128
#define OLED_PANEL_DOT_MATRIX_HEIGHT	32
#define	SP9020_DOT_MATRIX_MEM_WIDTH		128
#define	SP9020_DOT_MATRIX_MEM_HEIGHT	40
#define	SP9020_DOT_MATRIX_MEM_SIZE_BYTE	(SP9020_DOT_MATRIX_MEM_WIDTH*SP9020_DOT_MATRIX_MEM_HEIGHT/8)

#define SP9020_DELAY					(0)
#define SP9020_RST_DELAY_MS				(255)

#define	IMX_ECSPI_TX_FIFO_MAX_ENTRY		64

#define STM_GYRO_COMBO_RD_BIT			0x80
#define STM_GYRO_COMBO_WR_BIT			0x00

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
CVWSPI::CVWSPI(s32 i_n_gpio_fd, s32 i_n_spi_fd, s32 (*log)(s32, s32, const s8* ,...))
{
	print_log = log;
	io = new CMutex;
	m_n_spi_fd = i_n_spi_fd;
	m_n_gpio_fd = i_n_gpio_fd;
	m_b_display_onoff = -1;

	print_log(DEBUG, 1, "[SPI] CVWSPI()\n");
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
CVWSPI::~CVWSPI()
{
    
    safe_delete(io);
    
    
    print_log(DEBUG, 1, "[SPI] ~CVWSPI()\n");
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
s32 CVWSPI::oled_write(u8* i_p_tx, u8* o_p_rx, u8 i_c_len)
{
	ioctl_data_t 	iod_t;
	
	iod_t.size = i_c_len;
	memcpy(iod_t.data, i_p_tx, i_c_len);
	
	if (ioctl( m_n_spi_fd, VW_MSG_IOCTL_SPI_OLED_WRITE ,(char*)&iod_t ) < 0)
	{
  		print_log( ERROR, 1, "[SPI] SPI write error!!! len : %d\n",i_c_len);
  		return -1;
  	}
  
	return 0;
}

/******************************************************************************/
/**
* \brief        gpio_pin_ss
* \param        i_c_ctrl        ON/OFF
* \return       none
* \note
*******************************************************************************/
void CVWSPI::gpio_pin_ss(u8 i_c_ctrl)
{
	ioctl_data_t 	iod_t;
	iod_t.data[0] = i_c_ctrl;
	ioctl( m_n_gpio_fd, VW_MSG_IOCTL_GPIO_OLED_SS ,(char*)&iod_t );	
	sleep_ex(1);
	
}

/******************************************************************************/
/**
* \brief        gpio_pin_a0
* \param        i_c_ctrl        ON/OFF
* \return       none
* \note
*******************************************************************************/
void CVWSPI::gpio_pin_a0(u8 i_c_ctrl)
{
	ioctl_data_t 	iod_t;
	iod_t.data[0] = i_c_ctrl;
	ioctl( m_n_gpio_fd, VW_MSG_IOCTL_GPIO_OLED_A0, (char*)&iod_t );
	sleep_ex(1);
}

/******************************************************************************/
/**
* \brief        gpio_pin_reset
* \param        i_c_ctrl        ON/OFF (active Low)
* \return       none
* \note
*******************************************************************************/
void CVWSPI::gpio_pin_reset(u8 i_c_ctrl)
{
	ioctl_data_t 	iod_t;
	iod_t.data[0] = i_c_ctrl;
	ioctl( m_n_gpio_fd, VW_MSG_IOCTL_GPIO_OLED_RESET ,(char*)&iod_t ); 
}

/******************************************************************************/
/**
* \brief        oled_sw_reset
* \param        
* \return       
* \note
*******************************************************************************/
void CVWSPI::oled_sw_reset(void)
{
 	// Software reset
	oled_sp9020_write(SP9020_REG_SOFT_RST);

	delay(SP9020_RST_DELAY_MS);
}

/********************************************************************************
 * \brief		oled_init
 * \param
 * \return
 * \note
 ********************************************************************************/
void CVWSPI::oled_init(void) 
{	
	u8	p_data[2];
 	
 	oled_sw_reset();

	// Dot Matrix Display Stand-by off -> OSC start
	p_data[0] = 0x00;
	oled_sp9020_write(SP9020_REG_DSTBY_ONOFF, p_data, 1);
	
	// OLED panel off, Dot matrix display off
	oled_display_onoff(0);
	
	// Display Direction
	// MIN X -> MAX X, MIN Y <- MAX Y
	p_data[0] = 0x01;
	oled_sp9020_write(SP9020_REG_DISP_DIRECTION, p_data, 1);
	
	// Peak Pulse Width Set -> 5 sclk (default)
	p_data[0] = 0x05;
	oled_sp9020_write(SP9020_REG_DISP_PEAK_WIDTH, p_data, 1);
	
	// Dot Matrix Current Level Set -> 1.25uA step * 0x2a = 52.5uA
	p_data[0] = 0x2A;
	oled_sp9020_write(SP9020_REG_PREC_DOT_CURRENT, p_data, 1);
	
	// Peak Pulse Delay Set -> 0 sclk (default)
	p_data[0] = 0x00;
	oled_sp9020_write(SP9020_REG_PEAK_DELAY, p_data, 1);
	
	// Row SCAN -> 0 (default)
	p_data[0] = 0x00;
	oled_sp9020_write(SP9020_REG_ROW_SCAN, p_data, 1);
	
	// Pre-Charge Width Set -> 8 sclk (default)
	p_data[0] = 0x08;
	oled_sp9020_write(SP9020_REG_PRECHARGE_WIDTH, p_data, 1);
	
	// Dot Matrix Frame Rate -> 120hz
	// 0 : 90Hz, 1: 120Hz, 2: 160Hz, 3: 180Hz
	p_data[0] = 0x01;
	oled_sp9020_write(SP9020_REG_DFRAME, p_data, 1);
	
	// Data Reverse
	p_data[0] = 0x00;
	oled_sp9020_write(SP9020_REG_DATA_REVERSE, p_data, 1);
	
	// Graphics Memory Writing Direction
	p_data[0] = 0x04;
	oled_sp9020_write(SP9020_REG_WRITE_DIRECTION, p_data, 1);
	
	// Column Sequence -> Mode 5 (Recommended)
	p_data[0] = 0x07;
	oled_sp9020_write(SP9020_REG_COUT_SEQ, p_data, 1);
	
	// Display Size X -> XS : 0, XE : 127
	p_data[0] = 0x00;
	p_data[1] = 0x7F;
	oled_sp9020_write(SP9020_REG_DISP_SIZE_X, p_data, 2);
	
	// Display Size Y -> YS : 0, YE : 32
	p_data[0] = 0x00;
	p_data[1] = 0x20;
	oled_sp9020_write(SP9020_REG_DISP_SIZE_Y, p_data, 2);

	// Column Display Start Address -> 0
	p_data[0] = 0x00;
	oled_sp9020_write(SP9020_REG_DISPLAY_ADDR_XS, p_data, 1);
	
	// Row Display Start Address -> 4
	p_data[0] = 0x04;
	oled_sp9020_write(SP9020_REG_DISPLAY_ADDR_YS, p_data, 1);		
	
	// Set External IREF resistor (Recommended)
	p_data[0] = 0x01;
	oled_sp9020_write(SP9020_REG_EXT_IREF, p_data, 1);

	// Set Internal Regulator for Row Scan -> Internal regulator enable, VCC_R = VCC_C *0.7
	p_data[0] = 0x11;
	oled_sp9020_write(SP9020_REG_VCC_R_SEL, p_data, 1);
	
	// DC-DC Set -> DC-DC Enable
	p_data[0] = 0x10;
	oled_sp9020_write(SP9020_REG_DC_DC_SET, p_data, 1);

	// Row Overlap Set
	p_data[0] = 0x0;
	oled_sp9020_write(SP9020_REG_ROW_OVERLAP, p_data, 1);
			
	oled_clear();
	
	// OLED panel on, Dot matrix display on
	oled_display_onoff(0);
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSPI::oled_sp9020_write(u8 i_n_idx, u8* i_p_val, u32 i_n_length)
{
	const s32 k_n_max_length = IMX_ECSPI_TX_FIFO_MAX_ENTRY;
	s32 n_ret = 0;
	u8* p_tx;
	u32 i;
	u8 p_rx[1];
	
	
	lock();
	sleep_ex(SP9020_DELAY);
	
	/* Write CMD */
	gpio_pin_ss(LOW);
	gpio_pin_a0(LOW);
	oled_write(&i_n_idx,p_rx,1);

	gpio_pin_a0(HIGH);	
	gpio_pin_ss(HIGH);
	/* Write CMD End */
	
	// End transmission if there is no data to transmit.
	if(i_p_val == NULL || i_n_length == 0)
	{
		unlock();
		return;
	}
			
	sleep_ex(SP9020_DELAY);
	
	/* Write Data */
	gpio_pin_ss(LOW);	

	for(i = 0; i < i_n_length; ) 
	{	
		p_tx = &i_p_val[i];
		if( (i_n_length - i) >= k_n_max_length)
		{
			sleep_ex(SP9020_DELAY);
			n_ret = oled_write(p_tx, 0, (u8)k_n_max_length);			
			
			i+=k_n_max_length;
		}
		else
		{	
			sleep_ex(SP9020_DELAY);
			n_ret = oled_write(p_tx, 0, (u8)(i_n_length - i));
			i+=	(i_n_length - i);		
		}
		
		if( n_ret < 0)
		{	
			break;
		}
	}
	
	gpio_pin_ss(HIGH);	
	/* Write Data End */
	
	unlock();
	sleep_ex(SP9020_DELAY);
}


/********************************************************************************
 * \brief		oled_set_memory_region
 * \param		u8 i_c_xs : dot matrix 메모리 영역의 시작 pixel X좌표(8의 배수여야 함)
 				u8 i_c_ys : dot matrix 메모리 영역의 시작 pixel Y좌표
 				u8 i_c_x : dot matrix 메모리 영역의 가로 pixel 개수(8의 배수여야 함)
 				u8 i_c_y : dot matrix 메모리 영역의 세로 pixel 개수
 * \return
 * \note		X값 입력 파라미터는 반드시 8의 배수여야 한다.
 ********************************************************************************/
void CVWSPI::oled_set_memory_region(u8 i_c_xs, u8 i_c_ys, u8 i_c_x, u8 i_c_y) 
{
	u8 c_addr = 0;
	
	/* 1byte 에 8pixel, memory상 영역 설정을 위한 address는 byte단위이므로 X값은 8로 나눠줌 */
	c_addr = i_c_xs / 8;
	oled_sp9020_write(SP9020_REG_DATA_BOX_ADDR_XS,&c_addr, 1);
	
	/* 1byte 에 8pixel, memory상 영역 설정을 위한 address는 byte단위이므로 X값은 8로 나눠줌 */
	c_addr = (i_c_xs + i_c_x) / 8 - 1;
	oled_sp9020_write(SP9020_REG_DATA_BOX_ADDR_XE,&c_addr, 1);
	
	c_addr = i_c_ys;
	oled_sp9020_write(SP9020_REG_DATA_BOX_ADDR_YS,&c_addr, 1);
	
	c_addr = i_c_ys + i_c_y - 1;
	oled_sp9020_write(SP9020_REG_DATA_BOX_ADDR_YE,&c_addr, 1);
}

/********************************************************************************
 * \brief		oled_display_onoff
 * \param		i_c_onoff
 				1: turn on the OLED panel, Dot matrix display on
 				0: turn off the OLED panel, Dot matrix display off
 * \return		none
 * \note
 ********************************************************************************/
void CVWSPI::oled_display_onoff(u8 i_c_onoff)
{
	if ( m_b_display_onoff == i_c_onoff)
		return;
	
	m_b_display_onoff = i_c_onoff;
	if( m_b_display_onoff == 1)
	{
		sleep_ex(100000);
		oled_sp9020_write(SP9020_REG_DDISP_ONOFF, &i_c_onoff, 1);
	}
	else
	{
		oled_clear();
		oled_sp9020_write(SP9020_REG_DDISP_ONOFF, &i_c_onoff, 1);
	}
} 

/********************************************************************************
 * \brief		oled_display
 * \param
 * \return
 * \note
 ********************************************************************************/
void CVWSPI::oled_display(u8* i_p_image, u32 i_n_length) 
{
	oled_sp9020_write(SP9020_REG_DATA_RW, i_p_image, i_n_length);
}


/********************************************************************************
 * \brief		oled_buffer_clear
 * \param
 * \return
 * \note
 ********************************************************************************/
void CVWSPI::oled_clear() 
{
	u8 p_reg[SP9020_DOT_MATRIX_MEM_SIZE_BYTE];
	memset(p_reg, 0, SP9020_DOT_MATRIX_MEM_SIZE_BYTE);
	
	oled_set_memory_region(0, 0, SP9020_DOT_MATRIX_MEM_WIDTH, SP9020_DOT_MATRIX_MEM_HEIGHT);

	oled_sp9020_write(SP9020_REG_DATA_RW, p_reg, SP9020_DOT_MATRIX_MEM_SIZE_BYTE);
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CVWSPI::acc_gyro_combo_sensor_init(void)
{
	u8	p_data[2];	
	
	p_data[0] = 0;
	p_data[0] = XL_LPF1_1 | XL_FS_2G | XL_ODR_416_HZ;
	acc_gyro_reg_write(ACC_GYRO_REG_CTRL_1_XL, p_data, 1);
	
	/*
	p_data[0] = 0;
	p_data[0] = XL_LPF1_1 | XL_FS_2G | XL_ODR_833_HZ;
	acc_gyro_reg_write(ACC_GYRO_REG_CTRL_1_XL, p_data, 1);

	p_data[0] = 0;
	p_data[0] = XL_INPUT_COMPOSITE_1 | XL_LPF2_XL_ENABLE;
	acc_gyro_reg_write(ACC_GYRO_REG_CTRL8_XL, p_data, 1);
	*/

	//Angular sensing control register. ????
	p_data[0] = 0;
	p_data[0] = G_FS_2000_DPS | G_ODR_52_HZ;
	acc_gyro_reg_write(ACC_GYRO_REG_CTRL_2_G, p_data, 1);

	/*
	//read test
	u8	p_test_data = 0;
	acc_gyro_reg_read(ACC_GYRO_REG_WHO_AM_I, &p_test_data, sizeof(p_test_data));
	print_log( INFO, 1, "[SPI] WHOAMI RETURN: 0x%02x\n", p_test_data);

	//write test		
	p_test_data = 0x3F;
	acc_gyro_reg_write(ACC_GYRO_REG_INT1_CTRL, &p_test_data, 1);
	acc_gyro_reg_read(ACC_GYRO_REG_INT1_CTRL, &p_test_data, 1);
	print_log(DEBUG, 1, "[TP] ACC_GYRO_REG_INT1_CTRL: 0x%X\n", p_test_data);
	*/
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CVWSPI::acc_gyro_probe(void)
{
	u8	p_data = 0;	
	
	acc_gyro_reg_read(ACC_GYRO_REG_WHO_AM_I, &p_data, sizeof(p_data));
	
	//print_log( INFO, 1, "[SPI] WHOAMI RETURN: 0x%02x\n", p_data);

	if( p_data == ACC_GYRO_REG_VAL_WHO_AM_I )
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
void CVWSPI::acc_gyro_get_pos(s16* o_p_x, s16* o_p_y, s16* o_p_z)
{
	acc_data_t	data_t;
	memset(&data_t, 0, sizeof(acc_data_t));
	
	acc_gyro_reg_read(ACC_GYRO_REG_OUTX_L_XL, (u8*)&data_t, sizeof(acc_data_t));
	
	*o_p_x = data_t.w_x;
	*o_p_y = data_t.w_y;
	*o_p_z = data_t.w_z;

	//print_log(DEBUG, 1, "[SPI][TP] x: %d y: %d z: %d \n", data_t.w_x, data_t.w_y, data_t.w_z);

}
	
/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
s32 CVWSPI::acc_gyro_reg_write(u8 i_c_addr, u8* i_p_data, u8 i_c_len)
{
	ioctl_data_t 	iod_t;
	
	iod_t.size = i_c_len;
	iod_t.data[0] = (i_c_addr & 0x7f) | STM_GYRO_COMBO_WR_BIT;
	memcpy(&iod_t.data[1], i_p_data, i_c_len);
	
	if (ioctl( m_n_spi_fd, VW_MSG_IOCTL_SPI_ACC_GYRO_REG_WRITE ,(char*)&iod_t ) < 0)
	{
  		print_log( ERROR, 1, "[SPI] ACC gyro combo SPI write error!!!\n");
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
s32 CVWSPI::acc_gyro_reg_read(u8 i_c_addr, u8* o_p_data, u8 i_c_len)
{
	ioctl_data_t 	iod_t;
	
	iod_t.size = i_c_len;
	iod_t.data[0] = (i_c_addr & 0x7f) | STM_GYRO_COMBO_RD_BIT;
	
	if (ioctl( m_n_spi_fd, VW_MSG_IOCTL_SPI_ACC_GYRO_REG_READ ,(char*)&iod_t ) < 0)
	{
  		print_log( ERROR, 1, "[SPI] ACC gyro combo SPI write error!!!\n");
  		return -1;
  	}
  	
  	memcpy(o_p_data, iod_t.data, i_c_len);
  
	return 0;
}
