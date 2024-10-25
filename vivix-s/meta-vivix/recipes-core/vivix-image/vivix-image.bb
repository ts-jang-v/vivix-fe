SUMMARY = "A console-only image that fully supports the target device \
hardware."

LICENSE = "MIT"

MACHINE_FEATURES = ""

IMAGE_FEATURES = ""

DISTRO_FEATURES = "usrmerge sysvinit"


DISTRO_FEATURES_BACKFILL_CONSIDERED = "systemd"
INIT_MANAGER = "sysvinit"
VIRTUAL-RUNTIME_init_manager = "sysvinit"
VIRTUAL-RUNTIME_initscripts = "initscripts"
VIRTUAL-RUNTIME_login_manager = "busybox"
VIRTUAL-RUNTIME_dev_manager = "udev"
PREFERRED_PROVIDER_udev = "sysvinit"
PREFERRED_PROVIDER_udev-utils = "sysvinit"


PACKAGE_INSTALL = "${VIRTUAL-RUNTIME_base-utils} sysvinit udev base-passwd busybox-syslog initscripts"
PACKAGE_INSTALL += "firmware-imx-sdma-imx7d firmware-nxp-wifi-nxp-common firmware-nxp-wifi-nxp8987-sdio"
PACKAGE_INSTALL += "gdb tcpdump vsftpd e2fsprogs-mke2fs libgpiod-tools u-boot-fw-utils"
PACKAGE_INSTALL += "lighttpd lighttpd-module-cgi lighttpd-module-h2 iw wireless-regdb-static"
PACKAGE_INSTALL += "e2fsprogs-e2fsck wireless-tools uci ethtool dropbear init-ifupdown"
PACKAGE_INSTALL += "v4l-utils mtd-utils config-files"

PACKAGE_EXCLUDE = "kernel-image-*"

inherit core-image



