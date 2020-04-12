#include "mid_ipu_classify.h"
#include "mid_system_config.h"
#include "mid_VideoEncoder.h"

#include "mid_dla.h"

#include <fstream>
#include <iostream>

#define MAX_POSSIBILITY_NUM            (5)

extern int DLAtoRECT(MI_VENC_CHN s32VencChn, int recCnt, ST_DlaRectInfo_T* pRecInfo, MI_BOOL bShow, MI_BOOL bShowBorder);

CMidIPUClassify::CMidIPUClassify(IPU_InitInfo_S &stInitInfo) : CMidIPUInterface(stInitInfo)
{
    m_vpeChn            = 0;
    m_vpePort            = 2;
    m_divpChn            = Mixer_Divp_GetChannleNum();
    m_ipuChn             = 0;
    m_u32OutBufDepth     = 1;
    m_u32InBufDepth        = 0;
    m_s32Fd                = -1;
    memset(&m_stNPUDesc, 0, sizeof(MI_IPU_SubNet_InputOutputDesc_t));

    (void)InitResource();
}

CMidIPUClassify::~CMidIPUClassify()
{
    Mixer_Divp_PutChannleNum(m_divpChn);

    (void)ReleaseResource();
}

int CMidIPUClassify::InitResource()
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_S32 s32Cnt = 0;
    std::ifstream LabelFile;

    if(MI_SUCCESS != IPUCreateDevice(m_stIPUInfo.szIPUfirmware, MAX_VARIABLE_BUF_SIZE))
    {
        MIXER_ERR("IPUCreateDevice error, %s, ret:[0x%x]\n", m_stIPUInfo.szIPUfirmware, s32Ret);
        return -1;
    }

    if(MI_SUCCESS != IPUCreateChannel(&m_ipuChn, m_stIPUInfo.szModelFile, m_u32InBufDepth, m_u32OutBufDepth))
    {
        MIXER_ERR("IPUCreateChannel error, chn:%d, model:%s, ret:[0x%x]\n", m_ipuChn, m_stIPUInfo.szModelFile, s32Ret);
        IPUDestroyDevice();
        return -1;
    }

    if (MI_SUCCESS != (s32Ret = MI_IPU_GetInOutTensorDesc(m_ipuChn, &m_stNPUDesc)))
    {
        MIXER_ERR("MI_IPU_GetInOutTensorDesc error, chn:%d, ret:[0x%x]\n", m_ipuChn, s32Ret);
        IPUDestroyChannel(m_ipuChn, m_u32InBufDepth, m_u32OutBufDepth);
        IPUDestroyDevice();
        return -1;
    }

    MIXER_DBG("H:%d,W:%d,C:%d,format:%s\n", m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[1],
                m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[2], m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[3],
                (m_stNPUDesc.astMI_InputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_U8) ? "MI_IPU_FORMAT_U8" :
                (m_stNPUDesc.astMI_InputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_NV12) ? "MI_IPU_FORMAT_NV12" :
                (m_stNPUDesc.astMI_InputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_INT16) ? "MI_IPU_FORMAT_INT16" :
                (m_stNPUDesc.astMI_InputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_INT32) ? "MI_IPU_FORMAT_INT32" :
                (m_stNPUDesc.astMI_InputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_INT8) ? "MI_IPU_FORMAT_INT8" :
                (m_stNPUDesc.astMI_InputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_FP32) ? "MI_IPU_FORMAT_FP32" : "UNKNOWN");

    LabelFile.open(m_stIPUInfo.u.ExtendInfo1.szLabelFile);
    s32Cnt = 0;
    while(1)
    {
        if(LabelFile.eof() || (s32Cnt >= LABEL_CLASS_COUNT))
        {
            break;
        }

        LabelFile.getline(&m_szLabelName[s32Cnt][0], LABEL_NAME_MAX_SIZE);

        // MIXER_DBG("m_szLabelName[%d]=%s\n", s32Cnt, m_szLabelName[s32Cnt]);

        s32Cnt++;
    }

    (void)InitStreamSource();

    return MI_SUCCESS;
}

