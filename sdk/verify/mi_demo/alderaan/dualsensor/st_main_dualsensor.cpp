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
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/syscall.h>

#include "st_common.h"
#include "st_vif.h"
#include "st_vpe.h"
#include "st_venc.h"

#include "BasicUsageEnvironment.hh"
#include "liveMedia.hh"
#include "Live555RTSPServer.hh"

//#include "mi_rgn.h"
//#include "mi_isp_pretzel.h"
#include "mi_iqserver.h"
#include "mi_sensor.h"
#include "mi_sensor_datatype.h"
#include "mi_divp.h"
//#include "mi_vdisp.h"
#define RTSP_LISTEN_PORT        554
#define MAIN_STREAM                "main_stream"
#define SUB_STREAM0                "sub_stream0"
#define SUB_STREAM1                "sub_stream1"
#define SUB_STREAM2                "sub_stream2"
#define SUB_STREAM3                "sub_stream3"

#define PATH_PREFIX                "/mnt"

#define EnableStream0 (1)
#define EnableStream1 (1)

#define EnGetIrStream (0)

struct ST_Stream_Attr_T
{
    ST_Sys_Input_E enInput;
    MI_U32     u32InputChn;
    MI_U32     u32InputPort;
    MI_VENC_CHN vencChn;
    MI_VENC_ModType_e eType;
    MI_U32    u32Width;
    MI_U32     u32Height;
    MI_U32 enFunc;
    const char    *pszStreamName;
};

typedef struct
{
    MI_VENC_CHN vencChn;
    MI_VENC_ModType_e enType;
    char szStreamName[64];

    MI_BOOL bWriteFile;
    int fd;
    char szDebugFile[128];
} ST_StreamInfo_T;


static Live555RTSPServer *g_pRTSPServer = NULL;
static MI_BOOL g_bExit = FALSE;

MI_BOOL bEnableRes = FALSE;
MI_BOOL bEnableVqe = FALSE;

#if EnGetIrStream==1
static struct ST_Stream_Attr_T g_stStreamAttr[5];
#else
static struct ST_Stream_Attr_T g_stStreamAttr[4];
#endif

void ST_Flush(void)
{
    char c;

    while((c = getchar()) != '\n' && c != EOF);
}

void *ST_OpenStream(char const *szStreamName, void *arg)
{
    ST_StreamInfo_T *pstStreamInfo = NULL;
    MI_U32 u32ArraySize = ARRAY_SIZE(g_stStreamAttr);
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    MI_U32 i = 0;
    MI_S32 s32Ret = MI_SUCCESS;

    pstStreamInfo = (ST_StreamInfo_T *)malloc(sizeof(ST_StreamInfo_T));

    if(pstStreamInfo == NULL)
    {
        ST_ERR("malloc error\n");
        return NULL;
    }

    memset(pstStreamInfo, 0, sizeof(ST_StreamInfo_T));

    for(i = 0; i < u32ArraySize; i ++)
    {
        if(!strncmp(szStreamName, pstStreamAttr[i].pszStreamName,
                    strlen(pstStreamAttr[i].pszStreamName)))
        {
            break;
        }
    }

    if(i >= u32ArraySize)
    {
        ST_ERR("not found this stream, \"%s\"", szStreamName);
        free(pstStreamInfo);
        return NULL;
    }

    pstStreamInfo->vencChn = pstStreamAttr[i].vencChn;
    pstStreamInfo->enType = pstStreamAttr[i].eType;
    snprintf(pstStreamInfo->szStreamName, sizeof(pstStreamInfo->szStreamName) - 1,
             "%s", szStreamName);

    ST_DBG("open stream \"%s\" success, chn:%d\n", szStreamName, pstStreamInfo->vencChn);

    s32Ret = MI_VENC_RequestIdr(pstStreamInfo->vencChn, TRUE);

    if(MI_SUCCESS != s32Ret)
    {
        ST_WARN("request IDR fail, error:%x\n", s32Ret);
    }

    return pstStreamInfo;
}

