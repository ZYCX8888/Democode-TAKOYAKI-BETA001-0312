#!/bin/sh

. ./_test.sh
ret_test=$?
if [ "${ret_test}" == "0" ]; then

log "\n\n${script}: 1 Channel basic Encoding\n"

expected_ret=0

#needed predefined variable in this script $of $codec, input $1: prefix of output file, output:$md5
run_case() {
  rm ${of} 2> /dev/null; #remove output file from previous test
  ${exe} 1 ${codec};     #how this program is executed.
  ret=$?                 #keep the result as run_std_case_check() input

  run_std_case_check $1

  #how result,$md5 should be checked
  run_std_md5_check $1
}

if true; then
  codec=h264
  p_basic="499c839456c62599eda0a4f2f51e1823"
  s_basic="80ccf4cabfaa48c9a13e5113ad057865"
  run_case basic
  # of=${out_path}/enc_d2c00.es
  # run_case basic
  # test "Check ${of} md5" `expr "${md5}" == "80ccf4cabfaa48c9a13e5113ad057865"`
fi

if true; then
  codec=h265
  p_basic="f624303f5ad0866621faf60f0b0542f6"
  s_basic="187cefe46c075427f5b01acf1f87a03a"
  run_case basic
  # of=${out_path}/enc_d0c00.es
  # run_case basic
  # test "Check ${of} md5" `expr "${md5}" == "187cefe46c075427f5b01acf1f87a03a"`
fi

if true; then
  codec=jpeg
  p_basic="3c8f041df11646369274d7df7f5a9b1c"
  s_basic="00000000000000000000000000000000"
  run_case basic
  # of=${out_path}/enc_d4c00.es
  # run_case basic
  # test "Check ${of} md5" `expr "${md5}" == "3c8f041df11646369274d7df7f5a9b1c"`
fi

print_result
fi