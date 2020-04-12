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
#include "mi_sensor.h"
#include "mi_sensor_datatype.h"
#include "mi_isp.h"
#include "mi_iqserver.h"
#include "mi_mipitx.h"

#define RTSP_LISTEN_PORT        554
static Live555RTSPServer *g_pRTSPServer = NULL;

#define PATH_PREFIX                "/mnt"

int s32LoadIQBin = 1;
#define NONHDR_PATH                "/customer/nohdr.bin"
#define HDR_PATH                "/customer/hdr.bin"

#define ST_MAX_PORT_NUM (4)
#define ST_MAX_SENSOR_NUM (3)

#define ST_MAX_STREAM_NUM (ST_MAX_PORT_NUM * ST_MAX_SENSOR_NUM)

#define ST_LDC_MAX_VIEWNUM (4)

#define ASCII_COLOR_GREEN                        "\033[1;32m"
#define ASCII_COLOR_END                          "\033[0m"
#define ASCII_COLOR_RED                          "\033[1;31m"

#define DBG_INFO(fmt, args...) printf(ASCII_COLOR_GREEN"%s[%d]: " fmt ASCII_COLOR_END, __FUNCTION__,__LINE__, ##args);
#define DBG_ERR(fmt, args...) printf(ASCII_COLOR_RED"%s[%d]: " fmt ASCII_COLOR_END, __FUNCTION__,__LINE__, ##args);

typedef struct ST_Sensor_Attr_s
{
    MI_U32 u32BindVifDev;
    MI_BOOL bUsed;
}ST_Sensor_Attr_t;

typedef struct ST_Vif_Attr_s
{
    MI_U32     u32BindVpeChan;
    MI_SYS_BindType_e       eBindType;

    MI_VIF_WorkMode_e       eWorkMode;
}ST_Vif_Attr_t;

typedef struct ST_VpePortAttr_s
{
    MI_BOOL bUsed;
    MI_U32  u32BindVencChan;
    MI_BOOL bMirror;
    MI_BOOL bFlip;
    MI_SYS_PixelFormat_e ePixelFormat;
    MI_SYS_WindowRect_t  stPortCrop;
    MI_SYS_WindowSize_t  stPortSize;
    MI_SYS_WindowSize_t  stOrigPortSize;
    MI_SYS_WindowRect_t  stOrigPortCrop;
    MI_S32 s32DumpBuffNum;
    char FilePath[256];
    pthread_mutex_t Portmutex;
    pthread_t pGetDatathread;
}ST_VpePortAttr_t;

typedef struct ST_VpeChannelAttr_s
{
    ST_VpePortAttr_t        stVpePortAttr[ST_MAX_PORT_NUM];
    MI_VPE_HDRType_e        eHdrType;
    MI_VPE_3DNR_Level_e     e3DNRLevel;
    MI_SYS_Rotate_e         eVpeRotate;
    MI_BOOL                 bChnMirror;
    MI_BOOL                 bChnFlip;
    MI_SYS_WindowRect_t     stVpeChnCrop;
    MI_SYS_WindowRect_t     stOrgVpeChnCrop;
    MI_BOOL                 bEnLdc;
    MI_U32                  u32ChnPortMode;
    MI_VPE_RunningMode_e    eRunningMode;

    char LdcCfgbin_Path[128];
    MI_U32 u32ViewNum;
    MI_U32                  u32LdcBinSize[ST_LDC_MAX_VIEWNUM];
    MI_S32  s32Rot[ST_LDC_MAX_VIEWNUM];
}ST_VpeChannelAttr_t;

typedef struct ST_VencAttr_s
{
    MI_U32     u32BindVpeChn;
    MI_U32     u32BindVpePort;
    MI_SYS_BindType_e  eBindType;
    MI_U32  u32BindParam;

    MI_VENC_CHN vencChn;
    MI_VENC_ModType_e eType;
    MI_U32    u32Width;
    MI_U32     u32Height;
    char szStreamName[128];
    MI_BOOL bUsed;
    MI_BOOL bStart;
}ST_VencAttr_t;

static MI_S32 gbPreviewByVenc = FALSE;
static ST_Sensor_Attr_t  gstSensorAttr[ST_MAX_SENSOR_NUM];
static ST_Vif_Attr_t     gstVifAttr[ST_MAX_SENSOR_NUM];
static ST_VpeChannelAttr_t gstVpeChnattr[ST_MAX_SENSOR_NUM];
static ST_VencAttr_t gstVencattr[ST_MAX_STREAM_NUM];
static MI_BOOL bExit = FALSE;


void ST_Flush(void)
{
    char c;

    while((c = getchar()) != '\n' && c != EOF);
}

void *ST_OpenStream(char const *szStreamName, void *arg)
{
    MI_U32 i = 0;
    MI_S32 s32Ret = MI_SUCCESS;

    for(i = 0; i < ST_MAX_STREAM_NUM; i ++)
    {
        if(!strncmp(szStreamName, gstVencattr[i].szStreamName,
                    strlen(szStreamName)))
        {
            break;
        }
    }

    if(i >= ST_MAX_STREAM_NUM)
    {
        ST_ERR("not found this stream, \"%s\"", szStreamName);
        return NULL;
    }

    ST_VencAttr_t *pstVencAttr = &gstVencattr[i];

    s32Ret = MI_VENC_RequestIdr(pstVencAttr->vencChn, TRUE);

    ST_DBG("open stream \"%s\" success, chn:%d\n", szStreamName, pstVencAttr->vencChn);

    if(MI_SUCCESS != s32Ret)
    {
        ST_WARN("request IDR fail, error:%x\n", s32Ret);
    }

    return pstVencAttr;
}

