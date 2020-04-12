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

#include <time.h>
#include<sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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
#include "isp_cus3a_if.h"
#include "isp_sigma3a_ext.h"

#define RTSP_LISTEN_PORT        554
#define MAIN_STREAM                "main_stream"
#define SUB_STREAM0                "sub_stream0"
#define SUB_STREAM1                "sub_stream1"

#define PATH_PREFIX                "/mnt"
#define DEBUG_ES_FILE            0
#define NONHDR_PATH                "/customer/nonHdr.bin"
#define HDR_PATH                "/customer/hdr.bin"

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
int s32LoadIQBin = 0;
static char szNonHdrIQBinPath[128];
static char szHdrIQBinPath[128];

static struct ST_Stream_Attr_T g_stStreamAttr[] =
{
    {
        .enInput = ST_Sys_Input_VPE,
        .u32InputChn = 0,
        .u32InputPort = 1,
        .vencChn = 0,
        .eType = E_MI_VENC_MODTYPE_H264E,
        .u32Width = 1920,
        .u32Height = 1080,
        .enFunc = ST_Sys_Func_RTSP,
        .pszStreamName = MAIN_STREAM,
    },
};


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

#if DEBUG_ES_FILE
    // whether write frame to file
    int len = 0;
    time_t now = 0;
    struct tm *tm = NULL;

    now = time(NULL);
    tm = localtime(&now);

    len += sprintf(pstStreamInfo->szDebugFile + len, "%s/", PATH_PREFIX);
    len += sprintf(pstStreamInfo->szDebugFile + len, "%s_venc%02d_", szStreamName, pstStreamInfo->vencChn);
    len += sprintf(pstStreamInfo->szDebugFile + len, "%d_", tm->tm_year + 1900);
    len += sprintf(pstStreamInfo->szDebugFile + len, "%02d_", tm->tm_mon);
    len += sprintf(pstStreamInfo->szDebugFile + len, "%02d-", tm->tm_mday);
    len += sprintf(pstStreamInfo->szDebugFile + len, "%02d_", tm->tm_hour);
    len += sprintf(pstStreamInfo->szDebugFile + len, "%02d_", tm->tm_min);
    len += sprintf(pstStreamInfo->szDebugFile + len, "%02d", tm->tm_sec);

    pstStreamInfo->bWriteFile = TRUE;
    pstStreamInfo->fd = 0;
    pstStreamInfo->fd = open(pstStreamInfo->szDebugFile,
                             O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if(pstStreamInfo->fd <= 0)
    {
        ST_WARN("create file %s error\n", pstStreamInfo->szDebugFile);
    }
    else
    {
        ST_DBG("open %s success\n", pstStreamInfo->szDebugFile);
    }

#endif

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

#if DEBUG_ES_FILE

        if((pstStreamInfo->bWriteFile == TRUE) &&
                (pstStreamInfo->fd > 0))
        {
            writeLen = write(pstStreamInfo->fd, stStream.pstPack[0].pu8Addr, stStream.pstPack[0].u32Len);

            if(writeLen != stStream.pstPack[0].u32Len)
            {
                ST_WARN("write file uncompletly\n");
            }
        }

#endif

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

#if DEBUG_ES_FILE

    if((pstStreamInfo->bWriteFile == TRUE) &&
            (pstStreamInfo->fd > 0))
    {
        close(pstStreamInfo->fd);
        pstStreamInfo->fd = 0;

        ST_DBG("close %s success\n", pstStreamInfo->szDebugFile);
    }

#endif
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


MI_S32 ST_BaseModuleInit(MI_SNR_PAD_ID_e eSnrPad, MI_VIF_DEV s32vifDev,MI_S32 HDRtype)
{
    MI_U32 u32ArraySize = ARRAY_SIZE(g_stStreamAttr);
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    MI_U32 i = 0;
    MI_U32 u32CapWidth = 0, u32CapHeight = 0;
    MI_VIF_FrameRate_e eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    MI_SYS_PixelFormat_e ePixFormat;
//    VIF_WORK_MODE_E eVifWorkMode = SAMPLE_VI_MODE_MIPI_1_1080P_REALTIME;
    MI_U32 u32VenBitRate = 0;
    MI_SNR_PADInfo_t  stPad0Info;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
    MI_VIF_HDRType_e eVifHdrType = E_MI_VIF_HDR_TYPE_OFF;
    MI_VPE_HDRType_e eVpeHdrType = E_MI_VPE_HDR_TYPE_OFF;
    MI_SNR_PAD_ID_e eSnrPadId = eSnrPad;
    MI_VIF_DEV vifDev = s32vifDev;
    MI_VIF_CHN vifChn = s32vifDev*4;
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

    g_stStreamAttr[0].u32Width = 1920;
    g_stStreamAttr[0].u32Height = 1080;

    /************************************************
    Step1:  init SYS
    *************************************************/
    STCHECKRESULT(ST_Sys_Init());

    /************************************************
    Step2:  init VIF(for IPC, only one dev)
    *************************************************/
    if(HDRtype > 0)
    {
        //eVifWorkMode = SAMPLE_VI_MODE_MIPI_1_1080P_REALTIME_HDR;
        eVifHdrType = E_MI_VIF_HDR_TYPE_DOL;
        eVpeHdrType = E_MI_VPE_HDR_TYPE_DOL;
        //ePixFormat =(MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(E_MI_SYS_DATA_PRECISION_10BPP, E_MI_SYS_PIXEL_BAYERID_RG);
    }

    MI_VIF_DevAttr_t stDevAttr;
    memset(&stDevAttr, 0x0, sizeof(MI_VIF_DevAttr_t));

    stDevAttr.eIntfMode = stPad0Info.eIntfMode;
    stDevAttr.eWorkMode = E_MI_VIF_WORK_MODE_RGB_REALTIME;
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

    /************************************************
    Step3:  init VPE (create one VPE)
    *************************************************/
    ST_VPE_ChannelInfo_T stVpeChannelInfo;
    MI_VPE_ChannelPara_t stChannelParam;

    memset(&stVpeChannelInfo, 0, sizeof(ST_VPE_ChannelInfo_T));
    stVpeChannelInfo.u16VpeMaxW = u32CapWidth;
    stVpeChannelInfo.u16VpeMaxH = u32CapHeight;
    stVpeChannelInfo.u32X = 0;
    stVpeChannelInfo.u32Y = 0;
    stVpeChannelInfo.u16VpeCropW = 0;
    stVpeChannelInfo.u16VpeCropH = 0;
    stVpeChannelInfo.eRunningMode = E_MI_VPE_RUN_REALTIME_MODE;
    stVpeChannelInfo.eFormat = ePixFormat;//E_MI_SYS_PIXEL_FRAME_RGB_BAYER_12BPP_GR;
    stVpeChannelInfo.eHDRtype = eVpeHdrType;

    memset(&stChannelParam, 0x00, sizeof(MI_VPE_ChannelPara_t));
    stChannelParam.eHDRType = eVpeHdrType;
    stChannelParam.e3DNRLevel = E_MI_VPE_3DNR_LEVEL2;
    printf(" MI_VPE_SetChannelParam 2  VpeChannel = %d, eHDRType= %d, e3DNRLevel = %d\n", 0, stChannelParam.eHDRType, stChannelParam.e3DNRLevel);
    MI_VPE_SetChannelParam(0, &stChannelParam);

    STCHECKRESULT(ST_Vpe_CreateChannel(0, &stVpeChannelInfo));

    STCHECKRESULT(ST_Vpe_StartChannel(0));

    STCHECKRESULT(ST_Vif_StartPort(0, vifChn, 0));

    MI_SNR_GetCurRes(eSnrPadId, &u8ResIndex, &stRes);
       printf("CurRes index %d, CurCrop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, ResDesc %s\n",
           u8ResIndex, stRes.stCropRect.u16X, stRes.stCropRect.u16Y, stRes.stCropRect.u16Width,stRes.stCropRect.u16Height,
           stRes.stOutputSize.u16Width, stRes.stOutputSize.u16Height, stRes.u32MaxFps,stRes.u32MinFps, stRes.strResDesc);

    /************************************************
    Step4:  init VENC
    *************************************************/
    MI_VENC_CHN VencChn = 0;
    MI_VENC_ChnAttr_t stChnAttr;

    for(i = 0; i < u32ArraySize; i ++)
    {
        memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));

        if(pstStreamAttr[i].u32Width * pstStreamAttr[i].u32Height >= 1920 * 1080)
        {
            u32VenBitRate = 1024 * 1024 * 4;
        }
        else
        {
            u32VenBitRate = 1024 * 1024 * 2;
        }

        if(pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_H264E)
        {
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
        }
        else if(pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_H265E)
        {
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = pstStreamAttr[i].u32Width;
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = pstStreamAttr[i].u32Height;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = pstStreamAttr[i].u32Width;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = pstStreamAttr[i].u32Height;
            stChnAttr.stVeAttr.stAttrH265e.bByFrame = TRUE;

            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265CBR;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32BitRate = u32VenBitRate;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateNum = 30;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32FluctuateLevel = 0;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32StatTime = 0;

        }
        else if(pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_JPEGE)
        {
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = pstStreamAttr[i].u32Width;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = pstStreamAttr[i].u32Height;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = pstStreamAttr[i].u32Width;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = pstStreamAttr[i].u32Height;
        }

        stChnAttr.stVeAttr.eType = pstStreamAttr[i].eType;
        VencChn = pstStreamAttr[i].vencChn;

        STCHECKRESULT(ST_Venc_CreateChannel(VencChn, &stChnAttr));
    }

    /************************************************
    Step1:  bind VIF->VPE
    *************************************************/
    ST_Sys_BindInfo_T stBindInfo;
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
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

    return MI_SUCCESS;
}


