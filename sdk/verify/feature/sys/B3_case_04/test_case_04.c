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
#include <pthread.h>
#include <sys/prctl.h>
#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "mi_sys.h"
#include "mi_vif.h"
#include "mi_vpe.h"
#include "mi_venc.h"
#include "mi_divp.h"
#include "mi_sensor.h"


#define STCHECKRESULT(result)\
    if (result != MI_SUCCESS)\
    {\
        printf("[%s %d]exec function failed\n", __FUNCTION__, __LINE__);\
        return 1;\
    }\
    else\
    {\
        printf("(%s %d)exec function pass\n", __FUNCTION__,__LINE__);\
    }


typedef struct STUB_Resolution_s
{
    MI_U32 u32MaxWidth;
    MI_U32 u32MaxHeight;
    MI_U32 u32OutWidth;
    MI_U32 u32OutHeight;
} STUB_Resolution_t;

typedef struct STUB_VencRes_s
{
    MI_VENC_ModType_e eModType;
    MI_U32 u32DevId;
    MI_U32 u32ChnId;
    MI_U32 u32PortId;
    pthread_t tid;
    MI_BOOL bThreadRunning;
} STUB_VencRes_t;

#define VENC_FPS 30
#define VENC_BITRATE 2000000

#define STUB_VIF_CHN_NUM 1
#define STUB_VPE_OUTPUT_PORT_NUM 3
#define STUB_DIVP_CHN_NUM 2
#define STUB_VENC_CHN_NUM 3
static STUB_VencRes_t _stStubVencRes[STUB_VENC_CHN_NUM];

STUB_Resolution_t _stVifResolution[STUB_VIF_CHN_NUM] =
{
    {1920, 1080, 1920, 1080},
};

STUB_Resolution_t _stVpeResolution[STUB_VPE_OUTPUT_PORT_NUM] =
{
    {1920, 1080, 1920, 1080},
    {1920, 1080, 704, 576},
    {1920, 1080, 704, 576},
};

STUB_Resolution_t _stDivpResolution[STUB_DIVP_CHN_NUM] =
{
    {1920, 1080, 1920, 1080},
    {704, 576, 640, 480},
};

STUB_Resolution_t _stVencResolution[STUB_VENC_CHN_NUM] =
{
    {1920, 1080, 1920, 1080},
    {640, 480, 640, 480},
    {704, 576, 704, 576},
};



void *venc_channel_func(void* p)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_VENC_ChnStat_t stStat;
    MI_VENC_Stream_t stStream;
    MI_VENC_Pack_t stPack;
    MI_U32 ret = MI_SUCCESS;
    MI_U32 u32GetFramesCount = 0;

    STUB_VencRes_t *pstStubVencRes = (STUB_VencRes_t *)p;
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VENC;
    stChnPort.u32DevId = pstStubVencRes->u32DevId;
    stChnPort.u32ChnId = pstStubVencRes->u32ChnId;
    stChnPort.u32PortId = pstStubVencRes->u32PortId;

    while(pstStubVencRes->bThreadRunning)
    {
        memset(&stStream, 0, sizeof(stStream));
        memset(&stPack, 0, sizeof(stPack));
        stStream.pstPack = &stPack;
        stStream.u32PackCount = 1;
        ret = MI_VENC_Query(stChnPort.u32ChnId, &stStat);
        if(ret != MI_SUCCESS || stStat.u32CurPacks == 0)
        {
            usleep(5*1000);
            continue;
        }

        ret = MI_VENC_GetStream(stChnPort.u32ChnId, &stStream, FALSE);
        if(MI_SUCCESS == ret)
        {
            MI_VENC_ReleaseStream(stChnPort.u32ChnId, &stStream);

            ++u32GetFramesCount;
            printf("channelId[%u] u32GetFramesCount[%u]\n", stChnPort.u32ChnId, u32GetFramesCount);
        }
    }

    return NULL;
}

