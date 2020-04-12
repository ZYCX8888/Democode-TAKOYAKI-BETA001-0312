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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#include "mi_venc.h"
#include "../mi_vpe_test.h"
#include "../st_vif.h"
#include "../i2c.h"
#define MAX_TEST_CHANNEL (16)
#define VPE_Port_VENC 0
#define VPE_OUTPORT 3
#define ALIGN_N(x, align)           (((x) + ((align) - 1)) & ~((align) - 1))

#if (VPE_OUTPORT == 3)
MI_SYS_PixelFormat_e ePixel = E_MI_SYS_PIXEL_FRAME_YUV_MST_420;
#else
MI_SYS_PixelFormat_e ePixel = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
#endif
MI_SYS_WindowRect_t stDispPortWin[6]={
{0,0,1280,720},
{1280,0,640,360},
{1280,360,640,360},
{0,720,640,360},
{640,720,640,360},
{1280,720,640,360}
    };


static test_vpe_Config stTest002[] = {
    {
        .inputFile  = TEST_VPE_CHNN_FILE420(002, 0, 1920x1080),
        .stSrcWin   = {0, 0, 1920, 1080},
        .stCropWin   = {0, 0, 1920, 1080},
        .stOutPort  = {
            [VPE_Port_VENC] = {
                .bEnable    = TRUE,
            },
            [VPE_OUTPORT] = {
                .bEnable    = TRUE,
            },
        },
    },
    {
        .inputFile = TEST_VPE_CHNN_FILE420(002, 0, 1920x1080),
            .stSrcWin   = {0, 0, 1920, 1080},
            .stCropWin   = {0, 0, 1920, 1080},
        .stOutPort  = {
             [VPE_Port_VENC] = {
                .bEnable    = TRUE,
            },
            [VPE_OUTPORT] = {
                .bEnable    = TRUE,
            },
        },
    },
    {
        .inputFile = TEST_VPE_CHNN_FILE420(002, 0, 1920x1080),
        .stSrcWin   = {0, 0, 1920, 1080},
        .stCropWin  = {0, 0, 1920, 1080},
        .stOutPort  = {
            [VPE_Port_VENC] = {
                .bEnable    = TRUE,
            },
            [VPE_OUTPORT] = {
                .bEnable    = TRUE,
            },
        },
    },
    {
        .inputFile = TEST_VPE_CHNN_FILE420(002, 0, 1920x1080),
        .stSrcWin   = {0, 0, 1920, 1080},
        .stCropWin  = {0, 0, 1920, 1080},
        .stOutPort  = {
            [VPE_Port_VENC] = {
                .bEnable    = TRUE,
            },
            [VPE_OUTPORT] = {
                .bEnable    = TRUE,
            },
        },
    },
        {
        .inputFile  = TEST_VPE_CHNN_FILE420(002, 0, 1920x1080),
        .stSrcWin   = {0, 0, 1920, 1080},
        .stCropWin   = {0, 0, 1920, 1080},
        .stOutPort  = {
            [VPE_Port_VENC] = {
                .bEnable    = TRUE,
            },
            [VPE_OUTPORT] = {
                .bEnable    = TRUE,
            },
        },
    },
    {
        .inputFile = TEST_VPE_CHNN_FILE420(002, 0, 1920x1080),
            .stSrcWin   = {0, 0, 1920, 1080},
            .stCropWin   = {0, 0, 1920, 1080},
        .stOutPort  = {
             [VPE_Port_VENC] = {
                .bEnable    = TRUE,
            },
            [VPE_OUTPORT] = {
                .bEnable    = TRUE,
            },
        },
    },
    {
        .inputFile = TEST_VPE_CHNN_FILE420(002, 0, 1920x1080),
        .stSrcWin   = {0, 0, 1920, 1080},
        .stCropWin  = {0, 0, 1920, 1080},
        .stOutPort  = {
            [VPE_Port_VENC] = {
                .bEnable    = TRUE,
            },
            [VPE_OUTPORT] = {
                .bEnable    = TRUE,
            },
        },
    },
    {
        .inputFile = TEST_VPE_CHNN_FILE420(002, 0, 1920x1080),
        .stSrcWin   = {0, 0, 1920, 1080},
        .stCropWin  = {0, 0, 1920, 1080},
        .stOutPort  = {
            [VPE_Port_VENC] = {
                .bEnable    = TRUE,
            },
            [VPE_OUTPORT] = {
                .bEnable    = TRUE,
            },
        },
    },
};

