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
//==============================================================================
//
//  File        : mi_mve.h
//  Description : Intelligent Video Engine
//  Author      : Hungpin Huang
//  Revision    : 1.0
//
//==============================================================================
#ifndef MI_MVE_H
#define MI_MVE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MVE_MAX_REGION_NUM    255

typedef void* MVE_HANDLE;
#define MVE_MAP_NUM 256
#define MVE_HIST_NUM 256

//==============================================================================
//
//                              DATA TYPE
//
//==============================================================================
typedef uint8_t       fixu0q8_t;
typedef uint16_t      fixu0q16_t;
typedef uint16_t      fixu8q8_t;
typedef int16_t       fixs9q7_t;
typedef int16_t       fixs1q15_t;
typedef uint32_t      fixu22q10_t;
typedef int32_t       fixs25q7_t;


//==============================================================================
//
//                              ENUMERATIONS
//
//==============================================================================
typedef enum _MI_MVE_RET
{
    MI_MVE_RET_SUCCESS                      = 0x00000000,   /*MVE execution success*/
    MI_MVE_RET_INIT_ERROR                   = 0x10000101,   /*MVE init error*/
    MI_MVE_RET_IC_CHECK_ERROR               = 0x10000102,   /*MVE platform check error*/
    MI_MVE_RET_INVALID_HANDLE               = 0x10000103,   /*Invalid MVE handle*/
    MI_MVE_RET_INVALID_PARAMETER            = 0x10000104,   /*Invalid MVE parameter*/
    MI_MVE_RET_INVALID_ADDRESS              = 0x10000105,   /*Invalid buffer address*/
    MI_MVE_RET_NOT_SUPPORT                  = 0x10000106,   /*Unsupported feature of MVE*/
    MI_MVE_RET_HARDWARE_TIMEOUT             = 0x10000107,   /*MVE hardware timeout*/
    MI_MVE_RET_HARDWARE_ERROR               = 0x10000108,   /*MVE hardware error*/
    MI_MVE_RET_HARDWARE_BUSY                = 0x10000109,   /*MVE hardware busy*/
    MI_MVE_RET_INVALID_BUFFER_NAME          = 0x1000010A,   /*Invalid buffer name when allocating MVE buffer*/
    MI_MVE_RET_MALLOC_ERROR                 = 0x1000010B    /*Allocate MVE buffer error*/
} MI_MVE_RET;

typedef enum _MVE_IMAGE_TYPE_E
{
    MVE_IMAGE_TYPE_U8C1                         = 0x00,
    MVE_IMAGE_TYPE_S8C1                         = 0x01,
    MVE_IMAGE_TYPE_YUV420SP                     = 0x02, /*YUV420 SemiPlanar*/
    MVE_IMAGE_TYPE_YUV422SP                     = 0x03, /*YUV422 SemiPlanar*/
    MVE_IMAGE_TYPE_YUV420P                      = 0x04, /*YUV420 Planar */
    MVE_IMAGE_TYPE_YUV422P                      = 0x05, /*YUV422 planar */
    MVE_IMAGE_TYPE_S8C2_PACKAGE                 = 0x06,
    MVE_IMAGE_TYPE_S8C2_PLANAR                  = 0x07,
    MVE_IMAGE_TYPE_S16C1                        = 0x08,
    MVE_IMAGE_TYPE_U16C1                        = 0x09,
    MVE_IMAGE_TYPE_U8C3_PACKAGE                 = 0x0A,
    MVE_IMAGE_TYPE_U8C3_PLANAR                  = 0x0B,
    MVE_IMAGE_TYPE_S32C1                        = 0x0C,
    MVE_IMAGE_TYPE_U32C1                        = 0x0D,
    MVE_IMAGE_TYPE_S64C1                        = 0x0E,
    MVE_IMAGE_TYPE_U64C1                        = 0x0F,
    MVE_IMAGE_TYPE_BUTT                         = 0xFF
} MVE_IMAGE_TYPE_E;

typedef enum _MVE_MEM_ALLOC_TYPE_E
{
    MVE_MEM_ALLOC_TYPE_VIRTUAL          = 0x0,
    MVE_MEM_ALLOC_TYPE_PHYSICAL         = 0x1
} MVE_MEM_ALLOC_TYPE_E;

typedef enum _MVE_CSC_MODE_E
{
    MVE_CSC_MODE_PIC_BT601_YUV2RGB              = 0x00, /*CSC: YUV2RGB, picture transfer mode, RGB value range [0, 255]*/
    MVE_CSC_MODE_PIC_BT601_RGB2YUV              = 0x01, /*CSC: RGB2YUV, picture transfer mode, Y:[16, 235],U/V:[16, 240]*/
    MVE_CSC_MODE_BUTT                           = 0xFF
} MVE_CSC_MODE_E;

typedef enum _MVE_SOBEL_OUT_CTRL_E
{
      MVE_SOBEL_OUT_CTRL_BOTH                   = 0x00, /*Output horizontal and vertical*/
      MVE_SOBEL_OUT_CTRL_HOR                    = 0x01, /*Output horizontal*/
      MVE_SOBEL_OUT_CTRL_VER                    = 0x02, /*Output vertical*/
      MVE_SOBEL_OUT_CTRL_BUTT                   = 0xFF
} MVE_SOBEL_OUT_CTRL_E;

