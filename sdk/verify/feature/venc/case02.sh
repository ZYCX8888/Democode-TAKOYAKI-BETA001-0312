#!/bin/sh

. _test.sh
ret_test=$?
if [ "${ret_test}" == "0" ]; then

log "\n\n${script}: 2 Channels Encoding\n"

expected_ret=0

#used for standard checking
#needed predefined variable in this script: $of, $codec, $unset_script, $expected_ret, $ret
#input $1: prefix of output file, output:$md5, $answer
run_2_streams_check() {
  assert "The program returns ${ret}." `expr "${ret}" == "${expected_ret}"`
  assert "Check any output file exists ${of0}" `expr " [ -s ${of0} ] "`
  assert "Check any output file exists ${of1}" `expr " [ -s ${of1} ] "`
  md5_0=`md5sum ${of0} | cut -d' ' -f1`
  md5_1=`md5sum ${of1} | cut -d' ' -f1`
  echo "md5:${md5_0}  $1.${codec}"
  echo "md5:${md5_1}  $2.${codec}"
  mv ${of0} ${to}/$1.${codec};
  mv ${of1} ${to}/$2.${codec};
  if [ "$?" != "0" ]; then echo -e $FAIL; fi
  if [ "{$unset_script}" != "" ]; then . $unset_script; fi
}

#must run after run_2_streams_check
#needed predefined variable in this script: $codec, $md5, $ans_ver
#input $1: prefix of output file, output:none
run_2_streams_md5_check() {
  eval answer0=\$${ans_ver}${1}
  eval answer1=\$${ans_ver}${2}
  check "Check $1.${codec} MD5" `expr "${md5_0}" == "${answer0}"`
  if [ "$?" != "0" ]; then echo "expected MD5: ${answer0}"; fi
  check "Check $2.${codec} MD5" `expr "${md5_1}" == "${answer1}"`
  if [ "$?" != "0" ]; then echo "expected MD5: ${answer1}"; fi
  (>&2 echo -e "")       #Add extra line into summary
}

#needed predefined variable in this script: $of, $codec, $unset_script, $expected_ret, $exe
#input $1: prefix of output file, output:$md5
run_case() {
  rm ${of0} 2> /dev/null; #remove output file from previous test
  rm ${of1} 2> /dev/null; #remove output file from previous test
  ${exe} 2 ${codec} ${codec};     #how this program is executed.
  ret=$?                 #keep the result as run_2_streams_check() input

  run_2_streams_check $1 $2

  #how result,$md5 should be checked
  run_2_streams_md5_check $1 $2
}


#the rest tests the CONFIG system
if true ; then

run_suite() {
  run_case 2_stream_c0 2_stream_c1
}

codec=h264
p_2_stream_c0="499c839456c62599eda0a4f2f51e1823"
p_2_stream_c1="e8abd94df707d2a2b6dfd67e771ca382"
s_2_stream_c0="00000000000000000000000000000000"
s_2_stream_c1="00000000000000000000000000000000"
run_suite

codec=h265
p_2_stream_c0="f624303f5ad0866621faf60f0b0542f6"
p_2_stream_c1="ea393fa59aefd264624948e43398142e"
s_2_stream_c0="00000000000000000000000000000000"
s_2_stream_c1="00000000000000000000000000000000"
run_suite

fi

print_result
fi