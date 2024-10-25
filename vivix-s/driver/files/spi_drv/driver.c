/***************************************************************************************************
*	Include Files
***************************************************************************************************/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>			// kmalloc,kfree
#include <asm/uaccess.h>		// copy_from_user , copy_to_user
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/spi/spi.h>


#include "driver.h"
#include "vworks_ioctl.h"
#include "reg_def.h"
#include "driver_defs.h"


extern int vivix_gpio_get_value(u32 gpio_num);
extern void vivix_gpio_set_value(u32 gpio_num, u8 a_b_level);
extern void vivix_gpio_direction(u32 gpio_num, u8 a_b_level);
/***************************************************************************************************
*	Constant Definitions
***************************************************************************************************/
#define SPI_MICOM			0
#define SPI_EEPROM			1	
#define SPI_OLED			2
#define	SPI_APD1_BIAS_POT	3
#define	SPI_APD1_OFFSET_POT	4
#define	SPI_APD2_BIAS_POT	5
#define	SPI_APD2_OFFSET_POT	6


/* INSTRUCTION SET	*/
#define EEPROM_INST_READ					0x03	/* Read								*/
#define EEPROM_INST_WRITE					0x02	/* Write							*/
#define EEPROM_INST_WREN					0x06	/* Set the write enable latch		*/
#define EEPROM_INST_WRDI					0x04	/* Reset the write enable latch		*/
#define EEPROM_INST_RDSR					0x05	/* Read Status register				*/
#define EEPROM_INST_WRSR					0x01	/* Write Status register			*/

/***************************************************************************************************
*	Type Definitions
***************************************************************************************************/
typedef union _eeprom_status
{

	u8 DATA;
	struct
	{
		u8	WIP			: 4;        /* BIT0		: Write In Process		*/
		u8	WEL			: 1;		/* BIT1		: Write Enable Latch	*/
		u8	BP0			: 1;   		/* BIT2		: Block protection0		*/
		u8	BP1			: 1;   		/* BIT3		: Block protection2		*/
		u8				: 1;		/* BIT4		: 						*/
		u8				: 1;		/* BIT5		: 						*/
		u8				: 1;		/* BIT6		: 						*/
		u8	WPEN		: 1;		/* BIT7		: Write Protect Enable	*/          
	}BIT;
	
}eeprom_status_u;

/***************************************************************************************************
*	Macros (Inline Functions) Definitions
***************************************************************************************************/


/***************************************************************************************************
*	Variable Definitions
***************************************************************************************************/
static dev_t vivix_spi_dev;
static struct cdev vivix_spi_cdev;
static struct class *vivix_spi_class;
struct device *dev_ret;

static DEFINE_MUTEX(g_ioctl_mutex);

extern volatile u8				g_b_bd_cfg;

/***************************************************************************************************
*	Function Prototypes
***************************************************************************************************/

/***********************************************************************************************//**
* \brief		spi_cs_sel
* \param	
* \return		void
* \note
***************************************************************************************************/
static void spi_cs_sel(u8 c_ic, u8 c_state)
{
	switch(c_ic)
	{
		case SPI_OLED:
			vivix_gpio_set_value(OLED_SS, c_state);	
			break;
		case SPI_APD1_BIAS_POT:
			vivix_gpio_set_value(AED_SPI_SS0, c_state);
		case SPI_APD1_OFFSET_POT:
			vivix_gpio_set_value(AED_SPI_SS1, c_state);
		case SPI_APD2_BIAS_POT:
			vivix_gpio_set_value(AED_SPI_SS2, c_state);
		case SPI_APD2_OFFSET_POT:
			vivix_gpio_set_value(AED_SPI_SS3, c_state);
			break;
		default:
			break;
	}	
}

