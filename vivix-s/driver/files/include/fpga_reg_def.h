/********************************************************************************  
 ��  �� : fpga reg define
 �ۼ��� : ������
 ��  �� : 0.0.1
 ��  �� : fpga register define
 ��  �� : 
 
          
          
 ��  ��:
         0.0.1  1012N Ÿ��

********************************************************************************/


#ifndef _FPGA_REG_DEF_
#define _FPGA_REG_DEF_



/***************************************************************************************************
*	Include Files
***************************************************************************************************/


/***************************************************************************************************
*	Constant Definitions
***************************************************************************************************/
#define 	FPGA_VERSION								0x0001
#define 	FPGA_SIZE_X									0x0010
#define 	FPGA_SIZE_Y									0x0011
#define 	FPGA_SIZE_X_AOI								0x0012
#define 	FPGA_SIZE_Y_AOI								0x0013
#define 	FPGA_SEL_XGEN_IF        					0x0020
#define 	FPGA_SEL_TRIGGER							0x0021
#define 	FPGA_LINE_INTR_DISABLE      				0x0025

#define 	FPGA_DEBOUNCE_LINE							0x0030
#define 	FPGA_DEBOUNCE_NON_LINE						0x0031
#define 	FPGA_EXPOSURE_TIME							0x0032
#define 	FPGA_EXP_OK_DELAY_TIME						0x0033
#define 	FPGA_INTRA_FLUSH							0x0034
#define 	FPGA_EXTRA_FLUSH							0x0035
#define 	FPGA_PANEL_BIAS								0x0036
#define 	FPGA_BINNING								0x0040
#define 	FPGA_TEST_PATTERN							0x0041
#define 	FPGA_SEL_TCON_OP            				0x0042

#define 	FPGA_OFFSET_CAP_ERR         				0x0044

//#define		FPGA_PUT_LINE_COUNT							0x0050
//#define		FPGA_GET_LINE_COUNT							0x0051
#define 	FPGA_TCON_STATE								0x0052
#define		TCON_TRIGGER_SRC    						0x0053
#define		FPGA_TCON_INT_MASK							0x0054
#define		FPGA_TCON_INT_ACK							0x0056

#define		FPGA_ROIC_CLK_EN							0x005F

#define 	FPGA_AED_SENSOR_HIGH_CNT    				0x0060
#define 	FPGA_AED_SENSOR_LOW_CNT     				0x0061
#define 	FPGA_AED_SENSOR_SEL         				0x0062
#define 	FPGA_AED_SENSOR_ERR         				0x0063
#define 	FPGA_TEMP0_LOW_WORD         				0x0070
#define 	FPGA_TEMP0_HIGH_WORD        				0x0071
#define 	FPGA_TEMP1_LOW_WORD         				0x0072
#define 	FPGA_TEMP1_HIGH_WORD        				0x0073
#define 	FPGA_TEMP2_LOW_WORD         				0x0074
#define 	FPGA_TEMP2_HIGH_WORD        				0x0075

#define 	FPGA_CONFIG_REG0            				0x0080
#define 	FPGA_CONFIG_REG1            				0x0081
#define 	FPGA_CONFIG_REG2	        				0x0082
#define 	FPGA_CONFIG_REG3            				0x0083
#define 	FPGA_CONFIG_REG4            				0x0084
#define 	FPGA_CONFIG_REG5            				0x0085
#define 	FPGA_CONFIG_REG6            				0x0086
#define 	FPGA_CONFIG_REG7            				0x0087
#define 	FPGA_CONFIG_REG8            				0x0088
#define 	FPGA_CONFIG_REG9            				0x0089
#define 	FPGA_CONFIG_REG10           				0x008A
#define 	FPGA_CONFIG_REG11           				0x008B
#define 	FPGA_CONFIG_REG12           				0x008C
#define 	FPGA_CONFIG_REG13           				0x008D
#define 	FPGA_CONFIG_REG14           				0x008E
#define 	FPGA_CONFIG_REG15           				0x008F
#define 	FPGA_RESET_REG              				0x0090
#define 	FPGA_ACTIVE_POWER_MODE      				0x0091

