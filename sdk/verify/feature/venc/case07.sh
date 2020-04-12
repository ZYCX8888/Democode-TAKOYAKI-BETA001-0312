#!/bin/sh

. _test.sh
ret_test=$?
if [ "${ret_test}" == "0" ]; then

log "\n\n${script}: Static  Channel Attribute: Vui\n"

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
  export VENC_GLOB_VUI=1
  run_case vui_1
}

codec=h264
p_vui_1="0856d1863ed11d9f4c012a8799ddbb82"
s_vui_1="00000000000000000000000000000000"
run_suite

codec=h265
p_vui_1="e2b1a183482130ed008ae750a4dc18e2"
s_vui_1="00000000000000000000000000000000"
run_suite

fi

print_result
fi