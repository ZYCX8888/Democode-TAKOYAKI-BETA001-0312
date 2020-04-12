#include <stdio.h>
#include <string.h>

#include "mi_common.h"
#include "mi_sys.h"
#include "mi_venc.h"

#include "st_common.h"

#include "sys.h"


std::map<std::string, Sys *> Sys::connectMap;
std::map<std::string, unsigned int> Sys::connectIdMap;
std::vector<Sys *> Sys::connectOrder;
dictionary * Sys::m_pstDict = NULL;

int Sys::GetIniInt(std::string section, std::string key)
{
    std::string strTmp;

    if (!m_pstDict)
    {
        printf("INI file not found!\n");
        assert(NULL);
    }
    strTmp = section + ':' + key;

    return iniparser_getint(m_pstDict, strTmp.c_str(), -1);
}
unsigned int Sys::GetIniUnsignedInt(std::string section, std::string key)
{
    std::string strTmp;

    if (!m_pstDict)
    {
        printf("INI file not found!\n");
        assert(NULL);
    }
    strTmp = section + ':' + key;

    return iniparser_getunsignedint(m_pstDict, strTmp.c_str(), -1);
}
char* Sys::GetIniString(std::string section, std::string key)
{
    std::string strTmp;

    if (!m_pstDict)
    {
        printf("INI file not found!\n");
        assert(NULL);
    }
    strTmp = section + ':' + key;

    return iniparser_getstring(m_pstDict, strTmp.c_str(), NULL);
}
void Sys::InitSys(std::string strIniPath, std::map<std::string, unsigned int> &mapModId)
{
    if (!m_pstDict)
    {
        m_pstDict = iniparser_load(strIniPath.c_str());
    }
    connectIdMap = mapModId;
    CreateConnection();
}
void Sys::DeinitSys()
{
    DestroyConnection();
    if (m_pstDict)
    {
        iniparser_freedict(m_pstDict);
    }
}