MI_S32 ST_BaseModuleUnInit(void)
{
    MI_U32 u32ArraySize = ARRAY_SIZE(g_stStreamAttr);
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    MI_U32 i = 0;

    /************************************************
    Step1:  destory VENC
    *************************************************/
    MI_VENC_CHN VencChn = 0;

    for(i = 0; i < u32ArraySize; i ++)
    {
        VencChn = pstStreamAttr[i].vencChn;

        STCHECKRESULT(ST_Venc_DestoryChannel(VencChn));
    }

    /************************************************
    Step2:  destory VPE
    *************************************************/
    STCHECKRESULT(ST_Vpe_StopChannel(0));
    STCHECKRESULT(ST_Vpe_DestroyChannel(0));

    /************************************************
    Step3:  destory VIF
    *************************************************/
    STCHECKRESULT(ST_Vif_StopPort(0, 0));
    STCHECKRESULT(ST_Vif_DisableDev(0));

    /************************************************
    Step4:  destory SYS
    *************************************************/

    STCHECKRESULT(ST_Sys_Exit());

    return MI_SUCCESS;
}

MI_S32 ST_VencStart(void)
{
    MI_U32 u32ArraySize = ARRAY_SIZE(g_stStreamAttr);
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    MI_U32 i = 0;
    ST_Sys_Input_E enInput = ST_Sys_Input_BUTT;
    ST_Sys_BindInfo_T stBindInfo;
    MI_VENC_CHN VencChn = 0;
    //MI_S32 s32Ret = MI_SUCCESS;
    ST_VPE_PortInfo_T stVpePortInfo;
    MI_U32 u32DevId = -1;

    for(i = 0; i < u32ArraySize; i ++)
    {
        enInput = pstStreamAttr[i].enInput;

        if(enInput == ST_Sys_Input_VPE)
        {
            memset(&stVpePortInfo, 0, sizeof(ST_VPE_PortInfo_T));

            stVpePortInfo.DepVpeChannel = pstStreamAttr[i].u32InputChn;
            stVpePortInfo.u16OutputWidth = pstStreamAttr[i].u32Width;
            stVpePortInfo.u16OutputHeight = pstStreamAttr[i].u32Height;
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            stVpePortInfo.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
            STCHECKRESULT(ST_Vpe_StartPort(pstStreamAttr[i].u32InputPort, &stVpePortInfo));

            // vpe port 2 can not attach osd, so use div
            VencChn = pstStreamAttr[i].vencChn;
            ExecFunc(MI_VENC_GetChnDevid(VencChn, &u32DevId), MI_SUCCESS);

            // vpe port 2 can not attach osd, so use divp
            memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = pstStreamAttr[i].u32InputChn;
            stBindInfo.stSrcChnPort.u32PortId = pstStreamAttr[i].u32InputPort;

            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
            stBindInfo.stDstChnPort.u32DevId = u32DevId;
            stBindInfo.stDstChnPort.u32ChnId = VencChn;
            stBindInfo.stDstChnPort.u32PortId = 0;

            stBindInfo.u32SrcFrmrate = 30;
            stBindInfo.u32DstFrmrate = 30;
            stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
            STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

        }
    }

    /************************************************
    Step3:  start VENC
    *************************************************/
    for(i = 0; i < u32ArraySize; i ++)
    {
        VencChn = pstStreamAttr[i].vencChn;

        ExecFunc(MI_VENC_GetChnDevid(VencChn, &u32DevId), MI_SUCCESS);
        STCHECKRESULT(ST_Venc_StartChannel(VencChn));
    }

    return MI_SUCCESS;
}

