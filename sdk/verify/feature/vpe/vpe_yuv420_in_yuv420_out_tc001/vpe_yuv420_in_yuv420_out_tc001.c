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
static test_vpe_Config stTest001 = {
    .inputFile  = TEST_VPE_CHNN_FILE420(001, 0, 720x240),
    .stSrcWin   = {0, 0, 720, 240},
    .stCropWin  = {0, 0, 720, 240},
    .stOutPort  = {
        {
            .outputFile = TEST_VPE_PORT_OUT_FILE(001, 0, 0, 352x288),
            .bEnable    = TRUE,
            .stPortWin  = {0, 0, 352, 288},
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
    time_t stTime = 0;
    char src_file[256];
    char dest_file[256];
    int cnt = 0;
    const char *pbaseDir = NULL;
    MI_SYS_FrameData_t framedata;
    int RotCnt = 0;
    MI_SYS_Rotate_e eRotate = E_MI_SYS_ROTATE_NONE;

    memset(&framedata, 0, sizeof(framedata));
    memset(&stTime, 0, sizeof(stTime));

    if (argc < 3)
    {
        printf("%s <test_dir> <count>.\n", argv[0]);
        printf("%s.\n", VPE_TEST_001_DESC);
        printf("Channel: %d.\n", 0);
        printf("InputFile: %s.\n", stTest001.inputFile);
        printf("OutputFile: %s.\n", stTest001.stOutPort[0].outputFile);
        return 0;
    }

    printf("%s %s %s\n", argv[0], argv[1], argv[2]);
    pbaseDir = argv[1];
    cnt = atoi(argv[2]);
    sprintf(src_file, "%s/%s", pbaseDir, stTest001.inputFile);
    ExecFunc(test_vpe_OpenSourceFile(src_file, &stTest001.src_fd), TRUE);
    sprintf(dest_file, "%s/%s", pbaseDir, stTest001.stOutPort[0].outputFile);
    ExecFunc(test_vpe_OpenDestFile(dest_file, &stTest001.stOutPort[0].dest_fd), TRUE);
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
    test_vpe_CreatChannel(VpeChannel, VpePort, &stTest001.stCropWin, &stTest001.stOutPort[0].stPortWin,E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420,E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420);

    MI_VPE_PortMode_t stVpeMode;
    memset(&stVpeMode, 0, sizeof(stVpeMode));
    ExecFunc(MI_VPE_GetPortMode(VpeChannel, VpePort, &stVpeMode), MI_VPE_OK);
    stVpeMode.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stVpeMode.u16Width = stTest001.stOutPort[0].stPortWin.u16Width;
    stVpeMode.u16Height= stTest001.stOutPort[0].stPortWin.u16Height;
    ExecFunc(MI_VPE_SetPortMode(VpeChannel, VpePort, &stVpeMode), MI_VPE_OK);

    // set vpe port buffer depth
    stVpeChnInputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChnInputPort0.u32DevId = 0;
    stVpeChnInputPort0.u32ChnId = 0;
    stVpeChnInputPort0.u32PortId = 0;

    stVpeChnOutputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChnOutputPort0.u32DevId = 0;
    stVpeChnOutputPort0.u32ChnId = 0;
    stVpeChnOutputPort0.u32PortId = 0;
    MI_SYS_SetChnOutputPortDepth(&stVpeChnOutputPort0, 1, 3);

    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    int frame_size = 0;
    int y_size = 0;
    int uv_size = 0;
    int width  = 0;
    int height = 0;

    do {
        MI_SYS_BufConf_t stBufConf;
        MI_S32 s32Ret;
        memset(&stBufConf , 0, sizeof(stBufConf));
        MI_VPE_TEST_DBG("%s()@line: Start get input buffer.\n", __func__, __LINE__);
        stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
        stBufConf.u64TargetPts = time(&stTime);
        stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
        stBufConf.stFrameCfg.u16Width = stTest001.stSrcWin.u16Width;
        stBufConf.stFrameCfg.u16Height = stTest001.stSrcWin.u16Height;
        if(MI_SUCCESS  == MI_SYS_ChnInputPortGetBuf(&stVpeChnInputPort0,&stBufConf,&stBufInfo,&hHandle , 0))
        {
            // Start user put int buffer
            width   = stTest001.stSrcWin.u16Width;
            height  = stTest001.stSrcWin.u16Height;
            y_size  = width*height;
            uv_size  = width*height/2;
            test_vpe_ShowFrameInfo("Input : ", &stBufInfo.stFrameData);
            //if (1 == test_vpe_GetOneFrameYUV420(stTest001.src_fd, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.pVirAddr[1], y_size, uv_size))
            if(1 == test_vpe_GetOneFrameYUV420ByStride(stTest001.src_fd, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.pVirAddr[1], y_size, uv_size, height, width, stBufInfo.stFrameData.u32Stride[0], stBufInfo.stFrameData.u32Stride[1]))
            {
                stTest001.src_offset += y_size + uv_size;

                MI_VPE_TEST_DBG("%s()@line%d: Start put input buffer.\n", __func__, __LINE__);
                s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, FALSE);
            }
            else
            {
                s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, TRUE);
                stTest001.src_offset = 0;
                stTest001.src_count = 0;
                test_vpe_FdRewind(stTest001.src_fd);
            }
        }

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
            frame_size = width*height;
            // put frame

            test_vpe_ShowFrameInfo("Output: ", &stBufInfo.stFrameData);
            test_vpe_PutOneFrame(stTest001.stOutPort[0].dest_fd, stTest001.stOutPort[0].dest_offset, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u32Stride[0], width, height);
            stTest001.stOutPort[0].dest_offset += frame_size;
            test_vpe_PutOneFrame(stTest001.stOutPort[0].dest_fd, stTest001.stOutPort[0].dest_offset, stBufInfo.stFrameData.pVirAddr[1], stBufInfo.stFrameData.u32Stride[1], width, height/2);
            stTest001.stOutPort[0].dest_offset += frame_size/2;

            MI_VPE_TEST_DBG("%s()@line%d: Start put output buffer.\n", __func__, __LINE__);
            MI_SYS_ChnOutputPortPutBuf(hHandle);

            MI_VPE_TEST_INFO("Test %d Pass.\n", cnt);
            cnt--;
        }
    }while (cnt > 0);

    test_vpe_CloseFd(stTest001.src_fd);
    test_vpe_CloseFd(stTest001.stOutPort[0].dest_fd);
    sync();

    test_vpe_DestroyChannel(VpeChannel, VpePort);
    ExecFunc(test_vpe_SysEnvDeinit(), TRUE);

    MI_VPE_TEST_DBG("%s()@line %d pass exit\n", __func__, __LINE__);
    return 0;
}
