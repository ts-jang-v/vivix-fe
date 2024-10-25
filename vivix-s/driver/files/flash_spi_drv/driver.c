/********************************************************************************  
 모  듈 : flahs driver
 작성자 : 서정한
 버  전 : 0.0.1
 설  명 : 
 참  고 : 
 
                    
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
#include <linux/cdev.h>
#include <linux/device.h>


#include "driver.h"
#include "vworks_ioctl.h"
#include "reg_def.h"

extern int vivix_gpio_get_value(u32 gpio_num);
extern void vivix_gpio_set_value(u32 gpio_num, u8 a_b_level);
extern void vivix_gpio_direction(u32 gpio_num, u8 a_b_level);

/***************************************************************************************************
*	Constant Definitionsg
***************************************************************************************************/
#define FCS_HIGH				vivix_gpio_set_value(FCS, HIGH)
#define FCS_LOW					vivix_gpio_set_value(FCS, LOW)

#define	FCLK_HIGH				vivix_gpio_set_value(FCLK, HIGH)  
#define	FCLK_LOW				vivix_gpio_set_value(FCLK, LOW) 

#define	FDATA3_HIGH				vivix_gpio_set_value(FD3, HIGH)
#define FDATA3_LOW				vivix_gpio_set_value(FD3, LOW)

#define	FDATA2_HIGH				vivix_gpio_set_value(FD2, HIGH)
#define FDATA2_LOW				vivix_gpio_set_value(FD2, LOW)

#define	FDATA1_HIGH				vivix_gpio_set_value(FD1, HIGH)
#define FDATA1_LOW				vivix_gpio_set_value(FD1, LOW)

#define	FDATA0_HIGH				vivix_gpio_set_value(FD0, HIGH)
#define FDATA0_LOW				vivix_gpio_set_value(FD0, LOW)

#define FDATA0_IN				vivix_gpio_get_value(FD0)
#define FDATA1_IN				vivix_gpio_get_value(FD1)
#define FDATA2_IN				vivix_gpio_get_value(FD2)
#define FDATA3_IN				vivix_gpio_get_value(FD3)

#define FLASH_WRITE_MAX_SIZE	256

#define CMD_WRITE_DIS			0x04
#define CMD_WRITE_EN			0x06

#define CMD_READ_STAUTS			0x05

#define CMD_BULK_ERASE			0xC7
#define CMD_64KB_BLOCK_ERASE	0xD8
#define CMD_32KB_BLOCK_ERASE	0x52
#define CMD_SECTOR_ERASE		0x20

#define	CMD_ENTER_QPI			0x38
#define CMD_EXIT_QPI			0xFF

#define CMD_FAST_READ			0x0B
#define CMD_PAGE_PROGRAM		0x02

#define CMD_READ_MF_DEV_ID		0x9F


/***************************************************************************************************
*	Type Definitions
***************************************************************************************************/

typedef union _flash_status_reg
{
	union
	{
		u8	DATA;
		struct
		{
			u8	WPROG			: 1;		/* BIT0		: write in progress		*/
			u8	WEN_LAT			: 1;   		/* BIT1		: write enable latch	*/
			u8	BLK_PROTECT		: 3;        /* BIT2:4	: block protect			*/
			u8	TB_PROTECT		: 1;        /* BIT5		: top/bottom protect 	*/
			u8	SECT_PROTECT	: 1;        /* BIT6		: sector protect		*/
			u8	SR_PROTECT		: 1;        /* BIT7		: status register protect */
		}BIT;
	}STATUS;
	
}flash_status_reg_u;

/***************************************************************************************************
*	Macros (Inline Functions) Definitions
***************************************************************************************************/



/***************************************************************************************************
*	Variable Definitions
***************************************************************************************************/
int set_type = 0;

extern u16 tg_reg_read(u32 addr);

static DEFINE_MUTEX(g_ioctl_mutex);

