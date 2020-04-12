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

#include "mi_sys.h"
#include "mi_divp.h"
#include "mi_divp_datatype.h"
#include "mi_disp_datatype.h"

#include "st_common.h"
#include "st_vpe.h"
#include "st_vdisp.h"
#include "st_vif.h"

#define DEFAULT_MAX_PICW 1920
#define DEFAULT_MAX_PICH 1080

MI_S32 ST_Sys_Init(void)
{
    MI_SYS_Version_t stVersion;
    MI_U64 u64Pts = 0;
    STCHECKRESULT(MI_SYS_Init()); //Sys Init
    memset(&stVersion, 0x0, sizeof(MI_SYS_Version_t));
    STCHECKRESULT(MI_SYS_GetVersion(&stVersion));
    ST_INFO("(%d) u8Version:0x%llx\n", __LINE__, stVersion.u8Version);
    STCHECKRESULT(MI_SYS_GetCurPts(&u64Pts));
    ST_INFO("(%d) u64Pts:0x%llx\n", __LINE__, u64Pts);

    u64Pts = 0xF1237890F1237890;
    STCHECKRESULT(MI_SYS_InitPtsBase(u64Pts));

    u64Pts = 0xE1237890E1237890;
    STCHECKRESULT(MI_SYS_SyncPts(u64Pts));

    return MI_SUCCESS;
}

MI_S32 ST_Sys_Exit(void)
{
    STCHECKRESULT(MI_SYS_Exit());

    return MI_SUCCESS;
}

MI_S32 ST_Sys_Bind(ST_Sys_BindInfo_t *pstBindInfo)
{
    ExecFunc(MI_SYS_BindChnPort(&pstBindInfo->stSrcChnPort, &pstBindInfo->stDstChnPort, \
        pstBindInfo->u32SrcFrmrate, pstBindInfo->u32DstFrmrate), MI_SUCCESS);

    return MI_SUCCESS;
}

MI_S32 ST_Sys_UnBind(ST_Sys_BindInfo_t *pstBindInfo)
{
    ExecFunc(MI_SYS_UnBindChnPort(&pstBindInfo->stSrcChnPort, &pstBindInfo->stDstChnPort), MI_SUCCESS);

    return MI_SUCCESS;
}