MI_U32 u32GetCnt=0, u32ReleaseCnt=0;
int ST_VideoReadStream(void *handle, unsigned char *ucpBuf, int BufLen, struct timeval *p_Timestamp, void *arg)
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_S32 len = 0;
    MI_U32 u32DevId = 0;
    MI_VENC_Stream_t stStream;
    MI_VENC_Pack_t stPack;
    MI_VENC_ChnStat_t stStat;
    MI_VENC_CHN vencChn ;

    if(handle == NULL)
    {
        return -1; 
    }

    ST_VencAttr_t *pstStreamInfo = (ST_VencAttr_t *)handle;

    vencChn = pstStreamInfo->vencChn;

    if(pstStreamInfo->bStart == FALSE)
        return 0;

    s32Ret = MI_VENC_GetChnDevid(vencChn, &u32DevId);

    if(MI_SUCCESS != s32Ret)
    {
        ST_INFO("MI_VENC_GetChnDevid %d error, %X\n", vencChn, s32Ret);
    }

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

    if(MI_SUCCESS == s32Ret)
    {
        u32GetCnt++;
        len = stStream.pstPack[0].u32Len;
        memcpy(ucpBuf, stStream.pstPack[0].pu8Addr, MIN(len, BufLen));

        s32Ret = MI_VENC_ReleaseStream(vencChn, &stStream);
        if(s32Ret != MI_SUCCESS)
        {
            ST_WARN("RELEASE venc buffer fail\n");
        }
        u32ReleaseCnt ++;
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

    ST_VencAttr_t *pstStreamInfo = (ST_VencAttr_t *)handle;

    ST_DBG("close \"%s\" success\n", pstStreamInfo->szStreamName);
    return 0;
}

