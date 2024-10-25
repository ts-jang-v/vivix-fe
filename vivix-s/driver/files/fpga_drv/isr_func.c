/********************************************************************************  
 모  듈 : isr function
 작성자 : 서정한
 버  전 : 0.0.1
 설  명 : interrupt발생시 동작 
 참  고 : 
 
          1. SDMA 사용해서 data 복사및 관리
          
 버  전:
         0.0.1  fpga driver 에서 호출 및 사용

********************************************************************************/

/***************************************************************************************************
*	Include Files
***************************************************************************************************/
#include <asm/types.h>

#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#if 1//COMPILE_ERR
#include <linux/dma/imx-dma.h>
#else
#include <linux/platform_data/dma-imx.h>
#endif
#include <linux/gpio.h>
#include <linux/kthread.h>

#include "driver.h"
#include "api_functions.h"
#include "fpga_reg_def.h"
#include "irq_numbers.h"
#include "isr_func.h"
#include "soc_memory_map.h"
#include "vworks_ioctl.h"


/***************************************************************************************************
*	Constant Definitions
***************************************************************************************************/

#define ROIC_POWER_MODE_IDLE	5
//#define ROIC_POWER_MODE_EXPOSER	4
#define ROIC_POWER_MODE_EXPOSER	0

#define VDD_CORE_ENABLE			(5)		// VDD_CNT GPIO1_IO05	output

/***************************************************************************************************
*	Type Definitions
***************************************************************************************************/

/***************************************************************************************************
*	Macros (Inline Functions) Definitions
***************************************************************************************************/


/***************************************************************************************************
*	Variable Definitions
***************************************************************************************************/
extern line_buf_q_t*					g_p_queue_buf_t[10];
extern u32								g_n_num_of_buffer;
extern u32								g_n_write_buf_idx;
extern u32								g_n_read_buf_idx;
extern bool                             g_b_write_error;

u16										g_INT_STATUS = 0;
u8										g_b_idle_power_mode = ROIC_POWER_MODE_IDLE;
u8										g_b_capt_power_mode = ROIC_POWER_MODE_EXPOSER;

extern int 								g_exp_section;

extern struct scatterlist 				g_psg_src[1], g_psg_dst[1];
extern u32								g_neim_memory_addr;
extern struct dma_chan*					g_pdma_m2m_chan;
extern struct dma_slave_config 			g_dma_m2m_config_t;
extern struct dma_async_tx_descriptor*	g_pdma_m2m_desc;

extern u32								g_tg_buf_size;
extern u32								g_tg_height;

extern u32								g_exp_irq_num;

extern TG_STATE    					    g_tg_state_e;

extern bool			                    g_b_readout_done;
extern bool                             g_b_immediate_transfer;
extern u16                              g_w_offset_error;

extern bool                             g_b_irq_enable;

extern mmap_t*			g_p_cal_data_mmap_t;
extern tg_buf_t*		g_p_line_tg_buf_t;
u8						g_c_transfer_start;

extern u8								g_c_exp_ok;
extern u8								g_c_line_count;
extern bool							g_b_bat_charge_blocked;
extern bool							g_b_bat_charge_full;

extern exp_wait_queue_ctx				exposure_wait_queue_ctx;
extern wait_queue_head_t				exposure_wait_queue;

char g_p_state_print_buf[8] = {	0,};
/***************************************************************************************************
*	Function Prototypes
***************************************************************************************************/
extern void vivix_gpio_set_value(u32 gpio_num, u8 a_b_level);

static void queue_set_error(void);
static void queue_clear(void);

void	load_cal_start( u32 i_n_address );
void	load_cal_stop(void);
void	load_cal_transfer(void);
static void load_cal_transfer_done(void *a_pdata);

static TG_STATE get_tg_state(void) { return g_tg_state_e; }
static void set_tg_state(TG_STATE i_tg_state_e) { g_tg_state_e = i_tg_state_e; }
static char *print_tg_state(TG_STATE i_tg_state_e);

/***********************************************************************************************//**
* \brief		get queue_size
* \param		
* \return		status
* \note
***************************************************************************************************/
int queue_size( line_buf_q_t * pq )
{
	if( pq->nrear <= pq->nfront )
	{
		return( pq->nfront - pq->nrear );
	}
	return 1;
}

