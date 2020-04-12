#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include <map>
#include <string>

#include "mi_venc.h"
#include "mi_common.h"

#include "st_common.h"

#include "rtsp.h"

#include "BasicUsageEnvironment.hh"
#include "liveMedia.hh"
#include "Live555RTSPServer.hh"

#include "onvif_server.h"

#define RTSP_LISTEN_PORT        554
#define BUF_POOL_SIZE 2048 * 1024 // 2mb

std::map<std::string, stRtspInputInfo_t> Rtsp::mRtspInputInfo;
std::map<MI_U32, stRtspDataPackageHead_t> Rtsp::mapVencPackage;
stRtspDataMutexCond_t Rtsp::stDataMuxCond;

void Rtsp::DataReceiver(void *pData, unsigned int dataSize, void *pUsrData)
{
    stRtspDataPackage_t stRtspDataPackage;
    std::map<MI_U32, stRtspDataPackageHead_t>::iterator iter;
    MI_U32 u32ChnId = (MI_U32)pUsrData;
    MI_VENC_Stream_t *pstStream = (MI_VENC_Stream_t *)pData;
    MI_U32 u32Len = 0;

    assert(sizeof(MI_VENC_Stream_t) == dataSize);
    memset(&stRtspDataPackage, 0, sizeof(stRtspDataPackage_t));
    for (MI_U8 i = 0; i < pstStream->u32PackCount; i++)
    {
        ASSERT(pstStream->pstPack[i].u32Len);
        stRtspDataPackage.u32DataLen += pstStream->pstPack[i].u32Len;
    }
    stRtspDataPackage.pDataAddr = stRtspDataPackage.pDataAddrStart = stRtspDataPackage.pDataAddrAlign = (void *)malloc(stRtspDataPackage.u32DataLen);
    ASSERT(stRtspDataPackage.pDataAddr);
    for (MI_U8 i = 0; i < pstStream->u32PackCount; i++)
    {
        memcpy((char *)stRtspDataPackage.pDataAddrStart + u32Len, pstStream->pstPack[i].pu8Addr, pstStream->pstPack[i].u32Len);
        u32Len += pstStream->pstPack[i].u32Len;
    }
    //printf("Channel id %d addr %p size %d\n", u32ChnId, stRtspDataPackage.pDataAddr, stRtspDataPackage.u32DataLen);
    pthread_mutex_lock(&stDataMuxCond.mutex);
    iter = mapVencPackage.find(u32ChnId);
    if(iter == mapVencPackage.end())
    {
        printf("Error!!!! venc chn %d not open!!!\n", u32ChnId);
        free(stRtspDataPackage.pDataAddr);
        stRtspDataPackage.pDataAddr = stRtspDataPackage.pDataAddrStart = stRtspDataPackage.pDataAddrAlign = NULL;
        pthread_mutex_unlock(&stDataMuxCond.mutex);

        return;
    }
    if (iter->second.totalsize > BUF_POOL_SIZE)
    {
        printf("Error!!!! buf pool full!!!!!\n");
        free(stRtspDataPackage.pDataAddr);
        stRtspDataPackage.pDataAddr = stRtspDataPackage.pDataAddrStart = stRtspDataPackage.pDataAddrAlign = NULL;
        pthread_mutex_unlock(&stDataMuxCond.mutex);

        return;
    }
    iter->second.package.push_back(stRtspDataPackage);
    iter->second.totalsize += stRtspDataPackage.u32DataLen;
    PTH_RET_CHK(pthread_cond_signal(&stDataMuxCond.cond));
    pthread_mutex_unlock(&stDataMuxCond.mutex);

}
MI_S32 Rtsp::TermBufPool(void)
{
    stRtspDataPackage_t stRtspDataPackage;
    std::map<MI_U32, stRtspDataPackageHead_t>::iterator iter;
    std::map<std::string, stRtspInputInfo_t>::iterator itRtspInputInfo;

    for(itRtspInputInfo = mRtspInputInfo.begin(); itRtspInputInfo != mRtspInputInfo.end(); itRtspInputInfo++)
    {
        pthread_mutex_lock(&stDataMuxCond.mutex);
        iter = mapVencPackage.find(itRtspInputInfo->second.uintPreModChnId);
        if(iter != mapVencPackage.end())
        {
            memset(&stRtspDataPackage, 0, sizeof(stRtspDataPackage_t));
            stRtspDataPackage.bExit = TRUE;
            iter->second.package.push_back(stRtspDataPackage);
            PTH_RET_CHK(pthread_cond_signal(&stDataMuxCond.cond));
        }
        pthread_mutex_unlock(&stDataMuxCond.mutex);

    }

    return MI_SUCCESS;

    return 0;
}

