/***************************************************************************************************
*	Include Files
***************************************************************************************************/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>			// kmalloc,kfree
#include <linux/interrupt.h>
#include <asm/uaccess.h>		// copy_from_user , copy_to_user
#include <linux/poll.h>
#include <linux/cdev.h>
#include <linux/device.h>

#include "reg_def.h"
#include "driver.h"
#include "vworks_ioctl.h"
#include "irq_numbers.h"

void vivix_gpio_set_value(u32 gpio_num, u8 a_b_level);
int vivix_gpio_get_value(u32 gpio_num);
void vivix_gpio_direction(u32 gpio_num, u8 a_b_level);

/***************************************************************************************************
*	Constant Definitions
***************************************************************************************************/
/***************************************************************************************************
*	Type Definitions
***************************************************************************************************/


/***************************************************************************************************
*	Macros (Inline Functions) Definitions
***************************************************************************************************/
#define VER0_BIT                (vivix_gpio_get_value(VER0))
#define VER1_BIT                (vivix_gpio_get_value(VER1))
#define VER2_BIT                (vivix_gpio_get_value(VER2))
#define VER3_BIT                (vivix_gpio_get_value(VER3))
#define VER4_BIT                (vivix_gpio_get_value(VER4))
#define HW_VER                  (VER0_BIT | (VER1_BIT<<1) | (VER2_BIT<<2) | (VER3_BIT<<3) | (VER4_BIT<<4))

#define BD_CFG_0_BIT            (vivix_gpio_get_value(BD_CFG_0))
#define BD_CFG_1_BIT            (vivix_gpio_get_value(BD_CFG_1))
#define BD_CFG_2_BIT            (vivix_gpio_get_value(BD_CFG_2))
#define BD_CFG                  (BD_CFG_0_BIT | (BD_CFG_1_BIT<<1) | (BD_CFG_2_BIT<<2))

#define PWR_BUTTON_BIT          (vivix_gpio_get_value(PWR_BUTTON))
#define AP_BUTTON_BIT           (vivix_gpio_get_value(AP_BUTTON))

#define TETHER_EQUIP_BIT        (vivix_gpio_get_value(TETHER_EQUIP))
#define ADAPTOR_EQUIP_BIT       (vivix_gpio_get_value(ADAPTOR_EQUIP))

#define CHARGER_A_STAT1         (vivix_gpio_get_value(B_STAT1_A))
#define CHARGER_A_STAT2         (vivix_gpio_get_value(B_STAT2_A))
#define CHARGER_A_STATE         (CHARGER_A_STAT1 | (CHARGER_A_STAT2<<1))

#define CHARGER_A_PG            (vivix_gpio_get_value(B_nPG_A))
#define CHARGER_B_PG            (vivix_gpio_get_value(B_nPG_B))

#define CHARGER_B_STAT1         (vivix_gpio_get_value(B_STAT1_B))
#define CHARGER_B_STAT2         (vivix_gpio_get_value(B_STAT2_B))
#define CHARGER_B_STATE         (CHARGER_B_STAT1 | (CHARGER_B_STAT2<<1))

/***************************************************************************************************
*	Variable Definitions
***************************************************************************************************/
static dev_t vivix_gpio_dev;
static struct cdev vivix_gpio_cdev;
static struct class *vivix_gpio_class;

static DEFINE_MUTEX(g_ioctl_mutex);

spinlock_t g_gpio1_rd_t;
spinlock_t g_gpio2_rd_t;
spinlock_t g_gpio3_rd_t;
spinlock_t g_gpio4_rd_t;
spinlock_t g_gpio5_rd_t;

spinlock_t g_gpio1_gdir_t;
spinlock_t g_gpio2_gdir_t;
spinlock_t g_gpio3_gdir_t;
spinlock_t g_gpio4_gdir_t;
spinlock_t g_gpio5_gdir_t;

u32				g_n_impact_irq_num = 0;
u8				g_c_impact_event_flag = 0;
volatile bool	g_b_bat_charge_blocked = false;
volatile bool	g_b_bat_charge_full = false;
volatile bool	g_b_aed_pwr_onoff = false;
EXPORT_SYMBOL(g_b_bat_charge_blocked);
EXPORT_SYMBOL(g_b_bat_charge_full);
DECLARE_WAIT_QUEUE_HEAD(queue_read);


/***************************************************************************************************
*	Function Prototypes
***************************************************************************************************/