MI_U32 u32GetCnt=0, u32ReleaseCnt=0;
int ST_VideoReadStream(void *handle, unsigned char *ucpBuf, int BufLen, struct timeval *p_Timestamp, void *arg)
{
    //MI_SYS_ChnPort_t stVencChnInputPort;
    //MI_SYS_BUF_HANDLE hHandle;
    MI_SYS_BufInfo_t stBufInfo;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_S32 len = 0;
    MI_U32 u32DevId = 0;
    MI_VENC_Stream_t stStream;
    MI_VENC_Pack_t stPack;
    MI_VENC_ChnStat_t stStat;
    MI_VENC_CHN vencChn ;
    //int writeLen = 0;

    if(handle == NULL)
    {
        return -1; // disconnect
    }

    ST_StreamInfo_T *pstStreamInfo = (ST_StreamInfo_T *)handle;

    vencChn = pstStreamInfo->vencChn;

    s32Ret = MI_VENC_GetChnDevid(vencChn, &u32DevId);

    if(MI_SUCCESS != s32Ret)
    {
        ST_INFO("MI_VENC_GetChnDevid %d error, %X\n", vencChn, s32Ret);
    }

    /*stVencChnInputPort.eModId = E_MI_MODULE_ID_VENC;
    stVencChnInputPort.u32DevId = u32DevId;
    stVencChnInputPort.u32ChnId = vencChn;
    stVencChnInputPort.u32PortId = 0;*/

//    hHandle = MI_HANDLE_NULL;
    memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
    memset(&stStream, 0, sizeof(stStream));
    memset(&stPack, 0, sizeof(stPack));
    stStream.pstPack = &stPack;
    stStream.u32PackCount = 1;
    s32Ret = MI_VENC_Query(vencChn, &stStat);

    if(s32Ret != MI_SUCCESS || stStat.u32CurPacks == 0)
    {
        return 0;
    }

    s32Ret = MI_VENC_GetStream(vencChn, &stStream, 40);

    // ST_DBG("GET, seq:%d, s32Ret:%x\n", stStream.u32Seq, s32Ret);
    if(MI_SUCCESS == s32Ret)
    {
        u32GetCnt++;
        //len = fwrite(stStream.pstPack[0].pu8Addr, 1,stStream.pstPack[0].u32Len,fd);
        len = stStream.pstPack[0].u32Len;
        //memcpy(ucpBuf, stStream.pstPack[0].pu8Addr, len);
        memcpy(ucpBuf, stStream.pstPack[0].pu8Addr, MIN(len, BufLen));


        s32Ret = MI_VENC_ReleaseStream(vencChn, &stStream);
        if(s32Ret != MI_SUCCESS)
        {
            ST_WARN("RELEASE venc buffer fail\n");
        }
        u32ReleaseCnt ++;

         //ST_WARN("GetCnt %d, release cnt %d\n",u32GetCnt, u32ReleaseCnt);
        // ST_DBG("Release, s32Ret:%x\n", s32Ret);

        return len;
    }

    return 0;
}

int ST_CloseStream(void *handle, void *arg)
{
    if(handle == NULL)
    {
        return -1;
    }

    ST_StreamInfo_T *pstStreamInfo = (ST_StreamInfo_T *)handle;

    ST_DBG("close \"%s\" success\n", pstStreamInfo->szStreamName);

    free(pstStreamInfo);

    return 0;
}

MI_S32 ST_RtspServerStart(void)
{
    unsigned int rtspServerPortNum = RTSP_LISTEN_PORT;
    int iRet = 0;
    char *urlPrefix = NULL;
    int arraySize = ARRAY_SIZE(g_stStreamAttr);
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    int i = 0;
    ServerMediaSession *mediaSession = NULL;
    ServerMediaSubsession *subSession = NULL;
    Live555RTSPServer *pRTSPServer = NULL;

    pRTSPServer = new Live555RTSPServer();

    if(pRTSPServer == NULL)
    {
        ST_ERR("malloc error\n");
        return -1;
    }

#if 0
    pRTSPServer->addUserRecord("admin", "888888");
#endif

    iRet = pRTSPServer->SetRTSPServerPort(rtspServerPortNum);

    while(iRet < 0)
    {
        rtspServerPortNum++;

        if(rtspServerPortNum > 65535)
        {
            ST_INFO("Failed to create RTSP server: %s\n", pRTSPServer->getResultMsg());
            delete pRTSPServer;
            pRTSPServer = NULL;
            return -2;
        }

        iRet = pRTSPServer->SetRTSPServerPort(rtspServerPortNum);
    }

    urlPrefix = pRTSPServer->rtspURLPrefix();
    printf("=================URL===================\n");

    for(i = 0; i < arraySize; i ++)
    {
        if(pstStreamAttr[i].enFunc & ST_Sys_Func_RTSP)
        {
            printf("%s%s\n", urlPrefix, pstStreamAttr[i].pszStreamName);
        }
    }

    printf("=================URL===================\n");

    for(i = 0; i < arraySize; i ++)
    {
        if(pstStreamAttr[i].enFunc & ST_Sys_Func_RTSP)
        {
            pRTSPServer->createServerMediaSession(mediaSession,
                                                  pstStreamAttr[i].pszStreamName,
                                                  NULL, NULL);

            if(pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_H264E)
            {
                subSession = WW_H264VideoFileServerMediaSubsession::createNew(
                                 *(pRTSPServer->GetUsageEnvironmentObj()),
                                 pstStreamAttr[i].pszStreamName,
                                 ST_OpenStream,
                                 ST_VideoReadStream,
                                 ST_CloseStream, 30);
            }
            else if(pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_H265E)
            {
                subSession = WW_H265VideoFileServerMediaSubsession::createNew(
                                 *(pRTSPServer->GetUsageEnvironmentObj()),
                                 pstStreamAttr[i].pszStreamName,
                                 ST_OpenStream,
                                 ST_VideoReadStream,
                                 ST_CloseStream, 30);
            }
            else if(pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_JPEGE)
            {
                subSession = WW_JPEGVideoFileServerMediaSubsession::createNew(
                                 *(pRTSPServer->GetUsageEnvironmentObj()),
                                 pstStreamAttr[i].pszStreamName,
                                 ST_OpenStream,
                                 ST_VideoReadStream,
                                 ST_CloseStream, 30);
            }

            pRTSPServer->addSubsession(mediaSession, subSession);
            pRTSPServer->addServerMediaSession(mediaSession);
        }
    }

    pRTSPServer->Start();

    g_pRTSPServer = pRTSPServer;

    return 0;
}

