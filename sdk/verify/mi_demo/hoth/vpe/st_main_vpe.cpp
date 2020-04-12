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
#include "mi_eptz.h"
#include "mi_ldc.h"
#define RTSP_LISTEN_PORT        554
static Live555RTSPServer *g_pRTSPServer = NULL;

#define PATH_PREFIX                "/mnt"

int s32LoadIQBin = 1;
#define NONHDR_PATH                "/customer/nohdr.bin"
#define HDR_PATH                "/customer/hdr.bin"

#define ST_MAX_STREAM_NUM (4)
typedef struct ST_VpePortAttr_s
{
    MI_BOOL bUsed;
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
    ST_VpePortAttr_t        stVpePortAttr[ST_MAX_STREAM_NUM];
    MI_VPE_HDRType_e        eHdrType;
    MI_VPE_3DNR_Level_e     e3DNRLevel;
    MI_SYS_Rotate_e         eVpeRotate;
    MI_BOOL                 bChnMirror;
    MI_BOOL                 bChnFlip;
    MI_SYS_WindowRect_t     stVpeChnCrop;
    MI_SYS_WindowRect_t     stOrgVpeChnCrop;
    MI_BOOL                 bEnLdc;
    MI_U32                  u32ChnPortMode;
    char LdcCfgbin_Path[128];
    LDC_BIN_HANDLE          ldcBinBuffer;
    MI_U32                  u32LdcBinSize;
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
}ST_VencAttr_t;

static MI_S32 gbPreviewByVenc = FALSE;
static ST_VpeChannelAttr_t gstVpeChnattr;
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

MI_S32 ST_GetVpeOutputData()
{
    MI_S32  s32Portid = 0;
    MI_S32  s32DumpBuffNum =0;
    char sFilePath[128];
    time_t stTime = 0;
    
    ST_VpePortAttr_t *pstVpePortAttr = NULL;
    MI_VPE_PortMode_t stVpePortMode;
    memset(&stVpePortMode, 0x0, sizeof(MI_VPE_PortMode_t));
    memset(&stTime, 0, sizeof(stTime));

    printf("select port id:");
    scanf("%d", &s32Portid);
    ST_Flush();

    printf("Dump Buffer Num:");
    scanf("%d", &s32DumpBuffNum);
    ST_Flush();

    printf("write file path:\n");
    scanf("%s", sFilePath);
    ST_Flush();

    if(s32Portid >= ST_MAX_STREAM_NUM)
    {
        printf("port %d, not valid 0~3 \n", s32Portid);
        return 0;
    }

    pstVpePortAttr = &gstVpeChnattr.stVpePortAttr[s32Portid];

    if(pstVpePortAttr->bUsed != TRUE)
    {
        printf("port %d, not valid \n", s32Portid);
        return 0;
    }

    pthread_mutex_lock(&pstVpePortAttr->Portmutex);

    MI_VPE_GetPortMode(0, s32Portid, &stVpePortMode);
    sprintf(pstVpePortAttr->FilePath, "%s/vpeport%d_%dx%d_pixel%d_%ld.raw", sFilePath, s32Portid, stVpePortMode.u16Width, stVpePortMode.u16Height, stVpePortMode.ePixelFormat, time(&stTime));
    pstVpePortAttr->s32DumpBuffNum = s32DumpBuffNum;

    pthread_mutex_unlock(&pstVpePortAttr->Portmutex);

    return 0;

}

MI_S32 ST_VpeDisablePort()
{
    MI_S32  s32Portid = 0;
    ST_VpePortAttr_t *pstVpePortAttr = NULL;

    printf("select port id:");
    scanf("%d", &s32Portid);
    ST_Flush();

    if(s32Portid >= ST_MAX_STREAM_NUM)
    {
        printf("port %d, not valid 0~3 \n", s32Portid);
        return 0;
    }

    pstVpePortAttr = &gstVpeChnattr.stVpePortAttr[s32Portid];
    pstVpePortAttr->bUsed = FALSE;

    MI_VPE_DisablePort(0, s32Portid);

    return 0;
}

MI_S32 ST_GetVencOut()
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_VENC_Stream_t stStream;
    MI_VENC_Pack_t stPack;
    MI_U32 u32BypassCnt = 0;
    MI_S32 s32DumpBuffNum = 0;
    MI_S32  VencChn = 0;
    MI_VENC_Pack_t *pstPack = NULL;
    MI_U32  i=0;
    FILE *fp = NULL;
    char FilePath[256];
    char sFilePath[128];
    time_t stTime = 0;
    MI_VENC_ChnStat_t stStat;
    MI_VENC_ChnAttr_t stVencAttr;
    memset(&stVencAttr, 0x0, sizeof(MI_VENC_ChnAttr_t));

    memset(&stStat, 0x0, sizeof(MI_VENC_ChnStat_t));
    memset(&stStream, 0, sizeof(MI_VENC_Stream_t));
    memset(&stPack, 0, sizeof(MI_VENC_Pack_t));
    stStream.pstPack = &stPack;
    stStream.u32PackCount = 1;

    printf("select venc chn id:");
    scanf("%d", &VencChn);
    ST_Flush();

    printf("Dump Buffer Num:");
    scanf("%d", &s32DumpBuffNum);
    ST_Flush();

    printf("write file path:\n");
    scanf("%s", sFilePath);
    ST_Flush();

    s32Ret = MI_VENC_GetChnAttr(VencChn, &stVencAttr);

    if(s32Ret != MI_SUCCESS)
    {
        printf("channel %d, not valid \n", VencChn);
        return 0;
    }

    if(stVencAttr.stVeAttr.eType == E_MI_VENC_MODTYPE_JPEGE)
        sprintf(FilePath, "%s/venc_%ld.jpg", sFilePath, time(&stTime));
    else
        sprintf(FilePath, "%s/venc_%ld.es", sFilePath, time(&stTime));

    fp = fopen(FilePath,"wb");

    if(fp == NULL)
    {
        printf("open file %s fail \n",FilePath);
        return 0;
    }

    while(s32DumpBuffNum > 0)
    {
        s32Ret = MI_VENC_Query(VencChn, &stStat);
        if(s32Ret != MI_SUCCESS || stStat.u32CurPacks == 0)
        {
            usleep(1 * 1000);
            continue;
        }

        s32Ret = MI_VENC_GetStream(VencChn, &stStream, 100);
        if(MI_SUCCESS == s32Ret)
        {
            if (0 == u32BypassCnt)
            {
                printf("##########Start to write file %s, id %d !!!#####################\n",FilePath, s32DumpBuffNum);

                for(i = 0; i < stStream.u32PackCount; i ++)
                {
                    pstPack = &stStream.pstPack[i];
                    fwrite(pstPack->pu8Addr + pstPack->u32Offset, pstPack->u32Len - pstPack->u32Offset, 1, fp);
                }
                printf("##########End to write file id %d !!!#####################\n", s32DumpBuffNum);

                s32DumpBuffNum --;
            }
            else
            {
                u32BypassCnt--;
                printf("Bypasss frame %d !\n", u32BypassCnt);
            }
            s32Ret = MI_VENC_ReleaseStream(VencChn, &stStream);
            if(MI_SUCCESS != s32Ret)
            {
                ST_DBG("MI_VENC_ReleaseStream fail, ret:0x%x\n", s32Ret);
            }
        }

        usleep(200 * 1000);
    }
    fclose(fp);

    return 0;
}

