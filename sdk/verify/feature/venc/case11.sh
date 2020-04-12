#!/bin/sh

. _test.sh
ret_test=$?
if [ "${ret_test}" == "0" ]; then

log "\n\n${script}: Dynamic Channel Attribute: Height\n"

expected_ret=0

#needed predefined variable in this script: $of, $codec, $unset_script, $expected_ret, $exe
#input $1: prefix of output file, output:$md5
run_case() {
  rm ${of} 2> /dev/null; #remove output file from previous test
  ${exe} 1 ${codec};     #how this program is executed.
  ret=$?                 #keep the result as run_std_case_check() input

  run_std_case_check $1

  #how result,$md5 should be checked
  run_2_md5_check $1
}


#the rest tests the CONFIG system
if true ; then
export VENC_CH00_QP=45 #don't care about the bit stream quality

#in this test the change is set at frame 3 but it takes effect on frame 3 or frame 4.
#thus, there are 2 possible MD5 in most test cases.
run_suite() {
  export VENC_CHANGE_CHN_ATTR=3
  export VENC_ChangeHeight=-64 #352x224
  run_case set_ch_attr_3_224

  export VENC_CHANGE_CHN_ATTR=3
  export VENC_ChangeHeight=-80 #352x208
  run_case set_ch_attr_3_208

  export VENC_CHANGE_CHN_ATTR=3
  export VENC_ChangeHeight=-82 #352x206
  run_case set_ch_attr_3_206

  export VENC_CHANGE_CHN_ATTR=3
  export VENC_ChangeHeight=-83 #352x205
  expected_ret=252
  run_case set_ch_attr_3_205
  expected_ret=0
}

if true; then
codec=h264
p_set_ch_attr_3_224_1="59b0587c4ca57d8b899c0cfa663e9fcf"
p_set_ch_attr_3_224_2="b61be2e72a624f3783d8bde7ba25ed3b"
p_set_ch_attr_3_208_1="4daf5162d8bc9e7760f200038f56d2b8"
p_set_ch_attr_3_208_2="eeeb327ee75d95b7cab4ca56d0279284"
p_set_ch_attr_3_206_1="87729d23acd6ae3a010bb2bd3a628fe1"
p_set_ch_attr_3_206_2="82b8989ab594db6118abc60615fd9c4f"
p_set_ch_attr_3_205_1="00000000000000000000000000000000" #2 frames only
p_set_ch_attr_3_205_2="95691499dbc39e9591395602cd19e76b" #2 frames only
s_set_ch_attr_3_224_1="710d961387de932327503c7fd68f0c88"
s_set_ch_attr_3_208_1="6e1a38b1130fee41d74bde547605467f"
s_set_ch_attr_3_206_1="5cc40fd1e3200ab355b6b4e242e28c99"
s_set_ch_attr_3_205_1="f045378a25a41cdd78cbc4db13b75589"
s_set_ch_attr_3_224_2="00000000000000000000000000000000"
s_set_ch_attr_3_208_2="00000000000000000000000000000000"
s_set_ch_attr_3_206_2="00000000000000000000000000000000"
s_set_ch_attr_3_205_2="00000000000000000000000000000000"
run_suite
fi

if true; then
codec=h265
p_set_ch_attr_3_224_1="8d1aad145194c30ede0e6c5745ca2773"
p_set_ch_attr_3_224_2="6d37ee4d4fcef8d71a7b43773d395d81"
p_set_ch_attr_3_208_1="9dfeb541f9a781435dfe3f169449cab6"
p_set_ch_attr_3_208_2="c9f7e9e6cdb88d888d59c9a1268d46c3"
p_set_ch_attr_3_206_1="48f6168bb914dda797a53a0e78351e65"
p_set_ch_attr_3_206_2="64117c97dc1ed3c5342c6e5a25a96887"
p_set_ch_attr_3_205_1="45281fd6454ffda9c00cccab9e34070b" #2 frames only
p_set_ch_attr_3_205_2="00000000000000000000000000000000"
s_set_ch_attr_3_224_1="00000000000000000000000000000000"
s_set_ch_attr_3_224_2="00000000000000000000000000000000"
s_set_ch_attr_3_208_1="00000000000000000000000000000000"
s_set_ch_attr_3_208_2="00000000000000000000000000000000"
s_set_ch_attr_3_206_1="00000000000000000000000000000000"
s_set_ch_attr_3_206_2="00000000000000000000000000000000"
s_set_ch_attr_3_205_1="00000000000000000000000000000000"
s_set_ch_attr_3_205_2="00000000000000000000000000000000"
run_suite
fi

if true; then
codec=jpeg
p_set_ch_attr_3_224_1="13ae35f1fdb3887a8dcb126eccae1c1e"
p_set_ch_attr_3_224_2="c404adfc4b72b379005ca8522371cc10"
p_set_ch_attr_3_208_1="c1cbd9aa61cc06fa5bdaba8fb8b9b2d2"
p_set_ch_attr_3_208_2="eeeb327ee75d95b7cab4ca56d0279284"
p_set_ch_attr_3_206_1="62f315d21519c39329ec3e7c806b4dfd"
p_set_ch_attr_3_206_2="bded01a2ffc656351320aa07858bafb2"
p_set_ch_attr_3_205_1="16650cacb9226a9a338dcd1553eebcd9" #2 frames only
p_set_ch_attr_3_205_2="00000000000000000000000000000000"
s_set_ch_attr_3_224_1="00000000000000000000000000000000"
s_set_ch_attr_3_224_2="00000000000000000000000000000000"
s_set_ch_attr_3_208_1="00000000000000000000000000000000"
s_set_ch_attr_3_208_2="00000000000000000000000000000000"
s_set_ch_attr_3_206_1="00000000000000000000000000000000"
s_set_ch_attr_3_206_2="00000000000000000000000000000000"
s_set_ch_attr_3_205_1="00000000000000000000000000000000"
s_set_ch_attr_3_205_2="00000000000000000000000000000000"
run_suite
fi

print_result
fi
fi