int vpe_creat_channel(MI_U8 VpeChannel, MI_SYS_PixelFormat_e eInPixel, MI_SYS_WindowRect_t *pstCropWin)
{
    MI_VPE_ChannelAttr_t stChannelVpssAttr;
    MI_SYS_WindowRect_t stCropWin;
    stChannelVpssAttr.u16MaxW = 1920;
    stChannelVpssAttr.u16MaxH = 1080;
    stChannelVpssAttr.bNrEn= FALSE;
    stChannelVpssAttr.bEdgeEn= FALSE;
    stChannelVpssAttr.bEsEn= FALSE;
    stChannelVpssAttr.bContrastEn= FALSE;
    stChannelVpssAttr.bUvInvert= FALSE;
    stChannelVpssAttr.ePixFmt = eInPixel;
    stChannelVpssAttr.eRunningMode = E_MI_VPE_RUN_DVR_MODE;
    ExecFunc(MI_VPE_CreateChannel(VpeChannel, &stChannelVpssAttr), MI_VPE_OK);

    ExecFunc(MI_VPE_GetChannelAttr(VpeChannel, &stChannelVpssAttr), MI_VPE_OK);

    stChannelVpssAttr.bContrastEn = FALSE;
    stChannelVpssAttr.bNrEn = FALSE;
    ExecFunc(MI_VPE_SetChannelAttr(VpeChannel, &stChannelVpssAttr), MI_VPE_OK);

    ExecFunc(MI_VPE_GetChannelCrop(VpeChannel, &stCropWin), MI_VPE_OK);
    stCropWin.u16X = pstCropWin->u16X;
    stCropWin.u16Y = pstCropWin->u16Y;
    stCropWin.u16Width = pstCropWin->u16Width;
    stCropWin.u16Height = pstCropWin->u16Height;
    ExecFunc(MI_VPE_SetChannelCrop(VpeChannel, &stCropWin), MI_VPE_OK);
}

int vpe_create_port(MI_U8 VpeChannel, MI_U8 VpePort, MI_SYS_PixelFormat_e eOutPixel, MI_SYS_WindowRect_t *pstPortWin)
{
    MI_VPE_PortMode_t stVpeMode;
    memset(&stVpeMode, 0, sizeof(stVpeMode));
    ExecFunc(MI_VPE_GetPortMode(VpeChannel, VpePort, &stVpeMode), MI_VPE_OK);
    stVpeMode.ePixelFormat = eOutPixel;
    stVpeMode.u16Width = pstPortWin->u16Width;
    stVpeMode.u16Height= pstPortWin->u16Height;
    ExecFunc(MI_VPE_SetPortMode(VpeChannel, VpePort, &stVpeMode), MI_VPE_OK);
    ExecFunc(MI_VPE_EnablePort(VpeChannel, VpePort), MI_VPE_OK);
}

MI_S32 mi_disp_hdmiInit(void)
{
    MI_HDMI_InitParam_t stInitParam;
    MI_HDMI_Attr_t stAttr;
    MI_HDMI_DeviceId_e eHdmi = E_MI_HDMI_ID_0;

    stInitParam.pCallBackArgs = NULL;
    stInitParam.pfnHdmiEventCallback = NULL;

    MI_HDMI_Init(&stInitParam);

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
    stAttr.stVideoAttr.eColorType = E_MI_HDMI_COLOR_TYPE_RGB444;//default color type
    stAttr.stVideoAttr.eDeepColorMode = E_MI_HDMI_DEEP_COLOR_MAX;
    stAttr.stVideoAttr.eTimingType = E_MI_HDMI_TIMING_1080_60P;
    stAttr.stVideoAttr.eOutputMode = E_MI_HDMI_OUTPUT_MODE_HDMI;
    MI_HDMI_SetAttr(eHdmi, &stAttr);

    MI_HDMI_Start(eHdmi);
    return MI_SUCCESS;
}