/***********************************************************************************************//**
* \brief		vivix_tg_line_read_end signal
* \param		
* \return		status
* \note
***************************************************************************************************/
static void vivix_tg_line_read_end(void)
{
	tg_reg_write(FPGA_LINE_READ_END,1);
	//udelay(1);
	tg_reg_write(FPGA_LINE_READ_END,0);
}

/***********************************************************************************************//**
* \brief		dma_done interrupt
* \param		
* \return		status
* \note
***************************************************************************************************/
static void dma_done(void *a_pdata)
{
	line_buf_q_t* p_write_q_t;
	//disable_irq_nosync(g_exp_irq_num);

	if( tg_reg_read( FPGA_LINE_INTR_DISABLE ) == 1 )
	{
	    print_current_time();
	    printk("capture cancel %d : %d, %d\n",g_n_write_buf_idx,tg_reg_read(FPGA_GET_LINE_COUNT),tg_reg_read(FPGA_PUT_LINE_COUNT));
    	
    	enable_irq(g_exp_irq_num);
    	enable_irq(g_readout_req_irq_num);
    	vivix_tg_line_read_end();
	    
	    return;
	}
	
	p_write_q_t = g_p_queue_buf_t[g_n_write_buf_idx];

	//p_write_q_t->nfront++;
	p_write_q_t->nfront += g_c_line_count;
	if(p_write_q_t->nfront >= g_tg_height)	//1장의 영상 수집 완료를 확인하는 조건 
	{
	    print_current_time();
		printk("capture end %d-> %d, %d : %d, %d\n",g_n_write_buf_idx,p_write_q_t->nfront,p_write_q_t->nrear,tg_reg_read(FPGA_GET_LINE_COUNT),tg_reg_read(FPGA_PUT_LINE_COUNT));
		
		if( ++g_n_write_buf_idx >= g_n_num_of_buffer )
		{
			g_n_write_buf_idx = 0;
		}

		tg_reg_write( FPGA_LINE_INTR_DISABLE, 1 );		//line ready interrupt block (transfer disable)

		set_tg_state(eTG_STATE_FLUSH);
	}
	else if( p_write_q_t->nfront == g_c_line_count )
	{
	    print_current_time();
	    printk("capture start %d : %d, %d\n",g_n_write_buf_idx,tg_reg_read(FPGA_GET_LINE_COUNT),tg_reg_read(FPGA_PUT_LINE_COUNT));
	}
	enable_irq(g_exp_irq_num);
	enable_irq(g_readout_req_irq_num);
	vivix_tg_line_read_end();
	
	//printk("get: %d, put: %d\n",tg_reg_read(FPGA_GET_LINE_COUNT),tg_reg_read(FPGA_PUT_LINE_COUNT));
}

/***********************************************************************************************//**
* \brief		dma_start
* \param		
* \return		status
* \note
***************************************************************************************************/
void dma_start(tg_buf_t* a_pb, u16 a_cnt_line)
{
	g_psg_dst[0].page_link = (0x03 & g_psg_dst[0].page_link) | (unsigned long)virt_to_page(a_pb->pbuf);
	g_psg_dst[0].offset = offset_in_page(a_pb->pbuf);
	g_psg_dst[0].length = g_tg_buf_size * a_cnt_line;
	g_psg_dst[0].dma_address = (dma_addr_t)a_pb->paddr;
	
	g_psg_src[0].dma_address = WEIM_CS_BASE_ADDR + FPGA_READ_BUF_ADDR*sizeof(u16);
	g_psg_src[0].length = g_tg_buf_size * a_cnt_line;
#if 0//COMPILE_ERR	
	g_pdma_m2m_desc = g_pdma_m2m_chan->device->
			device_prep_dma_sg(g_pdma_m2m_chan, g_psg_dst, 1, g_psg_src, 1, 0);
#endif
	if(g_pdma_m2m_desc != NULL)
	{
		g_pdma_m2m_desc->callback = dma_done;	
		dmaengine_submit(g_pdma_m2m_desc);
		dma_async_issue_pending(g_pdma_m2m_chan);
	}
}

