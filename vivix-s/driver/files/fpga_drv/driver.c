/********************************************************************************  
 모  듈 : fpga driver
 작성자 : 서정한
 버  전 : 0.0.1
 설  명 : freescale, fpga driver 
 참  고 : 
 
          1. FPGA read, write, img data read 기능.
          
 버  전:
         0.0.1  linux 드라이버용으로 작성, freescale의 EIM SDMA 사용, 1012N 타겟

********************************************************************************/


/***************************************************************************************************
*	Include Files
***************************************************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>			// kmalloc,kfree
#include <linux/dma-mapping.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <asm/types.h>
#include <linux/delay.h>
#if 1//COMPILE_ERR
#include <linux/dma/imx-dma.h>
#else
#include <linux/platform_data/dma-imx.h>
#endif
#include <linux/cdev.h>
#include <linux/device.h>

#include <linux/dmaengine.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/platform_device.h> 
#include <linux/clk.h>

#include "driver.h"
#include "eim.h"
#include "fpga_reg_def.h"
#include "dispatch.h"
#include "driver_defs.h"
#include "soc_memory_map.h"
#include "api_functions.h"
#include "irq_numbers.h"
#include "isr_func.h"
#include "vworks_ioctl.h"




/***************************************************************************************************
*	Constant Definitionsg
***************************************************************************************************/
#define CCM_CSCMR1_ADDR			0x020C401C
#define IMX_GPIO_NR(bank, nr)   (((bank) - 1) * 32 + (nr))

/***************************************************************************************************
*	Type Definitions
***************************************************************************************************/
enum sdma_devtype {
	IMX31_SDMA,	/* runs on i.mx31 */
	IMX35_SDMA,	/* runs on i.mx35 and later */
};

typedef union _hw_ccm_cscmr1
{
    reg32_t ndata;
    struct _hw_ccm_cscmr1_bitfields
    {
        unsigned PERCLK_PODF : 6; //!< [5:0] Divider for perclk podf.
        unsigned RESERVED0 : 4; //!< [9:6] Reserved
        unsigned SSI1_CLK_SEL : 2; //!< [11:10] Selector for ssi1 clock multiplexer
        unsigned SSI2_CLK_SEL : 2; //!< [13:12] Selector for ssi2 clock multiplexer
        unsigned SSI3_CLK_SEL : 2; //!< [15:14] Selector for ssi3 clock multiplexer
        unsigned USDHC1_CLK_SEL : 1; //!< [16] Selector for usdhc1 clock multiplexer
        unsigned USDHC2_CLK_SEL : 1; //!< [17] Selector for usdhc2 clock multiplexer
        unsigned USDHC3_CLK_SEL : 1; //!< [18] Selector for usdhc3 clock multiplexer
        unsigned USDHC4_CLK_SEL : 1; //!< [19] Selector for usdhc4 clock multiplexer
        unsigned ACLK_PODF : 3; //!< [22:20] Divider for aclk clock root.
        unsigned ACLK_EIM_SLOW_PODF : 3; //!< [25:23] Divider for aclk_eim_slow clock root.
        unsigned RESERVED1 : 1; //!< [26] Reserved
        unsigned ACLK_SEL : 2; //!< [28:27] Selector for aclk root clock multiplexer
        unsigned ACLK_EIM_SLOW_SEL : 2; //!< [30:29] Selector for aclk_eim_slow root clock multiplexer
        unsigned RESERVED2 : 1; //!< [31] Reserved.
    } bit;
}hw_ccm_cscmr1_t;

/***************************************************************************************************
*	Macros (Inline Functions) Definitions
***************************************************************************************************/


/***************************************************************************************************
*	Variable Definitions
***************************************************************************************************/
static dev_t vivix_tg_dev;
static struct cdev vivix_tg_cdev;
static struct class *vivix_tg_class;

volatile bool					g_b_log_print = false;
line_buf_q_t*					g_p_queue_buf_t[10];
u32								g_n_num_of_buffer = 1;
u32								g_n_write_buf_idx = 0;
u32								g_n_read_buf_idx = 0;
bool                            g_b_write_error = false;
u16* 							g_pfpgaAddr;

u32								g_neim_memory_addr;

u32			 					g_nscript_addr = 0;

