/*
 * fpga_reg.h
 *
 *  Created on: 2015. 10. 14.
 *      Author: myunghee
 */

 
#ifndef _FPGA_REG_H_
#define _FPGA_REG_H_

/*******************************************************************************
*	Include Files
*******************************************************************************/

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/
#define REG_ADDRESS_MAX         (1024)

/*******************************************************************************
*	Type Definitions
*******************************************************************************/
/**
 * @brief   FPGA register �ּ�
 */
typedef enum
{
    eFPGA_REG_VERSION                       = 0x0001,
    eFPGA_REG_VERSION_SUB                   = 0x0002,
    eFPGA_REG_CONNECTION_TEST               = 0x0003,

    eFPGA_REG_PBA_VERSION                   = 0x0005,

    eFPGA_REG_CAPTURE_CNT_LOW               = 0x0009,
    eFPGA_REG_CAPTURE_CNT_HIGH              = 0x000A,

    eFPGA_REG_IMAGE_WIDTH                   = 0x0010,
    eFPGA_REG_IMAGE_HEIGHT                  = 0x0011,

    eFPGA_REG_FLUSH_GATE_HEIGHT             = 0x0015,

    eFPGA_REG_SEL_EDGE_DETECT               = 0x0019,		// [9] AED[1] Trigger Edge detect [8] AED[0] Trigger Edge detect 

    eFPGA_REG_TRIGGER_TYPE                  = 0x0021,
    eFPGA_REG_XRAY_READY                    = 0x0022,

    eFPGA_REG_TEST_AED_TRIGGER              = 0x0023,
    eFPGA_REG_LINE_INTR_DISABLE             = 0x0025,

    eFPGA_REG_OFFSET_TRIG_REQUEST           = 0x0026,

    eFPGA_REG_SW_TRIGGER  			        = 0x0028,
    eFPGA_REG_READOUT_SYNC 			        = 0x0029,

    eFPGA_REG_DEBOUCE_NON_LINE_OFF   	    = 0x002F,   //unit: 10us
    eFPGA_REG_DEBOUNCE_DR                   = 0x0030,
    eFPGA_REG_DEBOUNCE_AED		            = 0x0031,

    eFPGA_REG_EXP_TIME_LOW                  = 0x0032,
    eFPGA_REG_EXP_OK_DELAY_TIME             = 0x0033,
    eFPGA_REG_DRO_PANEL_BIAS_NUM	        = 0x0034,		/* Double readout mode���� GRF(VCOM, Panel bias) �ΰ� �ð� (flush �ֱ�) ���� �� */

    eFPGA_REG_EXTRA_FLUSH   		        = 0x0035,
    eFPGA_REG_PANEL_BIAS    		        = 0x0036,

    eFPGA_REG_XRAY_LATENCY_CNT  	        = 0x0037,
    eFPGA_REG_PSEUDO_FLUSH_NUM              = 0x0038,

    eFPGA_REG_EXTRA_FLUSH_FOR_FTM           = 0x0039,       // extra flush for Fast Tact-time Mode

    eFPGA_REG_EXP_TIME_HIGH                 = 0x003A,

    eFPGA_REG_DRO_2ND_EXP_TIME		        = 0x003E,		/* Double readout mode���� �ι�° ���� exposure time : 0x32�� * 0x3f�� */
    eFPGA_REG_DRO_2ND_EXP_RATIO		        = 0x003F,		/* 2��° ���� exposure time / 1��° ���� exposure time */

    eFPGA_REG_TEST_PATTERN                  = 0x0041,
    eFPGA_REG_SELECT_TCON_OP                = 0x0042,

    eFPGA_REG_OFFSET_CAP_CTRL               = 0x0043,
    eFPGA_REG_OFFSET_CAP_ERR                = 0x0044,

    eFPGA_REG_DIGITAL_GAIN                  = 0x0047,   // ������: ���� 6bit, �Ҽ���: ���� 10bit
    eFPGA_REG_DIGITAL_OFFSET                = 0x0048,
    eFPGA_REG_SW_OFFSET                     = 0x0049,
    eFPGA_REG_MSC_EN	                    = 0x004A,

    eFPGA_REG_GRAB_CONTROL	                = 0x004B,
    eFPGA_REG_ROIC_CMP_GAIN                 = 0x004C,

    eFPGA_REG_TCON_STATE                    = 0x0052,

    eFPGA_REG_IMAGE_ERROR_EXTRA_FLUSH       = 0x0056,
    eFPGA_REG_IMAGE_ERROR                   = 0x0057,

    eFPGA_REG_ROIC_CLK_EN                   = 0x005F,         //FPGA 및 ROIC driver에서 직접 control

    eFPGA_REG_AED_HIGH_CNT                  = 0x0060,
    eFPGA_REG_TCON_AED_SENSOR_OFF           = 0x0061,           //TCON_AED_SENSOR_OPTION
    eFPGA_REG_AED_SELECT                    = 0x0062,
    eFPGA_REG_AED_ERROR                     = 0x0063,

    eFPGA_REG_PULSE_AED_STB_CNT			    = 0x0065,
    eFPGA_REG_DEB_AED_STB_CNT			    = 0x0066,
    eFPGA_REG_SET_PULSE_AED_STB_TIME	    = 0x0068,
    eFPGA_REG_SET_DEB_AED_STB_TIME		    = 0x0069,

    eFPGA_REG_TRIG_OPTION		            = 0x006E,

    eFPGA_REG_DRO_OFFSET_ON		            = 0x0077,			/* Double readout offset on */
    eFPGA_REG_TCON_XRAY_EXPOSURE_TIME_HIGH  = 0x007C,
    eFPGA_REG_TCON_XRAY_EXPOSURE_TIME_LOW   = 0x007E,

    //ADI ROIC REG START

    eFPGA_REG_ROIC_CFG_0            	    = 0x0080,
    eFPGA_REG_ROIC_CFG_1            	    = 0x0081,
    eFPGA_REG_ROIC_CFG_2	        	    = 0x0082,
    eFPGA_REG_ROIC_CFG_3            	    = 0x0083,
    eFPGA_REG_ROIC_CFG_4            	    = 0x0084,
    eFPGA_REG_ROIC_CFG_5            	    = 0x0085,
    eFPGA_REG_ROIC_CFG_6            	    = 0x0086,
    eFPGA_REG_ROIC_CFG_7            	    = 0x0087,
    eFPGA_REG_ROIC_CFG_8            	    = 0x0088,
    eFPGA_REG_ROIC_CFG_9            	    = 0x0089,
    eFPGA_REG_ROIC_CFG_10           	    = 0x008A,
    eFPGA_REG_ROIC_CFG_11           	    = 0x008B,
    eFPGA_REG_ROIC_CFG_12           	    = 0x008C,
    eFPGA_REG_ROIC_CFG_13           	    = 0x008D,
    eFPGA_REG_ROIC_CFG_14           	    = 0x008E,
    eFPGA_REG_ROIC_CFG_15           	    = 0x008F,

    eFPGA_REG_ROIC_RESET                    = 0x0090,
    eFPGA_REG_ROIC_ACTIVE_POWER_MODE        = 0x0091,

    eFPGA_REG_ROIC_SYNC_WIDTH               = 0x00A0,
    eFPGA_REG_ROIC_ACLK_WIDTH               = 0x00A1,
    eFPGA_REG_ROIC_ACLK_NUM                 = 0x00A2,
    eFPGA_REG_ROIC_ACLK0                    = 0x00A3,
    eFPGA_REG_ROIC_ACLK1                    = 0x00A4,
    eFPGA_REG_ROIC_ACLK2                    = 0x00A5,
    eFPGA_REG_ROIC_ACLK3                    = 0x00A6,
    eFPGA_REG_ROIC_ACLK4                    = 0x00A7,
    eFPGA_REG_ROIC_ACLK5                    = 0x00A8,
    eFPGA_REG_ROIC_ACLK6                    = 0x00A9,
    eFPGA_REG_ROIC_ACLK7                    = 0x00AA,
    eFPGA_REG_ROIC_ACLK8                    = 0x00AB,
    eFPGA_REG_ROIC_ACLK9                    = 0x00AC,
    eFPGA_REG_ROIC_ACLK10                   = 0x00AD,
    eFPGA_REG_ROIC_ACLK11                   = 0x00AE,
    eFPGA_REG_ROIC_ACLK12                   = 0x00AF,
    eFPGA_REG_ROIC_ACLK13                   = 0x00B0,

    //ADI ROIC REG END

    eFPGA_REG_READ_STOP_ENABLE	            = 0x00B6,
    
    eFPGA_AED_ERR_NUM                       = 0x00C2,           /*EXP 구간에서 TCON_AED_ERR_NUM 이상 aed가 지속되면 오류로 판단하여 EXPOSURE 구간을 종료*/
    eFPGA_XRAY_DELAY_NUM                    = 0x00C3,           /*EXP 구간에서 TCON_XRAY_DELAY_TIME 이상 debounce aed 신호 입력이 없으면 오류로 판단하여 EXPOSURE 구간을 종료*/
    eFPGA_OFFSET_CAP_LIMIT_MIN              = 0x00C4,           /*capture EXPOSURE 구간(double readout에서 두번째 촬영)이 LIMIT범위(0xC4~0xC5) 밖이면 기존의 절반 길이로 설정하여 촬영*/
    eFPGA_OFFSET_CAP_LIMIT_MAX              = 0x00C5,           /*capture EXPOSURE 구간(double readout에서 두번째 촬영)이 LIMIT범위(0xC4~0xC5) 밖이면 기존의 절반 길이로 설정하여 촬영*/

    eFPGA_XRAY_FRAME_EXPOSURE_TIME_LOW      = 0x00C6,           /*X-RAY FRAME Exposure time*/
    eFPGA_XRAY_FRAME_EXPOSURE_TIME_HIGH     = 0x00C7,           /*X-RAY FRAME Exposure time*/
    eFPGA_OFFSET_FRAME_EXPOSURE_TIME_LOW    = 0x00C8,           /*OFFSET FRAME Exposure time*/
    eFPGA_OFFSET_FRAME_EXPOSURE_TIME_HIGH   = 0x00C9,           /*OFFSET FRAME Exposure time*/

    eFPGA_REG_STV_DIR			            = 0x00D0,
    
    eFPGA_REG_OE_READ_OUT		            = 0x00D9,   		/* The value of these bits corresponds to the rising edge of GCLK on read-out in a frame.*/
    eFPGA_REQ_GATE_STATE		            = 0x00DB,
    
    eFPGA_REG_GCLK_WIDTH                    = 0x00E1,
    eFPGA_REG_GCLK_NUM_FLUSH                = 0x00E2,
    
    eFPGA_REG_GCLK0_FLUSH                   = 0x00E4,
    eFPGA_REG_GCLK1_FLUSH                   = 0x00E5,
    eFPGA_REG_GCLK2_FLUSH                   = 0x00E6,
    eFPGA_REG_GCLK3_FLUSH                   = 0x00E7,
    eFPGA_REG_GCLK4_FLUSH                   = 0x00E8,
    eFPGA_REG_GCLK5_FLUSH                   = 0x00E9,
    eFPGA_REG_GCLK6_FLUSH                   = 0x00EA,
    eFPGA_REG_GCLK7_FLUSH                   = 0x00EB,
    
    eFPGA_GCLK0_READ_OUT         		    = 0x00EC,
    eFPGA_GCLK1_READ_OUT         		    = 0x00ED,
    eFPGA_GCLK2_READ_OUT         		    = 0x00EE,
    eFPGA_GCLK3_READ_OUT         		    = 0x00EF,
    eFPGA_GCLK4_READ_OUT         		    = 0x00F0,
    eFPGA_GCLK5_READ_OUT         		    = 0x00F1,
    eFPGA_GCLK6_READ_OUT         		    = 0x00F2,
    eFPGA_GCLK7_READ_OUT         		    = 0x00F3,
    eFPGA_GCLK8_READ_OUT         		    = 0x00F4,
    eFPGA_GCLK9_READ_OUT         		    = 0x00F5,
    eFPGA_GCLK10_READ_OUT        		    = 0x00F6,
    eFPGA_GCLK11_READ_OUT        		    = 0x00F7,
    eFPGA_GCLK12_READ_OUT        		    = 0x00F8,
    eFPGA_GCLK13_READ_OUT        		    = 0x00F9,
    eFPGA_GCLK14_READ_OUT        		    = 0x00FA,
    eFPGA_GCLK15_READ_OUT        		    = 0x00FB,
    
    eFPGA_REG_VCOM_MIN_CNT                  = 0x0100,
    eFPGA_REG_VCOM_REMAIN_CNT               = 0x0101,
    
    eFPGA_REG_VCOM_SWING_CNT_LOW            = 0x0102,
    eFPGA_REG_VCOM_SWING_CNT_HIGH           = 0x0103,
        
    eFPGA_REG_TRIGGER_TIME_STAMP_FLAG       = 0x0110,
    eFPGA_REG_TRIGGER_TIME_STAMP_00_LB      = 0x0111,
    eFPGA_REG_TRIGGER_TIME_STAMP_00_HB      = 0x0112,
    eFPGA_REG_TRIGGER_TIME_STAMP_01_LB      = 0x0113,
    eFPGA_REG_TRIGGER_TIME_STAMP_01_HB      = 0x0114,
    eFPGA_REG_TRIGGER_TIME_STAMP_02_LB      = 0x0115,
    eFPGA_REG_TRIGGER_TIME_STAMP_02_HB      = 0x0116,
        
    eFPGA_REG_DDR_IDLE_STATE			    = 0x0127,
    eFPGA_REG_SET_MIG_STATE_STB_CNT		    = 0x0128,
    eFPGA_REG_MIG_ERROR					    = 0x0129,
        
    eFPGA_REG_DETECTOR_MODEL_SEL		    = 0x0140,
        
    eFPGA_TI_DAC_MUX_SELECT				    = 0x0144,
        
    eFPGA_REG_DRO_IMG_CAPTURE_OPTION        = 0x01A0,		/* [0] 1: Double readout mode enable, 0: Single readout mode */
    eFPGA_REG_DRO_INTRA_FLUSH_NUM	        = 0x01A1,		/* Double readout mode���� ù��° ����� �ι�° ���� ���� �ð� ���� ��(flush �ֱ�) */   
    
	eFPGA_REG_TFT_AED_OPTION		        = 0x01A2,
    eFPGA_REG_TFT_AED_ROIC_SEL_L	        = 0x01A4,
    eFPGA_REG_TFT_AED_ROIC_SEL_H	        = 0x01A5,
    
    eFPGA_REG_TI_ROIC_CONFIG_00             = 0x0200,
    eFPGA_REG_TI_ROIC_CONFIG_10             = 0x0210,
    eFPGA_REG_TI_ROIC_CONFIG_11             = 0x0211,
    eFPGA_REG_TI_ROIC_CONFIG_12             = 0x0212,
    eFPGA_REG_TI_ROIC_CONFIG_13             = 0x0213,
    eFPGA_REG_TI_ROIC_CONFIG_16             = 0x0216,
    eFPGA_REG_TI_ROIC_CONFIG_18             = 0x0218,
    eFPGA_REG_TI_ROIC_CONFIG_2C             = 0x022C,
    eFPGA_REG_TI_ROIC_CONFIG_2E             = 0x022E,
    eFPGA_REG_TI_ROIC_CONFIG_30             = 0x0230,
    eFPGA_REG_TI_ROIC_CONFIG_31             = 0x0231,
    eFPGA_REG_TI_ROIC_CONFIG_32             = 0x0232,
    eFPGA_REG_TI_ROIC_CONFIG_33             = 0x0233,
    eFPGA_REG_TI_ROIC_CONFIG_40             = 0x0240,
    eFPGA_REG_TI_ROIC_CONFIG_41             = 0x0241,
    eFPGA_REG_TI_ROIC_CONFIG_42             = 0x0242,
    eFPGA_REG_TI_ROIC_CONFIG_43             = 0x0243,
    eFPGA_REG_TI_ROIC_CONFIG_46             = 0x0246,
    eFPGA_REG_TI_ROIC_CONFIG_47             = 0x0247,
    eFPGA_REG_TI_ROIC_CONFIG_4A             = 0x024A,
    eFPGA_REG_TI_ROIC_CONFIG_4B             = 0x024B,
    eFPGA_REG_TI_ROIC_CONFIG_4C             = 0x024C,
    eFPGA_REG_TI_ROIC_CONFIG_50             = 0x0250,
    eFPGA_REG_TI_ROIC_CONFIG_51             = 0x0251,
    eFPGA_REG_TI_ROIC_CONFIG_52             = 0x0252,
    eFPGA_REG_TI_ROIC_CONFIG_53             = 0x0253,
    eFPGA_REG_TI_ROIC_CONFIG_54             = 0x0254,
    eFPGA_REG_TI_ROIC_CONFIG_55             = 0x0255,
    eFPGA_REG_TI_ROIC_CONFIG_5A             = 0x025A,
    eFPGA_REG_TI_ROIC_CONFIG_5C             = 0x025C,
    eFPGA_REG_TI_ROIC_CONFIG_5D             = 0x025D,
    eFPGA_REG_TI_ROIC_CONFIG_5E             = 0x025E,
    eFPGA_REG_TI_ROIC_CONFIG_61             = 0x0261,
    
    //TEMP  
    eFPGA_REG_FW_TEST                       = 0x0265,
    
	eFPGA_REG_TI_ROIC_MCLK 		            = 0x0270,
	eFPGA_REG_TI_ROIC_DAC_COMP1	            = 0x0271,
	eFPGA_REG_TI_ROIC_DAC_COMP3             = 0x0272,
	eFPGA_REG_TI_ROIC_LOW_POWER             = 0x0273,
	eFPGA_REG_ROIC_GAIN_TYPE                = 0x0274,                   //20210907. SDK 하위호환을위해 VW ADI ROIC 에서도 해당 값을 이용하여 Gain type 설정 
    
	eFPGA_REG_SENSITIVITY 		            = 0x0275,
	eFPGA_REG_CUR_GAIN_RATIO	            = 0x0276,
    
	eFPGA_REG_LOGICAL_DIGITAL_GAIN          = 0x0280,
    eFPGA_REG_GAIN_COMPENSATION             = 0x0281,
    
    eFPGA_REG_TI_ROIC_BIT_ALIGN_ERR		    = 0x0299,
    
        
    
	eFPGA_TI_ROIC_SCAN				        = 0x0303,
    
    
    eFPGA_REG_MAX                           = REG_ADDRESS_MAX,
        
} FPGA_REG;

