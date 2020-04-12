/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.
  
  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (��Sigmastar Confidential Information��) by the recipient.
 Any unauthorized act including without limitation unauthorized disclosure,
 copying, use, reproduction, sale, distribution, modification, disassembling,
 reverse engineering and compiling of the contents of Sigmastar Confidential
 Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
 rights to any and all damages, losses, costs and expenses resulting therefrom.
*/
//#include "st_common.h"
#include "mi_venc.h"

#ifndef ExecFunc
#define ExecFunc(_func_, _ret_) \
        if (_func_ != _ret_)\
        {\
            printf("Test [%d]exec function failed\n", __LINE__);\
            return 1;\
        }\
        else\
        {\
            printf("Test [%d]exec function pass\n", __LINE__);\
        }
#endif

#define COLOR_NONE          "\033[0m"
#define COLOR_BLACK         "\033[0;30m"
#define COLOR_BLUE          "\033[0;34m"
#define COLOR_GREEN         "\033[0;32m"
#define COLOR_CYAN          "\033[0;36m"
#define COLOR_RED           "\033[0;31m"
#define COLOR_YELLOW        "\033[1;33m"
#define COLOR_WHITE         "\033[1;37m"

#define ST_NOP(fmt, args...)
#define ST_DBG(fmt, args...) ({do{printf(COLOR_GREEN"[DBG]:%s[%d]: "COLOR_NONE, __FUNCTION__,__LINE__);printf(fmt, ##args);}while(0);})
#define ST_WARN(fmt, args...) ({do{printf(COLOR_YELLOW"[WARN]:%s[%d]: "COLOR_NONE, __FUNCTION__,__LINE__);printf(fmt, ##args);}while(0);})
#define ST_INFO(fmt, args...) ({do{printf("[INFO]:%s[%d]: \n", __FUNCTION__,__LINE__);printf(fmt, ##args);}while(0);})
#define ST_ERR(fmt, args...) ({do{printf(COLOR_RED"[ERR]:%s[%d]: "COLOR_NONE, __FUNCTION__,__LINE__);printf(fmt, ##args);}while(0);})


// Insert User Data
MI_U8 _test_insert_data[8] = {0, 1, 2, 3, 4, 5, 6, 7};

// ROI Config
MI_VENC_RoiCfg_t _test_roi_cfg =
{
    .u32Index = 0,
    .bEnable = TRUE,
    .bAbsQp = FALSE,
    .s32Qp = -12,
    .stRect =
    {
        .u32Left = 32,
        .u32Top = 32,
        .u32Width = 256,
        .u32Height = 256,
    }
};

//IDR config
MI_BOOL g_idr_config = FALSE;
MI_BOOL _test_idr_enable = FALSE;
int _test_tmp_interval = 0, _test_idr_interval = 80;

//Slice Split
MI_VENC_ParamH265SliceSplit_t _test_split_265 =
{
    .bSplitEnable = TRUE,
    .u32SliceRowCount = 6,
};

MI_VENC_ParamH264SliceSplit_t _test_split_264 =
{
    .bSplitEnable = TRUE,
    .u32SliceRowCount = 6,
};

//Intra Pred
MI_VENC_ParamH265IntraPred_t _test_intra_pred_265 =
{
    .u32Intra32x32Penalty = 0,  //0~65535
    .u32Intra16x16Penalty = 0,  //0~65535
    .u32Intra8x8Penalty = 0,    //0~65535
};

MI_VENC_ParamH264IntraPred_t _test_intra_pred_264 =
{
    .bIntra16x16PredEn = TRUE,
    .bIntraNxNPredEn = TRUE,
    .bConstrainedIntraPredFlag = 0,
    .bIpcmEn = FALSE,
    .u32Intra16x16Penalty = 0,
    .u32Intra4x4Penalty = 0,
    .bIntraPlanarPenalty = FALSE,
};

//Inter Pred
MI_VENC_ParamH265InterPred_t _test_inter_pred_265 =
{
    .u32HWSize = 32,
    .u32VWSize = 16,
    .bInter16x16PredEn = TRUE,
    .bInter16x8PredEn = TRUE,
    .bInter8x16PredEn = TRUE,
    .bInter8x8PredEn = TRUE,
    .bExtedgeEn = FALSE,
};

