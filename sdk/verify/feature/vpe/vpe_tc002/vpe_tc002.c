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

#define MAX_TEST_CHANNEL (4)
static test_vpe_Config stTest002[] = {
    {
        .inputFile  = TEST_VPE_CHNN_FILE(002, 0, 1920x1080),
        .stSrcWin   = {0, 0, 1920, 1080},
        .stCropWin  = {0, 0, 1920, 1080},
        .stOutPort  = {
            {
                .outputFile = TEST_VPE_PORT_OUT_FILE(002, 0, 0, 720x576),
                .bEnable    = TRUE,
                .stPortWin  = {0, 0, 720, 576},
            },
        },
    },
    {
        .inputFile = TEST_VPE_CHNN_FILE(002, 1, 1280x720),
        .stSrcWin   = {0, 0, 1280, 720},
        .stCropWin  = {0, 0, 1280, 720},
        .stOutPort  = {
            {
                .outputFile = TEST_VPE_PORT_OUT_FILE(002, 1, 0, 640x360),
                .bEnable    = TRUE,
                .stPortWin  = {0, 0, 640, 360},
            },
        },
    },
    {
        .inputFile = TEST_VPE_CHNN_FILE(002, 2, 640x360),
        .stSrcWin   = {0, 0, 640, 360},
        .stCropWin  = {0, 0, 640, 360},
        .stOutPort  = {
            {
                .outputFile = TEST_VPE_PORT_OUT_FILE(002, 2, 0, 1280x720),
                .bEnable    = TRUE,
                .stPortWin  = {0, 0, 1280, 720},
            },
        },
    },
    {
        .inputFile = TEST_VPE_CHNN_FILE(002, 3, 960x540),
        .stSrcWin   = {0, 0, 960, 540},
        .stCropWin  = {0, 0, 960, 540},
        .stOutPort  = {
            {
                .outputFile = TEST_VPE_PORT_OUT_FILE(002, 3, 0, 1920x1080),
                .bEnable    = TRUE,
                .stPortWin  = {0, 0, 1920, 1080},
            },
        },
    },

};

//int test_vpe_TestCase002_main(int argc, const char *argv[])
int main(int argc, const char *argv[])
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_U32 i = 0;
    MI_SYS_ChnPort_t stVpeChnInputPort0;
    MI_SYS_ChnPort_t stVpeChnOutputPort0;
    MI_U32 count = 0;
    time_t stTime = 0;
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

    memset(&framedata, 0, sizeof(framedata));
    memset(&stTime, 0, sizeof(stTime));

    if (argc < 3)
    {
        printf("%s <test_dir> <count>.\n", argv[0]);
        printf("%s.\n", VPE_TEST_002_DESC);
        for (i = 0; i < sizeof(stTest002)/sizeof(stTest002[0]); i++)
        {
            printf("Channel: %d.\n", i);
            printf("InputFile: %s.\n", stTest002[i].inputFile);
            printf("OutputFile: %s.\n", stTest002[i].stOutPort[0].outputFile);
        }
        return 0;
    }

    pbaseDir = argv[1];
    count = atoi(argv[2]);
    printf("%s %s %d.\n", argv[0], pbaseDir, count);

    for (i = 0; i < sizeof(stTest002)/sizeof(stTest002[0]); i++)
    {
        sprintf(src_file, "%s/%s", pbaseDir, stTest002[i].inputFile);
        ExecFunc(test_vpe_OpenSourceFile(src_file, &stTest002[i].src_fd), TRUE);
        sprintf(dest_file, "%s/%s", pbaseDir, stTest002[i].stOutPort[0].outputFile);
        ExecFunc(test_vpe_OpenDestFile(dest_file, &stTest002[i].stOutPort[0].dest_fd), TRUE);
        stTest002[i].count = 0;
        stTest002[i].src_offset = 0;
        stTest002[i].src_count  = 0;
        stTest002[i].stOutPort[0].dest_offset = 0;
        stTest002[i].product = 0;
    }

    // init MI_SYS
    ExecFunc(test_vpe_SysEnvInit(), TRUE);

    // Create VPE channel
    for (i = 0; i < sizeof(stTest002)/sizeof(stTest002[0]); i++)
    {
        test_vpe_CreatChannel(i, 0, &stTest002[i].stCropWin, &stTest002[i].stOutPort[0].stPortWin,E_MI_SYS_PIXEL_FRAME_YUV422_YUYV,E_MI_SYS_PIXEL_FRAME_YUV422_YUYV);
        // set vpe port buffer depth
        stVpeChnOutputPort0.eModId = E_MI_MODULE_ID_VPE;
        stVpeChnOutputPort0.u32DevId = 0;
        stVpeChnOutputPort0.u32ChnId = i;
        stVpeChnOutputPort0.u32PortId = 0;
        MI_SYS_SetChnOutputPortDepth(&stVpeChnOutputPort0, 1, 3);
    }

    // Start capture data
    MI_U32 u32ValidChNum = sizeof(stTest002)/sizeof(stTest002[0]);
    do {
        MI_SYS_BufConf_t stBufConf;
        MI_S32 s32Ret;
        if (u32ValidChNum <= 0)
        {
            break;
        }

        for (i = 0; i < sizeof(stTest002)/sizeof(stTest002[0]); i++)
        {
            src_fd = stTest002[i].src_fd;
            dest_fd= stTest002[i].stOutPort[0].dest_fd;
            if (stTest002[i].count > count)
            {
                u32ValidChNum--;
                continue;
            }
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
                stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
                stBufConf.u64TargetPts = time(&stTime);
                stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
                stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
                stBufConf.stFrameCfg.u16Width = stTest002[i].stCropWin.u16Width;
                stBufConf.stFrameCfg.u16Height = stTest002[i].stCropWin.u16Height;
                if(MI_SUCCESS  == MI_SYS_ChnInputPortGetBuf(&stVpeChnInputPort0,&stBufConf,&stBufInfo,&hHandle, 0))
                {
                    // Start user put int buffer
                    width  = stBufInfo.stFrameData.u16Width;
                    height = stBufInfo.stFrameData.u16Height;
                    frame_size = width*height*YUV422_PIXEL_PER_BYTE;
                    test_vpe_ShowFrameInfo("Input : ", &stBufInfo.stFrameData);
                    if (1 == test_vpe_GetOneFrame(src_fd, stTest002[i].src_offset, stBufInfo.stFrameData.pVirAddr[0], frame_size))
                    {
                        stTest002[i].src_offset += frame_size;

                        MI_VPE_TEST_DBG("%s()@line%d: Start put input buffer.\n", __func__, __LINE__);
                        s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, FALSE);
                    }
                    else
                    {
                        s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, TRUE);
                        stTest002[i].src_offset = 0;
                        stTest002[i].src_count = 0;
                        test_vpe_FdRewind(src_fd);
                    }
                }
            }
            memset(&stVpeChnOutputPort0, 0, sizeof(stVpeChnOutputPort0));
            stVpeChnOutputPort0.eModId = E_MI_MODULE_ID_VPE;
            stVpeChnOutputPort0.u32DevId = 0;
            stVpeChnOutputPort0.u32ChnId = i;
            stVpeChnOutputPort0.u32PortId = 0;
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
