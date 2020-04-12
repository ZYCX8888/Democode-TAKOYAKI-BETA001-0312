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
#include "mixer.h"

MI_BOOL mixer_func_trace = FALSE;
MIXER_DBG_LEVEL_e mixer_debug_level = MIXER_DBG_NONE;

MI_U32 mixer_util_get_time()
{
    struct timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    return 1000000 * (t1.tv_sec) + (t1.tv_nsec) / 1000;
}

int mixer_write_yuv_file(FILE_HANDLE filehandle, MI_SYS_FrameData_t framedata)
{
    MI_U16 width, height;
    char *dst_buf;
    int i;

    //DBG_ENTER();

    if(!filehandle)
    {
        //printf("%s:%d filehandle is NULL,invalid parameter!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    if(framedata.ePixelFormat != E_MI_SYS_PIXEL_FRAME_YUV422_YUYV && framedata.ePixelFormat != E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
    {
        printf("%s:%d ePixelFormat %d not support!\n", __FUNCTION__, __LINE__, framedata.ePixelFormat);
        return -1;
    }

    width = framedata.u16Width;
    height = framedata.u16Height;

    switch(framedata.ePixelFormat)
    {
        case E_MI_SYS_PIXEL_FRAME_YUV422_YUYV:
            dst_buf = framedata.pVirAddr[0];

            for(i = 0; i < height; i++)
            {
                if(fwrite((char *)dst_buf, 1 , width * 2, filehandle) != width * 2)
                    goto ERR_RET;

                dst_buf += framedata.u32Stride[0];
            }

            return 0;

        case E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420:
            dst_buf = framedata.pVirAddr[0];

            for(i = 0; i < height; i++)
            {
                if(fwrite((char *)dst_buf, 1 , width, filehandle) != width)
                    goto ERR_RET;

                dst_buf += framedata.u32Stride[0];
            }

            dst_buf = framedata.pVirAddr[1];

            for(i = 0; i < height / 2; i++)
            {
                if(fwrite((char *)dst_buf, 1 , width, filehandle) != width)
                    goto ERR_RET;

                dst_buf += framedata.u32Stride[1];
            }

            return 0;

        default:
            DBG_ERR("invalid format\n");
            return -1;
    }

ERR_RET:
    DBG_ERR("fail\n");
    return -1;
}


int mixer_init_hdmi(void)
{
    MI_HDMI_InitParam_t stHdmiInitParam;
    MI_HDMI_Attr_t stHdmiAttr;
    MI_HDMI_DeviceId_e eHdmiDevId = E_MI_HDMI_ID_0;

    stHdmiInitParam.pCallBackArgs = NULL;
    stHdmiInitParam.pfnHdmiEventCallback = NULL;
    ExecFunc(MI_HDMI_Init(&stHdmiInitParam), MI_SUCCESS);
    ExecFunc(MI_HDMI_Open(eHdmiDevId), MI_SUCCESS);
    ExecFunc(MI_HDMI_GetAttr(eHdmiDevId, &stHdmiAttr), MI_SUCCESS);
    stHdmiAttr.stEnInfoFrame.bEnableAudInfoFrame  = FALSE;
    stHdmiAttr.stEnInfoFrame.bEnableAviInfoFrame  = FALSE;
    stHdmiAttr.stEnInfoFrame.bEnableSpdInfoFrame  = FALSE;
    stHdmiAttr.stAudioAttr.bEnableAudio = TRUE;
    stHdmiAttr.stAudioAttr.bIsMultiChannel = 0;
    stHdmiAttr.stAudioAttr.eBitDepth = E_MI_HDMI_BIT_DEPTH_16;
    stHdmiAttr.stAudioAttr.eCodeType = E_MI_HDMI_ACODE_PCM;
    stHdmiAttr.stAudioAttr.eSampleRate = E_MI_HDMI_AUDIO_SAMPLERATE_48K;
    stHdmiAttr.stVideoAttr.bEnableVideo = TRUE;
    stHdmiAttr.stVideoAttr.eColorType = E_MI_HDMI_COLOR_TYPE_RGB444;//default color type
    stHdmiAttr.stVideoAttr.eDeepColorMode = E_MI_HDMI_DEEP_COLOR_MAX;
    stHdmiAttr.stVideoAttr.eTimingType = E_MI_HDMI_TIMING_1080_60P;
    stHdmiAttr.stVideoAttr.eOutputMode = E_MI_HDMI_OUTPUT_MODE_HDMI;
    ExecFunc(MI_HDMI_SetAttr(eHdmiDevId, &stHdmiAttr), MI_SUCCESS);
    ExecFunc(MI_HDMI_Start(eHdmiDevId), MI_SUCCESS);
    return 0;
}

int mixer_deInit_hdmi(void)
{
    MI_HDMI_DeviceId_e eHdmi = E_MI_HDMI_ID_0;
    MI_HDMI_Stop(eHdmi);
    MI_HDMI_Close(eHdmi);
    MI_HDMI_DeInit();
    return MI_SUCCESS;
}

int mixer_bind_module(MI_SYS_ChnPort_t *pstSrcChnPort, MI_SYS_ChnPort_t *pstDstChnPort , MI_U32 u32SrcFrmrate,  MI_U32 u32DstFrmrate)
{
    DBG_INFO("src_fps:%u dst_fps:%u\n", u32SrcFrmrate, u32DstFrmrate);
    ExecFunc(MI_SYS_SetChnOutputPortDepth(pstSrcChnPort, 0, 4), MI_SUCCESS);
    ExecFunc(MI_SYS_BindChnPort(pstSrcChnPort, pstDstChnPort, u32SrcFrmrate, u32DstFrmrate), MI_SUCCESS);
    return 0;
}

void mixer_util_set_debug_level(MIXER_DBG_LEVEL_e debug_level)
{
    printf("mixer set debug_level %d\n", debug_level);
    mixer_debug_level = debug_level;
}

void mixer_util_set_func_trace(MI_BOOL bTrace)
{
    printf("mixer set func trace %d\n", bTrace);
    mixer_func_trace = bTrace;
}
