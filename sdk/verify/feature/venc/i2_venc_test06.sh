#!/bin/sh

. _test.sh

script=$(basename "$0")
script="${script%.*}"

#debug_level 3 would cause much delay and causing MI gets every interrupts.
#use debug_level 0 to make
echo 2 > $dbg_level #/sys/module/mi_venc/parameters/debug_level

log "\n\n${script}: Dynamic Channel Attribute: EnableIdr\n"
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
export VENC_GLOB_MAX_FRAMES=100
export VENC_GLOB_DUMP_ES=100
export VENC_GLOB_IN_MSG_LEVEL=0
export VENC_CH00_QP=45
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
export EnableIdr=0
export EnableIdrFrame=20
export VENC_CH00_QP=35
${exe} 1 ${codec};
test "Check any output file exists ${out_path}/${of}" `expr " [ -s ${out_path}/${of} ] "`
md5=`md5sum ${out_path}/${of} | cut -c-32`
#enableIdr=0,EnablrIdrFrame=20, FixQp35 case. Note that because it's asynchronous.
#the md5 might not be fixed each time it runs. "898ee640b64a41ab9ad536911a0b817b" was seen.
#2167b1379af011b54f568e66a6ac6fd2
#test "Check ${out_path}/${of} md5" `expr "${md5}" == "1d7fc7d84bf2f21e668ecd64f3d8af5c"`
mv ${out_path}/${of} ${to}/enable_idr_0.${codec}; . ${unset_script}
fi

if true; then
export EnableIdr=1
export EnableIdrFrame=47 #prime number
export VENC_CH00_QP=35
${exe} 1 ${codec};
test "Check any output file exists ${out_path}/${of}" `expr " [ -s ${out_path}/${of} ] "`
md5=`md5sum ${out_path}/${of} | cut -c-32`
#ae89489162aba609ee9b9bbc80b662d4 is also seen
#4d5a9263322634cded583d3c2452d621
#b3bf9e55e257d6a8f136fd861eae3418
#a986277d63bbedb4a73798f3839a9d29
#test "Check ${out_path}/${of} md5" `expr "${md5}" == "217f7ca716558b9ab662e906e5b0d21c"`
mv ${out_path}/${of} ${to}/enable_idr_1.${codec}; . ${unset_script}
#${exe}  1 ${codec}; mv ${out_path}/${of} ${to}/enable_idr_1.${codec}; . ${unset_script}
fi

if false; then #corner case
export EnableIdr=0
export EnableIdrFrame=0
export VENC_CH00_QP=35
${exe}  1 ${codec}; mv ${out_path}/${of} ${to}/enable_idr_0.${codec}; . ${unset_script}
fi
fi

print_result
