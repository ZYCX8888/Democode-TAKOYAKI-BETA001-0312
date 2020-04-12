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
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/prctl.h>

#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/resource.h>

#include<sys/mman.h>
#include<sys/types.h>
#include<sys/stat.h>

#include "mi_sys.h"
#include "mi_vdec.h"
#include "mi_vpe_datatype.h"
#include "mi_rgn.h"
#include "mi_divp.h"
#include "mi_divp_datatype.h"
#include "mi_vif.h"
#include "mi_vif_datatype.h"
#include "mi_vdisp.h"
#include "mi_vdisp_datatype.h"
#include "mi_hdmi.h"
#include "mi_hdmi_datatype.h"
#include "mi_disp.h"
#include "mi_disp_datatype.h"

#define TEST_WIDTH 1280
#define TEST_HEIGHT 720
#define TEST_MAX_CHN 8

#define STCHECKRESULT(result)\
    if (result != MI_SUCCESS)\
    {\
        printf("[%s %d]exec function failed\n", __FUNCTION__, __LINE__);\
        return 1;\
    }\
    else\
    {\
        printf("(%s %d)exec function pass\n", __FUNCTION__,__LINE__);\
    }
#define ExecFunc(_func_, _ret_) \
        if (_func_ != _ret_)\
        {\
            printf("Test [%d]exec function failed\n", __LINE__);\
            return 1;\
        }\
        else\
        {\
            printf("Test [%d]exec function pass\n", __LINE__);\
        }
typedef struct Rect_s
{
    MI_S32 s32X;
    MI_S32 s32Y;
    MI_U16 u16PicW;
    MI_U16 u16PicH;
} Rect_t;