int CMidIPUClassify::ReleaseResource()
{
    (void)IPUDestroyChannel(m_ipuChn, m_u32InBufDepth, m_u32OutBufDepth);
    (void)IPUDestroyDevice();

    (void)ReleaseStreamSource();

    return MI_SUCCESS;
}

void CMidIPUClassify::DealDataProcess()
{
    fd_set read_fds;
    struct timeval tv;
    MI_S32 s32Ret = 0;
    MI_SYS_ChnPort_t stChnOutputPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE stBufHandle;

    stChnOutputPort.eModId       = E_MI_MODULE_ID_DIVP;
    stChnOutputPort.u32DevId     = 0;
    stChnOutputPort.u32ChnId     = m_divpChn;
    stChnOutputPort.u32PortId     = 0;

    FD_ZERO(&read_fds);
    FD_SET(m_s32Fd, &read_fds);

    tv.tv_sec = 0;
    tv.tv_usec = 100 * 1000;
    s32Ret = select(m_s32Fd + 1, &read_fds, NULL, NULL, &tv);
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
        if(FD_ISSET(m_s32Fd, &read_fds))
        {
            memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
            if (MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(&stChnOutputPort, &stBufInfo, &stBufHandle))
            {
                usleep(1*1000);
            }
            else
            {
                (void)DoClassify(&stBufInfo);

                if (MI_SUCCESS != MI_SYS_ChnOutputPortPutBuf(stBufHandle))
                {
                    MIXER_ERR("MI_SYS_ChnOutputPortPutBuf error\n");
                }
            }
        }
    }
}
 MI_S32 CMidIPUClassify::SetIeParam(IeParamInfo tmp,MI_U8 scop)
{
  return 0;
}
int CMidIPUClassify::InitStreamSource()
{
    MI_VPE_PortMode_t stVpeMode;
    MI_SYS_WindowRect_t stCropWin;
    MI_DIVP_OutputPortAttr_t stDivpPortAttr;
    MI_S32 s32Ret = MI_SUCCESS;
    Mixer_Sys_BindInfo_T stBindInfo;
    MI_SYS_ChnPort_t stChnOutputPort;

    memset(&stVpeMode, 0, sizeof(MI_VPE_PortMode_t));
    ExecFunc(MI_VPE_GetPortMode(m_vpeChn, m_vpePort, &stVpeMode), MI_VPE_OK);

    stCropWin.u16Width = stVpeMode.u16Width;
    stCropWin.u16Height = stVpeMode.u16Height;
    stCropWin.u16X = 0;
    stCropWin.u16Y = 0;
    MIXERCHECKRESULT(Mixer_Divp_CreatChannel(m_divpChn, (MI_SYS_Rotate_e)0x0, &stCropWin));

    stDivpPortAttr.u32Width      = m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[1];
    stDivpPortAttr.u32Height     = m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[2];
    stDivpPortAttr.eCompMode     = E_MI_SYS_COMPRESS_MODE_NONE;
    stDivpPortAttr.ePixelFormat    = E_MI_SYS_PIXEL_FRAME_ABGR8888;

    MIXERCHECKRESULT(Mixer_Divp_SetOutputAttr(m_divpChn, &stDivpPortAttr));
    MIXERCHECKRESULT(Mixer_Divp_StartChn(m_divpChn));

    memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));

    stBindInfo.stSrcChnPort.eModId    = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId  = 0;
    stBindInfo.stSrcChnPort.u32ChnId  = m_vpeChn;
    stBindInfo.stSrcChnPort.u32PortId = m_vpePort;

    stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_DIVP;
    stBindInfo.stDstChnPort.u32DevId  = 0;
    stBindInfo.stDstChnPort.u32ChnId  = m_divpChn;
    stBindInfo.stDstChnPort.u32PortId = 0;

    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stBindInfo.stDstChnPort, 3, 3), MI_SUCCESS);

    stBindInfo.u32SrcFrmrate = 30; //MI_VideoEncoder::vpeframeRate;
    stBindInfo.u32DstFrmrate = 15; //MI_VideoEncoder::vpeframeRate;
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
#endif
    MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));

    stChnOutputPort.eModId       = E_MI_MODULE_ID_DIVP;
    stChnOutputPort.u32DevId     = 0;
    stChnOutputPort.u32ChnId     = m_divpChn;
    stChnOutputPort.u32PortId     = 0;
    s32Ret = MI_SYS_GetFd(&stChnOutputPort, &m_s32Fd);
    if (s32Ret < 0)
    {
        MIXER_ERR("divp ch: %d, get fd. err\n", stChnOutputPort.u32ChnId);
        return -1;
    }

    return MI_SUCCESS;
}