struct dma_chan *				g_pdma_m2m_chan;
struct dma_slave_config			g_dma_m2m_config_t;
struct dma_async_tx_descriptor*	g_pdma_m2m_desc;
struct scatterlist 				g_psg_src[1], g_psg_dst[1];

u32								g_tg_buf_size;
u32								g_tg_height;
u32								g_readout_req_irq_num;
u32								g_exp_irq_num;
u32								g_readout_done_irq_num;

bool			                g_b_readout_done;
TG_STATE    					g_tg_state_e;

// EXP 구간에서는 line interrupt disable 되고
// READOUT 시작에서 line interrupt를 enable(g_b_immediate_transfer==true)하거나
// READOUT 종료에서 line interrupt를 enable(g_b_immediate_transfer==false)한다.
bool                            g_b_immediate_transfer;
u16                             g_w_offset_error;

mmap_t*							g_p_cal_data_mmap_t;
u8								g_c_load_cal_result;
tg_buf_t*						g_p_line_tg_buf_t;

u8								g_c_exp_ok;		// 0: clear, 1: X-ray (Double Readout on), 2: Offset (Double Readout on)
												
u8								g_c_line_count;

extern volatile u8							g_c_bd_ver;
																					
exp_wait_queue_ctx	exposure_wait_queue_ctx;
DECLARE_WAIT_QUEUE_HEAD(queue_read);
DECLARE_WAIT_QUEUE_HEAD(exposure_wait_queue);
EXPORT_SYMBOL(exposure_wait_queue);
EXPORT_SYMBOL(exposure_wait_queue_ctx);

spinlock_t 						g_io_lock;
EXPORT_SYMBOL(g_io_lock);
/***************************************************************************************************
*	Function Prototypes
***************************************************************************************************/

/***********************************************************************************************//**
* \brief		EIM clk div
* \param		
* \return		status
* \note
***************************************************************************************************/
void ccm_eim_clk_div(void)
{
	u32*			p = ioremap(CCM_CSCMR1_ADDR, sizeof(u32));
	hw_ccm_cscmr1_t	ccm_cscmr1_reg;
	ccm_cscmr1_reg.ndata = *p;
	
	// divide by 1 000
	// divide by 2 001
	// divide by 3 010	==> 270/3 = 90MHz
	// divide by 6 101	==> 45MHz
	// divide by 8 111
#if 1
#define DIVIDE_BY_6	5
	ccm_cscmr1_reg.bit.ACLK_EIM_SLOW_PODF = DIVIDE_BY_6;
#undef 	DIVIDE_BY_3
#else
#define DIVIDE_BY_3	2
	ccm_cscmr1_reg.bit.ACLK_EIM_SLOW_PODF = DIVIDE_BY_3;
#undef 	DIVIDE_BY_3
#endif
	*p = ccm_cscmr1_reg.ndata;	
	iounmap(p);
}
	