//FPGA ISP REG. 추후 TG reg와 형식 통일 필요 
#define FPGA_IPS_REG_BASE							(0x4000)

#define FPGA_ISP_REG_OFFSET_CORR_ON					(FPGA_IPS_REG_BASE | 0x0073)
#define FPGA_ISP_REG_GAIN_CORR_ON					(FPGA_IPS_REG_BASE | 0x0083)

#define FPGA_ISP_REG_GAIN_CORR_TARGET_GRAY_LEVEL	(FPGA_IPS_REG_BASE | 0x0084)

#define FPGA_ISP_REG_OFFSET_CAL_ENABLE				(FPGA_IPS_REG_BASE | 0x0054)

#define FPGA_ISP_REG_IMAGE_LINE_COUNT				(FPGA_IPS_REG_BASE | 0x0035)

#define FPGA_ISP_REG_CRC_ON							(FPGA_IPS_REG_BASE | 0x0340)
#define FPGA_ISP_REG_CRC_START_ADDR_L				(FPGA_IPS_REG_BASE | 0x0343)
#define FPGA_ISP_REG_CRC_START_ADDR_H				(FPGA_IPS_REG_BASE | 0x0344)
#define FPGA_ISP_REG_CRC_END_ADDR_L					(FPGA_IPS_REG_BASE | 0x0345)
#define FPGA_ISP_REG_CRC_END_ADDR_H					(FPGA_IPS_REG_BASE | 0x0346)
#define FPGA_ISP_REG_CRC_DATA_L						(FPGA_IPS_REG_BASE | 0x0347)
#define FPGA_ISP_REG_CRC_DATA_H						(FPGA_IPS_REG_BASE | 0x0348)

typedef enum
{
    eTRIG_SELECT_ACTIVE_LINE  = 1,
    eTRIG_SELECT_NON_LINE     = 3,
}TRIGGER_SELECT;

typedef enum
{
    eGEN_INTERFACE_RAD  					= 0,
    eGEN_INTERFACE_PF   					= 1,        //Not use
    eGEN_INTERFACE_CF   					= 3,        //Not use
}GEN_INTERFACE_TYPE;

/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*	Variable Definitions
*******************************************************************************/


/*******************************************************************************
*	Function Prototypes
*******************************************************************************/

#endif /* end of _FPGA_REG_H_*/

