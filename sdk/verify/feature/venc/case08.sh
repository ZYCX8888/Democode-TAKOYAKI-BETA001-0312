#!/bin/sh

. _test.sh
ret_test=$?
if [ "${ret_test}" == "0" ]; then

log "\n\n${script}: Dynamic Channel Operation: SEI\n"

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
export VENC_GLOB_APPLY_CFG=1 #make something like nRows to take effect

run_suite() {
  export VENC_GLOB_USER_DATA="I have a pen."
  run_case sei_1
}

codec=h264
p_sei_1="26b1786a8448ef2b6f2a768ba96f9208"
s_sei_1="00000000000000000000000000000000"
run_suite

codec=h265
p_sei_1="91e64ed79c835dfe029460dc1228c281"
s_sei_1="00000000000000000000000000000000"
run_suite

fi

print_result
fi