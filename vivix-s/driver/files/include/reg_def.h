#ifndef VIVIX_GPIO_REG
#define VIVIX_GPIO_REG

#define SPI_DRIVER
/***************************************************************************************************
*   Include Files
***************************************************************************************************/
#include <linux/gpio.h>

#define FLEX_SPI_BUS_NUM	0
#define ECSPI1_BUS_NUM		1
#define ECSPI2_BUS_NUM		2
#define ECSPI3_BUS_NUM		3

/***************************************************************************************************
*   Constant Definitions
***************************************************************************************************/
#define GPIO1       (1)
#define GPIO2       (2)
#define GPIO3       (3)
#define GPIO4       (4)
#define GPIO5       (5)

#define INPUT		0
#define OUTPUT		1

#define LOW			0
#define	HIGH		1

enum {
	// GPIO1
	GPIO1_IO00 = 0,
	GPIO1_IO01,
	GPIO1_IO02,
	GPIO1_IO03,
	GPIO1_IO04,
	GPIO1_IO05,
	GPIO1_IO06,
	GPIO1_IO07,
	GPIO1_IO08,
	GPIO1_IO09,
	GPIO1_IO10,
	GPIO1_IO11,
	GPIO1_IO12,
	GPIO1_IO13,
	GPIO1_IO14,
	GPIO1_IO15,
	GPIO1_IO16,
	GPIO1_IO17,
	GPIO1_IO18,
	GPIO1_IO19,
	GPIO1_IO20,
	GPIO1_IO21,
	GPIO1_IO22,
	GPIO1_IO23,
	GPIO1_IO24,
	GPIO1_IO25,
	GPIO1_IO26,
	GPIO1_IO27,
	GPIO1_IO28,
	GPIO1_IO29,
	GPIO1_IO30,
	GPIO1_IO31,

	// GPIO2
	GPIO2_IO00 = 32,
	GPIO2_IO01,
	GPIO2_IO02,
	GPIO2_IO03,
	GPIO2_IO04,
	GPIO2_IO05,
	GPIO2_IO06,
	GPIO2_IO07,
	GPIO2_IO08,
	GPIO2_IO09,
	GPIO2_IO10,
	GPIO2_IO11,
	GPIO2_IO12,
	GPIO2_IO13,
	GPIO2_IO14,
	GPIO2_IO15,
	GPIO2_IO16,
	GPIO2_IO17,
	GPIO2_IO18,
	GPIO2_IO19,
	GPIO2_IO20,
	GPIO2_IO21,
	GPIO2_IO22,
	GPIO2_IO23,
	GPIO2_IO24,
	GPIO2_IO25,
	GPIO2_IO26,
	GPIO2_IO27,
	GPIO2_IO28,
	GPIO2_IO29,
	GPIO2_IO30,
	GPIO2_IO31,

	// GPIO3
	GPIO3_IO00 = 64,
	GPIO3_IO01,
	GPIO3_IO02,
	GPIO3_IO03,
	GPIO3_IO04,
	GPIO3_IO05,
	GPIO3_IO06,
	GPIO3_IO07,
	GPIO3_IO08,
	GPIO3_IO09,
	GPIO3_IO10,
	GPIO3_IO11,
	GPIO3_IO12,
	GPIO3_IO13,
	GPIO3_IO14,
	GPIO3_IO15,
	GPIO3_IO16,
	GPIO3_IO17,
	GPIO3_IO18,
	GPIO3_IO19,
	GPIO3_IO20,
	GPIO3_IO21,
	GPIO3_IO22,
	GPIO3_IO23,
	GPIO3_IO24,
	GPIO3_IO25,
	GPIO3_IO26,
	GPIO3_IO27,
	GPIO3_IO28,
	GPIO3_IO29,
	GPIO3_IO30,
	GPIO3_IO31,

