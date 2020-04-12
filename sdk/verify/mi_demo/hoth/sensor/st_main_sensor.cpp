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
#define RTSP_LISTEN_PORT        554
#define MAIN_STREAM                "main_stream"
#define SUB_STREAM0                "sub_stream0"
#define SUB_STREAM1                "sub_stream1"

#define PATH_PREFIX                "/mnt"
#define DEBUG_ES_FILE            0

#define NONHDR_PATH                "/customer/nonHdr.bin"
#define HDR_PATH                "/customer/hdr.bin"

int s32LoadIQBin = 0;
static char szNonHdrIQBinPath[128];
static char szHdrIQBinPath[128];

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

//used to post process yolov2 result
//static pthread_t post_process_tid;
//static pthread_mutex_t post_process_mut = PTHREAD_MUTEX_INITIALIZER;
//static pthread_cond_t post_process_cond = PTHREAD_COND_INITIALIZER;

/*static void *mluOutputCpuPtr = NULL;
static unsigned int g_out_n = 0;
static unsigned int g_out_c = 0;
static unsigned int g_out_h = 0;
static unsigned int g_out_w = 0;*/
#define tof_width 320
#define tof_height 248

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

pthread_t GetOutDataThread;
MI_BOOL g_Getflag =TRUE;
void *st_GetOutputDataThread(void * args)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    //MI_S32 s32Ret = MI_SUCCESS;
    //MI_S32 s32TimeOutMs = 20;
    MI_S32 s32ReadCnt = 0;
    FILE *fp = NULL;

    MI_VIF_DEV vifDev = 0;
    MI_VIF_CHN vifChn = 4;

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VIF;
    stChnPort.u32DevId = vifDev;
    stChnPort.u32ChnId = vifChn;
    stChnPort.u32PortId = 0;
    //MI_SYS_SetChnOutputPortDepth(&stChnPort, 2, 4);

    fp = fopen("/mnt/dump_vif4_port0.yuv","wb");
    if(fp == NULL)
        printf("file open fail\n");

    while (g_Getflag)
    {
        if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &hHandle))
        {
            int size = stBufInfo.stFrameData.u32BufSize;

            if (s32ReadCnt++ < 10)
            {
                fwrite(stBufInfo.stFrameData.pVirAddr[0], size, 1, fp);
                printf("\t vif(%d) size(%d) get buf cnt (%d)...w(%d)...h(%d)..\n", vifChn, size, s32ReadCnt, stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height);
            }
            MI_SYS_ChnOutputPortPutBuf(hHandle);
        }
        usleep(10*1000);
    }
    fclose(fp);
    printf("\n\n");
    usleep(3000000);

    return NULL;
}

MI_VIF_HDRType_e ST_ConvertVifHdr(MI_S32 HDRtype)
{
    MI_VIF_HDRType_e eVifHdr = E_MI_VIF_HDR_TYPE_OFF;
    switch(HDRtype)
    {
    case 1:
        eVifHdr = E_MI_VIF_HDR_TYPE_DOL;
        break;
    case 2:
        eVifHdr = E_MI_VIF_HDR_TYPE_VC;
        break;
    case 3:
        eVifHdr = E_MI_VIF_HDR_TYPE_EMBEDDED;
        break;
    case 4:
        eVifHdr = E_MI_VIF_HDR_TYPE_LI;
        break;
    default:
        break;
    }
    return eVifHdr;
 }

