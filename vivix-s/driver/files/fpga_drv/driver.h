/********************************************************************************  
 ?  ? : fpga driver
 ??? : ???
 ?  ? : 0.0.1
 ?  ? : freescale, fpga driver 
 ?  ? : 
 
          1. FPGA read, write, img data read ??.
          
 ?  ?:
         0.0.1  linux ??????? ??, freescale? EIM SDMA ??, 1012N ??

********************************************************************************/
#ifndef _DRIVER_H_
#define _DRIVER_H_



/***************************************************************************************************
*	Include Files
***************************************************************************************************/
#include <linux/fs.h>
#include "reg_def.h"


/***************************************************************************************************
*	Constant Definitions
***************************************************************************************************/
#define 	IMAGE_PHY_ADDR			0x2e000000
#define 	IMAGE_PHY_SIZE			0x02000000
	
#define 	VIVIX_2530VW_BUF_SIZE	4096
#define		VIVIX_2530VW_HEIGHT		2560

#define 	VIVIX_3643VW_BUF_SIZE	5120
#define		VIVIX_3643VW_HEIGHT		3072

#define 	VIVIX_4343VW_BUF_SIZE	6144
#define		VIVIX_4343VW_HEIGHT		3072                               
                                	
#define 	FPGA_READ_BUF_ADDR		0x00008000
#define 	FPGA_WRITE_BUF_ADDR		0x00002000

/***************************************************************************************************
*	Type Definitions
***************************************************************************************************/
typedef struct _tg_buf
{
	s32		nch;
	u32		nmask;
	u8*		pbuf;
	u32*	paddr;
}tg_buf_t;

typedef struct _line_buf_q
{
	tg_buf_t			*pbuf_t;
	s32					nfront;
	s32					nrear;
	struct semaphore	lock_t;	
}line_buf_q_t;

typedef enum
{
	eTG_STATE_FLUSH = 0,
	eTG_STATE_EXPOSURE,
	eTG_STATE_READOUT_BEGIN,
	eTG_STATE_READOUT_END,
}TG_STATE;

typedef enum
{
	eROIC_LP_POWER_DOWN = 0,
	eROIC_LP_NAP,
}ROIC_LOW_POWER;

/***************************************************************************************************
*	Macros (Inline Functions) Definitions
***************************************************************************************************/

#define 	DBG_PRINT(x, args...)  	 	if(x) printk(args);

/***************************************************************************************************
*	Variable Definitions
***************************************************************************************************/
extern wait_queue_head_t 	queue_read;
extern u32					g_readout_req_irq_num;
extern u32					g_exp_irq_num;
extern u32					g_readout_done_irq_num;
/***************************************************************************************************
*	Function Prototypes
***************************************************************************************************/
s32 vivix_tg_queue_buf_init(line_buf_q_t **a_p_queue_t);

int	 tg_initialize(u8 i_c_model);

#endif
