/********************************************************************************  
 모  듈 : TI AFE2256 CHIP DRIVER
 작성자 : 한영준
 버  전 : 0.0.1
 설  명 : ti roic driver
 참  고 : 
 
          1. reference : roic_driver
          
 버  전:
         0.0.1  

********************************************************************************/

/***************************************************************************************************
*	Include Files
***************************************************************************************************/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>		// copy_from_user , copy_to_user
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/spi/spi.h>


#include "driver.h"
#include "vworks_ioctl.h"
#include "fpga_reg_def.h"
#include "reg_def.h"

extern void tg_reg_write(u32 a_naddr, u16 a_wdata);
extern u16 tg_reg_read(u32 a_naddr);

extern int vivix_gpio_get_value(u32 gpio_num);
extern void vivix_gpio_set_value(u32 gpio_num, u8 a_b_level);
extern void vivix_gpio_direction(u32 gpio_num, u8 a_b_level);

extern exp_wait_queue_ctx				exposure_wait_queue_ctx;
extern wait_queue_head_t				exposure_wait_queue;

/***************************************************************************************************
*	Constant Definition
***************************************************************************************************/
#define		ROIC_SPI_NAME					"ti_afe2256_roic"

#define 	ROIC_LINE_NUM						(256)


#define 	TIROIC_REG_NUM					(eTI_ROIC_REG_MAX)									//TP profile 포함 
#define 	TIROIC_REG_TG_NUM				(eTI_ROIC_REG_55_IDX - eTI_ROIC_REG_40_IDX + 1)		//TP only 40h ~ 55h
#define 	TIROIC_REG_TP_START_IDX			eTI_ROIC_REG_40_IDX									//40h			
#define 	TIROIC_REG_TP_END_IDX			eTI_ROIC_REG_55_IDX									//55h

#define 	TI_ROIC_REG_00					(0x00)
#define 	TI_ROIC_REG_10					(0x10)
#define 	TI_ROIC_REG_11					(0x11)
#define 	TI_ROIC_REG_12					(0x12)
#define 	TI_ROIC_REG_13					(0x13)
#define 	TI_ROIC_REG_16					(0x16)
#define 	TI_ROIC_REG_18					(0x18)
#define 	TI_ROIC_REG_2C					(0x2C)
#define 	TI_ROIC_REG_30					(0x30)
#define 	TI_ROIC_REG_31					(0x31)
#define 	TI_ROIC_REG_32					(0x32)
#define 	TI_ROIC_REG_33					(0x33)
#define 	TI_ROIC_REG_40					(0x40)
#define 	TI_ROIC_REG_41					(0x41)
#define 	TI_ROIC_REG_42					(0x42)
#define 	TI_ROIC_REG_43					(0x43)
#define 	TI_ROIC_REG_46					(0x46)
#define 	TI_ROIC_REG_47					(0x47)
#define 	TI_ROIC_REG_4A					(0x4A)
#define 	TI_ROIC_REG_4B					(0x4B)
#define 	TI_ROIC_REG_4C					(0x4C)
#define 	TI_ROIC_REG_50					(0x50)
#define 	TI_ROIC_REG_51					(0x51)
#define 	TI_ROIC_REG_52					(0x52)
#define 	TI_ROIC_REG_53					(0x53)
#define 	TI_ROIC_REG_54					(0x54)
#define 	TI_ROIC_REG_55					(0x55)
#define 	TI_ROIC_REG_5A					(0x5A)
#define 	TI_ROIC_REG_5C					(0x5C)
#define 	TI_ROIC_REG_5D					(0x5D)
#define 	TI_ROIC_REG_5E					(0x5E)
#define 	TI_ROIC_REG_61					(0x61)

#define		ROIC_REG_13_NORMAL_MODE				0x0000
#define		ROIC_REG_13_QUICK_WAKEUP			0x0020
#define		ROIC_REG_13_NAP_MODE_WITH_PLL_ON	0xF827
#define		ROIC_REG_13_NAP_MODE_WITH_PLL_OFF	0xF8A7
#define		ROIC_REG_13_POWER_DOWN_WITH_PLL_ON	0xFD3F
#define		ROIC_REG_13_POWER_DOWN_WITH_PLL_OFF	0xFFBF

#define		ROIC_UPPER_SIDE_NUM_MAX			12
#define		ROIC_LOWER_SIDE_NUM_MAX			12

#define		TI_ROIC_BIT_ALIGNMENT_RETRY_NUM	0

//TI ROIC & DAC control through FPGA 
#define SPI_ROIC_DAC 1


/***************************************************************************************************
*	Type Definitions
***************************************************************************************************/

/***************************************************************************************************
*	Macros (Inline Functions) Definitions
***************************************************************************************************/			

/***************************************************************************************************
*	Variable Definitions
***************************************************************************************************/
static dev_t vivix_ti_afe2256_dev;
static struct cdev vivix_ti_afe2256_cdev;
static struct class *vivix_ti_afe2256_class;

static struct spi_device *roic_spi;

static DEFINE_MUTEX(g_ioctl_mutex);

bool										g_b_single_readout;
s32											g_n_roic_num;
s32											g_n_upper_side_roic_num;
s32											g_n_lower_side_roic_num;
s32											g_n_dac_num;

volatile bool								g_b_log_print = false;

const u16 register_addr[TIROIC_REG_NUM] = {0x00, 0x10, 0x11, 0x12, 0x13,
										   0x16, 0x18, 0x2c, 0x2e, 0x30,
										   0x31, 0x32, 0x33,
										   0x40, 0x41, 0x42, 0x43, 0x46, // tp registers
										   0x47, 0x4a, 0x4b, 0x4c, 0x50, // tp registers
										   0x51, 0x52, 0x53, 0x54, 0x55, // tp registers
										   0x5a, 0x5c, 0x5d, 0x5e, 0x61};

// TG profile을 제외한 default 값들을 집어넣음
//[20190415][yongjin] 0x5E, 13bit는 dstasheet에 없는 hidden register로 Radio 촬영 시 나오는 불특정 노이즈 현상을 개선하기 위하여 추가
//동작은 reset circuit을 disable 시킨다.
//해당 bit는 readout 시에만 활성화되어야 하는데 촬영 중에 FPGA가 TPa, TPb를 번갈아가며 활성화시키는 현재 구조상 사용에 문제가 있음.
//이를 해결하기 위하여 TI 측에서 제안한 방법은 0x41 register를 이용하는 방법으로 TPa에 0x102, TPb에 0x00을 입력하면 TPb 활성화 시에도 0x5e 13bit가 set되어 있어도 괜찮음
u16 register_tp_none[TIROIC_REG_NUM] = {0x0000, 0x0800, 0x0430, 0x4000, 0xFFBF,
										0x00C0, 0x0001, 0x0000, 0x0000, 0x0000,
										0x0000, 0x0000, 0x0000,
										0x0000, 0x0000, 0x0000, 0x0000, 0x0000, // tp registers
										0x0000, 0x0000, 0x0000, 0x0000, 0x0000, // tp registers
										0x0000, 0x0000, 0x0000, 0x0000, 0x0000, // tp registers
										0x0000, 0x73DE, 0x0610, 0x2000, 0x4000};
// 40h ~ 55h
// for READOUT
u16 register_tpa[TIROIC_REG_TG_NUM] = {0x0120, 0x0102, 0x2166, 0x71FE, 0x2B67,	// tp registers
									   0xA6FF, 0x21F0, 0x0000, 0x0000, 0x0000,	// tp registers
									   0x71FF, 0x0000, 0x0000, 0x0000, 0x0000}; // tp registers

// for IDLE
u16 register_tpb[TIROIC_REG_TG_NUM] = {0x0101, 0x0000, 0x0000, 0x0000, 0x0000,	// tp registers
									   0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	// tp registers
									   0x0000, 0x0000, 0x0000, 0x0000, 0x0000}; // tp registers

u32 dac_default								= 	0x2E147;


/***************************************************************************************************
*	Function Prototypes
***************************************************************************************************/
void ti_roic_enter_low_power_mode(void);

/***********************************************************************************************//**
* \brief		spi_roic_read
* \param
* \return
* \note
***************************************************************************************************/
void spi_roic_read(u8 i_c_addr, u16* o_pdata)
{
	u8 c_addr = 0; 
	u16 w_data = 0;
	u8 p_wdata[3];
	u8 p_rdata[3];
	struct spi_transfer t;
	int ret;
	
	// ROIC register addr 00h의 REG_READ(bit1) 를 1로 set하여 register read 가능하도록 함 
	c_addr = 0x00;
	w_data = 0x0002;
	p_wdata[0] = c_addr;
	p_wdata[1] = (u8)(w_data >> 8);
	p_wdata[2] = (u8)(w_data);
	ret = spi_write(roic_spi, p_wdata, sizeof(p_wdata));
	if (ret < 0)
	{
		printk("%s[%d] spi_transfer fail(%d)\n", __func__, __LINE__, ret);
	}
	
	// register read 
	p_wdata[0] = i_c_addr;
	t.tx_buf = p_wdata;
	t.len	 = sizeof(p_wdata);
	t.rx_buf = p_rdata;
	ret = spi_sync_transfer(roic_spi, &t, 1);
	if (ret < 0)
	{
		printk("%s[%d] spi_transfer fail(%d)\n", __func__, __LINE__, ret);
	}

	w_data = (((p_rdata[1]) << 8) | (p_rdata[0]));

	*o_pdata = w_data;
}

/***********************************************************************************************//**
* \brief		spi_roic_write
* \param
* \return
* \note
***************************************************************************************************/
void spi_roic_write(u8 i_c_addr, u16* i_pdata)
{
	u8 c_addr = 0;
	u16 w_data = 0;
	u8 p_wdata[3];
	struct spi_transfer t;
	int ret;
	
	// ROIC register addr 00h의 REG_READ(bit1) 를 0으로 set하여 register write 가능하도록 함 
	c_addr = 0x00;
	w_data = 0x0000;
	p_wdata[0] = c_addr;
	p_wdata[1] = (u8)(w_data >> 8);
	p_wdata[2] = (u8)(w_data);
	ret = spi_write(roic_spi, p_wdata, 3);
	if (ret < 0)
	{
		printk("%s[%d] spi_transfer fail(%d)\n", __func__, __LINE__, ret);
	}
	
	// register write 
	c_addr = i_c_addr;
	w_data = *i_pdata;
	p_wdata[0] = c_addr;
	p_wdata[1] = (u8)(w_data >> 8);
	p_wdata[2] = (u8)(w_data);
	ret = spi_write(roic_spi, p_wdata, 3);
	if (ret < 0)
	{
		printk("%s[%d] spi_transfer fail(%d)\n", __func__, __LINE__, ret);
	}
}

/***********************************************************************************************//**
* \brief		spi_dac_read
* \param
* \return
* \note
***************************************************************************************************/
static int dac_transfer_init(void)
{
	int ret;
	
	roic_spi->bits_per_word = 16;
	ret = spi_setup(roic_spi);
	if (ret)
		printk("%s[%d] Failed to setup dac_spi_dev\n", __func__, __LINE__);

	return ret;
}

static int dac_transfer_done(void)
{
	int ret;
	
	roic_spi->bits_per_word = 24;
	ret = spi_setup(roic_spi);
	if (ret)
		printk("%s[%d] Failed to setup dac_spi_dev\n", __func__, __LINE__);

	return ret;
}

void spi_dac_read(u16* o_pdata)
{
	u16 w_data = 0;
	u8 p_rdata[2];
	int ret;

	dac_transfer_init();

	ret = spi_read(roic_spi, p_rdata, 2);
	if (ret < 0)
	{
		printk("%s[%d] spi_transfer fail(%d)\n", __func__, __LINE__, ret);
	}
	
	w_data = ((p_rdata[1] << 8) | p_rdata[0]);
	*o_pdata = w_data;

	dac_transfer_done();
}

/***********************************************************************************************//**
* \brief		spi_dac_write
* \param
* \return
* \note
***************************************************************************************************/
void spi_dac_write(u16* i_pdata)
{
	u16 w_data = 0;
	u8 p_wdata[2];
	int ret;
	
	dac_transfer_init();
	
	memcpy(&w_data,i_pdata, sizeof(w_data));
	p_wdata[0] = (u8)(w_data >> 8);
	p_wdata[1] = (u8)(w_data);
	
	ret = spi_write(roic_spi, p_wdata, 2);
	if (ret < 0)
	{
		printk("%s[%d] spi_transfer fail(%d)\n", __func__, __LINE__, ret);
	}
	
	dac_transfer_done();
}