MI_VPE_HDRType_e ST_ConvertVpeHdr(MI_S32 HDRtype)
{
    MI_VPE_HDRType_e eVpeHdr = E_MI_VPE_HDR_TYPE_OFF;
    switch(HDRtype)
    {
    case 1:
        eVpeHdr = E_MI_VPE_HDR_TYPE_DOL;
        break;
    case 2:
        eVpeHdr = E_MI_VPE_HDR_TYPE_VC;
        break;
    case 3:
        eVpeHdr = E_MI_VPE_HDR_TYPE_EMBEDDED;
        break;
    case 4:
        eVpeHdr = E_MI_VPE_HDR_TYPE_LI;
        break;
    default:
        break;
    }
    return eVpeHdr;
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

    eVifHdrType = ST_ConvertVifHdr(HDRtype);
    eVpeHdrType = ST_ConvertVpeHdr(HDRtype);

     if(E_MI_VIF_HDR_TYPE_OFF==eVifHdrType || E_MI_VIF_HDR_TYPE_EMBEDDED==eVifHdrType
	 || E_MI_VIF_HDR_TYPE_LI==eVifHdrType)
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
    stVpeChannelInfo.e3DNRLevel = E_MI_VPE_3DNR_LEVEL2;

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

MI_BOOL ST_DoGetVifRawData(MI_S32 HDRtype)
{
    MI_SNR_PADInfo_t  stPad0Info;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
    MI_U32 u32CapWidth = 0, u32CapHeight = 0;
    MI_VIF_FrameRate_e eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    MI_SYS_PixelFormat_e ePixFormat;
    MI_VIF_DEV vifDev = 1;
    MI_VIF_CHN vifChn = 4;

    MI_SNR_GetPadInfo(E_MI_SNR_PAD_ID_0, &stPad0Info);
    MI_SNR_GetPlaneInfo(E_MI_SNR_PAD_ID_0, 0, &stSnrPlane0Info);
    u32CapWidth = stSnrPlane0Info.stCapRect.u16Width;
    u32CapHeight = stSnrPlane0Info.stCapRect.u16Height;
    eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);

    /************************************************
    Step2:  init VIF(for IPC, only one dev)
    *************************************************/
    MI_VIF_Dev2SnrPadMuxCfg_t stVifDevMap[4];
    memset(stVifDevMap, 0xff, sizeof(MI_VIF_Dev2SnrPadMuxCfg_t)*4);

    if(HDRtype > 0)
    {
       /* stVifDevMap[0].eSensorPadID = E_MI_VIF_SNRPAD_ID_0;
        stVifDevMap[0].u32PlaneID = 1;
        stVifDevMap[1].eSensorPadID = E_MI_VIF_SNRPAD_ID_0;
        stVifDevMap[1].u32PlaneID = 0;
        stVifDevMap[2].eSensorPadID = E_MI_VIF_SNRPAD_ID_0;
        stVifDevMap[2].u32PlaneID = 1;*/
        printf("HDR ON not support");
        return 0;
    }
    else
    {
        stVifDevMap[0].eSensorPadID = E_MI_VIF_SNRPAD_ID_0;
        stVifDevMap[0].u32PlaneID = 0XFF;
        stVifDevMap[1].eSensorPadID = E_MI_VIF_SNRPAD_ID_0;
        stVifDevMap[1].u32PlaneID = 0XFF;
    }
    printf("devmap %p\n", stVifDevMap);
    MI_VIF_SetDev2SnrPadMux(stVifDevMap, 4);

    MI_VIF_DevAttr_t stDevAttr;
    memset(&stDevAttr, 0x0, sizeof(MI_VIF_DevAttr_t));

    stDevAttr.eIntfMode = stPad0Info.eIntfMode;
    stDevAttr.eWorkMode = E_MI_VIF_WORK_MODE_RGB_FRAMEMODE;
    stDevAttr.eHDRType = E_MI_VIF_HDR_TYPE_OFF;
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
    {
        MI_SYS_ChnPort_t stChnPort;
        MI_SYS_BufInfo_t stBufInfo;
        MI_SYS_BUF_HANDLE hHandle;
        MI_S32 s32WriteCnt = 0;
        FILE *fp = NULL;
        char aName[128];
        struct timeval CurTime;
        MI_U64 u64Time = 0;

        memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
        stChnPort.eModId = E_MI_MODULE_ID_VIF;
        stChnPort.u32DevId = 0;
        stChnPort.u32ChnId = vifChn;
        stChnPort.u32PortId = 0;

        gettimeofday(&CurTime,NULL);
        u64Time = CurTime.tv_sec*1000;

        sprintf(aName, "/mnt/dump_vif4_port0_%dx%d_pts%llu.yuv",u32CapWidth,u32CapHeight, u64Time);

        fp = fopen(aName,"wb");
        if(fp == NULL)
            printf("file open fail\n");

        while (s32WriteCnt < 10)
        {
            if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &hHandle))
            {
                int size = stBufInfo.stFrameData.u32BufSize;
                fwrite(stBufInfo.stFrameData.pVirAddr[0], size, 1, fp);
                s32WriteCnt++;
                printf("\t vif(%d) size(%d) get buf cnt (%d)...w(%d)...h(%d)..\n", vifChn, size, s32WriteCnt, stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height);
                MI_SYS_ChnOutputPortPutBuf(hHandle);
            }
            usleep(10*1000);
        }
        fclose(fp);
    }
    STCHECKRESULT(ST_Vif_StopPort(vifChn, 0));
    STCHECKRESULT(ST_Vif_DisableDev(vifDev));

    return MI_SUCCESS;
}



