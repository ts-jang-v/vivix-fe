/********************************************************************************  
 모  듈 : roic driver
 작성자 : 서정한
 버  전 : 0.0.1
 설  명 : ADAS1256 ADC driver (1255 호환)
 참  고 : 
 
          1. reference : 1212 spi source 
          
 버  전:
         0.0.1  linux 드라이버용으로 작성, 1012N 타겟

********************************************************************************/

/***************************************************************************************************
*	Include Files
***************************************************************************************************/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>			// kmalloc,kfree
#include <asm/uaccess.h>		// copy_from_user , copy_to_user
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/cdev.h>
#include <linux/device.h>


#include "driver.h"
#include "vworks_ioctl.h"
#include "reg_def.h"
#include "driver_defs.h"
#include "fpga_reg_def.h"

extern int vivix_gpio_get_value(u32 gpio_num);
extern void vivix_gpio_set_value(u32 gpio_num, u8 a_b_level);
extern void vivix_gpio_direction(u32 gpio_num, u8 a_b_level);

extern void tg_reg_write(u32 a_naddr, u16 a_wdata);
extern u16 	tg_reg_read(u32 addr);

extern exp_wait_queue_ctx				exposure_wait_queue_ctx;
extern wait_queue_head_t				exposure_wait_queue;

/***************************************************************************************************
*	Constant Definitionsg
***************************************************************************************************/
#define ROIC_BIT_SIZE	16

#define POWER_UP		vivix_gpio_set_value(RO_PWR_EN, LOW)		//ACTIVE LOW	//initialize는 gpio driver 참고 
#define	POWER_DOWN		vivix_gpio_set_value(RO_PWR_EN, HIGH)		//ACTIVE LOW

#define	CS_HIGH			vivix_gpio_set_value(AED_SPI_SS0, HIGH)				//ACTIVE HIGH
#define CS_LOW			vivix_gpio_set_value(AED_SPI_SS0, LOW)				//ACTIVE HIGH

#define	CLK_HIGH		vivix_gpio_set_value(AED_SPI_CLK, HIGH)
#define CLK_LOW			vivix_gpio_set_value(AED_SPI_CLK, LOW)


#define	DATA_HIGH		vivix_gpio_set_value(AED_SPI_MOSI, HIGH)
#define DATA_LOW		vivix_gpio_set_value(AED_SPI_MOSI, LOW)

#define DATA_IN			(vivix_gpio_get_value(AED_SPI_MISO))

#define ROIC_ADC_SEL_NUM				8
#define ROIC_INSTANCE_NUM				31	//	instance num per adc

#define ROIC_CONFIG_REG_NUM				16

/***************************************************************************************************
*	Type Definitions
***************************************************************************************************/


/***************************************************************************************************
*	Macros (Inline Functions) Definitions
***************************************************************************************************/


/***************************************************************************************************
*	Variable Definitions
***************************************************************************************************/
static dev_t vivix_adi_adas1256_dev;
static struct cdev vivix_adi_adas1256_cdev;
static struct class *vivix_adi_adas1256_class;

int 						set_type = 0;
volatile u16 				g_w_reg_0_data=0x00;

u16 						g_pcal_coeff[ROIC_ADC_SEL_NUM][ROIC_INSTANCE_NUM][VIVIX_DAISY_CHAN_NUM_MAX];

spinlock_t 					g_write_reg_lock_t;
static DEFINE_MUTEX(g_ioctl_mutex);
s32							g_n_daisy_chain_num;
extern volatile u8			g_b_bd_cfg;
//extern u16 			*s_FpgaAddr;

/***************************************************************************************************
*	Function Prototypes
***************************************************************************************************/

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
    spin_lock_irqsave(&exposure_wait_queue_ctx.evt_thread_lock, irqflags);
    n_ret = exposure_wait_queue_ctx.evt_wait_cond;
    spin_unlock_irqrestore(&exposure_wait_queue_ctx.evt_thread_lock, irqflags);
    return n_ret;
}

/***********************************************************************************************//**
* \brief		모든 roic에 data저장
* \param		a_waddr : reg address
* \param		a_wdata : reg data
* \return		void
* \note
***************************************************************************************************/
void roic_write_all(u16 a_waddr,  u16 a_wdata)
{
	s32 nidx,njdx;
	u16 wclk_data=0x00;
	unsigned long flags;
	spin_lock_irqsave(&g_write_reg_lock_t, flags);


	CLK_LOW;
	DATA_LOW;
	CS_HIGH;
	
	wclk_data = 0x8000 | (a_waddr << 10) | (a_wdata &  0x03ff);

	for( nidx =0 ; nidx < g_n_daisy_chain_num ; nidx++)
	{
		for( njdx = 0 ; njdx < ROIC_BIT_SIZE ; njdx++)	
		{
			if( wclk_data & (0x0001 << ((ROIC_BIT_SIZE - 1) - njdx)))
			    DATA_HIGH;
			else
			    DATA_LOW;
			CLK_HIGH;
			CLK_LOW;
		}
	}
    DATA_LOW;
	CS_LOW;
	
	for(nidx =0 ;nidx < 3 ; nidx++)
    {
		CLK_HIGH;
		CLK_LOW;
	}
	
	spin_unlock_irqrestore(&g_write_reg_lock_t, flags);
	
}

