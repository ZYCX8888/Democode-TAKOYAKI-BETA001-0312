#!/bin/sh

. _test.sh

script=$(basename "$0")
script="${script%.*}"

#debug_level 3 would cause much delay and causing MI gets every interrupts.
#use debug_level 0 to make
echo 2 > $dbg_level #/sys/module/mi_venc/parameters/debug_level

log "\n\n${script}: Test Encoder feature setting system\n"
#if this passed, then run the next case to generate more stream

#${exe} 1 dummy
#ret=$?
#test "The program returns ${ret}." `expr "${ret}" == "0"`


#the rest tests the CONFIG system
if true ; then
#export VENC_GLOB_IN_FILE="/tmp/YUV420SP_720_480.yuv"
#export VENC_GLOB_IN_W=736
#export VENC_GLOB_IN_H=480
#export VENC_GLOB_IN_FILE="/tmp/YUV420SP_1920_1088.yuv"
#export VENC_GLOB_IN_W=1920
#export VENC_GLOB_IN_H=1088
export VENC_GLOB_IN_FILE=""
export VENC_GLOB_IN_W=352
export VENC_GLOB_IN_H=288
export VENC_GLOB_MAX_FRAMES=100
export VENC_GLOB_DUMP_ES=100
export VENC_GLOB_IN_MSG_LEVEL=0
export VENC_CH00_QP=45
export VENC_GLOB_AUTO_STOP=1
export VENC_GLOB_IN_FPS=30 #1000: asap
export VENC_GLOB_IN_FILE_CACHE_MB=30 #0: for non cache. 30 for FHD 10 frames.
export VENC_GLOB_APPLY_CFG=1 #make something like nRows to take effect

codec=h264
of=enc_d2c00.es
to=/mnt/out/es
unset_script=./_unset_config.sh
#The follows are settings
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

export nRows=2
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/nRows2.${codec}; . ${unset_script}

export nRows=5
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/nRows5.${codec}; . ${unset_script}

# export nDmv_X=32
# export nDmv_Y=16
# export bInter8x8PredEn=
# export bInter8x16PredEn=
# export bInter16x8PredEn
# export bInter16x16PredEn
# export bIntra16x16PredEn
# export constrained_intra_pred_flag
# export u32Intra16x16Penalty
# export u32Intra4x4Penalty
# export bIntraPlanarPenalty


#echo before cabac=${Cabac}.
#. ./_unset_config.sh
#echo after cabac=${Cabac}.
#rm -f $result
#${exe}  1 h264
#ret=$?
#test "The program returns ${ret}." `expr "${ret}" == "0"`

#H.264 174 mpix h.265 138 mpix.
#eval `cat ${result}`
#test "${FPSx100} FPSx100 meets requirement: " `expr "${FPSx100}" \>= "7500"`

#device2.ch00.ElementaryStream
#result=`md5sum $out_path/enc2.00.es | cut -c-32`
#test "Check enc2.00.es" `expr "${result}" == "4c31debc017d23b2ec80e4868ec9f4b4"`

#result=`md5sum $out_path/enc2.01.es | cut -c-32`
#test "Check enc2.01.es" `expr "${result}" == "06146b0e937279e05fdfc4e80c16d3de"`
fi

print_result