void Sys::CreateConnection()
{
    std::map<unsigned int, std::string>::iterator it;
    std::map<unsigned int, std::string> mapRootKeyStr;
    std::map<unsigned int, stModInputInfo_t>::iterator itMapIn;
    int rootCnt;
    char root[30];
    std::string rootName;
    int i = 0;
    Sys *pClass = NULL;

    printf("%d\n",iniparser_getint(m_pstDict, "ROOT:COUNT", -1));
    rootCnt = iniparser_getint(m_pstDict, "ROOT:COUNT", -1);
    for(i = 0; i < rootCnt; i++)
    {
        sprintf(root,"ROOT:NAME_%d",i);
        rootName = iniparser_getstring(m_pstDict, root, NULL);
        printf("%s\n",rootName.c_str());
        mapRootKeyStr[i] = rootName;
    }
    ST_Sys_Init();
    for (it = mapRootKeyStr.begin(); it != mapRootKeyStr.end(); ++it)
    {
        Implement(it->second);
    }

    //init
    for (i = connectOrder.size(); i != 0; i--)
    {
        pClass = connectOrder[i - 1];
        pClass->Init();
    }

    //bind
    for (i = connectOrder.size(); i != 0; i--)
    {
        pClass = connectOrder[i - 1];
        for (itMapIn = pClass->mapModInputInfo.begin(); itMapIn != pClass->mapModInputInfo.end(); ++itMapIn)
        {
            pClass->BindBlock(itMapIn->second);
        }
    }

    //start
    for (i = connectOrder.size(); i != 0; i--)
    {
        pClass = connectOrder[i - 1];
        pClass->Start();
    }
}
void Sys::DestroyConnection(void)
{
    Sys *pClass = NULL;
    std::map<unsigned int, stModInputInfo_t>::iterator itMapIn;
    unsigned int i = 0;

    //stop
    for (i = 0; i < connectOrder.size(); i++)
    {
        pClass = connectOrder[i];
        pClass->Stop();
    }

    //unbind
    for (i = 0; i < connectOrder.size(); i++)
    {
        pClass = connectOrder[i];
        for (itMapIn = pClass->mapModInputInfo.begin(); itMapIn != pClass->mapModInputInfo.end(); ++itMapIn)
        {
            pClass->UnBindBlock(itMapIn->second);
        }
    }

    //deinit
    for (i = 0; i < connectOrder.size(); i++)
    {
        pClass = connectOrder[i];
        pClass->Deinit();
        pClass->mapModInputInfo.clear();
        pClass->mapModOutputInfo.clear();
        delete (pClass);
        connectOrder[i] = NULL;
    }
    connectMap.clear();
    connectOrder.clear();
    ST_Sys_Exit();
}
void Sys::SetCurInfo(std::string &strKey)
{
    unsigned int inCnt = 0;
    unsigned int outCnt = 0;
    int tmpPos = 0;
    char tmpStr[20];
    char *pRes = NULL;
    unsigned int i = 0;
    std::string strTempString;
    std::string strTempPrevPort;
    stModInputInfo_t stInputInfo;
    stModOutputInfo_t stOutputInfo;

    inCnt = GetIniUnsignedInt(strKey, "IN_CNT");
    outCnt = GetIniUnsignedInt(strKey, "OUT_CNT");
    stModDesc.modKeyString = strKey;
    stModDesc.modId = FindBlockId(strKey);
    stModDesc.chnId = GetIniUnsignedInt(strKey, "CHN");
    stModDesc.devId = GetIniUnsignedInt(strKey, "DEV");
    //printf("modKeyString %s mod %d chn %d dev %d\n", stModDesc.modKeyString.c_str(), stModDesc.modId, stModDesc.chnId, stModDesc.devId);
    if (inCnt)
    {
        for (i = 0; i < inCnt; i++)
        {
            memset(tmpStr, 0, 20);
            sprintf(tmpStr, "IN_%d", i);
            pRes = GetIniString(strKey, tmpStr);
            stInputInfo.curIoKeyString = pRes;
            //printf("inKeyStr %s\n", stInputInfo.curIoKeyString.c_str());
            strTempString = GetIniString(pRes, "PREV");
            tmpPos = strTempString.find_first_of(':');
            strTempPrevPort = strTempString;
            strTempPrevPort.erase(0, tmpPos + 1);
            stInputInfo.stPrev.modKeyString = strTempString.erase(tmpPos, strTempString.size() - tmpPos);
            //printf("preModKeyString %s inPrePortStr %s\n", stInputInfo.stPrev.modKeyString.c_str(), strTempPrevPort.c_str());
            strTempString.erase();
            strTempString = GetIniString(stInputInfo.stPrev.modKeyString, strTempPrevPort.c_str());
            stInputInfo.stPrev.portId = GetIniUnsignedInt(strTempString, "PORT");
            stInputInfo.stPrev.frmRate = GetIniUnsignedInt(strTempString, "FPS");
            stInputInfo.curPortId = GetIniUnsignedInt(pRes, "PORT");
            stInputInfo.curFrmRate = GetIniUnsignedInt(pRes, "FPS");
            //printf("prePort %d preModFr %d port %d Fr %d\n", stInputInfo.stPrev.portId, stInputInfo.stPrev.frmRate, stInputInfo.curPortId, stInputInfo.curFrmRate);
            mapModInputInfo[stInputInfo.curPortId] = stInputInfo;
        }
    }
    if (outCnt)
    {
        for (i = 0; i < outCnt; i++)
        {
            memset(tmpStr, 0, 20);
            sprintf(tmpStr, "OUT_%d", i);
            pRes = GetIniString(strKey, tmpStr);
            stOutputInfo.curIoKeyString = pRes;
            stOutputInfo.curPortId = GetIniUnsignedInt(pRes, "PORT");
            stOutputInfo.curFrmRate = GetIniUnsignedInt(pRes, "FPS");
            mapModOutputInfo[stOutputInfo.curPortId] = stOutputInfo;
        }
    }
    connectMap[stModDesc.modKeyString] = this;
    connectOrder.push_back(this);

    return;
}
void Sys::BuildModTree()
{
    Sys *pPreClass;
    stModIoInfo_t stIoInfo;
    std::map<unsigned int, stModInputInfo_t>::iterator itMapInput;

    for (itMapInput = mapModInputInfo.begin(); itMapInput != mapModInputInfo.end(); ++itMapInput)
    {
        Implement(itMapInput->second.stPrev.modKeyString);
        pPreClass =  GetInstance(itMapInput->second.stPrev.modKeyString);
        if (pPreClass)
        {
            stIoInfo.modKeyString = stModDesc.modKeyString;
            stIoInfo.portId = itMapInput->second.curPortId;
            stIoInfo.frmRate = itMapInput->second.curFrmRate;
            pPreClass->mapModOutputInfo[itMapInput->second.stPrev.portId].vectNext.push_back(stIoInfo);
        }
    }
}
void Sys::Start()
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
}
void Sys::Stop()
{
    //printf("Mod %s stop!\n", stModDesc.modKeyString.c_str());
}
void Sys::BindBlock(stModInputInfo_t &stIn)
{
    stModDesc_t stPreDesc;
    ST_Sys_BindInfo_T stBindInfo;

    GetInstance(stIn.stPrev.modKeyString)->GetModDesc(stPreDesc);
    //printf("Bind!! Cur %s modid %d chn %d dev %d port %d fps %d\n", stIn.curIoKeyString.c_str(), stModDesc.modId, stModDesc.chnId, stModDesc.devId, stIn.curPortId, stIn.curFrmRate);
    //printf("Pre %s modid %d chn %d dev %d port %d fps %d\n", stIn.stPrev.modKeyString.c_str(), stPreDesc.modId, stPreDesc.chnId, stPreDesc.devId, stIn.stPrev.portId, stIn.stPrev.frmRate);
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.eBindType = (MI_SYS_BindType_e)GetIniInt(stIn.curIoKeyString, "BIND_TYPE");
    stBindInfo.u32BindParam = GetIniInt(stIn.curIoKeyString, "BIND_PARAM");
    stBindInfo.stSrcChnPort.eModId = (MI_ModuleId_e)stPreDesc.modId ;
    stBindInfo.stSrcChnPort.u32DevId = stPreDesc.devId;
    stBindInfo.stSrcChnPort.u32ChnId = stPreDesc.chnId;
    stBindInfo.stSrcChnPort.u32PortId = stIn.stPrev.portId;
    stBindInfo.u32SrcFrmrate = stIn.stPrev.frmRate;
    stBindInfo.stDstChnPort.eModId = (MI_ModuleId_e)stModDesc.modId;
    stBindInfo.stDstChnPort.u32DevId = stModDesc.devId;
    stBindInfo.stDstChnPort.u32ChnId = stModDesc.chnId;
    stBindInfo.stDstChnPort.u32PortId = stIn.curPortId;
    stBindInfo.u32DstFrmrate = stIn.curFrmRate;
    ST_Sys_Bind(&stBindInfo);
}
void Sys::UnBindBlock(stModInputInfo_t &stIn)
{
    stModDesc_t stPreDesc;
    ST_Sys_BindInfo_T stBindInfo;

    GetInstance(stIn.stPrev.modKeyString)->GetModDesc(stPreDesc);
    //printf("UnBind!! Cur %s modid %d chn %d dev %d port %d fps %d\n", stIn.curIoKeyString.c_str(), stModDesc.modId, stModDesc.chnId, stModDesc.devId, stIn.curPortId, stIn.curFrmRate);
    //printf("Pre %s modid %d chn %d dev %d port %d fps %d\n", stIn.stPrev.modKeyString.c_str(), stPreDesc.modId, stPreDesc.chnId, stPreDesc.devId, stIn.stPrev.portId, stIn.stPrev.frmRate);
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = (MI_ModuleId_e)stPreDesc.modId ;
    stBindInfo.stSrcChnPort.u32DevId = stPreDesc.devId;
    stBindInfo.stSrcChnPort.u32ChnId = stPreDesc.chnId;
    stBindInfo.stSrcChnPort.u32PortId = stIn.stPrev.portId;
    stBindInfo.u32SrcFrmrate = stIn.stPrev.frmRate;
    stBindInfo.stDstChnPort.eModId = (MI_ModuleId_e)stModDesc.modId;
    stBindInfo.stDstChnPort.u32DevId = stModDesc.devId;
    stBindInfo.stDstChnPort.u32ChnId = stModDesc.chnId;
    stBindInfo.stDstChnPort.u32PortId = stIn.curPortId;
    stBindInfo.u32DstFrmrate = stIn.curFrmRate;
    ST_Sys_UnBind(&stBindInfo);
}