/***********************************************************************************************//**
* \brief		모든 roic에 설정된 data읽어옴
* \param		a_waddr : reg address
* \param		a_wdata : reg data
* \note
***************************************************************************************************/
void roic_read_all(u16 a_waddr,  u16 *a_pdata)
{
	s32 	nidx,njdx;
	u16 	wclk_data=0x00;
	
	

	DATA_LOW;
	CLK_LOW;
	CS_HIGH;
	
	wclk_data = 0x4000 | (a_waddr << 10);


	for( nidx =0 ; nidx < g_n_daisy_chain_num ; nidx++)
	{
		for( njdx = 0 ; njdx < ROIC_BIT_SIZE ; njdx++)	
		{
			if( wclk_data & (0x0001 << ((ROIC_BIT_SIZE - 1) - njdx)))
			    DATA_HIGH;
			else
			    DATA_LOW;
			CLK_HIGH;
			
			CLK_LOW;
			
		}
	}

	CS_LOW;
    DATA_LOW;
    
    for( nidx =0 ;nidx < 3 ; nidx++)
    {
		CLK_HIGH;
		
		CLK_LOW;
		
	}
	
	CS_HIGH;

	for( nidx = 0; nidx < g_n_daisy_chain_num; nidx++)
	{
		a_pdata[(g_n_daisy_chain_num - 1) - nidx] = 0;
		
		for( njdx = 0 ; njdx < ROIC_BIT_SIZE ; njdx++)	
		{
			
			CLK_HIGH;
			
			CLK_LOW;
			
			if( DATA_IN)
				a_pdata[(g_n_daisy_chain_num - 1) - nidx] |= 0x8000 >> (njdx);
		}
	}

	CS_LOW;
	udelay(52);
}

/***********************************************************************************************//**
* \brief		roic power down
* \param
* \return
* \note
***************************************************************************************************/
void roic_power_down(u16 i_option)
{
	//u16 w_reg_0 = 0x0000;
	roic_write_all( 0, g_w_reg_0_data );
}

