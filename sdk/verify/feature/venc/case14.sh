#!/bin/sh

. _test.sh
ret_test=$?
if [ "${ret_test}" == "0" ]; then

log "\n\n${script}: Dynamic Channel Attribute: Super Frame & RC Priority\n"

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

set_short_test()
{
  export VENC_GLOB_MAX_FRAMES=20
  export VENC_GLOB_DUMP_ES=20
  export VENC_GLOB_APPLY_CFG=1 #make something like nRows to take effect
  export FixQp=0 #Set 1 to enable VENC_CH00_QP
    export VENC_CH00_QP=10 #make a very big stream
}

set_long_test()
{
  export VENC_GLOB_MAX_FRAMES=60
  export VENC_GLOB_DUMP_ES=60
}

#the rest tests the CONFIG system
if true ; then
#==== h264
#I frames: 23369, 24186
#P frames: 7167 8230 8113 8283 9289 8422 9578, -- 8320
#24000*8=192000, 9000*8=72000

#in MI:
# I:23405, 7171, 8234, 8117, 8287, 9293-d, 8426, 9582-d, I:24221-d, 8324

#==== h265
#I frames: 22409, 23430
#P frames: 8864, 10142, 10090, 10648, 11074, 9910, 11577 -- 10634
#23000*8=184000 11000*8=88000

#in MI
# I:22486, 8749, 10369, 10130, 10763, 11455-d, 10119, 11595-d, I:23507-d, 10611
default_bitrate=1000000
set_vbr() {
  export VENC_RC=1
  export RcType="Cbr" #vbr does not over
  export VbrMinQp=20
  export VbrMaxQp=40
  export Bitrate=${default_bitrate}
}

run_suite() {
  set_short_test
  export VENC_SUPER_FRAME_MODE=1 #discard
  run_case super1

  export VENC_SUPER_FRAME_MODE=2 #reencode
  run_case super2
}

run_long_suite() {
  set_long_test
  set_vbr
  export RcPrio=1
  export VENC_SUPER_FRAME_MODE=1 #discard
  run_case rc_prio_1
}

#H264/265/JPEG super1/2 case does not trigger super frame anymore due to encoder change.
#To be double verified.
if true; then
codec=h264
export SuperI=192000
export SuperP=72000
p_super1="39844d0e1ebc76b7917455fdc88caaf8" #"a7a182a998ec36979d246bc994415f71"
p_super2="25c30f98df41236badf0d6d1deed4104" #"47837a93786dd5fc97955be5f6af1694"
s_super1="00000000000000000000000000000000"
s_super2="00000000000000000000000000000000"
run_suite
fi

if true; then
codec=h265
export SuperI=184000
export SuperP=88000
p_super1="e1aada5a99bcdb92dd023866a81d29eb" #"e1aada5a99bcdb92dd023866a81d29eb"
p_super2="9995713053d9d4eed133ee43135d6be5" #"9995713053d9d4eed133ee43135d6be5"
s_super1="00000000000000000000000000000000"
s_super2="00000000000000000000000000000000"
run_suite
fi

if true; then
codec=h264
export SuperI=154000
export SuperP=30000
p_rc_prio_1="fa7da122cbe0f0e0e63f3432d9f4108d"
s_rc_prio_1="00000000000000000000000000000000"
run_long_suite
fi

if true; then
codec=h265
export SuperI=124000
export SuperP=50000
p_rc_prio_1="a59f5b648c18c538caf95a3578e31cf7"
s_rc_prio_1="00000000000000000000000000000000"
run_long_suite
fi

#9654, 9614, 9558, 9513, 9677, 9732, 9807, 9744, 9696, 9658,
#9700*8=77600
if true; then
codec=jpeg
#export SuperI=77600
export SuperI=315000
p_super1="d94114081e0225f042a230b82c5f1fa5" #"d9898beae7b88ec66113bef6a4fbf023"
p_super2="ffe0e3cf8cb30fffa519db808ea911e8" #"9ed9566ee74eaeca53efd453dde5adad"
s_super1="00000000000000000000000000000000"
s_super2="00000000000000000000000000000000"
run_suite
fi

fi

print_result
fi