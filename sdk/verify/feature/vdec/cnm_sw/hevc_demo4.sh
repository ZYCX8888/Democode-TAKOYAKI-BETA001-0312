#!/bin/sh
cmd="./w5_dec_test --codec=12 --disable-wtl --input=stream/hevc/two_reference_vrange_48.bsp" 
echo "$cmd"
$cmd

result="$?"

if [ "$result" == "0" ]; then
    echo "SUCCESS"
else
    echo "FAILURE"
fi
