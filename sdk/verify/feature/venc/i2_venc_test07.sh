#!/bin/sh

. _test.sh

script=$(basename "$0")
script="${script%.*}"

#debug_level 3 would cause much delay and causing MI gets every interrupts.
#use debug_level 0 to make
echo 2 > $dbg_level #/sys/module/mi_venc/parameters/debug_level

log "\n\n${script}: Static  Channel Attribute: Vui\n"
#if this passed, then run the next case to generate more stream


#the rest tests the CONFIG system
if true ; then
#export VENC_GLOB_IN_FILE="/tmp/YUV420SP_720_480.yuv"
#export VENC_GLOB_IN_W=736
#export VENC_GLOB_IN_H=480
#export VENC_GLOB_IN_FILE="/tmp/YUV420SP_1920_1088.yuv"
#export VENC_GLOB_IN_W=1920
#export VENC_GLOB_IN_H=1088

#don't set input file, use default value
#export VENC_GLOB_IN_FILE=""
#export VENC_GLOB_IN_W=352
#export VENC_GLOB_IN_H=288
export VENC_GLOB_MAX_FRAMES=5
export VENC_GLOB_DUMP_ES=5
export VENC_GLOB_IN_MSG_LEVEL=0
export VENC_CH00_QP=25
export VENC_GLOB_AUTO_STOP=1
export VENC_GLOB_IN_FPS=30 #1000: asap
export VENC_GLOB_IN_FILE_CACHE_MB=30 #0: for non cache. 30 for FHD 10 frames.
export VENC_GLOB_APPLY_CFG=1

codec=h264
of=enc_d2c00.es
to=/mnt/out/es
unset_script=./_unset_config.sh

if [ ! -d /mnt/out/ ]; then mkdir /mnt/out/; fi
if [ ! -d /mnt/out/es ]; then mkdir /mnt/out/es; fi

#==== settings ====

if true; then
export VENC_GLOB_VUI=1
${exe} 1 ${codec};
test "Check any output file exists ${out_path}/${of}" `expr " [ -s ${out_path}/${of} ] "`
md5=`md5sum ${out_path}/${of} | cut -c-32`
#echo $md5
test "Check ${out_path}/${of} md5" `expr "${md5}" == "67a58b953833df8833db919cf3d25720"`
mv ${out_path}/${of} ${to}/vui_1.${codec}; . ${unset_script}
fi

codec=h265
of=enc_d0c00.es

if true; then
export VENC_GLOB_VUI=1
${exe} 1 ${codec};
test "Check any output file exists ${out_path}/${of}" `expr " [ -s ${out_path}/${of} ] "`
md5=`md5sum ${out_path}/${of} | cut -c-32`
#echo $md5
test "Check ${out_path}/${of} md5" `expr "${md5}" == "fc54dc8566fb06057706d4b37c01832b"`
mv ${out_path}/${of} ${to}/vui_1.${codec}; . ${unset_script}
#${exe}  1 ${codec}; mv ${out_path}/${of} ${to}/enable_idr_1.${codec}; . ${unset_script}
fi


fi

print_result
