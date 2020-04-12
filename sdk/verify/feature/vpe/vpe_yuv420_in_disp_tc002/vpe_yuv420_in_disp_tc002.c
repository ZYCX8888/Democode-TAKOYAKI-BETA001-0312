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

#include "../mi_vpe_test.h"

#define MAX_TEST_CHANNEL (16)
#define VPE_OUTPORT 0
#if (VPE_OUTPORT == 3)
MI_SYS_PixelFormat_e ePixel = E_MI_SYS_PIXEL_FRAME_YUV_MST_420;
#else
MI_SYS_PixelFormat_e ePixel = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
#endif

static test_vpe_Config stTest002[] = {
    {
        .inputFile  = TEST_VPE_CHNN_FILE420(002, 0, 1920x1080),
        .stSrcWin   = {0, 0, 1920, 1080},
        .stCropWin   = {0, 0, 1920, 1080},
        .stOutPort  = {
            [VPE_OUTPORT] = {
                .bEnable    = TRUE,
            },
        },
    },
    {
        .inputFile = TEST_VPE_CHNN_FILE420(002, 1, 1280x720),
            .stSrcWin   = {0, 0, 1280, 720},
            .stCropWin   = {0, 0, 1280, 720},
        .stOutPort  = {
            [VPE_OUTPORT] = {
                .bEnable    = TRUE,
            },
        },
    },
    {
        .inputFile = TEST_VPE_CHNN_FILE420(002, 2, 640x360),
        .stSrcWin   = {0, 0, 640, 360},
        .stCropWin  = {0, 0, 640, 360},
        .stOutPort  = {
            [VPE_OUTPORT] = {
                .bEnable    = TRUE,
            },
        },
    },
    {
        .inputFile = TEST_VPE_CHNN_FILE420(002, 3, 640x360),
        .stSrcWin   = {0, 0, 640, 360},
        .stCropWin  = {0, 0, 640, 360},
        .stOutPort  = {
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
            [VPE_OUTPORT] = {
                .bEnable    = TRUE,
            },
        },
    },
    {
        .inputFile = TEST_VPE_CHNN_FILE420(002, 1, 1280x720),
            .stSrcWin   = {0, 0, 1280, 720},
            .stCropWin   = {0, 0, 1280, 720},
        .stOutPort  = {
            [VPE_OUTPORT] = {
                .bEnable    = TRUE,
            },
        },
    },
    {
        .inputFile = TEST_VPE_CHNN_FILE420(002, 2, 640x360),
        .stSrcWin   = {0, 0, 640, 360},
        .stCropWin  = {0, 0, 640, 360},
        .stOutPort  = {
            [VPE_OUTPORT] = {
                .bEnable    = TRUE,
            },
        },
    },
    {
        .inputFile = TEST_VPE_CHNN_FILE420(002, 3, 640x360),
        .stSrcWin   = {0, 0, 640, 360},
        .stCropWin  = {0, 0, 640, 360},
        .stOutPort  = {
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
            [VPE_OUTPORT] = {
                .bEnable    = TRUE,
            },
        },
    },
    {
        .inputFile = TEST_VPE_CHNN_FILE420(002, 1, 1280x720),
            .stSrcWin   = {0, 0, 1280, 720},
            .stCropWin   = {0, 0, 1280, 720},
        .stOutPort  = {
            [VPE_OUTPORT] = {
                .bEnable    = TRUE,
            },
        },
    },
    {
        .inputFile = TEST_VPE_CHNN_FILE420(002, 2, 640x360),
        .stSrcWin   = {0, 0, 640, 360},
        .stCropWin  = {0, 0, 640, 360},
        .stOutPort  = {
            [VPE_OUTPORT] = {
                .bEnable    = TRUE,
            },
        },
    },
    {
        .inputFile = TEST_VPE_CHNN_FILE420(002, 3, 640x360),
        .stSrcWin   = {0, 0, 640, 360},
        .stCropWin  = {0, 0, 640, 360},
        .stOutPort  = {
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
            [VPE_OUTPORT] = {
                .bEnable    = TRUE,
            },
        },
    },
    {
        .inputFile = TEST_VPE_CHNN_FILE420(002, 1, 1280x720),
            .stSrcWin   = {0, 0, 1280, 720},
            .stCropWin   = {0, 0, 1280, 720},
        .stOutPort  = {
            [VPE_OUTPORT] = {
                .bEnable    = TRUE,
            },
        },
    },
    {
        .inputFile = TEST_VPE_CHNN_FILE420(002, 2, 640x360),
        .stSrcWin   = {0, 0, 640, 360},
        .stCropWin  = {0, 0, 640, 360},
        .stOutPort  = {
            [VPE_OUTPORT] = {
                .bEnable    = TRUE,
            },
        },
    },
    {
        .inputFile = TEST_VPE_CHNN_FILE420(002, 3, 640x360),
        .stSrcWin   = {0, 0, 640, 360},
        .stCropWin  = {0, 0, 640, 360},
        .stOutPort  = {
            [VPE_OUTPORT] = {
                .bEnable    = TRUE,
            },
        },
    },
};

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