MI_S32 ST_Sys_CreateNvrChn(MI_S32 s32Channel, MI_S32 s32SplitMode, const ST_Sys_ChnInfo_t *pstChnInfo)
{
    MI_VDISP_DEV DevId = 0; //Only Support DevID == 0
    MI_VDEC_ChnAttr_t stChnAttr;
    MI_DIVP_ChnAttr_t stDivpChnAttr;
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;
    ST_VPE_ChannelInfo_t stVpeChannelInfo;
    ST_VPE_PortInfo_t stPortInfo;
    ST_Rect_t stRect;
    ST_Sys_BindInfo_t stBindInfo;

    //Vdec Create Chn
    stChnAttr.stVdecVideoAttr.u32RefFrameNum = 2;
    stChnAttr.eVideoMode = E_MI_VDEC_VIDEO_MODE_FRAME;
    stChnAttr.u32BufSize = 1024 * 1024; //
    stChnAttr.u32PicWidth = pstChnInfo->stTestDataInfo.u16PicWidth;
    stChnAttr.u32PicHeight = pstChnInfo->stTestDataInfo.u16PicHeight;
    stChnAttr.u32Priority = 0;
    stChnAttr.eCodecType = pstChnInfo->eCodecType;
    ExecFunc(MI_VDEC_CreateChn(s32Channel, &stChnAttr), MI_SUCCESS);
    ExecFunc(MI_VDEC_StartChn(s32Channel), MI_SUCCESS);

    //Divp Create Chn
    stDivpChnAttr.bHorMirror = FALSE;
    stDivpChnAttr.bVerMirror = FALSE;
    stDivpChnAttr.eDiType = E_MI_DIVP_DI_TYPE_OFF;
    stDivpChnAttr.eRotateType = E_MI_SYS_ROTATE_NONE;
    stDivpChnAttr.eTnrLevel = E_MI_DIVP_TNR_LEVEL_OFF;
    stDivpChnAttr.stCropRect.u16X = 0;
    stDivpChnAttr.stCropRect.u16Y = 0;
    stDivpChnAttr.stCropRect.u16Width = pstChnInfo->stTestDataInfo.u16PicWidth; //Vdec pic w
    stDivpChnAttr.stCropRect.u16Height = pstChnInfo->stTestDataInfo.u16PicHeight; //Vdec pic h
    stDivpChnAttr.u32MaxWidth = DEFAULT_MAX_PICW; //max size picture can pass
    stDivpChnAttr.u32MaxHeight = DEFAULT_MAX_PICH;
    STCHECKRESULT(MI_DIVP_CreateChn(s32Channel, &stDivpChnAttr));

    STCHECKRESULT(MI_DIVP_GetChnAttr(s32Channel, &stDivpChnAttr));
    STCHECKRESULT(MI_DIVP_SetChnAttr(s32Channel, &stDivpChnAttr));
    STCHECKRESULT(MI_DIVP_StartChn(s32Channel));
    memset(&stOutputPortAttr, 0, sizeof(stOutputPortAttr));
    stOutputPortAttr.eCompMode = E_MI_SYS_COMPRESS_MODE_NONE;
    stOutputPortAttr.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    stOutputPortAttr.u32Width = pstChnInfo->stTestDataInfo.u16PicWidth;
    stOutputPortAttr.u32Height = pstChnInfo->stTestDataInfo.u16PicHeight;

    STCHECKRESULT(MI_DIVP_SetOutputPortAttr(s32Channel, &stOutputPortAttr));

    //Bind Vdec to Divp
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDEC;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = s32Channel;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = s32Channel; //only equal zero
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = pstChnInfo->s32VdecFrameRate;
    stBindInfo.u32DstFrmrate = pstChnInfo->s32DivpFrameRate;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    ST_INFO("Bind VDEC-->>>DIVP....Channel(%d)..\n", s32Channel);

    if (E_ST_SPLIT_MODE_TWO == s32SplitMode)
    {
        //Create Vpe Channel
        stVpeChannelInfo.u16VpeMaxW = DEFAULT_MAX_PICW;
        stVpeChannelInfo.u16VpeMaxH = DEFAULT_MAX_PICH;
        stVpeChannelInfo.u32X = 0;
        stVpeChannelInfo.u32Y = 0;
        if (0 == s32Channel)
        {
            stVpeChannelInfo.u16VpeCropW = pstChnInfo->stTestDataInfo.u16PicWidth / 2; //Input data size
            stVpeChannelInfo.u16VpeCropH = pstChnInfo->stTestDataInfo.u16PicHeight / 2;
        }
        else
        {
            stVpeChannelInfo.u16VpeCropW = pstChnInfo->stTestDataInfo.u16PicWidth; //Input data size
            stVpeChannelInfo.u16VpeCropH = pstChnInfo->stTestDataInfo.u16PicHeight;
        }
        STCHECKRESULT(ST_Vpe_CreateChannel(s32Channel, &stVpeChannelInfo));

        stPortInfo.DepVpeChannel = s32Channel;
        stPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
        stPortInfo.u16OutputWidth = pstChnInfo->stDispRect.u16PicW;
        stPortInfo.u16OutputHeight = pstChnInfo->stDispRect.u16PicH;

        STCHECKRESULT(ST_Vpe_CreatePort(0, &stPortInfo)); //default support port0 --->>> vdisp
        STCHECKRESULT(ST_Vpe_StartChannel(s32Channel));
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = s32Channel;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = s32Channel; //only equal zero
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 30;
        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

        ST_INFO("----[STDBG]--VdispDevid(%d)--Channel(%d)--Rect(%d-%d-%d-%d)--\n", DevId, s32Channel,
            stRect.s32X, stRect.s32Y, stRect.u16PicW, stRect.u16PicH);

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = s32Channel;
        stBindInfo.stSrcChnPort.u32PortId = 0;
    } //only for 2 window split mode
    else
    {
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = s32Channel;
        stBindInfo.stSrcChnPort.u32PortId = 0;
    }
    //Create Vdisp Channel
    stRect.s32X = pstChnInfo->stDispRect.s32X;
    stRect.s32Y = pstChnInfo->stDispRect.s32Y;
    stRect.u16PicW = pstChnInfo->stDispRect.u16PicW;
    stRect.u16PicH = pstChnInfo->stDispRect.u16PicH;
    if ((1 == s32Channel) && (E_ST_SPLIT_MODE_TWO == s32SplitMode))
    {
        STCHECKRESULT(ST_Vdisp_SetInputPortAttr(DevId, VDISP_OVERLAYINPUTPORTID, &stRect));
        STCHECKRESULT(ST_Vdisp_EnableInputPort(DevId, VDISP_OVERLAYINPUTPORTID));
    }
    else
    {
        STCHECKRESULT(ST_Vdisp_SetInputPortAttr(DevId, s32Channel, &stRect));
        STCHECKRESULT(ST_Vdisp_EnableInputPort(DevId, s32Channel));
    }

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VDISP;
    stBindInfo.stDstChnPort.u32DevId = DevId;
    stBindInfo.stDstChnPort.u32ChnId = 0; //only equal zero
    stBindInfo.stDstChnPort.u32PortId = s32Channel;
    stBindInfo.u32SrcFrmrate = pstChnInfo->s32DivpFrameRate;
    stBindInfo.u32DstFrmrate = pstChnInfo->s32VdispFrameRate;
    if ((1 == s32Channel) && (E_ST_SPLIT_MODE_TWO == s32SplitMode))
    {
        stBindInfo.stDstChnPort.u32PortId = VDISP_OVERLAYINPUTPORTID;
    }
    else
    {
        stBindInfo.stDstChnPort.u32PortId = s32Channel;
    }

    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    printf("----[STDBG]--Bind--Src(%d-%d-%d-%d)--Dst(%d-%d-%d-%d)--\n", stBindInfo.stSrcChnPort.eModId,
        stBindInfo.stSrcChnPort.u32DevId, stBindInfo.stSrcChnPort.u32ChnId, stBindInfo.stSrcChnPort.u32PortId,
        stBindInfo.stDstChnPort.eModId, stBindInfo.stDstChnPort.u32DevId, stBindInfo.stDstChnPort.u32ChnId,
        stBindInfo.stDstChnPort.u32PortId);

    return MI_SUCCESS;
}

