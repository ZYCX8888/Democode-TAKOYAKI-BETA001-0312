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
export VENC_CH00_QP=20
export VENC_GLOB_AUTO_STOP=1
export VENC_GLOB_IN_FPS=30 #1000: asap
export VENC_GLOB_IN_FILE_CACHE_MB=30 #0: for non cache. 30 for FHD 10 frames.

codec=$1
of=enc_d2c00p00.es
to=/mnt/out/es
unset_script=./_unset_config.sh
#The follows are settings

if false; then
# export VENC_INTRA=1
export VENC_INTRA=0 #control app NOT to set the filter
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/VENC_INTRA0.${codec}; . ${unset_script}

export VENC_INTRA=1 #control app to set the filter
export constrained_intra_pred_flag=0
export u32Intra16x16Penalty=0
export u32Intra4x4Penalty=0
export bIntraPlanarPenalty=0
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/VENC_INTRA1.f0.16_0.4_0_p0.${codec}; . ${unset_script}

export VENC_INTRA=1 #control app to set the filter
export constrained_intra_pred_flag=1
export u32Intra16x16Penalty=0
export u32Intra4x4Penalty=0
export bIntraPlanarPenalty=0
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/VENC_INTRA1.f1.16_0.4_0_p0.${codec}; . ${unset_script}

export VENC_INTRA=1 #control app to set the filter
export constrained_intra_pred_flag=0
export u32Intra16x16Penalty=255
export u32Intra4x4Penalty=0
export bIntraPlanarPenalty=0
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/VENC_INTRA1.f0.16_FF.4_0_p0.${codec}; . ${unset_script}

export VENC_INTRA=1 #control app to set the filter
export constrained_intra_pred_flag=0
export u32Intra16x16Penalty=0
export u32Intra4x4Penalty=255
export bIntraPlanarPenalty=0
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/VENC_INTRA1.f0.16_0.4_FF_p0.${codec}; . ${unset_script}

export VENC_INTRA=1 #control app to set the filter
export constrained_intra_pred_flag=0
export u32Intra16x16Penalty=0
export u32Intra4x4Penalty=0
export bIntraPlanarPenalty=255
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/VENC_INTRA1.f0.16_0.4_0_pFF.${codec}; . ${unset_script}
fi

if true; then
# export VENC_INTER=1
export VENC_INTER=0 #control app NOT to set the filter
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/VENC_INTER0.${codec}; . ${unset_script}

export VENC_INTER=1 #control app to set the filter
export nDmv_X=32
export nDmv_Y=16
export bInter8x8PredEn=1
export bInter8x16PredEn=1
export bInter16x8PredEn=1
export bInter16x16PredEn=1
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/VENC_INTER1.x32.1111${codec}; . ${unset_script}

export VENC_INTER=1 #control app to set the filter
export nDmv_X=16
export nDmv_Y=8
export bInter8x8PredEn=1
export bInter8x16PredEn=1
export bInter16x8PredEn=1
export bInter16x16PredEn=1
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/VENC_INTER1.x16.1111${codec}; . ${unset_script}

export VENC_INTER=1 #control app to set the filter
export nDmv_X=32
export nDmv_Y=16
export bInter8x8PredEn=0
export bInter8x16PredEn=1
export bInter16x8PredEn=1
export bInter16x16PredEn=1

${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/VENC_INTER1.x32.0111${codec}; . ${unset_script}
export VENC_INTER=1 #control app to set the filter
export nDmv_X=32
export nDmv_Y=16
export bInter8x8PredEn=1
export bInter8x16PredEn=0
export bInter16x8PredEn=1
export bInter16x16PredEn=1
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/VENC_INTER1.x32.1011${codec}; . ${unset_script}

export VENC_INTER=1 #control app to set the filter
export nDmv_X=32
export nDmv_Y=16
export bInter8x8PredEn=1
export bInter8x16PredEn=1
export bInter16x8PredEn=0
export bInter16x16PredEn=1
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/VENC_INTER1.x32.1101${codec}; . ${unset_script}

export VENC_INTER=1 #control app to set the filter
export nDmv_X=32
export nDmv_Y=16
export bInter8x8PredEn=1
export bInter8x16PredEn=1
export bInter16x8PredEn=1
export bInter16x16PredEn=0
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/VENC_INTER1.x32.1110${codec}; . ${unset_script}
fi


if false; then
export Cabac=0
${exe}  1 ${codec}; mv ${out_path}/${of} ${to}/cabac0.${codec}; . ${unset_script}

export Cabac=1
${exe}  1 ${codec}; mv ${out_path}/${of} ${to}/cabac1.${codec}; . ${unset_script}


# Trans export qpoffset=0    #-12 ~ 12
export qpoffset=-12
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/qpoffset_n12.${codec}; . ${unset_script}

export qpoffset=0
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/qpoffset0.${codec}; . ${unset_script}

export qpoffset=12
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/qpoffset_p12.${codec}; . ${unset_script}
fi

if true; then
# export deblock_filter_control=1
export DEBLOCK_FILTER_CONTROL=0 #control app NOT to set the filter
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/DEBLOCK_FILTER_CONTROL0.${codec}; . ${unset_script}

export DEBLOCK_FILTER_CONTROL=1 #control app to set the filter
export disable_deblocking_filter_idc=0
export slice_alpha_c0_offset_div2=6
export slice_beta_offset_div2=-6
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/DEBLOCK_FILTER_CONTROL1.0.6.n6.${codec}; . ${unset_script}

export DEBLOCK_FILTER_CONTROL=1 #control app to set the filter
export disable_deblocking_filter_idc=1
export slice_alpha_c0_offset_div2=-5
export slice_beta_offset_div2=5
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/DEBLOCK_FILTER_CONTROL1.1.n5.5.${codec}; . ${unset_script}
fi

if true; then
# export nRows=2
export nRows=0
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/nRows0.${codec}; . ${unset_script}

export nRows=2
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/nRows2.${codec}; . ${unset_script}

export nRows=5
${exe} 1 ${codec}; mv ${out_path}/${of} ${to}/nRows5.${codec}; . ${unset_script}
fi

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

print_result