MI_S32 ST_RtspServerStop(void)
{
    if(g_pRTSPServer)
    {
        g_pRTSPServer->Join();
        delete g_pRTSPServer;
        g_pRTSPServer = NULL;
    }

    return 0;
}

MI_S32 ST_StreamInit(MI_SNR_PAD_ID_e eSnrPad, MI_VIF_DEV s32vifDev,MI_S32 HDRtype)
{
    MI_U32 i = 0;
    MI_U32 u32CapWidth = 0, u32CapHeight = 0;
    MI_VIF_FrameRate_e eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    MI_SYS_PixelFormat_e ePixFormat;
    MI_U32 u32VenBitRate = 0;
    MI_SNR_PADInfo_t  stPad0Info;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
    MI_VIF_HDRType_e eVifHdrType = E_MI_VIF_HDR_TYPE_OFF;
    MI_VPE_HDRType_e eVpeHdrType = E_MI_VPE_HDR_TYPE_OFF;
    MI_SNR_PAD_ID_e eSnrPadId = eSnrPad;
    MI_VIF_DEV vifDev = s32vifDev;
    MI_VIF_CHN vifChn = s32vifDev*4;
    MI_VPE_CHANNEL vpechn = s32vifDev;
    MI_U32 arraySize = ARRAY_SIZE(g_stStreamAttr);
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    memset(&stPad0Info, 0x0, sizeof(MI_SNR_PADInfo_t));

    if(HDRtype > 0)
        MI_SNR_SetPlaneMode(eSnrPadId, TRUE);
    else
        MI_SNR_SetPlaneMode(eSnrPadId, FALSE);

    MI_U32 u32ResCount =0;
    MI_U8 u8ResIndex =0;
    MI_SNR_Res_t stRes;
    MI_U8 u8ChocieRes =0;
    memset(&stRes, 0x0, sizeof(MI_SNR_Res_t));
    
    MI_SNR_QueryResCount(eSnrPadId, &u32ResCount);
    for(u8ResIndex=0; u8ResIndex < u32ResCount; u8ResIndex++)
    {
        MI_SNR_GetRes(eSnrPadId, u8ResIndex, &stRes);
        printf("index %d, Crop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, ResDesc %s\n",
        u8ResIndex, 
        stRes.stCropRect.u16X, stRes.stCropRect.u16Y, stRes.stCropRect.u16Width,stRes.stCropRect.u16Height,
        stRes.stOutputSize.u16Width, stRes.stOutputSize.u16Height, 
        stRes.u32MaxFps,stRes.u32MinFps, 
        stRes.strResDesc);
    }

    printf("choice which resolution use, cnt %d\n", u32ResCount);
    do
    {
        scanf("%d", &u8ChocieRes);
        ST_Flush();
        MI_SNR_QueryResCount(eSnrPadId, &u32ResCount);
        if(u8ChocieRes >= u32ResCount)
        {
            printf("choice err res %d > =cnt %d\n", u8ChocieRes, u32ResCount);
        }
    }while(u8ChocieRes >= u32ResCount);

    printf("You select %d res\n", u8ChocieRes);

    MI_SNR_SetRes(eSnrPadId,u8ChocieRes);
    MI_SNR_Enable(eSnrPadId);

    MI_SNR_GetPadInfo(eSnrPadId, &stPad0Info);
    MI_SNR_GetPlaneInfo(eSnrPadId, 0, &stSnrPlane0Info);

    u32CapWidth = stSnrPlane0Info.stCapRect.u16Width;
    u32CapHeight = stSnrPlane0Info.stCapRect.u16Height;
    eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);;

    /************************************************
    Step1:  init SYS
    *************************************************/
    STCHECKRESULT(ST_Sys_Init());

    /************************************************
    Step2:  init VIF(for IPC, only one dev)
    *************************************************/
    if(HDRtype > 0)
    {
        eVifHdrType = E_MI_VIF_HDR_TYPE_DOL;
        eVpeHdrType = E_MI_VPE_HDR_TYPE_DOL;
    }

    MI_VIF_DevAttr_t stDevAttr;
    memset(&stDevAttr, 0x0, sizeof(MI_VIF_DevAttr_t));

    stDevAttr.eIntfMode = stPad0Info.eIntfMode;
    stDevAttr.eWorkMode = E_MI_VIF_WORK_MODE_RGB_FRAMEMODE;
    stDevAttr.eHDRType = eVifHdrType;
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

    ExecFunc(MI_VIF_SetDevAttr(vifDev, &stDevAttr), MI_SUCCESS);
    ExecFunc(MI_VIF_EnableDev(vifDev), MI_SUCCESS);

    ST_VIF_PortInfo_T stVifPortInfoInfo;
    memset(&stVifPortInfoInfo, 0, sizeof(ST_VIF_PortInfo_T));
    stVifPortInfoInfo.u32RectX = stSnrPlane0Info.stCapRect.u16X;
    stVifPortInfoInfo.u32RectY = stSnrPlane0Info.stCapRect.u16Y;
    stVifPortInfoInfo.u32RectWidth = u32CapWidth;
    stVifPortInfoInfo.u32RectHeight = u32CapHeight;
    stVifPortInfoInfo.u32DestWidth = u32CapWidth;
    stVifPortInfoInfo.u32DestHeight = u32CapHeight;
    stVifPortInfoInfo.eFrameRate = eFrameRate;
    stVifPortInfoInfo.ePixFormat = ePixFormat;//E_MI_SYS_PIXEL_FRAME_RGB_BAYER_12BPP_GR;
    STCHECKRESULT(ST_Vif_CreatePort(vifChn, 0, &stVifPortInfoInfo));
    STCHECKRESULT(ST_Vif_StartPort(0, vifChn, 0));

    MI_ModuleId_e eVifModeId = E_MI_MODULE_ID_VIF;
    MI_U8 u8MmaHeap[128] = "mma_heap_name0";
    MI_SYS_SetChnMMAConf(eVifModeId, 0, vifChn, u8MmaHeap);

    /************************************************
    Step3:  init VPE (create one VPE)
    *************************************************/
    ST_VPE_ChannelInfo_T stVpeChannelInfo;

    memset(&stVpeChannelInfo, 0, sizeof(ST_VPE_ChannelInfo_T));
    stVpeChannelInfo.u16VpeMaxW = u32CapWidth;
    stVpeChannelInfo.u16VpeMaxH = u32CapHeight;
    stVpeChannelInfo.u32X = 0;
    stVpeChannelInfo.u32Y = 0;
    stVpeChannelInfo.u16VpeCropW = 0;
    stVpeChannelInfo.u16VpeCropH = 0;
    stVpeChannelInfo.eRunningMode = E_MI_VPE_RUN_CAM_MODE;
    stVpeChannelInfo.eFormat = ePixFormat;//E_MI_SYS_PIXEL_FRAME_RGB_BAYER_12BPP_GR;
    stVpeChannelInfo.eHDRtype = eVpeHdrType;
    if(eSnrPad == E_MI_SNR_PAD_ID_0)
        stVpeChannelInfo.eBindSensorId = E_MI_VPE_SENSOR0;
    else if(eSnrPad == E_MI_SNR_PAD_ID_1)
        stVpeChannelInfo.eBindSensorId = E_MI_VPE_SENSOR1;

    STCHECKRESULT(ST_Vpe_CreateChannel(vpechn, &stVpeChannelInfo));
    STCHECKRESULT(ST_Vpe_StartChannel(vpechn));