typedef enum _MVE_THRESH_MODE_E
{
    MVE_THRESH_MODE_BINARY                      = 0x00, /*srcVal <= lowThr, dstVal = minVal; srcVal > lowThr, dstVal = maxVal.*/
    MVE_THRESH_MODE_TRUNC                       = 0x01, /*srcVal <= lowThr, dstVal = srcVal; srcVal > lowThr, dstVal = maxVal.*/
    MVE_THRESH_MODE_TO_MINVAL                   = 0x02, /*srcVal ? lowThr, dstVal =  minVal; srcVal > lowThr, dstVal = srcVal.*/
    MVE_THRESH_MODE_MIN_MID_MAX                 = 0x03, /*srcVal ? lowThr, dstVal =  minVal; lowThr < srcVal <= highThr, dstVal = midVal; srcVal > highThr, dstVal = maxVal.*/
    MVE_THRESH_MODE_ORI_MID_MAX                 = 0x04, /*srcVal ? lowThr, dstVal =  srcVal; lowThr < srcVal <= highThr, dstVal = midVal; srcVal > highThr, dstVal = maxVal.*/
    MVE_THRESH_MODE_MIN_MID_ORI                 = 0x05, /*srcVal ? lowThr, dstVal =  minVal; lowThr < srcVal <= highThr, dstVal = midVal; srcVal > highThr, dstVal = srcVal.*/
    MVE_THRESH_MODE_MIN_ORI_MAX                 = 0x06, /*srcVal ? lowThr, dstVal =  minVal; lowThr < srcVal <= highThr, dstVal = srcVal; srcVal > highThr, dstVal = maxVal.*/
    MVE_THRESH_MODE_ORI_MID_ORI                 = 0x07, /*srcVal ? lowThr, dstVal =  srcVal; lowThr < srcVal <= highThr, dstVal = midVal; srcVal > highThr, dstVal = srcVal.*/
    MVE_THRESH_MODE_BUTT                        = 0xFF
} MVE_THRESH_MODE_E;

typedef enum _MVE_MAG_AND_ANG_OUT_CTRL_E
{
MVE_MAG_AND_ANG_OUT_CTRL_MAG                    = 0x00,
MVE_MAG_AND_ANG_OUT_CTRL_MAG_AND_ANG            = 0x01,
MVE_MAG_AND_ANG_OUT_CTRL_BUTT                   = 0xFF
} MVE_MAG_AND_ANG_OUT_CTRL_E;


typedef enum _MVE_SUB_MODE_E
{
    MVE_SUB_MODE_ABS                            = 0x00, /*Absolute value of the difference*/
    MVE_SUB_MODE_SHIFT                          = 0x01, /*The output result is obtained by shifting the result one digit right to reserve the signed bit.*/
    MVE_SUB_MODE_BUTT                           = 0xFF
} MVE_SUB_MODE_E;

typedef enum _MVE_THRESH_S16_MODE_E
{
    MVE_THRESH_S16_MODE_S16_TO_S8_MIN_MID_MAX   = 0x00,
    MVE_THRESH_S16_MODE_S16_TO_S8_MIN_ORI_MAX   = 0x01,
    MVE_THRESH_S16_MODE_S16_TO_U8_MIN_MID_MAX   = 0x02,
    MVE_THRESH_S16_MODE_S16_TO_U8_MIN_ORI_MAX   = 0x03,
    MVE_THRESH_S16_MODE_BUTT                    = 0xFF
} MVE_THRESH_S16_MODE_E;

typedef enum _MVE_THRESH_U16_MODE_E
{
    MVE_THRESH_U16_MODE_U16_TO_U8_MIN_MID_MAX   = 0x00,
    MVE_THRESH_U16_MODE_U16_TO_U8_MIN_ORI_MAX   = 0x01,
    MVE_THRESH_U16_MODE_BUTT                    = 0xFF
} MVE_THRESH_U16_MODE_E;

typedef enum _MVE_16BIT_TO_8BIT_MODE_E
{
    MVE_16BIT_TO_8BIT_MODE_S16_TO_S8            = 0x00,
    MVE_16BIT_TO_8BIT_MODE_S16_TO_U8_ABS        = 0x01,
    MVE_16BIT_TO_8BIT_MODE_S16_TO_U8_BIAS       = 0x02,
    MVE_16BIT_TO_8BIT_MODE_U16_TO_U8            = 0x03,
    MVE_16BIT_TO_8BIT_MODE_BUTT                 = 0xFF
} MVE_16BIT_TO_8BIT_MODE_E;

typedef enum _MVE_ORD_STAT_FILTER_MODE_E
{
    MVE_ORD_STAT_FILTER_MODE_MEDIAN             = 0x00,
    MVE_ORD_STAT_FILTER_MODE_MAX                = 0x01,
    MVE_ORD_STAT_FILTER_MODE_MIN                = 0x02,
    MVE_ORD_STAT_FILTER_MODE_BUTT               = 0xFF
} MVE_ORD_STAT_FILTER_MODE_E;

