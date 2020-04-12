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

#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#include "../mi_vpe_test.h"
static test_vpe_Config stTest007 = {
    .inputFile  = TEST_VPE_CHNN_FILE(007, 0, 1920x1080),
    .stSrcWin   = {0, 0, 1920, 1080},
    .stCropWin  = {0, 0, 1920, 1080},
    .stOutPort  = {
        {
            .outputFile = TEST_VPE_PORT_OUT_FILE(007, 0, 0, 720x576),
            .bEnable    = TRUE,
            .stPortWin  = {0, 0, 720, 576},
        },
    },
};
static MI_BOOL _gbStop = FALSE;

static void * test_vpe_GetLuma (void *pData)
{
    MI_BOOL *pbNeedStop = pData;
    MI_SYS_WindowRect_t stWinRect[4];
    MI_U32   au32LumaData[4];
    MI_VPE_RegionInfo_t stRegionInfo;
    int i = 0;
    while(i++ < 6)
    {
        sleep(1);
    }

    memset(&stRegionInfo, 0, sizeof(stRegionInfo));
    stRegionInfo.u32RegionNum = sizeof(stWinRect)/sizeof(stWinRect[0]);
    for (i = 0; i < stRegionInfo.u32RegionNum; i++)
    {
        stWinRect[i].u16X = 10 + i*10;
        stWinRect[i].u16Y = 20 + i*8;

        stWinRect[i].u16Width = 32;
        stWinRect[i].u16Height= 32;
        MI_VPE_TEST_INFO("Start RegionLuma: num = %u {[%d] = {x: %u, y: %u, w: %u, h: %u} }\n",
            stRegionInfo.u32RegionNum, i, stWinRect[i].u16X, stWinRect[i].u16Y, stWinRect[i].u16Width, stWinRect[i].u16Height);
    }
    stRegionInfo.pstWinRect = stWinRect;
    MI_VPE_GetChannelRegionLuma(0, &stRegionInfo, &au32LumaData[0], 300);//, MI_VPE_OK);
    for (i = 0; i < stRegionInfo.u32RegionNum; i++)
    {
        MI_VPE_TEST_INFO("Get RegionLuma: [%d]: %u.\n", i, au32LumaData[i]);
    }

    _gbStop = TRUE;
    return 0;
}

