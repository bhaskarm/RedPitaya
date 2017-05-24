#!/bin/bash

echo "This script will add waveform generator and switch monitor service"
cp waveform-gen.service /lib/systemd/system
systemctl enable waveform-gen.service
systemctl start  waveform-gen.service