typedef enum _MVE_BERNSEN_MODE_E
{
    MVE_BERNSEN_MODE_NORMAL                     = 0x00,
    MVE_BERNSEN_MODE_THRESH                     = 0x01,
    MVE_BERNSEN_MODE_BUTT                       = 0xFF
} MVE_BERNSEN_MODE_E;

typedef enum _MVE_INTEG_OUT_CTRL_E
{
    MVE_INTEG_OUT_CTRL_COMBINE                  = 0x00,
    MVE_INTEG_OUT_CTRL_SUM                      = 0x01,
    MVE_INTEG_OUT_CTRL_SQSUM                    = 0x02,
    MVE_INTEG_OUT_CTRL_BUTT                     = 0xFF
} MVE_INTEG_OUT_CTRL_E;

typedef enum _MVE_SAD_OUT_CTRL_E
{
    MVE_SAD_OUT_CTRL_16BIT_BOTH                 = 0x00,
    MVE_SAD_OUT_CTRL_8BIT_BOTH                  = 0x01,
    MVE_SAD_OUT_CTRL_16BIT_SAD                  = 0x02,
    MVE_SAD_OUT_CTRL_8BIT_SAD                   = 0x03,
    MVE_SAD_OUT_CTRL_THRESH                     = 0x04,
    MVE_SAD_OUT_CTRL_BUTT                       = 0xFF
} MVE_SAD_OUT_CTRL_E;

typedef enum _MVE_SAD_MODE_E
{
    MVE_SAD_MODE_MB_4X4                         = 0x00, /*4x4*/
    MVE_SAD_MODE_MB_8X8                         = 0x01, /*8x8*/
    MVE_SAD_MODE_MB_16X16                       = 0x02, /*16x16*/
    MVE_SAD_MODE_BUTT                           = 0xFF
} MVE_SAD_MODE_E;

typedef enum _MVE_NORM_GRAD_OUT_CTRL_E
{
    MVE_NORM_GRAD_OUT_CTRL_HOR_AND_VER          = 0x00,
    MVE_NORM_GRAD_OUT_CTRL_HOR                  = 0x01,
    MVE_NORM_GRAD_OUT_CTRL_VER                  = 0x02,
    MVE_NORM_GRAD_OUT_CTRL_COMBINE              = 0X03,
    MVE_NORM_GRAD_OUT_CTRL_BUTT                 = 0xFF
} MVE_NORM_GARD_OUT_CTRL_E;

typedef enum _MVE_SVM_TYPE_E
{
    MVE_SVM_TYPE_C_SVC                          = 0x00,
    MVE_SVM_TYPE_NU_SVC                         = 0x01,
    MVE_SVM_TYPE_BUTT                           = 0xFF
} MVE_SVM_TYPE_E;

typedef enum _MVE_SVM_KERNEL_TYPE_E
{
    MVE_SVM_KERNEL_TYPE_LINEAR                  = 0x00,
    MVE_SVM_KERNEL_TYPE_POLY                    = 0x01,
    MVE_SVM_KERNEL_TYPE_RBF                     = 0x02,
    MVE_SVM_KERNEL_TYPE_SIGMOID                 = 0x03,
    MVE_SVM_KERNEL_TYPE_BUTT                    = 0xFF
} MVE_SVM_KERNEL_TYPE_E;

typedef enum _MVE_LBP_CMP_MODE_E
{
    MVE_LBP_CMP_MODE_NORMAL                     = 0x00,
    MVE_LBP_CMP_MODE_ABS                        = 0x01,
    MVE_LBP_CMP_MODE_BUTT                       = 0xFF
} MVE_LBP_CMP_MODE_E;

typedef enum _MVE_ANN_MLP_ACTIV_FUNC_E
{
    MVE_ANN_MLP_ACTIV_FUNC_IDENTITY     = 0x0,
    MVE_ANN_MLP_ACTIV_FUNC_SIGMOID_SYM  = 0x1,
    MVE_ANN_MLP_ACTIV_FUNC_GAUSSIAN     = 0x2,

    MVE_ANN_MLP_ACTIV_FUNC_BUTT
}MVE_ANN_MLP_ACTIV_FUNC_E;

//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================
typedef struct _MVE_IMG_INFO
{
    int16_t width;
    int16_t height;
} MVE_IMG_INFO;

typedef struct _MVE_IMAGE_S
{
    MVE_IMAGE_TYPE_E enType;
    void   *pu32PhyAddr[3];
    uint8_t  *pu8Addr[3];
    uint16_t u16Stride[3];
    uint16_t u16Width;
    uint16_t u16Height;
    uint16_t u16Reserved;     /*Can be used such as elemSize*/
} MVE_IMAGE_S;

typedef MVE_IMAGE_S MVE_SRC_IMAGE_S;
typedef MVE_IMAGE_S MVE_DST_IMAGE_S;

typedef struct _MVE_FILTER_CTRL_S
{
    int8_t as8Mask[25];      /*Template parameter filter coefficient*/
    uint8_t u8Norm;           /*Normalization parameter, by right shift*/
} MVE_FILTER_CTRL_S;


typedef struct _MVE_CSC_CTRL_S
{
    MVE_CSC_MODE_E enMode;  /*Working mode*/
} MVE_CSC_CTRL_S;