	// GPIO4
	GPIO4_IO00 = 96,
	GPIO4_IO01,
	GPIO4_IO02,
	GPIO4_IO03,
	GPIO4_IO04,
	GPIO4_IO05,
	GPIO4_IO06,
	GPIO4_IO07,
	GPIO4_IO08,
	GPIO4_IO09,
	GPIO4_IO10,
	GPIO4_IO11,
	GPIO4_IO12,
	GPIO4_IO13,
	GPIO4_IO14,
	GPIO4_IO15,
	GPIO4_IO16,
	GPIO4_IO17,
	GPIO4_IO18,
	GPIO4_IO19,
	GPIO4_IO20,
	GPIO4_IO21,
	GPIO4_IO22,
	GPIO4_IO23,
	GPIO4_IO24,
	GPIO4_IO25,
	GPIO4_IO26,
	GPIO4_IO27,
	GPIO4_IO28,
	GPIO4_IO29,
	GPIO4_IO30,
	GPIO4_IO31,

	// GPIO5
	GPIO5_IO00 = 128,
	GPIO5_IO01,
	GPIO5_IO02,
	GPIO5_IO03,
	GPIO5_IO04,
	GPIO5_IO05,
	GPIO5_IO06,
	GPIO5_IO07,
	GPIO5_IO08,
	GPIO5_IO09,
	GPIO5_IO10,
	GPIO5_IO11,
	GPIO5_IO12,
	GPIO5_IO13,
	GPIO5_IO14,
	GPIO5_IO15,
	GPIO5_IO16,
	GPIO5_IO17,
	GPIO5_IO18,
	GPIO5_IO19,
	GPIO5_IO20,
	GPIO5_IO21,
	GPIO5_IO22,
	GPIO5_IO23,
	GPIO5_IO24,
	GPIO5_IO25,
	GPIO5_IO26,
	GPIO5_IO27,
	GPIO5_IO28,
	GPIO5_IO29,
	GPIO5_IO30,
	GPIO5_IO31
};

#define IMX_GPIO_NR(bank, nr)   (((bank) - 1) * 32 + (nr))

#if 1//testcode
#define AED_SPI_CLK			GPIO3_IO25
#define AED_SPI_MOSI		GPIO3_IO19
#define AED_SPI_MISO		GPIO3_IO20
#define AED_SPI_SS0			GPIO3_IO21 
#define AED_SPI_SS1			GPIO3_IO22
#define AED_SPI_SS2			GPIO3_IO23
#define AED_SPI_SS3			GPIO3_IO24
#define AED_PWR_EN		 	GPIO5_IO02
#define AED_TEMP			GPIO4_IO01 //TODO

#define OLED_SPI_CLK		GPIO1_IO10	
#define OLED_SPI_MOSI		GPIO1_IO09
#define OLED_SS				GPIO1_IO11
#define OLED_RST           	GPIO1_IO01
#define OLED_A0          	GPIO1_IO08

#define PMIC_RST            GPIO4_IO31    // PMIC_RST GPIO3_IO31 output

#define MCU_SPI_CLK			GPIO4_IO20			
#define MCU_SPI_MOSI		GPIO4_IO10
#define MCU_SPI_MISO		GPIO4_IO11
#define MCU_SPI_SS			GPIO4_IO00

#define MCU_RST				GPIO5_IO23

#define AP_BUTTON          	GPIO4_IO31    

#define B_CHG_EN_A        	GPIO5_IO00    
#define B_CHG_EN_B        	GPIO5_IO01    

#define B_nPG_A            	GPIO4_IO28    
#define B_nPG_B        	   	GPIO5_IO05   

#define B_STAT1_A          	GPIO4_IO29    
#define B_STAT2_A          	GPIO4_IO30    
#define B_STAT1_B          	GPIO5_IO03    
#define B_STAT2_B           GPIO5_IO04   

#define BOOT_LED_RST       	GPIO4_IO27    

#define TETHER_EQUIP       	GPIO4_IO24    
#define ADAPTOR_EQUIP      	GPIO4_IO25    

#define PCIE_RST       		GPIO4_IO21 

#define PHY_RESET          	GPIO1_IO00


#define FPGA_RST            GPIO1_IO01
#define FPGA_INIT          	GPIO1_IO05 
#define FPGA_CONFIG_EN		GPIO4_IO26
#define FPGA_PGM            FPGA_CONFIG_EN //?? 
#define FPGA_CONFIG_RST		GPIO1_IO06	
#define FPGA_DONE          	GPIO1_IO07 