#define 	FPGA_SYNC_WIDTH             				0x00A0
#define 	FPGA_ACLK_WIDTH             				0x00A1
#define 	FPGA_ACLK_NUM               				0x00A2
#define 	FPGA_ACLK0                  				0x00A3
#define 	FPGA_ACLK1                  				0x00A4
#define 	FPGA_ACLK2                  				0x00A5
#define 	FPGA_ACLK3                  				0x00A6
#define 	FPGA_ACLK4                  				0x00A7
#define 	FPGA_ACLK5                  				0x00A8
#define 	FPGA_ACLK6                  				0x00A9
#define 	FPGA_ACLK7                  				0x00AA
#define 	FPGA_ACLK8                  				0x00AB
#define 	FPGA_ACLK9                  				0x00AC
#define 	FPGA_ACLK10                 				0x00AD
#define 	FPGA_ACLK11                 				0x00AE
#define 	FPGA_ACLK12                 				0x00AF
#define 	FPGA_ACLK13                 				0x00B0
#define 	FPGA_DCLK_DELAY             				0x00C0
#define 	FPGA_MUTE_NUM               				0x00C1
#define 	FPGA_STV_DIR                				0x00D0
#define 	FPGA_STV_FLUSH              				0x00D1
#define 	FPGA_CPV_FLUSH              				0x00D2
#define 	FPGA_OE_FLUSH               				0x00D3
#define 	FPGA_STV_READ_OUT           				0x00D4
#define 	FPGA_CPV_READ_OUT           				0x00D5
#define 	FPGA_CPVB1_READ_OUT         				0x00D6
#define 	FPGA_CPVB2_READ_OUT         				0x00D7
#define 	FPGA_CPVB3_READ_OUT         				0x00D8
#define 	FPGA_OE_READ_OUT            				0x00D9
#define 	FPGA_XON                    				0x00DA
#define 	FPGA_GSYNC_WIDTH            				0x00E0
#define 	FPGA_GCLK_WIDTH             				0x00E1
#define 	FPGA_GCLK_NUM_FLUSH         				0x00E2
#define 	FPGA_GCLK_NUM_READ_OUT      				0x00E3
#define 	FPGA_GCLK0_FLUSH            				0x00E4
#define 	FPGA_GCLK1_FLUSH            				0x00E5
#define 	FPGA_GCLK2_FLUSH            				0x00E6
#define 	FPGA_GCLK3_FLUSH            				0x00E7
#define 	FPGA_GCLK4_FLUSH            				0x00E8
#define 	FPGA_GCLK5_FLUSH            				0x00E9
#define 	FPGA_GCLK6_FLUSH            				0x00EA
#define 	FPGA_GCLK7_FLUSH            				0x00EB
#define 	FPGA_GCLK0_READ_OUT         				0x00EC
#define 	FPGA_GCLK1_READ_OUT         				0x00ED
#define 	FPGA_GCLK2_READ_OUT         				0x00EE
#define 	FPGA_GCLK3_READ_OUT         				0x00EF
#define 	FPGA_GCLK4_READ_OUT         				0x00F0
#define 	FPGA_GCLK5_READ_OUT         				0x00F1
#define 	FPGA_GCLK6_READ_OUT         				0x00F2
#define 	FPGA_GCLK7_READ_OUT         				0x00F3
#define 	FPGA_GCLK8_READ_OUT         				0x00F4
#define 	FPGA_GCLK9_READ_OUT         				0x00F5
#define 	FPGA_GCLK10_READ_OUT        				0x00F6
#define 	FPGA_GCLK11_READ_OUT        				0x00F7
#define 	FPGA_GCLK12_READ_OUT        				0x00F8
#define 	FPGA_GCLK13_READ_OUT        				0x00F9
#define 	FPGA_GCLK14_READ_OUT        				0x00FA
#define 	FPGA_GCLK15_READ_OUT        				0x00FB

#define		FPGA_REG_MIG_ERROR							0x0129

#define		FPGA_DETECTOR_MODEL_SEL						0x0140
#define		FPGA_ROIC_U_SIZE							0x0141
#define		FPGA_ROIC_l_SIZE							0x0142
#define		FPGA_TI_ROIC_MUX_SELECT						0x0143
#define		FPGA_TI_DAC_MUX_SELECT						0x0144

#define		FPGA_REG_TFT_AED_OPTION						0x01A2
#define    	FPGA_REG_TFT_AED_ROIC_SEL_L					0x01A4
#define    	FPGA_REG_TFT_AED_ROIC_SEL_H					0x01A5
    