int main(int argc, char **argv)
{
    MI_S32 s32Channel = 0;
    MI_S32 i = 0;
    MI_U8 InputCmd[256];
    MI_DIVP_ChnAttr_t stDivpChnAttr;
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;
    MI_VDEC_ChnAttr_t stChnAttr;
    const Rect_t stDispRectx9[9] = {
        {0, 0, 640, 360}, {640, 0, 640, 360}, {1280, 0, 640, 360},
        {0, 360, 640, 360}, {640, 360, 640, 360}, {1280, 360, 640, 360},
        {0, 720, 640, 360}, {640, 720, 640, 360}, {1280, 720, 640, 360}};

    STCHECKRESULT(MI_SYS_Init());

    while (1)
    {
        fgets((char *)(InputCmd), (sizeof(InputCmd) - 1), stdin);
        if (strncmp(InputCmd, "exit", 4) == 0)
        {
            printf("prepare to exit!\n\n");
            break;
        }
        else if (strncmp(InputCmd, "vdec", 4) == 0)
        {
            //Vdec Create Chn
            for (i = 0; i < TEST_MAX_CHN; i++)
            {
                stChnAttr.stVdecVideoAttr.u32RefFrameNum = 2;
                stChnAttr.eVideoMode = E_MI_VDEC_VIDEO_MODE_FRAME;
                stChnAttr.u32BufSize = 512 * 1024;
                stChnAttr.u32PicWidth = 720;
                stChnAttr.u32PicHeight = 576;
                stChnAttr.u32Priority = 0;
                stChnAttr.eCodecType = E_MI_VDEC_CODEC_TYPE_H264;
                STCHECKRESULT(MI_VDEC_CreateChn(i, &stChnAttr));
                STCHECKRESULT(MI_VDEC_StartChn(i));
            }
            for (i = 0; i < TEST_MAX_CHN; i++)
            {
                STCHECKRESULT(MI_VDEC_StopChn(i));
                STCHECKRESULT(MI_VDEC_DestroyChn(i));
            }
            break;
        }
        else if (strncmp(InputCmd, "divp", 4) == 0)
        {
            //Divp Create Chn
            for (i = 0; i < TEST_MAX_CHN; i++)
            {
                stDivpChnAttr.bHorMirror = FALSE;
                stDivpChnAttr.bVerMirror = FALSE;
                stDivpChnAttr.eDiType = E_MI_DIVP_DI_TYPE_OFF;
                stDivpChnAttr.eRotateType = E_MI_SYS_ROTATE_NONE;
                stDivpChnAttr.eTnrLevel = E_MI_DIVP_TNR_LEVEL_OFF;
                stDivpChnAttr.stCropRect.u16X = 0;
                stDivpChnAttr.stCropRect.u16Y = 0;
                stDivpChnAttr.stCropRect.u16Width = 1024; //Vdec pic w
                stDivpChnAttr.stCropRect.u16Height = 768; //Vdec pic h
                stDivpChnAttr.u32MaxWidth = 1920; //max size picture can pass
                stDivpChnAttr.u32MaxHeight = 1080;
                STCHECKRESULT(MI_DIVP_CreateChn(i, &stDivpChnAttr));

                STCHECKRESULT(MI_DIVP_GetChnAttr(i, &stDivpChnAttr));
                STCHECKRESULT(MI_DIVP_SetChnAttr(i, &stDivpChnAttr));
                STCHECKRESULT(MI_DIVP_StartChn(i));
                memset(&stOutputPortAttr, 0, sizeof(stOutputPortAttr));
                stOutputPortAttr.eCompMode = E_MI_SYS_COMPRESS_MODE_NONE;
                stOutputPortAttr.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
                stOutputPortAttr.u32Width = 1024;
                stOutputPortAttr.u32Height = 768;

                STCHECKRESULT(MI_DIVP_SetOutputPortAttr(i, &stOutputPortAttr));
            }
            for (i = 0; i < TEST_MAX_CHN; i++)
            {
                STCHECKRESULT(MI_DIVP_StopChn(i));
                STCHECKRESULT(MI_DIVP_DestroyChn(i));
            }
            break;
        }
        else if (strncmp(InputCmd, "vif", 3) == 0)
        {
            MI_VIF_ChnPortAttr_t stChnPortAttr;
            MI_SYS_ChnPort_t stChnPort;
            MI_VIF_DevAttr_t stDevAttr;

            stDevAttr.eIntfMode = E_MI_VIF_MODE_BT1120_STANDARD;
            stDevAttr.eWorkMode = E_MI_VIF_WORK_MODE_1MULTIPLEX;
            stDevAttr.au32CompMask[0] = 0xFF000000;
            stDevAttr.au32CompMask[1] = 0xFF0000;
            stDevAttr.eClkEdge = E_MI_VIF_CLK_EDGE_SINGLE_UP;
            stDevAttr.as32AdChnId[0] = -1;
            stDevAttr.as32AdChnId[1] = -1;
            stDevAttr.as32AdChnId[2] = -1;
            stDevAttr.as32AdChnId[3] = -1;
            STCHECKRESULT(MI_VIF_SetDevAttr(0, &stDevAttr));
            STCHECKRESULT(MI_VIF_EnableDev(0)); //Create vif device

            for (i = 0; i < 4; i++) //test dev0, chn 0-3
            {
                stChnPortAttr.stCapRect.u16X = 0;
                stChnPortAttr.stCapRect.u16Y = 0;
                stChnPortAttr.stCapRect.u16Width = TEST_WIDTH;
                stChnPortAttr.stCapRect.u16Height = TEST_HEIGHT;
                stChnPortAttr.stDestSize.u16Width = TEST_WIDTH;
                stChnPortAttr.stDestSize.u16Height = TEST_HEIGHT;
                stChnPortAttr.eCapSel = E_MI_SYS_FIELDTYPE_BOTH;
                stChnPortAttr.eScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
                stChnPortAttr.ePixFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
                stChnPortAttr.bMirror = FALSE;
                stChnPortAttr.bFlip = FALSE;
                stChnPortAttr.eFrameRate = E_MI_VIF_FRAMERATE_FULL;
                STCHECKRESULT(MI_VIF_SetChnPortAttr(i, 0, &stChnPortAttr));

                STCHECKRESULT(MI_VIF_EnableChnPort(i, 0));
            }
            for (i = 0; i < 4; i++)
            {
                STCHECKRESULT(MI_VIF_DisableChnPort(i, 0));
            }
            STCHECKRESULT(MI_VIF_DisableDev(0));
            break;
        }
        else if (strncmp(InputCmd, "vdisp", 5) == 0)
        {
            MI_VDISP_InputPortAttr_t stInputPortAttr;
            MI_SYS_ChnPort_t stChnPort;
            MI_VDISP_OutputPortAttr_t stOutputPortAttr;

            STCHECKRESULT(MI_VDISP_Init());
            STCHECKRESULT(MI_VDISP_OpenDevice(0));

            for (i = 0; i < TEST_MAX_CHN; i++) //8 chn in
            {
                stInputPortAttr.s32IsFreeRun = 1;
                stInputPortAttr.u32OutX = stDispRectx9[i].s32X;
                stInputPortAttr.u32OutY = stDispRectx9[i].s32Y;
                stInputPortAttr.u32OutWidth = stDispRectx9[i].u16PicW;
                stInputPortAttr.u32OutHeight = stDispRectx9[i].u16PicH;
                STCHECKRESULT(MI_VDISP_SetInputPortAttr(0, i, &stInputPortAttr));
            }
            stOutputPortAttr.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
            stOutputPortAttr.u32BgColor = 0x0;
            stOutputPortAttr.u32FrmRate = 25;
            stOutputPortAttr.u32Width = 1920;
            stOutputPortAttr.u32Height = 1080;
            stOutputPortAttr.u64pts = 0;
            STCHECKRESULT(MI_VDISP_SetOutputPortAttr(0, 0, &stOutputPortAttr));
            stChnPort.eModId = E_MI_MODULE_ID_VDISP;
            stChnPort.u32DevId = 0;
            stChnPort.u32ChnId = 0;
            stChnPort.u32PortId = 0;
            STCHECKRESULT(MI_SYS_SetChnOutputPortDepth(&stChnPort, 2, 10));
            STCHECKRESULT(MI_VDISP_StartDev(0));

            for (i = 0; i < TEST_MAX_CHN; i++)
            {
                STCHECKRESULT(MI_VDISP_DisableInputPort(0, i));
            }

            STCHECKRESULT(MI_VDISP_StopDev(0));
            STCHECKRESULT(MI_VDISP_CloseDevice(0));
            STCHECKRESULT(MI_VDISP_Exit());
            break;
        }
        else if (strncmp(InputCmd, "vpe", 3) == 0)
        {
            MI_VPE_ChannelAttr_t stChannelVpssAttr;
            MI_SYS_WindowRect_t stCropWin;
            MI_VPE_PortMode_t stVpeMode;

            for (i = 0; i < TEST_MAX_CHN; i++)
            {
                stChannelVpssAttr.u16MaxW = TEST_WIDTH;
                stChannelVpssAttr.u16MaxH = TEST_HEIGHT;
                stChannelVpssAttr.bNrEn= FALSE;
                stChannelVpssAttr.bEdgeEn= FALSE;
                stChannelVpssAttr.bEsEn= FALSE;
                stChannelVpssAttr.bContrastEn= FALSE;
                stChannelVpssAttr.bUvInvert= FALSE;
                stChannelVpssAttr.ePixFmt = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
                STCHECKRESULT(MI_VPE_CreateChannel(i, &stChannelVpssAttr));
                STCHECKRESULT(MI_VPE_GetChannelAttr(i, &stChannelVpssAttr));
                stChannelVpssAttr.bContrastEn = TRUE;
                stChannelVpssAttr.bNrEn = TRUE;
                STCHECKRESULT(MI_VPE_SetChannelAttr(i, &stChannelVpssAttr));
                STCHECKRESULT(MI_VPE_GetChannelCrop(i, &stCropWin));
                stCropWin.u16X = 0;
                stCropWin.u16Y = 0;
                stCropWin.u16Width = TEST_WIDTH;
                stCropWin.u16Height = TEST_HEIGHT; //output size???

                STCHECKRESULT(MI_VPE_SetChannelCrop(i, &stCropWin));
                memset(&stVpeMode, 0, sizeof(stVpeMode));
                STCHECKRESULT(MI_VPE_GetPortMode(i, 0, &stVpeMode));//test port 0
                stVpeMode.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
                stVpeMode.u16Width = TEST_WIDTH;
                stVpeMode.u16Height= TEST_HEIGHT;
                STCHECKRESULT(MI_VPE_SetPortMode(i, 0, &stVpeMode));//test port 0
                STCHECKRESULT(MI_VPE_EnablePort(i, 0));
            }
            for (i = 0; i < TEST_MAX_CHN; i++)
            {
                STCHECKRESULT(MI_VPE_DisablePort(i, 0));//test port 0
                //STCHECKRESULT(MI_VPE_StopChannel(i));
                STCHECKRESULT(MI_VPE_DestroyChannel(i));
            }
            break;
        }
        else if (strncmp(InputCmd, "disp", 4) == 0)
        {
            MI_DISP_DEV DispDev = 0;
            MI_DISP_PubAttr_t stPubAttr;

            MI_DISP_LAYER DispLayer = 0;
            MI_U32 u32Toleration = 1000;
            MI_DISP_VideoLayerAttr_t stLayerAttr;
            MI_S32 s32InputPort = 0;
            MI_S32 InputPortNum = 1; //test use 0

            MI_DISP_InputPortAttr_t stInputPortAttr;

            memset(&stPubAttr, 0, sizeof(stPubAttr));
            stPubAttr.eIntfSync = E_MI_DISP_OUTPUT_1080P60;
            stPubAttr.eIntfType = E_MI_DISP_INTF_HDMI;
            ExecFunc(MI_DISP_SetPubAttr(DispDev, &stPubAttr), MI_SUCCESS);
            ExecFunc(MI_DISP_Enable(DispDev), MI_SUCCESS);

            memset(&stLayerAttr, 0, sizeof(stLayerAttr));

            stLayerAttr.stVidLayerSize.u16Width  = TEST_WIDTH;
            stLayerAttr.stVidLayerSize.u16Height = TEST_HEIGHT;

            stLayerAttr.stVidLayerDispWin.u16X      = 0;
            stLayerAttr.stVidLayerDispWin.u16Y      = 0;
            stLayerAttr.stVidLayerDispWin.u16Width  = 1920;
            stLayerAttr.stVidLayerDispWin.u16Height = 1080;

            ExecFunc(MI_DISP_SetVideoLayerAttr(DispLayer, &stLayerAttr), MI_SUCCESS);
            ExecFunc(MI_DISP_GetVideoLayerAttr(DispLayer, &stLayerAttr), MI_SUCCESS);
            ExecFunc(MI_DISP_BindVideoLayer(DispLayer, DispDev), MI_SUCCESS);
            ExecFunc(MI_DISP_EnableVideoLayer(DispLayer), MI_SUCCESS);

            for (s32InputPort = 0; s32InputPort < InputPortNum; s32InputPort++)
            {
                memset(&stInputPortAttr, 0, sizeof(stInputPortAttr));
                ExecFunc(MI_DISP_GetInputPortAttr(DispLayer, s32InputPort, &stInputPortAttr), MI_SUCCESS);
                stInputPortAttr.stDispWin.u16X      = stDispRectx9[s32InputPort].s32X;
                stInputPortAttr.stDispWin.u16Y      = stDispRectx9[s32InputPort].s32Y;
                stInputPortAttr.stDispWin.u16Width  = stDispRectx9[s32InputPort].u16PicW;
                stInputPortAttr.stDispWin.u16Height = stDispRectx9[s32InputPort].u16PicH;

                ExecFunc(MI_DISP_SetInputPortAttr(DispLayer, s32InputPort, &stInputPortAttr), MI_SUCCESS);
                ExecFunc(MI_DISP_GetInputPortAttr(DispLayer, s32InputPort, &stInputPortAttr), MI_SUCCESS);
                ExecFunc(MI_DISP_EnableInputPort(DispLayer, s32InputPort), MI_SUCCESS);
            }
            sleep(1);
            for (s32InputPort = 0; s32InputPort < InputPortNum; s32InputPort++)
            {
                ExecFunc(MI_DISP_DisableInputPort(DispLayer, s32InputPort), MI_SUCCESS);
            }
            ExecFunc(MI_DISP_DisableVideoLayer(DispLayer), MI_SUCCESS);
            ExecFunc(MI_DISP_UnBindVideoLayer(DispLayer, DispDev), MI_SUCCESS);
            ExecFunc(MI_DISP_Disable(DispDev), MI_SUCCESS);
            break;
        }
        else if (strncmp(InputCmd, "hdmi", 4) == 0)
        {
            MI_HDMI_InitParam_t stInitParam;
            MI_HDMI_DeviceId_e eHdmi = E_MI_HDMI_ID_0;
            MI_HDMI_Attr_t stAttr;

            stInitParam.pCallBackArgs = NULL;
            stInitParam.pfnHdmiEventCallback = NULL;

            ExecFunc(MI_HDMI_Init(&stInitParam), MI_SUCCESS);
            ExecFunc(MI_HDMI_Open(eHdmi), MI_SUCCESS);

            memset(&stAttr, 0, sizeof(MI_HDMI_Attr_t));
            stAttr.stEnInfoFrame.bEnableAudInfoFrame  = FALSE;
            stAttr.stEnInfoFrame.bEnableAviInfoFrame  = FALSE;
            stAttr.stEnInfoFrame.bEnableSpdInfoFrame  = FALSE;
            stAttr.stAudioAttr.bEnableAudio = TRUE;
            stAttr.stAudioAttr.bIsMultiChannel = 0;
            stAttr.stAudioAttr.eBitDepth = E_MI_HDMI_BIT_DEPTH_16;
            stAttr.stAudioAttr.eCodeType = E_MI_HDMI_ACODE_PCM;
            stAttr.stAudioAttr.eSampleRate = E_MI_HDMI_AUDIO_SAMPLERATE_48K;
            stAttr.stVideoAttr.bEnableVideo = TRUE;
            stAttr.stVideoAttr.eColorType = E_MI_HDMI_COLOR_TYPE_YCBCR444;//default color type
            stAttr.stVideoAttr.eDeepColorMode = E_MI_HDMI_DEEP_COLOR_MAX;
            stAttr.stVideoAttr.eTimingType = E_MI_HDMI_TIMING_720_60P;
            stAttr.stVideoAttr.eOutputMode = E_MI_HDMI_OUTPUT_MODE_HDMI;
            ExecFunc(MI_HDMI_SetAttr(eHdmi, &stAttr), MI_SUCCESS);
            ExecFunc(MI_HDMI_Start(eHdmi), MI_SUCCESS);
            sleep(1);
            ExecFunc(MI_HDMI_Stop(eHdmi), MI_SUCCESS);
            ExecFunc(MI_HDMI_Close(eHdmi), MI_SUCCESS);
            ExecFunc(MI_HDMI_DeInit(), MI_SUCCESS);
            break;
        }
    }

    STCHECKRESULT(MI_SYS_Exit());

    return 0;
}
