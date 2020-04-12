#include "mid_ipu_fdfr.h"
#include "mid_system_config.h"
#include "mid_VideoEncoder.h"
#include "mid_dla.h"

#include <math.h>

#define MID_FR_THRESHOLD    (0.0)

#define DIVP_WIDTH_ALIGN    (16)

#if 0
static int g_new_add_userid = -1;
static  char g_new_add_name[256] = {0};
static char g_new_del_name[256] = {0};
#endif
static IeParamInfo  personInfo;
extern int DLAtoRECT(MI_VENC_CHN s32VencChn, int recCnt, ST_DlaRectInfo_T* pRecInfo, MI_BOOL bShow, MI_BOOL bShowBorder);
extern MI_U32 g_rotation;
extern pthread_mutex_t g_mutex_UpadteOsdState;  
extern pthread_cond_t  g_cond_UpadteOsdState;  
extern MI_U32 g_UpadteOsdState;

CMidIPUFdFr::CMidIPUFdFr(IPU_InitInfo_S &stInitInfo) : CMidIPUInterface(stInitInfo)
{
    // fd info
    memset(&m_stFdModel, 0, sizeof(Model_Info_S));
    m_stFdModel.vpeChn            = 0;
    m_stFdModel.vpePort            = 2;
    m_stFdModel.divpChn            = Mixer_Divp_GetChannleNum();
    m_stFdModel.ipuChn            = 0;
    m_stFdModel.u32InBufDepth    = 1;
    m_stFdModel.u32OutBufDepth    = 1;
    m_stFdModel.s32Fd            = -1;
    m_stFdModel.pModelFile        = m_stIPUInfo.szModelFile;
    m_stFdModel.u32Width        = 640;
    m_stFdModel.u32Height        = 360;

    // fr info
    memset(&m_stFrModel, 0, sizeof(Model_Info_S));
    m_stFrModel.vpeChn            = 0;
    m_stFrModel.vpePort            = 2;
    m_stFrModel.divpChn            = Mixer_Divp_GetChannleNum();
    m_stFrModel.ipuChn            = 1;
    m_stFrModel.u32InBufDepth    = 1;
    m_stFrModel.u32OutBufDepth    = 1;
    m_stFrModel.s32Fd            = -1;
    m_stFrModel.pModelFile        = m_stIPUInfo.u.ExtendInfo2.szModelFile1;
    m_stFrModel.u32Width        = 1920;
    m_stFrModel.u32Height        = 1080;

    m_s32MaxFd = -1;
    m_dataReady = 0;
    memset(&personInfo,0,sizeof(IeParamInfo));
    (void)InitResource();
}

CMidIPUFdFr::~CMidIPUFdFr()
{
    Mixer_Divp_PutChannleNum(m_stFdModel.divpChn);
    Mixer_Divp_PutChannleNum(m_stFrModel.divpChn);

    (void)ReleaseResource();
}

int CMidIPUFdFr::InitResource()
{
    MI_S32 s32Ret = MI_SUCCESS;

    if(MI_SUCCESS != IPUCreateDevice(m_stIPUInfo.szIPUfirmware, MAX_VARIABLE_BUF_SIZE))
    {
        MIXER_ERR("IPUCreateDevice error, %s, ret:[0x%x]\n", m_stIPUInfo.szIPUfirmware, s32Ret);
        return -1;
    }

    if (MI_SUCCESS != InitStreamSource(m_stFdModel))
    {
        MIXER_ERR("InitStreamSource for fd error\n");
        IPUDestroyDevice();
        return -1;
    }

    if (MI_SUCCESS != InitStreamSource(m_stFrModel))
    {
        MIXER_ERR("InitStreamSource for fr error\n");
        ReleaseStreamSource(m_stFdModel);
        IPUDestroyDevice();
        return -1;
    }
    LoadFaceDb();
    return MI_SUCCESS;
}

int CMidIPUFdFr::ReleaseResource()
{
    (void)ReleaseStreamSource(m_stFdModel);
    (void)ReleaseStreamSource(m_stFrModel);

    (void)IPUDestroyDevice();

    return MI_SUCCESS;
}

void CMidIPUFdFr::DealDataProcess()
{
#if 0
    fd_set read_fds;
    struct timeval tv;
    MI_S32 s32Ret = 0;
    MI_SYS_ChnPort_t stChnOutputPort;
    MI_IPU_TensorVector_t Vector4Affine;

    FD_ZERO(&read_fds);
    FD_SET(m_stFdModel.s32Fd, &read_fds);
    FD_SET(m_stFrModel.s32Fd, &read_fds);

    tv.tv_sec = 0;
    tv.tv_usec = 100 * 1000;
    s32Ret = select(m_s32MaxFd + 1, &read_fds, NULL, NULL, &tv);
    if (s32Ret < 0)
    {
        // fail
    }
    else if (0 == s32Ret)
    {
        // timeout
    }
    else
    {
        if(FD_ISSET(m_stFdModel.s32Fd, &read_fds))
        {
            stChnOutputPort.eModId       = E_MI_MODULE_ID_DIVP;
            stChnOutputPort.u32DevId     = 0;
            stChnOutputPort.u32ChnId     = m_stFdModel.divpChn;
            stChnOutputPort.u32PortId     = 0;

            memset(&m_stFdModel.stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));

            if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stChnOutputPort, &m_stFdModel.stBufInfo,
                                    &m_stFdModel.stBufHandle))
            {
                m_dataReady |= 0x01;
            }
        }

        if (FD_ISSET(m_stFrModel.s32Fd, &read_fds))
        {
            stChnOutputPort.eModId       = E_MI_MODULE_ID_DIVP;
            stChnOutputPort.u32DevId     = 0;
            stChnOutputPort.u32ChnId     = m_stFrModel.divpChn;
            stChnOutputPort.u32PortId     = 0;

            memset(&m_stFrModel.stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));

            if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stChnOutputPort, &m_stFrModel.stBufInfo,
                                    &m_stFrModel.stBufHandle))
            {
                m_dataReady |= 0x01 << 1;
            }
        }

        if ((m_dataReady & 0x03) == 0x03)
        {
            m_dataReady = 0;

            DoFd(m_stFdModel.stBufInfo, &Vector4Affine);
            DoFr(m_stFrModel.stBufInfo, &Vector4Affine);
            PrintResult();

            MI_SYS_ChnOutputPortPutBuf(m_stFdModel.stBufHandle);
            MI_SYS_ChnOutputPortPutBuf(m_stFrModel.stBufHandle);
        }
    }
