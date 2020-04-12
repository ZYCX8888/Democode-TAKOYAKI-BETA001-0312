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
static test_vpe_Config stTest005 = {
#if 1
    .inputFile  = TEST_VPE_CHNN_FILE(005, 0, 960x540),
    .stSrcWin   = {0, 0, 960, 540},
    .stCropWin  = {0, 0, 960, 540},
#else
    .inputFile  = TEST_VPE_CHNN_FILE(005, 0, 1280x720),
    .stSrcWin   = {0, 0, 1280, 720},
    .stCropWin  = {0, 0, 1280, 720},

#endif
    .stOutPort  = {
        {
            .outputFile = TEST_VPE_PORT_OUT_FILE(005, 0, 0, 1920x1080),
            .bEnable    = TRUE,
            .stPortWin  = {0, 0, 1920, 1080},
        },
        {
            .outputFile = TEST_VPE_PORT_OUT_FILE(005, 0, 0, 1280x720),
            .bEnable    = TRUE,
            .stPortWin  = {0, 0, 1280, 720},
        },
        {
            .outputFile = TEST_VPE_PORT_OUT_FILE(005, 0, 0, 720x576),
            .bEnable    = TRUE,
            .stPortWin  = {0, 0, 720, 576},
        },
        {
            .outputFile = TEST_VPE_PORT_OUT_FILE(005, 0, 0, 640x360),
            .bEnable    = TRUE,
            .stPortWin  = {0, 0, 640, 360},
        },
    },
};

//int test_vpe_TestCase005_main(int argc, const char *argv[])
int main(int argc, const char *argv[])
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_VPE_CHANNEL VpeChannel;
    MI_VPE_PORT VpePort;
    MI_U32 i = 0;
    MI_SYS_ChnPort_t stVpeChnInputPort0;
    MI_SYS_ChnPort_t stVpeChnOutputPort0;
    char src_file[256];
    int cnt = 0;
    int source_cnt = 0;
    const char *pbaseDir = NULL;
    MI_SYS_FrameData_t framedata;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    int frame_size = 0;
    MI_U16 width  = 0;
    MI_U16 height = 0;
    MI_VPE_PortMode_t stVpeMode;
    int font;
    struct timeval stTv;
    MI_DISP_DEV DispDev = 0;
    MI_DISP_LAYER DispLayer = 0;
    MI_DISP_INPUTPORT LayerInputPort = 0;

    MI_SYS_WindowRect_t stCanvas = {0, 0, 720, 576};
    MI_SYS_WindowRect_t stDispWin = {0, 0, 1920, 1080};

    memset(&framedata, 0, sizeof(framedata));

    if (argc < 2)
    {
        printf("%s <test_dir>.\n", argv[0]);
        printf("%s.\n", VPE_TEST_005_DESC);
        printf("Channel: %d.\n", i);
        printf("InputFile: %s.\n", stTest005.inputFile);
        //printf("OutputFile: %s.\n", stTest005.stOutPort[0].outputFile);
        return 0;
    }

    printf("%s %s\n", argv[0], argv[1]);
    pbaseDir = argv[1];
    cnt = 60;
    sprintf(src_file, "%s/%s", pbaseDir, stTest005.inputFile);
    ExecFunc(test_vpe_OpenSourceFile(src_file, &stTest005.src_fd), TRUE);
    stTest005.count = 0;
    stTest005.src_offset = 0;
    stTest005.src_count  = 0;
    stTest005.product = 0;

    // init MI_SYS
    ExecFunc(test_vpe_SysEnvInit(), TRUE);

    // Create VPE channel
    VpeChannel = 0;
    VpePort = 0;
    test_vpe_CreatChannel(VpeChannel, VpePort, &stTest005.stCropWin, &stTest005.stOutPort[0].stPortWin,E_MI_SYS_PIXEL_FRAME_YUV422_YUYV,E_MI_SYS_PIXEL_FRAME_YUV422_YUYV);
    // set vpe port buffer depth
    stVpeChnInputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChnInputPort0.u32DevId = 0;
    stVpeChnInputPort0.u32ChnId = 0;
    stVpeChnInputPort0.u32PortId = 0;

    stVpeChnOutputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChnOutputPort0.u32DevId = 0;
    stVpeChnOutputPort0.u32ChnId = 0;
    stVpeChnOutputPort0.u32PortId = 0;
    MI_SYS_SetChnOutputPortDepth(&stVpeChnOutputPort0, 0, 3);

    stCanvas.u16X = stTest005.stOutPort[0].stPortWin.u16X;
    stCanvas.u16Y = stTest005.stOutPort[0].stPortWin.u16Width;
    stCanvas.u16Width = stTest005.stOutPort[0].stPortWin.u16Width;
    stCanvas.u16Height= stTest005.stOutPort[0].stPortWin.u16Height;

    stDispWin.u16X = stTest005.stOutPort[0].stPortWin.u16X;
    stDispWin.u16Y = stTest005.stOutPort[0].stPortWin.u16Y;
    stDispWin.u16Width = 1920;//stTest005.stOutPort[0].stPortWin.u16Width;
    stDispWin.u16Height= 1080;//stTest005.stOutPort[0].stPortWin.u16Height;

    test_vpe_InitDisp(DispDev, DispLayer, LayerInputPort, &stCanvas, &stDispWin,E_MI_SYS_PIXEL_FRAME_YUV422_YUYV);
    test_vpeBinderDisp(0, 0);

    do {
        MI_SYS_BufConf_t stBufConf;
        MI_S32 s32Ret;
        memset(&stBufConf , 0, sizeof(stBufConf));
        MI_VPE_TEST_DBG("%s()@line: Start get input buffer.\n", __func__, __LINE__);
        stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
        //stBufConf.u64TargetPts = time(&stTime);
        gettimeofday(&stTv, NULL);
        stBufConf.u64TargetPts = stTv.tv_sec*1000000 + stTv.tv_usec;
        stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
        stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
        stBufConf.stFrameCfg.u16Width = stTest005.stSrcWin.u16Width;
        stBufConf.stFrameCfg.u16Height = stTest005.stSrcWin.u16Height;
        if(MI_SUCCESS  == MI_SYS_ChnInputPortGetBuf(&stVpeChnInputPort0,&stBufConf,&stBufInfo,&hHandle , 0))
        {
            // Start user put int buffer
            width  = stBufInfo.stFrameData.u16Width;
            height = stBufInfo.stFrameData.u16Height;
            frame_size = width*height*YUV422_PIXEL_PER_BYTE;
            test_vpe_ShowFrameInfo("Input : ", &stBufInfo.stFrameData);
            if (1 == test_vpe_GetOneFrame(stTest005.src_fd, stTest005.src_offset, stBufInfo.stFrameData.pVirAddr[0], frame_size))
            {
                stTest005.src_offset += frame_size;
            }
            else
            {
                s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, TRUE);
                stTest005.src_offset = 0;
                stTest005.src_count = 0;
                test_vpe_FdRewind(stTest005.src_fd);
                continue;
            }

            memset(&stVpeMode, 0, sizeof(stVpeMode));
            ExecFunc(MI_VPE_GetPortMode(VpeChannel, VpePort, &stVpeMode), MI_VPE_OK);
            stVpeMode.u16Height = stTest005.stOutPort[source_cnt].stPortWin.u16Height;
            stVpeMode.u16Width  = stTest005.stOutPort[source_cnt].stPortWin.u16Width;
            ExecFunc(MI_VPE_SetPortMode(VpeChannel, VpePort, &stVpeMode), MI_VPE_OK);

            MI_VPE_TEST_INFO("dispWin[%d] = {width: %u, height: %u}.\n", source_cnt, stVpeMode.u16Width, stVpeMode.u16Height);
            if (++source_cnt >= (sizeof(stTest005.stOutPort)/sizeof(stTest005.stOutPort[0])))
            {
               source_cnt = 0;
            }


            MI_VPE_TEST_DBG("%s()@line%d: Start put input buffer.\n", __func__, __LINE__);
            s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, FALSE);
            cnt--;
        }