/**************************************************************************************************
 * Vpe Channel --->>> s32Channel
 * Vdisp port --->>> s32Channel
 *************************************************************************************************/
MI_S32 ST_Sys_CreateDvrChn(MI_S32 s32Channel, MI_S32 s32SplitMode, const ST_Sys_ChnInfo_t *pstChnInfo)
{
    MI_VPE_PORT VpePort = 0;
    MI_VIF_DEV VifDevId = 0;
    MI_VDISP_DEV DevId = 0; //Only Support DevID == 0
    ST_VPE_ChannelInfo_t stVpeChannelInfo;

    MI_VDEC_ChnAttr_t stChnAttr;
    MI_DIVP_ChnAttr_t stDivpChnAttr;
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;
    ST_VIF_PortInfo_t stVifPortInfoInfo;

    ST_VPE_PortInfo_t stPortInfo;
    ST_Rect_t stRect;
    ST_Sys_BindInfo_t stBindInfo;

    memset(&stVifPortInfoInfo, 0x0, sizeof(ST_VIF_PortInfo_t));
    stVifPortInfoInfo.u32RectX = 0;
    stVifPortInfoInfo.u32RectY = 0;
    stVifPortInfoInfo.u32RectWidth = pstChnInfo->stTestDataInfo.u16PicWidth;
    stVifPortInfoInfo.u32RectHeight = pstChnInfo->stTestDataInfo.u16PicHeight;

    stVifPortInfoInfo.u32DestWidth = pstChnInfo->stTestDataInfo.u16PicWidth;
    stVifPortInfoInfo.u32DestHeight = pstChnInfo->stTestDataInfo.u16PicHeight;

    STCHECKRESULT(ST_Vif_CreatePort(s32Channel, pstChnInfo->stChnPort[s32Channel].u32PortId, &stVifPortInfoInfo));
    STCHECKRESULT(ST_Vif_StartPort(s32Channel, pstChnInfo->stChnPort[s32Channel].u32PortId));
    //Create Vpe Channel
    stVpeChannelInfo.u16VpeMaxW = DEFAULT_MAX_PICW;
    stVpeChannelInfo.u16VpeMaxH = DEFAULT_MAX_PICH;
    stVpeChannelInfo.u32X = 0;
    stVpeChannelInfo.u32Y = 0;
    if (E_ST_SPLIT_MODE_TWO == s32SplitMode)
    {
        if (1 == s32Channel)
        {
            stVpeChannelInfo.u16VpeCropW = pstChnInfo->stTestDataInfo.u16PicWidth; //Input data size
            stVpeChannelInfo.u16VpeCropH = pstChnInfo->stTestDataInfo.u16PicHeight;
        }
        else
        {
            stVpeChannelInfo.u16VpeCropW = pstChnInfo->stTestDataInfo.u16PicWidth / 2; //Input data size
            stVpeChannelInfo.u16VpeCropH = pstChnInfo->stTestDataInfo.u16PicHeight / 2;
        }
    }
    else
    {
        stVpeChannelInfo.u16VpeCropW = pstChnInfo->stTestDataInfo.u16PicWidth; //Input data size
        stVpeChannelInfo.u16VpeCropH = pstChnInfo->stTestDataInfo.u16PicHeight;
    }
    STCHECKRESULT(ST_Vpe_CreateChannel(s32Channel, &stVpeChannelInfo));

    stPortInfo.DepVpeChannel = s32Channel;
    stPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    stPortInfo.u16OutputWidth = pstChnInfo->stDispRect.u16PicW;
    stPortInfo.u16OutputHeight = pstChnInfo->stDispRect.u16PicH;

    STCHECKRESULT(ST_Vpe_CreatePort(VpePort, &stPortInfo)); //default support port0 --->>> vdisp
    STCHECKRESULT(ST_Vpe_StartChannel(s32Channel));

    //Bind Vdec to Divp
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    if ((1 == s32Channel) && (E_ST_SPLIT_MODE_TWO == s32SplitMode))
    {
        stBindInfo.stSrcChnPort.u32ChnId = 0;
    }
    else
    {
        stBindInfo.stSrcChnPort.u32ChnId = s32Channel;
    }
    stBindInfo.stSrcChnPort.u32PortId = 0;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = s32Channel;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = pstChnInfo->s32VdecFrameRate;
    stBindInfo.u32DstFrmrate = pstChnInfo->s32DivpFrameRate;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    ST_INFO("----[STDBG]--VpeChn(%d)--VpeOutPort(%d)--VpeCropW(%d)--VpeCropH(%d)--VpeOutW(%d)--VpeOutH(%d)--\n",
        s32Channel, VpePort, stVpeChannelInfo.u16VpeCropW, stVpeChannelInfo.u16VpeCropH, pstChnInfo->stDispRect.u16PicW,
        pstChnInfo->stDispRect.u16PicH);

    //Create Vdisp Channel
    stRect.s32X = pstChnInfo->stDispRect.s32X;
    stRect.s32Y = pstChnInfo->stDispRect.s32Y;
    stRect.u16PicW = pstChnInfo->stDispRect.u16PicW;
    stRect.u16PicH = pstChnInfo->stDispRect.u16PicH;
    if ((1 == s32Channel) && (E_ST_SPLIT_MODE_TWO == s32SplitMode))
    {
        STCHECKRESULT(ST_Vdisp_SetInputPortAttr(DevId, VDISP_OVERLAYINPUTPORTID, &stRect));
        STCHECKRESULT(ST_Vdisp_EnableInputPort(DevId, VDISP_OVERLAYINPUTPORTID));
    }
    else
    {
        STCHECKRESULT(ST_Vdisp_SetInputPortAttr(DevId, s32Channel, &stRect));
        STCHECKRESULT(ST_Vdisp_EnableInputPort(DevId, s32Channel));
    }
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = s32Channel;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VDISP;
    stBindInfo.stDstChnPort.u32DevId = DevId;
    stBindInfo.stDstChnPort.u32ChnId = 0;

    if ((1 == s32Channel) && (E_ST_SPLIT_MODE_TWO == s32SplitMode))
    {
        stBindInfo.stDstChnPort.u32PortId = VDISP_OVERLAYINPUTPORTID;
    }
    else
    {
        stBindInfo.stDstChnPort.u32PortId = s32Channel;
    }
    stBindInfo.u32SrcFrmrate = pstChnInfo->s32VpeFrameRate;
    stBindInfo.u32DstFrmrate = pstChnInfo->s32VdispFrameRate;

    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    printf("----[STDBG]--Bind--Src(%d-%d-%d-%d)--Dst(%d-%d-%d-%d)--\n", stBindInfo.stSrcChnPort.eModId,
        stBindInfo.stSrcChnPort.u32DevId, stBindInfo.stSrcChnPort.u32ChnId, stBindInfo.stSrcChnPort.u32PortId,
        stBindInfo.stDstChnPort.eModId, stBindInfo.stDstChnPort.u32DevId, stBindInfo.stDstChnPort.u32ChnId,
        stBindInfo.stDstChnPort.u32PortId);

    return MI_SUCCESS;
}