MI_S32 ST_ReadLdcTableBin(const char *pConfigPath, LDC_BIN_HANDLE *tldc_bin, MI_U32 *pu32BinSize)
{
    struct stat statbuff;
    MI_U8 *pBufData = NULL;
    MI_S32 s32Fd = 0;
    MI_U32 u32Size = 0;

    if (pConfigPath == NULL)
    {
        ST_ERR("File path null!\n");
        return -1;
    }
    printf("Read file %s\n", pConfigPath);
    memset(&statbuff, 0, sizeof(struct stat));
    if(stat(pConfigPath, &statbuff) < 0)
    {
        ST_ERR("Bb table file not exit!\n");
        return -1;
    }
    else
    {
        if (statbuff.st_size == 0)
        {
            ST_ERR("File size is zero!\n");
            return -1;
        }
        u32Size = statbuff.st_size;
    }
    s32Fd = open(pConfigPath, O_RDONLY);
    if (s32Fd < 0)
    {
        ST_ERR("Open file[%d] error!\n", s32Fd);
        return -1;
    }
    pBufData = (MI_U8 *)malloc(u32Size);
    if (!pBufData)
    {
        ST_ERR("Malloc error!\n");
        close(s32Fd);

        return -1;
    }

    memset(pBufData, 0, u32Size);
    read(s32Fd, pBufData, u32Size);
    close(s32Fd);

    *tldc_bin = pBufData;
    *pu32BinSize = u32Size;

    printf("%d: read buffer %p \n",__LINE__, pBufData);
    printf("%d: &bin address %p, *binbuffer %p \n",__LINE__, tldc_bin, *tldc_bin);

    //free(pBufData);

    return MI_SUCCESS;
}

MI_S32 ST_SetLdcOnOff()
{
    MI_S32 s32LdcOnoff = 0;
    ST_VpeChannelAttr_t *pstVpeChnattr = &gstVpeChnattr;
    MI_VPE_ChannelPara_t stVpeChnParam;
    memset(&stVpeChnParam, 0x0, sizeof(MI_VPE_ChannelPara_t));

    printf("Set Ldc ON(1), OFF(0): \n");
    scanf("%d", &s32LdcOnoff);
    ST_Flush();

    if(s32LdcOnoff == TRUE && pstVpeChnattr->bEnLdc == FALSE)
    {
        printf("ldc onoff %d, before enldc %d need set bin path \n", s32LdcOnoff, pstVpeChnattr->bEnLdc);
        printf("set Ldc libaray cfg path:  ");
        scanf("%s", pstVpeChnattr->LdcCfgbin_Path);
        ST_Flush();

        ST_ReadLdcTableBin(pstVpeChnattr->LdcCfgbin_Path, &pstVpeChnattr->ldcBinBuffer, &pstVpeChnattr->u32LdcBinSize);
    }

    MI_VPE_StopChannel(0);

    MI_VPE_GetChannelParam(0, &stVpeChnParam);
    printf("get channel param  benldc %d, bmirror %d, bflip %d, e3dnrlevel %d, hdrtype %d \n", 
        stVpeChnParam.bEnLdc, stVpeChnParam.bMirror,stVpeChnParam.bFlip,stVpeChnParam.e3DNRLevel,stVpeChnParam.eHDRType);

    stVpeChnParam.bEnLdc = s32LdcOnoff;

    MI_VPE_SetChannelParam(0, &stVpeChnParam);

    if(s32LdcOnoff == TRUE && pstVpeChnattr->bEnLdc == FALSE)
    {
        MI_VPE_LDCBegViewConfig(0);

        MI_VPE_LDCSetViewConfig(0, pstVpeChnattr->ldcBinBuffer, pstVpeChnattr->u32LdcBinSize);

        MI_VPE_LDCEndViewConfig(0);

        free(pstVpeChnattr->ldcBinBuffer);

        pstVpeChnattr->bEnLdc = TRUE;
    }

    MI_VPE_StartChannel(0);

    return 0;
}

void * ST_GetVpeOutputDataThread(void * args)
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_U8  u8Portid = *((MI_U8 *)(args));
    FILE *fp = NULL;
    ST_VpePortAttr_t *pstVpePortAttr = &gstVpeChnattr.stVpePortAttr[u8Portid];
    MI_BOOL bFileOpen = FALSE;

    MI_SYS_ChnPort_t stChnPort;

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VPE;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = 0;
    stChnPort.u32PortId = u8Portid;

    if(pstVpePortAttr->bUsed == TRUE)
    {
        MI_SYS_SetChnOutputPortDepth(&stChnPort, 1, 4);
    }

    while (!bExit)
    {
        pthread_mutex_lock(&pstVpePortAttr->Portmutex);

        if(pstVpePortAttr->s32DumpBuffNum > 0 && bFileOpen == FALSE)
        {
            fp = fopen(pstVpePortAttr->FilePath ,"wb");
            if(fp == NULL)
            {
                printf("file %s open fail\n", pstVpePortAttr->FilePath);
                pstVpePortAttr->s32DumpBuffNum = 0;\
                pthread_mutex_unlock(&pstVpePortAttr->Portmutex);
                continue;
            }
            else
            {
                bFileOpen = TRUE;
            }
        }

        if(pstVpePortAttr->bUsed == TRUE)
        {
            if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &hHandle))
            {
                //printf("get out success \n");
                if(pstVpePortAttr->s32DumpBuffNum > 0)
                {
                    pstVpePortAttr->s32DumpBuffNum--;
                    printf("=======begin writ port %d file id %d, file path %s, bufsize %d, stride %d, height %d\n", u8Portid, pstVpePortAttr->s32DumpBuffNum, pstVpePortAttr->FilePath, 
                        stBufInfo.stFrameData.u32BufSize,stBufInfo.stFrameData.u32Stride[0], stBufInfo.stFrameData.u16Height);

                    fwrite(stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u32BufSize, 1, fp);
                    printf("=======end   writ port %d file id %d, file path %s \n", u8Portid, pstVpePortAttr->s32DumpBuffNum, pstVpePortAttr->FilePath);
                }

                //printf("begin release out \n");
                MI_SYS_ChnOutputPortPutBuf(hHandle);
                //printf("end release out \n");
            }
        }

        if(bFileOpen == TRUE && pstVpePortAttr->s32DumpBuffNum == 0)
        {
            fclose(fp);
            bFileOpen = FALSE;
        }

        pthread_mutex_unlock(&pstVpePortAttr->Portmutex);
        usleep(10*1000);
    }

    return NULL;
}