#define F_INF0_PORT         GPIO1_IO05
#if 1//?? TODO
#define F_INF1_PORT             (158)     // F_INF1 GPIO06_IO07 input -> 촬영시작, wake up intterupt
#define F_INF2_PORT             (158)    // F_INF2 GPIO06_IO11 input -> 촬영 종료, power down interrupt
#define F_INF3_PORT             (158)    // F_INF3 GPIO06_IO14 input
#endif


#define ACC_INT1       		GPIO5_IO24
#define ACC_INT2       		GPIO5_IO25




#endif

#define EXP_REQ             GPIO4_IO16    // EXP REQ GPIO4_IO16 output
#define EXP_OK              GPIO4_IO18    // EXP OK  GPIO4_IO18 input

#define TG_PWR_EN           GPIO4_IO19    // TG_PWR_EN GPIO4_IO19 output
#define RO_PWR_EN           GPIO4_IO20    // RO_PWR_EN GPIO4_IO20 output  //ACTIVE HIGH
#define ROIC_MCLK_EN        RO_PWR_EN

#define PWR_OFF             GPIO4_IO28    // PWR_OFF GPIO4_IO28 output

#define FCS                GPIO5_IO08    // F_FCS_B GPIO5_I8
#define FD0                GPIO5_IO07    // F_CFG_D0 GPIO5_I7
#define FD1                GPIO5_IO06    // F_CFG_D1 GPIO5_I6
#define FD2                GPIO5_IO05    // F_CFG_D2 GPIO5_I5
#define FD3                GPIO4_IO31    // F_CFG_D3 GPIO4_I31
#define FCLK               GPIO4_IO30    // F_CCLK GPIO4_I30

#define AEN_PIN            GPIO2_IO22    // MUX_AEN pin GPIO2_IO22 output
#define BEN_PIN            GPIO2_IO21    // MUX_BEN pin GPIO2_IO21 output

#define VER0               GPIO2_IO00    // VER0 GPIO2_IO00 input
#define VER1               GPIO2_IO01    // VER1 GPIO2_IO01 input
#define VER2               GPIO2_IO02    // VER2 GPIO2_IO02 input
#define VER3               GPIO2_IO03    // VER3 GPIO2_IO03 input
#define VER4               GPIO2_IO04    // VER4 GPIO2_IO04 input

#define BD_CFG_0           GPIO1_IO24    // BD_CFG_0 GPIO1_IO24 input
#define BD_CFG_1           GPIO1_IO28    // BD_CFG_1 GPIO1_IO28 input
#define BD_CFG_2           GPIO1_IO27    // BD_CFG_2 GPIO1_IO27 input


#define PWR_BUTTON         GPIO1_IO00    // M_BUTTON    GPIO1_IO00  input
#define POWER_LED          GPIO1_IO03    // POWER_LED   GPIO1_IO03  output




#define VDD_CORE_ENABLE    GPIO1_IO05    // VDD_CNT GPIO1_IO05   output


#define ADC_GPIO           GPIO4_IO14    // ADC_GPIO GPIO4_IO14  output


// ???????????????????????
#define EXP_SYNC_BIT_MASK  GPIO4_IO18    // EXP SYNC GPIO4_IO18 input
#define EXP_OK_BIT_MASK    GPIO4_IO16    // EXP OK  GPIO4_IO16 input

#if 0//TO_DO ???
#define A1_PD_BIT_MASK          (10)    //  A1_PD_BIT_MASK  GPIO6_IO10  output
#define A2_PD_BIT_MASK          (5)     //  A2_PD_BIT_MASK  GPIO2_IO05  output
#define A3_PD_BIT_MASK          (6)     //  A3_PD_BIT_MASK  GPIO2_IO06  output
#define A4_PD_BIT_MASK          (7)     //  A4_PD_BIT_MASK  GPIO2_IO07  output

