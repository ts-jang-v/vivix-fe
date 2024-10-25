/*
 * NXP FlexSPI(FSPI) controller driver for FPGA.
 *
 * Copyright 2019-2020 NXP
 * Copyright 2020 Puresoftware Ltd.
 *
 * FlexSPI is a flexsible SPI host controller which supports two SPI
 * channels and up to 4 external devices. Each channel supports
 * Single/Dual/Quad/Octal mode data transfer (1/2/4/8 bidirectional
 * data lines).
 *
 * FlexSPI controller is driven by the LUT(Look-up Table) registers
 * LUT registers are a look-up-table for sequences of instructions.
 * A valid sequence consists of four LUT registers.
 * Maximum 32 LUT sequences can be programmed simultaneously.
 *
 * LUTs are being created at run-time based on the commands passed
 * from the spi-mem framework, thus using single LUT index.
 *
 * Software triggered Flash read/write access by IP Bus.
 *
 * Memory mapped read access by AHB Bus.
 *
 * Based on SPI MEM interface and spi-fsl-qspi.c driver.
 *
 *     Frieder Schrempf <frieder.schrempf@kontron.de>
 */

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/sizes.h>
#include "flexspi_imx8.h"

static unsigned int pre_div = 2;
module_param(pre_div, uint, 0644);
MODULE_PARM_DESC(pre_div, "flexspi root clock pre divider value (1-8)");

static unsigned int post_div = 5;
module_param(post_div, uint, 0644);
MODULE_PARM_DESC(post_div, "flexspi root clock post divider value (1-64)");

#define FPGA_FLASH_SIZE		(16*1024)	/* FPGA device size 16*1024(KB) = 16MB */
#define ARD_SEQ_NUMBER	1			/* Sequence number for AHB read command */
#define ARD_SEQ_INDEX	0			/* Sequence ID for AHB read command */
#define AWR_SEQ_NUMBER	1			/* Sequence number for AHB read command */
#define AWR_SEQ_INDEX	1			/* Sequence number for AHB read command */
#define CMD_LUT_SEQ_IDX_RD_4BIT_SDR	0
#define CMD_LUT_SEQ_IDX_WR_4BIT_SDR	1
#define CMD_LUT_SEQ_IDX_RD_4BIT_DDR	0
#define CMD_LUT_SEQ_IDX_WR_4BIT_DDR	1
#define CMD_LUT_SEQ_IDX_READDATA    0
#define CMD_LUT_SEQ_IDX_WRITEDATA   1

const u32 customLUT[8] = {
	/* Read Data   cmd =0x09 */
	[4 * CMD_LUT_SEQ_IDX_RD_4BIT_SDR] =
        	FlexSPI_LUT_SEQ(LUT_CMD_SDR , kFlexSPI_4PAD, 0x09, LUT_READ_SDR, kFlexSPI_4PAD, 0x04),	

    	/* Write Data  cmd = 0x08 */
    	[4 * CMD_LUT_SEQ_IDX_WR_4BIT_SDR] =
        	FlexSPI_LUT_SEQ(LUT_CMD_SDR , kFlexSPI_4PAD, 0x08, LUT_WRITE_SDR, kFlexSPI_4PAD, 0x02),
};

static void flexspi_update_lut(FlexSPI_Type *base, u32 index, const u32 *cmd, u32 count)
{
	int i;
	volatile u32 *lutBase;

	/* Unlock LUT for update. */
	base->LUTKEY = FSPI_LUTKEY_VALUE;
	base->LUTCR  = 0x02;

	lutBase = &base->LUT[index];
	for (i = 0; i < count; i++)
	{
		*lutBase++ = *cmd++;
		printk("lut instr id-%d-cmd %x\n", i, *lutBase);
	}

	/* Lock LUT. */
	base->LUTKEY = FSPI_LUTKEY_VALUE;
	base->LUTCR  = 0x01;
}

static void flexspi_software_reset(FlexSPI_Type *base)
{
	base->MCR0 |= FSPI_MCR0_SWRST;
}