//int test_vpe_TestCase002_main(int argc, const char *argv[])
int main(int argc, const char *argv[])
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_U32 i = 0;
    int j = 0;
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

    if (argc < 6)
    {
        printf("%s <test_dir> <count> <ChannelNum> <DispDev> <DispLayer>.\n", argv[0]);
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

    pbaseDir = argv[1];
    count = atoi(argv[2]);
    u32ChannelNum = atoi(argv[3]);
    DispDev = atoi(argv[4]);
    DispLayer = atoi(argv[5]);

    printf("%s %s %d %d %d %d.\n", argv[0], pbaseDir, count,u32ChannelNum,DispDev, DispLayer);

    for (i = 0; i < u32ChannelNum; i++)
    {
        sprintf(src_file, "%s/%s", pbaseDir, stTest002[i].inputFile);
        ExecFunc(test_vpe_OpenSourceFile(src_file, &stTest002[i].src_fd), TRUE);

        stTest002[i].count = 0;
        stTest002[i].src_offset = 0;
        stTest002[i].src_count  = 0;
        stTest002[i].product = 0;
    }

    // init MI_SYS
    ExecFunc(test_vpe_SysEnvInit(), TRUE);

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

    for (i = 0; i < u32ChannelNum; i++)
    {
        for (j = 0; j < 4; j++)
        {
            if (stTest002[i].stOutPort[j].bEnable == TRUE)
            {
                stTest002[i].stOutPort[j].stPortWin.u16Width = u16VpeOutportWidth;
                stTest002[i].stOutPort[j].stPortWin.u16Height = u16VpeOutportHeight;
                test_vpe_CreatChannel(i, j, &stTest002[i].stCropWin, &stTest002[i].stOutPort[j].stPortWin,E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420,ePixel);
                // set vpe port buffer depth
                stVpeChnOutputPort0.eModId = E_MI_MODULE_ID_VPE;
                stVpeChnOutputPort0.u32DevId = 0;
                stVpeChnOutputPort0.u32ChnId = i;
                stVpeChnOutputPort0.u32PortId = j;
                MI_SYS_SetChnOutputPortDepth(&stVpeChnOutputPort0, 0, 3);
                break;
            }
        }
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

                stInputPortAttr.stDispWin.u16X = (i%u8Canvs)*u16VpeOutportWidth;
                stInputPortAttr.stDispWin.u16Y = (i/u8Canvs)*u16VpeOutportHeight;
                stInputPortAttr.stDispWin.u16Width = u16VpeOutportWidth;
                stInputPortAttr.stDispWin.u16Height =u16VpeOutportHeight;

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


    // Start capture data
    u32ChannelNum_tmp = u32ChannelNum;
    do {
        MI_SYS_BufConf_t stBufConf;
        MI_S32 s32Ret;
        if (u32ChannelNum <= 0)
        {
            break;
        }

        for (i = 0; i < u32ChannelNum; i++)
        {
            src_fd = stTest002[i].src_fd;
            if (stTest002[i].count >= count)
            {
                u32ChannelNum--;
                continue;
            }
            for (j = 0; j < 4; j++)
            {
                if (stTest002[i].stOutPort[j].bEnable == TRUE)
                {
                    if (1)//(stTest002[i].product == 0)
                    {
                        stTest002[i].product = 1;
                        memset(&stVpeChnInputPort0, 0, sizeof(stVpeChnInputPort0));
                        stVpeChnInputPort0.eModId = E_MI_MODULE_ID_VPE;
                        stVpeChnInputPort0.u32DevId = 0;
                        stVpeChnInputPort0.u32ChnId = i;
                        stVpeChnInputPort0.u32PortId = 0;

                        memset(&stBufConf ,  0 , sizeof(stBufConf));
                        MI_VPE_TEST_DBG("%s()@line%d: Start get chnn %d input buffer.\n", __func__, __LINE__, i);
                        gettimeofday(&stTime, NULL);
                        stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
                        stBufConf.u64TargetPts = stTime.tv_sec*1000000 + stTime.tv_usec;
                        stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
                        stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
                        stBufConf.stFrameCfg.u16Width = stTest002[i].stSrcWin.u16Width;
                        stBufConf.stFrameCfg.u16Height = stTest002[i].stSrcWin.u16Height;
                        if(MI_SUCCESS  == MI_SYS_ChnInputPortGetBuf(&stVpeChnInputPort0,&stBufConf,&stBufInfo,&hHandle, 0))
                        {
                            // Start user put int buffer
                            width   = stBufInfo.stFrameData.u16Width;
                            height  = stBufInfo.stFrameData.u16Height;
                            y_size  = width*height;
                            //width   = stBufInfo.stFrameData.u32Stride[1];
                            uv_size  = width*height/2;

                            test_vpe_ShowFrameInfo("Input : ", &stBufInfo.stFrameData);
                            if (1 == test_vpe_GetOneFrameYUV420(src_fd, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.pVirAddr[1], y_size, uv_size))
                            {
                                stTest002[i].src_offset += y_size + uv_size;

                                MI_VPE_TEST_DBG("%s()@line%d: Start put input buffer.\n", __func__, __LINE__);
                                s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, FALSE);
                                stTest002[i].count++;
                            }
                            else
                            {
                                s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, TRUE);
                                stTest002[i].src_offset = 0;
                                stTest002[i].src_count = 0;
                                test_vpe_FdRewind(src_fd);
                                continue;
                            }
                        }
                    }
                }
            }
        }
    }while (1);


    for (i = 0; i < u32ChannelNum_tmp; i++)
    {
        src_fd = stTest002[i].src_fd;
        bind(i,VPE_OUTPORT,DispDev,0,i,FALSE);
        test_vpe_CloseFd(src_fd);
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
