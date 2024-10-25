#!/bin/bash

PWD=$(pwd)
PACKAGE_DIR=$PWD/vivix-s/package
IMAGE_DIR=$PWD/build/tmp/deploy/images/*
IMAGE_COPY_IDR=$PWD/vivix-s/image
BOOLOADER=imx-boot-imx8mm-vivix-sd.bin-flash_evk
BOOLOADER_DST=imx-boot
RAMDISK=vivix-image-imx8mm-vivix.rootfs.cpio.gz
KERNEL=Image
DT=imx8mm-vivix.dtb

printf_exmple()
{
	echo ""
	echo "==============================================="
	echo "build  example"
	echo "==============================================="
	echo "#image build (kernel, ramdisk, dts, bootloader)"
	echo "./make.sh image"
	echo ""
	echo "#package build (app, driver)"
	echo "./make.sh package"
	echo ""
	exit 
}


ARG_NUM=$#

case $ARG_NUM in
	1)
		CMD=$1
		if [ $CMD = "image" ] || [ $CMD = "package" ]; 
		then
			source setup-environment build > /dev/null
		else
			printf_exmple
		fi
		;;
	*)
		printf_exmple
		;;
esac

if [ $CMD = "image" ]; 
then
	echo "start image build"
	
	bitbake vivix-image

	if [ ! -e $IMAGE_DIR/$BOOLOADER ];
	then
		bitbake imx-boot -c cleanall
		bitbake imx-boot
	fi

	cp -f $IMAGE_DIR/$BOOLOADER $IMAGE_COPY_IDR/$BOOLOADER_DST
	cp -f $IMAGE_DIR/$KERNEL $IMAGE_COPY_IDR
	cp -f $IMAGE_DIR/$DT $IMAGE_COPY_IDR
	cp -f $IMAGE_DIR/$RAMDISK $IMAGE_COPY_IDR
	cd $IMAGE_COPY_IDR
	mkimage -A arm64 -O linux -T ramdisk -C gzip -n "i.mx8 ramfs" -d vivix-image-imx8mm-vivix.rootfs.cpio.gz ramdisk > /dev/null
	rm $RAMDISK
	echo "done"
	echo "#image output directory"
	echo "$IMAGE_COPY_IDR"
elif [ $CMD = "package" ];
then
	echo "start package build"
	bitbake driver
	bitbake app
	cd $PACKAGE_DIR
	./package.sh
	echo "#package output directory"
	echo "$PACKAGE_DIR"
fi


