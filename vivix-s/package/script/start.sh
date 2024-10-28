#!/bin/sh
run=$(ps | grep 'vivix_' | grep -v grep | wc -l)
if [ $run = "1" ]
then
        exit 1
fi


bd_type=$(fw_printenv -n bd_type)

cd /run

insmod imx_flexspi_fpga.ko 

insmod qmi_helpers.ko
insmod ath.ko
insmod ath11k.ko
insmod ath11k_pci.ko

#mknod /dev/vivix_gpio c 246 0
#mknod /dev/vivix_spi c  247 0

insmod vivix_gpio.ko
insmod vivix_spi.ko

./logd & 

echo "Detector firmware start..."
#mknod /dev/vivix_flash c 250 0
#mknod /dev/vivix_tg c 248 0

insmod vivix_flash.ko
insmod vivix_tg.ko

#mknod /dev/vivix_ti_afe2256 c   240 0			#ti roic
#mknod /dev/vivix_adi_adas1256 c 241 0 			#adi roic

#insmod vivix_ti_afe2256.ko
#insmod vivix_adi_adas1256.ko

echo "/tmp/core" > /proc/sys/kernel/core_pattern
chmod +x vivix_detector
./vivix_detector
corelog='/mnt/mmc/p3/log/'$(date '+%Y_%m_%d')'.log'
echo "---------------------core dump start---------------------" >> $corelog
date >> $corelog
gdb /run/vivix_detector /tmp/core -batch -ex "bt" >> $corelog
echo "---------------------core dump end-----------------------" >> $corelog
sync



