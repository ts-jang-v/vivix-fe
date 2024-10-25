/*******************************************************************************
 모  듈 : fpga
 작성자 : 한명희
 버  전 : 0.0.1
 설  명 : fpga 제어
 참  고 : 

 버  전:
         0.0.1  최초 작성
*******************************************************************************/



/*******************************************************************************
*	Include Files
*******************************************************************************/



#ifdef TARGET_COM
#include <iostream>
#include <errno.h>          // errno
#include <stdio.h>			// fprintf()
#include <string.h>			// memset() memset/memcpy
#include <stdlib.h>			// malloc(), free(),exit(), EXIT_SUCCESS rand/rand 
#include <sys/ioctl.h>  	// ioctl()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>          // open() O_NDELAY
#include <unistd.h>
#include <math.h>			// fabs

#include "fpga.h"
#include "vw_file.h"
#include "vworks_ioctl.h"
#include "misc.h"
#else
#include "typedef.h"
#include "Ioctl.h"
#include "Dev_io.h"
#include "Thread.h"
#include "File.h"
#include "Common_def.h"
#define _IOWR(a,b,c) b
#include "../driver/include/vworks_ioctl.h"
#include "../app/detector/include/fpga.h"
#include <string.h>
//#include "Mutex.h"
//#include "Time.h"        // sleep_ex
//#include "Fpga_prom.h"
//#include "../app/detector/include/vw_system_def.h"
//#include "../app/detector/include/fpga_reg.h"
#endif

using namespace std;

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/

#define IMAGE_LINE_COUNT		(8)					// FPGA <-> RISC 간 영상 데이터 전송 시, 한 번에 전송하는 line 수
#define EXP_OK_WAIT_TIME		(50)

// ROIC_PWR_EN 신호 enable 직후부터 ROIC, GON, GOFF 전압 들어오기까지 실험 측정 치 - 약 250ms 
#define VW_ROIC_GATE_POWER_UP_TIME	        500
#define SIGNAL_BOARD_NUM_VW_REVISION (2)
#define CONTROL_BOARD_NUM_VW_REVISION (3)

/*******************************************************************************
*	Type Definitions
*******************************************************************************/
typedef union _fpga_reg_u
{
	union
	{
		u16	DATA;
		struct
		{
			u16	CHECK_TRIGGER		    : 1;	/* BIT0	: 	*/
			u16 ERR_INT_TRIGGER         : 1;
			u16 ERR_ACTIVE_TRIGGER      : 1;
			u16 ERR_PASSIVE_TRIGGER     : 1;
			u16 ERR_NON_LINE_TRIGGER    : 1;    /* BIT4 :   */
			u16 INT_TRIGGER             : 1;
			u16 ACTIVE_TRIGGER          : 1;
			u16 PASSIVE_TRIGGER         : 1;
			u16 NON_LINE_TRIGGER        : 1;    /* BIT8 :   */
			u16 IDLE                    : 1;
			u16 FLUSH                   : 1;
			u16 EXPOSURE                : 1;
			u16 READ_OUT_START          : 1;    /* BIT12 :   */
			u16 READ_OUT                : 1;
			u16 READ_OUT_DONE           : 1;
			u16 OP_FLUSH                : 1;    /* BIT15 :   */
		}BIT;
	}TCON_STATE;
	
	union
	{
		u16	DATA;
		struct
		{
			u16	CAP_ENABLE              : 1;	/* BIT0	: 	*/
			u16                         : 3;
			
			/* BIT4	: 0 - offset capture stop, 1 - AED capture after offset capture stop	*/
			u16	CAP_MODE                : 1;
			u16                         : 14;
		}BIT;
	}OFFSET_CTRL;

	union
	{
		u16	DATA;
		struct
		{
			u16	IFS					    : 5;	/* BIT0	: 	*/
			u16 NOT_USED		        : 1;
			u16 PWR_MODE			    : 3;
			u16 NOT_USED2			    : 7;
		}BIT;
	}ROIC_REG_0;

	union
	{
		u16	DATA;
		struct
		{
			u16	RefDAC				    : 8;	/* BIT0	: 	*/
			u16 AZEn			        : 1;
			u16 NOT_USED			    : 7;
		}BIT;
	}ROIC_REG_3;

	union
	{
		u16 DATA;
		struct
		{
			u16 READOUT_OPTION	: 1;		/* 1: double readout 0: single readout	*/
			u16 SMART_W			: 1;		/* SMART W ON/OFF		*/
			u16	RESERVED1		: 2;		
			u16	BYPASS_OPTION	: 1;
			u16	RESERVED2		: 11;		
		}BIT;
		
	}IMG_CAPTURE_OPT;

    union
	{
		u16 DATA;
		struct
		{
			u16 READOUT_SYNC	: 1;    // [20240426] [giwook] Readout Sync 사양서 3 FPGA Register Map 참고
			u16 RESERVED	    : 15;		
		}BIT;
		
	}READOUT_SYNC;
}fpga_reg_u;


/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/

/*******************************************************************************
*	Variable Definitions
*******************************************************************************/

/*  VW	Basic				VW PLUS Depostion(YMIT)	 VW PLUS Laminating(Hamamatsu)   */
u16 s_p_input_charge[eMODEL_MAX_NUM][ePANEL_TYPE_MAX][eSCINTILLATOR_TYPE_MAX][eROIC_TYPE_MAX][GAIN_TYPE_NUM] = 
{   /*  ROIC        ADI 1255               TI 2256GR            TI 2256TD (Not Use)        */
/* Gain type   0   1   2   3    4	 0   1   2   3   4	    0   1   2   3   4    */
/* 2530VW */
    { 
    /*BASIC PANEL */  
        {
    /*SCINTILATOR BASIC ~ LAMI & A - F */     
            { {00, 00, 00, 00, 00}, {28, 25, 23, 22, 20}, {00, 00, 00, 00, 00} },   //BASIC
            { {00, 00, 00, 00, 00}, {28, 25, 23, 22, 20}, {00, 00, 00, 00, 00} },   //DEPO
            { {18, 13, 10,  9,  6}, {30, 26, 24, 23, 21}, {00, 00, 00, 00, 00} },   //A
            { {24, 18, 14, 12,  9}, {31, 31, 28, 26, 23}, {00, 00, 00, 00, 00} },   //B
            { {20, 15, 12, 10,  7}, {31, 28, 26, 24, 22}, {00, 00, 00, 00, 00} },   //C
            { {23, 17, 13, 11,  8}, {31, 30, 27, 25, 23}, {00, 00, 00, 00, 00} },   //D
            { {15, 11,  9,  7,  5}, {28, 25, 23, 22, 20}, {00, 00, 00, 00, 00} },   //E
            { {11,  8,  6,  5,  4}, {25, 22, 21, 20, 19}, {00, 00, 00, 00, 00} },   //F
            { {13,  9,  7,  6,  4}, {26, 23, 22, 21, 19}, {00, 00, 00, 00, 00} },   //G 
            { {14, 10,  8,  7,  5}, {27, 24, 23, 21, 20}, {00, 00, 00, 00, 00} },   //H
            { {17, 12, 10,  8,  6}, {31, 27, 25, 23, 21}, {00, 00, 00, 00, 00} },   //I 
            { {19, 14, 11,  9,  7}, {31, 29, 27, 25, 22}, {00, 00, 00, 00, 00} },   //J 
            { {22, 16, 13, 11,  8}, {31, 29, 27, 25, 22}, {00, 00, 00, 00, 00} },   //K 
            { {26, 19, 15, 13,  9}, {31, 29, 27, 25, 22}, {00, 00, 00, 00, 00} },   //L 
            { {27, 20, 16, 13, 10}, {31, 29, 27, 25, 22}, {00, 00, 00, 00, 00} },   //M
            { {29, 21, 17, 14, 10}, {31, 29, 27, 25, 22}, {00, 00, 00, 00, 00} }    //N
             

        }
    },
/* 3643VW */
    { 
    /*BASIC PANEL */  
        {
    /*SCINTILATOR BASIC ~ LAMI & A - F */     
            { {00, 00, 00, 00, 00}, {30, 26, 24, 23, 21}, {00, 00, 00, 00, 00} },   //BASIC
            { {00, 00, 00, 00, 00}, {31, 28, 26, 24, 22}, {00, 00, 00, 00, 00} },   //DEPO
            { {18, 13, 10,  9,  6}, {30, 26, 24, 23, 21}, {00, 00, 00, 00, 00} },   //A
            { {24, 18, 14, 12,  9}, {31, 31, 28, 26, 23}, {00, 00, 00, 00, 00} },   //B
            { {20, 15, 12, 10,  7}, {31, 28, 26, 24, 22}, {00, 00, 00, 00, 00} },   //C
            { {23, 17, 13, 11,  8}, {31, 30, 27, 25, 23}, {00, 00, 00, 00, 00} },   //D
            { {15, 11,  9,  7,  5}, {28, 25, 23, 22, 20}, {00, 00, 00, 00, 00} },   //E
            { {11,  8,  6,  5,  4}, {25, 22, 21, 20, 19}, {00, 00, 00, 00, 00} },   //F
            { {13,  9,  7,  6,  4}, {26, 23, 22, 21, 19}, {00, 00, 00, 00, 00} },   //G 
            { {14, 10,  8,  7,  5}, {27, 24, 23, 21, 20}, {00, 00, 00, 00, 00} },   //H
            { {17, 12, 10,  8,  6}, {31, 27, 25, 23, 21}, {00, 00, 00, 00, 00} },   //I 
            { {19, 14, 11,  9,  7}, {31, 29, 27, 25, 22}, {00, 00, 00, 00, 00} },   //J 
            { {22, 16, 13, 11,  8}, {31, 29, 27, 25, 22}, {00, 00, 00, 00, 00} },   //K 
            { {26, 19, 15, 13,  9}, {31, 29, 27, 25, 22}, {00, 00, 00, 00, 00} },   //L 
            { {27, 20, 16, 13, 10}, {31, 29, 27, 25, 22}, {00, 00, 00, 00, 00} },   //M
            { {29, 21, 17, 14, 10}, {31, 29, 27, 25, 22}, {00, 00, 00, 00, 00} }    //N

        }
    },
/* 4343VW */
    { 
    /*BASIC PANEL */  
        {
    /*SCINTILATOR BASIC ~ LAMI & A - F */     
            { {00, 00, 00, 00, 00}, {30, 26, 24, 23, 21}, {00, 00, 00, 00, 00} },   //BASIC
            { {00, 00, 00, 00, 00}, {31, 28, 26, 24, 22}, {00, 00, 00, 00, 00} },   //DEPO     
            { {18, 13, 10,  9,  6}, {30, 26, 24, 23, 21}, {00, 00, 00, 00, 00} },   //A
            { {24, 18, 14, 12,  9}, {31, 31, 28, 26, 23}, {00, 00, 00, 00, 00} },   //B
            { {20, 15, 12, 10,  7}, {31, 28, 26, 24, 22}, {00, 00, 00, 00, 00} },   //C
            { {23, 17, 13, 11,  8}, {31, 30, 27, 25, 23}, {00, 00, 00, 00, 00} },   //D
            { {15, 11,  9,  7,  5}, {28, 25, 23, 22, 20}, {00, 00, 00, 00, 00} },   //E
            { {11,  8,  6,  5,  4}, {25, 22, 21, 20, 19}, {00, 00, 00, 00, 00} },   //F
            { {13,  9,  7,  6,  4}, {26, 23, 22, 21, 19}, {00, 00, 00, 00, 00} },   //G 
            { {14, 10,  8,  7,  5}, {27, 24, 23, 21, 20}, {00, 00, 00, 00, 00} },   //H
            { {17, 12, 10,  8,  6}, {31, 27, 25, 23, 21}, {00, 00, 00, 00, 00} },   //I 
            { {19, 14, 11,  9,  7}, {31, 29, 27, 25, 22}, {00, 00, 00, 00, 00} },   //J 
            { {22, 16, 13, 11,  8}, {31, 29, 27, 25, 22}, {00, 00, 00, 00, 00} },   //K 
            { {26, 19, 15, 13,  9}, {31, 29, 27, 25, 22}, {00, 00, 00, 00, 00} },   //L 
            { {27, 20, 16, 13, 10}, {31, 29, 27, 25, 22}, {00, 00, 00, 00, 00} },   //M
            { {29, 21, 17, 14, 10}, {31, 29, 27, 25, 22}, {00, 00, 00, 00, 00} }    //N
        }
    }
};

u16 s_p_roic_cmp_gain[eMODEL_MAX_NUM][ePANEL_TYPE_MAX][eSCINTILLATOR_TYPE_MAX][eROIC_TYPE_MAX][GAIN_TYPE_NUM] = 
{   /*  ROIC        ADI 1255               TI 2256GR            TI 2256TD (Not Use)        */
/* Gain type   0   1   2   3    4	 0   1   2   3   4	    0   1   2   3   4    */
/* 2530VW */
    { 
    /*BASIC PANEL */  
        {
    /*SCINTILATOR BASIC ~ LAMI & A - F */     
            { {0000, 0000, 0000, 0000, 0000}, { 998, 1024, 1024, 1075, 1024}, {0000, 0000, 0000, 0000, 0000} },   //BASIC
            { {0000, 0000, 0000, 0000, 0000}, { 998, 1024, 1024, 1075, 1024}, {0000, 0000, 0000, 0000, 0000} },   //DEPO
            { {1042, 1024, 1006, 1097, 1024}, {1047, 1024, 1047, 1117, 1117}, {0000, 0000, 0000, 0000, 0000} },   //A
            { {1011, 1024, 1011, 1051, 1078}, { 768, 1024, 1040, 1056, 1024}, {0000, 0000, 0000, 0000, 0000} },   //B
            { {1008, 1024, 1040, 1056, 1024}, { 945, 1024, 1083, 1063, 1103}, {0000, 0000, 0000, 0000, 0000} },   //C
            { {1024, 1024,  996, 1024, 1024}, { 819, 1024, 1024, 1024, 1092}, {0000, 0000, 0000, 0000, 0000} },   //D
            { {1024, 1024, 1067, 1024, 1024}, { 998, 1024, 1024, 1075, 1024}, {0000, 0000, 0000, 0000, 0000} },   //E
            { {1024, 1024,  996, 1024, 1138}, {1097, 1024, 1097, 1097, 1170}, {0000, 0000, 0000, 0000, 0000} },   //F 
            { {1075, 1024, 1024, 1075, 1024}, {1056, 1024, 1120, 1152, 1024}, {0000, 0000, 0000, 0000, 0000} },   //G
            { {1047, 1024, 1047, 1117, 1117}, {1024, 1024, 1138, 1024, 1138}, {0000, 0000, 0000, 0000, 0000} },   //H
            { {1063, 1024, 1083, 1063, 1103}, {1024, 1024, 1067, 1024, 1024}, {0000, 0000, 0000, 0000, 0000} },   //I
            { {1024, 1024, 1024, 1024, 1092}, { 878, 1024, 1097, 1097, 1024}, {0000, 0000, 0000, 0000, 0000} },   //J
            { {1039, 1024, 1054, 1084, 1084}, { 878, 1024, 1097, 1097, 1024}, {0000, 0000, 0000, 0000, 0000} },   //K
            { {1037, 1024, 1024, 1075, 1024}, { 878, 1024, 1097, 1097, 1024}, {0000, 0000, 0000, 0000, 0000} },   //L
            { {1024, 1024, 1036, 1024, 1073}, { 878, 1024, 1097, 1097, 1024}, {0000, 0000, 0000, 0000, 0000} },   //M
            { {1047, 1024, 1047, 1047, 1024}, { 878, 1024, 1097, 1097, 1024}, {0000, 0000, 0000, 0000, 0000} }    //N
        }
    },
/* 3643VW */
    { 
    /*BASIC PANEL */  
        {
    /*SCINTILATOR BASIC ~ LAMI & A - F */     
            { {0000, 0000, 0000, 0000, 0000}, {1047, 1024, 1047, 1117, 1117}, {0000, 0000, 0000, 0000, 0000} },   //BASIC
            { {0000, 0000, 0000, 0000, 0000}, { 945, 1024, 1083, 1063, 1103}, {0000, 0000, 0000, 0000, 0000} },   //DEPO    
            { {1042, 1024, 1006, 1097, 1024}, {1047, 1024, 1047, 1117, 1117}, {0000, 0000, 0000, 0000, 0000} },   //A
            { {1011, 1024, 1011, 1051, 1078}, { 768, 1024, 1040, 1056, 1024}, {0000, 0000, 0000, 0000, 0000} },   //B
            { {1008, 1024, 1040, 1056, 1024}, { 945, 1024, 1083, 1063, 1103}, {0000, 0000, 0000, 0000, 0000} },   //C
            { {1024, 1024,  996, 1024, 1024}, { 819, 1024, 1024, 1024, 1092}, {0000, 0000, 0000, 0000, 0000} },   //D
            { {1024, 1024, 1067, 1024, 1024}, { 998, 1024, 1024, 1075, 1024}, {0000, 0000, 0000, 0000, 0000} },   //E
            { {1024, 1024,  996, 1024, 1138}, {1097, 1024, 1097, 1097, 1170}, {0000, 0000, 0000, 0000, 0000} },   //F 
            { {1075, 1024, 1024, 1075, 1024}, {1056, 1024, 1120, 1152, 1024}, {0000, 0000, 0000, 0000, 0000} },   //G
            { {1047, 1024, 1047, 1117, 1117}, {1024, 1024, 1138, 1024, 1138}, {0000, 0000, 0000, 0000, 0000} },   //H
            { {1063, 1024, 1083, 1063, 1103}, {1024, 1024, 1067, 1024, 1024}, {0000, 0000, 0000, 0000, 0000} },   //I
            { {1024, 1024, 1024, 1024, 1092}, { 878, 1024, 1097, 1097, 1024}, {0000, 0000, 0000, 0000, 0000} },   //J
            { {1039, 1024, 1054, 1084, 1084}, { 878, 1024, 1097, 1097, 1024}, {0000, 0000, 0000, 0000, 0000} },   //K
            { {1037, 1024, 1024, 1075, 1024}, { 878, 1024, 1097, 1097, 1024}, {0000, 0000, 0000, 0000, 0000} },   //L
            { {1024, 1024, 1036, 1024, 1073}, { 878, 1024, 1097, 1097, 1024}, {0000, 0000, 0000, 0000, 0000} },   //M
            { {1047, 1024, 1047, 1047, 1024}, { 878, 1024, 1097, 1097, 1024}, {0000, 0000, 0000, 0000, 0000} }    //N
        }
    },
/* 4343VW */
    { 
    /*BASIC PANEL */  
        {
    /*SCINTILATOR BASIC ~ LAMI & A - F */     
            { {0000, 0000, 0000, 0000, 0000}, {1047, 1024, 1047, 1117, 1117}, {0000, 0000, 0000, 0000, 0000} },   //BASIC
            { {0000, 0000, 0000, 0000, 0000}, { 945, 1024, 1083, 1063, 1103}, {0000, 0000, 0000, 0000, 0000} },   //DEPO     
            { {1042, 1024, 1006, 1097, 1024}, {1047, 1024, 1047, 1117, 1117}, {0000, 0000, 0000, 0000, 0000} },   //A
            { {1011, 1024, 1011, 1051, 1078}, { 768, 1024, 1040, 1056, 1024}, {0000, 0000, 0000, 0000, 0000} },   //B
            { {1008, 1024, 1040, 1056, 1024}, { 945, 1024, 1083, 1063, 1103}, {0000, 0000, 0000, 0000, 0000} },   //C
            { {1024, 1024,  996, 1024, 1024}, { 819, 1024, 1024, 1024, 1092}, {0000, 0000, 0000, 0000, 0000} },   //D
            { {1024, 1024, 1067, 1024, 1024}, { 998, 1024, 1024, 1075, 1024}, {0000, 0000, 0000, 0000, 0000} },   //E
            { {1024, 1024,  996, 1024, 1138}, {1097, 1024, 1097, 1097, 1170}, {0000, 0000, 0000, 0000, 0000} },   //F 
            { {1075, 1024, 1024, 1075, 1024}, {1056, 1024, 1120, 1152, 1024}, {0000, 0000, 0000, 0000, 0000} },   //G
            { {1047, 1024, 1047, 1117, 1117}, {1024, 1024, 1138, 1024, 1138}, {0000, 0000, 0000, 0000, 0000} },   //H
            { {1063, 1024, 1083, 1063, 1103}, {1024, 1024, 1067, 1024, 1024}, {0000, 0000, 0000, 0000, 0000} },   //I
            { {1024, 1024, 1024, 1024, 1092}, { 878, 1024, 1097, 1097, 1024}, {0000, 0000, 0000, 0000, 0000} },   //J
            { {1039, 1024, 1054, 1084, 1084}, { 878, 1024, 1097, 1097, 1024}, {0000, 0000, 0000, 0000, 0000} },   //K
            { {1037, 1024, 1024, 1075, 1024}, { 878, 1024, 1097, 1097, 1024}, {0000, 0000, 0000, 0000, 0000} },   //L
            { {1024, 1024, 1036, 1024, 1073}, { 878, 1024, 1097, 1097, 1024}, {0000, 0000, 0000, 0000, 0000} },   //M
            { {1047, 1024, 1047, 1047, 1024}, { 878, 1024, 1097, 1097, 1024}, {0000, 0000, 0000, 0000, 0000} }    //N
        }
    }
};

u16 s_p_gain_ratio[5]	= 	{75,  100,  125,  150,  200};	

/*******************************************************************************
*	Function Prototypes
*******************************************************************************/



/******************************************************************************/
/**
* \brief        constructor
* \return       none
* \note
*******************************************************************************/
CFpga::CFpga(int (*log)(int,int,const char *,...))
{
    print_log = log;
    strcpy( m_p_log_id, "FPGA          " );
    
    io = new CMutex;
    
    int i;
    for( i = 0; i < REG_ADDRESS_MAX; i++ )
    {
        m_p_reg[i] = 0xFFFF;
    }
    
    m_n_fd      = -1;
    m_n_fd_roic = -1;
    
    m_b_config_is_running = false;
    m_w_config_progress = 0;
    m_b_config_result   = false;
    m_update_step_e     = eUPDATE_STEP_NONE;
    m_p_prom_c          = NULL;
    
    m_b_power_on = false;
    
    m_p_line_intr_c         = NULL;
    m_p_line_intr_lock_c    = new CMutex;
    
    m_b_exp_ok = false;

    m_thread_t = (pthread_t)0;
    m_c_roic_idle = 0;
    m_c_roic_drive = 0;
    m_w_aed_select = 0;
    
    m_p_flush_wait_c = NULL;
    
    m_n_expected_wakeup_time = 5;

    m_model_e = eMODEL_MAX_NUM;
    m_panel_type_e = ePANEL_TYPE_MAX;
    m_scintillator_type_e = eSCINTILLATOR_TYPE_MAX;
    m_submodel_type_e = eSUBMODEL_TYPE_MAX;
    m_roic_type_e = eROIC_TYPE_MAX;
    
    print_log(DEBUG, 1, "[%s] CFpga\n", m_p_log_id);

}
/******************************************************************************/
/**
* \brief        constructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CFpga::CFpga(void)
{
}

/******************************************************************************/
/**
* \brief        destructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CFpga::~CFpga()
{
    if( m_b_config_is_running )
    {
        config_stop();
    }
    
    if( m_n_fd != -1 )
    {
        close(m_n_fd);
    }
    
    if( m_n_fd_roic != -1 )
    {
        close(m_n_fd_roic);
    }

    safe_delete( io );
    
    safe_delete( m_p_line_intr_lock_c );
    
    safe_delete( m_p_prom_c );
    
    print_log(DEBUG, 1, "[%s] ~CFpga\n", m_p_log_id );
}

/******************************************************************************/
/**
 * @brief   FPGA register write
 * @param   
 * @return  
 * @note    register의 bit 단위 값을 변경할 때 사용한다.
 *          이 함수 호출 전 후로 lock()/unlock()이 호출되어야 한다.
*******************************************************************************/
void CFpga::io_write( u16 i_w_addr, u16 i_w_data )
{
	ioctl_data_t ctl_t;
	
	memset( &ctl_t, 0, sizeof(ioctl_data_t) );
	memcpy( &ctl_t.data[0], &i_w_addr, 2 );
	memcpy( &ctl_t.data[2], &i_w_data, 2 );
	
	ioctl( m_n_fd, VW_MSG_IOCTL_TG_REG_WRITE, &ctl_t ); 
    
}

/******************************************************************************/
/**
 * @brief   FPGA register read
 * @param   
 * @return  
 * @note    register의 bit 단위 값을 변경할 때 사용한다.
 *          이 함수 호출 전 후로 lock()/unlock()이 호출되어야 한다.
*******************************************************************************/
u16 CFpga::io_read( u16 i_w_addr )
{
    u16             w_data;
	ioctl_data_t	ctl_t;
	
	memset( &ctl_t, 0, sizeof(ioctl_data_t) );
	memcpy( &ctl_t.data[0], &i_w_addr, 2 );
	ioctl( m_n_fd, VW_MSG_IOCTL_TG_REG_READ, &ctl_t ); 
	memcpy( &w_data, &ctl_t.data[0], 2 );

    return w_data;
}

/******************************************************************************/
/**
 * @brief   FPGA register write
 * @param   
 * @return  
 * @note    //단순 FPGA register write 이외에 register 값 변경에의해 추가 동작이 필요한 registers 존재 
*******************************************************************************/
void CFpga::write( u16 i_w_addr, u16 i_w_data )
{
    print_log(DEBUG, 1, "[%s] write 0x%04X, 0x%04X\n", m_p_log_id, i_w_addr, i_w_data );
    
    if( i_w_addr == eFPGA_REG_DIGITAL_GAIN )
    {
        if( i_w_data == 0 )
        {
            return;
        }
        
        lock();
        io_write( i_w_addr, i_w_data );
        unlock();
    }
    else if( i_w_addr == eFPGA_REG_EXP_OK_DELAY_TIME )
    {
        lock();
        io_write( i_w_addr, i_w_data );
        unlock();
        
        update_pseudo_flush_num();
    }
    else if( i_w_addr == eFPGA_REG_EXP_TIME_LOW || i_w_addr == eFPGA_REG_EXP_TIME_HIGH )
    {
        lock();
        io_write( i_w_addr, i_w_data );
        unlock();
        
       	set_aed_error_time();
    }
    else if( i_w_addr == eFPGA_REG_TFT_AED_OPTION /*|| i_w_addr == eFPGA_REG_TRIGGER_TYPE*/ )
    {
        lock();
        io_write( i_w_addr, i_w_data );
        unlock();
        
       	tft_aed_control();
    }    
    else if( i_w_addr == eFPGA_REG_ROIC_GAIN_TYPE ) //ROIC 공통
    {
        lock();
        io_write( i_w_addr, i_w_data );
        unlock();

        set_gain_type();        
    }
    else if( ( m_roic_type_e == eROIC_TYPE_ADI_1255 ) && 
             (i_w_addr >= eFPGA_REG_ROIC_CFG_0 && i_w_addr <= eFPGA_REG_ROIC_CFG_15) ) //ADI ROIC 전용 Reg Addr 
    {
        lock();
        io_write( i_w_addr, i_w_data );
        unlock();

        roic_reg_write( (i_w_addr - eFPGA_REG_ROIC_CFG_0), i_w_data );
    
    }
    else if( ( m_roic_type_e == eROIC_TYPE_TI_2256GR ) && 
             ( i_w_addr >= eFPGA_REG_TI_ROIC_CONFIG_00 && i_w_addr <= eFPGA_REG_TI_ROIC_CONFIG_61 ) ) //TI ROIC 전용 Reg Addr 1
    {
        lock();
        io_write( i_w_addr, i_w_data );
        unlock();        

        roic_reg_write( (i_w_addr - eFPGA_REG_TI_ROIC_CONFIG_00), i_w_data );      
    }
    else if( ( m_roic_type_e == eROIC_TYPE_TI_2256GR ) && 
             ( i_w_addr == eFPGA_REG_TI_ROIC_DAC_COMP1 || i_w_addr == eFPGA_REG_TI_ROIC_DAC_COMP3 ) ) //TI ROIC 전용 Reg Addr 2
    {
        lock();
        io_write( i_w_addr, i_w_data );
        unlock();        

        dac_setup();
         
    }
    else if( ( m_roic_type_e == eROIC_TYPE_TI_2256GR ) && 
             ( i_w_addr == eFPGA_REG_TI_ROIC_LOW_POWER ) ) //TI ROIC 전용 Reg Addr 3
    {
        lock();
        io_write( i_w_addr, i_w_data );
        unlock();        

        set_roic_power_mode( i_w_data );
         
    }
    else //그 외 registers
    {
        lock();
        io_write( i_w_addr, i_w_data );
        unlock(); 
    }
            
#if 0
    else if( i_w_addr == FPGA_ISP_REG_OFFSET_CORR_ON )
    {        
    	lock();
        io_write( eFPGA_REG_DRO_OFFSET_ON, i_w_data );
        unlock();
    	print_log(DEBUG, 1, "[%s] Offset correction on (0x%04X -> 0x%04X) \n", m_p_log_id, i_w_addr, eFPGA_REG_DRO_OFFSET_ON );
	}
#endif
}

