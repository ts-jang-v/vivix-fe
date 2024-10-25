/******************************************************************************/
/**
 * @file    vw_spi.h
 * @brief   SPI device
 * @author  
*******************************************************************************/


#ifndef _VW_SPI_
#define _VW_SPI_

/*******************************************************************************
*   Include Files
*******************************************************************************/
#ifdef TARGET_COM
#include <linux/spi/spidev.h>
#include "typedef.h"
#include "mutex.h"
#include "vw_system_def.h"
#else
#include "typedef.h"
#include "Mutex.h"
#include "Vw_spi_double_function.h"
#define _IOWR(a,b,c) b
#endif

/*******************************************************************************
*   Constant Definitions
*******************************************************************************/
#define ACC_GYRO_REG_INT1_CTRL			0x0D
#define ACC_GYRO_REG_WHO_AM_I			0x0F
#define ACC_GYRO_REG_CTRL_1_XL			0x10
	#define XL_LPF1_BW_SEL_MASK			0x02
		#define XL_LPF1_0				0x00
		#define XL_LPF1_1				0x02
	#define XL_FS_MASK					0x0C
		#define XL_FS_2G				0x00
		#define XL_FS_4G				0x04
		#define XL_FS_8G				0x08
		#define XL_FS_16G				0x12
	#define XL_ODR_MASK					0xF0
		#define XL_ODR_1P6_HZ			0xB0
		#define XL_ODR_12P5_HZ			0x10
		#define XL_ODR_26_HZ			0x20
		#define XL_ODR_52_HZ			0x30
		#define XL_ODR_104_HZ			0x40
		#define XL_ODR_208_HZ			0x50
		#define XL_ODR_416_HZ			0x60
		#define XL_ODR_833_HZ			0x70
#define ACC_GYRO_REG_CTRL_2_G			0x11
	#define G_FS_MASK					0x0C
		#define G_FS_250_DPS			0x00
		#define G_FS_500_DPS			0x02
		#define G_FS_1000_DPS			0x03
		#define G_FS_2000_DPS			0x01
	#define G_ODR_MASK					0xF0
		#define G_ODR_12P5_HZ			0x10
		#define G_ODR_26_HZ				0x20
		#define G_ODR_52_HZ				0x30
		#define G_ODR_104_HZ			0x40
		#define G_ODR_208_HZ			0x50
		#define G_ODR_416_HZ			0x60
		#define G_ODR_833_HZ			0x70
		
#define ACC_GYRO_REG_CTRL_3_C			0x12
	#define	ADDR_AUTO_INC				0x04
#define ACC_GYRO_REG_CTRL_6_C			0x15
	#define	XL_OFFSET_WEIGHT			0x08

#define ACC_GYRO_REG_CTRL8_XL			0x17
	#define XL_LOW_PASS_ON_MASK			0x01
		#define XL_LOW_PASS_ON_0		0x00
		#define XL_LOW_PASS_ON_1		0x01
	#define XL_HP_SLOPE_XL_EN_MASK		0x04
		#define XL_HP_SLOPE_XL_EN_0		0x00
		#define XL_HP_SLOPE_XL_EN_1		0x04
	#define XL_INPUT_COMPOSITE_MASK		0x08
		#define XL_INPUT_COMPOSITE_0	0x00
		#define XL_INPUT_COMPOSITE_1	0x08
	#define XL_HP_REF_MODE_MASK			0x10
		#define XL_HP_REF_MODE_DISABLE	0x00
		#define XL_HP_REF_MODE_ENABLE	0x10
	#define XL_HPCF_XL_MASK				0x60
		#define XL_HPCF_XL_00			0x00
		#define XL_HPCF_XL_01			0x20
	#define XL_LPF2_XL_EN_MASK			0x80
		#define XL_LPF2_XL_DISABLE		0x00
		#define XL_LPF2_XL_ENABLE		0x80

#define ACC_GYRO_REG_STATUS				0x1E

#define ACC_GYRO_REG_OUTX_L_G			0x22
#define ACC_GYRO_REG_OUTX_L_XL			0x28

//offset correction expressed in two's complement, weight depends on the CTRL6_C(4) bit. the value must be in range [-127, 127]
//defulat: 2^-10 g/LSB (-127 ~ 127 은 -0.124g ~ 0.124g)
#define ACC_GYRO_REG_X_OFS_USR			0x73	
#define ACC_GYRO_REG_Y_OFS_USR			0x74
#define ACC_GYRO_REG_Z_OFS_USR			0x75

#define ACC_GYRO_REG_VAL_WHO_AM_I		0x6A		//ISM330DLC


/*******************************************************************************
*   Type Definitions
*******************************************************************************/
typedef struct _acc_data
{
	s16 w_x;
	s16 w_y;
	s16 w_z;
}acc_data_t;

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
class CVWSPI
{
private:
	s32     			(* print_log)(s32 level, s32 print, const s8* format, ...);
	CMutex *		    io;
	void			    lock(void){io->lock();}
	void			    unlock(void){io->unlock();}
    s32     			m_n_spi_fd;         // SPI    
    s32     			m_n_gpio_fd;        // gpio
    
    s8					m_b_display_onoff;

	s32 	oled_write(u8* i_p_tx, u8* o_p_rx, u8 i_c_len);
	void	gpio_pin_ss(u8 i_c_ctrl);
	void 	gpio_pin_a0(u8 i_c_ctrl);
	void 	gpio_pin_reset(u8 i_c_ctrl);

protected:
    
public:
	CVWSPI(s32 i_n_gpio_fd, s32 i_n_spi_fd, s32 (*log)(s32, s32, const s8* ,...));
	~CVWSPI();
	
	
	void oled_sp9020_write(u8 i_n_idx, u8* i_p_val = NULL, u32 i_n_length = 0);
		
	void oled_set_memory_region(u8 i_c_xs, u8 i_c_ys, u8 i_c_x, u8 i_c_y);
	void oled_init(void);
	void oled_sw_reset(void);

	void oled_display_onoff(u8 i_c_onoff);
	void oled_display(u8* i_p_image, u32 i_n_length);
	void oled_clear(void);

	//ISM330DLC
	void	acc_gyro_combo_sensor_init(void);
	bool	acc_gyro_probe(void);
	void 	acc_gyro_get_pos(s16* o_p_x, s16* o_p_y, s16* o_p_z);
	s32		acc_gyro_reg_write(u8 i_c_addr, u8* i_p_data, u8 i_c_len);
	s32		acc_gyro_reg_read(u8 i_c_addr, u8* o_p_data, u8 i_c_len);


};



#endif /* _VW_SPI_ */