typedef struct _MVE_FILTER_AND_CSC_CTRL_S
{
    MVE_CSC_MODE_E enMode;  /*CSC working mode*/
    int8_t  as8Mask[25];     /*Template parameter filter coefficient*/
    uint8_t  u8Norm;          /*Normalization parameter, by right shift*/
} MVE_FILTER_AND_CSC_CTRL_S;

typedef struct _MVE_SOBEL_CTRL_S
{
    MVE_SOBEL_OUT_CTRL_E enOutCtrl; /*Output format*/
    int8_t as8Mask[25];              /*Template parameter*/
} MVE_SOBEL_CTRL_S;

typedef struct _MVE_DILATE_CTRL_S
{
    uint8_t au8Mask[25];  /*The template parameter value must be 0 or 255.*/
} MVE_DILATE_CTRL_S;

typedef struct _MVE_ERODE_CTRL_S
{
    uint8_t au8Mask[25];  /*The template parameter value must be 0 or 255.*/
} MVE_ERODE_CTRL_S;

typedef struct _MVE_THRESH_CTRL_S
{
    MVE_THRESH_MODE_E enMode;
    uint8_t u8LowThr;             /*user-defined threshold, 0 <= u8LowThr <= 255*/
    uint8_t u8HighThr;            /*user-defined threshold, if enMode < MVE_THRESH_MODE_MIN_MID_MAX, u8HighThr is not used, else 0 <= u8LowThr <=    u8HighThr <= 255;*/
    uint8_t u8MinVal;             /*Minimum value when tri-level thresholding*/
    uint8_t u8MidVal;             /*Middle value when tri-level thresholding, if    enMode < 2, u32MidVal is not used; */
    uint8_t u8MaxVal;             /*Maximum value when tri-level thresholding*/
} MVE_THRESH_CTRL_S;

typedef struct _MVE_MAG_AND_ANG_CTRL_S
{
    MVE_MAG_AND_ANG_OUT_CTRL_E enOutCtrl;
    uint16_t u16Thr;
    int8_t  as8Mask[25];
} MVE_MAG_AND_ANG_CTRL_S;

typedef struct _MVE_SUB_CTRL_S
{
    MVE_SUB_MODE_E enMode;
} MVE_SUB_CTRL_S;

typedef struct _MVE_ADD_CTRL_S
{
    fixu0q16_t u0q16X;    /*x of "xA+yB"*/
    fixu0q16_t u0q16Y;    /*y of "xA+yB"*/
} MVE_ADD_CTRL_S;

typedef union _MVE_8BIT_U
{
    int8_t s8Val;
    uint8_t u8Val;
} MVE_8BIT_U;

typedef struct _MVE_THRESH_S16_CTRL_S
{
    MVE_THRESH_S16_MODE_E enMode;
    int16_t s16LowThr;               /*user-defined threshold*/
    int16_t s16HighThr;              /*user-defined threshold*/
    MVE_8BIT_U un8MinVal;           /*Minimum value when tri-level thresholding*/
    MVE_8BIT_U un8MidVal;           /*Middle value when tri-level thresholding*/
    MVE_8BIT_U un8MaxVal;           /*Maximum value when tri-level thresholding*/
} MVE_THRESH_S16_CTRL_S;

typedef struct _MVE_THRESH_U16_CTRL_S
{
    MVE_THRESH_U16_MODE_E enMode;
    uint16_t u16LowThr;
    uint16_t u16HighThr;
    uint8_t u8MinVal;
    uint8_t u8MidVal;
    uint8_t u8MaxVal;
} MVE_THRESH_U16_CTRL_S;

typedef struct _MVE_16BIT_TO_8BIT_CTRL_S
{
    MVE_16BIT_TO_8BIT_MODE_E enMode;
    uint16_t u16Denominator;
    uint8_t u8Numerator;
    int8_t s8Bias;
} MVE_16BIT_TO_8BIT_CTRL_S;

typedef struct _MVE_ORD_STAT_FILTER_CTRL_S
{
    MVE_ORD_STAT_FILTER_MODE_E enMode;
} MVE_ORD_STAT_FILTER_CTRL_S;

typedef struct _MVE_BERNSEN_CTRL_S
{
    MVE_BERNSEN_MODE_E enMode;
    uint8_t u8WinSize;    /*3 or 5*/
    uint8_t u8Thr;
} MVE_BERNSEN_CTRL_S;

typedef struct _MVE_CCL_CTRL_S
{
    uint16_t u16InitAreaThr;  /*Init threshold of region area*/
    uint16_t u16Step;         /*Increase area step for once*/
} MVE_CCL_CTRL_S;

typedef struct _MVE_REGION_S
{
    uint32_t u32Area;         /*Represented by the pixel number*/
    uint16_t u16Left;         /*Circumscribed rectangle left border*/
    uint16_t u16Right;        /*Circumscribed rectangle right border*/
    uint16_t u16Top;          /*Circumscribed rectangle top border*/
    uint16_t u16Bottom;       /*Circumscribed rectangle bottom border*/
} MVE_REGION_S;