MI_S32 ST_RtspServerStart(void)
{
    unsigned int rtspServerPortNum = RTSP_LISTEN_PORT;
    int iRet = 0;
    char *urlPrefix = NULL;
    int arraySize = ARRAY_SIZE(gstVencattr);
    ST_VencAttr_t *pstStreamAttr = gstVencattr;
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

    for(i = 0; i < arraySize; i ++)
    {
        if(pstStreamAttr[i].bUsed != TRUE)
            continue;

        printf("=================URL===================\n");
        printf("%s%s\n", urlPrefix, pstStreamAttr[i].szStreamName);
        printf("=================URL===================\n");

        pRTSPServer->createServerMediaSession(mediaSession,
                                              pstStreamAttr[i].szStreamName,
                                              NULL, NULL);

        if(pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_H264E)
        {
            subSession = WW_H264VideoFileServerMediaSubsession::createNew(
                             *(pRTSPServer->GetUsageEnvironmentObj()),
                             pstStreamAttr[i].szStreamName,
                             ST_OpenStream,
                             ST_VideoReadStream,
                             ST_CloseStream, 30);
        }
        else if(pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_H265E)
        {
            subSession = WW_H265VideoFileServerMediaSubsession::createNew(
                             *(pRTSPServer->GetUsageEnvironmentObj()),
                             pstStreamAttr[i].szStreamName,
                             ST_OpenStream,
                             ST_VideoReadStream,
                             ST_CloseStream, 30);
        }
        else if(pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_JPEGE)
        {
            subSession = WW_JPEGVideoFileServerMediaSubsession::createNew(
                             *(pRTSPServer->GetUsageEnvironmentObj()),
                             pstStreamAttr[i].szStreamName,
                             ST_OpenStream,
                             ST_VideoReadStream,
                             ST_CloseStream, 30);
        }

        pRTSPServer->addSubsession(mediaSession, subSession);
        pRTSPServer->addServerMediaSession(mediaSession);
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

MI_S32 ST_WriteOneFrame(FILE *fp, int offset, char *pDataFrame, int line_offset, int line_size, int lineNumber)
{
    int size = 0;
    int i = 0;
    char *pData = NULL;
    int yuvSize = line_size;
    MI_S32 s32Ret = -1;

    for(i = 0; i < lineNumber; i++)
    {
        pData = pDataFrame + line_offset * i;
        yuvSize = line_size;

        do
        {
            if(yuvSize < 256)
            {
                size = yuvSize;
            }
            else
            {
                size = 256;
            }

            size = fwrite(pData, 1, size, fp);

            if(size == 0)
            {
                break;
            }
            else if(size < 0)
            {
                break;
            }

            pData += size;
            yuvSize -= size;
        }
        while(yuvSize > 0);
        s32Ret = MI_SUCCESS;
    }

    return s32Ret;
}

MI_S32 ST_BaseModuleInit(MI_SNR_PAD_ID_e eSnrPad)
{
    MI_U32 u32CapWidth = 0, u32CapHeight = 0;
    MI_VIF_FrameRate_e eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    MI_SYS_PixelFormat_e ePixFormat;
    MI_SNR_PADInfo_t  stPad0Info;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
    MI_SNR_PAD_ID_e eSnrPadId = eSnrPad;
    MI_VIF_DEV vifDev = gstSensorAttr[eSnrPad].u32BindVifDev;
    MI_VIF_CHN vifChn = vifDev*4;
    MI_VPE_CHANNEL vpechn = gstVifAttr[vifDev].u32BindVpeChan;
    ST_VpeChannelAttr_t *pstVpeChnattr = &gstVpeChnattr[vpechn];
    ST_Vif_Attr_t *pstVifDevAttr = &gstVifAttr[vifDev];
    MI_U8 i=0;

    memset(&stPad0Info, 0x0, sizeof(MI_SNR_PADInfo_t));

     if(E_MI_VPE_HDR_TYPE_OFF== pstVpeChnattr->eHdrType 
        || E_MI_VPE_HDR_TYPE_EMBEDDED == pstVpeChnattr->eHdrType
        || E_MI_VPE_HDR_TYPE_LI== pstVpeChnattr->eHdrType)
    {
        MI_SNR_SetPlaneMode(eSnrPad, FALSE);
    }
    else
    {
        MI_SNR_SetPlaneMode(eSnrPad, TRUE);
    }

    MI_U32 u32ResCount =0;
    MI_U8 u8ResIndex =0;
    MI_SNR_Res_t stRes;
    MI_U8 u8ChocieRes =0;
    MI_S32 s32Input =0;
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
        scanf("%d", &s32Input);
        u8ChocieRes = (MI_U8)s32Input;
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
    //stSnrPlane0Info.eBayerId = E_MI_SYS_PIXEL_BAYERID_BG;
    ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);;
    /************************************************
    Step1:  init SYS
    *************************************************/
    STCHECKRESULT(ST_Sys_Init());

    /************************************************
    Step2:  init VIF(for IPC, only one dev)
    *************************************************/
    MI_VIF_DevAttr_t stDevAttr;
    memset(&stDevAttr, 0x0, sizeof(MI_VIF_DevAttr_t));

    stDevAttr.eIntfMode = stPad0Info.eIntfMode;
    stDevAttr.eWorkMode = pstVifDevAttr->eWorkMode;
    stDevAttr.eHDRType = (MI_VIF_HDRType_e)pstVpeChnattr->eHdrType;
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
    stVifPortInfoInfo.ePixFormat = ePixFormat;
    STCHECKRESULT(ST_Vif_CreatePort(vifChn, 0, &stVifPortInfoInfo));
    STCHECKRESULT(ST_Vif_StartPort(0, vifChn, 0));

    /************************************************
    Step3:  init VPE (create one VPE)
    *************************************************/
    MI_VPE_ChannelAttr_t stChannelVpeAttr;
    MI_VPE_ChannelPara_t stChannelVpeParam;
    
    memset(&stChannelVpeAttr, 0, sizeof(MI_VPE_ChannelAttr_t));
    memset(&stChannelVpeParam, 0x00, sizeof(MI_VPE_ChannelPara_t));
    
    stChannelVpeParam.eHDRType = pstVpeChnattr->eHdrType;
    stChannelVpeParam.e3DNRLevel = pstVpeChnattr->e3DNRLevel;
    stChannelVpeParam.bMirror = pstVpeChnattr->bChnMirror;
    stChannelVpeParam.bFlip = pstVpeChnattr->bChnFlip;
    MI_VPE_SetChannelParam(vpechn, &stChannelVpeParam);

    stChannelVpeAttr.u16MaxW = u32CapWidth;
    stChannelVpeAttr.u16MaxH = u32CapHeight;
    stChannelVpeAttr.ePixFmt = ePixFormat;
    stChannelVpeAttr.eRunningMode = pstVpeChnattr->eRunningMode;
    stChannelVpeAttr.eSensorBindId = (MI_VPE_SensorChannel_e)(eSnrPadId+1);
    stChannelVpeAttr.bEnLdc = pstVpeChnattr->bEnLdc;
    stChannelVpeAttr.u32ChnPortMode = pstVpeChnattr->u32ChnPortMode;
    ExecFunc(MI_VPE_CreateChannel(vpechn, &stChannelVpeAttr), MI_VPE_OK);

    STCHECKRESULT(MI_VPE_SetChannelRotation(vpechn, pstVpeChnattr->eVpeRotate));
    STCHECKRESULT(MI_VPE_SetChannelCrop(vpechn, &pstVpeChnattr->stVpeChnCrop));

    STCHECKRESULT(ST_Vpe_StartChannel(vpechn));

    for(i=0; i<ST_MAX_PORT_NUM-1; i++)
    {
        MI_VPE_PortMode_t stVpeMode;
        memset(&stVpeMode, 0, sizeof(stVpeMode));

        if(pstVpeChnattr->stVpePortAttr[i].bUsed == TRUE)
        {
            MI_VPE_SetPortCrop(vpechn, i, &pstVpeChnattr->stVpePortAttr[i].stPortCrop);

            stVpeMode.u16Width = pstVpeChnattr->stVpePortAttr[i].stPortSize.u16Width;
            stVpeMode.u16Height = pstVpeChnattr->stVpePortAttr[i].stPortSize.u16Height;
            stVpeMode.ePixelFormat = pstVpeChnattr->stVpePortAttr[i].ePixelFormat;
            stVpeMode.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
            stVpeMode.bMirror =pstVpeChnattr->stVpePortAttr[i].bMirror;
            stVpeMode.bFlip = pstVpeChnattr->stVpePortAttr[i].bFlip;

            ExecFunc(MI_VPE_SetPortMode(vpechn, i, &stVpeMode), MI_VPE_OK);

            ExecFunc(MI_VPE_EnablePort(vpechn, i), MI_VPE_OK);
        }
    }

    if(pstVpeChnattr->stVpePortAttr[3].bUsed == TRUE)
    {
        MI_VPE_PortMode_t stVpeMode;
        memset(&stVpeMode, 0, sizeof(stVpeMode));

        if(pstVpeChnattr->stVpeChnCrop.u16Width !=0 && pstVpeChnattr->stVpeChnCrop.u16Height !=0)
        {
            pstVpeChnattr->stVpePortAttr[3].stOrigPortSize.u16Width = pstVpeChnattr->stVpeChnCrop.u16Width;
            pstVpeChnattr->stVpePortAttr[3].stOrigPortSize.u16Height = pstVpeChnattr->stVpeChnCrop.u16Height;
        }
        else
        {
            pstVpeChnattr->stVpePortAttr[3].stOrigPortSize.u16Width = u32CapWidth;
            pstVpeChnattr->stVpePortAttr[3].stOrigPortSize.u16Height = u32CapHeight;
        }

        stVpeMode.ePixelFormat = pstVpeChnattr->stVpePortAttr[3].ePixelFormat;

        if(pstVpeChnattr->eVpeRotate == E_MI_SYS_ROTATE_90
            || pstVpeChnattr->eVpeRotate == E_MI_SYS_ROTATE_270)
        {
            pstVpeChnattr->stVpePortAttr[3].stPortSize.u16Width = pstVpeChnattr->stVpePortAttr[3].stOrigPortSize.u16Height;
            pstVpeChnattr->stVpePortAttr[3].stPortSize.u16Height = pstVpeChnattr->stVpePortAttr[3].stOrigPortSize.u16Width;
        }
        else
        {
            pstVpeChnattr->stVpePortAttr[3].stPortSize.u16Width = pstVpeChnattr->stVpePortAttr[3].stOrigPortSize.u16Width;
            pstVpeChnattr->stVpePortAttr[3].stPortSize.u16Height = pstVpeChnattr->stVpePortAttr[3].stOrigPortSize.u16Height;
        }
        
        if(gbPreviewByVenc == TRUE)
        {
            MI_U32 u32VencChn = pstVpeChnattr->stVpePortAttr[3].u32BindVencChan;
            ST_VencAttr_t *pstStreamAttr = &gstVencattr[u32VencChn];

            pstStreamAttr->u32Width = pstVpeChnattr->stVpePortAttr[3].stPortSize.u16Width;
            pstStreamAttr->u32Height= pstVpeChnattr->stVpePortAttr[3].stPortSize.u16Height;
        }

        ExecFunc(MI_VPE_SetPortMode(vpechn, 3, &stVpeMode), MI_VPE_OK);
        ExecFunc(MI_VPE_EnablePort(vpechn, 3), MI_VPE_OK);
    }
    /************************************************
    Step1:  bind VIF->VPE
    *************************************************/
    ST_Sys_BindInfo_T stBindInfo;
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = vifDev;
    stBindInfo.stSrcChnPort.u32ChnId = vifChn;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = vpechn;
    stBindInfo.stDstChnPort.u32PortId = 0;

    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    stBindInfo.eBindType = pstVifDevAttr->eBindType;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

    MI_U32 u32ChnId = 0;
    MI_MipiTx_ChannelAttr_t stMipiChnAttr;
    memset(&stMipiChnAttr, 0x0, sizeof(MI_MipiTx_ChannelAttr_t));

    stMipiChnAttr.eLaneNum = E_MI_MIPITX_LANE_NUM_4;
    stMipiChnAttr.ePixFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    stMipiChnAttr.u32Dclk = 445500*1000;
    stMipiChnAttr.u8DCLKDelay = 0;
    stMipiChnAttr.u32Width = 1920;
    stMipiChnAttr.u32Height = 1080;
    stMipiChnAttr.peChSwapType = NULL;

    MI_MipiTx_CreateChannel(u32ChnId, &stMipiChnAttr);

    MI_MipiTx_TimingConfig_t stTimingCfg;
    memset(&stTimingCfg, 0x0, sizeof(MI_MipiTx_TimingConfig_t));

    stTimingCfg.u8Lpx  = 0x07;
    stTimingCfg.u8ClkHsPrpr = 0x08;
    stTimingCfg.u8ClkZero = 0x20;
    stTimingCfg.u8ClkHsPre = 0x01;
    stTimingCfg.u8ClkHsPost = 0x10;
    stTimingCfg.u8ClkTrail = 0x0a;
    stTimingCfg.u8HsPrpr = 0x01;
    stTimingCfg.u8HsZero = 0x0F;
    stTimingCfg.u8HsTrail = 0X0A;

    MI_MipiTx_SetTimingConfig(u32ChnId, &stTimingCfg);

    MI_MipiTx_StartChannel(u32ChnId);

    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = vpechn;
    stBindInfo.stSrcChnPort.u32PortId = 1;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_MIPITX;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = u32ChnId;
    stBindInfo.stDstChnPort.u32PortId = 0;

    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

    return MI_SUCCESS;
}


MI_S32 ST_BaseModuleUnInit(MI_SNR_PAD_ID_e eSnrPad)
{
    MI_VIF_DEV vifDev = gstSensorAttr[eSnrPad].u32BindVifDev;
    MI_VIF_CHN vifChn = vifDev*4;
    MI_VPE_CHANNEL vpechn = gstVifAttr[vifDev].u32BindVpeChan;

    ST_VpeChannelAttr_t *pstVpeChnattr = &gstVpeChnattr[vpechn];
    MI_U32 i = 0;
    ST_Sys_BindInfo_T stBindInfo;

    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = vpechn;
    stBindInfo.stSrcChnPort.u32PortId = 1;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_MIPITX;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;

    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));


    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = vifDev;
    stBindInfo.stSrcChnPort.u32ChnId = vifChn;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = vpechn;
    stBindInfo.stDstChnPort.u32PortId = 0;

    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

    MI_MipiTx_StopChannel(0);
    MI_MipiTx_DestroyChannel(0);

    for(i = 0; i < ST_MAX_PORT_NUM; i ++)
    {
        if(pstVpeChnattr->stVpePortAttr[i].bUsed == TRUE)
        {
            STCHECKRESULT(ST_Vpe_StopPort(vpechn, i));
        }
    }

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

    MI_SNR_Disable(eSnrPad);
    /************************************************
    Step4:  destory SYS
    *************************************************/

    STCHECKRESULT(ST_Sys_Exit());

    return MI_SUCCESS;
}

