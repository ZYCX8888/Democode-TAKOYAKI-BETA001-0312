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
static test_vpe_Config stTest006 = {
    .inputFile  = TEST_VPE_CHNN_FILE(006, 0, 1920x1080),
    .stSrcWin   = {0, 0, 1920, 1080},
    .stCropWin  = {0, 0, 1920, 1080},
    .stOutPort  = {
        {
            .outputFile = TEST_VPE_PORT_OUT_FILE(006, 0, 0, 720x576),
            .bEnable    = TRUE,
            .stPortWin  = {0, 0, 720, 576},
        },
    },
};

//int test_vpe_TestCase006_main(int argc, const char *argv[])
int main(int argc, const char *argv[])
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_VPE_CHANNEL VpeChannel;
    MI_VPE_PORT VpePort;
    MI_U32 i = 0;
    MI_SYS_ChnPort_t stVpeChnInputPort0;
    MI_SYS_ChnPort_t stVpeChnOutputPort0;
    time_t stTime = 0;
    char src_file[256];
    char dest_file[256];
    int cnt = 0;
    const char *pbaseDir = NULL;
    MI_SYS_FrameData_t framedata;
    int b3dNrOn = 0;
    MI_VPE_ChannelAttr_t stChannelVpeAttr;

    memset(&framedata, 0, sizeof(framedata));
    memset(&stTime, 0, sizeof(stTime));

    if (argc < 4)
    {
        printf("%s <test_dir> <0:NR OFF, 1: NR ON> <count>.\n", argv[0]);
        printf("%s.\n", VPE_TEST_006_DESC);
        printf("Channel: %d.\n", i);
        printf("InputFile: %s.\n", stTest006.inputFile);
        printf("OutputFile: %s.\n", stTest006.stOutPort[0].outputFile);
        return 0;
    }

    printf("%s %s %s\n", argv[0], argv[1], argv[2], argv[3]);
    pbaseDir = argv[1];
    b3dNrOn = atoi(argv[2]);
    cnt     = atoi(argv[3]);
    sprintf(src_file, "%s/%s", pbaseDir, stTest006.inputFile);
    ExecFunc(test_vpe_OpenSourceFile(src_file, &stTest006.src_fd), TRUE);
    if (b3dNrOn == 0)
    {
        stTest006.stOutPort[0].outputFile = TEST_VPE_PORT_OUT_FILE(006, 0, 0, 720x576_3dNrOff);
    }
    else
    {
        stTest006.stOutPort[0].outputFile = TEST_VPE_PORT_OUT_FILE(006, 0, 0, 720x576_3dNrOn);
    }
    sprintf(dest_file, "%s/%s", pbaseDir, stTest006.stOutPort[0].outputFile);
    ExecFunc(test_vpe_OpenDestFile(dest_file, &stTest006.stOutPort[0].dest_fd), TRUE);
    stTest006.count = 0;
    stTest006.src_offset = 0;
    stTest006.src_count  = 0;
    stTest006.stOutPort[0].dest_offset = 0;
    stTest006.product = 0;

    // init MI_SYS
    ExecFunc(test_vpe_SysEnvInit(), TRUE);

    // Create VPE channel
    VpeChannel = 0;
    VpePort = 0;
    test_vpe_CreatChannel(VpeChannel, VpePort, &stTest006.stCropWin, &stTest006.stOutPort[0].stPortWin,E_MI_SYS_PIXEL_FRAME_YUV422_YUYV,E_MI_SYS_PIXEL_FRAME_YUV422_YUYV);

    memset(&stChannelVpeAttr, 0, sizeof(stChannelVpeAttr));
    ExecFunc(MI_VPE_GetChannelAttr(VpeChannel, &stChannelVpeAttr), MI_VPE_OK);
    if (b3dNrOn == 0)
    {
        stChannelVpeAttr.bNrEn = FALSE;
    }
    else
    {
        stChannelVpeAttr.bNrEn = TRUE;
    }
    ExecFunc(MI_VPE_SetChannelAttr(VpeChannel, &stChannelVpeAttr), MI_VPE_OK);
    MI_VPE_TEST_INFO("3D NR %s.\n", (b3dNrOn == 0) ? "ON": "OFF");

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
    int width  = 0;
    int height = 0;

    do {
        MI_SYS_BufConf_t stBufConf;
        MI_S32 s32Ret;
        memset(&stBufConf , 0, sizeof(stBufConf));
        MI_VPE_TEST_DBG("%s()@line: Start get input buffer.\n", __func__, __LINE__);
        stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
        stBufConf.u64TargetPts = time(&stTime);
        stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
        stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
        stBufConf.stFrameCfg.u16Width = stTest006.stSrcWin.u16Width;
        stBufConf.stFrameCfg.u16Height = stTest006.stSrcWin.u16Height;
        if(MI_SUCCESS  == MI_SYS_ChnInputPortGetBuf(&stVpeChnInputPort0,&stBufConf,&stBufInfo,&hHandle , 0))
        {
            // Start user put int buffer
            width  = stBufInfo.stFrameData.u16Width;
            height = stBufInfo.stFrameData.u16Height;
            frame_size = width*height*YUV422_PIXEL_PER_BYTE;
            test_vpe_ShowFrameInfo("Input : ", &stBufInfo.stFrameData);
            if (1 == test_vpe_GetOneFrame(stTest006.src_fd, stTest006.src_offset, stBufInfo.stFrameData.pVirAddr[0], frame_size))
            {
                stTest006.src_offset += frame_size;

                MI_VPE_TEST_DBG("%s()@line%d: Start put input buffer.\n", __func__, __LINE__);
                s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, FALSE);
            }
            else
            {
                s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, TRUE);
                stTest006.src_offset = 0;
                stTest006.src_count = 0;
                test_vpe_FdRewind(stTest006.src_fd);
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
            frame_size = width*height*YUV422_PIXEL_PER_BYTE;
            // put frame

            test_vpe_ShowFrameInfo("Output: ", &stBufInfo.stFrameData);
            test_vpe_PutOneFrame(stTest006.stOutPort[0].dest_fd, stTest006.stOutPort[0].dest_offset, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u32Stride[0], width*YUV422_PIXEL_PER_BYTE, height);
            stTest006.stOutPort[0].dest_offset += frame_size;

            MI_VPE_TEST_DBG("%s()@line%d: Start put output buffer.\n", __func__, __LINE__);
            MI_SYS_ChnOutputPortPutBuf(hHandle);

            MI_VPE_TEST_INFO("Test %d Pass.\n", cnt);
            cnt--;
        }
    }while (cnt > 0);

    test_vpe_CloseFd(stTest006.src_fd);
    test_vpe_CloseFd(stTest006.stOutPort[0].dest_fd);
    sync();

    test_vpe_DestroyChannel(VpeChannel, VpePort);
    ExecFunc(test_vpe_SysEnvDeinit(), TRUE);

    MI_VPE_TEST_DBG("%s()@line %d pass exit\n", __func__, __LINE__);
    return 0;
}