static void flexspi_clock_init(u32 mux, u32 pre, u32 post)
{
	volatile u32 __iomem *ccm_base = ioremap(CCM_BASE, SZ_64K);
	if(ccm_base == NULL){
		printk(KERN_WARNING "ccm_base ioremap failed\n");
	}

	/* Domain clocks needed all the time */
	*(ccm_base + CCM_CCGR47 / 4) = 0x3333;

	/* Selection of root clock sources and set pre/post divider*/
	*(ccm_base + QSPI_CLK_ROOT / 4) =
				CLK_ROOT_EN |
				MUX_CLK_ROOT_SELECT(mux) |
				PRE_PODF(pre - 1) |
				POST_PODF(post - 1);

	iounmap(ccm_base);
	if( post != 2)
		printk("set the flexspi sclk to %d MHz\n", 400/pre/post);
}

static void flexspi_get_default_config(flexspi_config_t *config)
{
	u8 i;

	/* Initializes the configure structure to zero. */
	(void)memset(config, 0, sizeof(*config));

	config->rxSampleClock		   = kFlexSPI_ReadSampleClkLoopbackInternally;
	config->enableSckFreeRunning   = false;
	config->enableCombination	   = false;
	config->enableDoze			   = true;
	config->enableHalfSpeedAccess  = false;
	config->enableSckBDiffOpt	   = false;
	config->enableSameConfigForAll = false;
	config->seqTimeoutCycle		   = 0xFFFFU;
	config->ipGrantTimeoutCycle    = 0xFFU;
	config->txWatermark			   = 8;
	config->rxWatermark			   = 8;

	config->ahbConfig.ahbGrantTimeoutCycle = 0xFFU;
	config->ahbConfig.ahbBusTimeoutCycle   = 0xFFFFU;
	config->ahbConfig.resumeWaitCycle	   = 0x20U;

	config->ahbConfig.enableClearAHBBufferOpt = false;
	config->ahbConfig.enableReadAddressOpt	  = false;
	config->ahbConfig.enableAHBPrefetch		  = false;
	config->ahbConfig.enableAHBBufferable	  = false;
	config->ahbConfig.enableAHBCachable		  = false;

	(void)memset(config->ahbConfig.buffer, 0, sizeof(config->ahbConfig.buffer));
	/* Use invalid master ID 0xF and buffer size 0 for the first several buffers. */
	for (i = 0; i < ((uint8_t)FSL_FEATURE_FlexSPI_AHB_BUFFER_COUNT - 1U); i++)
	{
		config->ahbConfig.buffer[i].enablePrefetch = true; /* Default enable AHB prefetch. */
		config->ahbConfig.buffer[i].masterIndex = 0xFU; /* Invalid master index which is not used, so will never hit. */
		config->ahbConfig.buffer[i].bufferSize = 0; /* Default buffer size 0 for buffer0 to buffer(FSL_FEATURE_FlexSPI_AHB_BUFFER_COUNT - 1U)*/
	}

	for (i = ((uint8_t)FSL_FEATURE_FlexSPI_AHB_BUFFER_COUNT - 1U);
		 i < (uint8_t)FSL_FEATURE_FlexSPI_AHB_BUFFER_COUNT; i++)
	{
		config->ahbConfig.buffer[i].enablePrefetch = true; /* Default enable AHB prefetch. */
		config->ahbConfig.buffer[i].masterIndex = 0x1U; /* Invalid master index is A53. */
		config->ahbConfig.buffer[i].bufferSize = 2048U; /* Default buffer size 2k bytes for BUFFER7. */
	}
}