MI_S32 ST_VencStart(MI_U32 u32MaxVencWidth, MI_U32 u32MaxVencHeight, MI_U32 u32VpeChn)
{
    MI_U32 u32VenBitRate = 0;
    MI_U32 i = 0;
    ST_Sys_BindInfo_T stBindInfo;
    MI_U32 u32DevId = -1;
    MI_VENC_ChnAttr_t stChnAttr;

    for(i = 0; i < ST_MAX_PORT_NUM; i ++)
    {
        MI_U32 u32VencChn = gstVpeChnattr[u32VpeChn].stVpePortAttr[i].u32BindVencChan;
        ST_VencAttr_t *pstStreamAttr = &gstVencattr[u32VencChn];

        memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));

        if(pstStreamAttr->bUsed != TRUE)
            continue;

        u32VenBitRate = ((pstStreamAttr->u32Width * pstStreamAttr->u32Height + 500000)/1000000)*1024*1024;

        DBG_INFO("chn %d, pichwidth %d, height %d, MaxWidth %d, MaxHeight %d \n", u32VencChn, 
            pstStreamAttr->u32Width, pstStreamAttr->u32Height, u32MaxVencWidth, u32MaxVencHeight);
        if(pstStreamAttr->eType == E_MI_VENC_MODTYPE_H264E)
        {
            stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = pstStreamAttr->u32Width;
            stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = pstStreamAttr->u32Height;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = u32MaxVencWidth;
            stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2;
            stChnAttr.stVeAttr.stAttrH264e.bByFrame = TRUE;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = u32MaxVencHeight;

            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = u32VenBitRate;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = 30;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32StatTime = 0;
        }
        else if(pstStreamAttr->eType == E_MI_VENC_MODTYPE_H265E)
        {
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = pstStreamAttr->u32Width;
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = pstStreamAttr->u32Height;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = u32MaxVencWidth;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = u32MaxVencHeight;
            stChnAttr.stVeAttr.stAttrH265e.bByFrame = TRUE;

            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265CBR;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32BitRate = u32VenBitRate;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateNum = 30;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32FluctuateLevel = 0;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32StatTime = 0;

        }
        else if(pstStreamAttr->eType == E_MI_VENC_MODTYPE_JPEGE)
        {
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = pstStreamAttr->u32Width;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = pstStreamAttr->u32Height;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = u32MaxVencWidth;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = u32MaxVencHeight;
        }

        stChnAttr.stVeAttr.eType = pstStreamAttr->eType;
        u32VencChn = pstStreamAttr->vencChn;

        STCHECKRESULT(ST_Venc_CreateChannel(u32VencChn, &stChnAttr));

        ExecFunc(MI_VENC_GetChnDevid(u32VencChn, &u32DevId), MI_SUCCESS);

        memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = pstStreamAttr->u32BindVpeChn;
        stBindInfo.stSrcChnPort.u32PortId = pstStreamAttr->u32BindVpePort;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
        stBindInfo.stDstChnPort.u32DevId = u32DevId;
        stBindInfo.stDstChnPort.u32ChnId = u32VencChn;
        stBindInfo.stDstChnPort.u32PortId = 0;

        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 30;
        stBindInfo.eBindType = pstStreamAttr->eBindType;
        stBindInfo.u32BindParam = pstStreamAttr->u32BindParam;
        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

        STCHECKRESULT(ST_Venc_StartChannel(u32VencChn));

        pstStreamAttr->bStart = TRUE;
    }

    return MI_SUCCESS;
}

