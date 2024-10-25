/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2019 NXP
 */

#ifndef __IMX8MM_VIVIX_H
#define __IMX8MM_VIVIX_H

#include <linux/sizes.h>
#include <linux/stringify.h>
#include <asm/arch/imx-regs.h>
#include "imx_env.h"

#define UBOOT_ITB_OFFSET			0x57C00
#define FSPI_CONF_BLOCK_SIZE		0x1000
#define UBOOT_ITB_OFFSET_FSPI  \
	(UBOOT_ITB_OFFSET + FSPI_CONF_BLOCK_SIZE)
#ifdef CONFIG_FSPI_CONF_HEADER
#define CFG_SYS_UBOOT_BASE  \
	(QSPI0_AMBA_BASE + UBOOT_ITB_OFFSET_FSPI)
#else
#define CFG_SYS_UBOOT_BASE	\
	(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)
#endif

#ifdef CONFIG_SPL_BUILD
/* malloc f used before GD_FLG_FULL_MALLOC_INIT set */
#define CFG_MALLOC_F_ADDR		0x930000
/* For RAW image gives a error info not panic */

#endif

#define PHY_ANEG_TIMEOUT 20000

#ifdef CONFIG_DISTRO_DEFAULTS
#define BOOT_TARGET_DEVICES(func) \
	func(USB, usb, 0) \
	func(MMC, mmc, 1) \
	func(MMC, mmc, 2)

#include <config_distro_bootcmd.h>
#else
#define BOOTENV
#endif

/* TO_DO emmc num update */
#define CFG_MFG_ENV_SETTINGS \
	CFG_MFG_ENV_SETTINGS_DEFAULT \
	"initrd_addr=0x43800000\0" \
	"initrd_high=0xffffffffffffffff\0" \
	"emmc_dev=2\0"\
	"sd_dev=1\0" \