typedef struct stSenderState_s
{
    E_SENDER_STATE eState;
    void *pData;
}stSenderState_t;
int Sys::CreateSender(unsigned int outPortId)
{
    ST_TEM_ATTR stTemAttr;
    stReceiverDesc_t stDest;

    PTH_RET_CHK(pthread_attr_init(&stTemAttr.thread_attr));
    memset(&stTemAttr, 0, sizeof(ST_TEM_ATTR));
    stTemAttr.fpThreadDoSignal = SenderState;
    stTemAttr.fpThreadWaitTimeOut = SenderMonitor;
    stTemAttr.u32ThreadTimeoutMs = 10;
    stTemAttr.bSignalResetTimer = 0;
    stTemAttr.stTemBuf.pTemBuffer = (void *)&(mapRecevier[outPortId]);
    stTemAttr.stTemBuf.u32TemBufferSize = 0;
    TemOpen(mapModOutputInfo[outPortId].curIoKeyString.c_str(), stTemAttr);

    return 0;
}
int Sys::StartSender(unsigned int outPortId, stReceiverPortDesc_t &stRecvPortDesc)
{
    ST_TEM_USER_DATA stUsrData;
    stSenderState_t stState;
    stState.eState = E_SENDER_STATE_START;
    stState.pData = (void *)&stRecvPortDesc;

    stUsrData.pUserData = &stState;
    stUsrData.u32UserDataSize = sizeof(stSenderState_t);
    TemSend(mapModOutputInfo[outPortId].curIoKeyString.c_str(), stUsrData);
    if (mapRecevier[outPortId].uintRefsCnt == 0)
    {
        TemStartMonitor(mapModOutputInfo[outPortId].curIoKeyString.c_str());
    }
    mapRecevier[outPortId].uintRefsCnt++;

    return 0;
}
int Sys::StopSender(unsigned int outPortId, stReceiverPortDesc_t &stRecvPortDesc)
{
    ST_TEM_USER_DATA stUsrData;
    stSenderState_t stState;
    stState.eState = E_SENDER_STATE_STOP;
    stState.pData = (void *)&stRecvPortDesc;

    if (mapRecevier[outPortId].uintRefsCnt)
        mapRecevier[outPortId].uintRefsCnt--;
    if (mapRecevier[outPortId].uintRefsCnt == 0)
    {
        TemStop(mapModOutputInfo[outPortId].curIoKeyString.c_str());
    }
    stUsrData.pUserData = &stState;
    stUsrData.u32UserDataSize = sizeof(stSenderState_t);
    TemSend(mapModOutputInfo[outPortId].curIoKeyString.c_str(), stUsrData);

    return 0;
}
int Sys::DestroySender(unsigned int outPortId)
{
    TemClose(mapModOutputInfo[outPortId].curIoKeyString.c_str());

    return 0;
}
int Sys::State(unsigned int outPortId, E_SENDER_STATE eState, stReceiverPortDesc_t &stRecPortDesc)
{
    pthread_mutex_lock(&mapRecevier[outPortId].stDeliveryMutex);
    if (stRecPortDesc.bStart)
    {
        switch (eState)
        {
            case E_SENDER_STATE_START:
            {
                if (stRecPortDesc.fpStateStart)
                    stRecPortDesc.fpStateStart(stRecPortDesc.pUsrData);
            }
            break;
            case E_SENDER_STATE_STOP:
            {
                if (stRecPortDesc.fpStateStop)
                    stRecPortDesc.fpStateStop(stRecPortDesc.pUsrData);
            }
            break;
            default:
                assert(0);
        }
    }
    pthread_mutex_unlock(&mapRecevier[outPortId].stDeliveryMutex);

    return 0;
}
int Sys::Send(unsigned int outPortId, void *pData, unsigned int intDataSize)
{
    std::map<std::string, stReceiverPortDesc_t>::iterator it;
    std::map<std::string, stReceiverPortDesc_t> *pMap = &mapRecevier[outPortId].mapPortDesc;

    pthread_mutex_lock(&mapRecevier[outPortId].stDeliveryMutex);
    for (it = pMap->begin(); it != pMap->end(); ++it)
    {
        if (it->second.bStart)
        {
            it->second.fpRec(pData, intDataSize, it->second.pUsrData);
        }
    }
    pthread_mutex_unlock(&mapRecevier[outPortId].stDeliveryMutex);

    return 0;
}
void *Sys::SenderState(ST_TEM_BUFFER stBuf, ST_TEM_USER_DATA stUsrData)
{
    stSenderState_t stSenderState;
    stReceiverDesc_t *pReceiver = (stReceiverDesc_t *)stBuf.pTemBuffer;
    Sys *pClass = pReceiver->pSysClass;

    assert(stUsrData.pUserData);
    assert(stUsrData.u32UserDataSize == sizeof(stSenderState_t));
    stSenderState = *((stSenderState_t *)stUsrData.pUserData);
    assert(stSenderState.pData);
    pClass->State(pReceiver->uintPort, stSenderState.eState, *((stReceiverPortDesc_t *)stSenderState.pData));

    return NULL;
}