/******************************************************************************/
/**
 * @brief   FPGA register read
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CFpga::read( u16 i_w_addr, bool i_b_saved )
{
    if( i_b_saved == true )
    {
        return m_p_reg[i_w_addr];
    }
    
    u16 data = 0;
    
    lock();
    data = io_read( i_w_addr );
    unlock();
    
    return data;   
}

/******************************************************************************/
/**
* \brief        start configuration thread
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CFpga::config_start( const s8* i_p_file_name )
{
    strcpy( m_p_config_file_name, i_p_file_name );
    m_w_config_progress = 0;
    m_b_config_result   = false;
    
    m_update_step_e     = eUPDATE_STEP_NONE;
    
    if( m_b_config_is_running == true )
    {
        print_log( ERROR, 1, "[%s] config is already started.\n", m_p_log_id);
        return false;
    }
    
    // launch config thread
    m_b_config_is_running = true;
	if( pthread_create(&m_thread_t, NULL, config_routine, ( void * ) this)!= 0 )
    {
    	print_log( ERROR, 1, "[%s] pthread_create:%s\n", m_p_log_id, strerror( errno ));

    	m_b_config_is_running = false;
    	
    	return false;
    }
    pthread_detach(m_thread_t);
    
    return true;
}

/******************************************************************************/
/**
* \brief        stop configuration thread
* \param        none
* \return       none
* \note
*******************************************************************************/
void CFpga::config_stop(void)
{
    if( m_b_config_is_running == false )
    {
        print_log( ERROR, 1, "[%s] config_proc is already stopped.\n", m_p_log_id);
        return;
    }
    
    m_b_config_is_running = false;

    //20210913. youngjun han
    //pthread_create 직후 pthread_detach 방식으로 변경
    /*
	if( pthread_join( m_thread_t, NULL ) != 0 )
	{
		print_log( ERROR, 1,"[%s] pthread_join: config_proc:%s\n", \
		                    m_p_log_id, strerror( errno ));
	}
    */

	//safe_delete( m_p_prom_c );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void* config_routine( void * arg )
{
	CFpga * fpga = (CFpga *)arg;
	fpga->config_proc();
	pthread_exit( NULL );
	return 0;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CFpga::config_proc(void)
{
    print_log(DEBUG, 1, "[%s] start config_proc(file: %s)...\n", \
                        m_p_log_id, m_p_config_file_name );
    
    FILE*   p_file;
    u32     n_file_size;
    u32     n_addr;
    u8      p_data[FLASH_PAGE_SIZE];
    u32     n_last_size;
    u8		c_mf_id = 0;
    u16		w_dev_id = 0;
    //CPROM   prom_c(print_log);
    float   f_progress;
    
    m_b_config_result   = true;
    
    p_file = fopen( m_p_config_file_name, "rb" );
    
    if( p_file == NULL )
    {
        print_log(ERROR, 1, "[%s] file(%s) open failed.\n", \
                            m_p_log_id, m_p_config_file_name);
        m_b_config_result = false;
    }
    else
    {
        enable_interrupt( false );
        
        // erase
        print_log(DEBUG, 1, "[%s] Start erasing...\n", m_p_log_id);
        
        m_update_step_e = eUPDATE_STEP_ERASE;
        
        m_p_prom_c->set_flash_access_enable(true); //pin enable
        m_p_prom_c->set_qpi_mode(true);
        m_p_prom_c->read_id(&c_mf_id, &w_dev_id);
		print_log(INFO, 1, "[%s] NOR flash Manufacturer ID : 0x%X\n", m_p_log_id, c_mf_id);
		print_log(INFO, 1, "[%s] NOR flash Device ID : 0x%X\n", m_p_log_id, w_dev_id);
        
        if( m_p_prom_c->erase() == false )
        {
            m_b_config_result = false;
        }
        else
        {
            m_update_step_e = eUPDATE_STEP_WRITE;
            m_w_config_progress = 30;
            print_log(DEBUG, 1, "[%s] Erase success! Start writing...\n", m_p_log_id);
            // write
            n_addr = 0;
            n_file_size = file_get_size( p_file );
            memset( p_data, 0, FLASH_PAGE_SIZE );
            
            while ( m_b_config_is_running == true
                    && m_b_config_result == true )
            {
                if( n_addr + FLASH_PAGE_SIZE < n_file_size )
                {
                    fread( p_data, 1, FLASH_PAGE_SIZE, p_file );
                    m_b_config_result = m_p_prom_c->write_page( n_addr, p_data, FLASH_PAGE_SIZE );
                    n_addr += FLASH_PAGE_SIZE;
                    
                    f_progress = (float)n_addr/(float)n_file_size;
                    f_progress = 30 + f_progress * 50;
                    m_w_config_progress = (u16)f_progress;
                }
                else
                {
                    n_last_size = n_file_size - n_addr;
                    print_log(DEBUG, 1, "[%s] last page size: %d\n", \
                                        m_p_log_id, n_last_size);
                    fread( p_data, 1, n_last_size, p_file );
                    m_b_config_result = m_p_prom_c->write_page( n_addr, p_data, n_last_size );
                    n_addr += n_last_size;
                    break;
                }
            }
            fclose( p_file );
            
            // verify
            if( m_b_config_result == false )
            {
                print_log(ERROR, 1, "[%s] PROM write failed(%d/%d)\n", \
                                    m_p_log_id, n_addr, n_file_size);
            }
            else
            {
                m_w_config_progress = 80;
                print_log(DEBUG, 1, "[%s] Write sucess! Start verification...\n", m_p_log_id);
                m_update_step_e = eUPDATE_STEP_VERIFY;
                
                if( m_p_prom_c->verify(m_p_config_file_name) == false )
                {
                    m_b_config_result = false;
                }
                else
                {
                    m_b_config_result = true;
                }
            }
            
            m_update_step_e = eUPDATE_STEP_END;
        }
        
        m_p_prom_c->set_qpi_mode(false);    
        m_p_prom_c->set_flash_access_enable(false); //pin enable
    }
    
    m_w_config_progress = 100;
  //  safe_delete(m_p_prom_c);
    
    print_log(DEBUG, 1, "[%s] stop config_proc(result: %d)\n", \
                        m_p_log_id, m_b_config_result);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CFpga::config_get_progress(void)
{
    if( m_update_step_e == eUPDATE_STEP_ERASE
        || m_update_step_e == eUPDATE_STEP_VERIFY )
    {
        return m_p_prom_c->get_progress();
    }
    
    return m_w_config_progress;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CFpga::set_offset_calibration( bool i_b_set )
{
    print_log( DEBUG, 1, "[%s] set_offset_calibration: %d\n", m_p_log_id, i_b_set );
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
DEV_ERROR CFpga::hw_initialize( MODEL i_model_e, PANEL_TYPE i_panel_type_e, SCINTILLATOR_TYPE i_scintillator_type_e, SUBMODEL_TYPE i_submodel_type_e, u8 ctrlBoardVer)
{
	ioctl_data_t 	iod_t;
    s32				n_status = -1;
    DEV_ERROR 		result = eDEV_ERR_NO;
    	
    m_model_e = i_model_e;
  	m_panel_type_e = i_panel_type_e;
    m_scintillator_type_e = i_scintillator_type_e;
  	m_submodel_type_e = i_submodel_type_e;
    mCtrlBoardVer = ctrlBoardVer;

    m_p_prom_c = new CPROM(print_log);
    
    print_log( DEBUG, 1, "[%s] init(model: %d)\n", m_p_log_id, m_model_e );

	sys_command("mkdir %s", SYS_EXPOSURE_LOG_PATH);
    
    // FPGA driver 열기
    m_n_fd = open( "/dev/vivix_tg", O_RDWR|O_NDELAY );
    
	if( m_n_fd < 0 )
	{		
		print_log(ERROR,1,"[%s] /dev/vivix_tg device open error.\n", m_p_log_id);
		
		return eDEV_ERR_FPGA_DRIVER_OPEN_FAILED;
	}
	
    // TG, ROIC power enable, FPGA program, done pin check
    tg_power_enable(true);

    if(is_config_done())
    {
    	print_log(DEBUG,1,"[%s] FPGA configuration success.\n", m_p_log_id);
    }
    else
    {
    	print_log(DEBUG,1,"[%s] FPGA configuration failed.\n", m_p_log_id);
    	result = eDEV_ERR_FPGA_DONE_PIN;
    }
        
     // FPGA 에 model type setting, model type에 따른 queue buffer 생성 등 driver 사용을 위한 초기화
    memset(&iod_t, 0, sizeof(ioctl_data_t));
    iod_t.data[0] = (u8)m_model_e;
    n_status = ioctl( m_n_fd, VW_MSG_IOCTL_TG_SET_MODEL_AND_INIT, &iod_t );

    if( n_status < 0 )
    {
		print_log(ERROR, 1, "[%s] TG set model and init failed(%d).\n", m_p_log_id, n_status);
    } 

    // ROIC power on  --> roic discovery 에서 수행 
    /*
    ro_power_enable(true);
    CTime* p_roic_power_wait = NULL;
    p_roic_power_wait = new CTime(VW_ROIC_GATE_POWER_UP_TIME);
    while( p_roic_power_wait->is_expired() == false )
    {
    	sleep_ex(50000);       // 50 ms
    }
    safe_delete(p_roic_power_wait);
    print_log(DEBUG, 1, "[%s] ROIC power is stabilized\n", m_p_log_id );
    m_b_power_on = true;
    */

    m_b_power_on = true;
    
	return result;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
DEV_ERROR CFpga::register_roic_type(ROIC_TYPE i_roic_type_e)
{
    DEV_ERROR 		result = eDEV_ERR_NO;
    	
    m_roic_type_e = i_roic_type_e;
  	    
    //ROIC Driver open 및 intialize. 꼭 reg_init 전에 수행되어야함.
    //20210331. Youngjun Han. FPGA driver 로부터 ROIC driver 분리
    //print_log(ERROR,1,"[%s] ROIC TYPE: %d (dip switch)\n", m_p_log_id, i_roic_type_e);			//TEMP
    switch(m_roic_type_e)
    {
        case eROIC_TYPE_TI_2256GR: 
            m_n_fd_roic = open("/dev/vivix_ti_afe2256",O_RDWR|O_NDELAY);	
            break;

        case eROIC_TYPE_ADI_1255:
            m_n_fd_roic = open("/dev/vivix_adi_adas1256",O_RDWR|O_NDELAY);
            break;

        default:
            m_n_fd_roic = -1;
            print_log(ERROR, 1, "[%s] Unknown ROIC TYPE: %d.\n", m_p_log_id, m_roic_type_e);
            break;
    }

	if( m_n_fd_roic < 0 )
    {		
		print_log(ERROR,1,"[%s] /dev/vivix_roic device open error.\n", m_p_log_id);
		return eDEV_ERR_ROIC_DRIVER_OPEN_FAILED;
    } 
	
	return result;
}


/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
DEV_ERROR CFpga::roic_initialize(void)
{
    DEV_ERROR 		result = eDEV_ERR_NO;
    
    roic_init_inner();	

	return result;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
DEV_ERROR CFpga::reg_initialize(void)
{
    // FPGA 초기 설정값 읽기
    FILE *fp;
    FILE_ERR err;
    DEV_ERROR result = eDEV_ERR_NO;

    print_log(DEBUG, 1, "[%s] loading reg file for single...\n", m_p_log_id);

    try
    {
        fp = fopen(CONFIG_FPGA_FILE, "rb");
        if (fp == NULL) //예외1: fpga.bin open 안됨
        {
            err = eFILE_ERR_OPEN;
            throw err;
        }

        err = file_check_crc(fp, (u8 *)m_p_reg, sizeof(m_p_reg));
        fclose(fp);

        if (err != eFILE_ERR_NO) // eFILE_ERR_CRC or eFILE_ERR_SIZE, 예외2: fpga.bin crc error
        {
            throw err;
        }
    }
    catch (FILE_ERR err)
    {
        switch (err)
        {
        case eFILE_ERR_OPEN:
            print_log(ERROR, 1, "[%s] fpga.bin open failed\n", m_p_log_id);
            result = eDEV_ERR_FPGA_INIT_FAILED;
            break;

        case eFILE_ERR_CRC:
            print_log(ERROR, 1, "[%s] fpga.bin crc error\n", m_p_log_id);
            result = eDEV_ERR_FPGA_INIT_FAILED;
            break;

        case eFILE_ERR_SIZE:
            print_log(ERROR, 1, "[%s] fpga.bin size error\n", m_p_log_id);
            result = eDEV_ERR_FPGA_INIT_FAILED;
            break;

        default:
            break;
        }
        reg_load_default(); //error 발생 시 default 값 load
    }
	
	if( reg_init_inner() == false )
	{
		return eDEV_ERR_FPGA_INIT_FAILED;
	}

	return result;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::apply_gain_table_by_panel_type(PANEL_TYPE i_panel_type_e)
{
	if(m_panel_type_e != i_panel_type_e)
	{
		print_log(ERROR, 1, "[%s] Panel type is changed(%d -> %d), apply new gain table\n", m_p_log_id, m_panel_type_e, i_panel_type_e );
		m_panel_type_e = i_panel_type_e;
		set_gain_type();
	}
}
	
/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::reg_load_default(void)
{
    /* ROIC 공통 */

    m_p_reg[eFPGA_REG_ROIC_GAIN_TYPE] = 1; 			/* Gain type 1 */
    m_p_reg[eFPGA_REG_MSC_EN] = 1;             //max saturation cut enable by default
    m_p_reg[eFPGA_REG_LOGICAL_DIGITAL_GAIN] = 1024;
    m_p_reg[eFPGA_REG_GAIN_COMPENSATION] = 0;
    m_p_reg[eFPGA_REG_DRO_IMG_CAPTURE_OPTION] = 1;		/* 0x1A0 Double readout option enable & Smart W option disable */

	//(Trigger Mode, Smart W 등의 초기값에 근거하여 해당 register의 초기값 결정 필요)
    m_p_reg[eFPGA_REG_TCON_AED_SENSOR_OFF] = 0;      	// 0: AED Sensor power ON. 1: AED Sensor power OFF 

	m_p_reg[eFPGA_AED_ERR_NUM] = 4900;
	m_p_reg[eFPGA_XRAY_DELAY_NUM] = 500;
	m_p_reg[eFPGA_OFFSET_CAP_LIMIT_MIN] = 500;
	m_p_reg[eFPGA_OFFSET_CAP_LIMIT_MAX] = 1050;

    m_p_reg[eFPGA_REG_AED_SELECT] = 3;      	// AED select 0, 1
    if (getCtrlVersion() < CONTROL_BOARD_NUM_VW_REVISION)  
    {
        m_p_reg[eFPGA_REG_SEL_EDGE_DETECT] = 0x100;		// [8] AED 0 rising edge(= 1), [9] AED 1 falling edge(= 0)
    }
    else
    {
        m_p_reg[eFPGA_REG_SEL_EDGE_DETECT] = 0x300;		// [8] AED 0 rising edge(= 1), [9] AED 1 rising edge(= 1)
    }

    //SENSITIVITY 사양
    //2530/3643/4343 Normal 모델: 600
    //2530/3643/4343 Plus 모델: 750
    if( m_submodel_type_e == eSUBMODEL_TYPE_BASIC  )
    {
        m_p_reg[eFPGA_REG_SENSITIVITY] = 600;
    }
    else if( m_submodel_type_e == eSUBMODEL_TYPE_PLUS )
    {
        m_p_reg[eFPGA_REG_SENSITIVITY] = 750;
    }
    else        //unknown
    {
        print_log(ERROR, 1, "[%s] Unknown SUBMODEL TYPE: %d\n", m_p_log_id, m_submodel_type_e);
    }

    switch(m_roic_type_e)
    {
        case eROIC_TYPE_ADI_1255:
            print_log(INFO, 1, "[%s] Load default register values For ADI ROIC...\n", m_p_log_id);  
            m_p_reg[0x0D] = 0;
            m_p_reg[0x0E] = 4;
            m_p_reg[0x0F] = 259;
            
            m_p_reg[0x12] = 0;
            m_p_reg[0x13] = 0;
            m_p_reg[0x14] = 4;
            
            m_p_reg[eFPGA_REG_FLUSH_GATE_HEIGHT] = 256;
            
            m_p_reg[0x20] = 0;
            m_p_reg[0x21] = 1;          // DR trigger
            
            m_p_reg[0x30] = 300;
            m_p_reg[0x31] = 300;
            m_p_reg[0x32] = 500;
            m_p_reg[0x33] = 1;
            m_p_reg[eFPGA_REG_DRO_PANEL_BIAS_NUM]		= 0;	/* 0x34 */
            
            m_p_reg[eFPGA_REG_EXTRA_FLUSH] = 535;
            m_p_reg[0x36] = 102;
            m_p_reg[eFPGA_REG_DRO_INTRA_FLUSH_NUM]		= 75;	/* 0x1A1 */
            
            m_p_reg[0x40] = 0;
            m_p_reg[0x41] = 0;
            m_p_reg[0x42] = 0;
            m_p_reg[0x43] = 0;
            m_p_reg[0x48] = 100;
            m_p_reg[0x4A] = 0;
            
            m_p_reg[eFPGA_REG_ROIC_CFG_0] = 0x0151;     // 1417N: IFS of CsI(bit[4:0] => 17(0x11)
            m_p_reg[eFPGA_REG_ROIC_CFG_1] = 0x01A0;
            m_p_reg[eFPGA_REG_ROIC_CFG_2] = 0x010E;
            m_p_reg[eFPGA_REG_ROIC_CFG_3] = 0x0107;
            m_p_reg[eFPGA_REG_ROIC_CFG_4] = 0x0092;
            m_p_reg[eFPGA_REG_ROIC_CFG_5] = 0x0014;
            m_p_reg[eFPGA_REG_ROIC_CFG_6] = 0x0058;
            m_p_reg[eFPGA_REG_ROIC_CFG_7] = 0x0036;
            m_p_reg[eFPGA_REG_ROIC_CFG_8] = 0x0179;
            m_p_reg[eFPGA_REG_ROIC_CFG_9] = 2;
            m_p_reg[eFPGA_REG_ROIC_CFG_10] = 0;
            m_p_reg[eFPGA_REG_ROIC_CFG_11] = 0x0018;
            m_p_reg[eFPGA_REG_ROIC_CFG_12] = 0x0102;
            m_p_reg[eFPGA_REG_ROIC_CFG_13] = 0x0023;
            m_p_reg[eFPGA_REG_ROIC_CFG_14] = 0x002B;
            m_p_reg[eFPGA_REG_ROIC_CFG_15] = 0x0008;
            
            m_p_reg[eFPGA_REG_ROIC_ACTIVE_POWER_MODE] = 0;
            
            m_p_reg[eFPGA_REG_ROIC_SYNC_WIDTH] = 0x0001;
            m_p_reg[eFPGA_REG_ROIC_ACLK_WIDTH] = 0x0001;
            m_p_reg[eFPGA_REG_ROIC_ACLK_NUM] = 0x000A;
            
            m_p_reg[eFPGA_REG_ROIC_ACLK0] = 49;
            m_p_reg[eFPGA_REG_ROIC_ACLK1] = 49;
            m_p_reg[eFPGA_REG_ROIC_ACLK2] = 700;
            m_p_reg[eFPGA_REG_ROIC_ACLK3] = 99;
            m_p_reg[eFPGA_REG_ROIC_ACLK4] = 2401;
            m_p_reg[eFPGA_REG_ROIC_ACLK5] = 3009;
            m_p_reg[eFPGA_REG_ROIC_ACLK6] = 99;
            m_p_reg[eFPGA_REG_ROIC_ACLK7] = 99;
            m_p_reg[eFPGA_REG_ROIC_ACLK8] = 2301;
            m_p_reg[eFPGA_REG_ROIC_ACLK9] = 49;
            m_p_reg[eFPGA_REG_ROIC_ACLK10] = 0;
            m_p_reg[eFPGA_REG_ROIC_ACLK11] = 0;
            m_p_reg[eFPGA_REG_ROIC_ACLK12] = 0;
            m_p_reg[eFPGA_REG_ROIC_ACLK13] = 0;
            
            m_p_reg[0xC0] = 0x0008;
            m_p_reg[0xC1] = 25;
            
            m_p_reg[0xD0] = 0;
            m_p_reg[0xD1] = 0x0003;
            m_p_reg[0xD2] = 0x0102;
            m_p_reg[0xD3] = 0x0502;
            m_p_reg[0xD4] = 0x0005;
            m_p_reg[0xD5] = 0x0102;
            m_p_reg[0xD6] = 0;
            m_p_reg[0xD7] = 0;
            m_p_reg[0xD8] = 0;
            m_p_reg[0xD9] = 0x0706;
            m_p_reg[0xDA] = 0;
            
            m_p_reg[0xE0] = 0x0001;
            m_p_reg[0xE1] = 0x0001;
            m_p_reg[0xE2] = 0x0006;
            m_p_reg[0xE3] = 0x0009;

            m_p_reg[eFPGA_REG_GCLK0_FLUSH] = 2;
            m_p_reg[eFPGA_REG_GCLK1_FLUSH] = 10;
            m_p_reg[eFPGA_REG_GCLK2_FLUSH] = 27;
            m_p_reg[eFPGA_REG_GCLK3_FLUSH] = 100;
            m_p_reg[eFPGA_REG_GCLK4_FLUSH] = 85;
            m_p_reg[eFPGA_REG_GCLK5_FLUSH] = 500;
            m_p_reg[eFPGA_REG_GCLK6_FLUSH] = 0;
            m_p_reg[eFPGA_REG_GCLK7_FLUSH] = 0;
            
            m_p_reg[eFPGA_GCLK0_READ_OUT] = 49;
            m_p_reg[eFPGA_GCLK1_READ_OUT] = 49;
            m_p_reg[eFPGA_GCLK2_READ_OUT] = 49;
            m_p_reg[eFPGA_GCLK3_READ_OUT] = 49;
            m_p_reg[eFPGA_GCLK4_READ_OUT] = 49;
            m_p_reg[eFPGA_GCLK5_READ_OUT] = 49;
            m_p_reg[eFPGA_GCLK6_READ_OUT] = 3007;
            m_p_reg[eFPGA_GCLK7_READ_OUT] = 2999;
            m_p_reg[eFPGA_GCLK8_READ_OUT] = 2557;
            m_p_reg[eFPGA_GCLK9_READ_OUT] = 0;
            m_p_reg[eFPGA_GCLK10_READ_OUT] = 0;
            m_p_reg[eFPGA_GCLK11_READ_OUT] = 0;
            m_p_reg[eFPGA_GCLK12_READ_OUT] = 0;
            m_p_reg[eFPGA_GCLK13_READ_OUT] = 0;
            m_p_reg[eFPGA_GCLK14_READ_OUT] = 0;

            m_p_reg[eFPGA_REG_DDR_IDLE_STATE] 	= 0;			/* 0x127 DDR idle state : power off -> FW 에서 correction 수행*/
            
            break;

        case eROIC_TYPE_TI_2256GR:
            print_log(INFO, 1, "[%s] Load default register values For TI ROIC...\n", m_p_log_id);  
            m_p_reg[0x0D] = 0;
            m_p_reg[0x0E] = 4;
            m_p_reg[0x0F] = 259;
            
            m_p_reg[0x12] = 0;
            m_p_reg[0x13] = 0;
            m_p_reg[0x14] = 4;
            
            m_p_reg[eFPGA_REG_FLUSH_GATE_HEIGHT] = 256;
            
            m_p_reg[0x20] = 0;
            m_p_reg[0x21] = 1;          // DR trigger
            m_p_reg[eFPGA_REG_READOUT_SYNC] = 0;

            m_p_reg[0x30] = 300;
            m_p_reg[0x31] = 300;
            m_p_reg[0x32] = 500;
            m_p_reg[0x33] = 1;
            
            m_p_reg[eFPGA_REG_DRO_PANEL_BIAS_NUM]		= 0;	/* 0x34 */
            
            m_p_reg[eFPGA_REG_EXTRA_FLUSH] = 400;            
            m_p_reg[0x36] = 0;
            
            m_p_reg[eFPGA_REG_DRO_2ND_EXP_RATIO]		= 100;	/* 0x3f */
            m_p_reg[eFPGA_REG_DRO_2ND_EXP_TIME]			= m_p_reg[0x32] * m_p_reg[eFPGA_REG_DRO_2ND_EXP_RATIO];	/* 0x3e */
            
            m_p_reg[0x40] = 0;
            m_p_reg[0x41] = 0;
            m_p_reg[0x42] = 0;
            m_p_reg[0x43] = 0;
            m_p_reg[0x48] = 100;
            m_p_reg[0x4A] = 1;
            
            m_p_reg[eFPGA_REG_ROIC_CLK_EN] = 0;         //MCLK Generation disable (default)	

            m_p_reg[eFPGA_REG_DRO_OFFSET_ON]			= 1;	/* 0x77 Double readout offset on */
            
            /* ROIC TPb register */
            m_p_reg[0x80] = 0x0101; 	// ROIC 0x40   
            m_p_reg[0x81] = 0x0000;		// ROIC 0x41
            m_p_reg[0x82] = 0x0000;		// ROIC 0x42
            m_p_reg[0x83] = 0x0000;		// ROIC 0x43
            m_p_reg[0x84] = 0x0000;		// ROIC 0x46
            m_p_reg[0x85] = 0x0000;		// ROIC 0x47
            m_p_reg[0x86] = 0x0000;		// ROIC 0x4a
            m_p_reg[0x87] = 0x0000;		// ROIC 0x4b
            m_p_reg[0x88] = 0x0000;		// ROIC 0x4C
            m_p_reg[0x89] = 0x0000;		// ROIC 0x50
            m_p_reg[0x8A] = 0x0000;		// ROIC 0x51
            m_p_reg[0x8B] = 0x0000;		// ROIC 0x52
            m_p_reg[0x8C] = 0x0000;		// ROIC 0x53
            m_p_reg[0x8D] = 0x0000;		// ROIC 0x54
            m_p_reg[0x8E] = 0x0000;		// ROIC 0x55
            
            m_p_reg[0x91] = 4;
            
            m_p_reg[0xD1] = 0x0003;
            m_p_reg[0xD2] = 0x0102;
            m_p_reg[0xD3] = 0x0502;
            m_p_reg[0xD4] = 0x0005;
            m_p_reg[0xD5] = 0x0102;
            m_p_reg[0xD6] = 0;
            m_p_reg[0xD7] = 0;
            m_p_reg[0xD8] = 0;
            m_p_reg[0xD9] = 0x0908;
            m_p_reg[0xDA] = 0;
            
            m_p_reg[0xE0] = 0x0001;
            m_p_reg[0xE1] = 0x0001;
            m_p_reg[0xE2] = 0x0006;
            m_p_reg[0xE3] = 0xb;
            m_p_reg[0xE4] = 2;
            m_p_reg[0xE5] = 10;
            m_p_reg[0xE6] = 27;
            m_p_reg[0xE7] = 100;
            m_p_reg[0xE8] = 85;
            m_p_reg[0xE9] = 500;
            m_p_reg[0xEA] = 0;
            m_p_reg[0xEB] = 0;
            
            m_p_reg[0xEC] = 0x17c;
            m_p_reg[0xED] = 0x672;
            m_p_reg[0xEE] = 0x19;
            m_p_reg[0xEF] = 0x19;

            m_p_reg[0xF0] = 0x19;
            m_p_reg[0xF1] = 0x19;
            m_p_reg[0xF2] = 0x19;
            m_p_reg[0xF3] = 0x19;
            m_p_reg[0xF4] = 0xaa;
            m_p_reg[0xF5] = 0xbb8;
            m_p_reg[0xF6] = 0x19;
            m_p_reg[0xF7] = 0;
            m_p_reg[0xF8] = 0;
            m_p_reg[0xF9] = 0;
            m_p_reg[0xFA] = 0;
            m_p_reg[0xFB] = 0;
            
            m_p_reg[eFPGA_REG_DDR_IDLE_STATE] 	= 0;			/* 0x127 DDR idle state : power off -> FW 에서 correction 수행*/
                
            m_p_reg[eFPGA_REG_DRO_INTRA_FLUSH_NUM]		= 75;	/* 0x1A1 */
            
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_00] = 0x0000;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_10] = 0x0800;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_11] = 0x0430;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_12] = 0x0000;

            if (hw_get_sig_version() < SIGNAL_BOARD_NUM_VW_REVISION)   m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_13] = 0xF8A7; /* ROIC Nap with PLL off */
            else                            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_13] = 0xFFBF; /* ROIC Power down with PLL off */
            
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_16] = 0x00C0;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_18] = 0x0001;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_2C] = 0x0001;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_30] = 0x0000;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_31] = 0x0000;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_32] = 0x0000;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_33] = 0x0000;
            
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_40] = 0x0105;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_41] = 0x0102;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_42] = 0x0649;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_43] = 0x4afe;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_46] = 0x0d4a;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_47] = 0xc1ff;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_4A] = 0x06bd;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_4B] = 0x0000;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_4C] = 0x0000;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_50] = 0x0000;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_51] = 0x4bff;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_52] = 0x0000;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_53] = 0x0000;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_54] = 0x0000;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_55] = 0x0000;
            
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_5A] = 0x0000;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_5C] = 0xCBDE;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_5D] = 0x0610;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_5E] = 0x2000;
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_61] = 0x4000;  
        
            m_p_reg[eFPGA_REG_TI_ROIC_MCLK] 	 = 1280;		/* MCLK_L 12.8MHz */
            m_p_reg[eFPGA_REG_TI_ROIC_DAC_COMP1] = 400;  		/* COMP1 0.4V */     
            m_p_reg[eFPGA_REG_TI_ROIC_DAC_COMP3] = 875;  		/* COMP3 0.875V */
            m_p_reg[eFPGA_REG_TI_ROIC_LOW_POWER] = 1;  			/* 0: Normal power, 1: Low power */

            m_p_reg[eFPGA_TI_ROIC_SCAN] 		= 0x0003; 
            break;

        default:
            print_log(ERROR, 1, "[%s] Unknown ROIC TYPE: %d\n", m_p_log_id, m_roic_type_e);
            break;
    }

}
																																		
