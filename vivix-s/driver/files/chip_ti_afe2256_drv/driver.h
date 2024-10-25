#ifndef __DRIVER_H
#define __DRIVER_H

#include <linux/fs.h>
#include "driver_defs.h"


                                	
/***************************************************************************************************
*	Constant Definitions
***************************************************************************************************/
//#define VIVIX_TI_AFE2256_DBG

#if defined(VIVIX_TI_AFE2256_DBG)
#define 	DBG_PRINT(args...)  	 	printk(args)
#else
#define 	DBG_PRINT(args...)   		
#endif

#define 	VIVIX_2530VW_ROIC_NUM			(8)
#define 	VIVIX_3643VW_ROIC_NUM			(10)
#define 	VIVIX_4343VW_ROIC_NUM			(12)

#define 	VIVIX_2530VW_DAC_NUM			(2)
#define 	VIVIX_3643VW_DAC_NUM			(2)
#define 	VIVIX_4343VW_DAC_NUM			(2)

#define		TFT_AED_VALID_FLUSH_START		(0x0002)
#define		TFT_AED_MODE_ENABLE				(0x0001)

//[2019/03/14][yongjin] TI ROIC는 TPA, TPB 그리고 실제 ROIC에 적힌 값을 읽고 쓸 수 있어야 함에 따라 32bit 주소 중 16, 17 bit를 이용하여 구분한다.
#define 	MARK_TI_ROIC_TPA				(0x00)
#define 	MARK_TI_ROIC_TPB				(0x01)
#define 	MARK_TI_ROIC_REAL				(0x02) //ROIC에 직접 쓰겠다라는 의미

#define 	MASK_TI_ROIC_TPA(x)				( ( ( (x) >> 16 ) & 0x00000003) ==  MARK_TI_ROIC_TPA )
#define 	MASK_TI_ROIC_TPB(x)				( ( ( (x) >> 16 ) & 0x00000003) ==  MARK_TI_ROIC_TPB )
#define 	MASK_TI_ROIC_REAL(x)			( ( ( (x) >> 16 ) & 0x00000003) ==  MARK_TI_ROIC_REAL )

/***************************************************************************************************
*	Type Definitions
***************************************************************************************************/                                         

typedef enum
{
	eTI_ROIC_REG_00_IDX = 0,
	eTI_ROIC_REG_10_IDX,
	eTI_ROIC_REG_11_IDX,
	eTI_ROIC_REG_12_IDX,
	eTI_ROIC_REG_13_IDX,
	eTI_ROIC_REG_16_IDX,
	eTI_ROIC_REG_18_IDX,
	eTI_ROIC_REG_2C_IDX,
	eTI_ROIC_REG_2E_IDX,
	eTI_ROIC_REG_30_IDX,
	eTI_ROIC_REG_31_IDX,
	eTI_ROIC_REG_32_IDX,
	eTI_ROIC_REG_33_IDX,
	
	/* Timing profile start */
	eTI_ROIC_REG_40_IDX,
	eTI_ROIC_REG_41_IDX,
	eTI_ROIC_REG_42_IDX,
	eTI_ROIC_REG_43_IDX,
	eTI_ROIC_REG_46_IDX,
	eTI_ROIC_REG_47_IDX,
	eTI_ROIC_REG_4A_IDX,
	eTI_ROIC_REG_4B_IDX,
	eTI_ROIC_REG_4C_IDX,
	eTI_ROIC_REG_50_IDX,
	eTI_ROIC_REG_51_IDX,
	eTI_ROIC_REG_52_IDX,
	eTI_ROIC_REG_53_IDX,
	eTI_ROIC_REG_54_IDX,
	eTI_ROIC_REG_55_IDX,
	/* Timing profile end */
	
	eTI_ROIC_REG_5A_IDX,
	eTI_ROIC_REG_5C_IDX,
	eTI_ROIC_REG_5D_IDX,
	eTI_ROIC_REG_5E_IDX,
	eTI_ROIC_REG_61_IDX,
	
	eTI_ROIC_REG_MAX,
	
}TI_ROIC_REG_IDX;

/***************************************************************************************************
*	Macros (Inline Functions) Definitions
***************************************************************************************************/

/***************************************************************************************************
*	Variable Definitions
***************************************************************************************************/

/***************************************************************************************************
*	Function Prototypes
***************************************************************************************************/
                   	
#endif /* __DRIVER_H */