/***********************************************************************************************//**
* \brief		roic power up
* \param
* \return
* \note
***************************************************************************************************/
void roic_power_up(u16 i_option)
{
	u16 w_reg_0 = g_w_reg_0_data;
    u16 w_reg_active = tg_reg_read(FPGA_ACTIVE_POWER_MODE);
	w_reg_0 &= ~(0x07 << 6);
	w_reg_0 |= (w_reg_active << 6);
	roic_write_all( 0, w_reg_0 );
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
			roic_power_up(0x00);
			//ti_roic_exit_low_power_mode();
		}
		else if( exposure_wait_queue_ctx.evt_wait_cond == 2 )
		{
			printk("ROIC power down\r\n");
			roic_power_down(0x00);
			//ti_roic_enter_low_power_mode();
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
* \brief		gpio 초기화
* \param		void
* \return		void
* \note
***************************************************************************************************/
void gpio_init(void)
{
	// SPI_SCLK GPIO3_IO21 //
	/* direction 설정 : output */
	vivix_gpio_direction(AED_SPI_CLK, OUTPUT);
	CLK_LOW; // 초기값 
	
	// SPI_MOSI GPIO3_IO28 //
	/* direction 설정 : output */
	vivix_gpio_direction(AED_SPI_MOSI, OUTPUT);
	DATA_LOW; // 초기값

	// SPI_MISO GPIO3_IO22 //
	/* direction 설정 : input */
	vivix_gpio_direction(AED_SPI_MISO, INPUT);

	// SPI_SS0 GPIO3_IO20 //
	/* direction 설정 : output */
	vivix_gpio_direction(AED_SPI_SS0, OUTPUT);
	CS_LOW;		//초기값
	
	//VW 모델의 경우, FPGA register를 통하여 ROIC_CLK_EN 컨트롤
	//gpio_gdir_gpio3(ROIC_CLK_EN,1);
}


/***********************************************************************************************//**
* \brief		모든 roic cmd 설정
* \param		a_wcmd : command
* \return		void
* \note
***************************************************************************************************/
void roic_cmd_write_all(u16 a_wcmd)
{
	s32 nidx,njdx;
	u16 wclk_data=0x00;

	CLK_LOW;
	DATA_LOW;
	CS_HIGH;
	
	wclk_data = a_wcmd;
	
	for( nidx =0 ; nidx < g_n_daisy_chain_num ; nidx++)
	{
		for( njdx = 0 ; njdx < ROIC_BIT_SIZE ; njdx++)	
		{
			if( wclk_data & (0x0001 << ((ROIC_BIT_SIZE - 1) - njdx)))
			    DATA_HIGH;
			else
			    DATA_LOW;
			CLK_HIGH;
			CLK_LOW;
		}
	}
    DATA_LOW;
	CS_LOW;
	CLK_LOW;
    
    for(nidx =0 ;nidx < 3 ; nidx++)
    {
		CLK_HIGH;
		CLK_LOW;
	}
}

/***********************************************************************************************//**
* \brief		모든 roic cmd array 설정
* \param		a_wcmd : command
* \return		void
* \note
***************************************************************************************************/
void roic_cmd_write_arrary(u16 * a_pcmd)
{
	s32 nidx,njdx;
	
	CLK_LOW;
	DATA_LOW;
	CS_HIGH;
	
	
	for( nidx =0 ; nidx < g_n_daisy_chain_num ; nidx++)
	{
		for( njdx = 0 ; njdx < ROIC_BIT_SIZE ; njdx++)	
		{
			if( a_pcmd[(g_n_daisy_chain_num - 1) - nidx] & (0x0001 << ((ROIC_BIT_SIZE - 1) - njdx)))
			    DATA_HIGH;
			else
			    DATA_LOW;
			CLK_HIGH;
			CLK_LOW;
		}
	}
    DATA_LOW;
	CS_LOW;
	CLK_LOW;
}

/***********************************************************************************************//**
* \brief		모든 roic cmd read
* \param		a_pdata : command read buffer
* \return		void
* \note
***************************************************************************************************/
void roic_cmd_read_all( u16 *a_pdata)
{
	s32 	nidx,njdx;	

	DATA_LOW;
	CLK_LOW;
	CS_HIGH;

	for( nidx = 0; nidx < g_n_daisy_chain_num; nidx++ )
	{
		a_pdata[(g_n_daisy_chain_num - 1) - nidx] = 0;
		
		for( njdx = 0 ; njdx < ROIC_BIT_SIZE ; njdx++)	
		{
			CLK_HIGH;
			CLK_LOW;
			if( DATA_IN)
			{
				a_pdata[(g_n_daisy_chain_num - 1) - nidx] |= 0x0001 << (ROIC_BIT_SIZE - (njdx+1));
			}
		}
	}

	CS_LOW;
	
    DATA_LOW;
    
    for( nidx =0 ;nidx < 3 ; nidx++)
    {
		CLK_HIGH;
		CLK_LOW;
	}
}

/***********************************************************************************************//**
* \brief		roic_init_sequence_sel
* \param
* \return
* \note
***************************************************************************************************/
void roic_init_sequence_sel(int sel,u16 coeff[][g_n_daisy_chain_num])
{
	int i;
	
	roic_cmd_write_all(0xC000);
	roic_cmd_write_all(0xC000 + (0x0001 << sel));
	roic_cmd_write_all(0xDC01);
	roic_cmd_write_all(0xC400);
	roic_cmd_write_all(0xC81F);
	roic_cmd_write_all(0xC89F);
	roic_cmd_write_all(0xC81F);
	roic_cmd_write_all(0xC800);
	roic_cmd_write_all(0xDC00);
	roic_cmd_write_all(0xC000);//END of ROM LOAD
	roic_cmd_write_all(0xC000);//START of RAM READ
	roic_cmd_write_all(0xC000 + (0x0001 << sel));
	roic_cmd_write_all(0xDC01);
	roic_cmd_write_all(0xC400);
	roic_cmd_write_all(0xC81F);
	
	for(i =0  ;i < ROIC_INSTANCE_NUM ; i++)
	{
		
		roic_cmd_write_all(0xC800 + i);
		roic_cmd_write_all(0xE000);
		
	
		roic_cmd_read_all((u16 *)coeff[i]);
	
	}
	
	roic_cmd_write_all(0xC81F);
	roic_cmd_write_all(0xC800);
	roic_cmd_write_all(0xC400);
	roic_cmd_write_all(0xDC00);
	roic_cmd_write_all(0xC000);
}

/***********************************************************************************************//**
* \brief		check_integrity
* \param
* \return
* \note
***************************************************************************************************/
bool check_integrity(u16 * buf)
{
	int i ,j,k;
	u16 data;
	int start,end;
	
	
	for(i = 0 ; i < ROIC_ADC_SEL_NUM ; i++)
	{
		for(j = 0 ; j < ROIC_INSTANCE_NUM ; j++)
		{
			data = buf[i * ROIC_INSTANCE_NUM + j];
			start = 0 ;
			end = 5;
			if(j == 0)
			{
				end = 3;
			}	
			else if(j == 30)
			{
				start = 2;	
			}
			
			
			for(k = start ; k < end ; k++)
			{
				u16 check = (data >> (k * 2)) & 0x03;
				
				if(!(check == 0x02 || check == 0x01))
				{
					printk("integrity check error %d,%d,%d[0x%04x]\n",i,j,k,data);				
					return false;	
				}
			}
		}	
	}
	
	return true;
}

/***********************************************************************************************//**
* \brief		roic_init_sequence
* \param
* \return
* \note
***************************************************************************************************/
s32 roic_init_sequence(u16 a_pcal_coeff[ROIC_ADC_SEL_NUM][ROIC_INSTANCE_NUM][g_n_daisy_chain_num])
{
	s32 n_ret = 0;
	int i,k,j,m;
	u16 integrity[ROIC_ADC_SEL_NUM * ROIC_INSTANCE_NUM];
	
	memset(a_pcal_coeff,0,ROIC_ADC_SEL_NUM * ROIC_INSTANCE_NUM * g_n_daisy_chain_num * sizeof(u16));	
	
	for(i = 0 ; i < ROIC_ADC_SEL_NUM ; i++)
	{
		roic_init_sequence_sel(i,a_pcal_coeff[i]);
	}
	
	
	m = 0;
	for(m = 0 ; m <g_n_daisy_chain_num ; m++)
	{
		k=0;	
		for(i= 0 ; i < ROIC_ADC_SEL_NUM ; i++)
		{
			for( j = 0 ; j < ROIC_INSTANCE_NUM ; j++)
			{
						
				integrity[k++] = a_pcal_coeff[i][j][m] & 0x3ff;
			}
		}
		
		if(check_integrity(integrity) == false)
		{
			//s("roic integrity fail roic[%d]\n",m+1);
			n_ret = -1;
		}
	}
	return n_ret;
}

/***********************************************************************************************//**
* \brief		roic_power_up_sequence_sel
* \param
* \return
* \note
***************************************************************************************************/
void roic_power_up_sequence_sel(int sel,u16  coeff[][g_n_daisy_chain_num])
{
	int i,j;
	
	roic_cmd_write_all(0xC000);
	roic_cmd_write_all(0xC000 + (0x0001 << sel));
	roic_cmd_write_all(0xDC01);
	roic_cmd_write_all(0xC400);
	roic_cmd_write_all(0xC81F);
	roic_cmd_write_all(0xC89F);
	roic_cmd_write_all(0xC81F);
	roic_cmd_write_all(0xC800);
	roic_cmd_write_all(0xDC00);
	roic_cmd_write_all(0xC000);//END of ROM LOAD
	
	//printf("##############################\n");

	roic_cmd_write_all(0xC000);//START of RAM READ
	roic_cmd_write_all(0xC000 + (0x0001 << sel));
	roic_cmd_write_all(0xDC01);
	roic_cmd_write_all(0xC400);
	roic_cmd_write_all(0xC81F);
	
	for(i =0  ;i < ROIC_INSTANCE_NUM ; i++)
	{
		u16 cmd[g_n_daisy_chain_num];
		
		memcpy(cmd,coeff[i],sizeof(u16) * g_n_daisy_chain_num);
		
		for(j =0 ; j < g_n_daisy_chain_num; j++)
		{
			cmd[j] += 0xC400;	
		}
		
		roic_cmd_write_arrary(cmd);
		roic_cmd_write_all(0xC840 + i);
		roic_cmd_write_all(0xC81f);
	}
	
	roic_cmd_write_all(0xC81F);
	roic_cmd_write_all(0xC800);
	roic_cmd_write_all(0xDC00);
	roic_cmd_write_all(0xC000);
#ifdef OPTION	
	roic_cmd_write_all(0xC000);
	roic_cmd_write_all(0xC000 + (0x0001 << sel);
	roic_cmd_write_all(0xDC01);
	roic_cmd_write_all(0xC400);
	roic_cmd_write_all(0xC81F);


	for(i =0  ;i < ROIC_INSTANCE_NUM ; i++)
	{
		roic_cmd_write_all(0xC800 + i);
		roic_cmd_write_all(0xE000);
		roic_cmd_read_all(coeff[i]);
	}

	roic_cmd_write_all(0xC81F);
	roic_cmd_write_all(0xC800);
	roic_cmd_write_all(0xC400);
	roic_cmd_write_all(0xDC00);
	roic_cmd_write_all(0xC000);
#endif 
}

/***********************************************************************************************//**
* \brief		roic_power_up_sequence
* \param
* \return
* \note
***************************************************************************************************/
void roic_power_up_sequence(u16 a_pcal_coeff[ROIC_ADC_SEL_NUM][ROIC_INSTANCE_NUM][g_n_daisy_chain_num])
{
	int i;
	for(i = 0 ; i < ROIC_ADC_SEL_NUM ; i++)
	{
		roic_power_up_sequence_sel(i,a_pcal_coeff[i]);
	}
	
}

/***********************************************************************************************//**
* \brief		ti_roic_info
* \param		
* \return		status
* \note
***************************************************************************************************/
int adi_roic_info(void)
{
	// u16 data = 0x00;
	// u32 dac_data = 0x00;
	// u16 print_option = 0xFF;
	// u16 scan_direction = 0;
	// u16 auto_reverse = 0;
	// u16 str = 0;
	// u16 integration = 0;
	// u16 power_mode = 0;
	// u16 input_charge_range = 0;
	
	// int i = 0;
	
	// u16 lsw_id = 0;
	// u16 mw_id = 0;
	// u16 msw_id = 0;
	
	//DBG_PRINT("\n%s : %d\n",__func__, __LINE__);
	
	printk( "-------------------- ROIC Gengeral --------------------\n" );
	printk("MODEL TYPE: ADI  1255\n");

	/*
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
	
	printk( "-------------------- ROIC ID --------------------\n" );			
	for( i = 0 ; i < g_n_roic_num ; i++)
	{
		ti_roic_read_sel(TI_ROIC_REG_31, &msw_id, i);
		ti_roic_read_sel(TI_ROIC_REG_32, &mw_id, i);
		ti_roic_read_sel(TI_ROIC_REG_33, &lsw_id, i);
		printk("Idx 0x%x, ID : 0x%04x%04x%04x\n", i, msw_id, mw_id, lsw_id);
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
					if( TI_ROIC_REG_31 == i || TI_ROIC_REG_32 == i || TI_ROIC_REG_33 == i)
						continue;
						
					ti_roic_read_all( register_addr[i], &data ); 
					printk("%02Xh: 0x%04X\n", register_addr[i], data );
			}
	}
	
	printk( "-------------------- DAC Data --------------------\n" );
	ti_dac_read_all(&dac_data);
	printk("DAC0: 0x%04X\n", dac_data );
	printk("DAC1: 0x%04X\n", dac_data );
	*/
	printk( "-------------------- Done --------------------\n" );
	return 0;
}

/***********************************************************************************************//**
* \brief		roic discovery
* \param
* \return
* \note
***************************************************************************************************/
s32 roic_discovery(void)
{
	u8 result = 1;	//1: discovered, 0: not discovered
	u32	addr = 0x000;

	u16 p_data[g_n_daisy_chain_num];
	memset(p_data, 0x00, sizeof(u16)*g_n_daisy_chain_num);
	
	//ADI는 별도의 identification code를 확인하는 register가 없음. 
	//따라서, 모든 레지스터의 not used (factory use only, read only) 값들 중, 초기값이 0이 아닌 값들을 읽어 확인하는 방식으로 구현
	//reg 9: bit 9-0: 7
	addr = 9;
	roic_read_all((u16)addr,p_data);
	//printk(KERN_INFO "[TP] adi roic reg9: %d\n", p_data[0]);
	result &= ((p_data[0] & 0x01FF) == 7); 
	mdelay(1);

	//reg 11: bit 5-0: 24
	addr = 11;
	roic_read_all((u16)addr,p_data);
	//printk(KERN_INFO "[TP] adi roic reg11: %d\n", p_data[0]);
	result &= ((p_data[0] & 0x001F) == 24); 
	mdelay(1);

	//reg 12: bit 7-0: 2
	addr = 12;
	roic_read_all((u16)addr,p_data);
	//printk(KERN_INFO "[TP] adi roic reg12: %d\n", p_data[0]);
	result &= ((p_data[0] & 0x007F) == 2); 
	mdelay(1);

	//reg 13: bit 9-0: 35
	addr = 13;
	roic_read_all((u16)addr,p_data);
	//printk(KERN_INFO "[TP] adi roic reg13: %d\n", p_data[0]);
	result &= ((p_data[0] & 0x01FF)) == 35; 
	mdelay(1);

	//reg 14: bit 9-0: 43
	addr = 14;
	roic_read_all((u16)addr,p_data);
	//printk(KERN_INFO "[TP] adi roic reg14: %d\n", p_data[0]);
	result &= ((p_data[0] & 0x01FF) == 43); 
	mdelay(1);

	//reg 15: bit 9-0: 8
	addr = 15;
	roic_read_all((u16)addr,p_data);
	//printk(KERN_INFO "[TP] adi roic reg15: %d\n", p_data[0]);
	result &= ((p_data[0] & 0x01FF) == 8); 
	mdelay(1);

	return result;
}

/***********************************************************************************************//**
* \brief		roic initialize
* \param
* \return
* \note
***************************************************************************************************/
s32 roic_initialize(void)
{
	s32	n_status = 0;
	unsigned long irqflags;
	
    spin_lock_irqsave(&exposure_wait_queue_ctx.evt_thread_lock,irqflags);
	
	if( exposure_wait_queue_ctx.evt_thread_stt == 0 )
	{
		exposure_wait_queue_ctx.evt_thread_stt = 1;
		exposure_wait_queue_ctx.evt_kthread = kthread_run(roic_powerup_thread, NULL, "roic_powerup_kthread");
		
		if (IS_ERR(exposure_wait_queue_ctx.evt_kthread)) 
		{	
			exposure_wait_queue_ctx.evt_thread_stt = 0;
			printk("\nVIVIX TG ERROR : kthread run : %p\n", exposure_wait_queue_ctx.evt_kthread);
		}
	}
	spin_unlock_irqrestore(&exposure_wait_queue_ctx.evt_thread_lock,irqflags);

	/*
	n_status = roic_init_sequence(g_pcal_coeff);
	if( n_status == 0 )
	{
		roic_power_up_sequence(g_pcal_coeff);	
	}		
	*/
	//Start power save kernel thread

	return n_status;
}

/***********************************************************************************************//**
* \brief		dispatch_open
* \param		
* \return		status
* \note
***************************************************************************************************/
int vivix_adi_adas1256_open(struct inode *inode, struct file  *filp)
{
	return 0;
}

/***********************************************************************************************//**
* \brief		dispatch_release
* \param		
* \return		status
* \note
***************************************************************************************************/
int vivix_adi_adas1256_release(struct inode *inode, struct file  *filp)
{
	return 0;
}

/***********************************************************************************************//**
* \brief		dispatch_read
* \param		
* \return		status
* \note
***************************************************************************************************/
ssize_t vivix_adi_adas1256_read(struct file  *filp, char  *buff, size_t  count, loff_t  *offp)
{

	return 0;	
}

/***********************************************************************************************//**
* \brief		dispatch_ioctl
* \param		
* \return		status
* \note
***************************************************************************************************/
long vivix_adi_adas1256_ioctl(struct file  *filp, unsigned int  cmd, unsigned long  args)
{
	int	rval = -ENOTTY;  	
	ioctl_data_t	ioBuffer;
	ioctl_data_t	*pB;
	u16				data;
	//u8				bclk_en;
	s32				n_ret;

	pB = &ioBuffer;

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
			switch( pB->data[0] )
			{
			case eBD_2530VW:
				g_n_daisy_chain_num = VIVIX_2530VW_DAISY_CHAIN_NUM;
				break;

			case eBD_3643VW:
				g_n_daisy_chain_num = VIVIX_3643VW_DAISY_CHAIN_NUM;
				break;

			case eBD_4343VW:
				g_n_daisy_chain_num = VIVIX_4343VW_DAISY_CHAIN_NUM;
				break;

			default:
				printk("unknown roic model : %d \n", pB->data[0]);
				break;
			}
			//printk("detector model for roic: %d \n", pB->data[0]);
		}

		case VW_MSG_IOCTL_ROIC_DISCOVERY:
		{
			u8	result = 0;
			result = roic_discovery();
			memcpy(pB->data,&result,sizeof(u8));
			break;
		}

		case VW_MSG_IOCTL_ROIC_REG_SPI_WRITE:
		case VW_MSG_IOCTL_ROIC_REG_SPI_WRITE_ALL:
		{
			u32	addr = 0;
			memcpy(&addr,&pB->data[0],4);	// 상위 2 바이트는 TI ROIC 의 tp a/b 구분용. ADI 에서는 안씀. 
			//printk("VW_MSG_IOCTL_ROIC_REG_SPI_WRITE_ALL addr: %d, %d\n", addr, (u16)addr);
			memcpy(&data,&pB->data[4],2);	

			if(addr == 0)
			{
				g_w_reg_0_data = data;	
			}	

			roic_write_all((u16)addr,data);		
			break;
		}
		
		case VW_MSG_IOCTL_ROIC_REG_SPI_READ:
		case VW_MSG_IOCTL_ROIC_REG_SPI_READ_ALL:
		{
			u32	addr = 0;
			u16 p_data[g_n_daisy_chain_num];
			u32 n_idx;
			u16 w_status = 0;
			
			memcpy(&addr,&pB->data[0],4);
			//printk("VW_MSG_IOCTL_ROIC_REG_SPI_READ addr: %d, %d\n", addr, (u16)addr);
			roic_read_all((u16)addr,p_data);

			for( n_idx = 1; n_idx < g_n_daisy_chain_num ; n_idx++)
			{				
				if( p_data[n_idx] != p_data[0])
				{
					w_status |= 1<<n_idx;
				}
			}
			memcpy(&pB->data[0],&p_data[0],2);
			memcpy(&pB->data[2],&w_status,2);
			
			break;
		}
		case VW_MSG_IOCTL_ROIC_CLK_EN:			//VW 회로 전용 ROIC CLK EN ON (GPIO대신 FPGA 경유)  //현재 코드 상 호출하는 부분이 없고, 동작을 보장할 수 없어 commented out												
		{

			/*
			memcpy(&bclk_en,&pB->data[0],1);
			tg_reg_write(FPGA_ROIC_CLK_EN, bclk_en);				
			//printk("roic clk en: %d \n", bclk_en);
			//gpio_dr_gpio4(ROIC_CLK_EN,bclk_en);
			*/
			break;
		}

		case VW_MSG_IOCTL_ROIC_INITIAL:		//1.부팅 시 호출 2. ROIC power-down -> up 시에도 호출
		{
			s32	n_status = 0;

			n_status = roic_initialize();
			memcpy(pB->data,&n_status,sizeof(n_status));
			break;
		}
		case VW_MSG_IOCTL_ROIC_POWERSAVE_OFF:
		{
			memcpy(&data,&pB->data[0],2);
			roic_power_up(0x00);
			break;
		}
		case VW_MSG_IOCTL_ROIC_POWERSAVE_ON:
		{
			roic_power_down(0x00);
			break;
		}

		case VW_MSG_IOCTL_RO_PWR_EN:
		{
			u8 b_en = pB->data[0];
			u16 reg_data = 0x0000;

			//printk("VW_MSG_IOCTL_RO_PWR_EN: %d \n", b_en);
			vivix_gpio_set_value(RO_PWR_EN, b_en);

			//ROIC RESET. (ADI ROIC의 경우, POWER OFF->ON시 항상 RESET 해주어야함)
			reg_data = tg_reg_read(FPGA_RESET_REG);
			tg_reg_write(FPGA_RESET_REG, reg_data | 0x0001);	//write( FPGA_RESET_REG, 1 );
			udelay(10);
			tg_reg_write(FPGA_RESET_REG, reg_data & 0xFFFE);	//write( FPGA_RESET_REG, 0 );
			mdelay(10);	// 10ms
			break;
		}

		case VW_MSG_IOCTL_ROIC_GAIN_TYPE_SET:
		{
			//ioctl.data [0:1]: w_input_charge

			//0. get IFS val
			u16 ifs_val = 0x0000;
			u16 temp_reg_0_data;

			memcpy(&ifs_val, pB->data, sizeof(u16));

			//1. read reg 0 (which contains IFS val) and only modify IFS
			temp_reg_0_data = g_w_reg_0_data;
			//printk("VW_MSG_IOCTL_ROIC_GAIN_TYPE_SET ifs_val: %d temp_reg_0_data: %d\n", ifs_val, temp_reg_0_data);					//DEBUG
			temp_reg_0_data &= 0xFFE0;	//IFS Filtering. Drop [4:0]
			temp_reg_0_data |= ifs_val;		
			//printk("VW_MSG_IOCTL_ROIC_GAIN_TYPE_SET temp_reg_0_data: %d\n", temp_reg_0_data);	//DEBUG
				
			//2. IFS set
			g_w_reg_0_data = temp_reg_0_data;
			roic_write_all((u16)0x0000,temp_reg_0_data);
			tg_reg_write(FPGA_CONFIG_REG0, temp_reg_0_data);	//write( FPGA_RESET_REG, 0 );	
			break;
		}

		case VW_MSG_IOCTL_ROIC_INFO:		
		{

			adi_roic_info();
			rval = 0;
			break;
		}

		case VW_MSG_IOCTL_ROIC_DATA_SYNC:
		case VW_MSG_IOCTL_ROIC_DATA_DELAY_CMP:
		case VW_MSG_IOCTL_ROIC_DATA_BIT_ALIGNMENT:
		case VW_MSG_IOCTL_ROIC_DATA_BIT_ALIGN_TEST:
		case VW_MSG_IOCTL_ROIC_DAC_SPI_WRITE:
		case VW_MSG_IOCTL_ROIC_DAC_SPI_WRITE_ALL:
		case VW_MSG_IOCTL_ROIC_DAC_SPI_READ:
		case VW_MSG_IOCTL_ROIC_DAC_SPI_READ_ALL:
		{	
			//ADI ROIC는 별도의 roic sync 동작 필요 없음

			break;
		}

		case VW_MSG_IOCTL_ROIC_SELFTEST:		
		case VW_MSG_IOCTL_ROIC_RESET:
		case VW_MSG_IOCTL_TFT_AED_ROIC_CTRL:
		{		
			//Not implemented
			rval = 0;
			break;
		}

		default:
			printk("roic spi:Unsupported IOCTL_Xxx (%02d)\n", _IOC_NR(cmd));
			break;
	}
	mutex_unlock(&g_ioctl_mutex);
	n_ret = copy_to_user((ioctl_data_t*)args, pB, sizeof(ioctl_data_t));
	return 0;
}

