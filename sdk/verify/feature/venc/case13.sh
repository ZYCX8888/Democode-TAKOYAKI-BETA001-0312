#!/bin/sh

. _test.sh
ret_test=$?
if [ "${ret_test}" == "0" ]; then

log "\n\n${script}: Dynamic Channel Attribute: QP, GOP, FPS, Bit Rate\n"

expected_ret=0

#needed predefined variable in this script: $of, $codec, $unset_script, $expected_ret, $exe
#input $1: prefix of output file, output:$md5
run_case() {
  rm ${of} 2> /dev/null; #remove output file from previous test
  ${exe} 1 ${codec};     #how this program is executed.
  ret=$?                 #keep the result as run_es_on_ok_check() input

  unset md5
  run_es_on_ok_check $1

  #how result,$md5 should be checked
  if [ "${md5}" != "" ]; then # $md5 remains unset, it means there is no output es to be checked.
    run_2_md5_check $1
  fi
}

#==== settings ====
default_bitrate=1000000

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

#the rest tests the CONFIG system
if true ; then
export VENC_GLOB_MAX_FRAMES=10
export VENC_GLOB_DUMP_ES=10
export VENC_GLOB_APPLY_CFG=0 #make something like nRows to take effect

run_suite() {
  set_gop
  export VENC_ChangeQp=-2
  run_case set_attr_qp_2

  set_gop
  export VENC_ChangeGop=-5
  run_case set_attr_gop_5

  set_cbr
  export VENC_ChangeToFpsN=60 #from 30 to 60 FPS
  run_case set_attr_fps_x2

  set_cbr
  export VENC_ChangeBitrate=2
  run_case set_attr_br_x2
}

if true; then
codec=h264
  p_set_attr_qp_2_1="b468f51bfa7548a700e0309a44de2232"
  p_set_attr_qp_2_2="153f8640e9a5aa0c760d21838923fea3"
 p_set_attr_gop_5_1="9108e5a494503de895ea9b892dead70e"
 p_set_attr_gop_5_2="3551e59573de3bb540bcd8e9f01da005"
p_set_attr_fps_x2_1="23a1ceca61f1eefda2614323ed5b5aec"
p_set_attr_fps_x2_2="16156468437525fb07496e61608c72b5"
 p_set_attr_br_x2_1="00e453bee02f0e21c70641eb9b4c30f0"
 p_set_attr_br_x2_2="741c517daa0f17143e1af253cc0f8280"
  s_set_attr_qp_2_1="00000000000000000000000000000000"
  s_set_attr_qp_2_2="00000000000000000000000000000000"
 s_set_attr_gop_5_1="00000000000000000000000000000000"
 s_set_attr_gop_5_2="00000000000000000000000000000000"
s_set_attr_fps_x2_1="00000000000000000000000000000000"
s_set_attr_fps_x2_2="00000000000000000000000000000000"
 s_set_attr_br_x2_1="00000000000000000000000000000000"
 s_set_attr_br_x2_2="00000000000000000000000000000000"
run_suite
fi

if true; then
codec=h265
  p_set_attr_qp_2_1="ee6db16256ac78641e1ba94b5c4034f6"
  p_set_attr_qp_2_2="3d60fb75015f025e2e6aa9c1ad6141c0"
 p_set_attr_gop_5_1="7f11575c0bb1089181b9a87109a65971"
 p_set_attr_gop_5_2="c3c5dc6845fbd447ca0ff09cf08b71ad"
p_set_attr_fps_x2_1="21487c5d56b2034e26066170e34110ba"
p_set_attr_fps_x2_2="e1a71a870fd6d522900f31b10e214438"
 p_set_attr_br_x2_1="bd73ad651e96cd38f4da2877ba0e7b09"
 p_set_attr_br_x2_2="b46337279d3630706edcf84151dde162"
  s_set_attr_qp_2_1="00000000000000000000000000000000"
  s_set_attr_qp_2_2="00000000000000000000000000000000"
 s_set_attr_gop_5_1="00000000000000000000000000000000"
 s_set_attr_gop_5_2="00000000000000000000000000000000"
s_set_attr_fps_x2_1="00000000000000000000000000000000"
s_set_attr_fps_x2_2="00000000000000000000000000000000"
 s_set_attr_br_x2_1="00000000000000000000000000000000"
 s_set_attr_br_x2_2="00000000000000000000000000000000"
run_suite
fi


fi

print_result
fi