#define    	FPGA_REG_TI_ROIC_CONFIG_00					0x0200
#define    	FPGA_REG_TI_ROIC_CONFIG_10					0x0210
#define    	FPGA_REG_TI_ROIC_CONFIG_11					0x0211
#define    	FPGA_REG_TI_ROIC_CONFIG_12					0x0212
#define    	FPGA_REG_TI_ROIC_CONFIG_13					0x0213
#define    	FPGA_REG_TI_ROIC_CONFIG_16					0x0216
#define    	FPGA_REG_TI_ROIC_CONFIG_18					0x0218
#define    	FPGA_REG_TI_ROIC_CONFIG_2C					0x022C
#define    	FPGA_REG_TI_ROIC_CONFIG_30					0x0230
#define    	FPGA_REG_TI_ROIC_CONFIG_31					0x0231
#define    	FPGA_REG_TI_ROIC_CONFIG_32					0x0232
#define    	FPGA_REG_TI_ROIC_CONFIG_33					0x0233
#define    	FPGA_REG_TI_ROIC_CONFIG_40					0x0240
#define    	FPGA_REG_TI_ROIC_CONFIG_41					0x0241
#define    	FPGA_REG_TI_ROIC_CONFIG_42					0x0242
#define    	FPGA_REG_TI_ROIC_CONFIG_43					0x0243
#define    	FPGA_REG_TI_ROIC_CONFIG_46					0x0246
#define    	FPGA_REG_TI_ROIC_CONFIG_47					0x0247
#define    	FPGA_REG_TI_ROIC_CONFIG_4A					0x024A
#define    	FPGA_REG_TI_ROIC_CONFIG_4B					0x024B
#define    	FPGA_REG_TI_ROIC_CONFIG_4C					0x024C
#define    	FPGA_REG_TI_ROIC_CONFIG_50					0x0250
#define    	FPGA_REG_TI_ROIC_CONFIG_51					0x0251
#define    	FPGA_REG_TI_ROIC_CONFIG_52					0x0252
#define    	FPGA_REG_TI_ROIC_CONFIG_53					0x0253
#define   	FPGA_REG_TI_ROIC_CONFIG_54					0x0254
#define    	FPGA_REG_TI_ROIC_CONFIG_55					0x0255
#define    	FPGA_REG_TI_ROIC_CONFIG_5A					0x025A
#define    	FPGA_REG_TI_ROIC_CONFIG_5C					0x025C
#define    	FPGA_REG_TI_ROIC_CONFIG_5D					0x025D
#define    	FPGA_REG_TI_ROIC_CONFIG_5E					0x025E
#define    	FPGA_REG_TI_ROIC_CONFIG_61					0x0261
#define    	FPGA_REG_TI_ROIC_BIT_ALIGN_ERR				0x0299
	
#define		FPGA_TI_ROIC_MCLK_SELECT					0x0300
#define		FPGA_TI_ROIC_SYNC_GEN_EN					0x0301
#define		FPGA_TI_ROIC_SYNC_TPSEL_SET					0x0302
#define		FPGA_TI_ROIC_SCAN							0x0303
#define		FPGA_TI_ROIC_ALIGN_MODE						0x0304
#define		FPGA_TI_ROIC_DELAY_COMPENSATION_EN			0x0305
#define		FPGA_TI_ROIC_TOP_DELAY_COMPENSATION_MODE	0x0306
#define		FPGA_TI_ROIC_BTM_DELAY_COMPENSATION_MODE	0x0307
#define		FPGA_TI_ROIC_MANUAL_CLK_DLY_U				0x0308
#define		FPGA_TI_ROIC_MANUAL_DATA_DLY_U				0x0309
#define		FPGA_TI_ROIC_MANUAL_CLK_DLY_L				0x030A
#define		FPGA_TI_ROIC_MANUAL_DATA_DLY_L				0x030B

#define		FPGA_TI_ROIC_AUTO_CAL_DLY_STATUS_U_LSB		0x0318
#define		FPGA_TI_ROIC_AUTO_CAL_DLY_STATUS_U_MSB		0x0319
#define		FPGA_TI_ROIC_AUTO_CAL_CLK_DLY_U_TST_0_LSB	0x031A
#define		FPGA_TI_ROIC_AUTO_CAL_DATA_DLY_U_TST_0_LSB	0x0332
#define		FPGA_TI_ROIC_AUTO_CAL_DLY_STATUS_L_LSB		0x034A
#define		FPGA_TI_ROIC_AUTO_CAL_DLY_STATUS_L_MSB		0x034B
#define		FPGA_TI_ROIC_AUTO_CAL_CLK_DLY_L_TST_0_LSB	0x034C
#define		FPGA_TI_ROIC_AUTO_CAL_DATA_DLY_L_TST_0_LSB	0x0364