static struct file_operations vivix_adi_adas1256_fops = {
    .read           = vivix_adi_adas1256_read,
    .open           = vivix_adi_adas1256_open,
    .release        = vivix_adi_adas1256_release,
    .unlocked_ioctl = vivix_adi_adas1256_ioctl,
};

/***********************************************************************************************//**
* \brief		roic 모듈 init
* \param		
* \return		status
* \note
***************************************************************************************************/
int vivix_adi_adas1256_drv_init(void)
{
	int ret;
	struct device *dev_ret;

	printk("\nVIVIX ADI ROIC Linux Device Driver Loading ... \n");

	gpio_init();

	ret = alloc_chrdev_region(&vivix_adi_adas1256_dev, 0, 1, ADI_ADAS1256_DRIVER_NAME);
	if (ret < 0)
	{
		printk("%s[%d] alloc_chrdev_region error(%d)\n", __func__, __LINE__, ret);
		return ret;
	}

	cdev_init(&vivix_adi_adas1256_cdev, &vivix_adi_adas1256_fops);
	ret = cdev_add(&vivix_adi_adas1256_cdev, vivix_adi_adas1256_dev, 1);
	if (ret < 0)
	{
		printk("%s[%d] cdev_add error(%d)\n", __func__, __LINE__, ret);
		unregister_chrdev_region(vivix_adi_adas1256_dev, 1);
		return ret;
	}

	vivix_adi_adas1256_class = class_create(ADI_ADAS1256_DRIVER_NAME);
	if (IS_ERR(vivix_adi_adas1256_class))
	{
		cdev_del(&vivix_adi_adas1256_cdev);
		unregister_chrdev_region(vivix_adi_adas1256_dev, 1);
		printk("%s[%d] class_create error(%ld) \n", __func__, __LINE__, PTR_ERR(vivix_adi_adas1256_class));
		return PTR_ERR(vivix_adi_adas1256_class);
	}

	dev_ret = device_create(vivix_adi_adas1256_class, NULL, vivix_adi_adas1256_dev, NULL, ADI_ADAS1256_DRIVER_NAME);
	if (IS_ERR(dev_ret))
	{
		class_destroy(vivix_adi_adas1256_class);
		cdev_del(&vivix_adi_adas1256_cdev);
		unregister_chrdev_region(vivix_adi_adas1256_dev, 1);
		printk("%s[%d] class_create error(%ld) \n", __func__, __LINE__, PTR_ERR(dev_ret));
		return PTR_ERR(dev_ret);
	}

	memset(g_pcal_coeff,0,sizeof(g_pcal_coeff));
	
	spin_lock_init(&g_write_reg_lock_t);

	roic_cmd_write_all(0xc000);
	
	return 0;
}

