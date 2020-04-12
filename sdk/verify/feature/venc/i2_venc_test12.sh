#!/bin/sh

. _test.sh

script=$(basename "$0")
script="${script%.*}"

echo 2 > $dbg_level

log "\n\n${script}: Static  Channel Attribute: LTR mode 1 & 3\n"
#if this passed, then run the next case to generate more stream


if true ; then

to=$mnt/out/es
unset_script=./_unset_config.sh

if [ ! -d $mnt/out/ ]; then mkdir $mnt/out/; fi
if [ ! -d $mnt/out/es ]; then mkdir $mnt/out/es; fi

#needed predefined variable in this script $of $codec, input $1: prefix of output file, output:$md5
expected_ret=0
run_case() {
  rm ${of} 2> /dev/null; ${exe} 1 ${codec} ;
  ret=$?
  test "The program returns ${ret}." `expr "${ret}" == "${expected_ret}"`
  test "Check output file existence: $1.${codec}" `expr " [ -s ${of} ] "`
  md5=`md5sum ${of} | cut -d' ' -f1`
  mv ${of} ${to}/$1.${codec}; ${unset_script}
}

#==== settings ====
codec=h264
of=${out_path}/enc_d2c00.es

export VENC_GLOB_MAX_FRAMES=20
export VENC_GLOB_DUMP_ES=20
export VENC_REF_EN=1
export VENC_GOP=30
if true; then
  export VENC_REF_Base=1
  export VENC_REF_Enhance=5
  export VENC_REF_EnablePred=0
  run_case ltr_mode1
  test "Check ${of} md5" `expr "${md5}" == "9882e66cbf5720313eb15d106dfffb4f"`
fi

if true; then
  export VENC_REF_Base=1
  export VENC_REF_Enhance=5
  export VENC_REF_EnablePred=1
  run_case ltr_mode3
  test "Check ${of} md5" `expr "${md5}" == "2aa1229fc8f5cbea3527190e51e54b4f"`
fi

if true;then
codec=h265
of=${out_path}/enc_d0c00.es

if true; then
  export VENC_REF_Base=1
  export VENC_REF_Enhance=5
  export VENC_REF_EnablePred=0
  run_case ltr_mode1
  test "Check ${of} md5" `expr "${md5}" == "8b92f59ba109958a9bad6f7bc79bb63b"`
fi

if true; then
  export VENC_REF_Base=1
  export VENC_REF_Enhance=5
  export VENC_REF_EnablePred=1
  run_case ltr_mode3
  test "Check ${of} md5" `expr "${md5}" == "3fc8f705285c5eed6d147ccb81822202"`
fi

fi #h265
fi #test case

print_result