/***********************************************************************************************//**
* \brief		gpio_dr_gpio1
* \param	
* \return		void
* \note			2014-08-21 오후 4:22:14
***************************************************************************************************/
static void gpio_dr_gpio1(u32 a_n_port, u8 a_b_level)
{
	unsigned long flags;
    spin_lock_irqsave(&g_gpio1_rd_t, flags);

	gpio_set_value(a_n_port, a_b_level);

	spin_unlock_irqrestore(&g_gpio1_rd_t, flags);
}

/***********************************************************************************************//**
* \brief		gpio_dr_gpio2
* \param	
* \return		void
* \note			2014-08-21 오후 4:22:14
***************************************************************************************************/
static void gpio_dr_gpio2(u32 a_n_port, u8 a_b_level)
{
	unsigned long flags;
    spin_lock_irqsave(&g_gpio2_rd_t, flags);
	
	gpio_set_value(a_n_port, a_b_level);

	spin_unlock_irqrestore(&g_gpio2_rd_t, flags);
}

/***********************************************************************************************//**
* \brief		gpio_dr_gpio3
* \param	
* \return		void
* \note			2014-08-21 오후 4:22:14
***************************************************************************************************/
static void gpio_dr_gpio3(u32 a_n_port, u8 a_b_level)
{
	unsigned long flags;
    spin_lock_irqsave(&g_gpio3_rd_t, flags);
	
	gpio_set_value(a_n_port, a_b_level);

	spin_unlock_irqrestore(&g_gpio3_rd_t, flags);
}

/***********************************************************************************************//**
* \brief		gpio_dr_gpio4
* \param	
* \return		void
* \note			2014-08-21 오후 4:22:14
***************************************************************************************************/
static void gpio_dr_gpio4(u32 a_n_port, u8 a_b_level)
{
	unsigned long flags;
    spin_lock_irqsave(&g_gpio4_rd_t, flags);

	gpio_set_value(a_n_port, a_b_level);

	spin_unlock_irqrestore(&g_gpio4_rd_t, flags);
}

/***********************************************************************************************//**
* \brief		gpio_dr_gpio5
* \param	
* \return		void
* \note			2014-08-21 오후 4:22:14
***************************************************************************************************/
static void gpio_dr_gpio5(u32 a_n_port, u8 a_b_level)
{
	unsigned long flags;
    spin_lock_irqsave(&g_gpio5_rd_t, flags);
	
	gpio_set_value(a_n_port, a_b_level);
	
	spin_unlock_irqrestore(&g_gpio5_rd_t, flags);
}

/***********************************************************************************************//**
* \brief		vivix_gpio_set_value
* \param	
* \return		void
* \note			
***************************************************************************************************/
void vivix_gpio_set_value(u32 gpio_num, u8 a_b_level)
{
	u8 gpio;

	gpio = (gpio_num / 32) + 1;

	switch (gpio)
	{
		case GPIO1:
			gpio_dr_gpio1(gpio_num, a_b_level);
			break;
		case GPIO2:
			gpio_dr_gpio2(gpio_num, a_b_level);
			break;
		case GPIO3:
			gpio_dr_gpio3(gpio_num, a_b_level);
			break;
		case GPIO4:
			gpio_dr_gpio4(gpio_num, a_b_level);
			break;
		case GPIO5:
			gpio_dr_gpio5(gpio_num, a_b_level);
			break;
		default:
			printk("%s[%d] invalid gpio_num(%d)\n", __func__, __LINE__, gpio_num);
			break;
	}
}
EXPORT_SYMBOL(vivix_gpio_set_value);

/***********************************************************************************************//**
* \brief		vivix_gpio_set_value
* \param	
* \return		void
* \note			
***************************************************************************************************/
int vivix_gpio_get_value(u32 gpio_num)
{
	return gpio_get_value(gpio_num);
}
EXPORT_SYMBOL(vivix_gpio_get_value);

/***********************************************************************************************//**
* \brief		gpio_gdir_gpio1
* \param	
* \return		void
* \note			2014-08-21 오후 5:51:06
***************************************************************************************************/
static void gpio_gdir_gpio1(u32 a_n_port, u8 a_b_level)
{
	unsigned long flags;
    spin_lock_irqsave(&g_gpio1_gdir_t, flags);
	if(a_b_level)
	{
		gpio_direction_output(a_n_port, 0);
		//gpio_direction_output(a_n_port, 1); //init value?
	}
	else
	{
		gpio_direction_input(a_n_port);
	}
	spin_unlock_irqrestore(&g_gpio1_gdir_t, flags);
}

