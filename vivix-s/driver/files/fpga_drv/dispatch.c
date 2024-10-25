
/***************************************************************************************************
*	Include Files
***************************************************************************************************/


#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/wait.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/page.h>
#include <asm/irq.h>
#include <linux/interrupt.h>  // Note: interrupt.h must be last to avoid compiler errors
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/gpio.h>


#include "api_functions.h"
#include "dispatch.h"
#include "driver.h"
#include "irq_numbers.h"
#include "vworks_ioctl.h"
#include "func_error.h"
#include "fpga_reg_def.h"
#include "isr_func.h"

extern int vivix_gpio_get_value(u32 gpio_num);
extern void vivix_gpio_set_value(u32 gpio_num, u8 a_b_level);

/***************************************************************************************************
*	Constant Definitionsg
***************************************************************************************************/

/***************************************************************************************************
*	Type Definitions
***************************************************************************************************/


/***************************************************************************************************
*	Macros (Inline Functions) Definitions
***************************************************************************************************/
#define     EXP_OK_BIT              (vivix_gpio_get_value(EXP_OK))
#define     FPGA_DONE_BIT           (vivix_gpio_get_value(FPGA_DONE))

/***************************************************************************************************
*	Variable Definitions
***************************************************************************************************/
extern line_buf_q_t*	g_p_queue_buf_t[10];
extern u32				g_n_num_of_buffer;
extern u32				g_n_write_buf_idx;
extern u32				g_n_read_buf_idx;
extern bool             g_b_write_error;

int						g_exp_section =0;
u16 					g_timeout;
extern u32				g_channel;
extern u32				g_tg_buf_size;
extern u32				g_tg_height;
extern u8				g_b_idle_power_mode;
extern u8				g_b_capt_power_mode;

static DEFINE_MUTEX(g_ioctl_mutex);

extern bool			    g_b_readout_done;

bool                    g_b_irq_enable=false;
extern bool             g_b_immediate_transfer;
extern u16              g_w_offset_error;

extern mmap_t*			g_p_cal_data_mmap_t;

extern u8				g_c_exp_ok;
extern u8				g_c_line_count;

extern u8 				g_c_tft_aed_enable;

/***************************************************************************************************
*	Function Prototypes
***************************************************************************************************/
int queue_size( line_buf_q_t * pq );

void	load_cal_start( u32 i_n_address );
void	load_cal_stop(void);

/***********************************************************************************************//**
* \brief		dispatch_open
* \param		
* \return		status
* \note
***************************************************************************************************/
int vivix_tg_open(struct inode *inode, struct file  *filp)
{

    return 0;
}

/***********************************************************************************************//**
* \brief		dispatch_release
* \param		
* \return		status
* \note
***************************************************************************************************/
int vivix_tg_release(struct inode *inode, struct file  *filp)
{
	return 0;
}

/***********************************************************************************************//**
* \brief		dispatch_read
* \param		buff - dst buffer
* \param		count - copy size
* \return		status
* \note
***************************************************************************************************/
ssize_t vivix_tg_read(struct file  *filp, char  *buff, size_t  count, loff_t  *offp)
{
	line_buf_q_t* p_read_q_t = g_p_queue_buf_t[g_n_read_buf_idx];
	u32 n_read_count = 0;
	u16 w_rear_count = 0;
	s32 result;

    if( g_b_write_error == true )
    {
        printk("[READ ERROR] %d\n", g_n_read_buf_idx );
        g_b_write_error = false;
        return -1;
    }
	
	if( p_read_q_t->nfront == 0 )
	{
	    printk("[READ ERROR] %d: no data to read(read: %d)\n", g_n_read_buf_idx, p_read_q_t->nrear );
	    return -1;
	}
	
	while( n_read_count < count )
	{
		result = copy_to_user(&buff[n_read_count], (u8 *)&p_read_q_t->pbuf_t[p_read_q_t->nrear + w_rear_count].pbuf[0], g_tg_buf_size);

		n_read_count += g_tg_buf_size;
		w_rear_count++;
	}
	//printk("[READ START] n_read_count: %d, count: %d, rear count: %d\n", n_read_count, count, w_rear_count );
	
	if( p_read_q_t->nrear == 0 )
	{
	    printk("[READ START] %d(front: %d)\n", g_n_read_buf_idx, p_read_q_t->nfront );
	}
	
	p_read_q_t->nrear += w_rear_count;
    	//if (++p_read_q_t->nrear >= g_tg_height)
    	if (p_read_q_t->nrear >= g_tg_height)
	{
		p_read_q_t->nfront = p_read_q_t->nrear = 0;
		
		printk("[READ COMPLETE] %d\n", g_n_read_buf_idx );
	}
	
	return count;
}