static MI_S32 enableVifDev(MI_U32 *pWidth, MI_U32 *pHeight, MI_SYS_PixelFormat_e *pPixeFormat)
{
    MI_SNR_PADInfo_t  stPad0Info;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
    MI_U32 u32ResCount =0;
    MI_U8 u8ResIndex =0;
    MI_SNR_Res_t stRes;
    MI_U32 u32ChocieRes =0;
    MI_VIF_DevAttr_t stDevAttr;
    MI_VIF_ChnPortAttr_t stVifChnPortAttr;

    memset(&stPad0Info, 0x0, sizeof(MI_SNR_PADInfo_t));
    memset(&stSnrPlane0Info, 0x0, sizeof(MI_SNR_PlaneInfo_t));
    memset(&stRes, 0x0, sizeof(MI_SNR_Res_t));

    MI_SNR_SetPlaneMode(E_MI_SNR_PAD_ID_0, FALSE);

    MI_SNR_QueryResCount(E_MI_SNR_PAD_ID_0, &u32ResCount);
    for(u8ResIndex=0; u8ResIndex < u32ResCount; u8ResIndex++)
    {
        MI_SNR_GetRes(E_MI_SNR_PAD_ID_0, u8ResIndex, &stRes);
        printf("index %d, Crop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, ResDesc %s\n",
        u8ResIndex,
        stRes.stCropRect.u16X, stRes.stCropRect.u16Y, stRes.stCropRect.u16Width,stRes.stCropRect.u16Height,
        stRes.stOutputSize.u16Width, stRes.stOutputSize.u16Height,
        stRes.u32MaxFps,stRes.u32MinFps,
        stRes.strResDesc);
    }

    printf("select 0 res\n");

    MI_SNR_SetRes(E_MI_SNR_PAD_ID_0,0);
    MI_SNR_Enable(E_MI_SNR_PAD_ID_0);

    MI_SNR_GetPadInfo(E_MI_SNR_PAD_ID_0, &stPad0Info);
    MI_SNR_GetPlaneInfo(E_MI_SNR_PAD_ID_0, 0, &stSnrPlane0Info);

    memset(&stDevAttr, 0x0, sizeof(MI_VIF_DevAttr_t));

    stDevAttr.eIntfMode = stPad0Info.eIntfMode;
    stDevAttr.eWorkMode = E_MI_VIF_WORK_MODE_RGB_REALTIME;
    stDevAttr.eHDRType = E_MI_VPE_HDR_TYPE_OFF;
    if(stDevAttr.eIntfMode == E_MI_VIF_MODE_BT656)
        stDevAttr.eClkEdge = stPad0Info.unIntfAttr.stBt656Attr.eClkEdge;
    else
        stDevAttr.eClkEdge = E_MI_VIF_CLK_EDGE_DOUBLE;

    if(stDevAttr.eIntfMode == E_MI_VIF_MODE_MIPI)
        stDevAttr.eDataSeq =stPad0Info.unIntfAttr.stMipiAttr.eDataYUVOrder;
    else
        stDevAttr.eDataSeq = E_MI_VIF_INPUT_DATA_YUYV;

    if(stDevAttr.eIntfMode == E_MI_VIF_MODE_BT656)
        memcpy(&stDevAttr.stSyncAttr, &stPad0Info.unIntfAttr.stBt656Attr.stSyncAttr, sizeof(MI_VIF_SyncAttr_t));

    stDevAttr.eBitOrder = E_MI_VIF_BITORDER_NORMAL;

    *pWidth = stSnrPlane0Info.stCapRect.u16Width;
    *pHeight = stSnrPlane0Info.stCapRect.u16Height;
    *pPixeFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);

    STCHECKRESULT(MI_VIF_SetDevAttr(0, &stDevAttr));
    STCHECKRESULT(MI_VIF_EnableDev(0));

    memset(&stVifChnPortAttr, 0, sizeof(MI_VIF_ChnPortAttr_t));
    stVifChnPortAttr.stCapRect.u16X = 0;
    stVifChnPortAttr.stCapRect.u16Y = 0;
    stVifChnPortAttr.stCapRect.u16Width  = stSnrPlane0Info.stCapRect.u16Width;
    stVifChnPortAttr.stCapRect.u16Height = stSnrPlane0Info.stCapRect.u16Height;
    stVifChnPortAttr.stDestSize.u16Width  = stSnrPlane0Info.stCapRect.u16Width;
    stVifChnPortAttr.stDestSize.u16Height = stSnrPlane0Info.stCapRect.u16Height;

    stVifChnPortAttr.ePixFormat = *pPixeFormat;//E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stVifChnPortAttr.eFrameRate = E_MI_VIF_FRAMERATE_FULL;//pstVifPortInfo->s32FrameRate;

    STCHECKRESULT(MI_VIF_SetChnPortAttr(0, 0, &stVifChnPortAttr));
    STCHECKRESULT(MI_VIF_EnableChnPort(0, 0));
    return 0;
}


