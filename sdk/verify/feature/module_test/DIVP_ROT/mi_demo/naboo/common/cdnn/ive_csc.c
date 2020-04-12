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
#include <stdio.h>
#include "ive_csc.h"

#define RAW_WIDTH    608
#define RAW_HEIGHT   352

int mve_nv12_to_rgb(MI_IVE_HANDLE mve_handle, MI_IVE_SrcImage_t *src, MI_IVE_DstImage_t *dst)
{
    MI_S32 ret;
    MI_IVE_CscCtrl_t ctrl = {.eMode = E_MI_IVE_CSC_MODE_PIC_BT601_YUV2RGB};

    // Run MI_MVE_CSC()
    ret = MI_IVE_Csc(mve_handle, src, dst, &ctrl, 0);
    if (ret != 0)
    {
        printf("MI_IVE_Csc() return ERROR 0x%X\n", ret);
    }

    return ret;
}
