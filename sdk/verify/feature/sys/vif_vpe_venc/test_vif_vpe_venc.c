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

typedef enum
{
   STUB_SENSOR_TYPE_IMX323 = 0,
   STUB_SENSOR_TYPE_IMX291 = 1,
   STUB_SENSOR_TYPE_IMX307 = 2,
}STUB_SensorType_e;


#define STUB_VENC_CHN_NUM 4

STUB_VencRes_t _stStubVencRes[STUB_VENC_CHN_NUM];
STUB_Resolution_t _stVifResolution[1] =
{
    {1920, 1088, 1920, 1088},
};

STUB_Resolution_t _stVpeResolution[STUB_VENC_CHN_NUM] =
{
    {1920, 1088, 1920, 1088},
    {1920, 1088, 640, 480},
    {1920, 1088, 640, 480},
    {1920, 1088, 704, 576},
};


STUB_Resolution_t _stVencResolution[STUB_VENC_CHN_NUM] =
{
    {1920, 1088, 1920, 1088},
    {640,  480,  640,  480},
    {640,  480,  640,  480},
    {704,  576,  704,  576},
};



static MI_VENC_ModType_e _aeModType[STUB_VENC_CHN_NUM] = {E_MI_VENC_MODTYPE_H264E};
static MI_U32 _u32FrameRate = 30;
static MI_U32 _u32ChnNum = 1;
static MI_U32 _u32RingLineCnt = 1088;
static MI_SYS_BindType_e _aeBindType[STUB_VENC_CHN_NUM] = {E_MI_SYS_BIND_TYPE_FRAME_BASE};
static MI_SYS_PixelFormat_e _eVpePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;

static MI_U32 _u32VpePortIdInUse = 0;


static MI_U32 _u32CropWidth = 0;
static MI_U32 _u32CropHeight = 0;

static STUB_SensorType_e _u32SensorType = STUB_SENSOR_TYPE_IMX323;

static MI_BOOL _bWriteFile = FALSE;

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

static MI_S32 STUB_GetVencConfig(MI_VENC_ModType_e eModType, MI_VENC_ChnAttr_t *pstVencChnAttr, MI_U32 u32ChnId)
{
    switch(eModType)
    {
        case E_MI_VENC_MODTYPE_H264E:
        {
            pstVencChnAttr->stVeAttr.eType = eModType;
            pstVencChnAttr->stVeAttr.stAttrH264e.u32PicWidth = _stVencResolution[u32ChnId].u32MaxWidth;
            pstVencChnAttr->stVeAttr.stAttrH264e.u32PicHeight = _stVencResolution[u32ChnId].u32MaxHeight;
            pstVencChnAttr->stVeAttr.stAttrH264e.u32MaxPicWidth = _stVencResolution[u32ChnId].u32OutWidth;
            pstVencChnAttr->stVeAttr.stAttrH264e.u32MaxPicHeight = _stVencResolution[u32ChnId].u32OutHeight;
            pstVencChnAttr->stVeAttr.stAttrH264e.bByFrame = TRUE;

            pstVencChnAttr->stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
            pstVencChnAttr->stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum = 30;
            pstVencChnAttr->stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = 1;
            pstVencChnAttr->stRcAttr.stAttrH264FixQp.u32Gop = 30;
            pstVencChnAttr->stRcAttr.stAttrH264FixQp.u32IQp = 25;
            pstVencChnAttr->stRcAttr.stAttrH264FixQp.u32PQp = 25;
        }
        break;
        case E_MI_VENC_MODTYPE_H265E:
        {
            pstVencChnAttr->stVeAttr.eType = eModType;
            pstVencChnAttr->stVeAttr.stAttrH265e.u32PicWidth = _stVencResolution[u32ChnId].u32MaxWidth;
            pstVencChnAttr->stVeAttr.stAttrH265e.u32PicHeight = _stVencResolution[u32ChnId].u32MaxHeight;
            pstVencChnAttr->stVeAttr.stAttrH265e.u32MaxPicWidth = _stVencResolution[u32ChnId].u32OutWidth;
            pstVencChnAttr->stVeAttr.stAttrH265e.u32MaxPicHeight = _stVencResolution[u32ChnId].u32OutHeight;
            pstVencChnAttr->stVeAttr.stAttrH265e.bByFrame = TRUE;

            pstVencChnAttr->stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265FIXQP;
            pstVencChnAttr->stRcAttr.stAttrH265FixQp.u32SrcFrmRateNum = 30;
            pstVencChnAttr->stRcAttr.stAttrH265FixQp.u32SrcFrmRateDen = 1;
            pstVencChnAttr->stRcAttr.stAttrH265FixQp.u32Gop = 30;
            pstVencChnAttr->stRcAttr.stAttrH265FixQp.u32IQp = 25;
            pstVencChnAttr->stRcAttr.stAttrH265FixQp.u32PQp = 25;
        }
        break;
        case E_MI_VENC_MODTYPE_JPEGE:
        {
            pstVencChnAttr->stVeAttr.eType = eModType;
            pstVencChnAttr->stVeAttr.stAttrJpeg.u32MaxPicWidth = _stVencResolution[u32ChnId].u32MaxWidth;
            pstVencChnAttr->stVeAttr.stAttrJpeg.u32MaxPicHeight = _stVencResolution[u32ChnId].u32MaxHeight;
            pstVencChnAttr->stVeAttr.stAttrJpeg.u32PicWidth = _stVencResolution[u32ChnId].u32OutWidth;
            pstVencChnAttr->stVeAttr.stAttrJpeg.u32PicHeight = _stVencResolution[u32ChnId].u32OutHeight;
            pstVencChnAttr->stVeAttr.stAttrJpeg.bByFrame = TRUE;
            pstVencChnAttr->stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
        }
        break;
        default:
            printf("unsupport eModType[%u]\n", eModType);
            return E_MI_ERR_FAILED;
    }

    return MI_SUCCESS;
}

