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

#define MAX_TEST_CHANNEL (1)
static test_vpe_Config stTest003[] = {
    {
        .inputFile  = TEST_VPE_CHNN_FILE(003, 0, 1920x1080),
        .stSrcWin   = {0, 0, 1920, 1080},
        .stCropWin  = {0, 0, 1920, 1080},
        .stOutPort  = {
            {
                .outputFile = TEST_VPE_PORT_OUT_FILE(003, 0, 0, 1280x720),
                .bEnable    = TRUE,
                .stPortWin  = {0, 0, 1280, 720},
            },
        },
    },
    {
        .inputFile  = TEST_VPE_CHNN_FILE(003, 0, 1280x720),
        .stSrcWin   = {0, 0, 1280, 720},
        .stCropWin  = {0, 0, 1280, 720},
        .stOutPort  = {
            {
                .outputFile = TEST_VPE_PORT_OUT_FILE(003, 0, 0, 1280x720),
                .bEnable    = TRUE,
                .stPortWin  = {0, 0, 1280, 720},
            },
        },
    },
    {
        .inputFile  = TEST_VPE_CHNN_FILE(003, 0, 960x540),
        .stSrcWin   = {0, 0, 960, 540},
        .stCropWin  = {0, 0, 960, 540},
        .stOutPort  = {
            {
                .outputFile = TEST_VPE_PORT_OUT_FILE(003, 0, 0, 1280x720),
                .bEnable    = TRUE,
                .stPortWin  = {0, 0, 1280, 720},
            },
        },
    },
    {
        .inputFile  = TEST_VPE_CHNN_FILE(003, 0, 640x360),
        .stSrcWin   = {0, 0, 640, 360},
        .stCropWin  = {0, 0, 640, 360},
        .stOutPort  = {
            {
                .outputFile = TEST_VPE_PORT_OUT_FILE(003, 0, 0, 1280x720),
                .bEnable    = TRUE,
                .stPortWin  = {0, 0, 1280, 720},
            },
        },
    },

};