MI_BOOL ST_DoChangeRes(MI_SNR_PAD_ID_e eSnrPad, MI_VIF_DEV s32vifDev, MI_S32 HDRtype)
{
    char select = 0xFF;
    MI_U8 u8ResIdx =0;
    //MI_U32 u32ResCnt =0;
    MI_SNR_Res_t stRes;

    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
   // MI_SNR_PAD_ID_e        ePADId = eSnrPad;
    ST_Sys_BindInfo_T stBindInfo;
    MI_VENC_CHN VencChn = 0;
    MI_U32 u32DevId = -1;
    MI_SNR_PADInfo_t  stPad0Info;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
    MI_U32 u32CapWidth = 0, u32CapHeight = 0;
    MI_VIF_FrameRate_e eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    MI_SYS_PixelFormat_e ePixFormat;
    MI_U32 u32ResCount =0;
    MI_U8 u8ResIndex =0;
    //MI_U8 u8ChocieRes =0;
    MI_VIF_HDRType_e eVifHdrType = E_MI_VIF_HDR_TYPE_OFF;
    MI_VPE_HDRType_e eVpeHdrType = E_MI_VPE_HDR_TYPE_OFF;
    MI_VIF_DEV vifDev = s32vifDev;
    MI_VIF_CHN vifChn = s32vifDev*4;
    memset(&stRes, 0x0, sizeof(MI_SNR_Res_t));

    MI_SNR_QueryResCount(eSnrPad, &u32ResCount);
    for(u8ResIndex=0; u8ResIndex < u32ResCount; u8ResIndex++)
    {
       MI_SNR_GetRes(eSnrPad, u8ResIndex, &stRes);
       printf("index %d, Crop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, ResDesc %s\n",
       u8ResIndex,
       stRes.stCropRect.u16X, stRes.stCropRect.u16Y, stRes.stCropRect.u16Width,stRes.stCropRect.u16Height,
       stRes.stOutputSize.u16Width, stRes.stOutputSize.u16Height,
       stRes.u32MaxFps,stRes.u32MinFps,
       stRes.strResDesc);
    }

    printf("select res\n");
    scanf("%c", &select);
    ST_Flush();

    if(select == 'q')
    {
        return 1;
    }

    /************************************************
    Step1:  unbind VIF->VPE
    *************************************************/
    ST_Venc_StopChannel(0);
    ST_Vpe_StopPort(0,pstStreamAttr[0].u32InputPort);
    STCHECKRESULT(ST_Vpe_StopChannel(0));
    //ST_RtspServerStop();

    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = vifChn;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;

    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

    int i=0;
    for(i=0; i<1;i++)
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

        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    }


    /************************************************
    Step2:  destory VPE
    *************************************************/
   // ST_Vpe_StopPort(0,1);
   // ST_Vpe_StopPort(0,3);
    STCHECKRESULT(ST_Vpe_DestroyChannel(0));
    /************************************************
    Step3:  destory VIF
    *************************************************/
    STCHECKRESULT(ST_Vif_StopPort(vifChn, 0));
    STCHECKRESULT(ST_Vif_DisableDev(vifDev));

    u8ResIdx = atoi(&select);
    if(HDRtype > 0)
    {
        eVifHdrType = E_MI_VIF_HDR_TYPE_DOL;
        eVpeHdrType = E_MI_VPE_HDR_TYPE_DOL;
        MI_SNR_SetPlaneMode(eSnrPad, TRUE);
    }
    else
    {
        eVifHdrType = E_MI_VIF_HDR_TYPE_OFF;
        eVpeHdrType = E_MI_VPE_HDR_TYPE_OFF;
        MI_SNR_SetPlaneMode(eSnrPad, FALSE);
    }
    MI_SNR_SetRes(eSnrPad,u8ResIdx);
    MI_SNR_Enable(eSnrPad);

    MI_SNR_GetPadInfo(eSnrPad, &stPad0Info);
    MI_SNR_GetPlaneInfo(eSnrPad, 0, &stSnrPlane0Info);
    u32CapWidth = stSnrPlane0Info.stCapRect.u16Width;
    u32CapHeight = stSnrPlane0Info.stCapRect.u16Height;
    eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);

    /************************************************
    Step2:  init VIF(for IPC, only one dev)
    *************************************************/
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
    stVifPortInfoInfo.u32RectX = stSnrPlane0Info.stCapRect.u16X;
    stVifPortInfoInfo.u32RectY = stSnrPlane0Info.stCapRect.u16Y;
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

    memset(&stVpeChannelInfo, 0, sizeof(ST_VPE_ChannelInfo_T));
    stVpeChannelInfo.u16VpeMaxW = u32CapWidth;/******res change, MAX for ISP input need change********/
    stVpeChannelInfo.u16VpeMaxH = u32CapHeight;
    stVpeChannelInfo.u32X = 0;
    stVpeChannelInfo.u32Y = 0;
    stVpeChannelInfo.u16VpeCropW = 0;
    stVpeChannelInfo.u16VpeCropH = 0;
    stVpeChannelInfo.eRunningMode = E_MI_VPE_RUN_REALTIME_MODE;
    stVpeChannelInfo.eFormat = ePixFormat;//E_MI_SYS_PIXEL_FRAME_RGB_BAYER_12BPP_GR;
    stVpeChannelInfo.eHDRtype = eVpeHdrType;

    STCHECKRESULT(ST_Vpe_CreateChannel(0, &stVpeChannelInfo));
    ST_VPE_PortInfo_T stVpePortInfo;

    for(i=0; i<1;i++)
    {
        memset(&stVpePortInfo, 0, sizeof(ST_VPE_PortInfo_T));
        stVpePortInfo.DepVpeChannel = pstStreamAttr[i].u32InputChn;
        stVpePortInfo.u16OutputWidth = pstStreamAttr[i].u32Width;
        stVpePortInfo.u16OutputHeight = pstStreamAttr[i].u32Height;
        stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        stVpePortInfo.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        STCHECKRESULT(ST_Vpe_StartPort(pstStreamAttr[i].u32InputPort, &stVpePortInfo));
    }

    STCHECKRESULT(ST_Vpe_StartChannel(0));

    STCHECKRESULT(ST_Vif_StartPort(0, vifChn, 0));
    /************************************************
    Step1:  bind VIF->VPE
    *************************************************/
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

    for(i=0; i<1;i++)
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
        stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    }
    ST_Venc_StartChannel(0);
    //ST_RtspServerStart();

    return 0;
}

