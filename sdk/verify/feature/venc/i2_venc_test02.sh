#!/bin/sh

. ./_test.sh

script=$(basename "$0")
script="${script%.*}"

#debug_level 3 would cause much delay and causing MI gets every interrupts.
#use debug_level 0 to make
echo 0 > $dbg_level #/sys/module/mi_venc/parameters/debug_level

log "\n\n${script}: 2 channels encoding.\n"

to=$mnt/out/es
if [ ! -d $mnt/out/ ]; then mkdir $mnt/out/; fi
if [ ! -d $mnt/out/es ]; then mkdir $mnt/out/es; fi

#needed predefined variable in this script $of $codec, input $1: prefix of output file, output:$md5
run_case() {
  rm ${of1} 2> /dev/null;
  rm ${of2} 2> /dev/null;
  ${exe} 2 ${codec} ${codec};
  ret=$?
  test "The program returns ${ret}."
  test "Check any output file exists ${of1}" `expr " [ -s ${of1} ] "`
  test "Check any output file exists ${of2}" `expr " [ -s ${of2} ] "`
  md5_1=`md5sum ${of1} | cut -d' ' -f1`
  md5_2=`md5sum ${of2} | cut -d' ' -f1`
  mv ${of1} ${to}/$1_c0.${codec};
  mv ${of2} ${to}/$1_c1.${codec};
}

if true; then
  codec=h264
  of1=${out_path}/enc_d2c00.es
  of2=${out_path}/enc_d3c01.es
  run_case 2chn
  test "Check ${of1} md5" `expr "${md5_1}" == "80ccf4cabfaa48c9a13e5113ad057865"`
  test "Check ${of2} md5" `expr "${md5_2}" == "64b3f4127588978a64feafa7046f23bb"`
fi

if true; then
  codec=h265
  of1=${out_path}/enc_d0c00.es
  of2=${out_path}/enc_d1c01.es
  run_case 2chn
  test "Check ${of1} md5" `expr "${md5_1}" == "187cefe46c075427f5b01acf1f87a03a"`
  test "Check ${of2} md5" `expr "${md5_2}" == "6d27a5ada714c0d9302ff62bbc8f0c11"`
fi

if true; then
  codec=jpeg
  of1=${out_path}/enc_d4c00.es
  of2=${out_path}/enc_d4c01.es
  run_case 2chn
  test "Check ${of1} md5" `expr "${md5_1}" == "3c8f041df11646369274d7df7f5a9b1c"`
  test "Check ${of2} md5" `expr "${md5_2}" == "3c8f041df11646369274d7df7f5a9b1c"`
fi

if false; then
${exe} 2 h264 h264
ret=$?

test "The program returns ${ret}." `expr "${ret}" == "0"`

#device2.ch00.ElementaryStream
out_file0=enc_d2c00p00.es
result=`md5sum $out_path/${out_file0} | cut -c-32`
test "Check ${out_path}/${out_file0} ${out_file0}" `expr "${result}" == "${md5ch0}"`

out_file1=enc_d2c01p00.es
result=`md5sum $out_path/${out_file1} | cut -c-32`
test "Check ${out_path}/${out_file1}" `expr "${result}" == "${md5ch1}"`
#echo result=${result}
fi
print_result