void *venc_channel_func(void* p)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_U32 u32GetFramesCount;
    char filename[128];
    FILE *pfile = NULL;
    MI_VENC_ChnStat_t stStat;
    MI_VENC_Stream_t stStream;
    MI_VENC_Pack_t stPack;
    MI_U32 ret = MI_SUCCESS; 

    STUB_VencRes_t *pstStubVencRes = (STUB_VencRes_t *)p;
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VENC;
    stChnPort.u32DevId = pstStubVencRes->u32DevId;
    stChnPort.u32ChnId = pstStubVencRes->u32ChnId;
    stChnPort.u32PortId = pstStubVencRes->u32PortId;

    sprintf(filename, "venc%d.es", stChnPort.u32ChnId);
    printf("start to record %s\n", filename);
    pfile = fopen(filename, "wb");
    if(NULL == pfile)
    {
        printf("error: fopen venc.es failed\n");
    }

    u32GetFramesCount = 0;
    MI_SYS_SetChnOutputPortDepth(&stChnPort,5,20);
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
            if(pfile)
            {
                if(_bWriteFile)
                {
                    printf("write len = %u\n",stStream.pstPack->u32Len);
                    fwrite(stStream.pstPack->pu8Addr, 1,  (MI_S32)stStream.pstPack->u32Len, pfile);
                    fflush(pfile);
                }
            }
            MI_VENC_ReleaseStream(stChnPort.u32ChnId, &stStream);

            ++u32GetFramesCount;
            printf("channelId[%u] u32GetFramesCount[%u]\n", stChnPort.u32ChnId, u32GetFramesCount);
            //usleep(1000/_u32FrameRate *1000);
        }
    }

    if(pfile)
    {
        fclose(pfile);
    }
    printf("exit record\n");
    return NULL;
}