MI_BOOL ST_DoChangeHDRtype(MI_S32 *HDRtype)
{
    MI_S32 select = 0;
    MI_U8 u8ResIdx =0;
    //MI_U32 u32ResCnt =0;
    MI_SNR_Res_t stRes;
    MI_SNR_PAD_ID_e eSnrPad = E_MI_SNR_PAD_ID_0;

   // MI_SNR_PAD_ID_e        ePADId = eSnrPad;
    ST_Sys_BindInfo_T stBindInfo;
    MI_SNR_PADInfo_t  stPad0Info;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
    MI_U32 u32CapWidth = 0, u32CapHeight = 0;
    MI_VIF_FrameRate_e eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    MI_SYS_PixelFormat_e ePixFormat;
    //MI_U8 u8ChocieRes =0;
    MI_VIF_HDRType_e eVifHdrType = E_MI_VIF_HDR_TYPE_OFF;
    MI_VPE_HDRType_e eVpeHdrType = E_MI_VPE_HDR_TYPE_OFF;
    MI_VIF_DEV vifDev = 0;
    MI_VIF_CHN vifChn = 0;
    MI_VPE_ChannelPara_t stVpeChParam;
    memset(&stVpeChParam, 0x0, sizeof(MI_VPE_ChannelPara_t));
    memset(&stRes, 0x0, sizeof(MI_SNR_Res_t));

    printf("Use HDR ?\n 0 not use, 1 use DOL, 2 use VC, 3 use EMBEDDED, 4 use LI\n");
    printf("sony sensor(ex imx307) use DOL, sc sensor(ex sc4238) use VC\n");
    scanf("%d", &select);
    ST_Flush();
    printf("You select %d HDR\n", select);

    *HDRtype = select;

    /************************************************
    Step1:  unbind VIF->VPE
    *************************************************/
    STCHECKRESULT(ST_Vpe_StopChannel(0));

    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = vifChn;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;

    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

    /************************************************
    Step3:  destory VIF
    *************************************************/
    STCHECKRESULT(ST_Vif_StopPort(vifChn, 0));
    STCHECKRESULT(ST_Vif_DisableDev(vifDev));
    MI_SNR_Disable(eSnrPad);

    eVifHdrType = ST_ConvertVifHdr(*HDRtype);
    eVpeHdrType = ST_ConvertVpeHdr(*HDRtype);

    if(E_MI_VIF_HDR_TYPE_OFF==eVifHdrType || E_MI_VIF_HDR_TYPE_EMBEDDED==eVifHdrType
	 || E_MI_VIF_HDR_TYPE_LI==eVifHdrType)
    {
        MI_SNR_SetPlaneMode(eSnrPad, FALSE);
    }
    else
    {
        MI_SNR_SetPlaneMode(eSnrPad, TRUE);
    }
	
    MI_SNR_SetRes(eSnrPad,u8ResIdx);
    MI_SNR_Enable(eSnrPad);

    MI_SNR_GetPadInfo(eSnrPad, &stPad0Info);
    MI_SNR_GetPlaneInfo(eSnrPad, 0, &stSnrPlane0Info);
    u32CapWidth = stSnrPlane0Info.stCapRect.u16Width;
    u32CapHeight = stSnrPlane0Info.stCapRect.u16Height;
    eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);

    /************************************************
    Step2:  init VIF(for IPC, only one dev)
    *************************************************/
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
    stVifPortInfoInfo.u32RectX = stSnrPlane0Info.stCapRect.u16X;
    stVifPortInfoInfo.u32RectY = stSnrPlane0Info.stCapRect.u16Y;
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
    MI_VPE_GetChannelParam(0, &stVpeChParam);
    stVpeChParam.eHDRType = eVpeHdrType;
    //if(E_MI_VPE_3DNR_LEVEL_OFF == stVpeChParam.e3DNRLevel)
    //{
    //    stVpeChParam.e3DNRLevel = E_MI_VPE_3DNR_LEVEL2;
    //}
    //else
    //{
    //    stVpeChParam.e3DNRLevel = E_MI_VPE_3DNR_LEVEL_OFF;
    //}
    MI_VPE_SetChannelParam(0, &stVpeChParam);

    STCHECKRESULT(ST_Vpe_StartChannel(0));

    STCHECKRESULT(ST_Vif_StartPort(0, vifChn, 0));
    /************************************************
    Step1:  bind VIF->VPE
    *************************************************/
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

    if(s32LoadIQBin != 0)
    {
        // wait vpe has stream
        usleep(2000 * 1000);
        MI_ISP_API_CmdLoadBinFile(0, (char *)(*HDRtype ? szHdrIQBinPath : szNonHdrIQBinPath),  1234);
    }

    return 0;

}
#define MI_VIF_MAX_DEV_NUM 2
int main(int argc, char **argv)
{
    MI_S32 s32EncType = 0; // 0: h264, 1: h265
    //MI_S32 s32LoadIQ = 0;	// 0: not load, else load IQ file
    MI_S32 s32SetIQPath = 0;

    MI_S32 s32HDRtype = 0;
    MI_SNR_PAD_ID_e eSnrPad = E_MI_SNR_PAD_ID_0;
    MI_VIF_DEV vifDev = 0;

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
    printf("Use HDR ?\n 0 not use, 1 use DOL, 2 use VC, 3 use EMBEDDED, 4 use LI\n");
    printf("sony sensor(ex imx307) use DOL, sc sensor(ex sc4238) use VC\n");
    scanf("%d", &s32HDRtype);
    ST_Flush();
    printf("You select %d HDR\n", s32HDRtype);

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
        //printf("select 0:change res \n");
        //printf("select 1: get vif rawdata\n");
	 printf("select 2: change hdrtype\n");
        printf("select 11: exit\n");
        scanf("%d", &u32Select);
        ST_Flush();
        //if(u32Select == 0)
        //    g_bExit = ST_DoChangeRes(eSnrPad, vifDev, s32HDRtype);
        //else if(u32Select == 1)
        //{
        //    g_bExit = ST_DoGetVifRawData(s32HDRtype);
        //}
        //else if(u32Select == 2)
        if(u32Select == 2)
        {
            g_bExit = ST_DoChangeHDRtype(&s32HDRtype);
        }
        else if(u32Select == 11)
        {
            g_bExit = TRUE;
        }

        usleep(100 * 1000);
    }

    usleep(100 * 1000);
    g_Getflag =FALSE;
    ST_RtspServerStop();
    STCHECKRESULT(ST_VencStop());
    STCHECKRESULT(ST_BaseModuleUnInit());

    return 0;
}

