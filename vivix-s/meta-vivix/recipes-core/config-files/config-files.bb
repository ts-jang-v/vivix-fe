LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://fw_env.config"

do_install() {
	install -d 0755 ${D}/vivix
	install -d 0755 ${D}${sysconfdir}
	install -m 0755 ${WORKDIR}/fw_env.config ${D}${sysconfdir}
}

FILES:${PN} = "/etc/fw_env.config"
FILES:${PN} += "/vivix"
