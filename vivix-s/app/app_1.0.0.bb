UMMARY = "Hello World Program"

# License information 
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://app \
		   file://web \
		   file://target.mak \
		   file://Makefile \
		  "

S = "${WORKDIR}"

APP_INSTALL_DIR = "${TOPDIR}/../vivix-s/package/output/app"
WEB_INSTALL_DIR = "${TOPDIR}/../vivix-s/package/output/www"

APP_BIN_DIR = "${WORKDIR}/app/bin"
WWW_DIR = "${WORKDIR}/web/www_dir"

EXTRA_OEMAKE += "MAK_DIR=${WORKDIR}"
EXTRA_OEMAKE += "SYS_INC_DIR=${WORKDIR}/app/include"
EXTRA_OEMAKE += "BIN_DIR=${WORKDIR}/app/bin"
EXTRA_OEMAKE += "LIB_DIR=${WORKDIR}/app/lib"

do_configure() {
	cp -f ${TOPDIR}/../vivix-s/driver/files/include/vworks_ioctl.h ${S}/app/include
}

do_compile() {
    oe_runmake
}

do_install() {
	#install -m 0755 ${APP_BIN_DIR}/vivix_detector ${APP_INSTALL_DIR} 
	#install -m 0755 ${APP_BIN_DIR}/logd ${APP_INSTALL_DIR}
	#install -m 0755 ${WWW_DIR}/* ${WEB_INSTALL_DIR}
	cp -f ${APP_BIN_DIR}/* ${APP_INSTALL_DIR} 
	chmod +x ${APP_INSTALL_DIR}/* 
	cp -arf ${WWW_DIR}/* ${WEB_INSTALL_DIR}
}