typedef struct _MVE_CCBLOB_S
{
    uint16_t u16CurAreaThr;                       /*Threshold of the result regions' area*/
    int8_t  s8LabelStatus;                       /*-1: Labeled failed; 0: Labeled successfully*/
    uint8_t  u8RegionNum;                         /*Number of valid region, non-continuous stored*/
    MVE_REGION_S astRegion[MVE_MAX_REGION_NUM]; /*Valid regions with 'u32Area>0' and 'label = ArrayIndex+1'*/
} MVE_CCBLOB_S;

typedef struct _MVE_INTEG_CTRL_S
{
    MVE_INTEG_OUT_CTRL_E enOutCtrl;
} MVE_INTEG_CTRL_S;

typedef struct _MVE_SAD_CTRL_S
{
    MVE_SAD_MODE_E enMode;
    MVE_SAD_OUT_CTRL_E enOutCtrl;
    uint16_t u16Thr;                  /*srcVal <= u16Thr, dstVal = minVal; srcVal > u16Thr, dstVal = maxVal.*/
    uint8_t u8MinVal;                 /*Min value*/
    uint8_t u8MaxVal;                 /*Max value*/
} MVE_SAD_CTRL_S;

typedef struct _MVE_NCC_DST_MEM_S
{
    uint64_t u64Numerator;
    uint64_t u64QuadSum1;
    uint64_t u64QuadSum2;
} MVE_NCC_DST_MEM_S;

typedef struct _MVE_MAP_LUT_MEM_S
{
    uint8_t  au8Map[MVE_MAP_NUM];
} MVE_MAP_LUT_MEM_S;

typedef struct _MVE_MEM_INFO_S
{
    void  *pu32PhyAddr;
    uint8_t  *pu8VirAddr;
    uint32_t  u32Size;
} MVE_MEM_INFO_S;

typedef MVE_MEM_INFO_S MVE_SRC_MEM_INFO_S;
typedef MVE_MEM_INFO_S MVE_DST_MEM_INFO_S;

typedef struct _MVE_HIST_MEM_S
{
    uint32_t  au32Hist[MVE_HIST_NUM];
} MVE_HIST_TABLE_MEM_S;

typedef struct _MVE_EQUALIZE_HIST_CTRL_S
{
    MVE_MEM_INFO_S stMem;
} MVE_EQUALIZE_HIST_CTRL_S;

typedef struct _MVE_EQUALIZE_HIST_CTRL_MEM_S
{
    uint32_t au32Hist[MVE_HIST_NUM];
    uint8_t au8Map[MVE_MAP_NUM];
} MVE_EQUALIZE_HIST_CTRL_MEM_S;

typedef struct _MVE_NORM_GARD_CTRL_S
{
    MVE_NORM_GARD_OUT_CTRL_E enOutCtrl;
    int8_t as8Mask[25];
    uint8_t u8Norm;
} MVE_NORM_GRAD_CTRL_S;

typedef struct _MVE_CANNY_HYS_EDGE_CTRL_S
{
    MVE_MEM_INFO_S stMem;
    uint16_t u16LowThr;
    uint16_t u16HighThr;
    int8_t as8Mask[25];
} MVE_CANNY_HYS_EDGE_CTRL_S;

typedef struct _MVE_POINT_U16_S
{
    uint16_t u16X;
    uint16_t u16Y;
} MVE_POINT_U16_S;

typedef struct _MVE_CANNY_STACK_SIZE_S
{
    uint32_t u32StackSize;    /*Stack size for output*/
    uint8_t u8Reserved[12];   /*For 16 byte align*/
} MVE_CANNY_STACK_SIZE_S;

typedef struct _MVE_LOOK_UP_TABLE_S
{
    MVE_MEM_INFO_S stTable;
    uint16_t u16ElemNum;      /*LUT's elements number*/
    uint8_t u8TabInPreci;
    uint8_t u8TabOutNorm;
    int32_t s32TabInLower;   /*LUT's original input lower limit*/
    int32_t s32TabInUpper;   /*LUT's original input upper limit*/
} MVE_LOOK_UP_TABLE_S;

typedef struct _MVE_SVM_MODEL_S
{
    MVE_SVM_TYPE_E enType;
    MVE_SVM_KERNEL_TYPE_E enKernelType;
    MVE_MEM_INFO_S  stSv;               /*SV memory*/
    MVE_MEM_INFO_S  stDf;               /*Decision functions memory*/
    uint32_t u32TotalDfSize;              /*All decision functions coef size in byte*/

    uint16_t u16FeatureDim;
    uint16_t u16SvTotal;
    uint8_t  u8ClassCount;
} MVE_SVM_MODEL_S;

typedef struct _MVE_LBP_CTRL_S
{
    MVE_LBP_CMP_MODE_E enMode;
    MVE_8BIT_U un8BitThr;
} MVE_LBP_CTRL_S;

typedef struct _MVE_GMM_CTRL_S
{
    fixu22q10_t     u22q10NoiseVar;   /*Initial noise Variance*/
    fixu22q10_t     u22q10MaxVar;     /*Max Variance*/
    fixu22q10_t     u22q10MinVar;     /*Min Variance*/
    fixu0q16_t     u0q16LearnRate;    /*Learning rate*/
    fixu0q16_t     u0q16BgRatio;      /*Background ratio*/
    fixu8q8_t     u8q8VarThr;         /*Variance Threshold*/
    fixu0q16_t     u0q16InitWeight;   /*Initial Weight*/
    uint8_t         u8ModelNum;       /*Model number: 3 or 5*/
} MVE_GMM_CTRL_S;