MI_S32 ST_VencStop(void)
{
    MI_U32 u32ArraySize = ARRAY_SIZE(g_stStreamAttr);
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    MI_U32 i = 0;
    ST_Sys_Input_E enInput = ST_Sys_Input_BUTT;
    ST_Sys_BindInfo_T stBindInfo;
    MI_VENC_CHN VencChn = 0;

    /************************************************
    Step1:  unbind VIF->VPE
    *************************************************/
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

    /************************************************
    Step2:  unbind and stop VPE port
    *************************************************/
    //ST_VPE_PortInfo_T stVpePortInfo;
    MI_U32 u32DevId = -1;

    for(i = 0; i < u32ArraySize; i ++)
    {
        enInput = pstStreamAttr[i].enInput;

        if(enInput == ST_Sys_Input_VPE)
        {


                VencChn = pstStreamAttr[i].vencChn;
                ExecFunc(MI_VENC_GetChnDevid(VencChn, &u32DevId), MI_SUCCESS);

                memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
                stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
                stBindInfo.stSrcChnPort.u32DevId = 0;
                stBindInfo.stSrcChnPort.u32ChnId = pstStreamAttr[i].u32InputChn;
                stBindInfo.stSrcChnPort.u32PortId = pstStreamAttr[i].u32InputPort;

                stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
                stBindInfo.stDstChnPort.u32DevId = u32DevId;
                stBindInfo.stDstChnPort.u32ChnId = VencChn;
                stBindInfo.stDstChnPort.u32PortId = 0;

                stBindInfo.u32SrcFrmrate = 30;
                stBindInfo.u32DstFrmrate = 30;
                STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));


            STCHECKRESULT(ST_Vpe_StopPort(pstStreamAttr[i].u32InputChn, pstStreamAttr[i].u32InputPort));
        }
    }

    /************************************************
    Step3:  stop VENC
    *************************************************/
    for(i = 0; i < u32ArraySize; i ++)
    {
        VencChn = pstStreamAttr[i].vencChn;

        STCHECKRESULT(ST_Venc_StopChannel(VencChn));
    }

    return MI_SUCCESS;
}