/***********************************************************************************************//**
* \brief		roic 모듈 해제
* \param		
* \return		status
* \note
***************************************************************************************************/
void vivix_adi_adas1256_drv_exit(void)
{
	unsigned long irqflags;
	
	printk("\nVIVIX ADI ROIC Linux Device Driver Unloading ... \n");

    spin_lock_irqsave(&exposure_wait_queue_ctx.evt_thread_lock,irqflags);
	if ( (exposure_wait_queue_ctx.evt_thread_stt == 1) && (exposure_wait_queue_ctx.evt_kthread) )
	{
		exposure_wait_queue_ctx.evt_thread_stt = 0;
    	kthread_stop(exposure_wait_queue_ctx.evt_kthread);
    }
	spin_unlock_irqrestore(&exposure_wait_queue_ctx.evt_thread_lock,irqflags);

	device_destroy(vivix_adi_adas1256_class, vivix_adi_adas1256_dev);
	class_destroy(vivix_adi_adas1256_class);
	cdev_del(&vivix_adi_adas1256_cdev);
	unregister_chrdev_region(vivix_adi_adas1256_dev, 1);

	return;
}



module_init(vivix_adi_adas1256_drv_init);
module_exit(vivix_adi_adas1256_drv_exit);


//MODULE_AUTHOR("vivix@vieworks.co.kr");
MODULE_LICENSE("Dual BSD/GPL");

/* end of driver.c */