u8		g_b_quad_flag = 0;
//extern u16 			*s_FpgaAddr;

static dev_t vivix_flash_dev;
static struct cdev vivix_flash_cdev;
static struct class *vivix_flash_class;

/***************************************************************************************************
*	Function Prototypes
***************************************************************************************************/
void flash_spi_read_write_quad(u8* a_pwdata,  u32 a_wwlength,u8* a_prdata,  u32 a_wrlength, u32 a_wdummy);
void flash_spi_read_write_single(u8* a_pwdata,  u32 a_wwlength,u8* a_prdata,  u32 a_wrlength, u32 a_wdummy);
void (*spi_read_write_sel)(u8* a_pwdata,  u32 a_wwlength,u8* a_prdata,  u32 a_wrlength, u32 a_wdummy);

/***********************************************************************************************//**
* \brief		flash_write_en(quad)
* \param		void
* \return		void
* \note
***************************************************************************************************/
static void flash_write_enable(void)
{
	u8 	pwdata[1];

	/* WRITE ENABLE CMD*/
	pwdata[0] = CMD_WRITE_EN;	
	flash_spi_read_write_quad(pwdata,1,0,0,0);
}

/***********************************************************************************************//**
* \brief		flash_write_en(quad)
* \param		void
* \return		void
* \note
***************************************************************************************************/
static void flash_write_disable(void)
{
	u8 	pwdata[1];

	/* WRITE ENABLE CMD*/
	pwdata[0] = CMD_WRITE_DIS;	
	flash_spi_read_write_quad(pwdata,1,0,0,0);
}

/***********************************************************************************************//**
* \brief		enter_qpi_mode
* \param		void
* \return		void
* \note
***************************************************************************************************/
static void enter_qpi_mode(void)
{
	u8 	pwdata[1];
	
	/* Enter QPI CMD*/
    pwdata[0] = CMD_ENTER_QPI;
    flash_spi_read_write_single(pwdata, 1, 0, 0, 0);
}

/***********************************************************************************************//**
* \brief		exit_qpi_mode
* \param		void
* \return		void
* \note
***************************************************************************************************/
static void exit_qpi_mode(void)
{
	u8 	pwdata[1];
	
	/* Exit QPI CMD*/
    pwdata[0] = CMD_EXIT_QPI;
    flash_spi_read_write_quad(pwdata, 1, 0, 0, 0);
}


/***********************************************************************************************//**
* \brief		gpio 초기화
* \param		void
* \return		void
* \note
***************************************************************************************************/
void gpio_init(void)
{
	// SPI_SCLK GPIO4_I21 //
	/* direction 설정 : output */
	vivix_gpio_direction(FCLK, OUTPUT);
	vivix_gpio_direction(FD3, OUTPUT);
	vivix_gpio_direction(FD2, OUTPUT);
	vivix_gpio_direction(FD1, OUTPUT);
	vivix_gpio_direction(FD0, OUTPUT);
	vivix_gpio_direction(FCS, OUTPUT);
	
	
		/* direction 설정 : output */
	vivix_gpio_direction(AEN_PIN, OUTPUT);    // AEN pin

	vivix_gpio_direction(BEN_PIN, OUTPUT);    // BEN pin
	
	/* aen off, ben on */
	vivix_gpio_set_value(AEN_PIN, HIGH);
	vivix_gpio_set_value(BEN_PIN, LOW);

	vivix_gpio_set_value(FD3, HIGH);
	vivix_gpio_set_value(FD2, HIGH);
	vivix_gpio_set_value(FD1, HIGH);
	vivix_gpio_set_value(FD0, HIGH);
}

