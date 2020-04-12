#!/bin/sh

. _test.sh
ret_test=$?
if [ "${ret_test}" == "0" ]; then

log "\n\n${script}: Static  Channel Attribute: Crop\n"

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


#the rest tests the CONFIG system
if true ; then
export VENC_CH00_QP=45 #don't care about the bit stream quality
export VENC_GLOB_APPLY_CFG=1 #make something like nRows to take effect

run_suite() {
  export VENC_Crop=1
  export VENC_Crop_X=256
  export VENC_Crop_Y=16
  export VENC_Crop_W=96
  export VENC_Crop_H=64
  expected_ret=${expected_ret1}
  run_case crop_256_16_96_64

  export VENC_Crop=1
  export VENC_Crop_X=256
  export VENC_Crop_Y=32
  export VENC_Crop_W=96
  export VENC_Crop_H=64
  expected_ret=${expected_ret2}
  run_case crop_256_32_96_64

  export VENC_Crop=1
  export VENC_Crop_X=16
  export VENC_Crop_Y=16
  export VENC_Crop_W=320
  export VENC_Crop_H=256
  expected_ret=${expected_ret3}
  run_case crop_16_16_320_256
}

if true; then
codec=h264
expected_ret1=1 #Y must align 32
expected_ret2=0
expected_ret3=1 #Y must align 32
p_crop_256_16_96_64=""
p_crop_256_32_96_64="13dfddfa49f7a4985c24845ce9ea84d2"
p_crop_16_16_320_256=""
s_crop_256_16_96_64="00000000000000000000000000000000"
s_crop_256_32_96_64="00000000000000000000000000000000"
s_crop_16_16_320_256="00000000000000000000000000000000"
run_suite
fi

if true; then
codec=h265
expected_ret1=1 #w must > 128
expected_ret2=1 #w must > 128
expected_ret3=0
p_crop_256_16_96_64="710d961387de932327503c7fd68f0c88"
p_crop_256_32_96_64="6e1a38b1130fee41d74bde547605467f"
p_crop_16_16_320_256="a2078d56b2f15f3d2c0a31b135ca8bdd"
s_crop_256_16_96_64="00000000000000000000000000000000"
s_crop_256_32_96_64="00000000000000000000000000000000"
s_crop_16_16_320_256="00000000000000000000000000000000"
run_suite
fi

if true; then
codec=jpeg
expected_ret1=0
expected_ret2=0
expected_ret3=1 #X must align 256
p_crop_256_16_96_64="6b3c148bd5d3a58caea317a9f54185ac"
p_crop_256_32_96_64="35d13a22b5075154ebcf5bc2c503c663"
p_crop_16_16_320_256=""
s_crop_256_16_96_64="00000000000000000000000000000000"
s_crop_256_32_96_64="00000000000000000000000000000000"
s_crop_16_16_320_256="00000000000000000000000000000000"
run_suite
fi

fi

print_result
fi