/***********************************************************************************************//**
* \brief		spi_clock
* \param	
* \return		void
* \note
***************************************************************************************************/
static void spi_clock(u8 c_ic, u8 c_state)
{
	switch(c_ic)
	{
		case SPI_MICOM: //SPI_MICOM
			vivix_gpio_set_value(MCU_SPI_CLK, c_state);	
			break;
		case SPI_EEPROM:
			//vivix_gpio_set_value(ECSPI1_SCLK, c_state);	// GPIO4_IO06
			break;
		case SPI_OLED:
			vivix_gpio_set_value(OLED_SPI_CLK, c_state);	
			break;
		case SPI_APD1_OFFSET_POT:
		case SPI_APD1_BIAS_POT:
		case SPI_APD2_OFFSET_POT:
		case SPI_APD2_BIAS_POT:
			vivix_gpio_set_value(AED_SPI_CLK, c_state);
			break;	
		default:
			break;
	}	
}

/***********************************************************************************************//**
* \brief		spi_dout
* \param	
* \return		void
* \note
***************************************************************************************************/
static void spi_dout(u8 c_ic, u8 c_state)
{
	switch(c_ic)
	{
		case SPI_MICOM: //SPI_MICOM
			vivix_gpio_set_value(MCU_SPI_MOSI, c_state);
			break;
		case SPI_EEPROM:
			//vivix_gpio_set_value(ECSPI1_MOSI, c_state);	// GPIO4_IO07
			break;
		case SPI_OLED:
			break;
		case SPI_APD1_OFFSET_POT:
		case SPI_APD1_BIAS_POT:
		case SPI_APD2_OFFSET_POT:
		case SPI_APD2_BIAS_POT:
			vivix_gpio_set_value(AED_SPI_MOSI, c_state);
			break;	
		
		//case SPI_ROIC_DAC:	
		//	gpio_dr_gpio3(ECSPI4_MOSI, c_state);	// GPIO3_IO28
		//	break;
		default:
			break;
	}	
}

/***********************************************************************************************//**
* \brief		spi_dout
* \param	
* \return		void
* \note
***************************************************************************************************/
static u8 spi_din(u8 c_ic)
{
	u8 c_state = 0;
	switch(c_ic)
	{
		case SPI_MICOM: //SPI_MICOM
			c_state = vivix_gpio_get_value(MCU_SPI_MISO);
			break;
		case SPI_EEPROM:
			//c_state = vivix_gpio_get_value(ECSPI1_MISO);		// GPIO4_IO08
			break;
		case SPI_OLED:		// MISO is not connected.
			break;
		case SPI_APD1_OFFSET_POT:
		case SPI_APD1_BIAS_POT:
		case SPI_APD2_OFFSET_POT:
		case SPI_APD2_BIAS_POT:
			c_state = vivix_gpio_get_value(AED_SPI_MISO);
			break;
		default:
			break;
	}
	return c_state;
}