MI_S32 ST_Sys_DestroyDvrChn(MI_S32 s32Channel, MI_S32 s32SplitMode)
{
    MI_VPE_PORT VpePort = 0;
    MI_VDISP_DEV DevId = 0; //Only Support DevID == 0
    ST_VPE_ChannelInfo_t stVpeChannelInfo;
    ST_Rect_t stRect;
    ST_Sys_BindInfo_t stBindInfo;

    //UnBind VIF to VPE
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = 0;

    if ((1 == s32Channel) && (E_ST_SPLIT_MODE_TWO == s32SplitMode))
    {
        stBindInfo.stSrcChnPort.u32ChnId = 0;
    }
    else
    {
        stBindInfo.stSrcChnPort.u32ChnId = s32Channel;
    }
    stBindInfo.stSrcChnPort.u32PortId = 0;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = s32Channel;
    stBindInfo.stDstChnPort.u32PortId = 0;
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    STCHECKRESULT(ST_Vif_StopPort(s32Channel, 0));

    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = s32Channel;
    stBindInfo.stSrcChnPort.u32PortId = 0;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VDISP;
    stBindInfo.stDstChnPort.u32DevId = DevId;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    if ((1 == s32Channel) && (E_ST_SPLIT_MODE_TWO == s32SplitMode))
    {
        stBindInfo.stDstChnPort.u32PortId = VDISP_OVERLAYINPUTPORTID;
    }
    else
    {
        stBindInfo.stDstChnPort.u32PortId = s32Channel;
    }
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));//UnBind Vpe to Vdisp

    //Stop Vpe Channel
    STCHECKRESULT(ST_Vpe_StopPort(s32Channel, VpePort));
    STCHECKRESULT(ST_Vpe_StopChannel(s32Channel));
    STCHECKRESULT(ST_Vpe_DestroyChannel(s32Channel));

    //Stop Vdisp Channel
    if ((1 == s32Channel) && (E_ST_SPLIT_MODE_TWO == s32SplitMode))
    {
        STCHECKRESULT(ST_Vdisp_DisableInputPort(DevId, VDISP_OVERLAYINPUTPORTID));
    }
    else
    {
        STCHECKRESULT(ST_Vdisp_DisableInputPort(DevId, s32Channel));
    }

    return MI_SUCCESS;
}