#if 0
        stVpeChnOutputPort0.eModId = E_MI_MODULE_ID_VPE;
        stVpeChnOutputPort0.u32DevId = 0;
        stVpeChnOutputPort0.u32ChnId = 0;
        stVpeChnOutputPort0.u32PortId = 0;
        MI_VPE_TEST_DBG("%s()@line%d: Start Get output buffer.\n", __func__, __LINE__);

        if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChnOutputPort0 , &stBufInfo,&hHandle))
        {
            MI_VPE_TEST_DBG("get output buf ok\n");
            MI_VPE_TEST_DBG("@@@--->buf type %d\n", stBufInfo.eBufType);
            // Add user write buffer to file
            width  = stBufInfo.stFrameData.u16Width;
            height = stBufInfo.stFrameData.u16Height;
            frame_size = width*height*YUV422_PIXEL_PER_BYTE;
            // put frame

            test_vpe_ShowFrameInfo("Output: ", &stBufInfo.stFrameData);

            for (i = 0; i < sizeof(stTest005.stOutPort)/sizeof(stTest005.stOutPort[0]); i++)
            {
                if ((width == stTest005.stOutPort[i].stPortWin.u16Width)
                    && (height == stTest005.stOutPort[i].stPortWin.u16Height))
                {
                    MI_VPE_TEST_INFO("Find { w: %u, h: %u } -> [%d] = {u16Width: %u, u16Height: %u}.\n", width, height, i, stTest005.stOutPort[i].stPortWin.u16Width, stTest005.stOutPort[i].stPortWin.u16Height);
                    test_vpe_PutOneFrame(stTest005.stOutPort[i].dest_fd, stTest005.stOutPort[i].dest_offset, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u32Stride[0], width*YUV422_PIXEL_PER_BYTE, height);
                    stTest005.stOutPort[i].dest_offset += frame_size;
                    break;
                }
            }

            MI_VPE_TEST_DBG("%s()@line%d: Start put output buffer.\n", __func__, __LINE__);
            MI_SYS_ChnOutputPortPutBuf(hHandle);

            MI_VPE_TEST_INFO("Test %d Pass.\n", cnt);
        }
#endif
    }while (cnt > 0);

    test_vpe_CloseFd(stTest005.src_fd);
    test_vpeUnBinderDisp(0, 0);

    test_vpe_DestroyChannel(VpeChannel, VpePort);

    test_vpe_DeinitDisp(DispDev, DispLayer, LayerInputPort);
    ExecFunc(test_vpe_SysEnvDeinit(), TRUE);

    MI_VPE_TEST_DBG("%s()@line %d pass exit\n", __func__, __LINE__);
    return 0;
}