#endif
    MI_S32 s32Ret = 0;
    MI_SYS_ChnPort_t stChnOutputPort;
    MI_IPU_TensorVector_t Vector4Affine;
    MI_U32 u32Count = 0;

    // get fd data
    memset(&stChnOutputPort, 0, sizeof(MI_SYS_ChnPort_t));
    stChnOutputPort.eModId       = E_MI_MODULE_ID_DIVP;
    stChnOutputPort.u32DevId     = 0;
    stChnOutputPort.u32ChnId     = m_stFdModel.divpChn;
    stChnOutputPort.u32PortId     = 0;

    do
    {
        memset(&m_stFdModel.stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
        s32Ret = MI_SYS_ChnOutputPortGetBuf(&stChnOutputPort, &m_stFdModel.stBufInfo, &m_stFdModel.stBufHandle);
        if (s32Ret != MI_SUCCESS)
        {
            usleep(10 * 1000);
        }
        else
        {
            m_dataReady |= 0x01;
        }
        u32Count ++;
    } while ((MI_SUCCESS != s32Ret) && (u32Count <= 5));

    // get fr data
    u32Count = 0;
    memset(&stChnOutputPort, 0, sizeof(MI_SYS_ChnPort_t));
    stChnOutputPort.eModId       = E_MI_MODULE_ID_DIVP;
    stChnOutputPort.u32DevId     = 0;
    stChnOutputPort.u32ChnId     = m_stFrModel.divpChn;
    stChnOutputPort.u32PortId     = 0;

    do
    {
        memset(&m_stFrModel.stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
        s32Ret = MI_SYS_ChnOutputPortGetBuf(&stChnOutputPort, &m_stFrModel.stBufInfo, &m_stFrModel.stBufHandle);
        if (s32Ret != MI_SUCCESS)
        {
           // MIXER_DBG("MI_SYS_ChnOutputPortGetBuf is failed [%0x]!\n",s32Ret);
            usleep(10 * 1000);
        }
        else
        {
            m_dataReady |= 0x01 << 1;
        }
        u32Count ++;
    } while ((MI_SUCCESS != s32Ret) && (u32Count <= 5));
    
    if ((m_dataReady & 0x03) == 0x03)
    {
        //struct timeval curStamp, lastStamp;
		//memset(&curStamp,0,sizeof(timeval));
		//memset(&lastStamp,0,sizeof(timeval));
		//gettimeofday(&lastStamp, NULL);
		//MIXER_DBG("last bufTSDiff: %d s  %d us\n", lastStamp.tv_sec,lastStamp.tv_usec);
        DoFd(m_stFdModel.stBufInfo, &Vector4Affine);
        DoFr(m_stFrModel.stBufInfo, &Vector4Affine);
		//gettimeofday(&curStamp, NULL);
		//MIXER_DBG("cur bufTSDiff: %d s  %d us\n", curStamp.tv_sec,curStamp.tv_usec);
        PrintResult();
    }

    if(m_dataReady & 0x01)
    {
        MI_SYS_ChnOutputPortPutBuf(m_stFdModel.stBufHandle);
    }

    if ((m_dataReady >> 1) & 0x01)
    {
        MI_SYS_ChnOutputPortPutBuf(m_stFrModel.stBufHandle);
    }

    m_dataReady = 0;
}

int CMidIPUFdFr::InitStreamSource(Model_Info_S &stModelInfo)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_VPE_PortMode_t stVpeMode;
    MI_SYS_WindowRect_t stCropWin;
    MI_DIVP_OutputPortAttr_t stDivpPortAttr;
    Mixer_Sys_BindInfo_T stBindInfo;
    MI_SYS_ChnPort_t stChnOutputPort;
    MI_U32 u32Temp = 0;

    if(MI_SUCCESS != IPUCreateChannel(&stModelInfo.ipuChn, stModelInfo.pModelFile, stModelInfo.u32InBufDepth,
                                        stModelInfo.u32OutBufDepth))
    {
        MIXER_ERR("IPUCreateChannel error, chn:%d, model:%s, ret:[0x%x]\n", stModelInfo.ipuChn, stModelInfo.pModelFile, s32Ret);
        return -1;
    }

    if (MI_SUCCESS != (s32Ret = MI_IPU_GetInOutTensorDesc(stModelInfo.ipuChn, &stModelInfo.stNPUDesc)))
    {
        MIXER_ERR("MI_IPU_GetInOutTensorDesc error, chn:%d, ret:[0x%x]\n", stModelInfo.ipuChn, s32Ret);
        IPUDestroyChannel(stModelInfo.ipuChn, stModelInfo.u32InBufDepth, stModelInfo.u32OutBufDepth);
        return -1;
    }

    MIXER_DBG("H:%d,W:%d,C:%d,format:%s, u32InputTensorCount:%d, u32OutputTensorCount:%d\n",
                stModelInfo.stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[1],
                stModelInfo.stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[2],
                stModelInfo.stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[3],
                (stModelInfo.stNPUDesc.astMI_InputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_U8) ? "MI_IPU_FORMAT_U8" :
                (stModelInfo.stNPUDesc.astMI_InputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_NV12) ? "MI_IPU_FORMAT_NV12" :
                (stModelInfo.stNPUDesc.astMI_InputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_INT16) ? "MI_IPU_FORMAT_INT16" :
                (stModelInfo.stNPUDesc.astMI_InputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_INT32) ? "MI_IPU_FORMAT_INT32" :
                (stModelInfo.stNPUDesc.astMI_InputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_INT8) ? "MI_IPU_FORMAT_INT8" :
                (stModelInfo.stNPUDesc.astMI_InputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_FP32) ? "MI_IPU_FORMAT_FP32" : "UNKNOWN",
                stModelInfo.stNPUDesc.u32InputTensorCount, stModelInfo.stNPUDesc.u32OutputTensorCount);

    memset(&stVpeMode, 0, sizeof(MI_VPE_PortMode_t));
    ExecFunc(MI_VPE_GetPortMode(stModelInfo.vpeChn, stModelInfo.vpePort, &stVpeMode), MI_VPE_OK);

    stCropWin.u16Width = stVpeMode.u16Width;
    stCropWin.u16Height = stVpeMode.u16Height;
    stCropWin.u16X = 0;
    stCropWin.u16Y = 0;
    MIXERCHECKRESULT(Mixer_Divp_CreatChannel(stModelInfo.divpChn, (MI_SYS_Rotate_e)0x0, &stCropWin));

    if(((g_rotation & 0xFFFF) == 90) || ((g_rotation & 0xFFFF) == 270))
    {
        u32Temp                        = stModelInfo.u32Width;
        stModelInfo.u32Width        = stModelInfo.u32Height;
        stModelInfo.u32Height        = u32Temp;
        stModelInfo.u32Width        = ALIGN_UP(stModelInfo.u32Width, DIVP_WIDTH_ALIGN);

        stDivpPortAttr.u32Width      = stModelInfo.u32Width;
        stDivpPortAttr.u32Height     = stModelInfo.u32Height;
    }
    else
    {
        stModelInfo.u32Width        = ALIGN_UP(stModelInfo.u32Width, DIVP_WIDTH_ALIGN);

        stDivpPortAttr.u32Width      = stModelInfo.u32Width;
        stDivpPortAttr.u32Height     = stModelInfo.u32Height;
    }

    stDivpPortAttr.eCompMode     = E_MI_SYS_COMPRESS_MODE_NONE;
   // stDivpPortAttr.ePixelFormat    = E_MI_SYS_PIXEL_FRAME_ARGB8888;
    stDivpPortAttr.ePixelFormat    = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;

    MIXERCHECKRESULT(Mixer_Divp_SetOutputAttr(stModelInfo.divpChn, &stDivpPortAttr));
    MIXERCHECKRESULT(Mixer_Divp_StartChn(stModelInfo.divpChn));

    memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));

    stBindInfo.stSrcChnPort.eModId    = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId  = 0;
    stBindInfo.stSrcChnPort.u32ChnId  = stModelInfo.vpeChn;
    stBindInfo.stSrcChnPort.u32PortId = stModelInfo.vpePort;

    stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_DIVP;
    stBindInfo.stDstChnPort.u32DevId  = 0;
    stBindInfo.stDstChnPort.u32ChnId  = stModelInfo.divpChn;
    stBindInfo.stDstChnPort.u32PortId = 0;

    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stBindInfo.stDstChnPort, 3, 3), MI_SUCCESS);

    stBindInfo.u32SrcFrmrate = 30; //MI_VideoEncoder::vpeframeRate;
    stBindInfo.u32DstFrmrate = 30; //MI_VideoEncoder::vpeframeRate;
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
#endif
    MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));

    stChnOutputPort.eModId       = E_MI_MODULE_ID_DIVP;
    stChnOutputPort.u32DevId     = 0;
    stChnOutputPort.u32ChnId     = stModelInfo.divpChn;
    stChnOutputPort.u32PortId     = 0;
    s32Ret = MI_SYS_GetFd(&stChnOutputPort, &stModelInfo.s32Fd);
    if (s32Ret < 0)
    {
        MIXER_ERR("divp ch: %d, get fd. err\n", stChnOutputPort.u32ChnId);
        return -1;
    }

    m_s32MaxFd = MAX(m_s32MaxFd, stModelInfo.s32Fd);

    MIXER_DBG("m_s32Fd:%d\n", stModelInfo.s32Fd);

    return MI_SUCCESS;
}