MI_VENC_ParamH264InterPred_t _test_inter_pred_264 =
{
    .u32HWSize = 32,
    .u32VWSize = 16,
    .bInter16x16PredEn = TRUE,
    .bInter16x8PredEn = TRUE,
    .bInter8x16PredEn = TRUE,
    .bInter8x8PredEn = TRUE,
    .bExtedgeEn = FALSE,
};

//Trans
MI_VENC_ParamH265Trans_t _test_trans_265 =
{
    .u32IntraTransMode = 0,
    .u32InterTransMode = 0,
    .s32ChromaQpIndexOffset = 0,
};

MI_VENC_ParamH264Trans_t _test_trans_264 =
{
    .u32IntraTransMode = 0,
    .u32InterTransMode = 0,
    .s32ChromaQpIndexOffset = 0,
};

//Entropy
MI_VENC_ParamH264Entropy_t _test_entropy_264 =
{
    .u32EntropyEncModeI = 1,
    .u32EntropyEncModeP = 1,
};

//VUI
MI_VENC_ParamH264Vui_t _test_vui_264 =
{
    .stVuiAspectRatio =
    {
        .u8AspectRatioInfoPresentFlag = 0,
        .u8AspectRatioIdc = 1,
        .u8OverscanInfoPresentFlag = 0,
        .u8OverscanAppropriateFlag = 0,
        .u16SarWidth = 1,
        .u16SarHeight = 1,
    },
    .stVuiTimeInfo =
    {
        .u8TimingInfoPresentFlag = 0,
        .u8FixedFrameRateFlag = 1,
        .u32NumUnitsInTick = 1,
        .u32TimeScale = 60,
    },
    .stVuiVideoSignal =
    {
        .u8VideoSignalTypePresentFlag = 1,
        .u8VideoFormat = 5,
        .u8VideoFullRangeFlag = 1,
        .u8ColourDescriptionPresentFlag = 1,
        .u8ColourPrimaries = 1,
        .u8TransferCharacteristics = 1,
        .u8MatrixCoefficients = 1,
    },
};

MI_VENC_ParamH265Vui_t _test_vui_265 =
{
    .stVuiAspectRatio =
    {
        //.u8AspectRatioInfoPresentFlag = 0,
        //.u8AspectRatioIdc = 1,
        //.u8OverscanInfoPresentFlag = 0,
        //.u8OverscanAppropriateFlag = 0,
        .u16SarWidth = 1,
        .u16SarHeight = 1,
    },
    .stVuiTimeInfo =
    {
        .u8TimingInfoPresentFlag = 0,
        //.u8FixedFrameRateFlag = 1,
        //.u32NumUnitsInTick = 1,
        //.u32TimeScale = 60,
    },
    .stVuiVideoSignal =
    {
        .u8VideoSignalTypePresentFlag = 1,
        .u8VideoFormat = 5,
        .u8VideoFullRangeFlag = 1,
        .u8ColourDescriptionPresentFlag = 1,
        .u8ColourPrimaries = 1,
        .u8TransferCharacteristics = 1,
        .u8MatrixCoefficients = 1,
    },
};

//LTR
MI_VENC_ParamRef_t _test_ltr =
{
    .u32Base = 1,
    .u32Enhance = 49,
    .bEnablePred = FALSE,
};

//Crop
MI_VENC_CropCfg_t _test_crop =
{
    .bEnable = FALSE,
    .stRect =
    {
        .u32Left = 64,      //H264:256x, H265:32x
        .u32Top = 32,       //H264:32x, H265:2x
        .u32Width = 1024,   //H264:16x, H265:32x
        .u32Height =512,    //H264:16x, H265:32x
    }
};

//Deblocking
MI_VENC_ParamH265Dblk_t _test_dblk_265 =
{
    .disable_deblocking_filter_idc = 1,
    .slice_tc_offset_div2 = 0,
    .slice_beta_offset_div2 = 0,
};

MI_VENC_ParamH264Dblk_t _test_dblk_264 =
{
    .disable_deblocking_filter_idc = 1,
    .slice_alpha_c0_offset_div2 = 0,
    .slice_beta_offset_div2 = 0,
};