static MI_S32 STUB_BaseModuleInit(void)
{
    MI_VIF_DevAttr_t stVifDevAttr;
    MI_VIF_ChnPortAttr_t stVifChnPortAttr;
    MI_U32 u32VifDevId;
    MI_U32 u32VifChnId;
    MI_U32 u32VifPortId;

    MI_VPE_ChannelAttr_t stVpeChnAttr;
    MI_VPE_PortMode_t stVpeMode;
    MI_U32 u32VpeDevId;
    MI_U32 u32VpeChnId;
    MI_U32 u32VpePortId;

    MI_VENC_ChnAttr_t stVencChnAttr;
    MI_U32 u32VencDevId;
    MI_U32 u32VencChnId;
    MI_U32 u32VencPortId;

    MI_DIVP_ChnAttr_t stDivpChnAttr;
    MI_DIVP_OutputPortAttr_t stDivpOutputPortAttr;
    MI_U32 u32DivpDevId;
    MI_U32 u32DivpChnId;
    MI_U32 u32DivpPortId;

    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32SrcFrmrate;
    MI_U32 u32DstFrmrate;

    MI_SYS_BindType_e eBindType;
    MI_U32 u32RingLineCnt;

    MI_U32 u32Index;

    MI_U32 u32Width, u32Height;
    MI_SYS_PixelFormat_e eSensorRawType;

    /************************************************
    Step1:  init SYS
    *************************************************/
    STCHECKRESULT(MI_SYS_Init());

    /************************************************
    Step2:  init VIF
    *************************************************/
    u32VifDevId = 0;
    u32VifChnId = 0;
    u32VifPortId = 0;
    enableVifDev(&u32Width, &u32Height, &eSensorRawType);

    /************************************************
    Step3:  init VPE
    *************************************************/
    u32VpeDevId = 0;
    u32VpeChnId = 0;
    memset(&stVpeChnAttr, 0x0, sizeof(MI_VPE_ChannelAttr_t));
    stVpeChnAttr.u16MaxW = u32Width;
    stVpeChnAttr.u16MaxH = u32Height;
    stVpeChnAttr.bNrEn = FALSE;
    stVpeChnAttr.bEsEn = FALSE;
    stVpeChnAttr.bEdgeEn = FALSE;
    stVpeChnAttr.bUvInvert = FALSE;
    stVpeChnAttr.bContrastEn = FALSE;
    stVpeChnAttr.ePixFmt  = eSensorRawType;
    stVpeChnAttr.eHDRType = E_MI_VPE_HDR_TYPE_OFF;
    stVpeChnAttr.eRunningMode = E_MI_VPE_RUN_REALTIME_MODE;
    STCHECKRESULT(MI_VPE_CreateChannel(u32VpeChnId, &stVpeChnAttr));
    STCHECKRESULT(MI_VPE_StartChannel(u32VpeChnId));

    for(u32VpePortId=0; u32VpePortId<STUB_VPE_OUTPUT_PORT_NUM; ++u32VpePortId)
    {
        memset(&stVpeMode, 0x00, sizeof(stVpeMode));
        stVpeMode.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        stVpeMode.ePixelFormat  = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        stVpeMode.u16Width  = _stVpeResolution[u32VpePortId].u32OutWidth;
        stVpeMode.u16Height = _stVpeResolution[u32VpePortId].u32OutHeight;

        STCHECKRESULT(MI_VPE_SetPortMode(u32VpeChnId, u32VpePortId, &stVpeMode));
        STCHECKRESULT(MI_VPE_GetPortMode(u32VpeChnId, u32VpePortId, &stVpeMode));
        printf("Get VpeMode(port:%d w:%d h:%d PixelFormat:%d CompressMode:%d)\n",
               u32VpePortId, stVpeMode.u16Width, stVpeMode.u16Height, stVpeMode.ePixelFormat, stVpeMode.eCompressMode);
        STCHECKRESULT(MI_VPE_EnablePort(u32VpeChnId, u32VpePortId));
    }

    /************************************************
    Step3:  init DIVP
    *************************************************/
    u32DivpDevId = 0;
    u32DivpPortId = 0;
    for(u32DivpChnId=0; u32DivpChnId<STUB_DIVP_CHN_NUM; ++u32DivpChnId)
    {
        memset(&stDivpChnAttr, 0x0, sizeof(MI_DIVP_ChnAttr_t));
        stDivpChnAttr.bHorMirror = FALSE;
        stDivpChnAttr.bVerMirror = FALSE;
        stDivpChnAttr.eDiType = E_MI_DIVP_DI_TYPE_OFF;
        stDivpChnAttr.eRotateType = E_MI_SYS_ROTATE_NONE;
        stDivpChnAttr.eTnrLevel = E_MI_DIVP_TNR_LEVEL_OFF;
        stDivpChnAttr.stCropRect.u16X = 0;
        stDivpChnAttr.stCropRect.u16Y = 0;
        stDivpChnAttr.stCropRect.u16Width = _stDivpResolution[u32DivpChnId].u32MaxWidth;
        stDivpChnAttr.stCropRect.u16Height = _stDivpResolution[u32DivpChnId].u32MaxHeight;
        stDivpChnAttr.u32MaxWidth = _stDivpResolution[u32DivpChnId].u32MaxWidth;
        stDivpChnAttr.u32MaxHeight = _stDivpResolution[u32DivpChnId].u32MaxHeight;
        STCHECKRESULT(MI_DIVP_CreateChn(u32DivpChnId, &stDivpChnAttr));
        STCHECKRESULT(MI_DIVP_SetChnAttr(u32DivpChnId, &stDivpChnAttr));

        memset(&stDivpOutputPortAttr, 0, sizeof(MI_DIVP_OutputPortAttr_t));
        stDivpOutputPortAttr.eCompMode = E_MI_SYS_COMPRESS_MODE_NONE;
        stDivpOutputPortAttr.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        stDivpOutputPortAttr.u32Width = _stDivpResolution[u32DivpChnId].u32OutWidth;
        stDivpOutputPortAttr.u32Height = _stDivpResolution[u32DivpChnId].u32OutHeight;
        STCHECKRESULT(MI_DIVP_SetOutputPortAttr(u32DivpChnId, &stDivpOutputPortAttr));

        STCHECKRESULT(MI_DIVP_StartChn(u32DivpChnId));
    }


    /************************************************
    Step4:  init VENC
    *************************************************/
    u32VencDevId = 0;
    u32VencPortId = 0;
    for(u32VencChnId=0; u32VencChnId<STUB_VENC_CHN_NUM; ++u32VencChnId)
    {
        memset(&stVencChnAttr, 0x0, sizeof(MI_VENC_ChnAttr_t));
        stVencChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
        stVencChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = _stVencResolution[u32VencChnId].u32MaxWidth;
        stVencChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = _stVencResolution[u32VencChnId].u32MaxHeight;
        stVencChnAttr.stVeAttr.stAttrH264e.u32PicWidth= _stVencResolution[u32VencChnId].u32OutWidth;
        stVencChnAttr.stVeAttr.stAttrH264e.u32PicHeight = _stVencResolution[u32VencChnId].u32OutHeight;
        stVencChnAttr.stVeAttr.stAttrH264e.bByFrame = TRUE;
        stVencChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2;
        stVencChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
        if(stVencChnAttr.stVeAttr.stAttrH264e.u32PicWidth >= 1920 || stVencChnAttr.stVeAttr.stAttrH264e.u32PicHeight  >= 1080)
        {
            stVencChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = 3000000;
        }
        else
        {
            stVencChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = 1000000;
        }
        stVencChnAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
        stVencChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = VENC_FPS * 2;
        stVencChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = VENC_FPS;
        stVencChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
        stVencChnAttr.stRcAttr.stAttrH264Cbr.u32StatTime = 0;
        stVencChnAttr.stVeAttr.eType  = E_MI_VENC_MODTYPE_H264E;
        STCHECKRESULT(MI_VENC_CreateChn(u32VencChnId, &stVencChnAttr));
        STCHECKRESULT(MI_VENC_StartRecvPic(u32VencChnId));
    }

    /************************************************
    Step5:  Bind src(devId, chnId, portId)->dst(devId, chnId, portId)
    *************************************************/

    //Bind VIF(0, 0, 0)->VPE(0, 0, 0)
    u32VifDevId = 0;
    u32VifChnId = 0;
    u32VifPortId = 0;
    u32VpeDevId = 0;
    u32VpeChnId = 0;
    u32VpePortId = 0;
    memset(&stSrcChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    memset(&stDstChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stSrcChnPort.u32DevId = u32VifDevId;
    stSrcChnPort.u32ChnId = u32VifChnId;
    stSrcChnPort.u32PortId = u32VifPortId;
    stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stDstChnPort.u32DevId = u32VpeDevId;
    stDstChnPort.u32ChnId = u32VpeChnId;
    stDstChnPort.u32PortId = u32VpePortId;
    u32SrcFrmrate = 30;
    u32DstFrmrate = 30;
    eBindType = E_MI_SYS_BIND_TYPE_REALTIME;//E_MI_SYS_BIND_TYPE_FRAME_BASE;
    STCHECKRESULT(MI_SYS_BindChnPort2(&stSrcChnPort, &stDstChnPort, u32SrcFrmrate, u32DstFrmrate, eBindType, 0));

    //bind vpe(0, 0, 0..1)->divp(0, 0..1, 0), divp(0, 0..1, 0)->venc(0, 0..1, 0)
    u32DivpDevId = 0;
    u32DivpPortId = 0;
    u32VpeDevId = 0;
    u32VpeChnId = 0;
    u32VencDevId = 0;
    u32VencPortId = 0;
    for(u32Index=0; u32Index<STUB_DIVP_CHN_NUM; ++u32Index)
    {
        u32VpePortId = u32Index;
        u32DivpChnId = u32Index;
        u32VencChnId = u32Index;

        //bind vpe(0, 0, 0..1)->divp(0, 0..1, 0)
        memset(&stSrcChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
        memset(&stDstChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
        stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stSrcChnPort.u32DevId = u32VpeDevId;
        stSrcChnPort.u32ChnId = u32VpeChnId;
        stSrcChnPort.u32PortId = u32VpePortId;
        stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stDstChnPort.u32DevId = u32DivpDevId;
        stDstChnPort.u32ChnId = u32DivpChnId;
        stDstChnPort.u32PortId = u32DivpPortId;
        u32SrcFrmrate = 30;
        u32DstFrmrate = 30;
        eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
        STCHECKRESULT(MI_SYS_BindChnPort2(&stSrcChnPort, &stDstChnPort, u32SrcFrmrate, u32DstFrmrate, eBindType, 0));

        //bind divp(0, 0..1, 0)->venc(0, 0..1, 0)
        memset(&stSrcChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
        memset(&stDstChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
        stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stSrcChnPort.u32DevId = u32DivpDevId;
        stSrcChnPort.u32ChnId = u32DivpChnId;
        stSrcChnPort.u32PortId = u32DivpPortId;
        stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
        stDstChnPort.u32DevId = u32VencDevId;
        stDstChnPort.u32ChnId = u32VencChnId;
        stDstChnPort.u32PortId = u32VencPortId;
        u32SrcFrmrate = 30;
        u32DstFrmrate = 30;
        eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
        STCHECKRESULT(MI_SYS_BindChnPort2(&stSrcChnPort, &stDstChnPort, u32SrcFrmrate, u32DstFrmrate, eBindType, 0));
    }

    //bind vpe(0, 0, 2)->venc(0, 2, 0)
    u32VpePortId = 2;
    u32VencChnId = 2;
    memset(&stSrcChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    memset(&stDstChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stSrcChnPort.u32DevId = u32VpeDevId;
    stSrcChnPort.u32ChnId = u32VpeChnId;
    stSrcChnPort.u32PortId = u32VpePortId;
    stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stDstChnPort.u32DevId = u32VencDevId;
    stDstChnPort.u32ChnId = u32VencChnId;
    stDstChnPort.u32PortId = u32VencPortId;
    u32SrcFrmrate = 30;
    u32DstFrmrate = 30;
    eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    STCHECKRESULT(MI_SYS_BindChnPort2(&stSrcChnPort, &stDstChnPort, u32SrcFrmrate, u32DstFrmrate, eBindType, u32RingLineCnt));

    for(u32VencChnId=0; u32VencChnId<STUB_VENC_CHN_NUM; ++u32VencChnId)
    {
        STCHECKRESULT(MI_VENC_GetChnDevid(u32VencChnId, &u32VencDevId));
        _stStubVencRes[u32VencChnId].u32DevId = u32VencDevId;
        _stStubVencRes[u32VencChnId].u32ChnId = u32VencChnId;
        _stStubVencRes[u32VencChnId].u32PortId = u32VencPortId;
        _stStubVencRes[u32VencChnId].bThreadRunning = TRUE;
        pthread_create(&_stStubVencRes[u32VencChnId].tid, NULL, venc_channel_func, &_stStubVencRes[u32VencChnId]);
    }

    return MI_SUCCESS;
}

static MI_S32 STUB_BaseModuleDeinit(void)
{
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32VifDevId;
    MI_U32 u32VifChnId;
    MI_U32 u32VifPortId;
    MI_U32 u32VpeDevId;
    MI_U32 u32VpeChnId;
    MI_U32 u32VpePortId;
    MI_U32 u32DivpDevId;
    MI_U32 u32DivpChnId;
    MI_U32 u32DivpPortId;
    MI_U32 u32VencDevId;
    MI_U32 u32VencChnId;
    MI_U32 u32VencPortId;

    MI_U32 u32Index;

    for(u32VencChnId=0; u32VencChnId<STUB_VENC_CHN_NUM; ++u32VencChnId)
    {
        _stStubVencRes[u32VencChnId].bThreadRunning = FALSE;
        pthread_join(_stStubVencRes[u32VencChnId].tid, NULL);
    }

    //unbind: divp(0, 0..1, 0)-x->venc(0, 0..1, 0) vpe(0, 0..1, 0)-x->divp(0, 0..1, 0)
    u32VpeDevId = 0;
    u32VpeChnId = 0;
    u32DivpDevId = 0;
    u32DivpPortId = 0;
    u32VencDevId = 0;
    u32VencPortId = 0;
    for(u32Index=0; u32Index<STUB_DIVP_CHN_NUM; ++u32Index)
    {
        u32VpePortId = u32Index;
        u32DivpChnId = u32Index;
        u32VencChnId = u32Index;

        //unbind divp(0, 0..1, 0)->venc(0, 0..1, 0)
        memset(&stSrcChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
        memset(&stDstChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
        stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stSrcChnPort.u32DevId = u32DivpDevId;
        stSrcChnPort.u32ChnId = u32DivpChnId;
        stSrcChnPort.u32PortId = u32DivpPortId;
        stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
        stDstChnPort.u32DevId = u32VencDevId;
        stDstChnPort.u32ChnId = u32VencChnId;
        stDstChnPort.u32PortId = u32VencPortId;
        STCHECKRESULT(MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort));

        //unbind vpe(0, 0, 0..1)->divp(0, 0..1, 0)
        memset(&stSrcChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
        memset(&stDstChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
        stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stSrcChnPort.u32DevId = u32VpeDevId;
        stSrcChnPort.u32ChnId = u32VpeChnId;
        stSrcChnPort.u32PortId = u32VpePortId;
        stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stDstChnPort.u32DevId = u32DivpDevId;
        stDstChnPort.u32ChnId = u32DivpChnId;
        stDstChnPort.u32PortId = u32DivpPortId;
        STCHECKRESULT(MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort));
    }

     //unbind vpe(0, 0, 2)->venc(0, 2, 0)
    u32VpePortId = 2;
    u32VencChnId = 2;
    memset(&stSrcChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    memset(&stDstChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stSrcChnPort.u32DevId = u32VpeDevId;
    stSrcChnPort.u32ChnId = u32VpeChnId;
    stSrcChnPort.u32PortId = u32VpePortId;
    stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stDstChnPort.u32DevId = u32VencDevId;
    stDstChnPort.u32ChnId = u32VencChnId;
    stDstChnPort.u32PortId = u32VencPortId;
    STCHECKRESULT(MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort));

    u32VifDevId = 0;
    u32VifChnId = 0;
    u32VifPortId = 0;
    u32VpeDevId = 0;
    u32VpeChnId = 0;
    u32VpePortId = 0;
    memset(&stSrcChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    memset(&stDstChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stSrcChnPort.u32DevId = u32VifDevId;
    stSrcChnPort.u32ChnId = u32VifChnId;
    stSrcChnPort.u32PortId = u32VifPortId;
    stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stDstChnPort.u32DevId = u32VpeDevId;
    stDstChnPort.u32ChnId = u32VpeChnId;
    stDstChnPort.u32PortId = u32VpePortId;
    STCHECKRESULT(MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort));

    for(u32VencChnId=0; u32VencChnId<STUB_VENC_CHN_NUM; ++u32VencChnId)
    {
        STCHECKRESULT(MI_VENC_StopRecvPic(u32VencChnId));
        STCHECKRESULT(MI_VENC_DestroyChn(u32VencChnId));
    }

    for(u32DivpChnId=0; u32DivpChnId<STUB_DIVP_CHN_NUM; ++u32DivpChnId)
    {
        STCHECKRESULT(MI_DIVP_StopChn(u32DivpChnId));
        STCHECKRESULT(MI_DIVP_DestroyChn(u32DivpChnId));
    }

    u32VpeChnId = 0;
    for(u32VpePortId=0; u32VpePortId<STUB_VPE_OUTPUT_PORT_NUM; ++u32VpePortId)
    {
        STCHECKRESULT(MI_VPE_DisablePort(u32VpeChnId, u32VpePortId));
    }
    STCHECKRESULT(MI_VPE_StopChannel(u32VpeChnId));
    STCHECKRESULT(MI_VPE_DestroyChannel(u32VpeChnId));

    STCHECKRESULT(MI_VIF_DisableChnPort(u32VifChnId, u32VifPortId));
    STCHECKRESULT(MI_VIF_DisableDev(u32VifDevId));

    STCHECKRESULT(MI_SYS_Exit());
    return MI_SUCCESS;
}

MI_S32 main(int argc, char **argv)
{
    char ch = '\0';

    STCHECKRESULT(STUB_BaseModuleInit());

    printf("press q to exit\n");
    while((ch = getchar()) != 'q')
    {
        printf("press q to exit\n");
        usleep(1*1000*1000);
    }
    printf("exit\n");

    STCHECKRESULT(STUB_BaseModuleDeinit());

    return 0;
}