/***********************************************************************************************//**
* \brief		flash_spi_read_write_quad
* \param		a_pwdata
* \param		a_wwlength
* \return		void
* \note
***************************************************************************************************/
void flash_spi_read_write_quad(u8* a_pwdata,  u32 a_wwlength,u8* a_prdata,  u32 a_wrlength, u32 a_wdummy)
{
	s32 nidx,njdx;
	u8  bdata;

	

	FCLK_HIGH;
	FDATA0_LOW;
	FDATA1_LOW;
	FDATA2_LOW;
	FDATA3_LOW;
	FCS_LOW;


	for( nidx =0 ; nidx < a_wwlength ; nidx++)
	{
		bdata = a_pwdata[nidx];

		for( njdx = 0 ; njdx < 8 ; njdx+=4)
		{
			if(( bdata >> (7-njdx)) & 1)		    FDATA3_HIGH;
			else					   				FDATA3_LOW;
			if(( bdata >> (6-njdx)) & 1)		    FDATA2_HIGH;
			else					   				FDATA2_LOW;
			if(( bdata >> (5-njdx)) & 1)		    FDATA1_HIGH;
			else					   				FDATA1_LOW;			
			if(( bdata >> (4-njdx)) & 1)		    FDATA0_HIGH;
			else					   				FDATA0_LOW;
				
			FCLK_LOW;
			FCLK_HIGH;
		}
	}
 
 	for( njdx = 0; njdx < a_wdummy; njdx++)
 	{
 		FCLK_LOW;			
		FCLK_HIGH;			
 	}
	
	vivix_gpio_direction(FD0, INPUT);
	vivix_gpio_direction(FD1, INPUT);
	vivix_gpio_direction(FD2, INPUT);
	vivix_gpio_direction(FD3, INPUT);
	
	
	for( nidx = 0 ; nidx < a_wrlength; nidx++)
	{
		bdata = 0;
		for( njdx = 0 ; njdx < 8 ; njdx+=4)
		{			
			FCLK_LOW;
			FCLK_HIGH;
			if(FDATA3_IN)		    bdata |= 1 << (7-njdx);
			if(FDATA2_IN)		    bdata |= 1 << (6-njdx);
			if(FDATA1_IN)		    bdata |= 1 << (5-njdx);
			if(FDATA0_IN)		    bdata |= 1 << (4-njdx);		
		}
		a_prdata[nidx] = bdata;
	}
	
	
	FCS_HIGH;
	
	vivix_gpio_direction(FD0, OUTPUT);
	vivix_gpio_direction(FD1, OUTPUT);
	vivix_gpio_direction(FD2, OUTPUT);
	vivix_gpio_direction(FD3, OUTPUT);
	
}

/***********************************************************************************************//**
* \brief		flash_spi_read_write_single
* \param		a_pdata
* \param		a_wlength
* \return		void
* \note
***************************************************************************************************/
void flash_spi_read_write_single(u8* a_pwdata,  u32 a_wwlength,u8* a_prdata,  u32 a_wrlength, u32 a_wdummy)
{
	s32 nidx,njdx;
	u8  bdata;
	

	FCLK_HIGH;
	FDATA0_LOW;
	FCS_LOW;


	for( nidx =0 ; nidx < a_wwlength ; nidx++)
	{
		bdata = a_pwdata[nidx];

		for( njdx = 0 ; njdx < 8 ; njdx++)
		{
			if(( bdata >> (7-njdx)) & 1)		    FDATA0_HIGH;
			else					   				FDATA0_LOW;
			FCLK_LOW;			
			FCLK_HIGH;			
		}
	}
 
 	for( njdx = 0; njdx < a_wdummy; njdx++)
 	{
 		FCLK_LOW;			
		FCLK_HIGH;			
 	}

	vivix_gpio_direction(FD1, INPUT);
	
	for( nidx = 0 ; nidx < a_wrlength; nidx++)
	{
		bdata = 0;
		for( njdx = 0 ; njdx < 8 ; njdx++)
		{			
			
			FCLK_LOW;
			FCLK_HIGH;
			if(FDATA1_IN)		    bdata |= 1 << (7-njdx);
			
			
		}
		a_prdata[nidx] = bdata;
	}
	
	
	FCS_HIGH;

	vivix_gpio_direction(FD1, OUTPUT);
	
	
}


