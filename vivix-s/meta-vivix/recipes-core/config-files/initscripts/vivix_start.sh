#!/bin/sh
#if [ $USER != "root" ]
#then
#exit 0
#fi

if [ "$1" = "stop" ]; then
	exit 0
fi

if [ -b /dev/mmcblk1p2 ] && [ ! -e /vivix/logd ]; then
        echo "############### start Install Package to eMMC"
        mkdir /tmp/mnt
        mount /dev/mmcblk1p2 /tmp/mnt 
        if [ -e /tmp/mnt/boot/app_image.gz ]; then
                cp /tmp/mnt/boot/app_image.gz /tmp
                gunzip /tmp/app_image.gz
                mkdir /tmp/app_mount
                mount -t ext4 -o loop /tmp/app_image /tmp/app_mount/
                cp -ar /tmp/app_mount/* /vivix
                sync
		else
                echo "############## #install fail!!!"
        fi
        umount /tmp/mnt
        echo "############### done"
		exit 0
fi

cp -ar /vivix/* /run/
#/run/start.sh