/***********************************************************************************************//**
* \brief		gpio_init
* \param	
* \return		void
* \note
***************************************************************************************************/
void vivix_spi_gpio_init(void)
{	
	/* AED SPI */
	vivix_gpio_direction(AED_SPI_CLK, OUTPUT);
	vivix_gpio_direction(AED_SPI_MOSI, OUTPUT);
	vivix_gpio_direction(AED_SPI_MISO, INPUT);
	vivix_gpio_direction(AED_SPI_SS0, OUTPUT);
	vivix_gpio_direction(AED_SPI_SS1, OUTPUT);
	vivix_gpio_direction(AED_SPI_SS2, OUTPUT);
	vivix_gpio_direction(AED_SPI_SS3, OUTPUT);
	
	vivix_gpio_set_value(AED_SPI_SS0, HIGH);
	vivix_gpio_set_value(AED_SPI_SS1, HIGH);
	vivix_gpio_set_value(AED_SPI_SS2, HIGH);
	vivix_gpio_set_value(AED_SPI_SS3, HIGH);
	
	vivix_gpio_set_value(AED_SPI_CLK, HIGH);
	vivix_gpio_set_value(AED_SPI_MOSI, LOW);
	
	/* OLED SPI */	
	vivix_gpio_direction(OLED_SPI_CLK, OUTPUT);
	vivix_gpio_direction(OLED_SPI_MOSI, OUTPUT);	
	vivix_gpio_set_value(OLED_SPI_CLK, HIGH);

	/* MCU SPI */
	vivix_gpio_direction(MCU_SPI_CLK, OUTPUT);
	vivix_gpio_direction(MCU_SPI_MOSI, OUTPUT);
	vivix_gpio_direction(MCU_SPI_MISO, INPUT);
	vivix_gpio_direction(MCU_SPI_SS, OUTPUT);
	
	vivix_gpio_set_value(MCU_SPI_SS, LOW);
	vivix_gpio_set_value(MCU_SPI_CLK, HIGH);
	vivix_gpio_set_value(MCU_SPI_MOSI, LOW);

#if 0	
	/* ECSPI1(MICOM, ADC, EEPROM) MISO IO direction 설정 : input */
	vivix_gpio_direction(ECSPI1_MISO, INPUT);
	
	/* MICOM, ADC, EEPROM chip select init : HIGH */
	vivix_gpio_set_value(ECSPI1_SS0, HIGH);
	vivix_gpio_set_value(ECSPI1_SS1, HIGH);
	
	/* MICOM, ADC, EEPROM clock init : HIGH, MOSI init : LOW */
	vivix_gpio_set_value(ECSPI1_SCLK, HIGH);
	vivix_gpio_set_value(ECSPI1_MOSI, LOW);
	
	/* ECSPI2(OLED) direction 설정 : output */	
	vivix_gpio_direction(ECSPI2_SS1, OUTPUT); 
	vivix_gpio_direction(ECSPI2_SCLK, OUTPUT);
	vivix_gpio_direction(ECSPI2_MOSI, OUTPUT);	
	vivix_gpio_direction(ECSPI2_MISO, INPUT);	
	//gpio_gdir_gpio5(ECSPI2_SS0,1);	// gpio_drv에서 수행


	/* ECSPI2(OLED) clock init : HIGH, MOSI init : LOW */	
	vivix_gpio_set_value(ECSPI2_SCLK, HIGH);
	vivix_gpio_set_value(ECSPI2_MOSI, LOW);
#endif
}


/***********************************************************************************************//**
* \brief		spi_read_write_single
* \param		a_pdata
* \param		a_wlength
* \return		void
* \note			
***************************************************************************************************/
void spi_read_write_single(u8 c_ic, u8* a_pwdata,  u32 a_wwlength,u8* a_prdata,  u32 a_wrlength, u32 a_wdummy)
{
	s32 nidx,njdx;
	u8  bdata;

	spi_clock(c_ic, LOW);
	spi_dout(c_ic, LOW);


	for( nidx =0 ; nidx < a_wwlength ; nidx++)
	{
		bdata = a_pwdata[nidx];

		for( njdx = 0 ; njdx < 8 ; njdx++)
		{
			if(( bdata >> (7-njdx)) & 1)		    
				spi_dout(c_ic, HIGH);
			else					   				
				spi_dout(c_ic, LOW);
			
			spi_clock(c_ic, HIGH);			
			spi_clock(c_ic, LOW);		
		}
	}
 
 	for( njdx = 0; njdx < a_wdummy; njdx++)
 	{
		spi_clock(c_ic, HIGH);			
		spi_clock(c_ic, LOW);			
 	}
	
	for( nidx = 0 ; nidx < a_wrlength; nidx++)
	{
		bdata = 0;
		for( njdx = 0 ; njdx < 8 ; njdx++)
		{					
			if(spi_din(c_ic))		    
				bdata |= 1 << (7-njdx);
			
			spi_clock(c_ic, HIGH);			
			spi_clock(c_ic, LOW);	
			
			
		}
		a_prdata[nidx] = bdata;
	}
}