/***********************************************************************************************//**
* \brief		gpio_gdir_gpio2
* \param	
* \return		void
* \note			2014-08-21 오후 5:51:06
***************************************************************************************************/
static void gpio_gdir_gpio2(u32 a_n_port, u8 a_b_level)
{
	unsigned long flags;
    spin_lock_irqsave(&g_gpio2_gdir_t, flags);
	if(a_b_level)
	{
		gpio_direction_output(a_n_port, 0);
		//gpio_direction_output(a_n_port, 1); //init value?
	}
	else
	{
		gpio_direction_input(a_n_port);
	}
	spin_unlock_irqrestore(&g_gpio2_gdir_t, flags);
}

/***********************************************************************************************//**
* \brief		gpio_gdir_gpio3
* \param	
* \return		void
* \note			2014-08-21 오후 5:51:06
***************************************************************************************************/
static void gpio_gdir_gpio3(u32 a_n_port, u8 a_b_level)
{
	unsigned long flags;
    spin_lock_irqsave(&g_gpio3_gdir_t, flags);
	if(a_b_level)
	{
		gpio_direction_output(a_n_port, 0);
		//gpio_direction_output(a_n_port, 1); //init value?
	}
	else
	{
		gpio_direction_input(a_n_port);
	}
	spin_unlock_irqrestore(&g_gpio3_gdir_t, flags);
}

/***********************************************************************************************//**
* \brief		gpio_gdir_gpio4
* \param	
* \return		void
* \note			2014-08-21 오후 5:51:06
***************************************************************************************************/
static void gpio_gdir_gpio4(u32 a_n_port, u8 a_b_level)
{
	unsigned long flags;
    spin_lock_irqsave(&g_gpio4_gdir_t, flags);
	if(a_b_level)
	{
		gpio_direction_output(a_n_port, 0);
		//gpio_direction_output(a_n_port, 1); //init value?
	}
	else
	{
		gpio_direction_input(a_n_port);
	}
	spin_unlock_irqrestore(&g_gpio4_gdir_t, flags);
}

/***********************************************************************************************//**
* \brief		gpio_gdir_gpio5
* \param	
* \return		void
* \note			2014-08-21 오후 5:51:06
***************************************************************************************************/
static void gpio_gdir_gpio5(u32 a_n_port, u8 a_b_level)
{
	unsigned long flags;
    spin_lock_irqsave(&g_gpio5_gdir_t, flags);
	if(a_b_level)
	{
		gpio_direction_output(a_n_port, 0);
		//gpio_direction_output(a_n_port, 1); //init value?
	}
	else
	{
		gpio_direction_input(a_n_port);
	}
	spin_unlock_irqrestore(&g_gpio5_gdir_t, flags);
}

/***********************************************************************************************//**
* \brief		vivix_gpio_direction
* \param	
* \return		void
* \note			2014-08-21 오후 4:22:14
***************************************************************************************************/
void vivix_gpio_direction(u32 gpio_num, u8 a_b_level)
{
	u8 gpio;

	gpio = (gpio_num / 32) + 1;

	switch (gpio)
	{
		case GPIO1:
			gpio_gdir_gpio1(gpio_num, a_b_level);
			break;
		case GPIO2:
			gpio_gdir_gpio2(gpio_num, a_b_level);
			break;
		case GPIO3:
			gpio_gdir_gpio3(gpio_num, a_b_level);
			break;
		case GPIO4:
			gpio_gdir_gpio4(gpio_num, a_b_level);
			break;
		case GPIO5:
			gpio_gdir_gpio5(gpio_num, a_b_level);
			break;
		default:
			printk("%s[%d] invalid gpio_num(%d)\n", __func__, __LINE__, gpio_num);
			break;
	}
}
EXPORT_SYMBOL(vivix_gpio_direction);

/***********************************************************************************************//**
* \brief		get control version 
* \param	
* \return		HW_VER = (VER0_BIT | (VER1_BIT<<1) | (VER2_BIT<<2) | (VER3_BIT<<3) | (VER4_BIT<<4))
* \note			
***************************************************************************************************/
u8 hw_get_ctrl_version(void)
{
	return (u8)HW_VER;	
}
EXPORT_SYMBOL(hw_get_ctrl_version);