int CMidIPUFdFr::ReleaseStreamSource(Model_Info_S &stModelInfo)
{
    Mixer_Sys_BindInfo_T stBindInfo;

    (void)IPUDestroyChannel(stModelInfo.ipuChn, stModelInfo.u32InBufDepth, stModelInfo.u32OutBufDepth);

    if (stModelInfo.s32Fd > 0)
    {
        MI_SYS_CloseFd(stModelInfo.s32Fd);
        stModelInfo.s32Fd = -1;
    }

    memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));

    stBindInfo.stSrcChnPort.eModId    = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId  = 0;
    stBindInfo.stSrcChnPort.u32ChnId  = stModelInfo.vpeChn;
    stBindInfo.stSrcChnPort.u32PortId = stModelInfo.vpePort;

    stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_DIVP;
    stBindInfo.stDstChnPort.u32DevId  = 0;
    stBindInfo.stDstChnPort.u32ChnId  = stModelInfo.divpChn;
    stBindInfo.stDstChnPort.u32PortId = 0;

    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stBindInfo.stDstChnPort, 3, 3), MI_SUCCESS);

    stBindInfo.u32SrcFrmrate = 30; //MI_VideoEncoder::vpeframeRate;
    stBindInfo.u32DstFrmrate = 15; //MI_VideoEncoder::vpeframeRate;
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
#endif
    MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

    MIXERCHECKRESULT(Mixer_Divp_StopChn(stModelInfo.divpChn));
    MIXERCHECKRESULT(Mixer_Divp_DestroyChn(stModelInfo.divpChn));

    return MI_SUCCESS;
}
MI_S32 CMidIPUFdFr::SetIeParam(IeParamInfo tmp,MI_U8 scop)
{ 
	 personInfo.box_id = tmp.box_id ;
	 if((0 == scop) || (1 == scop))
	 {
	   memcpy(personInfo.NewAddName,tmp.NewAddName,sizeof(personInfo.NewAddName));
	 }
	 if((0 == scop) || (2 == scop))
	 {
		memcpy(personInfo.NewDelName,tmp.NewDelName,sizeof(personInfo.NewDelName));
	 }
     return 0;
}
MI_S32 CMidIPUFdFr::GetIeParam(IeParamInfo &tmp)
{
  tmp.box_id = personInfo.box_id;
  if(strlen(personInfo.NewAddName) && personInfo.box_id)
  {
    memcpy(tmp.NewAddName,personInfo.NewAddName,sizeof(tmp.NewAddName));
	return 1;
  }
  if(strlen(personInfo.NewDelName))
  {
     memcpy(tmp.NewDelName,personInfo.NewDelName,sizeof(tmp.NewDelName));
	 return -1;
  }
  return 0;
}
void CMidIPUFdFr::LoadFaceDb()
{
    MI_S32 s32Num = 0;
    MI_S32 i = 0;

    m_faceDB.LoadFromFileBinay(m_stIPUInfo.u.ExtendInfo2.szFaceDBFile, m_stIPUInfo.u.ExtendInfo2.szNameListFile);
    s32Num = m_faceDB.persons.size();

    MIXER_DBG("%d \n", s32Num);
    for (i = 0; i < s32Num; i++)
    {
        MIXER_DBG("%s \n", m_faceDB.persons[i].name.c_str());
        int feat_num = m_faceDB.persons[i].features.size();
        for (int j = 0; j < feat_num; j++)
        {
            MIXER_DBG("%d %f %f \n", m_faceDB.persons[i].features[j].length, m_faceDB.persons[i].features[j].pData[0],
                                        m_faceDB.persons[i].features[j].pData[1]);
            MIXER_DBG("%s \n", m_faceDB.persons[i].name.c_str());
        }
    }
    MIXER_DBG("%d %f %f \n", m_faceDB.persons[0].features[0].length, m_faceDB.persons[0].features[0].pData[0],
                                m_faceDB.persons[0].features[0].pData[1]);
}

