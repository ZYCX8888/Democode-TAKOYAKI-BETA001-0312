#!/bin/sh

. _test.sh
ret_test=$?
if [ "${ret_test}" == "0" ]; then

log "\n\n${script}: Dynamic Channel Attribute: EnableIdr\n"

expected_ret=0

#needed predefined variable in this script: $of, $codec, $unset_script, $expected_ret, $exe
#input $1: prefix of output file, output:$md5
run_case() {
  rm ${of} 2> /dev/null; #remove output file from previous test
  ${exe} 1 ${codec};     #how this program is executed.
  ret=$?                 #keep the result as run_std_case_check() input

  run_std_case_check $1

  #how result,$md5 should be checked
  run_std_md5_check $1
}


#the rest tests the CONFIG system
if true ; then
export VENC_GLOB_MAX_FRAMES=20
export VENC_GLOB_DUMP_ES=20
export VENC_GLOB_APPLY_CFG=1 #make something like nRows to take effect

run_suite() {
  export EnableIdr=1
  export EnableIdrFrame=11 #prime number
  export VENC_GOP=20
  export VENC_CH00_QP=35
  run_case enable_idr_1
}

codec=h264
p_enable_idr_1="c05424920c62a143670dcf009f7555c2"
s_enable_idr_1="00000000000000000000000000000000"
run_suite

codec=h265
p_enable_idr_1="98476761478637e69a3c1733637a5a9b"
s_enable_idr_1="00000000000000000000000000000000"
run_suite

fi

print_result
fi