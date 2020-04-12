#!/bin/sh

. _test.sh

script=$(basename "$0")
script="${script%.*}"

echo 2 > $dbg_level

log "\n\n${script}: Dynamic Channel Attribute: Height\n"
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
  echo md5:${md5}
  mv ${of} ${to}/$1.${codec}; ${unset_script}
}

#==== settings ====
codec=h264
of=${out_path}/enc_d2c00.es

if true; then
  export VENC_CHANGE_CHN_ATTR=3
  export VENC_ChangeHeight=-64 #352x224
  run_case set_ch_attr_3_224
  test "Check ${of} md5" `expr "${md5}" == "518f0e29da827ba8cb9938393c9bbabc"`
fi

if true; then
  export VENC_CHANGE_CHN_ATTR=3
  export VENC_ChangeHeight=-80 #352x208
  run_case set_ch_attr_3_208
  test "Check ${of} md5" `expr "${md5}" == "58d39e5d8af93e879de4ef27bfd85162"`
fi

if true; then
  export VENC_CHANGE_CHN_ATTR=3
  export VENC_ChangeHeight=-82 #352x206
  run_case set_ch_attr_3_206
  test "Check ${of} md5" `expr "${md5}" == "8feb5d8708162c2efa3fb96c7bb333be"`
fi

if true; then
  export VENC_CHANGE_CHN_ATTR=3
  export VENC_ChangeHeight=-83 #352x205
  expected_ret=252
  run_case set_ch_attr_3_205
  test "Check ${of} md5" `expr "${md5}" == "f045378a25a41cdd78cbc4db13b75589"` #2 frames only
fi
expected_ret=0

codec=h265
of=${out_path}/enc_d0c00.es

if true; then
  export VENC_CHANGE_CHN_ATTR=3
  export VENC_ChangeHeight=-64 #352x224
  run_case set_ch_attr_3_224
  test "Check ${of} md5" `expr "${md5}" == "8d1aad145194c30ede0e6c5745ca2773"`
fi

if true; then
  export VENC_CHANGE_CHN_ATTR=3
  export VENC_ChangeHeight=-80 #352x208
  run_case set_ch_attr_3_208
  test "Check ${of} md5" `expr "${md5}" == "9dfeb541f9a781435dfe3f169449cab6"`
fi

if true; then
  export VENC_CHANGE_CHN_ATTR=3
  export VENC_ChangeHeight=-82 #352x206
  run_case set_ch_attr_3_206
  test "Check ${of} md5" `expr "${md5}" == "48f6168bb914dda797a53a0e78351e65"`
fi

if true; then
  export VENC_CHANGE_CHN_ATTR=3
  export VENC_ChangeHeight=-83 #352x205
  expected_ret=252
  run_case set_ch_attr_3_205
  test "Check ${of} md5" `expr "${md5}" == "45281fd6454ffda9c00cccab9e34070b"` #2 frames only
fi
expected_ret=0

codec=jpeg
of=${out_path}/enc_d4c00.es

if true; then
  export VENC_CHANGE_CHN_ATTR=3
  export VENC_ChangeHeight=-64 #352x224
  run_case set_ch_attr_3_224
  test "Check ${of} md5" `expr "${md5}" == "13ae35f1fdb3887a8dcb126eccae1c1e"`
fi

if true; then
  export VENC_CHANGE_CHN_ATTR=3
  export VENC_ChangeHeight=-80 #352x208
  run_case set_ch_attr_3_208
  test "Check ${of} md5" `expr "${md5}" == "c1cbd9aa61cc06fa5bdaba8fb8b9b2d2"`
fi

if true; then
  export VENC_CHANGE_CHN_ATTR=3
  export VENC_ChangeHeight=-82 #352x206
  run_case set_ch_attr_3_206
  test "Check ${of} md5" `expr "${md5}" == "62f315d21519c39329ec3e7c806b4dfd"`
fi

if true; then
  export VENC_CHANGE_CHN_ATTR=3
  export VENC_ChangeHeight=-83 #352x205
  expected_ret=252
  run_case set_ch_attr_3_205
  test "Check ${of} md5" `expr "${md5}" == "16650cacb9226a9a338dcd1553eebcd9"` #2 frames only
fi
expected_ret=0

fi

print_result