int CMidIPUFdFr::ScaleToModelSize(const MI_SYS_BufInfo_t &stBufInfo, MI_IPU_TensorVector_t* pstInputTensorVector,
                                    Model_Info_S &stModelInfo)
{
    unsigned char *pSrcImage = NULL;
    unsigned char *pDst = NULL;

    #if 0
    static MI_S32 debug_i = 0;
    char szDebugName[64] = {0, };
    #endif

    if (pstInputTensorVector == NULL)
    {
        return -1;
    }

    // MIXER_DBG("eBufType:%d, ePixelFormat:%d\n", stBufInfo.eBufType, stBufInfo.stFrameData.ePixelFormat);
    switch(stBufInfo.eBufType)
    {
        case E_MI_SYS_BUFDATA_RAW:
        {
            // stInputTensorVector.stArrayTensors[0].phyTensorAddr[0] = pstBufInfo->stRawData.phyAddr;
            // stInputTensorVector.stArrayTensors[0].pstTensorData[0] = pstBufInfo->stRawData.pVirAddr;
        }
        break;

        case E_MI_SYS_BUFDATA_FRAME:
        {
            if ((stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_ABGR8888) ||
                (stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_ARGB8888) ||
                (stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420))
            {
                pSrcImage = (unsigned char *)stBufInfo.stFrameData.pVirAddr[0];
            }
        }
        break;
        default:
        {
            MIXER_ERR("invalid buf type, %d\n", (int)stBufInfo.eBufType);
            return -1;
        }
    }

    // model channel
    if (stModelInfo.stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[3] == 3)
    {
        if ((stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_ARGB8888) ||
            (stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_ARGB8888))
        {
            cv::Mat srcMat(stBufInfo.stFrameData.u16Height, stBufInfo.stFrameData.u16Width ,CV_8UC4, pSrcImage);
            cv::Mat resizeMat;

            if (((MI_U32)srcMat.size().width != m_stFdModel.stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[2]) &&
                ((MI_U32)srcMat.size().height != m_stFdModel.stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[1]))
            {
                cv::resize(srcMat, resizeMat, cv::Size(m_stFdModel.stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[2],
                                            m_stFdModel.stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[1]));
            }
            else
            {
                resizeMat = srcMat;
            }

            #if 0
            memset(szDebugName, 0, sizeof(szDebugName));
            snprintf(szDebugName, sizeof(szDebugName) - 1, "resizeMat_%04d.bmp", debug_i ++);
            cv::imwrite(szDebugName, resizeMat);
            #endif

            pSrcImage = (unsigned char *)resizeMat.data;
            pDst = (unsigned char *)pstInputTensorVector->astArrayTensors[0].ptTensorData[0];
            memcpy(pDst, pSrcImage, resizeMat.size().width * resizeMat.size().height * 4);

            #if 0
            int fd = -1;
            memset(szDebugName, 0, sizeof(szDebugName));
            snprintf(szDebugName, sizeof(szDebugName) - 1, "%04d.raw", debug_i ++);
            fd = open(szDebugName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (fd > 0)
            {
                write(fd, pSrcImage, resizeMat.size().width * resizeMat.size().height * 4);
                close(fd);
            }
            #endif
        }
        else if (stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
        {
            cv::Mat srcMat(stBufInfo.stFrameData.u16Height * 3 / 2, stBufInfo.stFrameData.u16Width ,CV_8UC1, pSrcImage);
            cv::Mat resizeMat;

            if (((MI_U32)srcMat.size().width != m_stFdModel.stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[2]) &&
                ((MI_U32)srcMat.size().height != m_stFdModel.stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[1]))
            {
                cv::resize(srcMat, resizeMat, cv::Size(m_stFdModel.stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[2],
                                            m_stFdModel.stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[1]));
            }
            else
            {
                resizeMat = srcMat;
            }

            pSrcImage = (unsigned char *)resizeMat.data;
            pDst = (unsigned char *)pstInputTensorVector->astArrayTensors[0].ptTensorData[0];
            memcpy(pDst, pSrcImage, resizeMat.size().width * resizeMat.size().height);
        }
    }

    return MI_SUCCESS;
}

int CMidIPUFdFr::DoFd(const MI_SYS_BufInfo_t &stBufInfo, MI_IPU_TensorVector_t *Vector4Affine)
{
    MI_S32 s32Ret;
    MI_IPU_TensorVector_t stInputTensorVector, stOutputTensorVector;

    if (m_stFdModel.stNPUDesc.u32InputTensorCount != 1)
    {
        MIXER_ERR("error: FDA network input count isn't 1\n");
        return E_IPU_ERR_ILLEGAL_INPUT_OUTPUT_PARAM;
    }

    // prepare input vector
    memset(&stInputTensorVector, 0, sizeof(MI_IPU_TensorVector_t));
    // stInputTensorVector.u32TensorCount = m_stFdModel.stNPUDesc.u32InputTensorCount;
    if (MI_SUCCESS != (s32Ret = MI_IPU_GetInputTensors(m_stFdModel.ipuChn, &stInputTensorVector)))
    {
        MIXER_ERR("MI_IPU_GetInputTensors error, ret[0x%x]\n", s32Ret);
        return -1;
    }

#if 0
    switch(stBufInfo.eBufType)
    {
        case E_MI_SYS_BUFDATA_RAW:
        {
            // stInputTensorVector.stArrayTensors[0].phyTensorAddr[0] = pstBufInfo->stRawData.phyAddr;
            // stInputTensorVector.stArrayTensors[0].pstTensorData[0] = pstBufInfo->stRawData.pVirAddr;
        }
        break;

        case E_MI_SYS_BUFDATA_FRAME:
        {
            if (stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_ABGR8888 ||
                stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_ARGB8888)
            {
                stInputTensorVector.stArrayTensors[0].pstTensorData[0] = stBufInfo.stFrameData.pVirAddr[0];
                stInputTensorVector.stArrayTensors[0].phyTensorAddr[0] = stBufInfo.stFrameData.phyAddr[0];

                stInputTensorVector.stArrayTensors[0].phyTensorAddr[1] = stBufInfo.stFrameData.phyAddr[1];
                stInputTensorVector.stArrayTensors[0].pstTensorData[1] = stBufInfo.stFrameData.pVirAddr[1];
            }
        }
        break;

        default:
        {
            MIXER_ERR("invalid buf type, %d\n", (int)stBufInfo.eBufType);
            return -1;
        }
    }
#endif
     
    (void)ScaleToModelSize(stBufInfo, &stInputTensorVector, m_stFdModel);

    memset(&stOutputTensorVector, 0, sizeof(MI_IPU_TensorVector_t));
    if (MI_SUCCESS != (s32Ret = MI_IPU_GetOutputTensors(m_stFdModel.ipuChn, &stOutputTensorVector)))
    {
        MIXER_ERR("MI_IPU_GetOutputTensors error, ret[0x%x]\n", s32Ret);
        MI_IPU_PutInputTensors(m_stFdModel.ipuChn, &stInputTensorVector);
        return -1;
    }

    if(MI_SUCCESS != (s32Ret = MI_IPU_Invoke(m_stFdModel.ipuChn, &stInputTensorVector, &stOutputTensorVector)))
    {
        MIXER_ERR("MI_IPU_Invoke error, ret[0x%x]\n", s32Ret);
        MI_IPU_PutOutputTensors(m_stFdModel.ipuChn,&stOutputTensorVector);
        MI_IPU_PutInputTensors(m_stFdModel.ipuChn, &stInputTensorVector);
        return -1;
    }

    *Vector4Affine = stOutputTensorVector;

    // release input vector
    MI_IPU_PutInputTensors(m_stFdModel.ipuChn, &stInputTensorVector);

    return MI_SUCCESS;
}

int CMidIPUFdFr::DoFr(const MI_SYS_BufInfo_t &stBufInfo, MI_IPU_TensorVector_t *Vector4Affine)
{
    MI_S32 s32Ret;
    MI_IPU_TensorVector_t stInputTensorVector, stOutputTensorVector;
    stFdaOutputDataDesc_t data;
    if(NULL == Vector4Affine)
    {
      return -1;
    }
    //prepare FR input vector 
    memset(&stInputTensorVector, 0, sizeof(MI_IPU_TensorVector_t));
    if (MI_SUCCESS != (s32Ret = MI_IPU_GetInputTensors(m_stFrModel.ipuChn, &stInputTensorVector)))
    {
        MIXER_ERR("MI_IPU_GetInputTensors error, ret[0x%x]\n", s32Ret);
        return -1;
    }

    //prepare FR output vector
    memset(&stOutputTensorVector, 0, sizeof(MI_IPU_TensorVector_t));
    if (MI_SUCCESS != (s32Ret = MI_IPU_GetOutputTensors(m_stFrModel.ipuChn, &stOutputTensorVector)))
    {
        MIXER_ERR("MI_IPU_GetOutputTensors error, ret[0x%x]\n", s32Ret);
        MI_IPU_PutInputTensors(m_stFrModel.ipuChn, &stInputTensorVector);
        return -1;
    }

    data.pfBBox = (float*)(Vector4Affine->astArrayTensors[0].ptTensorData[0]);
    data.pfLms = (float*)(Vector4Affine->astArrayTensors[1].ptTensorData[0]);
    data.pfScores = (float*)(Vector4Affine->astArrayTensors[2].ptTensorData[0]);
    data.pDectCount = (float*)(Vector4Affine->astArrayTensors[3].ptTensorData[0]);
    DoTrack(&data);
    MI_IPU_PutOutputTensors(m_stFdModel.ipuChn, Vector4Affine);

    MI_S8 ret = DoRecognition(stBufInfo, &stInputTensorVector, &stOutputTensorVector);

	//MIXER_DBG("=====DoRecognition====%d\n",ret);
	
    MI_IPU_PutInputTensors(m_stFrModel.ipuChn, &stInputTensorVector);
    MI_IPU_PutOutputTensors(m_stFrModel.ipuChn, &stOutputTensorVector);

    return MI_SUCCESS;
}

void CMidIPUFdFr::DoTrack(stFdaOutputDataDesc_t *pstFdaOutputData)
{
    std::vector<DetBBox> tempDetboxes;
	DetBBox detbox;
    std::vector <std::vector<TrackBBox>> detFrameDatas;
    std::vector <TrackBBox> detFrameData;

    SaveFdaOutData(pstFdaOutputData,tempDetboxes);
    int count = tempDetboxes.size();
    for (int i = 0; i < count; i++)
    {
        detbox = tempDetboxes[i];
        if (detbox.score > 0.5)
        {
            if (detbox.x1 < 0 || detbox.y1 < 0 ||
                detbox.x2 > (float)m_stFrModel.u32Width || detbox.y2> (float)m_stFrModel.u32Height)
                continue;

            TrackBBox cur_box;
            cur_box.x = detbox.x1;
            cur_box.y = detbox.y1;
            cur_box.w = detbox.x2 - detbox.x1;
            cur_box.h = detbox.y2 - detbox.y1;
            cur_box.score = detbox.score;
			cur_box.lm1_x = detbox.lm1_x;
		    cur_box.lm1_y = detbox.lm1_y;
			cur_box.lm2_x = detbox.lm2_x;
			cur_box.lm2_y = detbox.lm2_y;
			cur_box.lm3_x = detbox.lm3_x;
			cur_box.lm3_y = detbox.lm3_y;
			cur_box.lm4_x = detbox.lm4_x;
			cur_box.lm4_y = detbox.lm4_y;
			cur_box.lm5_x = detbox.lm5_x;
			cur_box.lm5_y = detbox.lm5_y;
			cur_box.classID = 0;

			detFrameData.push_back(cur_box);
	        MIXER_INFO("2==%d==+++++++++  X1: %.2f  Y1: %.2f  X2: %.2f  Y2: %.2f score :%.2f  ++++++++++\n",
            i, cur_box.x, cur_box.y, cur_box.w, cur_box.h, cur_box.score);
        }
    }

    detFrameDatas.push_back(detFrameData);
    m_vecTrackBBoxs = m_BBoxTracker.track_iou(detFrameDatas);
}
#if 1
std::vector<DetBBox> CMidIPUFdFr::SaveFdaOutData(stFdaOutputDataDesc_t *pstFdaOutputData,std::vector<DetBBox> & detboxes)
{
    float fcount = *(pstFdaOutputData->pDectCount);
    int count = fcount;
//    MIXER_DBG("=============face count%d============\n", count);
    detboxes.clear();
    for (int i = 0; i < count; i++)
    {
        DetBBox box;
        if (*(pstFdaOutputData->pfScores + i) > MID_FR_THRESHOLD) {
            box.y1 = *(pstFdaOutputData->pfBBox + i * ALIGN_UP(4, INNER_MOST_ALIGNMENT)) * m_stFrModel.u32Height;
            box.x1 = *(pstFdaOutputData->pfBBox + i * ALIGN_UP(4, INNER_MOST_ALIGNMENT)+1) * m_stFrModel.u32Width;
            box.y2 = *(pstFdaOutputData->pfBBox + i * ALIGN_UP(4, INNER_MOST_ALIGNMENT)+2) * m_stFrModel.u32Height;
            box.x2 = *(pstFdaOutputData->pfBBox + i * ALIGN_UP(4, INNER_MOST_ALIGNMENT) + 3) * m_stFrModel.u32Width;
            box.score = *(pstFdaOutputData->pfScores+ i);
            box.lm1_x = *(pstFdaOutputData->pfLms + i * ALIGN_UP(10,INNER_MOST_ALIGNMENT)) * m_stFrModel.u32Width;
            box.lm1_y = *(pstFdaOutputData->pfLms + i * ALIGN_UP(10,INNER_MOST_ALIGNMENT) + 1) * m_stFrModel.u32Height;
            box.lm2_x = *(pstFdaOutputData->pfLms + i * ALIGN_UP(10,INNER_MOST_ALIGNMENT) + 2) * m_stFrModel.u32Width;
            box.lm2_y = *(pstFdaOutputData->pfLms + i * ALIGN_UP(10,INNER_MOST_ALIGNMENT) + 3) * m_stFrModel.u32Height;
            box.lm3_x = *(pstFdaOutputData->pfLms + i * ALIGN_UP(10,INNER_MOST_ALIGNMENT) + 4) * m_stFrModel.u32Width;
            box.lm3_y = *(pstFdaOutputData->pfLms + i * ALIGN_UP(10,INNER_MOST_ALIGNMENT) + 5) * m_stFrModel.u32Height;
            box.lm4_x = *(pstFdaOutputData->pfLms + i * ALIGN_UP(10,INNER_MOST_ALIGNMENT) + 6) * m_stFrModel.u32Width;
            box.lm4_y = *(pstFdaOutputData->pfLms + i * ALIGN_UP(10,INNER_MOST_ALIGNMENT) + 7) * m_stFrModel.u32Height;
            box.lm5_x = *(pstFdaOutputData->pfLms + i * ALIGN_UP(10,INNER_MOST_ALIGNMENT) + 8) * m_stFrModel.u32Width;
            box.lm5_y = *(pstFdaOutputData->pfLms + i * ALIGN_UP(10,INNER_MOST_ALIGNMENT) + 9) * m_stFrModel.u32Height;
            MIXER_INFO("1==%d==+++++++++  X1: %.2f  Y1: %.2f  X2: %.2f  Y2: %.2f  ++++++++++\n", i, box.x1, box.y1, box.x2, box.y2);
            detboxes.push_back(box);
        }
    }

    return detboxes;

}

#else
std::vector<DetBBox> CMidIPUFdFr::SaveFdaOutData(stFdaOutputDataDesc_t *pstFdaOutputData,std::vector<DetBBox> &detboxes)
{
    float fcount = *(pstFdaOutputData->pDectCount);
    int count = fcount;

    // MIXER_DBG("=============face count:%d============\n", count);
    detboxes.clear();
    for (int i = 0; i < count; i++)
    {
        DetBBox box;
        if (*(pstFdaOutputData->pfScores + i) > MID_FR_THRESHOLD)
        {
            box.y1 = (*(pstFdaOutputData->pfBBox + i * 4) * m_stFdModel.u32Height);
            box.x1 = (*(pstFdaOutputData->pfBBox + i * 4 + 1) * m_stFdModel.u32Width);
            box.y2 = (*(pstFdaOutputData->pfBBox + i * 4 + 2) * m_stFdModel.u32Height);
            box.x2 = (*(pstFdaOutputData->pfBBox + i * 4 + 3) * m_stFdModel.u32Width);
            box.score = (*(pstFdaOutputData->pfScores+ i));
            box.lm1_x = (*(pstFdaOutputData->pfLms + i * 10) * (float)m_stFdModel.u32Width);
            box.lm1_y = (*(pstFdaOutputData->pfLms + i * 10 + 1) * (float)m_stFdModel.u32Height);
            box.lm2_x = (*(pstFdaOutputData->pfLms + i * 10 + 2) * (float)m_stFdModel.u32Width);
            box.lm2_y = (*(pstFdaOutputData->pfLms + i * 10 + 3) * (float)m_stFdModel.u32Height);
            box.lm3_x = (*(pstFdaOutputData->pfLms + i * 10 + 4) * (float)m_stFdModel.u32Width);
            box.lm3_y = (*(pstFdaOutputData->pfLms + i * 10 + 5) * (float)m_stFdModel.u32Height);
            box.lm4_x = (*(pstFdaOutputData->pfLms + i * 10 + 6) * (float)m_stFdModel.u32Width);
            box.lm4_y = (*(pstFdaOutputData->pfLms + i * 10 + 7) * (float)m_stFdModel.u32Height);
            box.lm5_x = (*(pstFdaOutputData->pfLms + i * 10 + 8) * (float)m_stFdModel.u32Width);
            box.lm5_y = (*(pstFdaOutputData->pfLms + i * 10 + 9) * (float)m_stFdModel.u32Height);
            MIXER_DBG("1==%d==+++++++++  X1: %.2f  Y1: %.2f  X2: %.2f  Y2: %.2f  ++++++++++\n", i, box.x1, box.y1, box.x2, box.y2);
            detboxes.push_back(box);
        }
    }

    return detboxes;
}
#endif

MI_S8 CMidIPUFdFr::DoRecognition(const MI_SYS_BufInfo_t &stBufInfo,
                                MI_IPU_TensorVector_t *inputVector,
                                MI_IPU_TensorVector_t *outputVector)
{
    void* pimg = NULL;
    int s32Ret, count = m_vecTrackBBoxs.size();
    std::map<int, stCountName_t> mapLastIDName;
    mapLastIDName.swap(m_mapCurIDName);
    m_vectResult.clear();
    IeParamInfo ieParam;
	memset(&ieParam,0,sizeof(IeParamInfo));
	MI_S32 ret = GetIeParam(ieParam);
	
    bool bDataBaseChanged = FALSE;
    if(-1 == ret)
    {
         m_faceDB.DelPerson((const char*)ieParam.NewDelName);
         MIXER_DBG("del persion %s\n",ieParam.NewDelName);
		 memset(&ieParam,0,sizeof(IeParamInfo));
		 SetIeParam(ieParam,2);
         bDataBaseChanged = TRUE;
    }
    switch(stBufInfo.eBufType)
    {
        case E_MI_SYS_BUFDATA_RAW:
        {
           pimg = stBufInfo.stRawData.pVirAddr;
        }
        break;

        case E_MI_SYS_BUFDATA_FRAME:
        {
            if ((stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_ABGR8888) ||
                (stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_ARGB8888) ||
                (stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420))
            {
                pimg = stBufInfo.stFrameData.pVirAddr[0];
            }
        }
        break;
        default:
        {
            MIXER_ERR("invalid buf type, %d\n", (int)stBufInfo.eBufType);
            return 0;
        }
    }

    for(int i = 0; i < count; i++)
    {
        int id = m_vecTrackBBoxs[i].id;
        stCountName_t countname = SearchNameByID(mapLastIDName, id);
        int index = m_vecTrackBBoxs[i].boxes.size()-1;
        TrackBBox & trackBox = m_vecTrackBBoxs[i].boxes[index];
        bool bForceFR = FALSE;
		if(1 == ret)
		{
	       MIXER_DBG("add persion name %s  addid=%d  id=%d countname=%s\n",ieParam.NewAddName,ieParam.box_id,id,countname.Name);
		}
        if((1 == ret)&&(id == ieParam.box_id))
        {
            bForceFR = TRUE;
        }

        if(countname.Name.empty() == FALSE && bForceFR == FALSE)
        {
            m_mapCurIDName.insert({id, countname});
        }
        else
        {
            MIXER_INFO("3==%d==+++++++++  X: %.2f  Y: %.2f  H: %.2f  W: %.2f  ++++++++++\n", i, trackBox.x, trackBox.y, trackBox.h, trackBox.w);
            if (trackBox.h < 120 && trackBox.w < 120)
            {
                goto SHOWFACE;
            }

            if ((stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_ABGR8888) ||
                (stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_ARGB8888))
            {
                cv::Mat frame(stBufInfo.stFrameData.u16Height, stBufInfo.stFrameData.u16Width, CV_8UC4, pimg);
                unsigned char* pimg112 = (unsigned char* )inputVector->astArrayTensors[0].ptTensorData[0];
                cv::Mat crop(112, 112, CV_8UC4, pimg112);

                float face5point[10] =
                {
                    trackBox.lm1_x, trackBox.lm1_y,
                    trackBox.lm2_x, trackBox.lm2_y,
                    trackBox.lm3_x, trackBox.lm3_y,
                    trackBox.lm4_x, trackBox.lm4_y,
                    trackBox.lm5_x, trackBox.lm5_y
                };

                FaceRecognizeUtils::CropImage_112x112<float>(frame, face5point, crop);
            }
            else if (stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
            {
                cv::Mat frame(stBufInfo.stFrameData.u16Height * 3 / 2, stBufInfo.stFrameData.u16Width ,CV_8UC1, pimg);
                unsigned char* pimg112 = (unsigned char* )inputVector->astArrayTensors[0].ptTensorData[0];
                cv::Mat crop(112*3/2, 112, CV_8UC1, pimg112,ALIGN_UP(112,16));
                float face5point[10] =
                {
                    trackBox.lm1_x, trackBox.lm1_y,
                    trackBox.lm2_x, trackBox.lm2_y,
                    trackBox.lm3_x, trackBox.lm3_y,
                    trackBox.lm4_x, trackBox.lm4_y,
                    trackBox.lm5_x, trackBox.lm5_y
                };


                FaceRecognizeUtils::CropImage_112x112_YUV420_NV12(frame, face5point, crop);
                #if 0
                {
                    cv::Mat BGR_crop;
                    char char_crop[32];
                    sprintf(char_crop, "crop/crop_%d.jpg", nnncrop++);
                    cv::cvtColor(crop, BGR_crop, cv::COLOR_YUV2BGR_NV12);

                    cv::imwrite(char_crop,BGR_crop);
                    if (nnncrop == 50) {
                      nnncrop = 0;
                    }
                }
                #endif
            }


            s32Ret = MI_IPU_Invoke(m_stFrModel.ipuChn, inputVector, outputVector);
            if (s32Ret != MI_SUCCESS) {
                MIXER_INFO("FR invoke error:%d\n", s32Ret);
                continue;
            }

            for (unsigned int j = 0; j < outputVector->u32TensorCount; j++) {
                  countname.Name = getFaceNamebyFeature((float*)outputVector->astArrayTensors[0].ptTensorData[0]);
                if (countname.Name.empty() == FALSE )
                {
                    m_mapCurIDName.insert({id, countname});
                }
                else
                {
                    countname.Name = "unknown";
                    m_mapCurIDName.insert({id, countname});
                }
				if((bForceFR && !strcmp(countname.Name.c_str(), "unknown"))||((strcmp(countname.Name.c_str(),ieParam.NewAddName)==0)&&(strlen(ieParam.NewAddName)!=0)))
                {
                    m_faceDB.AddPersonFeature((char*)ieParam.NewAddName, (float*)outputVector->astArrayTensors[0].ptTensorData[0]);
                    MIXER_DBG("add %s to database\n",ieParam.NewAddName);
                    bDataBaseChanged = TRUE;
                }
            }
        }

SHOWFACE:
        //save result info
        stFaceInfo_t stFaceInfo;
        memset(&stFaceInfo, 0, sizeof(stFaceInfo_t));
        // box

        stFaceInfo.faceH = trackBox.h;
        stFaceInfo.faceW = trackBox.w;
        stFaceInfo.xPos  = trackBox.x;
        stFaceInfo.yPos  = trackBox.y;
        stFaceInfo.winWid = m_stFrModel.u32Width;
        stFaceInfo.winHei =  m_stFrModel.u32Height;
        m_vectResult.push_back(stFaceInfo);

        if(countname.Name.empty())
        {
            snprintf(stFaceInfo.faceName, sizeof(stFaceInfo.faceName) - 1, "%sID :%d", "unknown",id);
        }
        else
        {
           snprintf(stFaceInfo.faceName, sizeof(stFaceInfo.faceName) - 1, "%sID :%d", countname.Name.c_str(), id);
		}
        m_vectResult.push_back(stFaceInfo);
    }

    if (bDataBaseChanged)
    {
        m_faceDB.SaveToFileBinary(m_stIPUInfo.u.ExtendInfo2.szFaceDBFile, m_stIPUInfo.u.ExtendInfo2.szNameListFile);
        m_mapCurIDName.clear();
		bDataBaseChanged = FALSE;
    }

    std::map<int, stCountName_t>::iterator iter_end = m_mapCurIDName.begin();
    for (; iter_end != m_mapCurIDName.end(); ) {
        iter_end->second.Count++;
        if ((iter_end->second.Count % 10 == 0)  && (!strcmp(iter_end->second.Name.c_str(), "unknown"))) {
            m_mapCurIDName.erase(iter_end ++);
        }
        else
        {
            iter_end++;
        }
    }
	ret = 0;
	memset(&ieParam,0,sizeof(IeParamInfo));
	SetIeParam(ieParam,0);
	return ret;
}

void CMidIPUFdFr::PrintResult()
{
    std::vector<stFaceInfo_t>::iterator itVectFace;

    stFaceInfo_t *ptmp = NULL;
    MI_U16 i = 0x0;
    ST_DlaRectInfo_T stDlaRectInfo[MAX_DLA_RECT_NUMBER];

    for (itVectFace = m_vectResult.begin(); \
            (itVectFace != m_vectResult.end()) && (i < MAX_DLA_RECT_NUMBER); \
            itVectFace++, i++)
    {
        memset(&stDlaRectInfo[i], 0, sizeof(stDlaRectInfo[i]));

        ptmp = &(*itVectFace);
        stDlaRectInfo[i].rect.u32X = (ptmp->xPos *  m_stFrModel.u32Width) / m_stFrModel.u32Width;
        stDlaRectInfo[i].rect.u32Y = (ptmp->yPos *  m_stFrModel.u32Height) /  m_stFrModel.u32Height;
        stDlaRectInfo[i].rect.u16PicW = (ptmp->faceW *  m_stFrModel.u32Width) /  m_stFrModel.u32Width;
        stDlaRectInfo[i].rect.u16PicH = (ptmp->faceH *  m_stFrModel.u32Height) /  m_stFrModel.u32Height;

        memcpy(stDlaRectInfo[i].szObjName, ptmp->faceName, sizeof(ptmp->faceName) - 1);

        MIXER_INFO("i:%d, x:%d, y:%d, w:%d, h:%d, name:%s, faceName:%s winWid=%d winHei=%d\n", i, stDlaRectInfo[i].rect.u32X,\
                                                        stDlaRectInfo[i].rect.u32Y,\
                                                        stDlaRectInfo[i].rect.u16PicW,\
                                                        stDlaRectInfo[i].rect.u16PicH,\
                                                        stDlaRectInfo[i].szObjName,\
                                                        ptmp->faceName,
                                                        ptmp->winWid,
                                                        ptmp->winHei);
    }

      DLAtoRECT(0, i, stDlaRectInfo, TRUE, TRUE);
      pthread_mutex_lock(&g_mutex_UpadteOsdState);  
	  g_UpadteOsdState++;
	  pthread_cond_signal(&g_cond_UpadteOsdState);  
      pthread_mutex_unlock(&g_mutex_UpadteOsdState);  
      m_vectResult.clear();
}

stCountName_t CMidIPUFdFr::SearchNameByID(std::map<int, stCountName_t> &mapLastIDName, int id)
{
    std::map<int, stCountName_t>::iterator iter;
    iter = mapLastIDName.find(id);
    if(iter==mapLastIDName.end())
    {
       stCountName_t countname;
       countname.Count = 0;
       countname.Name = "";
       return countname;
    }
    else
    {
        return iter->second;
    }
}

std::string CMidIPUFdFr::getFaceNamebyFeature(void* feat1)
{
    return m_faceRecognizer.find_name(m_faceDB, (float *)feat1);
}