/***********************************************************************************************//**
* \brief		check if it is the board made after vw revision 
* \param		None
* \return		true: vw revision board
* \return		false: NOT vw revision board
* \note			vw revision board has the board number greater than 2
***************************************************************************************************/
bool is_vw_revision_board(void)
{
	if( hw_get_ctrl_version() <= 2) return false;
	else return true;
}
EXPORT_SYMBOL(is_vw_revision_board);

/***********************************************************************************************//**
* \brief		gpio_init
* \param	
* \return		void
* \note			2014-06-17 오후 3:50:03
***************************************************************************************************/
static void gpio_init(void)
{
	spin_lock_init(&g_gpio1_rd_t);
	spin_lock_init(&g_gpio2_rd_t);
	spin_lock_init(&g_gpio3_rd_t);
	spin_lock_init(&g_gpio4_rd_t);
	spin_lock_init(&g_gpio5_rd_t);
	
	spin_lock_init(&g_gpio1_gdir_t);
	spin_lock_init(&g_gpio2_gdir_t);
	spin_lock_init(&g_gpio3_gdir_t);
	spin_lock_init(&g_gpio4_gdir_t);
	spin_lock_init(&g_gpio5_gdir_t);
#if 0 //TO_DO
	
	gpio_gdir_gpio6(F_INF0_PORT,0);
	gpio_gdir_gpio6(F_INF1_PORT,0);
	gpio_gdir_gpio6(F_INF2_PORT,0);
#endif

	vivix_gpio_direction(FPGA_INIT, INPUT);
	vivix_gpio_direction(FPGA_DONE, INPUT); 
	vivix_gpio_direction(EXP_OK, INPUT); 
	
	vivix_gpio_set_value(TG_PWR_EN, HIGH); 
	vivix_gpio_direction(FPGA_PGM, OUTPUT); 
	vivix_gpio_direction(TG_PWR_EN, OUTPUT); 
	vivix_gpio_direction(RO_PWR_EN, OUTPUT);
	vivix_gpio_direction(AED_PWR_EN, OUTPUT);
	vivix_gpio_direction(FPGA_RST, OUTPUT);
	vivix_gpio_direction(EXP_REQ, OUTPUT);
#if 0 //TO_DO	
	vivix_gpio_direction(PMIC_RST, OUTPUT);	
	vivix_gpio_set_value(PMIC_RST, HIGH);
#endif

	vivix_gpio_direction(PWR_OFF, OUTPUT);
	vivix_gpio_set_value(PWR_OFF, LOW);
	
	vivix_gpio_set_value(RO_PWR_EN, LOW);
	vivix_gpio_set_value(AED_PWR_EN, LOW);
	vivix_gpio_set_value(EXP_REQ, LOW);


	vivix_gpio_direction(TETHER_EQUIP, INPUT);
	vivix_gpio_direction(ADAPTOR_EQUIP, INPUT);

	vivix_gpio_direction(B_CHG_EN_A, OUTPUT);
	vivix_gpio_direction(B_CHG_EN_B, OUTPUT);

	vivix_gpio_set_value(B_CHG_EN_A, LOW);
	vivix_gpio_set_value(B_CHG_EN_B, LOW);	
	
	vivix_gpio_direction(B_STAT1_A, INPUT);	// B_STAT1_A_BIT
	vivix_gpio_direction(B_STAT2_A, INPUT);	// B_STAT2_A_BIT
	vivix_gpio_direction(B_STAT1_B, INPUT);	// B_STAT1_B_BIT
	vivix_gpio_direction(B_STAT2_B, INPUT);	// B_STAT2_B_BIT
	
	vivix_gpio_direction(B_nPG_A, INPUT);	// B_nPG_A_BIT
	vivix_gpio_direction(B_nPG_B, INPUT);	// B_nPG_B_BIT

	
	vivix_gpio_direction(PHY_RESET, OUTPUT);	
	vivix_gpio_set_value(PHY_RESET, HIGH);	
	
	
	/* direction 설정 : input */	
	vivix_gpio_direction(VER0, INPUT);
	vivix_gpio_direction(VER1, INPUT);
	vivix_gpio_direction(VER2, INPUT);
	vivix_gpio_direction(VER3, INPUT);
	vivix_gpio_direction(VER4, INPUT);
	
	vivix_gpio_direction(BD_CFG_0, INPUT);
	vivix_gpio_direction(BD_CFG_1, INPUT);
	vivix_gpio_direction(BD_CFG_2, INPUT);
	
	vivix_gpio_direction(PWR_BUTTON, INPUT);
	vivix_gpio_direction(AP_BUTTON, INPUT);
	
	vivix_gpio_direction(OLED_RST, OUTPUT);	//OLED_RST
	vivix_gpio_direction(OLED_A0, OUTPUT);		//OLED_RS
	vivix_gpio_direction(OLED_SS, OUTPUT);		//OLED_SS
	
	vivix_gpio_set_value(OLED_RST, HIGH);
	vivix_gpio_set_value(OLED_A0, HIGH);
	vivix_gpio_set_value(OLED_SS, HIGH);

	vivix_gpio_direction(VDD_CORE_ENABLE, OUTPUT);
	vivix_gpio_set_value(VDD_CORE_ENABLE, HIGH);
	
	if( !is_vw_revision_board() )
	{
		vivix_gpio_direction(POWER_LED, OUTPUT);
		vivix_gpio_set_value(POWER_LED, LOW);
		vivix_gpio_set_value(BOOT_LED_RST, HIGH);
	}

	vivix_gpio_set_value(ADC_GPIO, LOW);

	vivix_gpio_direction(ACC_INT1, INPUT);
	vivix_gpio_direction(ACC_INT2, INPUT);

	g_n_impact_irq_num = gpio_to_irq(ACC_INT1);	
}

