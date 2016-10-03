#!/bin/bash

echo "This script will setup the Red Pitaya for use after first boot. It copies libraries to the right location and enables read/write to the filesystem"
rw # enable read/write of the filesystem
cp /opt/redpitaya/lib/librp.so /lib/
systemctl stop redpitaya_nginx
systemctl stop redpitaya_wyliodrin
systemctl disable redpitaya_nginx
systemctl disable redpitaya_wyliodrin
systemctl enable redpitaya_scpi
systemctl start  redpitaya_scpi