int CMidIPUClassify::ReleaseStreamSource()
{
    Mixer_Sys_BindInfo_T stBindInfo;

    if (m_s32Fd > 0)
    {
        MI_SYS_CloseFd(m_s32Fd);
        m_s32Fd = -1;
    }

    memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));

    stBindInfo.stSrcChnPort.eModId    = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId  = 0;
    stBindInfo.stSrcChnPort.u32ChnId  = m_vpeChn;
    stBindInfo.stSrcChnPort.u32PortId = m_vpePort;

    stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_DIVP;
    stBindInfo.stDstChnPort.u32DevId  = 0;
    stBindInfo.stDstChnPort.u32ChnId  = m_divpChn;
    stBindInfo.stDstChnPort.u32PortId = 0;

    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stBindInfo.stDstChnPort, 3, 3), MI_SUCCESS);

    stBindInfo.u32SrcFrmrate = 30; //MI_VideoEncoder::vpeframeRate;
    stBindInfo.u32DstFrmrate = 15; //MI_VideoEncoder::vpeframeRate;
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
#endif
    MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

    MIXERCHECKRESULT(Mixer_Divp_StopChn(m_divpChn));
    MIXERCHECKRESULT(Mixer_Divp_DestroyChn(m_divpChn));

    return MI_SUCCESS;
}

int CMidIPUClassify::DoClassify(MI_SYS_BufInfo_t* pstBufInfo)
{
    MI_IPU_TensorVector_t stInputTensorVector;
    MI_IPU_TensorVector_t stOutputTensorVector;
    MI_S32 s32Ret = MI_SUCCESS;

    if (pstBufInfo == NULL)
    {
        return MI_SUCCESS;
    }

    memset(&stInputTensorVector, 0, sizeof(MI_IPU_TensorVector_t));
    stInputTensorVector.u32TensorCount = m_stNPUDesc.u32InputTensorCount;
    switch(pstBufInfo->eBufType)
    {
        case E_MI_SYS_BUFDATA_RAW:
        {
            stInputTensorVector.astArrayTensors[0].phyTensorAddr[0] = pstBufInfo->stRawData.phyAddr;
            stInputTensorVector.astArrayTensors[0].ptTensorData[0] = pstBufInfo->stRawData.pVirAddr;
        }
        break;

        case E_MI_SYS_BUFDATA_FRAME:
        {
            stInputTensorVector.astArrayTensors[0].phyTensorAddr[0] = pstBufInfo->stFrameData.phyAddr[0];
            stInputTensorVector.astArrayTensors[0].ptTensorData[0] = pstBufInfo->stFrameData.pVirAddr[0];

            stInputTensorVector.astArrayTensors[0].phyTensorAddr[1] = pstBufInfo->stFrameData.phyAddr[1];
            stInputTensorVector.astArrayTensors[0].ptTensorData[1] = pstBufInfo->stFrameData.pVirAddr[1];
        }
        break;
        default:
        {
            MIXER_ERR("invalid buf type, %d\n", (int)pstBufInfo->eBufType);
            return -1;
        }
    }

    memset(&stOutputTensorVector, 0, sizeof(MI_IPU_TensorVector_t));
    if (MI_SUCCESS != (s32Ret = MI_IPU_GetOutputTensors(m_ipuChn, &stOutputTensorVector)))
    {
        MIXER_ERR("MI_IPU_GetOutputTensors error, ret[0x%x]\n", s32Ret);
        // MI_IPU_PutInputTensors(m_ipuChn, &stInputTensorVector);
        return -1;
    }

    if(MI_SUCCESS != (s32Ret = MI_IPU_Invoke(m_ipuChn, &stInputTensorVector, &stOutputTensorVector)))
    {
        MIXER_ERR("MI_IPU_Invoke error, ret[0x%x]\n", s32Ret);
        MI_IPU_PutOutputTensors(m_ipuChn,&stOutputTensorVector);
        return -1;
    }

    PrintResult(&stOutputTensorVector);

    MI_IPU_PutOutputTensors(m_ipuChn,&stOutputTensorVector);

    return MI_SUCCESS;
}

