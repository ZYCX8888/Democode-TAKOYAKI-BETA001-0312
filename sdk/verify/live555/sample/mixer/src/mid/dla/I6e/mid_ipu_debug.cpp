#include "mid_ipu_debug.h"
#include "mid_system_config.h"
#include "mid_VideoEncoder.h"
#include "mid_dla.h"

#include <math.h>

#define MID_FR_THRESHOLD    (0.0)

#define DIVP_WIDTH_ALIGN    (16)

extern int DLAtoRECT(MI_VENC_CHN s32VencChn, int recCnt, ST_DlaRectInfo_T* pRecInfo, MI_BOOL bShow, MI_BOOL bShowBorder);
extern MI_U32 g_rotation;

CMidIPUDebug::CMidIPUDebug(IPU_InitInfo_S &stInitInfo) : CMidIPUInterface(stInitInfo)
{
    // fd info
    memset(&m_stDebugFdaModel, 0, sizeof(Model_Info_S));
    m_stDebugFdaModel.vpeChn            = 0;
    m_stDebugFdaModel.vpePort            = 2;
    m_stDebugFdaModel.divpChn            = Mixer_Divp_GetChannleNum();;
    m_stDebugFdaModel.ipuChn            = 0;
    m_stDebugFdaModel.u32InBufDepth    = 1;
    m_stDebugFdaModel.u32OutBufDepth    = 1;
    m_stDebugFdaModel.s32Fd            = -1;
    m_stDebugFdaModel.pModelFile        = m_stIPUInfo.szModelFile;
    m_stDebugFdaModel.u32Width        = 640;
    m_stDebugFdaModel.u32Height        = 360;

    // fr info
    memset(&m_stDebugFrModel, 0, sizeof(Model_Info_S));
    m_stDebugFrModel.vpeChn            = 0;
    m_stDebugFrModel.vpePort            = 2;
    m_stDebugFrModel.divpChn            = Mixer_Divp_GetChannleNum();
    m_stDebugFrModel.ipuChn            = 1;
    m_stDebugFrModel.u32InBufDepth    = 1;
    m_stDebugFrModel.u32OutBufDepth    = 1;
    m_stDebugFrModel.s32Fd            = -1;
    m_stDebugFrModel.pModelFile        = m_stIPUInfo.u.ExtendInfo2.szModelFile1;
    m_stDebugFrModel.u32Width        = 1280;
    m_stDebugFrModel.u32Height        = 720;

    m_s32MaxFd = -1;
    m_dataReady = 0;

    (void)InitResource();
}

CMidIPUDebug::~CMidIPUDebug()
{
    Mixer_Divp_PutChannleNum(m_stDebugFdaModel.divpChn);
    Mixer_Divp_PutChannleNum(m_stDebugFrModel.divpChn);

    (void)ReleaseResource();
}

int CMidIPUDebug::InitResource()
{
    MI_S32 s32Ret = MI_SUCCESS;

    if(MI_SUCCESS != IPUCreateDevice(m_stIPUInfo.szIPUfirmware, 2 * MAX_VARIABLE_BUF_SIZE))
    {
        MIXER_ERR("IPUCreateDevice error, %s, ret:[0x%x]\n", m_stIPUInfo.szIPUfirmware, s32Ret);
        return -1;
    }

    if (MI_SUCCESS != InitStreamSource(m_stDebugFdaModel))
    {
        MIXER_ERR("InitStreamSource for fd error\n");
        IPUDestroyDevice();
        return -1;
    }

    if (MI_SUCCESS != InitStreamSource(m_stDebugFrModel))
    {
        MIXER_ERR("InitStreamSource for fr error\n");
        ReleaseStreamSource(m_stDebugFdaModel);
        IPUDestroyDevice();
        return -1;
    }

    return MI_SUCCESS;
}

int CMidIPUDebug::ReleaseResource()
{
    (void)ReleaseStreamSource(m_stDebugFdaModel);
    (void)ReleaseStreamSource(m_stDebugFrModel);

    (void)IPUDestroyDevice();

    return MI_SUCCESS;
}

void CMidIPUDebug::DealDataProcess()
{
    MI_S32 s32Ret = 0;
    MI_SYS_ChnPort_t stChnOutputPort;
    MI_U32 u32Count = 0;
    static MI_U64 lasttime = 0x0;
    MI_U64 time = 0x0;
    ST_DlaRectInfo_T stDlaRectInfo;
    static MI_U32 count = 0x0;
    // get fd data
    memset(&stChnOutputPort, 0, sizeof(MI_SYS_ChnPort_t));
    stChnOutputPort.eModId       = E_MI_MODULE_ID_DIVP;
    stChnOutputPort.u32DevId     = 0;
    stChnOutputPort.u32ChnId     = m_stDebugFdaModel.divpChn;
    stChnOutputPort.u32PortId     = 0;

    do
    {
        memset(&m_stDebugFdaModel.stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
        s32Ret = MI_SYS_ChnOutputPortGetBuf(&stChnOutputPort, &m_stDebugFdaModel.stBufInfo, &m_stDebugFdaModel.stBufHandle);
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
    stChnOutputPort.u32ChnId     = m_stDebugFrModel.divpChn;
    stChnOutputPort.u32PortId     = 0;

    do
    {
        memset(&m_stDebugFrModel.stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
        s32Ret = MI_SYS_ChnOutputPortGetBuf(&stChnOutputPort, &m_stDebugFrModel.stBufInfo, &m_stDebugFrModel.stBufHandle);
        if (s32Ret != MI_SUCCESS)
        {
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
        if(MI_SUCCESS == MI_SYS_GetCurPts(&time))
        {
            if((time - lasttime) > 100*1000)
            {
                lasttime = time;
                count++;
                stDlaRectInfo.rect.u32X = 0 + 8 * count;
                stDlaRectInfo.rect.u32Y = 0+ 8 * count;
                stDlaRectInfo.rect.u16PicW = 300;
                stDlaRectInfo.rect.u16PicH = 300;
                DLAtoRECT(0, 1, &stDlaRectInfo, TRUE, TRUE);
                if(count >= 50)
                    count = 0x0;
            }
        }

    }

    if(m_dataReady & 0x01)
    {
        MI_SYS_ChnOutputPortPutBuf(m_stDebugFdaModel.stBufHandle);
    }

    if ((m_dataReady >> 1) & 0x01)
    {
        MI_SYS_ChnOutputPortPutBuf(m_stDebugFrModel.stBufHandle);
    }

    m_dataReady = 0;
}
MI_S32 CMidIPUDebug::SetIeParam(IeParamInfo tmp,MI_U8 scop)
{
  return 0;
}
int CMidIPUDebug::InitStreamSource(Model_Info_S &stModelInfo)
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

int CMidIPUDebug::ReleaseStreamSource(Model_Info_S &stModelInfo)
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
    stBindInfo.u32DstFrmrate = 30; //MI_VideoEncoder::vpeframeRate;
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
#endif
    MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

    MIXERCHECKRESULT(Mixer_Divp_StopChn(stModelInfo.divpChn));
    MIXERCHECKRESULT(Mixer_Divp_DestroyChn(stModelInfo.divpChn));

    return MI_SUCCESS;
}


