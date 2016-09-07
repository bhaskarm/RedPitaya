#!/bin/bash

echo "This script will write an SD card imagee to the SD card device"

read -p "Enter the name of SD card image file [debian.img] : " img_file
img_file=${img_file:-debian.img}
read -p "Enter the SD card device [/dev/mmcblk0] : " sd_dev
sd_dev=${sd_dev:-/dev/mmcblk0}
echo "Unmounting $sd_dev";
sudo umount $sd_dev?*
echo "Writing $img_file to $sd_dev"
sudo dd if=$img_file of=$sd_dev bs=4M status=progress
echo "Script finished"