void *ST_IQthread(void * args)
{
    MI_VIF_ChnPortAttr_t stVifAttr;
    MI_VPE_ChannelPara_t stVpeParam;
    MI_VPE_HDRType_e  eLastHdrType = E_MI_VPE_HDR_TYPE_MAX;
    MI_ISP_IQ_PARAM_INIT_INFO_TYPE_t status;
    MI_U8  u8ispreadycnt = 0;

    memset(&stVifAttr, 0x0, sizeof(MI_VIF_ChnPortAttr_t));
    memset(&stVpeParam, 0x0, sizeof(MI_VPE_ChannelPara_t));

    MI_IQSERVER_Open(1920, 1080, 0);

    while(1)
    {
        if(u8ispreadycnt > 100)
        {
            printf("%s:%d, isp ready time out \n", __FUNCTION__, __LINE__);
            u8ispreadycnt = 0;
        }

        MI_ISP_IQ_GetParaInitStatus(0, &status);
        if(status.stParaAPI.bFlag != 1)
        {
            usleep(10*1000);
            u8ispreadycnt++;
            continue;
        }

        u8ispreadycnt = 0;

        MI_VPE_GetChannelParam(0, &stVpeParam);
        if(eLastHdrType != stVpeParam.eHDRType)
        {
            printf("hdr type change before %d, current %d, load api bin\n", eLastHdrType, stVpeParam.eHDRType);
            MI_ISP_API_CmdLoadBinFile(0, (char *)((stVpeParam.eHDRType>0) ? HDR_PATH : NONHDR_PATH),  1234);
        }
        eLastHdrType = stVpeParam.eHDRType;

        usleep(10*1000);
    }

    return  NULL;
}

MI_S32 ST_BaseModuleInit(MI_SNR_PAD_ID_e eSnrPad, MI_VIF_DEV s32vifDev)
{
    MI_U32 u32CapWidth = 0, u32CapHeight = 0;
    MI_VIF_FrameRate_e eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    MI_SYS_PixelFormat_e ePixFormat;
    MI_SNR_PADInfo_t  stPad0Info;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
    MI_SNR_PAD_ID_e eSnrPadId = eSnrPad;
    MI_VIF_DEV vifDev = s32vifDev;
    MI_VIF_CHN vifChn = s32vifDev*4;
    MI_VPE_CHANNEL vpechn = s32vifDev;
    
    ST_VpeChannelAttr_t *pstVpeChnattr = &gstVpeChnattr;
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
    stDevAttr.eWorkMode = E_MI_VIF_WORK_MODE_RGB_REALTIME;
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
    stChannelVpeAttr.eRunningMode = E_MI_VPE_RUN_REALTIME_MODE;
    stChannelVpeAttr.eSensorBindId = E_MI_VPE_SENSOR0;
    stChannelVpeAttr.bEnLdc = pstVpeChnattr->bEnLdc;
    stChannelVpeAttr.u32ChnPortMode = pstVpeChnattr->u32ChnPortMode;
    ExecFunc(MI_VPE_CreateChannel(vpechn, &stChannelVpeAttr), MI_VPE_OK);

    if(pstVpeChnattr->bEnLdc == TRUE
        && pstVpeChnattr->ldcBinBuffer != NULL)
    {
        MI_VPE_LDCBegViewConfig(vpechn);

        MI_VPE_LDCSetViewConfig(vpechn, pstVpeChnattr->ldcBinBuffer, pstVpeChnattr->u32LdcBinSize);

        MI_VPE_LDCEndViewConfig(vpechn);

        free(pstVpeChnattr->ldcBinBuffer);
    }
    else
        printf("benldc %d, ldc bin buffer %p \n",pstVpeChnattr->bEnLdc, pstVpeChnattr->ldcBinBuffer);

    STCHECKRESULT(MI_VPE_SetChannelRotation(vpechn, pstVpeChnattr->eVpeRotate));
    STCHECKRESULT(MI_VPE_SetChannelCrop(vpechn, &pstVpeChnattr->stVpeChnCrop));

    STCHECKRESULT(ST_Vpe_StartChannel(vpechn));

    for(i=0; i<ST_MAX_STREAM_NUM-1; i++)
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
        ExecFunc(MI_VPE_EnablePort(vpechn, 3), MI_VPE_OK);
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
    ST_VpeChannelAttr_t *pstVpeChnattr = &gstVpeChnattr;
    MI_U32 i = 0;
    ST_Sys_BindInfo_T stBindInfo;

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

    for(i = 0; i < ST_MAX_STREAM_NUM; i ++)
    {
        if(pstVpeChnattr->stVpePortAttr[i].bUsed == TRUE)
        {
            STCHECKRESULT(ST_Vpe_StopPort(0, i));
        }
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

    MI_SNR_Disable(E_MI_SNR_PAD_ID_0);
    /************************************************
    Step4:  destory SYS
    *************************************************/

    STCHECKRESULT(ST_Sys_Exit());

    return MI_SUCCESS;
}

MI_S32 ST_VencStart(void)
{
    MI_U32 u32VenBitRate = 0;
    ST_VencAttr_t *pstStreamAttr = gstVencattr;
    MI_U32 i = 0;
    ST_Sys_BindInfo_T stBindInfo;
    MI_VENC_CHN VencChn = 0;
    MI_U32 u32DevId = -1;
    #if 0
    MI_U32 u32MaxVencWidth = 3840;
    MI_U32 u32MaxVencHeight = 2160;
    #else
    MI_U32 u32MaxVencWidth = 1920;
    MI_U32 u32MaxVencHeight = 1088;
    #endif
    /************************************************
    Step4:  init VENC
    *************************************************/
    MI_VENC_ChnAttr_t stChnAttr;

    for(i = 0; i < ST_MAX_STREAM_NUM; i ++)
    {
        memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));

        if(pstStreamAttr[i].bUsed != TRUE)
            continue;

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
        else if(pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_H265E)
        {
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = pstStreamAttr[i].u32Width;
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = pstStreamAttr[i].u32Height;
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
        else if(pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_JPEGE)
        {
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = pstStreamAttr[i].u32Width;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = pstStreamAttr[i].u32Height;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = u32MaxVencWidth;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = u32MaxVencHeight;
        }

        stChnAttr.stVeAttr.eType = pstStreamAttr[i].eType;
        VencChn = pstStreamAttr[i].vencChn;

        STCHECKRESULT(ST_Venc_CreateChannel(VencChn, &stChnAttr));

        ExecFunc(MI_VENC_GetChnDevid(VencChn, &u32DevId), MI_SUCCESS);

        memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = pstStreamAttr[i].u32BindVpeChn;
        stBindInfo.stSrcChnPort.u32PortId = pstStreamAttr[i].u32BindVpePort;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
        stBindInfo.stDstChnPort.u32DevId = u32DevId;
        stBindInfo.stDstChnPort.u32ChnId = VencChn;
        stBindInfo.stDstChnPort.u32PortId = 0;

        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 30;
        stBindInfo.eBindType = pstStreamAttr[i].eBindType;
        stBindInfo.u32BindParam = pstStreamAttr[i].u32BindParam;
        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

        STCHECKRESULT(ST_Venc_StartChannel(VencChn));
    }

    return MI_SUCCESS;
}