#define 	FPGA_PUT_LINE_COUNT 						0x4030
#define 	FPGA_GET_LINE_COUNT 						0x4031
#define 	FPGA_LINE_READ_END 							0x4032


/***************************************************************************************************
*	Type Definitions
***************************************************************************************************/

/***************************************************************************************************
*	Macros (Inline Functions) Definitions
***************************************************************************************************/

typedef union _fpga_reg
{
	union
	{
		u16	DATA;
		struct
		{
			u16	RDY_PUT			: 1;		/* BIT0		: ready to put line		*/
			u16					: 3;        			
			u16	ERR_ACT			: 1;        /* BIT4		: error internal-capture on active-line		*/	
			u16	ERR_PAS			: 1;        /* BIT5		: error internal-caputre on passive-line		*/	
			u16	ERR_NON			: 1;        /* BIT6		: error internal-capture non-line		*/	
			u16	ERR_CAP			: 1;        /* BIT7		: error internal-capture		*/
			u16	ERR_TRIG		: 1;        /* BIT8		: error non-line trigger		*/	
			u16	EXP_START		: 1;        /* BIT9		: exposure		*/	
			u16	READ_START		: 1;        /* BIT10	: read-out start		*/		
			u16	READ_DONE		: 1;	    /* BIT11	: read-out done		*/		
			u16	BIVERSE_PANEL	: 1;	    /* BIT12	: biverse panel bias		*/		
			u16					: 3;        
					
					
		}BIT;
	}INT;
	
	union
	{
		u16 DATA;
		struct
		{
			u16	CHK_TRIG		: 1;		/* BIT0		: check trigger		*/
			u16	ERR_IN_TRIG		: 1;        /* BIT1		: error internal trigger		*/			
			u16	ERR_ACT_LINE	: 1;        /* BIT2		: error internal active-line trigger		*/	
			u16	ERR_PAS_TRIG	: 1;        /* BIT3		: error internal passive-line trigger	*/	
			u16	ERR_NON_TRIG	: 1;        /* BIT4		: error internal non-line trigger	*/	
			u16	OFFSET_CAP   	: 1;        /* BIT5		: offset capture		*/
			u16	ACT_TRIG		: 1;        /* BIT6		: active line trigger		*/	
			u16	PAS_TRIG		: 1;        /* BIT7		: passive line trigger		*/	
			u16	NON_TRIG		: 1;        /* BIT8		: non line trigger		*/		
			u16	IDLE			: 1;	    /* BIT9		: idle		*/		
			u16	FLUSH			: 1;	    /* BIT10	: flush		*/		
			u16	EXPOSURE		: 1;	    /* BIT11	: exposure		*/
			u16 READ_OUT_START	: 1;	    /* BIT12	: read out start		*/
			u16	READ_OUT		: 1;	    /* BIT13	: read out		*/
			u16	READ_OUT_DONE	: 1;	    /* BIT14	: read out done		*/
			u16	OP_FLUSH		: 1;	    /* BIT15	: op flush		*/								
		}BIT;
		
	}TCON_STATE;
	
	union
	{
		u16 DATA;
		struct
		{
			u16	ACTIVE_LINE		: 1;		/* BIT0		: active line trigger		*/
			u16	NON_LINE		: 1;        /* BIT1		: non-line trigger		*/			
			u16	OFFSET      	: 1;        /* BIT2		: offset trigger		*/	
			u16 BIT3			: 1;
			u16 TRANS_FRAME		: 1;		/* BIT4		: ������ ���۵� exposure interrupt���� Ȯ�� */
			u16 XRAY_FRAME		: 1;		/* BIT5		: X-ray �������� ���� */
			u16 SOFFSET_FRAME	: 1;		/* BIT6		: double readout offset ó���ϴ� ���, offset frame ���� Ȯ�� */
			u16	            	: 9;
		}BIT;
		
	}TRIGGER_SRC;
}fpga_reg_u;


typedef enum
{
    eTRIG_SELECT_ACTIVE_LINE  = 1,
    eTRIG_SELECT_NON_LINE     = 3,
}TRIGGER_SELECT;


/***************************************************************************************************
*	Variable Definitions
***************************************************************************************************/


/***************************************************************************************************
*	Function Prototypes
***************************************************************************************************/





#endif