void * Sys::SenderMonitor(ST_TEM_BUFFER stBuf)
{
    MI_SYS_ChnPort_t stChnOutputPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_S32 s32Fd;
    MI_S32 s32Ret;
    fd_set read_fds;
    struct timeval tv;
    MI_VENC_Stream_t stStream;
    MI_VENC_Pack_t stPack[16];
    MI_VENC_ChnStat_t stStat;
    stReceiverDesc_t *pReceiver = (stReceiverDesc_t *)stBuf.pTemBuffer;
    Sys *pClass = pReceiver->pSysClass;

    stChnOutputPort.eModId = (MI_ModuleId_e)pClass->stModDesc.modId;
    stChnOutputPort.u32DevId = (MI_U32)pClass->stModDesc.devId;
    stChnOutputPort.u32ChnId = (MI_U32)pClass->stModDesc.chnId;
    stChnOutputPort.u32PortId = (MI_U32)pReceiver->uintPort;
    switch (pClass->stModDesc.modId)
    {
        case E_SYS_MOD_VPE:
        case E_SYS_MOD_DIVP:
        {
            s32Ret = MI_SYS_GetFd(&stChnOutputPort, &s32Fd);
            if (s32Ret < 0)
                return NULL;

            FD_ZERO(&read_fds);
            FD_SET(s32Fd, &read_fds);
            tv.tv_sec = 0;
            tv.tv_usec = 10 * 1000;
            s32Ret = select(s32Fd + 1, &read_fds, NULL, NULL, &tv);
            if (s32Ret < 0)
            {
                MI_SYS_CloseFd(s32Fd);

                return NULL;
            }
            else if (0 == s32Ret)
            {
                MI_SYS_CloseFd(s32Fd);

                return NULL;
            }
            if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stChnOutputPort , &stBufInfo,&hHandle))
            {
                pClass->Send(pReceiver->uintPort, &stBufInfo, sizeof(MI_SYS_BufInfo_t));
                MI_SYS_ChnOutputPortPutBuf(hHandle);
            }
            MI_SYS_CloseFd(s32Fd);
        }
        break;
        case E_SYS_MOD_VENC:
        {
            s32Fd = MI_VENC_GetFd((MI_VENC_CHN)stChnOutputPort.u32ChnId);
            if (s32Fd < 0)
                return 0;

            FD_ZERO(&read_fds);
            FD_SET(s32Fd, &read_fds);
            tv.tv_sec = 0;
            tv.tv_usec = 10 * 1000;
            s32Ret = select(s32Fd + 1, &read_fds, NULL, NULL, &tv);
            if (s32Ret < 0)
            {
                MI_VENC_CloseFd(s32Fd);

                return NULL;
            }
            else if (0 == s32Ret)
            {
                MI_VENC_CloseFd(s32Fd);

                return NULL;
            }
            else
            {
                memset(&stStream, 0, sizeof(stStream));
                memset(stPack, 0, sizeof(stPack));
                stStream.pstPack = stPack;
                s32Ret = MI_VENC_Query(stChnOutputPort.u32ChnId, &stStat);
                if(s32Ret != MI_SUCCESS || stStat.u32CurPacks == 0)
                {
                    MI_VENC_CloseFd(s32Fd);

                    return NULL;
                }
                stStream.u32PackCount = stStat.u32CurPacks;
                s32Ret = MI_VENC_GetStream(stChnOutputPort.u32ChnId, &stStream, 40);
                if(MI_SUCCESS == s32Ret)
                {
                    //printf("Receiver %p Get venc chn %d and send to port %d this is %s\n", pReceiver, stChnOutputPort.u32ChnId, pReceiver->uintPort, pClass->stModDesc.modKeyString.c_str());
                    pClass->Send(pReceiver->uintPort, &stStream, sizeof(MI_VENC_Stream_t));
                    MI_VENC_ReleaseStream(stChnOutputPort.u32ChnId, &stStream);
                }
            }
            MI_VENC_CloseFd(s32Fd);
        }
        break;
        default:
            printf("Do not support this mod %d now\n", pClass->stModDesc.modId);
            break;
    }
    return NULL;
}