MI_S32 ST_VencStop(void)
{
    MI_U32 u32ArraySize = ARRAY_SIZE(gstVencattr);
    ST_VencAttr_t *pstStreamAttr = gstVencattr;
    MI_U32 i = 0;
    ST_Sys_BindInfo_T stBindInfo;
    MI_VENC_CHN VencChn = 0;

    /************************************************
    Step1:  unbind VIF->VPE
    *************************************************/

    /************************************************
    Step2:  unbind and stop VPE port
    *************************************************/
    MI_U32 u32DevId = -1;

    for(i = 0; i < u32ArraySize; i ++)
    {
        if(pstStreamAttr[i].bUsed != TRUE)
            continue;

        VencChn = pstStreamAttr[i].vencChn;
        ExecFunc(MI_VENC_GetChnDevid(VencChn, &u32DevId), MI_SUCCESS);

        memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = pstStreamAttr[i].u32BindVpeChn;
        stBindInfo.stSrcChnPort.u32PortId = pstStreamAttr[i].u32BindVpePort;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
        stBindInfo.stDstChnPort.u32DevId = u32DevId;
        stBindInfo.stDstChnPort.u32ChnId = VencChn;
        stBindInfo.stDstChnPort.u32PortId = 0;

        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 30;
        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));


        STCHECKRESULT(ST_Venc_StopChannel(VencChn));
        STCHECKRESULT(ST_Venc_DestoryChannel(VencChn));
    }

    return MI_SUCCESS;
}

