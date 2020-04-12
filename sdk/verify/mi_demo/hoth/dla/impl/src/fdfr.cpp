#include <stdio.h>

#include "mi_common.h"
#include "mi_sys.h"

#include "fdfr.h"


Fdfr::Fdfr()
{
}
Fdfr::~Fdfr()
{
}
void Fdfr::Init()
{
}
void Fdfr::Deinit()
{
}
void Fdfr::Start()
{
    ST_TEM_ATTR stTemAttr;
    PTH_RET_CHK(pthread_attr_init(&stTemAttr.thread_attr));

    memset(&stTemAttr, 0, sizeof(ST_TEM_ATTR));
    stTemAttr.fpThreadDoSignal = NULL;
    stTemAttr.fpThreadWaitTimeOut = DataMonitor;
    stTemAttr.u32ThreadTimeoutMs = 10;
    stTemAttr.bSignalResetTimer = 0;
    stTemAttr.stTemBuf.pTemBuffer = (void *)this;
    stTemAttr.stTemBuf.u32TemBufferSize = 0;
    TemOpen(stModDesc.modKeyString.c_str(), stTemAttr);
    TemStartMonitor(stModDesc.modKeyString.c_str());
}
void Fdfr::Stop()
{
    TemStop(stModDesc.modKeyString.c_str());
    TemClose(stModDesc.modKeyString.c_str());
}
void Fdfr::BindBlock(stModInputInfo_t &stIn)
{
    stFdfrInputInfo_t stFdfrInputInfo;
    stModDesc_t stPreModDesc;
    MI_SYS_ChnPort_t stChnPort;

    memset(&stChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    memset(&stFdfrInputInfo, 0, sizeof(stFdfrInputInfo_t));
    GetInstance(stIn.stPrev.modKeyString)->GetModDesc(stPreModDesc);
    stFdfrInputInfo.intPreChn = (unsigned int)stPreModDesc.chnId;
    stFdfrInputInfo.intPreDev= (unsigned int)stPreModDesc.devId;
    stFdfrInputInfo.intPreMod = (unsigned int)stPreModDesc.modId;
    stFdfrInputInfo.intPreOutPort = (unsigned int)stIn.stPrev.portId;
    stFdfrInputInfo.intInPort = stIn.curPortId;
    vectFdFrInput.push_back(stFdfrInputInfo);
    stChnPort.eModId = (MI_ModuleId_e)stFdfrInputInfo.intPreMod;
    stChnPort.u32ChnId = (MI_U32)stFdfrInputInfo.intPreChn;
    stChnPort.u32PortId = (MI_U32)stFdfrInputInfo.intPreOutPort;
    stChnPort.u32DevId = (MI_U32)stFdfrInputInfo.intPreDev;
    MI_SYS_SetChnOutputPortDepth(&stChnPort, 3, 3);

}
void Fdfr::UnBindBlock(stModInputInfo_t &stIn)
{
    std::vector<stFdfrInputInfo_t>::iterator it;

    for (it = vectFdFrInput.begin(); it != vectFdFrInput.end(); it++)
    {
        if (it->intInPort == stIn.curPortId)
        {
            vectFdFrInput.erase(it);

            break;
        }
    }
}
void Fdfr::DoFd(MI_SYS_BufInfo_t &stBufInfo)
{
    //printf("FD get data: buf type %d format %d addr %llx w %d h %d\n", stBufInfo.eBufType, stBufInfo.stFrameData.ePixelFormat, stBufInfo.stFrameData.phyAddr[0], stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height);
}
#define RAMDOM(start, end) ((end > start)?((rand()%(end - start)) + start):0)
void Fdfr::DoFr(MI_SYS_BufInfo_t &stBufInfo)
{
    stFaceInfo_t stFaceInfo;
    const char *NameList[] = {"Albert", "Nancy", "Peter", "Bryce", "Laura", "Mary", "Sam", "Tommy", "Abel"};
    unsigned int i = 0;
    unsigned int j = 0;

    memset(&stFaceInfo, 0, sizeof(stFaceInfo_t));
    // 1 crop by divp
    //printf("FR get data: buf type %d format %d addr %llx w %d h %d\n", stBufInfo.eBufType, stBufInfo.stFrameData.ePixelFormat, stBufInfo.stFrameData.phyAddr[0], stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height);

    // do recognition

    // insert the result
    for (i = 0; i < sizeof(NameList) / sizeof(char *); i++)
    {
        strcpy(stFaceInfo.faceName, NameList[i]);
        stFaceInfo.faceW = RAMDOM(116, 400);
        stFaceInfo.faceH = RAMDOM(116, 400);
        stFaceInfo.xPos = RAMDOM(0, 1920);
        stFaceInfo.yPos = RAMDOM(0, 1080);
        stFaceInfo.winWid = 1920;
        stFaceInfo.winHei = 1080;
        vectResult.push_back(stFaceInfo);
    }
    for (j = i; j < 15; j++)
    {
        memset(stFaceInfo.faceName, 0, 50);
        stFaceInfo.faceW = RAMDOM(116, 400);
        stFaceInfo.faceH = RAMDOM(116, 400);
        stFaceInfo.xPos = RAMDOM(0, 1920);
        stFaceInfo.yPos = RAMDOM(0, 1080);
        stFaceInfo.winWid = 1920;
        stFaceInfo.winHei = 1080;
        vectResult.push_back(stFaceInfo);
    }
}
void Fdfr::SendResult()
{
    ST_TEM_USER_DATA stUsrData;
    std::map<unsigned int, stModOutputInfo_t>::iterator itFdfrOut;
    std::vector<stFaceInfo_t>::iterator itVectFace;
    char *pTempData = NULL;

    stUsrData.u32UserDataSize = sizeof(stFaceInfo_t) * vectResult.size();
    if (stUsrData.u32UserDataSize)
    {
        stUsrData.pUserData = malloc(stUsrData.u32UserDataSize);
        pTempData = (char *)stUsrData.pUserData;
        assert(stUsrData.pUserData);
        for (itVectFace = vectResult.begin(); itVectFace != vectResult.end(); itVectFace++)
        {
            memcpy(pTempData, &(*itVectFace), sizeof(stFaceInfo_t));
            pTempData += sizeof(stFaceInfo_t);
        }
        for(itFdfrOut = mapModOutputInfo.begin(); itFdfrOut != mapModOutputInfo.end(); itFdfrOut++)
        {
            TemSend(mapModOutputInfo[itFdfrOut->second.curPortId].curIoKeyString.c_str(), stUsrData);
            TemStartOneShot(mapModOutputInfo[itFdfrOut->second.curPortId].curIoKeyString.c_str());
        }
        free(stUsrData.pUserData);
    }
    vectResult.clear();
}
void *Fdfr::DataMonitor(ST_TEM_BUFFER stBuf)
{
    MI_SYS_ChnPort_t stChnOutputPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_S32 s32Fd;
    MI_S32 s32Ret;
    fd_set read_fds;
    struct timeval tv;
    Fdfr *pClass = (Fdfr *)stBuf.pTemBuffer;
    std::vector<stFdfrInputInfo_t>::iterator it;
    std::vector<MI_SYS_BUF_HANDLE> vectHandle;
    std::vector<MI_S32> vectFd;
    std::vector<MI_SYS_BUF_HANDLE>::iterator itHandle;
    std::vector<MI_S32>::iterator itFd;
    MI_SYS_BufInfo_t astBufInfo[2]; //0 : fd, 1 : fr

    memset(astBufInfo, 0, sizeof(MI_SYS_BufInfo_t) * 2);
    for (it = pClass->vectFdFrInput.begin(); it != pClass->vectFdFrInput.end(); it++)
    {
        stChnOutputPort.eModId = (MI_ModuleId_e)it->intPreMod;
        stChnOutputPort.u32ChnId = (MI_U32)it->intPreChn;
        stChnOutputPort.u32PortId = (MI_U32)it->intPreOutPort;
        stChnOutputPort.u32DevId = (MI_U32)it->intPreDev;
        switch (stChnOutputPort.eModId)
        {
            case E_MI_MODULE_ID_VPE:
            case E_MI_MODULE_ID_DIVP:
            {
                s32Ret = MI_SYS_GetFd(&stChnOutputPort, &s32Fd);
                if (s32Ret < 0)
                {
                    goto EXIT;
                }
                vectFd.push_back(s32Fd);
                FD_ZERO(&read_fds);
                FD_SET(s32Fd, &read_fds);
                tv.tv_sec = 0;
                tv.tv_usec = 10 * 1000;
                s32Ret = select(s32Fd + 1, &read_fds, NULL, NULL, &tv);
                if (s32Ret < 0)
                {
                    goto EXIT;
                }
                else if (0 == s32Ret)
                {
                    goto EXIT;
                }
                //Sys get data..
                if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stChnOutputPort, &stBufInfo,&hHandle))
                {
                    if (it->intInPort == 0)
                        astBufInfo[0] = stBufInfo;
                    else if (it->intInPort == 1)
                        astBufInfo[1] = stBufInfo;
                    else
                        assert(0);

                    vectHandle.push_back(hHandle);
                }
                else
                {
                    goto EXIT;
                }
            }
            break;
            default:
                assert(0);
        }
    }
    pClass->DoFd(astBufInfo[0]);
    pClass->DoFr(astBufInfo[1]);
    pClass->SendResult();