MI_S32 ST_VencStop(MI_U32 u32VpeChn)
{
    MI_U32 i = 0;
    ST_Sys_BindInfo_T stBindInfo;
    MI_VENC_CHN VencChn = 0;
    MI_U32 u32DevId = -1;

    for(i = 0; i < ST_MAX_PORT_NUM; i ++)
    {
        MI_U32 u32VencChn = gstVpeChnattr[u32VpeChn].stVpePortAttr[i].u32BindVencChan;
        ST_VencAttr_t *pstStreamAttr = &gstVencattr[u32VencChn];

        if(pstStreamAttr->bUsed != TRUE)
            continue;

        VencChn = pstStreamAttr->vencChn;
        ExecFunc(MI_VENC_GetChnDevid(VencChn, &u32DevId), MI_SUCCESS);

        memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = pstStreamAttr->u32BindVpeChn;
        stBindInfo.stSrcChnPort.u32PortId = pstStreamAttr->u32BindVpePort;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
        stBindInfo.stDstChnPort.u32DevId = u32DevId;
        stBindInfo.stDstChnPort.u32ChnId = VencChn;
        stBindInfo.stDstChnPort.u32PortId = 0;

        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 30;
        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));


        STCHECKRESULT(ST_Venc_StopChannel(VencChn));
        STCHECKRESULT(ST_Venc_DestoryChannel(VencChn));

        pstStreamAttr->bStart = FALSE;
    }

    return MI_SUCCESS;
}