/***********************************************************************************************//**
* \brief		EIM 메모리 설정
* \param		
* \return		status
* \note
***************************************************************************************************/
static void eim_hw_prepare(void)
{
	/* EIM CLK div */
	ccm_eim_clk_div();
	
	/* Init EIM */
    eim_init(EIM_CS0, DSZ_16_LOW, true, false);
    
    /* Burst Clock Start */
    eim_cfg_set(EIM_CS0, GCR1_BCS, 0);

    /* Bust Colck Divisor */
    eim_cfg_set(EIM_CS0, GCR1_BCD, 0);

    /* Read Fix Latency */
    eim_cfg_set(EIM_CS0, GCR1_RFL, true);

    /* Write Fix Latency */
    eim_cfg_set(EIM_CS0, GCR1_WFL, true);

    /* Multiplexed Mode(address/data multiplexed) */
    eim_cfg_set(EIM_CS0, GCR1_MUM, true);

    /* Synchronous Read Data */
    eim_cfg_set(EIM_CS0, GCR1_SDR, 1);

    /* Synchronous Write Data */
    eim_cfg_set(EIM_CS0, GCR1_SWR, 1);
    
    eim_cfg_set(EIM_CS0, GCR1_GBC, 1);
    
    eim_cfg_set(EIM_CS0, GCR1_CSREC, 1);
    
    eim_cfg_set(EIM_CS0, GCR1_BL, 3);
    
	eim_cfg_set(EIM_CS0, GCR1_WC, 0);
    /* Adress Hold Time */
    eim_cfg_set(EIM_CS0, GCR2_ADH, 1);

    /* Read Wait State Control */
    //eim_cfg_set(EIM_CS0, RCR1_RWSC, 6);
    eim_cfg_set(EIM_CS0, RCR1_RWSC, 7);

    /* ADV Negation */
    eim_cfg_set(EIM_CS0, RCR1_RADVN, 1);
    eim_cfg_set(EIM_CS0, RCR1_RADVA, 0);
    
    /* OE Assertion */
    eim_cfg_set(EIM_CS0, RCR1_OEA, 0);
    
    /* OE Negation */
    eim_cfg_set(EIM_CS0, RCR1_OEN, 0);

    /* Write Wait State Control */
    eim_cfg_set(EIM_CS0, WCR1_WWSC, 4);
    
    /* ADV Negation */
    eim_cfg_set(EIM_CS0, WCR1_WADVN, 1);    
    
    /* BE Assertion */
    eim_cfg_set(EIM_CS0, WCR1_WBEA, 0);    
    
    /* BE Negation */
    eim_cfg_set(EIM_CS0, WCR1_WBEN, 0);    
    
    /* Write CS Assertion */
    eim_cfg_set(EIM_CS0, WCR1_WCSA, 0);    
    
    /* WE Assertion */
    eim_cfg_set(EIM_CS0, WCR1_WEA, 0);    
    
    /* WE Negation */
    eim_cfg_set(EIM_CS0, WCR1_WEN, 0);    
    
    eim_cfg_set(EIM_CS0, WCR_BCM, 1);    
    
    eim_cfg_set(EIM_CS0, WCR_GBCD, 0);
    
    eim_cfg_set(EIM_CS0, WCR_CON_BLK_SEL, 1);
    
}

/***********************************************************************************************//**
* \brief		GPIO input 및 intr설정
* \param		
* \return		status
* \note
***************************************************************************************************/
void gpio_init(void)
{
	/* irq num 설정 */
#if 0//TO_DO
	g_readout_req_irq_num = gpio_to_irq(IMX_GPIO_NR(GPIO6,F_INF0_PORT));	
	g_exp_irq_num =  gpio_to_irq(IMX_GPIO_NR(GPIO6,F_INF1_PORT));
	g_readout_done_irq_num = gpio_to_irq(IMX_GPIO_NR(GPIO6,F_INF2_PORT));
#endif
	
}

/***********************************************************************************************//**
* \brief		dma_m2m_filter
* \param		
* \return		status
* \note
***************************************************************************************************/
static bool dma_m2m_filter(struct dma_chan *chan, void *param)
{
	if (!imx_dma_is_general_purpose(chan))
		return false;
	chan->private = param;
	return true;
}

/***********************************************************************************************//**
* \brief		sdma 초기설정
* \param		
* \return		status
* \note
***************************************************************************************************/
void sdma_init(void)
{	
	dma_cap_mask_t dma_m2m_mask;
	struct imx_dma_data m2m_dma_data = {0};
	
	dma_cap_zero(dma_m2m_mask);
	dma_cap_set(DMA_SLAVE, dma_m2m_mask);
	
	m2m_dma_data.peripheral_type = IMX_DMATYPE_MEMORY;
	m2m_dma_data.priority = DMA_PRIO_HIGH;
					   
	g_pdma_m2m_chan = dma_request_channel(dma_m2m_mask, dma_m2m_filter, &m2m_dma_data);
	if (!g_pdma_m2m_chan) {
		printk("Error opening the SDMA memory to memory channel\n");
		return;
	}
	
	g_dma_m2m_config_t.direction = DMA_MEM_TO_MEM;
	g_dma_m2m_config_t.dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;	
	dmaengine_slave_config(g_pdma_m2m_chan, &g_dma_m2m_config_t);
	
	sg_init_table(g_psg_src, 1);
	
	g_psg_src[0].page_link = (0x03 & g_psg_src[0].page_link) | (unsigned long)virt_to_page(g_neim_memory_addr);
	g_psg_src[0].offset = offset_in_page(g_neim_memory_addr);
	g_psg_src[0].length = g_tg_buf_size;
	g_psg_src[0].dma_address = WEIM_CS_BASE_ADDR + FPGA_READ_BUF_ADDR*sizeof(u16);	
	
	sg_init_table(g_psg_dst, 1);

}

