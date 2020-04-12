#!/bin/sh

. _test.sh
ret_test=$?
if [ "${ret_test}" == "0" ]; then

log "\n\n${script}: Dynamic Channel Attribute: Frame Lost Strategy\n"

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
    run_std_md5_check $1
  fi
}

#==== settings ====

#setting for this test script
#loop the same pattern with 10 frames each GOP and change attribute right on next GOP
set_gop() {
  export VENC_GLOB_MAX_FRAMES=20
  export VENC_GLOB_DUMP_ES=20
  export VENC_GOP=10
}

set_cbr() {
  set_gop
  export VENC_RC=1
  export RcType="Cbr"
  export Bitrate=${default_bitrate}
}

set_vbr() {
  set_cbr
  export RcType="Vbr"
  export VbrMinQp=20
  export VbrMaxQp=40
}

#export VENC_GLOB_AUTO_STOP=0
run_suite() {
  set_vbr
  export VENC_FRAME_LOST=2 #same effect of 1 (enabled) but more debug info for parameter setting tuning
  export FrmLostGap=1
  export FrmLostBps=${target_bitrate1}
  run_case frm_lost_gap1

  set_vbr
  export VENC_FRAME_LOST=2 #same effect of 1 (enabled) but more debug info for parameter setting tuning
  export FrmLostGap=1
  export FrmLostBps=${target_bitrate2}
  run_case frm_lost_gap2
}

#the rest tests the CONFIG system

if true; then
codec=h264
default_bitrate=1000000
target_bitrate1=2000000 #not trigger any frame
target_bitrate2=800000  #not a very reasonable setting but it triggers 4 frames
p_frm_lost_gap1="18a389a86f51920f44be96910c6ad2e5"
p_frm_lost_gap2="8584538caa403016555b25c3820012c0"
s_frm_lost_gap1="00000000000000000000000000000000"
s_frm_lost_gap2="00000000000000000000000000000000"
run_suite
fi

if true; then
codec=h265
default_bitrate=1000000
target_bitrate1=2000000
target_bitrate2=1000000
p_frm_lost_gap1="570c7690b705f9f977a71160f02622e0"
p_frm_lost_gap2="5228176b5af5b2dea33a02725b0d5946"
s_frm_lost_gap1="00000000000000000000000000000000"
s_frm_lost_gap2="00000000000000000000000000000000"
run_suite
fi

print_result
fi