#if EnGetIrStream == 1
    {
        MI_U8 u8MmaHeap[128] = "mma_heap_name0";
        MI_SYS_SetChnMMAConf(E_MI_MODULE_ID_VPE, 0, vpechn, u8MmaHeap);
    }
#endif

    /************************************************
    Step1:  bind VIF->VPE
    *************************************************/
    ST_Sys_BindInfo_T stBindInfo;
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = s32vifDev;
    stBindInfo.stSrcChnPort.u32ChnId = vifChn;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = vpechn;
    stBindInfo.stDstChnPort.u32PortId = 0;

    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

    printf("**************arraysize %d\n", arraySize);
    for(i=0; i< arraySize; i++)
    {
        printf("i %d, stream input chn %d, vpe chn %d \n", i, pstStreamAttr[i].u32InputChn, vpechn);
        if(pstStreamAttr[i].u32InputChn == vpechn)
        {
            ST_VPE_PortInfo_T stVpePortInfo;
            MI_VENC_CHN VencChn = pstStreamAttr[i].vencChn;
            MI_U32 u32VencDevId = 0xff;
            //MI_U16  u16VpePortWidth=1920, u16VpePortheight=1080;
            memset(&stVpePortInfo, 0, sizeof(ST_VPE_PortInfo_T));

            stVpePortInfo.DepVpeChannel = vpechn;
            stVpePortInfo.u16OutputWidth = pstStreamAttr[i].u32Width;
            stVpePortInfo.u16OutputHeight = pstStreamAttr[i].u32Height;
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            stVpePortInfo.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
            STCHECKRESULT(ST_Vpe_StartPort(pstStreamAttr[i].u32InputPort , &stVpePortInfo));

            MI_SNR_GetCurRes(eSnrPadId, &u8ResIndex, &stRes);
               printf("CurRes index %d, CurCrop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, ResDesc %s\n",
                   u8ResIndex, stRes.stCropRect.u16X, stRes.stCropRect.u16Y, stRes.stCropRect.u16Width,stRes.stCropRect.u16Height,
                   stRes.stOutputSize.u16Width, stRes.stOutputSize.u16Height, stRes.u32MaxFps,stRes.u32MinFps, stRes.strResDesc);

            /************************************************
            Step4:  init VENC
            *************************************************/
            MI_VENC_ChnAttr_t stChnAttr;
            memset(&stChnAttr, 0x0, sizeof(MI_VENC_ChnAttr_t));
            u32VenBitRate = 1024 * 1024 * 2;
            stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = pstStreamAttr[i].u32Width;
            stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = pstStreamAttr[i].u32Height;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = pstStreamAttr[i].u32Width;
            stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2;
            stChnAttr.stVeAttr.stAttrH264e.bByFrame = TRUE;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = pstStreamAttr[i].u32Height;
            
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = u32VenBitRate;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = 30;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32StatTime = 0;
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
            printf("creat venc chn %d \n", VencChn);
            STCHECKRESULT(ST_Venc_CreateChannel(VencChn, &stChnAttr));
            STCHECKRESULT(ST_Venc_StartChannel(VencChn));

            ExecFunc(MI_VENC_GetChnDevid(VencChn, &u32VencDevId), MI_SUCCESS);
            // vpe port 2 can not attach osd, so use divp
            memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = vpechn;
            stBindInfo.stSrcChnPort.u32PortId = pstStreamAttr[i].u32InputPort;

            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
            stBindInfo.stDstChnPort.u32DevId = u32VencDevId;
            stBindInfo.stDstChnPort.u32ChnId = VencChn;
            stBindInfo.stDstChnPort.u32PortId = 0;

            stBindInfo.u32SrcFrmrate = 30;
            stBindInfo.u32DstFrmrate = 30;
            stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
            STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
        }
    }
    return MI_SUCCESS;
}