static void flexspi_init(FlexSPI_Type *base, const flexspi_config_t *flexspiConfig)
{
	u8 i;
	u32 configValue;

	/* Reset peripheral before configuring it. */
	flexspi_software_reset(base);

	/* Configure MCR0 configuration items */
	configValue = base->MCR0;
	configValue =
			FSPI_MCR0_RXCLKSRC(flexspiConfig->rxSampleClock) |
			FSPI_MCR0_DOZE_EN |
			FSPI_MCR0_IP_TIMEOUT(0xFF) |
			FSPI_MCR0_AHB_TIMEOUT(0xFF) |
			FSPI_MCR0_SCRFRUN(flexspiConfig->enableSckFreeRunning) |
			FSPI_MCR0_OCTCOMB(flexspiConfig->enableCombination);
	base->MCR0= configValue;

	/* Configure MCR1 configurations. */
	configValue =
			FSPI_MCR1_SEQ_TIMEOUT(flexspiConfig->seqTimeoutCycle) |
			FSPI_MCR1_AHB_TIMEOUT(flexspiConfig->ahbConfig.ahbBusTimeoutCycle);
	base->MCR1= configValue;

	/* Configure MCR2 configurations. */
	configValue = base->MCR2;
	configValue |=
			FSPI_MCR2_RESUMEWAIT(flexspiConfig->ahbConfig.resumeWaitCycle) |
			FSPI_MCR2_SCKBDIFFOPT(flexspiConfig->enableSckBDiffOpt) |
			FSPI_MCR2_SAMEDEVICE(flexspiConfig->enableSameConfigForAll) |
			FSPI_MCR2_CLRAHBBUFOPT(flexspiConfig->ahbConfig.enableClearAHBBufferOpt);
	base->MCR2= configValue;

	/* Configure AHB control items. */
	configValue = base->AHBCR;
	configValue |=
			FSPI_AHBCR_RDADDROPT(flexspiConfig->ahbConfig.enableReadAddressOpt) |
			FSPI_AHBCR_PREFETCHEN(flexspiConfig->ahbConfig.enableAHBPrefetch) |
			FSPI_AHBCR_BUFFERABLEEN(flexspiConfig->ahbConfig.enableAHBBufferable) |
			FSPI_AHBCR_CACHABLEEN(flexspiConfig->ahbConfig.enableAHBCachable);
	base->AHBCR= configValue;

	/* Configure AHB rx buffers. */
	for (i = 0; i < (uint32_t)FSL_FEATURE_FlexSPI_AHB_BUFFER_COUNT; i++)
	{
		configValue =
				FSPI_AHBRXBUFCR0_PREFETCHEN(flexspiConfig->ahbConfig.buffer[i].enablePrefetch) |
				FSPI_AHBRXBUFCR0_PRIORITY(flexspiConfig->ahbConfig.buffer[i].priority) |
				FSPI_AHBRXBUFCR0_MSTRID(flexspiConfig->ahbConfig.buffer[i].masterIndex) |
				FSPI_AHBRXBUFCR0_BUFSZ(flexspiConfig->ahbConfig.buffer[i].bufferSize / 8U);	/* AHB RX Buffer Size in 64 bits */
		base->AHBRXBUFCR0[i] = configValue;
	}

	/* Reset flash size on all 4 ports */
	for (i = 0; i < kFlexSPI_PortCount; i++)
	{
		base->FLSHCR0[i] = 0;
	}
}

static void flexspi_set_fpga_config(FlexSPI_Type *base)
{
	u32 reg;
	u32 configValue = 0;
    	bool enableWriteMask;
    	int index; 

	/* wait till controller is idle */
	do {
		reg = base->STS0;
		if ((reg & FLEXSPI_STS0_ARB_IDLE_MASK) &&
				(reg & FLEXSPI_STS0_SEQ_IDLE_MASK))
			break;
		udelay(1);
	} while (1);

	/* Configure FlexSPI_PortA1 flash size. */
	base->FLSHCR0[kFlexSPI_PortA1] = FPGA_FLASH_SIZE;	/* FLSHA1CR0 */

	/* Configure FlexSPI_PortA1 fpga parameters. */
	base->FLSHCR1[kFlexSPI_PortA1] =
			FSPI_FLSHCR1_CSINTERVAL(8) |
			FSPI_FLSHCR1_CSINTERVALUNIT(0) |
			FSPI_FLSHCR1_TCSH(0) |
			FSPI_FLSHCR1_TCSS(0) |
			FSPI_FLSHCR1_CAS(0) |
			FSPI_FLSHCR1_WA(0);

	/* Configure FlexSPI_PortA1 AHB operation items. */
	configValue = base->FLSHCR2[kFlexSPI_PortA1];

	if (AWR_SEQ_NUMBER > 0U){
		configValue |= 
				FlexSPI_FLSHCR2_AWRSEQID((uint32_t)AWR_SEQ_INDEX) |
				FlexSPI_FLSHCR2_AWRSEQNUM((uint32_t)AWR_SEQ_NUMBER - 1U);
    	}

	if (ARD_SEQ_NUMBER > 0U){
        	configValue |= 
				FlexSPI_FLSHCR2_ARDSEQID((uint32_t)ARD_SEQ_INDEX) |
                       		FlexSPI_FLSHCR2_ARDSEQNUM((uint32_t)ARD_SEQ_NUMBER - 1U);
    	}

	base->FLSHCR2[kFlexSPI_PortA1] = configValue;

	/* Configure DLL Value */
	base->DLLCR[kFlexSPI_PortA1] = 0x6F00;	/* set clock delay */

    /* Step into stop mode. */
    base->MCR0 |= FlexSPI_MCR0_MDIS_MASK;
	
    /* Configure write mask. */
    index= 0;//port A
    enableWriteMask = false;
	
    if (enableWriteMask)
    {
        base->FLSHCR4 &= ~FlexSPI_FLSHCR4_WMOPT1_MASK;
    }
    else
    {
        base->FLSHCR4 |= FlexSPI_FLSHCR4_WMOPT1_MASK;
    }

    if (index == 0U) /*PortA*/
    {
        base->FLSHCR4 &= ~FlexSPI_FLSHCR4_WMENA_MASK;
        base->FLSHCR4 |= FlexSPI_FLSHCR4_WMENA(enableWriteMask);
    }
    else
    {
        base->FLSHCR4 &= ~FlexSPI_FLSHCR4_WMENB_MASK;
        base->FLSHCR4 |= FlexSPI_FLSHCR4_WMENB(enableWriteMask);
    }

    /* Exit stop mode. */
    base->MCR0 &= ~FlexSPI_MCR0_MDIS_MASK;

	/* wait till controller is idle */
	do {
		reg = base->STS0;
		if ((reg & FLEXSPI_STS0_ARB_IDLE_MASK) &&
				(reg & FLEXSPI_STS0_SEQ_IDLE_MASK))
			break;
		udelay(1);
	} while (1);
}