/***********************************************************************************************//**
* \brief		vivix interrupt 수행 rising edge
* \param		
* \return		status
* \note			영상 데이터 획득 
***************************************************************************************************/
irqreturn_t  vivix_tg_interrupt(s32 a_nirq, void *a_pdev_id)
{
	line_buf_q_t* p_write_q_t;
	u16 w_interrupt_disable;
	u16 w_line_count;
	
	disable_irq_nosync(a_nirq);
	disable_irq_nosync(g_exp_irq_num);
	
	if( g_b_irq_enable == false )
	{
		vivix_tg_line_read_end();
	    enable_irq(a_nirq);
	    enable_irq(g_exp_irq_num);
	    
	    return IRQ_HANDLED;
	}
	
	w_interrupt_disable = tg_reg_read( FPGA_LINE_INTR_DISABLE );
	
	if( w_interrupt_disable == 1 )
	{
	    printk("[LINEREAD] invalid interrupt\n" );
	    
	    vivix_tg_line_read_end();
	    enable_irq(a_nirq);
	    enable_irq(g_exp_irq_num);
	    
	    return IRQ_HANDLED;
	}
	
	p_write_q_t = g_p_queue_buf_t[g_n_write_buf_idx];

    if (get_tg_state() == eTG_STATE_FLUSH)
	{
		printk("[LINEREAD(FLUSH)] %d:%d\n", g_n_write_buf_idx , p_write_q_t->nfront);
		
		vivix_tg_line_read_end();
		enable_irq(a_nirq);
	    enable_irq(g_exp_irq_num);
		
		return	IRQ_HANDLED;
    }
    
    //cntLine = g_c_line_count;
    w_line_count = g_tg_height - p_write_q_t->nfront;
    if( w_line_count >= g_c_line_count )
    {
    	w_line_count = g_c_line_count;
    }
    
    if( (p_write_q_t->nfront + w_line_count) > g_tg_height)
    {
        printk("Buffer overflow!(Q front: %d, cntLine: %d)\n", p_write_q_t->nfront, w_line_count); 
    }
    else
    {
        dma_start(&p_write_q_t->pbuf_t[p_write_q_t->nfront], (u8)w_line_count);        
    }

    return	IRQ_HANDLED;
}

/***********************************************************************************************//**
* \brief		vivix exp interrupt 수행 rising edge
* \param		
* \return		status
* \note
***************************************************************************************************/
irqreturn_t  vivix_exp_interrupt(s32 a_nirq, void *a_pdev_id)
{
	fpga_reg_u reg_u;
	fpga_reg_u src_reg_u;
	unsigned long irqflags;
	
	disable_irq_nosync(a_nirq);
	
	if( g_b_irq_enable == false )
	{
	    enable_irq(a_nirq);
	    return IRQ_HANDLED;
	}
	
	src_reg_u.TRIGGER_SRC.DATA = tg_reg_read(TCON_TRIGGER_SRC);
	reg_u.TCON_STATE.DATA = tg_reg_read(FPGA_TCON_STATE);
	
	/* FPGA가 영상 Readout 중에 F/W가 영상 읽어가면 noise끼는 문제가 있어 line interrupt disable */ 
	tg_reg_write( FPGA_LINE_INTR_DISABLE, 1 );
	
	if( src_reg_u.TRIGGER_SRC.BIT.OFFSET == 1 )
	{
	    g_b_write_error = false;
	}
	
	print_current_time();
	
	if( reg_u.TCON_STATE.BIT.EXPOSURE == 0 )
	{
		printk("[EXPOSURE(%s)] invalid interrupt(0x%04X/0x%04X)\n", print_tg_state(get_tg_state()), reg_u.TCON_STATE.DATA, src_reg_u.TRIGGER_SRC.DATA);
		enable_irq(a_nirq);
	    return IRQ_HANDLED;
	}

	g_w_offset_error = 0;
	
	//bat가 100% 찬 게 아니라면 bat_charge_block 진행
	if(g_b_bat_charge_full == false)
	{
		g_b_bat_charge_blocked = true;
	}

	// 촬영 시 roic power up 이전에 배터리 충전회로 끄도록 설정
	if( g_b_bat_charge_blocked == true )
	{
		vivix_gpio_set_value(B_CHG_EN_A, LOW);
		vivix_gpio_set_value(B_CHG_EN_B, LOW);
	}

	//하기 코드 실행 이후, roic_powerup_thread에서 exit_low_power_mode() 수행 
	//촬영 시작 시점 후, 하기 roic power up 수행 전, ROIC MCLK 신호 생성이 시작 되어야함. (MCLK 생성 이후, CLK 안정화 혹은 chip 동작 안정화까지 일정시간 대기는 필요 없는지?)
	tg_reg_write( FPGA_ROIC_CLK_EN, 1 );	//CLK ENABLE 
	spin_lock_irqsave(&exposure_wait_queue_ctx.evt_thread_lock, irqflags);
    exposure_wait_queue_ctx.evt_wait_cond = 1;
    spin_unlock_irqrestore(&exposure_wait_queue_ctx.evt_thread_lock, irqflags);
    wake_up_interruptible(&exposure_wait_queue);
    
    
	if( get_tg_state() != eTG_STATE_FLUSH )
	{
	    queue_set_error();
	}
	
	queue_clear();

	printk("[EXPOSURE(%s-%d/%d)] 0x%04X/0x%04X(0x%04X)\n", print_tg_state(get_tg_state()), g_n_read_buf_idx, g_n_write_buf_idx, reg_u.TCON_STATE.DATA, src_reg_u.TRIGGER_SRC.DATA, tg_reg_read(FPGA_CONFIG_REG0));

	set_tg_state(eTG_STATE_EXPOSURE);

	// VDD core disable
	vivix_gpio_set_value(VDD_CORE_ENABLE, LOW);
	
	if( src_reg_u.TRIGGER_SRC.BIT.XRAY_FRAME == 1 )
	{
		if( src_reg_u.TRIGGER_SRC.BIT.OFFSET == 0 )
		{
			g_c_exp_ok = 1;
		}
		else
		{
			g_c_exp_ok = 2;
		}
	}

	if( src_reg_u.TRIGGER_SRC.BIT.TRANS_FRAME == 1
		&& src_reg_u.TRIGGER_SRC.BIT.OFFSET == 0 )
	{
		g_exp_section  = 1;
		wake_up_interruptible(&queue_read);	
	}
	
	enable_irq(a_nirq);
	return	IRQ_HANDLED;
}