/******************************************************************************/
/**
 * @brief   quad spi 로 id read
 * @param
 * @return
 * @note    
*******************************************************************************/
void flash_read_id(u8* o_p_manufacturer_id, u16* o_p_device_id)
{
    u8 p_wdata[1] = {0,};
    u8 p_rdata[20] = {0,};

    p_wdata[0] = CMD_READ_MF_DEV_ID;

    flash_spi_read_write_quad(p_wdata, 1, p_rdata, 20, 0);

    *o_p_manufacturer_id = p_rdata[0];
    *o_p_device_id = ( p_rdata[1] << 8 ) | p_rdata[2];
}

int vivix_flash_open(struct inode *inode, struct file  *filp)
{


	return 0;
}


int vivix_flash_release(struct inode *inode, struct file  *filp)
{
	return 0;
}

ssize_t vivix_flash_read(struct file  *filp, char  *buff, size_t  count, loff_t  *offp)
{
	u8 pwdata[4];
	u8 prdata[256];
	s32	n_ret;
	
	
	n_ret = copy_from_user(pwdata, (u8*)buff, 4);	
	
	if( n_ret != 0)
	{
		printk(KERN_INFO "dispatch_read copy_from_user failed\n");
	}
	/* flash read */
	pwdata[0] = CMD_FAST_READ;
	flash_spi_read_write_quad(pwdata,4,prdata,count,2);
	
	n_ret = copy_to_user(buff, (u8*)prdata, count);
	
	if( n_ret != 0)
	{
		printk(KERN_INFO "dispatch_read copy_to_user failed\n");
	}

	return 0;	
}

ssize_t vivix_flash_write(struct file  *filp,const char  *buff, size_t  count, loff_t  *offp)
{
	u8                  p_buf[256 + 4];
	s32	n_ret;
	
	n_ret = copy_from_user(p_buf, (u8*)buff, count);
	if( n_ret != 0)
	{
		printk(KERN_INFO "dispatch_write copy_from_user failed\n");
	}
	flash_write_enable();	
	p_buf[0] = CMD_PAGE_PROGRAM;
	flash_spi_read_write_quad((char*)p_buf,count,0,0,0);
	
	return count;	
}

long vivix_flash_ioctl(struct file  *filp, unsigned int  cmd, unsigned long  args)
{
	int	rval = -ENOTTY;  	
	ioctl_data_t	ioBuffer;
	ioctl_data_t	*pB;
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
		case VW_MSG_IOCTL_FLASH_ERASE:
		{
			u8 	pwdata[1];

			flash_write_enable();
						
			/* flash erase */
			pwdata[0] = CMD_BULK_ERASE;
			
			flash_spi_read_write_quad(pwdata,1,0,0,0);
			
			break;
		}
		case VW_MSG_IOCTL_FLASH_STATUS:
		{
			u8 					pwdata[1];
			u8 					prdata[1];
			u8 					b_sel;
		
			memcpy(&b_sel,pB->data,1);

			if( b_sel == 0)
			{		
				pwdata[0] = CMD_READ_STAUTS;
				flash_spi_read_write_quad(pwdata,1,prdata,1,0);
			}

			memcpy(pB->data,prdata,1);						
			break;
		}
		case VW_MSG_IOCTL_FLASH_QPI_ENABLE:
		{
			u8 b_enable;
			memcpy(&b_enable,pB->data,1);
			
			if(b_enable == 1)
			{
				enter_qpi_mode();
			}
			else
			{
				exit_qpi_mode();
			}
			break;
		}	
		
		case VW_MSG_IOCTL_FLASH_WRITE_ENABLE:
		{
			u8	b_enable;
			memcpy(&b_enable,pB->data,1);
			
			if(b_enable == 1)
			{
				flash_write_enable();
			}
			else
			{
				flash_write_disable();
			}
			break;
		}
		case VW_MSG_IOCTL_FLASH_EN:
		{
			if(pB->data[0] == 1)
			{
				vivix_gpio_set_value(BEN_PIN, HIGH);
				vivix_gpio_set_value(AEN_PIN, LOW);
			}
			else
			{
				vivix_gpio_set_value(AEN_PIN, HIGH);
				vivix_gpio_set_value(BEN_PIN, LOW);
			}
			break;
		}
		case VW_MSG_IOCTL_FLASH_READ_ID:
		{
			u8 c_mf_id = 0;
			u16 w_dev_id = 0;	
			flash_read_id(&c_mf_id, &w_dev_id);
			memcpy(pB->data,&c_mf_id,1);
			memcpy(&pB->data[1],&w_dev_id,2);
			break;
		}
		default:
			printk("Unsupported IOCTL_Xxx (%02d)\n", _IOC_NR(cmd));
			break;
	}
	mutex_unlock(&g_ioctl_mutex);
	n_ret = copy_to_user((ioctl_data_t*)args, pB, sizeof(ioctl_data_t));
	return 0;
}

