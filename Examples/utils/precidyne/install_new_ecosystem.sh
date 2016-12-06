#!/bin/bash

echo "This script will load a new ecosystem zip file into the Redpitaya filesystem"
echo "Enabling Write access to SD card"
mount -o rw,remount /opt/redpitaya
echo "Unzipping new ecosystem"
unzip -oq /root/ecosystem-0.94-0-devbuild.zip -d /opt/redpitaya
cp /opt/redpitaya/lib/librp.so /lib
echo "Reloading FPGA bitfile"
cat /opt/redpitaya/fpga/fpga.bit > /dev/xdevcfg
mount -o ro,remount /opt/redpitaya
