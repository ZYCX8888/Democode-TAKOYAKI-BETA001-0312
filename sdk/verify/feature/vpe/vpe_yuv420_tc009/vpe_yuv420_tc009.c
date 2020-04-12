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

#define MAX_TEST_CHANNEL (64)
#define VPE_OUTPORT 0

#if (VPE_OUTPORT == 3)
MI_SYS_PixelFormat_e ePixel = E_MI_SYS_PIXEL_FRAME_YUV_MST_420;
#else
MI_SYS_PixelFormat_e ePixel = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
#endif

static test_vpe_Config stTest002[MAX_TEST_CHANNEL] = {
    {
        .inputFile  = TEST_VPE_CHNN_FILE420(009, 0, 1280x720),
        .stSrcWin   = {0, 0, 1280, 720},
        .stCropWin   = {0, 0, 640, 360},
        .stOutPort  = {
      [VPE_OUTPORT] = {
                .outputFile = TEST_VPE_PORT_OUT_FILE(009, 0, 0, 128x128),
                .bEnable    = TRUE,
                .stPortWin  = {0, 0, 128, 128},
            },

        },
    },
    {
        .inputFile = TEST_VPE_CHNN_FILE420(009, 1, 640x360),
            .stSrcWin   = {0, 0, 640, 360},
            .stCropWin   = {0, 0, 640, 360},
        .stOutPort  = {
      [VPE_OUTPORT] = {
                .outputFile = TEST_VPE_PORT_OUT_FILE(009, 0, 0, 128X128),
                .bEnable    = TRUE,
                .stPortWin  = {0, 0, 128, 128},
            },

        },
    },
    {
        .inputFile = TEST_VPE_CHNN_FILE420(009, 2, 640x360),
        .stSrcWin   = {0, 0, 640, 360},
        .stCropWin  = {0, 0, 640, 360},
        .stOutPort  = {
      [VPE_OUTPORT] = {
                .outputFile = TEST_VPE_PORT_OUT_FILE(009, 0, 0, 128X128),
                .bEnable    = TRUE,
                .stPortWin  = {0, 0, 128, 128},
            },

        },
    },
    {
        .inputFile = TEST_VPE_CHNN_FILE420(009, 3, 640x360),
        .stSrcWin   = {0, 0, 640, 360},
        .stCropWin  = {0, 0, 640, 360},
        .stOutPort  = {
      [VPE_OUTPORT] = {
                .outputFile = TEST_VPE_PORT_OUT_FILE(009, 0, 0, 128X128),
                .bEnable    = TRUE,
                .stPortWin  = {0, 0, 128, 128},
            },
        },
    }
};