/***********************************************************************************************//**
* \brief		개별 roic 선택 
* \param		
* \param		
* \note			
***************************************************************************************************/
void roic_chip_select( u8 idx )
{
	u16 w_data = 0;
	
	if(idx >= g_n_roic_num)
	{
		printk("Invalid ROIC index : %d\n",idx );
		return;
	}
			
	w_data |= (0x01 << idx);
	tg_reg_write(FPGA_TI_ROIC_MUX_SELECT, w_data);
}

/***********************************************************************************************//**
* \brief		모든 roic 선택 
* \param		
* \param		
* \note			
***************************************************************************************************/
void roic_chip_select_all(void)
{
	u16 w_data = 0;
	u8  c_idx = 0;
	for(c_idx = 0; c_idx < g_n_roic_num; c_idx++)
	{
		w_data |= (0x01 << c_idx);
	}
	
	tg_reg_write(FPGA_TI_ROIC_MUX_SELECT, w_data);
}

/***********************************************************************************************//**
* \brief		roic chip select 해제 
* \param		
* \param		
* \note			  
***************************************************************************************************/
void roic_chip_deselect(void)
{
	tg_reg_write(FPGA_TI_ROIC_MUX_SELECT, 0);
}

/***********************************************************************************************//**
* \brief		DAC 선택 
* \param		
* \param		
* \note			 
***************************************************************************************************/
void dac_chip_select( u8 idx )
{
	u16 w_data = 0;
	
	if(idx >= g_n_dac_num)
	{
		printk("Invalid DAC index : %d\n",idx );
		return;
	}
	
	w_data |= (0x01 << idx);
	tg_reg_write(FPGA_TI_DAC_MUX_SELECT, w_data);
}

/***********************************************************************************************//**
* \brief		개별 roic 선택 
* \param		
* \param		
* \note			  
***************************************************************************************************/
void dac_chip_deselect(void)
{
	tg_reg_write(FPGA_TI_DAC_MUX_SELECT, 0);
}

/***********************************************************************************************//**
* \brief		개별  roic에 data저장
* \param		a_waddr : reg address
* \param		a_wdata : reg data
* \return		void
* \note			
***************************************************************************************************/
void ti_roic_write_sel(u16 a_waddr, u16 a_wdata, u8 idx)
{
	roic_chip_select( idx );
	spi_roic_write((u8)a_waddr, &a_wdata);
	roic_chip_deselect();
}

/***********************************************************************************************//**
* \brief		개별 roic에 설정된 data읽어옴
* \param		a_waddr : reg address
* \param		a_wdata : reg data
* \note			
***************************************************************************************************/
void ti_roic_read_sel( u16 a_waddr, u16* a_pdata, u8 idx )
{
	roic_chip_select( idx );
	spi_roic_read((u8)a_waddr, a_pdata);
	roic_chip_deselect();
}

/***********************************************************************************************//**
* \brief		모든 roic에 설정된 data읽어옴
* \param		a_waddr : reg address
* \param		a_wdata : reg data
* \note			모든 ROIC Register는 같은 값을 가지고 있다고 간주한다. 만약 다를 경우 Error
***************************************************************************************************/
void ti_roic_read_all( u16 a_waddr,  u16 *a_pdata)
{
	u8 nidx = 0x00;
	u16 n_data = 0x00;
	ti_roic_read_sel(a_waddr, a_pdata, 0x00);

	if(a_waddr == TI_ROIC_REG_33)
	{
		return;
	}

	for( nidx = 0; nidx < g_n_roic_num ; nidx++ )
	{
		ti_roic_read_sel(a_waddr, &n_data, nidx);
		if(n_data != *a_pdata)
		{
				//DBG_PRINT("Inconsistent data read idx0: 0x%04X		idx%d: 0x%04X\n", *a_pdata, nidx, n_data);
				printk("Inconsistent data read addr : 0x%x, idx0: 0x%04X		idx%d: 0x%04X\n", a_waddr, *a_pdata, nidx, n_data);
		}
	}
}

/***********************************************************************************************//**
* \brief		모든 roic에 data저장
* \param		a_waddr : reg address
* \param		a_wdata : reg data
* \return		void
* \note			DONE(V0.0)
***************************************************************************************************/
void ti_roic_write_all(u16 a_waddr, u16 a_wdata)
{
	roic_chip_select_all();
	spi_roic_write((u8)a_waddr, &a_wdata);
	roic_chip_deselect();
	/* roic_chip_select_all() 함수 이용하여 ROIC chip select를 한번에 한 후 동시에 write하도록 함 
	s32 nidx = 0;
	for( nidx = 0 ; nidx < g_n_roic_num ; nidx++)
	{		
		ti_roic_write_sel(a_waddr, a_wdata, nidx);
	}
	*/
	
	if( TI_ROIC_REG_11 == a_waddr)
	{
		u16 w_roic_reg11 = 0x00;
		u16 w_fpga_reg150 = 0x00;
		w_roic_reg11 = tg_reg_read(FPGA_REG_TI_ROIC_CONFIG_11);
		//ti_roic_read_all(TI_ROIC_REG_11, &w_roic_reg11);		
		w_roic_reg11 = (w_roic_reg11 >> 4) & 0x0003;
		
		w_fpga_reg150 = tg_reg_read(FPGA_TI_ROIC_MCLK_SELECT);
		w_fpga_reg150 &= ~(0x3 << 1);
		w_fpga_reg150 |= (w_roic_reg11 << 1);
		tg_reg_write( FPGA_TI_ROIC_MCLK_SELECT, w_fpga_reg150 );	
	}
}

/***********************************************************************************************//**
* \brief		
* \param		
* \param		
* \return		
* \note			
***************************************************************************************************/
void ti_dac_write_sel(u16 i_w_data, u8 idx)
{
	u16 w_dac_data = i_w_data;
	
	dac_chip_select( idx );
	spi_dac_write(&w_dac_data);
	dac_chip_deselect();
}

/***********************************************************************************************//**
* \brief		
* \param		
* \param		
* \return		
* \note			
***************************************************************************************************/
void ti_dac_read_sel(u16* p_data, u8 idx)
{
	u16 w_dac_data = 0;
	
	dac_chip_select( idx );
	spi_dac_read(&w_dac_data);
	dac_chip_deselect();
	
	*p_data = w_dac_data;
}

/***********************************************************************************************//**
* \brief		
* \param		
* \param		
* \return		
* \note			
***************************************************************************************************/
void ti_dac_write_all(u16 data)
{
	u32 nidx = 0;
	for( nidx = 0 ; nidx < g_n_dac_num ; nidx++)
	{		
		ti_dac_write_sel(data, nidx);
	}
}

/***********************************************************************************************//**
* \brief		
* \param		
* \param		
* \return		
* \note			
***************************************************************************************************/
void ti_dac_read_all(u16* p_data)
{
	u32 nidx = 0x00;
	u16 w_data = 0x00;
	
	ti_dac_read_sel(p_data, nidx);
	for( nidx = 1 ; nidx < g_n_dac_num ; nidx++ )
	{
		ti_dac_read_sel(&w_data, nidx);
		if(w_data != *p_data)
		{
				printk("Inconsistent data collected during TI DAC READ ALL\n");
		}
	}
}

/*************************************************************************************************
 * \brief	roic registers 관련 배열에 값을 저장
 * \param	i_idx: timing profile 선택(1: tpa, 2:tpb), i_addr: roic register address, i_data: roic register data
 * \return	None
 * \note	tp관련 regs(0x40~0x55)면 관련 배열(register_tpa, register_tpb)에만 값을 변경, tp 관련 regs가 아니면 register_tp_none값 변경 후에 roic reg write도 진행
 ***************************************************************************************************/
void ti_roic_tp_write(u8 i_idx, u16 i_addr, u16 i_data)
{
	int i;		
	u32 tg_idx = TIROIC_REG_NUM;		
	
	//get tg idx 
	for(i = 0; i < TIROIC_REG_NUM; i++)
	{
		if( register_addr[i] == i_addr )
		{
			tg_idx = i;
			break;
		}
	}
	
	if( TIROIC_REG_NUM == i )
	{
		printk("invalid ti roic register address : 0x%x\n", i_addr);
		return;
	}
			
	if( tg_idx >= TIROIC_REG_TP_START_IDX && tg_idx <= TIROIC_REG_TP_END_IDX)
	{
		tg_idx -= TIROIC_REG_TP_START_IDX;
		
		switch(i_idx)
		{				
			case 0: //tpa
					register_tpa[tg_idx] = i_data;
					break;
					
			case 1: //tpb
					register_tpb[tg_idx] = i_data;
					break;
			default :
					printk("invalid tg register profile index\n");
					break;
		}
	}
	else
	{
		register_tp_none[tg_idx] = i_data;
		ti_roic_write_all(i_addr, i_data);
	}
}

/***********************************************************************************************//**
* \brief		
* \param		
* \param		
* \return		
* \note			
***************************************************************************************************/
void ti_roic_tp_read(u8 i_idx, u16 i_addr, u16* o_p_data)
{
	int i;		
	u32 tg_idx = TIROIC_REG_NUM;		
	
	//get tg idx 
	for(i = 0; i < TIROIC_REG_NUM; i++)
	{
		if( register_addr[i] == i_addr )
		{
			tg_idx = i;
			break;
		}
	}
	
	if( TIROIC_REG_NUM == i )
	{
		printk("invalid ti roic register address : 0x%x\n", i_addr);
		return;
	}
			
	if( tg_idx >= TIROIC_REG_TP_START_IDX && tg_idx <= TIROIC_REG_TP_END_IDX)
	{
		tg_idx -= TIROIC_REG_TP_START_IDX;
		
		switch(i_idx)
		{				
			case 0: //tpa
					*o_p_data = register_tpa[tg_idx];
					break;
					
			case 1: //tpb
					*o_p_data = register_tpb[tg_idx];
					break;
			default :
					printk("invalid tg register profile index\n");
					break;
		}
	}
	else
	{
		ti_roic_read_all(i_addr, o_p_data);
	}
}

/***********************************************************************************************//**
* \brief		ti_roic_selftest
* \param		
* \return		status
* \note
***************************************************************************************************/
bool ti_roic_selftest(void)
{
	// TESTING TIROIC

	u16 addr = 0x40;
	u16 val16 = 0xAAAA;
	u16 rval16 = 0x0000;
	int i = 0;
	bool b_ret = true;
	
	for( i = 0; i < 5; i ++)
	{	
		ti_roic_write_all(addr, val16);
		ti_roic_read_all(addr, &rval16);	
		
		udelay(10);
		
		if(val16 != rval16)
		{			
			b_ret = false;
			printk("TI ROIC Driver Selftest Fail(Roic)\n");
		}
		else
		{
			printk("TI ROIC Driver Selftest success(Roic)\n");
		}
			
		val16++;
	}					
	
	return b_ret;
}