//int test_vpe_TestCase003_main(int argc, const char *argv[])
int main(int argc, const char *argv[])
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_U32 i = 0;
    MI_SYS_ChnPort_t stVpeChnInputPort0;
    MI_SYS_ChnPort_t stVpeChnOutputPort0;
    MI_S32 count = 0;
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
    struct timeval stTv;
    MI_DISP_DEV DispDev = 0;
    MI_DISP_LAYER DispLayer = 0;
    MI_DISP_INPUTPORT LayerInputPort = 0;

    MI_SYS_WindowRect_t stCanvas = {0, 0, 720, 576};
    MI_SYS_WindowRect_t stDispWin = {0, 0, 1920, 1080};

    memset(&framedata, 0, sizeof(framedata));

    if (argc < 3)
    {
        printf("%s <test_dir> <count>.\n", argv[0]);
        printf("%s.\n", VPE_TEST_003_DESC);
        for (i = 0; i < sizeof(stTest003)/sizeof(stTest003[0]); i++)
        {
            printf("Channel: %d.\n", 0);
            printf("InputFile: %s.\n", stTest003[i].inputFile);
            //printf("OutputFile: %s.\n", stTest003[i].stOutPort[0].outputFile);
        }
        return 0;
    }

    pbaseDir = argv[1];
    count = atoi(argv[2]);
    printf("%s %s %d.\n", argv[0], pbaseDir, count);

    for (i = 0; i < sizeof(stTest003)/sizeof(stTest003[0]); i++)
    {
        sprintf(src_file, "%s/%s", pbaseDir, stTest003[i].inputFile);
        ExecFunc(test_vpe_OpenSourceFile(src_file, &stTest003[i].src_fd), TRUE);
        if (i == 0)
        {
            sprintf(dest_file, "%s/%s", pbaseDir, stTest003[i].stOutPort[0].outputFile);
            ExecFunc(test_vpe_OpenDestFile(dest_file, &stTest003[i].stOutPort[0].dest_fd), TRUE);
        }
        else
        {
            stTest003[i].stOutPort[0].dest_fd = stTest003[0].stOutPort[0].dest_fd;
        }
        stTest003[i].count = 0;
        stTest003[i].src_offset = 0;
        stTest003[i].src_count  = 0;
        stTest003[i].stOutPort[0].dest_offset = 0;
        stTest003[i].product = 0;
    }

    // init MI_SYS
    ExecFunc(test_vpe_SysEnvInit(), TRUE);

    // Create VPE channel
    for (i = 0; i < MAX_TEST_CHANNEL; i++)
    {
        test_vpe_CreatChannel(i, 0, &stTest003[i].stCropWin, &stTest003[i].stOutPort[0].stPortWin,E_MI_SYS_PIXEL_FRAME_YUV422_YUYV,E_MI_SYS_PIXEL_FRAME_YUV422_YUYV);
        // set vpe port buffer depth
        stVpeChnOutputPort0.eModId = E_MI_MODULE_ID_VPE;
        stVpeChnOutputPort0.u32DevId = 0;
        stVpeChnOutputPort0.u32ChnId = i;
        stVpeChnOutputPort0.u32PortId = 0;
        MI_SYS_SetChnOutputPortDepth(&stVpeChnOutputPort0, 0, 3);
    }

    stCanvas.u16X = stTest003[0].stOutPort[0].stPortWin.u16X;
    stCanvas.u16Y = stTest003[0].stOutPort[0].stPortWin.u16Width;
    stCanvas.u16Width = stTest003[0].stOutPort[0].stPortWin.u16Width;
    stCanvas.u16Height= stTest003[0].stOutPort[0].stPortWin.u16Height;

    stDispWin.u16X = stTest003[0].stOutPort[0].stPortWin.u16X;
    stDispWin.u16Y = stTest003[0].stOutPort[0].stPortWin.u16Y;
    stDispWin.u16Width = 1920;//stTest003[0].stOutPort[0].stPortWin.u16Width;
    stDispWin.u16Height= 1080;//stTest003[0].stOutPort[0].stPortWin.u16Height;

    test_vpe_InitDisp(DispDev, DispLayer, LayerInputPort, &stCanvas, &stDispWin,E_MI_SYS_PIXEL_FRAME_YUV422_YUYV);
    test_vpeBinderDisp(0, 0);

    // Start capture data
    while (count > 0)
    {
        MI_SYS_BufConf_t stBufConf;
        MI_S32 s32Ret;

        for (i = 0; ((i < sizeof(stTest003)/sizeof(stTest003[0])) && (count > 0)); i++)
        {
            src_fd = stTest003[i].src_fd;
            dest_fd= stTest003[i].stOutPort[0].dest_fd;

            memset(&stVpeChnInputPort0, 0, sizeof(stVpeChnInputPort0));
            stVpeChnInputPort0.eModId = E_MI_MODULE_ID_VPE;
            stVpeChnInputPort0.u32DevId = 0;
            stVpeChnInputPort0.u32ChnId = 0;
            stVpeChnInputPort0.u32PortId = 0;

            memset(&stBufConf ,  0 , sizeof(stBufConf));
            MI_VPE_TEST_DBG("%s()@line%d: Start get chnn %d input buffer.\n", __func__, __LINE__, i);
            stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
            //stBufConf.u64TargetPts = time(&stTime);
            gettimeofday(&stTv, NULL);
            stBufConf.u64TargetPts = stTv.tv_sec*1000000 + stTv.tv_usec;
            stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
            stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
            stBufConf.stFrameCfg.u16Width = stTest003[i].stCropWin.u16Width;
            stBufConf.stFrameCfg.u16Height = stTest003[i].stCropWin.u16Height;
            if(MI_SUCCESS  == MI_SYS_ChnInputPortGetBuf(&stVpeChnInputPort0,&stBufConf,&stBufInfo,&hHandle, 0))
            {
                // Start user put int buffer
                width  = stBufInfo.stFrameData.u16Width;
                height = stBufInfo.stFrameData.u16Height;
                frame_size = width*height*YUV422_PIXEL_PER_BYTE;
                test_vpe_ShowFrameInfo("Input : ", &stBufInfo.stFrameData);
                if (1 == test_vpe_GetOneFrame(src_fd, stTest003[i].src_offset, stBufInfo.stFrameData.pVirAddr[0], frame_size))
                {
                    stTest003[i].src_offset += frame_size;

                    MI_VPE_TEST_DBG("%s()@line%d: Start put input buffer.\n", __func__, __LINE__);
                    s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, FALSE);
                    count--;
                }
                else
                {
                    s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, TRUE);
                    stTest003[i].src_offset = 0;
                    stTest003[i].src_count = 0;
                    test_vpe_FdRewind(src_fd);
                }
            }
        }

        MI_VPE_TEST_INFO("Calc %d Pass.\n", count);
    }

    test_vpeUnBinderDisp(0, 0);
    for (i = 0; i < sizeof(stTest003)/sizeof(stTest003[0]); i++)
    {
        src_fd = stTest003[i].src_fd;
        test_vpe_CloseFd(src_fd);
        if (i == 0)
        {
            dest_fd= stTest003[i].stOutPort[0].dest_fd;
            test_vpe_DestroyChannel(i, 0);
        }
    }

    test_vpe_DeinitDisp(DispDev, DispLayer, LayerInputPort);
    ExecFunc(test_vpe_SysEnvDeinit(), TRUE);

    MI_VPE_TEST_DBG("%s()@line %d pass exit\n", __func__, __LINE__);
    return 0;
}
