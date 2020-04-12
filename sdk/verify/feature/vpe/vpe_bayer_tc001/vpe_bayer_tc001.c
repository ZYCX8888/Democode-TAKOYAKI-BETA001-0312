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

MI_S32 testBayer_vpe_CreatChannelOutput(MI_VPE_CHANNEL VpeChannel, MI_VPE_PORT VpePort, MI_SYS_WindowRect_t *pstCropWin, MI_SYS_WindowRect_t *pstDispWin, MI_SYS_PixelFormat_e ePixFmt)
{
    MI_VPE_ChannelAttr_t stChannelVpssAttr;
    MI_SYS_WindowRect_t stCropWin;
    stChannelVpssAttr.u16MaxW = 3840;
    stChannelVpssAttr.u16MaxH = 2160;
    stChannelVpssAttr.bNrEn= FALSE;
    stChannelVpssAttr.bEdgeEn= FALSE;
    stChannelVpssAttr.bEsEn= FALSE;
    stChannelVpssAttr.bContrastEn= FALSE;
    stChannelVpssAttr.bUvInvert= FALSE;
    stChannelVpssAttr.ePixFmt = E_MI_SYS_PIXEL_FRAME_RGB_BAYER_10BPP_RG;
    stChannelVpssAttr.eRunningMode = E_MI_VPE_RUN_CAM_MODE;
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
    MI_VPE_PortMode_t stVpeMode;
    memset(&stVpeMode, 0, sizeof(stVpeMode));
    ExecFunc(MI_VPE_GetPortMode(VpeChannel, VpePort, &stVpeMode), MI_VPE_OK);
    stVpeMode.ePixelFormat = ePixFmt;
    stVpeMode.u16Width = pstDispWin->u16Width;
    stVpeMode.u16Height= pstDispWin->u16Height;
    ExecFunc(MI_VPE_SetPortMode(VpeChannel, VpePort, &stVpeMode), MI_VPE_OK);
    ExecFunc(MI_VPE_EnablePort(VpeChannel, VpePort), MI_VPE_OK);
    ExecFunc(MI_VPE_StartChannel (VpeChannel), MI_VPE_OK);
}


//int test_vpe_TestCase001_main(int argc, const char *argv[])
int main(int argc, const char *argv[])
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_VPE_CHANNEL VpeChannel;
    MI_VPE_PORT VpePort;
    MI_SYS_ChnPort_t stVpeChnInputPort0;
    MI_SYS_ChnPort_t stVpeChnOutputPort0;
    time_t stTime = 0;
    const char *src_file;
    int src_fd =0;
    int dest_fd =0;
    MI_U32 u32InputW =0, u32InputH =0;
    MI_SYS_WindowRect_t stCropWin;
    MI_SYS_WindowRect_t stPortWin;
    char dest_file[256];
    int cnt = 0;
    MI_SYS_FrameData_t framedata;
    int dest_offset =0;

    memset(&framedata, 0, sizeof(framedata));
    memset(&stTime, 0, sizeof(stTime));

    if (argc < 3)
    {
        printf("%s <File_dir> <width> <height> <count>.\n", argv[0]);
        printf("%s.\n", "bayer file one channel test");
        return 0;
    }

    printf("%s %s %s %s %s\n", argv[0], argv[1], argv[2], argv[3], argv[4]);
    src_file = argv[1];
    u32InputW = atoi(argv[2]);
    u32InputH = atoi(argv[3]);
    cnt = atoi(argv[4]);
    stCropWin.u16X =0;
    stCropWin.u16Y =0;
    stCropWin.u16Width = u32InputW;
    stCropWin.u16Height = u32InputH;
    stPortWin.u16Width = 1920;
    stPortWin.u16Height = 1080;
    ExecFunc(test_vpe_OpenSourceFile(src_file, &src_fd), TRUE);
    sprintf(dest_file, "%s/%s", ".", "YUV422_1920X1080.yuv");
    ExecFunc(test_vpe_OpenDestFile(dest_file, &dest_fd), TRUE);
    dest_offset = 0;

    // init MI_SYS
    ExecFunc(test_vpe_SysEnvInit(), TRUE);

    // Create VPE channel
    VpeChannel = 0;
    VpePort = 0;
    testBayer_vpe_CreatChannelOutput(VpeChannel, VpePort, &stCropWin, &stPortWin,E_MI_SYS_PIXEL_FRAME_YUV422_YUYV);

    // set vpe port buffer depth
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

    stVpeChnInputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChnInputPort0.u32DevId = 0;
    stVpeChnInputPort0.u32ChnId = 0;
    stVpeChnInputPort0.u32PortId = 0;

    do {
        MI_SYS_BufConf_t stBufConf;
        MI_S32 s32Ret;
        memset(&stBufConf , 0, sizeof(stBufConf));
        MI_VPE_TEST_DBG("%s()@line: Start get input buffer.\n", __func__, __LINE__);
        stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
        stBufConf.u64TargetPts = time(&stTime);
        stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_RGB_BAYER_10BPP_RG;
        stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
        stBufConf.stFrameCfg.u16Width = u32InputW;
        stBufConf.stFrameCfg.u16Height = u32InputH;
        if(MI_SUCCESS  == MI_SYS_ChnInputPortGetBuf(&stVpeChnInputPort0,&stBufConf,&stBufInfo,&hHandle , 0))
        {
            // Start user put int buffer
            width  = stBufInfo.stFrameData.u16Width;
            height = stBufInfo.stFrameData.u16Height;
            frame_size = width*height*2;
            test_vpe_ShowFrameInfo("Input : ", &stBufInfo.stFrameData);
            if (1 == test_vpe_GetOneFrameYUV422ByStride(src_fd, stBufInfo.stFrameData.pVirAddr[0],height, width, stBufInfo.stFrameData.u32Stride[0]))
            {

                MI_VPE_TEST_DBG("%s()@line%d: Start put input buffer.\n", __func__, __LINE__);
                MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, FALSE);
            }
            else
            {
                MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, TRUE);
                test_vpe_FdRewind(src_fd);
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
            frame_size = width*height*2;
            // put frame

            test_vpe_ShowFrameInfo("Output: ", &stBufInfo.stFrameData);
            test_vpe_PutOneFrame(dest_fd, dest_offset, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u32Stride[0], width*YUV422_PIXEL_PER_BYTE, height);
            dest_offset += frame_size;

            MI_VPE_TEST_DBG("%s()@line%d: Start put output buffer.\n", __func__, __LINE__);
            MI_SYS_ChnOutputPortPutBuf(hHandle);
            //printf("getchar before\n");
            //getchar();
            MI_VPE_TEST_INFO("Test %d Pass.\n", cnt);
            cnt--;
        }
    }while (cnt > 0);

    test_vpe_CloseFd(src_fd);
    test_vpe_CloseFd(dest_fd);
    sync();

    test_vpe_DestroyChannel(VpeChannel, VpePort);
    ExecFunc(test_vpe_SysEnvDeinit(), TRUE);

    MI_VPE_TEST_DBG("%s()@line %d pass exit\n", __func__, __LINE__);
    return 0;
}