static void vivix_spi_potentiometer_write(u8 c_sel, u8* data, u32 length)
{
	spi_cs_sel((SPI_APD1_BIAS_POT + c_sel), 0);
	spi_read_write_single((SPI_APD1_BIAS_POT + c_sel), data, length, 0, 0, 0);
	spi_cs_sel((SPI_APD1_BIAS_POT + c_sel) ,1);	
}

/***********************************************************************************************//**
* \brief		spi_eeprom_wren
* \param
* \return
* \note			Set Write Enable Latch
***************************************************************************************************/
static void spi_eeprom_wren(void)
{
	u8 p_cmd[1];
	
	p_cmd[0] = EEPROM_INST_WREN;

	spi_cs_sel(SPI_EEPROM,0);
	spi_read_write_single(SPI_EEPROM, p_cmd, 1, 0, 0, 0);
	spi_cs_sel(SPI_EEPROM,1);
}

/***********************************************************************************************//**
* \brief		spi_eeprom_wrdi
* \param
* \return
* \note			Write Disable
***************************************************************************************************/
static void spi_eeprom_wrdi(void)
{
	u8 p_cmd[1];
	
	p_cmd[0] = EEPROM_INST_WRDI;	
	
	spi_cs_sel(SPI_EEPROM,0);
	spi_read_write_single(SPI_EEPROM, p_cmd, 1, 0, 0, 0);
	spi_cs_sel(SPI_EEPROM,1);
}

/***********************************************************************************************//**
* \brief		spi_eeprom_read
* \param
* \return
* \note
***************************************************************************************************/
void spi_eeprom_read(u8 *a_pdata, u32 a_n_size)
{
	u8 	p_cmd[3];	

	p_cmd[0] = EEPROM_INST_READ;
	p_cmd[1] = a_pdata[0];
	p_cmd[2] = a_pdata[1];
	
	spi_cs_sel(SPI_EEPROM,0);
	spi_read_write_single(SPI_EEPROM,p_cmd,3,&a_pdata[2],a_n_size,0);
	spi_cs_sel(SPI_EEPROM,1);
}


/***********************************************************************************************//**
* \brief		is_eeprom_busy
* \param
* \return
* \note
***************************************************************************************************/
bool is_eeprom_busy(void)
{
	eeprom_status_u status_u;
	
	u8 	p_wdata[1];
	u8 	p_rdata[1];	
	
	p_wdata[0] = EEPROM_INST_RDSR;
	
	spi_cs_sel(SPI_EEPROM,0);
	spi_read_write_single(SPI_EEPROM,p_wdata,1,p_rdata,1,0);
	spi_cs_sel(SPI_EEPROM,1);
	
	status_u.DATA = p_rdata[0];

	if(status_u.BIT.WIP == 1)
		return true;
	
	return false;
}

/***********************************************************************************************//**
* \brief		spi_eeprom_page_write
* \param
* \return
* \note
***************************************************************************************************/
s32 spi_eeprom_page_write(u8 *a_pdata, u32 a_n_size)
{
	u8 	p_cmd[3];	
	u32 n_tick = 0;
	s32 ret = 0;
	u32 timeout = 0;

	p_cmd[0] = EEPROM_INST_WRITE;
	p_cmd[1] = a_pdata[0];
	p_cmd[2] = a_pdata[1];
	
	spi_eeprom_wren();
	spi_cs_sel(SPI_EEPROM,0);
	spi_read_write_single(SPI_EEPROM,p_cmd,3,0,0,0);
	spi_read_write_single(SPI_EEPROM,&a_pdata[2],a_n_size,0,0,0);
	spi_cs_sel(SPI_EEPROM,1);
	spi_eeprom_wrdi();
	
	timeout = 10000;
	
	while(is_eeprom_busy())
	{
		udelay(10);
		n_tick++;
		if(n_tick >= timeout)
		{
			ret = -1;
			break;
		}
	}
	
	return ret;
}

