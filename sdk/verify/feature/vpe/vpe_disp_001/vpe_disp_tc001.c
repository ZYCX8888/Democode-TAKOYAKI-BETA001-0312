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
#include "mi_disp.h"
#include "mi_hdmi.h"

static test_vpe_Config stTest001 = {
    .inputFile  = TEST_VPE_CHNN_FILE(001, 0, 1280x720),
    .stSrcWin   = {0, 0, 1280, 720},
    .stCropWin  = {0, 0, 1280, 720},
    .stOutPort  = {
        {
            .outputFile = TEST_VPE_PORT_OUT_FILE(001, 0, 0, 720x576),
            .bEnable    = TRUE,
            .stPortWin  = {0, 0, 720, 576},
        },
    },
};


//int test_vpe_TestCase001_main(int argc, const char *argv[])
int main(int argc, const char *argv[])
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_VPE_CHANNEL VpeChannel;
    MI_VPE_PORT VpePort;
    MI_SYS_ChnPort_t stVpeChnInputPort0;
    MI_SYS_ChnPort_t stVpeChnOutputPort0;
    char src_file[256];
    int cnt = 0;
    const char *pbaseDir = NULL;
    MI_SYS_FrameData_t framedata;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    int frame_size = 0;
    int width  = 0;
    int height = 0;
    MI_DISP_DEV DispDev = 0;
    MI_DISP_LAYER DispLayer = 0;
    MI_DISP_INPUTPORT LayerInputPort = 0;
    struct timeval stTv;

    MI_SYS_WindowRect_t stCanvas = {0, 0, 720, 576};
    MI_SYS_WindowRect_t stDispWin = {0, 0, 1920, 1080};

    memset(&framedata, 0, sizeof(framedata));

    if (argc < 3)
    {
        printf("%s <test_dir> <count>.\n", argv[0]);
        printf("%s.\n", VPE_TEST_001_DESC);
        printf("Channel: %d.\n", 0);
        printf("InputFile: %s.\n", stTest001.inputFile);
        //printf("OutputFile: %s.\n", stTest001.stOutPort[0].outputFile);
        return 0;
    }

    printf("%s %s %s\n", argv[0], argv[1], argv[2]);
    pbaseDir = argv[1];
    cnt = atoi(argv[2]);
    sprintf(src_file, "%s/%s", pbaseDir, stTest001.inputFile);
    ExecFunc(test_vpe_OpenSourceFile(src_file, &stTest001.src_fd), TRUE);
    stTest001.count = 0;
    stTest001.src_offset = 0;
    stTest001.src_count  = 0;
    stTest001.stOutPort[0].dest_offset = 0;
    stTest001.product = 0;

    // init MI_SYS
    ExecFunc(test_vpe_SysEnvInit(), TRUE);

    // Create VPE channel
    VpeChannel = 0;
    VpePort = 0;
    test_vpe_CreatChannel(VpeChannel, VpePort, &stTest001.stCropWin, &stTest001.stOutPort[0].stPortWin,E_MI_SYS_PIXEL_FRAME_YUV422_YUYV,E_MI_SYS_PIXEL_FRAME_YUV422_YUYV);

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

    test_vpe_InitDisp(DispDev, DispLayer, LayerInputPort, &stCanvas, &stDispWin,E_MI_SYS_PIXEL_FRAME_YUV422_YUYV);
    test_vpeBinderDisp(0, 0);

    do {
        MI_SYS_BufConf_t stBufConf;
        memset(&stBufConf , 0, sizeof(stBufConf));
        MI_VPE_TEST_DBG("%s()@line: Start get input buffer.\n", __func__, __LINE__);
        stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
        //stBufConf.u64TargetPts = time(&stTime);
        gettimeofday(&stTv, NULL);
        stBufConf.u64TargetPts = stTv.tv_sec*1000000 + stTv.tv_usec;
        stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
        stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
        stBufConf.stFrameCfg.u16Width = stTest001.stSrcWin.u16Width;
        stBufConf.stFrameCfg.u16Height = stTest001.stSrcWin.u16Height;
        if(MI_SUCCESS  == MI_SYS_ChnInputPortGetBuf(&stVpeChnInputPort0,&stBufConf,&stBufInfo,&hHandle , 0))
        {
            // Start user put int buffer
            width  = stBufInfo.stFrameData.u16Width;
            height = stBufInfo.stFrameData.u16Height;
            //frame_size = width*height*YUV422_PIXEL_PER_BYTE;
            frame_size = height * stBufInfo.stFrameData.u32Stride[0];
            test_vpe_ShowFrameInfo("Input : ", &stBufInfo.stFrameData);
            //if (1 == test_vpe_GetOneFrame(stTest001.src_fd, stTest001.src_offset, stBufInfo.stFrameData.pVirAddr[0], frame_size))
            if (1 == test_vpe_GetOneFrameYUV422ByStride(stTest001.src_fd, stBufInfo.stFrameData.pVirAddr[0], height, width, stBufInfo.stFrameData.u32Stride[0]))
            {
                stTest001.src_offset += frame_size;

                MI_VPE_TEST_DBG("%s()@line%d: Start put input buffer.\n", __func__, __LINE__);
                MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, FALSE);
                cnt--;
            }
            else
            {
                stTest001.src_offset = 0;
                stTest001.src_count = 0;
                MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, TRUE);
                test_vpe_FdRewind(stTest001.src_fd);
            }
        }
    }while (cnt > 0);

    test_vpeUnBinderDisp(0, 0);

    test_vpe_CloseFd(stTest001.src_fd);
    test_vpe_DestroyChannel(VpeChannel, VpePort);

    test_vpe_DeinitDisp(DispDev, DispLayer, LayerInputPort);
    ExecFunc(test_vpe_SysEnvDeinit(), TRUE);

    MI_VPE_TEST_DBG("%s()@line %d pass exit\n", __func__, __LINE__);
    return 0;
}
