#!/bin/sh
cmd="./avc_dec --codec avc_dec -i stream/avc/akiyo_qcif_dbk.avccfg_9frm.264 -c -y 0 -o output_orig.yuv"
echo "$cmd"
$cmd

cmd="./scaler -i output_orig.yuv -x 176 -y 144 --scl-list 88x88,100x100,136x120 -o scaled.yuv -n 0 -g 0 -h 0"
echo "$cmd"
$cmd

cmd="./w5_dec_test -c 1 --codec=0 --inplace=1 --scl-list=88x88,100x100,136x120 --ref-yuv=scaled.yuv --input=stream/avc/akiyo_qcif_dbk.avccfg_9frm.264" 
echo "$cmd"
$cmd

result="$?"

if [ "$result" == "0" ]; then
    echo "SUCCESS"
else
    echo "FAILURE"
fi
