/*
 * mid_fdfr.cpp- Sigmastar
 *
 * Copyright (C) 2018 Sigmastar Technology Corp.
 *
 * Author: XXXX <XXXX@sigmastar.com.cn>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <poll.h>

#include "mid_utils.h"
#include "mid_common.h"
#include "module_common.h"
//#include "ssnn.h"
#include "mid_dla.h"
#include "mi_common_datatype.h"
#include "mi_sys_datatype.h"
#include "mi_divp_datatype.h"
#include "mi_divp.h"

#if TARGET_CHIP_I6E
#include "mi_ipu.h"
#include "mi_ipu_datatype.h"
#include "mid_ipu_classify.h"
#include "mid_ipu_detect.h"
#include "mid_ipu_fdfr.h"
#include "mid_ipu_hc.h"
#include "mid_ipu_debug.h"
#endif

#include "mid_system_config.h"
#include "mid_VideoEncoder.h"

BOOL dla_init = false;
static IPU_InitInfo_S tmpIPU_InitInfo_S;
BOOL g_DlaExit = FALSE;
static BOOL initDlaInfo = false;

#ifndef BUILD_UCLIBC_PROG

#if TARGET_CHIP_I5
#include "yolov2_offline.h"
#include "tem.h"
#define GET_TIME_DIFF(x, rec) do {   \
    unsigned int Oldtime = GetCurTime();   \
    rec = x;  \
    printf("FUNC %s, RUN %d ms\n", #x, GetCurTime() - Oldtime);   \
}while (0)

#define GET_TIME_DIFF_VOID(x) do {   \
    unsigned int Oldtime = GetCurTime();   \
    x;  \
    printf("FUNC %s, RUN %d ms\n", #x, GetCurTime() - Oldtime);   \
}while (0)

extern MI_S32 g_ieWidth;
extern MI_S32 g_ieHeight;
extern int g_ieFrameInterval;
extern  pthread_mutex_t g_mutex_UpadteOsdState;  
extern  pthread_cond_t  g_cond_UpadteOsdState;  
extern BOOL g_UpadteOsdState;

static pthread_t g_pthreadDla;
extern DLAtoRECT(MI_VENC_CHN s32VencChn, int recCnt, ST_DlaRectInfo_T* pRecInfo, MI_BOOL bShow, MI_BOOL bShowBorder);
BOOL GetDlaState()
{
   return !g_DlaExit;
}
BOOL isDlaInit()
{
  return dla_init;
}
static unsigned int GetCurTime(void)
{
    struct timespec ts;
    unsigned int ms;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ms = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
    if(ms == 0)
    {
        ms = 1;
    }
    return ms;
}

static yolov2_postProcess *g_postProcess = NULL;
static void *mluOutputCpuPtr = NULL;
static unsigned int g_out_n = 0;
static unsigned int g_out_c = 0;
static unsigned int g_out_h = 0;
static unsigned int g_out_w = 0;

#define DLA_MODEL_FILE      "/customer/yolov2_rgba.cambricon"
#define DLA_LABEL_FILE      "/customer/label_map.txt"

ST_DLAInfo_T gstDLAInfo;

void *ST_DLAProc(void *args)
{
    ST_DLAInfo_T *pstDLAInfo = &gstDLAInfo;
    MI_U32 u32Width = 0, u32Height = 0;
    MI_DIVP_OutputPortAttr_t stDivpOutputPortAttr;
    MI_DIVP_CHN divpChn = 0;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE stBufHandle;
    struct pollfd stFd;
    void *pData = NULL;
    bool new_detect_avail = false;
    MI_U32 u32GetTime = 0;

    if(0 != access(DLA_MODEL_FILE, F_OK) || 0 != access(DLA_LABEL_FILE, F_OK))
    {
        ST_ERR("szModelFile:%s or szLabelFile:%s not exist!!!\n", pstDLAInfo->szModelFile, pstDLAInfo->szLabelFile);
        return NULL;
    }

    yolov2_detect dlaDetect(pstDLAInfo->szModelFile, pstDLAInfo->szLabelFile);

    ST_DBG("szModelFile:%s, szLabelFile:%s\n", pstDLAInfo->szModelFile, pstDLAInfo->szLabelFile);
    u32Width = dlaDetect.getInputWidth();
    u32Height = dlaDetect.getInputHeight();

    g_postProcess->setInputSize(u32Width, u32Height);
    //get output shape
    dlaDetect.getOutputShape(&g_out_n, &g_out_c, &g_out_h, &g_out_w);

    ST_DBG("u32Width:%d, u32Height:%d\n", u32Width, u32Height);

    divpChn = pstDLAInfo->divpChn;
    memset(&stDivpOutputPortAttr, 0, sizeof(MI_DIVP_OutputPortAttr_t));

    s32Ret = MI_DIVP_GetOutputPortAttr(divpChn, &stDivpOutputPortAttr);

    if(MI_SUCCESS != s32Ret)
    {
        ST_ERR("MI_DIVP_GetOutputPortAttr err:%x, chn:%d\n", s32Ret, divpChn);
        return NULL;
    }

    if(pstDLAInfo->enPixelFormat != stDivpOutputPortAttr.ePixelFormat ||
            u32Width != stDivpOutputPortAttr.u32Width ||
            u32Height != stDivpOutputPortAttr.u32Height)
    {
        ST_INFO("reset divp output size from %dx%d to %dx%d\n",
                stDivpOutputPortAttr.u32Width, stDivpOutputPortAttr.u32Height, u32Width, u32Height);

        stDivpOutputPortAttr.u32Width = u32Width;
        stDivpOutputPortAttr.u32Height = u32Height;

        s32Ret = MI_DIVP_SetOutputPortAttr(divpChn, &stDivpOutputPortAttr);

        if(MI_SUCCESS != s32Ret)
        {
            ST_ERR("MI_DIVP_SetOutputPortAttr err:%x, chn:%d\n", s32Ret, divpChn);
            return NULL;
        }
    }

    memset(&stChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = divpChn;
    stChnPort.u32PortId = 0;
    stFd.events = POLLIN | POLLERR;
    s32Ret = MI_SYS_GetFd(&stChnPort, &stFd.fd);
    if(MI_SUCCESS != s32Ret)
    {
        ST_ERR("MI_SYS_GetFd err:%x, chn:%d\n", s32Ret, divpChn);
        return NULL;
    }

    s32Ret = MI_SYS_SetChnOutputPortDepth(&stChnPort, 2, 3);
    if(MI_SUCCESS != s32Ret)
    {
        ST_ERR("MI_SYS_SetChnOutputPortDepth err:%x, chn:%d\n", s32Ret, divpChn);
        return NULL;
    }
    while(!g_DlaExit)
    {
        s32Ret = poll(&stFd, 1, 200);
        if(s32Ret < 0)
        {
            printf("poll error!\n");
            continue;
        }
        else if(s32Ret == 0)
        {
            printf("Get fd time out!\n");
            continue;
        }
        else
        {
            if((stFd.revents & POLLIN) != POLLIN)
            {
                printf("error !\n");
                continue;
            }
        }
        s32Ret = MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &stBufHandle);
        if(MI_SUCCESS != s32Ret)
        {
            printf("MI_SYS_ChnOutputPortGetBuf fail!\n");
            continue;
        }
        switch(stBufInfo.eBufType)
        {
            case E_MI_SYS_BUFDATA_FRAME:
                {
                    pData = (void *)(stBufInfo.stFrameData.phyAddr[0]);

                    if(pData)
                    {
                        dlaDetect.setInputPhyAddr(pData);
                        dlaDetect.do_detect();
                        dlaDetect.syncDetectFinish();
                        dlaDetect.copyDetectResult2Host();
                        mluOutputCpuPtr = dlaDetect.getDetectData();
                        if(mluOutputCpuPtr)
                        {
                            g_postProcess->setProcessData(mluOutputCpuPtr, g_out_n, g_out_c, g_out_h, g_out_w);
                            mluOutputCpuPtr = NULL;
                            new_detect_avail = true;
                        }
                        else
                        {
                            new_detect_avail = false;
                        }
                        if(new_detect_avail)
                        {
                            g_postProcess->do_postProcess();
                            int in_w = g_postProcess->getInputWidth();
                            int in_h = g_postProcess->getInputHeight();
                            unsigned int i = 0;
                            std::vector<Rectangle> rects;
                            Rectangle *pstRes;
                            ST_TEM_USER_DATA stUsrData;

                            {
                                g_postProcess->get_process_Rectangle(rects, in_w, in_h);
                                if (rects.size())
                                {
                                    pstRes = (Rectangle *)malloc(rects.size() * sizeof(Rectangle));
                                    ASSERT(pstRes);
                                    memset(pstRes, 0, rects.size() * sizeof(Rectangle));
                                    for (i = 0; i < rects.size(); i++)
                                    {
                                        memcpy(&pstRes[i], &rects[i], sizeof(Rectangle));
                                    }

                                    stUsrData.pUserData = pstRes;
                                    stUsrData.u32UserDataSize = rects.size() * sizeof(Rectangle);
                                    TemSend("dla_get_res", stUsrData);
                                    free(pstRes);
                                    u32GetTime = GetCurTime();
                                }
                                else
                                {
                                    if (GetCurTime() - u32GetTime > 500)
                                    {
                                        stUsrData.pUserData = NULL;
                                        stUsrData.u32UserDataSize = 0;
                                        TemSend("dla_get_res", stUsrData);
                                    }
                                }
                            }
                        }
                    }
                }
                break;
            default:
                {
                    ST_DBG("eBufType:%d\n", (int)stBufInfo.eBufType);
                }
        }
        s32Ret = MI_SYS_ChnOutputPortPutBuf(stBufHandle);
        if(MI_SUCCESS != s32Ret)
        {
            ST_ERR("MI_SYS_ChnOutputPortPutBuf err, %x\n", s32Ret);
        }
    }

    return NULL;
}


void ST_DLADrawResult_Process(ST_DLAInfo_T *pstDLAInfo, Rectangle *pArraRes, int resCnt)
{
    int i;
    ST_DlaRectInfo_T stDlaRectInfo[MAX_DLA_RECT_NUMBER];

    for(i=0; i<resCnt; i++)
    {
        if(i >= MAX_DLA_RECT_NUMBER) break;
        //        printf("dlaDesult idx=%d,x=%d, y=%d, w=%d, h=%d, objname=%s\n",i,
        //            pArraRes[i].leftTop.x,      pArraRes[i].leftTop.y,
        //            pArraRes[i].rightBottom.x,  pArraRes[i].rightBottom.y,
        //            pArraRes[i].szObjName);
        stDlaRectInfo[i].rect.u32X = pArraRes[i].leftTop.x;
        stDlaRectInfo[i].rect.u32Y = pArraRes[i].leftTop.y;
        stDlaRectInfo[i].rect.u16PicW = pArraRes[i].rightBottom.x - pArraRes[i].leftTop.x;
        stDlaRectInfo[i].rect.u16PicH = pArraRes[i].rightBottom.y - pArraRes[i].leftTop.y;
        memset(stDlaRectInfo[i].szObjName, 0, sizeof(stDlaRectInfo[i].szObjName));
        strcpy(stDlaRectInfo[i].szObjName, pArraRes[i].szObjName);
    }
    DLAtoRECT(0, resCnt, stDlaRectInfo, TRUE, TRUE);
}

static void * ST_DLAGetResult(ST_TEM_BUFFER stTemBuf, ST_TEM_USER_DATA stUsrData, pthread_mutex_t *pMutex)
{
    ST_DLAInfo_T *pDlaInfo = (ST_DLAInfo_T *)stTemBuf.pTemBuffer;
    Rectangle *pstRes = (Rectangle *)stUsrData.pUserData;
    int resCnt = stUsrData.u32UserDataSize / sizeof(Rectangle);

    ASSERT(pDlaInfo);

    //GET_TIME_DIFF_VOID(ST_DLADrawResult_Process(pDlaInfo, pstRes, resCnt));
    ST_DLADrawResult_Process(pDlaInfo, pstRes, resCnt);

    return NULL;
}

void* mid_dla_Task(void *argu)
{
    ST_TEM_ATTR stAttr;
    pthread_attr_t stSigMonThreadAttr;

    memset(&gstDLAInfo, 0, sizeof(ST_DLAInfo_T));
    snprintf(gstDLAInfo.szModelFile, sizeof(gstDLAInfo.szModelFile) - 1, "%s", DLA_MODEL_FILE);
    snprintf(gstDLAInfo.szLabelFile, sizeof(gstDLAInfo.szLabelFile) - 1, "%s", DLA_LABEL_FILE);
    gstDLAInfo.divpChn = MIXER_DIVP_CHNID_FOR_DLA;
    gstDLAInfo.enPixelFormat = E_MI_SYS_PIXEL_FRAME_ABGR8888;

    g_postProcess = new yolov2_postProcess(gstDLAInfo.szLabelFile);

    pthread_attr_init(&stSigMonThreadAttr);
    memset(&stAttr, 0, sizeof(ST_TEM_ATTR));
    stAttr.fpThreadDoSignal = ST_DLAGetResult;
    stAttr.fpThreadWaitTimeOut = NULL;
    stAttr.thread_attr = stSigMonThreadAttr;
    stAttr.u32ThreadTimeoutMs = 0xFFFFFFFF;
    stAttr.bSignalResetTimer = 0;
    stAttr.stTemBuf.pTemBuffer = (void *)&gstDLAInfo;
    stAttr.stTemBuf.u32TemBufferSize = 0;
    TemOpen("dla_get_res", stAttr);

    return ST_DLAProc(argu);
}

int mid_dla_Initial(int param)
{
    g_DlaExit = FALSE;
    pthread_create(&g_pthreadDla, NULL, mid_dla_Task, (void *)param);
    pthread_setname_np(g_pthreadDla , "Dla_Task");
    return 0;
}

int mid_dla_Uninitial(void)
{
    g_DlaExit = TRUE;
    pthread_join(g_pthreadDla, NULL);
    TemClose("dla_get_res");
    delete g_postProcess;
    printf("[%s] thread join-\n", __FUNCTION__);
    return 0;
}

#elif TARGET_CHIP_I6E

extern int DLAtoRECT(MI_VENC_CHN s32VencChn, int recCnt, ST_DlaRectInfo_T* pRecInfo, MI_BOOL bShow, MI_BOOL bShowBorder);

void *Monitor(void * argc);

#define IPU_PRIMMA_SIZE (2 * 1024 * 1024)

CDlaManager * CDlaManager::instance()
{
    static CDlaManager _instance;

    return &_instance;
}

CDlaManager::CDlaManager()
{
    memset(&m_stInitInfo, 0, sizeof(IPU_InitInfo_S));
    m_bRun             = FALSE;
    m_IPUInterface    = NULL;
    m_bInit            = FALSE;
	dla_init = m_bInit;
}

CDlaManager::~CDlaManager()
{
}
MI_S32 CDlaManager::Init()
{
    switch (m_stInitInfo.enModelType)
    {
        case Model_Type_Classify:
        {
            m_IPUInterface = new CMidIPUClassify(m_stInitInfo);
        }
        break;

        case Model_Type_Detect:
        {
            m_IPUInterface = new CMidIPUDetect(m_stInitInfo);
        }
        break;

        case Model_Type_FaceReg:
        {
            m_IPUInterface = new CMidIPUFdFr(m_stInitInfo);
        }
        break;
        case Model_Type_Hc:
        {
            m_IPUInterface = new CMidIPUHc(m_stInitInfo);
        }
        break;
        case Model_Type_Debug:
        {
            m_IPUInterface = new CMidIPUDebug(m_stInitInfo);
        }
        break;
        default:
        {
            m_IPUInterface = NULL;
        }
        break;
    }

    if (m_IPUInterface == NULL)
    {
        // MIXER_ERR("invalid model type, %d\n", (int)m_stInitInfo.enModelType);
        return -1;
    }

    m_bRun = TRUE;
	pthread_attr_t attr; 
	MI_S32 policy = 0;
	pthread_attr_init (&attr);
	policy = SCHED_FIFO;
    Mixer_set_thread_policy(&attr,policy);
	struct sched_param s_parm;
    s_parm.sched_priority = 80;//sched_get_priority_max(SCHED_FIFO);
    pthread_attr_setschedparam(&attr, &s_parm);
	policy = Mixer_get_thread_policy(&attr);
    Mixer_show_thread_priority(&attr, policy);
    Mixer_get_thread_priority(&attr);
    pthread_create(&m_pthread, NULL, Monitor, (void *)this);
    pthread_setname_np(m_pthread , "Dla_Task");

    m_bInit = TRUE;
    dla_init = m_bInit;
    return MI_SUCCESS;
}
MI_S32 CDlaManager::UnInit()
{
    if (m_IPUInterface == NULL)
    {
        return MI_SUCCESS;
    }

    m_bRun = FALSE;
    pthread_join(m_pthread, NULL);

    delete m_IPUInterface;
    m_IPUInterface = NULL;

    m_bInit = FALSE;
	dla_init = m_bInit;
    return MI_SUCCESS;
}

MI_S32 CDlaManager::Start()
{
    return MI_SUCCESS;
}

MI_S32 CDlaManager::Stop()
{
    return MI_SUCCESS;
}

MI_S32 CDlaManager::SetInitInfo(const IPU_InitInfo_S *pstInitInfo)
{
    if (pstInitInfo == NULL)
    {
        return -1;
    }

    memcpy(&m_stInitInfo, pstInitInfo, sizeof(IPU_InitInfo_S));
    memcpy(&tmpIPU_InitInfo_S, pstInitInfo, sizeof(IPU_InitInfo_S));
    MIXER_DBG("ipu info....\n");
    printf("model type:%s\n", (m_stInitInfo.enModelType == Model_Type_Classify) ? "object classify" :
                                (m_stInitInfo.enModelType == Model_Type_Detect) ? "object detect" :
                            (m_stInitInfo.enModelType == Model_Type_FaceReg) ? "Face recognition" : \
                            (m_stInitInfo.enModelType == Model_Type_Hc)  ?  "HC recognition" : "invalid model type");

    printf("frameware:%s\n", m_stInitInfo.szIPUfirmware);
    printf("model file:%s\n", m_stInitInfo.szModelFile);
    printf("extend info:\n");
    if (m_stInitInfo.enModelType == Model_Type_Classify ||
        m_stInitInfo.enModelType == Model_Type_Detect)
    {
        printf("    label file:%s\n", m_stInitInfo.u.ExtendInfo1.szLabelFile);
    }
    else if (m_stInitInfo.enModelType == Model_Type_FaceReg)
    {
        printf("    model file1:%s\n", m_stInitInfo.u.ExtendInfo2.szModelFile1);
        printf("    face db file:%s\n", m_stInitInfo.u.ExtendInfo2.szFaceDBFile);
        printf("    name list file:%s\n", m_stInitInfo.u.ExtendInfo2.szNameListFile);
    }
    initDlaInfo = TRUE;
    if (m_bInit)
    {
        UnInit();
    }

    Init();
    
    return MI_SUCCESS;
}

void CDlaManager::Monitor1()
{
    while (m_bRun)
    {
        if (m_IPUInterface)
        {
            m_IPUInterface->DealDataProcess();
        }
    }
}

void *Monitor(void * argc)
{
    CDlaManager *pDlaManager = (CDlaManager *)argc;

    pDlaManager->Monitor1();

    return NULL;
}

int mid_dla_Initial(int param)
{
#if 0
    if(MI_SUCCESS != g_CDlaManager->Init())
    {
        MIXER_ERR("dla manager is err\n");
        return -1;
    }

    if(MI_SUCCESS !=g_CDlaManager->Start())
    {
        MIXER_ERR("dla start is err\n");
        return -1;
    }
#endif
    if(initDlaInfo)
   	{
      g_CDlaManager->SetInitInfo(&tmpIPU_InitInfo_S);
   	}
    else
    {
        MIXER_DBG("mid_dla_Initial DlaInfo is not init\n");
    }
    return 0;
}


int mid_dla_Uninitial(void)
{
    g_CDlaManager->Stop();
    g_CDlaManager->UnInit();

    MIXER_DBG("[%s] thread join-\n", __FUNCTION__);

    return 0;
}

int mid_dla_SetParam(const MI_S8* param)
{
    if(NULL == param)
    {
        MIXER_ERR("[%s] param err.\n", __FUNCTION__);
        return -1;
    }
    IeParamInfo tmp;

    memcpy(&tmp, param, sizeof(IeParamInfo));

    MIXER_DBG("id:%d, new add name:%s, new del name:%s\n", tmp.box_id, tmp.NewAddName, tmp.NewDelName);

    g_CDlaManager->m_IPUInterface->SetIeParam(tmp,0);

    return MI_SUCCESS;
}

int mid_dla_SetInitInfo(const MI_S8* param)
{
    if(NULL == param)
    {
        MIXER_ERR("[%s] param err.\n", __FUNCTION__);
        return -1;
    }

    g_CDlaManager->SetInitInfo((IPU_InitInfo_S *)param);

    return MI_SUCCESS;
}

#else

int mid_dla_Initial(int param)
{
    printf("[%s] Only 326D/329Q/520D support dla\n", __FUNCTION__);
    return 0;
}

int mid_dla_Uninitial(void)
{
    printf("[%s] Only 326D/329Q/520D support dla\n", __FUNCTION__);
    return 0;
}

#endif

#else

int mid_dla_Initial(int param)
{
    printf("[%s] Only 326D/329Q/520D support dla\n", __FUNCTION__);
    return 0;
}

int mid_dla_Uninitial(void)
{
    printf("[%s] Only 326D/329Q/520D support dla\n", __FUNCTION__);
    return 0;
}

#endif