/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    FPGA register programming & ROIC register programming & FW use FPGA register programming
*******************************************************************************/
bool CFpga::reg_init_inner(void)
{
    u16 i, data;
    bool b_result = true;
    
    //roic_init();
    /*양산품 대응 코드*/
    change_reg_setting_for_MP();

    for( i = 0; i < REG_ADDRESS_MAX; i++ )
    {
    	// 0xd0 STV Direction 값 read-only 처림 동작하여 초기화 항목에서 제외
    	// 기존 양산품 대응을 위한 예외처리
    	if( i == eFPGA_REG_STV_DIR )
        	continue;
        
        if( m_p_reg[i] != 0xFFFF )
        {
            write( i, m_p_reg[i] );
            data = read(i);
            
            if( data != m_p_reg[i]
            	&& data != 0xDEAD )
            {
                print_log(ERROR, 1, "[%s] addr: 0x%04X, data: 0x%04X(w: 0x%04X)\n", m_p_log_id, i, data, m_p_reg[i]);
                b_result = false;
            }
        }
    }

	return b_result;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::set_aed_error_time(void)
{
	u32 n_exp_time = get_exp_time();
	u16 w_aed_error_time = 0;
	
	// user mode 에서 설정가능한 exposure time 최대값이 10초
	if(n_exp_time > 10000)
	{
		w_aed_error_time = 10000;
	}	
	else
	{
		w_aed_error_time = (u16)n_exp_time;
	}
	
	// exposure time + 여유분 3s
	w_aed_error_time += 3000;
	
	// exposure time이 500ms와 같이 짧게 설정된 경우에도 최소 5초간은 aed triggr 신호가 감지되어야 error 처리하도록 함
	if(w_aed_error_time < 5000)
	{
		w_aed_error_time = 5000;
	}

	write( eFPGA_REG_AED_HIGH_CNT, w_aed_error_time);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::set_expected_wakeup_time(void)
{
	// single readout offset 이면 wake up time 7초
	if(is_double_readout_enabled() == false)
		m_n_expected_wakeup_time = 7;
	else	// double readout offset 이면 wake up time 5초
		m_n_expected_wakeup_time = 5;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CFpga::get_expected_wakeup_time(void)
{
	return m_n_expected_wakeup_time;
}

		
/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::roic_init_inner(void)
{
	
	ioctl_data_t 	iod_t;
    s32				n_status = -1;
    //bool            clk_enable  = true;
 
    ioctl( m_n_fd_roic, VW_MSG_IOCTL_ROIC_INITIAL, &iod_t );
    memcpy( &n_status, iod_t.data, sizeof(n_status) ); 

    if( n_status < 0 )
    {
		print_log(ERROR, 1, "[%s] ROIC init is failed(%d).\n", m_p_log_id, n_status);
    } 

    write( eFPGA_REG_ROIC_RESET, 0 );
    write( eFPGA_REG_ROIC_RESET, 2 );
    
	write( eFPGA_REG_AED_ERROR, 0 );
	
	write( eFPGA_REG_SET_PULSE_AED_STB_TIME, 300 );
	
	set_tg_line_count(IMAGE_LINE_COUNT);
	
	update_pseudo_flush_num();
	
	set_sensitivity();
	
	set_aed_error_time();
	set_expected_wakeup_time();	
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       저장에 실패한 레지스터의 주소
* \note
*******************************************************************************/
u16 CFpga::reg_save( u16 i_w_start, u16 i_w_end )
{
    u16 i;
    
    if( i_w_start >= REG_ADDRESS_MAX
        || i_w_end >= REG_ADDRESS_MAX )
    {
        return REG_ADDRESS_MAX;
    }
    
    print_log(DEBUG, 1, "[%s] start: 0x%04X, end: 0x%04X\n", m_p_log_id, i_w_start, i_w_end);
    for( i = i_w_start; i <= i_w_end; i++ )
    {
        m_p_reg[i] = read(i);
    }    
    return 0;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
bool CFpga::reg_save_file(void)
{
    FILE *fp;
	FILE_ERR err;
	
	fp = fopen(CONFIG_FPGA_FILE, "wb");
	
	if( fp == NULL )
	{
		print_log(ERROR,1,"[%s] %s open(w) failed\n", m_p_log_id, CONFIG_FPGA_FILE );
		return false;
	}
	
	file_write_bin( fp, (u8*)m_p_reg, sizeof(m_p_reg) );
	fclose(fp);
	
	err = file_copy_to_flash(CONFIG_FPGA_FILE, SYS_CONFIG_PATH);
	
	if( err == eFILE_ERR_NO )
    {
        print_log(DEBUG, 1, "[%s] %s write success\n", m_p_log_id, CONFIG_FPGA_FILE );
        return true;
    }
    
    return false;
}

/******************************************************************************/
/**
* \brief        
* \param        none
* \return       none
* \note
*******************************************************************************/
void CFpga::reg_print( bool i_b_local )
{
    u16 i;
    
    for( i = 0; i < REG_ADDRESS_MAX; i++ )
    {
        if( m_p_reg[i] != 0xFFFF )
        {
            printf("[0x%04X] 0x%04X(read: 0x%04X)\n", i, m_p_reg[i], read(i) );
        }
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::set_test_pattern( u16 i_w_type )
{
    u16 w_reg;
    
    if( i_w_type == 0 )
    {
        lock();
        w_reg = io_read( eFPGA_REG_SELECT_TCON_OP );
        w_reg = w_reg & 0xFEFF;     // clear bit 8
        io_write( eFPGA_REG_SELECT_TCON_OP, w_reg );
        unlock();
        
    }
    else if( i_w_type < 10 )
    {
        lock();
        w_reg = io_read( eFPGA_REG_SELECT_TCON_OP );
        w_reg = w_reg | 0x0100;     // set bit 8
        io_write( eFPGA_REG_SELECT_TCON_OP, w_reg );
        unlock();
        
        write( eFPGA_REG_TEST_PATTERN, (i_w_type - 1) );
        
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::get_version( s8* o_p_ver )
{
    u16 w_debug;
    u16 w_version;
    s8  p_ver[128];
    
    memset( p_ver, 0, sizeof(p_ver) );
    w_version = read(eFPGA_REG_VERSION);
    w_debug = read(eFPGA_REG_VERSION_SUB);
    
    sprintf(p_ver, "%d.%d.%d", (w_version & 0xFF00)>>8, (w_version & 0x00FF), w_debug);
    
    strcpy( o_p_ver, p_ver );
}

			
/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::power_down(void)
{
    if( m_b_power_on == true )
    {
        m_b_power_on = false;
        
    	enable_interrupt( false );
    	
        // TG PWR disable, ROIC PWR disable
    	power_enable( false );
        
        print_log(DEBUG, 1, "[%s] Power down\n", m_p_log_id );
        
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CFpga::is_config_done(void)
{
    bool            b_result = false;
    ioctl_data_t 	iod_t;
    iod_t.data[0] = 0;
    
    // DONE pin check
    CTime*  p_wait_c = new CTime(5000);
    u16     w_count = 0;
    
    while( w_count < 3 )
    {
        ioctl( m_n_fd, VW_MSG_IOCTL_TG_FPGA_DONE, &iod_t ); 
        
        if( iod_t.data[0] > 0 )
        {
            print_log(DEBUG, 1, "[%s] FPGA configuration complete\n", m_p_log_id );
            b_result = true;
            break;
        }
        
        if( p_wait_c == NULL )
        {
            ioctl( m_n_fd, VW_MSG_IOCTL_TG_FPGA_CONFIG, &iod_t );
            p_wait_c = new CTime(5000);
        }
        else if( p_wait_c->is_expired() == true )
        {
            print_log(ERROR, 1, "[%s] FPGA done pin timeout(%d)\n", m_p_log_id, w_count );
            safe_delete(p_wait_c);
            w_count++;
        }
        else
        {
            sleep_ex(100000);       // 100 ms
        }
        
    }
    safe_delete(p_wait_c);
    
    return b_result;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CFpga::is_register_ready(void)
{
    bool    b_result = false;
    CTime*  p_wait_c = new CTime(5000);
    
    while( p_wait_c->is_expired() == false )
    {
        if( read(0) == 0xDEAD )
        {
            b_result = true;
            break;
        }
    	sleep_ex(50000);
    }
    
    safe_delete(p_wait_c);
    
    if( b_result == false )
    {
        print_log( ERROR, 1, "[%s] fpga register ready failed\n", m_p_log_id );
    }
    
    return b_result;
}
	
/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CFpga::power_up(void)
{
    bool 	b_result = false;
    u16		i;
    
    if( m_b_power_on == false )
    {
        print_log(DEBUG, 1, "[%s] Power up\n", m_p_log_id );
        power_enable( true );

        for( i = 0; i < 3; i++ )
        {
            if( is_config_done() == true )
            {
                if( is_register_ready() == true )
                {
                    b_result = true;
                    break;
                }
            }
        }
        
    	if( b_result == true )
    	{
    	    m_b_power_on = true;
    	    reg_init_inner();
            roic_init_inner();
    	    write(eFPGA_REG_DETECTOR_MODEL_SEL, m_model_e);
    	    
			if( is_ready(5000, true) == true )
            {
            	safe_delete(m_p_flush_wait_c);
            	if( is_double_readout_enabled() == false )
                {
                	print_log( ERROR, 1, "[%s] flush begin \n", m_p_log_id );
	                m_p_flush_wait_c = new CTime(5000);
	            }
                
                //write( eFPGA_REG_XRAY_READY, 1 );
                
                //enable_interrupt( true );
            }
            else
            {
                b_result        = false;
                m_b_power_on    = false;
                print_log( ERROR, 1, "[%s] fpga power up failed\n", m_p_log_id );
            }
        }
    }
    
    return b_result;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CFpga::is_ready( u16 i_w_timeout, bool i_b_wakeup )
{
    fpga_reg_u reg_u;
    CTime wait_c(i_w_timeout);
    
    if( m_b_power_on == false
        || m_b_config_is_running == true )
    {
        return false;
    }
    
    if( i_b_wakeup == false
        && read(eFPGA_REG_XRAY_READY) == 0 )
    {
        return false;
    }
    
    while(1)
    {
        reg_u.TCON_STATE.DATA = read(eFPGA_REG_TCON_STATE);
    
        if( reg_u.TCON_STATE.BIT.IDLE == 1
            || reg_u.TCON_STATE.BIT.FLUSH == 1 )
        {
            return true;
        }
        
        if( wait_c.is_expired() == true )
        {
            print_log(ERROR, 1, "[%s] FPGA ready timeout (0x%04X)\n", m_p_log_id, reg_u.TCON_STATE.DATA);
            break;
        }
        else
        {
            sleep_ex(10000);       // 10 ms
        }
    }
    
    return false;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CFpga::line_intr_check(void)
{
    ioctl_data_t	ctl_t;
    u16             w_offset_err = 0;
    bool            b_readout_done_timeout = false;
	
	memset( &ctl_t, 0, sizeof(ioctl_data_t) );
	ioctl( m_n_fd, VW_MSG_IOCTL_TG_READOUT_DONE, &ctl_t ); 
	
	line_intr_lock();
	
	if( ctl_t.data[0] > 0
	    && m_p_line_intr_c == NULL )
	{
	    print_log(DEBUG, 1, "[%s] readout done\n", m_p_log_id );
	    
	    memcpy( &w_offset_err, &ctl_t.data[1], sizeof(w_offset_err) );
	    
	    safe_delete( m_p_line_intr_c );
	    m_p_line_intr_c = new CTime(3000);
	}
	
	if( m_p_line_intr_c != NULL )
	{
	    if( m_p_line_intr_c->is_expired() == true )
    	{
    	    print_log(DEBUG, 1, "[%s] readout done time out !!!!!!!!!!\n", m_p_log_id );
    	    b_readout_done_timeout = true;
    	}
    }
    
    line_intr_unlock();
    
    if( b_readout_done_timeout == true )
    {
        line_intr_end();
    }
	
	return w_offset_err;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::line_intr_end(void)
{
    ioctl_data_t	ctl_t;
    
	ioctl( m_n_fd, VW_MSG_IOCTL_TG_READOUT_DONE_CLEAR, &ctl_t );
	
	print_log(DEBUG, 1, "[%s] readout done clear\n", m_p_log_id );
	
	line_intr_lock();
	safe_delete( m_p_line_intr_c );
	line_intr_unlock();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::set_capture_count( u32 i_n_count )
{
    write( eFPGA_REG_CAPTURE_CNT_LOW,   (i_n_count & 0x0000FFFF) );
    write( eFPGA_REG_CAPTURE_CNT_HIGH,  (i_n_count & 0xFFFF0000) >> 16 );
    
    print_log(DEBUG, 1, "[%s] catpure count: %d\n", m_p_log_id, get_capture_count());
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CFpga::get_capture_count(void)
{
    u32 n_count;
    
    n_count = read( eFPGA_REG_CAPTURE_CNT_LOW );
    n_count = n_count | (read( eFPGA_REG_CAPTURE_CNT_HIGH ) << 16);
    
    return n_count;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::offset_start( bool i_b_start, u32 i_n_target_count, u32 i_n_skip_count )
{
    u16 w_reg;
    
    lock();
    
    w_reg = io_read( eFPGA_REG_OFFSET_CAP_CTRL );
    
    if( i_b_start == true )
    {
        w_reg = w_reg | 0x0001;     // set bit 0
		io_write( FPGA_ISP_REG_OFFSET_CAL_ENABLE, 1 );
    }
    else
    {
        w_reg = w_reg & 0xFFFE;     // clear bit 0
		io_write( FPGA_ISP_REG_OFFSET_CAL_ENABLE, 0 );
    }
    io_write( eFPGA_REG_OFFSET_CAP_CTRL, w_reg );
    unlock();
    
    print_log(DEBUG, 1, "[%s] write 0x%04X, 0x%04X\n", m_p_log_id, eFPGA_REG_OFFSET_CAP_CTRL, w_reg );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::offset_set_recapture( bool i_b_recapture )
{
    u16 w_reg;
    
    lock();
    w_reg = io_read( eFPGA_REG_OFFSET_CAP_CTRL );
    
    if( i_b_recapture == true )
    {
        w_reg = w_reg | 0x0010;
    }
    else
    {
        w_reg = w_reg & 0xFFEF;
    }
    io_write( eFPGA_REG_OFFSET_CAP_CTRL, w_reg );
    unlock();
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::aed_trigger(void)
{
    u16 w_reg;
    
    lock();
    w_reg = io_read( eFPGA_REG_TRIGGER_TYPE );
    w_reg = w_reg | 0x0100;
    io_write( eFPGA_REG_TRIGGER_TYPE, w_reg );
    unlock();
    
    printf("AED trigger start ========\n");
    
    
    write( eFPGA_REG_TEST_AED_TRIGGER, 1 );
    
    CTime wait_c(10);
    while( wait_c.is_expired() == false )
    {
        sleep_ex(1000);       // 10 ms
    }
    
    printf("AED trigger wait ========\n");
        
    write( eFPGA_REG_TEST_AED_TRIGGER, 0 );
    
    lock();
    w_reg = io_read( eFPGA_REG_TRIGGER_TYPE );
    w_reg = w_reg & 0xFEFF;
    io_write( eFPGA_REG_TRIGGER_TYPE, w_reg );
    unlock();
    
    printf("AED trigger end ========\n");
    
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::set_immediate_transfer( bool i_b_enable )
{
    ioctl_data_t	ctl_t;
    
    ctl_t.data[0] = i_b_enable;
	
	ioctl( m_n_fd, VW_MSG_IOCTL_TG_IMMEDIATE_TRANSFER, &ctl_t );
	
	print_log(DEBUG, 1, "[%s] immediate transfer: %d\n", m_p_log_id, i_b_enable );
	
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CFpga::get_pre_exp_flush_num(void)
{
    return read( eFPGA_REG_XRAY_LATENCY_CNT );
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CFpga::is_flush_enough(void)
{
    CTime   wait_c(500);
    u16     w_time = read(eFPGA_REG_PULSE_AED_STB_CNT);
    
    if( w_time == 0 )
    {
        return true;
    }
    
    print_log(DEBUG, 1, "[%s] flush wait time: %d\n", m_p_log_id, w_time );
    
    while( wait_c.is_expired() == false )
    {
        if( read(eFPGA_REG_PULSE_AED_STB_CNT) == 0 )
        {
            return true;
        }
        sleep_ex(10000);
    }
    
    print_log(DEBUG, 1, "[%s] flush wait timeout: %d\n", m_p_log_id, read(eFPGA_REG_PULSE_AED_STB_CNT) );
    
    return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::flush_wait_end(void)
{
	if(m_p_flush_wait_c == NULL)
		return;
		
    while( m_p_flush_wait_c->is_expired() == false )
    {
        sleep_ex(500000);       // 500ms
    }
    
    safe_delete( m_p_flush_wait_c );
    
    print_log( ERROR, 1, "[%s] flush end \n", m_p_log_id );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CFpga::get_vcom_swing_time(void)
{
    u32 n_time;
    
    n_time = read( eFPGA_REG_VCOM_SWING_CNT_LOW );
    n_time = n_time | (read( eFPGA_REG_VCOM_SWING_CNT_HIGH ) << 16);
    
    return n_time;    
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16  CFpga::get_vcom_remain(void)
{
    return read( eFPGA_REG_VCOM_REMAIN_CNT );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CFpga::is_extra_flush_error(void)
{
    fpga_reg_u  reg_u;
    u16         w_error;
    
    lock();
    reg_u.TCON_STATE.DATA   = io_read( eFPGA_REG_TCON_STATE );
    w_error                 = io_read( eFPGA_REG_IMAGE_ERROR_EXTRA_FLUSH );
    unlock();

    if( reg_u.TCON_STATE.BIT.FLUSH == 1
        && w_error > 0 )
    {
        return true;
    }
    
    return false;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CFpga::is_xray_detected_in_extra_flush_section(void)
{
    if( is_extra_flush_error() == false )
    {
        return false;
    }
    
    sleep_ex(10000);
    
    if( is_extra_flush_error() == false )
    {
        return false;
    }
    
    write(eFPGA_REG_IMAGE_ERROR_EXTRA_FLUSH, 0);
    
    print_log(DEBUG, 1, "[%s] x-ray is detected in extra flush section\n", m_p_log_id);
    
    return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::enable_interrupt( bool i_b_enable )
{
    ioctl_data_t 	iod_t;

    if( i_b_enable == true )
    {
        iod_t.data[0] = 1;
    }
    else
    {
        iod_t.data[0] = 0;
    }
    
    ioctl( m_n_fd, VW_MSG_IOCTL_TG_INTERRUPT_ENABLE, &iod_t );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::power_enable( bool i_b_on )
{
	tg_power_enable(i_b_on);
	ro_power_enable(i_b_on);
	
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::tg_power_enable( bool i_b_on )
{
    ioctl_data_t 	iod_t;
    
    iod_t.data[0] = (u8)i_b_on;
    
    ioctl( m_n_fd, VW_MSG_IOCTL_TG_PWR_EN, &iod_t ); 
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::ro_power_enable( bool i_b_on )
{
    ioctl_data_t 	iod_t;
	iod_t.data[0] = (u8)i_b_on;
    ioctl( m_n_fd_roic, VW_MSG_IOCTL_RO_PWR_EN, &iod_t ); 
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CFpga::reinitialize(void)
{
    bool b_ret;

    b_ret = is_config_done();

    if (b_ret)
    {
        reg_init_inner();
        roic_init_inner();
    }

    write(eFPGA_REG_DETECTOR_MODEL_SEL, m_model_e);
    return b_ret;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CFpga::get_temperature( u8 i_c_id )
{
    u32 n_temp_h, n_temp_l;
    float f_temp;
    
    if( i_c_id == 0 )
    {
        n_temp_l = read(0x70);
        n_temp_h = read(0x71);
    }
    else
    {
        n_temp_l = read(0x72);
        n_temp_h = read(0x73);
    }
    
    if( n_temp_l == 0 )
    {
        return 0;
    }
    
    f_temp = 421.0f - (751.0f * (float)n_temp_h) / (float)n_temp_l;	/* for TMP05S	*/
    
    if( f_temp < 0 )
    {
        return 0;
    }
    
    return (u16)(f_temp * 10);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::set_tg_time( u32 i_n_time )
{
    ioctl_data_t 	iod_t;
    s8              p_date[256];\
    u32             n_time;
    
    if( i_n_time == 0 )
    {
        n_time = time(NULL);
    }
    else
    {
        n_time = i_n_time;
    }
    
    memset( p_date, 0, sizeof(p_date) );
	get_time_string( n_time, p_date );
	
	memset( &iod_t, 0, sizeof(ioctl_data_t) );
	memcpy( iod_t.data, &n_time, sizeof(n_time) );
	
	ioctl( m_n_fd, VW_MSG_IOCTL_TG_TIME, &iod_t );

	print_log(DEBUG, 1, "[%s] set_tg_time: %s(%d)\n", m_p_log_id, p_date, n_time );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::print_trigger_time(void)
{
    u32 p_time[3];
    u16 w_low, w_high;
    
    memset( p_time, 0, sizeof(p_time) );
    
    w_low       = read(eFPGA_REG_TRIGGER_TIME_STAMP_00_LB);
    w_high      = read(eFPGA_REG_TRIGGER_TIME_STAMP_00_HB);
    p_time[0]   = (w_high << 16) | w_low;

    w_low       = read(eFPGA_REG_TRIGGER_TIME_STAMP_01_LB);
    w_high      = read(eFPGA_REG_TRIGGER_TIME_STAMP_01_HB);
    p_time[1]   = (w_high << 16) | w_low;

    w_low       = read(eFPGA_REG_TRIGGER_TIME_STAMP_02_LB);
    w_high      = read(eFPGA_REG_TRIGGER_TIME_STAMP_02_HB);
    p_time[2]   = (w_high << 16) | w_low;
    
    
    exposure_print_interval( p_time[0], p_time[1], p_time[2]  );
    print_log(INFO, 1, "[%s] EXP_REQ  ~ EXP_OK  : %d ms\n", m_p_log_id, p_time[0] );
    print_log(INFO, 1, "[%s] X-ray on ~ EXP_OK  : %d ms\n", m_p_log_id, p_time[1] );
    print_log(INFO, 1, "[%s] EXP_OK   ~ X-ray on: %d ms\n", m_p_log_id, p_time[2] );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CFpga::exposure_request(TRIGGER_TYPE i_type_e)
{
    bool			b_result = true;
    u16				w_wait_time = read(eFPGA_REG_EXP_OK_DELAY_TIME);
    ioctl_data_t 	iod_t;
    CTime*			p_wait_c;
    
    if( is_ready() == false )
    {
        print_log(ERROR, 1, "[%s] exp req. is not ready.\n", m_p_log_id);
        return false;
    }
    
	print_log(DEBUG, 1, "[%s] EXPOSURE REQUEST is started(wait: %d ms).\n", m_p_log_id, w_wait_time);
	
	m_b_exp_ok = false;
	
    gettimeofday(&m_exp_req_timeval, NULL);
	if( clock_gettime(CLOCK_MONOTONIC, &m_exp_req_timespec) < 0 )
	{
	    print_log(ERROR,1,"[%s] exp req time check error\n", m_p_log_id);
	}
	
	if( i_type_e == eTRIGGER_TYPE_SW )
	{
		write( eFPGA_REG_SW_TRIGGER, 0 );
		write( eFPGA_REG_SW_TRIGGER, 1 );
		
		// normal grab of VIVIX Setup: auto clear
		write( eFPGA_REG_GRAB_CONTROL, 1 );
		write( eFPGA_REG_SW_TRIGGER, 0 );
	}
    else
    {
    	// exp_req ON
	    iod_t.data[0] = 1;
		ioctl( m_n_fd, VW_MSG_IOCTL_TG_EXP_REQ, &iod_t );
	
		if( i_type_e == eTRIGGER_TYPE_PACKET
			&& w_wait_time > EXP_OK_WAIT_TIME )
		{
			return true;
		}
	}
	
	iod_t.data[0] = 0;
    p_wait_c = new CTime(w_wait_time + 1000);
    
    while(1)
    {
        ioctl( m_n_fd, VW_MSG_IOCTL_TG_EXP_OK, &iod_t );
        
        if( iod_t.data[0] == 1 )
        {
            print_log(DEBUG, 1, "[%s] EXPOSURE OK\n", m_p_log_id);
            m_b_exp_ok = true;
            break;
        }
        
        if( iod_t.data[1] == 1 )
        {
            print_log(DEBUG, 1, "[%s] EXPOSURE OK(X-ray)\n", m_p_log_id);
            m_b_exp_ok = true;
            break;
        }
        
        if( iod_t.data[1] == 2 )
        {
            print_log(DEBUG, 1, "[%s] EXPOSURE OK(offset)\n", m_p_log_id);
            break;
        }
        
        if( p_wait_c->is_expired() == true )
        {
            print_log(ERROR, 1, "[%s] EXPOSURE OK time out\n", m_p_log_id);
            b_result = false;
            break;
        }
    }
    
	if( clock_gettime(CLOCK_MONOTONIC, &m_exp_ok_timespec) < 0 )
	{
	    print_log(ERROR,1,"[%s] exp ok time check error\n", m_p_log_id);
	}

    safe_delete(p_wait_c);
    
    if( i_type_e != eTRIGGER_TYPE_SW )
	{
    	exposure_request_stop(false);
    }
    
    return b_result;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CFpga::exposure_ok(void)
{
	if( m_b_exp_ok == true )
	{
		m_b_exp_ok = false;
		return true;
	}
	
	return false;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::exposure_request_stop(bool i_b_time_stamp)
{
	ioctl_data_t 	iod_t;
	
	if( i_b_time_stamp == true )
	{
		if( clock_gettime(CLOCK_MONOTONIC, &m_exp_ok_timespec) < 0 )
		{
		    print_log(ERROR,1,"[%s] exp ok time check error\n", m_p_log_id);
		}
	}
    iod_t.data[0] = 0;
    ioctl( m_n_fd, VW_MSG_IOCTL_TG_EXP_REQ, &iod_t );

}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u16 CFpga::exposure_ok_get_wait_time(void)
{
	u16 w_time = read(eFPGA_REG_EXP_OK_DELAY_TIME);
	
	if( w_time > EXP_OK_WAIT_TIME )
	{
		return w_time;
	}
	
	return 0;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::exposure_print_interval( s32 i_n_time1, s32 i_n_time2, s32 i_n_time3 )
{
 	struct timespec calc_t;  
 	u32             n_interval_time = 0;
 	s8				p_buffer [80];
 	
    if( trigger_get_type() != eTRIG_SELECT_NON_LINE )
    {
		calc_t.tv_sec = m_exp_ok_timespec.tv_sec - m_exp_req_timespec.tv_sec;
	    calc_t.tv_nsec = m_exp_ok_timespec.tv_nsec - m_exp_req_timespec.tv_nsec;	
	    
	    if(calc_t.tv_nsec < 0)
	    {
	    	calc_t.tv_sec--;
	    	calc_t.tv_nsec = calc_t.tv_nsec + 1000000000;
	    }	
	    
	    n_interval_time = calc_t.tv_sec * 1000 + calc_t.tv_nsec / 1000000;
	}
	
    print_log( INFO, 1, "[%s] interval time: %d ms\n", m_p_log_id, n_interval_time );
    
    if( file_get_count(SYS_EXPOSURE_LOG_PATH, "*.exp") > 90 )
    {
    	file_delete_oldest(SYS_EXPOSURE_LOG_PATH);
    }
    
    sprintf( p_buffer, "%d, %d, %d, %d", n_interval_time, i_n_time1, i_n_time2, i_n_time3 );
	
    exposure_print_interval_to_file(p_buffer);
	
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::exposure_aed(void)
{
	if( trigger_get_type() == eTRIG_SELECT_NON_LINE )
	{
		gettimeofday(&m_exp_req_timeval, NULL);
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::exposure_print_interval_to_file( s8* i_p_interval_time )
{
    // open log file
    u32			n_time = time(NULL);
    struct tm*  p_gmt_tm_t;
    FILE*		p_file;
    s8			p_file_name[30];
        
    p_gmt_tm_t = localtime( (time_t*)&n_time );
    
    sprintf( p_file_name, "%s%04d_%02d_%02d.exp", SYS_EXPOSURE_LOG_PATH, \
                        p_gmt_tm_t->tm_year+1900, p_gmt_tm_t->tm_mon+1, p_gmt_tm_t->tm_mday );

	p_file = fopen( p_file_name, "a+" );
	
	if( p_file == NULL )
	{
		print_log( ERROR, 1, "[%s] file open failed: %s\n", m_p_log_id, p_file_name );
		return;
	}

 	s8			p_buffer [80];
 	s8			p_current_time[84] = "";
 	s32			n_milli;
    
    strftime( p_buffer, 80, "%Y-%m-%d %H:%M:%S", localtime(&m_exp_req_timeval.tv_sec));
    
    n_milli = m_exp_req_timeval.tv_usec / 1000;
    sprintf( p_current_time, "%s.%03d", p_buffer, n_milli);
    
    fprintf( p_file, "%s, %s\n", p_current_time, i_p_interval_time );
    
    fclose( p_file );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::offset_set_enable( bool i_b_enable )
{
	if( is_double_readout_enabled() == true )
	{
		write( eFPGA_REG_DRO_OFFSET_ON, i_b_enable );
		
		if( i_b_enable == true )
		{
			lock();
	        io_write( FPGA_ISP_REG_OFFSET_CORR_ON, false );
	        unlock();
		}
	}
	else
	{
    	lock();
        io_write( FPGA_ISP_REG_OFFSET_CORR_ON, i_b_enable );
        unlock();
        
		if( i_b_enable == true )
		{
	        write( eFPGA_REG_DRO_OFFSET_ON, false );
		}
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CFpga::offset_get_enable(void)
{
	if( is_double_readout_enabled() == true )
	{
		if( read( eFPGA_REG_DRO_OFFSET_ON ) == 1 )
		{
			return true;
		}
	}
	else
	{
		if( read( FPGA_ISP_REG_OFFSET_CORR_ON ) == 1 )
		{
			return true;
		}
	}
	
	return false;
}
  
/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::gain_set_enable( bool i_b_enable )
{
	write( FPGA_ISP_REG_GAIN_CORR_ON, i_b_enable );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CFpga::gain_get_enable(void)
{
	if( read( FPGA_ISP_REG_GAIN_CORR_ON ) == 1 )
	{
		return true;
	}
	
	return false;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::gain_set_pixel_average( u16 i_w_average )
{
	write( FPGA_ISP_REG_GAIN_CORR_TARGET_GRAY_LEVEL, i_w_average );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::set_power_on_tft( bool i_on )
{
	u16 data;
	
    lock();
    data = io_read( eFPGA_REG_ROIC_CFG_2 );
    
    if( i_on == true )
    {
    	data = data | 0x0100;
    }
    else
    {
    	data = data & 0xFEFF;
    }
    
    io_write( eFPGA_REG_ROIC_CFG_2, data );
    
    roic_reg_write( (eFPGA_REG_ROIC_CFG_2 - eFPGA_REG_ROIC_CFG_0), data );
    unlock();
    
    print_log( DEBUG, 1, "[%s] set_power_on_tft: %d(0x%04X)\n", m_p_log_id, i_on, data );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::set_tg_line_count( u8 i_c_count )
{
    ioctl_data_t 	iod_t;
	
	memset( &iod_t, 0, sizeof(ioctl_data_t) );
	
	iod_t.data[0] = i_c_count;
	
	ioctl( m_n_fd, VW_MSG_IOCTL_TG_LINE_COUNT, &iod_t );
	
	write(FPGA_ISP_REG_IMAGE_LINE_COUNT, i_c_count);

	print_log(DEBUG, 1, "[%s] set_tg_line_count: %d\n", m_p_log_id, i_c_count );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::update_pseudo_flush_num(void)
{
	u16 i;
	u16 w_exp_ok_delay_time = read(eFPGA_REG_EXP_OK_DELAY_TIME);
	u16 w_width = read(eFPGA_REG_GCLK_WIDTH);
	u16 w_count = read(eFPGA_REG_GCLK_NUM_FLUSH);
	u32 n_sum = 0;
	float flush_time;
	
	if( w_exp_ok_delay_time > 0 )
	{
		for( i = 0; i < w_count; i++ )
		{
			n_sum += read(eFPGA_REG_GCLK0_FLUSH + i);
		}
		
		flush_time = ((n_sum + (w_width * w_count)) * 20 * 256)/1000000.0f;
		
		flush_time = w_exp_ok_delay_time/flush_time + 0.5f + 1;
		
		write(eFPGA_REG_PSEUDO_FLUSH_NUM, (u16)flush_time);
	}
	else
	{
		write(eFPGA_REG_PSEUDO_FLUSH_NUM, 1);
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CFpga::get_ddr_crc( u32 i_n_base_address, u32 i_n_crc_len )
{
	u32 n_crc = 0;
	u16 w_addr_low, w_addr_high, w_buffer;
	
	CTime* p_wait_c;
	
	// crc start address
	w_addr_low = (u16)(i_n_base_address & 0x0000FFFF);
	write(FPGA_ISP_REG_CRC_START_ADDR_L, w_addr_low);
	
	w_addr_high = (u16)(i_n_base_address >> 16);
	write(FPGA_ISP_REG_CRC_START_ADDR_H, w_addr_high);
	
	// crc end address
	w_buffer = w_addr_low | (i_n_crc_len & 0x0000FFFF);
	write(FPGA_ISP_REG_CRC_END_ADDR_L, w_buffer);
	
	w_buffer = (u16)(i_n_crc_len >> 16);
	w_buffer = w_buffer | w_addr_high;
	write(FPGA_ISP_REG_CRC_END_ADDR_H, w_buffer);
	
	// start crc calc.
	write(FPGA_ISP_REG_CRC_ON, 0);
	write(FPGA_ISP_REG_CRC_ON, 1);
	
	p_wait_c = new CTime(1000);
	while( p_wait_c->is_expired() == false )
	{
		if( read(FPGA_ISP_REG_CRC_DATA_H) > 0
			|| read(FPGA_ISP_REG_CRC_DATA_L) > 0 )
		{
			break;
		}
		sleep_ex(10000);
	}
	safe_delete(p_wait_c);
	
	n_crc = read(FPGA_ISP_REG_CRC_DATA_L);
	n_crc = n_crc | (read(FPGA_ISP_REG_CRC_DATA_H) << 16);
	
	write(FPGA_ISP_REG_CRC_ON, 0);
	
	return n_crc;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CFpga::is_offset_data_valid( u32 i_n_crc, u32 i_n_crc_len )
{
	u32 n_crc;
	
	
	print_log(DEBUG, 1, "[%s] offset data crc: 0x%08X, len: 0x%08X\n", m_p_log_id, i_n_crc, i_n_crc_len );
	
	// offset data base address is 0x0400_0000 on FPGA's DDR
	n_crc = get_ddr_crc(0x4000000, i_n_crc_len);

	if( i_n_crc == n_crc )
	{
		print_log(DEBUG, 1, "[%s] offset data is valid.\n", m_p_log_id );
		return true;
	}
	else
	{
		print_log(DEBUG, 1, "[%s] offset data is not valid(0x%08X).\n", m_p_log_id, n_crc );
		return false;
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CFpga::is_gain_data_valid( u32 i_n_crc, u32 i_n_crc_len )
{
	u32 n_crc;
	
	print_log(DEBUG, 1, "[%s] gain data crc: 0x%08X, len: 0x%08X\n", m_p_log_id, i_n_crc, i_n_crc_len );
	
	// gain data base address is 0x0800_0000 on FPGA's DDR
	n_crc = get_ddr_crc(0x8000000, i_n_crc_len);

	if( i_n_crc == n_crc )
	{
		print_log(DEBUG, 1, "[%s] gain data is valid.\n", m_p_log_id );
		return true;
	}
	else
	{
		print_log(DEBUG, 1, "[%s] gain data is not valid(0x%08X).\n", m_p_log_id, n_crc );
		return false;
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::device_pgm(void)
{
	ioctl_data_t 	iod_t;
    
    // PGM pin control
    ioctl( m_n_fd, VW_MSG_IOCTL_TG_FPGA_CONFIG, &iod_t );
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
 *******************************************************************************/
void CFpga::roic_dac_write_sel( u8 i_c_sel, u16 i_w_data )
{
    ioctl_data_t 				iod_t;
    memset(&iod_t, 0, sizeof(ioctl_data_t));
    iod_t.data[0] = i_c_sel;
    memcpy( &iod_t.data[1], &i_w_data, sizeof(i_w_data) );
    ioctl( m_n_fd_roic, VW_MSG_IOCTL_ROIC_DAC_SPI_WRITE, &iod_t ); 
}

/***********************************************************************************************//**
* \brief		roic_init_initialize
* \param
* \return
* \note			
***************************************************************************************************/
void CFpga::dac_setup(void)
{
	float f_input_volt = 0;
	float f_dac_volt = 0;
	u32	n_data = 0;
	u16 w_rdata = 0;
	u16 w_code = 0;
	
	w_rdata = read(eFPGA_REG_TI_ROIC_DAC_COMP1);
	f_input_volt = (float)w_rdata / 1000.0f;
	
	f_dac_volt = (f_input_volt*65536.0f/2.5f);
	n_data = (u32)(f_dac_volt);
	
	if(n_data > 0xffff)
		w_code = 0xffff;
	else
		w_code = (u16)n_data;
	
	roic_dac_write_sel(0, w_code);
	print_log(DEBUG, 1, "[%s] DAC COMP1 volt :  %.3f(code : 0x%04x)\n", m_p_log_id, f_input_volt, w_code);

	w_rdata = read(eFPGA_REG_TI_ROIC_DAC_COMP3);
	f_input_volt = (float)w_rdata / 1000.0f;
	
	f_dac_volt = (f_input_volt*65536.0f/2.5f);
	n_data = (u32)(f_dac_volt);
	
	if(n_data > 0xffff)
		w_code = 0xffff;
	else
		w_code = (u16)n_data;
	
	roic_dac_write_sel(1, w_code);
	print_log(DEBUG, 1, "[%s] DAC COMP3 volt :  %.3f(code : 0x%04x)\n", m_p_log_id, f_input_volt, w_code);

}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
 *******************************************************************************/
u16 CFpga::roic_reg_read( u32 i_n_addr )
{
    ioctl_data_t	iod_t;
    u16				w_data;

    memcpy( iod_t.data, &i_n_addr, sizeof(i_n_addr) );
    ioctl( m_n_fd_roic , VW_MSG_IOCTL_ROIC_REG_SPI_READ_ALL ,&iod_t ); 
    memcpy( &w_data, iod_t.data, sizeof(w_data) );

    return w_data;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
 *******************************************************************************/
void CFpga::roic_reg_write( u32 i_n_addr, u16 i_w_data )
{
    ioctl_data_t 				iod_t;

    memcpy( iod_t.data, &i_n_addr, sizeof(i_n_addr) );
    memcpy( &iod_t.data[sizeof(i_n_addr)], &i_w_data, sizeof(i_w_data) );
    ioctl( m_n_fd_roic, VW_MSG_IOCTL_ROIC_REG_SPI_WRITE_ALL, &iod_t ); 
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
 *******************************************************************************/
u16 CFpga::roic_reg_read( u8 i_c_idx, u32 i_n_addr )
{
    ioctl_data_t	iod_t;
    u16				w_data;

    memcpy( iod_t.data, &i_n_addr, sizeof(i_n_addr) );
    memcpy( &iod_t.data[sizeof(i_n_addr)], &i_c_idx, sizeof(i_c_idx) );
    ioctl( m_n_fd_roic , VW_MSG_IOCTL_ROIC_REG_SPI_READ ,&iod_t ); 
    memcpy( &w_data, iod_t.data, sizeof(w_data) );

    return w_data;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
 *******************************************************************************/
void CFpga::roic_reg_write(  u8 i_c_idx, u32 i_n_addr, u16 i_w_data )
{
    ioctl_data_t 				iod_t;

    memcpy( iod_t.data, &i_n_addr, sizeof(i_n_addr) );    
    memcpy( &iod_t.data[sizeof(i_n_addr)], &i_w_data, sizeof(i_w_data) );
    memcpy( &iod_t.data[sizeof(i_n_addr) + sizeof(i_w_data)], &i_c_idx, sizeof(i_c_idx) );
    ioctl( m_n_fd_roic, VW_MSG_IOCTL_ROIC_REG_SPI_WRITE, &iod_t ); 
}

/******************************************************************************/
/**
 * @brief	
 * @param	none
 * @return
 * @note
*******************************************************************************/
void CFpga::roic_info(void)
{
    ioctl_data_t 				iod_t;
    ioctl( m_n_fd_roic, VW_MSG_IOCTL_ROIC_INFO, &iod_t ); 
}

/******************************************************************************/
/**
 * @brief	
 * @param	none
 * @return
 * @note
*******************************************************************************/
void CFpga::roic_reset(void)
{
    ioctl_data_t 				iod_t;
    ioctl( m_n_fd_roic, VW_MSG_IOCTL_ROIC_RESET, &iod_t ); 
}

/******************************************************************************/
/**
 * @brief	
 * @param	none
 * @return
 * @note
*******************************************************************************/
void CFpga::roic_sync(void)
{
    ioctl_data_t 				iod_t;
    ioctl( m_n_fd_roic, VW_MSG_IOCTL_ROIC_DATA_SYNC, &iod_t ); 
}

/******************************************************************************/
/**
 * @brief	
 * @param	none
 * @return
 * @note
*******************************************************************************/
void CFpga::roic_delay_compensation(void)
{
    ioctl_data_t 				iod_t;
    ioctl( m_n_fd_roic, VW_MSG_IOCTL_ROIC_DATA_DELAY_CMP, &iod_t ); 
}

/******************************************************************************/
/**
 * @brief	
 * @param	none
 * @return
 * @note
*******************************************************************************/
void CFpga::roic_bit_alignment(void)
{
    ioctl_data_t 				iod_t;
    ioctl( m_n_fd_roic, VW_MSG_IOCTL_ROIC_DATA_BIT_ALIGNMENT, &iod_t ); 
}

/******************************************************************************/
/**
 * @brief	
 * @param	none
 * @return
 * @note
*******************************************************************************/
void CFpga::roic_bit_align_test(u16 w_retry_num)
{
    ioctl_data_t 				iod_t;
    memset(&iod_t, 0, sizeof(ioctl_data_t));
    memcpy(iod_t.data, &w_retry_num, sizeof(w_retry_num));
    ioctl( m_n_fd_roic, VW_MSG_IOCTL_ROIC_DATA_BIT_ALIGN_TEST, &iod_t ); 
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
 *******************************************************************************/
void CFpga::roic_power_down()
{
    ioctl_data_t 				iod_t;

    ioctl( m_n_fd_roic, VW_MSG_IOCTL_ROIC_POWERSAVE_ON, &iod_t ); 
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
 *******************************************************************************/
void CFpga::roic_power_up()
{
    ioctl_data_t 				iod_t;

    ioctl( m_n_fd_roic, VW_MSG_IOCTL_ROIC_POWERSAVE_OFF, &iod_t ); 
}

/******************************************************************************/
/**
 * @brief   AED trigger에 의한 촬영을 block 하지 않고 정상 촬영되도록 함			
 * @param   
 * @return  
 * @note    
 *******************************************************************************/
void CFpga::disable_aed_trig_block(void)
{
    write( eFPGA_REG_TRIG_OPTION, 1 );
}

/******************************************************************************/
/**
 * @brief   AED trigger에 의한 촬영을 block 하지 않고 정상 촬영되도록 함			
 * @param   
 * @return  
 * @note    
 *******************************************************************************/
bool CFpga::is_disable_aed_trig_block(void)
{
    if( read( eFPGA_REG_TRIG_OPTION ) == 0 )
    {
    	return true;
    }
    
    return false;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
 *******************************************************************************/
void CFpga::double_readout_enable(bool i_b_en)
{
	fpga_reg_u reg;
	reg.IMG_CAPTURE_OPT.DATA = read(eFPGA_REG_DRO_IMG_CAPTURE_OPTION);
	reg.IMG_CAPTURE_OPT.BIT.READOUT_OPTION = i_b_en;
    write( eFPGA_REG_DRO_IMG_CAPTURE_OPTION, reg.IMG_CAPTURE_OPT.DATA );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
 *******************************************************************************/
bool CFpga::is_double_readout_enabled(void)
{
	fpga_reg_u reg;
	reg.IMG_CAPTURE_OPT.DATA = read(eFPGA_REG_DRO_IMG_CAPTURE_OPTION);
    return (reg.IMG_CAPTURE_OPT.BIT.READOUT_OPTION  ==0)?false:true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
 *******************************************************************************/
void CFpga::set_gain_type(void)
{
    u16 w_gain_type = 0x00;
	u16 w_input_charge = 0;
	u16 w_roic_cmp_gain = 0;
	ioctl_data_t 	iod_t;
    memset(&iod_t, 0, sizeof(ioctl_data_t));
    
    //0. get index
    w_gain_type = read(eFPGA_REG_ROIC_GAIN_TYPE);

    //1. get IFS/Inputcharge range & roic cmp gain
    w_input_charge = s_p_input_charge[m_model_e][m_panel_type_e][m_scintillator_type_e][m_roic_type_e][w_gain_type];
	w_roic_cmp_gain = s_p_roic_cmp_gain[m_model_e][m_panel_type_e][m_scintillator_type_e][m_roic_type_e][w_gain_type];
    print_log(DEBUG, 1, "[%s] model: %d panel_e: %d scintillator_e: %d roic_e: %d gain_idx: %d \n", m_p_log_id, m_model_e, m_panel_type_e, m_scintillator_type_e, m_roic_type_e, w_gain_type);
	print_log(DEBUG, 1, "[%s] input_charge/IFS: %d w_roic_cmp_gain: %d\n", m_p_log_id, w_input_charge, w_roic_cmp_gain);

    //2. set ioctl data
    memcpy(iod_t.data, &w_input_charge, sizeof(u16));
    
    //3. call ioctl
    ioctl( m_n_fd_roic, VW_MSG_IOCTL_ROIC_GAIN_TYPE_SET, &iod_t );

    //4. do extra work (fpga gain compensation & gain ratio set)

	// Set ROIC compensation gain : gain ratio를 panel type에 관계 없이 동일하게 하기 위한 보정 값 
	write(eFPGA_REG_ROIC_CMP_GAIN, w_roic_cmp_gain);

    // Set gain ratio : Viewer의 EI 계산에 이용되기 위해 전달되는 값
    write( eFPGA_REG_CUR_GAIN_RATIO, s_p_gain_ratio[w_gain_type] );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
 *******************************************************************************/
u16 CFpga::get_input_charge_by_scintillator_type(SCINTILLATOR_TYPE i_e_scintillator_type)
{
    u16 w_gain_type = 0x00;
	u16 w_input_charge = 0;
    
    //0. get index
    w_gain_type = read(eFPGA_REG_ROIC_GAIN_TYPE);

    //1. get IFS/Inputcharge range & roic cmp gain
    w_input_charge = s_p_input_charge[m_model_e][m_panel_type_e][i_e_scintillator_type][m_roic_type_e][w_gain_type];
	
    return w_input_charge;
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
 *******************************************************************************/
u16 CFpga::get_roic_cmp_gain_by_scintillator_type(SCINTILLATOR_TYPE i_e_scintillator_type)
{
    u16 w_gain_type = 0x00;
	u16 w_roic_cmp_gain = 0;
    
    //0. get index
    w_gain_type = read(eFPGA_REG_ROIC_GAIN_TYPE);

    //1. get IFS/Inputcharge range & roic cmp gain
	w_roic_cmp_gain = s_p_roic_cmp_gain[m_model_e][m_panel_type_e][i_e_scintillator_type][m_roic_type_e][w_gain_type];
   
   return w_roic_cmp_gain;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
 *******************************************************************************/
u32 CFpga::get_exp_time(void)
{
	u32 n_exp_time = 0;
	u16 w_data;
	
	w_data = read(eFPGA_REG_EXP_TIME_HIGH);
	n_exp_time = (w_data << 16) & 0xffff0000;
	w_data = read(eFPGA_REG_EXP_TIME_LOW);
	n_exp_time |= (w_data & 0x0000ffff);
	
	return n_exp_time;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::print_line_error(void)
{
	print_log(DEBUG, 1, "[%s] ROIC HS CNT TOP : %d\n", m_p_log_id, read(0x398));
	print_log(DEBUG, 1, "[%s] ROIC HS CNT BTM : %d\n", m_p_log_id, read(0x399));
	print_log(DEBUG, 1, "[%s] ROIC HS CNT Line : %d\n", m_p_log_id, read(0x39A));
	print_log(DEBUG, 1, "[%s] ISP WR LINE : %d\n", m_p_log_id, read(0x4030));
	print_log(DEBUG, 1, "[%s] ISP RD LIND : %d\n", m_p_log_id, read(0x4031));
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
u32 CFpga::get_roic_addr( u32 i_n_addr )
{
	if( i_n_addr < eFPGA_REG_TI_ROIC_CONFIG_00 )
	{
		print_log(ERROR, 1, "[%s] invalid roic addr : 0x%08X\n", m_p_log_id, i_n_addr);
		return 0xFF;
	}
	
	return ( i_n_addr - eFPGA_REG_TI_ROIC_CONFIG_00 );
}

/******************************************************************************/
/**
 * @brief   
 * @param   i_w_mode: 0 = normal power, 1 = low power
 * @return  
 * @note    
*******************************************************************************/
void CFpga::set_roic_power_mode( u16 i_w_mode )
{
	u16 w_data = 0;
	if( i_w_mode == 0 )
	{
		// 0x2e [15:0] bit: 0x0000
		io_write( eFPGA_REG_TI_ROIC_CONFIG_2E, 0x0000 );
		roic_reg_write( get_roic_addr(eFPGA_REG_TI_ROIC_CONFIG_2E), 0x0000 );
		
		// 0x5d [2:1] bit: 0x0
		lock();
		w_data = io_read( eFPGA_REG_TI_ROIC_CONFIG_5D );
		w_data = w_data & ~(0x0006);
		io_write( eFPGA_REG_TI_ROIC_CONFIG_5D, w_data );
		unlock();
		roic_reg_write( get_roic_addr(eFPGA_REG_TI_ROIC_CONFIG_5D), w_data );
		
		// 0x12 [15] bit: 0x0
		lock();
		w_data = io_read( eFPGA_REG_TI_ROIC_CONFIG_12 );
		w_data = w_data & ~(0x8000);
		io_write( eFPGA_REG_TI_ROIC_CONFIG_12, w_data );
		unlock();
		roic_reg_write( get_roic_addr(eFPGA_REG_TI_ROIC_CONFIG_12), w_data );
		
		// 0x16 [7:0] bit: ? (TODO. low noise 모드 적용 시 정확히 확인 필요)
		io_write( eFPGA_REG_TI_ROIC_CONFIG_16, 0x00C0 );	
		roic_reg_write( get_roic_addr(eFPGA_REG_TI_ROIC_CONFIG_16), 0x00C0 );
		
		// 0x4c [15:0] bit: 0x0000
		io_write( eFPGA_REG_TI_ROIC_CONFIG_4C, 0x0000 );
		roic_reg_write( get_roic_addr(eFPGA_REG_TI_ROIC_CONFIG_4C), 0x0000 );
	}
	else
	{
		// 0x2e [15:0] bit: 0x5700
		io_write( eFPGA_REG_TI_ROIC_CONFIG_2E, 0x5700 );
		roic_reg_write( get_roic_addr(eFPGA_REG_TI_ROIC_CONFIG_2E), 0x5700 );
		
		// 0x5d [2:1] bit: 0x6
		lock();
		w_data = io_read( eFPGA_REG_TI_ROIC_CONFIG_5D );
		w_data = w_data | 0x0006;
		io_write( eFPGA_REG_TI_ROIC_CONFIG_5D, w_data );
		unlock();
		roic_reg_write( get_roic_addr(eFPGA_REG_TI_ROIC_CONFIG_5D), w_data );
		
		// 0x12 [15] bit: 0x0
		lock();
		w_data = io_read( eFPGA_REG_TI_ROIC_CONFIG_12 );
		w_data = w_data | 0x8000;
		io_write( eFPGA_REG_TI_ROIC_CONFIG_12, w_data );
		unlock();
		roic_reg_write( get_roic_addr(eFPGA_REG_TI_ROIC_CONFIG_12), w_data );	
		
		// 0x16 [7:0] bit: ? (TODO. low noise 모드 적용 시 정확히 확인 필요)
		io_write( eFPGA_REG_TI_ROIC_CONFIG_16, 0x00C0 );	
		roic_reg_write( get_roic_addr(eFPGA_REG_TI_ROIC_CONFIG_16), 0x00C0 );
		
		// 0x4c [15:0] bit: 0x0001
		io_write( eFPGA_REG_TI_ROIC_CONFIG_4C, 0x0001 );
		roic_reg_write( get_roic_addr(eFPGA_REG_TI_ROIC_CONFIG_4C), 0x0001 );
	}
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::tft_aed_control(void)
{
	ioctl_data_t ctl_t;
	
	if( is_ready() == false )
		return;

    ioctl( m_n_fd_roic, VW_MSG_IOCTL_TFT_AED_ROIC_CTRL, &ctl_t );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CFpga::is_gate_line_defect_enabled(void)
{
	return (bool)read(eFPGA_REG_READ_STOP_ENABLE);
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::gate_line_defect_enable(bool i_b_enable)
{
	write(eFPGA_REG_READ_STOP_ENABLE, i_b_enable);
}

/******************************************************************************/
/**
 * @brief   sensitivity 가 입력되지 않은 기존 fw인 경우 기본 값 입력을 위해 이 함수 사용
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CFpga::set_sensitivity()
{
	u16 w_sensitivity = read(eFPGA_REG_SENSITIVITY);
	if(w_sensitivity == 0)
	{
        //SENSITIVITY 사양
        //2530/3643/4343 Normal 모델: 600
        //2530/3643/4343 Plus 모델: 750
        if( m_submodel_type_e == eSUBMODEL_TYPE_BASIC  )
        {
            m_p_reg[eFPGA_REG_SENSITIVITY] = 600;
        }
        else if( m_submodel_type_e == eSUBMODEL_TYPE_PLUS )
        {
            m_p_reg[eFPGA_REG_SENSITIVITY] = 750;
        }
        else        //unknown
        {
            print_log(ERROR, 1, "[%s] Unknown SUBMODEL TYPE: %d\n", m_p_log_id, m_submodel_type_e);
        }
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    DEBUG 목적 
*******************************************************************************/
void CFpga::print_gain_type_table(void)
{
	//u16 s_p_input_charge[eMODEL_MAX_NUM][ePANEL_TYPE_MAX][eSCINTILLATOR_TYPE_MAX][eROIC_TYPE_MAX][GAIN_TYPE_NUM] = 
	for(int i = 0; i < eMODEL_MAX_NUM; i++)
	{
		print_log(DEBUG, 1, "MODEL_TYPE: %d:\n", i);
		for(int j = 0; j < ePANEL_TYPE_MAX; j++)
		{
			print_log(DEBUG, 1, "\tePANEL_TYPE: %d:\n", j);
			for(int k = 0; k < eSCINTILLATOR_TYPE_MAX; k++)
			{
				print_log(DEBUG, 1, "\t\teSCINTILLATOR_TYPE: %d:\n", k);
				for(int q = 0; q < eROIC_TYPE_MAX; q++)
				{
                    print_log(DEBUG, 1, "\t\t\teROIC_TYPE: %d:\n", q);
                    for(int r = 0; r < GAIN_TYPE_NUM; r++)
                    {
                        print_log(DEBUG, 1, "\t\t\t\tgain type %d: %d\t %d\n", r, s_p_input_charge[i][j][k][q][r], s_p_roic_cmp_gain[i][j][k][q][r]);
                    }
				}
			}
		}
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  1: smart w on
 * 			0: smart w off
 * @note     
*******************************************************************************/
u8 CFpga::get_smart_w_onoff(void)
{
	fpga_reg_u reg;
	reg.IMG_CAPTURE_OPT.DATA = read(eFPGA_REG_DRO_IMG_CAPTURE_OPTION);
	return (u8) reg.IMG_CAPTURE_OPT.BIT.SMART_W;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note     
*******************************************************************************/
u8 CFpga::set_smart_w_onoff(u8 i_enable)
{
	fpga_reg_u reg;
	reg.IMG_CAPTURE_OPT.DATA = read(eFPGA_REG_DRO_IMG_CAPTURE_OPTION);

    if (reg.IMG_CAPTURE_OPT.BIT.READOUT_OPTION == 0) //single readout이면 error 처리
    {
        return 1;
    }
    else
    {
        reg.IMG_CAPTURE_OPT.BIT.SMART_W = i_enable;
        write(eFPGA_REG_DRO_IMG_CAPTURE_OPTION, reg.IMG_CAPTURE_OPT.DATA);

        //fpga reg -> m_p_reg	
        reg_save(eFPGA_REG_DRO_IMG_CAPTURE_OPTION, eFPGA_REG_DRO_IMG_CAPTURE_OPTION);
        return 0;
    }
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note     unit: 10us
*******************************************************************************/
u16 CFpga::get_aed_off_debounce_time(void)
{
	u16 reg = read(eFPGA_REG_DEBOUCE_NON_LINE_OFF);
	return reg; 
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note     unit: 10us
*******************************************************************************/
u8 CFpga::set_aed_off_debounce_time(u16 i_w_debounce_time)
{
	write(eFPGA_REG_DEBOUCE_NON_LINE_OFF, i_w_debounce_time);

	//fpga reg -> m_p_reg
	reg_save(eFPGA_REG_DEBOUCE_NON_LINE_OFF, eFPGA_REG_DEBOUCE_NON_LINE_OFF);
	return 0;
}

/******************************************************************************/
/**
 * @brief   현재 설정된 촬영 모드 GET (RAD/CF/PF)
 * @param   
 * @return  
 * @note     VW 모델의 경우, RAD 모드만 사용
*******************************************************************************/
GEN_INTERFACE_TYPE CFpga::get_acq_mode(void)
{
	return eGEN_INTERFACE_RAD;
}

/******************************************************************************/
/**
 * @brief   양산품의 fpga register 설정 대응을 위한 함수
 * @param
 * @return  
 * @note    
 *******************************************************************************/
void CFpga::change_reg_setting_for_MP(void)
{
    if (hw_get_sig_version() < SIGNAL_BOARD_NUM_VW_REVISION)
    {
        if (m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_13] == 0xFFBF)
        {
            m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_13] = 0xF8A7;
            print_log(ERROR, 1, "[%s] addr: 0x%04X, data: 0xFFBF->0x%04X\n", m_p_log_id, eFPGA_REG_TI_ROIC_CONFIG_13, m_p_reg[eFPGA_REG_TI_ROIC_CONFIG_13]);
        }
    }
}

/******************************************************************************/
/**
 * @brief   get signal version from FPGA 
 * @param   None
 * @return  signal version
 * @note    
 *      signal version < 2: roic idle power mode: nap pll off
 *      signal version >=2: roic idle power mode: power down pll off
 *******************************************************************************/
u16 CFpga::hw_get_sig_version(void)
{
    return read(eFPGA_REG_PBA_VERSION);
}

void CFpga::SetReadoutSync(void)
{
    fpga_reg_u fpgaReg;
    fpgaReg.READOUT_SYNC.DATA = GetReadoutSync();

    fpgaReg.READOUT_SYNC.BIT.READOUT_SYNC = 1;

    write(eFPGA_REG_READOUT_SYNC, fpgaReg.READOUT_SYNC.DATA);
}

u16 CFpga::GetReadoutSync(void)
{
    u16 reg = read(eFPGA_REG_READOUT_SYNC);
	return reg; 
}