static MI_S32 STUB_BaseModuleInit(void)
{
    MI_U32 u32VifDevId;
    MI_U32 u32VifChnId;
    MI_U32 u32VifPortId;

    MI_VPE_ChannelAttr_t stVpeChnAttr;
    MI_VPE_PortMode_t stVpePortMode;
    MI_U32 u32VpeDevId;
    MI_U32 u32VpeChnId;
    MI_U32 u32VpePortId;

    MI_VENC_ChnAttr_t stVencChnAttr;
    MI_U32 u32VencDevId;
    MI_U32 u32VencChnId;
    MI_U32 u32VencPortId;

    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32SrcFrmrate;
    MI_U32 u32DstFrmrate;
    MI_SYS_BindType_e eBindType;

    MI_SYS_WindowRect_t stVpeCrop;
    MI_U32 u32Width, u32Height;
    MI_SYS_PixelFormat_e eSensorRawType;

    char ch;

    int i;


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
    printf("vpe raw type = %d\n", stVpeChnAttr.ePixFmt );
    STCHECKRESULT(MI_VPE_CreateChannel(u32VpeChnId, &stVpeChnAttr));
    STCHECKRESULT(MI_VPE_StartChannel(u32VpeChnId));
    //STCHECKRESULT(MI_VPE_EnablePort(u32VpeChnId, u32VpePortId));

    for(u32VpePortId=0; u32VpePortId<_u32ChnNum; u32VpePortId++)
    {
        memset(&stVpePortMode, 0x00, sizeof(stVpePortMode));
        stVpePortMode.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        stVpePortMode.ePixelFormat  = _eVpePixelFormat;
        stVpePortMode.u16Width  = _stVpeResolution[u32VpePortId].u32OutWidth;
        stVpePortMode.u16Height = _stVpeResolution[u32VpePortId].u32OutHeight;
        printf("[%s][%d] ePixelFormat[%u] u16Width[%u]\n", __FUNCTION__, __LINE__,
            stVpePortMode.ePixelFormat, stVpePortMode.u16Width, stVpePortMode.u16Height);

        if(_u32ChnNum == 1)
        {
            if(_u32VpePortIdInUse == 2)
            {
                //before config port2, config port1
                if(_aeBindType[u32VpePortId] == E_MI_SYS_BIND_TYPE_REALTIME || _aeBindType[u32VpePortId] == E_MI_SYS_BIND_TYPE_HW_RING)
                {
                    printf("in realtime/ring mode, need config port1 firstly\n");
                    printf("[%s][%d] u32VpePortId[1] MI_VPE_SetPortMode\n", __FUNCTION__, __LINE__);
                    STCHECKRESULT(MI_VPE_SetPortMode(u32VpeChnId, 1, &stVpePortMode));
                }
            }

            u32VpePortId = _u32VpePortIdInUse;
            printf("[%s][%d] u32VpePortId[%u]\n", __FUNCTION__, __LINE__, u32VpePortId);
        }

        printf("[%s][%d] u32VpePortId[%u] MI_VPE_SetPortMode\n", __FUNCTION__, __LINE__, u32VpePortId);
        STCHECKRESULT(MI_VPE_SetPortMode(u32VpeChnId, u32VpePortId, &stVpePortMode));
        //STCHECKRESULT(MI_VPE_GetPortMode(u32VpeChnId, u32VpePortId, &stVpePortMode));

        memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
        stChnPort.eModId = E_MI_MODULE_ID_VPE;
        stChnPort.u32DevId = u32VpeDevId;
        stChnPort.u32ChnId = u32VpeChnId;
        stChnPort.u32PortId = u32VpePortId;
        STCHECKRESULT(MI_SYS_SetChnOutputPortDepth(&stChnPort, 0, 5));
    }

    if(_u32CropWidth > 0 && _u32CropHeight > 0)
    {
        memset(&stVpeCrop, 0x0, sizeof(MI_SYS_WindowRect_t));
        stVpeCrop.u16X = 0;
        stVpeCrop.u16Y = 0;
        stVpeCrop.u16Width = _u32CropWidth;
        stVpeCrop.u16Height = _u32CropHeight;
        //MI_VPE_SetChannelCrop(u32VpeChnId,  &stVpeCrop);
        MI_VPE_SetPortCrop(u32VpeChnId, _u32VpePortIdInUse, &stVpeCrop);
        printf("vpe port[%u] crop u16Width = %u, u16Height = %u\n", _u32VpePortIdInUse, stVpeCrop.u16Width, stVpeCrop.u16Height);
    }

    /************************************************
    Step4:  init VENC
    *************************************************/
    u32VencDevId = 0;
    u32VencChnId = 0;
    u32VencPortId = 0;
    for(u32VencChnId=0; u32VencChnId<_u32ChnNum; u32VencChnId++)
    {
        memset(&stVencChnAttr, 0x0, sizeof(MI_VENC_ChnAttr_t));
        STUB_GetVencConfig(_aeModType[u32VencChnId], &stVencChnAttr, u32VencChnId);
        STCHECKRESULT(MI_VENC_CreateChn(u32VencChnId, &stVencChnAttr));
    }


    /************************************************
    Step5:  Bind VIF->VPE
    *************************************************/
    memset(&stSrcChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    memset(&stDstChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stSrcChnPort.u32DevId = u32VifDevId;
    stSrcChnPort.u32ChnId = u32VifChnId;
    stSrcChnPort.u32PortId = u32VifPortId;
    stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stDstChnPort.u32DevId = u32VpeDevId;
    stDstChnPort.u32ChnId = u32VpeChnId;
    stDstChnPort.u32PortId = 0;
    u32SrcFrmrate = 30;
    u32DstFrmrate = 30;
    eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
    STCHECKRESULT(MI_SYS_BindChnPort2(&stSrcChnPort, &stDstChnPort, u32SrcFrmrate, u32DstFrmrate, eBindType, 0));


    /************************************************
    Step6:  Bind VPE->VENC
    *************************************************/
    for(i=0; i<_u32ChnNum; ++i)
    {
        memset(&stSrcChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
        memset(&stDstChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
        stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stSrcChnPort.u32DevId = u32VpeDevId;
        stSrcChnPort.u32ChnId = u32VpeChnId;
        if(_u32ChnNum==1)
            stSrcChnPort.u32PortId = _u32VpePortIdInUse;
        else
            stSrcChnPort.u32PortId = i;


        printf("[%s][%d] u32VpePortId[%u] VencChnNum[%u] bindType[%u] _u32RingLineCnt[%u]\n", __FUNCTION__, __LINE__,
            stSrcChnPort.u32PortId, i, _aeBindType[i], _u32RingLineCnt);
        stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
        STCHECKRESULT(MI_VENC_GetChnDevid(i, &u32VencDevId));
        stDstChnPort.u32DevId = u32VencDevId;
        stDstChnPort.u32ChnId = i;
        stDstChnPort.u32PortId = u32VencPortId;
        u32SrcFrmrate = 30;
        u32DstFrmrate = 30;
        STCHECKRESULT(MI_SYS_BindChnPort2(&stSrcChnPort, &stDstChnPort, u32SrcFrmrate, u32DstFrmrate, _aeBindType[i], _u32RingLineCnt));
    }

    for(u32VencChnId=0; u32VencChnId<_u32ChnNum; u32VencChnId++)
    {
        STCHECKRESULT(MI_VENC_StartRecvPic(u32VencChnId));
    }

    for(u32VpePortId=0; u32VpePortId<_u32ChnNum; u32VpePortId++)
    {
        if(_u32ChnNum==1)
            u32VpePortId = _u32VpePortIdInUse;

        printf("[%s][%d] u32VpePortId[%u] MI_VPE_EnablePort\n", __FUNCTION__, __LINE__, u32VpePortId);
        STCHECKRESULT(MI_VPE_EnablePort(u32VpeChnId, u32VpePortId));
    }

    for(u32VencChnId=0; u32VencChnId<_u32ChnNum; u32VencChnId++)
    {
        _stStubVencRes[u32VencChnId].eModType = _aeModType[u32VencChnId];
        STCHECKRESULT(MI_VENC_GetChnDevid(u32VencChnId, &u32VencDevId));
        _stStubVencRes[u32VencChnId].u32DevId = u32VencDevId;
        _stStubVencRes[u32VencChnId].u32ChnId = u32VencChnId;
        _stStubVencRes[u32VencChnId].u32PortId = u32VencPortId;
        _stStubVencRes[u32VencChnId].bThreadRunning = TRUE;
        pthread_create(&_stStubVencRes[u32VencChnId].tid, NULL, venc_channel_func, &_stStubVencRes[u32VencChnId]);
    }

    printf("press q to exit\n");
    while((ch = getchar()) != 'q')
    {
        printf("press q to exit\n");
        usleep(1*1000*1000);
    }
    printf("exit\n");

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
    MI_U32 u32VencDevId;
    MI_U32 u32VencChnId;
    MI_U32 u32VencPortId;

    int i;

    for(u32VencChnId=0; u32VencChnId<_u32ChnNum; u32VencChnId++)
    {
        _stStubVencRes[u32VencChnId].bThreadRunning = FALSE;
        pthread_join(_stStubVencRes[u32VencChnId].tid, NULL);
    }

    u32VpeDevId = 0;
    u32VpeChnId = 0;
    u32VencDevId = 0;
    u32VencPortId = 0;
    for(i=0; i<_u32ChnNum; i++)
    {
        memset(&stSrcChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
        memset(&stDstChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
        stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stSrcChnPort.u32DevId = u32VpeDevId;
        stSrcChnPort.u32ChnId = u32VpeChnId;
        if(_u32ChnNum<(_u32VpePortIdInUse+1) && (i==_u32ChnNum-1))//use as ring/IMI
            stSrcChnPort.u32PortId = _u32VpePortIdInUse;
        else
            stSrcChnPort.u32PortId = i;

        stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
        STCHECKRESULT(MI_VENC_GetChnDevid(i, &u32VencDevId));
        stDstChnPort.u32DevId = u32VencDevId;
        stDstChnPort.u32ChnId = i;
        stDstChnPort.u32PortId = u32VencPortId;
        STCHECKRESULT(MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort));
    }

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

    for(u32VencChnId=0; u32VencChnId<_u32ChnNum; u32VencChnId++)
    {
        STCHECKRESULT(MI_VENC_StopRecvPic(u32VencChnId));
        STCHECKRESULT(MI_VENC_DestroyChn(u32VencChnId));
    }

    for(u32VpePortId=0; u32VpePortId<_u32ChnNum; u32VpePortId++)
    {
        if(_u32ChnNum<(_u32VpePortIdInUse+1) && (u32VpePortId==_u32ChnNum-1))//use as ring/IMI
            u32VpePortId = _u32VpePortIdInUse;

        STCHECKRESULT(MI_VPE_DisablePort(u32VpeChnId, u32VpePortId));
        STCHECKRESULT(MI_VPE_StopChannel(u32VpeChnId));
        STCHECKRESULT(MI_VPE_DestroyChannel(u32VpeChnId));
    }

    STCHECKRESULT(MI_VIF_DisableChnPort(u32VifChnId, u32VifPortId));
    STCHECKRESULT(MI_VIF_DisableDev(u32VifDevId));

    STCHECKRESULT(MI_SYS_Exit());
    return MI_SUCCESS;
}

void display_help()
{
    printf("\n");
    printf("Usage: prog [-f frame rate] [-n num of channel][-m mod type]\n\n");
    printf("\t\t-c  : set crop window. eg: -c 1280_736\n");
    printf("\t\t-f  : set frame rate. eg: -f 30\n");
    printf("\t\t-n  : num of channel. eg: -n 2\n");
    printf("\t\t-m  : mod type. eg: -m 3 (hint: 2:h264 3:h265 4:jpeg)\n");
    printf("\t\t-b  : bind type. eg: -b 16 (hint: 1:frame base 4:realtime 16:hw ring)\n");
    printf("\t\t-l  : ring line count. eg: -l 736\n");
    printf("\t\t-p  : which port use as ring mode. eg: -p 0\n");
    printf("\t\t-r  : set resolution of each channel. eg: -n 2 -r 1280_736_352_288\n");
    printf("\t\t-w  : enable write file. eg: -w 1\n");
    printf("\t\t-P  : set vpe output pixel format. eg: -P 0(hint: 0:yuyv422 11:nv12)\n");
    printf("\t\t-h  : displays this help\n");
}

MI_S32 main(int argc, char **argv)
{
    MI_S32 s32Result;
    char *str = NULL;
    MI_S32 width, height;
    int i;
    while((s32Result = getopt(argc, argv, "c:f:n:m:b:l:p:r:R:s:w:P:h")) != -1)
    {
        switch(s32Result)
        {
            case 'c':
            {
                str = strtok(optarg, "_");
                if(NULL != str)
                {
                    _u32CropWidth = atoi(str);
                }
                str = strtok(NULL, "_");
                if(NULL != str)
                {
                    _u32CropHeight = atoi(str);
                }
                printf("_u32CropWidth = %u, _u32CropHeight = %u\n", _u32CropWidth, _u32CropHeight);
            }
            break;
            case 'f':
            {
                str = strtok(optarg, "_");
                _u32FrameRate = atoi(str);
                printf("_u32FrameRate[%u]\n", _u32FrameRate);

            }
            break;
            case 'n':
            {
                str = strtok(optarg, "_");
                _u32ChnNum = atoi(str);
                if(_u32ChnNum > STUB_VENC_CHN_NUM)
                {
                    printf("warning, the max channel num is[%u]\n", STUB_VENC_CHN_NUM);
                    _u32ChnNum = STUB_VENC_CHN_NUM;
                }
                printf("_u32ChnNum[%u]\n", _u32ChnNum);
            }
            break;
            case 'm':
            {
                str = strtok(optarg, "_");
                for(i=0; i<_u32ChnNum && str; i++)
                {
                    _aeModType[i] = atoi(str);
                    str = strtok(NULL, "_");
                    printf("_aeModType[%d]:(%u)\n", i, _aeModType[i]);
                }
            }
            break;
            case 'b':
            {
                str = strtok(optarg, "_");
                for(i=0; i<_u32ChnNum && str; i++)
                {
                    _aeBindType[i] = atoi(str);
                    str = strtok(NULL, "_");
                    printf("_aeBindType[%d]:(%u)\n", i, _aeBindType[i]);
                }
            }
            break;
            case 'l':
            {
                str = strtok(optarg, "_");
                _u32RingLineCnt = atoi(str);
                printf("_u32RingLineCnt[%u]\n", _u32RingLineCnt);
            }
            break;
            case 'p':
            {
                str = strtok(optarg, "_");
                _u32VpePortIdInUse = atoi(str);
                printf("_u32VpePortIdInUse[%u]\n", _u32VpePortIdInUse);
            }
            break;
            case 'r':
            {
                str = strtok(optarg, "_");

                for(i=0; i<_u32ChnNum && str; i++)
                {
                    width = atoi(str);
                    str = strtok(NULL, "_");
                    height = atoi(str);
                    str = strtok(NULL, "_");

                    _stVpeResolution[i].u32OutWidth = width;
                    _stVpeResolution[i].u32OutHeight = height;

                    _stVencResolution[i].u32MaxWidth = width;
                    _stVencResolution[i].u32MaxHeight= height;
                    _stVencResolution[i].u32OutWidth= width;
                    _stVencResolution[i].u32OutHeight= height;

                    printf("chnId[%u] Width = %u, Height = %u\n", i, width, height);
                }
            }
            break;
            case 'R':
            {
                str = strtok(optarg, "_");
                if(NULL != str)
                {
                    _stVifResolution[0].u32OutWidth = atoi(str);
                }
                str = strtok(NULL, "_");
                if(NULL != str)
                {
                    _stVifResolution[0].u32OutHeight = atoi(str);
                }
                printf("SensorWidth = %u, SensorHeight = %u\n", _stVifResolution[0].u32OutWidth, _stVifResolution[0].u32OutHeight);
            }
            break;
            case 's':
            {
                str = strtok(optarg, "_");
                _u32SensorType = atoi(str);
                printf("sensor type %u\n", _u32SensorType);
            }
            break;
            case 'w':
            {
                str = strtok(optarg, "_");
                _bWriteFile = atoi(str);
                printf("_bWriteFile[%u]\n", _bWriteFile);
            }
            break;
            case 'P':
            {
                str = strtok(optarg, "_");
                _eVpePixelFormat = atoi(str);
                printf("_eVpePixelFormat[%u]\n", _eVpePixelFormat);

            }
            break;
            default:
                display_help();
                break;
        }
    }

    STCHECKRESULT(STUB_BaseModuleInit());
    STCHECKRESULT(STUB_BaseModuleDeinit());
    return 0;
}

