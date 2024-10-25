#!/bin/sh

OUTPUT_DRV_DIR=$(pwd)/output/driver
OUTPUT_APP_DIR=$(pwd)/output/app

MODULE_FILES="vivix_adi_adas1256.ko vivix_flash.ko vivix_gpio.ko \
vivix_spi.ko vivix_tg.ko vivix_ti_afe2256.ko"
APP_FILES="vivix_detector logd"

echo "step 1. copy binary, driver"
for var in $MODULE_FILES
do
	if ! [ -f $OUTPUT_DRV_DIR/$var ]; then
		echo "$var not exist"
		exit
	fi
done

for var in $APP_FILES
do
	if ! [ -f $OUTPUT_APP_DIR/$var ]; then
		echo "$var not exist"
		exit
	fi
done

if [ -d app ]; then
	rm -rf app
fi
mkdir app
cp -f script/* app/
cp -ar $OUTPUT_APP_DIR/* app/
cp -ar $OUTPUT_DRV_DIR/* app/
mkdir app/www
cp -ar output/www/* app/www/
cd app
tar zcvf www.tar.gz www
cd ..
rm -rf app/www

echo "step 2. set package environment"
chown $USER:$USER ./ -R
chmod +x app/*.sh
dos2unix app/*.sh
dos2unix app/*.xml

echo "step 3. copy org image"
cp -f app_image.gz.org app_image.gz

echo "step 4. decompress image"
gunzip app_image.gz

echo "step 5. mount image"
if [ -d app_mount ]; then
	rm -rf app_mount
fi
mkdir app_mount
sudo mount -t ext4 -o loop app_image app_mount/

echo "step 6. remove all files"
sudo rm -rf app_mount/*

echo "step 7. copy to image"
sudo cp -ar app/* app_mount/

sleep 1
echo "step 8. umount image"
sudo umount app_mount/
rmdir app_mount

echo "step 9. compress the image"
gzip -9 app_image
echo "done"