int bind(MI_U8 SrcChnl,MI_U8 SrcPort,MI_U8 u8DevId, MI_U8 DstChn,MI_U8 DstPort, MI_BOOL Flag)
{
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32SrcFrmrate;
    MI_U32 u32DstFrmrate;

    stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stSrcChnPort.u32DevId = 0;
    stSrcChnPort.u32ChnId = SrcChnl;
    stSrcChnPort.u32PortId = SrcPort;

    stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
    stDstChnPort.u32DevId = u8DevId;
    stDstChnPort.u32ChnId = DstChn;
    stDstChnPort.u32PortId = DstPort;

    u32SrcFrmrate = 30;
    u32DstFrmrate = 30;

    if(Flag == TRUE)
    {
        ExecFunc(MI_SYS_BindChnPort(&stSrcChnPort, &stDstChnPort, u32SrcFrmrate, u32DstFrmrate), MI_SUCCESS);
    }
    else if(Flag == FALSE)
    {
        ExecFunc(MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort), MI_SUCCESS);
    }
}

void *getEsBuffer(void *args)
{
        MI_SYS_ChnPort_t stVencChnInputPort;
        MI_U32 u32DevId;
        MI_SYS_BUF_HANDLE hHandle;
        MI_SYS_BufInfo_t stBufInfo;
        MI_U8 u8VencChn = *((MI_U8 *)args);
        MI_VENC_GetChnDevid(u8VencChn, &u32DevId);
        stVencChnInputPort.eModId = E_MI_MODULE_ID_VENC;
        stVencChnInputPort.u32ChnId = u8VencChn;
        stVencChnInputPort.u32DevId = u32DevId;
        stVencChnInputPort.u32PortId = 0;

        hHandle = MI_HANDLE_NULL;
        memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));

        if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVencChnInputPort, &stBufInfo, &hHandle))
        {
            if(hHandle == MI_HANDLE_NULL)
            {
                printf("%s %d NULL output port buffer handle.\n", __func__, __LINE__);
            }
            else if(stBufInfo.stRawData.pVirAddr == NULL)
            {
                printf("%s %d unable to read buffer. VA==0\n", __func__, __LINE__);
            }
            else if(stBufInfo.stRawData.u32ContentSize >= 200 * 1024)  //MAX_OUTPUT_ES_SIZE in KO
            {
                printf("%s %d unable to read buffer. buffer overflow\n", __func__, __LINE__);
            }

            write(stTest002[u8VencChn].stOutPort[VPE_Port_VENC].dest_fd, stBufInfo.stRawData.pVirAddr, stBufInfo.stRawData.u32ContentSize);

            //printf("%s %d, chn:%d write frame len=%d, real len=%d\n", __func__, __LINE__, stVencChnInputPort.u32ChnId,
            //    len, stBufInfo.stRawData.u32ContentSize);

            MI_SYS_ChnOutputPortPutBuf(hHandle);
        }
}
//int test_vpe_TestCase002_main(int argc, const char *argv[])
int main(int argc, const char *argv[])
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_U32 i = 0;
    int j = 0;
    pthread_t ptGetEs[6];
    MI_U32 u32ChannelNum = 0;
    MI_U32 u32ChannelNum_tmp = 0;
    MI_U8 DispDev=0;
    MI_U8 DispLayer=0;
    MI_SYS_ChnPort_t stVpeChnInputPort0;
    MI_SYS_ChnPort_t stVpeChnOutputPort0;
    MI_U32 count = 0;
    struct timeval stTime;
    char src_file[256];

    const char *pbaseDir = NULL;
    int src_fd;
    MI_SYS_FrameData_t framedata;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    int frame_size = 0;
    int width  = 0;
    int height = 0;
    int y_size = 0;
    int uv_size = 0;
    struct timeval stTv;
    memset(&framedata, 0, sizeof(framedata));
    memset(&stTime, 0, sizeof(stTime));

    if (argc < 4)
    {
        printf("%s <ChannelNum> <DispDev> <DispLayer>.\n", argv[0]);
        printf("%s.\n", VPE_TEST_002_DESC);
//        for (i = 0; i < sizeof(stTest002)/sizeof(stTest002[0]); i++)
        for (i = 0; i < 4; i++)
        {
            printf("Channel[%d][%d][%d][%d]:\n", i, 4+i, 8+i, 12+i);
            printf("InputFile: %s.\n", stTest002[i].inputFile);
            //printf("OutputFile: %s.\n", stTest002[i].stOutPort[0].outputFile);
        }
        return 0;
    }

    u32ChannelNum = atoi(argv[1]);
    DispDev = atoi(argv[2]);
    DispLayer = atoi(argv[3]);

    printf("%s %s %d %d %d %d.\n", argv[0], pbaseDir, count,u32ChannelNum,DispDev, DispLayer);

    for (i = 0; i < u32ChannelNum; i++)
    {
        //sprintf(src_file, "%s/%s", pbaseDir, stTest002[i].inputFile);
        //ExecFunc(test_vpe_OpenSourceFile(src_file, &stTest002[i].src_fd), TRUE);

        stTest002[i].count = 0;
        stTest002[i].src_offset = 0;
        stTest002[i].src_count  = 0;
        stTest002[i].product = 0;
    }

    // init MI_SYS
    ExecFunc(test_vpe_SysEnvInit(), TRUE);
    ExecFunc(vif_i2c_init(), 0);

    // Create VPE channel
    MI_U8 u8Canvs = 0;
    MI_U32 layerwidth = 0;
    MI_U32 layerHeight = 0;
    MI_U32 u16VpeOutportWidth = 0,u16VpeOutportHeight = 0;
    if(u32ChannelNum <= 1)
        u8Canvs = 1;
    else if(u32ChannelNum <= 4)
        u8Canvs = 2;
    else if(u32ChannelNum <= 9)
        u8Canvs = 3;
    else if(u32ChannelNum <= 16)
        u8Canvs = 4;

    MI_DISP_PubAttr_t stPubAttr;
    memset(&stPubAttr, 0, sizeof(stPubAttr));

    if(0 == DispDev)
    {
        layerwidth = 1920;
        layerHeight = 1080;
        stPubAttr.eIntfSync = E_MI_DISP_OUTPUT_1080P60;
        stPubAttr.eIntfType = E_MI_DISP_INTF_HDMI;
    }
    else if(1 == DispDev)
    {
        layerwidth = 1600;
        layerHeight = 1200;
        stPubAttr.eIntfSync = E_MI_DISP_OUTPUT_1600x1200_60;
        stPubAttr.eIntfType = E_MI_DISP_INTF_VGA;
    }

    u16VpeOutportWidth = (layerwidth / u8Canvs)/2*2;
    u16VpeOutportHeight = (layerHeight / u8Canvs)/2*2;

    for(i = 0; i < u32ChannelNum/2; i++)
        ST_Vif_CreateDev(i, SAMPLE_VI_MODE_2X_DOUBLE_1080P);

    for (i = 0; i < u32ChannelNum; i++)
    {
        ST_VIF_PortInfo_t stVifPortInfoInfo;
        memset(&stVifPortInfoInfo, 0x0, sizeof(ST_VIF_PortInfo_t));
        stVifPortInfoInfo.u32RectX = 0;
        stVifPortInfoInfo.u32RectY = 0;
        stVifPortInfoInfo.u32RectWidth = 1920;
        stVifPortInfoInfo.u32RectHeight = 1080;
        stVifPortInfoInfo.u32DestWidth = 1920;
        stVifPortInfoInfo.u32DestHeight = 1080;
        ST_Vif_CreatePort(i*2, 0, &stVifPortInfoInfo);

        vpe_creat_channel(i,E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, &stTest002[i].stCropWin);
        for (j = 0; j < 4; j++)
        {
            if (stTest002[i].stOutPort[j].bEnable == TRUE)
            {
                MI_SYS_PixelFormat_e eOutPixel;
                if(j == VPE_OUTPORT)
                {
                    if(i == 0)
                    {
                        stTest002[i].stOutPort[j].stPortWin.u16Width = 1280;
                        stTest002[i].stOutPort[j].stPortWin.u16Height = 720;
                    }
                    else
                    {
                        stTest002[i].stOutPort[j].stPortWin.u16Width = 640;
                        stTest002[i].stOutPort[j].stPortWin.u16Height = 360;
                    }
                    eOutPixel = ePixel;
                }
                else if(j == VPE_Port_VENC)
                {

                    stTest002[i].stOutPort[j].stPortWin.u16Width = 1920;
                    stTest002[i].stOutPort[j].stPortWin.u16Height = 1080;
                    eOutPixel = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
                }

                vpe_create_port(i, j, eOutPixel, &stTest002[i].stOutPort[j].stPortWin);
                // set vpe port buffer depth
                stVpeChnOutputPort0.eModId = E_MI_MODULE_ID_VPE;
                stVpeChnOutputPort0.u32DevId = 0;
                stVpeChnOutputPort0.u32ChnId = i;
                stVpeChnOutputPort0.u32PortId = j;
                MI_SYS_SetChnOutputPortDepth(&stVpeChnOutputPort0, 0, 3);
            }
        }
        ExecFunc(MI_VPE_StartChannel (i), MI_VPE_OK);
    }

       ExecFunc(MI_DISP_SetPubAttr(DispDev,  &stPubAttr), MI_DISP_SUCCESS);
       ExecFunc(MI_DISP_Enable(DispDev), MI_DISP_SUCCESS);

       mi_disp_hdmiInit();
       MI_HDMI_Attr_t stHdmiAttr;
       MI_HDMI_DeviceId_e eHdmi = 0;
       MI_HDMI_GetAttr(eHdmi,&stHdmiAttr);
       stHdmiAttr.stVideoAttr.eTimingType = E_MI_HDMI_TIMING_1080_60P;
       MI_HDMI_SetAttr(eHdmi,&stHdmiAttr);

       MI_U32 u32Toleration = 100000;
       MI_DISP_CompressAttr_t stCompressAttr;
       MI_DISP_VideoLayerAttr_t stLayerAttr, stLayerAttrBackup;
       memset(&stLayerAttr, 0, sizeof(stLayerAttr));

       stLayerAttr.stVidLayerSize.u16Width = layerwidth;
       stLayerAttr.stVidLayerSize.u16Height= layerHeight;
       stLayerAttr.stVidLayerDispWin.u16X = 0;
       stLayerAttr.stVidLayerDispWin.u16Y = 0;
       stLayerAttr.stVidLayerDispWin.u16Width = layerwidth;
       stLayerAttr.stVidLayerDispWin.u16Height = layerHeight;
       stLayerAttr.ePixFormat = ePixel;
       ExecFunc(MI_DISP_BindVideoLayer(DispLayer, DispDev), MI_DISP_SUCCESS);
       ExecFunc(MI_DISP_SetVideoLayerAttr(DispLayer, &stLayerAttr), MI_DISP_SUCCESS);
       ExecFunc(MI_DISP_GetVideoLayerAttr(DispLayer, &stLayerAttr), MI_DISP_SUCCESS);
       ExecFunc(MI_DISP_EnableVideoLayer(DispLayer), MI_DISP_SUCCESS);

        {
            MI_U8 i=0;

            for(i = 0; i<u32ChannelNum;i++)
            {
                MI_DISP_InputPortAttr_t stInputPortAttr;
                 MI_DISP_INPUTPORT LayerInputPort = i;
                memset(&stInputPortAttr, 0, sizeof(stInputPortAttr));

                ExecFunc(MI_DISP_GetInputPortAttr(DispLayer, LayerInputPort, &stInputPortAttr), MI_DISP_SUCCESS);

                stInputPortAttr.stDispWin.u16X = stDispPortWin[i].u16X;
                stInputPortAttr.stDispWin.u16Y = stDispPortWin[i].u16Y;
                stInputPortAttr.stDispWin.u16Width = stDispPortWin[i].u16Width;
                stInputPortAttr.stDispWin.u16Height =stDispPortWin[i].u16Height;

                ExecFunc(MI_DISP_SetInputPortAttr(DispLayer, LayerInputPort, &stInputPortAttr), MI_DISP_SUCCESS);
                ExecFunc(MI_DISP_GetInputPortAttr(DispLayer, LayerInputPort, &stInputPortAttr), MI_DISP_SUCCESS);
                MI_VPE_TEST_INFO("Dev: %d layer: %d Port[%d]: {%d, %d, %d, %d}.\n", 0, DispLayer, LayerInputPort,
                    stInputPortAttr.stDispWin.u16X, stInputPortAttr.stDispWin.u16Y, stInputPortAttr.stDispWin.u16Width, stInputPortAttr.stDispWin.u16Height);
                ExecFunc(MI_DISP_EnableInputPort(DispLayer, LayerInputPort), MI_SUCCESS);
                MI_DISP_SetInputPortSyncMode (DispLayer, LayerInputPort, E_MI_DISP_SYNC_MODE_FREE_RUN);
            }

            for(i = 0; i<u32ChannelNum;i++)
                bind(i,VPE_OUTPORT,DispDev,0,i,TRUE);
        }

       for (i = 0; i < u32ChannelNum; i++)
       {
            MI_VENC_ChnAttr_t stChnAttr;
            MI_U32 u32DevId = 0;
            MI_SYS_ChnPort_t stVencChnOutputPort;
            memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
            memset(&stVencChnOutputPort, 0, sizeof(MI_SYS_ChnPort_t));

            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth =
               ALIGN_N(stTest002[i].stOutPort[VPE_Port_VENC].stPortWin.u16Width, 16);
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight =
               ALIGN_N(stTest002[i].stOutPort[VPE_Port_VENC].stPortWin.u16Height, 8);
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth =
               ALIGN_N(stTest002[i].stOutPort[VPE_Port_VENC].stPortWin.u16Width, 16);
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight =
               ALIGN_N(stTest002[i].stOutPort[VPE_Port_VENC].stPortWin.u16Height, 8);
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265FIXQP;
            stChnAttr.stRcAttr.stAttrH265FixQp.u32SrcFrmRateNum = 30;
            stChnAttr.stRcAttr.stAttrH265FixQp.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH265FixQp.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH265FixQp.u32IQp = 25;
            stChnAttr.stRcAttr.stAttrH265FixQp.u32PQp = 25;

            MI_VENC_CreateChn(i, &stChnAttr);
            MI_VENC_GetChnDevid(i, &u32DevId);
            MI_VENC_StartRecvPic(i);

            stVencChnOutputPort.u32DevId = u32DevId;
            stVencChnOutputPort.eModId = E_MI_MODULE_ID_VENC;
            stVencChnOutputPort.u32ChnId = i;
            stVencChnOutputPort.u32PortId = 0;
            //This was set to (5, 10) and might be too big for kernel
            MI_SYS_SetChnOutputPortDepth(&stVencChnOutputPort, 2, 5);

            MI_SYS_ChnPort_t stSrcChnPort;
            MI_SYS_ChnPort_t stDstChnPort;
            MI_U32 u32SrcFrmrate;
            MI_U32 u32DstFrmrate;

            stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
            stSrcChnPort.u32DevId = 0;
            stSrcChnPort.u32ChnId = i;
            stSrcChnPort.u32PortId = VPE_Port_VENC;

            stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
            stDstChnPort.u32DevId = u32DevId;
            stDstChnPort.u32ChnId = i;
            stDstChnPort.u32PortId = 0;

            u32SrcFrmrate = 30;
            u32DstFrmrate = 30;

            ExecFunc(MI_SYS_BindChnPort(&stSrcChnPort, &stDstChnPort, u32SrcFrmrate, u32DstFrmrate), MI_SUCCESS);
            char szFileName[128];

            snprintf(szFileName, sizeof(szFileName) - 1, "venc_dev%d_chn%d_port%d_%dx%d_%s.es",
            u32DevId, i, 0, stTest002[i].stOutPort[VPE_Port_VENC].stPortWin.u16Width, stTest002[i].stOutPort[VPE_Port_VENC].stPortWin.u16Height, "h265");
            stTest002[i].stOutPort[VPE_Port_VENC].dest_fd = open(szFileName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if(stTest002[i].stOutPort[VPE_Port_VENC].dest_fd <= 0)
            {
                printf("%s %d create %s error\n", __func__, __LINE__, szFileName);
                return NULL;
            }
            MI_U8 u8VencChn = i;
            pthread_create(&ptGetEs[i], NULL, getEsBuffer, (void *)&u8VencChn);
       }

        for (i = 0; i < u32ChannelNum; i++)
        {
            MI_SYS_ChnPort_t stSrcChnPort;
            MI_SYS_ChnPort_t stDstChnPort;
            MI_U32 u32SrcFrmrate;
            MI_U32 u32DstFrmrate;
            stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
            stSrcChnPort.u32DevId = 0;
            stSrcChnPort.u32ChnId = i*2;
            stSrcChnPort.u32PortId = 0;

            stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
            stDstChnPort.u32DevId = 0;
            stDstChnPort.u32ChnId = i;
            stDstChnPort.u32PortId = 0;

            u32SrcFrmrate = 30;
            u32DstFrmrate = 30;

            ExecFunc(MI_SYS_BindChnPort(&stSrcChnPort, &stDstChnPort, u32SrcFrmrate, u32DstFrmrate), MI_SUCCESS);
        }

        for (i = 0; i < u32ChannelNum; i++)
            ST_Vif_StartPort(i*2, 0);

    // Start capture data
    u32ChannelNum_tmp = u32ChannelNum;
    do {
        usleep(1000);
    }while (1);


    for (i = 0; i < u32ChannelNum_tmp; i++)
    {
        MI_SYS_ChnPort_t stSrcChnPort;
        MI_SYS_ChnPort_t stDstChnPort;
        MI_U32 u32DevId;
        src_fd = stTest002[i].src_fd;
        bind(i,VPE_OUTPORT,DispDev,0,i,FALSE);
        MI_VENC_GetChnDevid(i, &u32DevId);

        stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stSrcChnPort.u32DevId = 0;
        stSrcChnPort.u32ChnId = i;
        stSrcChnPort.u32PortId = VPE_Port_VENC;

        stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
        stDstChnPort.u32DevId = u32DevId;
        stDstChnPort.u32ChnId = i;
        stDstChnPort.u32PortId = 0;
        ExecFunc(MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort), MI_SUCCESS);

        test_vpe_CloseFd(src_fd);
        test_vpe_CloseFd(stTest002[i].stOutPort[VPE_Port_VENC].dest_fd);
        MI_VENC_StopRecvPic(i);
        MI_VENC_DestroyChn(i);
        test_vpe_DestroyChannel(i, VPE_OUTPORT);
        ExecFunc(MI_DISP_DisableInputPort(DispLayer, i), MI_SUCCESS);
    }

    ExecFunc(MI_DISP_DisableVideoLayer(DispLayer), MI_DISP_SUCCESS);
    ExecFunc(MI_DISP_UnBindVideoLayer(DispLayer, DispDev), MI_DISP_SUCCESS);
    ExecFunc(MI_DISP_Disable(DispDev), MI_DISP_SUCCESS);
    ExecFunc(test_vpe_SysEnvDeinit(), TRUE);

    MI_VPE_TEST_DBG("%s()@line %d pass exit\n", __func__, __LINE__);
    return 0;
}