typedef struct _MVE_LINE_FILTER_HOR_CTRL_S
{
    uint8_t u8GapMinLen;
    uint8_t u8DensityThr;
    uint8_t u8HorThr;
} MVE_LINE_FILTER_HOR_CTRL_S;

typedef struct _MVE_LINE_FILTER_VER_CTRL_S
{
    uint8_t u8VerThr;
} MVE_LINE_FILTER_VER_CTRL_S;

typedef struct _MVE_NOISE_REMOVE_HOR_CTRL_S
{
    uint8_t u8HorThr;
    uint8_t u8HorThrMax;
} MVE_NOISE_REMOVE_HOR_CTRL_S;

typedef struct _MVE_NOISE_REMOVE_VER_CTRL_S
{
    uint8_t u8VerThr;
    uint8_t u8VerThrMax;
} MVE_NOISE_REMOVE_VER_CTRL_S;

typedef struct _MVE_ADP_THRESH_CTRL_S
{
    uint8_t u8RateThr;
    uint8_t u8HalfMaskx;
    uint8_t u8HalfMasky;
    int8_t s8Offset;
    uint8_t u8ValueThr;
} MVE_ADP_THRESH_CTRL_S;

typedef struct _MVE_RADON_CTRL_S
{
    uint8_t u8Theta;  /* Radon transform angle(range: 1 to 16, default: 8)*/
    uint8_t u8Thr;    /* Gray threshold(range:1 to 255, default: 80) */
} MVE_RADON_CTRL_S;

typedef struct _MVE_POINT_S25Q7_S
{
    fixs25q7_t s25q7X;  /*X coordinate*/
    fixs25q7_t s25q7Y;  /*Y coordinate*/
}  MVE_POINT_S25Q7_S;

typedef struct _MVE_MV_S9Q7_S
{
    int32_t s32Status;  /* Result of tracking: 0-success; -1-failure */
    fixs9q7_t s9q7Dx;    /* X-direction component of the movement */
    fixs9q7_t s9q7Dy;    /* Y-direction component of the movement */
}  MVE_MV_S9Q7_S;

typedef struct _MVE_LK_OPTICAL_FLOW_CTRL_S
{
    uint16_t  u16CornerNum;   /* maxCorners, Number of the feature points, [1, 200] */
    fixu0q8_t u0q8MinEigThr;  /* QualityLevel, minEigenvalue, Minimum eigenvalue threshold, [1, 255] */
    uint8_t   u8IterCount;    /* maxIterations, Maximum iteration times, [1, 20] */
    fixu0q8_t u0q8Epsilon;    /* minDisplacement, Threshold of iteration for dx^2 + dy^2 < u0q8Epsilon, [1, 255] */
    uint16_t  u16PyrLevel;    /* Change nPyramidLevels and subsampling */
}  MVE_LK_OPTICAL_FLOW_CTRL_S;

typedef struct _MVE_ST_CORNER_CTRL_S
{
    uint16_t maxCorners;          /* u16CornerNum, Number of the feature points, [1, 200] */
    uint16_t minDistance;         /* Minimum distance between two adjacent corner points, [1, 255] */
}  MVE_ST_CORNER_CTRL_S;

typedef struct _MVE_ANN_MLP_MODEL_S
{
    MVE_ANN_MLP_ACTIV_FUNC_E enActivFunc;
    MVE_MEM_INFO_S stWeight;
    uint32_t u32TotalWeightSize;
    uint16_t au16LayerCount[8];    /*8 layers, including input and output layers, every layerCount ??256*/
    uint16_t u16MaxCount;          /*MaxCount ??256*/
    uint8_t u8LayerNum;         /*2 < layerNum ??8*/
}MVE_ANN_MLP_MODEL_S;

typedef enum miMVE_BAT_OUT_CTRL_E
{
    MVE_BAT_OUT_CTRL_BOTH = 0x0,  /*Output horizontal and vertical*/
    MVE_BAT_OUT_CTRL_HOR  = 0x1,  /*Output horizontal*/
    MVE_BAT_OUT_CTRL_VER  = 0x2,  /*Output vertical*/
    MVE_BAT_OUT_CTRL_BUTT = 0xFF
} MVE_BAT_OUT_CTRL_E;

typedef struct _MVE_BAT_CTRL_S
{
    MVE_BAT_OUT_CTRL_E enMode;
    uint16_t u16HorTimes;
    uint16_t u16VerTimes;
} MVE_BAT_CTRL_S;

typedef enum _MVE_RESIZE_MODE_E
{
    MVE_RESIZE_TYPE_U8C1             = 0x0,
    MVE_RESIZE_TYPE_U8C3_PLANAR      = 0x1,
    MVE_RESIZE_TYPE_U8C3_PACKAGE     = 0x2,
    MVE_RESIZE_TYPE_YUV420SP         = 0x3,
    MVE_RESIZE_TYPE_BUTT             = 0xFF
}MVE_RESIZE_MODE_E;

