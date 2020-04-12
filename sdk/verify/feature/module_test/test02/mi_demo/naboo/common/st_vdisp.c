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
#include "mi_vdisp_datatype.h"

#include "st_vdisp.h"

MI_S32 ST_Vdisp_Init(void)
{
    STCHECKRESULT(MI_VDISP_Init());
    STCHECKRESULT(MI_VDISP_OpenDevice(MI_VDISP_DEV_0));

    return MI_SUCCESS;
}

MI_S32 ST_Vdisp_Exit(void)
{
    STCHECKRESULT(MI_VDISP_CloseDevice(MI_VDISP_DEV_0));
    STCHECKRESULT(MI_VDISP_Exit());

    return MI_SUCCESS;
}

MI_S32 ST_Vdisp_SetInputPortAttr(MI_VDISP_DEV DevId, MI_S32 s32InputPort, ST_Rect_t *pstRect)
{
    MI_VDISP_InputPortAttr_t stInputPortAttr;
    stInputPortAttr.s32IsFreeRun = 1;
    stInputPortAttr.u32OutX = pstRect->s32X;
    stInputPortAttr.u32OutY = pstRect->s32Y;
    stInputPortAttr.u32OutWidth = pstRect->u16PicW;
    stInputPortAttr.u32OutHeight = pstRect->u16PicH;
    STCHECKRESULT(MI_VDISP_SetInputPortAttr(DevId, s32InputPort, &stInputPortAttr));

    return MI_SUCCESS;
}

MI_S32 ST_Vdisp_EnableInputPort(MI_VDISP_DEV DevId, MI_VDISP_PORT PortId)
{
    STCHECKRESULT(MI_VDISP_EnableInputPort(DevId, PortId));

    return MI_SUCCESS;
}

MI_S32 ST_Vdisp_DisableInputPort(MI_VDISP_DEV DevId, MI_VDISP_PORT PortId)
{
    STCHECKRESULT(MI_VDISP_DisableInputPort(DevId, PortId));

    return MI_SUCCESS;
}

MI_S32 ST_Vdisp_SetOutputPortAttr(MI_VDISP_DEV DevId, MI_S32 s32OutputPort, ST_Rect_t *pstRect, MI_S32 s32FrmRate, MI_S32 s32FrmDepth)
{
    MI_VDISP_OutputPortAttr_t stOutputPortAttr;
    MI_SYS_ChnPort_t stChnPort;
    stOutputPortAttr.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;//420 for venc
    stOutputPortAttr.u32BgColor = YUYV_BLACK;
    stOutputPortAttr.u32FrmRate = s32FrmRate;//10
    stOutputPortAttr.u32Width = pstRect->u16PicW;
    stOutputPortAttr.u32Height = pstRect->u16PicH;
    stOutputPortAttr.u64pts = 0;
    STCHECKRESULT(MI_VDISP_SetOutputPortAttr(DevId, s32OutputPort, &stOutputPortAttr));
    stChnPort.eModId = E_MI_MODULE_ID_VDISP;
    stChnPort.u32DevId = DevId;
    stChnPort.u32ChnId = 0;
    stChnPort.u32PortId = s32OutputPort;
    STCHECKRESULT(MI_SYS_SetChnOutputPortDepth(&stChnPort, s32FrmDepth, 10)); //Default queue frame depth--->20

    return MI_SUCCESS;
}

MI_S32 ST_Vdisp_StartDevice(MI_VDISP_DEV DevId)
{
    STCHECKRESULT(MI_VDISP_StartDev(DevId));

    return MI_SUCCESS;
}

MI_S32 ST_Vdisp_StopDevice(MI_VDISP_DEV DevId)
{
    STCHECKRESULT(MI_VDISP_StopDev(DevId));

    return MI_SUCCESS;
}

MI_S32 ST_Sys_CreateVdispChn(MI_S32 s32Channel, ST_Sys_ChnInfo_t *pstChnInfo)
{
    MI_VDISP_DEV DevId = 0; //Only Support DevID == 0
    ST_Rect_t stRect;

    //Create Vdisp Channel
    stRect.s32X = pstChnInfo->stDispRect.s32X;
    stRect.s32Y = pstChnInfo->stDispRect.s32Y;
    stRect.u16PicW = pstChnInfo->stDispRect.u16PicW;
    stRect.u16PicH = pstChnInfo->stDispRect.u16PicH;
    printf("----[STDBG]--VdispDevid(%d)--Channel(%d)--Rect(%d-%d-%d-%d)--\n", DevId, s32Channel,
        stRect.s32X, stRect.s32Y, stRect.u16PicW, stRect.u16PicH);
    STCHECKRESULT(ST_Vdisp_SetInputPortAttr(DevId, s32Channel, &stRect));
    STCHECKRESULT(ST_Vdisp_EnableInputPort(DevId, s32Channel));

    return MI_SUCCESS;
}

MI_S32 ST_Sys_DestroyVdispChn(MI_S32 s32Channel)
{
    MI_VDISP_DEV DevId = 0; //Only Support DevID == 0

    //Create Vdisp Channel
    STCHECKRESULT(ST_Vdisp_DisableInputPort(DevId, s32Channel));

    return MI_SUCCESS;
}
