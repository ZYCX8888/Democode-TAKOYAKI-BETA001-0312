#!/bin/sh

. ./_test.sh

script=$(basename "$0")
script="${script%.*}"

echo 2 > $dbg_level

log "\n\n${script}: 1 channel basic encoding.\n"

to=$mnt/out/es
if [ ! -d $mnt/out/ ]; then mkdir $mnt/out/; fi
if [ ! -d $mnt/out/es ]; then mkdir $mnt/out/es; fi

#needed predefined variable in this script $of $codec, input $1: prefix of output file, output:$md5
run_case() {
  rm ${of} 2> /dev/null; ${exe} 1 ${codec} ;
  ret=$?
  test "The program returns ${ret}."
  test "Check any output file exists ${of}" `expr " [ -s ${of} ] "`
  md5=`md5sum ${of} | cut -d' ' -f1`
  mv ${of} ${to}/$1.${codec};
}

if true; then
  codec=h264
  of=${out_path}/enc_d2c00.es
  run_case basic
  test "Check ${of} md5" `expr "${md5}" == "80ccf4cabfaa48c9a13e5113ad057865"`
fi

if true; then
  codec=h265
  of=${out_path}/enc_d0c00.es
  run_case basic
  test "Check ${of} md5" `expr "${md5}" == "187cefe46c075427f5b01acf1f87a03a"`
fi

if true; then
  codec=jpeg
  of=${out_path}/enc_d4c00.es
  run_case basic
  test "Check ${of} md5" `expr "${md5}" == "3c8f041df11646369274d7df7f5a9b1c"`
fi

print_result
