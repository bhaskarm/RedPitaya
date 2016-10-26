#!/bin/bash
buf_idx=0
args=("$@")
if [ "$#" -gt 0 ]; then
    buf_idx=${args[0]}
fi
echo "Printing the config for waveform generator buffer $buf_idx"
addr=0x40200000
echo "Reset and Config"
/opt/redpitaya/bin/monitor 0x40200000
echo "Amplitude and Scale"
/opt/redpitaya/bin/monitor $(($((buf_idx*0x20)) + 0x40200004))
echo "Buffer end"
/opt/redpitaya/bin/monitor $(($((buf_idx*0x20)) + 0x40200008))
echo "Buffer start"
/opt/redpitaya/bin/monitor $(($((buf_idx*0x20)) + 0x4020000c))
echo "Pointer step"
/opt/redpitaya/bin/monitor $(($((buf_idx*0x20)) + 0x40200010))
echo "Curr Pointer"
/opt/redpitaya/bin/monitor $(($((buf_idx*0x20)) + 0x40200014))
echo "Cycles count"
/opt/redpitaya/bin/monitor $(($((buf_idx*0x20)) + 0x40200018))