/* DIP swtich control */
#define ROIC_TYPE_1_BIT_MASK_1717G  (5)             //  1717G ROIC_TYPE_BIT_MASK    NANDF_D5,  GPIO2_IO05 input, DIP SW1
#define ROIC_TYPE_2_BIT_MASK_1717G  (6)             //  1717G ROIC_TYPE_BIT_MASK    NANDF_D6,  GPIO2_IO06 input, DIP SW2
#define ROIC_SINGLE_READOUT_BIT_MASK_1717G  (7)     //  1717G ROIC_SINGLE_READOUT_BIT_MASK  NANDF_D7,  GPIO2_IO07 input, DIP SW3
#define ROIC_GATE_TYPE_1_BIT_MASK_1717G     (21)    //  1717G ROIC_GATE_TYPE_1_BIT_MASK,    SD1_DATA3, GPIO1_IO21 input, DIP SW4
#define ROIC_GATE_TYPE_2_BIT_MASK_1717G     (19)    //  1717G ROIC_GATE_TYPE_2_BIT_MASK,    GPIO1_IO19 input, DIP SW5
#define RESERVED_1_BIT_MASK_1717G   (17)            //  1717G RESERVED BIT_MASK SD1_DATA1, GPIO1_IO17 input, DIP SW6
#define RESERVED_2_BIT_MASK_1717G   (16)            //  1717G RESERVED BIT_MASK SD1_DATA0, GPIO1_IO16 input, DIP SW7
#define RESERVED_3_BIT_MASK_1717G   (20)            //  1717G RESERVED BIT_MASK SD1_CLK, GPIO1_IO20 input, DIP SW8

/* DIP swtich control */
#define ROIC_TYPE_1_BIT_MASK_0909G_1212G    (30)    //  0909G/1212G ROIC_TYPE_BIT_MASK  GPIO1_IO30 input
#define ROIC_TYPE_2_BIT_MASK_0909G_1212G    (31)    //  0909G/1212G ROIC_TYPE_BIT_MASK  GPIO1_IO31 input
#define RESERVED_1_BIT_MASK_0909G_1212G     (29)    //  0909G/1212G RESERVED BIT_MASK   GPIO1_IO29 input
#define RESERVED_2_BIT_MASK_0909G_1212G     (25)    //  0909G/1212G RESERVED BIT_MASK   GPIO1_IO25 input

/* fpga 200T configuration pin */
#define FPGA_RDWR_B                     (12) // FPGA_RDWR GPIO5_IO12 output
#define FPGA_CSI_B                      (13) // FPGA_CSI GPIO5_IO13 output
#define FPGA_RST_BIT_MASK               (14) // TG_PD GPIO5_IO14    output
#define FPGA_PROG_B                     (15) // PROG_PIN GPIO5_IO15 output
#define FPGA_INIT                       (16) // INIT PIN GPIO5_IO16 input
#define FPGA_DONE                       (17) // DONE PIN GPIO5_IO17 input
#define FPGA_CCLK                       (30) // F_CCLK GPIO4_IO30   output

/* fpga 200T configuration pin */
#define FPGA_RDWR_B                     (12) // FPGA_RDWR GPIO5_IO12 output
#define FPGA_CSI_B                      (13) // FPGA_CSI GPIO5_IO13 output
#define FPGA_RST_BIT_MASK               (14) // TG_PD GPIO5_IO14    output
#define FPGA_PROG_B                     (15) // PROG_PIN GPIO5_IO15 output
#define FPGA_INIT                       (16) // INIT PIN GPIO5_IO16 input
#define FPGA_DONE                       (17) // DONE PIN GPIO5_IO17 input
#define FPGA_CCLK                       (30) // F_CCLK GPIO4_IO30   output

/* fpga 75T configuration pin */
#define FPGA_75T_DONE_PIN               (2)  // DONE PIN GPIO1_IO02 input
#define FPGA_75T_PROG_PIN               (27) // PROG_PIN GPIO4_IO27 output

/* BRAM read / write interrupt pin (for calibration data transfer) */
#define ISP_BRAM_RDWR_INTR_BIT_MASK     (8)  // F_INF1  GPIO6_IO08  input