static void flexspi_fpga_init(FlexSPI_Type *base)
{
	flexspi_config_t flexspiConfig;

	/* Enable FLEXSPI root clock, that can config the registers */
	flexspi_clock_init(kCLOCK_QspiRootmuxSysPll1Div2, 1, 2);

	/* Get FLEXSPI default settings and configure the flexspi. */
	flexspi_get_default_config(&flexspiConfig);

	flexspiConfig.ahbConfig.enableAHBPrefetch		= true;
	flexspiConfig.ahbConfig.enableAHBBufferable		= true;
	flexspiConfig.ahbConfig.enableReadAddressOpt	= true;
	flexspiConfig.ahbConfig.enableAHBCachable		= true;
	flexspiConfig.ahbConfig.enableClearAHBBufferOpt	= true;
	
	flexspiConfig.enableCombination    = true;
	flexspiConfig.enableSckFreeRunning = true;

	flexspi_init(base, &flexspiConfig);

	/* Configure fpga settings according to serial fpga feature. */
	flexspi_set_fpga_config(base);

	/* UpdateLUT */
	flexspi_update_lut(base, 0, customLUT, ARRAY_SIZE(customLUT));

	/*
	 * Config FLEXSPI sclk frequency
	 * Set FLEXSPI source to 400MHz, set root clock to 400MHZ / 2 / 5 = 40MHZ defualt
	 * In DDR mode, sclk frequency is half of root clock
	 * In SDR mode, sclk frequency is same as root clock
	 */
	flexspi_clock_init(kCLOCK_QspiRootmuxSysPll1Div2, pre_div, post_div);

	/* Do software reset. */
	flexspi_software_reset(base);
}

static int __init flexspi_imx8_init(void)
{
	int ret;

	printk("imx8 flexspi fpga init!\n");

	flexspi_base = ioremap(FLEXSPI_BASE, SZ_64K);
	if(flexspi_base == NULL){
		printk(KERN_WARNING "flexspi_base ioremap failed!\n");
		ret = -EFAULT;
		return ret;
	}

	flexspi_fpga_init((FlexSPI_Type *)flexspi_base);

	printk("imx8 flexspi fpga init done!\n");

	return 0;
}

static void __exit flexspi_imx8_exit(void)
{
	printk("flexspi fpga dirver exit!\n");
	iounmap(flexspi_base);
}

module_init(flexspi_imx8_init);
module_exit(flexspi_imx8_exit);

MODULE_AUTHOR("www.nxp.com");
MODULE_LICENSE("GPL");
MODULE_VERSION("v0.1");
MODULE_DESCRIPTION("IMX8 FlexSPI for FPGA driver");