//int test_vpe_TestCase007_main(int argc, const char *argv[])
int main(int argc, const char *argv[])
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_VPE_CHANNEL VpeChannel;
    MI_VPE_PORT VpePort;
    MI_SYS_ChnPort_t stVpeChnInputPort0;
    MI_SYS_ChnPort_t stVpeChnOutputPort0;
    char src_file[256];
    const char *pbaseDir = NULL;
    MI_SYS_FrameData_t framedata;
    int cnt = 0;
    pthread_t thread;
    pthread_attr_t attr;
    int s;
    struct timeval stTv;
    MI_DISP_DEV DispDev = 0;
    MI_DISP_LAYER DispLayer = 0;
    MI_DISP_INPUTPORT LayerInputPort = 0;

    MI_SYS_WindowRect_t stCanvas = {0, 0, 720, 576};
    MI_SYS_WindowRect_t stDispWin = {0, 0, 1920, 1080};

    s = pthread_attr_init(&attr);
    if (s != 0)
        perror("pthread_attr_init");

    memset(&framedata, 0, sizeof(framedata));
    if (argc < 2)
    {
        printf("%s <test_dir>.\n", argv[0]);
        printf("%s.\n", VPE_TEST_007_DESC);
        printf("Channel: %d.\n", 0);
        printf("InputFile: %s.\n", stTest007.inputFile);
        return 0;
    }

    printf("%s %s\n", argv[0], argv[1]);
    pbaseDir = argv[1];
    sprintf(src_file, "%s/%s", pbaseDir, stTest007.inputFile);
    ExecFunc(test_vpe_OpenSourceFile(src_file, &stTest007.src_fd), TRUE);
    stTest007.count = 0;
    stTest007.src_offset = 0;
    stTest007.src_count  = 0;
    stTest007.stOutPort[0].dest_offset = 0;
    stTest007.product = 0;

    // init MI_SYS
    ExecFunc(test_vpe_SysEnvInit(), TRUE);

    // Create VPE channel
    VpeChannel = 0;
    VpePort = 0;
    test_vpe_CreatChannel(VpeChannel, VpePort, &stTest007.stCropWin, &stTest007.stOutPort[0].stPortWin,E_MI_SYS_PIXEL_FRAME_YUV422_YUYV,E_MI_SYS_PIXEL_FRAME_YUV422_YUYV);

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

    stCanvas.u16X = stTest007.stOutPort[0].stPortWin.u16X;
    stCanvas.u16Y = stTest007.stOutPort[0].stPortWin.u16Width;
    stCanvas.u16Width = stTest007.stOutPort[0].stPortWin.u16Width;
    stCanvas.u16Height= stTest007.stOutPort[0].stPortWin.u16Height;

    stDispWin.u16X = stTest007.stOutPort[0].stPortWin.u16X;
    stDispWin.u16Y = stTest007.stOutPort[0].stPortWin.u16Y;
    stDispWin.u16Width = 1920;//stTest007.stOutPort[0].stPortWin.u16Width;
    stDispWin.u16Height= 1080;//stTest007.stOutPort[0].stPortWin.u16Height;

    test_vpe_InitDisp(DispDev, DispLayer, LayerInputPort, &stCanvas, &stDispWin,E_MI_SYS_PIXEL_FRAME_YUV422_YUYV);
    test_vpeBinderDisp(0, 0);


    pthread_create(&thread, &attr, test_vpe_GetLuma, &_gbStop);
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
        //stBufConf.u64TargetPts = time(&stTime);
        gettimeofday(&stTv, NULL);
        stBufConf.u64TargetPts = stTv.tv_sec*1000000 + stTv.tv_usec;

        stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
        stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
        stBufConf.stFrameCfg.u16Width = stTest007.stSrcWin.u16Width;
        stBufConf.stFrameCfg.u16Height = stTest007.stSrcWin.u16Height;
        if(MI_SUCCESS  == MI_SYS_ChnInputPortGetBuf(&stVpeChnInputPort0,&stBufConf,&stBufInfo,&hHandle , 0))
        {
            // Start user put int buffer
            width  = stBufInfo.stFrameData.u16Width;
            height = stBufInfo.stFrameData.u16Height;
            frame_size = width*height*YUV422_PIXEL_PER_BYTE;
            test_vpe_ShowFrameInfo("Input : ", &stBufInfo.stFrameData);
            if (1 == test_vpe_GetOneFrame(stTest007.src_fd, stTest007.src_offset, stBufInfo.stFrameData.pVirAddr[0], frame_size))
            {
                stTest007.src_offset += frame_size;

                MI_VPE_TEST_DBG("%s()@line%d: Start put input buffer.\n", __func__, __LINE__);
                s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, FALSE);
            }
            else
            {
                s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, TRUE);
                stTest007.src_offset = 0;
                stTest007.src_count = 0;
                test_vpe_FdRewind(stTest007.src_fd);
            }
        }


    }while (_gbStop == FALSE);
    pthread_join(thread, NULL);
    s = pthread_attr_destroy(&attr);
    if (s != 0)
        perror("pthread_attr_destroy");

    test_vpe_CloseFd(stTest007.src_fd);
    test_vpeUnBinderDisp(0, 0);
    test_vpe_DestroyChannel(VpeChannel, VpePort);

    test_vpe_DeinitDisp(DispDev, DispLayer, LayerInputPort);
    ExecFunc(test_vpe_SysEnvDeinit(), TRUE);

    MI_VPE_TEST_DBG("%s()@line %d pass exit\n", __func__, __LINE__);
    return 0;
}