EXIT:
    for (itHandle = vectHandle.begin(); itHandle != vectHandle.end(); ++itHandle)
    {
        MI_SYS_ChnOutputPortPutBuf(*itHandle);
    }
    for (itFd = vectFd.begin(); itFd != vectFd.end(); ++itFd)
    {
        MI_SYS_CloseFd(*itFd);
    }
    vectHandle.clear();
    vectFd.clear();

    return NULL;
}

void Fdfr::DataReceiver(void *pData, unsigned int dataSize, void *pUsrData)
{
    MI_SYS_BufInfo_t *pstBufInfo;

    pstBufInfo = (MI_SYS_BufInfo_t *)pData;
    assert(sizeof(MI_SYS_BufInfo_t) == dataSize);
    assert(pstBufInfo->eBufType == E_MI_SYS_BUFDATA_FRAME);

    switch (pstBufInfo->stFrameData.ePixelFormat)
    {
        case E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420:
        {
            //do yuv420 copy...
        }
        break;
        case E_MI_SYS_PIXEL_FRAME_YUV422_YUYV:
        case E_MI_SYS_PIXEL_FRAME_ARGB8888:
        {
        }
        break;
        default:
            printf("Not support!!\n");
            assert(0);
            break;
    }
}
void * Fdfr::SenderSignal(ST_TEM_BUFFER stBuf, ST_TEM_USER_DATA stUsrData)
{
    stReceiverDesc_t *pReceiver = (stReceiverDesc_t *)stBuf.pTemBuffer;
    Fdfr *pClass = dynamic_cast<Fdfr *>(pReceiver->pSysClass);

    pClass->Send(pReceiver->uintPort, stUsrData.pUserData, stUsrData.u32UserDataSize);

    return NULL;
}
void * Fdfr::SenderSignalEnd(ST_TEM_BUFFER stBuf)
{
    stReceiverDesc_t *pReceiver = (stReceiverDesc_t *)stBuf.pTemBuffer;
    Fdfr *pClass = dynamic_cast<Fdfr *>(pReceiver->pSysClass);

    pClass->Send(pReceiver->uintPort, NULL, 0);

    return NULL;
}

