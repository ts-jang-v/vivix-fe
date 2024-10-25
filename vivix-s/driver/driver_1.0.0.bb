SUMMARY = "Example of how to build an external Linux kernel module"
DESCRIPTION = "${SUMMARY}"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=12f884d2ae1ff87c09e5b7ccc2c4ca7e"

inherit module

SRC_URI = "file://gpio_drv \
           file://spi_drv \
           file://fpga_drv \
           file://chip_adi_adas1256_drv \
           file://chip_ti_afe2256_drv \
           file://flash_spi_drv \
		   file://flex_spi \
           file://include \
           file://Makefile \
           file://COPYING \
          "

S = "${WORKDIR}"

INSTALL_DIR = "${TOPDIR}/../vivix-s/package/output/driver"

EXTRA_OEMAKE += "INCLUDEDIR=${WORKDIR}/include"

# The inherit of module.bbclass will automatically name module packages with
# "kernel-module-" prefix as required by the oe-core build environment.

do_configure() {
	cp ${S}/include/vworks_ioctl.h ${TMPDIR}
}

do_install() {
	#install -m 0755 vivix_gpio.ko ${INSTALL_DIR}
	#install -m 0755 vivix_spi.ko ${INSTALL_DIR}
	#install -m 0755 vivix_tg.ko ${INSTALL_DIR}
	#install -m 0755 vivix_flash.ko ${INSTALL_DIR}
	#install -m 0755 vivix_ti_afe2256.ko ${INSTALL_DIR}
	#install -m 0755 vivix_adi_adas1256.ko ${INSTALL_DIR}
	cp -f ${S}/*.ko ${INSTALL_DIR}
	chmod +x ${INSTALL_DIR}/*.ko
}

RPROVIDES:${PN} += "kernel-module-driver"