MI_S32 ST_StreamDeInit(MI_SNR_PAD_ID_e eSnrPad, MI_VIF_DEV s32vifDev)
{
    MI_SNR_PAD_ID_e eSnrPadId = eSnrPad;
    MI_VIF_DEV vifDev = s32vifDev;
    MI_VIF_CHN vifChn = s32vifDev*4;
    MI_VENC_CHN VencChn = s32vifDev;
    MI_U32 u32VencDevId = 0xff;
    MI_VPE_CHANNEL vpechn = s32vifDev;
    ST_Sys_BindInfo_T stBindInfo;
    MI_U32 arraySize = ARRAY_SIZE(g_stStreamAttr);
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    MI_U8 i=0;

    for(i=0; i< arraySize; i++)
    {
        printf("i %d, stream input chn %d, vpe chn %d \n", i, pstStreamAttr[i].u32InputChn, vpechn);
        if(pstStreamAttr[i].u32InputChn == vpechn)
        {
            VencChn = pstStreamAttr[i].vencChn;
            ExecFunc(MI_VENC_GetChnDevid(VencChn, &u32VencDevId), MI_SUCCESS);

            memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = vpechn;
            stBindInfo.stSrcChnPort.u32PortId = pstStreamAttr[i].u32InputPort ;

            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
            stBindInfo.stDstChnPort.u32DevId = u32VencDevId;
            stBindInfo.stDstChnPort.u32ChnId = VencChn;
            stBindInfo.stDstChnPort.u32PortId = 0;

            stBindInfo.u32SrcFrmrate = 30;
            stBindInfo.u32DstFrmrate = 30;
            STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

            STCHECKRESULT(ST_Vpe_StopPort(vpechn, pstStreamAttr[i].u32InputPort));

            STCHECKRESULT(ST_Venc_StopChannel(VencChn));
            STCHECKRESULT(ST_Venc_DestoryChannel(VencChn));
        }
    }

    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = s32vifDev;
    stBindInfo.stSrcChnPort.u32ChnId = vifChn;
    stBindInfo.stSrcChnPort.u32PortId = 0 ;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = vpechn;
    stBindInfo.stDstChnPort.u32PortId = 0;

    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

    /************************************************
    Step2:  destory VPE
    *************************************************/
    STCHECKRESULT(ST_Vpe_StopChannel(vpechn));
    STCHECKRESULT(ST_Vpe_DestroyChannel(vpechn));

    /************************************************
    Step3:  destory VIF
    *************************************************/
    STCHECKRESULT(ST_Vif_StopPort(vifChn, 0));
    STCHECKRESULT(ST_Vif_DisableDev(vifDev));

    MI_SNR_Disable(eSnrPadId);

    /************************************************
    Step4:  destory SYS
    *************************************************/

    STCHECKRESULT(ST_Sys_Exit());

    return MI_SUCCESS;
}

