#!/bin/sh
cmd="./w5_dec_test --codec=0 --disable-wtl --input=stream/avc/longterm_8b_05.cfg_0.264" 
echo "$cmd"
$cmd

result="$?"

if [ "$result" == "0" ]; then
    echo "SUCCESS"
else
    echo "FAILURE"
fi
