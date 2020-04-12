#!/bin/sh

. _test.sh
ret_test=$?
if [ "${ret_test}" == "0" ]; then

log "\n\n${script}: Static  Channel Attribute: LTR mode 1 & 3\n"

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


#the rest tests the CONFIG system
if true ; then
export VENC_GLOB_MAX_FRAMES=20
export VENC_GLOB_DUMP_ES=20
export VENC_GLOB_APPLY_CFG=0 #make something like nRows to take effect

run_suite() {
  export VENC_REF_EN=1
  export VENC_GOP=30
  export VENC_REF_Base=1
  export VENC_REF_Enhance=5
  export VENC_REF_EnablePred=0
  run_case ltr_mode1

  export VENC_REF_EN=1
  export VENC_GOP=30
  export VENC_REF_Base=1
  export VENC_REF_Enhance=5
  export VENC_REF_EnablePred=1
  run_case ltr_mode3
}

if true; then
codec=h264
p_ltr_mode1="edc85b84ec08a2e50bb4547d47b14d42"
p_ltr_mode3="5c4d28027dbc7db6ac8f34b68614334d"
s_ltr_mode1="00000000000000000000000000000000"
s_ltr_mode3="00000000000000000000000000000000"
run_suite
fi

if true; then
codec=h265
p_ltr_mode1="8b92f59ba109958a9bad6f7bc79bb63b"
p_ltr_mode3="3fc8f705285c5eed6d147ccb81822202"
s_ltr_mode1="00000000000000000000000000000000"
s_ltr_mode3="00000000000000000000000000000000"
run_suite
fi


fi

print_result
fi