/***********************************************************************************************//**
* \brief		vivix readout done interrupt 수행 rising edge
* \param		
* \return		status
* \note
***************************************************************************************************/
irqreturn_t  vivix_readout_done_interrupt(s32 a_nirq, void *a_pdev_id)
{
	/* readout start, readout end 에서 각각 발생, 
	즉 pdr_wakeup interrupt 1회 발생 시 readout done interrupt 총 2회 발생 */
	fpga_reg_u reg_u;
	fpga_reg_u src_reg_u;
	u16 w_offset_error;
	unsigned long irqflags;
	
	disable_irq_nosync(a_nirq);
		
	if( g_b_irq_enable == false )
	{
	    enable_irq(a_nirq);
	    return IRQ_HANDLED;
	}
	
	reg_u.TCON_STATE.DATA = tg_reg_read(FPGA_TCON_STATE);
	src_reg_u.TRIGGER_SRC.DATA = tg_reg_read(TCON_TRIGGER_SRC);
		
	print_current_time();
		
	if( reg_u.TCON_STATE.BIT.READ_OUT_START == 1
	    || reg_u.TCON_STATE.BIT.READ_OUT == 1 )
	{
		set_tg_state(eTG_STATE_READOUT_BEGIN);
		
		if(src_reg_u.TRIGGER_SRC.BIT.TRANS_FRAME != 1)
		{
			printk("[READOUT BEGIN] 0x%04X/0x%04X, The image wouldn't be transferred.\n", reg_u.TCON_STATE.DATA, src_reg_u.TRIGGER_SRC.DATA);		
		}
		else
		{
			w_offset_error = tg_reg_read(FPGA_OFFSET_CAP_ERR);
		
			if( g_b_immediate_transfer == true )
			{
            	if( src_reg_u.TRIGGER_SRC.BIT.OFFSET == 1
                	&& w_offset_error > 0 )
	            {
	                g_w_offset_error = w_offset_error;
	                tg_reg_write( FPGA_LINE_INTR_DISABLE, 1 );
	            }
			    else
			    {
	                tg_reg_write( FPGA_LINE_INTR_DISABLE, 0 );
			    }
			}
			
			printk("[READOUT BEGIN(%d/%d)] 0x%04X/0x%04X(trans: %d, error: 0x%04X)\n", \
		                    g_n_read_buf_idx, g_n_write_buf_idx, \
		                    reg_u.TCON_STATE.DATA, src_reg_u.TRIGGER_SRC.DATA, \
		                    g_b_immediate_transfer, g_w_offset_error);
		}
		
		enable_irq(a_nirq);
		return	IRQ_HANDLED;
	}
	
	if( get_tg_state() != eTG_STATE_READOUT_BEGIN )
	{
	    printk("[READOUT      ] invalid interrupt(0x%04x)\n", reg_u.TCON_STATE.DATA);
		enable_irq(a_nirq);
		return	IRQ_HANDLED;
	}
	
	set_tg_state(eTG_STATE_READOUT_END);

	spin_lock_irqsave(&exposure_wait_queue_ctx.evt_thread_lock, irqflags);
    exposure_wait_queue_ctx.evt_wait_cond = 2;
    spin_unlock_irqrestore(&exposure_wait_queue_ctx.evt_thread_lock, irqflags);
    wake_up_interruptible(&exposure_wait_queue);

	tg_reg_write( FPGA_ROIC_CLK_EN, 0 );			//ROIC MCLK  generation block (ROIC no more operative). 
													//영상 수집이 완료되는 시점에 power save 목적으로 MCLK 신호 생성을 중지한다. 

	// vdd core enable
	vivix_gpio_set_value(VDD_CORE_ENABLE, HIGH);
	
	if( g_w_offset_error == 0 )
	{
	    g_w_offset_error = tg_reg_read(FPGA_OFFSET_CAP_ERR);
	}
	g_b_readout_done = true;
	
	printk("[READOUT END  ] 0x%04X/0x%04X(trans: %d, error: 0x%04X, mig err: 0x%04X)\n", reg_u.TCON_STATE.DATA, src_reg_u.TRIGGER_SRC.DATA, g_b_immediate_transfer, g_w_offset_error, tg_reg_read(FPGA_REG_MIG_ERROR));
	if(src_reg_u.TRIGGER_SRC.BIT.TRANS_FRAME == 1)
	{	
	    if( src_reg_u.TRIGGER_SRC.BIT.OFFSET == 1
	        && g_w_offset_error > 0 )
	    {
	   	    tg_reg_write( FPGA_LINE_INTR_DISABLE, 1 );
	        
	        if( g_b_immediate_transfer == true )
	        {
	            queue_set_error();
	        }
			set_tg_state(eTG_STATE_FLUSH);
	   	}
	   	else if( g_b_immediate_transfer == false )
		{
		    tg_reg_write( FPGA_LINE_INTR_DISABLE, 0 );
		    printk("[READOUT END  ] %d, 0x%04X: 0x%04X, 0x%04X: 0x%04X\n", tg_reg_read( FPGA_LINE_INTR_DISABLE), FPGA_GET_LINE_COUNT, tg_reg_read(FPGA_GET_LINE_COUNT), FPGA_PUT_LINE_COUNT, tg_reg_read(FPGA_PUT_LINE_COUNT));
	
		}
	}
	
	enable_irq(a_nirq);
	return	IRQ_HANDLED;
}

