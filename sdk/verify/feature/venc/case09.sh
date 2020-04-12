#!/bin/sh

. _test.sh
ret_test=$?
if [ "${ret_test}" == "0" ]; then

log "\n\n${script}: Static  Channel Attribute: ROI & BG Frame Rate\n"

expected_ret=0

#needed predefined variable in this script: $of, $codec, $unset_script, $expected_ret, $exe
#input $1: prefix of output file, output:$md5
run_case() {
  rm ${of} 2> /dev/null; #remove output file from previous test
  ${exe} 1 ${codec};     #how this program is executed.
  ret=$?                 #keep the result as run_std_case_check() input

  #run_std_case_check $1
  unset md5
  run_es_on_ok_check $1

  #how result,$md5 should be checked
  if [ "${md5}" != "" ]; then # $md5 remains unset, it means there is no output es to be checked.
    run_std_md5_check $1
  fi
}

set_cbr() {
  export VENC_RC=1
  export RcType="Cbr"
  export Bitrate=1000000
}

#the rest tests the CONFIG system
if true ; then
export VENC_GLOB_APPLY_CFG=1 #make something like nRows to take effect

run_suite() {
if true; then
  set_cbr
  export VENC_ROI0_EN=1
  export VENC_ROI0_X=0
  export VENC_ROI0_Y=0
  export VENC_ROI0_W=192
  export VENC_ROI0_H=160
  export VENC_ROI0_Qp=7
  run_case roi_0_0_192_160

  set_cbr
  export VENC_ROI0_EN=1
  export VENC_ROI0_X=128
  export VENC_ROI0_Y=96
  export VENC_ROI0_W=96
  export VENC_ROI0_H=96
  export VENC_ROI0_Qp=7
  run_case roi_128_96_96_96

  expected_ret=1
  set_cbr
  export VENC_ROI0_EN=1
  export VENC_ROI0_X=1
  export VENC_ROI0_Y=2
  export VENC_ROI0_W=3
  export VENC_ROI0_H=4
  export VENC_ROI0_Qp=7
  run_case roi_1_2_3_4
  expected_ret=0
fi
  #test bg frame rate
if true; then
  set_cbr
  export VENC_ROI0_EN=1
  export VENC_ROI0_X=0
  export VENC_ROI0_Y=0
  export VENC_ROI0_W=192
  export VENC_ROI0_H=160
  export VENC_ROI0_Qp=7
  export VENC_ROI_SRC_RATE=3
  export VENC_ROI_DST_RATE=1
  run_case roi_0_0_192_160_bg
fi
}

if true; then
codec=h264
   p_roi_0_0_192_160="30583ebf8b2fe854602c967f6e03a7d9" #"4591af5ce710e67303c7fa01d09a0f1a"
  p_roi_128_96_96_96="4c0c3380fc5ee88fbbf7667082120761" #"95935f6b53ed7515a9ae5f74f3f08ec9"
p_roi_0_0_192_160_bg="fa86f0c5776b95e5edb85ad711e379c0"
   s_roi_0_0_192_160="00000000000000000000000000000000"
  s_roi_128_96_96_96="00000000000000000000000000000000"
s_roi_0_0_192_160_bg="00000000000000000000000000000000"
run_suite
fi

if true; then
codec=h265
   p_roi_0_0_192_160="9ca2bcae279d830d1eb66ae4bd46c6e6" #"d16d6d7c3e5532e1a773cae773b0b9b8"
  p_roi_128_96_96_96="044f95f21b9476b0a9a0700b4e4ff0bf" #"a2a968857ce01f2d8628b4aae84a268d"
p_roi_0_0_192_160_bg="08507a8985a8163f3ce8c19327df37fb"
   s_roi_0_0_192_160="00000000000000000000000000000000"
  s_roi_128_96_96_96="00000000000000000000000000000000"
s_roi_0_0_192_160_bg="00000000000000000000000000000000"
run_suite
fi

fi

print_result
fi