#define TG_PD_BIT_MASK                  (19) // TG_PD GPIO4_IO19    output
#define ROIC_CLK_EN                     (20) // RO_CLK_EN GPIO4_IO20    output

#define DFF_OE                          (00) // GPIO1_IO00 output

// [20190625][yongjin] ROIC IO voltage 1.8V를 맞추기 위하여 SPI PIN을 변경 함
//  Hardware rev. 2 ~ 3 : Ver1
#define ROIC_SPI_SCLK_VER1_PINGRP       (4)
#define ROIC_SPI_SCLK_VER1_PORT         (21)                    // SPI_SCLK GPIO4_IO21
#define ROIC_SPI_MOSI_VER1_PINGRP       (4)
#define ROIC_SPI_MOSI_VER1_PORT         (22)                    // SPI_MOSI GPIO4_IO22
#define ROIC_SPI_MISO_VER1_PINGRP       (4)
#define ROIC_SPI_MISO_VER1_PORT         (23)                    // SPI_MISO GPIO4_IO23
#define ROIC_SPI_SS_VER1_PINGRP         (4)
#define ROIC_SPI_SS_VER1_PORT           (24)                    // SPI_SS0 GPIO4_IO24
//  Hardware rev. 4 ~ : Ver2
#define ROIC_SPI_SCLK_VER2_PINGRP       (1)
#define ROIC_SPI_SCLK_VER2_PORT         (15)                    // SPI_SCLK GPIO1_IO15
#define ROIC_SPI_MOSI_VER2_PINGRP       (1)
#define ROIC_SPI_MOSI_VER2_PORT         (14)                    // SPI_MOSI GPIO1_IO14
#define ROIC_SPI_MISO_VER2_PINGRP       (1)
#define ROIC_SPI_MISO_VER2_PORT         (13)                    // SPI_MISO GPIO1_IO13
#define ROIC_SPI_SS_VER2_PINGRP         (1)
#define ROIC_SPI_SS_VER2_PORT           (12)                    // SPI_SS0 GPIO1_IO12

#define ROIC_SCLK_MUX1_OE_BIT_MASK      (22)                    //GPIO5_IO22
#define ROIC_SCLK_MUX2_OE_BIT_MASK      (23)                    //GPIO5_IO23
#define ROIC_SCLK_MUX3_OE_BIT_MASK      (24)                    //GPIO5_IO24
#define ROIC_SCLK_MUX_S0_BIT_MASK       (25)                    //GPIO5_IO25
#define ROIC_SCLK_MUX_S1_BIT_MASK       (26)                    //GPIO5_IO26
#define ROIC_SCLK_MUX_S2_BIT_MASK       (27)                    //GPIO5_IO27

#define ROIC_MCLK_EN_BIT_MASK           (20)                    // ROIC_CLK_EN  GPIO4_IO20  //MAY NOT NEED
#define ROIC_TPSEL_BIT_MASK             (20)                    // GPIO5_IO20

#define DAC_SPI_SCLK_PINGRP             (6)
#define DAC_SPI_SCLK_PORT               (5)                     //CSI0_DAT19, GPIO6_IO05
#define DAC_SPI_MOSI_PINGRP             (6)
#define DAC_SPI_MOSI_PORT               (4)                     //CSI0_DAT17, GPIO6_IO04
#define DAC_SPI_MISO_PINGRP             (6)
#define DAC_SPI_MISO_PORT               (3)                     //CSI0_DAT18, GPIO6_IO03
#define DAC1_SPI_SS_PINGRP              (6)
#define DAC1_SPI_SS_PORT                (1)                     //CSI0_DAT15, GPIO6_IO01
#define DAC2_SPI_SS_PINGRP              (6)
#define DAC2_SPI_SS_PORT                (2)                     //CSI0_DAT16, GPIO6_IO02
#define DAC_RST_BIT_MASK                (00)                    //CSI0_DAT14, GPIO6_IO00


#endif



#define UART1_TX_MUX_REG        0x020E0330
#define UART1_PAD_MUX_ATL5      5
#define UART1_PAD_MUX_ATL1      1

/***************************************************************************************************
*   Macros (Inline Functions) Definitions
***************************************************************************************************/


#endif 