/***********************************************************************************************//**
* \brief		queue buffer 초기화
* \param		
* \return		status
* \note
***************************************************************************************************/
s32 vivix_tg_queue_buf_init(line_buf_q_t **a_p_queue_t)
{	
	void*		img_buf_virt;
	dma_addr_t 	img_phy_t;
	s32	 		n_idx;
	line_buf_q_t*	p_queue_t;
	
	p_queue_t 	= kmalloc(sizeof(line_buf_q_t),GFP_KERNEL);
	if(p_queue_t == 0)
	{
		printk("kmalloc failed\n");
		return -ENOMEM;
	}
	
	p_queue_t->pbuf_t = kmalloc(g_tg_height*sizeof(tg_buf_t),GFP_KERNEL);
	if(p_queue_t->pbuf_t == 0)
	{
		printk("kmalloc failed\n");
		return -ENOMEM;
	}
	

	// buffer init
	p_queue_t->nfront = p_queue_t->nrear= 0;
	
	
	img_buf_virt = dma_alloc_coherent(NULL, g_tg_buf_size * g_tg_height,&img_phy_t, GFP_KERNEL);	
	if(!img_buf_virt)
	{
		printk("large buffer allocation failed...\n"); 
		for(n_idx = 0 ; n_idx < g_tg_height ; n_idx++)
		{
			img_buf_virt = dma_alloc_coherent(NULL, g_tg_buf_size ,&img_phy_t, GFP_KERNEL);	
			if(!img_buf_virt)
			{
				printk("dma_alloc_coherent failed\n");
				return -ENOMEM;
			}
			
			p_queue_t->pbuf_t[n_idx].pbuf = (u8*)img_buf_virt;
			p_queue_t->pbuf_t[n_idx].paddr = (u32*)img_phy_t;
			
			if(p_queue_t->pbuf_t[n_idx].pbuf == NULL)
			{
				printk("buffer alloc error %d->%x\n",n_idx,(u32)p_queue_t->pbuf_t[n_idx].pbuf);
				return -ENOMEM;
			}
		}

	}
	else
	{
	
		for(n_idx = 0 ; n_idx < g_tg_height ; n_idx++)
		{
			p_queue_t->pbuf_t[n_idx].pbuf = (u8*)((u32)img_buf_virt + n_idx * g_tg_buf_size);
			p_queue_t->pbuf_t[n_idx].paddr = (u32*)((u32)img_phy_t + (n_idx * g_tg_buf_size));
			
			if(p_queue_t->pbuf_t[n_idx].pbuf == NULL)
			{
				printk("buffer alloc error %d->%x\n",n_idx,(u32)p_queue_t->pbuf_t[n_idx].pbuf);
				return -ENOMEM;
			}
		}
	}
	*a_p_queue_t = p_queue_t;
	return 0;	
}

/***********************************************************************************************//**
* \brief		queue buffer 초기화
* \param		
* \return		status
* \note
***************************************************************************************************/
s32 vivix_tg_line_buf_init(tg_buf_t **a_p_tg_buf_t)
{	
	void*		img_buf_virt;
	dma_addr_t 	img_phy_t;
	tg_buf_t*	p_tg_buf_t;
	
	p_tg_buf_t 	= kmalloc(sizeof(tg_buf_t),GFP_KERNEL);
	if(p_tg_buf_t == 0)
	{
		printk("kmalloc failed\n");
		return -ENOMEM;
	}
	
	img_buf_virt = dma_alloc_coherent(NULL, g_tg_buf_size, &img_phy_t, GFP_KERNEL);	
	
	p_tg_buf_t->pbuf = (u8*)((u32)img_buf_virt);
	p_tg_buf_t->paddr = (u32*)((u32)img_phy_t);

	*a_p_tg_buf_t = p_tg_buf_t;
	
	return 0;	
}

