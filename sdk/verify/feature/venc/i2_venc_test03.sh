#!/bin/sh

. _test.sh

script=$(basename "$0")
script="${script%.*}"

#debug_level 3 would cause much delay and causing MI gets every interrupts.
#use debug_level 0 to make
echo 0 > $dbg_level #/sys/module/mi_venc/parameters/debug_level

log "\n\n${script}: Encoder performance\n"

#${exe} 1 dummy
#ret=$?
#test "The program returns ${ret}." `expr "${ret}" == "0"`


#the rest tests the CONFIG system
if true ; then
export VENC_GLOB_IN_FILE="/tmp/YUV420SP_640_480.yuv"
export VENC_GLOB_IN_W=640
export VENC_GLOB_IN_H=480

#export VENC_GLOB_IN_FILE="/tmp/YUV420SP_720_480.yuv"
#export VENC_GLOB_IN_W=736
#export VENC_GLOB_IN_H=480

#export VENC_GLOB_IN_FILE="/tmp/YUV420SP_1920_1088.yuv"
#export VENC_GLOB_IN_W=1920
#export VENC_GLOB_IN_H=1088

export VENC_GLOB_MAX_FRAMES=300
export VENC_GLOB_DUMP_ES=0
export VENC_GLOB_IN_MSG_LEVEL=0
export VENC_CH00_QP=45
export VENC_GLOB_AUTO_STOP=1
export VENC_GLOB_IN_FPS=1000 #asap
export VENC_GLOB_IN_FILE_CACHE_MB=30 #0: for non cache. 30 for FHD 10 frames.

rm $result
${exe}  1 h264
ret=$?
test "The program returns ${ret}." `expr "${ret}" == "0"`

#H.264 174 mpix h.265 138 mpix.
eval `cat ${result}`
test "${FPSx100} FPSx100 meets requirement: " `expr "${FPSx100}" \>= "7500"`

#device2.ch00.ElementaryStream
#result=`md5sum $out_path/enc2.00.es | cut -c-32`
#test "Check enc2.00.es" `expr "${result}" == "4c31debc017d23b2ec80e4868ec9f4b4"`

#result=`md5sum $out_path/enc2.01.es | cut -c-32`
#test "Check enc2.01.es" `expr "${result}" == "06146b0e937279e05fdfc4e80c16d3de"`
fi

print_result
