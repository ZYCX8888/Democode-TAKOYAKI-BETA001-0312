#!/bin/sh
echo "Generating YUVs..."
cmd="./avc_dec --codec avc_dec -i stream/avc/longterm_8b_05.cfg_0.264 -c -y 0 -o longterm_8b_05.cfg_0.264.yuv"
echo "$cmd"
$cmd

cmd="./avc_dec --codec avc_dec -i stream/avc/akiyo_qcif_dbk.avccfg_9frm.264 -c -y 0 -o akiyo_qcif_dbk.avccfg_9frm.264.yuv"
echo "$cmd"
$cmd

cmd="./hevc_dec -i stream/hevc/one_reference_vrange_48.bsp -c -y 0 -o one_reference_vrange_48.bsp.yuv"
echo "$cmd"
$cmd

cmd="./hevc_dec -i stream/hevc/two_reference_vrange_48.bsp -c -y 0 -o two_reference_vrange_48.bsp.yuv"
echo "$cmd"
$cmd

cmd="./multi_instance_test --multicmd=32_instances.cmd"
echo "$cmd"
$cmd

result="$?"

if [ "$result" == "0" ]; then
    echo "SUCCESS"
else
    echo "FAILURE"
fi