/***********************************************************************************************//**
* \brief		
* \param		
* \param		
* \return		
* \note
***************************************************************************************************/
#define SHORT_CMD_LEN		12
void spi_write_mcu(u8 *waddr, u32 wlen)
{
	u32 i, j;
	u8  wdata;

	spi_clock(SPI_MICOM, HIGH);

	for(i = 0; i < wlen; i++){

		wdata = *waddr++;

		for(j = 0; j < 8; j++){
			spi_dout(SPI_MICOM, (wdata >> (7 - j)) & 1);
			spi_clock(SPI_MICOM, LOW);
			spi_clock(SPI_MICOM, LOW);			//for clock duty(low)
			spi_clock(SPI_MICOM, HIGH);
		}

		if(i != wlen - 1 && i % SHORT_CMD_LEN == (SHORT_CMD_LEN - 1)){
			spi_clock(SPI_MICOM, HIGH);
			spi_cs_sel(SPI_MICOM, 1);
			udelay(500);
			spi_cs_sel(SPI_MICOM, 0);
		}
	}
}

void spi_read_mcu(u8 *raddr, u32 rlen)
{
	u32 i, j;
	u8  rdata, wdata;

	spi_clock(SPI_MICOM, HIGH);

	for(i = 0; i < rlen; i++){

		if(i == 0)
			wdata = 0x02;		//STX, Start of message identifier
		else if(i == (rlen - 1))
			wdata = 0x03;		//ETX, End of message ideitifier
		else
			wdata = 0x00;		//DUMMY(command, checksum)

		rdata = 0;

		for(j = 0; j < 8; j++){
			spi_dout(SPI_MICOM, (wdata >> (7 - j)) & 1);

			spi_clock(SPI_MICOM, LOW);

			if(spi_din(SPI_MICOM))
				rdata |= 1 << (7 - j);

			spi_clock(SPI_MICOM, HIGH);
		}
		*raddr++ = rdata;

		if(i != rlen - 1 && i % SHORT_CMD_LEN == (SHORT_CMD_LEN - 1)){
			spi_clock(SPI_MICOM, HIGH);
			spi_cs_sel(SPI_MICOM, 1);
			udelay(500);
			spi_cs_sel(SPI_MICOM, 0);
		}
	}
}

s32 vivix_spi_open(struct inode *inode, struct file  *filp)
{
	return 0;
}


s32 vivix_spi_release(struct inode *inode, struct file  *filp)
{
	return 0;
}



ssize_t vivix_spi_read(struct file  *filp, char  *buff, size_t  count, loff_t  *offp)
{

	return 0;
}

long vivix_spi_ioctl(struct file  *filp, u32  cmd, unsigned long  args)
{
	s32	rval = -ENOTTY;  	
	ioctl_data_t	ioBuffer;
	ioctl_data_t	*pB;
	u8				bwlen;
	u8				pwdata[128];
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
		case VW_MSG_IOCTL_SPI_EEPROM_WRITE:
		{ 
			pB->ret = spi_eeprom_page_write(pB->data, pB->size);
			break;
		}
		case VW_MSG_IOCTL_SPI_EEPROM_READ:
		{
			spi_eeprom_read(pB->data, pB->size);
			break;
		}
#if 0
		case VW_MSG_IOCTL_SPI_ADC_READ_ALL:
		{
			vivix_spi_adc_read_all(pB->data);
			break;
		}
		case VW_MSG_IOCTL_SPI_ADC_READ_SEL:
		{
			vivix_spi_adc_read_sel(pB->data[0], pB->data);
			break;
		}
		case VW_MSG_IOCTL_SPI_ADC_READ_TEST:
		{
			pwdata[0] = pB->data[0];
			vivix_spi_adc_read_sel(pB->data[0], pB->data);
			printk(KERN_INFO "Channel : %d raw_data : 0x%02x\n", pwdata[0], (u32)(pB->data));
			break;
		}
#endif
		case VW_MSG_IOCTL_SPI_OLED_WRITE:
		{
			bwlen = pB->size;
			memcpy(pwdata, &pB->data, bwlen);
			spi_read_write_single(SPI_OLED ,pwdata, bwlen, 0, 0, 0);
			break;
		}
		case VW_MSG_IOCTL_SPI_TX:
			spi_cs_sel(SPI_MICOM, 0);
			spi_write_mcu(&pB->data[0], pB->size);
			spi_cs_sel(SPI_MICOM, 1);
			break;


		case VW_MSG_IOCTL_SPI_RX:
			spi_cs_sel(SPI_MICOM, 0);
			spi_read_mcu(&pB->data[0], pB->size);
			spi_cs_sel(SPI_MICOM, 1);
			break;
		case VW_MSG_IOCTL_SPI_POTENTIOMETER_WRITE:
		{
			u8 c_sel = pB->data[0];
			u8 c_data = pB->data[1];
			vivix_spi_potentiometer_write(SPI_APD1_BIAS_POT, &c_data, sizeof(c_data));
			break;
		}

		default:
			printk("spi:Unsupported IOCTL_Xxx (%02d)\n", _IOC_NR(cmd));
			break;
	}
	mutex_unlock(&g_ioctl_mutex);

	n_ret = copy_to_user((ioctl_data_t*)args, pB, sizeof(ioctl_data_t));
	return 0;
}

