#!/bin/sh

. _test.sh

ret_test=$?
if [ "${ret_test}" == "0" ]; then

log "\n\n${script}: JPEG Q-Factor\n"

if true; then

expected_ret=0
#needed predefined variable in this script: $of, $codec, $unset_script, $expected_ret, $exe
#input $1: prefix of output file, output:$md5
run_case() {
  rm ${of} 2> /dev/null;
  ${exe} 1 ${codec} ;
  ret=$?

  run_std_case_check $1
  if [ "${md5}" != "" ]; then # $md5 remains unset, it means there is no output es to be checked.
    run_std_md5_check $1
  fi
}

export VENC_GLOB_APPLY_CFG=1
run_suite() {
  export Qfactor=100
  run_case q_factor_100

  export Qfactor=50
  run_case q_factor_50

  export Qfactor=25
  run_case q_factor_25
}

codec=jpeg
p_q_factor_100="c008dae42e79324576c2f3e838127a3a"
p_q_factor_50="6f375671418a25dcf498ceb4f2091fb9"
p_q_factor_25="3c8f041df11646369274d7df7f5a9b1c"
s_q_factor_100="00000000000000000000000000000000"
s_q_factor_50="00000000000000000000000000000000"
s_q_factor_25="00000000000000000000000000000000"
run_suite

fi

print_result
fi