MI_BOOL ST_DoChangeHDRtype()
{
    char select = 0xFF;
    ST_Sys_BindInfo_T stBindInfo;
    MI_U32 u32CapWidth = 0, u32CapHeight = 0;
    MI_VIF_FrameRate_e eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    MI_SYS_PixelFormat_e ePixFormat;
    MI_SNR_PADInfo_t  stPad0Info;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
    MI_VIF_HDRType_e eVifHdrType = E_MI_VIF_HDR_TYPE_OFF;
    MI_VPE_HDRType_e eVpeHdrType = E_MI_VPE_HDR_TYPE_OFF;
    MI_SNR_PAD_ID_e eSnrPadId = E_MI_SNR_PAD_ID_0;
    MI_VIF_DEV vifDev = 0;
    MI_VIF_CHN vifChn = 0;
    MI_VPE_ChannelPara_t stVpeChParam;
    MI_U32 u32ResCount =0;
    MI_U8 u8ResIndex =0;
    MI_SNR_Res_t stRes;
    MI_U32 u32Select = 0xff;
    MI_S32 eHDRtype = 0;

    memset(&stRes, 0x0, sizeof(MI_SNR_Res_t));
    memset(&stPad0Info, 0x0, sizeof(MI_SNR_PADInfo_t));
    memset(&stSnrPlane0Info, 0x0, sizeof(MI_SNR_PlaneInfo_t));
    memset(&stVpeChParam, 0x0, sizeof(MI_VPE_ChannelPara_t));

    printf("HDR type?\n 0:OFF, 1:VC, 2:DOL, 3:EMBEDDED, 4:LI\n");
    scanf("%c", &select);
    ST_Flush();

    if(select == 'q')
    {
        return 1;
    }

    eHDRtype = atoi(&select);

    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;

    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

    MI_VPE_StopChannel(0);

    STCHECKRESULT(ST_Vif_StopPort(0, 0));
    STCHECKRESULT(ST_Vif_DisableDev(0));

    MI_SNR_Disable(eSnrPadId);

    if(eHDRtype > 0)
        MI_SNR_SetPlaneMode(eSnrPadId, TRUE);
    else
        MI_SNR_SetPlaneMode(eSnrPadId, FALSE);

    MI_SNR_QueryResCount(eSnrPadId, &u32ResCount);
    for(u8ResIndex=0; u8ResIndex < u32ResCount; u8ResIndex++)
    {
       MI_SNR_GetRes(eSnrPadId, u8ResIndex, &stRes);
       printf("index %d, Crop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, ResDesc %s\n",
       u8ResIndex, 
       stRes.stCropRect.u16X, stRes.stCropRect.u16Y, stRes.stCropRect.u16Width,stRes.stCropRect.u16Height,
       stRes.stOutputSize.u16Width, stRes.stOutputSize.u16Height, 
       stRes.u32MaxFps,stRes.u32MinFps, 
       stRes.strResDesc);
    }

    printf("select res\n");
    scanf("%d", &u32Select);

    MI_SNR_SetRes(eSnrPadId,u32Select);
    MI_SNR_Enable(eSnrPadId);

    MI_SNR_GetPadInfo(eSnrPadId, &stPad0Info);
    MI_SNR_GetPlaneInfo(eSnrPadId, 0, &stSnrPlane0Info);

    u32CapWidth = stSnrPlane0Info.stCapRect.u16Width;
    u32CapHeight = stSnrPlane0Info.stCapRect.u16Height;
    eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);;

    if(eHDRtype > 0)
    {
        eVifHdrType = (MI_VIF_HDRType_e)eHDRtype;
        eVpeHdrType = (MI_VPE_HDRType_e)eHDRtype;
    }

    MI_VIF_DevAttr_t stDevAttr;
    memset(&stDevAttr, 0x0, sizeof(MI_VIF_DevAttr_t));

    stDevAttr.eIntfMode = stPad0Info.eIntfMode;
    stDevAttr.eWorkMode = E_MI_VIF_WORK_MODE_RGB_FRAMEMODE;
    stDevAttr.eHDRType = eVifHdrType;
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

    ExecFunc(MI_VIF_SetDevAttr(vifDev, &stDevAttr), MI_SUCCESS);
    ExecFunc(MI_VIF_EnableDev(vifDev), MI_SUCCESS);

    ST_VIF_PortInfo_T stVifPortInfoInfo;
    memset(&stVifPortInfoInfo, 0, sizeof(ST_VIF_PortInfo_T));
    stVifPortInfoInfo.u32RectX = 0;
    stVifPortInfoInfo.u32RectY = 0;
    stVifPortInfoInfo.u32RectWidth = u32CapWidth;
    stVifPortInfoInfo.u32RectHeight = u32CapHeight;
    stVifPortInfoInfo.u32DestWidth = u32CapWidth;
    stVifPortInfoInfo.u32DestHeight = u32CapHeight;
    stVifPortInfoInfo.eFrameRate = eFrameRate;
    stVifPortInfoInfo.ePixFormat = ePixFormat;//E_MI_SYS_PIXEL_FRAME_RGB_BAYER_12BPP_GR;
    STCHECKRESULT(ST_Vif_CreatePort(vifChn, 0, &stVifPortInfoInfo));

    STCHECKRESULT(ST_Vif_StartPort(0, vifChn, 0));

    MI_VPE_GetChannelParam(0, &stVpeChParam);
    stVpeChParam.eHDRType = eVpeHdrType;
    MI_VPE_SetChannelParam(0, &stVpeChParam);

    MI_VPE_StartChannel(0);
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = vifChn;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;

    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

    return MI_SUCCESS;
}