static struct file_operations vivix_flash_fops = {
    .read           = vivix_flash_read,
    .write          = vivix_flash_write,
    .open           = vivix_flash_open,
    .release        = vivix_flash_release,
    .unlocked_ioctl = vivix_flash_ioctl,
};

int vivix_nflash_init(void)
{
	int ret;
	struct device *dev_ret;

	printk("\nVIVIX SPI FLASH  Linux Device Driver Loading ... \n");
	
	gpio_init();

	ret = alloc_chrdev_region(&vivix_flash_dev, 0, 1, FLASH_DRIVER_NAME);
	if (ret < 0)
	{
		printk("%s[%d] alloc_chrdev_region error(%d)\n", __func__, __LINE__, ret);
		return ret;
	}

	cdev_init(&vivix_flash_cdev, &vivix_flash_fops);
	ret = cdev_add(&vivix_flash_cdev, vivix_flash_dev, 1);
	if (ret < 0)
	{
		printk("%s[%d] cdev_add error(%d)\n", __func__, __LINE__, ret);
		unregister_chrdev_region(vivix_flash_dev, 1);
		return ret;
	}

	vivix_flash_class = class_create(FLASH_DRIVER_NAME);
	if (IS_ERR(vivix_flash_class))
	{
		cdev_del(&vivix_flash_cdev);
		unregister_chrdev_region(vivix_flash_dev, 1);
		printk("%s[%d] class_create error(%ld) \n", __func__, __LINE__, PTR_ERR(vivix_flash_class));
		return PTR_ERR(vivix_flash_class);
	}

	dev_ret = device_create(vivix_flash_class, NULL, vivix_flash_dev, NULL, FLASH_DRIVER_NAME);
	if (IS_ERR(dev_ret))
	{
		class_destroy(vivix_flash_class);
		cdev_del(&vivix_flash_cdev);
		unregister_chrdev_region(vivix_flash_dev, 1);
		printk("%s[%d] class_create error(%ld) \n", __func__, __LINE__, PTR_ERR(dev_ret));
		return PTR_ERR(dev_ret);
	}
	
	spi_read_write_sel = flash_spi_read_write_single;
	g_b_quad_flag = 0;
	
	return 0;
}

void vivix_nflash_exit(void)
{
	printk("\nVIVIX SPI FLASH Unloading Driver... \n");
	
	device_destroy(vivix_flash_class, vivix_flash_dev);
	class_destroy(vivix_flash_class);
	cdev_del(&vivix_flash_cdev);
	unregister_chrdev_region(vivix_flash_dev, 1);

	return;
}





module_init(vivix_nflash_init);
module_exit(vivix_nflash_exit);


//MODULE_AUTHOR("vivix@vieworks.co.kr");
MODULE_LICENSE("Dual BSD/GPL");

/* end of driver.c */

