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
#include "st_vif.h"
#include "st_common.h"

MI_S32 ST_Vif_Init(void)
{
    printf("ST Vif Init!!!\n");
    return MI_SUCCESS;
}

MI_S32 ST_Vif_Exit(void)
{
    printf("ST Vif Exit!!!\n");
    return MI_SUCCESS;
}

MI_S32 ST_Vif_CreateDev(MI_VIF_DEV VifDev)
{
    MI_VIF_DevAttr_t stDevAttr;
    stDevAttr.eIntfMode = E_MI_VIF_MODE_BT1120_STANDARD;
    stDevAttr.eWorkMode = E_MI_VIF_WORK_MODE_1MULTIPLEX;
    stDevAttr.au32CompMask[0] = 0xFF000000;
    stDevAttr.au32CompMask[1] = 0xFF0000;
    stDevAttr.eClkEdge = E_MI_VIF_CLK_EDGE_SINGLE_UP;
    stDevAttr.as32AdChnId[0] = -1;
    stDevAttr.as32AdChnId[1] = -1;
    stDevAttr.as32AdChnId[2] = -1;
    stDevAttr.as32AdChnId[3] = -1;
    ExecFunc(MI_VIF_SetDevAttr(VifDev, &stDevAttr), MI_SUCCESS);
    ExecFunc(MI_VIF_EnableDev(VifDev), MI_SUCCESS);
    return MI_SUCCESS;
}
// MI_S32 ST_Vif_CreateChannel(MI_VIF_CHN VifChn)
// {
// 	return MI_SUCCESS;
// }

MI_S32 ST_Vif_CreatePort(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort, ST_VIF_PortInfo_t *pstPortInfoInfo)
{
    MI_VIF_ChnPortAttr_t stChnPortAttr;
    stChnPortAttr.stCapRect.u16X = pstPortInfoInfo->u32RectX;
    stChnPortAttr.stCapRect.u16Y = pstPortInfoInfo->u32RectY;
    stChnPortAttr.stCapRect.u16Width = pstPortInfoInfo->u32RectWidth;
    stChnPortAttr.stCapRect.u16Height = pstPortInfoInfo->u32RectHeight;
    stChnPortAttr.stDestSize.u16Width = pstPortInfoInfo->u32DestWidth;
    stChnPortAttr.stDestSize.u16Height = pstPortInfoInfo->u32DestHeight;
    stChnPortAttr.eCapSel = E_MI_SYS_FIELDTYPE_BOTH;
    stChnPortAttr.eScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    stChnPortAttr.ePixFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stChnPortAttr.bMirror = FALSE;
    stChnPortAttr.bFlip = FALSE;
    stChnPortAttr.eFrameRate = E_MI_VIF_FRAMERATE_FULL;
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
    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stChnPort, 3, 5), MI_SUCCESS);
    ExecFunc(MI_VIF_EnableChnPort(VifChn, VifPort), MI_SUCCESS);
    return MI_SUCCESS;
}

MI_S32 ST_Vif_StopPort(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort)
{
    ExecFunc(MI_VIF_DisableChnPort(VifChn, VifPort), MI_SUCCESS);
    return MI_SUCCESS;
}

MI_S32 ST_Vif_DisableDev(MI_VIF_DEV VifDev)
{
    ExecFunc(MI_VIF_DisableDev(VifDev), MI_SUCCESS);
    return MI_SUCCESS;
}