int Fdfr::CreateSender(unsigned int outPortId)
{
    ST_TEM_ATTR stTemAttr;
    stReceiverDesc_t stDest;

    PTH_RET_CHK(pthread_attr_init(&stTemAttr.thread_attr));
    memset(&stTemAttr, 0, sizeof(ST_TEM_ATTR));
    stTemAttr.fpThreadDoSignal = SenderSignal;
    stTemAttr.fpThreadWaitTimeOut = SenderSignalEnd;
    stTemAttr.u32ThreadTimeoutMs = 500;
    stTemAttr.bSignalResetTimer = TRUE;
    stTemAttr.stTemBuf.pTemBuffer = (void *)&(mapRecevier[outPortId]);
    stTemAttr.stTemBuf.u32TemBufferSize = 0;
    TemOpen(mapModOutputInfo[outPortId].curIoKeyString.c_str(), stTemAttr);

    return 0;
}
int Fdfr::StartSender(unsigned int outPortId, stReceiverPortDesc_t &stRecvPortDesc)
{
    return 0;
}
int Fdfr::StopSender(unsigned int outPortId, stReceiverPortDesc_t &stRecvPortDesc)
{
    return 0;
}
int Fdfr::DestroySender(unsigned int outPortId)
{
    TemClose(mapModOutputInfo[outPortId].curIoKeyString.c_str());

    return 0;
}

