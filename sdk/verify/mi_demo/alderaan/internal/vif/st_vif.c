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

#include "st_vif.h"
#include "st_common.h"

//#define UVC_SUPPORT_LL

MI_S32 ST_Vif_EnableDev(MI_VIF_DEV VifDev,MI_VIF_HDRType_e eHdrType, MI_SNR_PADInfo_t *pstSnrPadInfo)
{
    MI_VIF_DevAttr_t stDevAttr;

    memset(&stDevAttr, 0x0, sizeof(MI_VIF_DevAttr_t));

    stDevAttr.eIntfMode = pstSnrPadInfo->eIntfMode;
    stDevAttr.eWorkMode = E_MI_VIF_WORK_MODE_RGB_REALTIME;
    stDevAttr.eHDRType = eHdrType;
    if(stDevAttr.eIntfMode == E_MI_VIF_MODE_BT656)
        stDevAttr.eClkEdge = pstSnrPadInfo->unIntfAttr.stBt656Attr.eClkEdge;
    else
        stDevAttr.eClkEdge = E_MI_VIF_CLK_EDGE_DOUBLE;

    if(stDevAttr.eIntfMode == E_MI_VIF_MODE_MIPI)
        stDevAttr.eDataSeq =pstSnrPadInfo->unIntfAttr.stMipiAttr.eDataYUVOrder;
    else
        stDevAttr.eDataSeq = E_MI_VIF_INPUT_DATA_YUYV;

    if(stDevAttr.eIntfMode == E_MI_VIF_MODE_BT656)
        memcpy(&stDevAttr.stSyncAttr, &pstSnrPadInfo->unIntfAttr.stBt656Attr.stSyncAttr, sizeof(MI_VIF_SyncAttr_t));

    stDevAttr.eBitOrder = E_MI_VIF_BITORDER_NORMAL;

    ExecFunc(MI_VIF_SetDevAttr(VifDev, &stDevAttr), MI_SUCCESS);
    ExecFunc(MI_VIF_EnableDev(VifDev), MI_SUCCESS);

    return MI_SUCCESS;
}

MI_S32 ST_Vif_DisableDev(MI_VIF_DEV VifDev)
{
    ExecFunc(MI_VIF_DisableDev(VifDev), MI_SUCCESS);

    return MI_SUCCESS;
}

#if 0
MI_S32 ST_Vif_StartPort(MI_VIF_DEV VifDev, MI_VIF_CHN VifChn,
                        MI_VIF_PORT VifPort, ST_VIF_PortInfo_T *pstPortInfoInfo)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_VIF_ChnPortAttr_t stChnPortAttr;

    memset(&stChnPortAttr, 0, sizeof(MI_VIF_ChnPortAttr_t));
    stChnPortAttr.stCapRect.u16X = pstPortInfoInfo->u32RectX;
    stChnPortAttr.stCapRect.u16Y = pstPortInfoInfo->u32RectY;
    stChnPortAttr.stCapRect.u16Width = pstPortInfoInfo->u32RectWidth;
    stChnPortAttr.stCapRect.u16Height = pstPortInfoInfo->u32RectHeight;
    stChnPortAttr.stDestSize.u16Width = pstPortInfoInfo->u32DestWidth;
    stChnPortAttr.stDestSize.u16Height = pstPortInfoInfo->u32DestHeight;
    stChnPortAttr.eCapSel = E_MI_SYS_FIELDTYPE_BOTH;
    if (pstPortInfoInfo->u32IsInterlace)
    {
        stChnPortAttr.eScanMode = E_MI_SYS_FRAME_SCAN_MODE_INTERLACE;
    }
    else
    {
        stChnPortAttr.eScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    }
    stChnPortAttr.ePixFormat = pstPortInfoInfo->ePixFormat;//E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stChnPortAttr.bMirror = FALSE;
    stChnPortAttr.bFlip = FALSE;
    stChnPortAttr.eFrameRate = pstPortInfoInfo->s32FrameRate;
#ifdef UVC_SUPPORT_LL
	stChnPortAttr.u32FrameModeLineCount = 10;
#endif
    ExecFunc(MI_VIF_SetChnPortAttr(VifChn, VifPort, &stChnPortAttr), MI_SUCCESS);

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VIF;
    stChnPort.u32DevId = VifDev;
    stChnPort.u32ChnId = VifChn;
    stChnPort.u32PortId = VifPort;
    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stChnPort, 0, 6), MI_SUCCESS);

    ExecFunc(MI_VIF_EnableChnPort(VifChn, VifPort), MI_SUCCESS);

    return MI_SUCCESS;
}
#endif


MI_S32 ST_Vif_CreatePort(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort, ST_VIF_PortInfo_T *pstPortInfoInfo)
{
    MI_VIF_ChnPortAttr_t stChnPortAttr;

    memset(&stChnPortAttr, 0, sizeof(MI_VIF_ChnPortAttr_t));
    stChnPortAttr.stCapRect.u16X = pstPortInfoInfo->u32RectX;
    stChnPortAttr.stCapRect.u16Y = pstPortInfoInfo->u32RectY;
    stChnPortAttr.stCapRect.u16Width = pstPortInfoInfo->u32RectWidth;
    stChnPortAttr.stCapRect.u16Height = pstPortInfoInfo->u32RectHeight;
    stChnPortAttr.stDestSize.u16Width = pstPortInfoInfo->u32DestWidth;
    stChnPortAttr.stDestSize.u16Height = pstPortInfoInfo->u32DestHeight;
    stChnPortAttr.eCapSel = E_MI_SYS_FIELDTYPE_BOTH;
    if (pstPortInfoInfo->u32IsInterlace)
    {
        stChnPortAttr.eScanMode = E_MI_SYS_FRAME_SCAN_MODE_INTERLACE;
    }
    else
    {
        stChnPortAttr.eScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    }
    stChnPortAttr.ePixFormat = pstPortInfoInfo->ePixFormat;//E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stChnPortAttr.eFrameRate = pstPortInfoInfo->eFrameRate;
#ifdef UVC_SUPPORT_LL
	stChnPortAttr.u32FrameModeLineCount = 10;
#endif
    ExecFunc(MI_VIF_SetChnPortAttr(VifChn, VifPort, &stChnPortAttr), MI_SUCCESS);

    return MI_SUCCESS;
}

MI_S32 ST_Vif_StartPort(MI_VIF_DEV VifDev, MI_VIF_CHN VifChn, MI_VIF_PORT VifPort)
{
    MI_SYS_ChnPort_t stChnPort;

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VIF;
    stChnPort.u32DevId = VifDev;
    stChnPort.u32ChnId = VifChn;
    stChnPort.u32PortId = VifPort;

    MI_SYS_SetChnOutputPortDepth(&stChnPort, 0, 6);

    ExecFunc(MI_VIF_EnableChnPort(VifChn, VifPort), MI_SUCCESS);
    return MI_SUCCESS;
}
MI_S32 ST_Vif_StopPort(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort)
{
    ExecFunc(MI_VIF_DisableChnPort(VifChn, VifPort), MI_SUCCESS);

    return MI_SUCCESS;
}