/***********************************************************************************************//**
* \brief		dispatch_poll
* \param		
* \return		status
* \note
***************************************************************************************************/
unsigned int vivix_tg_poll(struct file * filp,poll_table * wait)
{
	u32 mask=0x00;
	line_buf_q_t* p_read_q_t = g_p_queue_buf_t[g_n_read_buf_idx];
	int n_exp = 0;

	poll_wait(filp,&queue_read,wait);

	if(g_exp_section > 0)
	{
	    g_exp_section = 0;
	    mask |= POLLPRI ;
	    
	    n_exp = 1;
    }
	
	if(queue_size(p_read_q_t) > 0)
	{
	    mask |= POLLIN | POLLRDNORM;
	}

	return mask;
}

/***********************************************************************************************//**
* \brief		dispatch_mmap
* \param		
* \return		status
* \note
***************************************************************************************************/
/*
 * allocArea is the memory block that we allocate, of which s_p_memArea points to the
 * bit we actually use.
 */
static void *s_p_allocArea;
static void *s_p_memArea;
static int s_n_memLength;

static DEFINE_SPINLOCK(s_smemdrv_lock);
//static spinlock_t s_smemdrv_lock;

/*
 * Allocate the shared memory.  This is called directly by other kernel routines that access
 * the shared memory, and indirectly by user space via smemdrv_mmap.
 */
extern int smemdrv_get_memory(int length, void **pointer)
{
    /*
     * If the shared memory hasn't already been allocated, then allocate it.
     */
    if (!s_p_memArea) {
        void *newAllocArea;
        void *newMemArea;
        unsigned long irqState;
        int memSize;
        int i;
        
        /*
         * For the memory mapping, we'll need to allocate a whole number of pages, so we'll need to
         * round the size up.  Then we need it to start on a page boundary, and as we don't know
         * where kmalloc will put things, we need to add almost a whole extra page so that we can
         * be sure we can find an area of the correct length somewhere in the page.
         */
        length = (length + PAGE_SIZE + 1) & PAGE_MASK;
        memSize = length + PAGE_SIZE - 1;
        newAllocArea = kmalloc(memSize, GFP_KERNEL);
        if (!newAllocArea) {
            printk(KERN_CRIT "%s: Couldn't allocate memory to share\n", __func__);
            return(-ENOMEM);
        }
    
        /*
         * s_p_memArea is the page aligned area we actually use.
         */
        newMemArea = (void *)(((unsigned long)newAllocArea + PAGE_SIZE - 1) & PAGE_MASK);
        memset(newMemArea, 0, length);
        printk(KERN_INFO "%s: allocArea %p, s_p_memArea %p, size %d\n", __func__, newAllocArea, 
                newMemArea, memSize);
    
        /*
         * Mark the pages as reserved.
         */
        for (i = 0; i < memSize; i+= PAGE_SIZE) {
                SetPageReserved(virt_to_page(newMemArea + i));
        }
    
        /*
         * While we were doing that, it's possible that someone else has got in and already
         * allocated the memory.  So check for that, and if so discard the memory we allocated.
         */
        spin_lock_irqsave(&s_smemdrv_lock, irqState);
        if (s_p_memArea) {
            /*
             * Free the memory we just allocated.
             */
            spin_unlock_irqrestore(&s_smemdrv_lock, irqState);
            kfree(newAllocArea);
        } else {
            /*
             * We still need some memory, so save the stuff we have.
             */
            s_p_memArea = newMemArea;
            s_p_allocArea = newAllocArea;
            s_n_memLength = length;
            spin_unlock_irqrestore(&s_smemdrv_lock, irqState);
        }
    }

    /*
     * We now have an allocated area - check that it's the right length.
     */
    if (s_n_memLength < length) {
        printk(KERN_CRIT "%s: requested %d bytes, but already allocated %d\n", __func__,
                length, s_n_memLength);
        return(-EFBIG);
    }

    /*
     * If the user has requested a pointer to the memory area, supply it for them.
     */
    if (pointer) {
        *pointer = s_p_memArea;
    }

    return(0);
}