MI_BOOL ST_DoSetChnZoom()
{
    float r = 16.0/9;
    MI_U16 ystep = 8;
    MI_U16 xstep =ALIGN_UP((MI_U16)(r*ystep), 2);
    int oriW = 0, oriH = 0;
    MI_SYS_WindowRect_t stCropInfo;
    MI_VIF_ChnPortAttr_t stVifPortInfo;
    MI_U32 u32SleepTimeUs = 0;
    MI_U32 u32Fps =0;
    MI_S32 s32ZoomPosition = 0;
    MI_S32 s32PortZoom = 0;
    MI_BOOL bZoomDone = TRUE;
    memset(&stVifPortInfo, 0x0, sizeof(MI_VIF_ChnPortAttr_t));
    memset(&stCropInfo, 0, sizeof(MI_SYS_WindowRect_t));

    MI_VIF_GetChnPortAttr(0,0,&stVifPortInfo);
    oriW = stVifPortInfo.stCapRect.u16Width;
    oriH = stVifPortInfo.stCapRect.u16Height;

    MI_SNR_GetFps(E_MI_SNR_PAD_ID_0, &u32Fps);

    u32SleepTimeUs = 1000000/u32Fps;
    printf("fps %d, sleeptime %d \n", u32Fps, u32SleepTimeUs);

    printf("set zoom position: 1.vif, 2.vpe isp dma, 3.vpe scl pre-crop");
    scanf("%d", &s32ZoomPosition);
    ST_Flush();

    if(s32ZoomPosition == 3)
    {
        printf("select which port zoom: 0:port0, 1:port1, 2:port2, 3: all port \n");
        scanf("%d", &s32PortZoom);
        ST_Flush();
    }

    while(1)
    {
        if(bZoomDone == TRUE)
        {
            stCropInfo.u16X += xstep;
            stCropInfo.u16Y += ystep;
            stCropInfo.u16Width = oriW - (2 * stCropInfo.u16X);
            stCropInfo.u16Height = oriH -(2 * stCropInfo.u16Y);

            stCropInfo.u16Width = ALIGN_UP(stCropInfo.u16Width, 2);
            stCropInfo.u16Height = ALIGN_UP(stCropInfo.u16Height, 2);

            if(stCropInfo.u16Width < 660 || stCropInfo.u16Height < 360)
            {
                bZoomDone = FALSE;
            }
        }
        else
        {
            stCropInfo.u16X -= xstep;
            stCropInfo.u16Y -= ystep;
            stCropInfo.u16Width = oriW - (2 * stCropInfo.u16X);
            stCropInfo.u16Height = oriH -(2 * stCropInfo.u16Y);

            stCropInfo.u16Width = ALIGN_UP(stCropInfo.u16Width, 2);
            stCropInfo.u16Height = ALIGN_UP(stCropInfo.u16Height, 2);

            if(stCropInfo.u16Width > oriW || stCropInfo.u16Height > oriH)
            {
                break;
            }
        }

        if(s32ZoomPosition == 1)
        {
            MI_VIF_ChnPortAttr_t stChnPortAttr;
            ExecFunc(MI_VIF_GetChnPortAttr(0, 0, &stChnPortAttr), MI_SUCCESS);

            memcpy(&stChnPortAttr.stCapRect, &stCropInfo, sizeof(MI_SYS_WindowRect_t));

            stChnPortAttr.stDestSize.u16Width = stCropInfo.u16Width;
            stChnPortAttr.stDestSize.u16Height = stCropInfo.u16Height;

            ExecFunc(MI_VIF_SetChnPortAttr(0, 0, &stChnPortAttr), MI_SUCCESS);
        }
        else if(s32ZoomPosition == 2)
        {
            STCHECKRESULT(MI_VPE_SetChannelCrop(0,  &stCropInfo));
            STCHECKRESULT(MI_VPE_GetChannelCrop(0,  &stCropInfo));
        }
        else if(s32ZoomPosition == 3)
        {
            if(s32PortZoom == 3)
            {
                MI_VPE_SetPortCrop(0, 0, &stCropInfo);
                MI_VPE_SetPortCrop(0, 1, &stCropInfo);
                MI_VPE_SetPortCrop(0, 2, &stCropInfo);
            }
            else
                MI_VPE_SetPortCrop(0, s32PortZoom, &stCropInfo);
        }
        printf("after crop down x:%d y:%d w:%d h:%d\n", stCropInfo.u16X, stCropInfo.u16Y, stCropInfo.u16Width, stCropInfo.u16Height);

        //ST_Flush();

        usleep(u32SleepTimeUs);
    }

    return 0;
}

//#define ALIGN_DOWN(val, alignment) (((val)/(alignment))*(alignment))
//#define ALIGN_UP(val, alignment) ((( (val)+(alignment)-1)/(alignment))*(alignment))

void ST_ResetArgs(MI_S32 s32EncType)
{
    MI_U32 u32ArraySize = ARRAY_SIZE(g_stStreamAttr);
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    MI_U32 i = 0;

    // the last one not reset
    for(i = 0; i<u32ArraySize-1; i ++)
    {
        if(pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_JPEGE)
        {
            continue;
        }

        if(s32EncType == 0)
        {
            pstStreamAttr[i].eType = E_MI_VENC_MODTYPE_H264E;
        }
        else if(s32EncType == 1)
        {
            pstStreamAttr[i].eType = E_MI_VENC_MODTYPE_H265E;
        }
    }
}

void ST_HandleSig(MI_S32 signo)
{
    if(signo == SIGINT)
    {
        ST_INFO("catch Ctrl + C, exit normally\n");

        g_bExit = TRUE;
    }
}

