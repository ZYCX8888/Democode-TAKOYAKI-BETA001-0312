#!/bin/sh
cmd="./avc_dec --codec avc_dec -i stream/avc/longterm_8b_05.cfg_0.264 -c -y 0 -o output_orig.yuv"
echo "$cmd"
$cmd

cmd="./scaler -i output_orig.yuv -x 416 -y 240 --scl-list 88x88,100x100,136x120 -o scaled.yuv -n 0 -g 0 -h 0"
echo "$cmd"
$cmd

cmd="./w5_dec_test -c 1 --codec=0 --inplace=2 --scl-list=88x88,100x100,136x120 --ref-yuv=scaled.yuv --input=stream/avc/longterm_8b_05.cfg_0.264" 
echo "$cmd"
$cmd

result="$?"

if [ "$result" == "0" ]; then
    echo "SUCCESS"
else
    echo "FAILURE"
fi