int	vivix_tg_mmap(struct file * filp,struct vm_area_struct * vma)
{
    unsigned long length = vma->vm_end - vma->vm_start;

    int ret = smemdrv_get_memory(length, NULL);
    if (ret != 0) {
        printk(KERN_CRIT "%s: Couldn't allocate shared memory for user space\n", __func__);
        return(ret);
    }
	
	g_p_cal_data_mmap_t = (mmap_t*)s_p_memArea;

	if(remap_pfn_range(vma,
					   vma->vm_start,
					   virt_to_phys((void *)s_p_memArea)  >> PAGE_SHIFT,
					   length,
					   vma->vm_page_prot))
	{
		printk("dispatch_mmap error\n");
		return -EAGAIN;
	}

	return 0;
}


/***********************************************************************************************//**
* \brief		dispatch_ioctl
* \param		
* \return		status
* \note
***************************************************************************************************/
long vivix_tg_ioctl(struct file  *filp, unsigned int  cmd, unsigned long  args)
{
	int	rval = -ENOTTY;  	
	ioctl_data_t	ioBuffer;
	ioctl_data_t	*pB;
	u16				data;
	u32				addr=0x00;
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
		case VW_MSG_IOCTL_TG_SET_MODEL_AND_INIT:			
			rval = tg_initialize(pB->data[0]);
			if(rval != 0)
				printk(KERN_INFO "TG set model and init failed %d.\n", rval);
			break;

		case VW_MSG_IOCTL_TG_REG_WRITE:
			memcpy(&addr,pB->data,2);
			memcpy(&data,&pB->data[2],2);
			tg_reg_write(addr,data);
			break;

		case VW_MSG_IOCTL_TG_REG_WRITE_DUMP:
		{
			u16 n_addr;
			u16 n_length;
			
			memcpy(&n_addr,pB->data,2);
			memcpy(&n_length,&pB->data[2],2);

			tg_reg_write_dump(n_addr/2,n_length/2,(u16*)&pB->data[4]);
			break;
		}

		case VW_MSG_IOCTL_TG_REG_READ:
			memcpy(&addr,pB->data,2);
			data = tg_reg_read(addr);
			memcpy(pB->data,&data,2);
			break;
        
		case VW_MSG_IOCTL_TG_CLEAR:
            {
                break;
            }
        
        case VW_MSG_IOCTL_TG_INTERRUPT_ENABLE:
        {
            if( pB->data[0] > 0 )
            {
                g_b_irq_enable = true;
            }
            else
            {
                g_b_irq_enable = false;
            }
            break;
        }
        
		case VW_MSG_IOCTL_TG_POWER_MODE:
        {
            g_b_idle_power_mode = pB->data[0];
        	g_b_capt_power_mode = pB->data[1];
        	break;
        }
        
		case VW_MSG_IOCTL_TG_QUEUE_BUFFER_SIZE:
    	{
    		u32 n_num_of_buff;
    		s32 n_status = 0;
    		u32	n_idx;
    		memcpy(&n_num_of_buff,pB->data,4);
    		
    		if( n_num_of_buff > 10 || n_num_of_buff < 1)
    		{
    			n_status = -1;
    			
    			memcpy(pB->data,&n_status,4);
    			break;
    		}
    		g_n_num_of_buffer = n_num_of_buff;
    		for( n_idx = 0 ; n_idx < n_num_of_buff ; n_idx++ )
    		{
	    		if(g_p_queue_buf_t[n_idx] == 0)
	    		{	    			
	    			n_status = vivix_tg_queue_buf_init(&g_p_queue_buf_t[n_idx]);
	    		}
	    	}    		
    		
    		memcpy(pB->data,&n_status,4);
    		break;
    	}
        
		case VW_MSG_IOCTL_TG_FPGA_CONFIG:
		{
			vivix_gpio_set_value(AEN_PIN, LOW); // active(pull-up)
			vivix_gpio_set_value(BEN_PIN, HIGH); // active(pull-down)
			udelay(1);

			vivix_gpio_set_value(AEN_PIN, HIGH); // inactive(pull-up)
			vivix_gpio_set_value(BEN_PIN, LOW); // inactive(pull-down)

			/* pgm(pull-up) active --> inactive */
			vivix_gpio_set_value(FPGA_PGM, LOW);
			udelay(1);
			vivix_gpio_set_value(FPGA_PGM, HIGH);

			break;
		}
		
		case VW_MSG_IOCTL_TG_FPGA_DONE:
			pB->data[0] = FPGA_DONE_BIT;
			break;
		
		case VW_MSG_IOCTL_TG_PWR_EN:
		{
			u8 b_en;
            u32* reg_addr;
            u32 reg_data;
            
            reg_addr = ioremap(0x020e0004, 4);
            reg_data = readl(reg_addr);
            //printk("io mux: read -> 0x%08X\n", reg_data);
			
			b_en = pB->data[0];
            
            if( b_en == 0 )
            {
               // off
               reg_data = reg_data & 0xFFFFFFFE;
            }
            else
            {
               // on
               reg_data = reg_data | 0x1;
            }
            
            //printk("io mux: write -> 0x%08X\n", reg_data);
	        writel(reg_data, reg_addr);
	        iounmap(reg_addr);
			vivix_gpio_set_value(TG_PWR_EN, b_en);

			break;
		}
		
		case VW_MSG_IOCTL_TG_READOUT_DONE:
		{
		    pB->data[0] = g_b_readout_done;
		    
		    memcpy( &pB->data[1], &g_w_offset_error, sizeof(g_w_offset_error) );
		    break;
		}
		
		case VW_MSG_IOCTL_TG_READOUT_DONE_CLEAR:
		{
		    g_b_readout_done = false;
		    
		    break;
		}
		
		case VW_MSG_IOCTL_TG_IMMEDIATE_TRANSFER:
		{
		    g_b_immediate_transfer = pB->data[0];
		    
		    break;
		}
		
		case VW_MSG_IOCTL_TG_TIME:
		{
		    u32 n_time;
		    
		    memcpy( &n_time, pB->data, sizeof(n_time) );
		    printk("VW_MSG_IOCTL_TG_TIME: %d\n", n_time);
		    print_current_time();
		    break;
		}
		
		case VW_MSG_IOCTL_TG_LOAD_CAL_DATA:
		{
		    if( pB->data[0] == 1 )
		    {
		    	u32 n_address;
		    	memcpy( &n_address, &pB->data[1], sizeof(n_address) );
		    	
		    	load_cal_start( n_address );
		    }
		    else
		    {
		    	load_cal_stop();
		    }
		    
		    break;
		}
		
		case VW_MSG_IOCTL_TG_EXP_REQ:
		{
			g_c_exp_ok = 0;
			vivix_gpio_set_value(EXP_REQ, pB->data[0]);
			
			break;
		}
		
		case VW_MSG_IOCTL_TG_EXP_OK:
		{
			pB->data[0] = EXP_OK_BIT;
			pB->data[1] = g_c_exp_ok;
			
			break;
		}
		
		case VW_MSG_IOCTL_TG_LINE_COUNT:
		{
		    g_c_line_count = pB->data[0];
			break;
		}
	
		default:
        {
            printk("fpga_drv:Unsupported IOCTL_Xxx (%02d)\n", _IOC_NR(cmd));
			pB->ret = ApiUnsupportedFunction;
			break;
		}
	}
	mutex_unlock(&g_ioctl_mutex);
	n_ret = copy_to_user((ioctl_data_t*)args, pB, sizeof(ioctl_data_t));
	return 0;
}


/* end of dispatch.c */


