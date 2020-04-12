/*************************************************
*
* Copyright (c) 2006-2015 SigmaStar Technology Inc.
* All rights reserved.
*
**************************************************
* File name:  mid_vif.c
* Author:     andely.zhou@sigmastar.com.cn
* Version:    Initial Draft
* Date:       2018/6/13
* Description: vif module source file
*
*
*
* History:
*
*    1. Date  :        2018/6/13
*       Author:        andely.zhou@sigmastar.com.cn
*       Modification:  Created file
*
**************************************************/
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "mid_vif.h"
#include "mid_common.h"

static MI_U8 vifDevNum = 0;
static MI_U8 sensorNum = 1;
//#define UVC_SUPPORT_LL
static BOOL GetSensor1RawDataThrState = FALSE;
void Mixer_SetSensor1GetRawDataThrState(BOOL state)
{
  GetSensor1RawDataThrState = state;
}
BOOL  Mixer_GetSensor1GetRawDataThrState()
{
  return GetSensor1RawDataThrState;
}
MI_U8 Mixer_vifDevNumberGet(void)
{
  return vifDevNum;
}
void  Mixer_vifDevNumberSet(MI_U8 num)
{
  vifDevNum = num;
}
MI_U8 Mixer_GetSensorNum(void)
{
  printf("Get current sensorNum = %d\n",sensorNum);
  return sensorNum;
}
void Mixer_SetSensorNum(MI_U8 num)
{
  sensorNum = sensorNum <= 0 ? 1 : num;
  printf("Set current sensorNum = %d\n",sensorNum);
}
MI_S32 Mixer_Vif_EnableDev(MI_VIF_DEV VifDev, MI_VIF_HDRType_e eHdrType, MI_SNR_PADInfo_t *pstSnrPadInfo)
{
    MI_VIF_DevAttr_t stDevAttr;

    memset(&stDevAttr, 0x0, sizeof(MI_VIF_DevAttr_t));

    stDevAttr.eIntfMode = pstSnrPadInfo->eIntfMode;
    stDevAttr.eWorkMode = E_MI_VIF_WORK_MODE_RGB_REALTIME;
    if(2 == VifDev)
    {
      stDevAttr.eWorkMode = E_MI_VIF_WORK_MODE_RGB_FRAMEMODE;
    }
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

    if(stDevAttr.eIntfMode == E_MI_VIF_MODE_DIGITAL_CAMERA)
        memcpy(&stDevAttr.stSyncAttr, &pstSnrPadInfo->unIntfAttr.stParallelAttr.stSyncAttr, sizeof(MI_VIF_SyncAttr_t));

    stDevAttr.eBitOrder = E_MI_VIF_BITORDER_NORMAL;

    ExecFunc(MI_VIF_SetDevAttr(VifDev, &stDevAttr), MI_SUCCESS);
    ExecFunc(MI_VIF_EnableDev(VifDev), MI_SUCCESS);
    return MI_SUCCESS;
}

MI_S32 Mixer_Vif_DisableDev(MI_VIF_DEV VifDev)
{
    ExecFunc(MI_VIF_DisableDev(VifDev), MI_SUCCESS);

    return MI_SUCCESS;
}

MI_S32 Mixer_Vif_CreatePort(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort, Mixer_VIF_PortInfo_T *pstVifPortInfo)
{
    MI_VIF_ChnPortAttr_t stVifChnPortAttr;

    memset(&stVifChnPortAttr, 0x00, sizeof(MI_VIF_ChnPortAttr_t));
    stVifChnPortAttr.stCapRect.u16X = pstVifPortInfo->u32RectX;
    stVifChnPortAttr.stCapRect.u16Y = pstVifPortInfo->u32RectY;
    stVifChnPortAttr.stCapRect.u16Width  = pstVifPortInfo->u32RectWidth;
    stVifChnPortAttr.stCapRect.u16Height = pstVifPortInfo->u32RectHeight;
    stVifChnPortAttr.stDestSize.u16Width  = pstVifPortInfo->u32DestWidth;
    stVifChnPortAttr.stDestSize.u16Height = pstVifPortInfo->u32DestHeight;
    stVifChnPortAttr.ePixFormat = pstVifPortInfo->ePixFormat;
    stVifChnPortAttr.eFrameRate = (MI_VIF_FrameRate_e)pstVifPortInfo->s32FrameRate;
#if TARGET_CHIP_I6E
    stVifChnPortAttr.eCapSel =    E_MI_SYS_FIELDTYPE_NONE;
#elif TARGET_CHIP_I6B0
    stVifChnPortAttr.eCapSel =    E_MI_SYS_FIELDTYPE_NONE;
#else
    stVifChnPortAttr.eCapSel = E_MI_SYS_FIELDTYPE_BOTH;
#endif
    if (pstVifPortInfo->u32IsInterlace)
    {
        stVifChnPortAttr.eScanMode = E_MI_SYS_FRAME_SCAN_MODE_INTERLACE;
    }
    else
    {
        stVifChnPortAttr.eScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    }
    if(8 == VifChn)
    {
       stVifChnPortAttr.eCapSel = E_MI_SYS_FIELDTYPE_BOTH;
       stVifChnPortAttr.eScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    }
#ifdef UVC_SUPPORT_LL
    stVifChnPortAttr.u32FrameModeLineCount = 10;
#endif

    ExecFunc(MI_VIF_SetChnPortAttr(VifChn, VifPort, &stVifChnPortAttr), MI_SUCCESS);

    return MI_SUCCESS;
}

MI_S32 Mixer_Vif_StartPort(MI_VIF_DEV VifDev, MI_VIF_CHN VifChn, MI_VIF_PORT VifPort)
{
/*
    MI_SYS_ChnPort_t stChnPort;

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId    = E_MI_MODULE_ID_VIF;
    stChnPort.u32DevId  = VifDev;
    stChnPort.u32ChnId  = VifChn;
    stChnPort.u32PortId = VifPort;
    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stChnPort, 0, 6), MI_SUCCESS);  //mi default (0, 4)
*/
    ExecFunc(MI_VIF_EnableChnPort(VifChn, VifPort), MI_SUCCESS);

    return MI_SUCCESS;
}

MI_S32 Mixer_Vif_StopPort(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort)
{
    ExecFunc(MI_VIF_DisableChnPort(VifChn, VifPort), MI_SUCCESS);

    return MI_SUCCESS;
}