void ST_SetArgs()
{
    MI_S32 s32HDRtype = 0;
    MI_S32 s323DNRLevel = 0;
    MI_S32 s32Rotation = 0;
    MI_S32 s32ChannelCropX =0, s32ChannelCropY=0,s32ChannelCropW=0,s32ChannelCropH =0;
    MI_S32 s32bEnLdc =0;
    MI_S32 s32ChnPortMode = 0, s32PortSelect =0;
    MI_U8 i=0;

    ST_VpeChannelAttr_t *pstVpeChnattr = &gstVpeChnattr;

    printf("preview by venc ?(0: write file, 1:preview use rtsp)\n");
    scanf("%d", &gbPreviewByVenc);
    ST_Flush();

    printf("Use HDR ?\n 0 not use, 1 use DOL, 2 use VC, 3 use EMBEDDED, 4 use LI\n");
    printf("sony sensor(ex imx307) use DOL, sc sensor(ex sc4238) use VC\n");
    scanf("%d", &s32HDRtype);
    ST_Flush();

    printf("3dnr level(0~2):");
    scanf("%d", &s323DNRLevel);
    ST_Flush();

    printf("rotation(0:0, 1:90, 2:180, 3:270, 4:mirror, 5:flip):");
    scanf("%d", &s32Rotation);
    ST_Flush();

    printf("Channel Crop x:");
    scanf("%d", &s32ChannelCropX);
    ST_Flush();

    printf("Channel Crop y:");
    scanf("%d", &s32ChannelCropY);
    ST_Flush();

    printf("Channel Crop width:");
    scanf("%d", &s32ChannelCropW);
    ST_Flush();

    printf("Channel Crop height:");
    scanf("%d", &s32ChannelCropH);
    ST_Flush();

    printf("bEnLdc:");
    scanf("%d", &s32bEnLdc);
    ST_Flush();

    if(s32bEnLdc == TRUE)
    {
        printf("set Ldc libaray cfg path:  ");
        scanf("%s", pstVpeChnattr->LdcCfgbin_Path);
        ST_Flush();
    }

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

    for(i=0; i<ST_MAX_STREAM_NUM-1; i++)
    {
        printf("port %d 0:real, 1:frame:", i);
        scanf("%d", &s32PortSelect);
        ST_Flush();
        
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

    for(i=0; i<ST_MAX_STREAM_NUM; i++)
    {
        MI_S32 s32BusePort = 0;
        MI_S32 s32PortCropX =0, s32PortCropY=0,s32PortCropW=0,s32PortCropH =0;
        MI_S32 s32PortPixelFormat =0;
        MI_S32 s32PortMirror=0, s32PortFlip=0;
        MI_S32 s32PortW=0, s32PortH=0;
        printf("use port%d ?(0,not use, 1 use):", i);
        scanf("%d", &s32BusePort);
        ST_Flush();

        if(s32BusePort > 0)
        {
            ST_VpePortAttr_t  *pstVpePortAttr = &pstVpeChnattr->stVpePortAttr[i];
            if(i < ST_MAX_STREAM_NUM-1)
            {
                printf("port %d port crop x:", i);
                scanf("%d", &s32PortCropX);
                ST_Flush();

                printf("port %d port crop y:", i);
                scanf("%d", &s32PortCropY);
                ST_Flush();

                printf("port %d port crop width:", i);
                scanf("%d", &s32PortCropW);
                ST_Flush();

                printf("port %d port crop height:", i);
                scanf("%d", &s32PortCropH);
                ST_Flush();

                printf("port %d bmirror:", i);
                scanf("%d", &s32PortMirror);
                ST_Flush();

                printf("port %d bflip:", i);
                scanf("%d", &s32PortFlip);
                ST_Flush();

                printf("port %d port width:", i);
                scanf("%d", &s32PortW);
                ST_Flush();

                printf("port %d port height:", i);
                scanf("%d", &s32PortH);
                ST_Flush();

                if(gbPreviewByVenc > 0)
                {
                    MI_S32 s32BindType =0;
                    MI_S32 eType = 0;
                    ST_VencAttr_t *pstVencAttr = &gstVencattr[i];
                    printf("vpe port bindtype venc ?\n0 frame mode, 1 ring mode, 2 imi mode, 3 ring mode(half)\n");
                    scanf("%d", &s32BindType);
                    ST_Flush();
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

                    printf("venc encode type ?\n 2 h264, 3 h265, 4 JPEG\n");
                    scanf("%d", &eType);
                    ST_Flush();

                    if(eType == 4)
                    {
                        pstVencAttr->eType = E_MI_VENC_MODTYPE_JPEGE;
                        s32PortPixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
                    }
                    else
                    {
                        pstVencAttr->eType = (MI_VENC_ModType_e)eType;
                        s32PortPixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
                    }

                    pstVencAttr->vencChn = i;
                    pstVencAttr->bUsed = TRUE;
                    sprintf(pstVencAttr->szStreamName, "video%d", i);
                    pstVencAttr->u32Width = s32PortW;
                    pstVencAttr->u32Height = s32PortH;
                    pstVencAttr->u32BindVpeChn = 0;
                    pstVencAttr->u32BindVpePort = i;
                }
                else
                {
                    printf("port %d port pixel:", i);
                    printf("yuv422:0, argb8888:1, abgr8888:2, bgra8888:3, yuv420:11\n");
                    scanf("%d", &s32PortPixelFormat);
                    ST_Flush();
                }
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

    if(pstVpeChnattr->bEnLdc == TRUE)
        ST_ReadLdcTableBin(pstVpeChnattr->LdcCfgbin_Path, &pstVpeChnattr->ldcBinBuffer, &pstVpeChnattr->u32LdcBinSize);

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



MI_BOOL ST_DoChangeRes(MI_SNR_PAD_ID_e eSnrPad, MI_VIF_DEV s32vifDev)
{
    char select = 0xFF;
    MI_U8 u8ResIdx =0;
    MI_SNR_Res_t stRes;

    ST_VpeChannelAttr_t *pstVpeChnattr = &gstVpeChnattr;

    ST_Sys_BindInfo_T stBindInfo;
    MI_SNR_PADInfo_t  stPad0Info;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
    MI_U32 u32CapWidth = 0, u32CapHeight = 0;
    MI_VIF_FrameRate_e eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    MI_SYS_PixelFormat_e ePixFormat;
    MI_U32 u32ResCount =0;
    MI_U8 u8ResIndex =0;
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

    u8ResIdx = atoi(&select);
    if(u8ResIdx >= u32ResCount)
        return 0;

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

    STCHECKRESULT(ST_Vpe_StopChannel(0));
    /************************************************
    Step3:  destory VIF
    *************************************************/
    STCHECKRESULT(ST_Vif_StopPort(vifChn, 0));
    STCHECKRESULT(ST_Vif_DisableDev(vifDev));
    MI_SNR_Disable(eSnrPad);

    if(pstVpeChnattr->eHdrType > 0)
    {
        MI_SNR_SetPlaneMode(eSnrPad, TRUE);
    }
    else
    {
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
    stVifPortInfoInfo.ePixFormat = ePixFormat;//E_MI_SYS_PIXEL_FRAME_RGB_BAYER_12BPP_GR;
    STCHECKRESULT(ST_Vif_CreatePort(vifChn, 0, &stVifPortInfoInfo));
    STCHECKRESULT(ST_Vif_StartPort(0, vifChn, 0));

    STCHECKRESULT(ST_Vpe_StartChannel(0));

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

    return 0;
}

MI_BOOL ST_DoChangeHDRtype()
{
    MI_S32 select = 0;
    MI_U8 u8ResIdx =0;
    MI_SNR_Res_t stRes;
    MI_SNR_PAD_ID_e eSnrPad = E_MI_SNR_PAD_ID_0;

    ST_Sys_BindInfo_T stBindInfo;
    MI_SNR_PADInfo_t  stPad0Info;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
    MI_U32 u32CapWidth = 0, u32CapHeight = 0;
    MI_VIF_FrameRate_e eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    MI_SYS_PixelFormat_e ePixFormat;
    MI_VIF_HDRType_e eVifHdrType = E_MI_VIF_HDR_TYPE_OFF;
    MI_VPE_HDRType_e eVpeHdrType = E_MI_VPE_HDR_TYPE_OFF;
    MI_VIF_DEV vifDev = 0;
    MI_VIF_CHN vifChn = 0;
    MI_VPE_ChannelPara_t stVpeChParam;

    ST_VpeChannelAttr_t *pstVpeChnattr = &gstVpeChnattr;
    memset(&stVpeChParam, 0x0, sizeof(MI_VPE_ChannelPara_t));
    memset(&stRes, 0x0, sizeof(MI_SNR_Res_t));

    printf("Use HDR ?\n 0 not use, 1 use DOL, 2 use VC, 3 use EMBEDDED, 4 use LI\n");
    printf("sony sensor(ex imx307) use DOL, sc sensor(ex sc4238) use VC\n");
    scanf("%d", &select);
    ST_Flush();
    printf("You select %d HDR\n", select);

    if(select == 0)
    {
        eVifHdrType = E_MI_VIF_HDR_TYPE_OFF;
        eVpeHdrType = E_MI_VPE_HDR_TYPE_OFF;
    }
    else if(select == 1)

    {
        eVifHdrType = E_MI_VIF_HDR_TYPE_DOL;
        eVpeHdrType = E_MI_VPE_HDR_TYPE_DOL;
    }
    else if(select == 2)
    {
        eVifHdrType = E_MI_VIF_HDR_TYPE_VC;
        eVpeHdrType = E_MI_VPE_HDR_TYPE_VC;
    }
    else if(select == 3)
    {
        eVifHdrType = E_MI_VIF_HDR_TYPE_EMBEDDED;
        eVpeHdrType = E_MI_VPE_HDR_TYPE_EMBEDDED;
    }
    else if(select == 4)
    {
        eVifHdrType = E_MI_VIF_HDR_TYPE_LI;
        eVpeHdrType = E_MI_VPE_HDR_TYPE_LI;
    }
    else
    {
        printf("select hdrtype %d not support \n", select);
        return 0;
    }

    pstVpeChnattr->eHdrType = eVpeHdrType;

    /************************************************
    Step1:  unbind VIF->VPE
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

    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

    STCHECKRESULT(ST_Vpe_StopChannel(0));

    /************************************************
    Step3:  destory VIF
    *************************************************/
    STCHECKRESULT(ST_Vif_StopPort(vifChn, 0));
    STCHECKRESULT(ST_Vif_DisableDev(vifDev));
    MI_SNR_Disable(eSnrPad);

    if(E_MI_VIF_HDR_TYPE_OFF==eVifHdrType 
        || E_MI_VIF_HDR_TYPE_EMBEDDED==eVifHdrType
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
    stVifPortInfoInfo.ePixFormat = ePixFormat;
    STCHECKRESULT(ST_Vif_CreatePort(vifChn, 0, &stVifPortInfoInfo));
    STCHECKRESULT(ST_Vif_StartPort(0, vifChn, 0));

    /************************************************
    Step3:  init VPE (create one VPE)
    *************************************************/
    MI_VPE_GetChannelParam(0, &stVpeChParam);
    stVpeChParam.eHDRType = eVpeHdrType;
    MI_VPE_SetChannelParam(0, &stVpeChParam);

    STCHECKRESULT(ST_Vpe_StartChannel(0));

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

    return 0;

}

MI_BOOL ST_DoChangeRotate()
{
    ST_Sys_BindInfo_T stBindInfo;
    MI_VIF_CHN vifChn = 0;
    MI_S32 s32Rotation = 0;
    MI_S32 s32Mirror = 0;
    MI_S32 s32Flip = 0;
    ST_VpeChannelAttr_t *pstVpeChnattr = &gstVpeChnattr;
    ST_VencAttr_t *pstVencattr = gstVencattr;
    MI_U8 i=0;
    MI_VPE_CHANNEL  VpeChn = 0;

    printf("rotation(0:0, 1:90, 2:180, 3:270):");
    scanf("%d", &s32Rotation);
    ST_Flush();

    printf("bmirror 0: FALSE, 1:TRUE :");
    scanf("%d", &s32Mirror);
    ST_Flush();

    printf("bFlip 0: FALSE, 1:TRUE :");
    scanf("%d", &s32Flip);
    ST_Flush();

    pstVpeChnattr->eVpeRotate = (MI_SYS_Rotate_e)s32Rotation;
    pstVpeChnattr->bChnFlip = s32Flip;
    pstVpeChnattr->bChnMirror = s32Mirror;

    /************************************************
    Step1:  unbind VIF->VPE
    *************************************************/
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = vifChn;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = VpeChn;
    stBindInfo.stDstChnPort.u32PortId = 0;

    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

    if(gbPreviewByVenc == TRUE)
    {
        for(i=0; i<ST_MAX_STREAM_NUM; i++)
        {
            if(pstVencattr[i].bUsed == TRUE)
                ExecFunc(MI_VENC_StopRecvPic(pstVencattr[i].vencChn), MI_SUCCESS);
        }
    }

    STCHECKRESULT(ST_Vpe_StopChannel(VpeChn));
    STCHECKRESULT(ST_Vif_StopPort(vifChn, 0));

    /************************************************
    Step3:  init VPE (create one VPE)
    *************************************************/

    MI_VPE_SetChannelRotation(VpeChn, pstVpeChnattr->eVpeRotate);
    MI_VPE_ChannelPara_t stChnParam;
    memset(&stChnParam, 0x0, sizeof(MI_VPE_ChannelPara_t));

    MI_VPE_GetChannelParam(VpeChn, &stChnParam);
    stChnParam.bMirror = pstVpeChnattr->bChnMirror;
    stChnParam.bFlip = pstVpeChnattr->bChnFlip;
    MI_VPE_SetChannelParam(VpeChn, &stChnParam);

    for(i=0; i<ST_MAX_STREAM_NUM; i++)
    {
        MI_SYS_WindowRect_t stOutCropInfo;
        MI_VPE_PortMode_t stVpeMode;
        memset(&stOutCropInfo, 0x0, sizeof(MI_SYS_WindowRect_t));
        memset(&stVpeMode, 0x0, sizeof(MI_VPE_PortMode_t));

        if(pstVpeChnattr->stVpePortAttr[i].bUsed == TRUE)
        {
            MI_VPE_GetPortCrop(VpeChn, i, &stOutCropInfo);
            MI_VPE_GetPortMode(VpeChn , i, &stVpeMode);
            if(pstVpeChnattr->eVpeRotate == E_MI_SYS_ROTATE_90
                || pstVpeChnattr->eVpeRotate == E_MI_SYS_ROTATE_270)
            {
                stOutCropInfo.u16X = pstVpeChnattr->stVpePortAttr[i].stOrigPortCrop.u16Y;
                stOutCropInfo.u16Y = pstVpeChnattr->stVpePortAttr[i].stOrigPortCrop.u16X;
                stOutCropInfo.u16Width = pstVpeChnattr->stVpePortAttr[i].stOrigPortCrop.u16Height;
                stOutCropInfo.u16Height = pstVpeChnattr->stVpePortAttr[i].stOrigPortCrop.u16Width;

                stVpeMode.u16Width = pstVpeChnattr->stVpePortAttr[i].stOrigPortSize.u16Height;
                stVpeMode.u16Height = pstVpeChnattr->stVpePortAttr[i].stOrigPortSize.u16Width;
            }
            else
            {
                stOutCropInfo.u16X = pstVpeChnattr->stVpePortAttr[i].stOrigPortCrop.u16X;
                stOutCropInfo.u16Y = pstVpeChnattr->stVpePortAttr[i].stOrigPortCrop.u16Y;
                stOutCropInfo.u16Width = pstVpeChnattr->stVpePortAttr[i].stOrigPortCrop.u16Width;
                stOutCropInfo.u16Height = pstVpeChnattr->stVpePortAttr[i].stOrigPortCrop.u16Height;

                stVpeMode.u16Width = pstVpeChnattr->stVpePortAttr[i].stOrigPortSize.u16Width;
                stVpeMode.u16Height = pstVpeChnattr->stVpePortAttr[i].stOrigPortSize.u16Height;
            }
            MI_VPE_SetPortCrop(VpeChn, i, &stOutCropInfo);
            MI_VPE_SetPortMode(VpeChn , i, &stVpeMode);

            if(gbPreviewByVenc == TRUE)
            {
                MI_VENC_ChnAttr_t stChnAttr;
                memset(&stChnAttr, 0x0, sizeof(MI_VENC_ChnAttr_t));

                if(pstVencattr[i].bUsed == TRUE)
                {
                    ExecFunc(MI_VENC_GetChnAttr(pstVencattr[i].vencChn, &stChnAttr), MI_SUCCESS);
                    if(pstVencattr[i].eType == E_MI_VENC_MODTYPE_H264E)
                    {
                        stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = stVpeMode.u16Width;
                        stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = stVpeMode.u16Height;
                    }
                    else if(pstVencattr[i].eType == E_MI_VENC_MODTYPE_H264E)
                    {
                        stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = stVpeMode.u16Width;
                        stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = stVpeMode.u16Height;
                    }
                    else if(pstVencattr[i].eType == E_MI_VENC_MODTYPE_H264E)
                    {
                        stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = stVpeMode.u16Width;
                        stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = stVpeMode.u16Height;
                    }
                    ExecFunc(MI_VENC_SetChnAttr(pstVencattr[i].vencChn, &stChnAttr), MI_SUCCESS);
                }
            }
        }
    }

    STCHECKRESULT(ST_Vif_StartPort(0, vifChn, 0));
    STCHECKRESULT(ST_Vpe_StartChannel(VpeChn));
    if(gbPreviewByVenc == TRUE)
    {
        for(i=0; i<ST_MAX_STREAM_NUM; i++)
        {
            if(pstVencattr[i].bUsed == TRUE)
                ExecFunc(MI_VENC_StartRecvPic(pstVencattr[i].vencChn), MI_SUCCESS);
        }
    }

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
    stBindInfo.stDstChnPort.u32ChnId = VpeChn;
    stBindInfo.stDstChnPort.u32PortId = 0;

    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

    return 0;

}

MI_BOOL ST_ResetPortMode()
{
    MI_S32 s32Portid = 0;
    MI_S32 s32PortPixelFormat =0;
    MI_S32 s32PortMirror=0, s32PortFlip=0;
    MI_S32 s32PortW=0, s32PortH=0;
    ST_VpeChannelAttr_t *pstVpeChnattr = &gstVpeChnattr;
    ST_VencAttr_t *pstVencattr = gstVencattr;
    ST_VpePortAttr_t *pstVpePortAttr = pstVpeChnattr->stVpePortAttr;
    MI_VPE_PortMode_t  stVpePortMode;
    ST_Sys_BindInfo_T stBindInfo;
    MI_U32 u32DevId = 0;
    memset(&stVpePortMode, 0x0, sizeof(MI_VPE_PortMode_t));
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));

    printf("select port id:");
    scanf("%d", &s32Portid);
    ST_Flush();

    if(s32Portid >= ST_MAX_STREAM_NUM)
    {
        printf("port %d, not valid \n", s32Portid);
        return 0;
    }

    printf("port %d bmirror:", s32Portid);
    scanf("%d", &s32PortMirror);
    ST_Flush();
    
    printf("port %d bflip:", s32Portid);
    scanf("%d", &s32PortFlip);
    ST_Flush();
    
    printf("port %d port width:", s32Portid);
    scanf("%d", &s32PortW);
    ST_Flush();
    
    printf("port %d port height:", s32Portid);
    scanf("%d", &s32PortH);
    ST_Flush();

    MI_VPE_GetPortMode(0, s32Portid, &stVpePortMode);

    if(gbPreviewByVenc == FALSE)
    {
        printf("port %d port pixel:", s32Portid);
        printf("yuv422:0, argb8888:1, abgr8888:2, bgra8888:3, yuv420:11\n");
        scanf("%d", &s32PortPixelFormat);
        ST_Flush();
    }
    else
        s32PortPixelFormat = stVpePortMode.ePixelFormat;

    if(gbPreviewByVenc == TRUE)
    {
        ExecFunc(MI_VENC_GetChnDevid(pstVencattr[s32Portid].vencChn, &u32DevId), MI_SUCCESS);

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = 0;
        stBindInfo.stSrcChnPort.u32PortId = s32Portid;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
        stBindInfo.stDstChnPort.u32DevId = u32DevId;
        stBindInfo.stDstChnPort.u32ChnId = pstVencattr[s32Portid].vencChn;
        stBindInfo.stDstChnPort.u32PortId = 0;
        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

        if(pstVencattr[s32Portid].bUsed == TRUE)
            ExecFunc(MI_VENC_StopRecvPic(pstVencattr[s32Portid].vencChn), MI_SUCCESS);
    }

    MI_VPE_DisablePort(0, s32Portid);

    pstVpePortAttr[s32Portid].bMirror = s32PortMirror;
    pstVpePortAttr[s32Portid].bFlip = s32PortFlip;
    pstVpePortAttr[s32Portid].stOrigPortSize.u16Width = s32PortW;
    pstVpePortAttr[s32Portid].stOrigPortSize.u16Height = s32PortH;
    pstVpePortAttr[s32Portid].ePixelFormat = (MI_SYS_PixelFormat_e)s32PortPixelFormat;
    pstVpePortAttr[s32Portid].bUsed = TRUE;

    
    stVpePortMode.bMirror = pstVpePortAttr[s32Portid].bMirror;
    stVpePortMode.bFlip = pstVpePortAttr[s32Portid].bFlip;
    stVpePortMode.ePixelFormat = pstVpePortAttr[s32Portid].ePixelFormat;
    if(pstVpeChnattr->eVpeRotate == E_MI_SYS_ROTATE_90
        || pstVpeChnattr->eVpeRotate == E_MI_SYS_ROTATE_270)
    {
        stVpePortMode.u16Width = pstVpePortAttr[s32Portid].stOrigPortSize.u16Height;
        stVpePortMode.u16Height = pstVpePortAttr[s32Portid].stOrigPortSize.u16Width;
    }
    else
    {
        stVpePortMode.u16Width = pstVpePortAttr[s32Portid].stOrigPortSize.u16Width;
        stVpePortMode.u16Height = pstVpePortAttr[s32Portid].stOrigPortSize.u16Height;
    }

    MI_VPE_SetPortMode(0, s32Portid, &stVpePortMode);

    if(gbPreviewByVenc == TRUE)
    {
        MI_VENC_ChnAttr_t stChnAttr;
        memset(&stChnAttr, 0x0, sizeof(MI_VENC_ChnAttr_t));
    
        if(pstVencattr[s32Portid].bUsed == TRUE)
        {
            ExecFunc(MI_VENC_GetChnAttr(pstVencattr[s32Portid].vencChn, &stChnAttr), MI_SUCCESS);
            if(pstVencattr[s32Portid].eType == E_MI_VENC_MODTYPE_H264E)
            {
                stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = stVpePortMode.u16Width;
                stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = stVpePortMode.u16Height;
            }
            else if(pstVencattr[s32Portid].eType == E_MI_VENC_MODTYPE_H264E)
            {
                stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = stVpePortMode.u16Width;
                stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = stVpePortMode.u16Height;
            }
            else if(pstVencattr[s32Portid].eType == E_MI_VENC_MODTYPE_H264E)
            {
                stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = stVpePortMode.u16Width;
                stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = stVpePortMode.u16Height;
            }
            ExecFunc(MI_VENC_SetChnAttr(pstVencattr[s32Portid].vencChn, &stChnAttr), MI_SUCCESS);
        }
        else
            printf("port %d, venc buse %d \n", s32Portid, pstVencattr[s32Portid].bUsed);
    }

    MI_VPE_EnablePort(0, s32Portid);

    MI_SYS_ChnPort_t stChnPort;

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VPE;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = 0;
    stChnPort.u32PortId = s32Portid;
    MI_SYS_SetChnOutputPortDepth(&stChnPort, 1, 4);

    if(gbPreviewByVenc == TRUE)
    {
        if(pstVencattr[s32Portid].bUsed == TRUE)
            ExecFunc(MI_VENC_StartRecvPic(pstVencattr[s32Portid].vencChn), MI_SUCCESS);

        ExecFunc(MI_VENC_GetChnDevid(pstVencattr[s32Portid].vencChn, &u32DevId), MI_SUCCESS);

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = 0;
        stBindInfo.stSrcChnPort.u32PortId = s32Portid;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
        stBindInfo.stDstChnPort.u32DevId = u32DevId;
        stBindInfo.stDstChnPort.u32ChnId = pstVencattr[s32Portid].vencChn;
        stBindInfo.stDstChnPort.u32PortId = 0;

        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 30;
        stBindInfo.eBindType = pstVencattr[s32Portid].eBindType;
        stBindInfo.u32BindParam = pstVencattr[s32Portid].u32BindParam;
        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    }

    return 0;

}

MI_BOOL ST_DoSetChnCrop()
{
    MI_S32 s32ChannelCropX =0, s32ChannelCropY=0,s32ChannelCropW=0,s32ChannelCropH =0;
    ST_VpeChannelAttr_t *pstVpeChnattr = &gstVpeChnattr;
    MI_SYS_WindowRect_t stVpeChnCrop;
    memset(&stVpeChnCrop, 0x0, sizeof(MI_SYS_WindowRect_t));

    printf("Channel Crop x:");
    scanf("%d", &s32ChannelCropX);
    ST_Flush();

    printf("Channel Crop y:");
    scanf("%d", &s32ChannelCropY);
    ST_Flush();

    printf("Channel Crop width:");
    scanf("%d", &s32ChannelCropW);
    ST_Flush();

    printf("Channel Crop height:");
    scanf("%d", &s32ChannelCropH);
    ST_Flush();

    pstVpeChnattr->stOrgVpeChnCrop.u16X = s32ChannelCropX;
    pstVpeChnattr->stOrgVpeChnCrop.u16Y = s32ChannelCropY;
    pstVpeChnattr->stOrgVpeChnCrop.u16Width = s32ChannelCropW;
    pstVpeChnattr->stOrgVpeChnCrop.u16Height = s32ChannelCropH;

    if(pstVpeChnattr->eVpeRotate == E_MI_SYS_ROTATE_90
        || pstVpeChnattr->eVpeRotate == E_MI_SYS_ROTATE_270)
    {
        stVpeChnCrop.u16X = pstVpeChnattr->stOrgVpeChnCrop.u16Y;
        stVpeChnCrop.u16Y = pstVpeChnattr->stOrgVpeChnCrop.u16X;
        stVpeChnCrop.u16Width = pstVpeChnattr->stOrgVpeChnCrop.u16Height;
        stVpeChnCrop.u16Height = pstVpeChnattr->stOrgVpeChnCrop.u16Width;
    }
    else
    {
        stVpeChnCrop.u16X = pstVpeChnattr->stOrgVpeChnCrop.u16X;
        stVpeChnCrop.u16Y = pstVpeChnattr->stOrgVpeChnCrop.u16Y;
        stVpeChnCrop.u16Width = pstVpeChnattr->stOrgVpeChnCrop.u16Width;
        stVpeChnCrop.u16Height = pstVpeChnattr->stOrgVpeChnCrop.u16Height;
    }
    MI_VPE_SetChannelCrop(0,&stVpeChnCrop);

    return 0;
}

MI_BOOL ST_DoSetPortCrop()
{
    MI_S32 s32Portid = 0;
    MI_S32 s32PortCropX =0, s32PortCropY=0,s32PortCropW=0,s32PortCropH =0;
    ST_VpeChannelAttr_t *pstVpeChnattr = &gstVpeChnattr;
    ST_VpePortAttr_t *pstVpePortAttr = pstVpeChnattr->stVpePortAttr;
    MI_SYS_WindowRect_t stPortCropSize;
    memset(&stPortCropSize, 0x0, sizeof(MI_SYS_WindowRect_t));

    printf("select port id:");
    scanf("%d", &s32Portid);
    ST_Flush();

    if(s32Portid >= ST_MAX_STREAM_NUM || pstVpePortAttr[s32Portid].bUsed != TRUE)
    {
        printf("port %d, not valid \n", s32Portid);
        return 0;
    }

     printf("port %d port crop x:", s32Portid);
    scanf("%d", &s32PortCropX);
    ST_Flush();

    printf("port %d port crop y:", s32Portid);
    scanf("%d", &s32PortCropY);
    ST_Flush();

    printf("port %d port crop width:", s32Portid);
    scanf("%d", &s32PortCropW);
    ST_Flush();

    printf("port %d port crop height:", s32Portid);
    scanf("%d", &s32PortCropH);
    ST_Flush();

    pstVpePortAttr[s32Portid].stOrigPortCrop.u16X = s32PortCropX;
    pstVpePortAttr[s32Portid].stOrigPortCrop.u16Y = s32PortCropY;
    pstVpePortAttr[s32Portid].stOrigPortCrop.u16Width = s32PortCropW;
    pstVpePortAttr[s32Portid].stOrigPortCrop.u16Height = s32PortCropH;

    if(pstVpeChnattr->eVpeRotate == E_MI_SYS_ROTATE_90
        || pstVpeChnattr->eVpeRotate == E_MI_SYS_ROTATE_270)
    {
        stPortCropSize.u16X = pstVpePortAttr[s32Portid].stOrigPortCrop.u16Y;
        stPortCropSize.u16Y = pstVpePortAttr[s32Portid].stOrigPortCrop.u16X;
        stPortCropSize.u16Width = pstVpePortAttr[s32Portid].stOrigPortCrop.u16Height;
        stPortCropSize.u16Height = pstVpePortAttr[s32Portid].stOrigPortCrop.u16Width;
    }
    else
    {
        stPortCropSize.u16X = pstVpePortAttr[s32Portid].stOrigPortCrop.u16X;
        stPortCropSize.u16Y = pstVpePortAttr[s32Portid].stOrigPortCrop.u16Y;
        stPortCropSize.u16Width = pstVpePortAttr[s32Portid].stOrigPortCrop.u16Width;
        stPortCropSize.u16Height = pstVpePortAttr[s32Portid].stOrigPortCrop.u16Height;
    }
    
    MI_VPE_SetPortCrop(0 , s32Portid, &stPortCropSize);

    return 0;
}

int main(int argc, char **argv)
{
    MI_U8  i = 0;
    MI_SNR_PAD_ID_e eSnrPad = E_MI_SNR_PAD_ID_0;
    MI_VIF_DEV vifDev = 0;

    ST_SetArgs();

    STCHECKRESULT(ST_BaseModuleInit(eSnrPad, vifDev));

    if(gbPreviewByVenc == TRUE)
    {
        STCHECKRESULT(ST_VencStart());
        ST_RtspServerStart();
    }
#if 0
    pthread_t pIQthread;
    pthread_create(&pIQthread, NULL, ST_IQthread, NULL);
#endif

    MI_U8 u8PortId[4]={0,1,2,3};

    for(i=0; i< ST_MAX_STREAM_NUM; i++)
    {
        pthread_mutex_init(&gstVpeChnattr.stVpePortAttr[i].Portmutex, NULL);
        pthread_create(&gstVpeChnattr.stVpePortAttr[i].pGetDatathread, NULL, ST_GetVpeOutputDataThread, (void *)(&u8PortId[i]));
    }

    while(!bExit)
    {
        MI_U32 u32Select = 0xff;
        printf("select 0: change res \n");
        printf("select 1: change hdrtype\n");
        printf("select 2: change rotate\n");
        printf("select 3: change chancrop\n");
        printf("select 4: change portMode\n");
        printf("select 5: change portcrop\n");
        printf("select 6: Get port buffer\n");
        printf("select 7: disable port \n");
        printf("select 8: get venc out \n");
        printf("select 9: Ldc on/off \n");
        printf("select 11: exit\n");
        scanf("%d", &u32Select);
        ST_Flush();
        if(u32Select == 0)
            bExit = ST_DoChangeRes(eSnrPad, vifDev);
        else if(u32Select == 1)
        {
            bExit = ST_DoChangeHDRtype();
        }
        else if(u32Select == 2)
        {
            bExit =ST_DoChangeRotate();
        }
        else if(u32Select == 3)
        {
            bExit =ST_DoSetChnCrop();
        }
        else if(u32Select == 4)
        {
            bExit =ST_ResetPortMode();
        }
        else if(u32Select == 5)
        {
            bExit =ST_DoSetPortCrop();
        }
        else if(u32Select == 6)
        {
            bExit =ST_GetVpeOutputData();
        }
        else if(u32Select == 7)
        {
            bExit =ST_VpeDisablePort();
        }
        else if(u32Select == 8)
        {
            bExit =ST_GetVencOut();
        }
        else if(u32Select == 9)
        {
            bExit =ST_SetLdcOnOff();
        }
        else if(u32Select == 11)
        {
            bExit = TRUE;
        }

        usleep(100 * 1000);
    }

    usleep(100 * 1000);

    for(i=0; i< ST_MAX_STREAM_NUM; i++)
    {
        void *retarg = NULL;
        pthread_cancel(gstVpeChnattr.stVpePortAttr[i].pGetDatathread);
        pthread_join(gstVpeChnattr.stVpePortAttr[i].pGetDatathread, &retarg);
    }

    for(i=0; i< ST_MAX_STREAM_NUM; i++)
    {
        pthread_mutex_destroy(&gstVpeChnattr.stVpePortAttr[i].Portmutex);
    }

    if(gbPreviewByVenc == TRUE)
    {
        ST_RtspServerStop();
        STCHECKRESULT(ST_VencStop());
    }

    STCHECKRESULT(ST_BaseModuleUnInit());

    memset(&gstVpeChnattr, 0x0, sizeof(gstVpeChnattr));
    memset(&gstVencattr, 0x0, sizeof(ST_VencAttr_t)*ST_MAX_STREAM_NUM);

    return 0;
}