void ST_SetArgs(MI_U32 u32SensorId)
{
    MI_S32 s32HDRtype = 0;
    MI_S32 s323DNRLevel = 0;
    MI_S32 s32Rotation = 0;
    MI_S32 s32ChannelCropX =0, s32ChannelCropY=0,s32ChannelCropW=0,s32ChannelCropH =0;
    MI_S32 s32bEnLdc =0;
    MI_S32 s32ChnPortMode = 0, s32PortSelect =0;
    MI_U8 i=0;

    MI_U32 u32VifDev = gstSensorAttr[u32SensorId].u32BindVifDev;
    MI_U32 u32VpeChn = gstVifAttr[u32VifDev].u32BindVpeChan;

    DBG_INFO("set vpe channelid %d \n", u32VpeChn);

    ST_VpeChannelAttr_t *pstVpeChnattr = &gstVpeChnattr[u32VpeChn];

    gbPreviewByVenc = TRUE;
    s32HDRtype = 0;
    s323DNRLevel = 2;
    s32Rotation = 0;
    s32ChannelCropX = 0;
    s32ChannelCropY = 0;
    s32ChannelCropW = 0;
    s32ChannelCropH = 0;
    s32bEnLdc = 0;

    pstVpeChnattr->e3DNRLevel = (MI_VPE_3DNR_Level_e)s323DNRLevel;
    pstVpeChnattr->eHdrType = (MI_VPE_HDRType_e)s32HDRtype;
    if(s32Rotation < 4)
        pstVpeChnattr->eVpeRotate = (MI_SYS_Rotate_e)s32Rotation;
    else if(s32Rotation == 4)
    {
        pstVpeChnattr->bChnMirror = TRUE;
        pstVpeChnattr->bChnFlip = FALSE;
    }
    else if(s32Rotation == 5)
    {
        pstVpeChnattr->bChnMirror = FALSE;
        pstVpeChnattr->bChnFlip = TRUE;
    }

    pstVpeChnattr->stOrgVpeChnCrop.u16X = s32ChannelCropX;
    pstVpeChnattr->stOrgVpeChnCrop.u16Y = s32ChannelCropY;
    pstVpeChnattr->stOrgVpeChnCrop.u16Width = s32ChannelCropW;
    pstVpeChnattr->stOrgVpeChnCrop.u16Height = s32ChannelCropH;
    pstVpeChnattr->bEnLdc = s32bEnLdc;
    if(pstVpeChnattr->eVpeRotate == E_MI_SYS_ROTATE_90
        || pstVpeChnattr->eVpeRotate == E_MI_SYS_ROTATE_270)
    {
        pstVpeChnattr->stVpeChnCrop.u16X = pstVpeChnattr->stOrgVpeChnCrop.u16Y;
        pstVpeChnattr->stVpeChnCrop.u16Y = pstVpeChnattr->stOrgVpeChnCrop.u16X;
        pstVpeChnattr->stVpeChnCrop.u16Width = pstVpeChnattr->stOrgVpeChnCrop.u16Height;
        pstVpeChnattr->stVpeChnCrop.u16Height = pstVpeChnattr->stOrgVpeChnCrop.u16Width;
    }
    else
    {
        pstVpeChnattr->stVpeChnCrop.u16X = pstVpeChnattr->stOrgVpeChnCrop.u16X;
        pstVpeChnattr->stVpeChnCrop.u16Y = pstVpeChnattr->stOrgVpeChnCrop.u16Y;
        pstVpeChnattr->stVpeChnCrop.u16Width = pstVpeChnattr->stOrgVpeChnCrop.u16Width;
        pstVpeChnattr->stVpeChnCrop.u16Height = pstVpeChnattr->stOrgVpeChnCrop.u16Height;
    }

    for(i=0; i<ST_MAX_PORT_NUM-1; i++)
    {
        s32PortSelect = 0;

        if(s32PortSelect == 1)
        {
            switch(i)
            {
                case 0:
                    s32ChnPortMode |= E_MI_VPE_ZOOM_LDC_PORT0;
                    break;
                case 1:
                    s32ChnPortMode |= E_MI_VPE_ZOOM_LDC_PORT1;
                    break;
                case 2:
                    s32ChnPortMode |= E_MI_VPE_ZOOM_LDC_PORT2;
                    break;
                case 3:
                    s32ChnPortMode = s32ChnPortMode;
                    break;
                default:
                    s32ChnPortMode = 0;
                    break;
            }
        }
    }

    for(i=0; i<ST_MAX_PORT_NUM; i++)
    {
        MI_S32 s32PortCropX =0, s32PortCropY=0,s32PortCropW=0,s32PortCropH =0;
        MI_S32 s32PortPixelFormat =0;
        MI_S32 s32PortMirror=0, s32PortFlip=0;
        MI_S32 s32PortW=0, s32PortH=0;

        if(i== 0 || i==1)
        {
            ST_VpePortAttr_t  *pstVpePortAttr = &pstVpeChnattr->stVpePortAttr[i];
            if(i < ST_MAX_PORT_NUM-1)
            {
                s32PortW = 1920;
                s32PortH = 1080;
            }

            if(gbPreviewByVenc > 0)
            {
                MI_S32 s32BindType =0;
                MI_S32 eType = 0;
                MI_U32 u32VencChnid = pstVpePortAttr->u32BindVencChan;
                ST_VencAttr_t *pstVencAttr = &gstVencattr[u32VencChnid];
                s32BindType = 0;

                if(s32BindType == 0)
                {
                    pstVencAttr->eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
                    pstVencAttr->u32BindParam = 0;
                }
                else if(s32BindType == 1)
                {
                    pstVencAttr->eBindType = E_MI_SYS_BIND_TYPE_HW_RING;
                    pstVencAttr->u32BindParam = s32PortH;
                }
                else if(s32BindType == 2)
                {
                    pstVencAttr->eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
                    pstVencAttr->u32BindParam = 0;
                }
                else if(s32BindType == 3)
                {
                    pstVencAttr->eBindType = E_MI_SYS_BIND_TYPE_HW_RING;
                    pstVencAttr->u32BindParam = s32PortH/2;
                }

                eType = 2;

                if(eType == 4)
                {
                    pstVencAttr->eType = E_MI_VENC_MODTYPE_JPEGE;
                    if(pstVencAttr->eBindType == E_MI_SYS_BIND_TYPE_REALTIME)
                        s32PortPixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
                    else
                        s32PortPixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
                }
                else
                {
                    pstVencAttr->eType = (MI_VENC_ModType_e)eType;
                    s32PortPixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
                }

                pstVencAttr->vencChn = u32VencChnid;
                pstVencAttr->bUsed = TRUE;
                sprintf(pstVencAttr->szStreamName, "video%d", u32VencChnid);
                
                if(pstVpeChnattr->eVpeRotate == E_MI_SYS_ROTATE_90
                    || pstVpeChnattr->eVpeRotate == E_MI_SYS_ROTATE_270)
                {
                    pstVencAttr->u32Width = s32PortH;
                    pstVencAttr->u32Height = s32PortW;
                }
                else
                {
                    pstVencAttr->u32Width = s32PortW;
                    pstVencAttr->u32Height = s32PortH;
                }
                pstVencAttr->u32BindVpeChn = u32VpeChn;
                pstVencAttr->u32BindVpePort = i;

                if(i ==1)
                {
                    s32PortPixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
                    pstVencAttr->bUsed = FALSE;
                }
            }
            else
            {
                printf("port %d port pixel:", i);
                printf("yuv422:0, argb8888:1, abgr8888:2, bgra8888:3, yuv420:11\n");
                scanf("%d", &s32PortPixelFormat);
                ST_Flush();
            }

            pstVpePortAttr->bMirror = s32PortMirror;
            pstVpePortAttr->bFlip = s32PortFlip;
            pstVpePortAttr->stOrigPortCrop.u16X = s32PortCropX;
            pstVpePortAttr->stOrigPortCrop.u16Y = s32PortCropY;
            pstVpePortAttr->stOrigPortCrop.u16Width = s32PortCropW;
            pstVpePortAttr->stOrigPortCrop.u16Height = s32PortCropH;

            pstVpePortAttr->stOrigPortSize.u16Width = s32PortW;
            pstVpePortAttr->stOrigPortSize.u16Height = s32PortH;

            if(pstVpeChnattr->eVpeRotate == E_MI_SYS_ROTATE_90
                || pstVpeChnattr->eVpeRotate == E_MI_SYS_ROTATE_270)
            {
                pstVpePortAttr->stPortSize.u16Width = pstVpePortAttr->stOrigPortSize.u16Height;
                pstVpePortAttr->stPortSize.u16Height = pstVpePortAttr->stOrigPortSize.u16Width;

                pstVpePortAttr->stPortCrop.u16X = pstVpePortAttr->stOrigPortCrop.u16Y;
                pstVpePortAttr->stPortCrop.u16Y = pstVpePortAttr->stOrigPortCrop.u16X;
                pstVpePortAttr->stPortCrop.u16Width = pstVpePortAttr->stOrigPortCrop.u16Height;
                pstVpePortAttr->stPortCrop.u16Height = pstVpePortAttr->stOrigPortCrop.u16Width;
            }
            else
            {
                pstVpePortAttr->stPortSize.u16Width = pstVpePortAttr->stOrigPortSize.u16Width;
                pstVpePortAttr->stPortSize.u16Height = pstVpePortAttr->stOrigPortSize.u16Height;

                pstVpePortAttr->stPortCrop.u16X = pstVpePortAttr->stOrigPortCrop.u16X;
                pstVpePortAttr->stPortCrop.u16Y = pstVpePortAttr->stOrigPortCrop.u16Y;
                pstVpePortAttr->stPortCrop.u16Width = pstVpePortAttr->stOrigPortCrop.u16Width;
                pstVpePortAttr->stPortCrop.u16Height = pstVpePortAttr->stOrigPortCrop.u16Height;
            }
            pstVpePortAttr->ePixelFormat = (MI_SYS_PixelFormat_e)s32PortPixelFormat;
            pstVpePortAttr->bUsed = TRUE;
        }
    }

    if((s32ChnPortMode & E_MI_VPE_ZOOM_LDC_PORT1)
        || (s32ChnPortMode & E_MI_VPE_ZOOM_LDC_PORT2))
    {
        s32ChnPortMode |= E_MI_VPE_ZOOM_LDC_PORT1 | E_MI_VPE_ZOOM_LDC_PORT2;
    }

    pstVpeChnattr->u32ChnPortMode = s32ChnPortMode;
    printf("chnport mode %d \n", pstVpeChnattr->u32ChnPortMode);
}