typedef struct _MVE_RESIZE_CTRL_S
{
    MVE_RESIZE_MODE_E enMode;    /*Input and output mode*/
} MVE_RESIZE_CTRL_S;

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

MVE_HANDLE MI_MVE_Init(MVE_IMG_INFO *pstImgInfo);
MI_MVE_RET MI_MVE_Uninit(MVE_HANDLE pMveHandle);

MI_MVE_RET MI_MVE_AllocateImage(MVE_HANDLE pMveHandle, MVE_IMAGE_S *pstImage, MVE_IMAGE_TYPE_E enImageType, uint16_t u16Stride, uint16_t u16Width, uint16_t u16Height, const char *pszName, MVE_MEM_ALLOC_TYPE_E enAllocType);
MI_MVE_RET MI_MVE_FreeImage(MVE_HANDLE pMveHandle, MVE_IMAGE_S *pstImage);
MI_MVE_RET MI_MVE_AllocateBuffer(MVE_HANDLE pMveHandle, MVE_MEM_INFO_S *pstBuffer, int32_t u32Size, const char *pszName, MVE_MEM_ALLOC_TYPE_E enAllocType);
MI_MVE_RET MI_MVE_FreeBuffer(MVE_HANDLE pMveHandle, MVE_MEM_INFO_S *pstBuffer);

