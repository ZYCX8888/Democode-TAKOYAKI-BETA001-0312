#!/bin/sh

. _test.sh

script=$(basename "$0")
script="${script%.*}"

echo 2 > $dbg_level

log "\n\n${script}: Dynamic Channel Attribute: QP, Gop, FPS, Bitrate\n"
#if this passed, then run the next case to generate more stream


if true ; then

to=$mnt/out/es
unset_script=./_unset_config.sh

if [ ! -d $mnt/out/ ]; then mkdir $mnt/out/; fi
if [ ! -d $mnt/out/es ]; then mkdir $mnt/out/es; fi

#needed predefined variable in this script $of $codec, input $1: prefix of output file, output:$md5
expected_ret=0
run_case() {
  rm ${of} 2> /dev/null; ${exe} 1 ${codec} ;
  ret=$?
  test "The program returns ${ret}." `expr "${ret}" == "${expected_ret}"`
  test "Check output file existence: $1.${codec}" `expr " [ -s ${of} ] "`
  md5=`md5sum ${of} | cut -d' ' -f1`
  echo md5:${md5}
  mv ${of} ${to}/$1.${codec}; . ${unset_script}
}

#==== settings ====
default_bitrate=1000000

codec=h264
of=${out_path}/enc_d2c00.es

#setting for this test script
#loop the same pattern with 10 frames each GOP and change attribute right on next GOP
set_gop() {
  export VENC_GLOB_MAX_FRAMES=20
  export VENC_GLOB_DUMP_ES=20
  export VENC_CHANGE_CHN_ATTR=10
  export VENC_ChangeHeight=0
  export VENC_GOP=10
}

set_cbr() {
  set_gop
  export VENC_RC=1
  export RcType="Cbr"
  export Bitrate=${default_bitrate}
}

if true; then
  set_gop
  export VENC_ChangeQp=-2
  run_case set_ch_attr_qp2
  test "Check ${of} md5" `expr "${md5}" == "18709b0f97c91bded849dfe75569020e"`
fi

if true; then
  set_gop
  export VENC_ChangeGop=-5
  run_case set_ch_attr_gop10_5
  test "Check ${of} md5" `expr "${md5}" == "88163b6fe71fd4deb905bb05024f7283"`
fi

if true; then
  set_cbr
  export VENC_ChangeToFpsN=2
  run_case set_ch_attr_fps_x2
  test "Check ${of} md5" `expr "${md5}" == "8f9e9a4eb6408e2abf4358229a376287"`
fi

if true; then
  set_cbr
  export VENC_ChangeBitrate=2
  run_case set_ch_attr_br_x2
  test "Check ${of} md5" `expr "${md5}" == "a8864940ccb500da422c37eaa5ce9e70"` #2 frames only
fi

codec=h265
of=${out_path}/enc_d0c00.es

if true; then
  set_gop
  export VENC_ChangeQp=-2
  run_case set_ch_attr_qp2
  test "Check ${of} md5" `expr "${md5}" == "92955409aacc177aee17e652baf87c81"`
fi

if true; then
  set_gop
  export VENC_ChangeGop=-5
  run_case set_ch_attr_gop10_5
  test "Check ${of} md5" `expr "${md5}" == "f4265512f0ec14fe8a9483edb09c491e"`
fi

if true; then
  set_cbr
  export VENC_ChangeToFpsN=2
  run_case set_ch_attr_fps_x2
  test "Check ${of} md5" `expr "${md5}" == "0bcbb6cc1ee569ab4c6ab299bae648d9"`
fi

if true; then
  set_cbr
  export VENC_ChangeBitrate=2
  run_case set_ch_attr_br_x2
  test "Check ${of} md5" `expr "${md5}" == "330515de43abfef48e3847f95382ea77"` #2 frames only
fi


fi

print_result