MI_S32 ST_WriteOneFrame(int dstFd, int offset, char *pDataFrame, int line_offset, int line_size, int lineNumber)
{
    int size = 0;
    int i = 0;
    char *pData = NULL;
    int yuvSize = line_size;

    // seek to file offset
    //lseek(dstFd, offset, SEEK_SET);
    for(i = 0; i < lineNumber; i++)
    {
        pData = pDataFrame + line_offset * i;
        yuvSize = line_size;

        //printf("write lind %d begin\n", i);
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

            size = write(dstFd, pData, size);

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

        //printf("write lind %d end\n", i);
    }

    return MI_SUCCESS;
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


unsigned int poll_isp(int fd,int timeout)
{
    struct pollfd pfd;
    int ret = 0;
    unsigned int flag = 0;

    pfd.fd = fd;
    pfd.events = POLLIN|POLLRDNORM;

    ret = poll(&pfd,1,timeout);
    if(ret>0)
    {
        read(fd,(void*)&flag,sizeof(flag));
    }
    return flag;
}

static void test_mi_ispapi(void)
{
    FILE *fd;
    MI_ISP_AE_HW_STATISTICS_t *pAe;
    MI_ISP_AWB_HW_STATISTICS_t *pAwb;
    MI_ISP_HISTO_HW_STATISTICS_t *pHisto;
    Cus3AEnable_t *pCus3AEnable;
    CusAEInitParam_t *pAeInitParam;
    CusAEInfo_t *pAeInfo;
    CusAEResult_t *pAeResult;
    CusAWBInfo_t *pAwbInfo;
    CusAWBResult_t *pAwbResult;

    /*AE avg statistics*/
    pAe = (MI_ISP_AE_HW_STATISTICS_t *)malloc(sizeof(MI_ISP_AE_HW_STATISTICS_t));
    MI_ISP_AE_GetAeHwAvgStats(0,pAe);
    fd = fopen("ae.bin","w+b");
    fwrite(pAe->nAvg,sizeof(pAe->nAvg),1,fd);
    fclose(fd);
    printf("Save ae data to file ae.bin\n");
    free(pAe);

    /*AWB avg statistics*/
    pAwb = (MI_ISP_AWB_HW_STATISTICS_t *)malloc(sizeof(MI_ISP_AWB_HW_STATISTICS_t));
    MI_ISP_AWB_GetAwbHwAvgStats(0,pAwb);
    fd = fopen("awb.bin","w+b");
    fwrite(pAwb->nAvg,sizeof(pAwb->nAvg),1,fd);
    fclose(fd);
    printf("Save awb data to file ae.bin\n");
    free(pAwb);

    /*Histo0 avg statistics*/
    pHisto = (MI_ISP_HISTO_HW_STATISTICS_t *)malloc(sizeof(MI_ISP_HISTO_HW_STATISTICS_t));
    MI_ISP_AE_GetHisto0HwStats(0,pHisto);
    fd = fopen("histo0.bin","w+b");
    fwrite(pHisto->nHisto,sizeof(pHisto->nHisto),1,fd);
    fclose(fd);
    printf("Save histo0 data to file ae.bin\n");
    free(pHisto);

    /*Histo1 avg statistics*/
    pHisto = (MI_ISP_HISTO_HW_STATISTICS_t *)malloc(sizeof(MI_ISP_HISTO_HW_STATISTICS_t));
    MI_ISP_AE_GetHisto1HwStats(0,pHisto);
    fd = fopen("histo1.bin","w+b");
    fwrite(pHisto->nHisto,sizeof(pHisto->nHisto),1,fd);
    fclose(fd);
    printf("Save histo1 data to file ae.bin\n");
    free(pHisto);

    /*Check CUS3A*/
    //pCus3AEnable = (Cus3AEnable_t *)malloc(sizeof(Cus3AEnable_t));
    //pCus3AEnable->bAE = 1;
    //pCus3AEnable->bAWB = 1;
    //pCus3AEnable->bAF = 0;
    //MI_ISP_CUS3A_Enable(0,pCus3AEnable);
    //free(pCus3AEnable);

    /*Check AE init param*/
    pAeInitParam = (CusAEInitParam_t *)malloc(sizeof(CusAEInitParam_t));
    memset(pAeInitParam,0,sizeof(CusAEInitParam_t));
    MI_ISP_CUS3A_GetAeInitStatus(0,pAeInitParam);
    printf("AeInitParam ,shutter=%d,shutter_step=%d,sensor_gain_min=%d,sensor_gain_max=%d\n",
            pAeInitParam->shutter,
            pAeInitParam->shutter_step,
            pAeInitParam->sensor_gain_min,
            pAeInitParam->sensor_gain_max
          );
    free(pAeInitParam);

    /*Check AE param*/
    pAeInfo = (CusAEInfo_t *)malloc(sizeof(CusAEInfo_t));
    memset(pAeInfo,0,sizeof(CusAEInfo_t));
    MI_ISP_CUS3A_GetAeStatus(0,pAeInfo);
    printf("AeInitParam ,Shutter=%d,SensirGain=%d,IspGain=%d\n",
            pAeInfo->Shutter,
            pAeInfo->SensorGain,
            pAeInfo->IspGain
          );
    free(pAeInfo);

    /*Check AWB param*/
    pAwbInfo = (CusAWBInfo_t *)malloc(sizeof(CusAWBInfo_t));
    memset(pAwbInfo,0,sizeof(CusAWBInfo_t));
    MI_ISP_CUS3A_GetAwbStatus(0,pAwbInfo);
    printf("AwbInitParam ,Rgain=%d,Ggain=%d,Bgain=%d\n",
            pAwbInfo->CurRGain,
            pAwbInfo->CurGGain,
            pAwbInfo->CurBGain
          );
    free(pAwbInfo);

    /*Set AWB param*/
    pAwbResult = (CusAWBResult_t *)malloc(sizeof(CusAWBResult_t));
    memset(pAwbResult,0,sizeof(CusAWBResult_t));
    pAwbResult->Size = sizeof(CusAWBResult_t);
    pAwbResult->R_gain = 4096;
    pAwbResult->G_gain = 1024;
    pAwbResult->B_gain = 1024;
    MI_ISP_CUS3A_SetAwbParam(0,pAwbResult);
    free(pAwbResult);

#if 1
    int isp_fe = open("/dev/isp_fe",O_RDWR);
    int n =0;
    /*Check AE init param*/
    pAeResult = (CusAEResult_t *)malloc(sizeof(CusAEResult_t));
    for(n=0;n<120;++n)
    {
        unsigned int status = poll_isp(isp_fe,500);
        printf("ISP CH%d FE\n",status);
        memset(pAeResult,0,sizeof(CusAEResult_t));
        pAeResult->Size = sizeof(CusAEResult_t);
        pAeResult->Change = 1;
        pAeResult->u4BVx16384 = 16384;
        pAeResult->HdrRatio = 10;
        pAeResult->ShutterHdrShort = 300;
        pAeResult->Shutter = (300*n) % 30000;
        pAeResult->IspGain = 2048;
        pAeResult->SensorGain = 1024;
        pAeResult->SensorGainHdrShort = 4096;
        pAeResult->IspGainHdrShort = 1024;
        pAeResult->AvgY = 128;
        MI_ISP_CUS3A_SetAeParam(0,pAeResult);
    }
    free(pAeResult);
    close(isp_fe);
#endif
    //pCus3AEnable = (Cus3AEnable_t *)malloc(sizeof(Cus3AEnable_t));
    //pCus3AEnable->bAE = 0;
    //pCus3AEnable->bAWB = 0;
    //pCus3AEnable->bAF = 0;
    //MI_ISP_CUS3A_Enable(0,pCus3AEnable);
    //free(pCus3AEnable);
}

int ae_init(void* pdata, ISP_AE_INIT_PARAM *init_state)
{
    printf("****** ae_init ,shutter=%d,shutter_step=%d,sensor_gain_min=%d,sensor_gain_max=%d *******\n",
            init_state->shutter,
            init_state->shutter_step,
            init_state->sensor_gain,
            init_state->sensor_gain_max
          );
    return 0;
}

void ae_release(void* pdata)
{
    printf("************* ae_release *************\n");
}

void ae_run(void* pdata, const ISP_AE_INFO *info, ISP_AE_RESULT *result)
{
    #define log_info 1

    // Only one can be chosen (the following three define)
    #define shutter_test 0
    #define gain_test 0
    #define AE_sample 1

    static int AE_period = 4;
    static unsigned int fcount = 0;
    unsigned int max = info->AvgBlkY*info->AvgBlkX;
    unsigned int avg=0;
    unsigned int n;
    static int tmp=0;
    static int tmp1=0;

    result->Change              = 0;
    result->u4BVx16384          = 16384;
    result->HdrRatio            = 10;
    result->IspGain             = 1024;
    result->SensorGain          = 4096;
    result->Shutter             = 20000;
    result->IspGainHdrShort     = 1024;
    result->SensorGainHdrShort  = 1024;
    result->ShutterHdrShort     = 1000;
    //result->Size         = sizeof(CusAEResult_t);

    for(n=0;n<max;++n)
    {
        avg += info->avgs[n].y;
    }
    avg /= max;

    result->AvgY         = avg;

#if shutter_test // shutter test under constant sensor gain
    int Shutter_Step = 100; //per frame
    int Shutter_Max = 33333;
    int Shutter_Min = 150;
    int Gain_Constant = 10240;

    result->SensorGain = Gain_Constant;
    result->Shutter = info->Shutter;

    if(++fcount%AE_period == 0)
    {
        if (tmp==0){
            result->Shutter = info->Shutter + Shutter_Step*AE_period;
            //printf("[shutter-up] result->Shutter = %d \n", result->SensorGain);
        }else{
            result->Shutter = info->Shutter - Shutter_Step*AE_period;
            //printf("[shutter-down] result->Shutter = %d \n", result->SensorGain);
        }
        if (result->Shutter >= Shutter_Max){
            result->Shutter = Shutter_Max;
            tmp=1;
        }
        if (result->Shutter <= Shutter_Min){
            result->Shutter = Shutter_Min;
            tmp=0;
        }
    }

    result->Change = 1;

    #if log_info
        printf("fcount = %d, Image avg = 0x%X \n", fcount, avg);
        printf("tmp = %d, Shutter: %d -> %d \n", tmp, info->Shutter, result->Shutter);
    #endif
#endif

#if gain_test // gain test under constant shutter
    int Gain_Step = 1024; //per frame
    int Gain_Max = 1024*100;
    int Gain_Min = 1024*2;
    int Shutter_Constant = 20000;

    result->SensorGain = info->SensorGain;
    result->Shutter = Shutter_Constant;

    if(++fcount%AE_period == 0)
    {
        if (tmp1==0){
            result->SensorGain = info->SensorGain + Gain_Step*AE_period;
            //printf("[gain-up] result->SensorGain = %d \n", result->SensorGain);
        }else{
            result->SensorGain = info->SensorGain - Gain_Step*AE_period;
            //printf("[gain-down] result->SensorGain = %d \n", result->SensorGain);
        }
        if (result->SensorGain >= Gain_Max){
            result->SensorGain = Gain_Max;
            tmp1=1;
        }
        if (result->SensorGain <= Gain_Min) {
            result->SensorGain = Gain_Min;
            tmp1=0;
        }
        result->Change = 1;
    }
    #if log_info
        printf("fcount = %d, Image avg = 0x%X \n", fcount, avg);
        printf("tmp = %d, SensorGain: %d -> %d \n", tmp, info->SensorGain, result->SensorGain);
    #endif
#endif

#if AE_sample
    int y_lower = 0x28;
    int y_upper = 0x38;
    int change_ratio = 10; // percentage
    int Gain_Min = 1024*2;
    int Gain_Max = 1024*1000;
    int Shutter_Min = 150;
    int Shutter_Max = 33333;

    result->SensorGain = info->SensorGain;
    result->Shutter = info->Shutter;

    if(avg<y_lower){
        if (info->Shutter<Shutter_Max){
            result->Shutter = info->Shutter + (info->Shutter*change_ratio/100);
            if (result->Shutter > Shutter_Max) result->Shutter = Shutter_Max;
        }else{
            result->SensorGain = info->SensorGain + (info->SensorGain*change_ratio/100);
            if (result->SensorGain > Gain_Max) result->SensorGain = Gain_Max;
        }
    }else if(avg>y_upper){
        if (info->SensorGain>Gain_Min){
            result->SensorGain = info->SensorGain - (info->SensorGain*change_ratio/100);
            if (result->SensorGain < Gain_Min) result->SensorGain = Gain_Min;
        }else{
            result->Shutter = info->Shutter - (info->Shutter*change_ratio/100);
            if (result->Shutter < Shutter_Min) result->Shutter = Shutter_Min;
        }
        result->Change = 1;
    }
    #if log_info
        printf("fcount = %d, Image avg = 0x%X \n", fcount, avg);
        printf("SensorGain: %d -> %d \n", info->SensorGain, result->SensorGain);
        printf("Shutter: %d -> %d \n", info->Shutter, result->Shutter);
    #endif

#endif

}

int awb_init(void *pdata)
{
    printf("************ awb_init **********\n");
    return 0;
}

void awb_run(void* pdata, const ISP_AWB_INFO *info, ISP_AWB_RESULT *result)
{
    #define log_info 1

    static u32 count = 0;
    int avg_r = 0;
    int avg_g = 0;
    int avg_b = 0;
    int tar_rgain = 1024;
    int tar_bgain = 1024;
    int x = 0;
    int y = 0;

    result->R_gain = info->CurRGain;
    result->G_gain = info->CurGGain;
    result->B_gain = info->CurBGain;
    result->Change = 0;
    result->ColorTmp = 6000;

    if (++count % 4 == 0)
    {
        //center area YR/G/B avg
        for (y = 30; y<60; ++y)
        {
            for (x = 32; x<96; ++x)
            {
                avg_r += info->avgs[info->AvgBlkX*y + x].r;
                avg_g += info->avgs[info->AvgBlkX*y + x].g;
                avg_b += info->avgs[info->AvgBlkX*y + x].b;
            }
        }
        avg_r /= 30 * 64;
        avg_g /= 30 * 64;
        avg_b /= 30 * 64;

        if (avg_r <1)
            avg_r = 1;
        if (avg_g <1)
            avg_g = 1;
        if (avg_b <1)
            avg_b = 1;

#if log_info
        printf("AVG R / G / B = %d, %d, %d \n", avg_r, avg_g, avg_b);
#endif

        // calculate Rgain, Bgain
        tar_rgain = avg_g * 1024 / avg_r;
        tar_bgain = avg_g * 1024 / avg_b;

        if (tar_rgain > info->CurRGain) {
            if (tar_rgain - info->CurRGain < 384)
                result->R_gain = tar_rgain;
            else
                result->R_gain = info->CurRGain + (tar_rgain - info->CurRGain)/10;
        }else{
            if (info->CurRGain - tar_rgain < 384)
                result->R_gain = tar_rgain;
            else
                result->R_gain = info->CurRGain - (info->CurRGain - tar_rgain)/10;
        }

        if (tar_bgain > info->CurBGain) {
            if (tar_bgain - info->CurBGain < 384)
                result->B_gain = tar_bgain;
            else
                result->B_gain = info->CurBGain + (tar_bgain - info->CurBGain)/10;
        }else{
            if (info->CurBGain - tar_bgain < 384)
                result->B_gain = tar_bgain;
            else
                result->B_gain = info->CurBGain - (info->CurBGain - tar_bgain)/10;
        }

        result->Change = 1;
        result->G_gain = 1024;

#if log_info
        printf("[current] r=%d, g=%d, b=%d \n", info->CurRGain, info->CurGGain, info->CurBGain);
        printf("[result] r=%d, g=%d, b=%d \n", result->R_gain, result->G_gain, result->B_gain);
#endif
    }
}

void awb_release(void *pdata)
{
    printf("************ awb_release **********\n");
}
#define MI_VIF_MAX_DEV_NUM 3
int main(int argc, char **argv)
{
    MI_S32 s32EncType = 0; // 0: h264, 1: h265
    //MI_S32 s32LoadIQ = 0;	// 0: not load, else load IQ file
    MI_S32 s32SetIQPath = 0;

    MI_S32 s32HDRtype = 0;
    MI_SNR_PAD_ID_e eSnrPad = E_MI_SNR_PAD_ID_0;
    MI_VIF_DEV vifDev = 0;
    MI_S32 u32PrevSelect = 0;
    if(argc == 1)
    {
        printf("use default sensor pad %d, vif dev %d\n", eSnrPad,vifDev);
    }
    else
    {
        int s32SnrPad=0, s32vifDev=0;
        s32SnrPad = atoi(argv[1]);
        s32vifDev = atoi(argv[2]);
        printf("user set snr pad %d, vif dev %d\n", s32SnrPad, s32vifDev );

        eSnrPad = (s32SnrPad >= E_MI_SNR_PAD_ID_MAX) ? E_MI_SNR_PAD_ID_0 : (MI_SNR_PAD_ID_e)s32SnrPad;
        vifDev = (s32vifDev >= MI_VIF_MAX_DEV_NUM) ? 0 : (MI_VIF_DEV)s32vifDev;
        printf("use user set sensor pad %d, vif dev %d\n", eSnrPad,vifDev);
    }

    s32EncType = 0;
    printf("Use HDR ?\n 0 not use, 1 use DOL\n");
    scanf("%d", &s32HDRtype);
    ST_Flush();
    printf("You select %s HDR\n", s32HDRtype ? "use DOL" : "not use");

    printf("load IQ bin ?\n 0 not load, 1 load\n");
    scanf("%d", &s32LoadIQBin);
    ST_Flush();
    printf("You select %s iq bin\n", s32LoadIQBin ? "load" : "not load");

    if(s32LoadIQBin)
    {
        printf("non hdr path %s\n", NONHDR_PATH);
        printf("hdr path %s\n", HDR_PATH);
        printf("set iq bin path ?\n 0 use default path, 1 set path\n");
        scanf("%d", &s32SetIQPath);
        ST_Flush();

        if(s32SetIQPath)
        {
            printf("please input non hdr path. ex /customer/nonHdr.bin\n");
            scanf("%s", szNonHdrIQBinPath);
            ST_Flush();

            printf("please input hdr path. ex /customer/hdr.bin\n");
            scanf("%s", szHdrIQBinPath);
            ST_Flush();
        }
        else
        {
            strcpy(szNonHdrIQBinPath, NONHDR_PATH);
            strcpy(szHdrIQBinPath, HDR_PATH);
        }

        printf("non hdr iq bin %s\n", szNonHdrIQBinPath);
        printf("hdr iq bin %s\n", szHdrIQBinPath);
    }

    ST_ResetArgs(s32EncType);

    STCHECKRESULT(ST_BaseModuleInit(eSnrPad, vifDev, s32HDRtype));

    STCHECKRESULT(ST_VencStart());

    if(s32LoadIQBin != 0)
    {
        // wait vpe has stream
        usleep(2000 * 1000);
        MI_ISP_API_CmdLoadBinFile(0, (char *)(s32HDRtype ? szHdrIQBinPath : szNonHdrIQBinPath),  1234);
    }

    ST_RtspServerStart();

    printf("hdr type %d\n", s32HDRtype);
    while(!g_bExit)
    {
        MI_U32 u32Select = 0xff;
        printf("select 1: 3ainit\n");
        printf("select 2: 3aexit\n");
        printf("select 3: 3atest\n");
        printf("select 4: exit\n");

        scanf("%d", &u32Select);
        if(u32PrevSelect && u32PrevSelect == u32Select)
        {
            /*Duplicate select*/
        	printf("Duplicate select\n");
            continue;
        }

        ST_Flush();
        if (u32Select == 2)
        {
            CUS3A_RegInterface(0,NULL,NULL,NULL);
            CUS3A_Release();
            u32PrevSelect = u32Select;
            continue;
        }
        else if (u32Select == 3)
        {
            test_mi_ispapi();
            u32PrevSelect = u32Select;
            continue;
        }
        else if (u32Select == 1)
        {
            ISP_AE_INTERFACE tAeIf;
            ISP_AWB_INTERFACE tAwbIf;
            CUS3A_Init();

            /*AE*/
            tAeIf.ctrl = NULL;
            tAeIf.pdata = NULL;
            tAeIf.init = ae_init;
            tAeIf.release = ae_release;
            tAeIf.run = ae_run;

            /*AWB*/
            tAwbIf.ctrl = NULL;
            tAwbIf.pdata = NULL;
            tAwbIf.init = awb_init;
            tAwbIf.release = awb_release;
            tAwbIf.run = awb_run;

            CUS3A_RegInterface(0,&tAeIf,&tAwbIf,NULL);

            /*Register api agent*/
            MI_ISP_RegisterIspApiAgent(0, Cus3A_SetIspApiData, Cus3A_GetIspApiData);
            u32PrevSelect = u32Select;
            continue;
        }
        else if (u32Select == 4)
        {
            break;
        }

        usleep(100 * 1000);
    }

    usleep(100 * 1000);
    ST_RtspServerStop();
    STCHECKRESULT(ST_VencStop());
    STCHECKRESULT(ST_BaseModuleUnInit());

    return 0;
}

