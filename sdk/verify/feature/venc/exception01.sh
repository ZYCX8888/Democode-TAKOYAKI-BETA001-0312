#!/bin/sh

. _test.sh

ret_test=$?
if [ "${ret_test}" == "0" ]; then

log "\n\n${script}: Exception: No ISR.\n"
ver=2 #this script name is not start with 'c'. Manually set this variable to use Test Set 2 script

if true; then

export VENC_GLOB_MAX_FRAMES=200
export VENC_GLOB_DUMP_ES=200
export VENC_GLOB_AUTO_STOP=0

expected_ret=0
#needed predefined variable in this script: $of, $codec, $unset_script, $expected_ret, $exe
#input $1: prefix of output file, output:$md5
run_case() {
  rm ${of} 2> /dev/null;
  ${exe} 1 ${codec} &
  ret=$?

  echo then "ret = ${ret}"
  ping -w 3 0.0.0.0 > /dev/null
  #echo dmsg cmdq 1 > ${venc2}
  echo dmsg irq 1 > ${venc2}
  #run_std_case_check $1
}

codec=h264
of=${out_path}/enc_d2c00.es
run_case

eval `cat ${result}`
echo result = ${FPSx100}

fi

print_result
fi