MI_BOOL CMidIPUClassify::GetTopN(float aData[], int dataSize, int aResult[], int TopN)
{
    int i, j, k;
    float data = 0;
    MI_BOOL bSkip = FALSE;

    for (i = 0; i < TopN; i++)
    {
        data = -0.1f;
        for (j = 0; j < dataSize; j++)
        {
            if (aData[j] > data)
            {
                bSkip = FALSE;
                for (k = 0; k < i; k++)
                {
                    if (aResult[k] == j)
                    {
                        bSkip = TRUE;
                    }
                }

                if (bSkip == FALSE)
                {
                    aResult[i] = j;
                    data = aData[j];
                }
            }
        }
    }

    return TRUE;
}

void CMidIPUClassify::PrintResult(MI_IPU_TensorVector_t* pstOutputTensorVector)
{
    MI_S32 s32TopN[MAX_POSSIBILITY_NUM] = {0,};
    MI_S32 s32DimCount = 0;//
    MI_S32 s32ClassCount = 1;
    MI_S32 i = 0;
    ST_DlaRectInfo_T stRectInfo[MAX_POSSIBILITY_NUM];

    if (pstOutputTensorVector == NULL)
    {
        return;
    }

    s32DimCount = m_stNPUDesc.astMI_OutputTensorDescs[0].u32TensorDim;;
    for(i = 0;i < s32DimCount; i++)
    {
      s32ClassCount *= m_stNPUDesc.astMI_OutputTensorDescs[0].u32TensorShape[i];
    }
    float *pfData = (float *)pstOutputTensorVector->astArrayTensors[0].ptTensorData[0];

    // MIXER_DBG("the class Count:%d\n", s32ClassCount);

    GetTopN(pfData, s32ClassCount, s32TopN, MAX_POSSIBILITY_NUM);

    for(i = 0; i < MAX_POSSIBILITY_NUM; i++)
    {
        memset(&stRectInfo[i], 0, sizeof(ST_DlaRectInfo_T));

        stRectInfo[i].rect.u32X = 10;
        stRectInfo[i].rect.u32Y = 10 + 40 * i;
        snprintf(stRectInfo[i].szObjName, sizeof(stRectInfo[i].szObjName) - 1, "order %d %f %d %s",
                    i, pfData[s32TopN[i]], s32TopN[i], m_szLabelName[s32TopN[i]]);

        //MIXER_DBG("[%d,%d,%d,%d], name:%s\n", stRectInfo[i].rect.u32X, stRectInfo[i].rect.u32Y,
        //        stRectInfo[i].rect.u16PicW, stRectInfo[i].rect.u16PicH, stRectInfo[i].szObjName);
        // MIXER_DBG("order %d %f %d \"%s\"\n", i, pfData[s32TopN[i]], s32TopN[i], m_szLabelName[s32TopN[i]]);
    }

    DLAtoRECT(0, MAX_POSSIBILITY_NUM, stRectInfo, TRUE, FALSE);
}