/***********************************************************************************************//**
* \brief		dispatch_open
* \param	
* \return		
* \note			
***************************************************************************************************/
s32 vivix_gpio_open(struct inode *inode, struct file  *filp)
{
	return 0;
}

/***********************************************************************************************//**
* \brief		dispatch_release
* \param	
* \return		
* \note			
***************************************************************************************************/
s32 vivix_gpio_release(struct inode *inode, struct file  *filp)
{
	return 0;
}

/***********************************************************************************************//**
* \brief		dispatch_read
* \param	
* \return		
* \note			
***************************************************************************************************/
ssize_t vivix_gpio_read(struct file  *filp, char  *buff, size_t  count, loff_t  *offp)
{
	return 0;
}

/***********************************************************************************************//**
* \brief		dispatch_write
* \param	
* \return		
* \note			
***************************************************************************************************/
ssize_t vivix_gpio_write(struct file  *filp,const char  *buff, size_t  count, loff_t  *offp)
{
	if(strncmp(buff,"icewine",(count-1) > strlen("icewine") ? (count-1): strlen("icewine")) == 0)
	{
		u32 *p_pad_mux;
		
		p_pad_mux = ioremap(UART1_TX_MUX_REG, 4);
		
		*p_pad_mux = UART1_PAD_MUX_ATL1;		
				
		iounmap(p_pad_mux);
	}
	if(strncmp(buff,"icewine 0",(count-1) > strlen("icewine 0") ? (count-1): strlen("icewine 0")) == 0)
	{
		u32 *p_pad_mux;
		
		p_pad_mux = ioremap(UART1_TX_MUX_REG, 4);		
				
		*p_pad_mux = UART1_PAD_MUX_ATL5;
		
		iounmap(p_pad_mux);
	}
	
	return count;	
}

/***********************************************************************************************//**
* \brief		dispatch_poll
* \param		
* \return		status
* \note
***************************************************************************************************/
unsigned int vivix_gpio_poll(struct file * filp,poll_table * wait)
{
	u32 mask=0x00;

	poll_wait(filp,&queue_read,wait);

	if(g_c_impact_event_flag > 0)
	{
	    g_c_impact_event_flag = 0;
	    mask |= POLLIN ;
    }

	return mask;
}


/***********************************************************************************************//**
* \brief		vivix exp interrupt 수행 rising edge
* \param		
* \return		status
* \note
***************************************************************************************************/
irqreturn_t  impact_sensor_interrupt(s32 a_nirq, void *a_pdev_id)
{	
	disable_irq_nosync(a_nirq);
	
	g_c_impact_event_flag  = 1;
	wake_up_interruptible(&queue_read);	
	
	enable_irq(a_nirq);
	
	return	IRQ_HANDLED;
}

