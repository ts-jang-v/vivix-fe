FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
SRC_URI += "file://lighttpd.conf"
SRC_URI += "file://modules.conf"
SRC_URI += "file://conf.d"
SRC_URI += "file://vhosts.d"
SRC_URI += "file://cgi-bin"
SRC_URI += "file://html"
SRC_URI += "file://index.html"
SRC_URI += "file://lighttpd"

do_install:append() {

	rm -rf ${D}/www
	rm -rf ${D}${sysconfdir}/lighttpd.d
	rm -f ${D}/www/logs
	rm -f ${D}/www/var

	install -d ${D}${sysconfdir}/lighttpd/conf.d
	install -d ${D}${sysconfdir}/lighttpd/vhosts.d
	
	install -m 0644 ${WORKDIR}/modules.conf ${D}${sysconfdir}/lighttpd
	install -m 0644 ${WORKDIR}/conf.d/* ${D}${sysconfdir}/lighttpd/conf.d
	install -m 0644 ${WORKDIR}/vhosts.d/* ${D}${sysconfdir}/lighttpd/vhosts.d

	install -d  ${D}/var/www/cgi-bin
	install -m 0644 ${WORKDIR}/cgi-bin/* ${D}/var/www/cgi-bin
	
	install -d  ${D}/var/www/html
	install -m 0644 ${WORKDIR}/html/* ${D}/var/www/html

	install -d ${D}/var/www/icons
	
	install -d ${D}/var/www/lighttpd
	install -m 0644 ${WORKDIR}/index.html ${D}/var/www/lighttpd
	
	install -d ${D}/var/www/log

	install -d ${D}/var/lib/lighttpd
#install -d ${D}/var/log/lighttpd
}

FILES:${PN} += "/var"