/* Initial environment variables */
#define CFG_EXTRA_ENV_SETTINGS		\
	CFG_MFG_ENV_SETTINGS \
	BOOTENV \
	"bootcmd=run bsp_bootcmd\0" \
	"prepare_mcore=setenv mcore_clk clk-imx8mm.mcore_booted;\0" \
	"kernel_addr_r=" __stringify(CONFIG_SYS_LOAD_ADDR) "\0" \
	"image=Image\0" \
	"console=ttymxc1,115200\0" \
	"fdt_addr_r=0x43000000\0"			\
	"fdt_addr=0x43000000\0"			\
	"fdt_high=0xffffffffffffffff\0"		\
	"boot_fit=no\0" \
	"fdtfile=" CONFIG_DEFAULT_FDT_FILE "\0" \
	"bootm_size=0x10000000\0" \
	"mmcdev="__stringify(CONFIG_SYS_MMC_ENV_DEV)"\0" \
	"sd_copyprat=2\0" \
	"sd_copypath=boot\0" \
	"mmcpart=1\0" \
	"emmcpart=0\0" \
	"emmc_ospart=0\0" \
	"emmc_p1_start=85M\0" \
	"emmc_p1_size=1G\0" \
	"emmc_p2_size=28G\0" \
	"emmc_p3_size=-\0" \
	"emmc_partitions=name=p1,size=${emmc_p1_size},start=${emmc_p1_start};" \
		"name=p2,size=${emmc_p2_size};" \
		"name=p3,size=${emmc_p3_size};\0" \
	"emmc_set_partition=gpt write mmc 2 ${emmc_partitions}\0" \
	"emmc_kernel_offset=0x800\0" \
	"emmc_kernel_size=0x14000\0" \
	"load_kernel_from_sd=ext4load mmc ${sd_dev}:${sd_copyprat} ${loadaddr} ${sd_copypath}/${image};\0" \
	"copy_kernel_from_sd=mmc dev ${emmc_dev} ${emmc_ospart}; " \
		"mmc write ${loadaddr} ${emmc_kernel_offset} ${emmc_kernel_size}; " \
		"echo Update kernel;\0" \
	"emmc_ramdisk_offset=0x14800\0" \
	"emmc_ramdisk_size=0x14000\0" \
	"Ramdisk=ramdisk\0" \
	"load_ramdisk_from_sd=ext4load mmc ${sd_dev}:${sd_copyprat} ${initrd_addr} ${sd_copypath}/${Ramdisk};\0" \
	"copy_ramdisk_from_sd=mmc dev ${emmc_dev} ${emmc_ospart}; " \
	"mmc write ${initrd_addr} ${emmc_ramdisk_offset} ${emmc_ramdisk_size}; " \
		"echo Update ramdisk\0" \
	"emmc_fdt_offset=0x28800\0" \
	"emmc_fdt_size=0x400\0" \
	"load_fdt_from_sd=ext4load mmc ${sd_dev}:${sd_copyprat} ${fdt_addr} ${sd_copypath}/${fdtfile};\0" \
	"copy_fdt_from_sd=mmc dev ${emmc_dev} ${emmc_ospart}; " \
		"mmc write ${fdt_addr} ${emmc_fdt_offset} ${emmc_fdt_size}; " \
		"echo Update device tree\0" \
	"emmc_bootpart=1\0" \
	"emmc_boot_offset=0x42\0" \
	"emmc_boot_size=0x4000\0" \
	"boot_addr=0x80000000\0" \
	"bootloader=imx-boot\0" \
	"load_bootloader_from_sd=ext4load mmc ${sd_dev}:${sd_copyprat} ${boot_addr} ${sd_copypath}/${bootloader};\0" \
	"copy_bootloader_from_sd=mmc dev ${emmc_dev} ${emmc_bootpart}; " \
		"mmc write 0x80000000 ${emmc_boot_offset} ${emmc_boot_size}; " \
		"echo Update bootloader\0" \
	"mmcautodetect=yes\0" \
	"emmcargs=setenv bootargs root=/dev/ram0 rootwait rw rootfstype=ramfs console=${console} init=/sbin/init " \
		"maxcpus=2 console=${console} earlycon net.ifnames=0\0" \
	"load_kernel_from_emmc=mmc dev ${emmc_dev} ${emmc_ospart}; " \
		"mmc read ${loadaddr} ${emmc_kernel_offset} ${emmc_kernel_size};\0" \
	"load_ramdisk_from_emmc=mmc dev ${emmc_dev} ${emmc_ospart}; " \
		"mmc read ${initrd_addr} ${emmc_ramdisk_offset} ${emmc_ramdisk_size};\0" \
	"load_fdt_from_emmc=mmc dev ${emmc_dev} ${emmc_ospart}; " \
		"mmc read ${fdt_addr} ${emmc_fdt_offset} ${emmc_fdt_size};\0" \
	"copy_all_from_sd=run emmc_set_partition; " \
		"run load_bootloader_from_sd; " \
		"run copy_bootloader_from_sd; " \
		"run load_kernel_from_sd; " \
		"run copy_kernel_from_sd; " \
		"run load_ramdisk_from_sd; " \
		"run copy_ramdisk_from_sd; " \
		"run load_fdt_from_sd; " \
		"run copy_fdt_from_sd;\0" \
	"boot_from_emmc=run load_kernel_from_emmc; run load_ramdisk_from_emmc; " \
		"run load_fdt_from_emmc; " \
		"run emmcargs; " \
		"booti ${loadaddr} ${initrd_addr} ${fdt_addr};\0" \
	"bsp_bootcmd=echo Running BSP bootcmd ...; " \
	"if run boot_from_emmc; then " \
		"echo boot from eMMC;" \
	"else " \
		"echo fail boot from eMMC; echo Start Copy From SD; " \
		"if run copy_all_from_sd; then " \
			"run boot_from_emmc; " \
		"else " \
			"echo fail copy from sd; " \
		"fi; " \
	"fi;"

/* Link Definitions */

#define CFG_SYS_INIT_RAM_ADDR        0x40000000
#define CFG_SYS_INIT_RAM_SIZE        0x200000

#define CFG_SYS_SDRAM_BASE           0x40000000
#define PHYS_SDRAM                      0x40000000
#define PHYS_SDRAM_SIZE			0x80000000 /* 2GB DDR */

#define CFG_FEC_MXC_PHYADDR          0

#define CFG_MXC_UART_BASE		UART_BASE_ADDR(2)

#ifdef CONFIG_TARGET_IMX8MM_DDR4_EVK
#define CFG_SYS_FSL_USDHC_NUM	1
#else
#define CFG_SYS_FSL_USDHC_NUM	2
#endif
#define CFG_SYS_FSL_ESDHC_ADDR	0

#define CFG_SYS_NAND_BASE           0x20000000

#ifdef CONFIG_IMX_MATTER_TRUSTY
#define NS_ARCH_ARM64 1
#endif

#ifdef CONFIG_ANDROID_SUPPORT
#include "imx8mm_evk_android.h"
#endif

#endif
