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
#ifndef __MID_MD_H__
#define __MID_MD_H__

#define RAW_W            384
#define RAW_H            288
#define RAW_C 3

#define SAD_BLOCK_SIZE          8
#define MD_ROI_MAX              50
#define MD_PIXEL_DIFF           15
#define HCHD_DETECT_MAX           6
#define HCHD_PROBABILITY          0.7

#if TARGET_CHIP_I6 || TARGET_CHIP_I6B0
#if 0
typedef struct
{
    int width;
    int align_height;
    MI_IVE_HANDLE ive_handle;
    MVE_IMAGE_S *Y_image0, *Y_image1;
    MVE_IMAGE_S *SadResult, *ThdResult;
    MVE_SAD_CTRL_S *sad_ctrl;
} SAD_METHOD_0_Handle;
#endif

typedef struct ive_methon{
    int width;
    int align_height;
    MI_IVE_HANDLE ive_handle;
    MI_IVE_SrcImage_t *Y_image0, *Y_image1;
    MI_IVE_Image_t *SadResult, *ThdResult;
    MI_IVE_SadCtrl_t *sad_ctrl;
}SAD_METHOD_handle;

typedef struct
{
    int x_min;
    int x_max;
    int y_min;
    int y_max;
} BBOX;
typedef struct
{
    int max_num;
    int used_num;
    BBOX box[MD_ROI_MAX];
} ROI;
#endif

int mid_hchd_Initial(int param);
int mid_hchd_Uninitial();

#endif

