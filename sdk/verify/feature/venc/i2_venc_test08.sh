#!/bin/sh

. _test.sh

script=$(basename "$0")
script="${script%.*}"

#debug_level 3 would cause much delay and causing MI gets every interrupts.
#use debug_level 0 to make
echo 2 > $dbg_level #/sys/module/mi_venc/parameters/debug_level

log "\n\n${script}: Dynamic Channel Operation: SEI\n"
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

to=$mnt/out/es
unset_script=./_unset_config.sh

if [ ! -d $mnt/out/ ]; then mkdir $mnt/out/; fi
if [ ! -d $mnt/out/es ]; then mkdir $mnt/out/es; fi

#needed predefined variable in this script $of $codec, input $1: prefix of output file, output:$md5
run_case() {
  rm ${of} 2> /dev/null; ${exe} 1 ${codec} ;
  assert "Check any output file exists ${of}" `expr " [ -s ${of} ] "`
  md5=`md5sum ${of} | cut -d' ' -f1`
  mv ${of} ${to}/$1.${codec}; . ${unset_script}
}
#==== settings ====

codec=h264
of=${out_path}/enc_d2c00.es
if true; then
  export VENC_GLOB_USER_DATA=""
  run_case sei0
  check "Check ${of} md5" `expr "${md5}" == "80ccf4cabfaa48c9a13e5113ad057865"`
fi

if true; then
  export VENC_GLOB_USER_DATA="I have a pen."
  run_case sei1
  check "Check ${of} md5" `expr "${md5}" == "9ecd3b81fa0814040962557eb88ddd5d"`
fi


codec=h265
of=${out_path}/enc_d0c00.es

if true; then
  export VENC_GLOB_USER_DATA=""
  run_case sei0
  check "Check ${of} md5" `expr "${md5}" == "187cefe46c075427f5b01acf1f87a03a"`
fi

if true; then
  export VENC_GLOB_USER_DATA="I have a pen."
  run_case sei1
  check "Check ${of} md5" `expr "${md5}" == "0c88849ff19cac3fd083e5ea1e97f795"`
fi

fi
print_result
