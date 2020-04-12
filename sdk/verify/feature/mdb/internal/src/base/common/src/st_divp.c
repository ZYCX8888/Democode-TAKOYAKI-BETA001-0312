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
#include <unistd.h>
#include <string.h>

#include "mi_sys.h"

#include "st_common.h"
#include "st_divp.h"

MI_S32 ST_Divp_CreatChannel(MI_DIVP_CHN DivpChn, MI_SYS_Rotate_e eRoate, MI_SYS_WindowRect_t *pstCropWin)
{
    MI_DIVP_ChnAttr_t stAttr;
    memset(&stAttr, 0, sizeof(stAttr));

    stAttr.bHorMirror = false;
    stAttr.bVerMirror = false;
    stAttr.eDiType = E_MI_DIVP_DI_TYPE_OFF;
    stAttr.eRotateType = eRoate;
    stAttr.eTnrLevel = E_MI_DIVP_TNR_LEVEL_OFF;
    stAttr.stCropRect.u16X = pstCropWin->u16X;
    stAttr.stCropRect.u16Y = pstCropWin->u16Y;
    stAttr.stCropRect.u16Width = pstCropWin->u16Width;
    stAttr.stCropRect.u16Height = pstCropWin->u16Height;
    stAttr.u32MaxWidth = 3840;
    stAttr.u32MaxHeight = 2160;
    STCHECKRESULT(MI_DIVP_CreateChn(DivpChn, &stAttr));
    return MI_SUCCESS;
}

MI_S32 ST_Divp_SetOutputAttr(MI_DIVP_CHN DivpChn, MI_SYS_PixelFormat_e eOutPixel, MI_SYS_WindowRect_t *pstOutWin)
{
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;
    memset(&stOutputPortAttr, 0, sizeof(stOutputPortAttr));
    stOutputPortAttr.eCompMode = E_MI_SYS_COMPRESS_MODE_NONE;
    stOutputPortAttr.ePixelFormat = eOutPixel;
    stOutputPortAttr.u32Width = pstOutWin->u16Width;
    stOutputPortAttr.u32Height = pstOutWin->u16Height;
    STCHECKRESULT(MI_DIVP_SetOutputPortAttr(DivpChn, &stOutputPortAttr));
    return MI_SUCCESS;
}

MI_S32 ST_Divp_StartChn(MI_DIVP_CHN DivpChn)
{
    STCHECKRESULT(MI_DIVP_StartChn(DivpChn));
    return MI_SUCCESS;
}

MI_S32 ST_Divp_StopChn(MI_DIVP_CHN DivpChn)
{
    STCHECKRESULT(MI_DIVP_StopChn(DivpChn));
    return MI_SUCCESS;
}

MI_S32 ST_Divp_DestroyChn(MI_DIVP_CHN DivpChn)
{
    STCHECKRESULT(MI_DIVP_DestroyChn(DivpChn));
    return MI_SUCCESS;
}