/***********************************************************************************************//**
* \brief		수집 오류 발생 시 큐 초기화
* \param		
* \return		
* \note
***************************************************************************************************/
static void queue_set_error(void)
{
	line_buf_q_t* p_write_q_t;
	
	p_write_q_t = g_p_queue_buf_t[g_n_write_buf_idx];
	
	if( p_write_q_t->nfront != 0)
	{
        g_b_write_error = true;
        
        p_write_q_t->nfront = p_write_q_t->nrear = 0;
        
        printk("[WRITE ERROR] %d\n", g_n_write_buf_idx );		
		
		if(++g_n_write_buf_idx >= g_n_num_of_buffer )
		{
			g_n_write_buf_idx = 0;
		}
		printk("[WRITE ERROR] next : %d\n", g_n_write_buf_idx );
	}		
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
static void queue_clear(void)
{
    //if( g_n_read_buf_idx != g_n_write_buf_idx )
	{
	    printk("[QUEUE CLEAR] read : %d -> %d\n", g_n_read_buf_idx, g_n_write_buf_idx );
	    
		g_p_queue_buf_t[g_n_read_buf_idx]->nfront = 0;
		g_p_queue_buf_t[g_n_read_buf_idx]->nrear = 0;
		
		g_p_queue_buf_t[g_n_write_buf_idx]->nfront = 0;
		g_p_queue_buf_t[g_n_write_buf_idx]->nrear = 0;
		
		g_n_read_buf_idx = g_n_write_buf_idx;
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void load_cal_start( u32 i_n_address )
{
	u32 n_address = i_n_address;
	
	// address setting
	tg_reg_write(0x4232, (u16)(n_address & 0x0000FFFF));
	tg_reg_write(0x4233, (u16)((n_address & 0xFFFF0000) >> 16));
	
	// clear buffer
	tg_reg_write(0x4230, 0);
	tg_reg_write(0x4230, 3);
	tg_reg_write(0x4230, 0);
	
	g_c_transfer_start = 1;
	
	print_current_time();
	
	printk("cal data transfer start(0x%08X)\n", n_address);
	load_cal_transfer();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void load_cal_stop(void)
{
	g_c_transfer_start = 0;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void load_cal_transfer(void)
{
	u32 n_pos = g_p_cal_data_mmap_t->n_write_offset;
	
	if( g_c_transfer_start == 0 )
	{
		return;
	}
		
	memcpy( g_p_line_tg_buf_t->pbuf, &g_p_cal_data_mmap_t->p_data[n_pos], g_tg_buf_size );
	
	g_psg_dst[0].page_link = (0x03 & g_psg_dst[0].page_link) | (unsigned long)virt_to_page(g_p_line_tg_buf_t->pbuf);
	g_psg_dst[0].offset = offset_in_page(g_p_line_tg_buf_t->pbuf);
	g_psg_dst[0].length = g_tg_buf_size;
	g_psg_dst[0].dma_address = (dma_addr_t)g_p_line_tg_buf_t->paddr;
	
	g_psg_src[0].dma_address = WEIM_CS_BASE_ADDR + FPGA_WRITE_BUF_ADDR*sizeof(u16);	
	g_psg_src[0].length = g_tg_buf_size;
#if 0//COMPILE_ERR	
	g_pdma_m2m_desc = g_pdma_m2m_chan->device->
			device_prep_dma_sg(g_pdma_m2m_chan, g_psg_src, 1, g_psg_dst, 1, 0);
#endif
	
	if(g_pdma_m2m_desc != NULL)
	{
		g_pdma_m2m_desc->callback = load_cal_transfer_done;	
		dmaengine_submit(g_pdma_m2m_desc);
		dma_async_issue_pending(g_pdma_m2m_chan);
	}
	
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
static void load_cal_transfer_done(void *a_pdata)
{
	g_p_cal_data_mmap_t->n_write_offset += g_tg_buf_size;
	
	if( g_p_cal_data_mmap_t->n_write_offset == g_p_cal_data_mmap_t->n_read_offset )
	{
		print_current_time();
		printk("cal data transfer end\n");
	}
	else
	{
		load_cal_transfer();
	}
}

/******************************************************************************/
/**
 * @brief	현재 tg_state enum 값을 char 형태로 출력하기 위해 사용하는 함수
 * @param	i_tg_state_e: 현재 g_tg_state의 값
 * @return  g_p_state_print_buf = i_tg_state_e에 해당되는 문자열로 변환한 것
 * @note	g_p_state_print_buf에 6bytes를 사용해 출력하도록 함
 *******************************************************************************/
static char *print_tg_state(TG_STATE i_tg_state_e)
{
	switch (i_tg_state_e)
	{
	case eTG_STATE_FLUSH:
		strncpy(g_p_state_print_buf, "FLUSH ", sizeof("FLUSH "));
		break;
	case eTG_STATE_EXPOSURE:
		strncpy(g_p_state_print_buf, "EXP   ", sizeof("EXP   "));
		break;
	case eTG_STATE_READOUT_BEGIN:
		strncpy(g_p_state_print_buf, "RO_SRT", sizeof("RO_SRT"));
		break;
	case eTG_STATE_READOUT_END:
		strncpy(g_p_state_print_buf, "RO_END", sizeof("RO_END"));
		break;
	}

	return g_p_state_print_buf;
}