MI_S32 ST_Sys_DestroyNvrChn(MI_S32 s32Channel, MI_S32 s32SplitMode)
{
    MI_VDISP_DEV DevId = 0; //Only Support DevID == 0
    ST_Rect_t stRect;
    ST_Sys_BindInfo_t stBindInfo;

    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDEC;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = s32Channel;
    stBindInfo.stSrcChnPort.u32PortId = 0;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = s32Channel;
    stBindInfo.stDstChnPort.u32PortId = 0;
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));//UnBind Vdec to Divp

    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = s32Channel;
    stBindInfo.stSrcChnPort.u32PortId = 0;
    if (E_ST_SPLIT_MODE_TWO == s32SplitMode)
    {
        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = s32Channel;
        stBindInfo.stDstChnPort.u32PortId = 0;
        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));//UnBind Divp to Vpe

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = s32Channel;
        stBindInfo.stSrcChnPort.u32PortId = 0;
    }
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VDISP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0; //vdisp channel always equal zero
    if ((1 == s32Channel) && (E_ST_SPLIT_MODE_TWO == s32SplitMode))
    {
        stBindInfo.stDstChnPort.u32PortId = VDISP_OVERLAYINPUTPORTID;
    }
    else
    {
        stBindInfo.stDstChnPort.u32PortId = s32Channel;
    }
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo)); //UnBind Divp/Vpe to Vdisp
    STCHECKRESULT(MI_VDEC_StopChn(s32Channel));
    STCHECKRESULT(MI_VDEC_DestroyChn(s32Channel));

    STCHECKRESULT(MI_DIVP_StopChn(s32Channel));
    STCHECKRESULT(MI_DIVP_DestroyChn(s32Channel));

    //Stop Vpe Channel
    if (E_ST_SPLIT_MODE_TWO == s32SplitMode)
    {
        STCHECKRESULT(ST_Vpe_StopPort(s32Channel, 0));
        STCHECKRESULT(ST_Vpe_StopChannel(s32Channel));
        STCHECKRESULT(ST_Vpe_DestroyChannel(s32Channel));
    }
    //Destroy Vdisp Channel
    if ((1 == s32Channel) && (E_ST_SPLIT_MODE_TWO == s32SplitMode))
    {
        STCHECKRESULT(ST_Vdisp_DisableInputPort(DevId, VDISP_OVERLAYINPUTPORTID));
    }
    else
    {
        STCHECKRESULT(ST_Vdisp_DisableInputPort(DevId, s32Channel));
    }

    return MI_SUCCESS;
}