/***********************************************************************************************//**
* \brief		fpga 에 모델 설정 및 모델별 queue buffer, roic관련 변수 초기화
* \param		
* \return		
* \note
***************************************************************************************************/
int tg_initialize(u8 i_c_model)
{
	u32			n_idx;

	if(i_c_model == eBD_2530VW)
	{
		g_tg_buf_size = VIVIX_2530VW_BUF_SIZE;
		g_tg_height = VIVIX_2530VW_HEIGHT;
	}
	else if(i_c_model == eBD_3643VW)
	{
		g_tg_buf_size = VIVIX_3643VW_BUF_SIZE;
		g_tg_height = VIVIX_3643VW_HEIGHT;
	}
	else if(i_c_model == eBD_4343VW)
	{
		g_tg_buf_size = VIVIX_4343VW_BUF_SIZE;
		g_tg_height = VIVIX_4343VW_HEIGHT;
	}
	else
	{
		printk("unknown model type : %d \n",i_c_model);
	}
	
	/* queue buffer init */	
	memset(g_p_queue_buf_t,0,sizeof(g_p_queue_buf_t));
	
	for( n_idx = 0; n_idx < g_n_num_of_buffer; n_idx++ )
	{
		vivix_tg_queue_buf_init(&g_p_queue_buf_t[n_idx]);
	}
	
	vivix_tg_line_buf_init( &g_p_line_tg_buf_t );
		
	/* phy to vir addr for eim memory */
	// eim memory addr : WEIM_CS_BASE_ADDR 0x08000000
	// eim memory size : FPGA_BUF_ADDR 4096
	g_neim_memory_addr =  (u32)ioremap(WEIM_CS_BASE_ADDR,0x40000);
	if(g_neim_memory_addr == 0)
	{
		printk("g_neim_memory_addr error\n");
		return -ENOMEM;
	}
	g_pfpgaAddr = (u16*)g_neim_memory_addr;
	
	/* fpga interrupt setting */
	//	reg_u.INT.BIT.RDY_PUT = true;
	//	reg_u.INT.BIT.ERR_ACT = true;
	//	reg_u.INT.BIT.ERR_PAS = true;
	//	reg_u.INT.BIT.ERR_NON = true;
	//	reg_u.INT.BIT.ERR_CAP = true;
	//	reg_u.INT.BIT.ERR_TRIG = true;
	//	reg_u.INT.BIT.EXP_START = true;
	//	reg_u.INT.BIT.READ_DONE = true;	
		
	//tg_reg_write(FPGA_TCON_INT_MASK,reg_u.INT.DATA);
	
	/* sdma */
	sdma_init();	

	// 디텍터 모델 입력 및 기타 register 설정
	tg_reg_write(FPGA_DETECTOR_MODEL_SEL, i_c_model);

	return 0;
		
}

static struct file_operations vivix_tg_fops = {
    .read           = vivix_tg_read,
    .poll           = vivix_tg_poll,
    .open           = vivix_tg_open,
    .release        = vivix_tg_release,
    .unlocked_ioctl = vivix_tg_ioctl,
	.mmap			= vivix_tg_mmap,
};

