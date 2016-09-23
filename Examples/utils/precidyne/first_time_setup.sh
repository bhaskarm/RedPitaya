#!/bin/bash

echo "This script will setup the Red Pitaya for use after first boot. It copies libraries to the right location and enables read/write to the filesystem"
rw # enable read/write of the filesystem
cp /opt/redpitaya/lib/librp.so /lib/
systemctl stop wylordin-server
systemctl disable wylordin-server
