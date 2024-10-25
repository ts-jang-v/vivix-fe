#!/bin/sh
EMMC_P1_DEV=/dev/mmcblk2p1
FS_CHECK=`blkid "$EMMC_P1_DEV" | grep ext4`
MMCP2DIR=/mnt/mmc/p2
MMCP3DIR=/mnt/mmc/p3

if [ "$1" = "stop" ]; then
	exit 0
fi

if [ ! -d "$MMCP2DIR" ]; then
        mkdir -p $MMCP2DIR
fi

if [ ! -d "$MMCP3DIR" ]; then
        mkdir -p $MMCP3DIR
fi

if [ -z "$FS_CHECK" ]; then
        mkfs.ext4 $EMMC_P1_DEV -E nodiscard > /dev/null
fi

mount -t ext4 $EMMC_P1_DEV /vivix