/***********************************************************************************************//**
* \brief		dispatch_ioctl
* \param	
* \return		
* \note			
***************************************************************************************************/
long vivix_gpio_ioctl(struct file  *filp, u32  cmd, unsigned long  args)
{
	s32	rval = -ENOTTY;  	
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
		case VW_MSG_IOCTL_GPIO_RISC_ACTIVE:
		{
			break;
		}
		
		case VW_MSG_IOCTL_GPIO_HW_VER:
		{
			pB->data[0] = HW_VER;
			
			break;
		}
		case VW_MSG_IOCTL_GPIO_TETHER_EQUIP:
		{
			pB->data[0] = TETHER_EQUIP_BIT; 
			
			break;
		}
		case VW_MSG_IOCTL_GPIO_ADAPTOR_EQUIP:
		{
			pB->data[0] = ADAPTOR_EQUIP_BIT; 
			
			break;
		}
		case VW_MSG_IOCTL_GPIO_UART1_TX_EN:
		{
			u32 *p_pad_mux;
			u8	b_en;		
			
			b_en = pB->data[0];
			
			p_pad_mux = ioremap(UART1_TX_MUX_REG, 4);
			if( b_en == 1)
			{
				*p_pad_mux = UART1_PAD_MUX_ATL1;
			}
			else
			{
				*p_pad_mux = UART1_PAD_MUX_ATL5;
			}
			iounmap(p_pad_mux);
			
			break;
		}
		case VW_MSG_IOCTL_GPIO_AP_BUTTON:
		{
			pB->data[0] = AP_BUTTON_BIT; 
			
			break;
		}
		case VW_MSG_IOCTL_GPIO_PWR_BUTTON:
		{
			pB->data[0] = PWR_BUTTON_BIT; 
			
			break;
		}
		case VW_MSG_IOCTL_GPIO_B_CHG_EN:
		{
			if(g_b_bat_charge_blocked == true)
			{
				pB->data[0] = 1;
				break;
			}
				
			/* 0: eBAT_CHARGE_0_ON_1_OFF
			   1: eBAT_CHARGE_0_OFF_1_ON
			   2: eBAT_CHARGE_0_ON_1_ON
			   3: eBAT_CHARGE_0_OFF_1_OFF */			
			if( pB->data[0] == 0)
			{
				vivix_gpio_set_value(B_CHG_EN_A, HIGH);
				vivix_gpio_set_value(B_CHG_EN_B, LOW);
			}
			else if( pB->data[0] == 1)
			{
				vivix_gpio_set_value(B_CHG_EN_A, LOW);
				vivix_gpio_set_value(B_CHG_EN_B, HIGH);
			}
			else if( pB->data[0] == 2)
			{
				vivix_gpio_set_value(B_CHG_EN_A, HIGH);
				vivix_gpio_set_value(B_CHG_EN_B, HIGH);
			}
			else if( pB->data[0] == 3)
			{
				vivix_gpio_set_value(B_CHG_EN_A, LOW);
				vivix_gpio_set_value(B_CHG_EN_B, LOW);
			}
			pB->data[0] = 0;
			break;
		}
		case VW_MSG_IOCTL_GPIO_B_CHG_RESTORE:
		{
			g_b_bat_charge_blocked = false;
			pB->data[0] = g_b_bat_charge_full;
			break;
		}
		case VW_MSG_IOCTL_GPIO_B_POWER_GOOD:
		{
			if( pB->data[0] == 0)
			{
				pB->data[0] = CHARGER_A_PG;
			}
			else if(pB->data[0] == 1 )
			{
				pB->data[0] = CHARGER_B_PG;
			}	
			
			break;					
		}
		case VW_MSG_IOCTL_GPIO_B_CHARGING_STAT:
		{
			if( pB->data[0] == 0)
			{
				pB->data[0] = CHARGER_A_STATE;
			}
			else if(pB->data[0] == 1 )
			{
				pB->data[0] = CHARGER_B_STATE;
			}
			
			break;						
		}
		case VW_MSG_IOCTL_GPIO_PHY_RESET:
		{
			vivix_gpio_set_value(PHY_RESET,pB->data[0]);

			break;
		}	
		case VW_MSG_IOCTL_GPIO_OLED_RESET:
		{
			vivix_gpio_set_value(OLED_RST,pB->data[0]);

			break;
		}
		case VW_MSG_IOCTL_GPIO_OLED_A0:
		{
			vivix_gpio_set_value(OLED_A0,pB->data[0]);

			break;
		}
		case VW_MSG_IOCTL_GPIO_OLED_SS:
		{
			vivix_gpio_set_value(OLED_SS,pB->data[0]);

			break;
		}	
		case VW_MSG_IOCTL_GPIO_VDD_CORE_ENABLE:
		{
			u8 b_en;			
			b_en = pB->data[0];
		
			vivix_gpio_set_value(VDD_CORE_ENABLE, b_en);		// 1: disable, 0: enable
			break;
		}
		case VW_MSG_IOCTL_GPIO_BOOT_END:
		{
			vivix_gpio_set_value(POWER_LED, HIGH);
			vivix_gpio_set_value(BOOT_LED_RST, LOW);
			break;
		}
		case VW_MSG_IOCTL_GPIO_BOOT_LED_RST_CTRL:
		{
			u8 b_en;			
			b_en = pB->data[0];
			vivix_gpio_set_value(BOOT_LED_RST, b_en);
			break;
		}
		case VW_MSG_IOCTL_GPIO_PWR_LED_CTRL:
		{
			u8 b_en;			
			b_en = pB->data[0];
			vivix_gpio_set_value(POWER_LED, b_en);
			break;
		}
		case VW_MSG_IOCTL_GPIO_AED_PWR_EN_STATUS:
		{
			pB->data[0] = g_b_aed_pwr_onoff;
			break;
		}
		case VW_MSG_IOCTL_GPIO_B_CHG_FULL:
		{
			g_b_bat_charge_full = pB->data[0];
			break;
		}
		case VW_MSG_IOCTL_GPIO_AED_PWR_EN_CNTL:
		{
			u8 b_en;			
			b_en = pB->data[0];
			
			vivix_gpio_set_value(AED_PWR_EN,b_en);
			if(b_en == 0)
			{
				g_b_aed_pwr_onoff = false;

				/* Sleep mode 진입 시, 모든 pin을 low로 당겨줌 */
				/* (APD1, 2 offset/bias potentiometer) SCLK, MOSI, SS IO direction 설정 : output */	
				vivix_gpio_direction(AED_SPI_CLK, OUTPUT);
				vivix_gpio_direction(AED_SPI_MOSI, OUTPUT);
				vivix_gpio_direction(AED_SPI_SS0, OUTPUT);
				vivix_gpio_direction(AED_SPI_SS1, OUTPUT);
				vivix_gpio_direction(AED_SPI_SS2, OUTPUT);
				vivix_gpio_direction(AED_SPI_SS3, OUTPUT);
				
				/* (APD1, 2 offset/bias potentiometer) MISO IO direction 설정 : input */
				vivix_gpio_direction(AED_SPI_MISO, OUTPUT);
				
				/* APD1, 2 offset/bias potentiometer chip select init : HIGH */
				vivix_gpio_set_value(AED_SPI_SS0, LOW);
				vivix_gpio_set_value(AED_SPI_SS1, LOW);
				vivix_gpio_set_value(AED_SPI_SS2, LOW);
				vivix_gpio_set_value(AED_SPI_SS3, LOW);
				
				/* (APD1, 2 offset/bias potentiometer) clock init : HIGH, MOSI init : LOW */
				vivix_gpio_set_value(AED_SPI_CLK, LOW);
				vivix_gpio_set_value(AED_SPI_MOSI, LOW);
				vivix_gpio_set_value(AED_SPI_MISO, LOW);
			}
			else
			{
				g_b_aed_pwr_onoff = true;

				/* (APD1, 2 offset/bias potentiometer) SCLK, MOSI, SS IO direction 설정 : output */	
				vivix_gpio_direction(AED_SPI_CLK, OUTPUT);
				vivix_gpio_direction(AED_SPI_MOSI, OUTPUT);
				vivix_gpio_direction(AED_SPI_SS0, OUTPUT);
				vivix_gpio_direction(AED_SPI_SS1, OUTPUT);
				vivix_gpio_direction(AED_SPI_SS2, OUTPUT);
				vivix_gpio_direction(AED_SPI_SS3, OUTPUT);
				
				/* (APD1, 2 offset/bias potentiometer) MISO IO direction 설정 : input */
				vivix_gpio_direction(AED_SPI_MISO, INPUT);
				
				/* APD1, 2 offset/bias potentiometer chip select init : HIGH */
				vivix_gpio_set_value(AED_SPI_SS0, HIGH);
				vivix_gpio_set_value(AED_SPI_SS1, HIGH);
				vivix_gpio_set_value(AED_SPI_SS2, HIGH);
				vivix_gpio_set_value(AED_SPI_SS3, HIGH);
				
				/* (APD1, 2 offset/bias potentiometer) clock init : HIGH, MOSI init : LOW */
				vivix_gpio_set_value(AED_SPI_CLK, HIGH);
				vivix_gpio_set_value(AED_SPI_MOSI, LOW);
			}

			break;
		}
		case VW_MSG_IOCTL_GPIO_PMIC_RST:
		{
			// active low, low입력되면 재부팅되어 PMIC_RST_BIT가 1로 초기화 되기 때문에 다시 1로 set해주지 않음
			//vivix_gpio_set_value(PMIC_RST, LOW); //TO_DO
			break;
		}
		case VW_MSG_IOCTL_GPIO_PWR_OFF:
		{
			vivix_gpio_set_value(PWR_OFF, HIGH);
			break;
		}
		default:
		{
			printk("gpio:Unsupported IOCTL_Xxx (%02d)\n", _IOC_NR(cmd));
			break;
		}
	}
	mutex_unlock(&g_ioctl_mutex);
	n_ret = copy_to_user((ioctl_data_t*)args, pB, sizeof(ioctl_data_t));
	return 0;
}