int main(int argc, char **argv)
{
    MI_U8  i = 0, j=0;
    MI_SNR_PAD_ID_e eSnrPad = E_MI_SNR_PAD_ID_0;

    gstSensorAttr[0].u32BindVifDev = 0;
    gstSensorAttr[1].u32BindVifDev = 2;

    if(argc > 1 && argc < 5)
    {
        for(i=0; i<argc-1;i++)
        {
            gstSensorAttr[i].bUsed = (MI_BOOL)atoi(argv[i+1]);
        }

        for(i=0; i<ST_MAX_SENSOR_NUM; i++)
        {
            gstVifAttr[i].eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
            gstVifAttr[i].eWorkMode = E_MI_VIF_WORK_MODE_RGB_FRAMEMODE;
            gstVifAttr[i].u32BindVpeChan = i;

            gstVpeChnattr[i].eRunningMode = E_MI_VPE_RUN_CAM_MODE;
            for(j=0; j< ST_MAX_PORT_NUM; j++)
            {
                gstVpeChnattr[i].stVpePortAttr[j].u32BindVencChan = i*ST_MAX_PORT_NUM + j;
            }
        }
    }
    else if(argc == 1)
    {
        gstSensorAttr[0].bUsed = TRUE;

        gstVifAttr[0].eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
        gstVifAttr[0].eWorkMode = E_MI_VIF_WORK_MODE_RGB_REALTIME;
        gstVifAttr[0].u32BindVpeChan = 0;

        gstVpeChnattr[0].eRunningMode = E_MI_VPE_RUN_REALTIME_MODE;
        for(j=0; j< ST_MAX_PORT_NUM; j++)
        {
            gstVpeChnattr[0].stVpePortAttr[j].u32BindVencChan = j;
        }
    }
    else
    {
        printf("realtime ./prog_vpe,  frame mode ./prog_vpe 1 0 0, which sensor use \n");
    }

    for(i=0; i<ST_MAX_SENSOR_NUM; i++)
    {
        if(gstSensorAttr[i].bUsed == TRUE)
            ST_SetArgs(i);
    }

    for(i=0; i<ST_MAX_SENSOR_NUM; i++)
    {
        if(gstSensorAttr[i].bUsed == TRUE)
        {
            eSnrPad = (MI_SNR_PAD_ID_e)i;
            MI_VIF_DEV vifDev = gstSensorAttr[eSnrPad].u32BindVifDev;
            MI_VPE_CHANNEL vpechn = gstVifAttr[vifDev].u32BindVpeChan;

            STCHECKRESULT(ST_BaseModuleInit(eSnrPad));

            if(gbPreviewByVenc == TRUE)
            {
                ST_VpeChannelAttr_t *pstVpeChnattr = &gstVpeChnattr[vpechn];
                MI_U32 u32MaxWidth =0, u32MaxHeight =0;
                if(pstVpeChnattr->eVpeRotate == E_MI_SYS_ROTATE_90
                    || pstVpeChnattr->eVpeRotate == E_MI_SYS_ROTATE_270)
                {
                    u32MaxWidth = 2160;
                    u32MaxHeight = 3840;
                }
                else
                {
                    u32MaxWidth = 3840;
                    u32MaxHeight = 2160;
                }
                
                STCHECKRESULT(ST_VencStart(u32MaxWidth,u32MaxHeight, vpechn));
            }
        }
    }

    ST_RtspServerStart();

#if 0
    pthread_t pIQthread;
    pthread_create(&pIQthread, NULL, ST_IQthread, NULL);
#endif


    for(i=0; i<ST_MAX_SENSOR_NUM; i++)
    {
        if(gstSensorAttr[i].bUsed == TRUE)
        {
            eSnrPad = (MI_SNR_PAD_ID_e)i;

            MI_SNR_PlaneInfo_t stSnrPlane0Info;
            memset(&stSnrPlane0Info, 0x0, sizeof(MI_SNR_PlaneInfo_t));

            MI_SNR_GetPlaneInfo(eSnrPad, 0, &stSnrPlane0Info);

            MI_IQSERVER_Open(stSnrPlane0Info.stCapRect.u16Width, stSnrPlane0Info.stCapRect.u16Height, 0);
        }
    }

    while(!bExit)
    {
        MI_U32 u32Select = 0xff;
        printf("select 11: exit\n");
        scanf("%d", &u32Select);
        ST_Flush();
        if(u32Select == 11)
        {
            bExit = TRUE;
        }

        usleep(100 * 1000);
    }

    usleep(100 * 1000);

    ST_RtspServerStop();

    for(i=0; i<ST_MAX_SENSOR_NUM; i++)
    {
        if(gstSensorAttr[i].bUsed == TRUE)
        {
            eSnrPad = (MI_SNR_PAD_ID_e)i;
            MI_VIF_DEV vifDev = gstSensorAttr[eSnrPad].u32BindVifDev;
            MI_VPE_CHANNEL vpechn = gstVifAttr[vifDev].u32BindVpeChan;

            if(gbPreviewByVenc == TRUE)
            {
                STCHECKRESULT(ST_VencStop(vpechn));
            }

            STCHECKRESULT(ST_BaseModuleUnInit(eSnrPad));
        }
    }

    memset(&gstSensorAttr, 0x0, sizeof(gstSensorAttr));
    memset(&gstVifAttr, 0x0, sizeof(gstVifAttr));
    memset(&gstVpeChnattr, 0x0, sizeof(gstVpeChnattr));
    memset(&gstVencattr, 0x0, sizeof(gstVencattr));

    return 0;
}


