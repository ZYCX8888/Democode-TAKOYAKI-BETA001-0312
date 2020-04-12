#!/bin/sh

ifconfig eth0 172.19.24.241

## libcnrt的调试信息
## export ENABLE_CNRT_PRINT_INFO=true

## CSpotter的license日期
date -s "2017-12-06 20:49"

## onvif正常使用
route add -net 239.255.255.250 netmask 255.255.255.255 eth0

## 调试core文件
ulimit -c unlimited
echo $PWD"/core-%e-%p-%t" > /proc/sys/kernel/core_pattern

export LD_LIBRARY_PATH=/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$PWD:$LD_LIBRARY_PATH

cd $PWD;
./gdb sample_rtsp_all
cd -;