MI_BOOL Rtsp::BufPoolEmptyAndWait(void)
{
    std::map<MI_U32, stRtspDataPackageHead_t>::iterator iter;
    std::map<std::string, stRtspInputInfo_t>::iterator itRtspInputInfo;

    pthread_mutex_lock(&stDataMuxCond.mutex);
    for(itRtspInputInfo = mRtspInputInfo.begin(); itRtspInputInfo != mRtspInputInfo.end(); itRtspInputInfo++)
    {
        iter = mapVencPackage.find((MI_VENC_CHN)itRtspInputInfo->second.uintPreModChnId);
        if(iter != mapVencPackage.end())
        {
            if (iter->second.package.size())
            {
                pthread_mutex_unlock(&stDataMuxCond.mutex);

                return FALSE;
            }
        }

    }
    pthread_cond_wait(&stDataMuxCond.cond, &stDataMuxCond.mutex);
    pthread_mutex_unlock(&stDataMuxCond.mutex);

    return TRUE;
}
MI_S32 Rtsp::OpenBufPool(MI_VENC_CHN stVencChn)
{

    pthread_mutex_lock(&stDataMuxCond.mutex);
    ASSERT(mapVencPackage[stVencChn].package.size() == 0);
    mapVencPackage[stVencChn].totalsize = 0;
    pthread_mutex_unlock(&stDataMuxCond.mutex);

    return MI_SUCCESS;

}
MI_S32 Rtsp::CloseBufPool(MI_VENC_CHN stVencChn)
{
    std::map<MI_U32, stRtspDataPackageHead_t>::iterator iter;
    std::vector<stRtspDataPackage_t>::iterator it;

    pthread_mutex_lock(&stDataMuxCond.mutex);
    iter = mapVencPackage.find((MI_U32)stVencChn);
    if(iter != mapVencPackage.end())
    {
        for (it = iter->second.package.begin(); it != iter->second.package.end(); ++it)
        {
            if (!it->bExit)
                continue;

            free(it->pDataAddr);
            iter->second.totalsize -= it->u32DataLen;
        }
        ASSERT(iter->second.totalsize == 0);
        iter->second.package.clear();
        mapVencPackage.erase(iter);
    }
    pthread_mutex_unlock(&stDataMuxCond.mutex);

    return MI_SUCCESS;
}
MI_S32 Rtsp::DequeueBufPool(MI_VENC_CHN stVencChn, void *pData, MI_U32 u32Size)
{
    MI_U32 sizeRet = 0;
    char * pCharAddr = NULL;
    std::map<MI_U32, stRtspDataPackageHead_t>::iterator iter;
    std::vector<stRtspDataPackage_t>::iterator it;

    BufPoolEmptyAndWait();
    pthread_mutex_lock(&stDataMuxCond.mutex);
    iter = mapVencPackage.find((MI_U32)stVencChn);
    if(iter != mapVencPackage.end())
    {
        pCharAddr = (char *)pData;
        if (iter->second.totalsize < u32Size)
        {
            for (it = iter->second.package.begin(); it != iter->second.package.end(); it = iter->second.package.begin())
            {
                if (!it->bExit)
                {
                    memcpy(pCharAddr, it->pDataAddrStart, it->u32DataLen);
                    pCharAddr += it->u32DataLen;
                    sizeRet += it->u32DataLen;
                    //printf("size %d data_length %d Free %p total %d package cnt %d\n", size, it->u32DataLen, it->pDataAddr, iter->second.totalsize, iter->second.package.size());
                    free(it->pDataAddr);
                    iter->second.totalsize -= it->u32DataLen;
                    iter->second.package.erase(it);
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            printf("Data too large! limitation %d, total %d\n", u32Size, iter->second.totalsize);
            for (it = iter->second.package.begin(); it != iter->second.package.end(); ++it)
            {
                if (it->bExit)
                    continue;
            
                free(it->pDataAddr);
                iter->second.totalsize -= it->u32DataLen;
            }
            ASSERT(iter->second.totalsize == 0);
            iter->second.package.clear();
            sizeRet = 0;
        }
    }
    pthread_mutex_unlock(&stDataMuxCond.mutex);

    return sizeRet;

}
MI_S32 Rtsp::FlushBufPool(MI_VENC_CHN stVencChn)
{
    std::map<MI_U32, stRtspDataPackageHead_t>::iterator iter;
    std::vector<stRtspDataPackage_t>::iterator it;

    iter = mapVencPackage.find((MI_U32)stVencChn);
    if(iter != mapVencPackage.end())
    {
        pthread_mutex_lock(&stDataMuxCond.mutex);
        for (it = iter->second.package.begin(); it != iter->second.package.end(); ++it)
        {
            if (it->bExit)
                continue;

            free(it->pDataAddr);
            iter->second.totalsize -= it->u32DataLen;
        }
        ASSERT(iter->second.totalsize == 0);
        iter->second.package.clear();
        pthread_mutex_unlock(&stDataMuxCond.mutex);
    }

    return MI_SUCCESS;
}
MI_S32 Rtsp::GetDataDirect(MI_VENC_CHN stVencChn, void *pData, MI_U32 u32Maxlen)
{
    MI_U32 u32Size = 0;
    MI_VENC_Stream_t stStream;
    MI_VENC_Pack_t stPack[16];
    MI_VENC_ChnStat_t stStat;
    MI_S32 s32Fd;
    MI_S32 s32Ret = MI_SUCCESS;
    struct timeval tv;
    fd_set read_fds;
    MI_U32 u32Len = 0;
    MI_U8 i = 0;

    s32Fd = MI_VENC_GetFd((MI_VENC_CHN)stVencChn);
    if (s32Fd < 0)
        return 0;

    FD_ZERO(&read_fds);
    FD_SET(s32Fd, &read_fds);

    tv.tv_sec = 0;
    tv.tv_usec = 10 * 1000;

    s32Ret = select(s32Fd + 1, &read_fds, NULL, NULL, &tv);
    if (s32Ret < 0)
    {
        goto RET;
    }
    else if (0 == s32Ret)
    {
        goto RET;
    }
    else
    {
        memset(&stStream, 0, sizeof(stStream));
        memset(stPack, 0, sizeof(stPack));
        stStream.pstPack = stPack;
        s32Ret = MI_VENC_Query(stVencChn, &stStat);
        if(s32Ret != MI_SUCCESS || stStat.u32CurPacks == 0)
        {
            return -1;
        }
        stStream.u32PackCount = stStat.u32CurPacks;
        s32Ret = MI_VENC_GetStream(stVencChn, &stStream, 40);
        if(MI_SUCCESS == s32Ret)
        {
            for (i = 0; i < stStream.u32PackCount; i++)
            {
                u32Size += stStream.pstPack[i].u32Len;
            }
            if (u32Size < u32Maxlen)
            {
                for (i = 0; i < stStream.u32PackCount; i++)
                {
                    memcpy((char *)pData + u32Len, stStream.pstPack[i].pu8Addr, stStream.pstPack[i].u32Len);
                    u32Len += stStream.pstPack[i].u32Len;
                }
                //printf("Get stream size %d addr %p\n", stStream.pstPack[0].u32Len, stRtspDataPackage.pDataAddr);
            }
            else
            {
                printf("Data too large! limitation %d, cur %d\n", u32Maxlen, u32Size);
            }
            MI_VENC_ReleaseStream(stVencChn, &stStream);
        }
    }
 RET:
    MI_VENC_CloseFd(s32Fd);

    return u32Size;
}

Rtsp::Rtsp()
{
    pRTSPServer = NULL;
    bOpenOnvif = FALSE;
}
Rtsp::~Rtsp()
{
}

void Rtsp::Init()
{
    int iRet = 0;
    unsigned int rtspServerPortNum = RTSP_LISTEN_PORT;

    bOpenOnvif = (unsigned char)GetIniUnsignedInt(stModDesc.modKeyString, "ONVIF");
    PTH_RET_CHK(pthread_mutex_init(&stDataMuxCond.mutex, NULL));
    PTH_RET_CHK(pthread_condattr_init(&stDataMuxCond.condattr));
    PTH_RET_CHK(pthread_condattr_setclock(&stDataMuxCond.condattr, CLOCK_MONOTONIC));
    PTH_RET_CHK(pthread_cond_init(&stDataMuxCond.cond, &stDataMuxCond.condattr));
    pRTSPServer = new Live555RTSPServer();
    if(pRTSPServer == NULL)
    {
        assert(0);
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
            return;
        }
        iRet = pRTSPServer->SetRTSPServerPort(rtspServerPortNum);
    }
}

void* Rtsp::OpenStream(char const * szStreamName, void * arg)
{
    stRtspInputInfo_t *pInfo = NULL;

    pInfo = &mRtspInputInfo[szStreamName];
    if (pInfo->intUseBulPool)
    {
        dynamic_cast<Rtsp *>(pInfo->pInstance)->StartReceiver(pInfo->uintCurInPortId);
    }
    ST_DBG("open stream \"%s\" success, chn:%d\n", szStreamName, (MI_VENC_CHN)pInfo->uintPreModChnId);

    return pInfo;
}

int Rtsp::ReadStream(void *handle, unsigned char *ucpBuf, int BufLen, struct timeval *p_Timestamp, void *arg)
{
    stRtspInputInfo_t *pInfo = (stRtspInputInfo_t *)handle;

    if (pInfo->intUseBulPool)
        return (int)DequeueBufPool((MI_VENC_CHN)pInfo->uintPreModChnId, (void *)ucpBuf, BufLen);
    else
        return (int)GetDataDirect((MI_VENC_CHN)pInfo->uintPreModChnId, (void *)ucpBuf, BufLen);

    return -1;
}

int Rtsp::CloseStream(void *handle, void *arg)
{
    stRtspInputInfo_t *pInfo = (stRtspInputInfo_t *)handle;

    ST_DBG("close \"%d\" success\n", pInfo->uintPreModChnId);

    if (pInfo->intUseBulPool)
    {
        dynamic_cast<Rtsp *>(pInfo->pInstance)->StopReceiver(pInfo->uintCurInPortId);
        FlushBufPool((MI_VENC_CHN)pInfo->uintPreModChnId);
    }

    return 0;
}
void Rtsp::BindBlock(stModInputInfo_t & stIn)
{
    stModDesc_t stPreDesc;
    stRtspInputInfo_t stRtspInputInfo;
    ServerMediaSession *mediaSession = NULL;
    ServerMediaSubsession *subSession = NULL;
    std::string strStrName;
    char *urlPrefix = NULL;

    GetInstance(stIn.stPrev.modKeyString)->GetModDesc(stPreDesc);
    stRtspInputInfo.uintPreModChnId = stPreDesc.chnId;
    stRtspInputInfo.intUseBulPool = GetIniInt(stIn.curIoKeyString, "IS_USE_BUF_POOL");
    stRtspInputInfo.uintCurInPortId = stIn.curPortId;
    stRtspInputInfo.pInstance = this;
    if (stRtspInputInfo.intUseBulPool)
    {
        OpenBufPool((MI_VENC_CHN)stRtspInputInfo.uintPreModChnId);
        CreateReceiver(stIn.curPortId, DataReceiver, NULL, NULL, (void *)stRtspInputInfo.uintPreModChnId);
    }
    strStrName = GetIniString(stIn.curIoKeyString, "STREAM_NAME");
    urlPrefix = pRTSPServer->rtspURLPrefix();
    printf("=================URL===================\n");
    printf("%s%s\n", urlPrefix, strStrName.c_str());
    printf("=================URL===================\n");

    stRtspInputInfo.intEncodeType = GetIniInt(stIn.stPrev.modKeyString, "EN_TYPE");
    pRTSPServer->createServerMediaSession(mediaSession,
                                          strStrName.c_str(),
                                          NULL, NULL);

    //printf("Bind!! Cur %s modid %d chn %d dev %d port %d fps %d\n", stIn.curIoKeyString.c_str(), stModDesc.modId, stModDesc.chnId, stModDesc.devId, stIn.curPortId, stIn.curFrmRate);
    //printf("Pre %s modid %d chn %d dev %d port %d fps %d\n", stIn.stPrev.modKeyString.c_str(), stPreDesc.modId, stPreDesc.chnId, stPreDesc.devId, stIn.stPrev.portId, stIn.stPrev.frmRate);

    switch (stRtspInputInfo.intEncodeType)
    {
        case E_MI_VENC_MODTYPE_H264E:
        {
            subSession = WW_H264VideoFileServerMediaSubsession::createNew(
                             *(pRTSPServer->GetUsageEnvironmentObj()),
                             strStrName.c_str(),
                             OpenStream,
                             ReadStream,
                             CloseStream, stIn.curFrmRate);
        }
        break;
        case E_MI_VENC_MODTYPE_H265E:
        {
            subSession = WW_H265VideoFileServerMediaSubsession::createNew(
                             *(pRTSPServer->GetUsageEnvironmentObj()),
                             strStrName.c_str(),
                             OpenStream,
                             ReadStream,
                             CloseStream, stIn.curFrmRate);
        }
        break;
        case E_MI_VENC_MODTYPE_JPEGE:
        {
            subSession = WW_JPEGVideoFileServerMediaSubsession::createNew(
                             *(pRTSPServer->GetUsageEnvironmentObj()),
                             strStrName.c_str(),
                             OpenStream,
                             ReadStream,
                             CloseStream, stIn.curFrmRate);
        }
        break;
        default:
            assert(0);
    }
    pRTSPServer->addSubsession(mediaSession, subSession);
    pRTSPServer->addServerMediaSession(mediaSession);
    mRtspInputInfo[strStrName] = stRtspInputInfo;
}

void Rtsp::Start()
{
#if 0
    std::map<unsigned int, stModOutputInfo_t>::iterator itMapOutput;
    std::vector<stModIoInfo_t>::iterator itVectIo;

    printf("======Mod %s dump in!======\n", stModDesc.modKeyString.c_str());
    for (itMapOutput = mapModOutputInfo.begin(); itMapOutput != mapModOutputInfo.end(); ++itMapOutput)
    {
        printf("OutKey %s port %d frm %d\n", itMapOutput->second.curIoKeyString.c_str(), itMapOutput->second.curPortId, itMapOutput->second.curFrmRate);
        for (itVectIo = itMapOutput->second.vectNext.begin(); itVectIo != itMapOutput->second.vectNext.end(); ++itVectIo)
        {
            printf("Next mod %s port %d frm %d\n", itVectIo->modKeyString.c_str(), itVectIo->portId, itVectIo->frmRate);
        }
    }
    printf("======Mod %s dump out!=====\n", stModDesc.modKeyString.c_str());
#endif
    pRTSPServer->Start();
    if (bOpenOnvif)
    {
        MST_ONVIF_Init();
        MST_ONVIF_StartTask();
    }
}
void Rtsp::Stop()
{
    if (bOpenOnvif)
    {
        MST_ONVIF_StopTask();
    }
    TermBufPool();
    if(pRTSPServer)
    {
        pRTSPServer->Join();
        delete pRTSPServer;
        pRTSPServer = NULL;
    }
}
void Rtsp::UnBindBlock(stModInputInfo_t &stIn)
{
    std::string strStrName;
    stRtspInputInfo_t stRtspInputInfo;
    stModDesc_t stPreDesc;

    GetInstance(stIn.stPrev.modKeyString)->GetModDesc(stPreDesc);
    strStrName = GetIniString(stIn.curIoKeyString, "STREAM_NAME");
    stRtspInputInfo = mRtspInputInfo[strStrName];
    if (stRtspInputInfo.intUseBulPool)
    {
        DestroyReceiver(stIn.curPortId);
        CloseBufPool((MI_VENC_CHN)stRtspInputInfo.uintPreModChnId);
    }

    //printf("UnBind!! Cur %s modid %d chn %d dev %d port %d fps %d\n", stIn.curIoKeyString.c_str(), stModDesc.modId, stModDesc.chnId, stModDesc.devId, stIn.curPortId, stIn.curFrmRate);
    //printf("Pre %s modid %d chn %d dev %d port %d fps %d\n", stIn.stPrev.modKeyString.c_str(), stPreDesc.modId, stPreDesc.chnId, stPreDesc.devId, stIn.stPrev.portId, stIn.stPrev.frmRate);
}

void Rtsp::Deinit()
{
    PTH_RET_CHK(pthread_condattr_destroy(&stDataMuxCond.condattr));
    PTH_RET_CHK(pthread_cond_destroy(&stDataMuxCond.cond));
    PTH_RET_CHK(pthread_mutex_destroy(&stDataMuxCond.mutex));
    mRtspInputInfo.clear();
}