MI_MVE_RET MI_MVE_Filter(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc,MVE_DST_IMAGE_S *pstDst, MVE_FILTER_CTRL_S *pstFltCtrl, bool bInstant);
MI_MVE_RET MI_MVE_FilterAndCSC(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S*pstSrc, MVE_DST_IMAGE_S *pstDst, MVE_FILTER_AND_CSC_CTRL_S *pstFltCscCtrl, bool bInstant);
MI_MVE_RET MI_MVE_CSC(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc,MVE_DST_IMAGE_S *pstDst, MVE_CSC_CTRL_S *pstCscCtrl, bool bInstant);
MI_MVE_RET MI_MVE_Sobel(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc,MVE_DST_IMAGE_S *pstDstH, MVE_DST_IMAGE_S *pstDstV, MVE_SOBEL_CTRL_S *pstSobelCtrl, bool bInstant);
MI_MVE_RET MI_MVE_MagAndAng(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc, MVE_DST_IMAGE_S *pstDstMag, MVE_DST_IMAGE_S *pstDstAng, MVE_MAG_AND_ANG_CTRL_S *pstMagAndAngCtrl, bool bInstant);
MI_MVE_RET MI_MVE_Dilate(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc, MVE_DST_IMAGE_S *pstDst, MVE_DILATE_CTRL_S *pstDilateCtrl,bool bInstant);
MI_MVE_RET MI_MVE_Erode(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc, MVE_DST_IMAGE_S *pstDst, MVE_ERODE_CTRL_S *pstErodeCtrl,bool bInstant);
MI_MVE_RET MI_MVE_Thresh(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc, MVE_DST_IMAGE_S *pstDst, MVE_THRESH_CTRL_S *pstThrCtrl, bool bInstant);
MI_MVE_RET MI_MVE_OrdStatFilter(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc,MVE_DST_IMAGE_S *pstDst,MVE_ORD_STAT_FILTER_CTRL_S *pstOrdStatFltCtrl, bool bInstant);
MI_MVE_RET MI_MVE_Bernsen(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc,MVE_DST_IMAGE_S *pstDst,MVE_BERNSEN_CTRL_S *pstBernsenCtrl,bool bInstant);
MI_MVE_RET MI_MVE_Map(MVE_HANDLE pMveHandle,MVE_SRC_IMAGE_S *pstSrc,MVE_MAP_LUT_MEM_S *pstMap, MVE_DST_IMAGE_S *pstDst,bool bInstant);
MI_MVE_RET MI_MVE_And(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc1, MVE_SRC_IMAGE_S *pstSrc2, MVE_DST_IMAGE_S *pstDst, bool bInstant);
MI_MVE_RET MI_MVE_Or(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc1, MVE_SRC_IMAGE_S *pstSrc2, MVE_DST_IMAGE_S *pstDst, bool bInstant);
MI_MVE_RET MI_MVE_Xor(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc1, MVE_SRC_IMAGE_S *pstSrc2, MVE_DST_IMAGE_S *pstDst, bool bInstant);
MI_MVE_RET MI_MVE_Sub(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc1, MVE_SRC_IMAGE_S *pstSrc2, MVE_DST_IMAGE_S *pstDst, MVE_SUB_CTRL_S *pstSubCtrl, bool bInstant);
MI_MVE_RET MI_MVE_Add(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc1, MVE_SRC_IMAGE_S *pstSrc2, MVE_DST_IMAGE_S *pstDst, MVE_ADD_CTRL_S *pstAddCtrl, bool bInstant);
MI_MVE_RET MI_MVE_Thresh_S16(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc, MVE_DST_IMAGE_S *pstDst, MVE_THRESH_S16_CTRL_S *pstThrS16Ctrl, bool bInstant);
MI_MVE_RET MI_MVE_Thresh_U16(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc, MVE_DST_IMAGE_S *pstDst, MVE_THRESH_U16_CTRL_S *pstThrU16Ctrl, bool bInstant);
MI_MVE_RET MI_MVE_16BitTo8Bit(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc, MVE_DST_IMAGE_S *pstDst, MVE_16BIT_TO_8BIT_CTRL_S *pst16BitTo8BitCtrl, bool bInstant);
MI_MVE_RET MI_MVE_CCL(MVE_HANDLE pMveHandle, MVE_IMAGE_S *pstSrcDst, MVE_CCBLOB_S *pstBlob, MVE_CCL_CTRL_S *pstCclCtrl, bool bInstant);
MI_MVE_RET MI_MVE_Integ(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc, MVE_DST_IMAGE_S *pstDst, MVE_INTEG_CTRL_S *pstIntegCtrl, bool bInstant);
MI_MVE_RET MI_MVE_NCC(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc1, MVE_SRC_IMAGE_S *pstSrc2, MVE_NCC_DST_MEM_S *pstDst, bool bInstant);
MI_MVE_RET MI_MVE_Hist(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc, MVE_DST_MEM_INFO_S *pstDst, bool bInstant);
MI_MVE_RET MI_MVE_EqualizeHist(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc, MVE_DST_IMAGE_S *pstDst, MVE_EQUALIZE_HIST_CTRL_S *pstEqualizeHistCtrl, bool bInstant);
MI_MVE_RET MI_MVE_SAD(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc1, MVE_SRC_IMAGE_S *pstSrc2, MVE_DST_IMAGE_S *pstDst1, MVE_DST_IMAGE_S *pstDst2, MVE_SAD_CTRL_S *pstSADCtrl,bool bInstant);
MI_MVE_RET MI_MVE_NormGrad(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc,MVE_DST_IMAGE_S *pstDstH, MVE_DST_IMAGE_S *pstDstV, MVE_DST_IMAGE_S *pstDstHV, MVE_NORM_GRAD_CTRL_S *pstNormGradCtrl, bool bInstant);
MI_MVE_RET MI_MVE_CannyHysEdge(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc, MVE_DST_IMAGE_S *pstEdge, MVE_DST_MEM_INFO_S *pstStack, MVE_CANNY_HYS_EDGE_CTRL_S *pstCannyHysEdgeCtrl, bool bInstant);
MI_MVE_RET MI_MVE_CannyEdge(MVE_HANDLE pMveHandle, MVE_IMAGE_S *pstEdge, MVE_MEM_INFO_S *pstStack);
MI_MVE_RET MI_MVE_GMM(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc, MVE_DST_IMAGE_S *pstFg, MVE_DST_IMAGE_S *pstBg, MVE_MEM_INFO_S *pstModel, MVE_GMM_CTRL_S *pstGmmCtrl, bool bInstant);
MI_MVE_RET MI_MVE_LBP(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc, MVE_DST_IMAGE_S *pstDst, MVE_LBP_CTRL_S *pstLbpCtrl, bool bInstant);
MI_MVE_RET MI_MVE_BAT(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc, MVE_DST_MEM_INFO_S *pstDstH, MVE_DST_MEM_INFO_S *pstDstV, MVE_BAT_CTRL_S *pstCtrl, bool bInstant);
MI_MVE_RET MI_MVE_LineFilterHor(MVE_HANDLE pMveHandle, MVE_IMAGE_S *pstSrcDst, MVE_LINE_FILTER_HOR_CTRL_S *pstLineFilterHorCtrl, bool bInstant);
MI_MVE_RET MI_MVE_LineFilterVer(MVE_HANDLE pMveHandle, MVE_IMAGE_S *pstSrcDst, MVE_LINE_FILTER_VER_CTRL_S *pstLineFilterVerCtrl, bool bInstant);
MI_MVE_RET MI_MVE_NoiseRemoveHor(MVE_HANDLE pMveHandle, MVE_IMAGE_S *pstSrcDst, MVE_NOISE_REMOVE_HOR_CTRL_S *pstNoiseRemoveHorCtrl, bool bInstant);
MI_MVE_RET MI_MVE_NoiseRemoveVer(MVE_HANDLE pMveHandle, MVE_IMAGE_S *pstSrcDst, MVE_NOISE_REMOVE_VER_CTRL_S *pstNoiseRemoveVerCtrl, bool bInstant);
MI_MVE_RET MI_MVE_AdpThresh(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc, MVE_SRC_IMAGE_S *pstInteg, MVE_DST_IMAGE_S *pstDst, MVE_ADP_THRESH_CTRL_S *pstAdpThrCtrl, bool bInstant);
MI_MVE_RET MI_MVE_LKOpticalFlow(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S* pstSrcPre, MVE_SRC_IMAGE_S* pstSrcCur, MVE_SRC_MEM_INFO_S *pstPoint, MVE_MEM_INFO_S *pstMv, MVE_LK_OPTICAL_FLOW_CTRL_S *pstLkOptiFlowCtrl, bool bInstant );
MI_MVE_RET MI_MVE_Resize(MVE_HANDLE pMveHandle, MVE_SRC_IMAGE_S *pstSrc,MVE_DST_IMAGE_S *pstDst, MVE_RESIZE_CTRL_S *pstResizeCtrl, bool bInstant);

#ifdef __cplusplus
}
#endif

#endif // MI_MVE_H
