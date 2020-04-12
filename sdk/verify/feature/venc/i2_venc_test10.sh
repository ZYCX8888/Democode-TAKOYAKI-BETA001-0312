#!/bin/sh

. _test.sh

script=$(basename "$0")
script="${script%.*}"

#debug_level 3 would cause much delay and causing MI gets every interrupts.
#use debug_level 0 to make
echo 2 > $dbg_level #/sys/module/mi_venc/parameters/debug_level

log "\n\n${script}: Static  Channel Attribute: Crop\n"
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

codec=h264
of=enc_d2c00.es
to=/mnt/out/es
unset_script=./_unset_config.sh

if [ ! -d /mnt/out/ ]; then mkdir /mnt/out/; fi
if [ ! -d /mnt/out/es ]; then mkdir /mnt/out/es; fi

#==== settings ====

if true; then
export VENC_Crop=1
export VENC_Crop_X=256
export VENC_Crop_Y=16
export VENC_Crop_W=96
export VENC_Crop_H=64
${exe} 1 ${codec};
test "Check any output file exists ${out_path}/${of}" `expr " [ -s ${out_path}/${of} ] "`
md5=`md5sum ${out_path}/${of} | cut -c-32`
#echo $md5
test "Check ${out_path}/${of} md5" `expr "${md5}" == "ba5d433fca2f392883f6c1e76b9ffa27"`
#do not unset variable, H.264 and H.265 are using the same variable
mv ${out_path}/${of} ${to}/crop_1.${codec}; ${unset_script}
fi

codec=h265
of=enc_d0c00.es

if true; then
export VENC_Crop=1
export VENC_Crop_X=16
export VENC_Crop_Y=16
export VENC_Crop_W=320
export VENC_Crop_H=256
${exe} 1 ${codec};
test "Check any output file exists ${out_path}/${of}" `expr " [ -s ${out_path}/${of} ] "`
md5=`md5sum ${out_path}/${of} | cut -c-32`
#echo $md5
test "Check ${out_path}/${of} md5" `expr "${md5}" == "72a22a0d0d469173c0b80f5ef1e9f4c9"`
mv ${out_path}/${of} ${to}/crop_1.${codec}; . ${unset_script}
fi


fi

print_result