//int test_vpe_TestCase002_main(int argc, const char *argv[])
int main(int argc, const char *argv[])
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_U32 i = 0;
    MI_SYS_ChnPort_t stVpeChnInputPort0;
    MI_SYS_ChnPort_t stVpeChnOutputPort0;
    MI_U32 count = 0;
    struct timeval stTime;
    char src_file[256];
    char dest_file[256];
    const char *pbaseDir = NULL;
    int src_fd, dest_fd;
    MI_SYS_FrameData_t framedata;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    int frame_size = 0;
    int width  = 0;
    int height = 0;
    int y_size = 0;
    int uv_size = 0;
    memset(&framedata, 0, sizeof(framedata));
    memset(&stTime, 0, sizeof(stTime));

    if (argc < 3)
    {
        printf("%s <test_dir> <count>.\n", argv[0]);
        printf("%s.\n", VPE_TEST_009_DESC);
        for (i = 0; i < 4; i++)
        {
            printf("Channel: %d.\n", i);
            printf("InputFile: %s.\n", stTest002[i].inputFile);
            printf("OutputFile: %s.\n", stTest002[i].stOutPort[VPE_OUTPORT].outputFile);
        }
        return 0;
    }

    pbaseDir = argv[1];
    count = atoi(argv[2]);
    int j = 0;
    printf("%s %s %d.\n", argv[0], pbaseDir, count);

    // init MI_SYS
    ExecFunc(test_vpe_SysEnvInit(), TRUE);

    for (i = 0; i < sizeof(stTest002)/sizeof(stTest002[0]); i++)
    {
        stTest002[i].inputFile = stTest002[i%4].inputFile;
        stTest002[i].stSrcWin = stTest002[i%4].stSrcWin;
        stTest002[i].stCropWin = stTest002[i%4].stCropWin;
        stTest002[i].stOutPort[VPE_OUTPORT] = stTest002[i%4].stOutPort[VPE_OUTPORT];

        sprintf(src_file, "%s/%s", pbaseDir, stTest002[i].inputFile);
        ExecFunc(test_vpe_OpenSourceFile(src_file, &stTest002[i].src_fd), TRUE);
        for (j = 0; j < 4; j++)
        {
            if (stTest002[i].stOutPort[j].bEnable == TRUE)
            {
                sprintf(dest_file, "%s/port%d_%s_chn%d.yuv", pbaseDir, j, stTest002[i].stOutPort[j].outputFile,i);
                ExecFunc(test_vpe_OpenDestFile(dest_file, &stTest002[i].stOutPort[j].dest_fd), TRUE);
                stTest002[i].stOutPort[0].dest_offset = 0;

                // Create VPE channel
                test_vpe_CreatChannel_MaxSize(i, j, &stTest002[i].stSrcWin, &stTest002[i].stCropWin, &stTest002[i].stOutPort[j].stPortWin,ePixel);
                // set vpe port buffer depth
                stVpeChnOutputPort0.eModId = E_MI_MODULE_ID_VPE;
                stVpeChnOutputPort0.u32DevId = 0;
                stVpeChnOutputPort0.u32ChnId = i;
                stVpeChnOutputPort0.u32PortId = j;
                MI_SYS_SetChnOutputPortDepth(&stVpeChnOutputPort0, 1, 3);
                break;
            }
        }
        stTest002[i].count = 0;
        stTest002[i].src_offset = 0;
        stTest002[i].src_count  = 0;
        stTest002[i].product = 0;
    }

    // Start capture data
    MI_S32 s32ValidChNum = sizeof(stTest002)/sizeof(stTest002[0]);
    do {
        MI_SYS_BufConf_t stBufConf;
        MI_S32 s32Ret;
        if (s32ValidChNum <= 0)
        {
            break;
        }

        for (i = 0; i < sizeof(stTest002)/sizeof(stTest002[0]); i++)
        {
            src_fd = stTest002[i].src_fd;
            if (stTest002[i].count >= count)
            {
                s32ValidChNum--;
                continue;
            }
            for (j = 0; j < 4; j++)
            {
                if (stTest002[i].stOutPort[j].bEnable == TRUE)
                {
                    dest_fd= stTest002[i].stOutPort[j].dest_fd;

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
                    memset(&stVpeChnOutputPort0, 0, sizeof(stVpeChnOutputPort0));
                    stVpeChnOutputPort0.eModId = E_MI_MODULE_ID_VPE;
                    stVpeChnOutputPort0.u32DevId = 0;
                    stVpeChnOutputPort0.u32ChnId = i;
                    stVpeChnOutputPort0.u32PortId = j;
                    MI_VPE_TEST_DBG("%s()@line%d: Start Get Chnn: %d Port: %d output buffer.\n", __func__, __LINE__, i, stVpeChnOutputPort0.u32PortId);

                    if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChnOutputPort0 , &stBufInfo,&hHandle))
                    {
                        MI_VPE_TEST_DBG("get output buf ok\n");
                        MI_VPE_TEST_DBG("@@@--->buf type %d\n", stBufInfo.eBufType);
                        // Add user write buffer to file
                        width  = stBufInfo.stFrameData.u16Width;
                        height = stBufInfo.stFrameData.u16Height;
                        frame_size = width*height*YUV422_PIXEL_PER_BYTE;
                        // put frame
                        stTest002[i].count++;

                        test_vpe_ShowFrameInfo("Output: ", &stBufInfo.stFrameData);
        //                test_vpe_PutOneFrame(dest_fd, stTest002[i].stOutPort[0].dest_offset, stBufInfo.stFrameData.pVirAddr[0],frame_size);
                        test_vpe_PutOneFrame(dest_fd, stTest002[i].stOutPort[0].dest_offset, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u32Stride[0], width*YUV422_PIXEL_PER_BYTE, height);
                        stTest002[i].stOutPort[0].dest_offset += frame_size;

                        MI_VPE_TEST_DBG("%s()@line%d: Start put chnn: %d output buffer.\n", __func__, __LINE__, i);
                        MI_SYS_ChnOutputPortPutBuf(hHandle);
                        stTest002[i].product = 0;

                        MI_VPE_TEST_DBG("TestOut %d Pass.\n", stTest002[i].count);
                    }
                    break;
                }
            }
        }
    }while (1);

    sync();
    for (i = 0; i < sizeof(stTest002)/sizeof(stTest002[0]); i++)
    {
        src_fd = stTest002[i].src_fd;
        dest_fd= stTest002[i].stOutPort[0].dest_fd;
        test_vpe_CloseFd(src_fd);
        test_vpe_CloseFd(dest_fd);
        test_vpe_DestroyChannel(i, 0);
    }
    ExecFunc(test_vpe_SysEnvDeinit(), TRUE);

    MI_VPE_TEST_DBG("%s()@line %d pass exit\n", __func__, __LINE__);
    return 0;
}