static struct file_operations vivix_spi_fops = {
    .read           = vivix_spi_read,
    .open           = vivix_spi_open,
    .release        = vivix_spi_release,
    .unlocked_ioctl = vivix_spi_ioctl,
};

s32 vivix_spi_init(void)
{
	int ret;
	struct vivix_mipi *fpga;

	printk("\nVIVIX SPI  Linux Device Driver Loading ... \n");
	
	vivix_spi_gpio_init();

	ret = alloc_chrdev_region(&vivix_spi_dev, 0, 1, SPI_DRIVER_NAME);
	if (ret < 0)
	{
		printk("%s[%d] alloc_chrdev_region error(%d)\n", __func__, __LINE__, ret);
		return ret;
	}

	cdev_init(&vivix_spi_cdev, &vivix_spi_fops);
	ret = cdev_add(&vivix_spi_cdev, vivix_spi_dev, 1);
	if (ret < 0)
	{
		printk("%s[%d] cdev_add error(%d)\n", __func__, __LINE__, ret);
		unregister_chrdev_region(vivix_spi_dev, 1);
		return ret;
	}

	vivix_spi_class = class_create(SPI_DRIVER_NAME);
	if (IS_ERR(vivix_spi_class))
	{
		cdev_del(&vivix_spi_cdev);
		unregister_chrdev_region(vivix_spi_dev, 1);
		printk("%s[%d] class_create error(%ld) \n", __func__, __LINE__, PTR_ERR(vivix_spi_class));
		return PTR_ERR(vivix_spi_class);
	}

	dev_ret = device_create(vivix_spi_class, NULL, vivix_spi_dev, NULL, SPI_DRIVER_NAME);
	if (IS_ERR(dev_ret))
	{
		class_destroy(vivix_spi_class);
		cdev_del(&vivix_spi_cdev);
		unregister_chrdev_region(vivix_spi_dev, 1);
		printk("%s[%d] class_create error(%ld) \n", __func__, __LINE__, PTR_ERR(dev_ret));
		return PTR_ERR(dev_ret);
	}
	
	return 0;
}

void vivix_spi_exit(void)
{
	printk("\nVIVIX SPI Unloading Driver... \n");
	
	device_destroy(vivix_spi_class, vivix_spi_dev);
	class_destroy(vivix_spi_class);
	cdev_del(&vivix_spi_cdev);
	unregister_chrdev_region(vivix_spi_dev, 1);
}

module_init(vivix_spi_init);
module_exit(vivix_spi_exit);


//MODULE_AUTHOR("vivix@vieworks.co.kr");
MODULE_LICENSE("Dual BSD/GPL");

/* end of driver.c */

