#!/bin/sh
cmd="./hevc_dec -i stream/hevc/one_reference_vrange_48.bsp -c -y 0 -o output_orig.yuv"
echo "$cmd"
$cmd

cmd="./scaler -i output_orig.yuv -x 1280 -y 736 --scl-list 320x240,640x480,800x600 -o scaled.yuv -n 0 -g 0 -h 0"
echo "$cmd"
$cmd

cmd="./w5_dec_test -c 1 --codec=12 --inplace=1 --scl-list=320x240,640x480,800x600 --ref-yuv=scaled.yuv --input=stream/hevc/one_reference_vrange_48.bsp" 
echo "$cmd"
$cmd

result="$?"

if [ "$result" == "0" ]; then
    echo "SUCCESS"
else
    echo "FAILURE"
fi