/***********************************************************************************************//**
* \brief		모듈 초기화
* \param		
* \return		status
* \note
***************************************************************************************************/
static int vivix_tg_init(void)
{
	u32 		nrc;
	int ret;
	struct device *dev_ret;
	
	printk("\nVIVIX TG SMI  Linux Device Driver Loading ... \n");

#if 0// TO_DO
	/* eim init */
	eim_hw_prepare();

	/* gpio init */
	gpio_init();
#endif

	/* 변수 초기화 */	
	g_b_readout_done = false;
    g_tg_state_e = eTG_STATE_FLUSH;
    g_b_immediate_transfer = false;
    g_w_offset_error = 0;
    
    g_c_exp_ok = 0;
    g_c_line_count = 1;    
    
    spin_lock_init(&exposure_wait_queue_ctx.evt_thread_lock);
	spin_lock_init(&g_io_lock);
    exposure_wait_queue_ctx.evt_wait_cond = 0; 

	ret = alloc_chrdev_region(&vivix_tg_dev, 0, 1, TG_DRIVER_NAME);
    if (ret < 0)
    {
        printk("%s[%d] alloc_chrdev_region error(%d)\n", __func__, __LINE__, ret);
        return ret;
    }

    cdev_init(&vivix_tg_cdev, &vivix_tg_fops);
    ret = cdev_add(&vivix_tg_cdev, vivix_tg_dev, 1);
    if (ret < 0)
    {
        printk("%s[%d] cdev_add error(%d)\n", __func__, __LINE__, ret);
        unregister_chrdev_region(vivix_tg_dev, 1);
        return ret;
    }

    vivix_tg_class = class_create(TG_DRIVER_NAME);
    if (IS_ERR(vivix_tg_class))
    {
        cdev_del(&vivix_tg_cdev);
        unregister_chrdev_region(vivix_tg_dev, 1);
        printk("%s[%d] class_create error(%ld) \n", __func__, __LINE__, PTR_ERR(vivix_tg_class));
        return PTR_ERR(vivix_tg_class);
    }

    dev_ret = device_create(vivix_tg_class, NULL, vivix_tg_dev, NULL, TG_DRIVER_NAME);
    if (IS_ERR(dev_ret))
    {
        class_destroy(vivix_tg_class);
        cdev_del(&vivix_tg_cdev);
        unregister_chrdev_region(vivix_tg_dev, 1);
        printk("%s[%d] class_create error(%ld) \n", __func__, __LINE__, PTR_ERR(dev_ret));
        return PTR_ERR(dev_ret);
    }




#if 0 //TO_DO	
	nrc =  request_irq(g_readout_req_irq_num,vivix_tg_interrupt,IRQF_TRIGGER_RISING, "TG_IRQ",NULL);
	if( nrc != 0 )
	{
		printk("\nVIVIX TG ERROR : request_irq  ret value : %d\n",nrc);
		return -EBADR;
	}
	
	nrc =  request_irq(g_exp_irq_num,vivix_exp_interrupt,IRQF_TRIGGER_RISING, "EXP_IRQ",NULL);
	if( nrc != 0 )
	{
		printk("\nVIVIX TG ERROR : request_irq  ret value : %d\n",nrc);
		return -EBADR;
	}
	
	nrc =  request_irq(g_readout_done_irq_num,vivix_readout_done_interrupt,IRQF_TRIGGER_RISING, "EXP_DONE_IRQ",NULL);
	if( nrc != 0 )
	{
		printk("\nVIVIX TG ERROR : request_irq  ret value : %d\n",nrc);
		return -EBADR;
	}
#endif
		
	return 0;
}

/***********************************************************************************************//**
* \brief		모듈 해제
* \param		
* \return		status
* \note
***************************************************************************************************/
static void vivix_tg_exit(void)
{
	u32 n_idx;
	printk("\nVIVIX TG SMI  Unloading Driver...\n");
	
	device_destroy(vivix_tg_class, vivix_tg_dev);
	class_destroy(vivix_tg_class);
	cdev_del(&vivix_tg_cdev);
	unregister_chrdev_region(vivix_tg_dev, 1);

#if 0// TO_DO
	free_irq(g_readout_req_irq_num,NULL);	
	free_irq(g_exp_irq_num,NULL);	

	iounmap((void*)g_neim_memory_addr);

	//dma_free_coherent(NULL,g_tg_buf_size * g_tg_height,g_img_buf_virt,g_img_phy_t);
	for ( n_idx = 0 ; n_idx < g_n_num_of_buffer ; n_idx++)
	{
		if(g_p_queue_buf_t[n_idx] != 0)
			dma_free_coherent(NULL,g_tg_buf_size * g_tg_height,g_p_queue_buf_t[n_idx]->pbuf_t[0].pbuf
										,(dma_addr_t)g_p_queue_buf_t[n_idx]->pbuf_t[0].paddr);	
										
		kfree(g_p_queue_buf_t[n_idx]);										
	}	
	

	dmaengine_terminate_all(g_pdma_m2m_chan);
	dma_release_channel(g_pdma_m2m_chan);
	g_pdma_m2m_chan = NULL;
#endif
}

module_init(vivix_tg_init);
module_exit(vivix_tg_exit);

MODULE_AUTHOR("vivix@vieworks.co.kr");
MODULE_LICENSE("Dual BSD/GPL");