MI_U64 ST_Sys_GetPts(MI_U32 u32FrameRate)
{
    if (0 == u32FrameRate)
    {
        return (MI_U64)(-1);
    }

    return (MI_U64)(1000 / u32FrameRate);
}

MI_S32 ST_Get_VoRectBySplitMode(MI_S32 s32SplitMode, MI_S32 s32VoChnIndex, MI_U16 u16LayerW, MI_U16 u16LayerH, ST_Rect_t *pstRect)
{
    MI_U16 u16LayerWidth = u16LayerW;
    MI_U16 u16LayerHeight = u16LayerH;
    MI_S32 s32DivW = 0, s32DivH = 0;
    MI_U16 u16VoWidth = 0, u16VoHeight = 0;
    MI_S32 s32SupportChnNum = 0;

    switch (s32SplitMode)
    {
        case E_ST_SPLIT_MODE_ONE:
        {
            s32DivW = 1;
            s32DivH = 1;
            s32SupportChnNum = 1;
            if (0 != s32VoChnIndex)
            {
                ST_ERR("s32VoChnIndex err(%d)!!!\n", s32VoChnIndex);
                return -1;
            }
            else
            {
                pstRect->s32X = 0;
                pstRect->s32Y = 0;
                pstRect->u16PicW = u16LayerWidth;
                pstRect->u16PicH = u16LayerHeight;
            }
            break;
        }
        case E_ST_SPLIT_MODE_TWO:
        {
            s32DivW = 4;
            s32DivH = 4;
            s32SupportChnNum = 2;
            u16VoWidth = u16LayerWidth / 4;//align
            u16VoHeight = u16LayerHeight / 4;
            if (0 == s32VoChnIndex)
            {
                pstRect->s32X = 0;
                pstRect->s32Y = 0;
                pstRect->u16PicW = u16LayerWidth;
                pstRect->u16PicH = u16LayerHeight;
            }
            else if (1 == s32VoChnIndex)
            {
                pstRect->s32X = (s32DivW - 1) * u16VoWidth;
                pstRect->s32Y = (s32DivH - 1) * u16VoHeight;
                pstRect->u16PicW = u16VoWidth;
                pstRect->u16PicH = u16VoHeight;
            }
            break;
        }
        case E_ST_SPLIT_MODE_FOUR:
        {
            s32DivW = 2;
            s32DivH = 2;
            s32SupportChnNum = 4;
            u16VoWidth = u16LayerWidth / 2;//align
            u16VoHeight = u16LayerHeight / 2;
            if ((s32VoChnIndex < s32SupportChnNum) && (s32VoChnIndex >=0))
            {
                pstRect->s32X = (s32VoChnIndex % s32DivW) * u16VoWidth;
                pstRect->s32Y = (s32VoChnIndex / s32DivW) * u16VoHeight;
                pstRect->u16PicW = u16VoWidth;
                pstRect->u16PicH = u16VoHeight;
            }
            else
            {
                ST_ERR("s32VoChnIndex err(%d)!!!\n", s32VoChnIndex);
                return -1;
            }
            break;
        }
        case E_ST_SPLIT_MODE_NINE:
        {
            s32DivW = 3;
            s32DivH = 3;
            s32SupportChnNum = 9;
            u16VoWidth = u16LayerWidth / 3;//align
            u16VoHeight = u16LayerHeight / 3;
            if ((s32VoChnIndex < s32SupportChnNum) && (s32VoChnIndex >=0))
            {
                pstRect->s32X = (s32VoChnIndex % s32DivW) * u16VoWidth;
                pstRect->s32Y = (s32VoChnIndex / s32DivW) * u16VoHeight;
                pstRect->u16PicW = u16VoWidth;
                pstRect->u16PicH = u16VoHeight;
            }
            else
            {
                ST_ERR("s32VoChnIndex err(%d)!!!\n", s32VoChnIndex);
                return -1;
            }
            break;
        }
        case E_ST_SPLIT_MODE_NINE_EX: //1 big win + 5 litte win
        {
            s32DivW = 3;
            s32DivH = 3;
            s32SupportChnNum = 6;
            u16VoWidth = u16LayerWidth / 3;//align
            u16VoHeight = u16LayerHeight / 3;
            break;
        }
        default:
            ST_ERR("Unsupported split mode(%d)!!!\n", s32SplitMode);
            return -1;
    }

    return MI_SUCCESS;
}

