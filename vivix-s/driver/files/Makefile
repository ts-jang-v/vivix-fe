obj-m := vivix_gpio.o vivix_spi.o vivix_tg.o vivix_flash.o vivix_ti_afe2256.o vivix_adi_adas1256.o imx_flexspi_fpga.o

vivix_gpio-y := gpio_drv/driver.o
vivix_spi-y := spi_drv/driver.o
vivix_tg-y := fpga_drv/driver.o fpga_drv/dispatch.o \
fpga_drv/api_functions.o fpga_drv/isr_func.o fpga_drv/eim.o
vivix_flash-y := flash_spi_drv/driver.o
vivix_ti_afe2256-y := chip_ti_afe2256_drv/driver.o
vivix_adi_adas1256-y := chip_adi_adas1256_drv/driver.o
imx_flexspi_fpga-y := flex_spi/imx_flexspi_fpga.o 

ccflags-y := -I$(INCLUDEDIR)

SRC := $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC)

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install

clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers
