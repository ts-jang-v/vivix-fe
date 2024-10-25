FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
SRC_URI += "file://mkfs.sh"
SRC_URI += "file://vivix_start.sh"

do_install:append() {
	install -m 755 ${WORKDIR}/mkfs.sh ${D}${sysconfdir}/init.d
	update-rc.d -r ${D} mkfs.sh start 03 S .
	
	install -m 755 ${WORKDIR}/vivix_start.sh ${D}${sysconfdir}/init.d
	update-rc.d -r ${D} vivix_start.sh defaults 90
}