/***********************************************************************************************//**
* \brief		get_active_tp
* \param		
* \return		status
* \note			return 0 : tpa / return 1: tpb / return else : error or some other tg profile 
***************************************************************************************************/
int get_active_tp(void)
{
	int i;
	u16 temp_tp[TIROIC_REG_TG_NUM] = { 0x0000, };
	for( i = 0; i < TIROIC_REG_TG_NUM; i++ )
			ti_roic_read_all(register_addr[TIROIC_REG_TP_START_IDX + i], temp_tp + i);
	
	if( memcmp( temp_tp, register_tpa, TIROIC_REG_TG_NUM) == 0 )
			return 1;
	else if( memcmp( temp_tp, register_tpb, TIROIC_REG_TG_NUM) == 0 )
			return 0;
	else
			return 2;
}
/***********************************************************************************************//**
* \brief		ti_roic_info
* \param		
* \return		status
* \note
***************************************************************************************************/
int ti_roic_info(void)
{
	u16 data = 0x00;
	u16 dac_data = 0x00;
	u16 print_option = 0xFF;
	u16 scan_direction = 0;
	u16 auto_reverse = 0;
	u16 str = 0;
	u16 integration = 0;
	u16 power_mode = 0;
	u16 input_charge_range = 0;
	
	int i = 0;
	
	tg_reg_write(FPGA_ROIC_CLK_EN, 0x01);	

	DBG_PRINT("\n%s : %d\n",__func__, __LINE__);
	
	printk( "-------------------- ROIC Gengeral --------------------\n" );
	printk("MODEL TYPE: TI  AFE2256\n");

	if( print_option & 0x02 )		//print tg profile
	{
			switch(get_active_tp())
			{
			case 0:
					printk("[TIROIC] ACTIVE TG : TPa\n");
					break;
			case 1:
					printk("[TIROIC] ACTIVE TG : TPb\n");
					break;
			default:
					printk("[TIROIC] ACTIVE TG : Unknown\n");
					break;
			}

			printk( "-------------------- TGa Profile  --------------------\n" );		
			for(i = 0; i < TIROIC_REG_TG_NUM; i++)
					printk("%02Xh: 0x%04X\n", register_addr[TIROIC_REG_TP_START_IDX + i], register_tpa[i] );
			
			printk( "-------------------- TGb Profile  --------------------\n" );		
			for(i = 0; i < TIROIC_REG_TG_NUM; i++)
					printk("%02Xh: 0x%04X\n", register_addr[TIROIC_REG_TP_START_IDX + i], register_tpb[i] );				
	}		
	
	if( print_option & 0x04 ) 	//print roic option
	{
			//REVERSE SCAN, AUTO_REVERSE, STR, INTG_MODE, POWER_MODE, INPUT_CHARGE_RANGE 
			ti_roic_read_all( TI_ROIC_REG_11, &data );
			scan_direction = data & 0x8000;
			ti_roic_read_all( TI_ROIC_REG_11, &data );
			auto_reverse = data & 0x2000;
			ti_roic_read_all( TI_ROIC_REG_11, &data );
			str = (data & 0x0030) >> 4;
			ti_roic_read_all( TI_ROIC_REG_5E, &data );
			integration = (data & 0x1000);
			ti_roic_read_all( TI_ROIC_REG_5D, &data );
			power_mode = (data & 0x0002);
			ti_roic_read_all( TI_ROIC_REG_5C, &data );
			input_charge_range = (data & 0xF800) >> 11;
			
			printk( "-------------------- ROIC Option --------------------\n" );
			if(scan_direction)
					printk("SCAN DIRECTION: backward scan\n");
			else
					printk("SCAN DIRECTION: forward scan\n");
			
			if(auto_reverse)
					printk("AUTO REVERSE: auto reverse on\n");
			else
					printk("AUTO REVERSE: auto reverse off\n");
				
			printk("STR VALUE: 0x%04X \n", str);
							
			if(integration)
					printk("INTEGRATION UP/DOWN: integration down\n");
			else
					printk("INTEGRATION UP/DOWN: integration up\n");			
			
			if(power_mode)
					printk("POWER MODE: low-noise mode\n");
			else
					printk("POWER MODE: normal-power mode\n");
	
			printk("INPUT CHARGE RANGE: 0x%04X\n", input_charge_range);
	}
	
	if( print_option & 0x01 ) 	//print raw 
	{
			printk( "-------------------- Raw Data --------------------\n" );
			for( i = 1; i < TIROIC_REG_NUM; i++)
			{
					ti_roic_read_all( register_addr[i], &data ); 
					printk("%02Xh: 0x%04X\n", register_addr[i], data );
			}
	}
	
	printk( "-------------------- DAC Data --------------------\n" );
	ti_dac_read_all(&dac_data);
	printk("DAC0: 0x%04X\n", dac_data );
	printk("DAC1: 0x%04X\n", dac_data );
	printk( "-------------------- Done --------------------\n" );

	tg_reg_write(FPGA_ROIC_CLK_EN, 0x00);	

	return 0;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
 *******************************************************************************/
 void ti_roic_delay_compensation(void)
{
	//////////////////////		
	//Delay Compensation//
	//////////////////////
	
	s32	n_status = 0;
	u16 config_mode = 0;
	u16 rval_top = 0;
	u16 rval_bottom = 0;
	int counter = 0;
	int	i = 0;
	 
	u32 transit_delay_reg_addr = 0;
	u16 transit_delay_reg_top_start = 0;
	u16 transit_delay_reg_top_end = 0;	
	u16 transit_delay_reg_bottom_start = 0;
	u16 transit_delay_reg_bottom_end = 0;
		
	u8 delay_candidate[32];
	u8 delay_candidate_min_idx = 0x00;
	u8 delay_candidate_max_idx = 0x00;
	u8 delay_candidate_min_candidate_idx = 0x00;
	u8 delay_candidate_cur_val = 0x00;
	u8 delay_candidate_prev_val = 0x00;	
			
	u16 delay_top = 0x0000;
	u16 delay_bottom = 0x0000;
	
	u32 channel = 0;	
	
	u8 t1 = 0x00;
	u8 t2 = 0x00;
	u8 t3 = 0x00;
	u8 t4 = 0x00;
	u8 t5 = 0x00;
	u8 t6 = 0x00;
	
	transit_delay_reg_top_start = FPGA_TI_ROIC_AUTO_CAL_CLK_DLY_U_TST_0_LSB;
	transit_delay_reg_top_end = transit_delay_reg_top_start +  g_n_upper_side_roic_num * 2 - 1 ;	
	transit_delay_reg_bottom_start = FPGA_TI_ROIC_AUTO_CAL_CLK_DLY_L_TST_0_LSB;
	transit_delay_reg_bottom_end = transit_delay_reg_bottom_start +  g_n_lower_side_roic_num * 2 - 1 ;
		
	
	memset(delay_candidate, 0x01, sizeof(delay_candidate));
	
	//20181204. youngjun han. Clock Serdes Delay Compensation 	
	//roic test pattern “aaaaaa”
	config_mode = tg_reg_read(FPGA_REG_TI_ROIC_CONFIG_10);
	//ti_roic_read_all(0x10, &config_mode);
	config_mode &= 0x800;
	ti_roic_write_all( 0x10, config_mode | 0x340 );

	ti_roic_write_all( 0x04, 0xaaaa );
	ti_roic_write_all( 0x05, 0xaaaa );
	ti_roic_write_all( 0x06, 0xaaaa );
	ti_roic_write_all( 0x07, 0xaaaa );
	ti_roic_write_all( 0x08, 0xaaaa );
	ti_roic_write_all( 0x09, 0xaaaa );
	
	//[20190328][yongjin] data delay compenstation data 초기화 
	for(i = 0; i < g_n_upper_side_roic_num; i++)
	{
		tg_reg_write( FPGA_TI_ROIC_MANUAL_DATA_DLY_U, i );				
		tg_reg_write( FPGA_TI_ROIC_MANUAL_DATA_DLY_L, i );
	}	
	
	tg_reg_write( FPGA_TI_ROIC_TOP_DELAY_COMPENSATION_MODE, 0x0003 );					// top auto clk delay compensation
	tg_reg_write( FPGA_TI_ROIC_BTM_DELAY_COMPENSATION_MODE, 0x0003 );					// bottom auto clk delay compensation
	tg_reg_write( FPGA_TI_ROIC_DELAY_COMPENSATION_EN, 0x0008 );							// auto delay compensation clear bit set
	udelay(10);
	tg_reg_write( FPGA_TI_ROIC_DELAY_COMPENSATION_EN, 0x0000 );							// auto delay compensation clear bit unset
	tg_reg_write( FPGA_TI_ROIC_DELAY_COMPENSATION_EN, 0x0001 );							// auto delay compensation enable
		
	do
	{			
		if( counter == 10 )    
		{			
			DBG_PRINT("Clock Delay Compensation FAIL(timeout)\n");
			n_status = 1;
			break;
		}
		
		rval_top = tg_reg_read( FPGA_TI_ROIC_AUTO_CAL_DLY_STATUS_U_LSB );					
		rval_bottom = tg_reg_read( FPGA_TI_ROIC_AUTO_CAL_DLY_STATUS_L_LSB );			
		
		if( 0x01 == rval_top && 0x01 == rval_bottom )
		{
			DBG_PRINT("Clock Delay Compensation Done.\n");
			break;
		}
		
		
		mdelay(2);	//2ms
		counter++;
	} while ( counter < 10 );
					
	//Getting clk_delay_top
	for(i = transit_delay_reg_top_start; i <= transit_delay_reg_top_end; i++)
	{
			rval_top = tg_reg_read( i );
			t1 = rval_top & 0x1F;
			t2 = (rval_top>>5) & 0x1F;
			t3 = (rval_top>>10) & 0x1F;
			//DBG_PRINT(0, "transit_delay_reg 0x%04X: 0x%04X 0x%04X 0x%04X 0x%04X \n", i, rval_top, t1, t2, t3);
			
			if(t1 != 0x1F )
					delay_candidate[t1] = 0x00;
			if(t2 != 0x1F )
					delay_candidate[t2] = 0x00;
			if(t3 != 0x1F )
					delay_candidate[t3] = 0x00;
	}

	//for(i = 0; i < 32; i++)
	//{
	//		DBG_PRINT(0,"clk_delay_candidate_top idx 0x%04X: 0x%04X\n", i, delay_candidate[i]);
	//}
	
	for(i = 0; i < 32; i++)
	{
			delay_candidate_cur_val = delay_candidate[i];
			
			if( delay_candidate_cur_val == 0x0001 && delay_candidate_prev_val == 0x0000 )
			{
				delay_candidate_min_candidate_idx = i;
			}
			else if( delay_candidate_cur_val == 0x0000 && delay_candidate_prev_val == 0x0001 )
			{
					if( (delay_candidate_max_idx - delay_candidate_min_idx) < (i - delay_candidate_min_candidate_idx) )
					{
						delay_candidate_min_idx = delay_candidate_min_candidate_idx;
						delay_candidate_max_idx = i;
					}
			}
			delay_candidate_prev_val = delay_candidate_cur_val;		
	}
	
	//IF range x ~ 32 is the longest range
	if( delay_candidate[31] == 0x0001 )
	{
			if( (delay_candidate_max_idx - delay_candidate_min_idx) < (31 - delay_candidate_min_candidate_idx) )
			{
				delay_candidate_min_idx = delay_candidate_min_candidate_idx;
				delay_candidate_max_idx = 31;
			}
	}
	
	DBG_PRINT("clk_delay_top range :  0x%04X ~ 0x%04X\n", delay_candidate_min_idx, delay_candidate_max_idx);
	delay_top = ((delay_candidate_max_idx - delay_candidate_min_idx) / 2) + delay_candidate_min_idx;
	
					
	//Getting clk_delay_bottom
	memset(delay_candidate, 0x0001, sizeof(delay_candidate));
	delay_candidate_min_idx = 0x0000;
	delay_candidate_max_idx = 0x0000;
	delay_candidate_min_candidate_idx = 0x0000;
	delay_candidate_cur_val = 0x0000;
	delay_candidate_prev_val = 0x0000;
			
	for(i = transit_delay_reg_bottom_start; i <= transit_delay_reg_bottom_end; i++)
	{
			rval_bottom = tg_reg_read( i );
			t1 = rval_bottom & 0x1F;
			t2 = (rval_bottom>>5) & 0x1F;
			t3 = (rval_bottom>>10) & 0x1F;
			//DBG_PRINT(0, "transit_delay_reg 0x%04X: 0x%04X 0x%04X 0x%04X 0x%04X \n", i, rval_top, t1, t2, t3);
			
			if(t1 != 0x1F )
					delay_candidate[t1] = 0x00;
			if(t2 != 0x1F )
					delay_candidate[t2] = 0x00;
			if(t3 != 0x1F )
					delay_candidate[t3] = 0x00;
	}
	
	//for(i = 0; i < 32; i++)
	//{
	//		DBG_PRINT(0,"clk_delay_candidate_bottom idx 0x%04X: 0x%04X\n", i, delay_candidate[i]);
	//}
	
	for(i = 0; i < 32; i++)
	{
			delay_candidate_cur_val = delay_candidate[i];
			
			if( delay_candidate_cur_val == 0x0001 && delay_candidate_prev_val == 0x0000 )
			{
						delay_candidate_min_candidate_idx = i;
			}
			else if( delay_candidate_cur_val == 0x0000 && delay_candidate_prev_val == 0x0001 )
			{
					if( (delay_candidate_max_idx - delay_candidate_min_idx) < (i - delay_candidate_min_candidate_idx) )
					{
						delay_candidate_min_idx = delay_candidate_min_candidate_idx;
						delay_candidate_max_idx = i;
					}
			}
			delay_candidate_prev_val = delay_candidate_cur_val;		
	}
	
	//IF range x ~ 32 is the longest range
	if( delay_candidate[31] == 0x0001 )
	{
			if( (delay_candidate_max_idx - delay_candidate_min_idx) < (31 - delay_candidate_min_candidate_idx) )
			{
				delay_candidate_min_idx = delay_candidate_min_candidate_idx;
				delay_candidate_max_idx = 31;
			}
	}
	
	delay_bottom = ((delay_candidate_max_idx - delay_candidate_min_idx) / 2) + delay_candidate_min_idx;
	DBG_PRINT("clk_delay_bottom range :  0x%04X ~ 0x%04X\n", delay_candidate_min_idx, delay_candidate_max_idx);		
	
//CLK_DELAY_COMPENSATION_END:
	
	//Setting clk_delay_top & clk_delay_bottom
	DBG_PRINT("clk_delay_top 0x%04X clk_delay_bottom 0x%04X\n", delay_top, delay_bottom);
	
	tg_reg_write( FPGA_TI_ROIC_DELAY_COMPENSATION_EN, 0x0000 );					// auto delay compensation disable
	tg_reg_write( FPGA_TI_ROIC_TOP_DELAY_COMPENSATION_MODE, 0x0000 );				// top register delay 설정
	tg_reg_write( FPGA_TI_ROIC_BTM_DELAY_COMPENSATION_MODE, 0x0000 );				// bottom register delay 설정

	tg_reg_write( FPGA_TI_ROIC_MANUAL_CLK_DLY_U, delay_top );			// top register delay 설정
	tg_reg_write( FPGA_TI_ROIC_MANUAL_CLK_DLY_L, delay_bottom );			// bottom register delay 설정
					
	//20181204. youngjun han. Data Serdes Delay Compensation
		
	transit_delay_reg_top_start = FPGA_TI_ROIC_AUTO_CAL_DATA_DLY_U_TST_0_LSB;
	transit_delay_reg_top_end = transit_delay_reg_top_start +  g_n_upper_side_roic_num * 2 - 1;
	
	transit_delay_reg_bottom_start = FPGA_TI_ROIC_AUTO_CAL_DATA_DLY_L_TST_0_LSB;
	transit_delay_reg_bottom_end = transit_delay_reg_bottom_start +  g_n_lower_side_roic_num * 2 - 1;
	
	tg_reg_write( FPGA_TI_ROIC_TOP_DELAY_COMPENSATION_MODE, 0x0005 );					// top auto clk delay compensation
	tg_reg_write( FPGA_TI_ROIC_BTM_DELAY_COMPENSATION_MODE, 0x0005 );					// bottom auto clk delay compensation
		
	tg_reg_write( FPGA_TI_ROIC_DELAY_COMPENSATION_EN, 0x0008 );							// auto delay compensation clear bit set
	udelay(10);
	tg_reg_write( FPGA_TI_ROIC_DELAY_COMPENSATION_EN, 0x0000 );							// auto delay compensation clear bit unset
	tg_reg_write( FPGA_TI_ROIC_DELAY_COMPENSATION_EN, 0x0001 );							// auto delay compensation enable
	
	counter = 0;
	do
	{		
		if( counter == 10 )    
		{			
			DBG_PRINT("Data Delay Compensation FAIL(timeout)\n");
			n_status = 1;
			break;
		}
		
		rval_top = tg_reg_read( FPGA_TI_ROIC_AUTO_CAL_DLY_STATUS_U_LSB );				
		rval_bottom = tg_reg_read( FPGA_TI_ROIC_AUTO_CAL_DLY_STATUS_L_LSB );					
				  
		if( 0x02 == rval_top && 0x02 == rval_bottom)
		{
			DBG_PRINT("Data Delay Compensation Done.\n");
			break;
		}
		  
		mdelay(2);	//2ms
		counter++;
	} while ( counter < 10 );	
	
	for(transit_delay_reg_addr = transit_delay_reg_top_start, channel = 0; transit_delay_reg_addr <= transit_delay_reg_top_end; transit_delay_reg_addr+=2, channel++)
	{					
		//Getting clk_delay_bottom
		memset(delay_candidate, 0x0001, sizeof(delay_candidate));
		delay_candidate_min_idx = 0x0000;
		delay_candidate_max_idx = 0x0000;
		delay_candidate_min_candidate_idx = 0x0000;
		delay_candidate_cur_val = 0x0000;
		delay_candidate_prev_val = 0x0000;
		
		rval_top = tg_reg_read( transit_delay_reg_addr );
		t1 = rval_top & 0x1F;
		t2 = (rval_top>>5) & 0x1F;
		t3 = (rval_top>>10) & 0x1F;
		
		rval_top = tg_reg_read( transit_delay_reg_addr + 1 );
		t4 = rval_top & 0x1F;
		t5 = (rval_top>>5) & 0x1F;
		t6 = (rval_top>>10) & 0x1F;
		
		//DBG_PRINT(0, "transit_delay_reg 0x%04X: 0x%04X 0x%04X 0x%04X 0x%04X 0x%04X 0x%04X 0x%04X\n", transit_delay_reg_addr, rval_top, t1, t2, t3, t4, t5, t6);
		
		if(t1 != 0x1F )
			delay_candidate[t1] = 0x00;
		if(t2 != 0x1F )
			delay_candidate[t2] = 0x00;
		if(t3 != 0x1F )
			delay_candidate[t3] = 0x00;		
		if(t4 != 0x1F )
			delay_candidate[t4] = 0x00;
		if(t5 != 0x1F )
			delay_candidate[t5] = 0x00;
		if(t6 != 0x1F )
			delay_candidate[t6] = 0x00;		
					
		//for(i = 0; i < 32; i++)
		//{
		//	DBG_PRINT(0, "delay_candidate_top idx 0x%04X: 0x%04X\n", i, delay_candidate[i]);
		//}
			
		for(i = 0; i < 32; i++)
		{
			delay_candidate_cur_val = delay_candidate[i];
			
			if( delay_candidate_cur_val == 0x0001 && delay_candidate_prev_val == 0x0000 )
			{
				delay_candidate_min_candidate_idx = i;
			}
			else if( delay_candidate_cur_val == 0x0000 && delay_candidate_prev_val == 0x0001 )
			{
				if( (delay_candidate_max_idx - delay_candidate_min_idx) < (i - delay_candidate_min_candidate_idx) )
				{
					delay_candidate_min_idx = delay_candidate_min_candidate_idx;
					delay_candidate_max_idx = i;
				}
			}
			delay_candidate_prev_val = delay_candidate_cur_val;		
		}
			
		//IF range x ~ 32 is the longest range
		if( delay_candidate[31] == 0x0001 )
		{
			if( (delay_candidate_max_idx - delay_candidate_min_idx) < (31 - delay_candidate_min_candidate_idx) )
			{
				delay_candidate_min_idx = delay_candidate_min_candidate_idx;
				delay_candidate_max_idx = 31;
			}
		}
			
		delay_top = ((delay_candidate_max_idx - delay_candidate_min_idx) / 2) + delay_candidate_min_idx;
				
		DBG_PRINT("Channel : %d, data_delay_top 0x%04X(0x%04X ~ 0x%04X)\n",channel,  delay_top, delay_candidate_min_idx, delay_candidate_max_idx);								
		
		tg_reg_write( FPGA_TI_ROIC_MANUAL_DATA_DLY_U, channel | (delay_top << 8)  );
				
		tg_reg_write( FPGA_TI_ROIC_DELAY_COMPENSATION_EN, 0x0003 );					// auto delay compensation enable & Top Data Delay Set
		tg_reg_write( FPGA_TI_ROIC_DELAY_COMPENSATION_EN, 0x0001 );					// auto delay compensation enable
	
	}	
	
	for(transit_delay_reg_addr = transit_delay_reg_bottom_start, channel = 0; transit_delay_reg_addr <= transit_delay_reg_bottom_end; transit_delay_reg_addr+=2, channel++)
	{
		//Getting clk_delay_bottom
		memset(delay_candidate, 0x0001, sizeof(delay_candidate));
		delay_candidate_min_idx = 0x0000;
		delay_candidate_max_idx = 0x0000;
		delay_candidate_min_candidate_idx = 0x0000;
		delay_candidate_cur_val = 0x0000;
		delay_candidate_prev_val = 0x0000;
		
		rval_bottom = tg_reg_read( transit_delay_reg_addr );
		t1 = rval_bottom & 0x1F;
		t2 = (rval_bottom>>5) & 0x1F;
		t3 = (rval_bottom>>10) & 0x1F;
		
		rval_bottom = tg_reg_read( transit_delay_reg_addr + 1 );
		t4 = rval_bottom & 0x1F;
		t5 = (rval_bottom>>5) & 0x1F;
		t6 = (rval_bottom>>10) & 0x1F;
		
		//DBG_PRINT(0, "transit_delay_reg 0x%04X: 0x%04X 0x%04X 0x%04X 0x%04X 0x%04X 0x%04X 0x%04X\n", transit_delay_reg_addr, rval_top, t1, t2, t3, t4, t5, t6);
		
		if(t1 != 0x1F )
			delay_candidate[t1] = 0x00;
		if(t2 != 0x1F )
			delay_candidate[t2] = 0x00;
		if(t3 != 0x1F )
			delay_candidate[t3] = 0x00;		
		if(t4 != 0x1F )
			delay_candidate[t4] = 0x00;
		if(t5 != 0x1F )
			delay_candidate[t5] = 0x00;
		if(t6 != 0x1F )
			delay_candidate[t6] = 0x00;		
					
		//for(i = 0; i < 32; i++)
		//{
		//	DBG_PRINT(0, "delay_candidate_bottom idx 0x%04X: 0x%04X\n", i, delay_candidate[i]);
		//}
			
		for(i = 0; i < 32; i++)
		{
			delay_candidate_cur_val = delay_candidate[i];
			
			if( delay_candidate_cur_val == 0x0001 && delay_candidate_prev_val == 0x0000 )
			{
				delay_candidate_min_candidate_idx = i;
			}
			else if( delay_candidate_cur_val == 0x0000 && delay_candidate_prev_val == 0x0001 )
			{
				if( (delay_candidate_max_idx - delay_candidate_min_idx) < (i - delay_candidate_min_candidate_idx) )
				{
					delay_candidate_min_idx = delay_candidate_min_candidate_idx;
					delay_candidate_max_idx = i;
				}
			}
			delay_candidate_prev_val = delay_candidate_cur_val;		
		}
			
		//IF range x ~ 32 is the longest range
		if( delay_candidate[31] == 0x0001 )
		{
			if( (delay_candidate_max_idx - delay_candidate_min_idx) < (31 - delay_candidate_min_candidate_idx) )
			{
				delay_candidate_min_idx = delay_candidate_min_candidate_idx;
				delay_candidate_max_idx = 31;
			}
		}
			
		delay_bottom = ((delay_candidate_max_idx - delay_candidate_min_idx) / 2) + delay_candidate_min_idx;
				
		DBG_PRINT("Channel : %d, data_delay_bottom 0x%04X(0x%04X ~ 0x%04X)\n",channel,  delay_bottom, delay_candidate_min_idx, delay_candidate_max_idx);				
		
		tg_reg_write( FPGA_TI_ROIC_MANUAL_DATA_DLY_L, channel | (delay_bottom << 8)  );
				
		tg_reg_write( FPGA_TI_ROIC_DELAY_COMPENSATION_EN, 0x0005 );					// auto delay compensation enable & Bottom Data Delay Set
		tg_reg_write( FPGA_TI_ROIC_DELAY_COMPENSATION_EN, 0x0001 );					// auto delay compensation enable
	
	}
	
//DATA_DELAY_COMPENSATION_END:	
	
	tg_reg_write( FPGA_TI_ROIC_DELAY_COMPENSATION_EN, 0x0000 );					// auto delay compensation disable
	tg_reg_write( FPGA_TI_ROIC_TOP_DELAY_COMPENSATION_MODE, 0x0000 );				// top register delay 설정
	tg_reg_write( FPGA_TI_ROIC_BTM_DELAY_COMPENSATION_MODE, 0x0000 );				// bottom register delay 설정
	
	config_mode = tg_reg_read(FPGA_REG_TI_ROIC_CONFIG_10);
	ti_roic_write_all( 0x10, config_mode );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
 *******************************************************************************/
 s32 ti_roic_bit_alignment(void)
{
	/////////////////				
	//BIT Alignment//
	/////////////////
	
	// bit alignment start
	s32	n_status = 0;
	int counter = 0;		
	u16 reg10h_val = 0x0000;
	u16 rval_top = 0;
	u16 rval_bottom = 0;
	u16 sval_upper = 0;
	u16 sval_lower = 0;
	u32	roic_index = 0;		
	bool timeout = false;
		
	sval_upper = sval_upper | (0x01 << ROIC_UPPER_SIDE_NUM_MAX); /* [12] : All upper side Serdes Channel Bit Align Done */
	sval_lower = sval_lower | (0x01 << ROIC_LOWER_SIDE_NUM_MAX); /* [12] : All lower side Serdes Channel Bit Align Done */
	for(roic_index = 0; roic_index < g_n_upper_side_roic_num; roic_index++)
	{
		sval_upper = 	sval_upper | (0x01 << roic_index);
	}
	
	for(roic_index = 0; roic_index < g_n_lower_side_roic_num; roic_index++)
	{
		sval_lower = 	sval_lower | (0x01 << roic_index);
	}	
	
	reg10h_val = tg_reg_read(FPGA_REG_TI_ROIC_CONFIG_10);
	//ti_roic_read_all(TI_ROIC_REG_10, &reg10h_val);
	reg10h_val |=	0x3C0; 		// 0x3C0: Sync/deskew test pattern value for reg 10h	
	ti_roic_write_all( TI_ROIC_REG_10, reg10h_val );
	

	tg_reg_write( FPGA_TI_ROIC_ALIGN_MODE, 0x4 );					// bit alignment clear bit set
	udelay(10);
	
	rval_top = tg_reg_read( FPGA_TI_ROIC_AUTO_CAL_DLY_STATUS_U_MSB );	
	DBG_PRINT("[TIROIC] align clear: 0x%04X val: 0x%04X\n", FPGA_TI_ROIC_AUTO_CAL_DLY_STATUS_U_MSB, rval_top);
	rval_bottom = tg_reg_read( FPGA_TI_ROIC_AUTO_CAL_DLY_STATUS_L_MSB );			
	DBG_PRINT("[TIROIC] align clear: 0x%04X val: 0x%04X\n", FPGA_TI_ROIC_AUTO_CAL_DLY_STATUS_L_MSB, rval_bottom);				
	
	tg_reg_write( FPGA_TI_ROIC_ALIGN_MODE, 0x0 );					// bit alignment clear bit unset
		
	tg_reg_write( FPGA_TI_ROIC_ALIGN_MODE, 0x1 );					// bit alignment enable start	

	counter = 0;
	do
	{
		rval_top = tg_reg_read( FPGA_TI_ROIC_AUTO_CAL_DLY_STATUS_U_MSB );	
		DBG_PRINT("[TIROIC]  Bitalign reg: 0x%04X val: 0x%04X\n", FPGA_TI_ROIC_AUTO_CAL_DLY_STATUS_U_MSB, rval_top);
		rval_bottom = tg_reg_read( FPGA_TI_ROIC_AUTO_CAL_DLY_STATUS_L_MSB );			
		DBG_PRINT("[TIROIC]  Bitalign reg: 0x%04X val: 0x%04X\n", FPGA_TI_ROIC_AUTO_CAL_DLY_STATUS_L_MSB, rval_bottom);				
		  
		if( sval_upper == rval_top && sval_lower == rval_bottom)
		{
			break;
		}
		
		counter ++;
		if( counter == 5 )    
		{			
			DBG_PRINT("[TIROIC]  bit alignment FAIL(timeout)\n");		 
			timeout = true;
			break;
		}	
		
		mdelay(2);
		  
	} while ( counter < 5 );	//while ( rval != 0x0005 );
	
	tg_reg_write( FPGA_TI_ROIC_ALIGN_MODE, 0x0 );					// bit alignment enable end	
	
	reg10h_val = 0x0000; 
	reg10h_val = tg_reg_read(FPGA_REG_TI_ROIC_CONFIG_10);
	//ti_roic_read_all(TI_ROIC_REG_10, &reg10h_val);
	ti_roic_write_all( TI_ROIC_REG_10, reg10h_val );		// test pattern disabled(image generation)
		
	rval_top = tg_reg_read( FPGA_TI_ROIC_AUTO_CAL_DLY_STATUS_U_MSB );	
	rval_bottom = tg_reg_read( FPGA_TI_ROIC_AUTO_CAL_DLY_STATUS_L_MSB );			
	if( sval_upper != rval_top || sval_lower != rval_bottom )
	{			
		n_status = -1;
	}
	
	DBG_PRINT("[TIROIC]  Bitalign reg: 0x%04X val: 0x%04X, reg: 0x%04X val : 0x%04X\n", FPGA_TI_ROIC_AUTO_CAL_DLY_STATUS_U_MSB, rval_top, FPGA_TI_ROIC_AUTO_CAL_DLY_STATUS_L_MSB, rval_bottom);	
	
	return n_status;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
 *******************************************************************************/
void ti_roic_bit_align_test(u16 w_retry_num)
{	
	u16 w_retry_cnt = 0;	
	
	g_b_log_print = true;
	
	do{
		ti_roic_delay_compensation();	
	}while(ti_roic_bit_alignment() < 0 && ++w_retry_cnt < w_retry_num);
	
	g_b_log_print = false;
}	

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
 *******************************************************************************/
u16 ti_roic_initialize_inner(void)
{	
		u32 idx = 0;
		
		DBG_PRINT("\n%s : %d\n",__func__, __LINE__);	
				
		//Initialize	
		//1.1 resetting 
		ti_roic_write_all(TI_ROIC_REG_00, 0x01);	
		udelay(1); // wait for 1 us 	//min 200ns
		
		//1.2 Skip quick wakeup mode
		//1.3 Set TRIM_LOAD (This loads the factory-set trim value to the #sel ROIC)
		ti_roic_write_all(TI_ROIC_REG_30, 0x2);
		udelay(10);					
		//ROIC_TPSEL_HIGH;			//no need in future
		tg_reg_write( FPGA_TI_ROIC_SYNC_TPSEL_SET, 0x9 );      // sync mask--> sync output disable & TPSEL_HIGH
		udelay(100);										   // wait for 100 us
		ti_roic_write_all(TI_ROIC_REG_30, 0x0);

		// writing registers
		// 1.4 Others	- Integrate-Up(0)/Down(1), Auto-reserve Scan disabled(0)/enabled(1),
		//				- Default Power mode(0)/Low-noise mode(1), forward scan(0)/reverse scan(1)
		for( idx = 1; idx < TIROIC_REG_NUM; idx++)
		{
			if(	idx >= TIROIC_REG_TP_START_IDX && idx <= TIROIC_REG_TP_END_IDX)
				continue;
			
			ti_roic_write_all(register_addr[idx], register_tp_none[idx]); 
		}
		
		return 0;
}

/***********************************************************************************************//**
* \brief		개별 roic에 설정된 data읽어옴
* \param		a_waddr : reg address
* \param		a_wdata : reg data
* \note			미사용. 추후 안정화 된 후 구현.
***************************************************************************************************/
void ti_roic_reset(void)
{

}

/***********************************************************************************************//**
* \brief		essential bit 설정
* \param		void
* \return		void
* \note			
***************************************************************************************************/
void ti_roic_setup_essential_bit(void)
{
	u16 w_data = 0;
	u16 w_intg_mode = 0;
	u16 w_power_mode = 0;
	
	w_data = tg_reg_read(FPGA_REG_TI_ROIC_CONFIG_5E);
	//ti_roic_read_all( TI_ROIC_REG_5E, &w_data );
	w_intg_mode = ((w_data >> 12) & 0x0001); // 0 : up, 1 : down
	
	w_data = tg_reg_read(FPGA_REG_TI_ROIC_CONFIG_5D);
	//ti_roic_read_all( TI_ROIC_REG_5D, &w_data );
	w_power_mode = ((w_data >> 1) & 0x0001); // 0 : normal power, 1 : low noise
	
	// essential bit 1, register 0x11 bit 10
	// must write 1
	w_data = tg_reg_read(FPGA_REG_TI_ROIC_CONFIG_11);
	//ti_roic_read_all(TI_ROIC_REG_11, &w_data);
	w_data |= (0x1 << 10);
	ti_roic_write_all(TI_ROIC_REG_11, w_data);

#if 0
/* input charge range 변경하면 ROIC 단위 level 차이 발생하는 현상 수정을 위해 아래 부분 삭제
   TI 문의 결과 Input charge range 변경하면 ROIC 별로 capacitor 용량 편차가 발생하여 gain 편차가 발생
   따라서 input charge range 별로 gain cal data를 따로 가져가야 하는 것이 권장되나, 
   Register 12h, bit 14 = 0 / Register 2Ch, bit 0 = 1 로 고정해서 설정하면 이 부분이 보완될 수 있음
   단, dynamic range에 손해를 볼 수 있음, 시료 수 늘려서 테스트 필요 */
	// essential bit 2, register 0x12 bit 14
	// if integrate-down, set 0
	// else if integrate-up, set 1	
	w_data = tg_reg_read(FPGA_REG_TI_ROIC_CONFIG_12);
	//ti_roic_read_all(TI_ROIC_REG_12, &w_data);
	w_data &= ~(0x1 << 14);
	w_data |= ((!w_intg_mode) << 14);
	ti_roic_write_all(TI_ROIC_REG_12, w_data);
#endif

	// essential bit 3, register 0x18 bit 0
	// must write 1
	w_data = tg_reg_read(FPGA_REG_TI_ROIC_CONFIG_18);
	//ti_roic_read_all(TI_ROIC_REG_18, &w_data);	
	w_data |= (0x1 << 0);
	ti_roic_write_all(TI_ROIC_REG_18, w_data);
	
	// essential bit 4, register 0x61 bit 14
	// must write 1
	w_data = tg_reg_read(FPGA_REG_TI_ROIC_CONFIG_61);
	//ti_roic_read_all(TI_ROIC_REG_61, &w_data);
	w_data |= (0x1 << 14);
	ti_roic_write_all(TI_ROIC_REG_61, w_data);
	
	// essential bit 5, 6, 7
	w_data = tg_reg_read(FPGA_REG_TI_ROIC_CONFIG_16);
	//ti_roic_read_all(TI_ROIC_REG_16, &w_data);
	// essential bit 5, register 0x16 bit 6 ~ 7
	// must write 0x11	
	w_data |= (0x11 << 6);
	// essential bit 6, register 0x16 bit 3
	// if normal power, set 0
	// else if low-noise, set 1
#if 0
	w_data &= ~(0x1 << 3);
	w_data |= (w_power_mode << 3);
#endif
	// essential bit 7, register 0x12 bit 8 ~ 15
	// if integrate-down, set 0xCA
	// else if integrate-up, set 0	
	w_data &= ~(0xFF << 8);
	w_intg_mode == 1 ? (w_data |= (0xCA << 8)) : (w_data &= (0x00FF));
	ti_roic_write_all(TI_ROIC_REG_16, w_data);

#if 0
/* input charge range 변경하면 ROIC 단위 level 차이 발생하는 현상 수정을 위해 아래 부분 삭제
   TI 문의 결과 Input charge range 변경하면 ROIC 별로 capacitor 용량 편차가 발생하여 gain 편차가 발생
   따라서 input charge range 별로 gain cal data를 따로 가져가야 하는 것이 권장되나, 
   Register 12h, bit 14 = 0 / Register 2Ch, bit 0 = 1 로 고정해서 설정하면 이 부분이 보완될 수 있음
   단, dynamic range에 손해를 볼 수 있음, 시료 수 늘려서 테스트 필요 */
	// essential bit 8, register 0x2C bit 0
	// if integrate-down, set 1
	// else if integrate-up, set 0	
	w_data = tg_reg_read(FPGA_REG_TI_ROIC_CONFIG_2C);
	//ti_roic_read_all(TI_ROIC_REG_2C, &w_data);
	w_data &= ~(0x1 << 0);
	w_data |= (w_intg_mode << 0);
	ti_roic_write_all(TI_ROIC_REG_2C, w_data);
#endif
}

/***********************************************************************************************//**
* \brief		restore_prev_config
* \param
* \return
* \note			ROIC test pattern, isolate panel 설정 사용을 위한 함수
				roic power down 과정에서 변경된 register값을 원래 설정값으로 복원한다.
***************************************************************************************************/
u16 get_tft_aed_option(void)
{
	u16 w_data = tg_reg_read(FPGA_REG_TFT_AED_OPTION);
	if(w_data == 0xDEAD)
		w_data = 0;
	
	return w_data;
}

/***********************************************************************************************//**
* \brief		ti_roic_sync
* \param
* \return
* \note			when tpa & tpb is reprogrammed apply them and reperform allignment
***************************************************************************************************/
int ti_roic_sync(void)
{
		int i = 0;
		int retry_cnt = 0;
		int n_result = 0;
		u16 w_tft_aed_opt = get_tft_aed_option();
		u8 c_roic_idx = 0;
		u16 w_roic_sel_l = tg_reg_read(FPGA_REG_TFT_AED_ROIC_SEL_L);
		
		//stop continuous sync 
		//writing tg profile reg for TGa
		
		tg_reg_write( FPGA_TI_ROIC_SYNC_TPSEL_SET, 0x9 );      // sync mask & TP_HIGH
		tg_reg_write( FPGA_TI_ROIC_SYNC_TPSEL_SET, 0x8 );      // sync unmask & TP_HIGH	
		//tg_reg_write( 0x152, 0x3 );      // sync mask & synctpsel_mode
		//tg_reg_write( 0x152, 0x2 );      // sync unmask & synctpsel_mode	
		
		tg_reg_write( FPGA_TI_ROIC_SYNC_GEN_EN, 0x1 );	//SYNC ONOFF. 주소값: 0x0181 
		tg_reg_write( FPGA_TI_ROIC_SYNC_GEN_EN, 0x0 );	
		
		for(i = 0; i < TIROIC_REG_TG_NUM; i ++)
		{
			ti_roic_write_all( register_addr[TIROIC_REG_TP_START_IDX+i], register_tpa[i] );				
		}

		//programming TGb
		
		tg_reg_write( FPGA_TI_ROIC_SYNC_TPSEL_SET, 0x1 );      // sync mask & TPSEL_LOW
		tg_reg_write( FPGA_TI_ROIC_SYNC_TPSEL_SET, 0x0 );      // sync unmask & TPSEL_LOW	
		//tg_reg_write( 0x152, 0x3 );      // sync mask & synctpsel_mode
		//tg_reg_write( 0x152, 0x2 );      // sync unmask & synctpsel_mode	
		
		tg_reg_write( FPGA_TI_ROIC_SYNC_GEN_EN, 0x1 );	//SYNC ONOFF. 주소값: 0x0181 
		tg_reg_write( FPGA_TI_ROIC_SYNC_GEN_EN, 0x0 );	
	
		if(w_tft_aed_opt & TFT_AED_MODE_ENABLE && tg_reg_read(FPGA_SEL_TRIGGER) == eTRIG_SELECT_NON_LINE )
		{
			for(c_roic_idx = 0; c_roic_idx < g_n_roic_num; c_roic_idx++)
			{
				if( w_roic_sel_l & (0x01 << c_roic_idx) )
				{
					for(i = 0; i < TIROIC_REG_TG_NUM; i ++)
					{
						ti_roic_write_sel( register_addr[TIROIC_REG_TP_START_IDX+i], register_tpa[i], c_roic_idx );
					}
				}
				else
				{
					for(i = 0; i < TIROIC_REG_TG_NUM; i ++)
					{
						ti_roic_write_sel( register_addr[TIROIC_REG_TP_START_IDX+i], register_tpb[i], c_roic_idx );
					}
				}							
			}
			
		}
		else
		{
			for(i = 0; i < TIROIC_REG_TG_NUM; i ++)
			{
				ti_roic_write_all( register_addr[TIROIC_REG_TP_START_IDX+i], register_tpb[i] );
			}
		}		
		
		//START getting valid data
		tg_reg_write( FPGA_TI_ROIC_SYNC_TPSEL_SET, 0x1 );      // sync mask & TPSEL_LOW
		tg_reg_write( FPGA_TI_ROIC_SYNC_TPSEL_SET, 0x0 );      // sync unmask & TPSEL_LOW	
		//tg_reg_write( 0x152, 0x3 );      // sync mask & synctpsel_mode
		//tg_reg_write( 0x152, 0x2 );      // sync unmask & synctpsel_mode	
		
		tg_reg_write( FPGA_TI_ROIC_SYNC_GEN_EN, 0x1 );	//SYNC ONOFF. 주소값: 0x0181 
		tg_reg_write( FPGA_TI_ROIC_SYNC_GEN_EN, 0x0 );	
		
		//SET TPb as a active profile
		
		tg_reg_write( FPGA_TI_ROIC_SYNC_TPSEL_SET, 0x8009 );      // sync mask & TPSEL_HIGH
		tg_reg_write( FPGA_TI_ROIC_SYNC_TPSEL_SET, 0x8008 );      // sync unmask & TPSEL_HIGH	
		//tg_reg_write( 0x152, 0x3 );      // sync mask & synctpsel_mode
		//tg_reg_write( 0x152, 0x2 );      // sync unmask & synctpsel_mode	
		
		tg_reg_write( FPGA_TI_ROIC_SYNC_GEN_EN, 0x1 );	//SYNC ONOFF. 주소값: 0x0181 
		tg_reg_write( FPGA_TI_ROIC_SYNC_GEN_EN, 0x0 );	
		
		ti_roic_setup_essential_bit();
		
		retry_cnt = 0;	
		do{
			ti_roic_delay_compensation();	
			n_result = ti_roic_bit_alignment();
		}while(n_result < 0 && ++retry_cnt < TI_ROIC_BIT_ALIGNMENT_RETRY_NUM);
		
		return n_result;
}

/***********************************************************************************************//**
* \brief		gpio 초기화
* \param		void
* \return		void
* \note			*CAUTION* Currently gpio init process only performs for GROUP0 ROIC
***************************************************************************************************/
int ti_roic_spi_init(struct spi_device *spi)
{		
	int ret;
	
	if (spi == NULL)
	{
		printk("%s[%d] init fail spi_devivce\n", __func__, __LINE__);
		return -ENODEV;
	}

	roic_spi = spi;

	roic_spi->mode = SPI_MODE_0; //??
	roic_spi->max_speed_hz = 10000000;
	roic_spi->bits_per_word = 24;
	roic_spi->chip_select = 0;

	ret = spi_setup(roic_spi);
	if (ret)
	{
		printk("%s[%d] Failed to setup roic_spi_dev\n", __func__, __LINE__);
		spi_unregister_device(roic_spi);
		return -ENODEV;
	}


	return 0;
}

/***********************************************************************************************//**
* \brief		roic power down
* \param
* \return
* \note			FPGA_REG_TI_ROIC_CONFIG_13 (FPGA 0x213번지)에 idle 상태에서의 power mode가 저장된다.
				저장된 power mode가 normal이면 아무 동작을 수행하지 않아도 된다.
				저장된 power mode가power down혹은 nap모드이면 roic power 모드를 해당 값으로 변경하고, power down동작을 수행한다.
***************************************************************************************************/
void ti_roic_enter_low_power_mode(void)
{
	u16 w_data = 0;
	int n_result = 0;
	u16 w_idle_pm = tg_reg_read(FPGA_REG_TI_ROIC_CONFIG_13);
	
	u16 w_tft_aed_opt = get_tft_aed_option();
	u16 w_roic_sel_l = tg_reg_read(FPGA_REG_TFT_AED_ROIC_SEL_L);	
	u8 c_roic_idx = 0;
	u16 w_5d_data = 0;
	
	if(w_tft_aed_opt & TFT_AED_MODE_ENABLE && tg_reg_read(FPGA_SEL_TRIGGER) == eTRIG_SELECT_NON_LINE )
	{
		// TFT AED mode enable 이며, 현재 AED trigger mode 로 설정되어 있으면, idle일 때 STR = 2설정
		w_data = tg_reg_read(FPGA_REG_TI_ROIC_CONFIG_11);
		//ti_roic_read_all(TI_ROIC_REG_11, &w_data);
		w_data &= ~(0x3 << 4);
		w_data |= (0x2 << 4);
		ti_roic_write_all( TI_ROIC_REG_11, w_data);
		
		// ROIC clock에 영향을 주는 STR 값 변경되었으므로 delay compensation, bit alignment 수행
		ti_roic_sync();

		w_5d_data = tg_reg_read(FPGA_REG_TI_ROIC_CONFIG_5D);
		
		for(c_roic_idx = 0; c_roic_idx < g_n_roic_num; c_roic_idx++)
		{
			if( w_roic_sel_l & (0x01 << c_roic_idx) )
				continue;
			
			if( ROIC_REG_13_POWER_DOWN_WITH_PLL_OFF == w_idle_pm
				|| ROIC_REG_13_POWER_DOWN_WITH_PLL_ON == w_idle_pm )
			{
				w_5d_data |= (0x01 << 15);
				ti_roic_write_sel( TI_ROIC_REG_5D, w_5d_data, c_roic_idx);
			}	
			
			w_5d_data |= (0x01 << 14);
			ti_roic_write_sel( TI_ROIC_REG_5D, w_5d_data, c_roic_idx);
			
			ti_roic_write_sel( TI_ROIC_REG_13, w_idle_pm, c_roic_idx);				
		}
		
		/* TFT AED enable & AED mode 이면 촬영 종료 후 idle에서 STR = 2변경, bit align 성공하면
		   이제부터 유효한 TFT AED 모드 flush를 할 수 있다고 FPGA에 알려 주기 위해 
		   TFT_AED_VALID_FLUSH_START bit 1로 set 해줌 */
		if(n_result == 0) // Bit alignment 성공하면
		{
			w_tft_aed_opt |= TFT_AED_VALID_FLUSH_START;
			tg_reg_write(FPGA_REG_TFT_AED_OPTION, w_tft_aed_opt);
		}
	}
	else	
	{
		if( ROIC_REG_13_NORMAL_MODE != w_idle_pm )
		{
			w_data = tg_reg_read(FPGA_REG_TI_ROIC_CONFIG_5D);
			//ti_roic_read_all( TI_ROIC_REG_5D, &w_data);
			
			if( ROIC_REG_13_POWER_DOWN_WITH_PLL_OFF == w_idle_pm
				|| ROIC_REG_13_POWER_DOWN_WITH_PLL_ON == w_idle_pm )
			{
				w_data |= (0x01 << 15);
				ti_roic_write_all( TI_ROIC_REG_5D, w_data);
			}	
			
			w_data |= (0x01 << 14);
			ti_roic_write_all( TI_ROIC_REG_5D, w_data);
			
			ti_roic_write_all( TI_ROIC_REG_13, w_idle_pm);	
		}
	}
}
	
/***********************************************************************************************//**
* \brief		roic power up
* \param
* \return
* \note			ROIC 를 normal mode로 변경한다. 
				변경된 power mode는 roic에 직접 변경하고, FPGA_REG_TI_ROIC_CONFIG_13 (FPGA 0x213번지)에 저장되지 않는다. 
				
				FPGA_REG_TI_ROIC_CONFIG_13 (FPGA 0x213번지)에 idle 상태에서의 power mode를 저장한다.
				저장된 power mode가 normal이면 아무 동작을 수행하지 않아도 된다.
				저장된 power mode가power down혹은 nap모드이면 roic power 모드를 해당 값으로 변경하고, power down동작을 수행한다.
***************************************************************************************************/
void ti_roic_exit_low_power_mode(void)
{
	u16 w_data = 0;
	u16 w_idle_pm = tg_reg_read(FPGA_REG_TI_ROIC_CONFIG_13);
	u16 w_tft_aed_opt = get_tft_aed_option();
	
	// TFT AED enable & AED mode 이면 촬영 시작 시 TFT_AED_VALID_FLUSH_START bit clear 해줌
	if(w_tft_aed_opt & TFT_AED_MODE_ENABLE && tg_reg_read(FPGA_SEL_TRIGGER) == eTRIG_SELECT_NON_LINE )
	{
		w_tft_aed_opt &= ~TFT_AED_VALID_FLUSH_START;
		tg_reg_write(FPGA_REG_TFT_AED_OPTION, w_tft_aed_opt);
	}
		
	if( ROIC_REG_13_NORMAL_MODE != w_idle_pm )
	{	
		ti_roic_write_all( TI_ROIC_REG_13, ROIC_REG_13_QUICK_WAKEUP);
		
		w_data = tg_reg_read(FPGA_REG_TI_ROIC_CONFIG_5D);
		//ti_roic_read_all( TI_ROIC_REG_5D, &w_data);
		
		w_data &= ~(0x01 << 14);
		ti_roic_write_all( TI_ROIC_REG_5D, w_data);
		
		if( ROIC_REG_13_POWER_DOWN_WITH_PLL_OFF == w_idle_pm
			|| ROIC_REG_13_POWER_DOWN_WITH_PLL_ON == w_idle_pm )
		{
			w_data &= ~(0x01 << 15);
			ti_roic_write_all( TI_ROIC_REG_5D, w_data);
		}
		
		ti_roic_write_all( TI_ROIC_REG_13, ROIC_REG_13_NORMAL_MODE);	
	}
	
	// Readout mode에서는 STR = 3 설정
	w_data = tg_reg_read(FPGA_REG_TI_ROIC_CONFIG_11);
	//ti_roic_read_all(TI_ROIC_REG_11, &w_data);
	w_data |= (0x3 << 4);
	ti_roic_write_all( TI_ROIC_REG_11, w_data);
		
	// Timing profile a,b 설정, bit alignment 실패 시 에러 처리
	if( ti_roic_sync() < 0 )
	{
		tg_reg_write( FPGA_REG_TI_ROIC_BIT_ALIGN_ERR, 1 );
	}
	else
	{
		tg_reg_write( FPGA_REG_TI_ROIC_BIT_ALIGN_ERR, 0 );
	}
			
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 exposure_evt_check(void)
{
	u32 n_ret;
	unsigned long irqflags;
    spin_lock_irqsave(&exposure_wait_queue_ctx.evt_thread_lock,irqflags);
    n_ret = exposure_wait_queue_ctx.evt_wait_cond;
    spin_unlock_irqrestore(&exposure_wait_queue_ctx.evt_thread_lock,irqflags);
    return n_ret;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
s32 roic_powerup_thread(void *param)
{
	s32 ret;
	unsigned long irqflags;
	
	while(!kthread_should_stop())
	{
		ret = wait_event_interruptible(exposure_wait_queue, exposure_evt_check() > 0);
	
		if (ret != 0) {
	    	printk("ERR: wait_event_interruptible\r\n");
		}
		
		/* smart offset(double readout) 사용 시에, offset영상/X-ray영상 모두 exposure에서 wake-up 후 readout done시 power-down 필요 */
		
    	spin_lock_irqsave(&exposure_wait_queue_ctx.evt_thread_lock,irqflags);
		if( exposure_wait_queue_ctx.evt_wait_cond == 1 )
		{
			printk("ROIC power up\r\n");
			//roic_power_up(0x00);
			ti_roic_exit_low_power_mode();
		}
		else if( exposure_wait_queue_ctx.evt_wait_cond == 2 )
		{
			printk("ROIC power down\r\n");
			//roic_power_down(0x00);
			ti_roic_enter_low_power_mode();
		}
		else
		{
			printk("Invalid wait condition\r\n");
		}
		exposure_wait_queue_ctx.evt_wait_cond = 0;	
		spin_unlock_irqrestore(&exposure_wait_queue_ctx.evt_thread_lock,irqflags);
		
	}

	return 0;
}

/***********************************************************************************************//**
* \brief		roic_init_initialize
* \param
* \return
* \note			
***************************************************************************************************/
void ti_roic_initialize(void)
{
	unsigned long irqflags;

	tg_reg_write(FPGA_ROIC_CLK_EN, 0x01);	

	ti_roic_initialize_inner();
	ti_roic_sync();
	
	ti_roic_enter_low_power_mode();

	tg_reg_write(FPGA_ROIC_CLK_EN, 0x00);			

	if( exposure_wait_queue_ctx.evt_thread_stt == 0 )
	{
		spin_lock_irqsave(&exposure_wait_queue_ctx.evt_thread_lock,irqflags);
		exposure_wait_queue_ctx.evt_thread_stt = 1;
		exposure_wait_queue_ctx.evt_kthread = kthread_run(roic_powerup_thread, NULL, "roic_powerup_kthread");

		if (IS_ERR(exposure_wait_queue_ctx.evt_kthread)) 
		{	
			exposure_wait_queue_ctx.evt_thread_stt = 0;
			printk("\nVIVIX TG ERROR : kthread run : %p\n", exposure_wait_queue_ctx.evt_kthread);
		}
		spin_unlock_irqrestore(&exposure_wait_queue_ctx.evt_thread_lock,irqflags);
	}

	printk( "-------------------- ROIC INFO --------------------\n" );
	printk("MODEL TYPE: TI  AFE2256\n");
	printk("STATUS: INITIALIZED\n");
	printk( "-------------------- ROIC INFO --------------------\n" );
}

/***********************************************************************************************//**
* \brief		roic discovery
* \param
* \return
* \note
***************************************************************************************************/
s32 roic_discovery(void)
{
	u16 data = 0x0000;
	u16 idx;
	s32 result = 1;

	tg_reg_write(FPGA_ROIC_CLK_EN, 0x01);	
	
	//Initialize	
	//1.1 resetting 
	ti_roic_write_all(TI_ROIC_REG_00, 0x01);	
	udelay(1);			

	//1.2 Skip quick wakeup mode
	//1.3 Set TRIM_LOAD (This loads the factory-set trim value to the #sel ROIC)
	ti_roic_write_all(TI_ROIC_REG_30, 0x2);
	udelay(10);					// wait for 100 us
	//ROIC_TPSEL_HIGH;			//no need in future
	tg_reg_write( FPGA_TI_ROIC_SYNC_TPSEL_SET, 0x9 );      // sync mask & set  TP 1					
	udelay(100);
	ti_roic_write_all(TI_ROIC_REG_30, 0x0);
	
	//TI ROIC의 경우 모든 roic reg 값이 reset 후 0. DIE ID인 0x31, 0x32 0x33 제외
	//그러나 DIE_ID 값은 ROIC 별로 고유하므로, 특정 값으로 TI ROIC 임을 구분할 수는 없음.
	//따라서 하기 로직으로 TI ROIC 임을 구분하기로함
	//TI ROIC 구분 로직: 모든 레지스터의 값이 0이고, 31, 32, 33만 값이 존재하면 TIROIC로 간주
	for( idx = 1; idx < TIROIC_REG_NUM; idx++)
	{
		ti_roic_read_all( register_addr[idx], &data ); 

		if( ( register_addr[idx] == 0x31 ) || 
			( register_addr[idx] == 0x32 ) ||
			( register_addr[idx] == 0x33 ) )
		{
			if(data == 0x0000)
				result = 0;
		}
		else
		{
			if(data)
				result = 0;
		}

		//printk("%02Xh: 0x%04X\n", register_addr[idx], data );
	}

	tg_reg_write(FPGA_ROIC_CLK_EN, 0x00);	

	return result;
}

/***********************************************************************************************//**
* \brief		vivix_ti_afe2256_driver_init initialize
* \param		
* \return		status
* \note			기존 fpga.cpp 내 int tg_initialize(u8 i_c_model) 함수 일부 참조
***************************************************************************************************/
int vivix_ti_afe2256_driver_init(u8 i_c_model)
{
	//모델에 따른 ROIC 설정 값 설정
	switch(i_c_model)
	{
	case eBD_2530VW:
		g_n_roic_num = VIVIX_2530VW_ROIC_NUM;			
		g_n_dac_num = VIVIX_2530VW_DAC_NUM;
		break;

	case eBD_3643VW:
		g_n_roic_num = VIVIX_3643VW_ROIC_NUM;			
		g_n_dac_num = VIVIX_3643VW_DAC_NUM;
		break;

	case eBD_4343VW:
		g_n_roic_num = VIVIX_4343VW_ROIC_NUM;			
		g_n_dac_num = VIVIX_4343VW_DAC_NUM;
		break;

	default:			//-
		printk("unknown model type : %d \n",i_c_model);	
		break;
	}

	//printk("vivix_ti_afe2256_driver_init g_n_roic_num: %d g_n_dac_num: %d\n", g_n_roic_num, g_n_dac_num);
	//mdelay(5000);

    // 디텍터 모델 입력, FPGA 소스 통합관리
    g_n_upper_side_roic_num = tg_reg_read(FPGA_ROIC_U_SIZE) / ROIC_LINE_NUM;
    g_n_lower_side_roic_num = g_n_roic_num - g_n_upper_side_roic_num;
    
    // Bit alignment error 초기화
    tg_reg_write( FPGA_REG_TI_ROIC_BIT_ALIGN_ERR, 0 );  

	/*
    if( false == ti_roic_selftest() )
	{
		printk("VIVIX CHIP_TI_AFE2256 self test fail\n");
	}
	*/

	//mdelay(5000);

	return 0;
}

/***********************************************************************************************//**
* \brief		dispatch_ioctl
* \param		
* \return		status
* \note
***************************************************************************************************/
long vivix_ti_afe2256_ioctl(struct file  *filp, unsigned int  cmd, unsigned long  args)
{
	long			rval = 0;  
	ioctl_data_t	io_buffer_t; 		
	ioctl_data_t	*pB;
	u8	 			idx = 0x00;
	u16 			data = 0x0000;

	pB = &io_buffer_t;
	memset( pB, 0x00, sizeof(ioctl_data_t) );

	if (_IOC_SIZE(cmd) == sizeof(ioctl_data_t)) 
	{
		rval = copy_from_user(pB, (ioctl_data_t*)args, sizeof(ioctl_data_t));
		if (rval != 0) 
		{
			printk(KERN_INFO "copy_from_user failed\n");
			return (-EFAULT);
		}
	}
	
	mutex_lock(&g_ioctl_mutex);
	switch (cmd)
	{	
		case VW_MSG_IOCTL_ROIC_SET_PREREQUISITE:
		{
			vivix_ti_afe2256_driver_init(pB->data[0]);
			break;
		}

		case VW_MSG_IOCTL_ROIC_DISCOVERY:
		{
			u8	result = 0;

			result = roic_discovery();
			memcpy(pB->data,&result,sizeof(u8));
			break;
			
		}

		case VW_MSG_IOCTL_RO_PWR_EN:
		{
			u8 b_en;			
			b_en = pB->data[0];
			vivix_gpio_set_value(RO_PWR_EN, b_en);

			break;
		}

		case VW_MSG_IOCTL_ROIC_INFO:		//act as write sel
		{		
			ti_roic_info();
			rval = 0;
			break;
		}

		case VW_MSG_IOCTL_ROIC_SELFTEST:		//act as write sel
		{		
			ti_roic_selftest();
			rval = 0;
			break;
		}
				
		case VW_MSG_IOCTL_ROIC_REG_SPI_WRITE:		//act as write sel
		{		
			u32	addr = 0x00;
			memcpy( &addr, pB->data, sizeof(addr) );
			memcpy( &data, (pB->data) + sizeof(addr), sizeof(data) );			
			memcpy( &idx, (pB->data) + sizeof(addr) + sizeof(data), sizeof(idx) );			
			
			if( true == MASK_TI_ROIC_TPA(addr))
			{
				ti_roic_tp_write(0x00, (u16)addr, data);
			}	
			else if( true == MASK_TI_ROIC_TPB(addr))
			{
				ti_roic_tp_write(0x01, (u16)addr, data);
			}
			else if( true == MASK_TI_ROIC_REAL(addr))
			{
				ti_roic_write_sel((u16)addr, data, idx);
			}  				
							
			rval = 0;
			break;
		}
		
		case VW_MSG_IOCTL_ROIC_REG_SPI_WRITE_ALL:
		{
			u32	addr = 0x00;
			memcpy( &addr,pB->data,sizeof(addr));
			memcpy( &data, (pB->data) + sizeof(addr), sizeof(data) );
			
			if( true == MASK_TI_ROIC_TPA(addr))
			{
				ti_roic_tp_write(0x00, (u16)addr, data);
			}	
			else if( true == MASK_TI_ROIC_TPB(addr))
			{
				ti_roic_tp_write(0x01, (u16)addr, data);
			}
			else if( true == MASK_TI_ROIC_REAL(addr))
			{
				ti_roic_write_all((u16)addr, data);
			} 
			
			rval = 0;
			break;
		}
		
		case VW_MSG_IOCTL_ROIC_REG_SPI_READ: //act as read sel // not use at the moment
		{
			u32	addr = 0x00;
			memcpy(&addr, pB->data, sizeof(addr));
			memcpy( &idx, (pB->data) + sizeof(addr), sizeof(idx) );											
			
			if( true == MASK_TI_ROIC_TPA(addr))
			{
				ti_roic_tp_read(0x00, (u16)addr, &data);
			}	
			else if( true == MASK_TI_ROIC_TPB(addr))
			{
				ti_roic_tp_read(0x01, (u16)addr, &data);
			}
			else if( true == MASK_TI_ROIC_REAL(addr))
			{
				ti_roic_read_sel( (u16)addr, &data, idx );
			}  					
		
			memcpy( pB->data, &data, sizeof(data) );
			rval = 0;
			break;
		}
		
		case VW_MSG_IOCTL_ROIC_REG_SPI_READ_ALL:
		{
			u32	addr = 0x00;
			memcpy( &addr, pB->data, sizeof(addr)) ;
			if( true == MASK_TI_ROIC_TPA(addr))
			{
				ti_roic_tp_read(0x00, (u16)addr, &data);
			}	
			else if( true == MASK_TI_ROIC_TPB(addr))
			{
				ti_roic_tp_read(0x01, (u16)addr, &data);
			}
			else if( true == MASK_TI_ROIC_REAL(addr))
			{
				ti_roic_read_all( (u16)addr, &data );
			}  					
								
			memcpy( pB->data, &data, sizeof(data) );
			//printk("[TI-drv] read-reg: 0x%04X read-val: 0x%04X\n", addr, data);
			rval = 0;
			break;
		}
		
		case VW_MSG_IOCTL_ROIC_CLK_EN:
		{		
			vivix_gpio_set_value(ROIC_MCLK_EN, HIGH);		// Sleep mode Enable. Always OFF, thus set to 1
			rval = 0;
			break;
		}
		
		case VW_MSG_IOCTL_ROIC_INITIAL:
		{			
			// reset
			tg_reg_write( FPGA_RESET_REG, 0 );
			mdelay(10);	// 10ms
			tg_reg_write( FPGA_RESET_REG, 1 );
			udelay(10);
			tg_reg_write( FPGA_RESET_REG, 0 );
			
			mdelay(10);	// 10ms

			ti_roic_initialize(); //TG profile sync
			rval = 0;
			break;
		}
		
		case VW_MSG_IOCTL_ROIC_DATA_SYNC:
		{	
			s32 n_result = -1;
			n_result = ti_roic_sync(); //TG profile sync
				
			memcpy( pB->data, &n_result, sizeof(n_result) );
			rval = 0;
			break;
		}
		
		case VW_MSG_IOCTL_ROIC_DATA_DELAY_CMP:
		{
			ti_roic_delay_compensation();		
			break;	
		}
		
		case VW_MSG_IOCTL_ROIC_DATA_BIT_ALIGNMENT:
		{
			ti_roic_bit_alignment();
			break;	
		}
		
		case VW_MSG_IOCTL_ROIC_DATA_BIT_ALIGN_TEST:
		{
			u16 w_retry_num = 0;
			memcpy(&w_retry_num, pB->data, sizeof(w_retry_num));
			ti_roic_bit_align_test(w_retry_num);
			break;	
		}
		
		case VW_MSG_IOCTL_ROIC_RESET:
		{
			//Not implemented
			rval = 0;
			break;
		}
	
		case VW_MSG_IOCTL_ROIC_DAC_SPI_WRITE:
		{
				u8 c_sel = pB->data[0];
				u16 dac_data = 0x00;
				memcpy( &dac_data, &pB->data[1], sizeof(u16) );
				//printk("[TI-drv] write-dac: 0x%04X\n", dac_data);
				ti_dac_write_sel(dac_data, c_sel);
				rval = 0;
				break;
		}

		case VW_MSG_IOCTL_ROIC_DAC_SPI_READ:
		{
				u8 c_sel = pB->data[0];
				u16 dac_data = 0x00;
				ti_dac_read_sel(&dac_data, c_sel);				
				memcpy( pB->data, &dac_data, sizeof(u16) );
				//printk("[TI-drv] read-dac: 0x%04X\n", dac_data);
				rval = 0;
				break;
		}

		case VW_MSG_IOCTL_ROIC_DAC_SPI_WRITE_ALL:
		{
			u16 dac_data = 0x00;
			memcpy( &dac_data, (pB->data), sizeof(u32) );
			//printk("[TI-drv] write-dac: 0x%04X\n", dac_data);
			ti_dac_write_all(dac_data);
			rval = 0;
			break;
		}

		case VW_MSG_IOCTL_ROIC_DAC_SPI_READ_ALL:
		{
			u16 dac_data = 0x00;
			ti_dac_read_all(&dac_data);				
			memcpy( pB->data, &dac_data, sizeof(u32) );
			//printk("[TI-drv] read-dac: 0x%04X\n", dac_data);
			rval = 0;
			break;
		}
					
		case VW_MSG_IOCTL_ROIC_POWERSAVE_OFF:
		{
			ti_roic_exit_low_power_mode();
			break;
		}
		
		case VW_MSG_IOCTL_ROIC_POWERSAVE_ON:
		{
			ti_roic_enter_low_power_mode();
			break;
		}

		case VW_MSG_IOCTL_TFT_AED_ROIC_CTRL:
		{
			if(get_tft_aed_option() & TFT_AED_MODE_ENABLE)
			{
				if(tg_reg_read(FPGA_SEL_TRIGGER) == eTRIG_SELECT_ACTIVE_LINE)
				{
					ti_roic_enter_low_power_mode();
				}
				else
				{	
					ti_roic_exit_low_power_mode();
					ti_roic_enter_low_power_mode();
				}
			}
			break;
		}
	
		case VW_MSG_IOCTL_ROIC_GAIN_TYPE_SET:
		{
			u16 w_0x5c_reg = 0x00;
			//기존 로직 application단 set_gain_type()에서 호출하고 있었음
			/*
			w_0x5c_reg = roic_reg_read((eFPGA_REG_TI_ROIC_CONFIG_5C - eFPGA_REG_TI_ROIC_CONFIG_00));
			w_0x5c_reg &= ~(0x1f << 11);
			w_0x5c_reg |= ((w_input_charge & 0x1f) << 11);
			write( eFPGA_REG_TI_ROIC_CONFIG_5C , w_0x5c_reg );  
			*/

			//0. get Input Charge range
			memcpy(&data,&pB->data[0],2);
			
			//1. set Input charge range from 5C tpA reg
			ti_roic_tp_read(0x00, (u16)TI_ROIC_REG_5C, &w_0x5c_reg);				//read TpA 5C
			w_0x5c_reg &= ~(0x1f << 11);											//filtering 상위 5bit drop
			w_0x5c_reg |= ((data & 0x1f) << 11);									//filtering 하위 11bit drop 후 | 연산
			ti_roic_tp_write(0x00, (u16)TI_ROIC_REG_5C, w_0x5c_reg);				//write TpA 5C
			tg_reg_write((u16)FPGA_REG_TI_ROIC_CONFIG_5C, w_0x5c_reg);				//write to FPGA REG for permenant configuration save
			break;
		}	

		//Not implemented
		case VW_MSG_IOCTL_ROIC_DAC_SPI_WRITE_FLOAT_ALL:
		case VW_MSG_IOCTL_ROIC_DAC_SPI_READ_FLOAT_ALL:
		{
			break;
		}

		default:
		{
				printk("ti roic drv:Unsupported IOCTL_Xxx (%02d)\n", _IOC_NR(cmd));
				return -1;
				break;
		}
	}
	mutex_unlock(&g_ioctl_mutex);
	rval = copy_to_user((ioctl_data_t*)args, pB, sizeof(ioctl_data_t));
	return rval;
}

/***********************************************************************************************//**
* \brief		dispatch_open
* \param		
* \return		status
* \note
***************************************************************************************************/
int vivix_ti_afe2256_open(struct inode *inode, struct file  *filp)
{
	return 0;
}

/***********************************************************************************************//**
* \brief		dispatch_release
* \param		
* \return		status
* \note
***************************************************************************************************/
int vivix_ti_afe2256_release(struct inode *inode, struct file  *filp)
{
	return 0;
}

/***********************************************************************************************//**
* \brief		dispatch_read
* \param		
* \return		status
* \note
***************************************************************************************************/
ssize_t vivix_ti_afe2256_read(struct file  *filp, char  *buff, size_t  count, loff_t  *offp)
{
	return 0;	
}

static struct file_operations vivix_ti_afe2256_fops = {
    .read           = vivix_ti_afe2256_read,
    .open           = vivix_ti_afe2256_open,
    .release        = vivix_ti_afe2256_release,
    .unlocked_ioctl = vivix_ti_afe2256_ioctl,
};

/***********************************************************************************************//**
* \brief		roic 모듈 loading 
* \param		
* \return		status
* \note			fpga 에 모델 설정 및 모델별 queue buffer, roic관련 변수 초기화
***************************************************************************************************/
static const struct of_device_id vivix_roic_of_match[] = {
    { .compatible = "vw,vivix_roic",
    },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, vivix_roic_of_match);

int vivix_ti_afe2256_probe(struct spi_device *spi)
{
	int ret;
	struct device *dev_ret;

	printk("\nVIVIX TI 2256 ROIC Linux Device Driver Loading ... \n");

	ti_roic_spi_init(spi);
	

	/* 변수 초기화 */	
	g_n_dac_num = 0;

	ret = alloc_chrdev_region(&vivix_ti_afe2256_dev, 0, 1, TI_AFE2256_DRIVER_NAME);
	if (ret < 0)
	{
		printk("%s[%d] alloc_chrdev_region error(%d)\n", __func__, __LINE__, ret);
		return ret;
	}

	cdev_init(&vivix_ti_afe2256_cdev, &vivix_ti_afe2256_fops);
	ret = cdev_add(&vivix_ti_afe2256_cdev, vivix_ti_afe2256_dev, 1);
	if (ret < 0)
	{
		printk("%s[%d] cdev_add error(%d)\n", __func__, __LINE__, ret);
		unregister_chrdev_region(vivix_ti_afe2256_dev, 1);
		return ret;
	}

	vivix_ti_afe2256_class = class_create(TI_AFE2256_DRIVER_NAME);
	if (IS_ERR(vivix_ti_afe2256_class))
	{
		cdev_del(&vivix_ti_afe2256_cdev);
		unregister_chrdev_region(vivix_ti_afe2256_dev, 1);
		printk("%s[%d] class_create error(%ld) \n", __func__, __LINE__, PTR_ERR(vivix_ti_afe2256_class));
		return PTR_ERR(vivix_ti_afe2256_class);
	}

	dev_ret = device_create(vivix_ti_afe2256_class, NULL, vivix_ti_afe2256_dev, NULL, TI_AFE2256_DRIVER_NAME);
	if (IS_ERR(dev_ret))
	{
		class_destroy(vivix_ti_afe2256_class);
		cdev_del(&vivix_ti_afe2256_cdev);
		unregister_chrdev_region(vivix_ti_afe2256_dev, 1);
		printk("%s[%d] class_create error(%ld) \n", __func__, __LINE__, PTR_ERR(dev_ret));
		return PTR_ERR(dev_ret);
	}
	
	return 0;
}

/***********************************************************************************************//**
* \brief		roic 모듈 해제
* \param		
* \return		status
* \note
***************************************************************************************************/
void vivix_ti_afe2256_remove(struct spi_device *spi)
{
	unsigned long irqflags;

	printk("\nVIVIX TI 2256 ROIC Linux Device Driver Unloading ... \n");

    spin_lock_irqsave(&exposure_wait_queue_ctx.evt_thread_lock,irqflags);
	if( (exposure_wait_queue_ctx.evt_thread_stt == 1) && (exposure_wait_queue_ctx.evt_kthread) )
	{
		exposure_wait_queue_ctx.evt_thread_stt = 0;
    	kthread_stop(exposure_wait_queue_ctx.evt_kthread);
    }
	spin_unlock_irqrestore(&exposure_wait_queue_ctx.evt_thread_lock,irqflags);

	device_destroy(vivix_ti_afe2256_class, vivix_ti_afe2256_dev);
	class_destroy(vivix_ti_afe2256_class);
	cdev_del(&vivix_ti_afe2256_cdev);
	unregister_chrdev_region(vivix_ti_afe2256_dev, 1);
}

static const struct spi_device_id vivix_roic_id[] = {
    { "vivix_roic", 0 },
    { }
};
MODULE_DEVICE_TABLE(spi, vivix_roic_id);

static struct spi_driver vivix_roic_driver = {
    .driver = {
        .name       = "vivix_roic",
        .of_match_table = vivix_roic_of_match,
    },

    .probe      = vivix_ti_afe2256_probe,
    .remove     = vivix_ti_afe2256_remove,
    .id_table   = vivix_roic_id,
};
module_spi_driver(vivix_roic_driver);

//MODULE_AUTHOR("vivix@vieworks.co.kr");
MODULE_LICENSE("Dual BSD/GPL");

/* end of driver.c */