static struct file_operations vivix_gpio_fops = {
	.read			= vivix_gpio_read,
	.write			= vivix_gpio_write,
	.poll			= vivix_gpio_poll,
	.open			= vivix_gpio_open,
	.release		= vivix_gpio_release,
	.unlocked_ioctl	= vivix_gpio_ioctl,
};

/***********************************************************************************************//**
* \brief		vivix_gpio_init
* \param	
* \return		
* \note			
***************************************************************************************************/
s32 vivix_gpio_init(void)
{
	int ret;
	struct device *dev_ret;
	u32 nrc;

	printk("\nVIVIX GPIO Linux Device Driver Loading ... \n");

	ret = alloc_chrdev_region(&vivix_gpio_dev, 0, 1, GPIO_DRIVER_NAME);
	if (ret < 0)
	{
		printk("%s[%d] alloc_chrdev_region error(%d)\n", __func__, __LINE__, ret);
		return ret;
	}

	cdev_init(&vivix_gpio_cdev, &vivix_gpio_fops);
	ret = cdev_add(&vivix_gpio_cdev, vivix_gpio_dev, 1);
	if (ret < 0)
	{
		printk("%s[%d] cdev_add error(%d)\n", __func__, __LINE__, ret);
		unregister_chrdev_region(vivix_gpio_dev, 1);
		return ret;
	}

	vivix_gpio_class = class_create(GPIO_DRIVER_NAME);
	if (IS_ERR(vivix_gpio_class))
	{
		cdev_del(&vivix_gpio_cdev);
		unregister_chrdev_region(vivix_gpio_dev, 1);
		printk("%s[%d] class_create error(%ld) \n", __func__, __LINE__, PTR_ERR(vivix_gpio_class));
		return PTR_ERR(vivix_gpio_class);
	}

	dev_ret = device_create(vivix_gpio_class, NULL, vivix_gpio_dev, NULL, GPIO_DRIVER_NAME);
	if (IS_ERR(dev_ret))
	{
		class_destroy(vivix_gpio_class);
		cdev_del(&vivix_gpio_cdev);
		unregister_chrdev_region(vivix_gpio_dev, 1);
		printk("%s[%d] class_create error(%ld) \n", __func__, __LINE__, PTR_ERR(dev_ret));
		return PTR_ERR(dev_ret);
	}

	gpio_init();
	
	nrc =  request_irq(g_n_impact_irq_num, impact_sensor_interrupt, IRQF_TRIGGER_RISING, "IMPACT_IRQ", NULL);
	if( nrc != 0 )
	{
		printk("\nVIVIX GPIO ERROR : IMPACT request_irq  ret value : %d\n",nrc);
	}
	
	return 0;
}

/***********************************************************************************************//**
* \brief		vivix_gpio_exit
* \param	
* \return		
* \note			
***************************************************************************************************/
void vivix_gpio_exit(void)
{
	printk("\nVIVIX GPIO Unloading Driver... \n");

	device_destroy(vivix_gpio_class, vivix_gpio_dev);
	class_destroy(vivix_gpio_class);
	cdev_del(&vivix_gpio_cdev);
	unregister_chrdev_region(vivix_gpio_dev, 1);

	free_irq(g_n_impact_irq_num,NULL);	
	
	return;
}


module_init(vivix_gpio_init);
module_exit(vivix_gpio_exit);


//MODULE_AUTHOR("vivix@vieworks.co.kr");
MODULE_LICENSE("Dual BSD/GPL");

/* end of driver.c */