MI_S32 ST_GetTimingInfo(MI_S32 s32ApTiming, MI_S32 *ps32HdmiTiming, MI_S32 *ps32DispTiming, MI_U16 *pu16DispW, MI_U16 *pu16DispH)
{
    switch (s32ApTiming)
    {
        case E_ST_TIMING_720P_50:
            *ps32DispTiming = E_MI_DISP_OUTPUT_720P50;
            *ps32HdmiTiming = E_MI_HDMI_TIMING_720_50P;
            *pu16DispW = 1280;
            *pu16DispH = 720;
            break;
        case E_ST_TIMING_720P_60:
            *ps32DispTiming = E_MI_DISP_OUTPUT_720P60;
            *ps32HdmiTiming = E_MI_HDMI_TIMING_720_60P;
            *pu16DispW = 1280;
            *pu16DispH = 720;
            break;
        case E_ST_TIMING_1080P_50:
            *ps32DispTiming = E_MI_DISP_OUTPUT_1080P50;
            *ps32HdmiTiming = E_MI_HDMI_TIMING_1080_50P;
            *pu16DispW = 1920;
            *pu16DispH = 1080;
            break;
        case E_ST_TIMING_1080P_60:
            *ps32DispTiming = E_MI_DISP_OUTPUT_1080P60;
            *ps32HdmiTiming = E_MI_HDMI_TIMING_1080_60P;
            *pu16DispW = 1920;
            *pu16DispH = 1080;
            break;
        case E_ST_TIMING_1600x1200_60:
            *ps32DispTiming = E_MI_DISP_OUTPUT_1600x1200_60;
            *ps32HdmiTiming = E_MI_HDMI_TIMING_1600x1200_60P;
            *pu16DispW = 1600;
            *pu16DispH = 1200;
            break;
        case E_ST_TIMING_1440x900_60:
            *ps32DispTiming = E_MI_DISP_OUTPUT_1440x900_60;
            *ps32HdmiTiming = E_MI_HDMI_TIMING_1440x900_60P;
            *pu16DispW = 1440;
            *pu16DispH = 900;
            break;
        case E_ST_TIMING_1280x1024_60:
            *ps32DispTiming = E_MI_DISP_OUTPUT_1280x1024_60;
            *ps32HdmiTiming = E_MI_HDMI_TIMING_1280x1024_60P;
            *pu16DispW = 1280;
            *pu16DispH = 1024;
            break;
        case E_ST_TIMING_1024x768_60:
            *ps32DispTiming = E_MI_DISP_OUTPUT_1024x768_60;
            *ps32HdmiTiming = E_MI_HDMI_TIMING_1024x768_60P;
            *pu16DispW = 1024;
            *pu16DispH = 768;
            break;
        case E_ST_TIMING_2560x1440_30:
            *ps32DispTiming = E_MI_DISP_OUTPUT_2560x1440_30;
            *ps32HdmiTiming = E_MI_HDMI_TIMING_1440_30P;
            *pu16DispW = 2560;
            *pu16DispH = 1440;
            break;
        case E_ST_TIMING_3840x2160_30:
            *ps32DispTiming = E_MI_DISP_OUTPUT_3840x2160_30;
            *ps32HdmiTiming = E_MI_HDMI_TIMING_4K2K_30P;
            *pu16DispW = 1920;
            *pu16DispH = 1080;
            break;
        case E_ST_TIMING_3840x2160_60:
            *ps32DispTiming = E_MI_DISP_OUTPUT_3840x2160_60;
            *ps32HdmiTiming = E_MI_HDMI_TIMING_4K2K_60P;
            *pu16DispW = 3840;
            *pu16DispH = 2160;
            break;
        default:
            ST_WARN("Unspport Ap timing (%d)\n", s32ApTiming);
            return -1;
    }

    return MI_SUCCESS;
}
