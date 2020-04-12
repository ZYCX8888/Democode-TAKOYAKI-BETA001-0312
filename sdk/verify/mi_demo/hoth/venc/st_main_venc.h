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
#include "st_common.h"
#include "mi_venc.h"
#include <stdbool.h>
#include <stdint.h>

// Insert User Data
MI_BOOL g_insert_user_data = FALSE;

MI_U8 _test_insert_data[8] = {0, 1, 2, 3, 4, 5, 6, 7};
MI_U8 _test_insert_data_tmp[8] = {0, 7, 6, 5, 4, 3, 2, 1};

#define DECL_VS2(A, B, name)\
    A ## H264 ## B name ## _h264; \
    A ## H265 ## B name ## _h265;

//IDR config
MI_BOOL g_idr_config = FALSE;
MI_BOOL _test_idr_enable = FALSE;
int _test_tmp_interval = 0, _test_idr_interval = 80;

#define VENC_TEST_ROI       (1 << 0)
#define VENC_TEST_INSERT    (1 << 1)
#define VENC_TEST_ENTROPY   (1 << 2)
#define VENC_TEST_REF       (1 << 3)
#define VENC_TEST_CROP      (1 << 4)
#define VENC_TEST_SLISPLT   (1 << 5)
#define VENC_TEST_INTRA     (1 << 6)
#define VENC_TEST_INTER     (1 << 7)
#define VENC_TEST_TRANS     (1 << 8)
#define VENC_TEST_VUI       (1 << 9)
#define VENC_TEST_DBLK      (1 << 10)

/*
MI_VENC_SuperFrameCfg_t _test_super_cfg =
{
    .eSuperFrmMode = E_MI_VENC_SUPERFRM_REENCODE,
    .u32SuperIFrmBitsThr = 100 * 1024 * 8,
    .u32SuperPFrmBitsThr = 50 * 1024 * 8,
    .u32SuperBFrmBitsThr = 2,
};
*/
