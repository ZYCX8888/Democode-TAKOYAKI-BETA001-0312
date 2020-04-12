#!/bin/sh

. _test.sh
ret_test=$?
if [ "${ret_test}" == "0" ]; then

log "\n\n${script}: Dynamic Channel Attribute: Multi-slice\n"

expected_ret=0

#needed predefined variable in this script: $of, $codec, $unset_script, $expected_ret, $exe
#input $1: prefix of output file, output:$md5
run_case() {
  rm ${of} 2> /dev/null; #remove output file from previous test
  ${exe} 1 ${codec};     #how this program is executed.
  ret=$?                 #keep the result as run_std_case_check() input

  run_std_case_check $1

  #how result,$md5 should be checked
  run_std_md5_check $1
}


#the rest tests the CONFIG system
if true ; then
export VENC_GLOB_MAX_FRAMES=100
export VENC_GLOB_DUMP_ES=100
export VENC_CH00_QP=45 #don't care about the bit stream quality

run_suite() {
  export nRows=0
  run_case nRows0

  export nRows=2
  run_case nRows2

  export nRows=5
  run_case nRows5
}

codec=h264
p_nRows0="710d961387de932327503c7fd68f0c88"
p_nRows2="6e1a38b1130fee41d74bde547605467f"
p_nRows5="5cc40fd1e3200ab355b6b4e242e28c99"
s_nRows0="00000000000000000000000000000000"
s_nRows2="00000000000000000000000000000000"
s_nRows5="00000000000000000000000000000000"
run_suite

codec=h265
p_nRows0="f2d666073f314ef0f7ed1c2215cc9e87"
p_nRows2="a4c6abae76376b623468fd7c940709ff"
p_nRows5="f0871daf8ac6c24cbb84dd00b62215a4"
s_nRows0="00000000000000000000000000000000"
s_nRows2="00000000000000000000000000000000"
s_nRows5="00000000000000000000000000000000"
run_suite


#The follows are settings
if false; then
export Cabac=0
${exe}  1 ${codec}; mv ${out_path}/${of} ${to}/cabac0.${codec}; . ${unset_script}

export Cabac=1
${exe}  1 ${codec}; mv ${out_path}/${of} ${to}/cabac1.${codec}; . ${unset_script}


# export qpoffset=0    #-12 ~ 12
export qpoffset=-12
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/qpoffset_12.${codec}; . ${unset_script}

export qpoffset=0
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/qpoffset0.${codec}; . ${unset_script}

export qpoffset=-12
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/qpoffset12.${codec}; . ${unset_script}


# export deblock_filter_control=1
export deblock_filter_control=1
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/deblock_filter_control1.${codec}; . ${unset_script}

export deblock_filter_control=0
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/deblock_filter_control0.${codec}; . ${unset_script}


# export disable_deblocking_filter_idc=1
export disable_deblocking_filter_idc=1
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/disable_deblocking_filter_idc1.${codec}; . ${unset_script}

export disable_deblocking_filter_idc=0
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/disable_deblocking_filter_idc0.${codec}; . ${unset_script}


# export slice_alpha_c0_offset_div2=-16
export slice_alpha_c0_offset_div2=-16
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/slice_alpha_c0_offset_div2_n16.${codec}; . ${unset_script}

export slice_alpha_c0_offset_div2=0
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/slice_alpha_c0_offset_div2_0.${codec}; . ${unset_script}

export slice_alpha_c0_offset_div2=16
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/slice_alpha_c0_offset_div2_p16.${codec}; . ${unset_script}


# export slice_beta_offset_div2=-6
export slice_beta_offset_div2=-6
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/slice_beta_offset_div2_n6.${codec}; . ${unset_script}

export slice_beta_offset_div2=0
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/slice_beta_offset_div2_0.${codec}; . ${unset_script}

export slice_beta_offset_div2=6
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/slice_beta_offset_div2_p6.${codec}; . ${unset_script}
# export nRows=2
export nRows=0
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/nRows0.${codec}; . ${unset_script}
fi

#export nRows=2
#${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/nRows2.${codec}; . ${unset_script}

#export nRows=5
#${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/nRows5.${codec}; . ${unset_script}

fi

print_result
fi