int Sys::CreateReceiver(unsigned int inPortId, DeliveryRecFp funcRecFp, DeliveryState funcStart, DeliveryState funcStop, void *pUsrData)
{
    Sys *pPrevClass = NULL;
    unsigned int intPrevOutPort = 0;
    stReceiverDesc_t stReceiverDesc;
    stReceiverPortDesc_t stReceiverPortDesc;

    if (!funcRecFp)
    {
        printf("funcRecFp is null!\n");

        return -1;
    }
    if (mapModInputInfo.find(inPortId) == mapModInputInfo.end())
    {
        printf("Can not find input port %d\n", inPortId);

        return -1;
    }
    pPrevClass = GetInstance(mapModInputInfo[inPortId].stPrev.modKeyString);
    if (!pPrevClass)
    {
        printf("Prev class is null!\n");

        return -1;
    }
    intPrevOutPort = mapModInputInfo[inPortId].stPrev.portId;
    stReceiverPortDesc.bStart = FALSE;
    stReceiverPortDesc.fpRec = funcRecFp;
    stReceiverPortDesc.fpStateStart = funcStart;
    stReceiverPortDesc.fpStateStop = funcStop;
    stReceiverPortDesc.pUsrData = pUsrData;
    if (pPrevClass->mapRecevier.find(intPrevOutPort) == pPrevClass->mapRecevier.end())
    {
        stReceiverDesc.mapPortDesc[mapModInputInfo[inPortId].curIoKeyString] = stReceiverPortDesc;
        stReceiverDesc.pSysClass = pPrevClass;
        stReceiverDesc.uintPort = intPrevOutPort;
        stReceiverDesc.stDeliveryMutex = PTHREAD_MUTEX_INITIALIZER;
        stReceiverDesc.uintRefsCnt = 0;
        pPrevClass->mapRecevier[intPrevOutPort] = stReceiverDesc;
        pPrevClass->CreateSender(intPrevOutPort);
    }
    else
    {
        pthread_mutex_lock(&pPrevClass->mapRecevier[intPrevOutPort].stDeliveryMutex);
        pPrevClass->mapRecevier[intPrevOutPort].mapPortDesc[mapModInputInfo[inPortId].curIoKeyString] = stReceiverPortDesc;
        pthread_mutex_unlock(&pPrevClass->mapRecevier[intPrevOutPort].stDeliveryMutex);
    }

    return 0;
}
int Sys::StartReceiver(unsigned int inPortId)
{
    Sys *pPrevClass = NULL;
    unsigned int intPrevOutPort = 0;

    if (mapModInputInfo.find(inPortId) == mapModInputInfo.end())
    {
        printf("Can not find input port %d\n", inPortId);

        return -1;
    }
    pPrevClass = GetInstance(mapModInputInfo[inPortId].stPrev.modKeyString);
    if (!pPrevClass)
    {
        printf("Prev class is null!\n");

        return -1;
    }
    intPrevOutPort = mapModInputInfo[inPortId].stPrev.portId;
    if (pPrevClass->mapRecevier.find(intPrevOutPort) != pPrevClass->mapRecevier.end())
    {
        pthread_mutex_lock(&pPrevClass->mapRecevier[intPrevOutPort].stDeliveryMutex);
        pPrevClass->mapRecevier[intPrevOutPort].mapPortDesc[mapModInputInfo[inPortId].curIoKeyString].bStart = TRUE;
        pthread_mutex_unlock(&pPrevClass->mapRecevier[intPrevOutPort].stDeliveryMutex);
        pPrevClass->StartSender(intPrevOutPort, pPrevClass->mapRecevier[intPrevOutPort].mapPortDesc[mapModInputInfo[inPortId].curIoKeyString]);
    }
    else
    {
        printf("Receiver did not create. inpot id %d current %s prev %s\n", inPortId, mapModInputInfo[inPortId].curIoKeyString.c_str(), mapModInputInfo[inPortId].stPrev.modKeyString.c_str());

        return -1;
    }

    return 0;
}
int Sys::StopReceiver(unsigned int inPortId)
{
    Sys *pPrevClass = NULL;
    unsigned int intPrevOutPort = 0;
    stReceiverDesc_t stReceiverDesc;

    if (mapModInputInfo.find(inPortId) == mapModInputInfo.end())
    {
        printf("Can not find input port %d\n", inPortId);

        return -1;
    }
    pPrevClass = GetInstance(mapModInputInfo[inPortId].stPrev.modKeyString);
    if (!pPrevClass)
    {
        printf("Prev class is null!\n");

        return -1;
    }
    intPrevOutPort = mapModInputInfo[inPortId].stPrev.portId;
    if (pPrevClass->mapRecevier.find(intPrevOutPort) != pPrevClass->mapRecevier.end())
    {
        pPrevClass->StopSender(intPrevOutPort, pPrevClass->mapRecevier[intPrevOutPort].mapPortDesc[mapModInputInfo[inPortId].curIoKeyString]);
        pthread_mutex_lock(&pPrevClass->mapRecevier[intPrevOutPort].stDeliveryMutex);
        pPrevClass->mapRecevier[intPrevOutPort].mapPortDesc[mapModInputInfo[inPortId].curIoKeyString].bStart = FALSE;
        pthread_mutex_unlock(&pPrevClass->mapRecevier[intPrevOutPort].stDeliveryMutex);
    }
    else
    {
        printf("Receiver did not create. inpot id %d current %s prev %s\n", inPortId, mapModInputInfo[inPortId].curIoKeyString.c_str(), mapModInputInfo[inPortId].stPrev.modKeyString.c_str());

        return -1;
    }

    return 0;
}
int Sys::DestroyReceiver(unsigned int inPortId)
{
    Sys *pPrevClass = NULL;
    unsigned int intPrevOutPort = 0;
    std::map<unsigned int, stReceiverDesc_t>::iterator it;

    if (mapModInputInfo.find(inPortId) == mapModInputInfo.end())
    {
        printf("Can not find input port %d\n", inPortId);

        return -1;
    }
    pPrevClass = GetInstance(mapModInputInfo[inPortId].stPrev.modKeyString);
    if (!pPrevClass)
    {
        printf("Prev class is null!\n");

        return -1;
    }
    intPrevOutPort = mapModInputInfo[inPortId].stPrev.portId;
    if (pPrevClass->mapRecevier.find(intPrevOutPort) != pPrevClass->mapRecevier.end())
    {
        if (pPrevClass->mapRecevier[intPrevOutPort].mapPortDesc.find(mapModInputInfo[inPortId].curIoKeyString) != pPrevClass->mapRecevier[intPrevOutPort].mapPortDesc.end())
        {
            pPrevClass->mapRecevier[intPrevOutPort].mapPortDesc.erase(mapModInputInfo[inPortId].curIoKeyString);
        }
        if (pPrevClass->mapRecevier[intPrevOutPort].mapPortDesc.size() == 0)
        {
            pPrevClass->DestroySender(intPrevOutPort);
            pPrevClass->mapRecevier.erase(intPrevOutPort);
        }
    }

    return 0;
}