int main(int argc, char **argv)
{
    MI_S32 s32HDRtype = 1;
    MI_SNR_PAD_ID_e eSnrPad = E_MI_SNR_PAD_ID_0;
    MI_VIF_DEV vifDev = 0;
    MI_BOOL bPad0Used=FALSE;
    MI_BOOL bPad1Used=FALSE;
    if(argc < 3)
    {
        printf("./prog_dualsensor  0/1(Pad0 ON/OFF)  0/1(Pad1 ON/OFF)\n");
        return 0;
    }

    bPad0Used = atoi(argv[1]);
    bPad1Used = atoi(argv[2]);
    g_stStreamAttr[0].pszStreamName = MAIN_STREAM;
    g_stStreamAttr[1].pszStreamName = SUB_STREAM0;
    g_stStreamAttr[2].pszStreamName = SUB_STREAM1;
    g_stStreamAttr[3].pszStreamName = SUB_STREAM2;

    g_stStreamAttr[0].enInput =ST_Sys_Input_VPE;
    g_stStreamAttr[0].u32InputChn = 0;
    g_stStreamAttr[0].u32InputPort = 0;
    g_stStreamAttr[0].vencChn = 0;
    g_stStreamAttr[0].eType = E_MI_VENC_MODTYPE_H264E;
    g_stStreamAttr[0].u32Width = 1920;
    g_stStreamAttr[0].u32Height = 1080;
    g_stStreamAttr[0].enFunc = ST_Sys_Func_RTSP;
    
    g_stStreamAttr[1].enInput =ST_Sys_Input_VPE;
    g_stStreamAttr[1].u32InputChn = 0;
    g_stStreamAttr[1].u32InputPort = 1;
    g_stStreamAttr[1].vencChn = 1;
    g_stStreamAttr[1].eType = E_MI_VENC_MODTYPE_H264E;
    g_stStreamAttr[1].u32Width = 1280;
    g_stStreamAttr[1].u32Height = 720;
    g_stStreamAttr[1].enFunc = ST_Sys_Func_RTSP;

    g_stStreamAttr[2].enInput =ST_Sys_Input_VPE;
    g_stStreamAttr[2].u32InputChn = 2;
    g_stStreamAttr[2].u32InputPort = 0;
    g_stStreamAttr[2].vencChn = 2;
    g_stStreamAttr[2].eType = E_MI_VENC_MODTYPE_H264E;
    g_stStreamAttr[2].u32Width = 1280;
    g_stStreamAttr[2].u32Height = 720;
    g_stStreamAttr[2].enFunc = ST_Sys_Func_RTSP;
    
    g_stStreamAttr[3].enInput =ST_Sys_Input_VPE;
    g_stStreamAttr[3].u32InputChn = 2;
    g_stStreamAttr[3].u32InputPort = 1;
    g_stStreamAttr[3].vencChn = 3;
    g_stStreamAttr[3].eType = E_MI_VENC_MODTYPE_H264E;
    g_stStreamAttr[3].u32Width = 1280;
    g_stStreamAttr[3].u32Height = 720;
    g_stStreamAttr[3].enFunc = ST_Sys_Func_RTSP;
#if EnGetIrStream==1
    g_stStreamAttr[4].enInput =ST_Sys_Input_VPE;
    g_stStreamAttr[4].u32InputChn = 0;
    g_stStreamAttr[4].u32InputPort = 4;
    g_stStreamAttr[4].vencChn = 2;
    g_stStreamAttr[4].eType = E_MI_VENC_MODTYPE_H264E;
    g_stStreamAttr[4].u32Width = 1920/2;
    g_stStreamAttr[4].u32Height = 1080/2;
    g_stStreamAttr[4].enFunc = ST_Sys_Func_RTSP;
    g_stStreamAttr[4].pszStreamName = SUB_STREAM3;
#endif
    if(bPad0Used == TRUE)
    {
        eSnrPad = E_MI_SNR_PAD_ID_0; vifDev = 0; s32HDRtype = 0;

        printf("Use HDR type?\n 0:OFF, 1:VC, 2:DOL, 3:EMBEDDED, 4:LI\n");
        scanf("%d", &s32HDRtype);
        ST_Flush();
        printf("You select HDR type %s\n", (s32HDRtype ==0) ?  "not use":
                            (s32HDRtype == 1) ?"use VC":
                            (s32HDRtype == 2) ?"use DOL" :
                            (s32HDRtype == 3) ?"use EMBEDDED" :
                            (s32HDRtype == 4) ?"use LI" : "HDR type err");

        STCHECKRESULT(ST_StreamInit(eSnrPad, vifDev, s32HDRtype));
    }

    if(bPad1Used == TRUE)
    {
        eSnrPad = E_MI_SNR_PAD_ID_1;vifDev = 2; s32HDRtype = 0;
        STCHECKRESULT(ST_StreamInit(eSnrPad, vifDev, s32HDRtype));
    }

    ST_RtspServerStart();

    MI_IQSERVER_Open(1920,1080, 0);
    printf("hdr type %d\n", s32HDRtype);
    while(!g_bExit)
    {
        char select = 0xff;
        printf("select 1: change sensor pad 0 hdr type\n");
        printf("select 2: change zoom\n");
        scanf("%c", &select);
        ST_Flush();
        if(select == 'q')
        {
            break;
        }
        else if(select == '1')
        {
            g_bExit = ST_DoChangeHDRtype();
        }
        else if(select == '2')
        {
            g_bExit = ST_DoSetChnZoom();
        }
        usleep(100 * 1000);
    }

    usleep(100 * 1000);
    ST_RtspServerStop();

    if(bPad0Used == TRUE)
    {
        eSnrPad = E_MI_SNR_PAD_ID_0; vifDev =0;
        STCHECKRESULT(ST_StreamDeInit(eSnrPad, vifDev));
    }

    if(bPad1Used == TRUE)
    {
        eSnrPad = E_MI_SNR_PAD_ID_1; vifDev =2;
        STCHECKRESULT(ST_StreamDeInit(eSnrPad, vifDev));
    }

    memset(&g_stStreamAttr, 0x0, sizeof(ST_Stream_Attr_T)*2);
    return 0;
}

