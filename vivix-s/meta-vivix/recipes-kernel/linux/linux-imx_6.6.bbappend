FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
SRC_URI += "file://spi.patch"
SRC_URI += "file://mipi.patch"
SRC_URI += "file://imx8mm-vivix;subdir=git/arch/arm64/configs"
SRC_URI += "file://imx8mm-vivix.dtsi;subdir=git/arch/arm64/boot/dts/freescale"
SRC_URI += "file://imx8mm-vivix.dts;subdir=git/arch/arm64/boot/dts/freescale"
#SRC_URI += "file://efinix_fpga_mipi_spi.c;subdir=git/drivers/media/platform/mxc/capture"
SRC_URI += "file://efinix_fpga_mipi.c;subdir=git/drivers/media/platform/mxc/capture"

INSTALL_DIR = "${TOPDIR}/../vivix-s/package/output/driver"

KBUILD_DEFCONFIG ?= ""
KBUILD_DEFCONFIG:imx8mm-vivix = "imx8mm-vivix"

do_copy_defconfig() {
	install -d ${WORKDIR}/build
	mkdir -p ${WORKDIR}/build
	cp ${WORKDIR}/git/arch/arm64/configs/imx8mm-vivix ${WORKDIR}/build/.config
}

do_install:append() {
	cp -f ${WORKDIR}/build/drivers/soc/qcom/qmi_helpers.ko ${INSTALL_DIR}
	cp -f ${WORKDIR}/build/drivers/net/wireless/ath/ath.ko ${INSTALL_DIR}
	cp -f ${WORKDIR}/build/drivers/net/wireless/ath/ath11k/ath11k.ko ${INSTALL_DIR}
	cp -f ${WORKDIR}/build/drivers/net/wireless/ath/ath11k/ath11k_pci.ko ${INSTALL_DIR}
}
